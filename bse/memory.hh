// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_MEMORY_HH__
#define __BSE_MEMORY_HH__

#include <bse/bseenums.hh>

namespace Bse {

/// Utilities for allocating cache line aligned memory from huge pages.
namespace FastMemory {

/// Minimum alignment >= cache line size, see getconf LEVEL1_DCACHE_LINESIZE.
inline constexpr size_t cache_line_size = 64;

} // FastMemory

// Allocate cache-line aligned memory block from fast memory pool, MT-Safe.
void*   fast_mem_alloc  (size_t size);
// Free a memory block allocated with aligned_malloc(), MT-Safe.
void    fast_mem_free   (void *mem);

/// Array with cache-line-alignment containing a fixed numer of PODs.
template<typename T, size_t ALIGNMENT = FastMemory::cache_line_size>
class FastMemArray {
  static_assert (std::is_trivially_copyable<T>::value);
  static_assert (ALIGNMENT <= FastMemory::cache_line_size);
  static_assert ((ALIGNMENT & (ALIGNMENT - 1)) == 0);
  static_assert (alignof (T) <= ALIGNMENT);
  const size_t n_elements_ = 0;
  T     *const data_ = nullptr;
  BSE_CLASS_NON_COPYABLE (FastMemArray);
protected:
  void     range_check (size_t n) const
  {
    if (n >= n_elements_)
      throw std::out_of_range (string_format ("FastMemArray::range_check: n >= size(): %u >= %u", n, size()));
  }
public:
  FastMemArray (size_t n_elements) :
    n_elements_ (n_elements),
    data_ ((T*) fast_mem_alloc (sizeof (T) * n_elements_))
  {
    std::fill (begin(), end(), T());
  }
  FastMemArray (const vector<T>& elements) :
    n_elements_ (elements.size()),
    data_ ((T*) fast_mem_alloc (sizeof (T) * n_elements_))
  {
    std::copy (elements.begin(), elements.end(), begin());
  }
  ~FastMemArray()
  {
    static_assert (std::is_trivially_destructible<T>::value);
    fast_mem_free (data_);
  }
  T*       begin      ()                { return &data_[0]; }
  T*       end        ()                { return &data_[n_elements_]; }
  size_t   size       () const          { return n_elements_; }
  T&       operator[] (size_t n)        { return data_[n]; }
  const T& operator[] (size_t n) const  { return data_[n]; }
  T&       at         (size_t n)        { range_check (n); return data_[n]; }
  const T& at         (size_t n) const  { range_check (n); return data_[n]; }
};

namespace FastMemory {

// == NewDeleteBase ==
class NewDeleteBase {
  static constexpr const std::align_val_t __cxxalignment = std::align_val_t (__STDCPP_DEFAULT_NEW_ALIGNMENT__);
  static void    delete_  (void *ptr, std::size_t sz, std::align_val_t al);
  static void*   new_     (std::size_t sz, std::align_val_t al);
public:
  // 'static' is implicit for operator new/delete
  void* operator new      (std::size_t sz)                            { return new_ (sz, __cxxalignment); }
  void* operator new[]    (std::size_t sz)                            { return new_ (sz, __cxxalignment); }
  void* operator new      (std::size_t sz, std::align_val_t al)       { return new_ (sz, al); }
  void* operator new[]    (std::size_t sz, std::align_val_t al)       { return new_ (sz, al); }
  // variants without size_t MUST NOT be defined for the following to be used
  void  operator delete   (void *ptr, std::size_t sz)                 { delete_ (ptr, sz, __cxxalignment); }
  void  operator delete[] (void *ptr, std::size_t sz)                 { delete_ (ptr, sz, __cxxalignment); }
  void  operator delete   (void *ptr, std::size_t sz, std::align_val_t al) { delete_ (ptr, sz, al); }
  void  operator delete[] (void *ptr, std::size_t sz, std::align_val_t al) { delete_ (ptr, sz, al); }
};

/// Internal allocator handle.
struct Allocator;
using AllocatorP = std::shared_ptr<Allocator>;

/// Reference for an allocated memory block.
struct Block {
  void  *const block_start = nullptr;
  const uint32 block_length = 0;
  Block& operator= (const Block &src) { this->~Block(); new (this) Block (src); return *this; }
  /*copy*/ Block   (const Block &src) = default;
  /*dflt*/ Block   () = default;
};

/// Memory area (over-)aligned to cache size and utilizing huge pages.
struct Arena {
  /// Create isolated memory area.
  explicit Arena     (uint32 mem_size, uint32 alignment = cache_line_size);
  /// Alignment for block addresses and length.
  size_t   alignment () const;
  /// Address of memory area.
  uint64   location  () const;
  /// Reserved memory area in bytes.
  uint64   reserved  () const;
  /// Create a memory block from cache-line aligned memory area, MT-Unsafe.
  Block    allocate  (uint32 length) const;
  Block    allocate  (uint32 length, std::nothrow_t) const;
  /// Realease a previously allocated block, MT-Unsafe.
  void     release   (Block allocatedblock) const;
protected:
  AllocatorP fma; ///< Identifier for the associated memory allocator.
  explicit Arena     (AllocatorP);
};

} // FastMemory

} // Bse

#endif /* __BSE_MEMORY_HH__ */
