// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <bse/testing.hh>
#include <bse/unicode.hh>
#include <bse/memory.hh>

static constexpr size_t RUNS = 1;
static constexpr double MAXTIME = 0.15;
static constexpr double M = 1000000;

// == Unicode Tests ==
static std::string
all_codepoints_to_utf8 ()
{
  std::vector<uint32_t> codepoints;
  for (size_t i = 1; i <= Bse::unicode_last_codepoint; i++)
    if (Bse::unicode_is_assigned (i))
      codepoints.push_back (i);
  return Bse::string_from_unicode (codepoints);
}

static void
utf8_codepoint_bench()
{
  std::string big = all_codepoints_to_utf8();
  const size_t expected = Bse::utf8len (big.c_str());
  double bench_time;
  Bse::Test::Timer timer (MAXTIME);

  auto loop_g_utf8_to_ucs4_fast = [&] () {
    for (size_t j = 0; j < RUNS; j++)
      {
        glong long_result;
        gunichar *gu = g_utf8_to_ucs4_fast (big.c_str(), -1, &long_result);
        gulong result = long_result;
        TCMP (expected, ==, result);
        g_free (gu);
      }
  };
  bench_time = timer.benchmark (loop_g_utf8_to_ucs4_fast);
  Bse::printerr ("  BENCH    g_utf8_to_ucs4_fast:          %11.1f MChar/s\n", big.size() * RUNS / bench_time / M);

  auto loop_bse_utf8_to_unicode = [&] () {
    for (size_t j = 0; j < RUNS; j++)
      {
        std::vector<uint32_t> codepoints;
        codepoints.resize (expected);   // force reallocation to be comparable with g_utf8_to_ucs4_fast
        size_t result = Bse::utf8_to_unicode (big.c_str(), codepoints.data());
        TCMP (expected, ==, result);
      }
  };
  bench_time = timer.benchmark (loop_bse_utf8_to_unicode);
  Bse::printerr ("  BENCH    Bse::utf8_to_unicode:         %11.1f MChar/s\n", big.size() * RUNS / bench_time / M);

  std::vector<uint32_t> codepoints;
  codepoints.resize (expected);
  auto loop_inplace_utf8_to_unicode = [&] () {
    for (size_t j = 0; j < RUNS; j++)
      {
        size_t result = Bse::utf8_to_unicode (big.c_str(), codepoints.data());
        TCMP (expected, ==, result);
      }
  };
  bench_time = timer.benchmark (loop_inplace_utf8_to_unicode);
  Bse::printerr ("  BENCH         utf8_to_unicode inplace: %11.1f MChar/s\n", big.size() * RUNS / bench_time / M);

  glong gresult;
  gunichar *gu = g_utf8_to_ucs4_fast (big.c_str(), -1, &gresult);
  TASSERT (expected == (size_t) gresult);
  for (size_t i = 0; i < expected; i++)
    if (gu[i] != codepoints[i])
      {
        Bse::printerr ("  BENCH      0x%06x) 0x%06x != 0x%06x\n", i + 1, gu[i], codepoints[i]);
        TCMP (gu[i], ==, codepoints[i]);
      }
  g_free (gu);
}
TEST_BENCH (utf8_codepoint_bench);

static size_t
not_0x80_strlen_utf8 (const std::string &str)
{
  size_t length = 0;
  for (char c : str) {
    if ((c & 0xC0) != 0x80)
      ++length;
  }
  return length;
}

static void
utf8_strlen_bench (const std::string &str, const std::string &what)
{
  const size_t expected = Bse::utf8len (str.c_str());
  double bench_time;
  Bse::Test::Timer timer (MAXTIME);

  auto glib_utf8len_loop = [&] () {
    for (size_t j = 0; j < RUNS; j++)
      {
        size_t result = g_utf8_strlen (str.c_str(), -1);
        TCMP (expected, ==, result);
      }
  };
  bench_time = timer.benchmark (glib_utf8len_loop);
  Bse::printerr ("  BENCH    g_utf8_strlen:                %11.1f MChar/s %s\n", str.size() * RUNS / bench_time / M, what);

  auto bse_utf8len_loop = [&] () {
    for (size_t j = 0; j < RUNS; j++)
      {
        size_t result = Bse::utf8len (str.c_str());
        TCMP (expected, ==, result);
      }
  };
  bench_time = timer.benchmark (bse_utf8len_loop);
  Bse::printerr ("  BENCH    Bse::utf8len:                 %11.1f MChar/s %s\n", str.size() * RUNS / bench_time / M, what);

  auto simple_utf8len_loop = [&] () {
    for (size_t j = 0; j < RUNS; j++)
      {
        size_t result = not_0x80_strlen_utf8 (str.c_str());
        TCMP (expected, ==, result);
      }
  };
  bench_time = timer.benchmark (simple_utf8len_loop);
  Bse::printerr ("  BENCH    not_0x80_strlen_utf8:         %11.1f MChar/s %s\n", str.size() * RUNS / bench_time / M, what);
}

