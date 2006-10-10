/* Birnet
 * Copyright (C) 2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
//#define TEST_VERBOSE
#include <birnet/birnettests.h>
using namespace Birnet;

namespace {
using namespace Birnet;

static void
random_tf8_and_unichar_test (void)
{
  TSTART ("utf8<->unichar");
  const uint count = 1000000;
  for (uint i = 0; i < count; i++)
    {
      if (i % 20000 == 0)
        TOK();
      unichar nc, uc = rand() % (0x100 << (i % 24));
      if (!uc)
        continue;
      char buffer[8], gstr[10] = { 0, };
      int l;

      l = utf8_from_unichar (uc, buffer);
      TCHECK (l > 0);
      TCHECK (l <= 7);
      TCHECK (buffer[l] == 0);
      TCHECK (l == g_unichar_to_utf8 (uc, gstr));
      TCHECK (strcmp (gstr, buffer) == 0);
      nc = utf8_to_unichar (buffer);
      TCHECK (nc == g_utf8_get_char (buffer));
      TCHECK (uc == nc);
      TCHECK (l == utf8_from_unichar (uc, NULL));
      char *p1 = utf8_next (buffer);
      TCHECK (p1 == buffer + l);
      char *p2 = utf8_prev (p1);
      TCHECK (p2 == buffer);

      char cbuffer[1024];
      snprintf (cbuffer, 1024, "x%sy7", buffer);
      char *cur = cbuffer, *pn, *gn, *pp;
      /* x */
      pn = utf8_find_next (cur);
      TCHECK (pn == cur + 1);
      gn = g_utf8_find_next_char (cur, NULL);
      TCHECK (pn == gn);
      pp = utf8_find_prev (pn, cbuffer);
      TCHECK (pp == cur);
      /* random unichar */
      cur = pn;
      pn = utf8_find_next (cur);
      TCHECK (pn == cur + l);
      gn = g_utf8_find_next_char (cur, NULL);
      TCHECK (pn == gn);
      pp = utf8_find_prev (pn, cbuffer);
      TCHECK (pp == cur);
      /* y */
      cur = pn;
      pn = utf8_find_next (cur);
      TCHECK (pn == cur + 1);
      gn = g_utf8_find_next_char (cur, NULL);
      TCHECK (pn == gn);
      pp = utf8_find_prev (pn, cbuffer);
      TCHECK (pp == cur);
      /* 7 (last) */
      cur = pn;
      pn = utf8_find_next (cur);
      TCHECK (pn == cur + 1);
      gn = g_utf8_find_next_char (cur, NULL);
      TCHECK (pn == gn);
      pp = utf8_find_prev (pn, cbuffer);
      TCHECK (pp == cur);
      /* last with bounds */
      pn = utf8_find_next (cur, cur + strlen (cur));
      TCHECK (pn == NULL);
      gn = g_utf8_find_next_char (cur, cur + strlen (cur));
      TCHECK (pn == gn);
      /* first with bounds */
      pp = utf8_find_prev (cbuffer, cbuffer);
      TCHECK (pp == NULL);
    }
  TDONE();
}

