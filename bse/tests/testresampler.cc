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
#include <bse/bseresamplerimpl.hh>
#include <bse/bseblockutils.hh>
#include <birnet/birnettests.h>
#include <bse/bsemain.h>
#include "topconfig.h"

#include <stdio.h>
#include <math.h>
#include <sys/time.h>
#include <time.h>

#include <string>
#include <vector>

using std::string;
using std::vector;
using std::min;
using std::max;
using std::copy;
using namespace Bse::Resampler;

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

struct Options {
  guint			  block_size;
  double		  frequency;
  double		  freq_min;
  double		  freq_max;
  double		  freq_inc;
  bool                    freq_scan_verbose;
  double                  precision_assert_db;
  BseResampler2Precision  precision;
  string		  program_name;

  Options() :
    block_size (128),
    frequency (440.0),
    freq_min (-1),
    freq_max (-1),
    freq_inc (0),
    freq_scan_verbose (false),
    precision_assert_db (0),
    precision (BSE_RESAMPLER2_PREC_96DB),
    program_name ("testresampler")
  {
  }
  void parse (int *argc_p, char **argv_p[]);
} options;

static void
usage ()
{
  g_printerr ("usage: testresampler [ <options>... ] <mode>\n");
  g_printerr ("\n");
  g_printerr ("The following characters are required for specifying the mode:\n"); 
  g_printerr ("\n");
  g_printerr ("  p - performance\n");
  g_printerr ("  a - accuracy\n");
  g_printerr ("  g - generate output for gnuplot\n");
  g_printerr ("  i - impulse response\n");
  g_printerr ("  F - filter implementation\n");
  g_printerr ("\n");
  g_printerr ("  d - downsample\n");
  g_printerr ("  u - upsample\n");
  g_printerr ("  s - subsample (= downsample and upsample after that)\n");
  g_printerr ("  o - oversample (= upsample and downsample after that)\n");
  g_printerr ("\n");
  g_printerr ("  f - fast implementation (sse) (optional)\n");
  g_printerr ("\n");
  g_printerr ("The following options can be used to change the default parameters for tests:\n");
  g_printerr ("\n");
  g_printerr (" --frequency=<freq>         use <freq> as test frequency [%f]\n", options.frequency);
  g_printerr (" --block-size=<bs>          use <bs> as block size [%d]\n", options.block_size);
  g_printerr (" --precision=<bits>         use a filter for <bits> precision signals [%d]\n", static_cast<int> (options.precision));
  g_printerr ("\n");
  g_printerr ("For the accuracy test, the following options enable testing multiple frequencies:\n");
  g_printerr ("\n");
  g_printerr (" --freq-scan=<fmin>,<fmax>,<ffact>\n");
  g_printerr ("                            scan frequency frequency range [<fmin>,<fmax>]\n");
  g_printerr ("                            incrementing frequencies by <ffact> after each scan point\n");
  g_printerr (" --freq-scan-verbose        verbose output for range scanning [%s]\n", options.precision_assert_db ? "true" : "false");
  g_printerr (" --precision-assert-db=<db> assert that the effective precision is <db> dB [%f]\n", options.precision_assert_db);
  g_printerr ("\n");
  g_printerr ("Examples:\n");
  g_printerr ("  # check performance of fast upsampling with 256 value blocks:\n");
  g_printerr ("  testresampler --block-size=256 puf\n");
  g_printerr ("  # check accuracy of standard upsampling:\n");
  g_printerr ("  testresampler au\n");
  g_printerr ("  # check accuracy of standard upsampling using a 500 Hz frequency:\n");
  g_printerr ("  testresampler --frequency=500 --block-size=128 au\n");
  g_printerr ("  # check accuracy of upsampling with a frequency-range and a minimum\n");
  g_printerr ("  # precision, using coefficients designed for 20 bits precision:\n");
  g_printerr ("  testresampler --precision=20 --freq-scan=50,18000,50 --precision-assert-db=100 au\n");
}


