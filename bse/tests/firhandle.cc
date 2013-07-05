// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include <bse/bsedefs.hh>
// #define TEST_VERBOSE
#include <sfi/sfitests.hh>
#include <bse/gsldatahandle.hh>
#include <bse/gsldatautils.hh>
#include <bse/bsemain.hh>
#include "topconfig.h"
#include <math.h>
#include <stdlib.h>
#include <complex>
#include <vector>

using Rapicorn::string_format;
using std::vector;
using std::min;
using std::max;

static void
read_through (GslDataHandle *handle)
{
  int64 n_values = gsl_data_handle_n_values (handle);
  int64 offset = 0;
  while (offset < n_values)
    {
      // we don't use 1024 here, because we know that it is the FIR handle internal buffer size
      gfloat values[700];
      int64 values_read = gsl_data_handle_read (handle, offset, 700, values);
      g_assert (values_read > 0);
      offset += values_read;
    }

  g_assert (offset == n_values);
}

static double
phase_diff (double p1,
            double p2)
{
  double diff = p1 - p2;

  // normalize to range [-pi..pi]
  while (diff > M_PI)
    diff -= 2 * M_PI;

  while (diff < -M_PI)
    diff += 2 * M_PI;

  return diff;
}

static double
band_min (const vector<double>& scanned_freq,
          const vector<double>& scanned_values,
	  double                start_freq,
	  double                end_freq)
{
  g_assert (scanned_freq.size() == scanned_values.size());

  bool	  init = false;
  double  min_value = 1e19;
  for (size_t i = 0; i < scanned_values.size(); i++)
    {
      if (scanned_freq[i] >= start_freq && scanned_freq[i] <= end_freq)
	{
	  if (init)
	    min_value = min (scanned_values[i], min_value);
	  else
	    {
	      min_value = scanned_values[i];
	      init = true;
	    }
	}
    }
  g_assert (init);
  return min_value;
}

static double
band_max (const vector<double>& scanned_freq,
          const vector<double>& scanned_values,
	  double                start_freq,
	  double                end_freq)
{
  g_assert (scanned_freq.size() == scanned_values.size());

  bool	  init = false;
  double  max_value = -1e19;
  for (size_t i = 0; i < scanned_values.size(); i++)
    {
      if (scanned_freq[i] >= start_freq && scanned_freq[i] <= end_freq)
	{
	  if (init)
	    max_value = max (scanned_values[i], max_value);
	  else
	    {
	      max_value = scanned_values[i];
	      init = true;
	    }
	}
    }
  g_assert (init);
  return max_value;
}

enum FirHandleType
{
  FIR_HIGHPASS,
  FIR_LOWPASS
};

static const char*
handle_name (FirHandleType type)
{
  switch (type)
    {
      case FIR_HIGHPASS:  return "Highpass";
      case FIR_LOWPASS:	  return "Lowpass";
      default:		  g_assert_not_reached();
    }
}

