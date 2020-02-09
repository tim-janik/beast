// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "memory.hh"
#include "bse/internal.hh"
#include <bse/testing.hh>
#include <sys/mman.h>

#define MEM_ALIGN(addr, alignment)      (alignment * ((size_t (addr) + alignment - 1) / alignment))
#define CHECK_FREE_OVERLAPS             0       /* paranoid chcks that slow down */

static constexpr size_t INTERNAL_MEMORY_AREA_SIZE = 4 * 1024 * 1024;

namespace Bse {

class LargeAllocation {
  using ReleaseMFP = void (LargeAllocation::*) ();
  void      *start_ = nullptr;
  size_t     length_ = 0;
  ReleaseMFP release_ = nullptr;
  void  free_start ()           { free (start_); }
  void  munmap_start ()         { munmap (start_, length_); }
  void  unadvise_free_start ()  { madvise (start_, length_, MADV_NOHUGEPAGE); free (start_); }
  /*ctor*/         LargeAllocation (void *m, size_t l, ReleaseMFP r) : start_ (m), length_ (l), release_ (r) {}
  explicit         LargeAllocation (const LargeAllocation&) = delete; // move-only
  LargeAllocation& operator=       (const LargeAllocation&) = delete; // move-only
public:
  /*move*/         LargeAllocation (LargeAllocation &&t) { operator= (std::move (t)); }
  /*ctor*/         LargeAllocation ()           {}
  /*dtor*/        ~LargeAllocation ()           { if (release_) (this->*release_) (); release_ = nullptr; }
  LargeAllocation& operator=       (LargeAllocation &&t)
  {
    std::swap (start_, t.start_);
    std::swap (length_, t.length_);
    std::swap (release_, t.release_);
    return *this;
  }
  char*  mem       () const { return (char*) start_; }
  size_t size      () const { return length_; }
  size_t alignment () const { return start_ ? size_t (1) << __builtin_ctz (size_t (start_)) : 0; }
  static LargeAllocation
  allocate (const size_t length, size_t minalign)
  {
    const size_t minhugepage = 2 * 1024 * 1024;
    // try reserved hugepages for large allocations
    if (length >= minhugepage)
      {
        const int protection = PROT_READ | PROT_WRITE;
        const int flags = MAP_PRIVATE | MAP_ANONYMOUS;
        void *memory = mmap (nullptr, length, protection, flags | MAP_HUGETLB, -1, 0);
        if (memory != MAP_FAILED)
          {
            assert_return ((size_t (memory) & (minalign - 1)) == 0, {}); // ensure alignment
            return { memory, length, &LargeAllocation::munmap_start };
          }
      }
    // try transparent hugepages for large allocations
    if (length >= minhugepage)
      {
        void *memory = std::aligned_alloc (std::max (minhugepage, minalign), length);
        if (memory)
          {
            assert_return ((size_t (memory) & (minalign - 1)) == 0, {}); // ensure alignment
            ReleaseMFP release;
            // linux/Documentation/admin-guide/mm/transhuge.rst
            if (madvise (memory, length, MADV_HUGEPAGE) >= 0)
              release = &LargeAllocation::unadvise_free_start;
            else
              release = &LargeAllocation::free_start;
            return { memory, length, release };
          }
      }
    // otherwise fallback to aligned_alloc for other allocations
    void *memory = std::aligned_alloc (minalign, length);
    if (!memory)
      return {};
    assert_return ((size_t (memory) & (minalign - 1)) == 0, {}); // ensure alignment
    return { memory, length, &LargeAllocation::free_start };
  }
};

struct Extent32 {
  uint32   start = 0;
  uint32   length = 0;
  explicit Extent32 (uint32 sz = 0) : length (sz) {}
  explicit Extent32 (uint32 st, uint32 len) : start (st), length (len) {}
  void     reset    (uint32 sz = 0)     { start = 0; length = sz; }
  void     zero     (char *area) const  { memset (area + start, 0, length); }
};

static uint32 shared_area_nextid = BSE_STARTID_MEMORY_AREA;

// SequentialFitAllocator
struct SequentialFitAllocator {
  LargeAllocation       blob;
  std::vector<Extent32> extents; // free list
  const uint32          mem_alignment, mem_id;
  bool                  external;
  SequentialFitAllocator (LargeAllocation &&newblob, uint32 alignment, bool is_external) :
    blob (std::move (newblob)), mem_alignment (alignment), mem_id (shared_area_nextid++), external (is_external)
  {
    assert_return (size() > 0);
    assert_return (mem_alignment <= blob.alignment());
    assert_return ((size_t (blob.mem()) & (blob.alignment() - 1)) == 0);
    assert_return (mem_id != 0);
    if (size() >= 1024 * 1024)
      extents.reserve (1024);
    assert_return (size() <= 4294967295);
    Extent32 area { 0, uint32_t (size()) };
    area.zero (blob.mem());
    release_ext (area);
    assert_return (area.length == blob.size());
  }
  ~SequentialFitAllocator()
  {
    const size_t s = sum();
    if (s != blob.size())
      warning ("%s:%s: deleting area while bytes are unreleased: %zd", __FILE__, __func__, blob.size() - s);
  }
  char*
  memory () const
  {
    return blob.mem();
  }
  size_t
  size () const
  {
    return blob.size();
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
  release_ext (const Extent32 &ext)
  {
    assert_return (ext.length > 0);
    assert_return (ext.start + ext.length <= blob.size());
    ext.zero (blob.mem());
    ssize_t overlaps_existing = -1, before = -1, after = -1;
    for (size_t i = 0; i < extents.size(); i++)
      if (ext.start == extents[i].start + extents[i].length)
        {
          after = i;
          if (UNLIKELY (before >= 0))
            break;
        }
      else if (ext.start + ext.length == extents[i].start)
        {
          before = i;
          if (UNLIKELY (after >= 0))
            break;
        }
      else if (CHECK_FREE_OVERLAPS &&
               ext.start + ext.length > extents[i].start &&
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
  }
  ssize_t
  fit_block (size_t length) const
  {
    ssize_t candidate = -1;
    for (size_t j = 0; j < extents.size(); j++)
      {
        const size_t i = extents.size() - 1 - j; // recent blocks are at the end
        if (ISLIKELY (length == extents[i].length))
          return i;
        if (UNLIKELY (length < extents[i].length) and
            (UNLIKELY (candidate < 0) ||
             extents[i].length < extents[candidate].length))
          candidate = i;
      }
    return candidate;
  }
  bool
  alloc_ext (Extent32 &ext)
  {
    assert_return (ext.start == 0, false);
    assert_return (ext.length > 0, false);
    const uint32 aligned_length = MEM_ALIGN (ext.length, mem_alignment);
    // find block
    const ssize_t candidate = fit_block (aligned_length);
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
#if 0 // only needed for deferred coalescing which rarely speeds things up
  void
  coalesce_extents()
  {
    if (extents.size())
      {
        auto isless_start = [] (const Extent32 &a, const Extent32 &b) -> bool {
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
  }
#endif
};
static std::vector<SequentialFitAllocator*> shared_areas;

static FastMemoryArea
create_internal_memory_area (uint32 mem_size, uint32 alignment, bool external)
{
  auto blob = LargeAllocation::allocate (MEM_ALIGN (mem_size, alignment), alignment);
  if (!blob.mem())
    fatal_error ("BSE: failed to allocate aligned memory (%u bytes): %s", mem_size, strerror (errno));
  shared_areas.push_back (new SequentialFitAllocator (std::move (blob), alignment, external));
  SequentialFitAllocator &sa = *shared_areas.back();
  assert_return (sa.mem_id == BSE_STARTID_MEMORY_AREA + shared_areas.size() - 1, {});
  return FastMemoryArea { sa.mem_id, sa.mem_alignment, uint64 (sa.memory()), sa.size() };
}

static SequentialFitAllocator*
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

FastMemoryArea
FastMemoryArea::find (uint32 mem_id)
{
  SequentialFitAllocator *sa = get_shared_area (mem_id);
  return sa ? FastMemoryArea { sa->mem_id, sa->mem_alignment, uint64 (sa->memory()), sa->size() } : FastMemoryArea{};
}

FastMemoryArea
FastMemoryArea::create (uint32 mem_size, uint32 alignment)
{
  return create_internal_memory_area (mem_size, alignment, true);
}

static FastMemoryBlock
fast_mem_allocate_aligned_block (uint32 length)
{
  FastMemoryBlock am;
  // allocate from shared memory areas
  Extent32 ext { 0, length };
  for (size_t i = 0; i < shared_areas.size(); i++)
    if (ISLIKELY (shared_areas[i]->external == false) and
        shared_areas[i]->alloc_ext (ext))
      {
        am.mem_id = shared_areas[i]->mem_id;
        am.block_length = ext.length;
        am.block_start = shared_areas[i]->memory() + ext.start;
        return am;
      }
  // allocate a new area
  const FastMemoryArea ma = create_internal_memory_area (INTERNAL_MEMORY_AREA_SIZE, FastMemoryArea::minimum_alignment, false);
  const uint32 mem_id = ma.mem_id;
  SequentialFitAllocator &sa = *get_shared_area (mem_id);
  const bool block_in_new_area = sa.alloc_ext (ext);
  assert_return (block_in_new_area, am);
  am.mem_id = sa.mem_id;
  am.block_length = ext.length;
  am.block_start = sa.memory() + ext.start;
  return am;
}

FastMemoryBlock
FastMemoryArea::allocate (uint32 length) const
{
  assert_return (length <= INTERNAL_MEMORY_AREA_SIZE, {});
  FastMemoryBlock am;
  return_unless (length > 0, {});
  Extent32 ext { 0, length };
  if (mem_id == 0)
    return fast_mem_allocate_aligned_block (length);
  // allocate from known memory area, mem_id != 0
  SequentialFitAllocator *memory_area_from_mem_id = get_shared_area (mem_id);
  assert_return (memory_area_from_mem_id != NULL, {});
  SequentialFitAllocator &sa = *memory_area_from_mem_id;
  if (sa.alloc_ext (ext))
    {
      am.mem_id = sa.mem_id;
      am.block_length = ext.length;
      am.block_start = sa.memory() + ext.start;
    }
  return am;
}

void
FastMemoryBlock::release () const
{
  SequentialFitAllocator *memory_area_from_mem_id = get_shared_area (mem_id);
  assert_return (memory_area_from_mem_id != NULL);
  SequentialFitAllocator &sa = *memory_area_from_mem_id;
  assert_return (block_start >= sa.memory());
  assert_return (block_start < sa.memory() + sa.size());
  const uint32 block_offset = ((char*) block_start) - sa.memory();
  assert_return (block_offset + block_length <= sa.size());
  Extent32 ext { block_offset, block_length };
  sa.release_ext (ext);
}

// == MemoryMetaTable ==
struct MemoryMetaInfo {
  std::mutex mutex;
  std::vector<Bse::FastMemoryBlock> ablocks;
};

static MemoryMetaInfo&
mm_info_lookup (void *ptr)
{
  static MemoryMetaInfo mm_info[1024];
  const size_t arrsz = sizeof (mm_info) / sizeof (mm_info[0]);
  union { uint64_t v; uint8_t a[8]; } u { uintptr_t (ptr) };
  const uint64_t M = 11400714819323198487ull; // golden ratio, rounded up to next odd
  const uint64_t S = 0xcbf29ce484222325;
  size_t hash = S; // swap a[0]..a[7] on big-endian for good avalange effect
  hash = (u.a[0] ^ hash) * M;
  hash = (u.a[1] ^ hash) * M;
  hash = (u.a[2] ^ hash) * M;
  hash = (u.a[3] ^ hash) * M;
  hash = (u.a[4] ^ hash) * M;
  hash = (u.a[5] ^ hash) * M;
  hash = (u.a[6] ^ hash) * M;
  hash = (u.a[7] ^ hash) * M;
  return mm_info[hash % arrsz];
}

static void
mm_info_push_mt (const FastMemoryBlock &ablock) // MT-Safe
{
  MemoryMetaInfo &mi = mm_info_lookup (ablock.block_start);
  std::lock_guard<std::mutex> locker (mi.mutex);
  mi.ablocks.push_back (ablock);
}

static FastMemoryBlock
mm_info_pop_mt (void *block_start) // MT-Safe
{
  MemoryMetaInfo &mi = mm_info_lookup (block_start);
  std::lock_guard<std::mutex> locker (mi.mutex);
  auto it = std::find_if (mi.ablocks.begin(), mi.ablocks.end(),
                          [block_start] (const auto &ab) {
                            return ab.block_start == block_start;
                          });
  FastMemoryBlock ab;
  if (it != mi.ablocks.end())   // found it, now pop
    {
      ab = *it;
      if (it < mi.ablocks.end() - 1)
        *it = mi.ablocks.back(); // swap with tail for quick shrinking
      mi.ablocks.resize (mi.ablocks.size() - 1);
    }
  return ab;
}

// == aligned malloc/calloc/free ==
static std::mutex fast_mem_mutex;

void*
fast_mem_alloc (size_t size)
{
  std::unique_lock<std::mutex> shortlock (fast_mem_mutex);
  FastMemoryBlock ab = FastMemoryArea { 0, }.allocate (size); // MT-Safe
  shortlock.unlock();
  void *const ptr = ab.block_start;
  if (ptr)
    mm_info_push_mt (ab);
  return ptr;
}

void
fast_mem_free (void *mem)
{
  if (mem)
    {
      FastMemoryBlock ab = mm_info_pop_mt (mem);
      if (ab.block_start)
        { // MT-Safe
          std::lock_guard<std::mutex> locker (fast_mem_mutex);
          ab.release();
        }
      else
        Bse::printerr ("%s: invalid memory pointer: %p\n", mem);
    }
}

} // Bse

// == Allocator Tests ==
#include "randomhash.hh"
namespace { // Anon
using namespace Bse;

BSE_INTEGRITY_TEST (bse_aligned_allocator_tests);
static void
bse_aligned_allocator_tests()
{
  const ssize_t kb = 1024, asz = 4 * 1024;
  // create small area
  const FastMemoryArea ma = create_internal_memory_area (asz, FastMemoryArea::minimum_alignment, true);
  SequentialFitAllocator *memory_area_from_mem_id = get_shared_area (ma.mem_id);
  assert_return (memory_area_from_mem_id != NULL);
  SequentialFitAllocator &sa = *memory_area_from_mem_id;
  assert_return (sa.sum() == asz);
  // allocate 4 * 1mb
  bool success;
  Extent32 s1 (kb);
  success = sa.alloc_ext (s1);
  assert_return (success);
  assert_return (sa.sum() == asz - kb);
  Extent32 s2 (kb - 1);
  success = sa.alloc_ext (s2);
  assert_return (success && s2.length == kb); // check alignment
  assert_return (sa.sum() == asz - 2 * kb);
  Extent32 s3 (kb);
  success = sa.alloc_ext (s3);
  assert_return (success);
  assert_return (sa.sum() == asz - 3 * kb);
  Extent32 s4 (kb);
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
  // test general purpose allocations exceeding a single FastMemoryArea
  std::vector<void*> ptrs;
  size_t sum = 0;
  while (sum < 37 * 1024 * 1024)
    {
      const size_t sz = random_irange (8, 98304);
      ptrs.push_back (fast_mem_alloc (sz));
      assert_return (ptrs.back() != nullptr);
      sum += sz;
    }
  while (!ptrs.empty())
    {
      fast_mem_free (ptrs.back());
      ptrs.pop_back();
    }
}

} // Anon
