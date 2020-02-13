// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include <bse/bseresampler.hh>
#include <bse/testing.hh>
#include <bse/bsemain.hh>
#include <bse/bsemath.hh>
#include <bse/bsemathsignal.hh>
#include <bse/gslfft.hh>
#include "bse/internal.hh"
#include "testresampler.hh"
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>
#include <time.h>
#include <string>
#include <vector>

using namespace Bse;

using std::string;
using std::vector;
using std::min;
using std::max;
using std::copy;

enum TestType
{
  TEST_NONE,
  TEST_PERFORMANCE,
  TEST_ACCURACY,
  TEST_ERROR_TABLE,
  TEST_ERROR_SPECTRUM,
  TEST_IMPULSE,
  TEST_FILTER_IMPL
};

enum ResampleType
{
  RES_DOWNSAMPLE,
  RES_UPSAMPLE,
  RES_SUBSAMPLE,
  RES_OVERSAMPLE
};

namespace { // Anon

static ResampleType resample_type = RES_UPSAMPLE;
static TestType test_type = TEST_NONE;

struct Options {
  uint                    block_size          = 128;
  double                  frequency           = 440.0;
  double                  freq_min            = -1;
  double                  freq_max            = -1;
  double                  freq_inc            = 0;
  bool                    freq_scan_verbose   = false;
  double                  max_threshold_db    = 0;
  Resampler2::Precision   precision           = Resampler2::PREC_96DB;
  bool                    filter_impl_verbose = false;
  bool                    verbose             = false;
  bool                    use_sse             = false;
  bool                    standalone          = false;
  string                  program_name        = "testresampler";

  void parse (int *argc_p, char **argv_p[]);
} options;

static string verbose_output;
} // Anon

static void
usage ()
{
  printf ("usage: testresampler <command> [ <options>... ]\n");
  printf ("\n");
  printf ("Commands:\n");
  printf ("  perf                  report sine resampling performance\n");
  printf ("  accuracy              compare resampled sine signal against\n");
  printf ("                        ideal output, report time-domain errors\n");
  printf ("  error-table           print sine signals (index, resampled-value, ideal-value,\n");
  printf ("                                            diff-value)\n");
  printf ("  error-spectrum        compare resampled sine signal against ideal output,\n");
  printf ("                        print error spectrum (frequency, error-db)\n");
  printf ("  dirac                 print impulse response (response-value)\n");
  printf ("  filter-impl           tests SSE filter implementation for correctness\n");
  printf ("                        doesn't test anything when running without SSE support\n");
  printf ("\n");
  printf ("Resample options:\n");
  printf ("  --up                  use upsampling by a factor of 2 as resampler [default]\n");
  printf ("  --down                use downsampling by a factor of 2 as resampler\n");
  printf ("  --subsample           perform --down and --up\n");
  printf ("  --oversample          perform --up and --down\n");
  printf ("  --precision=<bits>    choose resampling filter for <bits> precision\n");
  printf ("                        supported precisions: 8, 12, 16, 20, 24 [%d]\n", static_cast<int> (options.precision));
  printf ("  --precision-linear    use linear interpolation (very bad quality)\n");
  printf ("  --fpu                 disables loading of SSE or similarly optimized code\n");
  printf ("\n");
  printf ("Options:\n");
  printf (" --frequency=<freq>     use <freq> as sine test frequency [%f]\n", options.frequency);
  printf (" --block-size=<bs>      use <bs> as resampler block size [%d]\n", options.block_size);
  printf (" --filter-impl-verbose  print reordered coefficients (debugging only)\n");
  printf (" --verbose              enable verbose output for accuracy or filter-impl\n");
  printf ("\n");
  printf ("Accuracy test options:\n");
  printf (" --freq-scan=<fmin>,<fmax>,<finc>\n");
  printf ("                        scan frequency frequency range [<fmin>..<fmax>]\n");
  printf ("                        incrementing frequencies by <finc> after each scan point\n");
  printf (" --freq-scan-verbose    print frequency scanning error table (freq, dB-diff)\n");
  printf (" --max-threshold=<val>  check that the effective precision is at least <val> dB [%f]\n", options.max_threshold_db);
  /*           ::::::::::::::::::::|::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
  printf ("\n");
  printf ("Examples:\n");
  printf ("  # check performance of upsampling with 256 value blocks:\n");
  printf ("  testresampler perf --block-size=256\n");
  printf ("  # check accuracy of upsampling a 440 Hz sine signal:\n");
  printf ("  testresampler accuracy\n");
  printf ("  # check accuracy of downsampling using a 500 Hz frequency:\n");
  printf ("  testresampler accuracy --frequency=500 --block-size=128\n");
  printf ("  # check accuracy of upsampling with a frequency-range and a minimum\n");
  printf ("  # precision, using coefficients designed for 20 bits precision:\n");
  printf ("  testresampler accuracy --precision=20 --freq-scan=50,18000,50 --max-threshold=110 --up\n");
  printf ("  # plot the errors occuring in this frequency scan with gnuplot, including the max threshold\n");
  printf ("  testresampler accuracy --precision=20 --freq-scan=50,18000,50 --freq-scan-verbose --max-threshold=110 --up > x\n");
  printf ("  gnuplot <(echo 'plot [0:][:0] \"x\" with lines, -110; pause -1')\n");

}

static bool
check_arg (uint         argc,
           char        *argv[],
           uint        *nth,
           const char  *opt,              /* for example: --foo */
           const char **opt_arg = NULL)   /* if foo needs an argument, pass a pointer to get the argument */
{
  assert_return (opt != NULL, false);
  assert_return (*nth < argc, false);

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
  _exit (1);
}

