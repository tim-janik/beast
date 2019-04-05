// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "aida.hh"

#include <cmath>
#include <cstring>
#include <cxxabi.h> // abi::__cxa_demangle
#include <stdio.h>
#include <stdarg.h>
#include <sched.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <semaphore.h>
#include <poll.h>
#include <stddef.h>             // ptrdiff_t
#include <signal.h>
#ifdef  HAVE_SYS_EVENTFD_H
#include <sys/eventfd.h>
#endif // HAVE_SYS_EVENTFD_H
#include <stdexcept>
#include <cassert>              // FIXME
#include <deque>
#include <unordered_map>
#include <unordered_set>

// == Customizable Macros ==
#define AIDA_DEBUG(...)                         AIDA_DIAGNOSTIC_IMPL (__FILE__, __LINE__, "", 'D', ::Aida::posix_sprintf (__VA_ARGS__).c_str(), false)
#define AIDA_WARN(...)                          AIDA_DIAGNOSTIC_IMPL (__FILE__, __LINE__, __func__, 'W', ::Aida::posix_sprintf (__VA_ARGS__).c_str(), false)
#ifndef AIDA_DIAGNOSTIC_IMPL
/** Macro to print out Aida diagnostics (like failed assertions or debug messages) */
#define AIDA_DIAGNOSTIC_IMPL(file, line, func, kind, message, will_abort)       do { ::std::string s; \
  if (file) s += file;                                                  \
  if (line) s += ::Aida::posix_sprintf (":%u", line);                   \
  if (file) s += ": ";                                                  \
  if (func) s += func + ::std::string (": ");                           \
  if (kind == 'A') s += "assertion failed: ";                           \
  if (kind == 'E' && will_abort) s += "fatal-error: ";                  \
  if (kind == 'E' && !will_abort) s += "error: ";                       \
  if (kind == 'W') s += "warning: ";                                    \
  s += message;                                                         \
  if (s.size() && s[s.size() - 1] != '\n') s += "\n";                   \
  fputs (s.c_str(), stderr);                                            \
  } while (0)
#endif
#ifndef AIDA_DEFER_GARBAGE_COLLECTION
#define AIDA_DEFER_GARBAGE_COLLECTION(msecs, func, data)        ({ func (data); 0; })
#define AIDA_DEFER_GARBAGE_COLLECTION_CANCEL(id)                ({ (void) id; })
#endif
#ifndef assert_return
#define assert_return(cond,...)         assert (cond)
#endif
#ifndef return_unless
#define return_unless(cond, ...)        do { if (cond) break; return __VA_ARGS__; } while (0)
#endif

// == printf helper ==
#define LLI     (long long int)
#define LLU     (long long unsigned int)
#ifndef AIDA_POSIX_VSPRINTF
/** Macro to print into a ::std::string with printf()-like `format` using the POSIX locale (or use "C" as fallback, see ::locale_t). */
#define AIDA_POSIX_VSPRINTF(format, vargs)                           ({ \
  static locale_t posix_c_locale = newlocale (LC_ALL_MASK, "C", NULL);  \
  locale_t saved_locale = uselocale (posix_c_locale);                   \
  char buffer[4000 + 1 + 1] = { 0, };                                   \
  vsnprintf (buffer, 4000, format, vargs);                              \
  buffer[4000] = 0;                                                     \
  uselocale (saved_locale);                                             \
  ::std::string s (buffer); s; })
#endif

