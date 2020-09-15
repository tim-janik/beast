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
// This must mimick what Jsonipc::normalize_typename does
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

static const StringVector&
find_normalized_type (const std::string &abstypename, std::string *kind)
{
  const auto it = aux_data_map().find (abstypename);
  if (it != aux_data_map().end())
    {
      IntrospectionEntry &info = it->second;
      if (!info.entries.size() && info.auxentries && info.length)
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

/// Match 'MEMBER.property=VALUE' against @a kvpair, returns [ strlen (MEMBER), VALUE ] if @a property matches.
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

/// Return enum blurb in `enumtypename` for the exact numeric `value`.
std::string
Introspection::enumerator_blurb_from_value (const std::string &enumtypename, int64_t value)
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
        {
          const String blurb_prefix = kv.substr (0, mvpair.first) + ".blurb=";
          for (const String &bv : kvlist)
            if (strncmp (bv.c_str(), blurb_prefix.c_str(), blurb_prefix.size()) == 0)
              return bv.c_str() + blurb_prefix.size();
        }
    }
  return enumerator_from_value (enumtypename, value);
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
  const ssize_t slength = length;
  AIDA_ASSERT_RETURN (auxentry && length > 0 && auxentry[length - 1] == 0);
  AIDA_ASSERT_RETURN (strncmp (auxentry, "typename=", 9) == 0); // first element is the type name
  const char *type_name = auxentry + 9;
  const char *fundamental = type_name + strlen (type_name) + 1; // second element is the fundamental type
  AIDA_ASSERT_RETURN (slength > fundamental + 5 - auxentry && strncmp (fundamental, "type=", 5) == 0);
  fundamental += 5;
  AIDA_ASSERT_RETURN (fundamental[0] != 0);
  const char *entries = fundamental + strlen (fundamental) + 1;
  AIDA_ASSERT_RETURN (slength > entries - auxentry);
  aux_data_map()[type_name] = IntrospectionEntry { type_name, fundamental, auxentry, length };
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
  "enumerators=UNTYPED;VOID;BOOL;INT32;INT64;FLOAT64;STRING;ENUM;SEQUENCE;RECORD;INSTANCE;ANY\0"
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
  "ANY.value=89\0"
};

const char*
type_kind_name (TypeKind type_kind)
{
  return Introspection::legacy_enumerator ("Aida.TypeKind", type_kind);
}

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
    case INSTANCE:      new (&u_.ibasep()) ImplicitBaseP (clone.u_.ibasep());                      break;
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
    case ENUM:
    case STRING:        std::swap (u.vstring(), v.vstring()); break;
    case SEQUENCE:      std::swap (u.vanys(), v.vanys());     break;
    case RECORD:        std::swap (u.vfields(), v.vfields()); break;
    case INSTANCE:      std::swap (u.ibasep(), v.ibasep());   break;
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
    case INSTANCE:      u_.ibasep().~ImplicitBaseP();           break;
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
    case INSTANCE: new (&u_.ibasep()) ImplicitBaseP();  break;
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
    case INSTANCE:   s += posix_sprintf ("(ImplicitBaseP (ptr=%p))", u_.ibasep().get());             break;
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
    case BOOL: case INT32: // chain
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
      return u_.ibasep().get() == clone.u_.ibasep().get();
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
    case BOOL: case INT32:
    case INT64:         return u_.vint64 != 0;
    case ENUM:
    case STRING:        return !u_.vstring().empty();
    case SEQUENCE:      return !u_.vanys().empty();
    case RECORD:        return !u_.vfields().empty();
    case INSTANCE:      return u_.ibasep().get() != NULL;
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
    return u_.ibasep();
  return ImplicitBaseP();
}

#if 0
/// Use Any.get<DerivedType>() instead.
RemoteHandle
Any::get_untyped_remote_handle () const
{
  RemoteHandle rh;
  if (kind() == INSTANCE)
    rh = u_.rhandle();
  return rh;
}
#endif

void
Any::set_ibasep (ImplicitBaseP ibaseptr)
{
  ensure (INSTANCE);
  u_.ibasep() = ibaseptr;
}

#if 0
void
Any::set_handle (const RemoteHandle &handle)
{
  ensure (INSTANCE);
  u_.rhandle() = handle;
}
#endif

const Any*
Any::get_any () const
{
  if (kind() == ANY && u_.vany)
    return u_.vany;
  return this;
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

// == EventFd ==
EventFd::EventFd () :
  fds { -1, -1 }
{}

int
EventFd::open ()
{
  if (opened())
    return 0;
  AIDA_UNUSED long nflags;
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
std::string
CallableIface::__typename__ () const
{
  const StringVector &types = __typelist_mt__();
  return types.empty() ? "" : types[0];
}

// == PropertyAccessor ==
PropertyAccessor::~PropertyAccessor()
{}

} // Aida
