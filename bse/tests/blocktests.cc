/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2006 Tim Janik
 * Copyright (C) 2006 Stefan Westerfeld
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
#include <bse/bseblockutils.hh>
// #define TEST_VERBOSE
#include <sfi/sfitests.h>
#include <bse/bsemain.h>
#include "topconfig.h"

template<typename T> static bool
block_check (guint    n,
             const T *block,
             T        value)
{
  while (n--)
    if (block[n] != value)
      {
        TPRINT ("%f != %f", block[n], value);
        return false;
      }
  return true;
}

/**
 * Shuffles a block, using the O(n) algorithm called the Knuth shuffle
 * or Fisher-Yates shuffle, for instance explained on
 *
 *   http://en.wikipedia.org/wiki/Shuffle#Shuffling_algorithms
 *
 * This creates a random permutation of the elements, where each permutation
 * is equally likely (given that the random generator is good).
 *
 * Since number of possible permutations is n_elements!, which grows rather
 * fast, it can easily happen that some permutations can never be generated
 * due to limited periodicity of the random number generator or weak seeding.
 */
template<typename T> static void
block_shuffle (guint n_elements,
               T    *elements)
{
  for (guint i = 0; i < n_elements; i++)
    {
      guint exchange = g_random_int_range (i, n_elements);
      T tmp = elements[exchange];
      elements[exchange] = elements[i];
      elements[i] = tmp;
    }
}

static void
build_ascending_random_block (guint  n_values,
                              float *fblock)
{
  /* build block with ascending random values approximately in the range -1..1 */
  fblock[0] = g_random_double_range (-1.05, -0.95);
  for (guint i = 1; i < n_values; i++)
    fblock[i] = fblock[i-1] + g_random_double_range (1e-10, 4.0 / n_values);
}

