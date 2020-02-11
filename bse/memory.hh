// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_MEMORY_HH__
#define __BSE_MEMORY_HH__

#include <bse/bseenums.hh>

namespace Bse {

// Allocate cache-line aligned memory block from fast memory pool, MT-Safe.
void*   fast_mem_alloc  (size_t size);
// Free a memory block allocated with aligned_malloc(), MT-Safe.
void    fast_mem_free   (void *mem);

namespace FastMemory {

/// Minimum alignment >= cache line size, see getconf LEVEL1_DCACHE_LINESIZE.
constexpr inline size_t cache_line_size = 64;

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
