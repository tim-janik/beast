// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __AIDA_CXX_AIDA_HH__
#define __AIDA_CXX_AIDA_HH__

#include <cassert>              // FIXME

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

namespace Aida {

// == Auxillary macros ==
#define AIDA_CPP_STRINGIFYi(s)  #s // indirection required to expand __LINE__ etc
#define AIDA_CPP_STRINGIFY(s)   AIDA_CPP_STRINGIFYi (s)
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
#define AIDA_ASSERT_RETURN(expr,...)      do { if (AIDA_ISLIKELY (expr)) break; AIDA_ASSERTION_FAILED (__FILE__, __LINE__, #expr); return __VA_ARGS__; } while (0)
#define AIDA_ASSERT_RETURN_UNREACHED(...) do { AIDA_ASSERTION_FAILED (__FILE__, __LINE__, ! "unreached"); return __VA_ARGS__; } while (0)
#else   // !__GNUC__
#define AIDA_UNUSED
#define AIDA_DEPRECATED
#define AIDA_PURE
#define AIDA_NORETURN
#define AIDA_NOINLINE
#define AIDA_PRINTF(fix, arx)
#define AIDA_ISLIKELY(expr)     expr
#define AIDA_UNLIKELY(expr)     expr
#define AIDA_ASSERT_RETURN(expr,...)      do { } while (0)
#define AIDA_ASSERT_RETURN_UNREACHED(...) do { return __VA_ARGS__; } while (0)
#endif
#ifndef AIDA_ASSERTION_FAILED
#define AIDA_ASSERTION_FAILED(f,l,expr)   assert (expr);
#endif
#define AIDA_LIKELY             AIDA_ISLIKELY
#define AIDA_CLASS_NON_COPYABLE(ClassName)  \
  /*copy-ctor*/ ClassName  (const ClassName&) = delete; \
  ClassName&    operator=  (const ClassName&) = delete

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
class RemoteHandle;
class OrbObject;
class ImplicitBase;
class BaseConnection;
class ClientConnection;
class ServerConnection;
union ProtoUnion;
class ProtoMsg;
class ProtoReader;
typedef std::shared_ptr<OrbObject>    OrbObjectP;
typedef std::shared_ptr<ImplicitBase> ImplicitBaseP;
typedef std::shared_ptr<BaseConnection> BaseConnectionP;
typedef std::shared_ptr<ClientConnection> ClientConnectionP;
typedef std::shared_ptr<ServerConnection> ServerConnectionP;
typedef ProtoMsg* (*DispatchFunc) (ProtoReader&);
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

/// IsRemoteHandleDerived<T> - Check if @a T derives from Aida::RemoteHandle.
template<class T> using IsRemoteHandleDerived = ::std::integral_constant<bool, ::std::is_base_of<RemoteHandle, T>::value>;

/// IsImplicitBaseDerived<T> - Check if @a T derives from Aida::ImplicitBase.
template<class T> using IsImplicitBaseDerived = ::std::integral_constant<bool, ::std::is_base_of<ImplicitBase, T>::value>;

/// Has__accept__<T,Visitor> - Check if @a T provides a member template __accept__<>(Visitor).
template<class, class, class = void> struct Has__accept__ : std::false_type {};
template<class T, class V>
struct Has__accept__<T, V, void_t< decltype (std::declval<T>().template __accept__<V> (*(V*) NULL)) >> : std::true_type {};

/// Has__accept_accessor__<T,Visitor> - Check if @a T provides a member template __accept_accessor__<>(Visitor).
template<class, class, class = void> struct Has__accept_accessor__ : std::false_type {};
template<class T, class V>
struct Has__accept_accessor__<T, V, void_t< decltype (std::declval<T>().template __accept_accessor__<V> (*(V*) NULL)) >> : std::true_type {};

/// Has__aida_from_any__<T> - Check if @a T provides a member __aida_from_any__(const Any&).
template<class, class = void> struct Has__aida_from_any__ : std::false_type {};
template<class T>
struct Has__aida_from_any__<T, void_t< decltype (std::declval<T>().__aida_from_any__ (std::declval<Aida::Any>())) >> : std::true_type {};

/// Has__aida_to_any__<T> - Check if @a T provides a member Has__aida_to_any__().
template<class, class = void> struct Has__aida_to_any__ : std::false_type {};
template<class T>
struct Has__aida_to_any__<T, void_t< decltype (std::declval<T>().__aida_to_any__ ()) >> : std::true_type {};

/// Provide the member typedef type which is the element_type of the shared_ptr type @a T.
template<typename T> struct RemoveSharedPtr                                             { typedef T type; };
template<typename T> struct RemoveSharedPtr<::std::shared_ptr<T>>                       { typedef T type; };
template<typename T> struct RemoveSharedPtr<const ::std::shared_ptr<T>>                 { typedef T type; };
template<typename T> struct RemoveSharedPtr<volatile ::std::shared_ptr<T>>              { typedef T type; };
template<typename T> struct RemoveSharedPtr<const volatile ::std::shared_ptr<T>>        { typedef T type; };

// == String Utilitiies ==
String       posix_sprintf                (const char *format, ...) AIDA_PRINTF (1, 2);
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

// == PosixLocaleGuard ==
/// Use the POSIX locale in the current thread once a PosixLocaleGuard is created and until it is destroyed.
class PosixLocaleGuard {
  struct  Locale;
  Locale &locale_;
  int64   localemem_[1];
public:
  /*ctor*/  PosixLocaleGuard();
  /*dtor*/ ~PosixLocaleGuard();
};

// == VirtualEnableSharedFromThis ==
/// Helper class for VirtualEnableSharedFromThis.
struct VirtualEnableSharedFromThisBase :
    public virtual std::enable_shared_from_this<VirtualEnableSharedFromThisBase> {
  virtual ~VirtualEnableSharedFromThisBase() = 0;
};
/// Virtual base class template that provides std::enable_shared_from_this for multiple inheritance.
template<class T>
struct VirtualEnableSharedFromThis : public virtual VirtualEnableSharedFromThisBase {
  std::shared_ptr<T>       shared_from_this()       { return std::dynamic_pointer_cast<T> (VirtualEnableSharedFromThisBase::shared_from_this()); }
  std::shared_ptr<const T> shared_from_this() const { return std::dynamic_pointer_cast<T> (VirtualEnableSharedFromThisBase::shared_from_this()); }
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

// == EnumValue ==
/// Aida info for enumeration values.
struct EnumValue {
  int64       value;
  const char *ident, *label, *blurb;
  template<class EValue>
  constexpr EnumValue (EValue dflt) : value (int64 (dflt)), ident (0), label (0), blurb (0) {}
  constexpr EnumValue ()            : value (0), ident (0), label (0), blurb (0) {}
  constexpr EnumValue (int64 v, const char *vident, const char *vlabel, const char *vblurb) :
    value (v), ident (vident), label (vlabel), blurb (vblurb) {}
};
typedef std::vector<EnumValue> EnumValueVector;

// == Enum ==
/// Class for enum type introspection.
class EnumInfo {
  const String           enum_name_;
  const EnumValue *const values_;
  const uint32_t         n_values_;
  const bool             flags_;
  explicit               EnumInfo          (const String &enum_name, bool isflags, uint32_t n_values, const EnumValue *values);
  static const EnumInfo& cached_enum_info  (const String &enum_name, bool isflags, uint32_t n_values, const EnumValue *values);
  template<size_t N>
  static const EnumInfo& cached_enum_info  (const String &enum_name, bool isflags, const EnumValue (&varray)[N])
  { return cached_enum_info (enum_name, isflags, N, varray); }
public:
  String          name              () const;                           ///< Retrieve the enum type name for this Enum.
  EnumValue       find_value        (const String &name) const;         ///< Find first enum value matching @a name.
  int64           value_from_string (const String &valuestring) const;  ///< Reconstruct an enum value from @a valuestring.
  bool            flags_enum        () const;   ///< Whether enum values support bit combinations to form flags.
  bool            has_values        () const;   ///< Indicate if the value_vector() is non-empty.
  EnumValueVector value_vector      () const;   ///< Retrieve the list of possible enum values as a std::vector<>.
  /// Find first enum value equal to @a value.
  template<typename T, REQUIRES< std::is_enum<T>::value > = true>
  EnumValue       find_value        (T     value) const                     { return find_value (int64 (value)); }
  EnumValue       find_value        (int64 value) const;
  /// Create a string representing @a value.
  template<typename T, REQUIRES< std::is_enum<T>::value > = true>
  String          value_to_string   (T     value) const                     { return value_to_string (int64 (value)); }
  String          value_to_string   (int64 value) const;
  String          value_to_string   (int64 value, const String &joiner) const;
  /// Template to be specialised by introspectable enums.
  template<typename EnumType> friend
  const EnumInfo& enum_info         ()
  {
    static_assert (std::is_enum<EnumType>::value, "");
    return cached_enum_info (typeid_name<EnumType>(), false, 0, NULL);
  }
};
template<typename EnumType> const EnumInfo& enum_info (); // clang++ needs this extra prototype of the above friend

template<typename EnumType> EnumType
enum_value_from_string (const String &valuestring)      ///< Type-safe variant of EnumInfo.value_from_string().
{ return (EnumType) Aida::enum_info<EnumType>().value_from_string (valuestring); }
template<typename EnumType> String
enum_value_to_string (EnumType evalue)                  ///< Type-safe variant of EnumInfo.value_to_string().
{ return Aida::enum_info<EnumType>().value_to_string (evalue); }
template<typename EnumType> String
enum_value_to_string (EnumType evalue, const String &joiner) ///< Variant of enum_value_to_string() with value joiner.
{ return Aida::enum_info<EnumType>().value_to_string (int64 (evalue), joiner); }

// == AuxDataRegistry ==
class AuxDataRegistry {
  static void register_aux_data (const char *auxentry, size_t length);
public:
  static std::vector<std::string> lookup          (const std::string &abstypename);
  template<size_t I>              AuxDataRegistry (const char (&auxentry) [I])
  {
    static_assert (I >= 1, "");
    register_aux_data (auxentry, I);
  }
};

/// Split @a char_array at '\\0' and merge with @a v1 .. @a vf.
std::vector<String> aux_vector_split    (const char *char_array, size_t length); // Splits @a char_array at '\\0'
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
  INSTANCE       = 'C', ///< Interface instance type.
  REMOTE         = 'r', ///< RemoteHandle type.
  TRANSITION     = 'T', ///< Instance or RemoteHandle in transition between remotes.
  ANY            = 'Y', ///< Generic type to hold any other type.
};
template<> const EnumInfo& enum_info<TypeKind> ();

const char* type_kind_name (TypeKind type_kind); ///< Obtain TypeKind names as a string.

// == TypeHash ==
struct TypeHash {
  uint64 typehi, typelo;
  constexpr      TypeHash   (uint64 hi, uint64 lo) : typehi (hi), typelo (lo) {}
  constexpr      TypeHash   () : typehi (0), typelo (0)                       {}
  String         to_string  () const;
  constexpr bool operator== (const TypeHash &z) const                         { return typehi == z.typehi && typelo == z.typelo; }
  friend    bool operator<  (const TypeHash &a, const TypeHash &b)
  {
    return AIDA_UNLIKELY (a.typehi == b.typehi) ? a.typelo < b.typelo : a.typehi < b.typehi;
  }
};
typedef std::vector<TypeHash> TypeHashList;


// == Internal Type Hashes ==
#define AIDA_HASH___TYPELIST__          0x00a1dad1ce0ff1ceULL, 0xa1dad1ce0ff1ce01ULL
#define AIDA_HASH___AUX_DATA__          0x00a1dad1ce0ff1ceULL, 0xa1dad1ce0ff1ce02ULL
#define AIDA_HASH___DIR__               0x00a1dad1ce0ff1ceULL, 0xa1dad1ce0ff1ce03ULL
#define AIDA_HASH___GET__               0x00a1dad1ce0ff1ceULL, 0xa1dad1ce0ff1ce04ULL
#define AIDA_HASH___SET__               0x00a1dad1ce0ff1ceULL, 0xa1dad1ce0ff1ce05ULL
#define AIDA_HASH___EVENT_ATTACH__      0x00a1dad1ce0ff1ceULL, 0xa1dad1ce0ff1ce06ULL
#define AIDA_HASH___EVENT_DETACHID__    0x00a1dad1ce0ff1ceULL, 0xa1dad1ce0ff1ce07ULL
#define AIDA_HASH___EVENT_DETACHNS__    0x00a1dad1ce0ff1ceULL, 0xa1dad1ce0ff1ce08ULL
#define AIDA_HASH___EVENT_CALLBACK__    0x00a1dad1ce0ff1ceULL, 0xa1dad1ce0ff1ce09ULL


// === EventFd ===
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

// == Type Utilities ==
template<class Y> struct ValueType           { typedef Y T; };
template<class Y> struct ValueType<Y&>       { typedef Y T; };
template<class Y> struct ValueType<const Y&> { typedef Y T; };

// == Message IDs ==
enum MessageId {
  // none                   = 0x0000000000000000
  MSGID_CALL_ONEWAY         = 0x1000000000000000ULL, ///< One-way method call (void return).
  MSGID_EMIT_ONEWAY         = 0x2000000000000000ULL, ///< One-way signal emissions (void return).
  //MSGID_META_ONEWAY       = 0x3000000000000000ULL, ///< One-way method call (void return).
  MSGID_CONNECT             = 0x4000000000000000ULL, ///< Signal handler (dis-)connection, expects CONNECT_RESULT.
  MSGID_CALL_TWOWAY         = 0x5000000000000000ULL, ///< Two-way method call, expects CALL_RESULT.
  MSGID_EMIT_TWOWAY         = 0x6000000000000000ULL, ///< Two-way signal emissions, expects EMIT_RESULT.
  //MSGID_META_TWOWAY       = 0x7000000000000000ULL, ///< Two-way method call, expects META_REPLY.
  // meta_exception         = 0x8000000000000000
  MSGID_DISCONNECT          = 0xa000000000000000ULL, ///< Signal destroyed, disconnect all handlers.
  MSGID_CONNECT_RESULT      = 0xc000000000000000ULL, ///< Result message for CONNECT.
  MSGID_CALL_RESULT         = 0xd000000000000000ULL, ///< Result message for CALL_TWOWAY.
  MSGID_EMIT_RESULT         = 0xe000000000000000ULL, ///< Result message for EMIT_TWOWAY.
  //MSGID_META_REPLY        = 0xf000000000000000ULL, ///< Result message for MSGID_META_TWOWAY.
  // meta messages and results
  MSGID_META_HELLO          = 0x7100000000000000ULL, ///< Hello from client, expects WELCOME.
  MSGID_META_WELCOME        = 0xf100000000000000ULL, ///< Hello reply from server, contains remote_origin.
  MSGID_META_GARBAGE_SWEEP  = 0x7200000000000000ULL, ///< Garbage collection cycle, expects GARBAGE_REPORT.
  MSGID_META_GARBAGE_REPORT = 0xf200000000000000ULL, ///< Reports expired/retained references.
  MSGID_META_SEEN_GARBAGE   = 0x3300000000000000ULL, ///< Client indicates garbage collection may be useful.
};
/// Check if msgid is a reply for a two-way call (one of the _RESULT or _REPLY message ids).
inline constexpr bool msgid_is_result (MessageId msgid) { return (msgid & 0xc000000000000000ULL) == 0xc000000000000000ULL; }

/// Helper structure to pack MessageId, sender and receiver connection IDs.
union IdentifierParts {
  uint64        vuint64;
  struct { // MessageId bits
# if __BYTE_ORDER == __LITTLE_ENDIAN
    uint        sender_connection : 16, free16 : 16, destination_connection : 16, free8 : 8, message_id : 8;
# elif __BYTE_ORDER == __BIG_ENDIAN
    uint        message_id : 8, free8 : 8, destination_connection : 16, free16 : 16, sender_connection : 16;
# endif
  };
  static_assert (__BYTE_ORDER == __LITTLE_ENDIAN || __BYTE_ORDER == __BIG_ENDIAN, "__BYTE_ORDER unknown");
  constexpr IdentifierParts (uint64 vu64) : vuint64 (vu64) {}
  constexpr IdentifierParts (MessageId id, uint destination, uint sender) :
# if __BYTE_ORDER == __LITTLE_ENDIAN
    sender_connection (sender), free16 (0), destination_connection (destination), free8 (0), message_id (IdentifierParts (id).message_id)
# elif __BYTE_ORDER == __BIG_ENDIAN
    message_id (IdentifierParts (id).message_id), free8 (0), destination_connection (destination), free16 (0), sender_connection (sender)
# endif
  {}
};
constexpr uint64 CONNECTION_MASK = 0x0000ffff;

// == OrbObject ==
/// Internal management structure for remote objects.
class OrbObject {
  const uint64  orbid_;
protected:
  explicit                  OrbObject         (uint64 orbid);
  virtual                  ~OrbObject         ();
public:
  uint64                    orbid             () const       { return orbid_; }
  virtual ClientConnection* client_connection ();
  static uint64             orbid_make        (uint16 connection, uint16 type_index, uint32 counter)
  { return (uint64 (connection) << 48) | (uint64 (type_index) << 32) | counter; }
};

// == RemoteHandle ==
class RemoteHandle {
  OrbObjectP        orbop_;
  template<class Parent>
  struct NullRemoteHandleT : public Parent {
    TypeHashList __aida_typelist__ () const { return TypeHashList(); }
  };
  typedef NullRemoteHandleT<RemoteHandle> NullRemoteHandle;
  static OrbObjectP __aida_null_orb_object__ ();
protected:
  explicit          RemoteHandle             (OrbObjectP);
  explicit          RemoteHandle             () : orbop_ (__aida_null_orb_object__()) {}
  const OrbObjectP& __aida_orb_object__      () const   { return orbop_; }
  void              __aida_upgrade_from__    (const OrbObjectP&);
  void              __aida_upgrade_from__    (const RemoteHandle &rhandle) { __aida_upgrade_from__ (rhandle.__aida_orb_object__()); }
public:
  /*copy*/                RemoteHandle         (const RemoteHandle &y) : orbop_ (y.orbop_) {}
  virtual                ~RemoteHandle         ();
  TypeHashList            __aida_typelist__    () const;
  std::vector<String>     __aida_aux_data__    () const;
  std::vector<String>     __aida_dir__         () const;
  Any                     __aida_get__         (const String &name) const;
  bool                    __aida_set__         (const String &name, const Any &any);
  ClientConnection*       __aida_connection__  () const { return orbop_->client_connection(); }
  uint64                  __aida_orbid__       () const { return orbop_->orbid(); }
  static NullRemoteHandle __aida_null_handle__ ()       { return NullRemoteHandle(); }
  // Support event handlers
  uint64                  __event_attach__     (const String &type, EventHandlerF handler);
  bool                    __event_detach__     (uint64 connection_id);
  uint64                  __event_detach__     (const String &type);
  // Determine if this RemoteHandle contains an object or null handle.
  explicit    operator bool () const noexcept               { return 0 != __aida_orbid__(); }
  bool        operator==    (std::nullptr_t) const noexcept { return 0 == __aida_orbid__(); }
  bool        operator!=    (std::nullptr_t) const noexcept { return 0 != __aida_orbid__(); }
  bool        operator==    (const RemoteHandle &rh) const noexcept { return __aida_orbid__() == rh.__aida_orbid__(); }
  bool        operator!=    (const RemoteHandle &rh) const noexcept { return !operator== (rh); }
  friend bool operator==    (std::nullptr_t nullp, const RemoteHandle &shd) noexcept { return shd == nullp; }
  friend bool operator!=    (std::nullptr_t nullp, const RemoteHandle &shd) noexcept { return shd != nullp; }
private:
  template<class TargetHandle> static typename
  std::enable_if<(std::is_base_of<RemoteHandle, TargetHandle>::value &&
                  !std::is_same<RemoteHandle, TargetHandle>::value), TargetHandle>::type
  __aida_reinterpret_down_cast__ (RemoteHandle smh)     ///< Reinterpret & dynamic cast, use discouraged.
  {
    TargetHandle target;
    target.__aida_upgrade_from__ (smh);                 // like reinterpret_cast<>
    return TargetHandle::down_cast (target);            // like dynamic_cast<>
  }
  friend class BaseConnection;
};

// == RemoteMember ==
template<class RemoteHandle>
class RemoteMember : public RemoteHandle {
public:
  inline   RemoteMember (const RemoteHandle &src) : RemoteHandle() { *this = src; }
  explicit RemoteMember () : RemoteHandle() {}
  void     operator=   (const RemoteHandle &src) { RemoteHandle::operator= (src); }
};

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
  typedef std::vector<Any>   AnyList; ///< Vector of Any structures for use in #SEQUENCE types.
  /// Vector of fields (named Any structures) for use in #RECORD types.
  class AnyDict : public std::vector<Field> {
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
  typedef RemoteMember<RemoteHandle> ARemoteHandle;
  union {
    uint64 vuint64; int64 vint64; double vdouble; Any *vany;
    struct { int64 venum64; const EnumInfo *enum_info; };
    int64 dummy_[AIDA_I64ELEMENTS (std::max (std::max (sizeof (String), sizeof (std::vector<void*>)),
                                             std::max (sizeof (ImplicitBaseP), sizeof (ARemoteHandle))))];
    AnyDict&             vfields () { return *(AnyDict*) this; static_assert (sizeof (AnyDict) <= sizeof (*this), ""); }
    const AnyDict&       vfields () const { return *(const AnyDict*) this; }
    AnyList&             vanys   () { return *(AnyList*) this; static_assert (sizeof (AnyList) <= sizeof (*this), ""); }
    const AnyList&       vanys   () const { return *(const AnyList*) this; }
    String&              vstring () { return *(String*) this; static_assert (sizeof (String) <= sizeof (*this), ""); }
    const String&        vstring () const { return *(const String*) this; }
    ImplicitBaseP&       ibase   () { return *(ImplicitBaseP*) this; static_assert (sizeof (ImplicitBaseP) <= sizeof (*this), ""); }
    const ImplicitBaseP& ibase   () const { return *(const ImplicitBaseP*) this; }
    ARemoteHandle&       rhandle () { return *(ARemoteHandle*) this; static_assert (sizeof (ARemoteHandle) <= sizeof (*this), ""); }
    const ARemoteHandle& rhandle () const { return *(const ARemoteHandle*) this; }
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
  const EnumInfo& get_enum_info ();                     ///< Get enum info for an Any holding an enum, undefined otherwise.
  void            set_enum      (const EnumInfo &einfo,
                                 int64 value);          ///< Set Any to hold an enum value.
private:
  template<class A, class B> using IsConvertible = ///< Avoid pointer->bool reduction for std::is_convertible<>.
    ::std::integral_constant<bool, ::std::is_convertible<A, B>::value && (!::std::is_pointer<A>::value || !IsBool<B>::value)>;
  template<class T>          using IsConstCharPtr        = ::std::is_same<const char*, typename ::std::decay<T>::type>;
  template<class T>          using IsImplicitBaseDerivedP =
    ::std::integral_constant<bool, (DerivesSharedPtr<T>::value && // check without SFINAE error on missing T::element_type
                                    ::std::is_base_of<ImplicitBase, typename RemoveSharedPtr<T>::type >::value)>;
  bool               get_bool    () const;
  void               set_bool    (bool value);
  void               set_int64   (int64 value);
  void               set_double  (double value);
  std::string        get_string  () const;
  void               set_string  (const std::string &value);
  int64              get_enum    (const EnumInfo &einfo) const;
  template<typename Enum>
  Enum               get_enum    () const               { return Enum (get_enum (enum_info<Enum>())); }
  template<typename Enum>
  void               set_enum    (Enum value)           { return set_enum (enum_info<Enum>(), int64 (value)); }
  const AnyList&     get_seq     () const;
  void               set_seq     (const AnyList &seq);
  const AnyDict&     get_rec     () const;
  void               set_rec     (const AnyDict &rec);
  ImplicitBaseP      get_ibasep  () const;
  void               set_ibase   (ImplicitBase *ibase);
  template<typename C>
  C*                 cast_ibase  () const               { return dynamic_cast<C*> (get_ibasep().get()); }
  template<typename SP>
  SP                 cast_ibasep () const               { return std::dynamic_pointer_cast<typename SP::element_type> (get_ibasep()); }
  RemoteHandle       get_handle  () const;
  template<typename H>
  H                  cast_handle () const               { return H::down_cast (get_handle()); }
  void               set_handle  (const RemoteHandle &handle);
  const Any*         get_any     () const;
  void               set_any     (const Any *value);
public:
  // Type get() const;
  template<typename T, REQUIRES< IsBool<T>::value > = true>                            T    get () const { return get_bool(); }
  template<typename T, REQUIRES< IsInteger<T>::value > = true>                         T    get () const { return as_int64(); }
  template<typename T, REQUIRES< std::is_floating_point<T>::value > = true>            T    get () const { return as_double(); }
  template<typename T, REQUIRES< DerivesString<T>::value > = true>                     T    get () const { return get_string(); }
  template<typename T, REQUIRES< std::is_enum<T>::value > = true>                      T    get () const { return get_enum<T>(); }
  template<typename T, REQUIRES< std::is_same<const AnyList*, T>::value > = true>      T    get () const { return &get_seq(); }
  template<typename T, REQUIRES< std::is_same<const AnyList&, T>::value > = true>      T    get () const { return get_seq(); }
  template<typename T, REQUIRES< IsConvertible<const AnyList, T>::value > = true>      T    get () const { return get_seq(); }
  template<typename T, REQUIRES< std::is_same<const AnyDict*, T>::value > = true>      T    get () const { return &get_rec(); }
  template<typename T, REQUIRES< std::is_same<const AnyDict&, T>::value > = true>      T    get () const { return get_rec(); }
  template<typename T, REQUIRES< IsConvertible<const AnyDict, T>::value > = true>      T    get () const { return get_rec(); }
  template<typename T, REQUIRES< IsImplicitBaseDerived<T>::value > = true>             T&   get () const { return *cast_ibase<T>(); }
  template<typename T, REQUIRES< IsImplicitBaseDerivedP<T>::value > = true>            T    get () const { return cast_ibasep<T>(); }
  template<typename T, REQUIRES< IsRemoteHandleDerived<T>::value > = true>             T    get () const { return cast_handle<T>(); }
  template<typename T, REQUIRES< IsConvertible<const Any, T>::value > = true>          T    get () const { return *get_any(); }
  // void set (const Type&);
  template<typename T, REQUIRES< IsBool<T>::value > = true>                            void set (T v) { return set_bool (v); }
  template<typename T, REQUIRES< IsInteger<T>::value > = true>                         void set (T v) { return set_int64 (v); }
  template<typename T, REQUIRES< std::is_floating_point<T>::value > = true>            void set (T v) { return set_double (v); }
  template<typename T, REQUIRES< DerivesString<T>::value > = true>                     void set (T v) { return set_string (v); }
  template<typename T, REQUIRES< IsConstCharPtr<T>::value > = true>                    void set (T v) { return set_string (v); }
  template<typename T, REQUIRES< std::is_enum<T>::value > = true>                      void set (T v) { return set_enum<T> (v); }
  template<typename T, REQUIRES< std::is_convertible<T, AnyList>::value > = true>      void set (const T &v) { return set_seq (v); }
  template<typename T, REQUIRES< std::is_convertible<T, AnyList>::value > = true>      void set (const T *v) { return set_seq (*v); }
  template<typename T, REQUIRES< std::is_convertible<T, AnyDict>::value > = true>      void set (const T &v) { return set_rec (v); }
  template<typename T, REQUIRES< std::is_convertible<T, AnyDict>::value > = true>      void set (const T *v) { return set_rec (*v); }
  template<typename T, REQUIRES< IsImplicitBaseDerived<T>::value > = true>             void set (T &v) { return set_ibase (&v); }
  template<typename T, REQUIRES< IsImplicitBaseDerivedP<T>::value > = true>            void set (T v) { return set_ibase (v.get()); }
  template<typename T, REQUIRES< IsRemoteHandleDerived<T>::value > = true>             void set (T v) { return set_handle (v); }
  template<typename T, REQUIRES< std::is_base_of<Any, T>::value > = true>              void set (const T &v) { return set_any (&v); }
  // convenience
  static Any          any_from_strings (const std::vector<std::string> &string_container);
  std::vector<String> any_to_strings   () const;
  void                to_transition    (BaseConnection &base_connection);
  void                from_transition  (BaseConnection &base_connection);
  String              repr             (const String &field_name = "") const;
  String              to_string        () const; ///< Retrieve string representation of Any for printouts.
  int64               as_int64         () const; ///< Obtain contents as int64.
  double              as_double        () const; ///< Obtain contents as double.
  const Any&          as_any           () const { return kind() == ANY ? *u_.vany : *this; } ///< Obtain contents as Any.
};
typedef Any::AnyDict AnyDict;
typedef Any::AnyList AnyList;

// == Event ==
class Event : public virtual VirtualEnableSharedFromThis<Event> {
  AnyDict  fields_;
public:
  explicit       Event      (const String &type);
  explicit       Event      (const AnyDict &adict);
  Any&           operator[] (const String &name)          { return fields_[name]; }
  const Any&     operator[] (const String &name) const    { return fields_[name]; }
  const AnyDict& fields     () const                      { return fields_; }
};

// == ImplicitBase ==
/// Abstract base interface that all IDL interfaces are implicitely derived from.
class ImplicitBase : public virtual VirtualEnableSharedFromThis<ImplicitBase> {
protected:
  virtual                    ~ImplicitBase        () = 0; // abstract class
public:
  virtual std::string         __aida_type_name__  () const = 0; ///< Retrieve the IDL type name of an instance.
  virtual TypeHashList        __aida_typelist__   () const = 0;
  virtual const StringVector& __aida_aux_data__   () const = 0;
  virtual std::vector<String> __aida_dir__        () const = 0;
  virtual Any                 __aida_get__        (const String &name) const = 0;
  virtual bool                __aida_set__        (const String &name, const Any &any) = 0;
  uint64                      __event_attach__    (const String &type, EventHandlerF handler);
  bool                        __event_detach__    (int64 connection_id);
  uint64                      __event_detach__    (const String &type);
  void                        __event_emit__      (const Event &event);
};


// == ProtoMsg ==
union ProtoUnion {
  int64        vint64;
  double       vdouble;
  Any         *vany;
  String      *vstr;
  void        *pmem[2];                                 // equate sizeof (ProtoMsg)
  uint8        bytes[8];                                // ProtoMsg types
  struct { uint32 index, capacity; };                   // ProtoMsg.buffermem[0]
};

class ProtoMsg { // buffer for marshalling procedure calls
  friend class ProtoReader;
  void               check_internal ();
  inline ProtoUnion& upeek (uint32 n) const { return buffermem[offset() + n]; }
protected:
  ProtoUnion        *buffermem;
  inline void        check ()      { if (AIDA_UNLIKELY (size() > capacity())) check_internal(); }
  inline uint32      offset () const { const uint32 offs = 1 + (capacity() + 7) / 8; return offs; }
  inline TypeKind    type_at  (uint32 n) const { return TypeKind (buffermem[1 + n/8].bytes[n%8]); }
  inline void        set_type (TypeKind ft)  { buffermem[1 + size()/8].bytes[size()%8] = ft; }
  inline ProtoUnion& getu () const           { return buffermem[offset() + size()]; }
  inline ProtoUnion& addu (TypeKind ft) { set_type (ft); ProtoUnion &u = getu(); buffermem[0].index++; check(); return u; }
  inline ProtoUnion& uat (uint32 n) const { return AIDA_LIKELY (n < size()) ? upeek (n) : *(ProtoUnion*) NULL; }
  explicit           ProtoMsg (uint32 _ntypes);
  explicit           ProtoMsg (uint32, ProtoUnion*, uint32);
public:
  virtual      ~ProtoMsg ();
  inline uint32 size     () const          { return buffermem[0].index; }
  inline uint32 capacity () const          { return buffermem[0].capacity; }
  inline uint64 first_id () const          { return AIDA_LIKELY (buffermem && size() && type_at (0) == INT64) ? upeek (0).vint64 : 0; }
  inline void add_bool   (bool    vbool)   { ProtoUnion &u = addu (BOOL); u.vint64 = vbool; }
  inline void add_int64  (int64 vint64)    { ProtoUnion &u = addu (INT64); u.vint64 = vint64; }
  inline void add_evalue (int64 vint64)    { ProtoUnion &u = addu (ENUM); u.vint64 = vint64; }
  inline void add_double (double vdouble)  { ProtoUnion &u = addu (FLOAT64); u.vdouble = vdouble; }
  inline void add_orbid  (uint64 objid)    { ProtoUnion &u = addu (TRANSITION); u.vint64 = objid; }
  void        add_string (const String &s);
  void        add_any    (const Any &vany, BaseConnection &bcon);
  inline void add_header1 (MessageId m, uint64 h, uint64 l) { add_int64 (IdentifierParts (m).vuint64); add_int64 (h); add_int64 (l); }
  inline void add_header2 (MessageId m, uint64 h, uint64 l) { add_int64 (IdentifierParts (m).vuint64); add_int64 (h); add_int64 (l); }
  inline ProtoMsg& add_rec      (uint32 nt) { ProtoUnion &u = addu (RECORD); return *new (&u) ProtoMsg (nt); }
  inline ProtoMsg& add_seq      (uint32 nt) { ProtoUnion &u = addu (SEQUENCE); return *new (&u) ProtoMsg (nt); }
  inline void      reset        ();
  String           first_id_str () const;
  String           to_string    () const;
  static String    type_name    (int field_type);
  static ProtoMsg* _new         (uint32 _ntypes); // Heap allocated ProtoMsg
  // static ProtoMsg* new_error (const String &msg, const String &domain = "");
  static ProtoMsg* new_result        (MessageId m, uint64 h, uint64 l, uint32 n = 1);
  static ProtoMsg* renew_into_result (ProtoMsg *fb, MessageId m, uint64 h, uint64 l, uint32 n = 1);
  static ProtoMsg* renew_into_result (ProtoReader &fbr, MessageId m, uint64 h, uint64 l, uint32 n = 1);
  inline void operator<<= (uint32 v)          { ProtoUnion &u = addu (INT64); u.vint64 = v; }
  inline void operator<<= (ULongIffy v)       { ProtoUnion &u = addu (INT64); u.vint64 = v; }
  inline void operator<<= (uint64 v)          { ProtoUnion &u = addu (INT64); u.vint64 = v; }
  inline void operator<<= (int32 v)           { ProtoUnion &u = addu (INT64); u.vint64 = v; }
  inline void operator<<= (LongIffy v)        { ProtoUnion &u = addu (INT64); u.vint64 = v; }
  inline void operator<<= (int64 v)           { ProtoUnion &u = addu (INT64); u.vint64 = v; }
  inline void operator<<= (bool   v)          { ProtoUnion &u = addu (BOOL); u.vint64 = v; }
  inline void operator<<= (double v)          { ProtoUnion &u = addu (FLOAT64); u.vdouble = v; }
  inline void operator<<= (EnumValue e)       { ProtoUnion &u = addu (ENUM); u.vint64 = e.value; }
  inline void operator<<= (const String &s)   { add_string (s); }
  inline void operator<<= (const TypeHash &h) { *this <<= h.typehi; *this <<= h.typelo; }
  void        operator<<= (const Any &vany);
  void        operator<<= (const RemoteHandle &rhandle);
  void        operator<<= (ImplicitBase *instance);
};

class ProtoReader { // read ProtoMsg contents
  const ProtoMsg    *fb_;
  uint32             nth_;
  void               check_request (int type);
  ImplicitBaseP      pop_interface ();
  inline void        request (int t) { if (AIDA_UNLIKELY (nth_ >= n_types() || get_type() != t)) check_request (t); }
  inline ProtoUnion& fb_getu (int t) { request (t); return fb_->upeek (nth_); }
  inline ProtoUnion& fb_popu (int t) { request (t); ProtoUnion &u = fb_->upeek (nth_++); return u; }
public:
  explicit               ProtoReader (const ProtoMsg &fb) : fb_ (&fb), nth_ (0) {}
  uint64                 debug_bits  ();
  inline const ProtoMsg* proto_msg   () const { return fb_; }
  inline void            reset       (const ProtoMsg &fb) { fb_ = &fb; nth_ = 0; }
  inline void            reset       () { fb_ = NULL; nth_ = 0; }
  inline uint32          remaining   () { return n_types() - nth_; }
  inline void            skip        () { if (AIDA_UNLIKELY (nth_ >= n_types())) check_request (0); nth_++; }
  inline void            skip_header () { skip(); skip(); skip(); }
  inline uint32          n_types     () { return fb_->size(); }
  inline TypeKind        get_type    () { return fb_->type_at (nth_); }
  inline int64           get_bool    () { ProtoUnion &u = fb_getu (BOOL); return u.vint64; }
  inline int64           get_int64   () { ProtoUnion &u = fb_getu (INT64); return u.vint64; }
  inline int64           get_evalue  () { ProtoUnion &u = fb_getu (ENUM); return u.vint64; }
  inline double          get_double  () { ProtoUnion &u = fb_getu (FLOAT64); return u.vdouble; }
  inline const String&   get_string  () { ProtoUnion &u = fb_getu (STRING); return *u.vstr; }
  inline const ProtoMsg& get_rec     () { ProtoUnion &u = fb_getu (RECORD); return *(ProtoMsg*) &u; }
  inline const ProtoMsg& get_seq     () { ProtoUnion &u = fb_getu (SEQUENCE); return *(ProtoMsg*) &u; }
  inline int64           pop_bool    () { ProtoUnion &u = fb_popu (BOOL); return u.vint64; }
  inline int64           pop_int64   () { ProtoUnion &u = fb_popu (INT64); return u.vint64; }
  inline int64           pop_evalue  () { ProtoUnion &u = fb_popu (ENUM); return u.vint64; }
  inline double          pop_double  () { ProtoUnion &u = fb_popu (FLOAT64); return u.vdouble; }
  inline const String&   pop_string  () { ProtoUnion &u = fb_popu (STRING); return *u.vstr; }
  inline uint64          pop_orbid   () { ProtoUnion &u = fb_popu (TRANSITION); return u.vint64; }
  Any                    pop_any     (BaseConnection &bcon);
  inline const ProtoMsg& pop_rec     () { ProtoUnion &u = fb_popu (RECORD); return *(ProtoMsg*) &u; }
  inline const ProtoMsg& pop_seq     () { ProtoUnion &u = fb_popu (SEQUENCE); return *(ProtoMsg*) &u; }
  inline void operator>>= (uint32 &v)          { ProtoUnion &u = fb_popu (INT64); v = u.vint64; }
  inline void operator>>= (ULongIffy &v)       { ProtoUnion &u = fb_popu (INT64); v = u.vint64; }
  inline void operator>>= (uint64 &v)          { ProtoUnion &u = fb_popu (INT64); v = u.vint64; }
  inline void operator>>= (int32 &v)           { ProtoUnion &u = fb_popu (INT64); v = u.vint64; }
  inline void operator>>= (LongIffy &v)        { ProtoUnion &u = fb_popu (INT64); v = u.vint64; }
  inline void operator>>= (int64 &v)           { ProtoUnion &u = fb_popu (INT64); v = u.vint64; }
  inline void operator>>= (bool &v)            { ProtoUnion &u = fb_popu (BOOL); v = u.vint64; }
  inline void operator>>= (double &v)          { ProtoUnion &u = fb_popu (FLOAT64); v = u.vdouble; }
  inline void operator>>= (EnumValue &e)       { ProtoUnion &u = fb_popu (ENUM); e.value = u.vint64; }
  inline void operator>>= (String &s)          { ProtoUnion &u = fb_popu (STRING); s = *u.vstr; }
  inline void operator>>= (TypeHash &h)        { *this >>= h.typehi; *this >>= h.typelo; }
  inline void operator>>= (std::vector<bool>::reference v) { bool b; *this >>= b; v = b; }
  void        operator>>= (Any &vany);
  void        operator>>= (RemoteHandle &rhandle);
  template<class Target> std::shared_ptr<Target> pop_instance () { return std::dynamic_pointer_cast<Target> (pop_interface()); }
};


// == Connections ==
/// Base connection context for ORB message exchange.
class BaseConnection {
  const std::string protocol_;
  BaseConnection   *peer_;
  AIDA_CLASS_NON_COPYABLE (BaseConnection);
protected:
  explicit               BaseConnection  (const std::string &protocol);
  virtual               ~BaseConnection  ();
  virtual void           remote_origin   (ImplicitBaseP rorigin) = 0;
  virtual RemoteHandle   remote_origin   () = 0;
  virtual void           receive_msg     (ProtoMsg*) = 0; ///< Accepts an incoming message, transfers memory.
  void                   post_peer_msg   (ProtoMsg*);     ///< Send message to peer, transfers memory.
  void                   peer_connection (BaseConnection &peer);
public:
  BaseConnection&        peer_connection () const;
  bool                   has_peer        () const;
  String                 protocol        () const { return protocol_; }
  virtual int            notify_fd       () = 0; ///< Returns fd for POLLIN, to wake up on incomming events.
  virtual bool           pending         () = 0; ///< Indicate whether any incoming events are pending that need to be dispatched.
  virtual void           dispatch        () = 0; ///< Dispatch a single event if any is pending.
  template<class H> H    remote_origin   ();
};

/// Function typoe for internal signal handling.
typedef ProtoMsg* SignalEmitHandler (const ProtoMsg*, void*);

/// Connection context for IPC servers. @nosubgrouping
class ServerConnection : public BaseConnection {
  friend  class ClientConnection;
  AIDA_CLASS_NON_COPYABLE (ServerConnection);
  static ServerConnectionP make_server_connection (const String &protocol);
protected:
  /*ctor*/                  ServerConnection      (const std::string &protocol);
  virtual                  ~ServerConnection      ();
  virtual void              cast_interface_handle (RemoteHandle &rhandle, ImplicitBaseP ibase) = 0;
public:
  typedef std::function<void (Aida::ProtoReader&)> EmitResultHandler;
  template<class C>
  static ServerConnectionP  bind                    (const String &protocol, std::shared_ptr<C> object_ptr);
  void                      post_peer_msg           (ProtoMsg *pm)      { BaseConnection::post_peer_msg (pm); }
  virtual void              emit_result_handler_add (size_t id, const EmitResultHandler &handler) = 0;
  virtual void              add_interface           (ProtoMsg &fb, ImplicitBaseP ibase) = 0;
  virtual ImplicitBaseP     pop_interface           (ProtoReader &fr) = 0;
protected: /// @name Registry for IPC method lookups
  static DispatchFunc find_method (uint64 hi, uint64 lo); ///< Lookup method in registry.
public:
  struct MethodEntry       { uint64 hashhi, hashlo; DispatchFunc dispatcher; };
  struct MethodRegistry    /// Registry structure for IPC method stubs.
  {
    template<size_t S> MethodRegistry  (const MethodEntry (&static_const_entries)[S])
    { for (size_t i = 0; i < S; i++) register_method (static_const_entries[i]); }
  private: static void register_method  (const MethodEntry &mentry);
  };
};

/// Connection context for IPC clients. @nosubgrouping
class ClientConnection : public BaseConnection {
  AIDA_CLASS_NON_COPYABLE (ClientConnection);
protected:
  explicit              ClientConnection (const std::string &protocol);
  virtual              ~ClientConnection ();
public: /// @name API for remote calls.
  static ClientConnectionP  connect           (const String &protocol);
  virtual ProtoMsg*         call_remote       (ProtoMsg*) = 0; ///< Carry out a remote call syncronously, transfers memory.
  virtual void              add_handle        (ProtoMsg &fb, const RemoteHandle &rhandle) = 0;
  virtual void              pop_handle        (ProtoReader &fr, RemoteHandle &rhandle) = 0;
  /// Set callback for wakeups when new events may need dispatching
  virtual void              notify_callback   (const std::function<void (ClientConnection&)> &cb) = 0;
public: /// @name API for signal event handlers.
  virtual size_t        signal_connect    (uint64 hhi, uint64 hlo, const RemoteHandle &rhandle, SignalEmitHandler seh, void *data) = 0;
  virtual bool          signal_disconnect (size_t signal_handler_id) = 0;
};


// == ProtoScpope ==
/// ProtoScpope keeps track of the ServerConnection and ClientConnection during RPC marshalling.
class ProtoScope {
  bool nested_;
public:
  /// Start/create an RPC scope for a connection pair within the current thread.
  explicit                 ProtoScope                (ClientConnection &client_connection);
  explicit                 ProtoScope                (ServerConnection &server_connection);
  /*dtor*/                ~ProtoScope                (); ///< Finish/destroy an RPC scope.
  ProtoMsg*                invoke                    (ProtoMsg *pm); ///< Carry out a remote call syncronously, transfers memory.
  void                     post_peer_msg             (ProtoMsg *pm); ///< Send message to peer, transfers memory.
  static ClientConnection& current_client_connection (); ///< Access the client connection of the current thread-specific RPC scope.
  static ServerConnection& current_server_connection (); ///< Access the server connection of the current thread-specific RPC scope.
  static BaseConnection&   current_base_connection   (); ///< Access the client or server connection of the current thread-specific RPC scope.
  AIDA_CLASS_NON_COPYABLE (ProtoScope);
};
struct ProtoScopeCall1Way : ProtoScope {
  ProtoScopeCall1Way (ProtoMsg &pm, const RemoteHandle &rhandle, uint64 hashi, uint64 hashlo);
};
struct ProtoScopeCall2Way : ProtoScope {
  ProtoScopeCall2Way (ProtoMsg &pm, const RemoteHandle &rhandle, uint64 hashi, uint64 hashlo);
};
struct ProtoScopeEmit1Way : ProtoScope {
  ProtoScopeEmit1Way (ProtoMsg &pm, ServerConnection &server_connection, uint64 hashi, uint64 hashlo);
};
struct ProtoScopeEmit2Way : ProtoScope {
  ProtoScopeEmit2Way (ProtoMsg &pm, ServerConnection &server_connection, uint64 hashi, uint64 hashlo);
};
struct ProtoScopeDisconnect : ProtoScope {
  ProtoScopeDisconnect (ProtoMsg &pm, ServerConnection &server_connection, uint64 hashi, uint64 hashlo);
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

inline void
ProtoMsg::reset()
{
  if (!buffermem)
    return;
  while (size() > 0)
    {
      buffermem[0].index--; // causes size()--
      switch (type_at (size()))
        {
        case STRING:    { ProtoUnion &u = getu(); delete u.vstr; }; break;
        case ANY:       { ProtoUnion &u = getu(); delete u.vany; }; break;
        case SEQUENCE:
        case RECORD:    { ProtoUnion &u = getu(); ((ProtoMsg*) &u)->~ProtoMsg(); }; break;
        default: ;
        }
    }
}

/// Initialize the ServerConnection of @a C and accept connections via @a protocol
template<class C> ServerConnectionP
ServerConnection::bind (const String &protocol, std::shared_ptr<C> object_ptr)
{
  AIDA_ASSERT_RETURN (object_ptr != NULL, NULL);
  auto server_connection = make_server_connection (protocol);
  if (server_connection)
    server_connection->remote_origin (object_ptr);
  return server_connection;
}

/// Retrieve the remote origin of type @a Handle from a connection.
template<class Handle> Handle
BaseConnection::remote_origin ()
{
  return RemoteHandle::__aida_reinterpret_down_cast__<Handle> (remote_origin());
}

} // Aida

#include "visitor.hh"

#endif // __AIDA_CXX_AIDA_HH__