static void
test_fill (void)
{
  TSTART ("BlockFill");
  float fblock1[1024];

  bse_block_fill_uint32 (1024, (uint32*) (void*) fblock1, 0);
  TASSERT (block_check (1024, fblock1, 0.f) == true);

  bse_block_fill_float (1024, fblock1, 17.786);
  TASSERT (block_check (1024, fblock1, 17.786f) == true);

  Bse::Block::fill (1024, fblock1, 17.786f);
  TASSERT (block_check (1024, fblock1, 17.786f) == true);

  Bse::Block::fill (1024, (uint32*) (void*) fblock1, 0);
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
  bse_block_copy_uint32 (1024, (uint32*) (void*) fblock1, (uint32*) (void*) fblock2);
  TASSERT (block_check (1024, fblock1, -213e+3F) == true);

  Bse::Block::fill (1024, fblock1, -8763e-4f);
  Bse::Block::copy (1024, fblock1, fblock2);
  TASSERT (block_check (1024, fblock1, -213e+3F) == true);

  Bse::Block::fill (1024, fblock1, -8763e-4f);
  Bse::Block::copy (1024, (uint32*) (void*) fblock1, (uint32*) (void*) fblock2);
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
test_sub (void)
{
  TSTART ("BlockSub");
  float fblock1[1024], fblock2[1024];

  Bse::Block::fill (1024, fblock1, 2.f);
  Bse::Block::fill (1024, fblock2, 3.f);
  bse_block_sub_floats (1024, fblock1, fblock2);
  TASSERT (block_check (1024, fblock1, -1.f) == true);
  TASSERT (block_check (1024, fblock2, 3.f) == true);

  Bse::Block::fill (1024, fblock1, 2.f);
  Bse::Block::sub (1024, fblock1, fblock2);
  TASSERT (block_check (1024, fblock1, -1.f) == true);
  TASSERT (block_check (1024, fblock2, 3.f) == true);
  
  TDONE();
}

static void
test_mul (void)
{
  TSTART ("BlockMul");
  float fblock1[1024], fblock2[1024];

  Bse::Block::fill (1024, fblock1, 2.f);
  Bse::Block::fill (1024, fblock2, 3.f);
  bse_block_mul_floats (1024, fblock1, fblock2);
  TASSERT (block_check (1024, fblock1, 6.f) == true);
  TASSERT (block_check (1024, fblock2, 3.f) == true);

  Bse::Block::fill (1024, fblock1, 2.f);
  Bse::Block::mul (1024, fblock1, fblock2);
  TASSERT (block_check (1024, fblock1, 6.f) == true);
  TASSERT (block_check (1024, fblock2, 3.f) == true);
  
  TDONE();
}

static void
test_square_sum (void)
{
  TSTART ("BlockSquareSum");
  float fblock[1024];
  float min_value, max_value;

  for (int i = 0; i < 10; i++)
    {
      float energy, energy_db;

      for (int i = 0; i < 1024; i++)
	fblock[i] = sin (i * 2 * M_PI / 1024);

      energy = bse_block_calc_float_square_sum (1024, fblock) / 1024.;
      energy_db = 10 * log10 (energy);

      TPRINT ("sine wave: energy = %f, energy_db = %f\n", energy, energy_db);
      TASSERT (fabs (energy - 0.5) < 0.0000001);

      energy = bse_block_calc_float_range_and_square_sum (1024, fblock, &min_value, &max_value) / 1024.;
      TASSERT (fabs (energy - 0.5) < 0.0000001);

      for (int i = 0; i < 1024; i++)
	fblock[i] = i < 512 ? -1 : 1;

      energy = bse_block_calc_float_square_sum (1024, fblock) / 1024.;
      energy_db = 10 * log10 (energy);

      TPRINT ("square wave: energy = %f, energy_db = %f\n", energy, energy_db);
      TASSERT (fabs (energy - 1.0) < 0.0000001);

      energy = bse_block_calc_float_range_and_square_sum (1024, fblock, &min_value, &max_value) / 1024.;
      TASSERT (fabs (energy - 1.0) < 0.0000001);

      /* square sum (and energy) should not depend on ordering of the elements */
      block_shuffle (1024, fblock);
    }

  TDONE();
}

static void
test_range (void)
{
  TSTART ("BlockRange");

  float fblock[1024];

  build_ascending_random_block (1024, fblock);

  float correct_min_value = fblock[0];
  float correct_max_value = fblock[1023];

  for (int i = 0; i < 10; i++)
    {
      /* shuffle block into quasi random order */
      block_shuffle (1024, fblock);

      /* check that correct minimum and maximum is still found */
      float min_value = 0, max_value = 0;
      bse_block_calc_float_range (1024, fblock, &min_value, &max_value);

      TASSERT (min_value == correct_min_value);
      TASSERT (max_value == correct_max_value);

      bse_block_calc_float_range_and_square_sum (1024, fblock, &min_value, &max_value);

      TASSERT (min_value == correct_min_value);
      TASSERT (max_value == correct_max_value);
    }
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

#define RUNS    11

const int BLOCK_SIZE = 1024;
/*
 * to make benchmarks with different blocksizes comparable,
 * results will be scaled to a standard block size (1024)
 */
const double BENCH_SCALE = 1024. / BLOCK_SIZE;

static inline void
bench_fill (void)
{
  float fblock[BLOCK_SIZE];
  GTimer *timer = g_timer_new();
  g_timer_start (timer);
  const guint dups = TEST_CALIBRATION (50.0, Bse::Block::fill (BLOCK_SIZE, fblock, 2.f));
  g_timer_stop (timer);
  double c = g_timer_elapsed (timer, NULL);
  
  double m = 9e300;
  for (guint i = 0; i < RUNS; i++)
    {
      g_timer_start (timer);
      for (guint j = 0; j < dups; j++)
        Bse::Block::fill (BLOCK_SIZE, fblock, 2.f);
      g_timer_stop (timer);
      double e = g_timer_elapsed (timer, NULL);
      if (e < m)
        m = e;
    }
  treport_minimized ("Block::fill", 1000000.0 * m / dups * BENCH_SCALE, TUNIT_USEC);
  if (0)
    g_print ("FillBench:            %.6f msecs (test-duration: %.6f calibration: %.6f)\n",
             1000.0 * m / dups * BENCH_SCALE, m * RUNS, c);
}

static inline void
bench_copy (void)
{
  float src_fblock[BLOCK_SIZE], dest_fblock[BLOCK_SIZE];
  Bse::Block::fill (BLOCK_SIZE, src_fblock, 2.f);
  Bse::Block::fill (BLOCK_SIZE, dest_fblock, 0.f);
  GTimer *timer = g_timer_new();
  g_timer_start (timer);
  const guint dups = TEST_CALIBRATION (50.0, Bse::Block::copy (BLOCK_SIZE, dest_fblock, src_fblock));
  g_timer_stop (timer);
  double c = g_timer_elapsed (timer, NULL);
  
  double m = 9e300;
  for (guint i = 0; i < RUNS; i++)
    {
      g_timer_start (timer);
      for (guint j = 0; j < dups; j++)
        Bse::Block::copy (BLOCK_SIZE, dest_fblock, src_fblock);
      g_timer_stop (timer);
      double e = g_timer_elapsed (timer, NULL);
      if (e < m)
        m = e;
    }
  g_assert (dest_fblock[0] == 2.f);
  treport_minimized ("Block::copy", 1000000.0 * m / dups * BENCH_SCALE, TUNIT_USEC);
  if (0)
    g_print ("CopyBench:            %.6f msecs (test-duration: %.6f calibration: %.6f)\n",
             1000.0 * m / dups * BENCH_SCALE, m * RUNS, c);
}

static inline void
bench_add (void)
{
  float fblock1[BLOCK_SIZE], fblock2[BLOCK_SIZE];
  Bse::Block::fill (BLOCK_SIZE, fblock1, 2.f);
  Bse::Block::fill (BLOCK_SIZE, fblock2, 3.f);
  GTimer *timer = g_timer_new();
  g_timer_start (timer);
  const guint dups = TEST_CALIBRATION (50.0, Bse::Block::add (BLOCK_SIZE, fblock1, fblock2));
  g_timer_stop (timer);
  double c = g_timer_elapsed (timer, NULL);
  
  double m = 9e300;
  for (guint i = 0; i < RUNS; i++)
    {
      g_timer_start (timer);
      for (guint j = 0; j < dups; j++)
        Bse::Block::add (BLOCK_SIZE, fblock1, fblock2);
      g_timer_stop (timer);
      double e = g_timer_elapsed (timer, NULL);
      if (e < m)
        m = e;
    }
  treport_minimized ("Block::add", 1000000.0 * m / dups * BENCH_SCALE, TUNIT_USEC);
  if (0)
    g_print ("AddBench:             %.6f msecs (test-duration: %.6f calibration: %.6f)\n",
             1000.0 * m / dups * BENCH_SCALE, m * RUNS, c);
}

static inline void
bench_sub (void)
{
  float fblock1[BLOCK_SIZE], fblock2[BLOCK_SIZE];
  Bse::Block::fill (BLOCK_SIZE, fblock1, 2.f);
  Bse::Block::fill (BLOCK_SIZE, fblock2, 3.f);
  GTimer *timer = g_timer_new();
  g_timer_start (timer);
  const guint dups = TEST_CALIBRATION (50.0, Bse::Block::sub (BLOCK_SIZE, fblock1, fblock2));
  g_timer_stop (timer);
  double c = g_timer_elapsed (timer, NULL);
  
  double m = 9e300;
  for (guint i = 0; i < RUNS; i++)
    {
      g_timer_start (timer);
      for (guint j = 0; j < dups; j++)
        Bse::Block::sub (BLOCK_SIZE, fblock1, fblock2);
      g_timer_stop (timer);
      double e = g_timer_elapsed (timer, NULL);
      if (e < m)
        m = e;
    }
  treport_minimized ("Block::sub", 1000000.0 * m / dups * BENCH_SCALE, TUNIT_USEC);
  if (0)
    g_print ("SubBench:             %.6f msecs (test-duration: %.6f calibration: %.6f)\n",
             1000.0 * m / dups * BENCH_SCALE, m * RUNS, c);
}

static inline void
bench_mul (void)
{
  float fblock1[BLOCK_SIZE], fblock2[BLOCK_SIZE];
  Bse::Block::fill (BLOCK_SIZE, fblock1, 2.f);
  Bse::Block::fill (BLOCK_SIZE, fblock2, 1.0000001); /* use a small factor to avoid inf after many block multiplications */
  GTimer *timer = g_timer_new();
  g_timer_start (timer);
  const guint dups = TEST_CALIBRATION (50.0, Bse::Block::mul (BLOCK_SIZE, fblock1, fblock2));
  g_timer_stop (timer);
  double c = g_timer_elapsed (timer, NULL);
  
  double m = 9e300;
  for (guint i = 0; i < RUNS; i++)
    {
      g_timer_start (timer);
      for (guint j = 0; j < dups; j++)
        Bse::Block::mul (BLOCK_SIZE, fblock1, fblock2);
      g_timer_stop (timer);
      double e = g_timer_elapsed (timer, NULL);
      if (e < m)
        m = e;
    }
  g_assert (fblock1[0] < 1e30); /* not close to infinity */
  treport_minimized ("Block::mul", 1000000.0 * m / dups * BENCH_SCALE, TUNIT_USEC);
  if (0)
    g_print ("MulBench:             %.6f msecs (test-duration: %.6f calibration: %.6f)\n",
             1000.0 * m / dups * BENCH_SCALE, m * RUNS, c);
}

static inline void
bench_scale (void)
{
  float fblock1[BLOCK_SIZE], fblock2[BLOCK_SIZE];
  Bse::Block::fill (BLOCK_SIZE, fblock1, 0.f);
  Bse::Block::fill (BLOCK_SIZE, fblock2, 3.f);
  GTimer *timer = g_timer_new();
  g_timer_start (timer);
  const guint dups = TEST_CALIBRATION (50.0, Bse::Block::scale (BLOCK_SIZE, fblock1, fblock2, 2.f));
  g_timer_stop (timer);
  double c = g_timer_elapsed (timer, NULL);

  double m = 9e300;
  for (guint i = 0; i < RUNS; i++)
    {
      g_timer_start (timer);
      for (guint j = 0; j < dups; j++)
        Bse::Block::scale (BLOCK_SIZE, fblock1, fblock2, 2.f);
      g_timer_stop (timer);
      double e = g_timer_elapsed (timer, NULL);
      if (e < m)
        m = e;
    }
  treport_minimized ("Block::scale", 1000000.0 * m / dups * BENCH_SCALE, TUNIT_USEC);
  if (0)
    g_print ("ScaleBench:           %.6f msecs (test-duration: %.6f calibration: %.6f)\n",
             1000.0 * m / dups * BENCH_SCALE, m * RUNS, c);
}

static inline void
bench_range (void)
{
  float fblock[BLOCK_SIZE];
  build_ascending_random_block (BLOCK_SIZE, fblock);

  float correct_min_value = fblock[0];
  float correct_max_value = fblock[BLOCK_SIZE - 1];
  float min_value, max_value;

  /* shuffle block into quasi random order */
  block_shuffle (BLOCK_SIZE, fblock);

  GTimer *timer = g_timer_new();
  g_timer_start (timer);
  const guint dups = TEST_CALIBRATION (50.0, Bse::Block::range (BLOCK_SIZE, fblock, min_value, max_value));
  g_timer_stop (timer);
  double c = g_timer_elapsed (timer, NULL);
  
  double m = 9e300;
  for (guint i = 0; i < RUNS; i++)
    {
      g_timer_start (timer);
      for (guint j = 0; j < dups; j++)
        Bse::Block::range (BLOCK_SIZE, fblock, min_value, max_value);
      g_timer_stop (timer);
      double e = g_timer_elapsed (timer, NULL);
      if (e < m)
        m = e;
    }
  g_assert (min_value == correct_min_value);
  g_assert (max_value == correct_max_value);
  treport_minimized ("Block::range", 1000000.0 * m / dups * BENCH_SCALE, TUNIT_USEC);
  if (0)
    g_print ("RangeBench:           %.6f msecs (test-duration: %.6f calibration: %.6f)\n",
             1000.0 * m / dups * BENCH_SCALE, m * RUNS, c);
}

static inline void
bench_square_sum (void)
{
  float fblock[BLOCK_SIZE];
  Bse::Block::fill (BLOCK_SIZE, fblock, 2.f);
  GTimer *timer = g_timer_new();
  g_timer_start (timer);
  const guint dups = TEST_CALIBRATION (50.0, Bse::Block::square_sum (BLOCK_SIZE, fblock));
  g_timer_stop (timer);
  double c = g_timer_elapsed (timer, NULL);
  
  double m = 9e300;
  for (guint i = 0; i < RUNS; i++)
    {
      g_timer_start (timer);
      for (guint j = 0; j < dups; j++)
        Bse::Block::square_sum (BLOCK_SIZE, fblock);
      g_timer_stop (timer);
      double e = g_timer_elapsed (timer, NULL);
      if (e < m)
        m = e;
    }
  treport_minimized ("Block::square_sum", 1000000.0 * m / dups * BENCH_SCALE, TUNIT_USEC);
  if (0)
    g_print ("SquareSumBench:       %.6f msecs (test-duration: %.6f calibration: %.6f)\n",
             1000.0 * m / dups * BENCH_SCALE, m * RUNS, c);
}

static inline void
bench_range_and_square_sum (void)
{
  float fblock[BLOCK_SIZE];
  build_ascending_random_block (BLOCK_SIZE, fblock);

  float correct_min_value = fblock[0];
  float correct_max_value = fblock[BLOCK_SIZE - 1];
  float min_value, max_value;

  /* shuffle block into quasi random order */
  block_shuffle (BLOCK_SIZE, fblock);

  GTimer *timer = g_timer_new();
  g_timer_start (timer);
  const guint dups = TEST_CALIBRATION (50.0, Bse::Block::range_and_square_sum (BLOCK_SIZE, fblock, min_value, max_value));
  g_timer_stop (timer);
  double c = g_timer_elapsed (timer, NULL);
  
  double m = 9e300;
  for (guint i = 0; i < RUNS; i++)
    {
      g_timer_start (timer);
      for (guint j = 0; j < dups; j++)
        Bse::Block::range_and_square_sum (BLOCK_SIZE, fblock, min_value, max_value);
      g_timer_stop (timer);
      double e = g_timer_elapsed (timer, NULL);
      if (e < m)
        m = e;
    }
  g_assert (min_value == correct_min_value);
  g_assert (max_value == correct_max_value);
  treport_minimized ("Block::range_and_square_sum", 1000000.0 * m / dups * BENCH_SCALE, TUNIT_USEC);
  if (0)
    g_print ("Range+SquareSumBench: %.6f msecs (test-duration: %.6f calibration: %.6f)\n",
             1000.0 * m / dups * BENCH_SCALE, m * RUNS, c);
}

static void
run_tests()
{
  test_fill();
  test_copy();
  test_add();
  test_sub();
  test_mul();
  test_scale();
  /* the next two functions test the range_and_square_sum function, too */
  test_range();
  test_square_sum();

  if (sfi_init_settings().test_perf)
    {
      bench_fill();
      bench_copy();
      bench_add();
      bench_sub();
      bench_mul();
      bench_scale();
      bench_range();
      bench_square_sum();
      bench_range_and_square_sum();
    }
}

int
main (int   argc,
      char *argv[])
{
  /* usually we'd call bse_init_test() here, but we have tests to rnu before plugins are loaded */
  sfi_init_test (&argc, &argv, NULL);
  { /* bse_init_test() usually does this for us */
    SfiCPUInfo ci = sfi_cpu_info();
    char *cname = g_strdup_printf ("%s+%s", ci.machine, bse_block_impl_name());
    treport_cpu_name (cname);
    g_free (cname);
  }
  
  TSTART ("Running Default Block Ops");
  TASSERT (Bse::Block::default_singleton() == Bse::Block::current_singleton());
  TDONE();

  run_tests(); /* run tests on FPU */
 
  /* load plugins */
  SfiInitValue config[] = {
    { "load-core-plugins", "1" },
    { NULL },
  };
  bse_init_test (&argc, &argv, config);

  /* check for possible specialization */
  if (Bse::Block::default_singleton() == Bse::Block::current_singleton())
    return 0;   /* nothing changed */

  TSTART ("Running Intrinsic Block Ops");
  TASSERT (Bse::Block::default_singleton() != Bse::Block::current_singleton());
  TDONE();

  run_tests(); /* run tests with intrinsics */

  return 0;
}
