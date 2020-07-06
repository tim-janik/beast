// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __JSONIPC_JSONIPC_HH__
#define __JSONIPC_JSONIPC_HH__

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <stdarg.h>
#include <cxxabi.h> // abi::__cxa_demangle
#include <algorithm>
#include <functional>
#include <typeindex>
#include <memory>
#include <vector>
#include <unordered_map>
#include <map>
#include <set>

// Much of the API and some implementation ideas are influenced by https://github.com/pmed/v8pp/ and https://www.jsonrpc.org/.

#define JSONIPC_ISLIKELY(expr)          __builtin_expect (bool (expr), 1)
#define JSONIPC_ASSERT_RETURN(expr,...) do { if (JSONIPC_ISLIKELY (expr)) break; fprintf (stderr, "%s:%d: assertion failed: %s\n", __FILE__, __LINE__, #expr); return __VA_ARGS__; } while (0)

namespace Jsonipc {

// == Json types ==
using JsonValue = rapidjson::GenericValue<rapidjson::UTF8<char>, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> >;
using JsonAllocator = rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>;

// == C++ Utilities ==
/// Construct a std::string with printf-like syntax, ignoring locale settings.
static std::string
string_format (const char *format, ...)
{
  va_list vargs;
  va_start (vargs, format);
  static locale_t posix_c_locale = newlocale (LC_ALL_MASK, "C", NULL);
  locale_t saved_locale = uselocale (posix_c_locale);
  constexpr const size_t maxlen = 8192;
  char buffer[maxlen + 1 + 1] = { 0, };
  vsnprintf (buffer, maxlen, format, vargs);
  buffer[maxlen] = 0;
  uselocale (saved_locale);
  va_end (vargs);
  return std::string (buffer);
}

/// REQUIRES<value> - Simplified version of std::enable_if<cond,bool>::type to use SFINAE in function templates.
template<bool value> using REQUIRES = typename ::std::enable_if<value, bool>::type;

/// REQUIRESv<value> - Simplified version of std::enable_if<cond,void>::type to use SFINAE in struct templates.
template<bool value> using REQUIRESv = typename ::std::enable_if<value, void>::type;

/// Template class to identify std::shared_ptr<> classes
template<typename>   struct IsSharedPtr                     : std::false_type {};
template<typename T> struct IsSharedPtr<std::shared_ptr<T>> : std::true_type  {};

/// Test string equality at compile time.
static inline constexpr bool
constexpr_equals (const char *a, const char *b, size_t n)
{
  return n == 0 || (a[0] == b[0] && (a[0] == 0 || constexpr_equals (a + 1, b + 1, n - 1)));
}

/** Demangle a std::typeinfo.name() string into a proper C++ type name.
 * This function uses abi::__cxa_demangle() from <cxxabi.h> to demangle C++ type names,
 * which works for g++, libstdc++, clang++, libc++.
 */
static inline std::string
string_demangle_cxx (const char *mangled_identifier)
{
  int status = 0;
  char *malloced_result = abi::__cxa_demangle (mangled_identifier, NULL, NULL, &status);
  std::string result = malloced_result && !status ? malloced_result : mangled_identifier;
  if (malloced_result)
    free (malloced_result);
  return result;
}

/// Provide demangled stringified name for a type `T`.
template<class T> static inline std::string
rtti_typename()
{
  return string_demangle_cxx (typeid (T).name());
}

/// Provide demangled stringified name for the runtime type of object `o`.
template<class T> static inline std::string
rtti_typename (T &o)
{
  return string_demangle_cxx (typeid (o).name());
}

/// Has___typename__<T> - Check if @a T provides a @a __typename__() method.
template<class, class = void> struct Has___typename__ : std::false_type {};
template<typename T>          struct Has___typename__<T, std::void_t< decltype (std::declval<const T&>().__typename__()) > > : std::true_type {};

/// Provide the __typename__() of @a object, or its rtti_typename().
template<typename T, REQUIRES< Has___typename__<T>::value > = true> static inline std::string
get___typename__ (const T &o)
{
  return o.__typename__();
}
template<typename T, REQUIRES< !Has___typename__<T>::value > = true> static inline std::string
get___typename__ (const T &o)
{
  return rtti_typename (o);
}

// == Forward Decls ==
class InstanceMap;
template<typename> struct Class;

// == Scope ==
/// Keep track of temporary instances during IpcDispatcher::dispatch_message().
class Scope {
  std::vector<std::shared_ptr<void>> locals_;
  InstanceMap                       &instance_map_;
  static std::vector<Scope*>&
  stack()
  {
    static thread_local std::vector<Scope*> stack_;
    return stack_;
  }
  static Scope*
  head()
  {
    auto &stack_ = stack();
    return stack_.empty() ? nullptr : stack_.back();
  }
public:
  template<typename T> static std::shared_ptr<T>
  make_shared ()
  {
    Scope *scope = head();
    if (!scope)
      throw std::logic_error ("Jsonipc::Scope::make_shared(): invalid Scope: nullptr");
    std::shared_ptr<T> sptr;
    if (scope)
      {
        sptr = std::make_shared<T>();
        scope->locals_.push_back (sptr);
      }
    return sptr;
  }
  static InstanceMap*
  instance_map ()
  {
    Scope *scope = head();
    if (!scope)
      throw std::logic_error ("Jsonipc::Scope::instance_map(): invalid Scope: nullptr");
    return scope ? &scope->instance_map_ : nullptr;
  }
  Scope (InstanceMap &instance_map) :
    instance_map_ (instance_map)
  {
    auto &stack_ = stack();
    stack_.push_back (this);
  }
  ~Scope()
  {
    auto &stack_ = stack();
    stack_.erase (std::remove (stack_.begin(), stack_.end(), this), stack_.end());
  }
};

// == Convert ==
/// Template class providing C++ <-> JsonValue conversions for various types
template<typename T, typename Enable = void>
struct Convert;

// int types
template<typename T>
struct Convert<T, REQUIRESv< std::is_integral<T>::value > > {
  static T
  from_json (const JsonValue &value, T fallback = T())
  {
    if      (value.IsBool())    return value.GetBool();
    else if (value.IsInt())     return value.GetInt();
    else if (value.IsUint())    return value.GetUint();
    else if (value.IsInt64())   return value.GetInt64();
    else if (value.IsUint64())  return value.GetUint64();
    else if (value.IsDouble())  return value.GetDouble();
    else                        return fallback;        // !IsNumber()
  }
  static JsonValue
  to_json (T i, JsonAllocator &allocator)
  {
    return JsonValue (i);
  }
};

// bool type
template<>
struct Convert<bool> {
  static bool
  from_json (const JsonValue &value, bool fallback = bool())
  {
    return Convert<uint64_t>::from_json (value, fallback);
  }
  static JsonValue
  to_json (bool b, JsonAllocator &allocator)
  {
    return JsonValue (b);
  }
};

// floating point types
template<typename T>
struct Convert<T, REQUIRESv< std::is_floating_point<T>::value >> {
  static T
  from_json (const JsonValue &value, T fallback = T())
  {
    if      (value.IsBool())    return value.GetBool();
    else if (value.IsInt())     return value.GetInt();
    else if (value.IsUint())    return value.GetUint();
    else if (value.IsInt64())   return value.GetInt64();
    else if (value.IsUint64())  return value.GetUint64();
    else if (value.IsDouble())  return value.GetDouble();
    else                        return fallback;        // !IsNumber()
  }
  static JsonValue
  to_json (T f, JsonAllocator &allocator)
  {
    return JsonValue (f);
  }
};

// const char* type
template<>
struct Convert<const char*> {
  static const char*
  from_json (const JsonValue &value, const char *fallback = "")
  {
    return value.IsString() ? value.GetString() : fallback;
  }
  static JsonValue
  to_json (const char *str, size_t l, JsonAllocator &allocator)
  {
    return str ? JsonValue (str, l, allocator) : JsonValue();
  }
  static JsonValue
  to_json (const char *str, JsonAllocator &allocator)
  {
    return str ? JsonValue (str, strlen (str), allocator) : JsonValue();
  }
};

// std::string
template<>
struct Convert<std::string> {
  static std::string
  from_json (const JsonValue &value, const std::string &fallback = std::string())
  {
    return value.IsString() ? std::string (value.GetString(), value.GetStringLength()) : fallback;
  }
  static JsonValue
  to_json (const std::string &s, JsonAllocator &allocator)
  {
    return JsonValue (s.data(), s.size(), allocator);
  }
};

/// DerivesVector<T> - Check if @a T derives from std::vector<>.
template<class T, typename = void> struct DerivesVector : std::false_type {};
// Use void_t to prevent errors for T without vector's typedefs
template<class T> struct DerivesVector<T, std::void_t< typename T::value_type, typename T::allocator_type >> :
  std::is_base_of< std::vector<typename T::value_type, typename T::allocator_type>, T > {};

// std::vector
template<typename T>
struct Convert<T, REQUIRESv< DerivesVector<T>::value >> {
  static T
  from_json (const JsonValue &jarray)
  {
    T vec;
    if (jarray.IsArray())
      {
        vec.reserve (jarray.Size());
        for (size_t i = 0; i < jarray.Size(); i++)
          vec.emplace_back (Convert<typename T::value_type>::from_json (jarray[i]));
      }
    return vec;
  }
  static JsonValue
  to_json (const T &vec, JsonAllocator &allocator)
  {
    JsonValue jarray (rapidjson::kArrayType);
    jarray.Reserve (vec.size(), allocator);
    for (size_t i = 0; i < vec.size(); ++i)
      jarray.PushBack (Convert<typename T::value_type>::to_json (vec[i], allocator).Move(), allocator);
    return jarray;
  }
};

// const reference types
template<typename T>
struct Convert<T&> : Convert<T> {};

// reference types
template<typename T>
struct Convert<T const&> : Convert<T> {};

/// Convert JsonValue to C++ value
template<typename T> static inline auto
from_json (const JsonValue &value)
  -> decltype (Convert<T>::from_json (value))
{
  return Convert<T>::from_json (value);
}

/// Convert JsonValue to C++ value with fallback for failed conversions
template<typename T> static inline auto
from_json (const JsonValue &value, const T &fallback)
  -> decltype (Convert<T>::from_json (value, fallback))
{
  return Convert<T>::from_json (value, fallback);
}

/// Convert C++ value to JsonValue
template<typename T> static inline JsonValue
to_json (const T &value, JsonAllocator &allocator)
{
  return Convert<T>::to_json (value, allocator);
}

/// Convert C++ value to JsonValue
template<> inline JsonValue
to_json<const char*> (const char *const &value, JsonAllocator &allocator)
{
  return Convert<const char*>::to_json (value, allocator);
}

/// Convert C++ char array to JsonValue
template<size_t N> static inline auto
to_json (const char (&c)[N], JsonAllocator &allocator)
{
  return Convert<const char*>::to_json (c, N - 1, allocator);
}
template<size_t N> static inline auto
to_json (const char (&c)[N], size_t l, JsonAllocator &allocator)
{
  return Convert<const char*>::to_json (c, l, allocator);
}

// == CallbackInfo ==
struct CallbackInfo;
using Closure = std::function<std::string* (CallbackInfo&)>;

/// Context for calling C++ functions from Json
struct CallbackInfo final {
  CallbackInfo (const JsonValue &args, JsonAllocator *allocator = nullptr) :
    args_ (args), doc_ (allocator)
  {}
  const JsonValue& ntharg       (size_t index) const { static JsonValue j0; return index < args_.Size() ? args_[index] : j0; }
  size_t           n_args       () const                { return args_.Size(); }
  Closure*         find_closure (const char *methodname);
  JsonAllocator&   allocator    ()                      { return doc_.GetAllocator(); }
  void             set_result   (JsonValue &result)     { result_ = result; have_result_ = true; } // move-semantic!
  JsonValue&       get_result   ()                      { return result_; }
  bool             have_result  () const                { return have_result_; }
  rapidjson::Document& document ()                      { return doc_; }
  static constexpr const char *method_not_found  = "Method not found";  // -32601
  static constexpr const char *invalid_params    = "Invalid params";    // -32602
  static constexpr const char *internal_error    = "Internal error";    // -32603
  static constexpr const char *application_error = "Application error"; // -32500
private:
  size_t
  thisid () const
  {
    const JsonValue &value = ntharg (0);
    if (value.IsObject())
      {
        auto it = value.FindMember ("$id");
        if (it != value.MemberEnd())
          return Convert<size_t>::from_json (it->value);
      }
    return 0;
  }
  const JsonValue &args_;
  JsonValue    result_;
  bool         have_result_ = false;
  rapidjson::Document doc_;
};

// == FunctionTraits ==
/// Template class providing return type and argument type information for functions
template<typename F>                   struct FunctionTraits;
template<typename R, typename ...Args> struct FunctionTraits<R (Args...)> {
  using ReturnType = R;
  using Arguments = std::tuple<Args...>;
};

// Member function pointer
template<typename C, typename R, typename ...Args>
struct FunctionTraits<R (C::*) (Args...)> : FunctionTraits<R (C&, Args...)> {
  // HAS_THIS = true
};

// Const member function pointer
template<typename C, typename R, typename ...Args>
struct FunctionTraits<R (C::*) (Args...) const> : FunctionTraits<R (const C&, Args...)> {
  // HAS_THIS = true
};

// Reference/const callables
template<typename F>
struct FunctionTraits<F&> : FunctionTraits<F> {};
template<typename F>
struct FunctionTraits<const F> : FunctionTraits<F> {};

// Member object pointer
template<typename C, typename R>
struct FunctionTraits<R (C::*)> : FunctionTraits<R (C&)> {
  template<typename D = C> using PointerType = R (D::*);
};

// == CallTraits ==
/// Template class providing conversion helpers for JsonValue to indexed C++ function argument
template<typename F>
struct CallTraits {
  using FuncType = typename std::decay<F>::type;
  using Arguments = typename FunctionTraits<FuncType>::Arguments;
  static constexpr const bool   HAS_THIS = std::is_member_function_pointer<FuncType>::value;
  static constexpr const size_t N_ARGS   = std::tuple_size<Arguments>::value - HAS_THIS;

