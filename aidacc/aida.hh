// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __AIDA_CXX_AIDA_HH__
#define __AIDA_CXX_AIDA_HH__

#include <string>
#include <vector>
#include <stdint.h>             // uint32_t
#include <stdarg.h>
#include <type_traits>
#include <future>
#include <set>
#include <map>

// == config (FIXME) ==
#define HAVE_SYS_EVENTFD_H 1
#ifndef __G_MAIN_H__
typedef struct _GSource GSource;
#endif // __G_MAIN_H__

namespace Aida {

// == Auxillary macros ==
#define AIDA_CPP_STRINGIFYi(s)  #s // indirection required to expand __LINE__ etc
#define AIDA_CPP_STRINGIFY(s)   AIDA_CPP_STRINGIFYi (s)
#define AIDA_CPP_PASTE2(a,b)    a ## b
#define AIDA_CPP_PASTE(a,b)     AIDA_CPP_PASTE2 (a, b)
#define AIDA_I64ELEMENTS(size)  (((size) + sizeof (int64) - 1) / sizeof (int64)) ///< Length of int64[] array to hold @a size.
#if     __GNUC__ >= 4 || defined __clang__
#define AIDA_UNUSED             __attribute__ ((__unused__))
#define AIDA_DEPRECATED         __attribute__ ((__deprecated__))
#define AIDA_PURE               __attribute__ ((pure))
#define AIDA_NORETURN           __attribute__ ((__noreturn__))
#define AIDA_NOINLINE           __attribute__ ((noinline))
#define AIDA_PRINTF(fix, arx)   __attribute__ ((__format__ (__printf__, fix, arx)))
#define AIDA_ISLIKELY(expr)     __builtin_expect (bool (expr), 1)
#define AIDA_UNLIKELY(expr)     __builtin_expect (bool (expr), 0)
#else   // !__GNUC__
#define AIDA_UNUSED
#define AIDA_DEPRECATED
#define AIDA_PURE
#define AIDA_NORETURN
#define AIDA_NOINLINE
#define AIDA_PRINTF(fix, arx)
#define AIDA_ISLIKELY(expr)     expr
#define AIDA_UNLIKELY(expr)     expr
#endif
#define AIDA_ASSERTION_FAILED(stmt)       ::Aida::assertion_failed (__FILE__, __LINE__, __func__, stmt)
#define AIDA_ASSERT_RETURN(expr,...)      do { if (AIDA_ISLIKELY (expr)) break; AIDA_ASSERTION_FAILED (#expr); return __VA_ARGS__; } while (0)
#define AIDA_ASSERT_RETURN_UNREACHED(...) do { AIDA_ASSERTION_FAILED (NULL); return __VA_ARGS__; } while (0)
#define AIDA_LIKELY             AIDA_ISLIKELY
#define AIDA_CLASS_NON_COPYABLE(ClassName)  \
  /*copy-ctor*/ ClassName  (const ClassName&) = delete; \
  ClassName&    operator=  (const ClassName&) = delete

// == Operations on flags enum classes ==
#define AIDA_DEFINE_ENUM_EQUALITY(Enum)         \
  constexpr bool    operator== (Enum v, int64_t n) { return int64_t (v) == n; } \
  constexpr bool    operator== (int64_t n, Enum v) { return n == int64_t (v); } \
  constexpr bool    operator!= (Enum v, int64_t n) { return int64_t (v) != n; } \
  constexpr bool    operator!= (int64_t n, Enum v) { return n != int64_t (v); }
#define AIDA_DEFINE_FLAGS_ARITHMETIC(Enum)      \
  constexpr int64_t operator>> (Enum v, int64_t n) { return int64_t (v) >> n; } \
  constexpr int64_t operator<< (Enum v, int64_t n) { return int64_t (v) << n; } \
  constexpr int64_t operator^  (Enum v, int64_t n) { return int64_t (v) ^ n; } \
  constexpr int64_t operator^  (int64_t n, Enum v) { return n ^ int64_t (v); } \
  constexpr Enum    operator^  (Enum v, Enum w)    { return Enum (int64_t (v) ^ w); } \
  constexpr int64_t operator|  (Enum v, int64_t n) { return int64_t (v) | n; } \
  constexpr int64_t operator|  (int64_t n, Enum v) { return n | int64_t (v); } \
  constexpr Enum    operator|  (Enum v, Enum w)    { return Enum (int64_t (v) | w); } \
  constexpr int64_t operator&  (Enum v, int64_t n) { return int64_t (v) & n; } \
  constexpr int64_t operator&  (int64_t n, Enum v) { return n & int64_t (v); } \
  constexpr Enum    operator&  (Enum v, Enum w)    { return Enum (int64_t (v) & w); } \
  constexpr int64_t operator~  (Enum v)            { return ~int64_t (v); } \
  constexpr int64_t operator+  (Enum v)            { return +int64_t (v); } \
  constexpr int64_t operator-  (Enum v)            { return -int64_t (v); } \
  constexpr int64_t operator+  (Enum v, int64_t n) { return int64_t (v) + n; } \
  constexpr int64_t operator+  (int64_t n, Enum v) { return n + int64_t (v); } \
  constexpr int64_t operator-  (Enum v, int64_t n) { return int64_t (v) - n; } \
  constexpr int64_t operator-  (int64_t n, Enum v) { return n - int64_t (v); } \
  constexpr int64_t operator*  (Enum v, int64_t n) { return int64_t (v) * n; } \
  constexpr int64_t operator*  (int64_t n, Enum v) { return n * int64_t (v); } \
  constexpr int64_t operator/  (Enum v, int64_t n) { return int64_t (v) / n; } \
  constexpr int64_t operator/  (int64_t n, Enum v) { return n / int64_t (v); } \
  constexpr int64_t operator%  (Enum v, int64_t n) { return int64_t (v) % n; } \
  constexpr int64_t operator%  (int64_t n, Enum v) { return n % int64_t (v); } \
  constexpr Enum&   operator^= (Enum &e, int64_t n) { e = Enum (e ^ n); return e; } \
  constexpr Enum&   operator|= (Enum &e, int64_t n) { e = Enum (e | n); return e; } \
  constexpr Enum&   operator&= (Enum &e, int64_t n) { e = Enum (e & n); return e; } \
  constexpr Enum&   operator+= (Enum &e, int64_t n) { e = Enum (e + n); return e; } \
  constexpr Enum&   operator-= (Enum &e, int64_t n) { e = Enum (e - n); return e; } \
  constexpr Enum&   operator*= (Enum &e, int64_t n) { e = Enum (e * n); return e; } \
  constexpr Enum&   operator/= (Enum &e, int64_t n) { e = Enum (e / n); return e; } \
  constexpr Enum&   operator%= (Enum &e, int64_t n) { e = Enum (e % n); return e; } \
  AIDA_DEFINE_ENUM_EQUALITY (Enum)

// == Type Imports ==
using std::vector;
typedef int8_t              int8;
typedef uint8_t             uint8;
typedef int16_t             int16;
typedef uint16_t            uint16;
typedef int32_t             int32;
typedef uint32_t            uint32;
typedef int64_t             int64;
typedef uint64_t            uint64;
typedef std::string         String;
typedef std::vector<String> StringVector;

// == Forward Declarations ==
class Any;
class Event;
class ImplicitBase;
class CallableIface;
class SharedFromThisIface;
struct PropertyAccessor;
typedef std::shared_ptr<SharedFromThisIface> SharedFromThisP;
typedef std::shared_ptr<CallableIface> CallableIfaceP;
typedef std::shared_ptr<ImplicitBase> ImplicitBaseP;
typedef std::function<void (const Event&)> EventHandlerF;

// == C++ Traits ==
/// Template to map all type arguments to void, useful for SFINAE, see also WG21 N3911.
template<class...> using void_t = void;

/// REQUIRES<value> - Simplified version of std::enable_if<> to use SFINAE in function templates.
template<bool value> using REQUIRES = typename ::std::enable_if<value, bool>::type;

/// IsBool<T> - Check if @a T is of type 'bool'.
template<class T> using IsBool = ::std::is_same<bool, typename ::std::remove_cv<T>::type>;

/// IsComparable<T> - Check if type @a T is comparable for equality.
/// If @a T is a type that can be compared with operator==, provide the member constant @a value equal true, otherwise false.
template<class, class = void> struct IsComparable : std::false_type {}; // IsComparable false case, picked if operator== is missing.
template<class T> struct IsComparable<T, void_t< decltype (std::declval<T>() == std::declval<T>()) >> : std::true_type {};

/// IsInteger<T> - Check if @a T is of integral type (except bool).
template<class T> using IsInteger = ::std::integral_constant<bool, !IsBool<T>::value && ::std::is_integral<T>::value>;

/// DerivesString<T> - Check if @a T is of type 'std::string'.
template<class T> using DerivesString = typename std::is_base_of<::std::string, T>;

/// DerivesSharedPtr<T> - Check if @a T derives from std::shared_ptr<>.
template<class T, typename = void> struct DerivesSharedPtr : std::false_type {};
template<class T> struct DerivesSharedPtr<T, void_t< typename T::element_type > > :
std::is_base_of< std::shared_ptr<typename T::element_type>, T > {};

/// DerivesVector<T> - Check if @a T derives from std::vector<>.
template<class T, typename = void> struct DerivesVector : std::false_type {};
// use void_t to prevent errors for T without vector's typedefs
template<class T> struct DerivesVector<T, void_t< typename T::value_type, typename T::allocator_type > > :
    std::is_base_of< std::vector<typename T::value_type, typename T::allocator_type>, T > {};

/// IsImplicitBaseDerived<T> - Check if @a T derives from Aida::ImplicitBase.
template<class T> using IsImplicitBaseDerived = ::std::integral_constant<bool, ::std::is_base_of<ImplicitBase, T>::value>;

/// Helper structure to implement Has___visit__
struct HasHelper : std::true_type {
  static constexpr auto visitor_lambda = [] (auto, const char*) {};
};
/// Has___visit__<T> - Check if @a T provides a @a __visit__(Visitor) method template.
template<class, class = void> struct
Has___visit__ : std::false_type {};
template<class T> struct
Has___visit__<T, void_t< decltype (std::declval<T&>().__visit__ (HasHelper::visitor_lambda)) > > : std::true_type {};

/// Has___typedata__<T> - Check if `T` provides a `__typedata__()` method.
template<class, class = void> struct
Has___typedata__ : std::false_type {};
template<class T> struct
Has___typedata__<T, void_t< decltype (std::declval<T&>().__typedata__()) > > : std::true_type {};

/// Retrieve `__typedata__()` result from an instance or an empty fallback.
template<typename T> const StringVector&
typedata_from_type (const T &o)
{
  if constexpr (Has___typedata__<T>::value)
    return o.__typedata__();
  static const StringVector fallback;
  return fallback;
}

/// Provide the member typedef type which is the element_type of the shared_ptr type @a T.
template<typename T> struct RemoveSharedPtr                                             { typedef T type; };
template<typename T> struct RemoveSharedPtr<::std::shared_ptr<T>>                       { typedef T type; };
template<typename T> struct RemoveSharedPtr<const ::std::shared_ptr<T>>                 { typedef T type; };
template<typename T> struct RemoveSharedPtr<volatile ::std::shared_ptr<T>>              { typedef T type; };
template<typename T> struct RemoveSharedPtr<const volatile ::std::shared_ptr<T>>        { typedef T type; };

// == String Utilitiies ==
bool         string_match_identifier_tail (const String &ident, const String &tail);
bool         string_startswith            (const String &string, const String &fragment);
String       string_to_cquote             (const String &str);
bool         string_to_bool               (const String &string, bool fallback = false);
int64        string_to_int                (const String &string, size_t *consumed = NULL, uint base = 10);
uint64       string_to_uint               (const String &string, size_t *consumed = NULL, uint base = 10);
String       string_from_double           (double value);
String       string_join                  (const String &junctor, const StringVector &strvec);
StringVector string_split_any             (const String &string, const String &splitchars = "", size_t maxn = size_t (-1));
bool         string_option_check          (const String &option_string, const String &option);
String       string_demangle_cxx          (const char *mangled_identifier);
void         assertion_failed             (const char *file, int line, const char *func, const char *stmt);

/// Provide demangled stringified name for a @a Type.
template<class T> AIDA_PURE static inline String
typeid_name()
{
  return string_demangle_cxx (typeid (T).name());
}

/** Simple, very fast and well known hash function as constexpr with good dispersion.
 * This is the 64bit version of the well known
 * [FNV-1a](https://en.wikipedia.org/wiki/Fowler-Noll-Vo_hash_function)
 * hash function, implemented as a C++11 constexpr for a byte range.
 */
template<class Num> static inline constexpr uint64_t
fnv1a_bytehash64 (const Num *const ztdata, const Num *const ztend, uint64_t hash = 0xcbf29ce484222325)
{
  static_assert (sizeof (Num) == 1, "");
  return AIDA_LIKELY (ztdata < ztend) ? fnv1a_bytehash64 (ztdata + 1, ztend, 0x100000001b3 * (hash ^ uint8_t (ztdata[0]))) : hash;
}
template<class Num> static inline constexpr uint64_t
fnv1a_bytehash64 (const Num *const ztdata, size_t n)
{
  static_assert (sizeof (Num) == 1, "");
  return fnv1a_bytehash64 (ztdata, ztdata + n);
}

// == SharedFromThis ==
/// Helper class for SharedFromThis.
class SharedFromThisIface {
protected:
  virtual SharedFromThisP __shared_from_this__ () = 0;
};

/// Virtual base class template that provides shared_from_this() for multiple inheritance.
template<class T>
class SharedFromThis : public virtual SharedFromThisIface {
public:
  /// Retrieve std::shared_ptr<>() from this.
  std::shared_ptr<T>       shared_from_this()       { return std::dynamic_pointer_cast<T> (__shared_from_this__()); }
  /// Retrieve std::shared_ptr<const>() from const this.
  std::shared_ptr<const T> shared_from_this() const { return std::dynamic_pointer_cast<T> (const_cast<SharedFromThis*> (this)->__shared_from_this__()); }
};

/// A (virtual) base class template that provides shared_from_this() for multiple inheritance.
template<class T>
class EnableSharedFromThis :
  public virtual SharedFromThis<T>,
  public virtual std::enable_shared_from_this<SharedFromThisIface>
{
  virtual SharedFromThisP
  __shared_from_this__ () override
  {
    return this->std::enable_shared_from_this<SharedFromThisIface>::shared_from_this();
  }
protected:
  virtual ~EnableSharedFromThis ()                              {}
  /*ctor*/ EnableSharedFromThis () = default;
  /*copy*/ EnableSharedFromThis (const EnableSharedFromThis&) = default;
public:
  using SharedFromThis<T>::shared_from_this;
};

// == LongIffy and ULongIffy ==
///@{
/** LongIffy, ULongIffy, CastIffy, UCastIffy - types for 32bit/64bit overloading.
 * On 64bit, int64_t is aliased to "long int" which is 64 bit wide.
 * On 32bit, int64_t is aliased to "long long int", which is 64 bit wide (and long is 32bit wide).
 * For int-type function overloading, this means that int32, int64 and either "long" or "long long"
 * need to be overloaded, depending on platform. To aid this case, LongIffy and ULongIffy are defined
 * to signed and unsigned "long" (for 32bit) and "long long" (for 64bit). Correspondingly, CastIffy
 * and UCastIffy are defined to signed and unsigned int32 (for 32bit) or int64 (for 64bit), so
 * LongIffy can be cast losslessly into a known type.
 */
#if     __SIZEOF_LONG__ == 8    // 64bit
typedef long long signed int    LongIffy;
typedef long long unsigned int  ULongIffy;
typedef int64_t                 CastIffy;
typedef uint64_t                UCastIffy;
static_assert (__SIZEOF_LONG_LONG__ == 8, "__SIZEOF_LONG_LONG__");
static_assert (__SIZEOF_INT__ == 4, "__SIZEOF_INT__");
#elif   __SIZEOF_LONG__ == 4    // 32bit
typedef long signed int         LongIffy;
typedef long unsigned int       ULongIffy;
typedef int32_t                 CastIffy;
typedef uint32_t                UCastIffy;
static_assert (__SIZEOF_LONG_LONG__ == 8, "__SIZEOF_LONG_LONG__");
static_assert (__SIZEOF_INT__ == 4, "__SIZEOF_INT__");
#else
#error  "Unknown long size:" __SIZEOF_LONG__
#endif
static_assert (sizeof (CastIffy) == sizeof (LongIffy), "CastIffy == LongIffy");
static_assert (sizeof (UCastIffy) == sizeof (ULongIffy), "UCastIffy == ULongIffy");
///@}

// == EventDispatcher ==
class EventDispatcher {
  struct DispatcherImpl;
  DispatcherImpl *o_ = NULL;
  struct ConnectionImpl;
public:
  struct EventConnection : private std::weak_ptr<EventDispatcher::ConnectionImpl> {
    friend              class EventDispatcher;
    bool                connected       () const;
    void                disconnect      () const;
  };
  /*dtor*/             ~EventDispatcher ();
  void                  reset           ();
  void                  emit            (const Event &event);
  EventConnection       attach          (const String &eventselector, EventHandlerF handler);
};
using IfaceEventConnection = EventDispatcher::EventConnection;

// == CallableIface ==
class CallableIface : public virtual SharedFromThis<CallableIface> {
public:
  /// Attach an Event handler, returns an event connection handle that can be used for disconnection.
  virtual IfaceEventConnection __attach__               (const String &eventselector, EventHandlerF handler) = 0;
  /// Retrieve the IDL type names of an instance, save to be called multi-threaded.
  virtual StringVector         __typelist_mt__          () const = 0;
  /// Retrieve the IDL type name of an instance.
  std::string                  __typename__             () const;
  /// Retrieve the IDL type names of an instance, wrapper for __typelist_mt__().
  StringVector                 __typelist__             () const        { return __typelist_mt__(); }
};

// == IntrospectionRegistry ==
class Introspection {
public:
  static const StringVector&     find_type              (const std::string &abstypename, std::string *kind = NULL);
  static std::string             find_type_kind         (const std::string &abstypename);
  static std::string             find_value             (const std::string &key, const StringVector &kvlist, const std::string &fallback = "");
  static StringVector            find_nested            (const std::string &field, const StringVector &kvlist);
  static std::string             strip_type_prefix      (const std::string &dottedstring);
  // enum helpers
  static StringVector            list_enumerators       (const std::string &enum_typename);
  static std::string             match_enumerator       (const std::string &partialenumerator, const std::string &enum_context);
  static StringVector            find_enumerator        (const std::string &enumerator);
  static int64_t                 enumerator_to_value    (const std::string &enumerator, const std::string &enum_context = "");
  static std::string             enumerator_from_value  (const std::string &enumtypename, int64_t value);
  static const char*             legacy_enumerator      (const std::string &enumtypename, int64_t value);
};

/// Return the numeric value of `valuestring` in `enum_typename`.
/// See also the underlying Introspection::enumerator_to_value().
template<typename EnumType> EnumType
enum_value_from_string (const String &valuestring)
{ return EnumType (Introspection::enumerator_to_value (valuestring, typeid_name<EnumType>())); }

/// Return fully qualified enumerator for `evalue`.
/// See also the underlying Introspection::enumerator_from_value().
template<typename EnumType> String
enum_value_to_string (EnumType evalue)
{ return Introspection::enumerator_from_value (typeid_name<EnumType>(), int64_t (evalue)); }

/// Retrieve the enumerator without type name from enum_value_to_string<>().
template<typename EnumType> String
enum_value_to_short_string (EnumType evalue)
{ return Introspection::strip_type_prefix (enum_value_to_string<EnumType> (evalue)); }

// == IntrospectionRegistry ==
class IntrospectionRegistry {
  static void register_aux_data (const char *auxentry, size_t length);
public:
  template<size_t I>             IntrospectionRegistry  (const char (&auxentry) [I])
  {
    static_assert (I >= 1, "");
    register_aux_data (auxentry, I);
  }
};

/// Split @a char_array at '\\0' and construct vector.
std::vector<String> aux_vector_split    (const char *char_array, size_t length); // Splits @a char_array at '\\0'
/// Split @a auxinfo00, assert key=value contents and '\\0\\0' ending.
std::vector<String> aux_vector_split    (const char *auxinfo00);
/// Merge string vectors @a v0 .. @a vf.
std::vector<String> aux_vectors_combine (const std::vector<String> &v0 = std::vector<String>(),
                                         const std::vector<String> &v1 = std::vector<String>(),
                                         const std::vector<String> &v2 = std::vector<String>(),
                                         const std::vector<String> &v3 = std::vector<String>(),
                                         const std::vector<String> &v4 = std::vector<String>(),
                                         const std::vector<String> &v5 = std::vector<String>(),
                                         const std::vector<String> &v6 = std::vector<String>(),
                                         const std::vector<String> &v7 = std::vector<String>(),
                                         const std::vector<String> &v8 = std::vector<String>(),
                                         const std::vector<String> &v9 = std::vector<String>(),
                                         const std::vector<String> &va = std::vector<String>(),
                                         const std::vector<String> &vb = std::vector<String>(),
                                         const std::vector<String> &vc = std::vector<String>(),
                                         const std::vector<String> &vd = std::vector<String>(),
                                         const std::vector<String> &ve = std::vector<String>(),
                                         const std::vector<String> &vf = std::vector<String>());
/// Retrive the value of @a field.key from @a auxvector, or @a fallback if none is found.
String aux_vector_find (const std::vector<String> &auxvector, const String &field, const String &key, const String &fallback = "");
/// Split @a options at ':' and check that each option is present (enabled) in @a field.key from @a auxvector.
bool   aux_vector_check_options (const std::vector<String> &auxvector, const String &field, const String &key, const String &options);

// == TypeKind ==
/// Classification enum for the underlying type.
enum TypeKind {
  UNTYPED        = 0,   ///< Type indicator for unused Any instances.
  VOID           = 'v', ///< 'void' type.
  BOOL           = 'b', ///< Boolean type.
  INT32          = 'i', ///< Signed numeric type for 32bit.
  INT64          = 'l', ///< Signed numeric type for 64bit.
  FLOAT64        = 'd', ///< Floating point type of IEEE-754 Double precision.
  STRING         = 's', ///< String type for character sequence in UTF-8 encoding.
  ENUM           = 'E', ///< Enumeration type to represent choices.
  SEQUENCE       = 'Q', ///< Type to form sequences of an other type.
  RECORD         = 'R', ///< Record type containing named fields.
  INSTANCE       = 'C', ///< ImplicitBase type.
  ANY            = 'Y', ///< Generic type to hold any other type.
};

const char* type_kind_name (TypeKind type_kind); ///< Obtain TypeKind names as a string.

// == EventFd ==
/// Wakeup facility for IPC.
class EventFd
{
  int      fds[2];
  void     operator= (const EventFd&) = delete; // no assignments
  explicit EventFd   (const EventFd&) = delete; // no copying
public:
  explicit EventFd   ();
  int      open      (); ///< Opens the eventfd and returns -errno.
  bool     opened    (); ///< Indicates whether eventfd has been opened.
  void     wakeup    (); ///< Wakeup polling end.
  int      inputfd   (); ///< Returns the file descriptor for POLLIN.
  bool     pollin    (); ///< Checks whether events are pending.
  void     flush     (); ///< Clear pending wakeups.
  /*Des*/ ~EventFd   ();
};

// == ScopedSemaphore ==
class ScopedSemaphore {
  unsigned long mem_[4];
  /*copy*/         ScopedSemaphore (const ScopedSemaphore&) = delete;
  ScopedSemaphore& operator=       (const ScopedSemaphore&) = delete;
public:
  explicit  ScopedSemaphore () noexcept;  ///< Create a process-local semaphore.
  int       post            () noexcept;  ///< Unlock ScopedSemaphore.
  int       wait            () noexcept;  ///< Unlock ScopedSemaphore.
  /*dtor*/ ~ScopedSemaphore () noexcept;
};

// == Type Utilities ==
template<class Y> struct ValueType           { typedef Y T; };
template<class Y> struct ValueType<Y&>       { typedef Y T; };
template<class Y> struct ValueType<const Y&> { typedef Y T; };

// == Any Type ==
class Any /// Generic value type that can hold values of all other types.
{
  ///@cond
  template<class Any> struct AnyField : Any { // We must wrap Any::Field into a template, because "Any" is not yet fully defined.
    std::string name;
    AnyField () = default;
    template<class V> inline
    AnyField (const std::string &_name, V &&value) : Any (::std::forward<V> (value)), name (_name) {}
  };
  ///@endcond
public:
#ifndef DOXYGEN
  typedef AnyField<Any> Field;  // See DOXYGEN section for the "unwrapped" definition.
#else // DOXYGEN
  struct Field : Any    /// Any::Field is an Any with a std::string @a name attached.
  {
    String name;        ///< The @a name of this Any::Field, as used in e.g. #RECORD types.
    Field();            ///< Default initialize Any::Field.
    template<class V>
    Field (const String &name, V &&value); ///< Initialize Any::Field with a @a name and an Any initialization @a value.
  };
#endif // DOXYGEN
  typedef std::vector<Any> AnySeq; ///< Vector of Any structures for use in #SEQUENCE types.
  /// Vector of fields (named Any structures) for use in #RECORD types.
  class AnyRec : public std::vector<Field> {
    typedef std::vector<Field> FieldVector;
  public:
    void         add        (const String &name, const Any &value)      { (*this)[name] = value; }
    Any&         operator[] (const String &name);
    const Any&   operator[] (const String &name) const;
    using FieldVector::operator[]; // still allow: Field& operator[] (size_t __n);
  };
protected:
  template<class Rec> static void any_from_record (Any &any, const Rec &record);
private:
  TypeKind type_kind_;
  ///@cond
  union {
    uint64 vuint64; int64 vint64; double vdouble; Any *vany;
    int64 dummy_[AIDA_I64ELEMENTS (std::max (std::max (sizeof (String), sizeof (std::vector<void*>)), sizeof (ImplicitBaseP)))];
    AnyRec&              vfields () { return *(AnyRec*) this; static_assert (sizeof (AnyRec) <= sizeof (*this), ""); }
    const AnyRec&        vfields () const { return *(const AnyRec*) this; }
    AnySeq&              vanys   () { return *(AnySeq*) this; static_assert (sizeof (AnySeq) <= sizeof (*this), ""); }
    const AnySeq&        vanys   () const { return *(const AnySeq*) this; }
    String&              vstring () { return *(String*) this; static_assert (sizeof (String) <= sizeof (*this), ""); }
    const String&        vstring () const { return *(const String*) this; }
    ImplicitBaseP&       ibasep  () const { return *(ImplicitBaseP*) this; static_assert (sizeof (ImplicitBaseP) <= sizeof (*this), ""); }
  } u_;
  ///@endcond
  void    ensure  (TypeKind _kind) { if (AIDA_LIKELY (kind() == _kind)) return; rekind (_kind); }
  void    rekind  (TypeKind _kind);
public:
  /*dtor*/ ~Any    ();                                  ///< Any destructor.
  /*ctor*/  Any    ();                                  ///< Default initialize Any with no type.
  /// Initialize Any from a @a anany which is of Any or derived type.
  template<class V, REQUIRES< ::std::is_base_of< Any, typename std::remove_reference<V>::type >::value > = true> inline
  explicit  Any (V &&anany) : Any()     { this->operator= (::std::forward<V> (anany)); }
  /// Initialize Any and set its contents from @a value.
  template<class V, REQUIRES< !::std::is_base_of< Any, typename std::remove_reference<V>::type >::value > = true> inline
  explicit  Any    (V &&value) : Any()  { set (::std::forward<V> (value)); }
  /*copy*/  Any    (const Any &clone);                  ///< Copy constructor.
  /*move*/  Any    (Any &&other);                       ///< Move constructor.
  Any& operator=   (const Any &clone);                  ///< Set @a this Any to a copy of @a clone.
  Any& operator=   (Any &&other);                       ///< Move @a other into @a this Any.
  bool operator==  (const Any &clone) const;            ///< Check if Any is exactly equal to @a clone.
  bool operator!=  (const Any &clone) const;            ///< Check if Any is not equal to @a clone, see operator==().
  TypeKind  kind   () const { return type_kind_; }      ///< Obtain the type kind for the contents of this Any.
  void      swap   (Any           &other);              ///< Swap the contents of @a this and @a other in constant time.
  void      clear  ();                                  ///< Erase Any contents, making it empty like a newly constructed Any().
  bool      empty  () const;                            ///< Returns true if Any is newly constructed or after clear().
  void      set_enum (const String &enumerator);        ///< Set Any to hold an enumerator value.
  template<class T> using ToAnyRecConvertible =         ///< Check is_convertible<a T, AnyRec>.
    ::std::integral_constant<bool, ::std::is_convertible<T, AnyRec>::value && !::std::is_base_of<Any, T>::value>;
  template<class T> using ToAnySeqConvertible =        ///< Check is_convertible<a T, AnySeq > but give precedence to AnyRec.
    ::std::integral_constant<bool, ::std::is_convertible<T, AnySeq>::value && !(::std::is_convertible<T, AnyRec>::value ||
                                                                                 ::std::is_base_of<Any, T>::value)>;
private:
  template<class A, class B> using IsDecayedSame =
    ::std::integral_constant<bool, ::std::is_same<typename ::std::decay<A>::type, typename ::std::decay<B>::type>::value>;
  template<class A, class B> using IsConvertible = ///< Avoid pointer->bool reduction for std::is_convertible<>.
    ::std::integral_constant<bool, ::std::is_convertible<A, B>::value && (!::std::is_pointer<A>::value || !IsBool<B>::value)>;
  template<class A, class B> using IsJustConvertible = ///< Avoid std::is_same<> ambiguity.
    ::std::integral_constant<bool, IsConvertible<A, B>::value && !IsDecayedSame<A, B>::value>;
  template<class T>          using IsConstCharPtr        = ::std::is_same<const char*, typename ::std::decay<T>::type>;
  template<class T>          using IsImplicitBaseDerivedP =
    ::std::integral_constant<bool, (DerivesSharedPtr<T>::value && // check without SFINAE error on missing T::element_type
                                    ::std::is_base_of<ImplicitBase, typename RemoveSharedPtr<T>::type >::value)>;
  template<class T> using ToAnyConvertible =            ///< Check is_convertible<a T, Any > but give precedence to AnySeq / AnyRec.
    ::std::integral_constant<bool, ::std::is_base_of<Any, T>::value || (::std::is_convertible<T, Any>::value &&
                                                                        !::std::is_convertible<T, AnySeq>::value &&
                                                                        !::std::is_convertible<T, AnyRec>::value)>;
  bool               get_bool    () const;
  void               set_bool    (bool value);
  void               set_int64   (int64 value);
  void               set_double  (double value);
  std::string        get_string  () const;
  void               set_string  (const std::string &value);
  std::string        get_enum    () const;
  const AnySeq&      get_seq     () const;
  void               set_seq     (const AnySeq &seq);
  const AnyRec&      get_rec     () const;
  void               set_rec     (const AnyRec &rec);
  void               set_ibasep  (ImplicitBaseP ibaseptr);
  ImplicitBaseP      get_ibasep  () const;
  template<typename C>
  C*                 cast_ibase  () const               { return dynamic_cast<C*> (get_ibasep().get()); }
  template<typename SP>
  SP                 cast_ibasep () const               { return std::dynamic_pointer_cast<typename SP::element_type> (get_ibasep()); }
  const Any*         get_any     () const;
  void               set_any     (const Any *value);
public:
  // Type get() const;
  template<typename T, REQUIRES< IsBool<T>::value > = true>                            T    get () const { return get_bool(); }
  template<typename T, REQUIRES< IsInteger<T>::value > = true>                         T    get () const { return as_int64(); }
  template<typename T, REQUIRES< std::is_floating_point<T>::value > = true>            T    get () const { return as_double(); }
  template<typename T, REQUIRES< DerivesString<T>::value > = true>                     T    get () const { return get_string(); }
  template<typename T, REQUIRES< std::is_enum<T>::value > = true>                      T    get () const { return enum_value_from_string<T> (get_enum()); }
  template<TypeKind V, REQUIRES< V == Aida::ENUM > = true>                   std::string    get () const { return get_enum(); }
  template<typename T, REQUIRES< std::is_same<const AnySeq*, T>::value > = true>       T    get () const { return &get_seq(); }
  template<typename T, REQUIRES< IsDecayedSame<const AnySeq&, T>::value > = true>      T    get () const { return get_seq(); }
  template<typename T, REQUIRES< IsJustConvertible<const AnySeq, T>::value > = true>   T    get () const { return get_seq(); }
  template<typename T, REQUIRES< std::is_same<const AnyRec*, T>::value > = true>       T    get () const { return &get_rec(); }
  template<typename T, REQUIRES< IsDecayedSame<const AnyRec&, T>::value > = true>      T    get () const { return get_rec(); }
  template<typename T, REQUIRES< IsJustConvertible<const AnyRec, T>::value > = true>   T    get () const { return get_rec(); }
  template<typename T, REQUIRES< IsImplicitBaseDerived<T>::value > = true>             T&   get () const { return *cast_ibase<T>(); }
  template<typename T, REQUIRES< IsImplicitBaseDerivedP<T>::value > = true>            T    get () const { return cast_ibasep<T>(); }
  template<typename T, REQUIRES< std::is_base_of<const Any, T>::value > = true>        T    get () const { return *get_any(); }
  // void set (const Type&);
  template<typename T, REQUIRES< IsBool<T>::value > = true>                            void set (T v) { return set_bool (v); }
  template<typename T, REQUIRES< IsInteger<T>::value > = true>                         void set (T v) { return set_int64 (v); }
  template<typename T, REQUIRES< std::is_floating_point<T>::value > = true>            void set (T v) { return set_double (v); }
  template<typename T, REQUIRES< DerivesString<T>::value > = true>                     void set (T v) { return set_string (v); }
  template<typename T, REQUIRES< IsConstCharPtr<T>::value > = true>                    void set (T v) { return set_string (v); }
  template<typename T, REQUIRES< std::is_enum<T>::value > = true>                      void set (T v) { return set_enum (enum_value_to_string<T> (v)); }
  template<TypeKind V, REQUIRES< V == Aida::ENUM > = true>                             void set (const std::string &enumerator) { set_enum (enumerator); }
  template<typename T, REQUIRES< ToAnySeqConvertible<T>::value > = true>               void set (const T &v) { return set_seq (v); }
  template<typename T, REQUIRES< ToAnySeqConvertible<T>::value > = true>               void set (const T *v) { return set_seq (*v); }
  template<typename T, REQUIRES< ToAnyRecConvertible<T>::value > = true>               void set (const T &v) { return set_rec (v); }
  template<typename T, REQUIRES< ToAnyRecConvertible<T>::value > = true>               void set (const T *v) { return set_rec (*v); }
  template<typename T, REQUIRES< IsImplicitBaseDerived<T>::value > = true>             void set (T &v) { set_ibasep (v.shared_from_this()); }
  template<typename T, REQUIRES< IsImplicitBaseDerivedP<T>::value > = true>            void set (T v) { set_ibasep (v); }
  template<typename T, REQUIRES< ToAnyConvertible<T>::value > = true>                  void set (const T &v) { return set_any (&v); }
  // convenience
  static Any          any_from_strings (const std::vector<std::string> &string_container);
  std::vector<String> any_to_strings   () const;
  String              repr             (const String &field_name = "") const;
  String              to_string        () const; ///< Retrieve string representation of Any for printouts.
  int64               as_int64         () const; ///< Obtain contents as int64.
  double              as_double        () const; ///< Obtain contents as double.
  const Any&          as_any           () const { return kind() == ANY ? *u_.vany : *this; } ///< Obtain contents as Any.
  template<class V, REQUIRES< !::std::is_base_of<Any, V>::value > = true> Any&
  operator= (const V &v)
  {
    set (v);
    return *this;
  }
  template<class R, REQUIRES< !::std::is_base_of<Any, R>::value > = true>
  operator R () const
  {
    return get<R>();
  }
};
typedef Any::AnyRec AnyRec;
typedef Any::AnySeq AnySeq;
template<class T> using ToAnyRecConvertible = Any::ToAnyRecConvertible<T>;
template<class T> using ToAnySeqConvertible = Any::ToAnySeqConvertible<T>;

// == Event ==
class Event : public virtual EnableSharedFromThis<Event> {
  AnyRec  fields_;
public:
  explicit       Event      (const String &type);
  explicit       Event      (const AnyRec &arec);
  Any&           operator[] (const String &name)          { return fields_[name]; }
  const Any&     operator[] (const String &name) const    { return fields_[name]; }
  const AnyRec&  fields     () const                      { return fields_; }
};

// == KeyValue ==
struct KeyValue {
  std::string key;
  Any         value;
};
namespace KeyValueArgs {
struct KeyValueConstructor {
  std::string key;
  template<typename T> inline KeyValue operator= (const T &value) { Any v; v.set (value); return KeyValue { key, v }; }
};
inline KeyValueConstructor operator""_v (const char *key, size_t) { return KeyValueConstructor { key }; }
};

// == ImplicitBase ==
/// Abstract base interface that all IDL interfaces are implicitely derived from.
class ImplicitBase : public virtual CallableIface, public virtual SharedFromThis<ImplicitBase> {
protected:
  virtual                    ~ImplicitBase        () = 0; // abstract class
public:
  using PropertyAccessorPred = std::function<bool (const PropertyAccessor&)>;
  virtual bool                __access__          (const std::string &propertyname, const PropertyAccessorPred&) = 0;
  uint64                      __event_attach__    (const String &type, EventHandlerF handler);          //: AIDAID
  bool                        __event_detach__    (int64 connection_id);                                //: AIDAID __event_detachid__
  void                        __event_emit__      (const Event &event);                                 //: AIDAID __event_callback__
  using SharedFromThis<ImplicitBase>::shared_from_this;
};

// == inline implementations ==
inline
Any::Any() :
  type_kind_ (UNTYPED), u_ {0}
{}

inline
Any::~Any ()
{
  clear();
}

// == PropertyAccessor ==
struct PropertyAccessor {
  using Setter = std::function<void(const Any&)>;
  using Getter = std::function<Any()>;
  virtual              ~PropertyAccessor();
  virtual TypeKind      kind     () const = 0;
  virtual std::string   name     () const = 0;
  virtual StringVector  typedata () const = 0;
  virtual Any           get      () const = 0;
  virtual void          set      (const Any&) const = 0;
};
template<class T, class V, typename = std::true_type> struct PropertyAccessorImpl;
// bool property
template<class T>
struct PropertyAccessorImpl<T,bool> : PropertyAccessor {
  PropertyAccessorImpl (const char *name, T &object, bool (T::*getter) () const, void (T::*setter) (bool), const char *auxvalues00) :
    object_ (object), name_ (name), auxvalues00_ (auxvalues00), getter_ (getter), setter_ (setter)
  {}
  virtual TypeKind     kind     () const        { return BOOL; }
  virtual std::string  name     () const        { return name_; }
  virtual StringVector typedata () const        { return aux_vector_split (auxvalues00_); }
  virtual Any          get      () const        { Any a; a.set<bool> ((object_.*getter_)()); return a; }
  virtual void         set (const Any &a) const { (object_.*setter_) (a.get<bool>()); }
private:
  T          &object_;
  const char *name_ = NULL, *auxvalues00_ = NULL;
  bool   (T::*getter_) () const = NULL;
  void   (T::*setter_) (bool) = NULL;
};
// int32 property
template<class T>
struct PropertyAccessorImpl<T,int> : PropertyAccessor {
  PropertyAccessorImpl (const char *name, T &object, int (T::*getter) () const, void (T::*setter) (int), const char *auxvalues00) :
    object_ (object), name_ (name), auxvalues00_ (auxvalues00), getter_ (getter), setter_ (setter)
  {}
  virtual TypeKind     kind     () const        { return INT32; }
  virtual std::string  name     () const        { return name_; }
  virtual StringVector typedata () const        { return aux_vector_split (auxvalues00_); }
  virtual Any          get      () const        { Any a; a.set<int> ((object_.*getter_)()); return a; }
  virtual void         set (const Any &a) const { (object_.*setter_) (a.get<int>()); }
private:
  T          &object_;
  const char *name_ = NULL, *auxvalues00_ = NULL;
  int    (T::*getter_) () const = NULL;
  void   (T::*setter_) (int) = NULL;
};
// int64 property
template<class T>
struct PropertyAccessorImpl<T,int64> : PropertyAccessor {
  PropertyAccessorImpl (const char *name, T &object, int64 (T::*getter) () const, void (T::*setter) (int64), const char *auxvalues00) :
    object_ (object), name_ (name), auxvalues00_ (auxvalues00), getter_ (getter), setter_ (setter)
  {}
  virtual TypeKind     kind     () const        { return INT64; }
  virtual std::string  name     () const        { return name_; }
  virtual StringVector typedata () const        { return aux_vector_split (auxvalues00_); }
  virtual Any          get      () const        { Any a; a.set<int64> ((object_.*getter_)()); return a; }
  virtual void         set (const Any &a) const { (object_.*setter_) (a.get<int64>()); }
private:
  T          &object_;
  const char *name_ = NULL, *auxvalues00_ = NULL;
  int64  (T::*getter_) () const = NULL;
  void   (T::*setter_) (int64) = NULL;
};
// float64 property
template<class T>
struct PropertyAccessorImpl<T,double> : PropertyAccessor {
  PropertyAccessorImpl (const char *name, T &object, double (T::*getter) () const, void (T::*setter) (double), const char *auxvalues00) :
    object_ (object), name_ (name), auxvalues00_ (auxvalues00), getter_ (getter), setter_ (setter)
  {}
  virtual TypeKind     kind     () const        { return FLOAT64; }
  virtual std::string  name     () const        { return name_; }
  virtual StringVector typedata () const        { return aux_vector_split (auxvalues00_); }
  virtual Any          get      () const        { Any a; a.set<double> ((object_.*getter_)()); return a; }
  virtual void         set (const Any &a) const { (object_.*setter_) (a.get<double>()); }
private:
  T          &object_;
  const char *name_ = NULL, *auxvalues00_ = NULL;
  double (T::*getter_) () const = NULL;
  void   (T::*setter_) (double) = NULL;
};
// string property
template<class T>
struct PropertyAccessorImpl<T,std::string> : PropertyAccessor {
  PropertyAccessorImpl (const char *name, T &object, std::string (T::*getter) () const, void (T::*setter) (const std::string&), const char *auxvalues00) :
    object_ (object), name_ (name), auxvalues00_ (auxvalues00), getter_ (getter), setter_ (setter)
  {}
  virtual TypeKind     kind     () const        { return STRING; }
  virtual std::string  name     () const        { return name_; }
  virtual StringVector typedata () const        { return aux_vector_split (auxvalues00_); }
  virtual Any          get      () const        { Any a; a.set<std::string> ((object_.*getter_)()); return a; }
  virtual void         set (const Any &a) const { (object_.*setter_) (a.get<std::string>()); }
private:
  T          &object_;
  const char *name_ = NULL, *auxvalues00_ = NULL;
  std::string (T::*getter_) () const = NULL;
  void   (T::*setter_) (const std::string&) = NULL;
};
// enum property
template<class T, class E>
struct PropertyAccessorImpl<T,E,typename std::is_enum<E>::type> : PropertyAccessor {
  PropertyAccessorImpl (const char *name, T &object, E (T::*getter) () const, void (T::*setter) (E), const char *auxvalues00) :
    object_ (object), name_ (name), auxvalues00_ (auxvalues00), getter_ (getter), setter_ (setter)
  {}
  virtual TypeKind     kind     () const        { return ENUM; }
  virtual std::string  name     () const        { return name_; }
  virtual StringVector typedata () const        { return aux_vector_split (auxvalues00_); }
  virtual Any          get      () const        { Any a; a.set<E> ((object_.*getter_)()); return a; }
  virtual void         set (const Any &a) const { (object_.*setter_) (a.get<E>()); }
private:
  T          &object_;
  const char *name_ = NULL, *auxvalues00_ = NULL;
  E      (T::*getter_) () const = NULL;
  void   (T::*setter_) (E) = NULL;
};
// Any property
template<class T, class A>
struct PropertyAccessorImpl<T,A,typename std::is_base_of<Any,A>::type> : PropertyAccessor {
  PropertyAccessorImpl (const char *name, T &object, A (T::*getter) () const, void (T::*setter) (const A&), const char *auxvalues00) :
    object_ (object), name_ (name), auxvalues00_ (auxvalues00), getter_ (getter), setter_ (setter)
  {}
  virtual TypeKind     kind     () const        { return ANY; }
  virtual std::string  name     () const        { return name_; }
  virtual StringVector typedata () const        { return aux_vector_split (auxvalues00_); }
  virtual Any          get      () const        { Any a; a.set<A> ((object_.*getter_)()); return a; }
  virtual void         set (const Any &a) const { (object_.*setter_) (a.get<A>()); }
private:
  T          &object_;
  const char *name_ = NULL, *auxvalues00_ = NULL;
  A      (T::*getter_) () const = NULL;
  void   (T::*setter_) (const A&) = NULL;
};
// Record property
template<class T, class Rec>
struct PropertyAccessorImpl<T,Rec,typename ToAnyRecConvertible<Rec>::type> : PropertyAccessor {
  PropertyAccessorImpl (const char *name, T &object, Rec (T::*getter) () const, void (T::*setter) (const Rec&), const char *auxvalues00) :
    object_ (object), name_ (name), auxvalues00_ (auxvalues00), getter_ (getter), setter_ (setter)
  {}
  virtual TypeKind     kind     () const        { return RECORD; }
  virtual std::string  name     () const        { return name_; }
  virtual StringVector typedata () const        { return aux_vector_split (auxvalues00_); }
  virtual Any          get      () const        { Any a; a.set<Rec> ((object_.*getter_)()); return a; }
  virtual void         set (const Any &a) const { (object_.*setter_) (a.get<Rec>()); }
private:
  T          &object_;
  const char *name_ = NULL, *auxvalues00_ = NULL;
  Rec    (T::*getter_) () const = NULL;
  void   (T::*setter_) (const Rec&) = NULL;
};
// Sequence property
template<class T, class Seq>
struct PropertyAccessorImpl<T,Seq,typename ToAnySeqConvertible<Seq>::type> : PropertyAccessor {
  PropertyAccessorImpl (const char *name, T &object, Seq (T::*getter) () const, void (T::*setter) (const Seq&), const char *auxvalues00) :
    object_ (object), name_ (name), auxvalues00_ (auxvalues00), getter_ (getter), setter_ (setter)
  {}
  virtual TypeKind     kind     () const        { return SEQUENCE; }
  virtual std::string  name     () const        { return name_; }
  virtual StringVector typedata () const        { return aux_vector_split (auxvalues00_); }
  virtual Any          get      () const        { Any a; a.set<Seq> ((object_.*getter_)()); return a; }
  virtual void         set (const Any &a) const { (object_.*setter_) (a.get<Seq>()); }
private:
  T          &object_;
  const char *name_ = NULL, *auxvalues00_ = NULL;
  Seq    (T::*getter_) () const = NULL;
  void   (T::*setter_) (const Seq&) = NULL;
};
// Interface property
template<class T, class I>
struct PropertyAccessorImpl<T,I,typename std::is_base_of<ImplicitBase,I>::type> : PropertyAccessor {
  using IPtr = std::shared_ptr<I>;
  PropertyAccessorImpl (const char *name, T &object, IPtr (T::*getter) () const, void (T::*setter) (I*), const char *auxvalues00) :
    object_ (object), name_ (name), auxvalues00_ (auxvalues00), getter_ (getter), setter_ (setter)
  {}
  virtual TypeKind     kind     () const        { return INSTANCE; }
  virtual std::string  name     () const        { return name_; }
  virtual StringVector typedata () const        { return aux_vector_split (auxvalues00_); }
  virtual Any          get      () const        { Any a; a.set<IPtr> ((object_.*getter_)()); return a; }
  virtual void         set (const Any &a) const { (object_.*setter_) (a.get<IPtr>().get()); }
private:
  T          &object_;
  const char *name_ = NULL, *auxvalues00_ = NULL;
  IPtr   (T::*getter_) () const = NULL;
  void   (T::*setter_) (I*) = NULL;
};

} // Aida

#endif // __AIDA_CXX_AIDA_HH__
