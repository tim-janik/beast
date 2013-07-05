// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include <bse/bsemain.hh>
// #define TEST_VERBOSE
#include <sfi/sfitests.hh>
#include <bse/bsemathsignal.hh>
#include <bse/bsecxxplugin.hh> // for generated types

#define	FLF	"26.20"

static void
check_cent_tune_fast (void)
{
  TSTART ("Cent Tune Function (fast Table based implementation)");
  const double epsilon = 1e-15;
  for (int i = -100; i <= +100; i++)
    {
      double setpoint = pow (2.0, 1. / 1200. * i);
      TCMP (fabs (bse_cent_tune_fast (i) - setpoint), <, epsilon);
      if (i % 13 == 0)
        TOK();
    }
  TDONE();
}

static void
check_cent_tune (void)
{
  TSTART ("Cent Tune Function");
  const double epsilon = 1e-14;
  int i = 0;
  for (double fine_tune = -3600; fine_tune < 3600; fine_tune += g_random_double())  /* 3 octaves */
    {
      double expected = pow (2.0, 1. / 1200. * fine_tune);
      TCMP (fabs (bse_cent_tune (fine_tune) - expected), <, epsilon);
      if (i++ % 500 == 0)
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
      TCMP (fabs (table[i] - setpoint), <, epsilon);
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
        TCMP (table[i - 1], <, table[i]);
        TCMP (table[i + 1], >, table[i]);
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
            g_print ("compose  : note=%4d fine_tune=%4d freq=%" FLF "f\n", j, k, freq);
          f = freq;
          note = bse_note_from_freq (musical_tuning, freq);
          TASSERT (note != BSE_NOTE_VOID);
          fine_tune = bse_note_fine_tune_from_note_freq (musical_tuning, note, freq);
          freq = bse_note_to_tuned_freq (musical_tuning, note, fine_tune);
          double freq_error = freq - f;
          double freq_ratio = MAX (freq, f) / MIN (freq, f);
          if (verbose)
            g_print ("decompose: note=%4d fine_tune=%4d freq=%" FLF "f   (diff=%" FLF "f)\n", note, fine_tune, freq, freq - f);
          if (ABS (k) < 11)
            TASSERT (note == j);
          if (musical_tuning == BSE_MUSICAL_TUNING_12_TET)
            TCMP (fabs (freq_error), <, 0.00000000001);   /* equal temperament is fairly accurate */
          else
            TCMP (freq_ratio, <, 1.00057778950655485930); /* detuning should be smaller than a cent */
        }
      if (j % 3 == 0)
        TOK();
    }
  TDONE();
}

static void
test_math_bits ()
{
  TCMP (bse_string_from_double (-1.0), ==, "-1.0");
  TCMP (bse_string_from_double (0.0), ==, "0.0");
  TCMP (bse_string_from_double (1000000000000000.75000000000000), ==, "1000000000000000.75");
  BseComplex c[] = { { 0, 0 }, { -1, +1 }, { +2, -3 } };
  const std::string c0 = bse_complex_str (c[0]);
  TCMP (c0, ==, "{0.0, 0.0}");
  const std::string c1 = bse_complex_str (c[1]);
  TCMP (c1, ==, "{-1.0, 1.0}");
  const std::string c2 = bse_complex_str (c[2]);
  TCMP (c2, ==, "{2.0, -3.0}");
  const std::string complex_list = bse_complex_list (RAPICORN_ARRAY_SIZE (c), c, "");
  TCMP (complex_list, ==, "0.0 0.0\n-1.0 1.0\n2.0 -3.0\n");
  double a[] = { 0, -1, +2, -3, +7.75, -1000000000000000.75 };
  const std::string poly1 = bse_poly_str (RAPICORN_ARRAY_SIZE (a) - 1, a, "x");
  TCMP (poly1, ==, "(0.0+x*(-1.0+x*(2.0+x*(-3.0+x*(7.75+x*(-1000000000000000.75))))))");
  const std::string poly2 = bse_poly_str1 (RAPICORN_ARRAY_SIZE (a) - 1, a, "y");
  TCMP (poly2, ==, "(-1.0*y + 2.0*y**2 + -3.0*y**3 + 7.75*y**4 + -1000000000000000.75*y**5)");
}

int
main (gint   argc,
      gchar *argv[])
{
  bse_init_test (&argc, argv);
  test_math_bits();
  check_cent_tune();
  check_cent_tune_fast();
  check_equal_tempered_tuning();
  BseMusicalTuningType last_tuning = BSE_MUSICAL_TUNING_YOUNG;
  /* check last tuning value by asserting defaulting behavior of succeding values */
  TCMP (bse_semitone_table_from_tuning (BseMusicalTuningType (last_tuning + 1)),
        ==,
        bse_semitone_table_from_tuning (BseMusicalTuningType (0)));
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
