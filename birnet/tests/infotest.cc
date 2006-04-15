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
// #define TEST_VERBOSE
#include <birnet/birnettests.h>

static void
test_cpu_info (void)
{
  TSTART ("CpuInfo");
  TOK();
  const BirnetCPUInfo *cpi = birnet_cpu_info ();
  TASSERT (cpi != NULL);
  gchar *cps = birnet_cpu_info_string (cpi);
  TASSERT (cps != NULL);
  TPRINT ("%s", cps);
  TOK();
  g_free (cps);
  TOK();
  TDONE();
}

int
main (int   argc,
      char *argv[])
{
  birnet_init_test (&argc, &argv);

  test_cpu_info();

  return 0;
}

/* vim:set ts=8 sts=2 sw=2: */