static void
test_with_sine_sweep (FirHandleType type)
{
  TSTART ("%s Handle (sweep)", handle_name (type));
  vector<float> sweep_sin (50000);
  vector<float> sweep_cos (50000);
  vector<double> sweep_freq (50000);

  const double start_freq = 50;
  const double end_freq = 24000;
  const double mix_freq = 48000;
  const double octaves = log (end_freq / start_freq) / log (2);

  double phase = 0; 

  for (size_t i = 0; i < sweep_sin.size(); i++)
    {
      sweep_sin[i] = sin (phase);
      sweep_cos[i] = cos (phase);
      sweep_freq[i] = pow (2.0, (i * octaves) / sweep_sin.size()) * start_freq;
      phase += sweep_freq[i] / mix_freq * 2.0 * M_PI;
      if (phase > 2.0 * M_PI)
	phase -= 2.0 * M_PI;
    }

  GslDataHandle *ihandle_sin = gsl_data_handle_new_mem (1, 32, mix_freq, 440, sweep_sin.size(), &sweep_sin[0], NULL);
  GslDataHandle *ihandle_cos = gsl_data_handle_new_mem (1, 32, mix_freq, 440, sweep_cos.size(), &sweep_cos[0], NULL);

  const int order = 64;
  GslDataHandle *fir_handle_sin = NULL;
  GslDataHandle *fir_handle_cos = NULL;

  if (type == FIR_HIGHPASS)
    {
      fir_handle_sin = bse_data_handle_new_fir_highpass (ihandle_sin, 9000.0, order);
      fir_handle_cos = bse_data_handle_new_fir_highpass (ihandle_cos, 9000.0, order);
    }
  else if (type == FIR_LOWPASS)
    {
      fir_handle_sin = bse_data_handle_new_fir_lowpass (ihandle_sin, 6000.0, order);
      fir_handle_cos = bse_data_handle_new_fir_lowpass (ihandle_cos, 6000.0, order);
    }

  BseErrorType error;
  error = gsl_data_handle_open (fir_handle_sin);
  TASSERT (error == 0);
  error = gsl_data_handle_open (fir_handle_cos);
  TASSERT (error == 0);

  GslDataPeekBuffer peek_buffer_sin = { +1 /* incremental direction */, 0, };
  GslDataPeekBuffer peek_buffer_cos = { +1 /* incremental direction */, 0, };

  vector<double> scanned_freq, scanned_level_db, scanned_abs_phase_diff;

  for (size_t i = ((order + 2) / 2); i < sweep_sin.size() - ((order + 2) / 2); i++)
    {
      double filtered_sin = gsl_data_handle_peek_value (fir_handle_sin, i, &peek_buffer_sin);
      double filtered_cos = gsl_data_handle_peek_value (fir_handle_cos, i, &peek_buffer_cos);
      std::complex<double> filtered (filtered_sin, filtered_cos);

      // compute frequency response
      double level = abs (filtered);
      double level_db = bse_db_from_factor (level, -200);
      scanned_freq.push_back (sweep_freq[i]);
      scanned_level_db.push_back (level_db);
      // printf ("%f %.17g\n", sweep_freq[i], scanned_level_db.back());

      if ((i & 15) == 0)
	{
	  // check that theoretical and scanned response match
	  double theoretical_level_db = bse_data_handle_fir_response_db (fir_handle_sin, sweep_freq[i]);
	  double theoretical_level = bse_db_to_factor (theoretical_level_db);
	  // printf ("%g %.17g\n", sweep_freq[i], fabs (level - theoretical_level));
	  TCMP (fabs (level - theoretical_level), <, 0.00035);
	}
      // compute phase response
      std::complex<double> orig (sweep_sin[i], sweep_cos[i]);
      scanned_abs_phase_diff.push_back (fabs (phase_diff (arg (orig), arg (filtered))));
      // printf ("%f %.17g\n", sweep_freq[i], scanned_abs_phase_diff.back());
    }
  if (type == FIR_HIGHPASS)
    {
      // stop band
      TCMP (band_max (scanned_freq, scanned_level_db,     0,  7050), <, -75);
      // transition band
      TCMP (band_min (scanned_freq, scanned_level_db,  7050,  9500), >, -77);
      TCMP (band_max (scanned_freq, scanned_level_db,  7050,  9500), <, -2.8);
      // passband (1)
      TCMP (band_min (scanned_freq, scanned_level_db,  9500, 11000), >, -2.82);
      TCMP (band_max (scanned_freq, scanned_level_db,  9500, 11000), <, -0.002);
      // passband (2)
      TCMP (band_min (scanned_freq, scanned_level_db, 11000, 24000), >, -0.004);
      TCMP (band_max (scanned_freq, scanned_level_db, 11000, 24000), <, 0.002);
      // zero phase in passband (2)
      TCMP (band_max (scanned_freq, scanned_abs_phase_diff, 11000, 24000), <, 0.0002);
    }
  else	// FIR_LOWPASS
    {
      // passband (2)
      TCMP (band_min (scanned_freq, scanned_level_db,     0,  5500), >, -0.002);
      TCMP (band_max (scanned_freq, scanned_level_db,     0,  5500), <, 0.002);
      // passband (1)
      TCMP (band_min (scanned_freq, scanned_level_db,  5500,  7000), >, -1.9);
      TCMP (band_max (scanned_freq, scanned_level_db,  5500,  7000), <, -0.001);
      // transition band
      TCMP (band_min (scanned_freq, scanned_level_db,  7000, 10000), >, -81);
      TCMP (band_max (scanned_freq, scanned_level_db,  7000, 10000), <, -1.8);
      // stop band
      TCMP (band_max (scanned_freq, scanned_level_db, 10000, 24000), <, -75);
      // zero phase in passband (2)
      TCMP (band_max (scanned_freq, scanned_abs_phase_diff, 0, 5500), <, 0.00002);
    }
  TDONE();
  /* test speed */
  double samples_per_second = 0;
  if (Rapicorn::Test::slow())
    {
      const guint RUNS = 10;
      GTimer *timer = g_timer_new();
      const guint dups = TEST_CALIBRATION (50.0, read_through (fir_handle_sin));

      double m = 9e300;
      for (guint i = 0; i < RUNS; i++)
        {
          g_timer_start (timer);
          for (guint j = 0; j < dups; j++)
            read_through (fir_handle_sin);
          g_timer_stop (timer);
          double e = g_timer_elapsed (timer, NULL);
          if (e < m)
            m = e;
        }
      samples_per_second = sweep_sin.size() / (m / dups);
      TMSG ("    %-28s : %+.14f samples/second",
            string_format ("%s O64 mono", handle_name (type)).c_str(),
            samples_per_second);
      TMSG ("    %-28s : %+.14f streams",
            string_format ("CPU %s mono", handle_name (type)).c_str(),
            samples_per_second / 44100.0);
    }
}

