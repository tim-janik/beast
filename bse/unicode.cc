// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "unicode.hh"

namespace Bse {

/// Decode valid UTF-8 sequences, invalid sequences are treated as Latin-1 characters.
template<bool CODEPOINT> static inline size_t // returns length of unicode char
utf8character (const char *str, uint32_t *unicode)
{
  /* https://en.wikipedia.org/wiki/UTF-8
    : 0000 0001 0010 0011 0100 0101 0110 0111 1000 1001 1010 1011 1100 1101 1110 1111
    :  000  001  002  003  004  005  006  007  010  011  012  013  014  015  016  017
    :    0    1    2    3    4    5    6    7    8    9   10   11   12   13   14   15
    :  0x0  0x1  0x2  0x3  0x4  0x5  0x6  0x7  0x8  0x9  0xA  0xB  0xC  0xD  0xE  0xF
   */
  const uint8_t c = str[0];
  // optimized for one-byte sequences
  if (__builtin_expect (c < 0xc0, true))
    {
      if (CODEPOINT)
        *unicode = c;                                   // valid if c <= 0x7F
      return 1;                                         // treat as Latin-1 otherwise
    }
  // multi-byte sequences
  switch (c & 0xF8)
    {
      uint8_t d, e, f;
    case 0xC0: case 0xC8: case 0xD0: case 0xD8:         // 2-byte sequence
      d = str[1];
      if (__builtin_expect ((d & 0xC0) != 0x80, false))
        goto one_byte;
      if (CODEPOINT)
        *unicode = ((c & 0x1f) << 6) + (d & 0x3f);
      return 2;                                         // valid
    case 0xE0: case 0xE8:                               // 3-byte sequence
      d = str[1];
      if (__builtin_expect ((d & 0xC0) != 0x80, false))
        goto one_byte;
      e = str[2];
      if (__builtin_expect ((e & 0xC0) != 0x80, false))
        goto one_byte;
      if (CODEPOINT)
        *unicode = ((c & 0x0f) << 12) + ((d & 0x3f) << 6) + (e & 0x3f);
      return 3;                                         // valid
    case 0xF0:                                          // 4-byte sequence
      d = str[1];
      if (__builtin_expect ((d & 0xC0) != 0x80, false))
        goto one_byte;
      e = str[2];
      if (__builtin_expect ((e & 0xC0) != 0x80, false))
        goto one_byte;
      f = str[3];
      if (__builtin_expect ((f & 0xC0) != 0x80, false))
        goto one_byte;
      if (CODEPOINT)
        *unicode = ((c & 0x07) << 18) + ((d & 0x3f) << 12) + ((e & 0x3f) << 6) + (f & 0x3f);
      return 4;                                         // valid
    default: one_byte:
      if (CODEPOINT)
        *unicode = c;                                   // treat as Latin-1 otherwise
      return 1;
    }
}

/// Returns length of unicode character in bytes
static inline size_t
utf8codepoint (const char *str, uint32_t *unicode)
{
  return utf8character<true> (str, unicode);
}

/// Returns length of unicode character in bytes
static inline size_t
utf8skip (const char *str)
{
  return utf8character<false> (str, NULL);
}

/// Count valid UTF-8 sequences, invalid sequences are counted as Latin-1 characters.
size_t
utf8len (const char *str)
{
  size_t l;
  for (l = 0; __builtin_expect (*str != 0, true); l++)
    str += utf8skip (str);
  return l;
}

/// Count valid UTF-8 sequences, invalid sequences are counted as Latin-1 characters.
size_t
utf8len (const std::string &str)
{
  const char *c = str.data(), *e = c + str.size();
  size_t l = 0;
  while (c < e)
    {
      c += utf8skip (c);
      l += 1;
    }
  return l;
}

/// Convert valid UTF-8 sequences to Unicode codepoints, invalid sequences are treated as Latin-1 characters.
/// The array @a codepoints must be able to hold at least as many elements as are characters stored in @a str.
/// Returns the number of codepoints stored in @a codepoints.
size_t
utf8_to_unicode (const char *str, uint32_t *codepoints)
{
  // assuming sizeof codepoints[] >= sizeof str[]
  size_t l;
  for (l = 0; __builtin_expect (*str != 0, true); l++)
    str += utf8codepoint (str, &codepoints[l]);
  return l;
}

/// Convert valid UTF-8 sequences to Unicode codepoints, invalid sequences are treated as Latin-1 characters.
/// Returns the number of codepoints newly stored in @a codepoints.
size_t
utf8_to_unicode (const std::string &str, std::vector<uint32_t> &codepoints)
{
  const size_t l = codepoints.size();
  const char *c = str.data(), *e = c + str.size();
  while (c < e)
    {
      uint32_t codepoint;
      c += utf8codepoint (c, &codepoint);
      codepoints.push_back (codepoint);
    }
  return codepoints.size() - l;
}

/// Convert @a codepoints into an UTF-8 string, using the shortest possible encoding.
std::string
string_from_unicode (const uint32_t *codepoints, size_t n_codepoints)
{
  std::string str;
  for (size_t i = 0; i < n_codepoints; i++)
    {
      const uint32_t u = codepoints[i];
      if (__builtin_expect (u <= 0x7F, true))
        {
          str.push_back (u);
          continue;
        }
      switch (u)
        {
        case 0x00000080 ... 0x000007FF:
          str.push_back (0xC0 + (u >> 6));
          str.push_back (0x80 + (u & 0x3F));
          break;
        case 0x00000800 ... 0x0000FFFF:
          str.push_back (0xE0 +  (u >> 12));
          str.push_back (0x80 + ((u >>  6) & 0x3F));
          str.push_back (0x80 +  (u & 0x3F));
          break;
        case 0x00010000 ... 0x0010FFFF:
          str.push_back (0xF0 +  (u >> 18));
          str.push_back (0x80 + ((u >> 12) & 0x3F));
          str.push_back (0x80 + ((u >>  6) & 0x3F));
          str.push_back (0x80 +  (u & 0x3F));
          break;
        default:
          break;
        }
    }
  return str;
}

/// Convert @a codepoints into an UTF-8 string, using the shortest possible encoding.
std::string
string_from_unicode (const std::vector<uint32_t> &codepoints)
{
  return string_from_unicode (codepoints.data(), codepoints.size());
}

} // Bse

