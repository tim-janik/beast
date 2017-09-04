// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_CXXAUX_HH__
#define __BSE_CXXAUX_HH__

#include <sfi/sysconfig.h>
#include <sys/types.h>                  // uint, ssize
#include <cstdint>                      // uint64_t
#include <memory>
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
#define BSE_CPP_PASTE2_(a,b)    a ## b                                  // Indirection helper, required to expand macros like __LINE__
#define BSE_CPP_PASTE2(a,b)     BSE_CPP_PASTE2_ (a,b)                   ///< Paste two macro arguments into one C symbol name
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
#define BSE_FORMAT(fx)          __attribute__ ((__format_arg__ (fx)))
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

/**
 * A std::make_shared<>() wrapper class to access private ctor & dtor.
 * To call std::make_shared<T>() on a class @a T, its constructor and
 * destructor must be public. For classes with private or protected
 * constructor or destructor, this class can be used as follows:
 * @code{.cc}
 * class Type {
 *   Type (ctor_args...);                // Private ctor.
 *   friend class FriendAllocator<Type>; // Allow access to ctor/dtor of Type.
 * };
 * std::shared_ptr<Type> t = FriendAllocator<Type>::make_shared (ctor_args...);
 * @endcode
 */
template<class T>
class FriendAllocator : public std::allocator<T> {
public:
  /// Construct type @a C object, standard allocator requirement.
  template<typename C, typename... Args> static inline void
  construct (C *p, Args &&... args)
  {
    ::new ((void*) p) C (std::forward<Args> (args)...);
  }
  /// Delete type @a C object, standard allocator requirement.
  template<typename C> static inline void
  destroy (C *p)
  {
    p->~C ();
  }
  /**
   * Construct an object of type @a T that is wrapped into a std::shared_ptr<T>.
   * @param args        The list of arguments to pass into a T() constructor.
   * @return            A std::shared_ptr<T> owning the newly created object.
   */
  template<typename ...Args> static inline std::shared_ptr<T>
  make_shared (Args &&... args)
  {
    return std::allocate_shared<T> (FriendAllocator(), std::forward<Args> (args)...);
  }
};

/** Shorthand for std::dynamic_pointer_cast<>(shared_from_this()).
 * A shared_ptr_cast() takes a std::shared_ptr or a pointer to an @a object that
 * supports std::enable_shared_from_this::shared_from_this().
 * Using std::dynamic_pointer_cast(), the shared_ptr passed in (or retrieved via
 * calling shared_from_this()) is cast into a std::shared_ptr<@a Target>, possibly
 * resulting in an empty (NULL) std::shared_ptr if the underlying dynamic_cast()
 * was not successful or if a NULL @a object was passed in.
 * Note that shared_from_this() can throw a std::bad_weak_ptr exception if
 * the object has no associated std::shared_ptr (usually during ctor and dtor), in
 * which case the exception will also be thrown from shared_ptr_cast<Target>().
 * However a shared_ptr_cast<Target*>() call will not throw and yield an empty
 * (NULL) std::shared_ptr<@a Target>. This is analogous to dynamic_cast<T@amp> which
 * throws, versus dynamic_cast<T*> which yields NULL.
 * @return A std::shared_ptr<@a Target> storing a pointer to @a object or NULL.
 * @throws std::bad_weak_ptr if shared_from_this() throws, unless the @a Target* form is used.
 */
template<class Target, class Source> std::shared_ptr<typename std::remove_pointer<Target>::type>
shared_ptr_cast (Source *object)
{
  if (!object)
    return NULL;
  // construct shared_ptr if possible
  typedef decltype (object->shared_from_this()) ObjectP;
  ObjectP sptr;
  if (std::is_pointer<Target>::value)
    try {
      sptr = object->shared_from_this();
    } catch (const std::bad_weak_ptr&) {
      return NULL;
    }
  else // for non-pointers, allow bad_weak_ptr exceptions
    sptr = object->shared_from_this();
  // cast into target shared_ptr<> type
  return std::dynamic_pointer_cast<typename std::remove_pointer<Target>::type> (sptr);
}
/// See shared_ptr_cast(Source*).
template<class Target, class Source> const std::shared_ptr<typename std::remove_pointer<Target>::type>
shared_ptr_cast (const Source *object)
{
  return shared_ptr_cast<Target> (const_cast<Source*> (object));
}
/// See shared_ptr_cast(Source*).
template<class Target, class Source> std::shared_ptr<typename std::remove_pointer<Target>::type>
shared_ptr_cast (std::shared_ptr<Source> &sptr)
{
  return std::dynamic_pointer_cast<typename std::remove_pointer<Target>::type> (sptr);
}
/// See shared_ptr_cast(Source*).
template<class Target, class Source> const std::shared_ptr<typename std::remove_pointer<Target>::type>
shared_ptr_cast (const std::shared_ptr<Source> &sptr)
{
  return shared_ptr_cast<Target> (const_cast<std::shared_ptr<Source>&> (sptr));
}

} // Bse

#endif // __BSE_CXXAUX_HH__
