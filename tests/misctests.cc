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

extern inline uint64_t asuint64     (double f)   { union { double _f; uint64_t _i; } u { f }; return u._i; }
extern inline double   asdouble     (uint64_t i) { union { uint64_t _i; double _f; } u { i }; return u._f; }
static inline float G_GNUC_CONST
arm_exp2f (float x)
{
  // MIT licensed, from: https://github.com/ARM-software/optimized-routines/blob/master/math/exp2f.c
  constexpr const ssize_t EXP2F_TABLE_BITS = 5;
  constexpr const ssize_t N = 1 << EXP2F_TABLE_BITS;
  constexpr const double SHIFT = 0x1.8p+52 / N;
  constexpr const ssize_t EXP2F_POLY_ORDER = 3;
  constexpr const double C[EXP2F_POLY_ORDER] = { 0x1.c6af84b912394p-5, 0x1.ebfce50fac4f3p-3, 0x1.62e42ff0c52d6p-1, };
  //                                             0.055503615593415351  0.240228452244572205  0.693147180691620290
  constexpr const uint64_t T[N] = {
    0x3ff0000000000000, 0x3fefd9b0d3158574, 0x3fefb5586cf9890f, 0x3fef9301d0125b51,
    0x3fef72b83c7d517b, 0x3fef54873168b9aa, 0x3fef387a6e756238, 0x3fef1e9df51fdee1,
    0x3fef06fe0a31b715, 0x3feef1a7373aa9cb, 0x3feedea64c123422, 0x3feece086061892d,
    0x3feebfdad5362a27, 0x3feeb42b569d4f82, 0x3feeab07dd485429, 0x3feea47eb03a5585,
    0x3feea09e667f3bcd, 0x3fee9f75e8ec5f74, 0x3feea11473eb0187, 0x3feea589994cce13,
    0x3feeace5422aa0db, 0x3feeb737b0cdc5e5, 0x3feec49182a3f090, 0x3feed503b23e255d,
    0x3feee89f995ad3ad, 0x3feeff76f2fb5e47, 0x3fef199bdd85529c, 0x3fef3720dcef9069,
    0x3fef5818dcfba487, 0x3fef7c97337b9b5f, 0x3fefa4afa2a490da, 0x3fefd0765b6e4540,
  };
  const double xd = x;

  /* x = k/N + r with r in [-1/(2N), 1/(2N)] and int k.  */
  double kd = xd + SHIFT;
  const uint64_t ki = asuint64 (kd);
  kd = force_double (force_double (kd) - SHIFT); /* k/N for int k.  */
  const double r = xd - kd;

  /* exp2(x) = 2^(k/N) * 2^r ~= s * (C0*r^3 + C1*r^2 + C2*r + 1) */
  uint64_t t = T[ki % N];
  t += ki << (52 - EXP2F_TABLE_BITS);
  const double s = asdouble(t);
  const double z = C[0] * r + C[1];
  const double r2 = r * r;
  double y = C[2] * r + 1;
  y = z * r2 + y;
  y = y * s;
  return y;
}

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
  const double exp2step = Bse::Test::slow() ? 0.0000001 : 0.0001;
  for (double d = -1; d <= +1; d += exp2step)
    {
      const double r = std::exp2 (d);
      const double err = std::fabs (r - fast_exp2 (d));
      TASSERT (err < 4e-7);
    }
  // check fast_log2(int), log2(2^(1..127)) must be calculated with 0 error
  p = 1.0;
  for (ssize_t i = 0; i <= 127; i++)
    {
      if (fast_log2 (p) != float (i))
        Bse::printerr ("fast_log2(%.17g)=%.17g\n", p, fast_log2(p));
      TASSERT (fast_log2 (p) == float (i));
      p *= 2;
    }
  TASSERT (-126 == fast_log2 (1.1754944e-38));
  TASSERT ( -64 == fast_log2 (5.421011e-20));
  TASSERT ( -16 == fast_log2 (1.525879e-5));
  TASSERT (  -8 == fast_log2 (0.00390625));
  TASSERT (  -4 == fast_log2 (0.0625));
  TASSERT (  -2 == fast_log2 (0.25));
  TASSERT (  -1 == fast_log2 (0.5));
  TASSERT (   0 == fast_log2 (1));
  // check fast_log2 error margin in [1/16..16]
  const double log2step = Bse::Test::slow() ? 0.0000001 : 0.0001;
  for (double d = 1 / 16.; d <= 16; d += log2step)
    {
      const double r = std::log2 (d);
      const double err = std::fabs (r - fast_log2 (d));
      if (!(err < 3.8e-6))
        Bse::printerr ("fast_log2(%.17g)=%.17g (diff=%.17g)\n", d, fast_log2 (d), r - fast_log2 (d));
      TASSERT (err < 3.8e-6);
    }
}
TEST_ADD (fast_math_test);