static double
raised_cosine_fade (int64 pos,
		    int64 length,
		    int64 fade_length)
{
  double fade_delta  = 1.0 / fade_length;
  double fade_factor = fade_delta * min (pos, length - pos);
  if (fade_factor >= 1.0)
    return 1.0;
  else
    return (0.5 - cos (fade_factor * PI) * 0.5);
}

static void
test_multi_channel (FirHandleType type)
{
  TSTART ("%s Handle (multichannel)", handle_name (type));
  for (int n_channels = 1; n_channels <= 10; n_channels++)
    {
      const double    mix_freq = 48000;
      const double    cutoff_freq = 7500;
      const double    test_freqs[] = {
	50, 100, 234.567, 557, 901, 1350, 1780, 2345, 3745, 4500,	     // below cutoff
	11000, 12000, 13945, 14753, 15934, 16734, 17943, 18930, 19320, 20940 // above cutoff
      };
      vector<float>   input (2500 * n_channels);
      vector<double>  expected (input.size());
      vector<double>  freq (n_channels);
      vector<double>  phase (n_channels);

      for (int c = 0; c < n_channels; c++)
	freq[c] = test_freqs [g_random_int_range (0, sizeof (test_freqs) / sizeof (test_freqs[0]))];

      for (size_t i = 0; i < input.size(); i++)
	{
	  const int	c           = i % n_channels;
	  const double  fade_factor = raised_cosine_fade (i / n_channels, input.size() / n_channels, 500);
	  const double  invalue     = sin (phase[c]) * fade_factor;

	  input[i] = invalue;
	  if ((freq[c] > cutoff_freq && type == FIR_HIGHPASS) || (freq[c] < cutoff_freq && type == FIR_LOWPASS))
	    expected[i] = invalue;

	  phase[c] += freq[c] / mix_freq * 2.0 * M_PI;
	  if (phase[c] > 2.0 * M_PI)
	    phase[c] -= 2.0 * M_PI;
	}

      GslDataHandle *ihandle = gsl_data_handle_new_mem (n_channels, 32, mix_freq, 440, input.size(), &input[0], NULL);
      const int order = 116;
      GslDataHandle *fir_handle = NULL;

      if (type == FIR_HIGHPASS)
	fir_handle = bse_data_handle_new_fir_highpass (ihandle, cutoff_freq, order);
      else
	fir_handle = bse_data_handle_new_fir_lowpass (ihandle, cutoff_freq, order);

      BseErrorType error;
      error = gsl_data_handle_open (fir_handle);
      TASSERT (error == 0);

      for (int repeat = 1; repeat <= 2; repeat++)
	{
	  GslDataPeekBuffer peek_buffer = { +1 /* incremental direction */, 0, };
	  double worst_diff = 0.0;
	  for (int64 i = 0; i < fir_handle->setup.n_values; i++)
	    {
	      double filtered = gsl_data_handle_peek_value (fir_handle, i, &peek_buffer);
	      worst_diff = max (filtered - expected[i], worst_diff);
	    }
	  double worst_diff_db = bse_db_from_factor (worst_diff, -200);
	  TOUT ("n_channels = %d: linear(%dst read) read worst_diff = %f (%f dB)\n",
                n_channels, repeat, worst_diff, worst_diff_db);
	  TASSERT (worst_diff_db < -90);
	}
    }
  TDONE();
}

