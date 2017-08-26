// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __BSE_SERIALIZE_HH__
#define __BSE_SERIALIZE_HH__

#include <bse/bseparam.hh>

namespace Bse {

class SerializeContext;

namespace Aux {
// Based on https://stackoverflow.com/a/35348334
template<typename Ret,             typename Arg, typename... Rest> Arg FirstArgumentHelper (Ret (*)    (Arg, Rest...));
template<typename Ret, typename F, typename Arg, typename... Rest> Arg FirstArgumentHelper (Ret (F::*) (Arg, Rest...));
template<typename Ret, typename F, typename Arg, typename... Rest> Arg FirstArgumentHelper (Ret (F::*) (Arg, Rest...) const);
template<typename F> decltype (FirstArgumentHelper ( &F::operator() )) FirstArgumentHelper (F);
template<typename T> using FirstArgument = decltype (FirstArgumentHelper (std::declval<T> ()));
/// Pointer wrapper that allows full type erasure (but much slower than dynamic_cast<>()).
class ErasedPtr {
  // Based on: http://twimgs.com/ddj/images/article/2011/0411/any_ptr.h/any_ptr.h
  // https://web.archive.org/web/2015/http://www.drdobbs.com/cpp/twisting-the-rtti-system-for-safe-dynami/229401004
  // https://web.archive.org/web/2015/http://www.drdobbs.com/cpp/twisting-the-rtti-system-for-safe-dynami/229401004?pgno=2
  std::shared_ptr<void>  ptr_;
  void      (ErasedPtr::*throw_)   () const;
  bool                  *armedp_ = NULL;
  template<class T> void thrower   () const                             { throw static_cast<T*> (ptr_.get()); }
  template<class T> struct DisarmableDeleter : std::default_delete<T> {
    bool armed_ = true;
    void operator() (T *deletable) { if (armed_) std::default_delete<T>::operator() (deletable); }
  };
public:
  bool                   empty     () const                             { return ptr_ == NULL; }
  bool                   holds     (void *vp) const                     { return ptr_.get() == vp; }
  /*ctor*/               ErasedPtr () : ptr_ (NULL), throw_ (NULL)      {}
  template<class T>      ErasedPtr (std::shared_ptr<T> objectp) :
    ptr_ (objectp), throw_ (&ErasedPtr::thrower<T>)
  {}
  template<class T>      ErasedPtr (T *newobject) :
    ptr_(), throw_ (&ErasedPtr::thrower<T>)
  {
    typedef DisarmableDeleter<T> DisarmableDeleterT;
    std::shared_ptr<T> tp = std::shared_ptr<T> (newobject, DisarmableDeleterT());
    DisarmableDeleterT *d = std::get_deleter<DisarmableDeleterT> (tp);
    armedp_ = &d->armed_;
    ptr_ = tp;
  }
  bool
  disarm_deleter () const
  {
    if (armedp_)
      *armedp_ = false;
    return armedp_ != NULL;
  }
  template<class U> std::shared_ptr<U>
  slow_cast () const
  {
    if (ptr_)
      {
        // throw (unknown) derived type, catch base type if possible, slow but valid C++
        try { (this->*throw_) (); }
        catch (U *basep) {
          // NOTE: for multiple inheritance, ptrdiff_t (ptr_.get()) != ptrdiff_t (basep)
          std::shared_ptr<U> targetp (ptr_,     // reuse ptr_ Deleter
                                      basep);   // aliasing constructor, get() yields basep
          return targetp;
        }
        catch (...) {}
      }
    return NULL;
  }
};

} // Aux

// == Type Traits ==
/// REQUIRES<value> - Simplified version of std::enable_if<> to use SFINAE in function templates.
template<bool value> using REQUIRES = typename ::std::enable_if<value, bool>::type;
/// IsCallable<F, class ...Args> - Check if @a F is callable as F(...).
template<class F, class ...Args> struct IsCallable {
  template<class F2, class ...Args2> static constexpr std::true_type
  test (decltype (std::declval<F2>() (std::declval<Args2>()...))*) { return {}; }   // pass callable F*
  template<class F2, class ...Args2> static constexpr std::false_type
  test (...) { return {}; }                                                         // catch all
  static constexpr bool value = decltype (test<F, Args...> (nullptr))::value;
};
/// HasBeginEnd<T> - Check if @a T provides .begin() and .end() iterator methods.
template<class, class = void> struct HasBeginEnd : std::false_type {}; // Assume false_type as a fallback.
template<class T> struct HasBeginEnd<T, void_t< decltype (std::declval<T>().begin()), decltype (std::declval<T>().end()) > > :
    std::is_same< decltype (std::declval<T>().begin()), decltype (std::declval<T>().end()) > {};
/// Template to be specialized for enums that support to and from std::string conversions.
template<class E> std::string enum_to_string   (E) = delete;
/// Template to be specialized for enums that support to and from std::string conversions.
template<class E> E           enum_from_string (const std::string&) = delete;
/// HasEnumAsString<T> - Check if enum type @a T specializes enum_to_string() and enum_from_string().
template<class, class = void> struct HasEnumAsString : std::false_type {}; // Assume false_type as a fallback.
template<class E> struct HasEnumAsString<E, void_t< decltype (enum_to_string<E> (std::declval<E>())),
                                                    decltype (enum_from_string<E> (std::string())) >
                                         > : std::true_type {};
/// IsVector<T> - Check if @a T is a std::vector<>.
template<class T, typename = void> struct IsVector : std::false_type {};
// use void_t to prevent errors for T without vector's typedefs
template<class T> struct IsVector<T, void_t< typename T::value_type, typename T::allocator_type > > :
    std::is_base_of< std::vector<typename T::value_type, typename T::allocator_type>, T > {};
/// Template to call serialize_content (T&,SerializeContext&) on a class @a T or invoke T::serialize_content(SerializeContext&).
template<class T> inline auto
serialize_content (T &t, SerializeContext &sc) -> decltype (t.serialize_content (*(SerializeContext*) NULL))
{ return t.serialize_content (sc); }
/// Has_serialize_content<T> - Check if @a T provides serialize_content() as a method or external function.
template<class, class = void> struct
Has_serialize_content : std::false_type {};
template<class T> struct
Has_serialize_content<T, void_t< decltype (serialize_content (std::declval<T&>(), *(SerializeContext*) NULL)) > > : std::true_type {};

// == SerializeContext ==
typedef std::shared_ptr<SerializeContext> SerializeContextP;
/// Serialization context to save or load C++ class graphs with named properties.
class SerializeContext : public virtual VirtualEnableSharedFromThis<SerializeContext> {
  friend class FriendAllocator<SerializeContext>; // Allows access to ctor/dtor.
  struct Private;
  Private           &p_;
  int64_t            privatemem_[(sizeof (std::shared_ptr<void*>) * 2 + 7) / 8];
  bool               is_range_;
  const String       sc_type   = "typeid";
  const String       sc_uid    = "this";
  const String       sc_ref    = "extern";
  const String       sc_object = "object";
  const String       sc_main   = "main";
  String             find_uid           (void *address, const std::type_info &type, bool isroot, bool *unsaved);
  void               refmap_add         (const String &refid, const Aux::ErasedPtr &eptr);
  Aux::ErasedPtr     refmap_fetch       (const String &refid);
  void               clear_maps         ();
protected:
  struct VPort; // Value Port
  // Storage methods, mostly type erased
  explicit           SerializeContext   (Private &priv);
  virtual           ~SerializeContext   ();
  void               set_error          (const String &error);
  void               enter_load         ();
  void               leave_load         ();
  void               enter_save         ();
  void               leave_save         ();
  SerializeContextP  find_nested_by_uid (const std::string &objectid);
  SerializeContextP  load_nested        (const String &attribute, char hint);
  SerializeContextP  save_nested        (const String &attribute, char hint);
  int64              load_integral      (const String &attribute, char hint, bool *found);
  void               save_integral      (const String &attribute, int64 val, char hint = 0);
  long double        load_float         (const String &attribute, char hint, bool *found);
  void               save_float         (const String &attribute, long double val, char hint = 0);
  std::string        load_string        (const String &attribute, char hint, bool *found);
  void               save_string        (const String &attribute, const std::string &val, char hint = 0);
  std::string        first_child_name   ();                             ///< Retrieve the name of the first child during load().
  size_t             count_children     ();                             ///< Retrieve the number of available children.
  void               rotate_children    ();                             ///< Rotate children, i.e. pop first and append it at the tail.
  String             debug_name         ();
  template<class T, REQUIRES< Has_serialize_content<T>::value > = true>
  std::string        save_object        (T &val);                       ///< Save @a val contents and return its unique Id.
  template<class T, REQUIRES< Has_serialize_content<T>::value > = true>
  std::shared_ptr<T> load_object        (const std::string &objectid,
                                         bool make_shared);             ///< Create and load any object identified through its @a objectid.
  template<class T, REQUIRES< Has_serialize_content<T>::value > = true>
  String             save               (T &val);                       ///< Save @a val contents and return its unique Id.
  template<class T, REQUIRES< Has_serialize_content<T>::value > = true>
  bool               load               (T &val);                       ///< Load @a val contents through its serialize_content() method.
  void               disown_all_pointers ();    ///< Prevent SerializeContext from deleting any pointers created during load().
  void               delete_all_pointers ();    ///< Delete all pointers created during load(), except disowned pointers and shared_ptr.
  friend class SerializeFromXML;
  friend class SerializeToXML;
public:
  VPort              operator[]         (const String &attribute);      ///< Retrieve value IO port.
  bool               is_root            () const;                       ///< Indicate the toplevel (non-nested) SerializeContext.
  bool               in_load            () const;                       ///< Indicate if @a this is currently in load().
  bool               in_save            () const;                       ///< Indicate if @a this is currently in save().
  bool               in_error           () const;                       ///< Indicate if an error occoured.
  std::string        error              () const;                       ///< Retrieve error description.
  std::string        version            () const;                       ///< Retrieve the version stored in the underlying archive.
  bool               version_after      (const std::string &old) const; ///< Check if version() is equal to or greater than @a old.
  static bool        version_after      (const std::string &newer,
                                         const std::string &older);     ///< Check if @a newer is equal to or greater than @a older.
  void               disown_pointer     (void *pointer);                ///< Prevent SerializeContext from deleting @a pointer at destruction.
  class Factory;
};

class SerializeToXML {
  SerializeContextP sc_;
  std::ostream     &os_;
  bool              flushed_ = false;
public:
  explicit SerializeToXML (std::ostream &os, const String &roottag = "", const String &version = "");
  bool     flush          ();
  virtual ~SerializeToXML ();
  /// Save @a val contents and return its unique Id.
  template<class T, REQUIRES< Has_serialize_content<T>::value > = true>
  String      save        (T &val) { return sc_->save (val); }
  bool        in_error    () const { return sc_->in_error(); }  ///< Indicate if an error occoured.
  std::string error       () const { return sc_->error(); }     ///< Retrieve error description.
};

class SerializeFromXML {
  SerializeContextP sc_;
public:
  explicit SerializeFromXML    (std::istream &is);
  virtual ~SerializeFromXML    ();
  void     disown_all_pointers ();      ///< Prevent SerializeContext from deleting any pointers created during load().
  void     delete_all_pointers ();      ///< Delete all pointers created during load(), except disowned pointers and shared_ptr.
  ///< Load @a val contents through its serialize_content() method.
  template<class T, REQUIRES< Has_serialize_content<T>::value > = true>
  bool        load        (T &val) { return sc_->load (val); }
  bool        in_error    () const { return sc_->in_error(); }  ///< Indicate if an error occoured.
  std::string error       () const { return sc_->error(); }     ///< Retrieve error description.
};

// == SerializeContext::Factory ==
class SerializeContext::Factory {
  static Factory       *factories_;
  Factory              *next_;
  static Factory*                      find_name        (const std::string &type_name);
  static std::string                   find_type        (const std::type_info &target, bool mandatory);
protected:
  void                                 refmap_add       (SerializeContext &bootsc, const String &refid, const Aux::ErasedPtr &eptr);
  explicit                             Factory          (Factory *register_factory);
  virtual                             ~Factory          ();
  virtual Factory*                     match_name       (const std::string &type_name) = 0;
  virtual std::string                  match_type       (const std::type_info &target) = 0;
  virtual Aux::ErasedPtr               load_object      (SerializeContext &bootsc, const String &refid, bool make_shared) = 0;
public:
  template<class T> static std::string name_type        (T &object, bool mandatory = true);
  static Aux::ErasedPtr                load_type        (const std::string &type_name, SerializeContext &bootsc,
                                                         const String &refid, bool make_shared);
};

void
SerializeContext::Factory::refmap_add (SerializeContext &bootsc, const String &refid, const Aux::ErasedPtr &eptr)
{
  return bootsc.refmap_add (refid, eptr);
}

template<class T> std::string
SerializeContext::Factory::name_type (T &object, bool mandatory)
{
  return find_type (typeid (object), mandatory);
}

Aux::ErasedPtr
SerializeContext::Factory::load_type (const std::string &type_name, SerializeContext &bootsc, const String &refid, bool make_shared)
{
  Factory *factory = find_name (type_name);
  if (factory)
    return factory->load_object (bootsc, refid, make_shared);
  else
    return Aux::ErasedPtr();
}

// == SerializeFactory ==
template<class T, REQUIRES< Has_serialize_content<T>::value > = true >
class SerializeFactory : SerializeContext::Factory {
  const std::type_info      &type_info_;
  const String               type_name_;
  virtual Factory*           match_name  (const std::string &type_name) override { return type_name_ == type_name ? this : NULL; }
  virtual std::string        match_type  (const std::type_info &target) override { return type_info_ == target ? type_name_ : ""; }
  virtual Aux::ErasedPtr
  load_object (SerializeContext &bootsc, const String &refid, bool make_shared) override
  {
    Aux::ErasedPtr eptr;
    assert_return (!bootsc.in_save(), eptr);
    assert_return (bootsc.in_load(), eptr);
    if (!bootsc.in_error())
      {
        T *t;
        if (make_shared)                // target is known to be stored as shared_ptr
          {
            std::shared_ptr<T> tp = std::make_shared<T>();
            t = tp.get();
            eptr = Aux::ErasedPtr (tp);
          }
        else
          {
            t = new T();
            eptr = Aux::ErasedPtr (t);
          }
        refmap_add (bootsc, refid, eptr);
        serialize_content (*t, bootsc);
      }
    return eptr;
  }
public:
  SerializeFactory (const String &type_name) : Factory (this), type_info_ (typeid (T)), type_name_ (type_name) {}
};
#define BSE_SERIALIZATION_EXPORT(TYPE)             BSE_SERIALIZATION_NAMED_EXPORT (TYPE, #TYPE)
#define BSE_SERIALIZATION_NAMED_EXPORT(TYPE, NAME) static ::Bse::SerializeFactory<TYPE> __bse_SerializeFactory__ ## TYPE (NAME)

// == SerializeContext::VPort ==
class SerializeContext::VPort {
  SerializeContext &sc_;
  String            attribute_;
  SerializeContextP scptr_;
  template<class T, REQUIRES<  HasEnumAsString<T>::value > = true> void serialize_enum (T &val);
  template<class T, REQUIRES< !HasEnumAsString<T>::value > = true> void serialize_enum (T &val);
public:
  VPort (SerializeContext &sc, const String &attribute) :
    sc_ (sc), attribute_ (attribute), scptr_ (sc_.shared_from_this())
  {}
  // accessors
  SerializeContext& serialize_context () const { return sc_; }
  String            attribute         () const { return attribute_; }
  // serializing
  template<class T, REQUIRES< std::is_enum<T>::value > = true>                  void operator>>= (T &val);
  template<class T, REQUIRES< std::is_arithmetic<T>::value > = true>            void operator>>= (T &val);
  template<class T, REQUIRES< std::is_base_of<std::string, T>::value > = true>  void operator>>= (T &val);
  template<class T, REQUIRES< IsVector<T>::value > = true>                      void operator>>= (T &val);
  template<class T, REQUIRES< Has_serialize_content<T>::value > = true>         void operator>>= (T &val);
  template<class T, REQUIRES< Has_serialize_content<T>::value > = true>         void operator>>= (T* &val);
  template<class T, REQUIRES< Has_serialize_content<T>::value > = true>         void operator>>= (std::shared_ptr<T> &val);
};

template<class T, REQUIRES< !HasEnumAsString<T>::value > > void
SerializeContext::VPort::serialize_enum (T &val)
{
  const char hint = 'E';
  if (sc_.in_load())
    {
      bool found = false;
      const int64 tmp = sc_.load_integral (attribute_, hint, &found);
      if (found)
        val = T (tmp);
    }
  if (sc_.in_save())
    sc_.save_integral (attribute_, val, hint);
}

template<class T, REQUIRES< HasEnumAsString<T>::value > > void
SerializeContext::VPort::serialize_enum (T &val)
{
  const char hint = 'E';
  if (sc_.in_load())
    {
      bool found = false;
      const std::string tmp = sc_.load_string (attribute_, hint, &found);
      if (found)
        {
          if (tmp.size() && tmp[0] >= '0' && tmp[0] <= '9')
            val = T (string_to_int (tmp));
          else
            val = enum_from_string<T> (tmp);
        }
    }
  if (sc_.in_save())
    sc_.save_string (attribute_, enum_to_string<T> (val), hint);
}

template<class T, REQUIRES< std::is_enum<T>::value > > inline void
SerializeContext::VPort::operator>>= (T &val)
{
  return serialize_enum (val);
}

template<class T, REQUIRES< std::is_arithmetic<T>::value > > inline void
SerializeContext::VPort::operator>>= (T &val)
{
  const char hint = std::is_same<bool, T>::value    ? 'B' :
                    std::is_unsigned<T>::value      ? 'U' :
                    std::is_same<char, T>::value    ? 'c' :
                    std::is_same<wchar_t, T>::value ? 'c' :
                    std::is_same<float,  T>::value  ? 'f' :
                    std::is_same<double, T>::value  ? 'd' :
                    '\0';
  if (std::is_floating_point<T>::value)
    {
      if (sc_.in_load())
        {
          bool found = false;
          const long double tmp = sc_.load_float (attribute_, hint, &found);
          if (found)
            val = tmp;
        }
      if (sc_.in_save())
        sc_.save_float (attribute_, val, hint);
    }
  else // std::is_integral<T>::value
    {
      if (sc_.in_load())
        {
          bool found = false;
          const int64 tmp = sc_.load_integral (attribute_, hint, &found);
          if (found)
            val = tmp;
        }
      if (sc_.in_save())
        sc_.save_integral (attribute_, val, hint);
    }
}

template<class T, REQUIRES< std::is_base_of<std::string, T>::value > > inline void
SerializeContext::VPort::operator>>= (T &val)
{
  if (sc_.in_load())
    {
      bool found = false;
      const String tmp = sc_.load_string (attribute_, 0, &found);
      if (found)
        val = tmp;
    }
  if (sc_.in_save())
    sc_.save_string (attribute_, val);
}

template<class T, REQUIRES< IsVector<T>::value > > inline void
SerializeContext::VPort::operator>>= (T &val)
{
  typedef typename std::decay<typename T::value_type>::type ArgType;
  if (sc_.in_load())
    {
      SerializeContextP range_context = sc_.load_nested (attribute_, 'R');
      std::vector<VPort> ports;
      VPort childport (*range_context, "");
      if (range_context && range_context->in_load())
        {
          const size_t n_children = range_context->count_children();
          childport.attribute_ = range_context->first_child_name();
          for (size_t i = 0; i < n_children; i++)       // do *exactly* n_children rotations
            {
              if (childport.attribute_.empty() == false && !range_context->in_error())
                {
                  ArgType element = ArgType();
                  childport >>= element;
                  if (!range_context->in_error())
                    val.push_back (element);
                }
              range_context->rotate_children();         // rotating *all* chldren preserves order
              childport.attribute_ = range_context->first_child_name();
            }
        }
    }
  if (sc_.in_save())
    {
      SerializeContextP range_context = sc_.save_nested (attribute_, 'R');
      if (range_context && range_context->in_save() && !range_context->in_error())
        for (auto &e : val)
          {
            ArgType &element = e;
            (*range_context)["e"] >>= element;
            if (range_context->in_error())
              break;
          }
    }
}

template<class T, REQUIRES< Has_serialize_content<T>::value > > inline void
SerializeContext::VPort::operator>>= (T &val)
{
  if (sc_.in_load())
    {
      SerializeContextP class_context = sc_.load_nested (attribute_, 'O');
      if (class_context && class_context->in_load() && !class_context->in_error())
        serialize_content (val, *class_context);
    }
  if (sc_.in_save())
    {
      SerializeContextP class_context = sc_.save_nested (attribute_, 'O');
      if (class_context && class_context->in_save() && !class_context->in_error())
        {
          const String serialize_type = Factory::name_type (val, false);
          if (!serialize_type.empty())
            class_context->save_string (sc_.sc_type, serialize_type, '-');
          serialize_content (val, *class_context);
        }
    }
}

template<class T, REQUIRES< Has_serialize_content<T>::value > > inline void
SerializeContext::VPort::operator>>= (T* &val)
{
  if (sc_.in_load())
    {
      SerializeContextP class_context = sc_.load_nested (attribute_, 'O');
      if (class_context && class_context->in_load() && !class_context->in_error())
        {
          bool found = false;
          const String refid = class_context->load_string (sc_.sc_ref, '-', &found);
          if (refid.empty())
            val = NULL;
          else // !refid.empty()
            {
              std::shared_ptr<T> tmp = sc_.load_object<T> (refid, /*make_shared*/ false);
              if (!sc_.in_error())
                val = tmp.get();
            }
        }
    }
  if (sc_.in_save())
    {
      T *const obj = val;
      SerializeContextP class_context = sc_.save_nested (attribute_, 'O');
      if (class_context && class_context->in_save() && !class_context->in_error())
        {
          if (obj)
            {
              String uid = sc_.save_object (*obj);
              class_context->save_string (sc_.sc_ref, uid, '-');
            }
        }
    }
}

template<class T, REQUIRES< Has_serialize_content<T>::value > > inline void
SerializeContext::VPort::operator>>= (std::shared_ptr<T> &val)
{
  if (sc_.in_load())
    {
      SerializeContextP class_context = sc_.load_nested (attribute_, 'O');
      if (class_context && class_context->in_load() && !class_context->in_error())
        {
          bool found = false;
          const String refid = class_context->load_string (sc_.sc_ref, '-', &found);
          if (refid.empty())
            val = NULL;
          else // !refid.empty()
            {
              std::shared_ptr<T> tmp = sc_.load_object<T> (refid, /*make_shared*/ true);
              if (!sc_.in_error())
                val = tmp;
            }
        }
    }
  if (sc_.in_save())
    {
      T *const obj = val.get();
      SerializeContextP class_context = sc_.save_nested (attribute_, 'O');
      if (class_context && class_context->in_save() && !class_context->in_error())
        {
          if (obj)
            {
              String uid = sc_.save_object (*obj);
              class_context->save_string (sc_.sc_ref, uid, '-');
            }
        }
    }
}

// == SerializeContext impl ==
template<class T, REQUIRES< Has_serialize_content<T>::value > > std::shared_ptr<T>
SerializeContext::load_object (const std::string &objectid, bool make_shared)
{
  assert_return (!in_save(), NULL);
  assert_return (in_load() == !is_root(), NULL);
  if (objectid.empty())
    return NULL;
  enter_load();
  Aux::ErasedPtr eptr = refmap_fetch (objectid);
  std::shared_ptr<T> targetp = eptr.slow_cast<T>();
  if (!targetp && eptr.empty())
    {
      SerializeContextP class_context = find_nested_by_uid (objectid);
      if (class_context && class_context->in_load() && !class_context->in_error())
        {
          const String serialize_type = class_context->load_string (sc_type, '-', NULL);
          if (!serialize_type.empty())
            {
              Aux::ErasedPtr eptr = Factory::load_type (serialize_type, *class_context, objectid, make_shared);
              if (!class_context->in_error())
                targetp = eptr.slow_cast<T>();
            }
        }
    }
  else if (!targetp) // !eptr.empty()
    set_error (string_format ("object '%s' deserialization type mismatch", objectid));
  leave_load();
  return targetp;
}

template<class T, REQUIRES< Has_serialize_content<T>::value > > String
SerializeContext::save_object (T &val)
{
  String uid;
  assert_return (!in_load(), uid);
  assert_return (in_save(), uid);
  if (!in_error())
    {
      bool isunsaved = false;
      uid = find_uid (dynamic_cast<void*> (&val), typeid (val), is_root(), &isunsaved);
      SerializeContextP class_context = isunsaved ? save_nested (sc_object, 'S') : NULL;
      if (!uid.empty() && class_context && class_context->in_save() && !class_context->in_error())
        {
          class_context->save_string (sc_uid, uid, '-');
          const String serialize_type = Factory::name_type (val);
          if (!serialize_type.empty() && !class_context->in_error())
            class_context->save_string (sc_type, serialize_type, '-');
          if (!class_context->in_error())
            serialize_content (val, *class_context);
        }
    }
  return uid;
}

template<class T, REQUIRES< Has_serialize_content<T>::value > > bool
SerializeContext::load (T &val)
{
  assert_return (!in_save(), false);
  assert_return (!in_load(), false);
  if (in_error())
    return false;
  enter_load();
  SerializeContextP class_context = find_nested_by_uid (sc_main);
  if (class_context && class_context->in_load() && !class_context->in_error())
    {
      std::shared_ptr<T> sptr (std::shared_ptr<T>{},    // create shared_ptr without Deleter
                               std::addressof (val));   // point at &val without taking ownership
      refmap_add (sc_main, Aux::ErasedPtr (sptr));      // just remember &val as sc_main during load()
      serialize_content (val, *class_context);          // so we can resolve nested references to sc_main
    }
  clear_maps();
  leave_load();
  return !in_error();
}

template<class T, REQUIRES< Has_serialize_content<T>::value > > String
SerializeContext::save (T &val)
{
  String uid;
  assert_return (!in_load(), uid);
  assert_return (!in_save(), uid);
  enter_save();
  uid = save_object (val);
  clear_maps();
  leave_save();
  return uid;
}

SerializeContext::VPort
SerializeContext::operator[] (const String &attribute)
{
  return VPort (*this, attribute);
}

// == SerializableBase ==
class SerializableBase : public virtual VirtualEnableSharedFromThis<SerializableBase> {
protected:
  explicit       SerializableBase    ();
  virtual       ~SerializableBase    ();
  template<class Class, typename Ret, typename Arg> void
  serialize_property (SerializeContext &sc, const String &propertyname, Class *self, Ret (Class::*get) () const, void (Class::*set) (Arg))
  {
    auto ioport = sc[propertyname];
    if (sc.in_load())
      {
        typedef typename std::decay<Arg>::type ArgType;
        ArgType arg = ArgType();
        ioport >>= arg;
        (self->*set) (arg);
      }
    if (sc.in_save())
      {
        Ret val = (self->*get) ();
        ioport >>= val;
      }
  }
  /// Load or save the state and contents of @a this to @a SerializeContext.
  virtual void   serialize_content   (SerializeContext&)        {}
  friend class SerializeContext;
};

} // Bse

#endif // __BSE_SERIALIZE_HH__
