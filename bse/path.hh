// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __BSE_PATH_HH__
#define __BSE_PATH_HH__

#include <sfi/bcore.hh>

#ifdef  _WIN32 // includes _WIN64
#undef  BSE_UNIX_PATHS                          ///< Undefined on _WIN32 and _WIN64, defined on Unix.
#define BSE_DOS_PATHS                   1       ///< Undefined on Unix-like systems, defined on _WIN32 and _WIN64.
#define BSE_DIRCHAR                     '\\'
#define BSE_DIRCHAR2                    '/'
#define BSE_DIRSEPARATORS               "/\\"
#define BSE_SEARCHPATH_SEPARATOR        ';'
#define BSE_LIBEXT                      ".dll"
#else   // !_WIN32
#define BSE_UNIX_PATHS                  1       ///< Macro defined if Unix path syntax is used.
#undef  BSE_DOS_PATHS                           ///< Undefined on Unix-like systems, defined on _WIN32 and _WIN64.
#define BSE_DIRCHAR                     '/'     ///< Platform directory separator character, '/' on Unix-like systems, a '\\' on _WIN32.
#define BSE_DIRCHAR2                    '/'     ///< Secondary directory separator character, '/' on Unix-like systems.
#define BSE_DIRSEPARATORS               "/"     ///< List of platform directory separator characters, "/" on Unix-like systems, "/\\" on _WIN32.
#define BSE_SEARCHPATH_SEPARATOR        ':'     ///< Platform searchpath separator, ':' on Unix-like systems, ';' on _WIN32.
#define BSE_LIBEXT                      ".so"   ///< Dynamic library filename extension on this platform.
#endif  // !_WIN32

namespace Bse {

/// The Path namespace provides functions for file path manipulation and testing.
namespace Path {

String       dirname             (const String &path);
String       basename            (const String &path);
String       realpath            (const String &path);
String       abspath             (const String &path, const String &incwd = "");
bool         isabs               (const String &path);
bool         isdirname           (const String &path);
String       expand_tilde        (const String &path);
String       user_home           (const String &username = "");
String       data_home           ();
String       config_home         ();
String       config_names        ();
void         config_names        (const String &names);
String       cache_home          ();
String       runtime_dir         ();
String       config_dirs         ();
String       data_dirs           ();
String       skip_root           (const String &path);
String       join                (const String &frag0, const String &frag1, const String &frag2 = "", const String &frag3 = "",
                                  const String &frag4 = "", const String &frag5 = "", const String &frag6 = "", const String &frag7 = "",
                                  const String &frag8 = "", const String &frag9 = "", const String &frag10 = "", const String &frag11 = "",
                                  const String &frag12 = "", const String &frag13 = "", const String &frag14 = "", const String &frag15 = "");
bool         check               (const String &file,
                                  const String &mode);
bool         equals              (const String &file1,
                                  const String &file2);
char*        memread             (const String &filename,
                                  size_t       *lengthp);
void         memfree             (char         *memread_mem);
bool         memwrite            (const String &filename, size_t len, const uint8 *bytes);
String       cwd                 ();
String       vpath_find          (const String &file, const String &mode = "e");
bool         searchpath_contains (const String &searchpath, const String &element);
String       searchpath_find     (const String &searchpath, const String &file, const String &mode = "e");
StringVector searchpath_list     (const String &searchpath, const String &mode = "e");
String       searchpath_multiply (const String &searchpath, const String &postfixes);
StringVector searchpath_split    (const String &searchpath);
String       searchpath_join     (const String &frag0, const String &frag1, const String &frag2 = "", const String &frag3 = "",
                                  const String &frag4 = "", const String &frag5 = "", const String &frag6 = "", const String &frag7 = "",
                                  const String &frag8 = "", const String &frag9 = "", const String &frag10 = "",
                                  const String &frag11 = "", const String &frag12 = "", const String &frag13 = "",
                                  const String &frag14 = "", const String &frag15 = "");

} // Path
} // Bse

#endif  // __BSE_PATH_HH__
