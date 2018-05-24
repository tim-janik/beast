// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __BSE_BACKTRACE_HH__
#define __BSE_BACKTRACE_HH__

#include <sfi/cxxaux.hh>

namespace Bse {

/// Print a C++ backtrace to stdout/stderr.
extern inline bool print_backtrace (const char *file, int line, const char *func) BSE_ALWAYS_INLINE BSE_COLD;

/// Print backtrace of the current line to stderr.
#define BSE_BACKTRACE()          ({ ::Bse::print_backtrace (__FILE__, __LINE__, __func__); })

// == Implementation ==
namespace Internal {
extern int (*backtrace_pointers)    (void **buffer, int size);  ///< Capture stack frames for a backtrace (on __GLIBC__).
bool         backtrace_print_frames (const char *file, int line, const char *func, void **ptrs, ssize_t nptrs) BSE_COLD;
} // Internal

inline bool
print_backtrace (const char *file, int line, const char *func)
{
  void *frames[1024];
  const int n = ::Bse::Internal::backtrace_pointers (frames, sizeof (frames) / sizeof (frames[0]));
  return ::Bse::Internal::backtrace_print_frames (file, line, func, frames, n);
}

} // Bse

#endif // __BSE_BACKTRACE_HH__
