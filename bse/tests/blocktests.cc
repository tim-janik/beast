/* BSE - Bedevilled Sound Engine
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
#include <bse/bseblockutils.hh>
//#define TEST_VERBOSE
#include <birnet/birnettests.h>
#include <bse/bsemain.h>
#include "topconfig.h"

template<typename T> static bool
block_check (guint    n,
             const T *block,
             T        value)
{
  while (n--)
    if (block[n] != value)
      return false;
  return true;
}

static void
test_fill (void)
{
  TSTART ("BlockFill");
  float fblock1[1024];

  bse_block_fill_uint32 (1024, (guint32*) fblock1, 0);
  TASSERT (block_check (1024, fblock1, 0.f) == true);

  bse_block_fill_float (1024, fblock1, 17.786);
  TASSERT (block_check (1024, fblock1, 17.786f) == true);

  Bse::Block::fill (1024, fblock1, 17.786f);
  TASSERT (block_check (1024, fblock1, 17.786f) == true);

  Bse::Block::fill (1024, (guint32*) fblock1, 0);
  TASSERT (block_check (1024, fblock1, 0.f) == true);

  TDONE();
}

static void
test_copy (void)
{
  TSTART ("BlockCopy");
  float fblock1[1024], fblock2[1024];

  Bse::Block::fill (1024, fblock2, -213e+3f);
  TASSERT (block_check (1024, fblock2, -213e+3f) == true);

  Bse::Block::fill (1024, fblock1, -8763e-4f);
  bse_block_copy_float (1024, fblock1, fblock2);
  TASSERT (block_check (1024, fblock1, -213e+3F) == true);

  Bse::Block::fill (1024, fblock1, -8763e-4f);
  bse_block_copy_uint32 (1024, (guint32*) fblock1, (guint32*) fblock2);
  TASSERT (block_check (1024, fblock1, -213e+3F) == true);

  Bse::Block::fill (1024, fblock1, -8763e-4f);
  Bse::Block::copy (1024, fblock1, fblock2);
  TASSERT (block_check (1024, fblock1, -213e+3F) == true);

  Bse::Block::fill (1024, fblock1, -8763e-4f);
  Bse::Block::copy (1024, (guint32*) fblock1, (guint32*) fblock2);
  TASSERT (block_check (1024, fblock1, -213e+3F) == true);

  TDONE();
}

static void
test_add (void)
{
  TSTART ("BlockAdd");
  float fblock1[1024], fblock2[1024];

  Bse::Block::fill (1024, fblock1, 2.f);
  Bse::Block::fill (1024, fblock2, 3.f);
  bse_block_add_floats (1024, fblock1, fblock2);
  TASSERT (block_check (1024, fblock1, 5.f) == true);
  TASSERT (block_check (1024, fblock2, 3.f) == true);

  Bse::Block::fill (1024, fblock1, 2.f);
  Bse::Block::add (1024, fblock1, fblock2);
  TASSERT (block_check (1024, fblock1, 5.f) == true);
  TASSERT (block_check (1024, fblock2, 3.f) == true);
  
  TDONE();
}

static void
test_scale (void)
{
  TSTART ("BlockScale");
  float fblock1[1024], fblock2[1024];

  Bse::Block::fill (1024, fblock1, 0.f);
  Bse::Block::fill (1024, fblock2, 3.f);
  bse_block_scale_floats (1024, fblock1, fblock2, 2.f);
  TASSERT (block_check (1024, fblock1, 6.f) == true);
  TASSERT (block_check (1024, fblock2, 3.f) == true);

  Bse::Block::fill (1024, fblock1, 0.f);
  Bse::Block::scale (1024, fblock1, fblock2, 2.f);
  TASSERT (block_check (1024, fblock1, 6.f) == true);
  TASSERT (block_check (1024, fblock2, 3.f) == true);
  
  TDONE();
}

#define RUNS    1000
#define DUPS    10

static inline void
scale_add (void)
{
  float fblock1[1024], fblock2[1024];
  Bse::Block::fill (1024, fblock1, 2.f);
  Bse::Block::fill (1024, fblock2, 3.f);
  GTimer *timer = g_timer_new();

  double m = 9e300;
  for (guint i = 0; i < RUNS; i++)
    {
      g_timer_start (timer);
      for (guint j = 0; j < DUPS; j++)
        Bse::Block::add (1024, fblock1, fblock2);
      g_timer_stop (timer);
      double e = g_timer_elapsed (timer, NULL);
      if (e < m)
        m = e;
    }
  g_print ("AddBench:   %.16gmsecs\n", 1000.0 * m / DUPS);
}

static inline void
scale_bench (void)
{
  float fblock1[1024], fblock2[1024];
  Bse::Block::fill (1024, fblock1, 0.f);
  Bse::Block::fill (1024, fblock2, 3.f);
  GTimer *timer = g_timer_new();

  double m = 9e300;
  for (guint i = 0; i < RUNS; i++)
    {
      g_timer_start (timer);
      for (guint j = 0; j < DUPS; j++)
        Bse::Block::scale (1024, fblock1, fblock2, 2.f);
      g_timer_stop (timer);
      double e = g_timer_elapsed (timer, NULL);
      if (e < m)
        m = e;
    }
  g_print ("ScaleBench: %.16gmsecs\n", 1000.0 * m / DUPS);
}

int
main (int   argc,
      char *argv[])
{
  birnet_init_test (&argc, &argv);

  TSTART ("Running Default Block Ops");
  TASSERT (Bse::Block::default_singleton() == Bse::Block::current_singleton());
  TDONE();

  /* standard FPU tests */
  test_fill();
  test_copy();
  test_add();
  test_scale();
  scale_add();
  scale_bench();
  
  /* load plugins */
  SfiRec *config = sfi_rec_new();
  sfi_rec_set_bool (config, "load-core-plugins", TRUE);
  bse_init_intern (&argc, &argv, __FILE__, config);

  /* check for possible specialization */
  if (Bse::Block::default_singleton() == Bse::Block::current_singleton())
    return 0;   /* nothing changed */

  TSTART ("Running Intrinsic Block Ops");
  TASSERT (Bse::Block::default_singleton() != Bse::Block::current_singleton());
  TDONE();

  /* intrinsic tests */
  test_fill();
  test_copy();
  test_add();
  test_scale();
  scale_add();
  scale_bench();

  return 0;
}
