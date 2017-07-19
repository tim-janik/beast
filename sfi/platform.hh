// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __BSE_PLATFORM_HH__
#define __BSE_PLATFORM_HH__

#include <sfi/cxxaux.hh>

namespace Bse {

// == Timestamp Handling ==
uint64  timestamp_startup    ();        // µseconds
uint64  timestamp_realtime   ();        // µseconds
uint64  timestamp_benchmark  ();        // nseconds
uint64  timestamp_resolution ();        // nseconds
String  timestamp_format     (uint64 stamp, uint maxlength = 8);
uint64  monotonic_counter    ();

// == process info ==
String      program_alias         ();                   ///< Retrieve the program name as used for logging or debug messages.
void        program_alias_init    (String customname);  ///< Set program_alias to a non-localized alias other than program_argv0 if desired.
String      application_name      ();                   ///< Retrieve the localized program name intended for user display.
void        application_name_init (String desktopname); ///< Set the application_name to a name other than program_alias if desired.
String      program_cwd           ();                   ///< The current working directory during startup.
std::string executable_name       ();                   ///< Retrieve the name part of executable_path().
std::string executable_path       ();                   ///< Retrieve the path to the currently running executable.

// == Debugging Aids ==
extern inline void breakpoint               () BSE_ALWAYS_INLINE;       ///< Cause a debugging breakpoint, for development only.
extern int       (*backtrace_pointers)      (void **buffer, int size);  ///< Capture stack frames for a backtrace (on __GLIBC__).
String             pretty_backtrace         (void **ptrs, ssize_t nptrs, const char *file, int line, const char *func);
StringVector       pretty_backtrace_symbols (void **pointers, const int nptrs);
#define BSE_BACKTRACE_MAXDEPTH   1024                   ///< Maximum depth for runtime backtrace generation.
/// Print backtrace of the current line to stderr.
#define BSE_BACKTRACE()          ({ ::Bse::printerr ("%s", BSE_BACKTRACE_STRING()); })
/// Generate a string that contains a backtrace of the current line.
#define BSE_BACKTRACE_STRING()   ({ void *__p_[BSE_BACKTRACE_MAXDEPTH]; \
      const String __s_ = ::Bse::pretty_backtrace (__p_, ::Bse::backtrace_pointers (__p_, sizeof (__p_) / sizeof (__p_[0])), \
                                                   __FILE__, __LINE__, __func__); __s_; })

// == Implementation Details ==
#if (defined __i386__ || defined __x86_64__)
inline void breakpoint() { __asm__ __volatile__ ("int $03"); }
#elif defined __alpha__ && !defined __osf__
inline void breakpoint() { __asm__ __volatile__ ("bpt"); }
#else   // !__i386__ && !__alpha__
inline void breakpoint() { __builtin_trap(); }
#endif

} // Bse

#endif // __BSE_PLATFORM_HH__