void
Options::parse (int   *argc_p,
                char **argv_p[])
{
  guint argc = *argc_p;
  gchar **argv = *argv_p;
  unsigned int i;

  assert_return (argc >= 0);

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
          _exit (0);
        }
      else if (strcmp (argv[i], "--version") == 0 ||
               strcmp (argv[i], "-v") == 0)
        {
          printf ("%s %s\n", program_name.c_str(), Bse::version().c_str());
          _exit (0);
        }
      else if (check_arg (argc, argv, &i, "--block-size", &opt_arg))
	{
	  block_size = atoi (opt_arg);
	  if ((block_size & 1) == 1)
	    {
	      block_size++;
	      printerr ("testresampler: block size needs to be even (fixed: using %d as block size)\n", block_size);
	    }

	  if (block_size < 2)
	    {
	      block_size = 2;
	      printerr ("testresampler: block size needs to be at least 2 (fixed: using %d as block size)\n", block_size);
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
            case 24: precision = Resampler2::Precision (p);
	      break;
	    default: printerr ("testresampler: unsupported precision: %d\n", p);
		     _exit (1);
	    }
	}
      else if (check_arg (argc, argv, &i, "--precision-linear"))
	{
	  precision = Resampler2::PREC_LINEAR;
	}
      else if (check_arg (argc, argv, &i, "--fpu"))
	{
	  use_sse = false;
	}
      else if (check_arg (argc, argv, &i, "--freq-scan", &opt_arg))
	{
	  gchar *oa = g_strdup (opt_arg);
	  gchar *fmin = strtok (oa, ",");
	  gchar *fmax = fmin ? strtok (NULL, ",") : NULL;
	  gchar *finc = fmax ? strtok (NULL, ",") : NULL;

	  if (finc)
	    {
	      freq_min = g_ascii_strtod (fmin, NULL);
	      freq_max = g_ascii_strtod (fmax, NULL);
	      freq_inc = g_ascii_strtod (finc, NULL);
	    }
	  if (freq_inc < 1)
	    {
	      printerr ("testresampler: invalid frequency scanning specification\n");
	      _exit (1);
	    }
	  g_free (oa);
	}
      else if (check_arg (argc, argv, &i, "--freq-scan-verbose"))
	freq_scan_verbose = true;
      else if (check_arg (argc, argv, &i, "--frequency", &opt_arg))
        frequency = g_ascii_strtod (opt_arg, NULL);
      else if (check_arg (argc, argv, &i, "--max-threshold", &opt_arg))
	{
	  max_threshold_db = g_ascii_strtod (opt_arg, NULL);
	  /* we allow both: specifying -96 or 96 to assert 96 dB precision */
	  if (max_threshold_db > 0)
	    max_threshold_db = -max_threshold_db;
	}
      else if (check_arg (argc, argv, &i, "--filter-impl-verbose"))
	filter_impl_verbose = true;
      else if (check_arg (argc, argv, &i, "--verbose"))
	verbose = true;
      else if (check_arg (argc, argv, &i, "--up"))
        resample_type = RES_UPSAMPLE;
      else if (check_arg (argc, argv, &i, "--down"))
        resample_type = RES_DOWNSAMPLE;
      else if (check_arg (argc, argv, &i, "--subsample"))
        resample_type = RES_SUBSAMPLE;
      else if (check_arg (argc, argv, &i, "--oversample"))
        resample_type = RES_OVERSAMPLE;
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

