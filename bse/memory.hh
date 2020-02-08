// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_MEMORY_HH__
#define __BSE_MEMORY_HH__

#include <bse/bseenums.hh>

namespace Bse {

struct FastMemoryBlock {
  uint32 mem_id = 0;
  uint32 block_length = 0;
  void  *block_start = nullptr;
  /// Realease a previously allocated block.
  void   release () const;
};

/// Memory area (over-)aligned to cache size and utilizing huge pages.
struct FastMemoryArea {
  uint32 mem_id = 0;           ///< Identifier for the associated memory area.
  uint32 mem_alignment = 0;    ///< Alignment for block addresses and length.
  uint64 mem_start = 0;        ///< Memory area location.
  uint64 mem_length = 0;       ///< Memory area length in bytes
  /// Minimum alignment >= cache line size, see getconf LEVEL1_DCACHE_LINESIZE.
  static constexpr size_t minimum_alignment = 64;
  /// Create a memory block from memory area @a mem_id, uses the internal cache-line aligned pool.
  FastMemoryBlock       allocate (uint32 length) const;
  /// Create isolated memory area.
  static FastMemoryArea create   (uint32 mem_size, uint32 alignment = minimum_alignment);
  /// Lookup a previously created memory area.
  static FastMemoryArea find     (uint32 mem_id);
};



} // Bse

#endif /* __BSE_MEMORY_HH__ */
