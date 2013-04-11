// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include <bse/bseresampler.hh>
#include <bse/bsemathsignal.hh>
#include <bse/bsemain.hh>
#include <bse/bseblockutils.hh>
#include <bse/gslfft.hh>
#include <sfi/sfitests.hh>
#include <birnet/birnet.hh>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
using Bse::Resampler::Resampler2;
using Birnet::AlignedArray;
using std::vector;
using std::max;
using std::min;
struct Options
{
  size_t test_size;
  size_t rand_samples;
  // default test parameters
  Options() :
    test_size (512),
    rand_samples (64)
  {
  }
} options;
class ResamplerTest
{
public:
  double passband_err;
  double stopband_err;
  double max_error;
  void  check_spectrum (const vector<float>& impulse_response, int p);
  void  check_resampler_up (BseResampler2Precision precision);
  void  check_resampler_down (BseResampler2Precision precision);
};
void
ResamplerTest::check_spectrum (const vector<float>& impulse_response, int p)
{
  vector<double> fft_in (4096), spect (fft_in.size());
  for (size_t i = 0; i < min (impulse_response.size(), fft_in.size()); i++)
    {
      fft_in[i] = impulse_response[i] / 2;
    }
  gsl_power2_fftar (fft_in.size(), &fft_in[0], &spect[0]);
  spect[1] = 0; // special packing
  passband_err = 0;
  stopband_err = 0;
  for (size_t i = 0; i < 4096; i += 2)
    {
      const double re = spect[i], im = spect[i + 1];
      const double mag = sqrt (re * re + im * im);
      const double freq = i * 44100.0 / 4096;
      if (freq < 18000)
        passband_err = max (passband_err, fabs (1 - mag));
      if (freq > 26100)
        stopband_err = max (stopband_err, fabs (mag));
    }
}
void
ResamplerTest::check_resampler_up (BseResampler2Precision precision)
{
  Resampler2 *ups = Resampler2::create (BSE_RESAMPLER2_MODE_UPSAMPLE, precision);
  AlignedArray<float,16> input (options.test_size);
  AlignedArray<float,16> output (options.test_size * 2);
  vector< vector<float> > results;
  for (size_t i = 0; i < (options.test_size / 2); i++)
    {
      input[i] = 1;
      ups->process_block (&input[0], input.size(), &output[0]);
      input[i] = 0;
      results.push_back (vector<float> (&output[0], &output[output.size()]));
      for (size_t j = 0; j < output.size(); j++)
        {
          if (j >= i * 2)
            {
              assert (output[j] == results[0][j - i * 2]);
            }
          // printf ("%d %.17g #p%d,%d\n", j, output[j], precision, i);
        }
    }
  for (size_t i = 0; i < options.rand_samples; i++)
    input[i] = g_random_double_range (-1, 1);
  ups->process_block (&input[0], input.size(), &output[0]);
  max_error = 0;
  for (size_t j = 0; j < output.size(); j++)
    {
      double acc = 0;
      for (size_t i = 0; i < options.rand_samples; i++)
        {
          acc += results[i][j] * input[i];
        }
      max_error = max (fabs (output[j] - acc), max_error);
    }
  check_spectrum (results[0], precision);
}
void
ResamplerTest::check_resampler_down (BseResampler2Precision precision)
{
  Resampler2 *downs = Resampler2::create (BSE_RESAMPLER2_MODE_DOWNSAMPLE, precision);
  AlignedArray<float,16> input (options.test_size * 2);
  AlignedArray<float,16> output (options.test_size);
  vector< vector<float> > results;
  for (size_t i = 0; i < (options.test_size / 2); i++)
    {
      input[i] = 1;
      downs->process_block (&input[0], input.size(), &output[0]);
      input[i] = 0;
      results.push_back (vector<float> (&output[0], &output[output.size()]));
      for (size_t j = 0; j < output.size(); j++)
        {
          if (j >= i/2)
            assert (output[j] == results[i % 2][j - i/2]);
          //printf ("%zd %.17g #%d,%zd\n", j, output[j], precision, i);
        }
    }
  for (size_t i = 0; i < options.rand_samples; i++)
    input[i] = g_random_double_range (-1, 1);
  downs->process_block (&input[0], input.size(), &output[0]);
  max_error = 0;
  for (size_t j = 0; j < output.size(); j++)
    {
      double acc = 0;
      for (size_t i = 0; i < options.rand_samples; i++)
        {
          acc += results[i][j] * input[i];
        }
      max_error = max (fabs (output[j] - acc), max_error);
    }
  /* The downsampler convolves the input with an FIR filter to achieve a
   * lowpass filter around half the sampling frequency. Since it throws
   * away every second sample, we need to merge the impulse responses
   * for two adjacent impulse responses to figure out the total impulse
   * response of the resampling FIR filter
   */
  vector<float> merged_ir;
  for (size_t i = 0; i < results[0].size(); i++)
    {
      merged_ir.push_back (results[1][i] * 2);
      merged_ir.push_back (results[0][i] * 2);
    }
  check_spectrum (merged_ir, precision);
}
static double
band_err (BseResampler2Precision p)
{
  /* the filter design is not always exactly as specified by the precision,
   * so sometimes we achieve a lower db value than requested, and sometimes
   * a higher db value than specified
   */
  switch (p)
    {
      case BSE_RESAMPLER2_PREC_LINEAR:  return -8.5;
      case BSE_RESAMPLER2_PREC_48DB:    return -51;
      case BSE_RESAMPLER2_PREC_72DB:    return -74;
      case BSE_RESAMPLER2_PREC_96DB:    return -95;
      case BSE_RESAMPLER2_PREC_120DB:   return -120;
      case BSE_RESAMPLER2_PREC_144DB:   return -144;
      default:                          g_assert_not_reached();
    }
}
static void
run_tests (const char *label)
{
  BseResampler2Precision p = BSE_RESAMPLER2_PREC_96DB;  // should not be equal to the first resampler precision
  for (int i = 0; i < 32; i++)
    {
      BseResampler2Precision new_p = Resampler2::find_precision_for_bits (i);
      if (new_p != p)
        {
          p = new_p;
          TSTART ("Resampler %s Precision %d", label, p);
          ResamplerTest rt_up;
          rt_up.check_resampler_up (p);
          TASSERT (bse_db_from_factor (rt_up.max_error, -200) < -125);
          TASSERT (bse_db_from_factor (rt_up.passband_err, -200) < band_err (p));
          TASSERT (bse_db_from_factor (rt_up.stopband_err, -200) < band_err (p));
          //printf ("## UP   %d %.17g %.17g %.17g\n", p, bse_db_from_factor (rt_up.max_error, -200),
                                                    //bse_db_from_factor (rt_up.passband_err, -200),
                                                    //bse_db_from_factor (rt_up.stopband_err, -200));
          ResamplerTest rt_down;
          rt_down.check_resampler_down (p);
          TASSERT (bse_db_from_factor (rt_up.max_error, -200) < -125);
          TASSERT (bse_db_from_factor (rt_up.passband_err, -200) < band_err (p));
          TASSERT (bse_db_from_factor (rt_up.stopband_err, -200) < band_err (p));
          //printf ("## DOWN %d %.17g %.17g %.17g\n", p, bse_db_from_factor (rt_down.max_error, -200),
                                                    //bse_db_from_factor (rt_down.passband_err, -200),
                                                    //bse_db_from_factor (rt_down.stopband_err, -200));
          TDONE();
        }
    }
}
int
main (int argc, char **argv)
{
  sfi_init_test (&argc, &argv, NULL);
  if (argc > 1)
    {
      options.test_size = atoi (argv[1]);
    }
  if (argc > 2)
    {
      options.rand_samples = atoi (argv[2]);
    }
  assert (options.rand_samples <= options.test_size / 2);
  assert (options.test_size >= 128);
  g_print ("Resampler test parameters: test_size=%zd rand_samples=%zd\n",
           options.test_size, options.rand_samples);
  run_tests ("FPU");
  /* load plugins */
  bse_init_test (&argc, argv, Bse::cstrings_to_vector ("load-core-plugins=1", NULL));
  /* check for possible specialization */
  if (Bse::Block::default_singleton() == Bse::Block::current_singleton())
    return 0;   /* nothing changed */
  run_tests ("SSE");
}
