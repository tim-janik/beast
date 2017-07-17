// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __BSE_PLATFORM_HH__
#define __BSE_PLATFORM_HH__

#include <sfi/cxxaux.hh>

namespace Bse {

// == process info ==
String      program_alias         ();                   ///< Retrieve the program name as used for logging or debug messages.
void        program_alias_init    (String customname);  ///< Set program_alias to a non-localized alias other than program_argv0 if desired.
String      application_name      ();                   ///< Retrieve the localized program name intended for user display.
void        application_name_init (String desktopname); ///< Set the application_name to a name other than program_alias if desired.
String      program_cwd           ();                   ///< The current working directory during startup.
std::string executable_name       ();                   ///< Retrieve the name part of executable_path().
std::string executable_path       ();                   ///< Retrieve the path to the currently running executable.

} // Bse

#endif // __BSE_PLATFORM_HH__