static void
test_seek (FirHandleType type)
{
  TSTART ("%s Handle (seek)", handle_name (type));
  for (int n_channels = 1; n_channels <= 3; n_channels++)
    {
      const double    mix_freq = 48000;
      const double    cutoff_freq = 11000;
      const int       order = 28;

      vector<float>   input (1 * 2 * 3 * 3000); // can be divided by n_channels
      vector<float>   output (input.size());

      for (size_t i = 0; i < input.size(); i++)
        input[i] = g_random_int_range (-1, 1);

      GslDataHandle *ihandle = gsl_data_handle_new_mem (n_channels, 32, mix_freq, 440, input.size(), &input[0], NULL);
      GslDataHandle *fir_handle = NULL;

      if (type == FIR_HIGHPASS)
	fir_handle = bse_data_handle_new_fir_highpass (ihandle, cutoff_freq, order);
      else
	fir_handle = bse_data_handle_new_fir_lowpass (ihandle, cutoff_freq, order);

      BseErrorType error;
      error = gsl_data_handle_open (fir_handle);
      TASSERT (error == 0);

      GslDataPeekBuffer peek_buffer = { +1 /* incremental direction */, 0, };
      for (size_t i = 0; i < output.size(); i++)
        output[i] = gsl_data_handle_peek_value (fir_handle, i, &peek_buffer);

      for (int t = 0; t < 400; t++)
	{
	  int64 start = rand() % fir_handle->setup.n_values;
	  int64 len = rand() % 1024;
	  len = min (fir_handle->setup.n_values - start, len);

	  float values[1024];
	  int64 offset = 0;
	  int64 values_todo = len;
	  while (values_todo > 0)
	    {
	      int64 l = gsl_data_handle_read (fir_handle, start + offset, values_todo, &values[offset]);
	      TASSERT (l > 0);
	      TASSERT (l <= values_todo);
	      values_todo -= l;
	      offset += l;
	    }
	  for (size_t i = 0; i < size_t (len); i++)
	    TASSERT (values[i] == output[i + start]);
	  if (t % 40 == 0)
	    TOK();
	}
    }
  TDONE();
}
int
main (int    argc,
      char **argv)
{
  bse_init_test (&argc, argv);
  test_with_sine_sweep (FIR_HIGHPASS);
  test_multi_channel (FIR_HIGHPASS);
  test_seek (FIR_HIGHPASS);
  test_with_sine_sweep (FIR_LOWPASS);
  test_multi_channel (FIR_LOWPASS);
  test_seek (FIR_LOWPASS);
  return 0;
}
