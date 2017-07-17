// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_BCORE_HH__
#define __BSE_BCORE_HH__

#include <sfi/cxxaux.hh>
#include <sfi/strings.hh>
#include <sfi/glib-extra.hh>

namespace Bse {

// == type aliases ==
typedef uint8_t         uint8;          ///< An 8-bit unsigned integer.
typedef uint16_t        uint16;         ///< A 16-bit unsigned integer.
typedef uint32_t        uint32;         ///< A 32-bit unsigned integer.
typedef uint64_t        uint64;         ///< A 64-bit unsigned integer, use PRI*64 in format strings.
typedef int8_t          int8;           ///< An 8-bit signed integer.
typedef int16_t         int16;          ///< A 16-bit signed integer.
typedef int32_t         int32;          ///< A 32-bit signed integer.
typedef int64_t         int64;          ///< A 64-bit unsigned integer, use PRI*64 in format strings.
typedef uint32_t        unichar;        ///< A 32-bit unsigned integer used for Unicode characters.
static_assert (sizeof (uint8) == 1 && sizeof (uint16) == 2 && sizeof (uint32) == 4 && sizeof (uint64) == 8, "");
static_assert (sizeof (int8)  == 1 && sizeof (int16)  == 2 && sizeof (int32)  == 4 && sizeof (int64)  == 8, "");
static_assert (sizeof (int) == 4 && sizeof (uint) == 4 && sizeof (unichar) == 4, "");
using   std::map;
using   std::vector;
typedef std::string String;             ///< Convenience alias for std::string.
typedef vector<String> StringVector;    ///< Convenience alias for a std::vector<std::string>.

// == Diagnostics ==
template<class... Args> String      string_format        (const char *format, const Args &...args) BSE_PRINTF (1, 0);
template<class... Args> String      string_locale_format (const char *format, const Args &...args) BSE_PRINTF (1, 0);
template<class... Args> void        printout             (const char *format, const Args &...args) BSE_PRINTF (1, 0);
template<class... Args> void        printerr             (const char *format, const Args &...args) BSE_PRINTF (1, 0);
template<class ...Args> void        fatal                (const char *format, const Args &...args) BSE_NORETURN;
template<class ...Args> void        warning              (const char *format, const Args &...args);
template<class ...Args> void        warn                 (const char *format, const Args &...args);
template<class ...Args> void        info                 (const char *format, const Args &...args);
template<class ...Args> inline void dump                 (const char *conditional, const char *format, const Args &...args) BSE_ALWAYS_INLINE;
template<class ...Args> inline void debug                (const char *conditional, const char *format, const Args &...args) BSE_ALWAYS_INLINE;
inline bool                         debug_enabled        (const char *conditional) BSE_ALWAYS_INLINE BSE_PURE;

// == Development Aids ==
extern inline void breakpoint () BSE_ALWAYS_INLINE;                    ///< Cause a debugging breakpoint, for development only.

// == Implementation Details ==
#if (defined __i386__ || defined __x86_64__)
inline void breakpoint() { __asm__ __volatile__ ("int $03"); }
#elif defined __alpha__ && !defined __osf__
inline void breakpoint() { __asm__ __volatile__ ("bpt"); }
#else   // !__i386__ && !__alpha__
inline void breakpoint() { __builtin_trap(); }
#endif

namespace Internal {
extern bool                         debug_any_enabled;  //< Indicates if $BSE_DEBUG enables some debug settings.
bool                                debug_key_enabled (const char *conditional) BSE_PURE;
void                                diagnostic        (char kind, const std::string &message);
void                                debug_diagnostic  (const char *prefix, const std::string &message);
void                                force_abort       () BSE_NORETURN;
void                                printout_string   (const String &string);
void                                printerr_string   (const String &string);
} // Internal

/// Print a message on stdout (and flush stdout) ala printf(), using the POSIX/C locale.
template<class... Args> void
printout (const char *format, const Args &...args)
{
  Internal::printout_string (string_format (format, args...));
}

/// Print a message on stderr (and flush stderr) ala printf(), using the POSIX/C locale.
template<class... Args> void
printerr (const char *format, const Args &...args)
{
  Internal::printerr_string (string_format (format, args...));
}

/// Issue a printf-like message if @a conditional is enabled by $BSE_DEBUG.
template<class ...Args> inline void BSE_ALWAYS_INLINE
dump (const char *conditional, const char *format, const Args &...args)
{
  if (BSE_UNLIKELY (Internal::debug_any_enabled) && Internal::debug_key_enabled (conditional))
    Internal::diagnostic (' ', string_format (format, args...));
}

/// Issue a printf-like debugging message if @a conditional is enabled by $BSE_DEBUG.
template<class ...Args> inline void BSE_ALWAYS_INLINE
debug (const char *conditional, const char *format, const Args &...args)
{
  if (BSE_UNLIKELY (Internal::debug_any_enabled) && Internal::debug_key_enabled (conditional))
    Internal::debug_diagnostic (conditional, string_format (format, args...));
}

/// Check if @a conditional is enabled by $BSE_DEBUG.
inline bool BSE_ALWAYS_INLINE BSE_PURE
debug_enabled (const char *conditional)
{
  if (BSE_UNLIKELY (Internal::debug_any_enabled))
    return Internal::debug_key_enabled (conditional);
  return false;
}

/// Issue a printf-like message and abort the program, this function will not return.
template<class ...Args> void BSE_NORETURN
fatal (const char *format, const Args &...args)
{
  Internal::diagnostic ('F', string_format (format, args...));
  Internal::force_abort();
}

/// Issue a printf-like warning message.
template<class ...Args> void BSE_NORETURN
warn (const char *format, const Args &...args)
{
  Internal::diagnostic ('W', string_format (format, args...));
}

/// Issue a printf-like warning message.
template<class ...Args> void BSE_NORETURN
warning (const char *format, const Args &...args)
{
  Internal::diagnostic ('W', string_format (format, args...));
}

/// Issue an informative printf-like message.
template<class ...Args> void BSE_NORETURN
info (const char *format, const Args &...args)
{
  Internal::diagnostic ('I', string_format (format, args...));
}

// == Assertions ==
/// Return from the current function if @a cond is unmet and issue an assertion warning.
#define BSE_ASSERT_RETURN(cond, ...)     do { if (BSE_ISLIKELY (cond)) break; ::Bse::assertion_failed (__FILE__, __LINE__, #cond); return __VA_ARGS__; } while (0)
/// Return from the current function and issue an assertion warning.
#define BSE_ASSERT_RETURN_UNREACHED(...) do { ::Bse::assertion_failed (__FILE__, __LINE__, NULL); return __VA_ARGS__; } while (0)
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

} // Bse

#endif // __BSE_BCORE_HH__
