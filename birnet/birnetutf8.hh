/* BirnetUtf8 - UTF-8 utilities
 * Copyright (C) 2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#ifndef __BIRNET_UTF8_HH__
#define __BIRNET_UTF8_HH__

#include <birnet/birnetutils.hh>

namespace Birnet {

namespace Unichar {
inline bool isvalid      (unichar uc) BIRNET_CONST;
bool        isalnum      (unichar uc) BIRNET_CONST;
bool        isalpha      (unichar uc) BIRNET_CONST;
bool        iscntrl      (unichar uc) BIRNET_CONST;
bool        isdigit      (unichar uc) BIRNET_CONST;
int         digit_value  (unichar uc) BIRNET_CONST;
bool        isgraph      (unichar uc) BIRNET_CONST;
bool        islower      (unichar uc) BIRNET_CONST;
unichar     tolower      (unichar uc) BIRNET_CONST;
bool        isprint      (unichar uc) BIRNET_CONST;
bool        ispunct      (unichar uc) BIRNET_CONST;
bool        isspace      (unichar uc) BIRNET_CONST;
bool        isupper      (unichar uc) BIRNET_CONST;
unichar     toupper      (unichar uc) BIRNET_CONST;
bool        isxdigit     (unichar uc) BIRNET_CONST;
int         xdigit_value (unichar uc) BIRNET_CONST;
bool        istitle      (unichar uc) BIRNET_CONST;
unichar     totitle      (unichar uc) BIRNET_CONST;
bool        isdefined    (unichar uc) BIRNET_CONST;
bool        iswide       (unichar uc) BIRNET_CONST;
bool        iswide_cjk   (unichar uc) BIRNET_CONST;
typedef enum {
  CONTROL,              FORMAT,                 UNASSIGNED,
  PRIVATE_USE,          SURROGATE,              LOWERCASE_LETTER,
  MODIFIER_LETTER,      OTHER_LETTER,           TITLECASE_LETTER,
  UPPERCASE_LETTER,     COMBINING_MARK,         ENCLOSING_MARK,
  NON_SPACING_MARK,     DECIMAL_NUMBER,         LETTER_NUMBER,
  OTHER_NUMBER,         CONNECT_PUNCTUATION,    DASH_PUNCTUATION,
  CLOSE_PUNCTUATION,    FINAL_PUNCTUATION,      INITIAL_PUNCTUATION,
  OTHER_PUNCTUATION,    OPEN_PUNCTUATION,       CURRENCY_SYMBOL,
  MODIFIER_SYMBOL,      MATH_SYMBOL,            OTHER_SYMBOL,
  LINE_SEPARATOR,       PARAGRAPH_SEPARATOR,    SPACE_SEPARATOR
} Type;
Type    get_type     (unichar uc) BIRNET_CONST;
typedef enum {
  BREAK_MANDATORY,        BREAK_CARRIAGE_RETURN,    BREAK_LINE_FEED,
  BREAK_COMBINING_MARK,   BREAK_SURROGATE,          BREAK_ZERO_WIDTH_SPACE,
  BREAK_INSEPARABLE,      BREAK_NON_BREAKING_GLUE,  BREAK_CONTINGENT,
  BREAK_SPACE,            BREAK_AFTER,              BREAK_BEFORE,
  BREAK_BEFORE_AND_AFTER, BREAK_HYPHEN,             BREAK_NON_STARTER,
  BREAK_OPEN_PUNCTUATION, BREAK_CLOSE_PUNCTUATION,  BREAK_QUOTATION,
  BREAK_EXCLAMATION,      BREAK_IDEOGRAPHIC,        BREAK_NUMERIC,
  BREAK_INFIX_SEPARATOR,  BREAK_SYMBOL,             BREAK_ALPHABETIC,
  BREAK_PREFIX,           BREAK_POSTFIX,            BREAK_COMPLEX_CONTEXT,
  BREAK_AMBIGUOUS,        BREAK_UNKNOWN,            BREAK_NEXT_LINE,
  BREAK_WORD_JOINER,      BREAK_HANGUL_L_JAMO,      BREAK_HANGUL_V_JAMO,
  BREAK_HANGUL_T_JAMO,    BREAK_HANGUL_LV_SYLLABLE, BREAK_HANGUL_LVT_SYLLABLE
} BreakType;
BreakType get_break  (unichar uc) BIRNET_CONST;

} // Unichar

/* --- UTF-8 movement --- */
inline const char*    utf8_next         (const char     *c);
inline char*          utf8_next         (char           *c);
inline const char*    utf8_prev         (const char     *c);
inline char*          utf8_prev         (char           *c);
inline const char*    utf8_find_next    (const char     *c,
                                         const char     *bound = NULL);
