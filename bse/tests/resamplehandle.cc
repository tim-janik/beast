/* BSE Resampling Datahandles Test
 * Copyright (C) 2001-2002 Tim Janik
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
#include <bse/gsldatahandle.h>
#include <bse/bsemain.h>
#include <bse/bsemathsignal.h>
#include <bse/gsldatautils.h>
#include <bse/bseblockutils.hh>
// #define TEST_VERBOSE
#include <birnet/birnettests.h>
#include <vector>

using std::vector;
using std::max;
using std::min;

void
read_through (GslDataHandle *handle)
{
  int64 n_values = gsl_data_handle_n_values (handle);
  int64 offset = 0;

  while (offset < n_values)
    {
      gfloat values[1024];
      int64 values_read = gsl_data_handle_read (handle, offset, 1024, values);
      g_assert (values_read > 0);
      offset += values_read;
    }

  g_assert (offset == n_values);
}

double
check (const vector<float>& input, const vector<double>& expected, int n_channels, int precision_bits, double max_db)
{
  g_return_val_if_fail (input.size() * 2 == expected.size(), 0);
  g_return_val_if_fail (input.size() % n_channels == 0, 0);
  
  GslDataHandle *ihandle = gsl_data_handle_new_mem (n_channels, 32, 44100, 440, input.size(), &input[0], NULL);
  GslDataHandle *rhandle = bse_data_handle_new_upsample2 (ihandle, precision_bits); 
  gsl_data_handle_unref (ihandle);

  BseErrorType error = gsl_data_handle_open (rhandle);
  if (error)
    exit (1);

  double worst_diff, worst_diff_db;

  /* read through the datahandle linearily _twice_ */
  for (int repeat = 1; repeat <= 2; repeat++)
    {
      GslDataPeekBuffer peek_buffer = { +1 /* incremental direction */, 0, };
      worst_diff = 0.0;
      for (int64 i = 0; i < rhandle->setup.n_values; i++)
	{
	  double resampled = gsl_data_handle_peek_value (rhandle, i, &peek_buffer);
	  worst_diff = max (resampled - expected[i], worst_diff);
	}
      worst_diff_db = bse_db_from_factor (worst_diff, -200);
      TPRINT ("linear(%dst read) read worst_diff = %f (%f dB)\n", repeat, worst_diff, worst_diff_db);
      TASSERT (worst_diff_db < max_db);
    }

  /* test seeking */
  worst_diff = 0.0;
  for (int i = 0; i < 1000; i++)
    {
      int64 start = rand() % rhandle->setup.n_values;
      int64 len = rand() % 1024;

      GslDataPeekBuffer peek_buffer = { +1 /* incremental direction */, 0, };
      for (int64 i = start; i < std::min (start + len, rhandle->setup.n_values); i++)
	{
	  double resampled = gsl_data_handle_peek_value (rhandle, i, &peek_buffer);
	  worst_diff = max (resampled - expected[i], worst_diff);
	}
    }
  worst_diff_db = bse_db_from_factor (worst_diff, -200);
  TPRINT ("seek worst_diff = %f (%f dB)\n", worst_diff, worst_diff_db);
  TASSERT (worst_diff_db < max_db);

  /* test speed */
  const guint RUNS = 10;
  GTimer *timer = g_timer_new();
  const guint dups = TEST_CALIBRATION (50.0, read_through (rhandle));
 
  double m = 9e300;
  for (guint i = 0; i < RUNS; i++)
    {
      g_timer_start (timer);
      for (guint j = 0; j < dups; j++)
	read_through (rhandle);
      g_timer_stop (timer);
      double e = g_timer_elapsed (timer, NULL);
      if (e < m)
        m = e;
    }
  double samples_per_second = input.size() / (m / dups);
  TPRINT ("  samples / second = %f\n", samples_per_second);
  TPRINT ("  which means the resampler can process %.2f 44100 Hz streams simultaneusly\n", samples_per_second / 44100.0);
  TPRINT ("  or one 44100 Hz stream takes %f %% CPU usage\n", 100.0 / (samples_per_second / 44100.0));

  gsl_data_handle_close (rhandle);
  gsl_data_handle_unref (rhandle);

  return samples_per_second / 44100.0;
}

