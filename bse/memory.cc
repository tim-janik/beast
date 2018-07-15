// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "memory.hh"
#include <sfi/testing.hh>

#define MEM_ALIGN(addr, alignment)      (alignment * ((size_t (addr) + alignment - 1) / alignment))
#define MEMORY_AREA_SIZE                (size_t (8) * 1024 * 1024)

namespace Bse {

struct SharedAreaExtent {
  uint32   start = 0;
  uint32   length = 0;
  explicit SharedAreaExtent (uint32 sz = 0) : length (sz) {}
  explicit SharedAreaExtent (uint32 st, uint32 len) : start (st), length (len) {}
  void     reset (uint32 sz = 0)                          { start = 0; length = sz; }
  void     zero (char *area) const                        { memset (area + start, 0, length); }
};

static uint32 shared_area_nextid = BSE_STARTID_MEMORY_AREA;

struct SharedArea {
  const uint32                  mem_id, mem_alignment;
  SharedAreaExtent              area;
  std::vector<SharedAreaExtent> extents; // free list
  char                         *memory = NULL;
  bool                          external;
  SharedArea (uint32 areasize, uint32 alignment, bool is_external) :
    mem_id (shared_area_nextid++), mem_alignment (alignment), external (is_external)
  {
    assert_return (areasize > 0 && alignment < areasize);
    assert_return (mem_id != 0);
    assert_return ((alignment & (alignment - 1)) == 0); // check for power of 2
    assert_return (area.length == 0 && areasize > 0);
    area.length = MEM_ALIGN (areasize, mem_alignment);
    assert_return (area.length >= areasize);
    area.start = 0;
    memory = new char[area.length];
    assert_return (memory != NULL);
    release_ext (area);
  }
  ~SharedArea()
  {
    const ssize_t s = sum();
    if (s != area.length)
      warning ("%s:%s: deleting area while bytes are unreleased: %zd", __FILE__, __func__, area.length - s);
    if (memory)
      delete[] memory;
  }
  size_t
  sum () const
  {
    size_t s = 0;
    for (const auto b : extents)
      s += b.length;
    return s;
  }
  void
  release_ext (const SharedAreaExtent &ext)
  {
    assert_return (ext.start >= area.start);
    assert_return (ext.length > 0);
    assert_return (ext.start + ext.length <= area.start + area.length);
    ssize_t overlaps_existing = -1, before = -1, after = -1;
    for (size_t i = 0; i < extents.size(); i++)
      if (ext.start == extents[i].start + extents[i].length)
        after = i;
      else if (ext.start + ext.length == extents[i].start)
        before = i;
      else if (ext.start + ext.length > extents[i].start &&
               ext.start < extents[i].start + extents[i].length)
        overlaps_existing = i;
    assert_return (overlaps_existing == -1);
    // merge with existing extents
    if (after >= 0)
      {
        extents[after].length += ext.length;
        if (before >= 0)
          {
            extents[after].length += extents[before].length;
            extents.erase (extents.begin() + before);
          }
        return;
      }
    if (before >= 0)
      {
        extents[before].length += ext.length;
        extents[before].start = ext.start;
        return;
      }
    // add isolated block to free list
    extents.push_back (ext);
    ext.zero (memory);
  }
  ssize_t
  fit_block (size_t length) const
  {
    ssize_t candidate = -1;
    for (size_t i = 0; i < extents.size(); i++)
      if (length == extents[i].length)
        return i;
      else if (length < extents[i].length)
        {
          if (candidate < 0)
            candidate = i;
          else if (extents[i].length < extents[candidate].length)
            candidate = i;
        }
    return candidate;
  }
  bool
  alloc_ext (SharedAreaExtent &ext)
  {
    assert_return (ext.start == 0, false);
    assert_return (ext.length > 0, false);
    const uint32 aligned_length = MEM_ALIGN (ext.length, mem_alignment);
    // find block
    ssize_t candidate = fit_block (aligned_length);
    // merge freed extents
    if (candidate < 0 && extents.size())
      {
        auto isless_start = [this] (const SharedAreaExtent &a, const SharedAreaExtent &b) -> bool {
          return a.start < b.start;
        };
        std::sort (extents.begin(), extents.end(), isless_start);
        for (size_t i = extents.size() - 1; i > 0; i--)
          if (extents[i-1].start + extents[i-1].length == extents[i].start) // adjacent
            {
              extents[i-1].length += extents[i].length;
              extents.erase (extents.begin() + i);
            }
      }
    // find block again
    candidate = fit_block (aligned_length);
    if (candidate < 0)
      return false;     // OOM
    // allocate from end of larger block
    extents[candidate].length -= aligned_length;
    ext.start = extents[candidate].start + extents[candidate].length;
    ext.length = aligned_length;
    // unlist if block wasn't larger
    if (extents[candidate].length == 0)
      extents.erase (extents.begin() + candidate);
    return true;
  }
};
static std::vector<SharedArea*> shared_areas;

static MemoryArea
create_internal_memory_area (uint32 mem_size, uint32 alignment, bool external)
{
  shared_areas.push_back (new SharedArea (mem_size, alignment, external));
  SharedArea &sa = *shared_areas.back();
  assert_return (sa.mem_id == BSE_STARTID_MEMORY_AREA + shared_areas.size() - 1, {});
  return MemoryArea { sa.mem_id, sa.mem_alignment, uint64 (sa.memory), sa.area.length };
}

static SharedArea*
get_shared_area (uint32 mem_id)
{
  if (mem_id >= BSE_STARTID_MEMORY_AREA)
    {
      const size_t index = mem_id - BSE_STARTID_MEMORY_AREA;
      if (index < shared_areas.size())
        return shared_areas[index];
    }
  return NULL;
}

MemoryArea
find_memory_area (uint32 mem_id)
{
  SharedArea *sa = get_shared_area (mem_id);
  return sa ? MemoryArea { sa->mem_id, sa->mem_alignment, uint64 (sa->memory), sa->area.length } : MemoryArea{};
}

MemoryArea
create_memory_area (uint32 mem_size, uint32 alignment)
{
  return create_internal_memory_area (mem_size, alignment, true);
}

AlignedBlock
allocate_aligned_block (uint32 mem_id, uint32 length)
{
  assert_return (length <= MEMORY_AREA_SIZE, {});
  AlignedBlock am;
  return_unless (length > 0, am);
  SharedAreaExtent ext { 0, length };
  // allocate from shared memory areas
  if (mem_id == 0)
    {
      for (size_t i = 0; i < shared_areas.size(); i++)
        if (shared_areas[i]->alloc_ext (ext))
          {
            am.mem_id = shared_areas[i]->mem_id;
            am.block_length = ext.length;
            am.block_start = shared_areas[i]->memory + ext.start;
            return am;
          }
      // allocate a new area
      const MemoryArea ma = create_internal_memory_area (MEMORY_AREA_SIZE, BSE_CACHE_LINE_ALIGNMENT, false);
      mem_id = ma.mem_id;
      SharedArea &sa = *get_shared_area (mem_id);
      const bool block_in_new_area = sa.alloc_ext (ext);
      assert_return (block_in_new_area, am);
      am.mem_id = sa.mem_id;
      am.block_length = ext.length;
      am.block_start = sa.memory + ext.start;
      return am;
    }
  // allocate from known memory area, mem_id != 0
  SharedArea *memory_area_from_mem_id = get_shared_area (mem_id);
  assert_return (memory_area_from_mem_id != NULL, {});
  SharedArea &sa = *memory_area_from_mem_id;
  if (sa.alloc_ext (ext))
    {
      am.mem_id = sa.mem_id;
      am.block_length = ext.length;
      am.block_start = sa.memory + ext.start;
    }
  return am;
}

void
release_aligned_block (const AlignedBlock &am)
{
  SharedArea *memory_area_from_mem_id = get_shared_area (am.mem_id);
  assert_return (memory_area_from_mem_id != NULL);
  SharedArea &sa = *memory_area_from_mem_id;
  assert_return (am.block_start >= sa.memory);
  assert_return (am.block_start < sa.memory + sa.area.length);
  const uint32 block_offset = ((char*) am.block_start) - sa.memory;
  assert_return (block_offset + am.block_length <= sa.area.length);
  SharedAreaExtent ext { block_offset, am.block_length };
  sa.release_ext (ext);
}

// TODO: alignment, huge pages, mmap

// == Allocator Tests ==
BSE_INTEGRITY_TEST (bse_aligned_allocator_tests);
static void
bse_aligned_allocator_tests()
{
  const ssize_t kb = 1024, asz = 4 * 1024;
  // create small area
  const MemoryArea ma = create_internal_memory_area (asz, BSE_CACHE_LINE_ALIGNMENT, true);
  SharedArea *memory_area_from_mem_id = get_shared_area (ma.mem_id);
  assert_return (memory_area_from_mem_id != NULL);
  SharedArea &sa = *memory_area_from_mem_id;
  assert_return (sa.sum() == asz);
  // allocate 4 * 1mb
  bool success;
  SharedAreaExtent s1 (kb);
  success = sa.alloc_ext (s1);
  assert_return (success);
  assert_return (sa.sum() == asz - kb);
  SharedAreaExtent s2 (kb - 1);
  success = sa.alloc_ext (s2);
  assert_return (success && s2.length == kb); // check alignment
  assert_return (sa.sum() == asz - 2 * kb);
  SharedAreaExtent s3 (kb);
  success = sa.alloc_ext (s3);
  assert_return (success);
  assert_return (sa.sum() == asz - 3 * kb);
  SharedAreaExtent s4 (kb);
  success = sa.alloc_ext (s4);
  assert_return (success);
  assert_return (sa.sum() == 0);
  // release with fragmentation
  sa.release_ext (s1);
  assert_return (sa.sum() == kb);
  sa.release_ext (s3);
  assert_return (sa.sum() == 2 * kb);
  // fail allocation due to fragmentation
  s1.reset (2 * kb);
  success = sa.alloc_ext (s1);
  assert_return (success == false);
  // release middle block and allocate coalesced result
  sa.release_ext (s2);
  assert_return (sa.sum() == 3 * kb);
  s1.reset (3 * kb);
  success = sa.alloc_ext (s1);
  assert_return (success);
  assert_return (sa.sum() == 0);
  // release all
  sa.release_ext (s1);
  sa.release_ext (s4);
  assert_return (sa.sum() == asz);
}

} // Bse
