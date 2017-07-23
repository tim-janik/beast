// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef _SFIDL_UTILS_HH_
#define _SFIDL_UTILS_HH_
#include <sfi/glib-extra.hh>
#include <sfi/sfi.hh>
#include <list>
#include <vector>

/// The Sfidl namespace contains implementation and API of the Sfi IDL compiler.
namespace Sfidl {

using Rapicorn::String;
using Rapicorn::string_tolower;
using Rapicorn::printerr;
using Rapicorn::string_from_int;
using Rapicorn::string_from_uint;
using Rapicorn::string_format;

/* common data structures */
using std::list;
using std::vector;

bool            isCxxTypeName (const String& str);
list<String>    symbolToList (const String& symbol);

} // Sfidl

#endif /* _SFIDL_UTILS_HH_ */
/* vim:set ts=8 sts=2 sw=2: */
