/* SSE optimized FIR Resampling code
 * Copyright (C) 2006 Stefan Westerfeld
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
#include <bse/bseresampler.hh>
#include <bse/bseresampler.tcc>

#include <stdio.h>
#include <math.h>
#include <vector>
#include <sys/time.h>
#include <time.h>

using std::vector;
using std::min;
using std::max;
using std::copy;
using namespace Bse::Resampler;

#include "testresamplercoeffs.h"

void
usage ()
{
  printf ("usage: testresampler <options> [ <block_size> ] [ <test_frequency> ]\n");
  printf ("\n");
  printf ("  p - performance\n");
  printf ("  a - accuracy\n");
  printf ("  g - generate output for gnuplot\n");
  printf ("  i - impulse response\n");
  printf ("  F - filter implementation\n");
  printf ("\n");
  printf ("  d - downsample\n");
  printf ("  u - upsample\n");
  printf ("  s - subsample (= downsample and upsample after that)\n");
  printf ("  o - oversample (= upsample and downsample after that)\n");
  printf ("\n");
  printf ("  f - fast implementation (sse)\n");
  printf ("\n");
  printf ("  block_size defaults to 128 values\n");
  printf ("  test_frequency defaults to 440 Hz\n");
  printf ("\n");
  printf ("examples:\n");
  printf ("  testresampler puf 256     # check performance of fast upsampling with 256 value blocks\n");
  printf ("  testresampler au          # check accuracy of standard upsampling\n");
  printf ("  testresampler au 128 500  # check accuracy of standard upsampling using a 500 Hz frequency\n");
}

enum TestType
{
  TEST_NONE,
  TEST_PERFORMANCE,
  TEST_ACCURACY,
  TEST_GNUPLOT,
  TEST_IMPULSE,
  TEST_FILTER_IMPL
} test_type = TEST_NONE;

enum ResampleType
{
  RES_NONE,
  RES_DOWNSAMPLE,
  RES_UPSAMPLE,
  RES_SUBSAMPLE,
  RES_OVERSAMPLE
} resample_type = RES_NONE;

enum ImplType
{
  IMPL_NORMAL,
  IMPL_SSE
} impl_type = IMPL_NORMAL;

unsigned int block_size = 128;
double test_frequency = 440.0;

double
gettime ()
{
  timeval tv;
  gettimeofday (&tv, 0);

  return tv.tv_sec + tv.tv_usec / 1000000.0;
}

int
test_filter_impl()
{
  int errors = 0;
  printf ("testing filter implementation:\n\n");

  for (guint order = 0; order < 64; order++)
    {
      vector<float> taps (order);
      for (guint i = 0; i < order; i++)
	taps[i] = i + 1;

      AlignedMem<float,16> sse_taps (fir_compute_sse_taps (taps));
      for (unsigned int i = 0; i < sse_taps.size(); i++)
	{
	  printf ("%3d", (int) (sse_taps[i] + 0.5));
	  if (i % 4 == 3)
	    printf ("  |");
	  if (i % 16 == 15)
	    printf ("   ||| upper bound = %d\n", (order + 6) / 4);
	}
      printf ("\n\n");

      AlignedMem<float,16> random_mem (order + 4);
      for (guint i = 0; i < order + 4; i++)
	random_mem[i] = 1.0 - rand() / (0.5 * RAND_MAX);

      /* FIXME: the problem with this test is that we explicitely test SSE code
       * here, but the test case is not compiled with -msse within the BEAST tree
       */
      float out[4];
      fir_process_4samples_sse (&random_mem[0], &sse_taps[0], order,
	                        &out[0], &out[1], &out[2], &out[3]);

      double adiff = 0.0;
      for (int i = 0; i < 4; i++)
	{
	  double diff = fir_process_one_sample<double> (&random_mem[i], &taps[0], order) - out[i];
	  adiff += fabs (diff);
	}
      if (adiff > 0.0001)
	{
	  printf ("*** order = %d, adiff = %f\n", order, adiff);
	  errors++;
	}
    }
  if (errors)
    printf ("*** %d errors detected\n", errors);
  else
    printf ("filter implementation ok.\n");

  return errors ? 1 : 0;
}

