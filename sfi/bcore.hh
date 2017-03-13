// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_BCORE_HH__
#define __BSE_BCORE_HH__

#ifdef  BSE_CONVENIENCE
#define RAPICORN_CONVENIENCE    BSE_CONVENIENCE
#endif
#include <rapicorn-core.hh>
#ifdef  RAPICORN_CONVENIENCE
#undef  fatal                           // avoid easy clashes
#endif
#include <sfi/glib-extra.hh>

namespace Bse {
using namespace Rapicorn;
using Rapicorn::uint8;
using Rapicorn::uint16;
using Rapicorn::uint32;
using Rapicorn::uint64;
using Rapicorn::int8;
using Rapicorn::int16;
using Rapicorn::int32;
using Rapicorn::int64;
using Rapicorn::unichar;
using Rapicorn::String;
using Rapicorn::string_format;
using Rapicorn::printout;
using Rapicorn::printerr;
namespace Path = Rapicorn::Path;


// == Utility Macros ==
#define BSE_ISLIKELY(expr)      RAPICORN_ISLIKELY(expr)         ///< Compiler hint that @a expr is likely to be true.
#define BSE_UNLIKELY(expr)      RAPICORN_UNLIKELY(expr)         ///< Compiler hint that @a expr is unlikely to be true.
#define BSE_LIKELY              BSE_ISLIKELY                    ///< Compiler hint that @a expr is likely to be true.
#define BSE_ABS(a)              ((a) < 0 ? -(a) : (a))          ///< Yield the absolute value of @a a.
#define BSE_MIN(a,b)            ((a) <= (b) ? (a) : (b))        ///< Yield the smaller value of @a a and @a b.
#define BSE_MAX(a,b)            ((a) >= (b) ? (a) : (b))        ///< Yield the greater value of @a a and @a b.
#define BSE_CLAMP(v,mi,ma)      ((v) < (mi) ? (mi) : ((v) > (ma) ? (ma) : (v))) ///< Yield @a v clamped to [ @a mi .. @a ma ].
#define BSE_ARRAY_SIZE(array)   (sizeof (array) / sizeof ((array)[0]))          ///< Yield the number of C @a array elements.
#define BSE_ALIGN(size, base)   ((base) * (((size) + (base) - 1) / (base)))     ///< Round up @a size to multiples of @a base.
#define BSE_CPP_STRINGIFY(s)    RAPICORN_CPP_STRINGIFY(s)    ///< Turn @a s into a C string literal.
#define BSE__HERE__             RAPICORN__HERE__             ///< Shorthand for a string literal containing __FILE__ ":" __LINE__
#define BSE_PURE	        RAPICORN_PURE           ///< A <a href="https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html">GCC Attribute</a>.
#define BSE_MALLOC	        RAPICORN_MALLOC         ///< A <a href="https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html">GCC Attribute</a>.
#define BSE_SENTINEL	        RAPICORN_SENTINEL       ///< A <a href="https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html">GCC Attribute</a>.
#define BSE_NORETURN	        RAPICORN_NORETURN       ///< A <a href="https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html">GCC Attribute</a>.
#define BSE_CONST	        RAPICORN_CONST          ///< A <a href="https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html">GCC Attribute</a>.
#define BSE_UNUSED	        RAPICORN_UNUSED         ///< A <a href="https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html">GCC Attribute</a>.
#define BSE_USED	        RAPICORN_USED           ///< A <a href="https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html">GCC Attribute</a>.
#define BSE_NO_INSTRUMENT	RAPICORN_NO_INSTRUMENT  ///< A <a href="https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html">GCC Attribute</a>.
#define BSE_DEPRECATED	        RAPICORN_DEPRECATED     ///< A <a href="https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html">GCC Attribute</a>.
#define BSE_ALWAYS_INLINE	RAPICORN_ALWAYS_INLINE  ///< A <a href="https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html">GCC Attribute</a>.
#define BSE_NOINLINE	        RAPICORN_NOINLINE       ///< A <a href="https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html">GCC Attribute</a>.
#define BSE_CONSTRUCTOR	        RAPICORN_CONSTRUCTOR    ///< A <a href="https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html">GCC Attribute</a>.
#define BSE_MAY_ALIAS	        RAPICORN_MAY_ALIAS      ///< A <a href="https://gcc.gnu.org/onlinedocs/gcc/Common-Type-Attributes.html">GCC Attribute</a>.
#define BSE_CLASS_NON_COPYABLE(ClassName) RAPICORN_CLASS_NON_COPYABLE (ClassName) ///< Delete copy ctor and assignment operator.
#define BSE_DECLARE_VLA(Type, var, count) RAPICORN_DECLARE_VLA (Type, var, count) ///< Declare a variable length array (clang++ uses std::vector<>).

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

// == Legacy ==
inline bool BSE_DEPRECATED bse_debug_enabled (const char *k)    { return debug_enabled (k); }

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

} // Bse

#endif // __BSE_BCORE_HH__