static void
utf8_strlen_bench_high_planes()
{
  std::string big = all_codepoints_to_utf8();
  utf8_strlen_bench (big, "(high planes)");
}
TEST_BENCH (utf8_strlen_bench_high_planes);

static void
utf8_strlen_bench_ascii()
{
  std::string big;
  big.resize (Bse::unicode_last_codepoint * 1.07); // roughly equivalent length to the high_planes test
  for (size_t i = 0; i < big.size(); i++)
    big[i] = (i + 1) % 0x80; // fill string with 0x01..0xf7 characters
  utf8_strlen_bench (big, "(ascii)");
}
TEST_BENCH (utf8_strlen_bench_ascii);

// == Allocator Tests ==
namespace { // Anon
using namespace Bse;

#define TEST_AREA_SIZE  (16 * 1024 * 1024)

static FastMemory::Arena fast_memory_arena { TEST_AREA_SIZE }; // FIXME: use ctor

static void
ensure_block_allocator_initialization()
{
  fast_mem_free (fast_mem_alloc (1024));
  const size_t areasize = 4 * 1024 * 1024;
  TASSERT (fast_memory_arena.reserved() >= areasize);
  FastMemory::Block b1 = fast_memory_arena.allocate (areasize / 4);
  FastMemory::Block b2 = fast_memory_arena.allocate (areasize / 4);
  FastMemory::Block b3 = fast_memory_arena.allocate (areasize / 4);
  FastMemory::Block b4 = fast_memory_arena.allocate (areasize / 4);
  fast_memory_arena.release (b1);
  fast_memory_arena.release (b2);
  fast_memory_arena.release (b3);
  fast_memory_arena.release (b4);
}

enum class AllocatorType {
  FastMemoryArea = 1,
  FastMemAlloc,
  PosixMemalign,
  LibcCalloc,
};

template<AllocatorType C> struct TestAllocator;

template<>
struct TestAllocator<AllocatorType::FastMemoryArea> {
  static std::string       name            ()      { return "Bse::FastMemoryArea"; }
  static FastMemory::Block allocate_block  (uint32 length)
  { return fast_memory_arena.allocate (length); }
  static void              release_block   (FastMemory::Block block)
  { fast_memory_arena.release (block); }
};

template<>
struct TestAllocator<AllocatorType::PosixMemalign> {
  static std::string    name                    ()      { return "posix_memalign"; }
  static FastMemory::Block
  allocate_block (uint32 length)
  {
    void *ptr = nullptr;
    const int posix_memalign_result = posix_memalign (&ptr, FastMemory::cache_line_size, length);
    TASSERT (posix_memalign_result == 0);
    return { ptr, length };
  }
  static void
  release_block (FastMemory::Block block)
  {
    memset (block.block_start, 0, block.block_length); // match release_aligned_block() semantics
    free (block.block_start);
  }
};

template<>
struct TestAllocator<AllocatorType::FastMemAlloc> {
  static std::string    name                    ()      { return "fast_mem_alloc"; }
  static FastMemory::Block
  allocate_block (uint32 length)
  {
    void *ptr = fast_mem_alloc (length);
    TASSERT (ptr != nullptr);
    return { ptr, length };
  }
  static void
  release_block (FastMemory::Block block)
  {
    memset (block.block_start, 0, block.block_length); // match release_aligned_block() semantics
    fast_mem_free (block.block_start);
  }
};

template<>
struct TestAllocator<AllocatorType::LibcCalloc> {
  static std::string    name                    ()      { return "::calloc (misaligned)"; }
  static FastMemory::Block
  allocate_block (uint32 length)
  {
    void *ptr = calloc (length, 1);
    TASSERT (ptr != nullptr);
    return { ptr, length };
  }
  static void
  release_block (FastMemory::Block block)
  {
    free (block.block_start);
  }
};

/* Use a simple, fast dedicated RNG, because:
 * a) we need to be able to reset the RNG to compare results from different runs;
 * b) it should be really fast to not affect the allocator benchmarking.
 */
static uint32 quick_rand32_seed = 2147483563;
static inline uint32
quick_rand32 ()
{
  quick_rand32_seed = 1664525 * quick_rand32_seed + 1013904223;
  return quick_rand32_seed;
}

template<AllocatorType AT> static void
bse_aligned_allocator_benchloop (uint32 seed)
{
  constexpr const size_t RUNS = 3;
  constexpr const int64 MAX_CHUNK_SIZE = 4096;
  constexpr const int64 N_ALLOCS = 4093;
  constexpr const int64 RESIDENT = N_ALLOCS / 3;
  static_assert (MAX_CHUNK_SIZE * N_ALLOCS <= TEST_AREA_SIZE);
  static FastMemory::Block blocks[N_ALLOCS];
  auto loop_aa = [&] () {
    quick_rand32_seed = seed;
    for (size_t j = 0; j < RUNS; j++)
      {
        // allocate random sizes
        for (size_t i = 0; i < N_ALLOCS; i++)
          {
            const size_t length = 1 + ((quick_rand32() * MAX_CHUNK_SIZE) >> 32);
            blocks[i] = TestAllocator<AT>::allocate_block (length);
            TASSERT (blocks[i].block_length > 0);
            if (i > RESIDENT && (i & 1))
              {
                FastMemory::Block &rblock = blocks[i - RESIDENT];
                // Bse::printerr ("%d) FastMemoryBlock{%u,%x,%x,%p}\n", i, rblock.shm_id, rblock.mem_offset, rblock.mem_length, rblock.mem_start);
                TestAllocator<AT>::release_block (rblock);
                rblock = {};
              }
          }
        // shuffle some blocks
        for (size_t j = 0; j < N_ALLOCS / 2; j++)
          {
            const uint i1 = j * 2;
            const uint i2 = (quick_rand32() * N_ALLOCS) >> 32;
            const uint i3 = (i1 + i2) / 2;
            if (i1 == i2 || i2 == i3 || i3 == i1)
              continue; // avoid double free
            const uint l1 = blocks[i1].block_length;
            const uint l2 = blocks[i2].block_length;
            const uint l3 = blocks[i3].block_length;
            if (l1)
              TestAllocator<AT>::release_block (blocks[i1]);
            if (l2)
              TestAllocator<AT>::release_block (blocks[i2]);
            if (l3)
              TestAllocator<AT>::release_block (blocks[i3]);
            blocks[i2] = TestAllocator<AT>::allocate_block (l1 ? l1 : MAX_CHUNK_SIZE / 3);
            blocks[i1] = TestAllocator<AT>::allocate_block (l3 ? l3 : MAX_CHUNK_SIZE / 3);
            blocks[i3] = TestAllocator<AT>::allocate_block (l2 ? l2 : MAX_CHUNK_SIZE / 3);
          }
        // release blocks randomized (frees ca 59%)
        for (size_t j = 0; j < N_ALLOCS; j++)
          {
            const uint i = (quick_rand32() * N_ALLOCS) >> 32;
            if (!blocks[i].block_length)
              continue;
            TestAllocator<AT>::release_block (blocks[i]);
            blocks[i] = {};
          }
        // release everything
        for (size_t i = 0; i < N_ALLOCS; i++)
          if (blocks[i].block_length)
            {
              TestAllocator<AT>::release_block (blocks[i]);
              blocks[i] = {};
            }
      }
  };
  Bse::Test::Timer timer (0.1);
  const double bench_aa = timer.benchmark (loop_aa);
  const size_t n_allocations = RUNS * N_ALLOCS * (1 + 3.0 / 2);
  const double ns_p_a = 1000000000.0 * bench_aa / n_allocations;
  Bse::printerr ("  BENCH    %-21s %u allocations in %.1f msecs, %.1fnsecs/allocation\n",
                 TestAllocator<AT>::name() + ":", n_allocations, 1000 * bench_aa, ns_p_a);
}

static void
aligned_allocator_bench31_aligned_block()
{
  ensure_block_allocator_initialization();
  bse_aligned_allocator_benchloop<AllocatorType::FastMemoryArea> (2654435769);
}
TEST_BENCH (aligned_allocator_bench31_aligned_block);

static void
aligned_allocator_bench31_memalign()
{
  ensure_block_allocator_initialization();
  bse_aligned_allocator_benchloop<AllocatorType::PosixMemalign> (2654435769);
  // phi = 1.61803398874989484820458683436563811772030917980576286213544862270526046281890244970720720418939113748475
  // 2^64 / phi = 11400714819323198485.95161058762180694985
  // 2^32 / phi = 2654435769.49723029647758477079
}
TEST_BENCH (aligned_allocator_bench31_memalign);

static void
aligned_allocator_bench31_calloc()
{
  ensure_block_allocator_initialization();
  bse_aligned_allocator_benchloop<AllocatorType::LibcCalloc> (2654435769);
}
TEST_BENCH (aligned_allocator_bench31_calloc);

static void
aligned_allocator_bench31_fast_mem_alloc()
{
  ensure_block_allocator_initialization();
  bse_aligned_allocator_benchloop<AllocatorType::FastMemAlloc> (2654435769);
}
TEST_BENCH (aligned_allocator_bench31_fast_mem_alloc);

} // Anon