static void
random_unichar_test (void)
{
  TSTART ("unichar classification");
  const uint count = 1000000;
  for (uint i = 0; i < count; i++)
    {
      unichar uc = rand() % (0x100 << (i % 24));
      unichar bc, gc;
      gboolean gb;
      bool bb;
      int bv, gv;
      if (i % 20000 == 0)
        TOK();

      bb = Unichar::isalnum (uc);
      gb = g_unichar_isalnum (uc);
      TCHECK (bb == gb);
      bb = Unichar::isalpha (uc);
      gb = g_unichar_isalpha (uc);
      TCHECK (bb == gb);
      bb = Unichar::iscntrl (uc);
      gb = g_unichar_iscntrl (uc);
      TCHECK (bb == gb);
      bb = Unichar::isdigit (uc);
      gb = g_unichar_isdigit (uc);
      TCHECK (bb == gb);
      bv = Unichar::digit_value (uc);
      gv = g_unichar_digit_value (uc);
      TCHECK (bv == gv);
      bv = Unichar::digit_value ('0' + uc % 10);
      gv = g_unichar_digit_value ('0' + uc % 10);
      TCHECK (bv == gv);
      bb = Unichar::isgraph (uc);
      gb = g_unichar_isgraph (uc);
      TCHECK (bv == gv);
      bb = Unichar::islower (uc);
      gb = g_unichar_islower (uc);
      TCHECK (bb == gb);
      bc = Unichar::tolower (uc);
      gc = g_unichar_tolower (uc);
      TCHECK (bc == gc);
      bb = Unichar::isprint (uc);
      gb = g_unichar_isprint (uc);
      TCHECK (bb == gb);
      bb = Unichar::ispunct (uc);
      gb = g_unichar_ispunct (uc);
      TCHECK (bb == gb);
      bb = Unichar::isspace (uc);
      gb = g_unichar_isspace (uc);
      TCHECK (bb == gb);
      bb = Unichar::isupper (uc);
      gb = g_unichar_isupper (uc);
      TCHECK (bb == gb);
      bc = Unichar::toupper (uc);
      gc = g_unichar_toupper (uc);
      TCHECK (bc == gc);
      bb = Unichar::isxdigit (uc);
      gb = g_unichar_isxdigit (uc);
      TCHECK (bb == gb);
      bv = Unichar::xdigit_value (uc);
      gv = g_unichar_xdigit_value (uc);
      TCHECK (bv == gv);
      bv = Unichar::xdigit_value ('0' + uc % 10);
      gv = g_unichar_xdigit_value ('0' + uc % 10);
      TCHECK (bv == gv);
      bv = Unichar::xdigit_value ('a' + uc % 6);
      gv = g_unichar_xdigit_value ('a' + uc % 6);
      TCHECK (bv == gv);
      bv = Unichar::xdigit_value ('A' + uc % 6);
      gv = g_unichar_xdigit_value ('A' + uc % 6);
      TCHECK (bv == gv);
      bb = Unichar::istitle (uc);
      gb = g_unichar_istitle (uc);
      TCHECK (bb == gb);
      bc = Unichar::totitle (uc);
      gc = g_unichar_totitle (uc);
      TCHECK (bc == gc);
      bb = Unichar::isdefined (uc);
      gb = g_unichar_isdefined (uc);
      TCHECK (bb == gb);
      bb = Unichar::iswide (uc);
      gb = g_unichar_iswide (uc);
      TCHECK (bb == gb);
#if GLIB_CHECK_VERSION (2, 10, 0)
      bb = Unichar::iswide_cjk (uc);
      gb = g_unichar_iswide_cjk (uc);
      TCHECK (bb == gb);
#endif
      TCHECK (Unichar::get_type (uc) == (int) g_unichar_type (uc));
      TCHECK (Unichar::get_break (uc) == (int) g_unichar_break_type (uc));
    }
  TDONE();
}

