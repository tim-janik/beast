// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __BSE_STRINGS_HH__
#define __BSE_STRINGS_HH__

#include <bse/cxxaux.hh>
#include <bse/formatter.hh>
#include <cstring>              // strerror

namespace Bse {
typedef std::string String;

// == C-String ==
bool    		       cstring_to_bool       (const char *string, bool fallback = false);

// == String Formatting ==
template<class... Args> String string_format         (const char *format, const Args &...args) BSE_PRINTF (1, 0);
template<class... Args> String string_locale_format  (const char *format, const Args &...args) BSE_PRINTF (1, 0);
String                         string_vprintf        (const char *format, va_list vargs);
String                         string_locale_vprintf (const char *format, va_list vargs);

// == String ==
String                          string_multiply          (const String &s, uint64 count);
String                          string_canonify          (const String &s, const String &valid_chars, const String &substitute);
bool                            string_is_canonified     (const String &s, const String &valid_chars);
String                          string_set_a2z           ();
String                          string_set_A2Z           ();
String                          string_set_ascii_alnum   ();
String  			string_tolower           (const String &str);
String  			string_toupper           (const String &str);
String  			string_totitle           (const String &str);
String                          string_capitalize        (const String &str, size_t maxn = size_t (-1), bool rest_tolower = true);
StringVector 			string_split             (const String &string, const String &splitter = "", size_t maxn = size_t (-1));
StringVector 			string_split_any         (const String &string, const String &splitchars = "", size_t maxn = size_t (-1));
String  			string_join              (const String &junctor, const StringVector &strvec);
bool    			string_to_bool           (const String &string, bool fallback = false);
String  			string_from_bool         (bool value);
uint64  			string_to_uint           (const String &string, size_t *consumed = NULL, uint base = 10);
String  			string_from_uint         (uint64 value);
bool    			string_has_int           (const String &string);
int64   			string_to_int            (const String &string, size_t *consumed = NULL, uint base = 10);
String  			string_from_int          (int64 value);
String  			string_from_float        (float value);
long double  			string_to_long_double    (const String &string);
long double  			string_to_long_double    (const char *dblstring, const char **endptr);
String                          string_from_long_double  (long double value);
double  			string_to_double         (const String &string);
double  			string_to_double         (const char *dblstring, const char **endptr);
String                          string_from_double       (double value);
inline String                   string_from_float        (double value)         { return string_from_double (value); }
inline double                   string_to_float          (const String &string) { return string_to_double (string); }
vector<double>                  string_to_double_vector  (const String         &string);
String                          string_from_double_vector(const vector<double> &dvec,
                                                          const String         &delim = " ");
String  			string_from_errno        (int         errno_val);
bool                            string_is_uuid           (const String &uuid_string); /* check uuid formatting */
int                             string_cmp_uuid          (const String &uuid_string1,
                                                          const String &uuid_string2); /* -1=smaller, 0=equal, +1=greater (assuming valid uuid strings) */
bool                            string_startswith        (const String &string, const String &fragment);
bool                            string_endswith          (const String &string, const String &fragment);
bool    string_match_identifier                          (const String &ident1, const String &ident2);
bool    string_match_identifier_tail                     (const String &ident, const String &tail);
String  string_from_pretty_function_name                 (const char *cxx_pretty_function);
String  string_to_cescape                                (const String &str);
String  string_to_cquote                                 (const String &str);
String  string_from_cquote                               (const String &input);
String  string_hexdump                                   (const void *addr, size_t length, size_t initial_offset = 0);
String  string_lstrip                                    (const String &input);
String  string_rstrip                                    (const String &input);
String  string_strip                                     (const String &input);
String  string_replace                                   (const String &input, const String &marker, const String &replacement, size_t maxn = ~size_t (0));
String  string_substitute_char                           (const String &input, const char match, const char subst);
void    string_vector_lstrip       (StringVector &svector);
void    string_vector_rstrip       (StringVector &svector);
void    string_vector_strip        (StringVector &svector);
void    string_vector_erase_empty  (StringVector &svector);
String  string_vector_find         (const StringVector &svector, const String &prefix, const String &fallback = "");
String  string_vector_find_value   (const StringVector &svector, const String &prefix, const String &fallback = "");
StringVector cstrings_to_vector    (const char*, ...) BSE_SENTINEL;
void         memset4		   (uint32 *mem, uint32 filler, uint length);
long double posix_locale_strtold   (const char *nptr, char **endptr);
long double current_locale_strtold (const char *nptr, char **endptr);

// == UTF-8 String Helpers ==
String string_normalize_nfc  (const String &src);                       // Normalized, composed form UTF-8 string
String string_normalize_nfd  (const String &src);
String string_normalize_nfkc (const String &src);
String string_normalize_nfkd (const String &src);
String string_casefold       (const String &src);
int    string_casecmp        (const String &s1, const String &s2);      // UTF-8 version of strcasecmp(3)
int    string_cmp            (const String &s1, const String &s2);      // UTF-8 version of strcmp(3)

// == Templated String Conversions ==

/// Convert a @a string to template argument type, such as bool, int, double.
template<typename Type> Type    string_to_type           (const String &string)
{ static_assert (!sizeof (Type), "string_to_type<>: unsupported Type");  __builtin_unreachable(); }

/// Create a @a string from a templated argument value, such as bool, int, double.
template<typename Type> String  string_from_type         (Type          value)
{ static_assert (!sizeof (Type), "string_from_type<>: unsupported Type");  __builtin_unreachable(); }

// specialisations for templated string conversions
template<> inline long double   string_to_type<long double>   (const String &string) { return string_to_long_double (string); }
template<> inline String        string_from_type<long double> (long double    value) { return string_from_long_double (value); }
template<> inline double        string_to_type<double>   (const String &string) { return string_to_double (string); }
template<> inline String        string_from_type<double> (double         value) { return string_from_double (value); }
template<> inline float         string_to_type<float>    (const String &string) { return string_to_float (string); }
template<> inline String        string_from_type<float>  (float         value)  { return string_from_float (value); }
template<> inline bool          string_to_type<bool>     (const String &string) { return string_to_bool (string); }
template<> inline String        string_from_type<bool>   (bool         value)   { return string_from_bool (value); }
template<> inline int16         string_to_type<int16>    (const String &string) { return string_to_int (string); }
template<> inline String        string_from_type<int16>  (int16         value)  { return string_from_int (value); }
template<> inline uint16        string_to_type<uint16>   (const String &string) { return string_to_uint (string); }
template<> inline String        string_from_type<uint16> (uint16        value)  { return string_from_uint (value); }
template<> inline int           string_to_type<int>      (const String &string) { return string_to_int (string); }
template<> inline String        string_from_type<int>    (int         value)    { return string_from_int (value); }
template<> inline uint          string_to_type<uint>     (const String &string) { return string_to_uint (string); }
template<> inline String        string_from_type<uint>   (uint           value) { return string_from_uint (value); }
template<> inline int64         string_to_type<int64>    (const String &string) { return string_to_int (string); }
template<> inline String        string_from_type<int64>  (int64         value)  { return string_from_int (value); }
template<> inline uint64        string_to_type<uint64>   (const String &string) { return string_to_uint (string); }
template<> inline String        string_from_type<uint64> (uint64         value) { return string_from_uint (value); }
template<> inline String        string_to_type<String>   (const String &string) { return string; }
template<> inline String        string_from_type<String> (String         value) { return value; }


// == String Options ==
bool    string_option_check     (const String   &option_string,
                                 const String   &option);
String  string_option_get       (const String   &option_string,
                                 const String   &option);
void    string_options_split    (const String   &option_string,
                                 vector<String> &option_names,
                                 vector<String> &option_values,
                                 const String   &empty_default = "");

// == Generic Key-Value-Pairs ==
String  kvpair_key              (const String &key_value_pair);
String  kvpair_value            (const String &key_value_pair);

// == Strings ==
/// Convenience Constructor for StringSeq or std::vector<std::string>
class Strings : public std::vector<std::string>
{
  typedef const std::string CS;
public:
  explicit Strings (CS &s1);
  explicit Strings (CS &s1, CS &s2);
  explicit Strings (CS &s1, CS &s2, CS &s3);
  explicit Strings (CS &s1, CS &s2, CS &s3, CS &s4);
  explicit Strings (CS &s1, CS &s2, CS &s3, CS &s4, CS &s5);
  explicit Strings (CS &s1, CS &s2, CS &s3, CS &s4, CS &s5, CS &s6);
  explicit Strings (CS &s1, CS &s2, CS &s3, CS &s4, CS &s5, CS &s6, CS &s7);
  explicit Strings (CS &s1, CS &s2, CS &s3, CS &s4, CS &s5, CS &s6, CS &s7, CS &s8);
  explicit Strings (CS &s1, CS &s2, CS &s3, CS &s4, CS &s5, CS &s6, CS &s7, CS &s8, CS &s9);
  explicit Strings (CS &s1, CS &s2, CS &s3, CS &s4, CS &s5, CS &s6, CS &s7, CS &s8, CS &s9, CS &sA);
  explicit Strings (CS &s1, CS &s2, CS &s3, CS &s4, CS &s5, CS &s6, CS &s7, CS &s8, CS &s9, CS &sA, CS &sB);
  explicit Strings (CS &s1, CS &s2, CS &s3, CS &s4, CS &s5, CS &s6, CS &s7, CS &s8, CS &s9, CS &sA, CS &sB, CS &sC);
};

// == Charset Conversions ==
bool    text_convert    (const String &to_charset,
                         String       &output_string,
                         const String &from_charset,
                         const String &input_string,
                         const String &fallback_charset = "ISO-8859-15",
                         const String &output_mark = "");

// == C strings ==
using         ::strerror;       // introduce (const char* strerror (int))
const char*     strerror ();    // simple wrapper for strerror (errno)

// == Implementations ==
#define BSE_STRING_VECTOR_FROM_ARRAY(ConstCharArray)               ({ \
  Bse::StringVector __a;                                           \
  const Bse::uint64 __l = BSE_ARRAY_SIZE (ConstCharArray);    \
  for (Bse::uint64 __ai = 0; __ai < __l; __ai++)                   \
    __a.push_back (ConstCharArray[__ai]);                               \
  __a; })
#define BSE_CQUOTE(str)    (Bse::string_to_cquote (str).c_str())

/// Formatted printing ala printf() into a String, using the POSIX/C locale.
template<class... Args> BSE_NOINLINE String
string_format (const char *format, const Args &...args)
{
  return Lib::StringFormatter::format (NULL, format, args...);
}

/// Formatted printing ala printf() into a String, using the current locale.
template<class... Args> BSE_NOINLINE String
string_locale_format (const char *format, const Args &...args)
{
  return Lib::StringFormatter::format<Lib::StringFormatter::CURRENT_LOCALE> (NULL, format, args...);
}

} // Bse

#endif  // __BSE_STRINGS_HH__

