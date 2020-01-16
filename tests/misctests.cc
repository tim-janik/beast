// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include <bse/bsemain.hh>
// #define TEST_VERBOSE
#include <bse/testing.hh>
#include <bse/bsemathsignal.hh>
#include <bse/bsecxxplugin.hh> // for generated types
#include "jsonipc/testjsonipc.cc" // test_jsonipc
#include <bse/signalmath.hh>

static void
test_jsonipc_functions()
{
  test_jsonipc();
}
TEST_ADD (test_jsonipc_functions);

using namespace Bse;

#define	FLF	"26.20"

static void
check_cent_tune_fast (void)
{
  // Cent Tune Function (fast Table based implementation)
  const double epsilon = 1e-15;
  for (int i = -100; i <= +100; i++)
    {
      double setpoint = pow (2.0, 1. / 1200. * i);
      TCMP (fabs (bse_cent_tune_fast (i) - setpoint), <, epsilon);
      if (i % 13 == 0)
        TOK();
    }
}
TEST_ADD (check_cent_tune_fast);

static void
check_cent_tune (void)
{
  // Cent Tune Function
  const double epsilon = 1e-14;
  int i = 0;
  for (double fine_tune = -3600; fine_tune < 3600; fine_tune += g_random_double())  /* 3 octaves */
    {
      double expected = pow (2.0, 1. / 1200. * fine_tune);
      TCMP (fabs (bse_cent_tune (fine_tune) - expected), <, epsilon);
      if (i++ % 500 == 0)
        TOK();
    }
}
TEST_ADD (check_cent_tune);

static void
check_equal_tempered_tuning (void)
{
  const double *table = bse_semitone_table_from_tuning (Bse::MusicalTuning::OD_12_TET); /* returns [-132..+132] */
  const double epsilon = 1e-11;
  for (int i = -132; i <= +132; i++)
    {
      double setpoint = pow (2.0, 1. / 12. * i);
      TCMP (fabs (table[i] - setpoint), <, epsilon);
      if (i % 13 == 0)
        TOK();
    }
}
TEST_ADD (check_equal_tempered_tuning);

static void
check_tuning_monotony (Bse::MusicalTuning musical_tuning)
{
  const double *table = bse_semitone_table_from_tuning (musical_tuning); /* returns [-132..+132] */
  for (int i = -132; i <= +132; i++)
    if (ABS (i) != 132)
      {
        TCMP (table[i - 1], <, table[i]);
        TCMP (table[i + 1], >, table[i]);
        if (i % 13 == 0)
          TOK();
      }
}

static void
check_freq_vs_notes (Bse::MusicalTuning musical_tuning)
{
  /* check freq/note mapping */
  for (int j = BSE_MIN_NOTE; j <= BSE_MAX_NOTE; j++)
    {
      for (int k = BSE_MIN_FINE_TUNE / 2; k <= BSE_MAX_FINE_TUNE / 2; k++)
        {
          double f, freq = bse_note_to_tuned_freq (musical_tuning, j, k);
          int note, fine_tune;
          int verbose = 0;
          if (verbose)
            printout ("compose  : note=%4d fine_tune=%4d freq=%" FLF "f\n", j, k, freq);
          f = freq;
          note = bse_note_from_freq (musical_tuning, freq);
          TASSERT (note != BSE_NOTE_VOID);
          fine_tune = bse_note_fine_tune_from_note_freq (musical_tuning, note, freq);
          freq = bse_note_to_tuned_freq (musical_tuning, note, fine_tune);
          double freq_error = freq - f;
          double freq_ratio = MAX (freq, f) / MIN (freq, f);
          if (verbose)
            printout ("decompose: note=%4d fine_tune=%4d freq=%" FLF "f   (diff=%" FLF "f)\n", note, fine_tune, freq, freq - f);
          if (ABS (k) < 11)
            TASSERT (note == j);
          if (musical_tuning == Bse::MusicalTuning::OD_12_TET)
            TCMP (fabs (freq_error), <, 0.00000000001);   /* equal temperament is fairly accurate */
          else
            TCMP (freq_ratio, <, 1.00057778950655485930); /* detuning should be smaller than a cent */
        }
      if (j % 3 == 0)
        TOK();
    }
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
  const std::string complex_list = bse_complex_list (BSE_ARRAY_SIZE (c), c, "");
  TCMP (complex_list, ==, "0.0 0.0\n-1.0 1.0\n2.0 -3.0\n");
  double a[] = { 0, -1, +2, -3, +7.75, -1000000000000000.75 };
  const std::string poly1 = bse_poly_str (BSE_ARRAY_SIZE (a) - 1, a, "x");
  TCMP (poly1, ==, "(0.0+x*(-1.0+x*(2.0+x*(-3.0+x*(7.75+x*(-1000000000000000.75))))))");
  const std::string poly2 = bse_poly_str1 (BSE_ARRAY_SIZE (a) - 1, a, "y");
  TCMP (poly2, ==, "(-1.0*y + 2.0*y**2 + -3.0*y**3 + 7.75*y**4 + -1000000000000000.75*y**5)");
}
TEST_ADD (test_math_bits);