static void
uuid_tests (void)
{
  TSTART ("uuid string test");
  TASSERT (string_is_uuid ("00000000-0000-0000-0000-000000000000") == true);
  TASSERT (string_is_uuid ("6ba7b812-9dad-11d1-80b4-00c04fd430c8") == true);
  TASSERT (string_is_uuid ("6BA7B812-9DAD-11D1-80B4-00C04FD430C8") == true);
  TASSERT (string_is_uuid ("a425fd92-4f06-11db-aea9-000102e7e309") == true);
  TASSERT (string_is_uuid ("A425FD92-4F06-11DB-AEA9-000102E7E309") == true);
  TASSERT (string_is_uuid ("dc380602-a739-4be1-a5cb-53c437ffe39f") == true);
  TASSERT (string_is_uuid ("DC380602-A739-4BE1-A5CB-53C437FFE39F") == true);
  // TASSERT (string_is_uuid (NULL) == false);
  TASSERT (string_is_uuid ("") == false);
  TASSERT (string_is_uuid ("gba7b812-9dad-11d1-80b4-00c04fd430c8") == false);
  TASSERT (string_is_uuid ("Gba7b812-9dad-11d1-80b4-00c04fd430c8") == false);
  TASSERT (string_is_uuid ("6ba7b812.9dad-11d1-80b4-00c04fd430c8") == false);
  TASSERT (string_is_uuid ("6ba7b812-9dad.11d1-80b4-00c04fd430c8") == false);
  TASSERT (string_is_uuid ("6ba7b812-9dad-11d1.80b4-00c04fd430c8") == false);
  TASSERT (string_is_uuid ("6ba7b812-9dad-11d1-80b4.00c04fd430c8") == false);
  TASSERT (string_is_uuid ("6ba7b812-9dad-11d1-80b4-00c04fd430c8-") == false);
  TASSERT (string_is_uuid ("6ba7b812-9dad-11d1-80b4-00c04fd430c80") == false);
  TASSERT (string_is_uuid ("6ba7b812-9dad-11d1-80b4-00c04fd430c") == false);
  TDONE();
  TSTART ("uuid string cmp");
  TASSERT (string_cmp_uuid ("00000000-0000-0000-0000-000000000000", "A425FD92-4F06-11DB-AEA9-000102E7E309") < 0);
  TASSERT (string_cmp_uuid ("A425FD92-4F06-11DB-AEA9-000102E7E309", "00000000-0000-0000-0000-000000000000") > 0);
  TASSERT (string_cmp_uuid ("00000000-0000-0000-0000-000000000000", "6ba7b812-9dad-11d1-80b4-00c04fd430c8") < 0);
  TASSERT (string_cmp_uuid ("6BA7B812-9DAD-11D1-80B4-00C04FD430C8", "00000000-0000-0000-0000-000000000000") > 0);
  TASSERT (string_cmp_uuid ("00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000") == 0);
  TASSERT (string_cmp_uuid ("6BA7B812-9DAD-11D1-80B4-00C04FD430C8", "A425FD92-4F06-11DB-AEA9-000102E7E309") < 0);
  TASSERT (string_cmp_uuid ("6ba7b812-9dad-11d1-80b4-00c04fd430c8", "A425FD92-4F06-11DB-AEA9-000102E7E309") < 0);
  TASSERT (string_cmp_uuid ("6BA7B812-9DAD-11D1-80B4-00C04FD430C8", "a425fd92-4f06-11db-aea9-000102e7e309") < 0);
  TASSERT (string_cmp_uuid ("6ba7b812-9dad-11d1-80b4-00c04fd430c8", "a425fd92-4f06-11db-aea9-000102e7e309") < 0);
  TASSERT (string_cmp_uuid ("A425FD92-4F06-11DB-AEA9-000102E7E309", "a425fd92-4f06-11db-aea9-000102e7e309") == 0);
  TASSERT (string_cmp_uuid ("6ba7b812-9DAD-11d1-80B4-00c04fd430c8", "6BA7B812-9dad-11D1-80b4-00C04FD430C8") == 0);
  TASSERT (string_cmp_uuid ("A425FD92-4F06-11DB-AEA9-000102E7E309", "6BA7B812-9DAD-11D1-80B4-00C04FD430C8") > 0);
  TASSERT (string_cmp_uuid ("A425FD92-4F06-11DB-AEA9-000102E7E309", "6ba7b812-9dad-11d1-80b4-00c04fd430c8") > 0);
  TASSERT (string_cmp_uuid ("a425fd92-4f06-11db-aea9-000102e7e309", "6BA7B812-9DAD-11D1-80B4-00C04FD430C8") > 0);
  TASSERT (string_cmp_uuid ("a425fd92-4f06-11db-aea9-000102e7e309", "6ba7b812-9dad-11d1-80b4-00c04fd430c8") > 0);
  TDONE();
}

} // Anon

int
main (int   argc,
      char *argv[])
{
  birnet_init_test (&argc, &argv);

  uuid_tests();
  random_unichar_test();
  random_tf8_and_unichar_test();
  
  return 0;
}

/* vim:set ts=8 sts=2 sw=2: */
