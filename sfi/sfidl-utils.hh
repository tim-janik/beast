// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef _SFIDL_UTILS_HH_
#define _SFIDL_UTILS_HH_
#include <sfi/glib-extra.hh>
#include <list>
#include <vector>

// Import simple types into global scope
typedef uint8_t         uint8;          ///< An 8-bit unsigned integer.
typedef uint16_t        uint16;         ///< A 16-bit unsigned integer.
typedef uint32_t        uint32;         ///< A 32-bit unsigned integer.
typedef uint64_t        uint64;         ///< A 64-bit unsigned integer, use PRI*64 in format strings.
typedef int8_t          int8;           ///< An 8-bit signed integer.
typedef int16_t         int16;          ///< A 16-bit signed integer.
typedef int32_t         int32;          ///< A 32-bit signed integer.
typedef int64_t         int64;          ///< A 64-bit unsigned integer, use PRI*64 in format strings.
typedef uint32_t        unichar;        ///< A 32-bit unsigned integer used for Unicode characters.

#define assert_return_unreached(...)    g_assert_not_reached()
#define assert_return(cond, ...)        do { if (cond) break; g_warning ("assertion failed: %s", #cond); return __VA_ARGS__; } while (0)

/// The Sfidl namespace contains implementation and API of the Sfi IDL compiler.
namespace Sfidl {

typedef std::string String;

String string_tolower   (const String &str);
String string_from_int  (int64 value);
String string_from_uint (uint64_t value);
String string_format    (const char *format, ...) __attribute__ ((__format__ (printf, 1, 2)));
void   printerr         (const char *format, ...) __attribute__ ((__format__ (printf, 1, 2)));

/* common data structures */
using std::list;
using std::vector;

bool            isCxxTypeName (const String& str);
list<String>    symbolToList (const String& symbol);

} // Sfidl

#endif /* _SFIDL_UTILS_HH_ */
/* vim:set ts=8 sts=2 sw=2: */