  // Argument type via INDEX
  template<size_t INDEX, bool> struct TupleElement { using Type = typename std::tuple_element<INDEX, Arguments>::type; };
  template<size_t INDEX>       struct TupleElement<INDEX, false> { using Type = void; }; // void iff INDEX out of range
  template<size_t INDEX>       using  ArgType     = typename TupleElement<HAS_THIS + INDEX, (INDEX < N_ARGS)>::Type;
  template<size_t INDEX>       using  ConvertType = decltype (Convert<ArgType<INDEX>>::from_json (std::declval<JsonValue>()));

  template<size_t INDEX> static ConvertType<INDEX>
  arg_from_json (const CallbackInfo &args)
  {
    return Convert<ArgType<INDEX>>::from_json (args.ntharg (HAS_THIS + INDEX));
  }
  template<typename T, size_t ...INDICES> static typename FunctionTraits<F>::ReturnType
  call_unpacked (T &obj, const F &func, const CallbackInfo &args, std::index_sequence<INDICES...>)
  {
    return (obj.*func) (arg_from_json<INDICES> (args)...);
  }
};

// == call_from_json ==
template<typename T, typename F> static inline typename FunctionTraits<F>::ReturnType
call_from_json (T &obj, const F &func, const CallbackInfo &args)
{
  using CallTraits = CallTraits<F>;
  return CallTraits::call_unpacked (obj, func, args, std::make_index_sequence<CallTraits::N_ARGS>());
}

// == InstanceMap ==
class InstanceMap {
  struct TypeidKey {
    const std::type_index tindex; void *ptr;
    bool
    operator< (const TypeidKey &other) const noexcept
    {
      return tindex < other.tindex || (tindex == other.tindex && ptr < other.ptr);
    }
  };
public:
  class Wrapper {
    virtual TypeidKey typeid_key     () = 0;
    virtual          ~Wrapper        () {}
    friend            class InstanceMap;
  public:
    virtual Closure*  lookup_closure (const char *method) = 0;
    virtual void      try_upcast     (const std::string &baseclass, void *sptrB) = 0;
  };
private:
  template<typename T>
  class InstanceWrapper : public Wrapper {
    std::shared_ptr<T> sptr_;
    virtual
    ~InstanceWrapper ()
    {
      // printf ("InstanceMap::Wrapper: %s: deleting %s wrapper: %p\n", __func__, rtti_typename<T>().c_str(), sptr_.get());
    }
  public:
    explicit  InstanceWrapper (const std::shared_ptr<T> &sptr) : sptr_ (sptr) {}
    Closure*  lookup_closure  (const char *method) override { return Class<T>::lookup_closure (method); }
    TypeidKey typeid_key      () override { return create_typeid_key (sptr_); }
    void      try_upcast      (const std::string &baseclass, void *sptrB) override
    { Class<T>::try_upcast (sptr_, baseclass, sptrB); }
    static TypeidKey
    create_typeid_key (const std::shared_ptr<T> &sptr)
    {
      return { typeid (T), sptr.get() };
    }
  };
  using WrapperMap = std::unordered_map<size_t, Wrapper*>;
  using TypeidMap = std::map<TypeidKey, size_t>;
  WrapperMap         wmap_;
  TypeidMap          typeid_map_;
  static size_t      next_counter() { static size_t counter_ = 0; return ++counter_; }
public:
  ~InstanceMap()
  {
    WrapperMap old;
    std::swap (old, wmap_);
    typeid_map_.clear();
    for (auto &pair : old)
      {
        Wrapper *wrapper = pair.second;
        delete wrapper;
      }
    JSONIPC_ASSERT_RETURN (wmap_.size() == 0); // deleters shouldn't re-add
    JSONIPC_ASSERT_RETURN (typeid_map_.size() == 0); // deleters shouldn't re-add
  }
  template<typename T> static size_t
  make_wrapper_id (const std::shared_ptr<T> &sptr)
  {
    JSONIPC_ASSERT_RETURN (sptr.get() != nullptr, 0);
    const TypeidKey tkey = InstanceWrapper<T>::create_typeid_key (sptr);
    InstanceMap *imap = Scope::instance_map();
    auto it = imap->typeid_map_.find (tkey);
    if (it != imap->typeid_map_.end())
      return it->second;
    const size_t id = next_counter();
    imap->wmap_[id] = new InstanceWrapper<T> (sptr);
    imap->typeid_map_[tkey] = id;
    return id;
    /* A note about TypeidKey:
     * Two tuples (TypeX,ptr0x123) and (TypeY,ptr0x123) holding the same pointer address can
     * occur if the RTII lookup to determine the actual Wrapper class fails, e.g. when
     * Class<MostDerived> is unregisterd. In this case, ptr0x123 can be wrapped multiple
     * times through different base classes.
     */
  }
  static bool
  forget_id (size_t thisid)
  {
    InstanceMap *imap = Scope::instance_map();
    auto &wmap_ = imap->wmap_;
    auto &typeid_map_ = imap->typeid_map_;
    const auto w = wmap_.find (thisid);
    if (w != wmap_.end())
      {
        Wrapper *wrapper = w->second;
        wmap_.erase (w);
        const auto t = typeid_map_.find (wrapper->typeid_key());
        if (t != typeid_map_.end())
          typeid_map_.erase (t);
        delete wrapper;
        return true;
      }
    return false;
  }
  static Wrapper*
  lookup_wrapper (size_t thisid)
  {
    InstanceMap *imap = Scope::instance_map();
    if (imap)
      {
        auto &wmap_ = imap->wmap_;
        auto it = wmap_.find (thisid);
        if (it != wmap_.end())
          return it->second;
      }
    return nullptr;
  }
};

inline Closure*
CallbackInfo::find_closure (const char *methodname)
{
  InstanceMap::Wrapper *wrapper = InstanceMap::lookup_wrapper (thisid());
  return wrapper ? wrapper->lookup_closure (methodname) : NULL;
}

// == ClassPrinter ==
struct ClassPrinter {
  static bool recording ()              { return recording_(); }
  static void recording (bool toggle)   { recording_ (toggle); }
  enum Tag { ENUMS = 1, CLASSES, SERIALIZABLE };
  ClassPrinter (Tag tag) :
    tag_ (tag)
  {
    auto &printers = printers_();
    printers.push_back (this);
  }
  static std::string
  to_string()
  {
    std::string all;
    for (auto &p : printers_())
      {
        p->done();
        all += p->out_;
      }
    return all;
  }
  /// Yield the Javascript identifier name by substituting ':+' with '.'
  static std::string
  normalize_typename (const std::string &string)
  {
    std::string normalized;
    auto is_identifier_char = [] (int ch) {
      return ( (ch >= 'A' && ch <= 'Z') ||
               (ch >= 'a' && ch <= 'z') ||
               (ch >= '0' && ch <= '9') ||
               ch == '_' || ch == '$' );
    };
    for (size_t i = 0; i < string.size() && string[i]; ++i)
      if (is_identifier_char (string[i]))
        normalized += string[i];
      else if (normalized.size() && normalized[normalized.size() - 1] != '.')
        normalized += '.';
    return normalized;
  }
  void
  done()
  {
    advance_state (0);                  // force close
  }
  void
  print (const std::string &classname, const std::string &what, const std::string &name, int64_t count)
  {
    if (what == "new")
      {
        JSONIPC_ASSERT_RETURN (classname_.empty() && !classname.empty());
        classname_ = classname;
        jsclass_ = canonify (classname);
        jsalias_ = name.empty() ? "" : canonify (name);
        out_ += "\nexport class " + jsclass_;
        if (jsclass_ != classname_)
          out_ += " // " + classname_;
        out_ += "\n";
        state_++;                       // start class
      }
    if (what == "inherit")
      {
        advance_state (1);              // start class
        if (jsprinter_extends)
          out_ += " /* extends " + name + " */\n";
        else
          out_ += "  extends " + canonify (name) + "\n";
        jsprinter_extends = true;
      }
    if (what == "method")
      {
        advance_state (2);              // force class open
        std::string args;
        for (int i = 0; i < count; i++)
          args += (i ? ", " : "") + string_format ("a%d", i + 1);
        out_ += string_format ("  %s (%s) { return $jsonipc.send ('%s', [this%s%s]); }\n",
                               name.c_str(), args.c_str(), name.c_str(), args.empty() ? "" : ", ", args.c_str());
      }
    if (what == "get")
      {
        advance_state (2);              // force class open
        out_ += string_format ("  async %s (v) { return await (arguments.length > 0 ? $jsonipc.send ('set/%s', [this, await v]) : $jsonipc.send ('get/%s', [this])); }\n",
                               name.c_str(), name.c_str(), name.c_str());
#if 0   // async property get
        out_ += string_format ("  get %s ()  { return $jsonipc.send ('get/%s', [this]); }\n",
                               name.c_str(), name.c_str());
#endif
      }
    if (what == "set")
      {
        advance_state (2);              // force class open
#if 0   // async property set
        out_ += string_format ("  set %s (v) { (async () => $jsonipc.send ('set/%s', [this, await v])) (); }\n",
                               name.c_str(), name.c_str());
#endif
      }
    if (what == "attribute")
      serializable_attributes.push_back (name);
    if (what == "enumvalue")
      {
        done();                         // enum values are defined *after* the class
        std::string jsname = normalize_typename (classname_ + "." + name);
        out_ += string_format ("%s.%s = \"%s\"; // %d\n", jsclass_.c_str(), name.c_str(), jsname.c_str(), count);
      }
    if (what == "done")
      advance_state (0);                // force close
  }
private:
  /// Enforce a canonical charset for a string.
  static std::string
  canonify (const std::string &string)
  {
    const char *const valids = "abcdefghijklmnopqrstuvwxyz" "0123456789" "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "_$";
    const char *const substitute = "_";
    const size_t l = string.size();
    const char *p = string.c_str();
    for (size_t i = 0; i < l; i++)
      if (!strchr (valids, p[i]))
        {
          // rewrite_string:
          std::string d = string.substr (0, i);
          bool collapse = true; // collapse substitutions
          d += substitute;
          for (++i; i < l; i++)
            if (strchr (valids, p[i]))
              {
                d += p[i];
                collapse = false;
              }
            else if (!collapse)
              {
                d += substitute;
                collapse = true;
              }
          return d;
        }
    // else, pass through
    return string;
  }
  void
  advance_state (int next)
  {
    if (state_ == next)
      return;
    if (next == 0)
      next = 999;
    if (state_ == 0 && state_ < next)   // no class yet
      {
        JSONIPC_ASSERT_RETURN (!"mixed Class<> setters");
        state_++;                       // class started
      }
    if (state_ == 1 && state_ < next)   // class started
      {
        if (tag_ == SERIALIZABLE)
          {
            out_ += "{\n";
          }
        else if (tag_ == CLASSES)
          {
            out_ += "{\n  constructor ($id) { ";
            if (jsprinter_extends)
              out_ += "super ($id); ";
            else
              out_ += "Object.defineProperty (this, '$id', { value: $id }); ";
            out_ += "if (new.target === " + jsclass_ + ") Object.freeze (this); ";
            out_ += "}\n";
          }
        else if (tag_ == ENUMS)
          out_ += "{";
        state_++;                       // class open
      }
    if (state_ == 2 && state_ < next)   // class open
      {
        if (tag_ == SERIALIZABLE)       // close serializable_
          {
            out_ += "  constructor (";
            uint n = 0;
            for (auto &prop : serializable_attributes)
              out_ += (n++ ? ", " : "") + prop;
            out_ += ") {\n";
            if (jsprinter_extends)
              out_ += "    super ();\n";
            for (auto &prop : serializable_attributes)  // attributes are defined *inside* the constructor
              out_ += string_format ("    this.%s = %s;\n", prop.c_str(), prop.c_str());
            out_ += "  }\n";
          }
        out_ += "}\n";
        if (tag_ == CLASSES ||          // support '$class' lookups
            tag_ == SERIALIZABLE)
          out_ += "$jsonipc.classes['" + classname_ + "'] = " + jsclass_ + ";\n";
        if (!jsalias_.empty())
          out_ += "export const " + jsalias_ + " = " + jsclass_ + ";\n";
        state_++;                       // class closed
      }
    if (state_ < next)
      state_ = 0;                       // close state
  }
  static std::vector<ClassPrinter*>& printers_() { static std::vector<ClassPrinter*> printers; return printers; }
  std::vector<std::string> serializable_attributes;
  std::string classname_, jsclass_, jsalias_, out_;
  int state_ = 0;
  Tag tag_ = Tag (0);
  bool jsprinter_extends = false;
  static bool recording_ (int v = -1)   { static bool r = false; if (v >= 0) r = v; return r; }
};

// == TypeInfo ==
class TypeInfo {
  ClassPrinter *printer_ = NULL;
protected:
  virtual ~TypeInfo() {}
  virtual ClassPrinter* create_printer () = 0;
  void
  print (const std::string &classname, const std::string &what, const std::string &name, int64_t count = 0)
  {
    if (!ClassPrinter::recording())
      return;
    if (!printer_)
      printer_ = create_printer();
    printer_->print (classname, what, name, count);
  }
};

// == Enum ==
template<typename T>
struct Enum : TypeInfo {
  static_assert (std::is_enum<T>::value, "");
  virtual ClassPrinter* create_printer () override { return new ClassPrinter (ClassPrinter::ENUMS); }
  using UnderlyingType = typename std::underlying_type<T>::type;
  Enum (const std::string &altname = "")
  {
    const std::string class_name = rtti_typename<T>();
    print (class_name, "new", altname);
  }
  Enum&
  set (T v, const char *valuename)
  {
    const std::string class_name = typename_of<T>();
    auto &entries_ = entries();
    Entry e { ClassPrinter::normalize_typename (class_name + "." + valuename), v };
    entries_.push_back (e);
    print (class_name, "enumvalue", valuename, UnderlyingType (v));
    return *this;
  }
  static bool
  has_names ()
  {
    return !entries().empty();
  }
  static const std::string&
  get_name (T v)
  {
    auto &entries_ = entries();
    for (const auto &e : entries_)
      if (v == e.value)
        return e.name;
    static std::string empty;
    return empty;
  }
  static T
  get_value (const std::string &name, T fallback)
  {
    auto c_isalnum = [] (char c) {
      return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
    };
    auto &entries_ = entries();
    for (const auto &e : entries_)
      if (name == e.name ||                                             // exact match, or
          (name.size() < e.name.size() &&                               // name starts at e.name word boundary
           !c_isalnum (e.name[e.name.size() - name.size() - 1]) &&      // and matches the tail of e.name
           e.name.compare (e.name.size() - name.size(), name.size(), name) == 0))
        return e.value;
    return fallback;
  }
private:
  struct Entry { const std::string name; T value; };
  static std::vector<Entry>& entries() { static std::vector<Entry> entries_; return entries_; }
  template<typename U> static std::string
  typename_of()
  {
    using Type = typename std::decay<U>::type;
    return rtti_typename<Type>();
  }
};

// enum types
template<typename T>
struct Convert<T, REQUIRESv< std::is_enum<T>::value > > {
  using UnderlyingType = typename std::underlying_type<T>::type;
  static T
  from_json (const JsonValue &value, T fallback = T())
  {
    if (value.IsString())
      {
        using EnumType = Enum<T>;
        const std::string string = Convert<std::string>::from_json (value);
        return EnumType::get_value (string, fallback);
      }
    return T (Convert<UnderlyingType>::from_json (value, UnderlyingType (fallback)));
  }
  static JsonValue
  to_json (T evalue, JsonAllocator &allocator)
  {
    using EnumType = Enum<T>;
    if (EnumType::has_names())
      {
        const std::string &name = EnumType::get_name (evalue);
        if (!name.empty())
          return Convert<std::string>::to_json (name, allocator);
      }
    return Convert<UnderlyingType>::to_json (UnderlyingType (evalue), allocator);
  }
};

// == Serializable ==
/// Jsonipc wrapper type for objects that support field-wise serialization to/from JSON.
template<typename T>
struct Serializable : TypeInfo {
  virtual ClassPrinter* create_printer () override { return new ClassPrinter (ClassPrinter::SERIALIZABLE); }
  /// Allow object handles to be streamed to/from Javascript, needs a Scope for temporaries.
  Serializable (const std::string &altname = "")
  {
    const std::string class_name = rtti_typename<T>();
    print (class_name, "new", altname);
    make_serializable<T>();
  }
  /// Add a member object pointer
  template<typename A, REQUIRES< std::is_member_object_pointer<A>::value > = true> Serializable&
  set (const char *name, A attribute)
  {
    using SetterAttributeType = typename FunctionTraits<A>::ReturnType;
    Accessors accessors;
    accessors.setter = [attribute] (T &obj, const JsonValue &value) -> void      { obj.*attribute = from_json<SetterAttributeType> (value); };
    accessors.getter = [attribute] (const T &obj, JsonAllocator &a) -> JsonValue { return to_json (obj.*attribute, a); };
    AccessorMap &amap = accessormap();
    auto it = amap.find (name);
    if (it != amap.end())
      throw std::runtime_error ("duplicate attribute registration: " + std::string (name));
    amap.insert (std::make_pair<std::string, Accessors> (name, std::move (accessors)));
    const std::string class_name = rtti_typename<T>();
    print (class_name, "attribute", name, 0);
    return *this;
  }
  static bool               is_serializable     ()                             { return serialize_from_json_() && serialize_to_json_(); }
  static JsonValue          serialize_to_json   (const T &o, JsonAllocator &a) { return serialize_to_json_() (o, a); }
  static std::shared_ptr<T> serialize_from_json (const JsonValue &value)       { return serialize_from_json_() (value); }
private:
  struct Accessors {
    std::function<void      (T&,const JsonValue&)>      setter;
    std::function<JsonValue (const T&, JsonAllocator&)> getter;
  };
  using AccessorMap = std::map<std::string, Accessors>;
  static AccessorMap& accessormap() { static AccessorMap amap; return amap; }
  template<typename U> static void
  make_serializable()
  {
    // implement serialize_from_json by calling all setters
    SerializeFromJson sfj = [] (const JsonValue &value) -> std::shared_ptr<T>  {
      std::shared_ptr<T> obj = Scope::make_shared<T>();
      if (!obj)
        return obj;
      AccessorMap &amap = accessormap();
      for (const auto &field : value.GetObject())
        {
          const std::string field_name = field.name.GetString();
          auto it = amap.find (field_name);
          if (it == amap.end())
            continue;
          Accessors &accessors = it->second;
          accessors.setter (*obj, field.value);
        }
      return obj;
    };
    serialize_from_json_() = sfj;
    // implement serialize_to_json by calling all getters
    SerializeToJson stj = [] (const T &object, JsonAllocator &allocator) -> JsonValue {
      JsonValue jobject (rapidjson::kObjectType);               // serialized result
      jobject.AddMember ("__typename__", JsonValue (get___typename__ (object).c_str(), allocator), allocator);
      AccessorMap &amap = accessormap();
      for (auto &it : amap)
        {
          const std::string field_name = it.first;
          Accessors &accessors = it.second;
          JsonValue result = accessors.getter (object, allocator);
          jobject.AddMember (JsonValue (field_name.c_str(), allocator), result, allocator);
        }
      return jobject;
    };
    serialize_to_json_() = stj;
  }
  using SerializeFromJson = std::function<std::shared_ptr<T> (const JsonValue&)>;
  using SerializeToJson = std::function<JsonValue (const T&, JsonAllocator&)>;
  static SerializeFromJson& serialize_from_json_ () { static SerializeFromJson impl; return impl; }
  static SerializeToJson&   serialize_to_json_   () { static SerializeToJson impl; return impl; }
};

// == Helper for known derived classes by RTTI typename ==
using WrapObjectFromBase = size_t (const std::string&, void*);

// This *MUST* use `extern inline` for the ODR to apply to its `static` variable
extern inline WrapObjectFromBase*
can_wrap_object_from_base (const std::string &rttiname, WrapObjectFromBase *handler = nullptr)
{
  static std::map<std::string, WrapObjectFromBase*> downcastwrappers;
  if (handler)
    {
      downcastwrappers[rttiname] = handler;
      return handler;
    }
  auto it = downcastwrappers.find (rttiname);
  return it != downcastwrappers.end() ? it->second : nullptr;
}

// == Class ==
template<typename T>
struct Class : TypeInfo {
  virtual ClassPrinter* create_printer () override { return new ClassPrinter (ClassPrinter::CLASSES); }
  Class (const std::string &altname = "")
  {
    const std::string class_name = classname();
    print (class_name, "new", altname);
  }
  // Inherit base class `B`
  template<typename B> Class&
  inherit()
  {
    add_base<B>();
    print (classname(), "inherit", rtti_typename<B>(), 0);
    return *this;
  }
  /// Add a member function pointer
  template<typename F, REQUIRES< std::is_member_function_pointer<F>::value > = true> Class&
  set (const char *name, const F &method)
  {
    add_member_function_closure (name, make_closure (method));
    print (classname(), "method", name, CallTraits<F>::N_ARGS);
    return *this;
  }
  /// Add a member object accessors
  template<typename R, typename A, typename C> Class&
  set (const char *name, R (C::*get) () const, void (C::*set) (A))
  {
    if (get)
      {
        add_member_function_closure (std::string ("get/") + name, make_closure (get));
        print (classname(), "get", name, 0);
      }
    if (set)
      {
        add_member_function_closure (std::string ("set/") + name, make_closure (set));
        print (classname(), "set", name, 0);
      }
    return *this;
  }
  /// Allow object handles without reference counting via shared_ptr
  Class&
  eternal ()
  {
    is_eternal_() = true;
    return *this;
  }
  using CopyCtor = std::function<std::shared_ptr<T> (const T&)>;
  /// Create reference counted object copies
  Class&
  copy (const CopyCtor &copyctor)
  {
    JSONIPC_ASSERT_RETURN (copyctor != nullptr, *this);
    copy_ctor_() = copyctor;
    return *this;
  }
  /// Create reference counted object copies via std::make_shared<>() (deprecated, leaky)
  Class&
  copyable()
  {
    return this->copy ([] (const T &o) { return std::make_shared<T> (o); });
  }
  static std::string
  classname ()
  {
    return typename_of<T>();
  }
  static std::shared_ptr<T>
  find_object (size_t thisid)
  {
    if (thisid)
      {
        InstanceMap::Wrapper *iw = InstanceMap::lookup_wrapper (thisid);
        if (iw)
          {
            std::shared_ptr<T> base_sptr = nullptr;
            iw->try_upcast (classname(), &base_sptr);
            if (base_sptr)
              return base_sptr;
          }
      }
    return nullptr;
  }
  static size_t
  wrap_object (const std::shared_ptr<T> &sptr)
  {
    return InstanceMap::make_wrapper_id<T> (sptr);
  }
  static std::shared_ptr<T>
  convert_from_json (const JsonValue &value)
  {
    if (value.IsObject())
      {
        auto it = value.FindMember ("$id");
        if (it != value.MemberEnd())
          {
            std::shared_ptr<T> sptr = find_object (Convert<size_t>::from_json (it->value));
            return sptr;
          }
      }
    return NULL;
  }
  static bool               is_eternal ()                { return is_eternal_(); }
  static std::shared_ptr<T> make_copy  (const T &object) { return copy_ctor_() ? copy_ctor_() (object) : NULL; }
private:
  static bool&     is_eternal_ () { static bool flag = false; return flag; }
  static CopyCtor& copy_ctor_  () { static CopyCtor func; return func; }
  template<typename U> static std::string
  typename_of()
  {
    using Type = typename std::decay<U>::type;
    return rtti_typename<Type>();
  }
  template<typename F>
  using HasVoidReturn = std::is_same<void, typename FunctionTraits<F>::ReturnType>;
  template<typename F, REQUIRES< HasVoidReturn<F>::value > = true> Closure
  make_closure (const F &method)
  {
    return [method] (const CallbackInfo &cbi) -> std::string* {
      const bool HAS_THIS = true;
      if (HAS_THIS + CallTraits<F>::N_ARGS != cbi.n_args())
        return new std::string (std::string (CallbackInfo::invalid_params) + ": wrong number of arguments");
      std::shared_ptr<T> instance = convert_from_json (cbi.ntharg (0));
      if (!instance)
        return new std::string (CallbackInfo::internal_error); // closure lookup without this?
      call_from_json (*instance, method, cbi);
      return NULL;
    };
  }
  template<typename F, REQUIRES< !HasVoidReturn<F>::value > = true> Closure
  make_closure (const F &method)
  {
    return [method] (CallbackInfo &cbi) -> std::string* {
      const bool HAS_THIS = true;
      if (HAS_THIS + CallTraits<F>::N_ARGS != cbi.n_args())
        return new std::string (std::string (CallbackInfo::invalid_params) + ": wrong number of arguments");
      std::shared_ptr<T> instance = convert_from_json (cbi.ntharg (0));
      if (!instance)
        return new std::string (CallbackInfo::internal_error); // closure lookup without this?
      JsonValue rv = to_json (call_from_json (*instance, method, cbi), cbi.allocator());
      cbi.set_result (rv);
      return NULL;
    };
  }
  void
  add_member_function_closure (const std::string &name, Closure &&closure)
  {
    MethodMap &mmap = methodmap();
    auto it = mmap.find (name);
    if (it != mmap.end())
      throw std::runtime_error ("duplicate method registration: " + name);
    mmap.insert (std::make_pair<std::string, Closure> (name.c_str(), std::move (closure)));
  }
  using MethodMap = std::map<std::string, Closure>;
  static MethodMap& methodmap() { static MethodMap methodmap_; return methodmap_; }
  struct BaseInfo {
    std::string basetypename;
    bool      (*upcast_impl)    (const std::shared_ptr<T>&, const std::string&, void*) = NULL;
    bool      (*downcast_impl)  (const std::string&, void*, std::shared_ptr<T>*) = NULL;
    Closure*  (*lookup_closure) (const char*) = NULL;
  };
  using BaseVec   = std::vector<BaseInfo>;
  template<typename B> void
  add_base ()
  {
    BaseVec &bvec = basevec();
    BaseInfo binfo { typename_of<B>(), &upcast_impl<B>, &Class<B>::template downcast_impl<T>, &Class<B>::lookup_closure, };
    for (const auto &it : bvec)
      if (it.basetypename == binfo.basetypename)
        throw std::runtime_error ("duplicate base registration: " + binfo.basetypename);
    if (bvec.empty())
      can_wrap_object_from_base (classname(), wrap_object_from_base);
    bvec.push_back (binfo);
  }
  static BaseVec&   basevec  () { static BaseVec basevec_;     return basevec_; }
  static size_t
  wrap_object_from_base (const std::string &baseclass, void *sptrB)
  {
    std::shared_ptr<T> sptr;
    downcast_impl<T> (baseclass, sptrB, &sptr);
    if (sptr)
      {
        JSONIPC_ASSERT_RETURN (rtti_typename (*sptr) == rtti_typename<T>(), 0);
        return wrap_object (sptr);
      }
    return 0;
  }
  template<typename B> static bool
  upcast_impl (const std::shared_ptr<T> &sptr, const std::string &baseclass, void *sptrB)
  {
    std::shared_ptr<B> bptr = sptr;
    return Class<B>::try_upcast (bptr, baseclass, sptrB);
  }
public:
  static Closure*
  lookup_closure (const char *methodname)
  {
    MethodMap &mmap = methodmap();
    auto it = mmap.find (methodname);
    if (it != mmap.end())
      return &it->second;
    BaseVec &bvec = basevec();
    for (const auto &base : bvec)
      {
        Closure *closure = base.lookup_closure (methodname);
        if (closure)
          return closure;
      }
    return NULL;
  }
  static bool
  try_upcast (std::shared_ptr<T> &sptr, const std::string &baseclass, void *sptrB)
  {
    if (classname() == baseclass)
      {
        std::shared_ptr<T> *baseptrp = static_cast<std::shared_ptr<T>*> (sptrB);
        *baseptrp = sptr;
        return true;
      }
    BaseVec &bvec = basevec();
    for (const auto &it : bvec)
      if (it.upcast_impl (sptr, baseclass, sptrB))
        return true;
    return false;
  }
  template<typename D> static bool
  downcast_impl (const std::string &baseclass, void *sptrB, std::shared_ptr<D> *sptrD)
  {
    if (classname() == baseclass)
      {
        std::shared_ptr<T> *bptr = static_cast<std::shared_ptr<T>*> (sptrB);
        *sptrD = std::dynamic_pointer_cast<D> (*bptr);
        return true;
      }
    BaseVec &bvec = basevec();
    for (const auto &it : bvec)
      {
        std::shared_ptr<T> sptr;
        if (it.downcast_impl (baseclass, sptrB, &sptr))
          {
            *sptrD = std::dynamic_pointer_cast<D> (sptr);
            return true;
          }
      }
    return false;
  }
};

/// Template class to identify wrappable classes
template<typename T, typename Enable = void>
struct IsWrappableClass;
template<typename T>
struct IsWrappableClass<T, REQUIRESv< std::is_class<T>::value &&
                                      !IsSharedPtr<T>::value &&
                                      !DerivesVector<T>::value >> : std::true_type {};
template<>
struct IsWrappableClass<std::string> : std::false_type {};
template<typename T>
struct IsWrappableClass<T, REQUIRESv< DerivesVector<T>::value >> : std::false_type {};

/// Convert wrapped Class shared pointer
template<typename T>
struct Convert<std::shared_ptr<T>, REQUIRESv< IsWrappableClass<T>::value >> {
  using ClassType = typename std::remove_cv<T>::type;
  static std::shared_ptr<T>
  from_json (const JsonValue &value)
  {
    return Class<ClassType>::convert_from_json (value);
  }
  static JsonValue
  to_json (const std::shared_ptr<T> &sptr, JsonAllocator &allocator)
  {
    if (sptr)
      {
        size_t thisid = 0;
        // try to call the most derived wrapper Class from the RTTI type of sptr
        std::string wraptype = rtti_typename (*sptr);
        WrapObjectFromBase *wrap_object_from_base = can_wrap_object_from_base (wraptype);
        std::string basetype = rtti_typename<T>();
        if (wrap_object_from_base)
          thisid = wrap_object_from_base (basetype, const_cast<std::shared_ptr<T>*> (&sptr));
        // fallback to wrap sptr as baseclass T
        if (!thisid)
          {
            wraptype = basetype;
            thisid = Class<ClassType>::wrap_object (sptr);
          }
        JsonValue jobject (rapidjson::kObjectType);
        jobject.AddMember ("$id", thisid, allocator);
        jobject.AddMember ("$class", JsonValue (wraptype.c_str(), allocator), allocator);
        return jobject;
      }
    return JsonValue(); // null
  }
};

/// Clear wrapped Class from lookup table
static inline void
forget_json_id (size_t id)
{
  InstanceMap *imap = Scope::instance_map();
  imap->forget_id (id);
}

/// Convert wrapped Class pointer
template<typename T>
struct Convert<T*, REQUIRESv< IsWrappableClass<T>::value >> {
  using ClassType = typename std::remove_cv<T>::type;
  static T*
  from_json (const JsonValue &value)
  {
    std::shared_ptr<T> sptr;
    if (Serializable<ClassType>::is_serializable() && value.IsObject())
      sptr = Serializable<ClassType>::serialize_from_json (value);
    else
      sptr = Convert<std::shared_ptr<T>>::from_json (value);
    return sptr.get();
  }
  static JsonValue
  to_json (const T *obj, JsonAllocator &allocator)
  {
    std::shared_ptr<T> sptr;
    if (Serializable<ClassType>::is_serializable())
      return obj ? Serializable<ClassType>::serialize_to_json (*obj, allocator) : JsonValue (rapidjson::kObjectType);
    if (obj)
      {
        if (Class<ClassType>::is_eternal())
          {
            auto nodeleter = [] (void*) {};
            sptr = std::shared_ptr<T> (const_cast<T*> (obj), nodeleter);
          }
        else
          sptr = Class<ClassType>::make_copy (*obj);
      }
    return Convert<std::shared_ptr<T>>::to_json (sptr, allocator);
  }
};

/// Convert wrapped Class
template<typename T>
struct Convert<T, REQUIRESv< IsWrappableClass<T>::value >> {
  static T&
  from_json (const JsonValue &value)
  {
    T *object = Convert<T*>::from_json (value);
    if (object)
      return *object;
    throw std::bad_cast ();
  }
  static JsonValue
  to_json (const T &object, JsonAllocator &allocator)
  {
    return Convert<T*>::to_json (&object, allocator);
  }
};

// == IpcDispatcher ==
struct IpcDispatcher {
  void
  add_method (const std::string &methodname, const Closure &closure)
  {
    extra_methods[methodname] = closure;
  }
  // Dispatch JSON message and return result. Requires a live Scope instance in the current thread.
  std::string
  dispatch_message (const std::string &message)
  {
    rapidjson::Document document;
    document.Parse (message.data(), message.size());
    size_t id = 0;
    if (document.HasParseError())
      return create_error (id, -32700, "Parse error");
    const char *methodname = NULL;
    const JsonValue *args = NULL;
    for (const auto &m : document.GetObject())
      if (m.name == "id")
        id = from_json<size_t> (m.value, 0);
      else if (m.name == "method")
        methodname = from_json<const char*> (m.value);
      else if (m.name == "params" && m.value.IsArray())
        args = &m.value;
    if (!id || !methodname || !args || !args->IsArray())
      return create_error (id, -32600, "Invalid Request");
    CallbackInfo cbi (*args);
    Closure *closure = cbi.find_closure (methodname);
    if (!closure)
      {
        const auto it = extra_methods.find (methodname);
        if (it != extra_methods.end())
          closure = &it->second;
        else if (strcmp (methodname, "$jsonipc.initialize") == 0)
          {
            static Closure initialize = [] (CallbackInfo &cbi) { return jsonipc_initialize (cbi); };
            closure = &initialize;
          }
      }
    if (!closure)
      return create_error (id, -32601, std::string (CallbackInfo::method_not_found) + ": unknown '" + methodname + "'");
    std::string *errorp = NULL;
    try {
      errorp = (*closure) (cbi);
    } catch (const std::exception &exc) {
      if (!exception_handler_)
        throw; // unhandled, rethrow
      const std::string excstr = exception_handler_ (exc);
      errorp = new std::string (CallbackInfo::application_error + std::string (": ") + excstr);
    }
    if (errorp)
      {
        const std::string error = *errorp;
        delete errorp;
        if (0 == strncmp (error.c_str(), CallbackInfo::method_not_found, strlen (CallbackInfo::method_not_found)))
          return create_error (id, -32601, error);
        if (0 == strncmp (error.c_str(), CallbackInfo::invalid_params, strlen (CallbackInfo::invalid_params)))
          return create_error (id, -32602, error);
        if (0 == strncmp (error.c_str(), CallbackInfo::internal_error, strlen (CallbackInfo::internal_error)))
          return create_error (id, -32603, error);
        if (0 == strncmp (error.c_str(), CallbackInfo::application_error, strlen (CallbackInfo::internal_error)))
          return create_error (id, -32500, error);
        return create_error (id, -32000, error);        // "Server error"
      }
    return create_reply (id, cbi.get_result(), !cbi.have_result(), cbi.document());
  }
  using ExceptionHandler = std::function<std::string (const std::exception&)>;
  /// Swap out a previously set exception handler.
  /// Setting an exception handler allows turning user code exceptions into `error -32500` replies.
  ExceptionHandler
  set_exception_handler (const ExceptionHandler &handler)
  {
    ExceptionHandler old = exception_handler_;
    exception_handler_ = handler;
    return old;
  }
private:
  std::map<std::string, Closure> extra_methods;
  ExceptionHandler exception_handler_;
  std::string
  create_reply (size_t id, JsonValue &result, bool skip_result, rapidjson::Document &d)
  {
    auto &a = d.GetAllocator();
    d.SetObject();
    d.AddMember ("id", id, a);
    d.AddMember ("result", result, a); // move-semantics!
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer (buffer);
    d.Accept (writer);
    std::string output { buffer.GetString(), buffer.GetSize() };
    return output;
  }
  std::string
  create_error (size_t id, int errorcode, const std::string &message)
  {
    rapidjson::Document d (rapidjson::kObjectType);
    auto &a = d.GetAllocator();
    d.AddMember ("id", id ? JsonValue (id) : JsonValue(), a);
    JsonValue error (rapidjson::kObjectType);
    error.AddMember ("code", errorcode, a);
    error.AddMember ("message", JsonValue (message.c_str(), a).Move(), a);
    d.AddMember ("error", error, a); // moves error to null
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer (buffer);
    d.Accept (writer);
    std::string output { buffer.GetString(), buffer.GetSize() };
    return output;
  }
  static size_t
  get_objectid (const Jsonipc::JsonValue &value)
  {
    if (value.IsObject())
      {
        auto it = value.FindMember ("$id");
        if (it != value.MemberEnd())
          return Jsonipc::from_json<size_t> (it->value);
      }
    return 0;
  }
  static std::string*
  jsonipc_initialize (CallbackInfo &cbi)
  {
    cbi.set_result (to_json (true, cbi.allocator()).Move());
    return nullptr; // no error
  }
};

} // Jsonipc

#endif // __JSONIPC_JSONIPC_HH__
