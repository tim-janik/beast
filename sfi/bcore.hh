// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_BCORE_HH__
#define __BSE_BCORE_HH__

#include <sfi/platform.hh>
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
using   Aida::Any;
using   Aida::EventFd;
using   Rapicorn::url_show;
using   Rapicorn::DataKey;
using   Rapicorn::DataListContainer;
using   Rapicorn::void_t;
using   Rapicorn::Blob;
using   Rapicorn::Res;
using   Rapicorn::TaskStatus;
using   Rapicorn::ThreadInfo;
using   Rapicorn::cpu_info;
using   Rapicorn::AsyncBlockingQueue;
using   Rapicorn::random_int64;
using   Rapicorn::random_float;
using   Rapicorn::random_irange;
using   Rapicorn::random_frange;
namespace ThisThread = Rapicorn::ThisThread;

// == Diagnostics ==
template<class... Args> String      string_format        (const char *format, const Args &...args) BSE_PRINTF (1, 0);
template<class... Args> String      string_locale_format (const char *format, const Args &...args) BSE_PRINTF (1, 0);
template<class... Args> void        printout             (const char *format, const Args &...args) BSE_PRINTF (1, 0);
template<class... Args> void        printerr             (const char *format, const Args &...args) BSE_PRINTF (1, 0);
template<class ...Args> void        fatal_error          (const char *format, const Args &...args) BSE_NORETURN;
template<class ...Args> void        warning              (const char *format, const Args &...args);
template<class ...Args> void        warn                 (const char *format, const Args &...args);
template<class ...Args> void        info                 (const char *format, const Args &...args);
template<class ...Args> inline void dump                 (const char *conditional, const char *format, const Args &...args) BSE_ALWAYS_INLINE;
template<class ...Args> inline void debug                (const char *conditional, const char *format, const Args &...args) BSE_ALWAYS_INLINE;
inline bool                         debug_enabled        (const char *conditional) BSE_ALWAYS_INLINE BSE_PURE;
String                              feature_toggle_find  (const String &config, const String &feature, const String &fallback = "0");
bool                                feature_toggle_bool  (const char *config, const char *feature);

// == Binary Lookups ==
template<typename RandIter, class Cmp, typename Arg, int case_lookup_or_sibling_or_insertion>
extern inline std::pair<RandIter,bool>
binary_lookup_fuzzy (RandIter begin, RandIter end, Cmp cmp_elements, const Arg &arg)
{
  RandIter current = end;
  size_t n_elements = end - begin, offs = 0;
  const bool want_lookup = case_lookup_or_sibling_or_insertion == 0;
  // const bool want_sibling = case_lookup_or_sibling_or_insertion == 1;
  const bool want_insertion_pos = case_lookup_or_sibling_or_insertion > 1;
  ssize_t cmp = 0;
  while (offs < n_elements)
    {
      size_t i = (offs + n_elements) >> 1;
      current = begin + i;
      cmp = cmp_elements (arg, *current);
      if (cmp == 0)
        return want_insertion_pos ? std::make_pair (current, true) : std::make_pair (current, /*ignored*/ false);
      else if (cmp < 0)
        n_elements = i;
      else /* (cmp > 0) */
        offs = i + 1;
    }
  /* check is last mismatch, cmp > 0 indicates greater key */
  return (want_lookup
          ? std::make_pair (end, /*ignored*/ false)
          : (want_insertion_pos && cmp > 0)
          ? std::make_pair (current + 1, false)
          : std::make_pair (current, false));
}

/** Perform a binary lookup to find the insertion position for a new element.
 * Return (end,false) for end-begin==0, or return (position,true) for exact match,
 * otherwise return (position,false) where position indicates the location for
 * the key to be inserted (and may equal end).
 */
template<typename RandIter, class Cmp, typename Arg>
extern inline std::pair<RandIter,bool>
binary_lookup_insertion_pos (RandIter begin, RandIter end, Cmp cmp_elements, const Arg &arg)
{
  return binary_lookup_fuzzy<RandIter,Cmp,Arg,2> (begin, end, cmp_elements, arg);
}

/** Perform a binary lookup to yield exact or nearest match.
 * return end for end-begin==0, otherwise return the exact match element, or,
 * if there's no such element, return the element last visited, which is pretty
 * close to an exact match (will be one off into either direction).
 */
