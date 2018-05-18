// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __BSE_BACKTRACE_HH__
#define __BSE_BACKTRACE_HH__

#include <sfi/cxxaux.hh>

namespace Bse {


// == Debugging Aids ==
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

} // Bse

#endif // __BSE_BACKTRACE_HH__