/// The Aida namespace provides all IDL functionality exported to C++.
namespace Aida {

static ::std::string
posix_sprintf (const char *format, ...)
{
  va_list args;
  va_start (args, format);
  ::std::string r = AIDA_POSIX_VSPRINTF (format, args);
  va_end (args);
  return r;
}

void
assertion_failed (const char *file, int line, const char *func, const char *stmt)
{
  AIDA_DIAGNOSTIC_IMPL (file, line, func, 'A', stmt ? stmt : "state unreachable", false);
}

// == Type Helpers ==
typedef std::weak_ptr<OrbObject>    OrbObjectW;

// == Prototypes ==
static void remote_handle_dispatch_event_discard_handler (Aida::ProtoReader &fbr);
static void remote_handle_dispatch_event_emit_handler    (Aida::ProtoReader &fbr);

// == Helper Classes ==
/// Create an instance of @a Class on demand that is constructed and never destructed.
/// DurableInstance<Class*> provides the memory for a @a Class instance and calls it's
/// constructor on demand, but it's destructor is never called (so the memory allocated
/// to the DurableInstance must not be freed). Due to its constexpr ctor and on-demand
/// creation of @a Class, a DurableInstance<> can be accessed at any time during the
/// static ctor (or dtor) phases and will always yield a properly initialized @a Class.
/// A DurableInstance is useful for static variables that need to be accessible from
/// other static ctor/dtor calls.
template<class Class>
class DurableInstance final {
  static_assert (std::is_class<Class>::value, "DurableInstance<Class> requires class template argument");
  Class *ptr_;
  uint64 mem_[(sizeof (Class) + sizeof (uint64) - 1) / sizeof (uint64)];
  void
  initialize() AIDA_NOINLINE
  {
    static std::mutex mtx;
    std::unique_lock<std::mutex> lock (mtx);
    if (ptr_ == NULL)
      ptr_ = new (mem_) Class(); // exclusive construction
  }
public:
  constexpr  DurableInstance() : ptr_ (NULL) {}
  /// Retrieve pointer to @a Class instance, always returns the same pointer.
  Class*
  operator->() AIDA_PURE
  {
    if (AIDA_UNLIKELY (ptr_ == NULL))
      initialize();
    return ptr_;
  }
  /// Retrieve reference to @a Class instance, always returns the same reference.
  Class&       operator*     () AIDA_PURE       { return *operator->(); }
  const Class* operator->    () const AIDA_PURE { return const_cast<DurableInstance*> (this)->operator->(); }
  const Class& operator*     () const AIDA_PURE { return const_cast<DurableInstance*> (this)->operator*(); }
  /// Check if @a this stores a @a Class instance yet.
  explicit     operator bool () const           { return ptr_ != NULL; }
};

// == VirtualEnableSharedFromThisBase ==
VirtualEnableSharedFromThisBase::~VirtualEnableSharedFromThisBase()
{} // force emission of vtable

// == String Utilitiies ==
static inline constexpr bool
c_isalnum (uint8 c)
{
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}

static inline constexpr char
identifier_char_canon (char c)
{
  if (c >= '0' && c <= '9')
    return c;
  else if (c >= 'A' && c <= 'Z')
    return c - 'A' + 'a';
  else if (c >= 'a' && c <= 'z')
    return c;
  else
    return '_';
}

static inline constexpr bool
identifier_match (const char *str1, const char *str2)
{
  while (*str1 && *str2)
    {
      const uint8 s1 = identifier_char_canon (*str1++);
      const uint8 s2 = identifier_char_canon (*str2++);
      if (s1 != s2)
        return false;
    }
  return *str1 == 0 && *str2 == 0;
}

static inline bool
match_identifier_detailed (const String &ident, const String &tail)
{
  AIDA_ASSERT_RETURN (ident.size() >= tail.size(), false);
  const char *word = ident.c_str() + ident.size() - tail.size();
  if (word > ident.c_str()) // allow partial matches on word boundary only
    {
      if (c_isalnum (word[-1]) && c_isalnum (word[0])) // no word boundary
        return false;
    }
  return identifier_match (word, tail.c_str());
}

/// Variant of string_match_identifier() that matches @a tail against @a ident at word boundary.
bool
string_match_identifier_tail (const String &ident, const String &tail)
{
  return ident.size() >= tail.size() && match_identifier_detailed (ident, tail);
}

/// Returns whether @a string starts with @a fragment.
bool
string_startswith (const String &string, const String &fragment)
{
  return fragment.size() <= string.size() && 0 == string.compare (0, fragment.size(), fragment);
}

/// Escape text like a C string.
/// Returns a string that escapes all characters with a backslash '\\' that need escaping in C language string syntax.
static String
string_to_cescape (const String &str)
{
  String buffer;
  for (String::const_iterator it = str.begin(); it != str.end(); it++)
    {
      const uint8 d = *it;
      if      (d == '\a')       buffer +=  "\\a";
      else if (d == '\b')       buffer +=  "\\b";
      else if (d == '\t')       buffer +=  "\\t";
      else if (d == '\n')       buffer +=  "\\n";
      else if (d == '\v')       buffer +=  "\\v";
      else if (d == '\f')       buffer +=  "\\f";
      else if (d == '\r')       buffer +=  "\\r";
      else if (d == '"')        buffer += "\\\"";
      else if (d == '\\')       buffer += "\\\\";
      else if (d < 32 || d > 126)
        buffer += posix_sprintf ("\\%03o", d);
      else
        buffer += d;
    }
  return buffer;
}

/// Returns a string as C string including double quotes.
String
string_to_cquote (const String &str)
{
  return String() + "\"" + string_to_cescape (str) + "\"";
}

/// Parse a string into a 64bit integer, optionally specifying the expected number base.
int64
string_to_int (const String &string, size_t *consumed, uint base)
{
  const char *const start = string.c_str(), *p = start;
  while (*p == ' ' || *p == '\n' || *p == '\t' || *p == '\r')
    p++;
  const bool negate = p[0] == '-';
  if (negate)
    p++;
  const bool hex = p[0] == '0' && (p[1] == 'X' || p[1] == 'x');
  const char *const number = hex ? p + 2 : p;
  char *endptr = NULL;
  const uint64_t result = strtoull (number, &endptr, hex ? 16 : base);
  if (consumed)
    {
      if (!endptr || endptr <= number)
        *consumed = 0;
      else
        *consumed = endptr - start;
    }
  if (result < 9223372036854775808ull)
    return negate ? -int64_t (result) : result;
  return negate ? -9223372036854775807ll - 1 : 9223372036854775808ull - 1;
}

/// Parse a string into a 64bit unsigned integer, optionally specifying the expected number base.
uint64
string_to_uint (const String &string, size_t *consumed, uint base)
{
  const char *const start = string.c_str(), *p = start;
  while (*p == ' ' || *p == '\n' || *p == '\t' || *p == '\r')
    p++;
  const bool hex = p[0] == '0' && (p[1] == 'X' || p[1] == 'x');
  const char *const number = hex ? p + 2 : p;
  char *endptr = NULL;
  const uint64 result = strtoull (number, &endptr, hex ? 16 : base);
  if (consumed)
    {
      if (!endptr || endptr <= number)
        *consumed = 0;
      else
        *consumed = endptr - start;
    }
  return result;
}

static bool
cstring_to_bool (const char *string, bool fallback)
{
  if (!string)
    return fallback;
  const char *p = string;
  // skip spaces
  while (*p && isspace (*p))
    p++;
  // ignore signs
  if (p[0] == '-' || p[0] == '+')
    {
      p++;
      // skip spaces
      while (*p && isspace (*p))
        p++;
    }
  // handle numbers
  if (p[0] >= '0' && p[0] <= '9')
    return 0 != string_to_uint (p);
  // handle special words
  if (strncasecmp (p, "ON", 2) == 0)
    return 1;
  if (strncasecmp (p, "OFF", 3) == 0)
    return 0;
  // empty string
  if (!p[0])
    return fallback;
  // anything else needs to resemble "yes" or "true"
  return strchr ("YyTt", p[0]);
}

/** Interpret a string as boolean value.
 * Interpret the string as number, "ON"/"OFF" or distinguish "false"/"true" or "yes"/"no" by starting letter.
 * For empty strings, @a fallback is returned.
 */
bool
string_to_bool (const String &string, bool fallback)
{
  return cstring_to_bool (string.c_str(), fallback);
}

/// Convert a double into a string, using the POSIX/C locale.
String
string_from_double (double value)
{
  if (std::isnan (value))
    return std::signbit (value) ? "-NaN" : "+NaN";
  if (std::isinf (value))
    return std::signbit (value) ? "-Infinity" : "+Infinity";
  return posix_sprintf ("%.17g", value);
}

/** Join a number of strings.
 * Join a string vector into a single string, using @a junctor inbetween each pair of strings.
 */
String
string_join (const String &junctor, const StringVector &strvec)
{
  String s;
  if (strvec.size())
    s = strvec[0];
  for (uint i = 1; i < strvec.size(); i++)
    s += junctor + strvec[i];
  return s;
}

/// Split a string, using any of the @a splitchars as delimiter.
/// Passing "" as @a splitter will split the string between all position.
StringVector
string_split_any (const String &string, const String &splitchars, size_t maxn)
{
  StringVector sv;
  size_t i, l = 0;
  if (splitchars.empty())
    {
      for (i = 0; i < string.size() && sv.size() < maxn; i++)
        sv.push_back (string.substr (i, 1));
      if (i < string.size())
        sv.push_back (string.substr (i, string.size() - i));
    }
  else
    {
      const char *schars = splitchars.c_str();
      l = 0;
      for (i = 0; i < string.size() && sv.size() < maxn; i++)
        if (strchr (schars, string[i]))
          {
            if (i >= l)
              sv.push_back (string.substr (l, i - l));
            l = i + 1;
          }
      i = string.size();
      if (i >= l)
        sv.push_back (string.substr (l, i - l));
    }
  return sv;
}

/** Demangle a std::typeinfo.name() string into a proper C++ type name.
 * This function uses abi::__cxa_demangle() from <cxxabi.h> to demangle C++ type names,
 * which works for g++, libstdc++, clang++, libc++.
 */
String
string_demangle_cxx (const char *mangled_identifier)
{
  int status = 0;
  char *malloced_result = abi::__cxa_demangle (mangled_identifier, NULL, NULL, &status);
  String result = malloced_result && !status ? malloced_result : mangled_identifier;
  if (malloced_result)
    free (malloced_result);
  return result;
}

// === String Options ===
static void
string_option_add (const String   &assignment,
                   vector<String> *option_namesp,
                   vector<String> &option_values,
                   const String   &empty_default,
                   const String   *filter)
{
  AIDA_ASSERT_RETURN ((option_namesp != NULL) ^ (filter != NULL));
  const char *n = assignment.c_str();
  while (isspace (*n))
    n++;
  const char *p = n;
  while (isalnum (*p) || *p == '-' || *p == '_')
    p++;
  const String name = String (n, p - n);
  if (filter && name != *filter)
    return;
  while (isspace (*p))
    p++;
  const String value = *p == '=' ? String (p + 1) : empty_default;
  if (!name.empty() && (*p == '=' || *p == 0)) // valid name
    {
      if (!filter)
        option_namesp->push_back (name);
      option_values.push_back (value);
    }
}

static void
string_options_split_filtered (const String &option_string, vector<String> *option_namesp,
                               vector<String> &option_values, const String &empty_default, const String *filter)
{
  const char *s = option_string.c_str();
  while (s)
    {
      // find next separator
      const char *b = strpbrk (s, ";:"); // find seperator
      string_option_add (String (s, b ? b - s : strlen (s)), option_namesp, option_values, empty_default, filter);
      s = b ? b + 1 : NULL;
    }
}

static String
string_option_find_value (const String &option_string, const String &option)
{
  vector<String> option_names, option_values;
  string_options_split_filtered (option_string, NULL, option_values, "1", &option);
  return option_values.empty() ? "0" : option_values[option_values.size() - 1];
}

/// Check if an option is set/unset in an options list string.
bool
string_option_check (const String &option_string, const String &option)
{
  const String value = string_option_find_value (option_string, option);
  return cstring_to_bool (value.c_str(), true);
}

// == Message IDs ==
/// Mask MessageId bits, see IdentifierParts.message_id.
static inline constexpr MessageId
msgid_mask (uint64 msgid)
{
  // return MessageId (IdentifierParts (IdentifierParts (msgid).message_id, 0, 0).vuint64);
  return MessageId (msgid & 0xff00000000000000ULL);
}

/// Add the highest bit that indicates results or replies, does not neccessarily yield a valid result message id.
static inline constexpr MessageId
msgid_as_result (MessageId msgid)
{
  return MessageId (msgid | 0x8000000000000000ULL);
}

/// Check if @a msgid expects a _RESULT or _REPLY message.
static inline constexpr bool
msgid_needs_result (MessageId msgid)
{
  return (msgid & 0xc000000000000000ULL) == 0x4000000000000000ULL;
}

/// Check if the @a msgid matches @a check_id, @a msgid will be masked accordingly.
static inline constexpr bool
msgid_is (uint64 msgid, MessageId check_id)
{
  return msgid_mask (msgid) == check_id;
}

// == EnumInfo ==
EnumInfo::EnumInfo (const String &enum_name, bool isflags, uint32_t n_values, const EnumValue *values) :
  enum_name_ (enum_name), values_ (values), n_values_ (n_values), flags_ (isflags)
{
  if (n_values)
    AIDA_ASSERT_RETURN (values != NULL);
}

String
EnumInfo::name () const
{
  return enum_name_;
}

bool
EnumInfo::flags_enum () const
{
  return flags_;
}

bool
EnumInfo::has_values () const
{
  return n_values_ > 0;
}

EnumValueVector
EnumInfo::value_vector () const
{
  EnumValueVector vv;
  for (size_t i = 0; i < n_values_; i++)
    vv.push_back (values_[i]);
  return vv;
}

EnumValue
EnumInfo::find_value (const String &name) const
{
  for (size_t i = 0; i < n_values_; i++)
    if (string_match_identifier_tail (values_[i].ident, name))
      return values_[i];
  return EnumValue();
}

EnumValue
EnumInfo::find_value (int64 value) const
{
  for (size_t i = 0; i < n_values_; i++)
    if (values_[i].value == value)
      return values_[i];
  return EnumValue();
}

String
EnumInfo::value_to_string (int64 value) const
{
  const EnumValue ev = find_value (value);
  if (ev.ident)
    return ev.ident;
  return value_to_string (value, "+");
}

String
EnumInfo::value_to_string (int64 value, const String &joiner) const
{
  // try exact match
  for (size_t i = 0; i < n_values_; i++)
    if (values_[i].value == value)
      return values_[i].ident;
  // try bit filling
  int64 filled = 0;
  std::vector<String> idents;
  for (size_t i = 0; i < n_values_; i++)
    if ((values_[i].value & ~value) == 0 &&     // must not set unrelated bits
        (values_[i].value & ~filled) != 0)      // must fill new bits
      {
        idents.push_back (values_[i].ident);
        filled |= values_[i].value;
        if (filled == value)
          break;
      }
  if (filled == value)
    return string_join (joiner, idents);
  // fallback
  return posix_sprintf ("0x%llx", LLU value);
}

int64
EnumInfo::value_from_string (const String &valuestring) const
{
  const EnumValue ev = find_value (valuestring);
  return ev.ident ? ev.value : string_to_int (valuestring);
}

// == IntrospectionEntryMap ==
struct IntrospectionEntry {
  const char  *type_name;
  const char  *fundamental;
  const char  *auxentries;
  size_t       length;
  StringVector entries;
};

using IntrospectionEntryMap = std::unordered_map<String, IntrospectionEntry>;

static inline IntrospectionEntryMap&
aux_data_map()
{
  static IntrospectionEntryMap aux_data_map;
  return aux_data_map;
}

// == Introspection ==
static inline bool
is_identifier_char (int ch)
{
  if ((ch >= 'A' && ch <= 'Z') ||
      (ch >= 'a' && ch <= 'z') ||
      (ch >= '0' && ch <= '9') ||
      ch == '_' || ch == '$')
    return true;
  return false;
}

static std::string
normalize_typename (const std::string &string)
{
  std::string normalized;
  for (size_t i = 0; i < string.size() && string[i]; ++i)
    if (is_identifier_char (string[i]))
      normalized += string[i];
    else if (normalized.size() && normalized[normalized.size() - 1] != '.')
      normalized += '.';
  return normalized;
}

const StringVector&
find_normalized_type (const std::string &abstypename, std::string *kind)
{
  const auto it = aux_data_map().find (abstypename);
  if (it != aux_data_map().end())
    {
      IntrospectionEntry &info = it->second;
      if (!info.entries.size())
        {
          static std::mutex mutex;
          std::lock_guard<std::mutex> locker (mutex);
          if (!info.entries.size())
            {
              StringVector entries = aux_vector_split (info.auxentries, info.length);
              std::swap (info.entries, entries);
            }
        }
      if (kind)
        *kind = info.fundamental;
      return info.entries;
    }
  if (kind)
    *kind = "";
  static const StringVector empty;
  return empty;
}

/// Retrieve the `key=value` pair properties associated with `abstypename`, optionally extracting `kind`.
const StringVector&
Introspection::find_type (const std::string &abstypename, std::string *kind)
{
  const StringVector &kvlist = find_normalized_type (abstypename, kind);
  if (kvlist.empty())
    {
      const String normalized = normalize_typename (abstypename);
      return find_normalized_type (normalized, kind);
    }
  return kvlist;
}

/// Convenience function to retrieve the `kind` string of `find_type()` for `abstypename`.
std::string
Introspection::find_type_kind (const std::string &abstypename)
{
  if (abstypename == "VOID" ||
      abstypename == "BOOL" ||
      abstypename == "INT32" ||
      abstypename == "INT64" ||
      abstypename == "FLOAT64" ||
      abstypename == "STRING" ||
      abstypename == "ENUM" ||
      abstypename == "RECORD" ||
      abstypename == "SEQUENCE" ||
      abstypename == "FUNC" ||
      abstypename == "INTERFACE" ||
      abstypename == "STREAM" ||
      abstypename == "ANY")
    return abstypename;
  std::string kind;
  find_type (abstypename, &kind);
  return kind;
}

/// Retrieve the value of a `key=value` pair matching `key` in `kvlist` or `fallback` if none is found.
std::string
Introspection::find_value (const std::string &key, const StringVector &kvlist, const std::string &fallback)
{
  const size_t l = key.size();
  for (size_t i = 0; i < kvlist.size(); i++)
    if (kvlist[i].size() > l && kvlist[i][l] == '=' && strncmp (&kvlist[i][0], &key[0], l) == 0)
      return &kvlist[i][l + 1];
  return fallback;
}

/// List all properties in `kvlist` that are prefixed under `field`.
StringVector
Introspection::find_nested (const std::string &field, const StringVector &kvlist)
{
  const size_t l = field.size();
  StringVector r;
  for (size_t i = 0; i < kvlist.size(); i++)
    if (kvlist[i].size() > l && kvlist[i][l] == '.' && strncmp (&kvlist[i][0], &field[0], l) == 0)
      r.push_back (&kvlist[i][l + 1]);
  return r;
}

static std::string
split_enumerator (const std::string &enumerator, std::string *tail)
{
  const size_t pdot = enumerator.rfind ('.');
  *tail = pdot != std::string::npos ? enumerator.substr (pdot + 1) : enumerator;
  return pdot != std::string::npos ? enumerator.substr (0, pdot) : "";
}

/// List all properties of `enumerator` in `enumtypename`.
StringVector
Introspection::find_enumerator (const std::string &enumerator)
{
  String tail, enumtype = split_enumerator (normalize_typename (enumerator), &tail);
  const StringVector &kvlist = find_normalized_type (enumtype, NULL);
  return find_nested (tail, kvlist);
}

/// Match 'MEMBER.property=VALUE' against @a kvpair, return <MEMBER,VALUE> if @a property matches.
static inline std::pair<size_t, const char*>
split_member_at_property (const char *const kv, const char *const property)
{
  const char *eq = strchr (kv, '=');
  const int   lp = strlen (property);
  const int   bt = 1 + lp;              // backtrack from '='
  if (eq && eq - kv > bt && eq[-bt] == '.' && strncmp (eq - lp, property, lp) == 0)
    return std::make_pair (eq - kv - bt, eq + 1);
  return std::make_pair (0, nullptr);
}

/// List all enumerator names for `enum_typename`.
StringVector
Introspection::list_enumerators (const std::string &enum_typename)
{
  const String normalized = normalize_typename (enum_typename);
  String kind;
  const StringVector &kvlist = find_normalized_type (normalized, &kind);
  if (kind == "ENUM")
    {
      String enumerators = find_value ("enumerators", kvlist);
      StringVector enumerator_list;
      for (const auto &shortname : string_split_any (enumerators, ";"))
        enumerator_list.push_back (normalized + "." + shortname);
      return enumerator_list;
    }
  return {};
}

/// Return enum value identifier in `enumtypename` with the exact numeric `value`.
std::string
Introspection::enumerator_from_value (const std::string &enumtypename, int64_t value)
{
  const String normalized = normalize_typename (enumtypename);
  const StringVector &kvlist = find_normalized_type (normalized, NULL);
  for (const String &kv : kvlist)
    {
      const char *kvc = kv.c_str();
      const auto mvpair = split_member_at_property (kvc, "value");
      if (!mvpair.second)
        continue;                                       // not an IDENT.value=123 entry
      size_t consumed = 0;
      const int64 pvalue = string_to_int (mvpair.second, &consumed);
      if (!consumed)
        continue;                                       // not a parsable value number
      if (value == pvalue)
        return normalized + "." + kv.substr (0, mvpair.first);
    }
  return "";
}

static std::string
resolve_enum_context (const std::string &partialenumerator, const std::string &enum_context, std::string *tail)
{
  const String partial = normalize_typename (partialenumerator);
  // determine enum type
  String enumname = split_enumerator (partial, tail);
  bool enum_match = Introspection::find_type_kind (enumname) == "ENUM";
  if (!enum_match)
    {
      // try enum_context
      enumname = normalize_typename (enum_context);
      enum_match = Introspection::find_type_kind (enumname) == "ENUM";
      if (!enum_match)
        {
          // try enum_context after splitting off enumerator
          const size_t cdot = enumname.rfind ('.');
          if (cdot != std::string::npos)
            {
              enumname = enumname.substr (0, cdot);
              enum_match = Introspection::find_type_kind (enumname) == "ENUM";
            }
        }
    }
  return enum_match ? enumname : "";    // returns "" if no matching enum type was found
}

/// Match `partialenumerator`, considering the enum/enumerator `enum_context`.
std::string
Introspection::match_enumerator (const std::string &partialenumerator, const std::string &enum_context)
{
  String tail, enumname = resolve_enum_context (partialenumerator, enum_context, &tail);
  if (enumname.empty())
    return "";                  // no matching enum type
  // match enumerator while allowing prefix omissions
  String kind;
  const StringVector &kvlist = find_normalized_type (enumname, &kind);
  if (kind == "ENUM")
    {
      const String enumerators = find_value ("enumerators", kvlist);
      for (const auto &enumerator : string_split_any (enumerators, ";"))
        if (string_match_identifier_tail (enumerator, tail))
          return enumname + "." + enumerator;
    }
  return "";                    // no matching enumerator
}

/// Retrieve numeric value for `enumerator` or 0.
int64_t
Introspection::enumerator_to_value (const std::string &enumerator, const std::string &enum_context)
{
  String tail, enumname = resolve_enum_context (enumerator, enum_context, &tail);
  String kind;
  const StringVector &kvlist = find_normalized_type (enumname, &kind);
  const char *fallback = NULL;
  if (kind == "ENUM")
    {
      for (const std::string &kv : kvlist)
        {
          const auto mvpair = split_member_at_property (kv.c_str(), "value");
          if (!mvpair.second)
            continue;                                   // not an IDENT.value=123 entry
          if (string_match_identifier_tail (kv.substr (0, mvpair.first), tail))
            return string_to_int (mvpair.second);
          if (!fallback)
            fallback = mvpair.second;                   // capture first enumerator as fallback
        }
    }
  return fallback ? string_to_int (fallback) : 0;
}

std::string
Introspection::strip_type_prefix (const std::string &dottedstring)
{
  const size_t dot = dottedstring.rfind ('.');
  return dot == std::string::npos ? dottedstring : dottedstring.substr (dot + 1);
}

/// Retrieve a statically allocated c_str for `string`, may be called from any thread.
static const char*
legacy_string_set_mt (const String &string)
{
  static std::set<std::string> strings;
  static std::mutex mutex;
  std::lock_guard<std::mutex> locker (mutex);
  auto iter_inserted = strings.insert (string);         // pair<iterator,bool>
  assert_return (iter_inserted.first != strings.end(), NULL);
  return iter_inserted.first->c_str();
}

/// Legacy variant of find_enumerator_name() that returns a c_str.
const char*
Introspection::legacy_enumerator (const std::string &enumtypename, int64_t value)
{
  const String name = enumerator_from_value (enumtypename, value);
  return legacy_string_set_mt (name);
}

// == IntrospectionRegistry ==
void
IntrospectionRegistry::register_aux_data (const char *auxentry, size_t length)
{
  AIDA_ASSERT_RETURN (auxentry && length > 0 && auxentry[length - 1] == 0);
  AIDA_ASSERT_RETURN (strncmp (auxentry, "typename=", 9) == 0);     // first element is the type name
  const char *type_name = auxentry + 9;
  const char *fundamental = type_name + strlen (type_name) + 1; // second element is the fundamental type
  AIDA_ASSERT_RETURN (fundamental - type_name < length && strncmp (fundamental, "type=", 5) == 0);
  fundamental += 5;
  AIDA_ASSERT_RETURN (fundamental[0] != 0);
  const char *entries = fundamental + strlen (fundamental) + 1;
  AIDA_ASSERT_RETURN (entries < auxentry + length);
  aux_data_map()[type_name] = IntrospectionEntry { type_name, fundamental, entries, length - (entries - auxentry) };
}

static std::vector<const char*>
split_aux_char_array (const char *char_array, size_t length)
{
  std::vector<const char*> cv;
  AIDA_ASSERT_RETURN (char_array && length >= 1, cv);
  AIDA_ASSERT_RETURN (char_array[length-1] == 0, cv);
  const char *p = char_array, *const end = char_array + length - 1;
  while (p < end)
    {
      const size_t l = strlen (p);
      cv.push_back (p);
      p += l + 1;
    }
  return cv;
}

std::vector<String>
aux_vector_split (const char *char_array, size_t length)
{
  std::vector<const char*> cv = split_aux_char_array (char_array, length);
  std::vector<std::string> sv (cv.begin(), cv.end());
  return sv;
}

std::vector<String>
aux_vector_split (const char *auxinfo00)
{
  if (!auxinfo00)
    return {};
  const char *p = auxinfo00;
  size_t l = strlen (p);
  while (l)
    {
      p += l + 1;
      l = strlen (p);
    }
  return aux_vector_split (auxinfo00, p - auxinfo00);
}

std::vector<String>
aux_vectors_combine (const std::vector<String> &v0, const std::vector<String> &v1, const std::vector<String> &v2,
                     const std::vector<String> &v3, const std::vector<String> &v4, const std::vector<String> &v5,
                     const std::vector<String> &v6, const std::vector<String> &v7, const std::vector<String> &v8,
                     const std::vector<String> &v9, const std::vector<String> &va, const std::vector<String> &vb,
                     const std::vector<String> &vc, const std::vector<String> &vd, const std::vector<String> &ve,
                     const std::vector<String> &vf)
{
  std::vector<std::string> sv (v0);
  sv.insert (sv.end(), v1.begin(), v1.end());
  sv.insert (sv.end(), v2.begin(), v2.end());
  sv.insert (sv.end(), v3.begin(), v3.end());
  sv.insert (sv.end(), v4.begin(), v4.end());
  sv.insert (sv.end(), v5.begin(), v5.end());
  sv.insert (sv.end(), v6.begin(), v6.end());
  sv.insert (sv.end(), v7.begin(), v7.end());
  sv.insert (sv.end(), v8.begin(), v8.end());
  sv.insert (sv.end(), v9.begin(), v9.end());
  sv.insert (sv.end(), va.begin(), va.end());
  sv.insert (sv.end(), vb.begin(), vb.end());
  sv.insert (sv.end(), vc.begin(), vc.end());
  sv.insert (sv.end(), vd.begin(), vd.end());
  sv.insert (sv.end(), ve.begin(), ve.end());
  sv.insert (sv.end(), vf.begin(), vf.end());
  return sv;
}

String
aux_vector_find (const std::vector<String> &auxvector, const String &field, const String &key, const String &fallback)
{
  const String name = field.empty() ? key + "=" : field + "." + key + "=";
  for (const auto &kv : auxvector)
    if (name.compare (0, name.size(), kv, 0, name.size()) == 0)
      return kv.substr (name.size());
  return fallback;
}

bool
aux_vector_check_options (const std::vector<String> &auxvector, const String &field, const String &key, const String &options)
{
  for (const String &option : string_split_any (options, ":;"))
    if (!string_option_check (aux_vector_find (auxvector, field, key), option))
      return false;
  return true;
}


// == TypeKind ==
static const Aida::IntrospectionRegistry aux_data_TypeKind = {
  "typename=Aida.TypeKind\0"
  "type=ENUM\0"
  "enumerators=UNTYPED;VOID;BOOL;INT32;INT64;FLOAT64;STRING;ENUM;SEQUENCE;RECORD;INSTANCE;TRANSITION;ANY\0"
  "UNTYPED.value=0\0"
  "VOID.value=118\0"
  "BOOL.value=98\0"
  "INT32.value=105\0"
  "INT64.value=108\0"
  "FLOAT64.value=100\0"
  "STRING.value=115\0"
  "ENUM.value=69\0"
  "SEQUENCE.value=81\0"
  "RECORD.value=82\0"
  "INSTANCE.value=67\0"
  "TRANSITION.value=84\0"
  "ANY.value=89\0"
};

const char*
type_kind_name (TypeKind type_kind)
{
  return Introspection::legacy_enumerator ("Aida.TypeKind", type_kind);
}

// == TypeHash ==
String
TypeHash::to_string () const
{
  return posix_sprintf ("(0x%016llx,0x%016llx)", LLU typehi, LLU typelo);
}

// == ProtoUnion ==
static_assert (sizeof (ProtoMsg) <= sizeof (ProtoUnion), "sizeof ProtoMsg");

// == ImplicitBase ==
class ImplicitBaseExtra {
  explicit           ImplicitBaseExtra (const ImplicitBaseExtra&) = delete;
  ImplicitBaseExtra& operator=         (const ImplicitBaseExtra&) = delete;
  explicit           ImplicitBaseExtra (ImplicitBaseExtra&&) = delete;
  ImplicitBaseExtra& operator=         (ImplicitBaseExtra&&) = delete;
public:
  explicit           ImplicitBaseExtra () = default;
  struct Handler {
    String        type;                         // event type name
    EventHandlerF handler;
    int64         id = 0;
  };
  std::vector<Handler> handlers;
  uint64               empty_handlers = 0;      // hint to reduce allocations
  uint64               signal_emissions = 0;    // hint to guard against reuse
};

typedef std::unordered_map<ImplicitBase*, ImplicitBaseExtra*> ImplicitBaseExtraMap;
static std::mutex           implicit_base_extras_mutex_;
static ImplicitBaseExtraMap implicit_base_extras_;

ImplicitBase::~ImplicitBase()
{
  // delete ImplicitBaseExtra
  std::unique_lock<std::mutex> locker (implicit_base_extras_mutex_);
  ImplicitBaseExtraMap::iterator it = implicit_base_extras_.find (this);
  if (AIDA_UNLIKELY (it != implicit_base_extras_.end()))
    {
      ImplicitBaseExtra *extra = it->second;
      implicit_base_extras_.erase (it);
      locker.unlock();
      AIDA_ASSERT_RETURN (extra->signal_emissions == 0);
      delete extra;
    }
  else
    locker.unlock();
}

static uint64 implicit_base_event_handler_next_id = 0xe000000 + 1; // see: BSE_STARTID_EVENT_HANDLER

/// Attach an event @a handler function to a specific event @a type.
uint64
ImplicitBase::__event_attach__ (const String &type, EventHandlerF handler)
{
  if (type.empty() || !handler)
    return 0;
  std::lock_guard<std::mutex> locker (implicit_base_extras_mutex_);
  ImplicitBaseExtra *extra = implicit_base_extras_[this];
  if (!extra)
    {
      extra = new ImplicitBaseExtra();
      implicit_base_extras_[this] = extra;
    }
  if (extra->empty_handlers && !extra->signal_emissions)
    {
      for (auto it = extra->handlers.begin(); it != extra->handlers.end(); /**/)
        if (it->id == 0)
          it = extra->handlers.erase (it);
        else
          ++it;
      extra->empty_handlers = 0;
    }
  extra->handlers.resize (extra->handlers.size() + 1);
  ImplicitBaseExtra::Handler &next = extra->handlers.back();
  next.id = implicit_base_event_handler_next_id++;
  next.type = type;
  next.handler = handler;
  return next.id;
}

/// Detach an event @a handler function through a @a connection_id returned from an __event_attach__() call.
bool
ImplicitBase::__event_detach__ (int64 connection_id)
{
  std::lock_guard<std::mutex> locker (implicit_base_extras_mutex_);
  ImplicitBaseExtraMap::iterator it = implicit_base_extras_.find (this);
  if (AIDA_LIKELY (it != implicit_base_extras_.end()))
    for (auto hit = it->second->handlers.begin(); hit != it->second->handlers.end(); ++hit)
      if (hit->id == connection_id)
        {
          hit->type.clear();
          hit->handler = NULL;
          hit->id = 0;
          if (it->second->signal_emissions)
            it->second->empty_handlers++;
          else
            it->second->handlers.erase (hit);
          return true;
        }
  return false;
}

/// Dispatch @a event by calling all handlers associated with the event type (and its namespace).
void
ImplicitBase::__event_emit__ (const Event &event)
{
  const String type = event["type"].get<String>();
  const char *const ctype = type.c_str(), *const colon = strchr (ctype, ':');
  const String plain_type = colon ? type.substr (0, colon - ctype) : type;
  if (type.empty())
    return;
  std::unique_lock<std::mutex> locker (implicit_base_extras_mutex_);
  ImplicitBaseExtraMap::iterator it = implicit_base_extras_.find (this);
  if (it == implicit_base_extras_.end())
    return; // no handlers
  ImplicitBaseExtra *const extra = it->second;
  // loop through all possible handlers in order, __attach__
  // ensures that new handlers are only *appended*
  uint64 pos = 0;
  extra->signal_emissions++;
  while (pos < extra->handlers.size())
    {
      const ImplicitBaseExtra::Handler &h = extra->handlers[pos++];
      if (h.id != 0 && (type == h.type || plain_type == h.type))
        {
          EventHandlerF func = h.handler;
          locker.unlock();
          func (event);
          locker.lock();
        }
    }
  extra->signal_emissions--;
}

// == Any ==
static_assert (sizeof (std::string) <= sizeof (Any), ""); // assert big enough Any impl

Any::Any (const Any &clone) :
  Any()
{
  this->operator= (clone);
}

Any::Any (Any &&other) :
  Any()
{
  this->swap (other);
}

Any&
Any::operator= (Any &&other)
{
  if (this == &other)
    return *this;
  clear();
  this->swap (other);
  return *this;
}

Any&
Any::operator= (const Any &clone)
{
  if (this == &clone)
    return *this;
  clear();
  type_kind_ = clone.type_kind_;
  switch (kind())
    {
    case ENUM:
    case STRING:        new (&u_.vstring()) String (clone.u_.vstring());                             break;
    case ANY:           u_.vany = clone.u_.vany ? new Any (*clone.u_.vany) : NULL;                   break;
    case SEQUENCE:      new (&u_.vanys()) AnySeq (clone.u_.vanys());                                 break;
    case RECORD:        new (&u_.vfields()) AnyRec (clone.u_.vfields());                             break;
    case INSTANCE:      new (&u_.rhandle()) ARemoteHandle (clone.u_.rhandle());                      break;
    case TRANSITION:    // u_.vint64 = clone.u_.vint64;
    default:            u_ = clone.u_;                                                               break;
    }
  return *this;
}

template<typename U> static inline void
swap_any_unions (TypeKind kind, U &u, U &v)
{
  switch (kind)
    {
    case UNTYPED: case BOOL: case INT32: case INT64: case FLOAT64:
    case TRANSITION:    std::swap (u, v);                     break;
    case ENUM:
    case STRING:        std::swap (u.vstring(), v.vstring()); break;
    case SEQUENCE:      std::swap (u.vanys(), v.vanys());     break;
    case RECORD:        std::swap (u.vfields(), v.vfields()); break;
    case INSTANCE:      std::swap (u.rhandle(), v.rhandle()); break;
    case ANY:           std::swap (u.vany, v.vany);           break;
    default:            AIDA_ASSERT_RETURN_UNREACHED();       break;
    }
}

void
Any::swap (Any &other)
{
  if (kind() == other.kind())
    {
      if (this != &other)
        swap_any_unions (kind(), u_, other.u_);
      return;
    }
  Any tmp;
  // this <--> tmp
  tmp.rekind (kind());
  swap_any_unions (tmp.kind(), tmp.u_, u_);
  // this <--> other
  rekind (other.kind());
  swap_any_unions (kind(), u_, other.u_);
  // tmp <--> other
  other.rekind (tmp.kind());
  swap_any_unions (other.kind(), other.u_, tmp.u_);
}

void
Any::clear()
{
  switch (kind())
    {
    case ENUM:
    case STRING:        u_.vstring().~String();                 break;
    case ANY:           delete u_.vany;                         break;
    case SEQUENCE:      u_.vanys().~AnySeq();                   break;
    case RECORD:        u_.vfields().~AnyRec();                 break;
    case INSTANCE:      u_.rhandle().~ARemoteHandle();          break;
    case TRANSITION: ;
    default: ;
    }
  type_kind_ = UNTYPED;
  memset (&u_, 0, sizeof (u_));
}

bool
Any::empty () const
{
  return kind() == UNTYPED;
}

void
Any::rekind (TypeKind _kind)
{
  clear();
  type_kind_ = _kind;
  switch (_kind)
    {
    case ENUM:
    case STRING:   new (&u_.vstring()) String();        break;
    case ANY:      u_.vany = NULL;                      break;
    case SEQUENCE: new (&u_.vanys()) AnySeq();          break;
    case RECORD:   new (&u_.vfields()) AnyRec();        break;
    case INSTANCE: new (&u_.rhandle()) ARemoteHandle(); break;
    default:                                            break;
    }
}

Any
Any::any_from_strings (const std::vector<std::string> &string_container)
{
  AnySeq av;
  av.resize (string_container.size());
  for (size_t i = 0; i < av.size(); i++)
    av[i].set (string_container[i]);
  Any any;
  any.set (av);
  return any;
}

std::vector<std::string>
Any::any_to_strings () const
{
  const AnySeq *av = get<const AnySeq*>();
  std::vector<std::string> sv;
  if (av)
    {
      sv.resize (av->size());
      for (size_t i = 0; i < av->size(); i++)
        sv[i] = (*av)[i].get<std::string>();
    }
  return sv;
}

template<class T> static String any_vector_to_string (const T *av);

template<> String
any_vector_to_string (const Any::AnyRec *vec)
{
  String s;
  if (vec)
    for (auto const &any : *vec)
      {
        if (!s.empty())
          s += ", ";
        s += any.name + ": ";
        if (any.kind() == STRING)
          s += string_to_cquote (any.to_string());
        else
          s += any.to_string();
      }
  s = s.empty() ? "{}" : "{ " + s + " }";
  return s;
}

template<> String
any_vector_to_string (const Any::AnySeq *vec)
{
  String s;
  if (vec)
    for (auto const &any : *vec)
      {
        if (!s.empty())
          s += ", ";
        if (any.kind() == STRING)
          s += string_to_cquote (any.to_string());
        else
          s += any.to_string();
      }
  s = s.empty() ? "[]" : "[ " + s + " ]";
  return s;
}

String
Any::repr (const String &field_name) const
{
  String s = "{ ";
  s += "type=" + string_to_cquote (type_kind_name (kind()));
  if (!field_name.empty())
    s += ", name=" + string_to_cquote (field_name);
  s += ", value=";
  if (kind() == ANY)
    s += u_.vany ? u_.vany->repr() : Any().repr();
  else if (kind() == STRING || kind() == ENUM)
    s += string_to_cquote (u_.vstring());
  else
    s += to_string();
  s += " }";
  return s;
}

/// Convert Any to a string, tries to model Python's str().
String
Any::to_string() const
{
  String s;
  switch (kind())
    {
      // fall through
    case BOOL: case INT32:
    case INT64:      s += posix_sprintf ("%lld", LLI u_.vint64);                                                 break;
    case FLOAT64:    s += posix_sprintf ("%.17g", u_.vdouble);                                                   break;
    case ENUM:
    case STRING:     s += u_.vstring();                                                                          break;
    case SEQUENCE:   s += any_vector_to_string (&u_.vanys());                                                    break;
    case RECORD:     s += any_vector_to_string (&u_.vfields());                                                  break;
    case INSTANCE:   s += posix_sprintf ("(RemoteHandle (orbid=0x#%08llx))", LLU u_.rhandle().__aida_orbid__()); break;
    case TRANSITION: s += posix_sprintf ("(Any (TRANSITION, orbid=0x#%08llx))", LLU u_.vint64);                  break;
    case ANY:
      s += "(Any (";
      if (u_.vany && u_.vany->kind() == STRING)
        s += string_to_cquote (u_.vany->to_string());
      else if (u_.vany && u_.vany->kind() != UNTYPED)
        s += u_.vany->to_string();
      s += "))";
      break;
    default:         ;
    case UNTYPED:    break;
    }
  return s;
}

bool
Any::operator== (const Any &clone) const
{
  if (type_kind_ != clone.type_kind_)
    return false;
  switch (kind())
    {
    case UNTYPED:     break;
    case TRANSITION: case BOOL: case INT32: // chain
    case INT64:    if (u_.vint64 != clone.u_.vint64) return false;                                       break;
    case FLOAT64:  if (u_.vdouble != clone.u_.vdouble) return false;                                     break;
    case ENUM:
    case STRING:   if (u_.vstring() != clone.u_.vstring()) return false;                                 break;
    case SEQUENCE:
      return u_.vanys() == clone.u_.vanys();
    case RECORD:
      return u_.vfields() == clone.u_.vfields();
    case ANY:
      if (!u_.vany || !clone.u_.vany)
        return u_.vany == clone.u_.vany;
      else
        return *u_.vany == *clone.u_.vany;
    case INSTANCE:
      return u_.rhandle().__iface_ptr__() == clone.u_.rhandle().__iface_ptr__();
    default:
      AIDA_ASSERT_RETURN_UNREACHED (false);
      return false;
    }
  return true;
}

bool
Any::operator!= (const Any &clone) const
{
  return !operator== (clone);
}

bool
Any::get_bool () const
{
  switch (kind())
    {
    case TRANSITION: case BOOL: case INT32:
    case INT64:         return u_.vint64 != 0;
    case ENUM:
    case STRING:        return !u_.vstring().empty();
    case SEQUENCE:      return !u_.vanys().empty();
    case RECORD:        return !u_.vfields().empty();
    case INSTANCE:      return u_.rhandle().__iface_ptr__() != NULL;
    default: ;
    }
  return 0;
}

void
Any::set_bool (bool value)
{
  ensure (BOOL);
  u_.vint64 = value;
}

int64
Any::as_int64 () const
{
  switch (kind())
    {
    case BOOL: case INT32:
    case INT64:         return u_.vint64;
    case FLOAT64:       return u_.vdouble;
    case STRING:        return u_.vstring().size();
    case SEQUENCE:      return u_.vanys().size();
    case RECORD:        return u_.vfields().size();
    case ENUM:
    default:            return 0;
    }
}

void
Any::set_int64 (int64 value)
{
  ensure (INT64);
  u_.vint64 = value;
}

void
Any::set_enum (const String &enumerator)
{
  ensure (ENUM);
  u_.vstring().assign (enumerator);
}

std::string
Any::get_enum() const
{
  return kind() == ENUM || kind() == STRING ? u_.vstring() : "";
}

double
Any::as_double () const
{
  return kind() == FLOAT64 ? u_.vdouble : as_int64();
}

void
Any::set_double (double value)
{
  ensure (FLOAT64);
  u_.vdouble = value;
}

std::string
Any::get_string () const
{
  switch (kind())
    {
    case BOOL:          return u_.vint64 ? "true" : "false";
    case INT32:
    case INT64:         return posix_sprintf ("%lli", LLI u_.vint64);
    case FLOAT64:       return string_from_double (u_.vdouble);
    case ENUM:
    case STRING:        return u_.vstring();
    default: ;
    }
  return "";
}

void
Any::set_string (const std::string &value)
{
  ensure (STRING);
  u_.vstring().assign (value);
}

const Any::AnySeq&
Any::get_seq () const
{
  if (kind() == SEQUENCE)
    return u_.vanys();
  static const AnySeq empty;
  return empty;
}

void
Any::set_seq (const AnySeq &seq)
{
  ensure (SEQUENCE);
  if (&seq != &u_.vanys())
    {
      AnySeq tmp (seq); // beware of internal references, copy before freeing
      std::swap (tmp, u_.vanys());
    }
}

const AnyRec&
Any::get_rec () const
{
  if (kind() == RECORD && !u_.vfields().empty())
    return u_.vfields();
  static const AnyRec empty;
  return empty;
}

void
Any::set_rec (const AnyRec &rec)
{
  ensure (RECORD);
  if (&rec != &u_.vfields())
    {
      AnyRec tmp (rec); // beware of internal references, copy before freeing
      std::swap (tmp, u_.vfields());
    }
}

ImplicitBaseP
Any::get_ibasep () const
{
  if (kind() == INSTANCE)
    {
      RemoteHandle rh = u_.rhandle();
      return rh.__iface_ptr__();
    }
  return ImplicitBaseP();
}

/// Use Any.get<DerivedType>() instead.
RemoteHandle
Any::get_untyped_remote_handle () const
{
  RemoteHandle rh;
  if (kind() == INSTANCE)
    rh = u_.rhandle();
  return rh;
}

void
Any::set_handle (const RemoteHandle &handle)
{
  ensure (INSTANCE);
  u_.rhandle() = handle;
}

const Any*
Any::get_any () const
{
  if (kind() == ANY && u_.vany)
    return u_.vany;
  static const Any empty;
  return &empty;
}

void
Any::set_any (const Any *value)
{
  ensure (ANY);
  if (u_.vany != value)
    {
      Any *old = u_.vany;
      u_.vany = value && value->kind() != UNTYPED ? new Any (*value) : NULL;
      delete old;
    }
}

Any&
Any::AnyRec::operator[] (const String &name)
{
  for (size_t i = 0; i < size(); i++)
    if (name == (*this)[i].name)
      return (*this)[i];
  push_back (Field (name, Any()));
  return (*this)[size() - 1];
}

const Any&
Any::AnyRec::operator[] (const String &name) const
{
  for (size_t i = 0; i < size(); i++)
    if (name == (*this)[i].name)
      return (*this)[i];
  static const Any empty;
  return empty;
}

// == Event ==
Event::Event (const String &type)
{
  fields_["type"].set (type);
}

Event::Event (const AnyRec &arec) :
  fields_ (arec)
{
  (void) fields_["type"]; // ensure field is present
}

// == OrbObject ==
OrbObject::OrbObject (uint64 orbid) :
  orbid_ (orbid)
{}

OrbObject::~OrbObject()
{}

ClientConnection*
OrbObject::client_connection ()
{
  return NULL;
}

class NullOrbObject : public virtual OrbObject {
public:
  explicit NullOrbObject  () : OrbObject (0) {}
  virtual  ~NullOrbObject () override        {}
};

// == RemoteHandle ==
static void (RemoteHandle::*pmf_upgrade_from)  (const OrbObjectP&);

OrbObjectP
RemoteHandle::__aida_null_orb_object__ ()
{
  static OrbObjectP null_orbo = [] () {                         // use lambda to sneak in extra code
    pmf_upgrade_from = &RemoteHandle::__aida_upgrade_from__;    // export accessor for internal maintenance
    return std::make_shared<NullOrbObject>();
  } ();                                                         // executes lambda atomically
  return null_orbo;
}

RemoteHandle::RemoteHandle () :
  orbop_ (__aida_null_orb_object__())
{}

RemoteHandle::RemoteHandle (OrbObjectP orbo) :
  orbop_ (orbo ? orbo : __aida_null_orb_object__())
{}

RemoteHandle::~RemoteHandle()
{}

/// Upgrade a @a Null RemoteHandle into a handle for an existing object.
void
RemoteHandle::__aida_upgrade_from__ (const OrbObjectP &orbop)
{
  AIDA_ASSERT_RETURN (__aida_orbid__() == 0);
  orbop_ = orbop ? orbop : __aida_null_orb_object__();
}

struct RemoteHandle::EventHandlerRelay {
  ExecutionContext    &iface_context_;
  ExecutionContext    &handler_context_;
  EventHandlerF        handler_;
  IfaceEventConnection iface_connection_;
  explicit EventHandlerRelay (ExecutionContext &handler_execution_context, EventHandlerF handler,
                              ExecutionContext &iface_execution_context) :
    iface_context_ (iface_execution_context), handler_context_ (handler_execution_context), handler_ (handler)
  {}
  bool
  connected ()
  {
    return handler_ != NULL;
  }
  void
  disconnect ()
  {
    if (handler_)
      {
        handler_ = NULL;
        IfaceEventConnection ifaceconnection = iface_connection_;
        auto remote_disconnect = [ifaceconnection] () { ifaceconnection.disconnect(); };
        iface_context_.enqueue_mt (new std::function<void()> (remote_disconnect));
        iface_connection_ = IfaceEventConnection(); // reset
      }
  }
  void
  local_handler (const Event &event)
  {
    if (handler_)
      handler_ (event);
  }
  struct RemoteHandler : std::shared_ptr<EventHandlerRelay> {
    void
    operator() (const Event &event)
    {
      std::shared_ptr<EventHandlerRelay> &relayp = *this;
      // relayp and event must be copied for use in a different thread
      auto localhandler = [relayp, event] () { relayp->local_handler (event); };
      relayp->handler_context_.enqueue_mt (new std::function<void()> (localhandler));
    }
  };
};

RemoteHandle::EventConnection
RemoteHandle::__attach__ (const String &eventselector, EventHandlerF handler)
{
  RemoteHandle::EventConnection empty;
  assert_return (*this != NULL, empty);
  assert_return (handler != NULL, empty);
  ExecutionContext *current_thread_execution_context = ExecutionContext::get_current ();
  assert_return (current_thread_execution_context != NULL, empty);
  ImplicitBase *iface = __iface_ptr__().get();
  ExecutionContext &iface_execution_context = iface->__execution_context_mt__();
  auto relay = std::make_shared<EventHandlerRelay> (*current_thread_execution_context, handler, iface_execution_context);
  EventHandlerRelay::RemoteHandler remotehandler = static_cast<EventHandlerRelay::RemoteHandler&> (relay);
  relay->iface_connection_ = remote_callr (*this, &ImplicitBase::__attach__, eventselector, remotehandler);
  std::weak_ptr<EventHandlerRelay> wptr = relay;
  return *static_cast<EventConnection*> (&wptr);
}

TypeHashList
RemoteHandle::__typelist__() const
{
  TypeHashList thl;
  assert_return (*this != NULL, thl);
  thl = remote_callc (*this, &ImplicitBase::__aida_typelist__);
  return thl;
}

bool
RemoteHandle::EventConnection::connected  () const
{
  std::shared_ptr<EventHandlerRelay> relay = this->lock();
  return relay && relay->connected();
}

void
RemoteHandle::EventConnection::disconnect () const
{
  std::shared_ptr<EventHandlerRelay> relay = this->lock();
  if (relay)
    relay->disconnect();
}

// == DetacherHooks ==
void
DetacherHooks::__clear_hooks__ (RemoteHandle *scopedhandle)
{
  while (!connections_.empty())
    {
      HandleEventConnection hcon = connections_.back();
      connections_.pop_back();
      hcon.disconnect();
    }
}

void
DetacherHooks::__swap_hooks__ (DetacherHooks &other)
{
  std::swap (other.connections_, connections_);
}

void
DetacherHooks::__manage_event__ (RemoteHandle *scopedhandle, const String &type, EventHandlerF handler)
{
  assert_return (scopedhandle != NULL);
  assert_return (*scopedhandle != NULL);
  HandleEventConnection hcon = scopedhandle->__attach__ (type, handler);
  connections_.push_back (hcon);
}

void
DetacherHooks::__manage_event__ (RemoteHandle *scopedhandle, const String &type, std::function<void()> vfunc)
{
  __manage_event__ (scopedhandle, type, [vfunc] (const Aida::Event&) { vfunc(); });
}

// == ProtoMsg ==
ProtoMsg::ProtoMsg (uint32 _ntypes) :
  buffermem (NULL)
{
  static_assert (sizeof (ProtoMsg) <= sizeof (ProtoUnion), "sizeof ProtoMsg");
  // buffermem layout: [{n_types,nth}] [{type nibble} * n_types]... [field]...
  const uint _offs = 1 + (_ntypes + 7) / 8;
  buffermem = new ProtoUnion[_offs + _ntypes];
  wmemset ((wchar_t*) buffermem, 0, sizeof (ProtoUnion[_offs]) / sizeof (wchar_t));
  buffermem[0].capacity = _ntypes;
  buffermem[0].index = 0;
}

ProtoMsg::ProtoMsg (uint32 _ntypes, ProtoUnion *_bmem, uint32 _bmemlen) :
  buffermem (_bmem)
{
  const uint32 _offs = 1 + (_ntypes + 7) / 8;
  assert (_bmem && _bmemlen >= sizeof (ProtoUnion[_offs + _ntypes]));
  wmemset ((wchar_t*) buffermem, 0, sizeof (ProtoUnion[_offs]) / sizeof (wchar_t));
  buffermem[0].capacity = _ntypes;
  buffermem[0].index = 0;
}

ProtoMsg::~ProtoMsg()
{
  reset();
  if (buffermem)
    delete [] buffermem;
}

void
ProtoMsg::check_internal ()
{
  AIDA_ASSERT_RETURN (size() <= capacity());
}

void
ProtoMsg::add_string (const String &s)
{
  ProtoUnion &u = addu (STRING);
  u.vstr = new String (s);
}

void
ProtoMsg::add_any (const Any &vany, BaseConnection &bcon)
{
  ProtoUnion &u = addu (ANY);
  u.vany = new Any (vany);
  // BROKEN
}

Any
ProtoReader::pop_any (BaseConnection &bcon)
{
  ProtoUnion &u = fb_popu (ANY);
  Any vany = *u.vany;
  // BROKEN
  return vany;
}

void
ProtoMsg::operator<<= (const Any &vany)
{
  add_any (vany, ProtoScope::current_base_connection());
}

void
ProtoReader::operator>>= (Any &vany)
{
  vany = pop_any (ProtoScope::current_base_connection());
}

void
ProtoMsg::operator<<= (const RemoteHandle &rhandle)
{
  ProtoScope::current_client_connection().add_handle (*this, rhandle);
}

void
ProtoReader::operator>>= (RemoteHandle &rhandle)
{
  ProtoScope::current_client_connection().pop_handle (*this, rhandle);
}

void
ProtoMsg::operator<<= (ImplicitBase *instance)
{
  ProtoScope::current_server_connection().add_interface (*this, instance ? instance->shared_from_this() : ImplicitBaseP());
}

ImplicitBaseP
ProtoReader::pop_interface ()
{
  return ProtoScope::current_server_connection().pop_interface (*this);
}

void
ProtoReader::check_request (int type)
{
  AIDA_ASSERT_RETURN (nth_ < n_types());
  AIDA_ASSERT_RETURN (get_type() == type);
}

uint64
ProtoReader::debug_bits ()
{
  return fb_->upeek (nth_).vint64;
}

std::string
ProtoMsg::first_id_str() const
{
  uint64 fid = first_id();
  return posix_sprintf ("%016llx", LLU fid);
}

static std::string
strescape (const std::string &str)
{
  std::string buffer;
  for (std::string::const_iterator it = str.begin(); it != str.end(); it++)
    {
      uint8_t d = *it;
      if (d < 32 || d > 126 || d == '?')
        buffer += posix_sprintf ("\\%03o", d);
      else if (d == '\\')
        buffer += "\\\\";
      else if (d == '"')
        buffer += "\\\"";
      else
        buffer += d;
    }
  return buffer;
}

std::string
ProtoMsg::type_name (int field_type)
{
  const char *tkn = type_kind_name (TypeKind (field_type));
  if (tkn)
    return tkn;
  return posix_sprintf ("<invalid:%d>", field_type);
}

std::string
ProtoMsg::to_string() const
{
  String s = posix_sprintf ("Aida::ProtoMsg(%p)={", this);
  s += posix_sprintf ("size=%u, capacity=%u", size(), capacity());
  ProtoReader fbr (*this);
  for (size_t i = 0; i < size(); i++)
    {
      const String tname = type_name (fbr.get_type());
      const char *tn = tname.c_str();
      switch (fbr.get_type())
        {
        case UNTYPED:
        case VOID:       s += posix_sprintf (", %s", tn); fbr.skip();                               break;
        case BOOL:       s += posix_sprintf (", %s: 0x%llx", tn, LLU fbr.pop_bool());               break;
        case ENUM:       s += posix_sprintf (", %s: 0x%llx", tn, LLU fbr.pop_evalue());             break;
        case INT32:      s += posix_sprintf (", %s: 0x%08llx", tn, LLU fbr.pop_int64());            break;
        case INT64:      s += posix_sprintf (", %s: 0x%016llx", tn, LLU fbr.pop_int64());           break;
        case FLOAT64:    s += posix_sprintf (", %s: %.17g", tn, fbr.pop_double());                  break;
        case STRING:     s += posix_sprintf (", %s: %s", tn, strescape (fbr.pop_string()).c_str()); break;
        case SEQUENCE:   s += posix_sprintf (", %s: %p", tn, &fbr.pop_seq());                       break;
        case RECORD:     s += posix_sprintf (", %s: %p", tn, &fbr.pop_rec());                       break;
        case TRANSITION: s += posix_sprintf (", %s: %p", tn, (void*) fbr.debug_bits()); fbr.skip(); break;
        case ANY:        s += posix_sprintf (", %s: %p", tn, (void*) fbr.debug_bits()); fbr.skip(); break;
        default:         s += posix_sprintf (", <unknown:%u>: %p", fbr.get_type(), (void*) fbr.debug_bits()); fbr.skip(); break;
        }
    }
  s += '}';
  return s;
}

#if 0
ProtoMsg*
ProtoMsg::new_error (const String &msg,
                        const String &domain)
{
  ProtoMsg *fr = ProtoMsg::_new (3 + 2);
  fr->add_header1 (MSGID_ERROR, 0, 0);
  fr->add_string (msg);
  fr->add_string (domain);
  return fr;
}
#endif

ProtoMsg*
ProtoMsg::new_result (MessageId m, uint64 h, uint64 l, uint32 n)
{
  ProtoMsg *fr = ProtoMsg::_new (3 + n);
  fr->add_header1 (m, h, l);
  return fr;
}

ProtoMsg*
ProtoMsg::renew_into_result (ProtoMsg *fb, MessageId m, uint64 h, uint64 l, uint32 n)
{
  if (fb->capacity() < 3 + n)
    return ProtoMsg::new_result (m, h, l, n);
  ProtoMsg *fr = fb;
  fr->reset();
  fr->add_header1 (m, h, l);
  return fr;
}

ProtoMsg*
ProtoMsg::renew_into_result (ProtoReader &fbr, MessageId m, uint64 h, uint64 l, uint32 n)
{
  ProtoMsg *fb = const_cast<ProtoMsg*> (fbr.proto_msg());
  fbr.reset();
  return renew_into_result (fb, m, h, l, n);
}

class ContiguousProtoMsg : public ProtoMsg {
  virtual
  ~ContiguousProtoMsg () override
  {
    reset();
    buffermem = NULL;
  }
  ContiguousProtoMsg (uint32 _ntypes, ProtoUnion *_bmem, uint32 _bmemlen) :
    ProtoMsg (_ntypes, _bmem, _bmemlen)
  {}
public:
  static ContiguousProtoMsg*
  _new (uint32 _ntypes)
  {
    const uint32 _offs = 1 + (_ntypes + 7) / 8;
    const size_t bmemlen = sizeof (ProtoUnion[_offs + _ntypes]);
    const size_t objlen = 8 * ((sizeof (ContiguousProtoMsg) + 7) / 8);
    uint8_t *omem = (uint8_t*) operator new (objlen + bmemlen);
    ProtoUnion *bmem = (ProtoUnion*) (omem + objlen);
    return new (omem) ContiguousProtoMsg (_ntypes, bmem, bmemlen);
  }
};

ProtoMsg*
ProtoMsg::_new (uint32 _ntypes)
{
  return ContiguousProtoMsg::_new (_ntypes);
}

// == ProtoScope ==
struct ProtoConnections {
  ServerConnection *server_connection;
  ClientConnection *client_connection;
  constexpr ProtoConnections() : server_connection (NULL), client_connection (NULL) {}
};
static __thread ProtoConnections current_thread_proto_connections;

ProtoScope::ProtoScope (ClientConnection &client_connection) :
  nested_ (false)
{
  assert (&client_connection);
  if (&client_connection == current_thread_proto_connections.client_connection &&
      NULL == current_thread_proto_connections.server_connection)
    nested_ = true;
  else
    {
      assert (current_thread_proto_connections.server_connection == NULL);
      assert (current_thread_proto_connections.client_connection == NULL);
      current_thread_proto_connections.client_connection = &client_connection;
      current_thread_proto_connections.server_connection = NULL;
    }
}

ProtoScope::ProtoScope (ServerConnection &server_connection) :
  nested_ (false)
{
  assert (&server_connection);
  if (&server_connection == current_thread_proto_connections.server_connection &&
      NULL == current_thread_proto_connections.client_connection)
    nested_ = true;
  else
    {
      assert (current_thread_proto_connections.server_connection == NULL);
      assert (current_thread_proto_connections.client_connection == NULL);
      current_thread_proto_connections.server_connection = &server_connection;
      current_thread_proto_connections.client_connection = NULL;
    }
}

ProtoScope::~ProtoScope ()
{
  if (!current_thread_proto_connections.client_connection)
    assert (current_thread_proto_connections.server_connection != NULL);
  if (!current_thread_proto_connections.server_connection)
    assert (current_thread_proto_connections.client_connection != NULL);
  if (!nested_)
    {
      current_thread_proto_connections.server_connection = NULL;
      current_thread_proto_connections.client_connection = NULL;
    }
}

ProtoMsg*
ProtoScope::invoke (ProtoMsg *pm)
{
  return current_client_connection().call_remote (pm);
}

void
ProtoScope::post_peer_msg (ProtoMsg *pm)
{
  return current_server_connection().post_peer_msg (pm);
}

ClientConnection&
ProtoScope::current_client_connection ()
{
  assert (current_thread_proto_connections.client_connection != NULL);
  return *current_thread_proto_connections.client_connection;
}

ServerConnection&
ProtoScope::current_server_connection ()
{
  assert (current_thread_proto_connections.server_connection != NULL);
  return *current_thread_proto_connections.server_connection;
}

BaseConnection&
ProtoScope::current_base_connection ()
{
  assert (current_thread_proto_connections.server_connection || current_thread_proto_connections.client_connection);
  if (current_thread_proto_connections.server_connection)
    return *current_thread_proto_connections.server_connection;
  else
    return *current_thread_proto_connections.client_connection;
}

// for debugging purposes, assert early that RemoteHandle is non-NULL
static inline const RemoteHandle&
assert_remote_handle (const RemoteHandle &remote_handle)
{
  assert (remote_handle != NULL);
  return remote_handle;
}

ProtoScopeCall1Way::ProtoScopeCall1Way (ProtoMsg &pm, const RemoteHandle &rhandle, uint64 hashi, uint64 hashlo) :
  ProtoScope (*assert_remote_handle (rhandle).__aida_connection__())
{
  pm.add_header2 (MSGID_CALL_ONEWAY, hashi, hashlo);
  pm <<= rhandle;
}

ProtoScopeCall2Way::ProtoScopeCall2Way (ProtoMsg &pm, const RemoteHandle &rhandle, uint64 hashi, uint64 hashlo) :
  ProtoScope (*assert_remote_handle (rhandle).__aida_connection__())
{
  pm.add_header2 (MSGID_CALL_TWOWAY, hashi, hashlo);
  pm <<= rhandle;
}

ProtoScopeEmit1Way::ProtoScopeEmit1Way (ProtoMsg &pm, ServerConnection &server_connection, uint64 hashi, uint64 hashlo) :
  ProtoScope (server_connection)
{
  pm.add_header1 (MSGID_EMIT_ONEWAY, hashi, hashlo);
}

ProtoScopeDisconnect::ProtoScopeDisconnect (ProtoMsg &pm, ServerConnection &server_connection, uint64 hashi, uint64 hashlo) :
  ProtoScope (server_connection)
{
  pm.add_header1 (MSGID_DISCONNECT, hashi, hashlo);
}

// == EventFd ==
EventFd::EventFd () :
  fds { -1, -1 }
{}

int
EventFd::open ()
{
  if (opened())
    return 0;
  long nflags;
#ifdef HAVE_SYS_EVENTFD_H
  do
    fds[0] = eventfd (0 /*initval*/, EFD_CLOEXEC | EFD_NONBLOCK);
  while (fds[0] < 0 && (errno == EAGAIN || errno == EINTR));
#else
  int err;
  do
    err = pipe2 (fds, O_CLOEXEC | O_NONBLOCK);
  while (err < 0 && (errno == EAGAIN || errno == EINTR));
  if (fds[1] >= 0)
    {
      nflags = fcntl (fds[1], F_GETFL, 0);
      assert (nflags & O_NONBLOCK);
      nflags = fcntl (fds[1], F_GETFD, 0);
      assert (nflags & FD_CLOEXEC);
    }
#endif
  if (fds[0] >= 0)
    {
      nflags = fcntl (fds[0], F_GETFL, 0);
      assert (nflags & O_NONBLOCK);
      nflags = fcntl (fds[0], F_GETFD, 0);
      assert (nflags & FD_CLOEXEC);
      return 0;
    }
  return -errno;
}

int
EventFd::inputfd () // fd for POLLIN
{
  return fds[0];
}

bool
EventFd::opened ()
{
  return inputfd() >= 0;
}

bool
EventFd::pollin ()
{
  struct pollfd pfd = { inputfd(), POLLIN, 0 };
  int presult;
  do
    presult = poll (&pfd, 1, -1);
  while (presult < 0 && (errno == EAGAIN || errno == EINTR));
  return pfd.revents != 0;
}

void
EventFd::wakeup()
{
  int err;
#ifdef HAVE_SYS_EVENTFD_H
  do
    err = eventfd_write (fds[0], 1);
  while (err < 0 && errno == EINTR);
#else
  char w = 'w';
  do
    err = write (fds[1], &w, 1);
  while (err < 0 && errno == EINTR);
#endif
  // EAGAIN occours if too many wakeups are pending
}

void
EventFd::flush()
{
  int err;
#ifdef HAVE_SYS_EVENTFD_H
  eventfd_t bytes8;
  do
    err = eventfd_read (fds[0], &bytes8);
  while (err < 0 && errno == EINTR);
#else
  char buffer[512]; // 512 is POSIX pipe atomic read/write size
  do
    err = read (fds[0], buffer, sizeof (buffer));
  while (err == 512 || (err < 0 && errno == EINTR));
#endif
  // EAGAIN occours if no wakeups are pending
}

EventFd::~EventFd ()
{
#ifdef HAVE_SYS_EVENTFD_H
  close (fds[0]);
#else
  close (fds[0]);
  close (fds[1]);
#endif
  fds[0] = -1;
  fds[1] = -1;
}

// == ScopedSemaphore ==
ScopedSemaphore::ScopedSemaphore () noexcept
{
  static_assert (sizeof (mem_) >= sizeof (sem_t), "");
  sem_t &sem = *(sem_t*) mem_;
  const int ret = sem_init (&sem, 0 /*pshared*/, 0 /*value*/);
  assert_return (ret == 0);
}

int
ScopedSemaphore::post () noexcept
{
  sem_t &sem = *(sem_t*) mem_;
  errno = 0;
  const int ret = sem_post (&sem);
  return ret ? errno : 0;
}

int
ScopedSemaphore::wait () noexcept
{
  sem_t &sem = *(sem_t*) mem_;
  errno = 0;
  const int ret = sem_wait (&sem);
  return ret ? errno : 0;
}

ScopedSemaphore::~ScopedSemaphore () noexcept
{
  sem_t &sem = *(sem_t*) mem_;
  const int ret = sem_destroy (&sem);
  assert_return (ret == 0);
}

// == lock-free, single-consumer queue ==
template<class Data>
struct MpScQueueF {
  struct Node {
    Node *next = NULL;
    Data data = Data();
  };
  bool
  push (Data data)
  {
    Node *node = new Node { NULL, data };
    Node *last_head;
    do
      node->next = last_head = head_;
    while (!__sync_bool_compare_and_swap (&head_, node->next, node));
    return last_head == NULL; // was empty
  }
  Data
  pop()
  {
    Node *node = pop_node();
    if (node)
      {
        Data d = node->data;
        delete node;
        return d;
      }
    else
      return Data();
  }
protected:
  Node*
  pop_node()
  {
    if (AIDA_UNLIKELY (!local_))
      {
        Node *prev, *next, *node;
        do
          node = head_;
        while (!__sync_bool_compare_and_swap (&head_, node, NULL));
        for (prev = NULL; node; node = next)
          {
            next = node->next;
            node->next = prev;
            prev = node;
          }
        local_ = prev;
      }
    if (local_)
      {
        Node *node = local_;
        local_ = node->next;
        return node;
      }
    else
      return NULL;
  }
private:
  Node  *head_ = NULL;
  Node  *local_ = NULL; // FIXME: ideally use a different cache line to avoid false sharing between pushing and popping threads
};

// == ExecutionContextImpl ==
struct ExecutionContext::Impl {
  EventFd              event_fd_;
private:
  MpScQueueF<Closure*> closure_queue_;
  Closure             *last_closure_ = NULL;
  enum Op { PEEK, POP, BLOCKING };
  Closure*
  get_closure (const Op op)
  {
    if (!last_closure_)
      do
        {
          // fetch new messages
          last_closure_ = closure_queue_.pop();
          if (!last_closure_)
            {
              event_fd_.flush();        // flush stale wakeups, to allow blocking until an empty => full transition
              last_closure_ = closure_queue_.pop();     // retry, to ensure we've not just discarded a real wakeup
            }
          if (last_closure_)
            break;
          // no messages available
          if (op == BLOCKING)
            event_fd_.pollin();
        }
      while (op == BLOCKING);
    Closure *closure = last_closure_;
    if (op != PEEK) // advance
      last_closure_ = NULL;
    return closure; // may be NULL
  }
public:
  void
  enqueue_closure_mt (Closure *closure, bool may_wakeup)
  {
    const bool was_empty = closure_queue_.push (closure);
    if (may_wakeup && was_empty)
      event_fd_.wakeup();                               // wakeups are needed to catch empty => full transition
  }
  Closure*              fetch_closure()         { return get_closure (POP); }
  bool                  has_closure()           { return get_closure (PEEK); }
  Closure*              pop_closure()           { return get_closure (BLOCKING); }
  Impl ()
  {
    const int create_wakeup_pipe_error = event_fd_.open();
    AIDA_ASSERT_RETURN (create_wakeup_pipe_error == 0);
  }
};

// == ExecutionContext ==
ExecutionContext::ExecutionContext() :
  m (*new Impl)
{}

ExecutionContext::~ExecutionContext()
{
  AIDA_ASSERT_RETURN_UNREACHED();
  delete &m;
}

/// Returns fd for POLLIN, to wake up on incomming events.
int
ExecutionContext::notify_fd ()
{
  return m.event_fd_.inputfd();
}

/// Indicate whether any incoming events are pending that need to be dispatched.
bool
ExecutionContext::pending ()
{
  return m.has_closure();
}

/// Dispatch a single event if any is pending.
void
ExecutionContext::dispatch ()
{
  Closure *closure = m.fetch_closure();
  if (AIDA_ISLIKELY (closure))
    {
      (*closure) ();
      delete closure;
    }
}

/// Add Closure to ExecutionContext and delete the closure after dispatching.
void
ExecutionContext::enqueue_mt (Closure *closure)
{
  if (closure)
    m.enqueue_closure_mt (closure, true);
}

/// Add Closure to be called by ExecutionContext (possibly remote).
void
ExecutionContext::enqueue_mt (const Closure &closure)
{
  enqueue_mt (new Closure (closure));
}

/// Create an ExecutionContext.
/// The current implementation of this instance must outlive the runtime and not be destroyed.
ExecutionContext*
ExecutionContext::new_context ()
{
  return new ExecutionContext();
}

static inline std::vector<ExecutionContext*>&
get_execution_context_stack()
{
  static thread_local std::vector<ExecutionContext*> ecstack;
  return ecstack;
}

/// Start using `ec` as active ExecutionContext in the current thread.
void
ExecutionContext::push_thread_current()
{
  auto &ecstack = get_execution_context_stack();
  ecstack.push_back (this);
}

/// Remove the active ExecutionContext from the current thread, after that a previously pushed context becomes active.
void
ExecutionContext::pop_thread_current()
{
  auto &ecstack = get_execution_context_stack();
  assert_return (ecstack.size() > 0);
  ecstack.pop_back();
}

/// Retrieve the active ExecutionContext from the current thread.
ExecutionContext*
ExecutionContext::get_current ()
{
  auto &ecstack = get_execution_context_stack();
  return ecstack.size() ? ecstack.back() : NULL;
}

struct ExecutionContextAdoptedDeleter {
  ExecutionContext &ec_;
  CallableIfaceP    sptr_;
  ExecutionContextAdoptedDeleter (ExecutionContext &ec, const CallableIfaceP &sptr) :
    ec_ (ec), sptr_ (sptr)
  {}
  static void
  deleter (ExecutionContextAdoptedDeleter *self)
  {
    AIDA_DEBUG ("Deleter: %s: queueing deleter for %p\n", __func__, self->sptr_.get());
    self->ec_.enqueue_mt (new std::function<void()> ([self] () {
          AIDA_DEBUG ("Deleter: %s: delete %p\n", __func__, self->sptr_.get());
          delete self;
        }));
  }
};

CallableIfaceP
ExecutionContext::adopt_deleter_mt (const CallableIfaceP &sharedptr)
{
  if (!sharedptr.get())
    return sharedptr;
  auto delp = std::get_deleter<void(*)(ExecutionContextAdoptedDeleter*)> (sharedptr);
  if (delp && ExecutionContextAdoptedDeleter::deleter == *delp)
    return sharedptr;
  // create new shared_ptr that's deleted in this ExecutionContext
  ExecutionContextAdoptedDeleter *ad = new ExecutionContextAdoptedDeleter (*this, sharedptr);
  auto adp = std::shared_ptr<ExecutionContextAdoptedDeleter> (ad, ExecutionContextAdoptedDeleter::deleter);
  assert_return (ExecutionContextAdoptedDeleter::deleter == *std::get_deleter<void(*)(ExecutionContextAdoptedDeleter*)> (adp), CallableIfaceP());
  // use aliasing shared_ptr to yield CallableIface* wihle deleting via adp
  auto remp = CallableIfaceP (adp, sharedptr.get());
  assert_return (ExecutionContextAdoptedDeleter::deleter == *std::get_deleter<void(*)(ExecutionContextAdoptedDeleter*)> (remp), CallableIfaceP());
  AIDA_DEBUG ("Deleter: %s: adopted deleter for %p\n", __func__, sharedptr.get());
  return remp;
}

/// Create a GLib event loop source for this ExecutionContext.
GSource*
ExecutionContext::create_gsource (const std::string &name, int priority)
{
  const int fds[] = { this->notify_fd() };
  struct CxxSource : GSource {
    Aida::ExecutionContext *ec;
  };
  auto prepare = [] (GSource *source, gint *timeout_p) -> gboolean {
    CxxSource &cs = *(CxxSource*) source;
    return cs.ec->pending();
  };
  auto check = [] (GSource *source) -> gboolean {
    CxxSource &cs = *(CxxSource*) source;
    return cs.ec->pending();
  };
  auto dispatch = [] (GSource *source, GSourceFunc callback, gpointer user_data) -> gboolean {
    CxxSource &cs = *(CxxSource*) source;
    cs.ec->dispatch();
    return true; // keep alive
  };
  auto finalize = [] (GSource *source) {
    // CxxSource &cs = *(CxxSource*) source;
    // this handler can be called from g_source_remove, or dispatch(), with or w/o main loop mutex... ;-(
    // delete cs.ec;
  };
  static GSourceFuncs cxx_source_funcs = { prepare, check, dispatch, finalize };
  GSource *source = g_source_new (&cxx_source_funcs, sizeof (CxxSource));
  CxxSource &cs = *(CxxSource*) source;
  cs.ec = this;
  g_source_set_name (source, name.c_str());
  g_source_set_priority (source, priority);
  for (size_t i = 0; i < sizeof (fds) / sizeof (fds[0]); i++)
    g_source_add_unix_fd (source, fds[i], G_IO_IN);
  return source;
}

// == EventDispatcher ==
struct EventDispatcher::ConnectionImpl final {
  const std::string  selector_;
  EventHandlerF      handler_;
  DispatcherImpl    &o_;
  ConnectionImpl (DispatcherImpl &edispatcher, const String &eventselector, EventHandlerF handler) :
    selector_ (eventselector), handler_ (handler), o_ (edispatcher)
  {}
  bool          connected       () const  { return NULL != handler_; }
  void          disconnect      ();
  void
  emit (const Event &event, const std::string &event_type, const std::string &general_type)
  {
    if (connected() &&
        (selector_ == event_type || selector_ == general_type))
      handler_ (event);
  }
};

struct EventDispatcher::DispatcherImpl final {
  using EventConnectionP = std::shared_ptr<ConnectionImpl>;
  std::vector<EventConnectionP> connections;
  uint                          in_emission = 0;
  bool                          needs_purging = false;
  void
  purge_connections ()
  {
    if (in_emission)
      {
        needs_purging = true;
        return;
      }
    needs_purging = false;
    for (size_t i = connections.size() - 1; i < connections.size(); i--)
      if (!connections[i]->connected())
        connections.erase (connections.begin() + i);
  }
  void
  emit (const Event &event)
  {
    const std::string event_type = event["type"].get<std::string>();
    if (event_type.empty())
      return;
    in_emission++;
    {
      const char *const ctype = event_type.c_str(), *const colon = strchr (ctype, ':');
      const std::string general_type = colon ? event_type.substr (0, colon - ctype) : event_type;
      for (auto &conp : connections)
        conp->emit (event, event_type, general_type);
    }
    in_emission--;
    if (in_emission == 0 && needs_purging)
      purge_connections();
  }
  ~DispatcherImpl()
  {
    assert_return (in_emission == 0);
    in_emission++; // short cut purge_connections() calls
    for (auto &conp : connections)
      conp->disconnect();
    in_emission--;
  }
};

void
EventDispatcher::ConnectionImpl::disconnect()
{
  const bool was_connected = connected();
  handler_ = NULL;
  if (was_connected)
    o_.purge_connections();
}

bool
EventDispatcher::EventConnection::connected () const
{
  std::shared_ptr<ConnectionImpl> con = this->lock();
  return con && con->connected();
}

void
EventDispatcher::EventConnection::disconnect () const
{
  std::shared_ptr<ConnectionImpl> con = this->lock();
  if (con)
    con->disconnect();
}

EventDispatcher::~EventDispatcher ()
{
  reset();
}

void
EventDispatcher::reset()
{
  DispatcherImpl *old = o_;
  o_ = NULL;
  delete old;
}

EventDispatcher::EventConnection
EventDispatcher::attach (const String &eventselector, EventHandlerF handler)
{
  if (!o_)
    o_ = new DispatcherImpl();
  o_->connections.push_back (std::make_shared<EventDispatcher::ConnectionImpl> (*o_, eventselector, handler));
  std::weak_ptr<EventDispatcher::ConnectionImpl> wptr = o_->connections.back();
  return *static_cast<EventConnection*> (&wptr);
}

void
EventDispatcher::emit (const Event &event)
{
  if (o_)
    o_->emit (event);
}

// == CallableIface ==
CallableIface::~CallableIface ()
{}

// == PropertyAccessor ==
PropertyAccessor::~PropertyAccessor()
{}

// == TransportChannel ==
class TransportChannel : public EventFd { // Channel for cross-thread ProtoMsg IO
  MpScQueueF<ProtoMsg*> msg_queue;
  ProtoMsg             *last_fb;
  enum Op { PEEK, POP, POP_BLOCKED };
  ProtoMsg*
  get_msg (const Op op)
  {
    if (!last_fb)
      do
        {
          // fetch new messages
          last_fb = msg_queue.pop();
          if (!last_fb)
            {
              flush();                          // flush stale wakeups, to allow blocking until an empty => full transition
              last_fb = msg_queue.pop();        // retry, to ensure we've not just discarded a real wakeup
            }
          if (last_fb)
            break;
          // no messages available
          if (op == POP_BLOCKED)
            pollin();
        }
      while (op == POP_BLOCKED);
    ProtoMsg *fb = last_fb;
    if (op != PEEK) // advance
      last_fb = NULL;
    return fb; // may be NULL
  }
public:
  void // takes pm ownership
  send_msg (ProtoMsg *pm, bool may_wakeup)
  {
    const bool was_empty = msg_queue.push (pm);
    if (may_wakeup && was_empty)
      wakeup();                                 // wakeups are needed to catch empty => full transition
  }
  ProtoMsg*  fetch_msg()     { return get_msg (POP); }
  bool       has_msg()       { return get_msg (PEEK); }
  ProtoMsg*  pop_msg()       { return get_msg (POP_BLOCKED); }
  ~TransportChannel ()
  {}
  TransportChannel () :
    last_fb (NULL)
  {
    const int create_wakeup_pipe_error = open();
    AIDA_ASSERT_RETURN (create_wakeup_pipe_error == 0);
  }
};

// == ObjectMap ==
template<class Instance>
class ObjectMap {
public:
  typedef std::shared_ptr<Instance>     InstanceP;
private:
  struct Entry {
    OrbObjectW  orbow;
    InstanceP   instancep;
  };
  uint64                                start_id_, id_mask_;
  std::vector<Entry>                    entries_;
  std::unordered_map<Instance*, uint64> map_;
  std::vector<uint>                     free_list_;
  class MappedObject : public virtual OrbObject {
    ObjectMap &omap_;
  public:
    explicit MappedObject (uint64 orbid, ObjectMap &omap) : OrbObject (orbid), omap_ (omap) { assert (orbid); }
    virtual ~MappedObject ()                              { omap_.delete_orbid (orbid()); }
  };
  void          delete_orbid            (uint64            orbid);
  uint          next_index              ();
public:
  explicit   ObjectMap          (size_t            start_id = 0) : start_id_ (start_id), id_mask_ (0xffffffffffffffff) {}
  /*dtor*/  ~ObjectMap          ()                 { assert (entries_.size() == 0); assert (map_.size() == 0); }
  OrbObjectP orbo_from_instance (InstanceP         instancep);
  InstanceP  instance_from_orbo (const OrbObjectP &orbo);
  OrbObjectP orbo_from_orbid    (uint64            orbid);
  void       assign_start_id    (uint64 start_id, uint64 mask = 0xffffffffffffffff);
};

template<class Instance> void
ObjectMap<Instance>::assign_start_id (uint64 start_id, uint64 id_mask)
{
  assert (entries_.size() == 0);
  assert ((start_id_ & id_mask) == start_id_);
  start_id_ = start_id;
  assert (id_mask > 0);
  id_mask_ = id_mask;
  assert (map_.size() == 0);
}

template<class Instance> void
ObjectMap<Instance>::delete_orbid (uint64 orbid)
{
  assert ((orbid & id_mask_) >= start_id_);
  const uint64 index = (orbid & id_mask_) - start_id_;
  assert (index < entries_.size());
  Entry &e = entries_[index];
  assert (e.orbow.expired());   // ensure last OrbObjectP reference has been dropped
  assert (e.instancep != NULL); // ensure *first* deletion attempt for this entry
  auto it = map_.find (e.instancep.get());
  assert (it != map_.end());
  map_.erase (it);
  e.instancep.reset();
  e.orbow.reset();
  free_list_.push_back (index);
}

template<class Instance> uint
ObjectMap<Instance>::next_index ()
{
  uint idx;
  const size_t FREE_LENGTH = 31;
  if (free_list_.size() > FREE_LENGTH)
    {
      const size_t prandom = fnv1a_bytehash64 ((uint8*) free_list_.data(), sizeof (*free_list_.data()) * free_list_.size());
      const size_t end = free_list_.size(), j = prandom % (end - 1);
      assert (j < end - 1); // use end-1 to avoid popping the last pushed slot
      idx = free_list_[j];
      free_list_[j] = free_list_[end - 1];
      free_list_.pop_back();
    }
  else
    {
      idx = entries_.size();
      entries_.resize (idx + 1);
    }
  return idx;
}

template<class Instance> OrbObjectP
ObjectMap<Instance>::orbo_from_instance (InstanceP instancep)
{
  OrbObjectP orbop;
  if (instancep)
    {
      uint64 orbid = map_[instancep.get()];
      if (AIDA_UNLIKELY (orbid == 0))
        {
          const uint64 index = next_index();
          orbid = start_id_ + index;
          orbop = std::make_shared<MappedObject> (orbid, *this); // calls delete_orbid from dtor
          Entry e { orbop, instancep };
          entries_[index] = e;
          map_[instancep.get()] = orbid;
        }
      else
        orbop = entries_[(orbid & id_mask_) - start_id_].orbow.lock();
    }
  return orbop;
}

template<class Instance> OrbObjectP
ObjectMap<Instance>::orbo_from_orbid (uint64 orbid)
{
  assert ((orbid & id_mask_) >= start_id_);
  const uint64 index = (orbid & id_mask_) - start_id_;
  if (index < entries_.size() && entries_[index].instancep) // check for deletion
    return entries_[index].orbow.lock();
  return OrbObjectP();
}

template<class Instance> std::shared_ptr<Instance>
ObjectMap<Instance>::instance_from_orbo (const OrbObjectP &orbo)
{
  const uint64 orbid = orbo ? orbo->orbid() : 0;
  if ((orbid & id_mask_) >= start_id_)
    {
      const uint64 index = (orbid & id_mask_) - start_id_;
      if (index < entries_.size())
        return entries_[index].instancep;
    }
  return NULL;
}

// == ConnectionRegistry ==
class ConnectionRegistry {
  std::mutex                   mutex_;
  std::vector<BaseConnection*> connections_;
public:
  void
  register_connection (BaseConnection &connection)
  {
    std::lock_guard<std::mutex> locker (mutex_);
    size_t i;
    for (i = 0; i < connections_.size(); i++)
      if (!connections_[i])
        break;
    if (i == connections_.size())
      connections_.resize (i + 1);
    connections_[i] = &connection;
  }
  void
  unregister_connection (BaseConnection &connection)
  {
    std::lock_guard<std::mutex> locker (mutex_);
    bool connection_found_and_unregistered = false;
    for (size_t i = 0; i < connections_.size(); i++)
      if (connections_[i] == &connection)
        {
          connections_[i] = NULL;
          connection_found_and_unregistered = true;
          break;
        }
    AIDA_ASSERT_RETURN (connection_found_and_unregistered);
  }
  ServerConnection*
  server_connection_from_protocol (const String &protocol)
  {
    std::lock_guard<std::mutex> locker (mutex_);
    for (size_t i = 0; i < connections_.size(); i++)
      {
        BaseConnection *bcon = connections_[i];
        if (bcon && protocol == bcon->protocol())
          {
            ServerConnection *scon = dynamic_cast<ServerConnection*> (bcon);
            if (scon)
              return scon;
          }
      }
    return NULL; // unmatched
  }
};
static DurableInstance<ConnectionRegistry> connection_registry; // keep ConnectionRegistry across static dtors

// == BaseConnection ==
BaseConnection::BaseConnection (const std::string &protocol) :
  protocol_ (protocol), peer_ (NULL)
{
  AIDA_ASSERT_RETURN (protocol.size() > 0);
  if (protocol_[0] == ':')
    AIDA_ASSERT_RETURN (protocol_[protocol_.size()-1] == ':');
  else
    AIDA_ASSERT_RETURN (string_startswith (protocol, "inproc://"));
}

BaseConnection::~BaseConnection ()
{}

void
BaseConnection::post_peer_msg (ProtoMsg *pm)
{
  AIDA_ASSERT_RETURN (pm != NULL);
#if 0
  {
    ProtoReader fbr (*pm);
    const uint64 msgid = fbr.pop_int64(), hashhigh = fbr.pop_int64(), hashlow = fbr.pop_int64();
    AIDA_MESSAGE ("orig=%p dest=%p msgid=%016x h=%016x l=%016x", this, &peer_connection(), msgid, hashhigh, hashlow);
  }
#endif
  peer_connection().receive_msg (pm);
}

BaseConnection&
BaseConnection::peer_connection () const
{
  assert (has_peer());
  return *peer_;
}

void
BaseConnection::peer_connection (BaseConnection &peer)
{
  assert (has_peer() == false);
  peer_ = &peer;
}

bool
BaseConnection::has_peer () const
{
  return peer_ != NULL;
}

/// Provide initial handle for remote connections.
void
BaseConnection::remote_origin (ImplicitBaseP)
{
  AIDA_ASSERT_RETURN_UNREACHED();
}

RemoteHandle
BaseConnection::remote_origin()
{
  AIDA_ASSERT_RETURN_UNREACHED (RemoteHandle());
}

// == ClientConnectionImpl ==
class ClientConnectionImpl : public ClientConnection {
  struct SignalHandler {
    uint64 hhi, hlo, cid;
    RemoteMember<RemoteHandle> remote;
    void *data;
  };
  typedef std::set<uint64> UIntSet;
  pthread_spinlock_t            signal_spin_;
  TransportChannel              transport_channel_;     // messages arriving at client
  sem_t                         transport_sem_;         // signal incomming results
  std::deque<ProtoMsg*>         event_queue_;           // messages pending for client
  typedef std::map<uint64, OrbObjectW> Id2OrboMap;
  Id2OrboMap                    id2orbo_map_;           // map server orbid -> OrbObjectP
  std::vector<SignalHandler*>   signal_handlers_;
  UIntSet                       ehandler_set; // client event handler
  std::function<void (ClientConnection&)> notify_cb_;
  bool                          blocking_for_sem_;
  bool                          seen_garbage_;
  SignalHandler*                signal_lookup (size_t handler_id);
public:
  ClientConnectionImpl (const std::string &protocol, ServerConnection &server_connection) :
    ClientConnection (protocol), blocking_for_sem_ (false), seen_garbage_ (false)
  {
    assert (!server_connection.has_peer());
    signal_handlers_.push_back (NULL); // reserve 0 for NULL
    pthread_spin_init (&signal_spin_, 0 /* pshared */);
    sem_init (&transport_sem_, 0 /* unshared */, 0 /* init */);
    connection_registry->register_connection (*this);
    peer_connection (server_connection);
  }
  ~ClientConnectionImpl ()
  {
    connection_registry->unregister_connection (*this);
    sem_destroy (&transport_sem_);
    pthread_spin_destroy (&signal_spin_);
    AIDA_ASSERT_RETURN (! "~ClientConnectionImpl not properly implemented");
  }
  virtual void
  receive_msg (ProtoMsg *fb) override
  {
    AIDA_ASSERT_RETURN (fb);
    transport_channel_.send_msg (fb, !blocking_for_sem_);
    notify_for_result();
  }
  void                 notify_for_result ()             { if (blocking_for_sem_) sem_post (&transport_sem_); }
  void                 block_for_result  ()             { AIDA_ASSERT_RETURN (blocking_for_sem_); sem_wait (&transport_sem_); }
  void                 gc_sweep          (const ProtoMsg *fb);
  virtual int          notify_fd         () override    { return transport_channel_.inputfd(); }
  virtual bool         pending           () override    { return !event_queue_.empty() || transport_channel_.has_msg(); }
  virtual ProtoMsg*    call_remote       (ProtoMsg*) override;
  ProtoMsg*            pop               ();
  virtual void         dispatch          () override;
  virtual void         add_handle        (ProtoMsg &fb, const RemoteHandle &rhandle) override;
  virtual void         pop_handle        (ProtoReader &fr, RemoteHandle &rhandle) override;
  virtual void         notify_callback   (const std::function<void (ClientConnection&)> &cb) override;
  virtual void         remote_origin     (ImplicitBaseP rorigin) override  { AIDA_ASSERT_RETURN_UNREACHED(); }
  virtual RemoteHandle remote_origin     () override;
  class ClientOrbObject;
  void
  client_orb_object_deleting (ClientOrbObject &coo)
  {
    if (!seen_garbage_)
      {
        seen_garbage_ = true;
        ProtoMsg *fb = ProtoMsg::_new (3);
        fb->add_header1 (MSGID_META_SEEN_GARBAGE, 0, 0);
        AIDA_DEBUG ("GC: ClientConnectionImpl: SEEN_GARBAGE (%016llx)", LLU coo.orbid());
        ProtoMsg *fr = this->call_remote (fb); // takes over fb
        assert (fr == NULL);
      }
  }
  class ClientOrbObject : public OrbObject {
    ClientConnectionImpl &client_connection_;
  public:
    explicit ClientOrbObject (uint64 orbid, ClientConnectionImpl &c) : OrbObject (orbid), client_connection_ (c) { assert (orbid); }
    virtual                  ~ClientOrbObject   () override          { client_connection_.client_orb_object_deleting (*this); }
    virtual ClientConnection* client_connection () override          { return &client_connection_; }
  };
};

ProtoMsg*
ClientConnectionImpl::pop ()
{
  if (event_queue_.empty())
    return transport_channel_.fetch_msg();
  ProtoMsg *fb = event_queue_.front();
  event_queue_.pop_front();
  return fb;
}

RemoteHandle
ClientConnectionImpl::remote_origin()
{
  RemoteMember<RemoteHandle> rorigin;
  if (!has_peer())
    {
      errno = EHOSTUNREACH; // ECONNREFUSED;
      return rorigin;
    }
  ProtoMsg *fb = ProtoMsg::_new (3);
  fb->add_header2 (MSGID_META_HELLO, 0, 0);
  ProtoMsg *fr = this->call_remote (fb); // takes over fb
  ProtoReader frr (*fr);
  const MessageId msgid = MessageId (frr.pop_int64());
  frr.skip(); // hashhigh
  frr.skip(); // hashlow
  AIDA_ASSERT_RETURN (msgid_is (msgid, MSGID_META_WELCOME), RemoteHandle());
  pop_handle (frr, rorigin);
  delete fr;
  errno = 0;
  return rorigin;
}

void
ClientConnectionImpl::add_handle (ProtoMsg &fb, const RemoteHandle &rhandle)
{
  fb.add_orbid (rhandle.__aida_orbid__());
}

void
ClientConnectionImpl::pop_handle (ProtoReader &fr, RemoteHandle &rhandle)
{
  const uint64 orbid = fr.pop_orbid();
  OrbObjectP orbop = id2orbo_map_[orbid].lock();
  if (AIDA_UNLIKELY (!orbop) && orbid)
    {
      orbop = std::make_shared<ClientOrbObject> (orbid, *this);
      id2orbo_map_[orbid] = orbop;
    }
  (rhandle.*pmf_upgrade_from) (orbop);
}

void
ClientConnectionImpl::notify_callback (const std::function<void (ClientConnection&)> &cb)
{
  notify_cb_ = cb;
}

void
ClientConnectionImpl::gc_sweep (const ProtoMsg *fb)
{
  ProtoReader fbr (*fb);
  const MessageId msgid = MessageId (fbr.pop_int64());
  assert (msgid_is (msgid, MSGID_META_GARBAGE_SWEEP));
  // collect expired object ids and send to server
  vector<uint64> trashids;
  for (auto it = id2orbo_map_.begin(); it != id2orbo_map_.end();)
    if (it->second.expired())
      {
        trashids.push_back (it->first);
        it = id2orbo_map_.erase (it);
      }
    else
      ++it;
  ProtoMsg *fr = ProtoMsg::_new (3 + 1 + trashids.size()); // header + length + items
  fr->add_header1 (MSGID_META_GARBAGE_REPORT, 0, 0); // header
  fr->add_int64 (trashids.size()); // length
  for (auto v : trashids)
    fr->add_int64 (v); // items
  AIDA_DEBUG ("GC: ClientConnectionImpl: GARBAGE_REPORT: %zu trash ids", trashids.size());
  post_peer_msg (fr);
  seen_garbage_ = false;
}

void
ClientConnectionImpl::dispatch ()
{
  ProtoMsg *fb = pop();
  if (fb == NULL)
    return;
  ProtoScope client_connection_protocol_scope (*this);
  ProtoReader fbr (*fb);
  const MessageId msgid = MessageId (fbr.pop_int64());
  const uint64  idmask = msgid_mask (msgid);
  switch (idmask)
    {
    case MSGID_EMIT_ONEWAY:
      remote_handle_dispatch_event_emit_handler (fbr);
      break;
    case MSGID_DISCONNECT:
      remote_handle_dispatch_event_discard_handler (fbr);
      break;
    case MSGID_META_GARBAGE_SWEEP:
      gc_sweep (fb);
      break;
    default: // result/reply messages are handled in call_remote
      {
        AIDA_WARN ("msgid should not occur: %016llx", LLU msgid);
        AIDA_ASSERT_RETURN (! "invalid message id");
      }
      break;
    }
  if (AIDA_UNLIKELY (fb))
    delete fb;
}

ProtoMsg*
ClientConnectionImpl::call_remote (ProtoMsg *fb)
{
  AIDA_ASSERT_RETURN (fb != NULL, NULL);
  // enqueue method call message
  const MessageId callid = MessageId (fb->first_id());
  const bool needsresult = msgid_needs_result (callid);
  if (!needsresult)
    {
      post_peer_msg (fb);
      return NULL;
    }
  const MessageId resultid = MessageId (msgid_mask (msgid_as_result (callid)));
  blocking_for_sem_ = true; // results will notify semaphore
  post_peer_msg (fb);
  ProtoMsg *fr;
  while (needsresult)
    {
      fr = transport_channel_.fetch_msg();
      while (AIDA_UNLIKELY (!fr))
        {
          block_for_result ();
          fr = transport_channel_.fetch_msg();
        }
      const uint64 retmask = msgid_mask (fr->first_id());
      if (retmask == resultid)
        break;
#if 0
      else if (msgid_is_error (retmask))
        {
          ProtoReader fbr (*fr);
          fbr.skip_header();
          std::string msg = fbr.pop_string();
          std::string dom = fbr.pop_string();
          AIDA_WARN ("%s: %s", dom.c_str(), msg.c_str());
          delete fr;
        }
#endif
      else if (retmask == MSGID_DISCONNECT || retmask == MSGID_EMIT_ONEWAY)
        event_queue_.push_back (fr);
      else if (retmask == MSGID_META_GARBAGE_SWEEP)
        {
          gc_sweep (fr);
          delete fr;
        }
      else
        {
          ProtoReader frr (*fr);
          const uint64 retid = frr.pop_int64(); // rethh = frr.pop_int64(), rethl = frr.pop_int64();
          AIDA_WARN ("msgid should not occur: %016llx", LLU retid);
          delete fr;
        }
    }
  blocking_for_sem_ = false;
  if (notify_cb_)
    notify_cb_ (*this);
  return fr;
}

ClientConnectionImpl::SignalHandler*
ClientConnectionImpl::signal_lookup (size_t signal_handler_id)
{
  const size_t handler_index = signal_handler_id ? signal_handler_id - 1 : size_t (-1);
  pthread_spin_lock (&signal_spin_);
  SignalHandler *shandler = handler_index < signal_handlers_.size() ? signal_handlers_[handler_index] : NULL;
  pthread_spin_unlock (&signal_spin_);
  return shandler;
}

// == ClientConnection ==
ClientConnection::ClientConnection (const std::string &protocol) :
  BaseConnection (protocol)
{}

ClientConnection::~ClientConnection ()
{}

/// Initialize the ClientConnection of @a H and accept connections via @a protocol, assigns errno.
ClientConnectionP
ClientConnection::connect (const std::string &protocol)
{
  ClientConnectionP connection;
  ServerConnection *scon = connection_registry->server_connection_from_protocol (protocol);
  if (!scon)
    {
      errno = EHOSTUNREACH; // ECONNREFUSED;
      return connection;
    }
  if (scon->has_peer())
    {
      errno = EBUSY;
      return connection;
    }
  connection = std::make_shared<ClientConnectionImpl> (protocol, *scon);
  assert (connection != NULL);
  scon->peer_connection (*connection);
  errno = 0;
  return connection;
}

// == ServerConnectionImpl ==
/// Transport and dispatch layer for messages sent between ClientConnection and ServerConnection.
class ServerConnectionImpl : public ServerConnection {
  TransportChannel         transport_channel_;  // messages arriving at server
  ObjectMap<ImplicitBase>  object_map_;         // map of all objects used remotely
  ImplicitBaseP            remote_origin_;
  std::unordered_map<size_t, EmitResultHandler> emit_result_map_;
  std::unordered_set<OrbObjectP> live_remotes_, *sweep_remotes_;
  uint64_t                 defer_garbage_collection_id_ = 0;
  AIDA_CLASS_NON_COPYABLE (ServerConnectionImpl);
  void                  start_garbage_collection ();
public:
  explicit              ServerConnectionImpl    (const std::string &protocol);
  virtual              ~ServerConnectionImpl    () override;
  virtual int           notify_fd               () override     { return transport_channel_.inputfd(); }
  virtual bool          pending                 () override     { return transport_channel_.has_msg(); }
  virtual void          dispatch                () override;
  virtual void          remote_origin           (ImplicitBaseP rorigin) override;
  virtual void          add_interface           (ProtoMsg &fb, ImplicitBaseP ibase) override;
  virtual ImplicitBaseP pop_interface           (ProtoReader &fr) override;
  virtual void          emit_result_handler_add (size_t id, const EmitResultHandler &handler) override;
  EmitResultHandler     emit_result_handler_pop (size_t id);
  virtual void          cast_interface_handle   (RemoteHandle &rhandle, ImplicitBaseP ibase) override;
  virtual RemoteHandle
  remote_origin () override
  {
    RemoteHandle rh;
    AIDA_ASSERT_RETURN_UNREACHED (rh);
    return rh;
  }
  virtual void
  receive_msg (ProtoMsg *fb) override
  {
    AIDA_ASSERT_RETURN (fb);
    transport_channel_.send_msg (fb, true);
  }
};

void
ServerConnectionImpl::start_garbage_collection()
{
  if (sweep_remotes_)
    {
      AIDA_ASSERT_RETURN (! "duplicate garbage collection request should not occur");
      return;
    }
  // GARBAGE_SWEEP
  sweep_remotes_ = new std::unordered_set<OrbObjectP>();
  sweep_remotes_->swap (live_remotes_);
  ProtoMsg *fb = ProtoMsg::_new (3);
  fb->add_header2 (MSGID_META_GARBAGE_SWEEP, 0, 0);
  AIDA_DEBUG ("GC: ServerConnectionImpl: GARBAGE_SWEEP: %zu candidates", sweep_remotes_->size());
  post_peer_msg (fb);
}

ServerConnectionImpl::ServerConnectionImpl (const std::string &protocol) :
  ServerConnection (protocol), remote_origin_ (NULL), sweep_remotes_ (NULL)
{
  connection_registry->register_connection (*this);
  const uint64 start_id = OrbObject::orbid_make (0,  // unused
                                                 0,  // unused
                                                 1); // counter = first object id
  object_map_.assign_start_id (start_id, OrbObject::orbid_make (0xffff, 0x0000, 0xffffffff));
}

ServerConnectionImpl::~ServerConnectionImpl()
{
  if (defer_garbage_collection_id_)
    {
      AIDA_DEFER_GARBAGE_COLLECTION_CANCEL (defer_garbage_collection_id_);
      defer_garbage_collection_id_ = 0;
    }
  connection_registry->unregister_connection (*this);
  AIDA_ASSERT_RETURN (! "~ServerConnectionImpl not properly implemented");
}

void
ServerConnectionImpl::remote_origin (ImplicitBaseP rorigin)
{
  if (rorigin)
    AIDA_ASSERT_RETURN (remote_origin_ == NULL);
  else // rorigin == NULL
    AIDA_ASSERT_RETURN (remote_origin_ != NULL);
  remote_origin_ = rorigin;
}

void
ServerConnectionImpl::add_interface (ProtoMsg &fb, ImplicitBaseP ibase)
{
  OrbObjectP orbop = object_map_.orbo_from_instance (ibase);
  fb.add_orbid (orbop ? orbop->orbid() : 0);
  if (orbop)
    live_remotes_.insert (orbop);
}

ImplicitBaseP
ServerConnectionImpl::pop_interface (ProtoReader &fr)
{
  const uint64 orbid = fr.pop_orbid();
  if (orbid)
    {
      OrbObjectP orbop = object_map_.orbo_from_orbid (orbid);
      return object_map_.instance_from_orbo (orbop);
    }
  return NULL;
}

void
ServerConnectionImpl::dispatch ()
{
  ProtoMsg *fb = transport_channel_.fetch_msg();
  if (!fb)
    return;
  ProtoScope server_connection_protocol_scope (*this);
  ProtoReader fbr (*fb);
  const MessageId msgid = MessageId (fbr.pop_int64());
  const uint64  idmask = msgid_mask (msgid);
  switch (idmask)
    {
    case MSGID_META_HELLO:
      {
        const uint64 hashhigh = fbr.pop_int64(), hashlow = fbr.pop_int64();
        AIDA_ASSERT_RETURN (hashhigh == 0 && hashlow == 0);
        AIDA_ASSERT_RETURN (fbr.remaining() == 0);
        fbr.reset (*fb);
        ImplicitBaseP rorigin = remote_origin_;
        ProtoMsg *fr = ProtoMsg::renew_into_result (fbr, MSGID_META_WELCOME, 0, 0, 1);
        add_interface (*fr, rorigin);
        if (AIDA_LIKELY (fr == fb))
          fb = NULL; // prevent deletion
        const uint64 resultmask = msgid_as_result (MessageId (idmask));
        AIDA_ASSERT_RETURN (fr && msgid_mask (fr->first_id()) == resultmask);
        post_peer_msg (fr);
      }
      break;
    case MSGID_CONNECT:
    case MSGID_CALL_TWOWAY:
    case MSGID_CALL_ONEWAY:
      {
        const uint64 hashhigh = fbr.pop_int64(), hashlow = fbr.pop_int64();
        const DispatchFunc server_method_implementation = find_method (hashhigh, hashlow);
        AIDA_ASSERT_RETURN (server_method_implementation != NULL);
        fbr.reset (*fb);
        ProtoMsg *fr = server_method_implementation (fbr);
        if (AIDA_LIKELY (fr == fb))
          fb = NULL; // prevent deletion
        if (idmask == MSGID_CALL_ONEWAY)
          AIDA_ASSERT_RETURN (fr == NULL);
        else // MSGID_CALL_TWOWAY
          {
            const uint64 resultmask = msgid_as_result (MessageId (idmask));
            AIDA_ASSERT_RETURN (fr && msgid_mask (fr->first_id()) == resultmask);
            post_peer_msg (fr);
          }
      }
      break;
    case MSGID_META_SEEN_GARBAGE:
      {
        const uint64 hashhigh = fbr.pop_int64(), hashlow = fbr.pop_int64();
        if (hashhigh == 0 && hashlow == 0) // convention, hash=(0,0)
          {
            auto start_collection = [] (void *data) -> int {
              ServerConnectionImpl *self = (ServerConnectionImpl*) data;
              self->defer_garbage_collection_id_ = 0;
              self->start_garbage_collection();
              return 0;
            };
            if (defer_garbage_collection_id_ == 0)
              defer_garbage_collection_id_ = AIDA_DEFER_GARBAGE_COLLECTION (5 * 1000, start_collection, this);
          }
      }
      break;
    case MSGID_META_GARBAGE_REPORT:
      if (sweep_remotes_)
        {
          const uint64 __attribute__ ((__unused__)) hashhigh = fbr.pop_int64(), hashlow = fbr.pop_int64();
          const uint64 sweeps = sweep_remotes_->size();
          const uint64 n_ids = fbr.pop_int64();
          std::unordered_set<uint64> trashids;
          trashids.reserve (n_ids);
          for (uint64 i = 0; i < n_ids; i++)
            trashids.insert (fbr.pop_int64());
          uint64 retain = 0;
          for (auto orbop : *sweep_remotes_)
            if (trashids.find (orbop->orbid()) == trashids.end())
              {
                live_remotes_.insert (orbop);   // retained objects
                retain++;
              }
          delete sweep_remotes_;                // deletes references
          sweep_remotes_ = NULL;
          AIDA_DEBUG ("GC: ServerConnectionImpl: GARBAGE_COLLECTED: considered=%llu retained=%llu purged=%llu active=%zu",
                      LLU sweeps, LLU retain, LLU (sweeps - retain), live_remotes_.size());
        }
      break;
    case MSGID_EMIT_RESULT:
      {
        fbr.skip(); // hashhigh
        fbr.skip(); // hashlow
        const uint64 emit_result_id = fbr.pop_int64();
        EmitResultHandler emit_result_handler = emit_result_handler_pop (emit_result_id);
        AIDA_ASSERT_RETURN (emit_result_handler != NULL);
        emit_result_handler (fbr);
      }
      break;
    default:
      {
        // const uint64 hashhigh = fbr.pop_int64(), hashlow = fbr.pop_int64();
        AIDA_WARN ("msgid should not occur: %016llx", LLU msgid);
        AIDA_ASSERT_RETURN (! "invalid message id");
      }
      break;
    }
  if (AIDA_UNLIKELY (fb))
    delete fb;
}

void
ServerConnectionImpl::cast_interface_handle (RemoteHandle &rhandle, ImplicitBaseP ibase)
{
  OrbObjectP orbo = object_map_.orbo_from_instance (ibase);
  (rhandle.*pmf_upgrade_from) (orbo);
  AIDA_ASSERT_RETURN (ibase == NULL || rhandle != NULL);
}

void
ServerConnectionImpl::emit_result_handler_add (size_t id, const EmitResultHandler &handler)
{
  AIDA_ASSERT_RETURN (emit_result_map_.count (id) == 0);        // PARANOID
  emit_result_map_[id] = handler;
}

ServerConnectionImpl::EmitResultHandler
ServerConnectionImpl::emit_result_handler_pop (size_t id)
{
  auto it = emit_result_map_.find (id);
  if (AIDA_LIKELY (it != emit_result_map_.end()))
    {
      EmitResultHandler emit_result_handler = it->second;
      emit_result_map_.erase (it);
      return emit_result_handler;
    }
  else
    return EmitResultHandler();
}

// == ServerConnection ==
ServerConnection::ServerConnection (const std::string &protocol) :
  BaseConnection (protocol)
{}

ServerConnection::~ServerConnection()
{}

ServerConnectionP
ServerConnection::make_server_connection (const String &protocol)
{
  assert (protocol.empty() == false);
  AIDA_ASSERT_RETURN (connection_registry->server_connection_from_protocol (protocol) == NULL, NULL);
  return std::make_shared<ServerConnectionImpl> (protocol);
}

struct HashTypeHash {
  inline size_t operator() (const TypeHash &t) const
  {
    return t.typehi ^ t.typelo;
  }
};

typedef std::unordered_map<TypeHash, DispatchFunc, HashTypeHash> DispatcherMap;
static DispatcherMap                    *global_dispatcher_map = NULL;
static pthread_mutex_t                   global_dispatcher_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool                              global_dispatcher_map_frozen = false;

static inline void
ensure_dispatcher_map()
{
  if (AIDA_UNLIKELY (global_dispatcher_map == NULL))
    {
      pthread_mutex_lock (&global_dispatcher_mutex);
      if (!global_dispatcher_map)
        global_dispatcher_map = new DispatcherMap();
      pthread_mutex_unlock (&global_dispatcher_mutex);
    }
}

DispatchFunc
ServerConnection::find_method (uint64 hashhi, uint64 hashlo)
{
  TypeHash typehash (hashhi, hashlo);
#if 1 // avoid costly mutex locking
  if (AIDA_UNLIKELY (global_dispatcher_map_frozen == false))
    {
      ensure_dispatcher_map();
      global_dispatcher_map_frozen = true;
    }
  return (*global_dispatcher_map)[typehash]; // unknown hashes *shouldn't* happen, see assertion in caller
#else
  ensure_dispatcher_map();
  pthread_mutex_lock (&global_dispatcher_mutex);
  DispatchFunc dispatcher_func = (*global_dispatcher_map)[typehash];
  pthread_mutex_unlock (&global_dispatcher_mutex);
  return dispatcher_func;
#endif
}

void
ServerConnection::MethodRegistry::register_method (const MethodEntry &mentry)
{
  ensure_dispatcher_map();
  AIDA_ASSERT_RETURN (global_dispatcher_map_frozen == false);
  pthread_mutex_lock (&global_dispatcher_mutex);
  DispatcherMap::size_type size_before = global_dispatcher_map->size();
  TypeHash typehash (mentry.hashhi, mentry.hashlo);
  (*global_dispatcher_map)[typehash] = mentry.dispatcher;
  DispatcherMap::size_type size_after = global_dispatcher_map->size();
  pthread_mutex_unlock (&global_dispatcher_mutex);
  // simple hash collision check (sanity check, see below)
  if (AIDA_UNLIKELY (size_before == size_after))
    AIDA_ASSERTION_FAILED (posix_sprintf ("method_hash_is_unregistered (%016llx%016llx)", LLU mentry.hashhi, LLU mentry.hashlo).c_str());
}

// == RemoteHandle Event Handlers ==
struct SimplePairHash {
public:
  template<typename T, typename U> size_t
  operator() (const std::pair<T, U> &pair) const
  {
    return std::hash<T>() (pair.first) ^ std::hash<U>() (pair.second);
  }
};
typedef std::unordered_map<std::pair<uint64, uint64>, EventHandlerF, SimplePairHash> RemoteHandleEventHandlerMap;
static std::mutex                  remote_handle_event_handler_map_mutex;
static RemoteHandleEventHandlerMap remote_handle_event_handler_map;

static void
remote_handle_event_handler_add (uint64 orbid, uint64 iface_hid, EventHandlerF handlerf)
{
  std::lock_guard<std::mutex> locker (remote_handle_event_handler_map_mutex);
  remote_handle_event_handler_map[{orbid, iface_hid}] = handlerf;
}

static EventHandlerF
remote_handle_event_handler_get (uint64 orbid, uint64 iface_hid)
{
  std::pair<uint64, uint64> key = {orbid, iface_hid};
  std::lock_guard<std::mutex> locker (remote_handle_event_handler_map_mutex);
  auto it = remote_handle_event_handler_map.find (key);
  if (it != remote_handle_event_handler_map.end())
    return it->second;
  return NULL;
}

static void
remote_handle_event_handler_del (uint64 orbid, uint64 iface_hid)
{
  std::pair<uint64, uint64> key = {orbid, iface_hid};
  std::lock_guard<std::mutex> locker (remote_handle_event_handler_map_mutex);
  remote_handle_event_handler_map.erase (key);
}

// == ImplicitBase <-> RemoteHandle RPC ==
String
RemoteHandle::__typename__ () const
{
  if (*this == NULL)
    return "";
  Aida::ProtoMsg &__p_ = *Aida::ProtoMsg::_new (3 + 1 + 0), *fr = NULL;
  Aida::ProtoScopeCall2Way __o_ (__p_, *this, AIDA_HASH___TYPENAME__);
  fr = __o_.invoke (&__p_);
  AIDA_ASSERT_RETURN (fr != NULL, "");
  Aida::ProtoReader __f_ (*fr);
  __f_.skip_header();
  std::string  retval;
  __f_ >>= retval;
  delete fr;
  return retval;
}

static Aida::ProtoMsg*
ImplicitBase____typename__ (Aida::ProtoReader &fbr)
{
  AIDA_ASSERT_RETURN (fbr.remaining() == 3 + 1 + 0, NULL);
  ImplicitBase *self;
  fbr.skip_header();
  self = fbr.pop_instance<ImplicitBase>().get();
  AIDA_ASSERT_RETURN (self != NULL, NULL);
  std::string rval = self->__typename__ ();
  Aida::ProtoMsg &rb = *ProtoMsg::renew_into_result (fbr, MSGID_CALL_RESULT, AIDA_HASH___TYPENAME__);
  rb <<= rval;
  return &rb;
}

TypeHashList
RemoteHandle::__aida_typelist__() const
{
  TypeHashList thl;
  if (*this == NULL)
    return thl;
  ProtoMsg &__p_ = *ProtoMsg::_new (3 + 1);
  ProtoScopeCall2Way __o_ (__p_, *this, AIDA_HASH___AIDA_TYPELIST__);
  ProtoMsg *__r_ = __o_.invoke (&__p_);
  AIDA_ASSERT_RETURN (__r_ != NULL, thl);
  ProtoReader __f_ (*__r_);
  __f_.skip_header();
  size_t len;
  __f_ >>= len;
  AIDA_ASSERT_RETURN (__f_.remaining() == len * 2, thl);
  for (size_t i = 0; i < len; i++)
    {
      TypeHash thash;
      __f_ >>= thash;
      thl.push_back (thash);
    }
  delete __r_;
  return thl;
}

static ProtoMsg*
ImplicitBase____aida_typelist__ (ProtoReader &__f_)
{
  AIDA_ASSERT_RETURN (__f_.remaining() == 3 + 1, NULL);
  __f_.skip_header();
  ImplicitBase *self = __f_.pop_instance<ImplicitBase>().get();
  TypeHashList thl;
  if (self) // allow NULL self to guard against invalid casts
    thl = self->__aida_typelist__();
  ProtoMsg &__r_ = *ProtoMsg::renew_into_result (__f_, MSGID_CALL_RESULT, AIDA_HASH___AIDA_TYPELIST__, 1 + 2 * thl.size());
  __r_ <<= int64_t (thl.size());
  for (size_t i = 0; i < thl.size(); i++)
    __r_ <<= thl[i];
  return &__r_;
}

std::vector<String>
RemoteHandle::__aida_dir__ () const
{
  if (*this == NULL)
    return StringVector();
  ProtoMsg &__b_ = *ProtoMsg::_new (3 + 1 + 0); // header + self + no-args
  ProtoScopeCall2Way __o_ (__b_, *this, AIDA_HASH___AIDA_DIR__);
  ProtoMsg *__r_ = __o_.invoke (&__b_);
  AIDA_ASSERT_RETURN (__r_ != NULL, std::vector<String>());
  ProtoReader __f_ (*__r_);
  __f_.skip_header();
  Any __v_;
  __f_ >>= __v_;
  delete __r_;
  return __v_.any_to_strings();
}

static ProtoMsg*
ImplicitBase____aida_dir__ (ProtoReader &__b_)
{
  AIDA_ASSERT_RETURN (__b_.remaining() == 3 + 1 + 0, NULL); // header + self + no-args
  __b_.skip_header();
  ImplicitBase *self = __b_.pop_instance<ImplicitBase>().get();
  AIDA_ASSERT_RETURN (self, NULL);
  std::vector<String> __s_ = self->__aida_dir__();
  Any __v_ = Any::any_from_strings (__s_);
  ProtoMsg &__r_ = *ProtoMsg::renew_into_result (__b_, MSGID_CALL_RESULT, AIDA_HASH___AIDA_DIR__);
  __r_ <<= __v_;
  return &__r_;
}

Any
RemoteHandle::__aida_get__ (const String &__n_) const
{
  if (*this == NULL)
    return Any();
  ProtoMsg &__b_ = *ProtoMsg::_new (3 + 1 + 1); // header + self + __n_
  ProtoScopeCall2Way __o_ (__b_, *this, AIDA_HASH___AIDA_GET__);
  __b_ <<= __n_;
  ProtoMsg *__r_ = __o_.invoke (&__b_);
  AIDA_ASSERT_RETURN (__r_ != NULL, Any());
  ProtoReader __f_ (*__r_);
  __f_.skip_header();
  Any __v_;
  __f_ >>= __v_;
  delete __r_;
  return __v_;
}

static ProtoMsg*
ImplicitBase____aida_get__ (ProtoReader &__b_)
{
  AIDA_ASSERT_RETURN (__b_.remaining() == 3 + 1 + 1, NULL); // header + self + __n_
  __b_.skip_header();
  ImplicitBase *self = __b_.pop_instance<ImplicitBase>().get();
  AIDA_ASSERT_RETURN (self, NULL);
  String __n_;
  __b_ >>= __n_;
  Any __v_ = self->__aida_get__ (__n_);
  ProtoMsg &__r_ = *ProtoMsg::renew_into_result (__b_, MSGID_CALL_RESULT, AIDA_HASH___AIDA_GET__);
  __r_ <<= __v_;
  return &__r_;
}

bool
RemoteHandle::__aida_set__ (const String &__n_, const Any &__a_)
{
  if (*this == NULL)
    return false;
  ProtoMsg &__b_ = *ProtoMsg::_new (3 + 1 + 2); // header + self + args
  ProtoScopeCall2Way __o_ (__b_, *this, AIDA_HASH___AIDA_SET__);
  __b_ <<= __n_;
  __b_ <<= __a_;
  ProtoMsg *__r_ = __o_.invoke (&__b_);
  AIDA_ASSERT_RETURN (__r_ != NULL, false);
  ProtoReader __f_ (*__r_);
  __f_.skip_header();
  bool __v_;
  __f_ >>= __v_;
  delete __r_;
  return __v_;
}

static ProtoMsg*
ImplicitBase____aida_set__ (ProtoReader &__b_)
{
  AIDA_ASSERT_RETURN (__b_.remaining() == 3 + 1 + 2, NULL); // header + self + args
  __b_.skip_header();
  ImplicitBase *self = __b_.pop_instance<ImplicitBase>().get();
  AIDA_ASSERT_RETURN (self, NULL);
  String __n_;
  __b_ >>= __n_;
  Any __a_;
  __b_ >>= __a_;
  bool __v_ = self->__aida_set__ (__n_, __a_);
  ProtoMsg &__r_ = *ProtoMsg::renew_into_result (__b_, MSGID_CALL_RESULT, AIDA_HASH___AIDA_SET__);
  __r_ <<= __v_;
  return &__r_;
}

bool
RemoteHandle::__event_detach__ (uint64 arg_connection_id)
{
  AIDA_ASSERT_RETURN (*this != NULL, false);
  Aida::ProtoMsg &__p_ = *Aida::ProtoMsg::_new (3 + 1 + 1), *fr = NULL;
  Aida::ProtoScopeCall2Way __o_ (__p_, *this, AIDA_HASH___EVENT_DETACHID__);
  remote_handle_event_handler_del (this->__aida_orbid__(), arg_connection_id);
  __p_ <<= int64_t (arg_connection_id);
  fr = __o_.invoke (&__p_);
  Aida::ProtoReader __f_ (*fr);
  __f_.skip_header();
  bool  retval;
  __f_ >>= retval;
  delete fr;
  return retval;
}

static Aida::ProtoMsg*
ImplicitBase____event_detachid__ (Aida::ProtoReader &fbr)
{
  AIDA_ASSERT_RETURN (fbr.remaining() == 3 + 1 + 1, NULL);
  ImplicitBase *self;
  fbr.skip_header();
  self = fbr.pop_instance<ImplicitBase>().get();
  AIDA_ASSERT_RETURN (self != NULL, NULL);
  int64_t  arg_connection_id;
  fbr >>= arg_connection_id;
  bool rval = self->__event_detach__ (arg_connection_id);
  Aida::ProtoMsg &rb = *ProtoMsg::renew_into_result (fbr, MSGID_CALL_RESULT, AIDA_HASH___EVENT_DETACHID__);
  rb <<= rval;
  return &rb;
}

uint64
RemoteHandle::__event_attach__ (const std::string &arg_typ3, EventHandlerF handler_func)
{
  AIDA_ASSERT_RETURN (*this != NULL, 0);
  AIDA_ASSERT_RETURN (handler_func != NULL, 0);
  Aida::ProtoMsg &__p_ = *Aida::ProtoMsg::_new (4 + 1), *fr = NULL;
  Aida::ProtoScopeCall2Way __o_ (__p_, *this, AIDA_HASH___EVENT_ATTACH__); // ids needed: 1 message type, 2 hash hi / lo, 1 for self
  __p_ <<= arg_typ3;
  fr = __o_.invoke (&__p_);
  Aida::ProtoReader __f_ (*fr);
  __f_.skip_header();
  int64_t  retval;
  __f_ >>= retval;
  delete fr;
  if (retval)
    remote_handle_event_handler_add (this->__aida_orbid__(), retval, handler_func);
  return retval;
}

static Aida::ProtoMsg*
ImplicitBase____event_attach__ (Aida::ProtoReader &fbr)
{
  Aida::ServerConnection *const server_connection = &Aida::ProtoScope::current_server_connection();
  AIDA_ASSERT_RETURN (server_connection != NULL, NULL);
  AIDA_ASSERT_RETURN (fbr.remaining() == 4 + 1, NULL);
  ImplicitBase *self;
  fbr.skip_header();
  self = fbr.pop_instance<ImplicitBase>().get();
  AIDA_ASSERT_RETURN (self != NULL, NULL);
  std::string  arg_type;
  fbr >>= arg_type;
  const uint64 current_event_handler_id = implicit_base_event_handler_next_id;
  // code executed when the attached handler function is deleted
  auto deleter = [self, server_connection, current_event_handler_id] (void*) {
    Aida::ProtoMsg &__p_ = *Aida::ProtoMsg::_new (3 + 1 + 1);
    Aida::ProtoScopeDisconnect __o_ (__p_, *server_connection, AIDA_HASH___EVENT_CALLBACK__);
    __p_ <<= self;
    __p_ <<= current_event_handler_id;
    server_connection->post_peer_msg (&__p_);
  };
  auto dtor = std::shared_ptr<void> (NULL, deleter);
  // code executed as attached handler
  auto handler = [self, current_event_handler_id, server_connection, dtor] (const Event &event) {
    Aida::ProtoMsg &__p_ = *Aida::ProtoMsg::_new (3 + 1 + 1 + 1);
    Aida::ProtoScopeEmit1Way __o_ (__p_, *server_connection, AIDA_HASH___EVENT_CALLBACK__);
    __p_ <<= self;
    __p_ <<= current_event_handler_id;
    Aida::Any arg_event (event.fields());
    __p_ <<= arg_event;
    server_connection->post_peer_msg (&__p_);
  };
  // attach handler to event type
  int64_t rval = self->__event_attach__ (arg_type, handler);
  const uint64 real_event_handler_id = rval;
  Aida::ProtoMsg &rb = *ProtoMsg::renew_into_result (fbr, MSGID_CALL_RESULT, AIDA_HASH___EVENT_ATTACH__);
  rb <<= rval;
  assert_return (current_event_handler_id == real_event_handler_id, &rb);
  return &rb;
}

static void
remote_handle_dispatch_event_discard_handler (Aida::ProtoReader &fbr)
{
  AIDA_ASSERT_RETURN (fbr.remaining() == 3-1 + 1 + 1); // ::dispatch popped msgid
  const uint64 echash[2] = { AIDA_HASH___EVENT_CALLBACK__ };
  const uint64 hashhigh = fbr.pop_int64(), hashlow = fbr.pop_int64();
  AIDA_ASSERT_RETURN (hashhigh == echash[0] && hashlow == echash[1]);
  RemoteHandle self;
  fbr >>= self;
  AIDA_ASSERT_RETURN (self != NULL);
  int64_t iface_hid;
  fbr >>= iface_hid;
  remote_handle_event_handler_del (self.__aida_orbid__(), iface_hid);
}

static void
remote_handle_dispatch_event_emit_handler (Aida::ProtoReader &fbr)
{
  AIDA_ASSERT_RETURN (fbr.remaining() == 3-1 + 1 + 1 + 1); // ::dispatch popped msgid
  const uint64 echash[2] = { AIDA_HASH___EVENT_CALLBACK__ };
  const uint64 hashhigh = fbr.pop_int64(), hashlow = fbr.pop_int64();
  AIDA_ASSERT_RETURN (hashhigh == echash[0] && hashlow == echash[1]);
  RemoteHandle self;
  fbr >>= self;
  AIDA_ASSERT_RETURN (self != NULL);
  int64_t iface_hid;
  fbr >>= iface_hid;
  Aida::Any arg_event;
  fbr >>= arg_event;
  Event event (arg_event.get<AnyRec>());
  EventHandlerF handler_func = remote_handle_event_handler_get (self.__aida_orbid__(), iface_hid);
  if (handler_func) // handler might have been deleted
    handler_func (event); // self
}

static const ServerConnection::MethodEntry implicit_base_methods[] = {
  { AIDA_HASH___TYPENAME__,             ImplicitBase____typename__, },
  { AIDA_HASH___AIDA_TYPELIST__,        ImplicitBase____aida_typelist__, },
  { AIDA_HASH___AIDA_DIR__,             ImplicitBase____aida_dir__, },
  { AIDA_HASH___AIDA_GET__,             ImplicitBase____aida_get__, },
  { AIDA_HASH___AIDA_SET__,             ImplicitBase____aida_set__, },
  { AIDA_HASH___EVENT_ATTACH__,         ImplicitBase____event_attach__, },
  { AIDA_HASH___EVENT_DETACHID__,       ImplicitBase____event_detachid__, },
};
static ServerConnection::MethodRegistry implicit_base_method_registry (implicit_base_methods);

} // Aida