template<typename RandIter, class Cmp, typename Arg>
extern inline RandIter
binary_lookup_sibling (RandIter begin, RandIter end, Cmp cmp_elements, const Arg &arg)
{
  return binary_lookup_fuzzy<RandIter,Cmp,Arg,1> (begin, end, cmp_elements, arg).first;
}

/** Perform binary lookup and yield exact match or @a end.
 * The arguments [ @a begin, @a end [ denote the range used for the lookup,
 * @a arg is passed along with the current element to the @a cmp_elements
 * function.
 */
template<typename RandIter, class Cmp, typename Arg>
extern inline RandIter
binary_lookup (RandIter begin, RandIter end, Cmp cmp_elements, const Arg &arg)
{
  /* return end or exact match */
  return binary_lookup_fuzzy<RandIter,Cmp,Arg,0> (begin, end, cmp_elements, arg).first;
}

/// Comparison function useful to sort lesser items first.
template<typename Value> static inline int
compare_lesser (const Value &v1, const Value &v2)
{
  return -(v1 < v2) | (v2 < v1);
}

/// Comparison function useful to sort greater items first.
template<typename Value> static inline int
compare_greater (const Value &v1, const Value &v2)
{
  return (v1 < v2) | -(v2 < v1);
}

// == Implementation Details ==
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

/** Issue a printf-like message and abort the program, this function will not return.
 * Avoid using this in library code, aborting may take precious user data with it,
 * library code should instead use info() or assert_return().
 */
template<class ...Args> void BSE_NORETURN
fatal_error (const char *format, const Args &...args)
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

// == Memory Utilities ==
int     fmsb          (uint64  word) BSE_CONST; ///< Find most significant bit set in a word.
void*   aligned_alloc (size_t  total_size, size_t alignment, uint8 **free_pointer);
void    aligned_free  (uint8 **free_pointer);

/// Class to maintain an array of aligned memory.
template<class T, int ALIGNMENT>
class AlignedArray {
  uint8 *unaligned_mem_;
  T     *data_;
  size_t n_elements_;
  void
  allocate_aligned_data()
  {
    static_assert (ALIGNMENT % sizeof (T) == 0, "ALIGNMENT must exactly fit a multiple of sizeof (T)");
    data_ = reinterpret_cast<T*> (aligned_alloc (n_elements_ * sizeof (T), ALIGNMENT, &unaligned_mem_));
  }
  // disallow copy constructor assignment operator
  RAPICORN_CLASS_NON_COPYABLE (AlignedArray);
public:
  AlignedArray (const vector<T>& elements) :
    n_elements_ (elements.size())
  {
    allocate_aligned_data();
    for (size_t i = 0; i < n_elements_; i++)
      new (data_ + i) T (elements[i]);
  }
  AlignedArray (size_t n_elements) :
    n_elements_ (n_elements)
  {
    allocate_aligned_data();
    for (size_t i = 0; i < n_elements_; i++)
      new (data_ + i) T();
  }
  ~AlignedArray()
  {
    // C++ destruction order: last allocated element is deleted first
    while (n_elements_)
      data_[--n_elements_].~T();
    aligned_free (&unaligned_mem_);
  }
  T&            operator[] (size_t pos)         { return data_[pos]; }
  const T&      operator[] (size_t pos) const   { return data_[pos]; }
  size_t        size       () const             { return n_elements_; }
};

// == Threading ==
/**
 * The Spinlock uses low-latency busy spinning to acquire locks.
 * This class is a thin wrapper around pthread_spin_lock() and related functions.
 * This class supports static construction.
 */
class Spinlock {
  pthread_spinlock_t spinlock_;
public:
  constexpr Spinlock    () : spinlock_ { BSE_SPINLOCK_INITIALIZER } {}
  void      lock        ()      { pthread_spin_lock (&spinlock_); }
  void      unlock      ()      { pthread_spin_unlock (&spinlock_); }
  bool      try_lock    ()      { return 0 == pthread_spin_trylock (&spinlock_); }
  typedef pthread_spinlock_t* native_handle_type;
  native_handle_type native_handle() { return &spinlock_; }
  /*ctor*/  Spinlock    (const Spinlock&) = delete;
  Spinlock& operator=   (const Spinlock&) = delete;
};

} // Bse

#endif // __BSE_BCORE_HH__