static bool
check_arg (uint         argc,
           char        *argv[],
           uint        *nth,
           const char  *opt,              /* for example: --foo */
           const char **opt_arg = NULL)   /* if foo needs an argument, pass a pointer to get the argument */
{
  g_return_val_if_fail (opt != NULL, false);
  g_return_val_if_fail (*nth < argc, false);

  const char *arg = argv[*nth];
  if (!arg)
    return false;

  uint opt_len = strlen (opt);
  if (strcmp (arg, opt) == 0)
    {
      if (opt_arg && *nth + 1 < argc)     /* match foo option with argument: --foo bar */
        {
          argv[(*nth)++] = NULL;
          *opt_arg = argv[*nth];
          argv[*nth] = NULL;
          return true;
        }
      else if (!opt_arg)                  /* match foo option without argument: --foo */
        {
          argv[*nth] = NULL;
          return true;
        }
      /* fall through to error message */
    }
  else if (strncmp (arg, opt, opt_len) == 0 && arg[opt_len] == '=')
    {
      if (opt_arg)                        /* match foo option with argument: --foo=bar */
        {
          *opt_arg = arg + opt_len + 1;
          argv[*nth] = NULL;
          return true;
        }
      /* fall through to error message */
    }
  else
    return false;

  usage();
  exit (1);
}

