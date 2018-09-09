// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef _SFIDL_UTILS_HH_
#define _SFIDL_UTILS_HH_
#include <sfi/glib-extra.hh>
#include <sfi/sfi.hh>
#include <list>
#include <vector>

/// The Sfidl namespace contains implementation and API of the Sfi IDL compiler.
namespace Sfidl {

using Bse::String;

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