static void
check_monotonic_tuning()
{
  const int64 last_tuning = int (Bse::MusicalTuning::YOUNG);
  /* check last tuning value by asserting defaulting behavior of succeding values */
  TCMP (bse_semitone_table_from_tuning (Bse::MusicalTuning (last_tuning + 1)),
        ==,
        bse_semitone_table_from_tuning (Bse::MusicalTuning (0)));
  /* check monotonic musical tuning systems */
  for (int j = int (Bse::MusicalTuning::OD_12_TET); j <= last_tuning; j++)
    {
      Bse::MusicalTuning musical_tuning = Bse::MusicalTuning (j);
      check_tuning_monotony (musical_tuning);
      check_freq_vs_notes (musical_tuning);
      TPASS ("Tuning System: %s\n", Aida::enum_value_to_short_string (musical_tuning));
    }
}
TEST_ADD (check_monotonic_tuning);

static void
fast_math_test()
{
  float f;
  double d;
  // check proper Inf and NaN handling (depends on compiler flags)
  f = d = .5 * INFINITY; TASSERT (d > 0 && f > 0 && std::isinf (f) && std::isinf (d));
  f = d = -3 * INFINITY; TASSERT (d < 0 && f < 0 && std::isinf (f) && std::isinf (d));
  f = f - f;            TASSERT (!(f == f) && std::isnan (f));  // Infinity - Infinity yields NaN
  d = 0.0 / 0.0;        TASSERT (!(d == d) && std::isnan (d));
  // check rounding
  TASSERT (int (+0.40) == +0.0 && Bse::irintf (+0.40) == +0.0);
  TASSERT (int (-0.40) == -0.0 && Bse::irintf (-0.40) == -0.0);
  TASSERT (int (+0.51) == +0.0 && Bse::irintf (+0.51) == +1.0);
  TASSERT (int (-0.51) == -0.0 && Bse::irintf (-0.51) == -1.0);
  TASSERT (int (+1.90) == +1.0 && Bse::irintf (+1.90) == +2.0);
  TASSERT (int (-1.90) == -1.0 && Bse::irintf (-1.90) == -2.0);
  // check fast_exp2(int), 2^(-126..+127) must be calculated with 0 error
  volatile float m = 1.0, p = 1.0;
  for (ssize_t i = 0; i <= 127; i++)
    {
      // Bse::printerr ("2^±%-3d = %15.10g, %-.14g\n", i, fast_exp2 (i), fast_exp2 (-i));
      TASSERT (fast_exp2 (i) == p);
      if (i != 127)
        TASSERT (fast_exp2 (-i) == m);
      else
        TASSERT (fast_exp2 (-i) <= m); // single precision result for 2^-127 is 0 or subnormal
      p *= 2;
      m /= 2;
    }
  // check fast_exp2 error margin in [-1..+1]
  const double step = Bse::Test::slow() ? 0.0000001 : 0.0001;
  for (double d = -1; d <= +1; d += step)
    {
      const double r = std::exp2 (d);
      const double err = std::fabs (r - fast_exp2 (d));
      TASSERT (err < 4e-7);
    }
}
TEST_ADD (fast_math_test);

#if 0
int
main (gint   argc,
      gchar *argv[])
{
  bse_init_test (&argc, argv);
  test_math_bits();
  check_cent_tune();
  check_cent_tune_fast();
  check_equal_tempered_tuning();
  check_monotonic_tuning();
  return 0;
}
#endif
