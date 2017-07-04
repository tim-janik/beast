// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_CXXAUX_HH__
#define __BSE_CXXAUX_HH__

#include <sfi/sysconfig.h>
#include <sys/types.h>                  // uint, ssize
#include <cstdint>                      // uint64_t
#include <vector>
#include <map>

namespace Bse {

// == uint ==
#if     BSE_SIZEOF_SYS_TYPESH_UINT == 0
typedef unsigned int            uint;   ///< Provide 'uint' if sys/types.h fails to do so.
#else
static_assert (BSE_SIZEOF_SYS_TYPESH_UINT == 4, "");
#endif
static_assert (sizeof (uint) == 4, "");

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

// == Utility Macros ==
#define BSE_CPP_STRINGIFY(s)    BSE_CPP_STRINGIFY_ (s)                  ///< Convert macro argument into a C const char*.
#define BSE_CPP_STRINGIFY_(s)   #s                                      // Indirection helper, required to expand macros like __LINE__
#define BSE_ISLIKELY(expr)      __builtin_expect (bool (expr), 1)       ///< Compiler hint to optimize for @a expr evaluating to true.
#define BSE_UNLIKELY(expr)      __builtin_expect (bool (expr), 0)       ///< Compiler hint to optimize for @a expr evaluating to false.
#define BSE_ABS(a)              ((a) < 0 ? -(a) : (a))                  ///< Yield the absolute value of @a a.
#define BSE_MIN(a,b)            ((a) <= (b) ? (a) : (b))                ///< Yield the smaller value of @a a and @a b.
#define BSE_MAX(a,b)            ((a) >= (b) ? (a) : (b))                ///< Yield the greater value of @a a and @a b.
#define BSE_CLAMP(v,mi,ma)      ((v) < (mi) ? (mi) : ((v) > (ma) ? (ma) : (v))) ///< Yield @a v clamped to [ @a mi .. @a ma ].
#define BSE_ARRAY_SIZE(array)   (sizeof (array) / sizeof ((array)[0]))          ///< Yield the number of C @a array elements.
#define BSE_ALIGN(size, base)   ((base) * (((size) + (base) - 1) / (base)))     ///< Round up @a size to multiples of @a base.
/// @addtogroup GCC Attributes
/// Bse macros that are shorthands for <a href="https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html">GCC Attributes</a>.
/// @{
#define BSE_ALWAYS_INLINE       __attribute__ ((always_inline))
#define BSE_CONST               __attribute__ ((__const__))
#define BSE_CONSTRUCTOR	        __attribute__ ((constructor,used))      // gcc-3.3 also needs "used" to emit code
#define BSE_DEPRECATED          __attribute__ ((__deprecated__))
#define BSE_MALLOC              __attribute__ ((__malloc__))
#define BSE_MAY_ALIAS           __attribute__ ((may_alias))
#define BSE_NOINLINE	        __attribute__ ((noinline))
#define BSE_NORETURN            __attribute__ ((__noreturn__))
#define BSE_NO_INSTRUMENT       __attribute__ ((__no_instrument_function__))
#define BSE_PRINTF(fx, ax)      __attribute__ ((__format__ (__printf__, fx, ax)))
#define BSE_PURE                __attribute__ ((__pure__))
#define BSE_SCANF(fx, ax)       __attribute__ ((__format__ (__scanf__, fx, ax)))
#define BSE_SENTINEL            __attribute__ ((__sentinel__))
#define BSE_UNUSED              __attribute__ ((__unused__))
#define BSE_USED                __attribute__ ((__used__))
/// @}

/// Return silently if @a cond does not evaluate to true, with return value @a ...
#define BSE_RETURN_UNLESS(cond, ...)      do { if (BSE_UNLIKELY (!bool (cond))) return __VA_ARGS__; } while (0)

/// Delete copy ctor and assignment operator.
#define BSE_CLASS_NON_COPYABLE(ClassName)  \
  /*copy-ctor*/ ClassName  (const ClassName&) = delete; \
  ClassName&    operator=  (const ClassName&) = delete
#ifdef __clang__ // clang++-3.8.0: work around 'variable length array of non-POD element type'
#define BSE_DECLARE_VLA(Type, var, count)          std::vector<Type> var (count)
#else // sane c++
#define BSE_DECLARE_VLA(Type, var, count)          Type var[count] ///< Declare a variable length array (clang++ uses std::vector<>).
#endif

} // Bse

#endif // __BSE_CXXAUX_HH__
