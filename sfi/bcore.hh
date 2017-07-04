// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_BCORE_HH__
#define __BSE_BCORE_HH__

#include <sfi/cxxaux.hh>
#include <rapicorn-core.hh>
#include <sfi/glib-extra.hh>

namespace Bse {
using namespace Rapicorn;
using Rapicorn::string_format;
using Rapicorn::printout;
using Rapicorn::printerr;
namespace Path = Rapicorn::Path;

// == Path Name Macros ==
#ifdef  _WIN32 // includes _WIN64
#undef  BSE_UNIX_PATHS                                                  ///< Undefined on _WIN32 and _WIN64, defined on Unix.
#define BSE_DOS_PATHS                1                                  ///< Undefined on Unix-like systems, defined on _WIN32 and _WIN64.
#else   // !_WIN32
#define BSE_UNIX_PATHS               1                                  ///< Undefined on _WIN32 and _WIN64, defined on Unix.
#undef  BSE_DOS_PATHS                                                   ///< Undefined on Unix-like systems, defined on _WIN32 and _WIN64.
#endif  // !_WIN32
#define BSE_DIR_SEPARATOR               RAPICORN_DIR_SEPARATOR          ///< A '/' on Unix-like systems, a '\\' on _WIN32.
#define BSE_DIR_SEPARATOR_S             RAPICORN_DIR_SEPARATOR_S        ///< A "/" on Unix-like systems, a "\\" on _WIN32.
#define BSE_SEARCHPATH_SEPARATOR        RAPICORN_SEARCHPATH_SEPARATOR   ///< A ':' on Unix-like systems, a ';' on _WIN32.
#define BSE_SEARCHPATH_SEPARATOR_S      RAPICORN_SEARCHPATH_SEPARATOR_S ///< A ":" on Unix-like systems, a ";" on _WIN32.
#define BSE_IS_ABSPATH(p)               RAPICORN_IS_ABSPATH (p)         ///< Checks root directory path component, plus drive letter on _WIN32.

// == Diagnostics ==
template<class ...Args> void        fatal             (const char *format, const Args &...args) RAPICORN_NORETURN;
template<class ...Args> void        warning           (const char *format, const Args &...args);
template<class ...Args> void        warn              (const char *format, const Args &...args);
template<class ...Args> void        info              (const char *format, const Args &...args);
template<class ...Args> inline void dump              (const char *conditional, const char *format, const Args &...args) RAPICORN_ALWAYS_INLINE;
template<class ...Args> inline void debug             (const char *conditional, const char *format, const Args &...args) RAPICORN_ALWAYS_INLINE;
inline bool                         debug_enabled     (const char *conditional) RAPICORN_ALWAYS_INLINE BSE_PURE;

// == Internal Implementation Details ==
namespace Internal {
extern bool                         debug_any_enabled;  //< Indicates if $BSE_DEBUG enables some debug settings.
bool                                debug_key_enabled (const char *conditional) BSE_PURE;
void                                diagnostic        (char kind, const std::string &message);
void                                force_abort       () RAPICORN_NORETURN;
} // Internal

/// Issue a printf-like message if @a conditional is enabled by $BSE_DEBUG.
template<class ...Args> inline void RAPICORN_ALWAYS_INLINE
dump (const char *conditional, const char *format, const Args &...args)
{
  if (BSE_UNLIKELY (Internal::debug_any_enabled) && Internal::debug_key_enabled (conditional))
    Internal::diagnostic (' ', string_format (format, args...));
}

/// Issue a printf-like debugging message if @a conditional is enabled by $BSE_DEBUG.
template<class ...Args> inline void RAPICORN_ALWAYS_INLINE
debug (const char *conditional, const char *format, const Args &...args)
{
  if (BSE_UNLIKELY (Internal::debug_any_enabled) && Internal::debug_key_enabled (conditional))
    Internal::diagnostic ('D', string_format (format, args...));
}

/// Check if @a conditional is enabled by $BSE_DEBUG.
inline bool RAPICORN_ALWAYS_INLINE BSE_PURE
debug_enabled (const char *conditional)
{
  if (BSE_UNLIKELY (Internal::debug_any_enabled))
    return Internal::debug_key_enabled (conditional);
  return false;
}

/// Issue a printf-like message and abort the program, this function will not return.
template<class ...Args> void RAPICORN_NORETURN
fatal (const char *format, const Args &...args)
{
  Internal::diagnostic ('F', string_format (format, args...));
  Internal::force_abort();
}

/// Issue a printf-like warning message.
template<class ...Args> void RAPICORN_NORETURN
warn (const char *format, const Args &...args)
{
  Internal::diagnostic ('W', string_format (format, args...));
}

/// Issue a printf-like warning message.
template<class ...Args> void RAPICORN_NORETURN
warning (const char *format, const Args &...args)
{
  Internal::diagnostic ('W', string_format (format, args...));
}

/// Issue an informative printf-like message.
template<class ...Args> void RAPICORN_NORETURN
info (const char *format, const Args &...args)
{
  Internal::diagnostic ('I', string_format (format, args...));
}

// == Assertions ==
/// Return from the current function if @a cond is unmet and issue an assertion warning.
#define BSE_ASSERT_RETURN(cond, ...)            AIDA_ASSERT_RETURN (cond, __VA_ARGS__)
/// Return from the current function and issue an assertion warning.
#define BSE_ASSERT_RETURN_UNREACHED(...)        AIDA_ASSERT_RETURN_UNREACHED (__VA_ARGS__)
#ifdef BSE_CONVENIENCE
/// Return from the current function if @a cond is unmet and issue an assertion warning.
#define assert_return(cond, ...)        BSE_ASSERT_RETURN (cond, __VA_ARGS__)
/// Return from the current function and issue an assertion warning.
#define assert_return_unreached(...)    BSE_ASSERT_RETURN_UNREACHED (__VA_ARGS__)
/// Hint to the compiler to optimize for @a cond == TRUE.
#define ISLIKELY(cond)  BSE_ISLIKELY (cond)
/// Hint to the compiler to optimize for @a cond == FALSE.
#define UNLIKELY(cond)  BSE_UNLIKELY (cond)
/// Return silently if @a cond does not evaluate to true with return value @a ...
#define return_unless(cond, ...)        BSE_RETURN_UNLESS (cond, __VA_ARGS__)
#endif // BSE_CONVENIENCE
using Rapicorn::Aida::assertion_failed_hook;
using Rapicorn::breakpoint;

} // Bse

#endif // __BSE_BCORE_HH__