static int
test_filter_impl()
{
  bool filter_ok = Resampler2::test_filter_impl (options.filter_impl_verbose);

  if (filter_ok)
    verbose_output += "filter implementation ok.\n";
  else
    verbose_output += "errors detected in filter implementation.\n";

  return filter_ok ? 0 : 1;
}

static double
gettime ()
{
  timeval tv;
  gettimeofday (&tv, 0);

  return tv.tv_sec + tv.tv_usec / 1000000.0;
}

template <int TEST, int RESAMPLE> int
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
  Resampler2 ups (Resampler2::UP, options.precision, options.use_sse);
  Resampler2 downs (Resampler2::DOWN, options.precision, options.use_sse);

  TASSERT (options.use_sse == ups.sse_enabled());
  TASSERT (options.use_sse == downs.sse_enabled());

  FastMemArray<float, 16> in_a (block_size * 2), out_a (block_size * 2), out2_a (block_size * 2);
  float *input = &in_a[0], *output = &out_a[0], *output2 = &out2_a[0]; /* ensure aligned data */

  if (TEST == TEST_PERFORMANCE)
    {
      const int REPETITIONS = (options.standalone ? 64'000'000 : 640'000) / block_size;
      const gdouble test_frequency = options.frequency;

      for (unsigned int i = 0; i < block_size; i++)
	input[i] = sin (i * test_frequency / 44100.0 * 2 * M_PI);

      double start_time = gettime();
      long long k = 0;
      for (int i = 0; i < REPETITIONS; i++)
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
	  verbose_output += "  (performance will be normalized to downsampler output samples)\n";
	  k /= 2;
	}
      else if (RESAMPLE == RES_UPSAMPLE)
	{
	  verbose_output += "  (performance will be normalized to upsampler input samples)\n";
	}
      verbose_output += string_format ("  total samples processed = %lld\n", k);
      verbose_output += string_format ("  processing_time = %f\n", end_time - start_time);
      verbose_output += string_format ("  samples / second = %f\n", k / (end_time - start_time));
      verbose_output += string_format ("  which means the resampler can process %.2f 44100 Hz streams simultaneusly\n",
	                               k / (end_time - start_time) / 44100.0);
      verbose_output += string_format ("  or one 44100 Hz stream takes %f %% CPU usage\n",
	                               100.0 / (k / (end_time - start_time) / 44100.0));
    }
  else if (TEST == TEST_ACCURACY || TEST == TEST_ERROR_TABLE || TEST == TEST_ERROR_SPECTRUM)
    {
      const bool freq_scanning = (options.freq_inc > 1);
      const double freq_min = freq_scanning ? options.freq_min : options.frequency;
      const double freq_max = freq_scanning ? options.freq_max : 1.5 * options.frequency;
      const double freq_inc = freq_scanning ? options.freq_inc : options.frequency;
      vector<double> error_spectrum_error;

      if (TEST == TEST_ACCURACY)
	{
	  if (freq_scanning)
	    verbose_output += string_format ("#   input frequency range used [ %.2f Hz, %.2f Hz ] (SR = 44100.0 Hz, freq increment = %.2f)\n",
		freq_min, freq_max, freq_inc);
	  else
	    verbose_output += string_format ("#   input frequency used to perform test = %.2f Hz (SR = 44100.0 Hz)\n", options.frequency);
	}

      double max_diff = 0;

      /* for getting the last frequency in ranges like [ 50, 18000, 50 ] scanned,
       * even in the presence of rounding errors, we add 1 Hz to the end frequency
       */
      for (double test_frequency = freq_min; test_frequency < (freq_max + 1); test_frequency += freq_inc)
	{
	  long long k = 0;
	  double phase = 0, output_phase = 0;
	  double test_frequency_max_diff = 0; /* for monitoring frequency scanning */

	  while (k < 20000)
	    {
	      guint misalign = rand() % 4;
              if (block_size <= misalign)
                continue;

	      int bs = rand() % (block_size + 1 - misalign);
              if (bs < 2)
                continue;

	      if (RESAMPLE == RES_DOWNSAMPLE || RESAMPLE == RES_SUBSAMPLE)
		bs -= bs & 1;

	      for (int i = 0; i < bs; i++)
		{
		  input[i+misalign] = sin (phase);
		  phase += test_frequency/44100.0 * 2 * M_PI;
                  while (phase > 2 * M_PI)
                    phase -= 2 * M_PI;
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
	      double correct_volume;
	      unsigned int out_bs;
	      float *check = output;

	      if (RESAMPLE == RES_UPSAMPLE)
		{
		  sin_shift = ups.delay();
		  freq_factor = 0.5;
		  out_bs = bs * 2;
		  correct_volume = 1;
		}
	      else if (RESAMPLE == RES_DOWNSAMPLE)
		{
		  sin_shift = downs.delay();
		  freq_factor = 2;
		  out_bs = bs / 2;
		  correct_volume = (test_frequency < (44100/4)) ? 1 : 0;
		}
	      else if (RESAMPLE == RES_OVERSAMPLE)
		{
		  sin_shift = ups.delay() * 0.5 + downs.delay();
		  freq_factor = 1;
		  check = output2;
		  out_bs = bs;
		  correct_volume = 1;
		}
	      else if (RESAMPLE == RES_SUBSAMPLE)
		{
		  sin_shift = downs.delay() * 2 + ups.delay();
		  freq_factor = 1;
		  check = output2;
		  out_bs = bs;
		  correct_volume = (test_frequency < (44100/4)) ? 1 : 0;
		}

	      for (unsigned int i = 0; i < out_bs; i++, k++)
		{
		  if (k > (ups.order() * 4))
		    {
		      /* The expected resampler output signal is a sine signal with
		       * different frequency and is phase shifted a bit.
		       */
		      double correct_output = sin (output_phase - sin_shift * 2 * freq_factor * test_frequency / 44100.0 * M_PI);

		      /* For some frequencies the expected output signal amplitude is
		       * zero, because it needed to be filtered out; this fact is
		       * taken into account here.
		       */
		      correct_output *= correct_volume;
		      if (TEST == TEST_ERROR_TABLE)
			{
			  verbose_output += string_format ("%lld %.17f %.17f %.17f\n", k, check[i], correct_output, correct_output - check[i]);
			}
		      else if (TEST == TEST_ERROR_SPECTRUM)
			{
			  error_spectrum_error.push_back (correct_output - check[i]);
			}
		      else
			{
			  test_frequency_max_diff = max (test_frequency_max_diff, check[i] - correct_output);
			  max_diff = max (max_diff, check[i] - correct_output);
			}
		    }
		  output_phase += freq_factor * test_frequency / 44100.0 * 2 * M_PI;
		  while (output_phase > 2 * M_PI)
		    output_phase -= 2 * M_PI;
		}
	    }
	  double test_frequency_max_diff_db = 20 * log (test_frequency_max_diff) / log (10);
	  if (options.freq_scan_verbose)
	    verbose_output += string_format ("%.17f %.17f\n", test_frequency, test_frequency_max_diff_db);
	}
      double max_diff_db = 20 * log (max_diff) / log (10);
      if (TEST == TEST_ACCURACY)
	{
	  verbose_output += string_format ("#   max difference between correct and computed output: %f = %f dB\n", max_diff, max_diff_db);
	  if (options.max_threshold_db < 0)
	    verbose_output += string_format ("#                             (threshold given by user: %f dB)\n", options.max_threshold_db);
	  if (max_diff_db > options.max_threshold_db)
            {
              verbose_output += string_format ("# TEST FAILED: accuracy not below threshold\n");
              return 1;
            }
	}
      else if (TEST == TEST_ERROR_SPECTRUM)
	{
	  const guint FFT_SIZE = 16384;
	  if (error_spectrum_error.size() < FFT_SIZE)
	    {
	      printerr ("too few values for computing error spectrum, increase block size\n");
	    }
	  else
	    {
	      double fft_error[FFT_SIZE];
	      double normalize = 0;

	      for (guint i = 0; i < FFT_SIZE; i++)
		{
		  double w = bse_window_blackman ((2.0 * i - FFT_SIZE) / FFT_SIZE);

		  normalize += w;
		  error_spectrum_error[i] *= w;
		}
	      /* Normalization 1: until here, we know how the multiplication
	       * with the window lessened the volume (energy) of the error signal.
	       */
	      normalize /= FFT_SIZE;
	      gsl_power2_fftar (FFT_SIZE, &error_spectrum_error[0], fft_error);
	      fft_error[1] = 0; // we don't process the extra value at FS/2

	      /* Normalization 2: the FFT produces FFT_SIZE/2 complex output values.
	       */
	      normalize *= (FFT_SIZE / 2);

	      double freq_scale = 1; /* subsample + oversample */
	      if (RESAMPLE == RES_UPSAMPLE)
		freq_scale = 2;
	      else if (RESAMPLE == RES_DOWNSAMPLE)
		freq_scale = 0.5;

	      for (guint i = 0; i < FFT_SIZE/2; i++)
		{
		  double normalized_error = bse_complex_abs (bse_complex (fft_error[i * 2], fft_error[i * 2 + 1])) / normalize;
		  double normalized_error_db = 20 * log (normalized_error) / log (10);

		  verbose_output += string_format ("%f %f\n", i / double (FFT_SIZE) * 44100 * freq_scale, normalized_error_db);
		}
	    }
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
	verbose_output += string_format ("%.17f\n", check[i]);
    }
  return 0;
}

template <int TEST> int
perform_test()
{
  const char *instruction_set = options.use_sse ? "SSE" : "FPU";

  switch (resample_type)
    {
    case RES_DOWNSAMPLE:  verbose_output += string_format ("for factor 2 downsampling using %s instructions\n", instruction_set);
			  return perform_test<TEST, RES_DOWNSAMPLE> ();
    case RES_UPSAMPLE:	  verbose_output += string_format ("for factor 2 upsampling using %s instructions\n", instruction_set);
			  return perform_test<TEST, RES_UPSAMPLE> ();
    case RES_SUBSAMPLE:	  verbose_output += string_format ("for factor 2 subsampling using %s instructions\n", instruction_set);
			  return perform_test<TEST, RES_SUBSAMPLE> ();
    case RES_OVERSAMPLE:  verbose_output += string_format ("for factor 2 oversampling using %s instructions\n", instruction_set);
			  return perform_test<TEST, RES_OVERSAMPLE> ();
    default:		  usage();
			  return 1;
    }
}

static int
perform_test()
{
  verbose_output = "";
  switch (test_type)
    {
    case TEST_PERFORMANCE:    verbose_output += "performance test "; return perform_test<TEST_PERFORMANCE> ();
    case TEST_ACCURACY:	      verbose_output += "# accuracy test "; return perform_test<TEST_ACCURACY> ();
    case TEST_ERROR_TABLE:    verbose_output += "# error table test "; return perform_test<TEST_ERROR_TABLE> ();
    case TEST_ERROR_SPECTRUM: verbose_output += "# error spectrum test "; return perform_test<TEST_ERROR_SPECTRUM> ();
    case TEST_IMPULSE:	      verbose_output += "# impulse response test "; return perform_test<TEST_IMPULSE> ();
    case TEST_FILTER_IMPL:    return test_filter_impl();
    default:		      usage(); return 1;
    }
}

static string
test_title()
{
  if (test_type == TEST_FILTER_IMPL)
    {
      return "testresampler filter implementation";
    }
  else
    {
      assert_return (test_type == TEST_ACCURACY, "*bad test type*");

      const char *instruction_set = options.use_sse ? "SSE" : "FPU";
      const char *rname = "*bad resample name*";
      switch (resample_type)
        {
          case RES_UPSAMPLE:    rname = "up  "; break;
          case RES_DOWNSAMPLE:  rname = "down"; break;
          case RES_SUBSAMPLE:   rname = "sub "; break;
          case RES_OVERSAMPLE:  rname = "over"; break;
        }
      return string_format ("testresampler accuracy/%s/%s %2d bit", instruction_set, rname, options.precision);
    }
}

static int standalone (int argc, char **argv) __attribute__((unused));

static int
standalone (int argc, char **argv)
{
  options.use_sse = Resampler2::sse_available();
  options.parse (&argc, &argv);

  if (argc == 2)
    {
      string command = argv[1];
      if (command == "perf" || command == "performance")
	test_type = TEST_PERFORMANCE;
      else if (command == "accuracy")
	test_type = TEST_ACCURACY;
      else if (command == "error-table")
	test_type = TEST_ERROR_TABLE;
      else if (command == "error-spectrum")
	test_type = TEST_ERROR_SPECTRUM;
      else if (command == "dirac")
	test_type = TEST_IMPULSE;
      else if (command == "filter-impl")
	test_type = TEST_FILTER_IMPL;
      else
	{
	  printerr ("testresampler: unknown mode command: '%s'\n", command.c_str());
	  _exit (1);
	}
    }
  else if (argc == 1)
    {
      usage();
      return 0;
    }
  else
    {
      printerr ("testresampler: too many arguments\n");
      _exit (1);
    }

  int result = perform_test();

  // tests that report pass or fail
  if (test_type == TEST_ACCURACY || test_type == TEST_FILTER_IMPL)
    {
      if (options.verbose)
        {
          printf ("%s", verbose_output.c_str());
        }
      else // summarize test result
        {
          if (result == 0)
            {
              printf ("  PASS     %s\n", test_title().c_str());
            }
          else
            {
              printf ("%s\n", verbose_output.c_str());
              printf ("  FAIL     %s\n", test_title().c_str());
            }
        }
    }
  else // always be verbose on the other tests
    {
      printf ("%s", verbose_output.c_str());
    }
  return result;
}

// == test collection ==
static void
run_testresampler (TestType tt)
{
  test_type = tt;
  const int result = perform_test();
  TASSERT (result == 0);
}

static bool
run_accuracy (ResampleType rtype, bool use_sse_if_available, int bits, double fmin, double fmax, double finc, double threshold)
{
  test_type = TEST_ACCURACY;
  resample_type = rtype;
  options.precision = Resampler2::find_precision_for_bits (bits);
  options.freq_min = fmin;
  options.freq_max = fmax;
  options.freq_inc = finc;
  options.max_threshold_db = threshold;
  /* we allow both: specifying -96 or 96 to assert 96 dB precision */
  if (options.max_threshold_db > 0)
    options.max_threshold_db = -options.max_threshold_db;
  //options.verbose = true;
  options.use_sse = Resampler2::sse_available() && use_sse_if_available;
  const int result = perform_test();
  if (options.verbose)
    printf ("%s", verbose_output.c_str());
  return result == 0;
}

static void
run_perf (ResampleType rtype, int bits)
{
  const int runs = Resampler2::sse_available() ? 2 : 1; // run test twice if we have both: FPU and SSE support

  for (int r = 0; r < runs; r++)
    {
      test_type = TEST_PERFORMANCE;
      resample_type = rtype;
      options.precision = Resampler2::find_precision_for_bits (bits);
      options.verbose = true;
      options.use_sse = r;

      const int result = perform_test();
      if (options.verbose)
        printf ("%s", verbose_output.c_str());
      TASSERT (result == 0);
    }
}

static void testresampler_check_filter_impl()           { run_testresampler (TEST_FILTER_IMPL); }
TEST_ADD (testresampler_check_filter_impl);

static void testresampler_check_precision_up8()         { TASSERT (run_accuracy (RES_UPSAMPLE, true, 8, 180, 18000, 1979, 45)); }
TEST_ADD (testresampler_check_precision_up8);
static void testresampler_check_precision_down12()      { TASSERT (run_accuracy (RES_DOWNSAMPLE, true, 12, 90, 9000, 997, 72)); }
TEST_ADD (testresampler_check_precision_down12);
static void testresampler_check_precision_up16()        { TASSERT (run_accuracy (RES_UPSAMPLE, true, 16, 180, 18000, 1453, 89.5)); }
TEST_ADD (testresampler_check_precision_up16);
static void testresampler_check_precision_over20()      { TASSERT (run_accuracy (RES_OVERSAMPLE, false, 20, 180, 18000, 1671, 113.5)); }
TEST_ADD (testresampler_check_precision_over20);
static void testresampler_check_precision_sub24()       { TASSERT (run_accuracy (RES_SUBSAMPLE, false, 24, 90, 9000, 983, 124.5)); }
TEST_ADD (testresampler_check_precision_sub24);

static void
testresampler_check_accuracy_full()
{
  if (Resampler2::sse_available())
    {
      // SSE upsampler tests
      TASSERT (run_accuracy (RES_UPSAMPLE, true, 8,  50, 18000, 50, 45));     // ideally: 48dB
      TASSERT (run_accuracy (RES_UPSAMPLE, true, 12, 50, 18000, 50, 66.5));   // ideally: 72dB
      TASSERT (run_accuracy (RES_UPSAMPLE, true, 16, 50, 18000, 50, 89));     // ideally: 96dB
      TASSERT (run_accuracy (RES_UPSAMPLE, true, 20, 50, 18000, 50, 113.5));  // ideally: 120dB
      TASSERT (run_accuracy (RES_UPSAMPLE, true, 24, 50, 18000, 50, 126));    // ideally: 144dB
      // SSE downsampler tests
      TASSERT (run_accuracy (RES_DOWNSAMPLE, true, 8,  25, 9000, 25, 51));    // ideally: 48dB
      TASSERT (run_accuracy (RES_DOWNSAMPLE, true, 12, 25, 9000, 25, 72));    // ideally: 72dB
      TASSERT (run_accuracy (RES_DOWNSAMPLE, true, 16, 25, 9000, 25, 95));    // ideally: 96dB
      TASSERT (run_accuracy (RES_DOWNSAMPLE, true, 20, 25, 9000, 25, 119.5)); // ideally: 120dB
      TASSERT (run_accuracy (RES_DOWNSAMPLE, true, 24, 25, 9000, 25, 131));   // ideally: 144dB
    }
  // FPU upsampler tests
  TASSERT (run_accuracy (RES_UPSAMPLE, false, 8,  50, 18000, 50, 45));     // ideally: 48dB
  TASSERT (run_accuracy (RES_UPSAMPLE, false, 12, 50, 18000, 50, 66.5));   // ideally: 72dB
  TASSERT (run_accuracy (RES_UPSAMPLE, false, 16, 50, 18000, 50, 89));     // ideally: 96dB
  TASSERT (run_accuracy (RES_UPSAMPLE, false, 20, 50, 18000, 50, 113.5));  // ideally: 120dB
  TASSERT (run_accuracy (RES_UPSAMPLE, false, 24, 50, 18000, 50, 126));    // ideally: 144dB
  // FPU downsampler tests
  TASSERT (run_accuracy (RES_DOWNSAMPLE, false, 8,  25, 9000, 25, 51));    // ideally: 48dB
  TASSERT (run_accuracy (RES_DOWNSAMPLE, false, 12, 25, 9000, 25, 72));    // ideally: 72dB
  TASSERT (run_accuracy (RES_DOWNSAMPLE, false, 16, 25, 9000, 25, 95));    // ideally: 96dB
  TASSERT (run_accuracy (RES_DOWNSAMPLE, false, 20, 25, 9000, 25, 119.5)); // ideally: 120dB
  TASSERT (run_accuracy (RES_DOWNSAMPLE, false, 24, 25, 9000, 25, 131));   // ideally: 144dB
  // sparse testing of sub- and oversampling (we don't test every combination of
  // flags here, but this is also an uncommon usage scenario)
  if (Resampler2::sse_available())
    {
      TASSERT (run_accuracy (RES_OVERSAMPLE, true, 8,  50, 18000, 50, 45));   // ideally: 48dB
      TASSERT (run_accuracy (RES_OVERSAMPLE, true, 16, 50, 18000, 50, 89));   // ideally: 96dB
      TASSERT (run_accuracy (RES_SUBSAMPLE,  true, 16, 25,  9000, 25, 85.5)); // ideally: 96dB
    }
  TASSERT (run_accuracy (RES_OVERSAMPLE, false, 16, 50, 18000, 50, 89));   // ideally: 96dB
  TASSERT (run_accuracy (RES_SUBSAMPLE,  false, 16, 25,  9000, 25, 85.5)); // ideally: 96dB
}
TEST_SLOW (testresampler_check_accuracy_full);

#if 0 // lengthy and verbose performance tests
#define TEST_PERF(fun)  TEST_BENCH (fun)
#else
struct DummyFunc { explicit DummyFunc (std::function<void()>) {} };
#define TEST_PERF(fun)  static const DummyFunc BSE_CPP_PASTE2 (__dummy, __LINE__) (fun);
#endif

static void testresampler_check_performance_up8()       { run_perf (RES_UPSAMPLE, 8); }
TEST_PERF (testresampler_check_performance_up8);
static void testresampler_check_performance_down8()     { run_perf (RES_DOWNSAMPLE, 8); }
TEST_PERF (testresampler_check_performance_down8);
static void testresampler_check_performance_sub8()      { run_perf (RES_SUBSAMPLE, 8); }
TEST_PERF (testresampler_check_performance_sub8);
static void testresampler_check_performance_over8()     { run_perf (RES_OVERSAMPLE, 8); }
TEST_PERF (testresampler_check_performance_over8);

static void testresampler_check_performance_up12()      { run_perf (RES_UPSAMPLE, 12); }
TEST_PERF (testresampler_check_performance_up12);
static void testresampler_check_performance_down12()    { run_perf (RES_DOWNSAMPLE, 12); }
TEST_PERF (testresampler_check_performance_down12);
static void testresampler_check_performance_sub12()     { run_perf (RES_SUBSAMPLE, 12); }
TEST_PERF (testresampler_check_performance_sub12);
static void testresampler_check_performance_over12()    { run_perf (RES_OVERSAMPLE, 12); }
TEST_PERF (testresampler_check_performance_over12);

static void testresampler_check_performance_up16()      { run_perf (RES_UPSAMPLE, 16); }
TEST_PERF (testresampler_check_performance_up16);
static void testresampler_check_performance_down16()    { run_perf (RES_DOWNSAMPLE, 16); }
TEST_PERF (testresampler_check_performance_down16);
static void testresampler_check_performance_sub16()     { run_perf (RES_SUBSAMPLE, 16); }
TEST_PERF (testresampler_check_performance_sub16);
static void testresampler_check_performance_over16()    { run_perf (RES_OVERSAMPLE, 16); }
TEST_PERF (testresampler_check_performance_over16);

static void testresampler_check_performance_up20()      { run_perf (RES_UPSAMPLE, 20); }
TEST_PERF (testresampler_check_performance_up20);
static void testresampler_check_performance_down20()    { run_perf (RES_DOWNSAMPLE, 20); }
TEST_PERF (testresampler_check_performance_down20);
static void testresampler_check_performance_sub20()     { run_perf (RES_SUBSAMPLE, 20); }
TEST_PERF (testresampler_check_performance_sub20);
static void testresampler_check_performance_over20()    { run_perf (RES_OVERSAMPLE, 20); }
TEST_PERF (testresampler_check_performance_over20);

static void testresampler_check_performance_up24()      { run_perf (RES_UPSAMPLE, 24); }
TEST_PERF (testresampler_check_performance_up24);
static void testresampler_check_performance_down24()    { run_perf (RES_DOWNSAMPLE, 24); }
TEST_PERF (testresampler_check_performance_down24);
static void testresampler_check_performance_sub24()     { run_perf (RES_SUBSAMPLE, 24); }
TEST_PERF (testresampler_check_performance_sub24);
static void testresampler_check_performance_over24()    { run_perf (RES_OVERSAMPLE, 24); }
TEST_PERF (testresampler_check_performance_over24);

int
test_resampler (int argc, char **argv)
{
  options.standalone = true;

  char first_argv[] = "testresampler";
  vector<char *> standalone_argv { first_argv };
  for (int i = 2; i < argc; i++)
    standalone_argv.push_back (argv[i]);

  return standalone (standalone_argv.size(), standalone_argv.data());
}