template<typename V, typename F, typename G> static long double
range_error (V vmin, V vmax, V vstep, F f, G g, V *errpos)
{
  long double err = 0;
  for (V input = vmin; input < vmax; input += vstep)
    {
      const long double vf = f (input), vg = g (input);
      const auto old = err;
      err = std::max (err, vf < vg ? vg - vf : vf - vg);
      // err = std::max (err, vf < vg ? (vg - vf) / vf : (vf - vg) / vg);
      if (old != err)
        *errpos = input;
    }
  return err;
}

struct RangeBenchResult {
  double bench_time = 0;
  double n_steps = 0;
  double diff = 0;
  float pos = NAN;
};

static float frange_accu;

template<typename Lambda> static void
frange_print_bench (Lambda lambda, const String &name, RangeBenchResult res)
{
  TBENCH ("%-18s # timing: fastest=%fs calls=%.1f/s diff=%.20f (@%f)\n",
          name, res.bench_time, res.n_steps / res.bench_time, res.diff, res.pos);
  TASSERT (frange_accu != 0);
  TASSERT (!std::isnan (frange_accu));
}
static constexpr const float EXP2RANGE_START = -90, EXP2RANGE_END = 90, EXP2RANGE_STEP = 0.001;
#define EXP2RANGE_BENCH(FUN) ({                    \
  frange_accu = 0;                                   \
  double t = timer.benchmark ([] () {                                              \
                                for (float n = EXP2RANGE_START; n <= EXP2RANGE_END; n += EXP2RANGE_STEP) \
                                  frange_accu += FUN (n); \
                              });                                       \
  RangeBenchResult r { t, (EXP2RANGE_END - EXP2RANGE_START) / EXP2RANGE_STEP }; \
  r.diff = range_error<float> (-1, +1, 0.0000001, exp2, FUN, &r.pos);   \
  r; })
static constexpr const float LOG2RANGE_START = 1e-38, LOG2RANGE_END = 90, LOG2RANGE_STEP = 0.001;
#define LOG2RANGE_BENCH(FUN) ({                    \
  frange_accu = 0;                                   \
  double t = timer.benchmark ([] () {                                              \
                                for (float n = LOG2RANGE_START; n <= LOG2RANGE_END; n += LOG2RANGE_STEP) \
                                  frange_accu += FUN (n); \
                              });                                       \
  RangeBenchResult r { t, (LOG2RANGE_END - LOG2RANGE_START) / LOG2RANGE_STEP };     \
  r.diff = range_error<float> (1e-7, +1, 1e-7, log2, FUN, &r.pos);   \
  r; })

static void
fast_math_bench()
{
  Test::Timer timer (0.15); // maximum seconds
  frange_print_bench (fast_log2, "fast_log2", LOG2RANGE_BENCH (fast_log2));
  frange_print_bench (log2f, "log2f", LOG2RANGE_BENCH (log2f));
  frange_print_bench (fast_exp2, "fast_exp2", EXP2RANGE_BENCH (fast_exp2));
  frange_print_bench (arm_exp2f, "arm_exp2f", EXP2RANGE_BENCH (arm_exp2f));
  frange_print_bench (exp2f, "exp2f", EXP2RANGE_BENCH (exp2f));
}
TEST_BENCH (fast_math_bench);

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
