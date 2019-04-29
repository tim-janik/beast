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
class ErasedPtr : public std::shared_ptr<void> {
  // Based on: http://twimgs.com/ddj/images/article/2011/0411/any_ptr.h/any_ptr.h
  // https://web.archive.org/web/2015/http://www.drdobbs.com/cpp/twisting-the-rtti-system-for-safe-dynami/229401004
  // https://web.archive.org/web/2015/http://www.drdobbs.com/cpp/twisting-the-rtti-system-for-safe-dynami/229401004?pgno=2
  using VoidPtr = std::shared_ptr<void>;
  void      (ErasedPtr::*throw_)   () const = NULL;
  template<class T> void thrower   () const                             { throw static_cast<T*> (this->get()); }
public:
  /*ctor*/               ErasedPtr () = default;
  template<class T>      ErasedPtr (const std::shared_ptr<T> &objectp) :
    VoidPtr (objectp), throw_ (&ErasedPtr::thrower<T>)
  {
    if (false) // AUDIT only, slow
      BSE_ASSERT_RETURN (ptrdiff_t (objectp.get()) == ptrdiff_t (dynamic_cast<void*> (objectp.get())));
    BSE_ASSERT_RETURN (typeid (*objectp.get()) == typeid (T));
  }
  template<class U> std::shared_ptr<U>
  slow_cast () const
  {
    if (*this)
      {
        // throw (unknown) derived type, catch base type if possible, slow but valid C++
        try { (this->*throw_) (); }
        catch (U *basep) {
          // NOTE: for multiple inheritance, ptrdiff_t (ptr_.get()) != ptrdiff_t (basep)
          std::shared_ptr<U> targetp (*this,    // reuse VoidPtr Deleter
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
/// HasEnumAsString<T> - Check if enum type @a T provides to_string() and from_string().
template<class, class = void> struct HasEnumAsString : std::false_type {}; // Assume false_type as a fallback.
template<class E> struct HasEnumAsString<E, void_t< decltype (to_string (std::declval<E>())),
                                                    decltype (from_string (std::string(), std::declval<E&>())) >
                                         > : std::true_type {};
/// IsVector<T> - Check if @a T is a std::vector<>.
template<class T, typename = void> struct IsVector : std::false_type {};
// use void_t to prevent errors for T without vector's typedefs
template<class T> struct IsVector<T, void_t< typename T::value_type, typename T::allocator_type > > :
    std::is_base_of< std::vector<typename T::value_type, typename T::allocator_type>, T > {};
/// Template to call serialize_content (T&,SerializeContext&) on a class @a T or invoke T::serialize_content(SerializeContext&).
template<class T> inline auto
serialize_content (T &t, SerializeContext &sc) -> decltype (t.serialize_content (std::declval<SerializeContext&>()))
{ return t.serialize_content (sc); }
/// Has_serialize_content<T> - Check if @a T provides serialize_content() as a method or external function.
template<class, class = void> struct
Has_serialize_content : std::false_type {};
template<class T> struct
Has_serialize_content<T, void_t< decltype (serialize_content (std::declval<T&>(), std::declval<SerializeContext&>())) > > : std::true_type {};
/// Helper structure to implement Has___visit__
struct HasHelper : std::true_type {
  static constexpr auto visitor_lambda = [] (auto, const char*) {};
};
/// Has___visit__<T> - Check if @a T provides a @a __visit__(Visitor) method template.
template<class, class = void> struct
Has___visit__ : std::false_type {};
template<class T> struct
Has___visit__<T, void_t< decltype (std::declval<T&>().__visit__ (HasHelper::visitor_lambda)) > > : std::true_type {};

/// Has__iface__<T> - Check if @a T provides a @a __iface__() method.
template<class, class = void> struct
Has__iface__ : std::false_type {};
template<class T> struct
Has__iface__<T, void_t< decltype (std::declval<T&>().__iface__()) > > : std::true_type {};

// == SerializeContext ==
typedef std::shared_ptr<SerializeContext> SerializeContextP;
typedef std::vector<std::shared_ptr<void>> VoidPtrVector;    ///< Convenience alias for a std::vector<std::shared_ptr<void>>.
/// Serialization context to save or load C++ class graphs with named properties.
class SerializeContext : public virtual Aida::EnableSharedFromThis<SerializeContext> {
  friend class FriendAllocator<SerializeContext>; // Allows access to ctor/dtor.
  struct Private;
  Private           &p_;
  int64_t            privatemem_[(sizeof (std::shared_ptr<void*>) * 2 + 7) / 8];
  bool               is_range_;
  static const char *attr_main;
  static const char *attr_this;
  static const char *attr_item;
  static const char *attr_link;
  static const char *attr_object;
  static const char *attr_record;
  static const char *attr_typeid;
  String             find_uid           (void *address, const std::type_info &type, bool isroot, bool *unsaved);
  void               refmap_add         (const String &refid, const Aux::ErasedPtr &eptr);
  Aux::ErasedPtr     refmap_fetch       (const String &refid);
  void               clear_maps         ();
protected:
  class VPort; // Value Port
  // Storage methods, mostly type erased
  explicit           SerializeContext   (Private &priv);
  virtual           ~SerializeContext   ();
  void               set_error          (const String &error);
  void               enter_load         ();
  void               leave_load         ();
  void               enter_save         ();
  void               leave_save         ();
  void               save_comment       (const String &comment);
  SerializeContextP  find_nested_by_uid (const std::string &objectid);
  SerializeContextP  load_nested        (const String &attribute, char hint);
  SerializeContextP  save_nested        (const String &attribute, char hint);
  int64              load_integral      (const String &attribute, char hint, bool *found);
  void               save_integral      (const String &attribute, int64 val, char hint = 0);
  long double        load_float         (const String &attribute, char hint, bool *found);
  void               save_float         (const String &attribute, long double val, char hint = 0);
  std::string        load_string        (const String &attribute, bool *found = NULL);
  void               save_string        (const String &attribute, const std::string &val, char hint = 0);
  std::string        first_child_name   ();                             ///< Retrieve the name of the first child during load().
  size_t             count_children     ();                             ///< Retrieve the number of available children.
  void               rotate_children    ();                             ///< Rotate children, i.e. pop first and append it at the tail.
  String             debug_name         ();
  template<class T>
  std::string        save_object        (T &val, bool isroot = false);  ///< Save @a val contents and return its unique Id.
  template<class T>
  std::shared_ptr<T> load_object        (const std::string &objectid);  ///< Create and load any object identified through its @a objectid.
  template<class T, REQUIRES< Has_serialize_content<T>::value && std::is_polymorphic<T>::value > = true>
  String             save               (T &val);                       ///< Save @a val contents and return its unique Id.
  template<class T, REQUIRES< Has___visit__<T>::value && !std::is_polymorphic<T>::value > = true>
  String             save               (T &val);                       ///< Save @a val contents.
  template<class T, REQUIRES< Has_serialize_content<T>::value && std::is_polymorphic<T>::value > = true>
  bool               load               (T &val);                       ///< Load @a val contents through its serialize_content() method.
  template<class T, REQUIRES< Has___visit__<T>::value && !std::is_polymorphic<T>::value > = true>
  bool               load               (T &val);                       ///< Load @a val contents through its __visit__() method.
  size_t             copy_all_pointers  (VoidPtrVector&);               ///< Copy all shared_ptr<> objects created during load().
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
  class Factory;
};

class SerializeToXML {
  SerializeContextP sc_;
  std::string       result_;
public:
  explicit SerializeToXML (const String &roottag = "", const String &version = "");
  virtual              ~SerializeToXML ();
  /// Save @a val contents and return its unique Id.
  template<class T, REQUIRES< Has_serialize_content<T>::value || Has___visit__<T>::value > = true>
  SerializeToXML&       save        (T &val) { sc_->save (val); return *this; }
  std::string           to_xml      ();
  bool                  in_error    () const { return sc_->in_error(); }  ///< Indicate if an error occoured.
  std::string           error       () const { return sc_->error(); }     ///< Retrieve error description.
};

class SerializeFromXML {
  SerializeContextP sc_;
public:
  explicit SerializeFromXML    (const std::string &input);
  virtual ~SerializeFromXML    ();
  size_t   copy_all_pointers   (VoidPtrVector&);                ///< Copy all shared_ptr<> objects created during load().
  ///< Load @a val contents through its serialize_content() method.
  template<class T, REQUIRES< Has_serialize_content<T>::value || Has___visit__<T>::value > = true>
  bool        load        (T &val) { return sc_->load (val); }
  bool        in_error    () const { return sc_->in_error(); }  ///< Indicate if an error occoured.
  std::string error       () const { return sc_->error(); }     ///< Retrieve error description.
};

// == SerializeContext::Factory ==
class SerializeContext::Factory {
  static Factory        *factories_;
  Factory               *next_;
  static Factory*        find_factory            (const std::string &factorytype);
  static Factory*        find_factory            (const std::type_info &typeinfo);       ///< Find factory or return NULL.
  static Factory*        lookup_factory          (const std::type_info &typeinfo);       ///< Warn if no factory can be found.
protected:
  const std::type_info  &typeinfo_;
  const std::string      factory_type_;
  String                 canonify_mangled        (const char *mangled);
  void                   refmap_add              (SerializeContext &sc, const String &refid, const Aux::ErasedPtr &eptr) { sc.refmap_add (refid, eptr); }
  explicit               Factory                 (const std::type_info &typeinfo);
  virtual               ~Factory                 ();
  virtual Aux::ErasedPtr create_and_load         (SerializeContext &bootsc, const String &refid) = 0;
  virtual Aux::ErasedPtr adopt_and_load          (void *vptr, SerializeContext &serialize_context, const String &refid) = 0;
  virtual bool           apply_serialize_content (void *vptr, SerializeContext &serialize_context) = 0;
public:
  String                 factory_type            () const       { return factory_type_; }
  static Aux::ErasedPtr  create_and_load         (const std::string &factorytype, SerializeContext &bootsc, const String &refid);
  template<class T, REQUIRES< std::is_polymorphic<T>::value > = true >
  static bool            call_serialize_content  (T &val, SerializeContext &serialize_context, String *serialize_type);
  template<class T, REQUIRES< !std::is_polymorphic<T>::value > = true >
  static bool            call_serialize_content  (T &val, SerializeContext &serialize_context, String *serialize_type)   { return false; }
  template<class T, REQUIRES< std::is_polymorphic<T>::value > = true >
  static Aux::ErasedPtr  adopt_and_load          (T &val, SerializeContext &serialize_context, const String &refid);
};

template<class T, REQUIRES< std::is_polymorphic<T>::value > > inline bool
SerializeContext::Factory::call_serialize_content (T &val, SerializeContext &serialize_context, String *serialize_type)
{
  // cast &val to most derived pointer via dynamic_cast<void*> and have it
  // handled by the factory registered for the most derived type.
  Factory *factory = lookup_factory (typeid (val));
  if (factory && !serialize_context.in_error())
    {
      *serialize_type = factory->factory_type();
      return factory->apply_serialize_content (dynamic_cast<void*> (&val), serialize_context);
    }
  return false;
}

template<class T, REQUIRES< std::is_polymorphic<T>::value > > inline Aux::ErasedPtr
SerializeContext::Factory::adopt_and_load (T &val, SerializeContext &serialize_context, const String &refid)
{
  // cast &val to most derived pointer via dynamic_cast<void*> and have it
  // handled by the factory registered for the most derived type.
  Factory *factory = lookup_factory (typeid (val));
  if (factory && !serialize_context.in_error())
    {
      return factory->adopt_and_load (dynamic_cast<void*> (&val), serialize_context, refid);
    }
  return Aux::ErasedPtr();
}

inline Aux::ErasedPtr
SerializeContext::Factory::create_and_load (const std::string &factorytype, SerializeContext &serialize_context, const String &refid)
{
  Factory *factory = find_factory (factorytype);
  if (factory && !serialize_context.in_error())
    return factory->create_and_load (serialize_context, refid);
  else
    return Aux::ErasedPtr();
}

// == SerializeFactory ==
template<class T, REQUIRES< Has_serialize_content<T>::value && std::is_polymorphic<T>::value > = true >
class SerializeFactory : SerializeContext::Factory {
  using MakeSharedFunc = std::function<std::shared_ptr<T>(const std::string&)>;
  using SerializeContentFunc = std::function<void(T&,SerializeContext&)>;
  MakeSharedFunc             makeshared_;
  SerializeContentFunc       serializecontent_;
  virtual bool
  apply_serialize_content (void *vptr, SerializeContext &serialize_context) override
  {
    BSE_ASSERT_RETURN (vptr != NULL, false);
    T *const t = static_cast<T*> (vptr);                // caller must pass `vptr` as most-derived
    BSE_ASSERT_RETURN (typeid (*t) == typeinfo_, false);
    serializecontent_ (*t, serialize_context);
    return true;
  }
  virtual Aux::ErasedPtr
  adopt_and_load (void *vptr, SerializeContext &serialize_context, const String &refid) override
  {
    Aux::ErasedPtr eptr;
    BSE_ASSERT_RETURN (vptr != NULL, eptr);
    T *const t = static_cast<T*> (vptr);                // caller must pass `vptr` as most-derived
    BSE_ASSERT_RETURN (typeid (*t) == typeinfo_, eptr);
    std::shared_ptr<T> tp (t, [] (T*) {});              // create shared_ptr without Deleter
    eptr = Aux::ErasedPtr (tp);                         // point at `t` without taking ownership
    refmap_add (serialize_context, refid, eptr);        // allow resolution of `refid` to `t` during load()
    serializecontent_ (*t, serialize_context);
    return eptr;
  }
  virtual Aux::ErasedPtr
  create_and_load (SerializeContext &bootsc, const String &refid) override
  {
    Aux::ErasedPtr eptr;
    BSE_ASSERT_RETURN (!bootsc.in_save(), eptr);
    BSE_ASSERT_RETURN (bootsc.in_load(), eptr);
    std::shared_ptr<T> tp = makeshared_ (factory_type());
    eptr = Aux::ErasedPtr (tp);
    refmap_add (bootsc, refid, eptr);
    serializecontent_ (*tp.get(), bootsc);
    return eptr;
  }
public:
  SerializeFactory (const MakeSharedFunc &makeshared = NULL, const SerializeContentFunc &serializecontent = NULL) :
    Factory (typeid (T)), makeshared_ (makeshared), serializecontent_ (serializecontent)
  {
    if (!makeshared_)
      makeshared_ = [] (const std::string&) { return std::make_shared<T>(); };
    if (!serializecontent_)
      serializecontent_ = [] (T &t, SerializeContext &sc) { serialize_content (t, sc); };
  }
};
#define BSE_SERIALIZATION_EXPORT(TYPE)             BSE_SERIALIZATION_NAMED_EXPORT (TYPE, #TYPE)
#define BSE_SERIALIZATION_NAMED_EXPORT(TYPE, NAME) static ::Bse::SerializeFactory<TYPE> __Bse_SerializeFactory__ ## TYPE ## __

// == SerializeContext::VPort ==
class SerializeContext::VPort {
  SerializeContext &sc_;
  String            attribute_;
  SerializeContextP scptr_;
  template<class T, REQUIRES<  HasEnumAsString<T>::value > = true> void serialize_enum (T &val);
  template<class T, REQUIRES< !HasEnumAsString<T>::value > = true> void serialize_enum (T &val);
  template<class T> static void save_vector (std::vector<T> &vec, SerializeContext &sc);
public:
  VPort (SerializeContext &sc, const String &attribute) :
    sc_ (sc), attribute_ (attribute), scptr_ (sc_.shared_from_this())
  {}
  // accessors
  SerializeContext& serialize_context () const { return sc_; }
  String            attribute         () const { return attribute_; }
  // serializing
  template<class T, REQUIRES< std::is_enum<T>::value > = true>                  void operator->* (T &val);
  template<class T, REQUIRES< std::is_arithmetic<T>::value > = true>            void operator->* (T &val);
  template<class T, REQUIRES< std::is_base_of<std::string, T>::value > = true>  void operator->* (T &val);
  template<class T, REQUIRES< IsVector<T>::value > = true>                      void operator->* (T &val);
  template<class T, REQUIRES< Has_serialize_content<T>::value > = true>         void operator->* (T &val);
  template<class T, REQUIRES< Has_serialize_content<T>::value > = true>         void operator->* (T* &val);
  template<class T, REQUIRES< Has_serialize_content<T>::value > = true>         void operator->* (std::shared_ptr<T> &val);
  template<class T, REQUIRES< Has___visit__<T>::value &&
                              !std::is_polymorphic<T>::value > = true>          void operator->* (T &val);
  template<class T, REQUIRES< std::is_base_of<const Any, T>::value > = true>    void operator->* (T &val);
  template<class T, REQUIRES< Aida::IsRemoteHandleDerived<T>::value &&
                              Has__iface__<T>::value > = true>                  void operator->* (T &val);
  template<class T, REQUIRES< Aida::IsRemoteHandleDerived<T>::value &&
                              !Has__iface__<T>::value > = true>                 void operator->* (T &val);
};

template<class T, REQUIRES< !HasEnumAsString<T>::value > > inline void
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
    sc_.save_integral (attribute_, int64 (val), hint);
}

template<class T, REQUIRES< HasEnumAsString<T>::value > > inline void
SerializeContext::VPort::serialize_enum (T &val)
{
  const char hint = 'E';
  if (sc_.in_load())
    {
      bool found = false;
      const std::string tmp = sc_.load_string (attribute_, &found);
      if (found)
        {
          if (tmp.size() && tmp[0] >= '0' && tmp[0] <= '9')
            val = T (string_to_int (tmp));
          else
            from_string (tmp, val);
        }
    }
  if (sc_.in_save())
    sc_.save_string (attribute_, to_string (val), hint);
}

template<class T, REQUIRES< std::is_enum<T>::value > > inline void
SerializeContext::VPort::operator->* (T &val)
{
  return serialize_enum (val);
}

template<class T, REQUIRES< std::is_arithmetic<T>::value > > inline void
SerializeContext::VPort::operator->* (T &val)
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
SerializeContext::VPort::operator->* (T &val)
{
  if (sc_.in_load())
    {
      bool found = false;
      const String tmp = sc_.load_string (attribute_, &found);
      if (found)
        val = tmp;
    }
  if (sc_.in_save())
    sc_.save_string (attribute_, val);
}

template<class T, REQUIRES< IsVector<T>::value > > inline void
SerializeContext::VPort::operator->* (T &val)
{
  typedef typename std::decay<typename T::value_type>::type ArgType;
  if (sc_.in_load())
    {
      SerializeContextP range_context = sc_.load_nested (attribute_, 'S');
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
                  childport ->* element;
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
      SerializeContextP range_context = sc_.save_nested (attribute_, 'S');
      if (!range_context || !range_context->in_save() || range_context->in_error())
        return;
      save_vector (val, *range_context);
    }
}

template<class T> inline void
SerializeContext::VPort::save_vector (std::vector<T> &vec, SerializeContext &sc)
{
  typedef typename std::decay<typename std::vector<T>::value_type>::type ValueType;
  for (auto &e : vec)
    if (!sc.in_error())
      {
        ValueType &element = e;
        sc[attr_item] ->* element;
      }
}

template<> inline void
SerializeContext::VPort::save_vector (std::vector<bool> &vec, SerializeContext &sc)
{
  for (bool bool_value : vec)
    if (!sc.in_error())
      sc[attr_item] ->* bool_value;
}

template<class T, REQUIRES< std::is_base_of<const Any, T>::value > > inline void
SerializeContext::VPort::operator->* (T &val)
{
  if (sc_.in_load())
    ;
  if (sc_.in_save())
    {
      if (!sc_.in_error())
        sc_.save_comment (attribute_ + ": Any");
    }
}

template<class T, REQUIRES< Has___visit__<T>::value && !std::is_polymorphic<T>::value > > inline void
SerializeContext::VPort::operator->* (T &val)
{
  if (sc_.in_load())
    {
      SerializeContextP rec_context = sc_.load_nested (attribute_, 'R');
      if (!rec_context || !rec_context->in_load() || rec_context->in_error())
        return;
      SerializeContext &rc = *rec_context;
      val.__visit__ ([&rc] (auto &v, const char *n)
                     {
                       if (!rc.in_error())
                         rc[n] ->* v;
                     });
    }
  if (sc_.in_save())
    {
      SerializeContextP rec_context = sc_.save_nested (attribute_, 'R');
      if (!rec_context || !rec_context->in_save() || rec_context->in_error())
        return;
      SerializeContext &rc = *rec_context;
      val.__visit__ ([&rc] (auto &v, const char *n)
                     {
                       if (!rc.in_error())
                         rc[n] ->* v;
                     });
    }
}

template<class T, REQUIRES< Has_serialize_content<T>::value > > inline void
SerializeContext::VPort::operator->* (T &val)
{
  if (sc_.in_load())
    {
      SerializeContextP class_context = sc_.load_nested (attribute_, 'O');
      if (class_context && class_context->in_load() && !class_context->in_error())
        {
          String factory_type;
          if (Factory::call_serialize_content (val, *class_context, &factory_type))
            {
              const String serialize_type = class_context->load_string (attr_typeid);
              if (!serialize_type.empty() && serialize_type != factory_type)
                warning ("SerializeContext: mismatching serialization type \"%s\" for Factory<%s>",
                         serialize_type, factory_type);
            }
          else
            serialize_content (val, *class_context);
        }
    }
  if (sc_.in_save())
    {
      SerializeContextP class_context = sc_.save_nested (attribute_, 'O');
      if (class_context && class_context->in_save() && !class_context->in_error())
        {
          String factory_type;
          if (Factory::call_serialize_content (val, *class_context, &factory_type))
            class_context->save_string (attr_typeid, factory_type, '^');
          else
            serialize_content (val, *class_context);
        }
    }
}

template<class T, REQUIRES< Aida::IsRemoteHandleDerived<T>::value && !Has__iface__<T>::value > > inline void
SerializeContext::VPort::operator->* (T &val)
{
  if (sc_.in_load())
    ;   // there is no way to load handles without __iface_ptr__ access
  if (sc_.in_save() && !sc_.in_error())
    sc_.save_comment (attribute_ + ": " + Aida::string_demangle_cxx (typeid (T).name()) + " ");
}

template<class T, REQUIRES< Aida::IsRemoteHandleDerived<T>::value && Has__iface__<T>::value > > inline void
SerializeContext::VPort::operator->* (T &val)
{
  using UnderlyingIface = typename std::remove_pointer<decltype (val.__iface__())>::type;
  using UnderlyingIfaceP = std::shared_ptr<UnderlyingIface>;
  if (sc_.in_load())
    {
      UnderlyingIfaceP ifacep;
      //this->operator->* (ifacep);
      SerializeContextP class_context = sc_.load_nested (attribute_, 'O');
      if (class_context && class_context->in_load() && !class_context->in_error())
        {
          bool found = false;
          const String refid = class_context->load_string (attr_link, &found);
          if (!refid.empty())
            {
              UnderlyingIfaceP tmp = sc_.load_object<UnderlyingIface> (refid);
              if (!sc_.in_error())
                ifacep = tmp;
            }
        }
      val = ifacep ? ifacep->__handle__() : T();
    }
  if (sc_.in_save())
    {
      UnderlyingIface *iface = val.__iface__();
      //this->operator->* (iface);
      SerializeContextP class_context = sc_.save_nested (attribute_, 'O');
      if (class_context && class_context->in_save() && !class_context->in_error() && iface)
        {
          String uid = sc_.save_object (*iface);
          class_context->save_string (attr_link, uid, '^');
        }
    }
}

template<class T, REQUIRES< Has_serialize_content<T>::value > > inline void
SerializeContext::VPort::operator->* (T* &val)
{
  if (sc_.in_load())
    {
      SerializeContextP class_context = sc_.load_nested (attribute_, 'O');
      if (class_context && class_context->in_load() && !class_context->in_error())
        {
          bool found = false;
          const String refid = class_context->load_string (attr_link, &found);
          if (refid.empty())
            val = NULL;
          else // !refid.empty()
            {
              std::shared_ptr<T> tmp = sc_.load_object<T> (refid);
              if (!sc_.in_error())
                val = tmp.get();
            }
        }
    }
  if (sc_.in_save())
    {
      T *const obj = val;
      SerializeContextP class_context = sc_.save_nested (attribute_, 'O');
      if (class_context && class_context->in_save() && !class_context->in_error() && obj)
        {
          String uid = sc_.save_object (*obj);
          class_context->save_string (attr_link, uid, '^');
        }
    }
}

template<class T, REQUIRES< Has_serialize_content<T>::value > > inline void
SerializeContext::VPort::operator->* (std::shared_ptr<T> &val)
{
  if (sc_.in_load())
    {
      SerializeContextP class_context = sc_.load_nested (attribute_, 'O');
      if (class_context && class_context->in_load() && !class_context->in_error())
        {
          bool found = false;
          const String refid = class_context->load_string (attr_link, &found);
          if (refid.empty())
            val = NULL;
          else // !refid.empty()
            {
              std::shared_ptr<T> tmp = sc_.load_object<T> (refid);
              if (!sc_.in_error())
                val = tmp;
            }
        }
    }
  if (sc_.in_save())
    {
      T *const obj = val.get();
      SerializeContextP class_context = sc_.save_nested (attribute_, 'O');
      if (class_context && class_context->in_save() && !class_context->in_error() && obj)
        {
          String uid = sc_.save_object (*obj);
          class_context->save_string (attr_link, uid, '^');
        }
    }
}

// == SerializeContext impl ==
template<class T> inline std::shared_ptr<T>
SerializeContext::load_object (const std::string &objectid)
{
  BSE_ASSERT_RETURN (!in_save(), NULL);
  BSE_ASSERT_RETURN (in_load(), NULL);
  if (objectid.empty())
    return NULL;
  enter_load();
  Aux::ErasedPtr eptr = refmap_fetch (objectid);
  std::shared_ptr<T> targetp = eptr.slow_cast<T>();
  if (!targetp && !eptr)
    {
      SerializeContextP class_context = find_nested_by_uid (objectid);
      if (class_context && class_context->in_load() && !class_context->in_error())
        {
          const String serialize_type = class_context->load_string (attr_typeid);
          if (!serialize_type.empty())
            {
              Aux::ErasedPtr eptr = Factory::create_and_load (serialize_type, *class_context, objectid);
              if (!class_context->in_error())
                targetp = eptr.slow_cast<T>();
            }
        }
    }
  if (!targetp) // !eptr.empty()
    set_error (string_format ("object '%s' deserialization type mismatch", objectid));
  leave_load();
  return targetp;
}

template<class T> inline String
SerializeContext::save_object (T &val, bool isroot)
{
  String uid;
  BSE_ASSERT_RETURN (!in_load(), uid);
  BSE_ASSERT_RETURN (in_save(), uid);
  if (!in_error())
    {
      bool isunsaved = false;
      uid = find_uid (dynamic_cast<void*> (&val), typeid (val), isroot, &isunsaved);
      SerializeContextP class_context = isunsaved ? save_nested (attr_object, '^') : NULL;
      if (!uid.empty() && class_context && class_context->in_save() && !class_context->in_error())
        {
          class_context->save_string (attr_this, uid, '^');
          String factory_type;
          if (Factory::call_serialize_content (val, *class_context, &factory_type))
            class_context->save_string (attr_typeid, factory_type, '^');
        }
    }
  return uid;
}

template<class T, REQUIRES< Has_serialize_content<T>::value && std::is_polymorphic<T>::value > > inline bool
SerializeContext::load (T &val)
{
  BSE_ASSERT_RETURN (!in_save(), false);
  BSE_ASSERT_RETURN (!in_load(), false);
  if (in_error())
    return false;
  enter_load();
  SerializeContextP class_context = find_nested_by_uid (attr_main);
  if (class_context && class_context->in_load() && !class_context->in_error())
    {
      // To resolve nested "main" references to `val`, use adopt_and_load() instead of
      // call_serialize_content(), which adds `val` to the refmap without deleter.
      Aux::ErasedPtr eptr = Factory::adopt_and_load (val, *class_context, attr_main);
    }
  clear_maps();
  leave_load();
  return !in_error();
}

template<class T, REQUIRES< Has_serialize_content<T>::value && std::is_polymorphic<T>::value > > inline String
SerializeContext::save (T &val)
{
  String uid;
  BSE_ASSERT_RETURN (!in_load(), uid);
  BSE_ASSERT_RETURN (!in_save(), uid);
  enter_save();
  uid = save_object (val, true);
  clear_maps();
  leave_save();
  return uid;
}

template<class T, REQUIRES< Has___visit__<T>::value && !std::is_polymorphic<T>::value > > inline String
SerializeContext::save (T &val)
{
  BSE_ASSERT_RETURN (!in_load(), "");
  BSE_ASSERT_RETURN (!in_save(), "");
  enter_save();
  if (!in_error())
    {
      SerializeContextP rec_context = save_nested (attr_record, 'R');
      if (rec_context && rec_context->in_save() && !rec_context->in_error())
        {
          SerializeContext &rc = *rec_context;
          rc.save_string (attr_typeid, Aida::string_demangle_cxx (typeid (T).name()), '^');
          val.__visit__ ([&rc] (auto &v, const char *n)
                         {
                           if (!rc.in_error())
                             rc[n] ->* v;
                         });
        }
    }
  clear_maps();
  leave_save();
  return "";
}

template<class T, REQUIRES< Has___visit__<T>::value && !std::is_polymorphic<T>::value > > inline bool
SerializeContext::load (T &val)
{
  BSE_ASSERT_RETURN (!in_save(), false);
  BSE_ASSERT_RETURN (!in_load(), false);
  if (in_error())
    return false;
  enter_load();
  if (!in_error())
    {
      SerializeContextP rec_context = load_nested (attr_record, 'R');
      if (rec_context && rec_context->in_load())
        {
          SerializeContext &rc = *rec_context;
          val.__visit__ ([&rc] (auto &v, const char *n)
                         {
                           if (!rc.in_error())
                             rc[n] ->* v;
                         });
        }
    }
  clear_maps();
  leave_load();
  return !in_error();
}

inline SerializeContext::VPort
SerializeContext::operator[] (const String &attribute)
{
  return VPort (*this, attribute);
}

// == SerializableBase ==
class SerializableBase : public virtual Aida::SharedFromThis<SerializableBase> {
protected:
  explicit       SerializableBase    ();
  virtual       ~SerializableBase    ();
  template<class Class, typename Ret, typename Arg> inline void
  serialize_property (SerializeContext &sc, const String &propertyname, Class *self, Ret (Class::*get) () const, void (Class::*set) (Arg))
  {
    auto ioport = sc[propertyname];
    if (sc.in_load())
      {
        typedef typename std::decay<Arg>::type ArgType;
        ArgType arg = ArgType();
        ioport ->* arg;
        (self->*set) (arg);
      }
    if (sc.in_save())
      {
        Ret val = (self->*get) ();
        ioport ->* val;
      }
  }
  /// Load or save the state and contents of @a this to @a SerializeContext.
  virtual void   serialize_content   (SerializeContext&)        {}
  friend class SerializeContext;
};

} // Bse

#endif // __BSE_SERIALIZE_HH__
