// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_MEMORY_HH__
#define __BSE_MEMORY_HH__

#include <bse/bseenums.hh>

// #define BSE_CACHE_LINE_ALIGNMENT   64      // generally enough on x86, see getconf LEVEL1_DCACHE_LINESIZE

namespace Bse {

struct MemoryArea {
  uint32 mem_id = 0;           ///< Identifier for the associated memory area.
  uint32 mem_alignment = 0;    ///< Alignment for block addresses and length.
  uint64 mem_start = 0;        ///< Memory area location.
  uint64 mem_length = 0;       ///< Memory area length in bytes
  static const constexpr size_t MEMORY_AREA_SIZE = size_t (4) * 1024 * 1024;
};

struct AlignedBlock {
  uint32 mem_id = 0;
  uint32 block_length = 0;
  void  *block_start = NULL;
};

/// Create isolated memory area, the MemoryArea.mem_id can be used for allocate_aligned_block().
MemoryArea      create_memory_area      (uint32 mem_size, uint32 alignment = BSE_CACHE_LINE_ALIGNMENT);
MemoryArea      find_memory_area        (uint32 mem_id);                ///< Lookup a previously created memory area.

/// Create a memory block from memory area @a mem_id, if 0, uses the internal cache-line aligned pool.
AlignedBlock    allocate_aligned_block  (uint32 mem_id, uint32 length);
void            release_aligned_block   (const AlignedBlock &block);    ///< Realease a previously allocated block.

} // Bse

#endif /* __BSE_MEMORY_HH__ */
