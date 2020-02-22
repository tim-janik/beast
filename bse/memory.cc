// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "memory.hh"
#include "bse/internal.hh"
#include <bse/testing.hh>
#include <sys/mman.h>
#include <unistd.h>     // _SC_PAGESIZE

#define MEM_ALIGN(addr, alignment)      (alignment * size_t ((size_t (addr) + alignment - 1) / alignment))
#define CHECK_FREE_OVERLAPS             0       /* paranoid chcks that slow down */

inline constexpr size_t MINIMUM_ARENA_SIZE = 4 * 1024 * 1024;
inline constexpr size_t MINIMUM_HUGEPAGE = 2 * 1024 * 1024;

namespace Bse {

namespace FastMemory {

class LargeAllocation {
  using ReleaseMFP = void (LargeAllocation::*) ();
  void      *start_ = nullptr;
  size_t     length_ = 0;
  ReleaseMFP release_ = nullptr;
  void  free_start ()            { free (start_); }
  void  munmap_start ()          { munmap (start_, length_); }
  void  unadvise_free_start ()   { madvise (start_, length_, MADV_NOHUGEPAGE); free_start(); }
  void  unadvise_munmap_start () { madvise (start_, length_, MADV_NOHUGEPAGE); munmap_start(); }
  /*ctor*/         LargeAllocation (void *m, size_t l, ReleaseMFP r) : start_ (m), length_ (l), release_ (r) {}
  explicit         LargeAllocation (const LargeAllocation&) = delete; // move-only
  LargeAllocation& operator=       (const LargeAllocation&) = delete; // move-only
public:
  /*move*/         LargeAllocation (LargeAllocation &&t) { operator= (std::move (t)); }
  /*ctor*/         LargeAllocation ()           {}
  /*dtor*/        ~LargeAllocation ()
  {
    if (release_)
      (this->*release_) ();
    release_ = nullptr;
    start_ = nullptr;
    length_ = 0;
  }
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
  allocate (const size_t length, size_t minalign, bool willgrow)
  {
    assert_return (0 == (minalign & (minalign - 1)), {});
    constexpr int protection = PROT_READ | PROT_WRITE;
    constexpr int flags = MAP_PRIVATE | MAP_ANONYMOUS;
    // try reserved hugepages for large allocations
    if (length == MEM_ALIGN (length, MINIMUM_HUGEPAGE) && minalign <= MINIMUM_HUGEPAGE)
      {
        void *memory = mmap (nullptr, length, protection, flags | MAP_HUGETLB, -1, 0);
        if (memory != MAP_FAILED)
          {
            assert_return ((size_t (memory) & (minalign - 1)) == 0, {}); // ensure alignment
            return { memory, length, &LargeAllocation::munmap_start };
          }
      }
    // try transparent hugepages for large allocations and large alignments
    if (length == MEM_ALIGN (length, std::max (minalign, MINIMUM_HUGEPAGE)))
      {
        static const size_t pagesize = sysconf (_SC_PAGESIZE);
        minalign = std::max (minalign, MINIMUM_HUGEPAGE);
        size_t areasize = minalign - pagesize + length;
        char *memory = (char*) mmap (nullptr, areasize, protection, flags, -1, 0);
        if (memory)
          {
            // discard unaligned head
            const uintptr_t start = uintptr_t (memory);
            size_t extra = MEM_ALIGN (start, minalign) - start;
            if (extra && munmap (memory, extra) != 0)
              printerr ("%s: munmap(%p,%u) failed: %s\n", __func__, memory, extra, strerror (errno));
            memory += extra;
            areasize -= extra;
            // discard unaligned tail
            extra = areasize - size_t (areasize / minalign) * minalign;
            areasize -= extra;
            if (extra && munmap (memory + areasize, extra) != 0)
              printerr ("%s: munmap(%p,%u) failed: %s\n", __func__, memory + areasize, extra, strerror (errno));
            // double check, use THP
            assert_return (areasize == length, {});
            assert_return ((size_t (memory) & (minalign - 1)) == 0, {}); // ensure alignment
            ReleaseMFP release;
            // linux/Documentation/admin-guide/mm/transhuge.rst
            if (madvise (memory, areasize, MADV_HUGEPAGE) >= 0)
              release = &LargeAllocation::unadvise_munmap_start;
            else
              release = &LargeAllocation::munmap_start;
            return { memory, areasize, release };
          }
      }
    // fallback to aligned_alloc with hugepages
    if (length == MEM_ALIGN (length, MINIMUM_HUGEPAGE) && minalign <= MINIMUM_HUGEPAGE)
      {
        void *memory = std::aligned_alloc (std::max (MINIMUM_HUGEPAGE, minalign), length);
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
    // otherwise fallback to just aligned_alloc
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
  void     zero     (char *area) const  { memset (__builtin_assume_aligned (area + start, cache_line_size), 0, length); }
};

// SequentialFitAllocator
struct SequentialFitAllocator {
  LargeAllocation       blob;
  std::vector<Extent32> extents; // free list
  const uint32          mem_alignment;
  SequentialFitAllocator (LargeAllocation &&newblob, uint32 alignment) :
    blob (std::move (newblob)), mem_alignment (alignment)
  {
    assert_return (size() > 0);
    assert_return (mem_alignment <= blob.alignment());
    assert_return ((size_t (blob.mem()) & (blob.alignment() - 1)) == 0);
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
    extents.clear();
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
  best_fit (size_t length) const
  {
    ssize_t candidate = -1;
    for (size_t j = 0; j < extents.size(); j++)
      {
        const size_t i = extents.size() - 1 - j; // recent blocks are at the end
        if (ISLIKELY (length > extents[i].length))
          continue;                                     // profiled, ISLIKELY saves ~7%
        if (ISLIKELY (length == extents[i].length))     // profiled, ISLIKELY saves ~20%
          return i;
        // length < extents[i].length
        if (ISLIKELY (candidate < 0) or                 // profiled, ISLIKELY saves ~20%
            ISLIKELY (extents[i].length < extents[candidate].length) or
            (ISLIKELY (extents[i].length == extents[candidate].length) and
             ISLIKELY (extents[i].start < extents[candidate].start)))
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
    const ssize_t candidate = best_fit (aligned_length);
    if (candidate < 0)
      return false;     // OOM
    // allocate from start of larger block (to facilitate future Arena growth)
    ext.start = extents[candidate].start;
    ext.length = aligned_length;
    if (UNLIKELY (extents[candidate].length > aligned_length))
      {
        extents[candidate].start += aligned_length;
        extents[candidate].length -= aligned_length;
      }
    else // unlist if block wasn't larger
      {
        // extents.erase (extents.begin() + candidate);
        extents[candidate] = extents.back();
        extents.resize (extents.size() - 1);
      }
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

struct Allocator : SequentialFitAllocator {
  Allocator (LargeAllocation &&newblob, uint32 alignment) :
    SequentialFitAllocator (std::move (newblob), alignment)
  {}
};

struct FastMemoryArena : Arena {
  FastMemoryArena (AllocatorP a) : Arena (a) {}
  static Allocator*
  allocator (const Arena &base)
  {
    static_assert (sizeof (Arena) == sizeof (FastMemoryArena));
    return reinterpret_cast<const FastMemoryArena*> (&base)->fma.get();
  }
};
static Allocator* fmallocator (const Arena &a) { return FastMemoryArena::allocator (a); }

// == Arena ==
Arena::Arena (AllocatorP xfma) :
  fma (xfma)
{}

static Arena
create_arena (uint32 mem_size, uint32 alignment, bool willgrow)
{
  alignment = std::max (alignment, uint32 (cache_line_size));
  mem_size = MEM_ALIGN (mem_size, alignment);
  auto blob = LargeAllocation::allocate (mem_size, alignment, willgrow);
  if (!blob.mem())
    fatal_error ("BSE: failed to allocate aligned memory (%u bytes): %s", mem_size, strerror (errno));
  FastMemory::AllocatorP fmap = std::make_shared<FastMemory::Allocator> (std::move (blob), alignment);
  return FastMemoryArena (fmap);
}

Arena::Arena (uint32 mem_size, uint32 alignment)
{
  assert_return (alignment <= 2147483648);
  assert_return (0 == (alignment & (alignment - 1)));
  assert_return (mem_size <= 2147483648);
  *this = create_arena (mem_size, alignment, false);
}

uint64
Arena::location () const
{
  return fma ? uint64 (fma->memory()) : 0;
}

uint64
Arena::reserved () const
{
  return fma ? fma->size() : 0;
}

size_t
Arena::alignment () const
{
  return fma ? fma->mem_alignment : 0;
}

Block
Arena::allocate (uint32 length, std::nothrow_t) const
{
  const Block zeroblock = { nullptr, 0 };
  assert_return (fma, zeroblock);
  return_unless (length > 0, zeroblock);
  Extent32 ext { 0, length };
  if (fma->alloc_ext (ext))
    return Block { fma->memory() + ext.start, ext.length };
  // FIXME: try growing
  return zeroblock;
}

Block
Arena::allocate (uint32 length) const
{
  Block ab = allocate (length, std::nothrow);
  if (!ab.block_start)
    throw std::bad_alloc();
  return ab;
}

void
Arena::release (Block ab) const
{
  assert_return (fma);
  assert_return (ab.block_start >= fma->memory());
  assert_return (ab.block_start < fma->memory() + fma->size());
  assert_return (0 == (size_t (ab.block_start) & alignment() - 1));
  assert_return (0 == (size_t (ab.block_length) & alignment() - 1));
  const size_t block_offset = ((char*) ab.block_start) - fma->memory();
  assert_return (block_offset + ab.block_length <= fma->size());
  Extent32 ext { uint32 (block_offset), ab.block_length };
  fma->release_ext (ext);
}

struct EmptyArena : Arena {
  EmptyArena() :
    Arena (AllocatorP (nullptr))
  {}
};

// == NewDeleteBase ==
static constexpr bool trace_NewDeleteBase = false;

void
NewDeleteBase::delete_ (void *ptr, std::size_t sz, std::align_val_t al)
{
  if (trace_NewDeleteBase)
    Bse::printerr ("del: %p (%d, %d)\n", ptr, sz, al);
  // sz and al are both bogus if delete is called via base class
  fast_mem_free (ptr);
  //::operator delete[] (ptr, al);
}

void*
NewDeleteBase::new_ (std::size_t sz, std::align_val_t al)
{
  //auto ptr = ::operator new[] (sz, al);
  void *ptr = nullptr;
  if (size_t (al) <= FastMemory::cache_line_size)
    ptr = fast_mem_alloc (sz);
  if (trace_NewDeleteBase)
    Bse::printerr ("new: %p (%d, %d)\n", ptr, sz, al);
  if (!ptr)
    throw std::bad_alloc();
  return ptr;
}

// == ArenaBlock ==
static std::mutex fast_mem_mutex;
static std::vector<FastMemory::Arena> &fast_mem_arenas = *new std::vector<FastMemory::Arena>();

struct ArenaBlock {
  void  *block_start = nullptr;
  uint32 block_length = 0;
  uint32 arena_index = ~0;
  ArenaBlock () = default;
  ArenaBlock (void *ptr, uint32 length, uint32 index) :
    block_start (ptr), block_length (length), arena_index (index)
  {
    assert_return (index < fast_mem_arenas.size());
  }
  ArenaBlock& operator= (const ArenaBlock &src) = default;
  /*copy*/ ArenaBlock   (const ArenaBlock &src) = default;
  Block    block        () const        { return { block_start, block_length }; }
};

static ArenaBlock
fast_mem_allocate_aligned_block (uint32 length)
{
  // try to allocate from existing arenas
  Extent32 ext { 0, length };
  for (uint32 i = 0; i < fast_mem_arenas.size(); i++)
    {
      FastMemory::Allocator *fma = fmallocator (fast_mem_arenas[i]);
      if (fma->alloc_ext (ext))
        {
          void *const ptr = fma->memory() + ext.start;
          return { ptr, ext.length, i };
        }
    }
  // FIXME: try to grow the *last* arena first
  // allocate a new area
  const bool willgrow = fast_mem_arenas.size() > 0; // prepare for growth after the first is too small
  Arena arena = create_arena (std::max (size_t (length), MINIMUM_ARENA_SIZE), cache_line_size, willgrow);
  assert_return (fmallocator (arena), {});
  const uint32 arena_index = fast_mem_arenas.size();
  fast_mem_arenas.push_back (arena);
  FastMemory::Allocator *fma = fmallocator (arena);
  if (fma->alloc_ext (ext))
    {
      void *const ptr = fma->memory() + ext.start;
      return { ptr, ext.length, arena_index };
    }
  fatal_error ("newly allocated arena too short for request: %u < %u", fma->size(), ext.length);
}

// == MemoryMetaTable ==
struct MemoryMetaInfo {
  std::mutex mutex;
  std::vector<ArenaBlock> ablocks;
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
mm_info_push_mt (const ArenaBlock &ablock) // MT-Safe
{
  MemoryMetaInfo &mi = mm_info_lookup (ablock.block_start);
  std::lock_guard<std::mutex> locker (mi.mutex);
  mi.ablocks.push_back (ablock);
}

static ArenaBlock
mm_info_pop_mt (void *block_start) // MT-Safe
{
  MemoryMetaInfo &mi = mm_info_lookup (block_start);
  std::lock_guard<std::mutex> locker (mi.mutex);
  auto it = std::find_if (mi.ablocks.begin(), mi.ablocks.end(),
                          [block_start] (const auto &ab) {
                            return ab.block_start == block_start;
                          });
  if (it != mi.ablocks.end())   // found it, now pop
    {
      const ArenaBlock ab = *it;
      if (it < mi.ablocks.end() - 1)
        *it = mi.ablocks.back(); // swap with tail for quick shrinking
      mi.ablocks.resize (mi.ablocks.size() - 1);
      return ab;
    }
  return {};
}

} // FastMemory

// == aligned malloc/calloc/free ==
void*
fast_mem_alloc (size_t size)
{
  std::unique_lock<std::mutex> shortlock (FastMemory::fast_mem_mutex);
  FastMemory::ArenaBlock ab = FastMemory::fast_mem_allocate_aligned_block (size); // MT-Guarded
  shortlock.unlock();
  void *const ptr = ab.block_start;
  if (ptr)
    mm_info_push_mt (ab);
  else
    fatal_error ("%s: failed to allocate %u bytes\n", __func__, size);
  return ptr;
}

void
fast_mem_free (void *mem)
{
  return_unless (mem);
  FastMemory::ArenaBlock ab = FastMemory::mm_info_pop_mt (mem);
  if (!ab.block_start)
    fatal_error ("%s: invalid memory pointer: %p\n", __func__, mem);
  std::lock_guard<std::mutex> locker (FastMemory::fast_mem_mutex);
  FastMemory::fast_mem_arenas[ab.arena_index].release (ab.block()); // MT-Guarded
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
  using FastMemory::Extent32;
  const ssize_t kb = 1024, asz = 4 * 1024;
  // create small area
  FastMemory::Arena arena { asz, FastMemory::cache_line_size };
  FastMemory::Allocator *fmap = fmallocator (arena);
  assert_return (fmap != nullptr);
  FastMemory::Allocator &fma = *fmap;
  assert_return (fma.sum() == asz);
  // allocate 4 * 1mb
  bool success;
  Extent32 s1 (kb);
  success = fma.alloc_ext (s1);
  assert_return (success);
  assert_return (fma.sum() == asz - kb);
  Extent32 s2 (kb - 1);
  success = fma.alloc_ext (s2);
  assert_return (success && s2.length == kb); // check alignment
  assert_return (fma.sum() == asz - 2 * kb);
  Extent32 s3 (kb);
  success = fma.alloc_ext (s3);
  assert_return (success);
  assert_return (fma.sum() == asz - 3 * kb);
  Extent32 s4 (kb);
  success = fma.alloc_ext (s4);
  assert_return (success);
  assert_return (fma.sum() == 0);
  // release with fragmentation
  fma.release_ext (s1);
  assert_return (fma.sum() == kb);
  fma.release_ext (s3);
  assert_return (fma.sum() == 2 * kb);
  // fail allocation due to fragmentation
  s1.reset (2 * kb);
  success = fma.alloc_ext (s1);
  assert_return (success == false);
  // release middle block and allocate coalesced result
  fma.release_ext (s2);
  assert_return (fma.sum() == 3 * kb);
  s1.reset (3 * kb);
  success = fma.alloc_ext (s1);
  assert_return (success);
  assert_return (fma.sum() == 0);
  // release all
  fma.release_ext (s1);
  fma.release_ext (s4);
  assert_return (fma.sum() == asz);
  // test general purpose allocations exceeding a single FastMemory::Arena
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
