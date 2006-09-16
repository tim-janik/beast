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
#include <bse/bsemathsignal.h>
#include <bse/bseresampler.hh>
#include <bse/bsemain.h>
// #define TEST_VERBOSE
#include <birnet/birnettests.h>

static void
test_c_api()
{
  BseResampler2 *resampler = bse_resampler2_create (BSE_RESAMPLER2_MODE_UPSAMPLE, BSE_RESAMPLER2_PREC_96DB);
  const int INPUT_SIZE = 1024, OUTPUT_SIZE = 2048;
  float in[INPUT_SIZE];
  float out[OUTPUT_SIZE];
  double error = 0;
  int i;

  for (i = 0; i < INPUT_SIZE; i++)
    in[i] = sin (i * 440 * 2 * M_PI / 44100) * bse_window_blackman ((double) (i * 2 - INPUT_SIZE) / INPUT_SIZE);

  bse_resampler2_process_block (resampler, in, INPUT_SIZE, out);

  int delay = bse_resampler2_order (resampler) + 2;
  for (i = 0; i < 2048; i++)
    {
      double expected = sin ((i - delay) * 220 * 2 * M_PI / 44100)
	              * bse_window_blackman ((double) ((i - delay) * 2 - OUTPUT_SIZE) / OUTPUT_SIZE);
      error = MAX (error, out[i] - expected);
    }

  double error_db = bse_db_from_factor (error, -200);

  bse_resampler2_destroy (resampler);

  TPRINT ("Test C API error: %f\n", error_db);
  TASSERT (error_db < -95);
}

int
main (int   argc,
      char *argv[])
{
  birnet_init_test (&argc, &argv);

  TSTART ("Resampler C API (FPU)");
  test_c_api();
  TDONE();

  /* load plugins */
  BirnetInitValue config[] = {
    { "load-core-plugins", "1" },
    { NULL },
  };
  bse_init_test (&argc, &argv, config);

#if 0 /* FIXME: C? */
  /* check for possible specialization */
  if (Bse::Block::default_singleton() == Bse::Block::current_singleton())
    return 0;   /* nothing changed */
#endif

  TSTART ("Resampler C API (SSE)");
  test_c_api();
  TDONE();

  return 0;
}
