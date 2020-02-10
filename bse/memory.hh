// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_MEMORY_HH__
#define __BSE_MEMORY_HH__

#include <bse/bseenums.hh>

namespace Bse {

/// Internal allocator handle.
struct FastMemoryAllocator;

/// Reference for an allocated memory block.
struct FastMemoryBlock {
  FastMemoryAllocator *fma = nullptr;
  uint32 block_length = 0;
  void  *block_start = nullptr;
  /// Realease a previously allocated block, MT-Unsafe.
  void   release () const;
};

/// Memory area (over-)aligned to cache size and utilizing huge pages.
struct FastMemoryArea {
  FastMemoryAllocator *fma = nullptr; ///< Identifier for the associated memory allocator.
  uint32 mem_alignment = 0;    ///< Alignment for block addresses and length.
  uint64 mem_start = 0;        ///< Memory area location.
  uint64 mem_length = 0;       ///< Memory area length in bytes
  /// Minimum alignment >= cache line size, see getconf LEVEL1_DCACHE_LINESIZE.
  static constexpr size_t minimum_alignment = 64;
  /// Create a memory block from cache-line aligned memory area, MT-Unsafe.
  FastMemoryBlock       allocate (uint32 length) const;
  FastMemoryBlock       allocate (uint32 length, std::nothrow_t) const;
  /// Create isolated memory area.
  static FastMemoryArea create   (uint32 mem_size, uint32 alignment = minimum_alignment);
};

// Allocate cache-line aligned memory block from fast memory pool, MT-Safe.
void*   fast_mem_alloc  (size_t size);
// Free a memory block allocated with aligned_malloc(), MT-Safe.
void    fast_mem_free   (void *mem);

} // Bse

#endif /* __BSE_MEMORY_HH__ */
