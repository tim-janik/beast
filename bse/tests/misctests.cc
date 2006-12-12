/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2001, 2002, 2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include <bse/bsemain.h>
// #define TEST_VERBOSE
#include <sfi/sfitests.h>
#include <bse/bsemathsignal.h>
#include <bse/bsecxxplugin.hh> // for generated types

#define	FLF	"26.20"

static void
check_cent_table (void)
{
  TSTART ("Cent Table");
  const double epsilon = 1e-15;
  for (int i = -100; i <= +100; i++)
    {
      double setpoint = pow (2.0, 1. / 1200. * i);
      TCHECK_CMP (fabs (bse_cent_factor (i) - setpoint), <, epsilon);
      if (i % 13 == 0)
        TOK();
    }
  TDONE();
}

static void
check_equal_tempered_tuning (void)
{
  TSTART ("Equal Temperament");
  const double *table = bse_semitone_table_from_tuning (BSE_MUSICAL_TUNING_12_TET); /* returns [-132..+132] */
  const double epsilon = 1e-11;
  for (int i = -132; i <= +132; i++)
    {
      double setpoint = pow (2.0, 1. / 12. * i);
      TCHECK_CMP (fabs (table[i] - setpoint), <, epsilon);
      if (i % 13 == 0)
        TOK();
    }
  TDONE();
}

static void
check_tuning_monotony (BseMusicalTuningType musical_tuning)
{
  TSTART ("Monotony");
  const double *table = bse_semitone_table_from_tuning (musical_tuning); /* returns [-132..+132] */
  for (int i = -132; i <= +132; i++)
    if (ABS (i) != 132)
      {
        TCHECK_CMP (table[i - 1], <, table[i]);
        TCHECK_CMP (table[i + 1], >, table[i]);
        if (i % 13 == 0)
          TOK();
      }
  TDONE();
}

static void
check_freq_vs_notes (BseMusicalTuningType musical_tuning)
{
  /* check freq/note mapping */
  TSTART ("Frequency lookup");
  for (int j = BSE_MIN_NOTE; j <= BSE_MAX_NOTE; j++)
    {
      for (int k = BSE_MIN_FINE_TUNE / 2; k <= BSE_MAX_FINE_TUNE / 2; k++)
        {
          double f, freq = bse_note_to_tuned_freq (musical_tuning, j, k);
          int note, fine_tune;
          int verbose = 0;
          if (verbose)
            g_print ("compose  : note=%4d fine_tune=%4d freq=%"FLF"f\n", j, k, freq);
          f = freq;
          note = bse_note_from_freq (musical_tuning, freq);
          TCHECK (note != BSE_NOTE_VOID);
          fine_tune = bse_note_fine_tune_from_note_freq (musical_tuning, note, freq);
          freq = bse_note_to_tuned_freq (musical_tuning, note, fine_tune);
          double freq_error = freq - f;
          double freq_ratio = MAX (freq, f) / MIN (freq, f);
          if (verbose)
            g_print ("decompose: note=%4d fine_tune=%4d freq=%"FLF"f   (diff=%"FLF"f)\n", note, fine_tune, freq, freq - f);
          if (ABS (k) < 11)
            TCHECK (note == j);
          if (musical_tuning == BSE_MUSICAL_TUNING_12_TET)
            TCHECK_CMP (fabs (freq_error), <, 0.00000000001);   /* equal temperament is fairly accurate */
          else
            TCHECK_CMP (freq_ratio, <, 1.00057778950655485930); /* detuning should be smaller than a cent */
        }
      if (j % 3 == 0)
        TOK();
    }
  TDONE();
}

int
main (gint   argc,
      gchar *argv[])
{
  bse_init_test (&argc, &argv, NULL);

  check_cent_table();
  check_equal_tempered_tuning();

  BseMusicalTuningType last_tuning = BSE_MUSICAL_TUNING_YOUNG;
  /* check last tuning value by asserting defaulting behavior of succeding values */
  TCHECK (bse_semitone_table_from_tuning (BseMusicalTuningType (last_tuning + 1)) == bse_semitone_table_from_tuning (BseMusicalTuningType (0)));

  /* check monotonic musical tuning systems */
  for (int j = BSE_MUSICAL_TUNING_12_TET; j <= last_tuning; j++)
    {
      BseMusicalTuningType musical_tuning = BseMusicalTuningType (j);
      g_printerr ("Tuning System: %s\n", sfi_enum2choice (musical_tuning, BSE_TYPE_MUSICAL_TUNING_TYPE));
      check_tuning_monotony (musical_tuning);
      check_freq_vs_notes (musical_tuning);
    }

  return 0;
}
