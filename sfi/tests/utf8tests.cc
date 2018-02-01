// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#undef G_LOG_DOMAIN
#define  G_LOG_DOMAIN __FILE__
#include <sfi/testing.hh>
#include <sfi/unicode.hh>

using namespace Bse;

constexpr size_t RUNS = 1;
constexpr double MAXTIME = 0.15;
constexpr double M = 1000000;

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
utf8_codepoint_bench (const String &str)
{
  const size_t expected = utf8len (str.c_str());
  double bench_time;
  Bse::Test::Timer timer (MAXTIME);

  auto loop_g_utf8_to_ucs4_fast = [&] () {
    for (size_t j = 0; j < RUNS; j++)
      {
        glong long_result;
        gunichar *gu = g_utf8_to_ucs4_fast (str.c_str(), -1, &long_result);
        gulong result = long_result;
        TCMP (expected, ==, result);
        g_free (gu);
      }
  };
  bench_time = timer.benchmark (loop_g_utf8_to_ucs4_fast);
  printerr ("  BENCH    g_utf8_to_ucs4_fast:          %11.1f mc/s\n", str.size() * RUNS / bench_time / M);

  auto loop_bse_utf8_to_unicode = [&] () {
    for (size_t j = 0; j < RUNS; j++)
      {
        std::vector<uint32_t> codepoints;
        codepoints.resize (expected);   // force reallocation to be comparable with g_utf8_to_ucs4_fast
        size_t result = utf8_to_unicode (str.c_str(), codepoints.data());
        TCMP (expected, ==, result);
      }
  };
  bench_time = timer.benchmark (loop_bse_utf8_to_unicode);
  printerr ("  BENCH    Bse::utf8_to_unicode:         %11.1f mc/s\n", str.size() * RUNS / bench_time / M);

  std::vector<uint32_t> codepoints;
  codepoints.resize (expected);
  auto loop_inplace_utf8_to_unicode = [&] () {
    for (size_t j = 0; j < RUNS; j++)
      {
        size_t result = utf8_to_unicode (str.c_str(), codepoints.data());
        TCMP (expected, ==, result);
      }
  };
  bench_time = timer.benchmark (loop_inplace_utf8_to_unicode);
  printerr ("  BENCH         utf8_to_unicode inplace: %11.1f mc/s\n", str.size() * RUNS / bench_time / M);

  glong gresult;
  gunichar *gu = g_utf8_to_ucs4_fast (str.c_str(), -1, &gresult);
  TASSERT (expected == (size_t) gresult);
  for (size_t i = 0; i < expected; i++)
    if (gu[i] != codepoints[i])
      {
        printerr ("  BENCH      0x%06x) 0x%06x != 0x%06x\n", i + 1, gu[i], codepoints[i]);
        TCMP (gu[i], ==, codepoints[i]);
      }
  g_free (gu);
}

static void
utf8_strlen_bench (const String &str, const String &what)
{
  const size_t expected = utf8len (str.c_str());
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
  printerr ("  BENCH    g_utf8_strlen:                %11.1f mc/s %s\n", str.size() * RUNS / bench_time / M, what);

  auto bse_utf8len_loop = [&] () {
    for (size_t j = 0; j < RUNS; j++)
      {
        size_t result = utf8len (str.c_str());
        TCMP (expected, ==, result);
      }
  };
  bench_time = timer.benchmark (bse_utf8len_loop);
  printerr ("  BENCH    Bse::utf8len:                 %11.1f mc/s %s\n", str.size() * RUNS / bench_time / M, what);

  auto simple_utf8len_loop = [&] () {
    for (size_t j = 0; j < RUNS; j++)
      {
        size_t result = not_0x80_strlen_utf8 (str.c_str());
        TCMP (expected, ==, result);
      }
  };
  bench_time = timer.benchmark (simple_utf8len_loop);
  printerr ("  BENCH    not_0x80_strlen_utf8:         %11.1f mc/s %s\n", str.size() * RUNS / bench_time / M, what);
}

static void
test_utf8_funcs()
{
  Blob b = Blob::from_file ("/etc/mailcap");
  const std::string str = b.string();
  size_t bse_utf8len, glib_utf8len;
  bse_utf8len = utf8len (str.c_str());
  glib_utf8len = g_utf8_strlen (str.c_str(), -1);
  TCMP (bse_utf8len, ==, glib_utf8len);
  std::vector<uint32_t> codepoints;
  for (size_t i = 1; i <= unicode_last_codepoint; i++)
    TASSERT (unicode_is_assigned (i) <= unicode_is_valid (i));
  for (size_t i = 1; i <= 0x10FFFF; i++)
    if (unicode_is_assigned (i))
      codepoints.push_back (i);
  std::string big = string_from_unicode (codepoints);
  bse_utf8len = utf8len (big.c_str());
  glib_utf8len = g_utf8_strlen (big.c_str(), -1);
  TCMP (bse_utf8len, ==, glib_utf8len);
  TCMP (bse_utf8len, ==, codepoints.size());
  if (true)
    {
      std::vector<uint32_t> tmp;
      const size_t tmp_result = utf8_to_unicode (big, tmp);
      TASSERT (tmp_result == tmp.size() && codepoints.size() == tmp_result);
      for (size_t i = 0; i < codepoints.size(); ++i)
        TASSERT (tmp[i] == codepoints[i]);
    }
  utf8_codepoint_bench (big);
  utf8_strlen_bench (big, "(high planes)");
  for (size_t i = 0; i < big.size(); i++)
    big[i] = (i + 1) % 0x80;
  utf8_strlen_bench (big, "(ascii)");
  // printerr ("SEE: %d == %d (%d)\n", bse_utf8len, glib_utf8len, str.size());
}
TEST_ADD (test_utf8_funcs);