void
Options::parse (int   *argc_p,
                char **argv_p[])
{
  guint argc = *argc_p;
  gchar **argv = *argv_p;
  unsigned int i;

  g_return_if_fail (argc >= 0);

  /*  I am tired of seeing .libs/lt-bsefcompare all the time,
   *  but basically this should be done (to allow renaming the binary):
   *
  if (argc && argv[0])
    program_name = argv[0];
  */

  for (i = 1; i < argc; i++)
    {
      const char *opt_arg;
      if (strcmp (argv[i], "--help") == 0 ||
          strcmp (argv[i], "-h") == 0)
        {
          usage();
          exit (0);
        }
      else if (strcmp (argv[i], "--version") == 0 ||
               strcmp (argv[i], "-v") == 0)
        {
          printf ("%s %s\n", program_name.c_str(), BSE_VERSION);
          exit (0);
        }
      else if (check_arg (argc, argv, &i, "--block-size", &opt_arg))
	{
	  block_size = atoi (opt_arg);
	  if ((block_size & 1) == 1)
	    {
	      block_size++;
	      g_printerr ("testresampler: block size needs to be even (fixed: using %d as block size)\n", block_size);
	    }

	  if (block_size < 2)
	    {
	      block_size = 2;
	      g_printerr ("testresampler: block size needs to be at least 2 (fixed: using %d as block size)\n", block_size);
	    }
	}
      else if (check_arg (argc, argv, &i, "--precision", &opt_arg))
	{
	  int p = atoi (opt_arg);
	  switch (p)
	    {
	    case 8:
	    case 12:
	    case 16:
	    case 20:
	    case 24: precision = static_cast<BseResampler2Precision> (p);
	      break;
	    default: g_printerr ("testresampler: unsupported precision: %d\n", p);
		     exit (1);
	    }
	}
      else if (check_arg (argc, argv, &i, "--freq-scan", &opt_arg))
	{
	  gchar *oa = g_strdup (opt_arg);
	  gchar *fmin = strtok (oa, ",");
	  gchar *fmax = fmin ? strtok (NULL, ",") : NULL;
	  gchar *ffact = fmax ? strtok (NULL, ",") : NULL;

	  if (ffact)
	    {
	      freq_min = g_ascii_strtod (fmin, NULL);
	      freq_max = g_ascii_strtod (fmax, NULL);
	      freq_inc = g_ascii_strtod (ffact, NULL);
	    }
	  if (freq_inc < 1)
	    {
	      g_printerr ("testresampler: invalid frequency scanning specification\n");
	      exit (1);
	    }
	  g_free (oa);
	}
      else if (check_arg (argc, argv, &i, "--freq-scan-verbose"))
	freq_scan_verbose = true;
      else if (check_arg (argc, argv, &i, "--frequency", &opt_arg))
        frequency = g_ascii_strtod (opt_arg, NULL);
      else if (check_arg (argc, argv, &i, "--precision-assert-db", &opt_arg))
	{
	  precision_assert_db = g_ascii_strtod (opt_arg, NULL);
	  /* we allow both: specifying -96 or 96 to assert 96 dB precision */
	  if (precision_assert_db > 0)
	    precision_assert_db = -precision_assert_db;
	}
    }

  /* resort argc/argv */
  guint e = 1;
  for (i = 1; i < argc; i++)
    if (argv[i])
      {
        argv[e++] = argv[i];
        if (i >= e)
          argv[i] = NULL;
      }
  *argc_p = e;
}


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

      AlignedArray<float,16> sse_taps (fir_compute_sse_taps (taps));
      for (unsigned int i = 0; i < sse_taps.size(); i++)
	{
	  printf ("%3d", (int) (sse_taps[i] + 0.5));
	  if (i % 4 == 3)
	    printf ("  |");
	  if (i % 16 == 15)
	    printf ("   ||| upper bound = %d\n", (order + 6) / 4);
	}
      printf ("\n\n");

      AlignedArray<float,16> random_mem (order + 4);
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
  const guint	block_size = (TEST == TEST_IMPULSE) ? 150 /* enough space for all possible tests */
						    : options.block_size;
  /* Initialize up- and downsampler via bse.
   *
   * We used to do this ourselves, but accessing the resampler via bse has some advantages;
   *  - it gets rid of coefficient duplication
   *  - it works fine on non-sse machines (bse fails gracefully if SSE can not be detected)
   *
   * The disadvantage of using the factory is:
   *  - we access the actual resampler via virtual methods, which is a slowdown
   *  - we can not provide optimal compiler flags (-funroll-loops -O3 is good for the resampler)
   *    which makes things even more slow
   */
  Resampler2 *ups = Resampler2::create (BSE_RESAMPLER2_MODE_UPSAMPLE, options.precision);
  Resampler2 *downs = Resampler2::create (BSE_RESAMPLER2_MODE_DOWNSAMPLE, options.precision);

  F4Vector in_v[block_size / 2 + 1], out_v[block_size / 2 + 1], out2_v[block_size / 2 + 1];
  float *input = &in_v[0].f[0], *output = &out_v[0].f[0], *output2 = &out2_v[0].f[0]; /* ensure aligned data */

  if (TEST == TEST_PERFORMANCE)
    {
      const gdouble test_frequency = options.frequency;

      for (unsigned int i = 0; i < block_size; i++)
	input[i] = sin (i * test_frequency / 44100.0 * 2 * M_PI);

      double start_time = gettime();
      long long k = 0;
      for (int i = 0; i < 500000; i++)
	{
	  if (RESAMPLE == RES_DOWNSAMPLE || RESAMPLE == RES_SUBSAMPLE)
	    {
	      downs->process_block (input, block_size, output);
	      if (RESAMPLE == RES_SUBSAMPLE)
		ups->process_block (output, block_size / 2, output2);
	    }
	  if (RESAMPLE == RES_UPSAMPLE || RESAMPLE == RES_OVERSAMPLE)
	    {
	      ups->process_block (input, block_size, output);
	      if (RESAMPLE == RES_OVERSAMPLE)
		downs->process_block (output, block_size * 2, output2);
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
      const bool freq_scanning = (options.freq_inc > 1);
      const double freq_min = freq_scanning ? options.freq_min : options.frequency;
      const double freq_max = freq_scanning ? options.freq_max : 1.5 * options.frequency;
      const double freq_inc = freq_scanning ? options.freq_inc : options.frequency;

      if (TEST == TEST_ACCURACY)
	{
	  if (freq_scanning)
	    printf ("#   input frequency range used [ %.2f Hz, %.2f Hz ] (SR = 44100.0 Hz, freq increment = %.2f)\n",
		freq_min, freq_max, freq_inc);
	  else
	    printf ("#   input frequency used to perform test = %.2f Hz (SR = 44100.0 Hz)\n", options.frequency);
	}

      double max_diff = 0;

      /* for getting the last frequency in ranges like [ 50, 18000, 50 ] scanned,
       * even in the presence of rounding errors, we add 1 Hz to the end frequency
       */
      for (double test_frequency = freq_min; test_frequency < (freq_max + 1); test_frequency += freq_inc)
	{
	  long long k = 0;
	  double phase = 0;
	  double test_frequency_max_diff = 0; /* for monitoring frequency scanning */

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
		  downs->process_block (input + misalign, bs, output);
		  if (RESAMPLE == RES_SUBSAMPLE)
		    ups->process_block (output, bs / 2, output2);
		}
	      if (RESAMPLE == RES_UPSAMPLE || RESAMPLE == RES_OVERSAMPLE)
		{
		  ups->process_block (input + misalign, bs, output);
		  if (RESAMPLE == RES_OVERSAMPLE)
		    downs->process_block (output, bs * 2, output2);
		}

	      /* validate output */
	      double sin_shift;
	      double freq_factor;
	      unsigned int out_bs;
	      float *check = output;

	      if (RESAMPLE == RES_UPSAMPLE)
		{
		  sin_shift = ups->order() + 2;		// 16 bits: 34
		  freq_factor = 0.5;
		  out_bs = bs * 2;
		}
	      else if (RESAMPLE == RES_DOWNSAMPLE)
		{
		  sin_shift = (downs->order() + 1) * 0.5;	// 16 bits: 16.5
		  freq_factor = 2;
		  out_bs = bs / 2;
		}
	      else if (RESAMPLE == RES_OVERSAMPLE)
		{
		  sin_shift = ups->order() + 1.5;		// 16 bits: 33.5
		  freq_factor = 1;
		  check = output2;
		  out_bs = bs;
		}
	      else if (RESAMPLE == RES_SUBSAMPLE)		// 16 bits: 67
		{
		  sin_shift = ups->order() * 2 + 3;
		  //printf ("Isshift = %f\n", sin_shift);
		  freq_factor = 1;
		  check = output2;
		  out_bs = bs;
		}

	      for (unsigned int i = 0; i < out_bs; i++, k++)
		if (k > (ups->order() * 4))
		  {
		    double correct_output = sin ((k - sin_shift) * 2 * freq_factor * test_frequency / 44100.0 * M_PI);
		    if (TEST == TEST_GNUPLOT)
		      printf ("%lld %.17f %.17f\n", k, check[i], correct_output);
		    else
		      {
			test_frequency_max_diff = max (test_frequency_max_diff, check[i] - correct_output);
			max_diff = max (max_diff, check[i] - correct_output);
		      }
		  }
	    }
	  double test_frequency_max_diff_db = 20 * log (test_frequency_max_diff) / log (10);
	  if (options.freq_scan_verbose)
	    printf ("%.17f %.17f\n", test_frequency, test_frequency_max_diff_db);
	}
      double max_diff_db = 20 * log (max_diff) / log (10);
      if (TEST == TEST_ACCURACY)
	{
	  printf ("#   max difference between correct and computed output: %f = %f dB\n", max_diff, max_diff_db);
	  g_assert (max_diff_db < options.precision_assert_db);
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
	  downs->process_block (input, block_size, output);
	  if (RESAMPLE == RES_SUBSAMPLE)
	    ups->process_block (output, block_size / 2, output2);
	}
      if (RESAMPLE == RES_UPSAMPLE || RESAMPLE == RES_OVERSAMPLE)
	{
	  ups->process_block (input, block_size, output);
	  if (RESAMPLE == RES_OVERSAMPLE)
	    downs->process_block (output, block_size * 2, output2);
	}

      float *check = output;
      if (RESAMPLE == RES_OVERSAMPLE || RESAMPLE == RES_SUBSAMPLE)
	check = output2;

      for (unsigned int i = 0; i < block_size; i++)
	printf ("%.17f\n", check[i]);
    }
  delete ups;
  delete downs;
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
    case TEST_ACCURACY:	    printf ("# accuracy test "); return perform_test<TEST_ACCURACY> ();
    case TEST_GNUPLOT:	    printf ("# gnuplot test "); return perform_test<TEST_GNUPLOT> ();
    case TEST_IMPULSE:	    printf ("# impulse response test "); return perform_test<TEST_IMPULSE> ();
    case TEST_FILTER_IMPL:  return test_filter_impl();
    default:		    usage(); return 1;
    }
}

int
main (int argc, char **argv)
{
  birnet_init_test (&argc, &argv);
  options.parse (&argc, &argv);

  if (argc == 2)
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
	    default:  g_printerr ("testresampler: unknown mode character '%c'\n", argv[1][i]);
		      exit (1);
	    }
	}
    }
  else if (argc == 1)
    {
      usage();
      return 0;
    }
  else
    {
      g_printerr ("testresampler: too many arguments\n");
      exit (1);
    }

  if (impl_type == IMPL_SSE || test_type == TEST_FILTER_IMPL)
    {
      /* load plugins */
      BirnetInitValue config[] = {
	{ "load-core-plugins", "1" },
	{ NULL },
      };
      bse_init_test (&argc, &argv, config);
      /* check for possible specialization */
      if (Bse::Block::default_singleton() == Bse::Block::current_singleton())
	{
	  fprintf (stderr, "testresampler: bse didn't detect SSE support, so SSE support can not be tested\n");
	  return 0;   /* don't break automated tests on non-SSE machines: return 0 */
	}
    }
  return perform_test();
}