template <int TEST, int RESAMPLE, int IMPL> int
perform_test()
{
  if (TEST == TEST_IMPULSE) /* need to have space for impulse response for all 4 tests */
    block_size = 150;

  /* initialize up- and downsampler */
  const int T = 32;
  const bool USE_SSE = (IMPL == IMPL_SSE);
  float utaps[T], dtaps[T];
  for (int i = 0; i < T; i++)
    {
      utaps[i] = halfband_fir_upsample2_96db_coeffs[i] * 2;
      dtaps[i] = halfband_fir_upsample2_96db_coeffs[i];
    }

  Upsampler2<T,USE_SSE> ups (utaps);
  Downsampler2<T,USE_SSE> downs (dtaps);

  F4Vector in_v[block_size / 2 + 1], out_v[block_size / 2 + 1], out2_v[block_size / 2 + 1];
  float *input = &in_v[0].f[0], *output = &out_v[0].f[0], *output2 = &out2_v[0].f[0]; /* ensure aligned data */

  if (TEST == TEST_PERFORMANCE)
    {
      for (unsigned int i = 0; i < block_size; i++)
	input[i] = sin (i * test_frequency / 44100.0 * 2 * M_PI);

      double start_time = gettime();
      long long k = 0;
      for (int i = 0; i < 500000; i++)
	{
	  if (RESAMPLE == RES_DOWNSAMPLE || RESAMPLE == RES_SUBSAMPLE)
	    {
	      downs.process_block (input, block_size, output);
	      if (RESAMPLE == RES_SUBSAMPLE)
		ups.process_block (output, block_size / 2, output2);
	    }
	  if (RESAMPLE == RES_UPSAMPLE || RESAMPLE == RES_OVERSAMPLE)
	    {
	      ups.process_block (input, block_size, output);
	      if (RESAMPLE == RES_OVERSAMPLE)
		downs.process_block (output, block_size * 2, output2);
	    }
	  k += block_size;
	}
      double end_time = gettime();
      if (RESAMPLE == RES_DOWNSAMPLE)
	{
	  printf ("  (performance will be normalized to downsampler output samples)\n");
	  k /= 2;
	}
      else if (RESAMPLE == RES_UPSAMPLE)
	{
	  printf ("  (performance will be normalized to upsampler input samples)\n");
	}
      printf ("  total samples processed = %lld\n", k);
      printf ("  processing_time = %f\n", end_time - start_time);
      printf ("  samples / second = %f\n", k / (end_time - start_time));
      printf ("  which means the resampler can process %.2f 44100 Hz streams simultaneusly\n",
	      k / (end_time - start_time) / 44100.0);
      printf ("  or one 44100 Hz stream takes %f %% CPU usage\n", 
	        100.0 / (k / (end_time - start_time) / 44100.0));
    }
  else if (TEST == TEST_ACCURACY || TEST == TEST_GNUPLOT)
    {
      long long k = 0;
      double phase = 0;
      double max_diff = 0;
      for (int b = 0; b < 1000; b++)
	{
	  int misalign = rand() % 4;
	  int bs = rand() % (block_size - misalign);

	  if (RESAMPLE == RES_DOWNSAMPLE || RESAMPLE == RES_SUBSAMPLE)
	    bs -= bs & 1;

	  for (int i = 0; i < bs; i++)
	    {
	      input[i+misalign] = sin (phase);
	      phase += test_frequency/44100.0 * 2 * M_PI;
	    }
	  if (RESAMPLE == RES_DOWNSAMPLE || RESAMPLE == RES_SUBSAMPLE)
	    {
	      downs.process_block (input + misalign, bs, output);
	      if (RESAMPLE == RES_SUBSAMPLE)
		ups.process_block (output, bs / 2, output2);
	    }
	  if (RESAMPLE == RES_UPSAMPLE || RESAMPLE == RES_OVERSAMPLE)
	    {
	      ups.process_block (input + misalign, bs, output);
	      if (RESAMPLE == RES_OVERSAMPLE)
		downs.process_block (output, bs * 2, output2);
	    }

	  /* validate output */
	  double sin_shift;
	  double freq_factor;
	  unsigned int out_bs;
	  float *check = output;

	  if (RESAMPLE == RES_UPSAMPLE)
	    {
	      sin_shift = 34;
	      freq_factor = 0.5;
	      out_bs = bs * 2;
	    }
	  else if (RESAMPLE == RES_DOWNSAMPLE)
	    {
	      sin_shift = 16.5;
	      freq_factor = 2;
	      out_bs = bs / 2;
	    }
	  else if (RESAMPLE == RES_OVERSAMPLE)
	    {
	      sin_shift = 33.5;
	      freq_factor = 1;
	      check = output2;
	      out_bs = bs;
	    }
	  else if (RESAMPLE == RES_SUBSAMPLE)
	    {
	      sin_shift = 67;
	      freq_factor = 1;
	      check = output2;
	      out_bs = bs;
	    }

	  for (unsigned int i = 0; i < out_bs; i++, k++)
	    if (k > 100)
	      {
		double correct_output = sin ((k - sin_shift) * 2 * freq_factor * test_frequency / 44100.0 * M_PI);
		if (TEST == TEST_GNUPLOT)
		  printf ("%lld %.17f %.17f\n", k, check[i], correct_output);
		else
		  max_diff = max (max_diff, check[i] - correct_output);
	      }
	}
      double max_diff_db = 20 * log (max_diff) / log (10);
      if (TEST == TEST_ACCURACY)
	{
	  printf ("  input frequency used to perform test = %.2f Hz (SR = 44100.0 Hz)\n", test_frequency);
	  printf ("  max difference between correct and computed output: %f = %f dB\n", max_diff, max_diff_db);
	}
    }
  else if (TEST == TEST_IMPULSE)
    {
      input[0] = 1;
      for (unsigned int i = 1; i < block_size; i++)
	{
	  input[i] = 0;
	  output[i] = 0;
	  output2[i] = 0;
	}

      if (RESAMPLE == RES_DOWNSAMPLE || RESAMPLE == RES_SUBSAMPLE)
	{
	  downs.process_block (input, block_size, output);
	  if (RESAMPLE == RES_SUBSAMPLE)
	    ups.process_block (output, block_size / 2, output2);
	}
      if (RESAMPLE == RES_UPSAMPLE || RESAMPLE == RES_OVERSAMPLE)
	{
	  ups.process_block (input, block_size, output);
	  if (RESAMPLE == RES_OVERSAMPLE)
	    downs.process_block (output, block_size * 2, output2);
	}

      float *check = output;
      if (RESAMPLE == RES_OVERSAMPLE || RESAMPLE == RES_SUBSAMPLE)
	check = output2;

      for (unsigned int i = 0; i < block_size; i++)
	printf ("%.17f\n", check[i]);
    }
  return 0;
}