void
run_tests()
{
  struct TestParameters {
    int bits;
    double mono_db;
    double stereo_db;
  } params[] =
  {
    { 8, -48, -48 },
    { 12, -72, -72 },
    { 16, -98, -95 },
    { 20, -120, -117 },
    { 24, -125, -125 }, /* this is not _really_ 24 bit, because the filter is computed using floats */
    { 0, 0, 0 }
  };

  for (int p = 0; params[p].bits; p++)
    {
      const int LEN = 44100*10;
      vector<float> input;
      vector<double> expected;
      for (int i = 0; i < LEN; i++)
	{
	  input.push_back (sin (i * 2 * M_PI * 440.0 / 44100.0) * bse_window_blackman (double (i * 2 - LEN) / LEN));

	  double j = i + 0.5; /* compute perfectly interpolated result */
	  expected.push_back (sin (i * 2 * M_PI * 440.0 / 44100.0) * bse_window_blackman (double (i * 2 - LEN) / LEN));
	  expected.push_back (sin (j * 2 * M_PI * 440.0 / 44100.0) * bse_window_blackman (double (j * 2 - LEN) / LEN));
	}

      // mono test
      {
	char *x = g_strdup_printf ("ResampleHandle %dbits mono", params[p].bits);
	TSTART (x);
	g_free (x);

	double streams = check (input, expected, 1, params[p].bits, params[p].mono_db);
	TDONE();

	g_printerr ("    ===> speed is equivalent to %.2f simultaneous 44100 Hz streams\n", streams);
      }

      input.clear();
      expected.clear();
      for (int i = 0; i < LEN; i++)
	{
	  input.push_back (sin (i * 2 * M_PI * 440.0 / 44100.0) * bse_window_blackman (double (i * 2 - LEN) / LEN));
	  input.push_back (sin (i * 2 * M_PI * 1000.0 / 44100.0) * bse_window_blackman (double (i * 2 - LEN) / LEN));

	  double j = i + 0.5; /* compute perfectly interpolated result */
	  expected.push_back (sin (i * 2 * M_PI * 440.0 / 44100.0) * bse_window_blackman (double (i * 2 - LEN) / LEN));
	  expected.push_back (sin (i * 2 * M_PI * 1000.0 / 44100.0) * bse_window_blackman (double (i * 2 - LEN) / LEN));
	  expected.push_back (sin (j * 2 * M_PI * 440.0 / 44100.0) * bse_window_blackman (double (j * 2 - LEN) / LEN));
	  expected.push_back (sin (j * 2 * M_PI * 1000.0 / 44100.0) * bse_window_blackman (double (j * 2 - LEN) / LEN));
	}

      // stereo test
      {
	char *x = g_strdup_printf ("ResampleHandle %dbits stereo", params[p].bits);
	TSTART (x);
	g_free (x);

	double streams = check (input, expected, 2, params[p].bits, params[p].stereo_db);
	TDONE();

	g_printerr ("    ===> speed is equivalent to %.2f simultaneous 44100 Hz streams\n", streams);
      }
    }
}

int
main (int   argc,
      char *argv[])
{
  birnet_init_test (&argc, &argv);
  //g_thread_init (NULL);
  //birnet_init_test (&argc, &argv);
  //bse_init_intern (&argc, &argv, "TestResample", NULL);
  //std::set_terminate (__gnu_cxx::__verbose_terminate_handler);

  printf ("*** tests on FPU\n");
  run_tests();

  /* load plugins */
  BirnetInitValue config[] = {
    { "load-core-plugins", "1" },
    { NULL },
  };
  bse_init_test (&argc, &argv, config);

  /* check for possible specialization */
  if (Bse::Block::default_singleton() == Bse::Block::current_singleton())
    return 0;   /* nothing changed */

  printf ("*** tests on SSE\n");
  run_tests();

  return 0;
}