inline char*          utf8_find_next    (char           *current,
                                         const char     *bound = NULL);
inline const char*    utf8_find_prev    (const char     *start,
                                         const char     *current);
inline char*          utf8_find_prev    (const char     *start,
                                         char           *currrent);
inline const char*    utf8_align        (const char     *start,
                                         const char     *current);
inline char*          utf8_align        (const char     *start,
                                         char           *current);
inline bool           utf8_aligned      (const char     *c);
unichar               utf8_to_unichar   (const char     *str);
int                   utf8_from_unichar (unichar         uc,
                                         char            str[8]);
bool                  utf8_validate     (const String   &string,
                                         int            *bound = NULL);

/* --- implementation bits --- */
namespace Unichar {
inline bool
isvalid (unichar uc)
{
  if (BIRNET_UNLIKELY (uc > 0xfdcf && uc < 0xfdf0))
    return false;
  if (BIRNET_UNLIKELY ((uc & 0xfffe) == 0xfffe))
    return false;
  if (BIRNET_UNLIKELY (uc > 0x10ffff))
    return false;
  if (BIRNET_UNLIKELY ((uc & 0xfffff800) == 0xd800))
    return false;
  return true;
}
} // Unichar

extern const int8 utf8_skip_table[256];

inline const char*
utf8_next (const char *c)
{
  return c + utf8_skip_table[(uint8) *c];
}

inline char*
utf8_next (char *c)
{
  return c + utf8_skip_table[(uint8) *c];
}

inline const char*
utf8_prev (const char *c)
{
  do
    c--;
  while ((*c & 0xc0) == 0x80);
  return c;
}

inline char*
utf8_prev (char *c)
{
  do
    c--;
  while ((*c & 0xc0) == 0x80);
  return c;
}

inline const char*
utf8_find_next (const char *c,
                const char *bound)
{
  if (*c)
    do
      c++;
    while ((!bound || c < bound) && (*c & 0xc0) == 0x80);
  return !bound || c < bound ? c : NULL;
}

inline char*
utf8_find_next (char       *c,
                const char *bound)
{
  return const_cast<char*> (utf8_find_next (const_cast<const char*> (c), bound));
}

inline const char*
utf8_find_prev (const char     *start,
                const char     *current)
{
  do
    current--;
  while (current >= start && (*current & 0xc0) == 0x80);
  return current >= start ? current : NULL;
}

inline char*
utf8_find_prev (const char     *start,
                char           *current)
{
  return const_cast<char*> (utf8_find_prev (start, const_cast<const char*> (current)));
}

inline const char*
utf8_align (const char     *start,
            const char     *current)
{
  while (current > start && (*current & 0xc0) == 0x80)
    current--;
  return current;
}

inline char*
utf8_align (const char *start,
            char       *current)
{
  return const_cast<char*> (utf8_align (start, const_cast<const char*> (current)));
}

inline bool
utf8_aligned (const char *c)
{
  return (*c & 0xc0) == 0x80;
}

} // Birnet

#endif /* __BIRNET_UTF8_HH__ */
/* vim:set ts=8 sts=2 sw=2: */
