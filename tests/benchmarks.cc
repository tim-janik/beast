// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <bse/testing.hh>
#include <bse/unicode.hh>

static constexpr size_t RUNS = 1;
static constexpr double MAXTIME = 0.15;
static constexpr double M = 1000000;

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

