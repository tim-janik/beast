// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_BCORE_HH__
#define __BSE_BCORE_HH__

#include <sfi/blob.hh>
#include <sfi/platform.hh>
#include <sfi/strings.hh>
#include <sfi/glib-extra.hh>
#include <numeric>
#include <algorithm>
#include <list>

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
using   Aida::void_t;

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

// == Small Utilities ==
/// Erase element @a value from std::vector @a v if it's present.
template<class V> bool
vector_erase_element (V &v, const typename V::value_type &value)
{
  typename V::iterator it = std::find (v.begin(), v.end(), value);
  if (it != v.end())
    {
      v.erase (it);
      return true;
    }
  return false;
}

/// Copy @a unordered_first .. @a unordered_end into @a output_iterator in the order given by @a ordered_first .. @a ordered_end.
template<class InputIterator, class OutputIterator> OutputIterator
copy_reordered (InputIterator const unordered_first, InputIterator const unordered_end,
                InputIterator const ordered_first, InputIterator const ordered_end,
                OutputIterator output_iterator)
{
  static_assert (std::is_same<std::random_access_iterator_tag,
                 typename std::iterator_traits<InputIterator>::iterator_category>::value,
                 "vector_copy_reordered() requires random access iterator as input");
  std::vector<bool> taken;
  taken.resize (unordered_end - unordered_first, false);
  // insert all ordered_first.. elements if present in unordered_first..
  for (InputIterator it = ordered_first; it != ordered_end; ++it)
    {
      InputIterator pos = std::find (unordered_first, unordered_end, *it);
      while (pos != unordered_end && taken[pos - unordered_first])
        pos = std::find (++pos, unordered_end, *it);   // keep searching for dups
      if (pos != unordered_end) // && !taken
        {
          taken[pos - unordered_first] = true;
          *output_iterator++ = *it;
        }
    }
  // insert all unordered_first.. elements not previously encountered
  for (ssize_t i = 0; i < unordered_end - unordered_first; i++)
    if (taken[i] == false)
      *output_iterator++ = *(unordered_first + i);
  return output_iterator;
}

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
extern bool                     debug_any_enabled;  //< Indicates if $BSE_DEBUG enables some debug settings.
bool                            debug_key_enabled       (const char *conditional) BSE_PURE;
void                            debug_diagnostic        (const char *prefix, const std::string &message);
std::string                     diagnostic_message      (const char *file, int line, const char *func, char kind, const std::string &info);
void                            diagnostic              (const char *file, int line, const char *func, char kind, const std::string &info);
void                            fatal_abort             (const std::string &message) BSE_NORETURN BSE_NOINLINE;
void                            printout_string         (const String &string);
void                            printerr_string         (const String &string);
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
    Internal::diagnostic (NULL, 0, NULL, ' ', string_format (format, args...));
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
  const String msg = Internal::diagnostic_message (NULL, 0, NULL, 'F', string_format (format, args...));
  printerr ("%s", msg);
  Internal::fatal_abort (msg);
}

/// Issue a printf-like warning message.
template<class ...Args> void BSE_NORETURN
warn (const char *format, const Args &...args)
{
  Internal::diagnostic (NULL, 0, NULL, 'W', string_format (format, args...));
}

/// Issue a printf-like warning message.
template<class ...Args> void BSE_NORETURN
warning (const char *format, const Args &...args)
{
  Internal::diagnostic (NULL, 0, NULL, 'W', string_format (format, args...));
}

/// Issue an informative printf-like message.
template<class ...Args> void BSE_NORETURN
info (const char *format, const Args &...args)
{
  Internal::diagnostic (NULL, 0, NULL, 'I', string_format (format, args...));
}

// == External Helpers ==
bool url_show (const char *url); ///< Display @a url via a suitable WWW user agent.

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
/// Yield the number of C @a array elements.
#define ARRAY_SIZE(array)               BSE_ARRAY_SIZE (array)
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
  BSE_CLASS_NON_COPYABLE (AlignedArray);
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

// == AsyncBlockingQueue ==
/** Asyncronous queue to push/pop values across thread boundaries.
 * The AsyncBlockingQueue is a thread-safe asyncronous queue which blocks in pop() until data is provided through push() from any thread.
 */
template<class Value>
class AsyncBlockingQueue {
  std::mutex              mutex_;
  std::condition_variable cond_;
  std::list<Value>        list_;
public:
  void  push    (const Value &v);
  Value pop     ();
  bool  pending ();
  void  swap    (std::list<Value> &list);
};

template<class Value> void
AsyncBlockingQueue<Value>::push (const Value &v)
{
  std::lock_guard<std::mutex> locker (mutex_);
  const bool notify = list_.empty();
  list_.push_back (v);
  if (BSE_UNLIKELY (notify))
    cond_.notify_all();
}

template<class Value> Value
AsyncBlockingQueue<Value>::pop ()
{
  std::unique_lock<std::mutex> locker (mutex_);
  while (list_.empty())
    cond_.wait (locker);
  Value v = list_.front();
  list_.pop_front();
  return v;
}

template<class Value> bool
AsyncBlockingQueue<Value>::pending()
{
  std::lock_guard<std::mutex> locker (mutex_);
  return !list_.empty();
}

template<class Value> void
AsyncBlockingQueue<Value>::swap (std::list<Value> &list)
{
  std::lock_guard<std::mutex> locker (mutex_);
  const bool notify = list_.empty();
  list_.swap (list);
  if (notify && !list_.empty())
    cond_.notify_all();
}

} // Bse

#endif // __BSE_BCORE_HH__