// == Testing ==
#include "testing.hh"
namespace { // Anon
using namespace Bse;

static constexpr size_t RUNS = 1;
static constexpr double MAXTIME = 0.15;
static constexpr double M = 1000000;

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
  printerr ("  BENCH    g_utf8_to_ucs4_fast:          %11.1f MChar/s\n", str.size() * RUNS / bench_time / M);

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
  printerr ("  BENCH    Bse::utf8_to_unicode:         %11.1f MChar/s\n", str.size() * RUNS / bench_time / M);

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
  printerr ("  BENCH         utf8_to_unicode inplace: %11.1f MChar/s\n", str.size() * RUNS / bench_time / M);

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
  printerr ("  BENCH    g_utf8_strlen:                %11.1f MChar/s %s\n", str.size() * RUNS / bench_time / M, what);

  auto bse_utf8len_loop = [&] () {
    for (size_t j = 0; j < RUNS; j++)
      {
        size_t result = utf8len (str.c_str());
        TCMP (expected, ==, result);
      }
  };
  bench_time = timer.benchmark (bse_utf8len_loop);
  printerr ("  BENCH    Bse::utf8len:                 %11.1f MChar/s %s\n", str.size() * RUNS / bench_time / M, what);

  auto simple_utf8len_loop = [&] () {
    for (size_t j = 0; j < RUNS; j++)
      {
        size_t result = not_0x80_strlen_utf8 (str.c_str());
        TCMP (expected, ==, result);
      }
  };
  bench_time = timer.benchmark (simple_utf8len_loop);
  printerr ("  BENCH    not_0x80_strlen_utf8:         %11.1f MChar/s %s\n", str.size() * RUNS / bench_time / M, what);
}

BSE_INTEGRITY_TEST (bse_test_utf8_funcs);

static void
bse_test_utf8_funcs()
{
  Blob b = Blob::from_file ("/etc/mailcap");
  const std::string str = b.string();
  size_t bse_utf8len, glib_utf8len;
  bse_utf8len = utf8len (str.c_str());
  glib_utf8len = g_utf8_strlen (str.c_str(), -1);
  TCMP (bse_utf8len, ==, glib_utf8len);
  std::vector<uint32_t> codepoints;
  size_t nc = 0, cc = 0, pc = 0;
  for (size_t i = 0; i <= unicode_last_codepoint; i++)
    {
      TASSERT (unicode_is_assigned (i) <= unicode_is_valid (i));
      TASSERT (unicode_is_noncharacter (i) != unicode_is_character (i));
      nc += unicode_is_noncharacter (i);
      cc += unicode_is_control_code (i);
      pc += unicode_is_private (i);
      if (i && unicode_is_assigned (i))
        codepoints.push_back (i);
    }
  TASSERT (nc == 66);
  TASSERT (cc == 65);
  TASSERT (pc == 6400 + 65534 + 65534);
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

} // Anon