template <int TEST, int RESAMPLE> int
perform_test()
{
  switch (impl_type)
    {
    case IMPL_SSE:    printf ("using SSE instructions\n"); return perform_test<TEST, RESAMPLE, IMPL_SSE> ();
    case IMPL_NORMAL: printf ("using FPU instructions\n"); return perform_test<TEST, RESAMPLE, IMPL_NORMAL> ();
    }
  g_assert_not_reached();
  return 1;
}

template <int TEST> int
perform_test ()
{
  switch (resample_type)
    {
    case RES_DOWNSAMPLE:  printf ("for factor 2 downsampling "); return perform_test<TEST, RES_DOWNSAMPLE> ();
    case RES_UPSAMPLE:	  printf ("for factor 2 upsampling "); return perform_test<TEST, RES_UPSAMPLE> ();
    case RES_SUBSAMPLE:	  printf ("for factor 2 subsampling "); return perform_test<TEST, RES_SUBSAMPLE> ();
    case RES_OVERSAMPLE:  printf ("for factor 2 oversampling "); return perform_test<TEST, RES_OVERSAMPLE> ();
    default:		  usage(); return 1;
    }
}

int
perform_test()
{
  switch (test_type)
    {
    case TEST_PERFORMANCE:  printf ("performance test "); return perform_test<TEST_PERFORMANCE> ();
    case TEST_ACCURACY:	    printf ("accuracy test "); return perform_test<TEST_ACCURACY> ();
    case TEST_GNUPLOT:	    printf ("# gnuplot test "); return perform_test<TEST_GNUPLOT> ();
    case TEST_IMPULSE:	    printf ("# impulse response test "); return perform_test<TEST_IMPULSE> ();
    case TEST_FILTER_IMPL:  return test_filter_impl();
    default:		    usage(); return 1;
    }
}

int
main (int argc, char **argv)
{
  if (argc >= 2)
    {
      for (unsigned int i = 0; i < strlen (argv[1]); i++)
	{
	  switch (argv[1][i])
	    {
	    case 'F': test_type = TEST_FILTER_IMPL; break;
	    case 'p': test_type = TEST_PERFORMANCE; break;
	    case 'a': test_type = TEST_ACCURACY; break;
	    case 'g': test_type = TEST_GNUPLOT; break;
	    case 'i': test_type = TEST_IMPULSE; break;

	    case 'd': resample_type = RES_DOWNSAMPLE; break;
	    case 'u': resample_type = RES_UPSAMPLE; break;
	    case 's': resample_type = RES_SUBSAMPLE; break;
	    case 'o': resample_type = RES_OVERSAMPLE; break;

	    case 'f': impl_type = IMPL_SSE; break;
	    }
	}
    }

  if (argc >= 3)
    block_size = atoi (argv[2]);

  if (argc >= 4)
    test_frequency = atoi (argv[3]);

  if ((block_size & 1) == 1)
    block_size++;

  if (block_size < 2)
    block_size = 2;

  return perform_test();
}
