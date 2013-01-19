// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "birnetutf8.hh"
#include <glib.h>
namespace Birnet {
namespace Unichar {
/* --- unichar ctype.h equivalents --- */
bool
isalnum (unichar uc)
{
  return g_unichar_isalnum (uc);
}
bool
isalpha (unichar uc)
{
  return g_unichar_isalpha (uc);
}
bool
iscntrl (unichar uc)
{
  return g_unichar_iscntrl (uc);
}
bool
isdigit (unichar uc)
{
  return g_unichar_isdigit (uc);
}
int
digit_value (unichar uc)
{
  return g_unichar_digit_value (uc);
}
bool
isgraph (unichar uc)
{
  return g_unichar_isgraph (uc);
}
bool
islower (unichar uc)
{
  return g_unichar_islower (uc);
}
unichar
tolower (unichar uc)
{
  return g_unichar_tolower (uc);
}
bool
isprint (unichar uc)
{
  return g_unichar_isprint (uc);
}
bool
ispunct (unichar uc)
{
  return g_unichar_ispunct (uc);
}
bool
isspace (unichar uc)
{
  return g_unichar_isspace (uc);
}
bool
isupper (unichar uc)
{
  return g_unichar_isupper (uc);
}
unichar
toupper (unichar uc)
{
  return g_unichar_toupper (uc);
}
bool
isxdigit (unichar uc)
{
  return g_unichar_isxdigit (uc);
}
int
xdigit_value (unichar uc)
{
  return g_unichar_xdigit_value (uc);
}
bool
istitle (unichar uc)
{
  return g_unichar_istitle (uc);
}
unichar
totitle (unichar uc)
{
  return g_unichar_totitle (uc);
}
bool
isdefined (unichar uc)
{
  return g_unichar_isdefined (uc);
}
bool
iswide (unichar uc)
{
  return g_unichar_iswide (uc);
}
bool
iswide_cjk (unichar uc)
{
#if GLIB_CHECK_VERSION (2, 12, 0)
  return g_unichar_iswide_cjk (uc);
#else
  return false;
#endif
}
Type
get_type (unichar uc)
{
  return Type (g_unichar_type (uc));
}
BreakType
get_break (unichar uc)
{
  return BreakType (g_unichar_break_type (uc));
}
/* --- ensure castable Birnet::Unichar::Type --- */
BIRNET_STATIC_ASSERT (Unichar::CONTROL == (int) G_UNICODE_CONTROL);
BIRNET_STATIC_ASSERT (Unichar::FORMAT == (int) G_UNICODE_FORMAT);
BIRNET_STATIC_ASSERT (Unichar::UNASSIGNED == (int) G_UNICODE_UNASSIGNED);
BIRNET_STATIC_ASSERT (Unichar::PRIVATE_USE == (int) G_UNICODE_PRIVATE_USE);
BIRNET_STATIC_ASSERT (Unichar::SURROGATE == (int) G_UNICODE_SURROGATE);
BIRNET_STATIC_ASSERT (Unichar::LOWERCASE_LETTER == (int) G_UNICODE_LOWERCASE_LETTER);
BIRNET_STATIC_ASSERT (Unichar::MODIFIER_LETTER == (int) G_UNICODE_MODIFIER_LETTER);
BIRNET_STATIC_ASSERT (Unichar::OTHER_LETTER == (int) G_UNICODE_OTHER_LETTER);
BIRNET_STATIC_ASSERT (Unichar::TITLECASE_LETTER == (int) G_UNICODE_TITLECASE_LETTER);
BIRNET_STATIC_ASSERT (Unichar::UPPERCASE_LETTER == (int) G_UNICODE_UPPERCASE_LETTER);
BIRNET_STATIC_ASSERT (Unichar::COMBINING_MARK == (int) G_UNICODE_COMBINING_MARK);
BIRNET_STATIC_ASSERT (Unichar::ENCLOSING_MARK == (int) G_UNICODE_ENCLOSING_MARK);
BIRNET_STATIC_ASSERT (Unichar::NON_SPACING_MARK == (int) G_UNICODE_NON_SPACING_MARK);
BIRNET_STATIC_ASSERT (Unichar::DECIMAL_NUMBER == (int) G_UNICODE_DECIMAL_NUMBER);
BIRNET_STATIC_ASSERT (Unichar::LETTER_NUMBER == (int) G_UNICODE_LETTER_NUMBER);
BIRNET_STATIC_ASSERT (Unichar::OTHER_NUMBER == (int) G_UNICODE_OTHER_NUMBER);
BIRNET_STATIC_ASSERT (Unichar::CONNECT_PUNCTUATION == (int) G_UNICODE_CONNECT_PUNCTUATION);
BIRNET_STATIC_ASSERT (Unichar::DASH_PUNCTUATION == (int) G_UNICODE_DASH_PUNCTUATION);
BIRNET_STATIC_ASSERT (Unichar::CLOSE_PUNCTUATION == (int) G_UNICODE_CLOSE_PUNCTUATION);
BIRNET_STATIC_ASSERT (Unichar::FINAL_PUNCTUATION == (int) G_UNICODE_FINAL_PUNCTUATION);
BIRNET_STATIC_ASSERT (Unichar::INITIAL_PUNCTUATION == (int) G_UNICODE_INITIAL_PUNCTUATION);
BIRNET_STATIC_ASSERT (Unichar::OTHER_PUNCTUATION == (int) G_UNICODE_OTHER_PUNCTUATION);
BIRNET_STATIC_ASSERT (Unichar::OPEN_PUNCTUATION == (int) G_UNICODE_OPEN_PUNCTUATION);
BIRNET_STATIC_ASSERT (Unichar::CURRENCY_SYMBOL == (int) G_UNICODE_CURRENCY_SYMBOL);
BIRNET_STATIC_ASSERT (Unichar::MODIFIER_SYMBOL == (int) G_UNICODE_MODIFIER_SYMBOL);
BIRNET_STATIC_ASSERT (Unichar::MATH_SYMBOL == (int) G_UNICODE_MATH_SYMBOL);
BIRNET_STATIC_ASSERT (Unichar::OTHER_SYMBOL == (int) G_UNICODE_OTHER_SYMBOL);
BIRNET_STATIC_ASSERT (Unichar::LINE_SEPARATOR == (int) G_UNICODE_LINE_SEPARATOR);
BIRNET_STATIC_ASSERT (Unichar::PARAGRAPH_SEPARATOR == (int) G_UNICODE_PARAGRAPH_SEPARATOR);
BIRNET_STATIC_ASSERT (Unichar::SPACE_SEPARATOR == (int) G_UNICODE_SPACE_SEPARATOR);
/* --- ensure castable Birnet::Unichar::BreakType --- */
BIRNET_STATIC_ASSERT (Unichar::BREAK_MANDATORY == (int) G_UNICODE_BREAK_MANDATORY);
BIRNET_STATIC_ASSERT (Unichar::BREAK_CARRIAGE_RETURN == (int) G_UNICODE_BREAK_CARRIAGE_RETURN);
BIRNET_STATIC_ASSERT (Unichar::BREAK_LINE_FEED == (int) G_UNICODE_BREAK_LINE_FEED);
BIRNET_STATIC_ASSERT (Unichar::BREAK_COMBINING_MARK == (int) G_UNICODE_BREAK_COMBINING_MARK);
BIRNET_STATIC_ASSERT (Unichar::BREAK_SURROGATE == (int) G_UNICODE_BREAK_SURROGATE);
BIRNET_STATIC_ASSERT (Unichar::BREAK_ZERO_WIDTH_SPACE == (int) G_UNICODE_BREAK_ZERO_WIDTH_SPACE);
BIRNET_STATIC_ASSERT (Unichar::BREAK_INSEPARABLE == (int) G_UNICODE_BREAK_INSEPARABLE);
BIRNET_STATIC_ASSERT (Unichar::BREAK_NON_BREAKING_GLUE == (int) G_UNICODE_BREAK_NON_BREAKING_GLUE);
BIRNET_STATIC_ASSERT (Unichar::BREAK_CONTINGENT == (int) G_UNICODE_BREAK_CONTINGENT);
BIRNET_STATIC_ASSERT (Unichar::BREAK_SPACE == (int) G_UNICODE_BREAK_SPACE);
BIRNET_STATIC_ASSERT (Unichar::BREAK_AFTER == (int) G_UNICODE_BREAK_AFTER);
BIRNET_STATIC_ASSERT (Unichar::BREAK_BEFORE == (int) G_UNICODE_BREAK_BEFORE);
BIRNET_STATIC_ASSERT (Unichar::BREAK_BEFORE_AND_AFTER == (int) G_UNICODE_BREAK_BEFORE_AND_AFTER);
BIRNET_STATIC_ASSERT (Unichar::BREAK_HYPHEN == (int) G_UNICODE_BREAK_HYPHEN);
BIRNET_STATIC_ASSERT (Unichar::BREAK_NON_STARTER == (int) G_UNICODE_BREAK_NON_STARTER);
BIRNET_STATIC_ASSERT (Unichar::BREAK_OPEN_PUNCTUATION == (int) G_UNICODE_BREAK_OPEN_PUNCTUATION);
BIRNET_STATIC_ASSERT (Unichar::BREAK_CLOSE_PUNCTUATION == (int) G_UNICODE_BREAK_CLOSE_PUNCTUATION);
BIRNET_STATIC_ASSERT (Unichar::BREAK_QUOTATION == (int) G_UNICODE_BREAK_QUOTATION);
BIRNET_STATIC_ASSERT (Unichar::BREAK_EXCLAMATION == (int) G_UNICODE_BREAK_EXCLAMATION);
BIRNET_STATIC_ASSERT (Unichar::BREAK_IDEOGRAPHIC == (int) G_UNICODE_BREAK_IDEOGRAPHIC);
BIRNET_STATIC_ASSERT (Unichar::BREAK_NUMERIC == (int) G_UNICODE_BREAK_NUMERIC);
BIRNET_STATIC_ASSERT (Unichar::BREAK_INFIX_SEPARATOR == (int) G_UNICODE_BREAK_INFIX_SEPARATOR);
BIRNET_STATIC_ASSERT (Unichar::BREAK_SYMBOL == (int) G_UNICODE_BREAK_SYMBOL);
BIRNET_STATIC_ASSERT (Unichar::BREAK_ALPHABETIC == (int) G_UNICODE_BREAK_ALPHABETIC);
BIRNET_STATIC_ASSERT (Unichar::BREAK_PREFIX == (int) G_UNICODE_BREAK_PREFIX);
BIRNET_STATIC_ASSERT (Unichar::BREAK_POSTFIX == (int) G_UNICODE_BREAK_POSTFIX);
BIRNET_STATIC_ASSERT (Unichar::BREAK_COMPLEX_CONTEXT == (int) G_UNICODE_BREAK_COMPLEX_CONTEXT);
BIRNET_STATIC_ASSERT (Unichar::BREAK_AMBIGUOUS == (int) G_UNICODE_BREAK_AMBIGUOUS);
BIRNET_STATIC_ASSERT (Unichar::BREAK_UNKNOWN == (int) G_UNICODE_BREAK_UNKNOWN);
BIRNET_STATIC_ASSERT (Unichar::BREAK_NEXT_LINE == (int) G_UNICODE_BREAK_NEXT_LINE);
BIRNET_STATIC_ASSERT (Unichar::BREAK_WORD_JOINER == (int) G_UNICODE_BREAK_WORD_JOINER);
#if GLIB_CHECK_VERSION (2, 10, 0)
BIRNET_STATIC_ASSERT (Unichar::BREAK_HANGUL_L_JAMO == (int) G_UNICODE_BREAK_HANGUL_L_JAMO);
BIRNET_STATIC_ASSERT (Unichar::BREAK_HANGUL_V_JAMO == (int) G_UNICODE_BREAK_HANGUL_V_JAMO);
BIRNET_STATIC_ASSERT (Unichar::BREAK_HANGUL_T_JAMO == (int) G_UNICODE_BREAK_HANGUL_T_JAMO);
BIRNET_STATIC_ASSERT (Unichar::BREAK_HANGUL_LV_SYLLABLE == (int) G_UNICODE_BREAK_HANGUL_LV_SYLLABLE);
BIRNET_STATIC_ASSERT (Unichar::BREAK_HANGUL_LVT_SYLLABLE == (int) G_UNICODE_BREAK_HANGUL_LVT_SYLLABLE);
#endif
} // Unichar
/* --- UTF-8 movement --- */
const int8 utf8_skip_table[256] = {
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1
};
static inline const int8
utf8_char_length (const uint8 c)
{
  return c < 0xfe ? utf8_skip_table[c] : -1;
}
static inline const uint8
utf8_length_bits (const uint8 l)
{
  const uint length_bits[] = { 0x00, 0x00, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, };
  return l <= 6 ? length_bits[l] : 0;
}
static inline const uint8
utf8_char_mask (const uint8 c)
{
  const uint8 char_masks[8] = { 0x00, 0x7f, 0x1f, 0x0f, 0x07, 0x03, 0x01, 0x00 };
  return char_masks[utf8_skip_table[c]];
}
static inline uint8
utf8_length_from_unichar (unichar uc)
{
  uint8 l = 1;
  l += uc >= 0x00000080; /* 2 */
  l += uc >= 0x00000800; /* 3 */
  l += uc >= 0x00010000; /* 4 */
  l += uc >= 0x00200000; /* 5 */
  l += uc >= 0x04000000; /* 6 */
  return l;
}
unichar
utf8_to_unichar (const char *str)
{
  uint8 clen = utf8_char_length (*str);
  uint8 mask = utf8_char_mask (*str);
  if (clen < 1)
    return 0xffffffff;
  unichar uc = *str & mask;
  for (uint i = 1; i < clen; i++)
    {
      uint8 c = str[i];
      if ((c & 0xc0) != 0x80)
        return 0xffffffff;
      uc = (uc << 6) + (c & 0x3f);
    }
  return uc;
}
int
utf8_from_unichar (unichar uc,
                   char    str[8])
{
  const int l = utf8_length_from_unichar (uc);
  if (!str)
    return l;
  uint i = l;
  str[i] = 0;
  while (--i)
    {
      str[i] = (uc & 0x3f) | 0x80;
      uc >>= 6;
    }
  str[i] = uc | utf8_length_bits (l); /* i == 0 */
  return l;
}
bool
utf8_validate (const String   &strng,
               int            *bound)
{
  const char *c = &strng[0];
  size_t l = strng.size();
  const gchar *end = NULL;
  gboolean gb = g_utf8_validate (c, l, &end);
  if (bound)
    *bound = !gb ? end - c : -1;
  return gb != false;
}
} // Birnet
