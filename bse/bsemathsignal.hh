// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_SIGNAL_H__
#define __BSE_SIGNAL_H__

#include <bse/bsemath.hh>
#include <bse/bseglobals.hh>
#include <bse/bsetype.hh> // for BseMusicalTuningType

/**
 * smallest value of a signal sample, greater than zero
 */
#define BSE_SIGNAL_EPSILON      (1.15e-14)      /* 1.16415321826934814453125e-9 ~= 1/2^33 */

/**
 * maximum value of a signal sample
 */
#define BSE_SIGNAL_KAPPA        (1.5)

/**
 * Catch edges in sync signals.
 * sync signals should be constant, do comparing against
 * an epsilon just hurts speed in the common case.
 */
#define BSE_SIGNAL_RAISING_EDGE(v1,v2)	((v1) < (v2))
/**
 * Inverse variant of BSE_SIGNAL_RAISING_EDGE().
 */
#define BSE_SIGNAL_FALLING_EDGE(v1,v2)	((v1) > (v2))

/**
 * Value changes in signals which represent frequencies.
 */
#define BSE_SIGNAL_FREQ_CHANGED(v1,v2)	(fabs ((v1) - (v2)) > 1e-7)
/**
 * Inverse variant of BSE_SIGNAL_FREQ_CHANGED().
 */
#define BSE_SIGNAL_FREQ_EQUALS(v1,v2)	(!BSE_SIGNAL_FREQ_CHANGED (v1, v2))

/**
 * Value changes in signals which represent modulation.
 */
#define BSE_SIGNAL_MOD_CHANGED(v1,v2)	(fabs ((v1) - (v2)) > 1e-8)

/**
 * Value changes in signals which represent dB ranges.
 */
#define BSE_SIGNAL_GAIN_CHANGED(v1,v2)	(fabs ((v1) - (v2)) > 1e-8)

/**
 * Convert between literal frequencies and signal values.
 */
#define BSE_SIGNAL_TO_FREQ_FACTOR	(BSE_MAX_FREQUENCY)
#define BSE_SIGNAL_FROM_FREQ_FACTOR	(1.0 / BSE_MAX_FREQUENCY)
#define BSE_SIGNAL_TO_FREQ(value)	(BSE_FREQ_FROM_VALUE (value))
#define BSE_SIGNAL_FROM_FREQ(freq)	(BSE_VALUE_FROM_FREQ (freq))

#define BSE_SIGNAL_CLIP(v)      bse_signal_value_clip (v)

static inline double   bse_signal_value_clip (double x)  G_GNUC_CONST;
static inline double G_GNUC_CONST
bse_signal_value_clip (double x)
{
  if (G_UNLIKELY (x > 1.0))
    return 1.0;
  if (G_UNLIKELY (x < -1.0))
    return -1.0;
  return x;
}


/* --- frequency modulation --- */
typedef struct {
  gfloat	fm_strength;		/* linear: 0..1, exponential: n_octaves */
  guint		exponential_fm : 1;
  gfloat	signal_freq;		/* for ifreq == NULL (as BSE_SIGNAL_FROM_FREQ) */
  gint		fine_tune;		/* -100..+100 */
} BseFrequencyModulator;

void	bse_frequency_modulator	(const BseFrequencyModulator	*fm,
				 guint				 n_values,
				 const gfloat			*ifreq,
				 const gfloat			*ifmod,
				 gfloat				*fm_buffer);


/* --- windows --- */
double	bse_window_bartlett	(double x);	/* narrowest */
double	bse_window_blackman	(double x);
double	bse_window_cos		(double x);
double	bse_window_hamming	(double x);
double	bse_window_sinc		(double x);
double	bse_window_rect		(double x);	/* widest */


/* --- function approximations --- */

/**
 * @param x	x as in atan(x)
 *
 * Fast atan(x)/(PI/2) approximation, with maximum error < 0.01 and
 * bse_approx_atan1(0)==0, according to the formula:
 * n1 = -0.41156875521951602506487246309908;
 * n2 = -1.0091272542790025586079663559158;
 * d1 = 0.81901156857081841441890603235599;
 * d2 = 1.0091272542790025586079663559158;
 * positive_atan1(x) = 1 + (n1 * x + n2) / ((1 + d1 * x) * x + d2);
 */
static inline double	bse_approx_atan1 	  (double x)  G_GNUC_CONST;

/**
 * @param boost_amount	boost amount between [0..1]
 * @return		prescale factor for bse_approx_atan1()
 *
 * Calculate the prescale factor for bse_approx_atan1(x*prescale) from
 * a linear boost factor, where 0.5 amounts to prescale=1.0, 1.0 results
 * in maximum boost and 0.0 results in maximum attenuation.
 */
double			bse_approx_atan1_prescale (double	   boost_amount);

/**
 * @param x	x within [0..1]
 * @return	y for circle approximation within [0..1]
 *
 * Fast approximation of the upper right quadrant of a circle.
 * Errors at x=0 and x=1 are zero, for the rest of the curve, the error
 * wasn't minimized, but distributed to best fit the curverture of a
 * quarter circle. The maximum error is below 0.092.
 */
static inline double	bse_approx_qcircle1	  (double x)  G_GNUC_CONST;

/**
 * @param x	x within [0..1]
 * @return	y for circle approximation within [0..1]
 *
 * Fast approximation of the upper left quadrant of a circle.
 * Errors at x=0 and x=1 are zero, for the rest of the curve, the error
 * wasn't minimized, but distributed to best fit the curverture of a
 * quarter circle. The maximum error is below 0.092.
 */
static inline double	bse_approx_qcircle2	  (double x)  G_GNUC_CONST;

/**
 * @param x	x within [0..1]
 * @return	y for circle approximation within [0..1]
 *
 * Fast approximation of the lower left quadrant of a circle.
 * Errors at x=0 and x=1 are zero, for the rest of the curve, the error
 * wasn't minimized, but distributed to best fit the curverture of a
 * quarter circle. The maximum error is below 0.092.
 */
static inline double	bse_approx_qcircle3	  (double x)  G_GNUC_CONST;

/**
 * @param x	x within [0..1]
 * @return	y for circle approximation within [0..1]
 *
 * Fast approximation of the lower right quadrant of a circle.
 * Errors at x=0 and x=1 are zero, for the rest of the curve, the error
 * wasn't minimized, but distributed to best fit the curverture of a
 * quarter circle. The maximum error is below 0.092.
 */
static inline double	bse_approx_qcircle4	  (double x)  G_GNUC_CONST;

/**
 * @param ex	exponent within [-127..+127]
 * @return	y approximating 2^ex
 *
 * Fast approximation of 2 raised to the power of ex.
 * Within -1..+1, the error stays below 0.01275 which corresponds to a sample
 * precision of 6.2 bit, the average error amounts to 0.001914.
 * For integer values of @a ex (i.e. @a ex - floor (@a ex) -> 0), the error
 * approaches zero. On a 2GHz machine, execution takes roughly 12.81 ns.
 */
static inline double    bse_approx2_exp2        (float ex)      G_GNUC_CONST;

/**
 * @param ex	exponent within [-127..+127]
 * @return	y approximating 2^ex
 *
 * Fast approximation of 2 raised to the power of ex.
 * Within -1..+1, the error stays below 0.001123415 which corresponds to a sample
 * precision of 9.7 bit, the average error amounts to 0.000133.
 * For integer values of @a ex (i.e. @a ex - floor (@a ex) -> 0), the error
 * approaches zero. On a 2GHz machine, execution takes roughly 13.74 ns.
 */
static inline double    bse_approx3_exp2        (float ex)      G_GNUC_CONST;

/**
 * @param ex	exponent within [-127..+127]
 * @return	y approximating 2^ex
 *
 * Fast approximation of 2 raised to the power of ex.
 * Within -1..+1, the error stays below 7.876055e-05 which corresponds to a sample
 * precision of 13.6 bit, the average error amounts to 7.7012792e-06.
 * For integer values of @a ex (i.e. @a ex - floor (@a ex) -> 0), the error
 * approaches zero. On a 2GHz machine, execution takes roughly 16.46 ns.
 */
static inline double    bse_approx4_exp2        (float ex)      G_GNUC_CONST;

/**
 * @param ex	exponent within [-127..+127]
 * @return	y approximating 2^ex
 *
 * Fast approximation of 2 raised to the power of ex.
 * Within -1..+1, the error stays below 4.60807023e-06 which corresponds to a sample
 * precision of 17.7 bit, the average error amounts to 3.842199e-07.
 * For integer values of @a ex (i.e. @a ex - floor (@a ex) -> 0), the error
 * approaches zero. On a 2GHz machine, execution takes roughly 18.51 ns.
 */
static inline double    bse_approx5_exp2        (float ex)      G_GNUC_CONST;

/**
 * @param ex	exponent within [-127..+127]
 * @return	y approximating 2^ex
 *
 * Fast approximation of 2 raised to the power of ex.
 * Within -1..+1, the error stays below 2.5505813e-07 which corresponds to a sample
 * precision of 21.9 bit, the average error amounts to 2.1028377e-08.
 * For integer values of @a ex (i.e. @a ex - floor (@a ex) -> 0), the error
 * approaches zero. On a 2GHz machine, execution takes roughly 21.84 ns.
 */
static inline double    bse_approx6_exp2        (float ex)      G_GNUC_CONST;

/**
 * @param ex	exponent within [-127..+127]
 * @return	y approximating 2^ex
 *
 * Fast approximation of 2 raised to the power of ex.
 * Within -1..+1, the error stays below 4.1074325e-08 which corresponds to a sample
 * precision of 24.5 bit, the average error amounts to 7.7448985e-09.
 * For integer values of @a ex (i.e. @a ex - floor (@a ex) -> 0), the error
 * approaches zero. On a 2GHz machine, execution takes roughly 23.79 ns.
 */
static inline double    bse_approx7_exp2        (float ex)      G_GNUC_CONST;

/**
 * @param ex	exponent within [-127..+127]
 * @return	y approximating 2^ex
 *
 * Fast approximation of 2 raised to the power of ex.
 * Within -1..+1, the error stays below 4.1074325e-08 which corresponds to a sample
 * precision of 24.5 bit, the average error amounts to 7.6776048e-09.
 * Note that there is no significant precision increment over bse_approx7_exp2().
 * For integer values of @a ex (i.e. @a ex - floor (@a ex) -> 0), the error
 * approaches zero. On a 2GHz machine, execution takes roughly 26.59 ns.
 */
static inline double    bse_approx8_exp2        (float ex)      G_GNUC_CONST;

/**
 * @param ex	exponent within [-127..+127]
 * @return	y approximating 2^ex
 *
 * Fast approximation of 2 raised to the power of ex.
 * Within -1..+1, the error stays below 4.1074325e-08 which corresponds to a sample
 * precision of 24.5 bit, the average error amounts to 7.677515903e-09.
 * Note that there is no significant precision increment over bse_approx7_exp2().
 * For integer values of @a ex (i.e. @a ex - floor (@a ex) -> 0), the error
 * approaches zero. On a 2GHz machine, execution takes roughly 29.40 ns.
 */
static inline double    bse_approx9_exp2        (float ex)      G_GNUC_CONST;

/**
 * @param x	exponent within [-127..+127]
 * @return	y approximating tanh(x)
 *
 * Fast approximation of the hyperbolic tangent of x.
 * Within -1..+1, the error stays below 0.00436 which corresponds to a sample
 * precision of 7.8 bit, the average error amounts to 0.00069220.
 * On a 2GHz machine, execution takes roughly 24.48 ns.
 */
static inline double    bse_approx2_tanh        (float x)       G_GNUC_CONST;

/**
 * @param x	exponent within [-127..+127]
 * @return	y approximating tanh(x)
 *
 * Fast approximation of the hyperbolic tangent of x.
 * Within -1..+1, the error stays below 0.0003857 which corresponds to a sample
 * precision of 7.8 bit, the average error amounts to 0.00004827.
 * On a 2GHz machine, execution takes roughly 25.78 ns.
 */
static inline double    bse_approx3_tanh        (float x)       G_GNUC_CONST;

/**
 * @param x	exponent within [-127..+127]
 * @return	y approximating tanh(x)
 *
 * Fast approximation of the hyperbolic tangent of x.
 * Within -1..+1, the error stays below 2.7017507e-05 which corresponds to a sample
 * precision of 15.1 bit, the average error amounts to 2.799594e-06.
 * On a 2GHz machine, execution takes roughly 28.41 ns.
 */
static inline double    bse_approx4_tanh        (float x)       G_GNUC_CONST;

/**
 * @param x	exponent within [-127..+127]
 * @return	y approximating tanh(x)
 *
 * Fast approximation of the hyperbolic tangent of x.
 * Within -1..+1, the error stays below 1.582042006e-06 which corresponds to a sample
 * precision of 19.2 bit, the average error amounts to 1.42780810e-07.
 * On a 2GHz machine, execution takes roughly 30.35 ns.
 */
static inline double    bse_approx5_tanh        (float x)       G_GNUC_CONST;

/**
 * @param x	exponent within [-127..+127]
 * @return	y approximating tanh(x)
 *
 * Fast approximation of the hyperbolic tangent of x.
 * Within -1..+1, the error stays below 9.7878796e-08 which corresponds to a sample
 * precision of 23.2 bit, the average error amounts to 1.3016999e-08.
 * On a 2GHz machine, execution takes roughly 34.29 ns.
 */
static inline double    bse_approx6_tanh        (float x)       G_GNUC_CONST;

/**
 * @param x	exponent within [-127..+127]
 * @return	y approximating tanh(x)
 *
 * Fast approximation of the hyperbolic tangent of x.
 * Within -1..+1, the error stays below 4.4375671e-08 which corresponds to a sample
 * precision of 24.4 bit, the average error amounts to 9.5028421e-09.
 * On a 2GHz machine, execution takes roughly 36.86 ns.
 */
static inline double    bse_approx7_tanh        (float x)       G_GNUC_CONST;

/**
 * @param x	exponent within [-127..+127]
 * @return	y approximating tanh(x)
 *
 * Fast approximation of the hyperbolic tangent of x.
 * Within -1..+1, the error stays below 4.4375671e-08 which corresponds to a sample
 * precision of 24.4 bit, the average error amounts to 9.49155722e-09.
 * Note that there is no significant precision increment over bse_approx7_tanh().
 * On a 2GHz machine, execution takes roughly 42.03 ns.
 */
static inline double    bse_approx8_tanh        (float x)       G_GNUC_CONST;

/**
 * @param x	exponent within [-127..+127]
 * @return	y approximating tanh(x)
 *
 * Fast approximation of the hyperbolic tangent of x.
 * Within -1..+1, the error stays below 4.4375671e-08 which corresponds to a sample
 * precision of 24.4 bit, the average error amounts to 9.49141881e-09.
 * Note that there is no significant precision increment over bse_approx7_tanh().
 * On a 2GHz machine, execution takes roughly 43.83 ns.
 */
static inline double    bse_approx9_tanh        (float x)       G_GNUC_CONST;

/**
 * @param value		value to saturate
 * @param limit		limit not to be exceeded by value
 * @return		@a value bounded by -limit and @a limit
 *
 * Clamp @a value within -limit and +limit. Limiting is performed
 * by floating point operations only, thus executing faster than
 * condition based branching code on most modern architectures.
 * On a 2GHz machine, execution takes roughly 6.86 ns.
 */
static inline double    bse_saturate_hard       (double value,
                                                 double limit)  G_GNUC_CONST;

/**
 * @param value		value to saturate
 * @param limit		limit not to be exceeded by value
 * @return		@a value bounded by -limit and @a limit
 *
 * Clamp @a value within -limit and +limit. Limiting is performed
 * by executing conditions and branches, so it will probably run
 * slower than bse_saturate_hard() on many machines.
 * On a 2GHz machine, execution takes roughly 8.29 ns.
 */
static inline double    bse_saturate_branching (double value,
                                                double limit)   G_GNUC_CONST;

/* --- semitone factors (for +-11 octaves) --- */
const double* bse_semitone_table_from_tuning (Bse::MusicalTuning musical_tuning); /* returns [-132..+132] */
double        bse_transpose_factor           (Bse::MusicalTuning musical_tuning, int index /* [-132..+132] */);

/* --- cents (1/100th of a semitone) --- */

double                  bse_cent_tune (double fine_tune);

/**
 * @param fine_tune	fine tuning in cent between -100 and 100
 * @return		a factor corresponding to this
 *
 * This function computes a factor which corresponds to a given fine tuning in
 * cent.  The result can be used as factor for the frequency or the play speed.
 * It is a faster alternative to bse_cent_tune(), and can only deal with
 * integer values between -100 and 100. The input is always CLAMPed to ensure
 * that it lies in this range.
 */
static inline double	bse_cent_tune_fast (int fine_tune /* -100..+100 */)   G_GNUC_CONST;

/* --- implementation details --- */
static inline double  G_GNUC_CONST
bse_approx_atan1 (double x)
{
  if (x < 0)	/* make use of -atan(-x)==atan(x) */
    {
      double numerator, denominator = -1.0;

      denominator += x * 0.81901156857081841441890603235599; /* d1 */
      numerator = x * 0.41156875521951602506487246309908; /* -n1 */
      denominator *= x;
      numerator  += -1.0091272542790025586079663559158; /* n2 */
      denominator += 1.0091272542790025586079663559158; /* d2 */

      return -1.0 - numerator / denominator;
    }
  else
    {
      double numerator, denominator = 1.0;

      denominator += x * 0.81901156857081841441890603235599; /* d1 */
      numerator = x * -0.41156875521951602506487246309908; /* n1 */
      denominator *= x;
      numerator  += -1.0091272542790025586079663559158; /* n2 */
      denominator += 1.0091272542790025586079663559158; /* d2 */

      return 1.0 + numerator / denominator;
    }
  /* atan1_positive(x)=1+(x*-0.411568755219516-1.009127254279)/((1+x*0.81901156857)*x+1.009127254279)
   * atan1(x)=x<0 ? -atan1_positive(-x) : atan1_positive(x)
   */
}

static inline double	G_GNUC_CONST
bse_approx_qcircle1 (double x)
{
  double numerator = 1.20460124790369468987715633298929 * x - 1.20460124790369468987715633298929;
  double denominator = x - 1.20460124790369468987715633298929;
  /* R1(x)=(1.2046012479036946898771563 * x - 1.2046012479036946898771563) / (x - 1.2046012479036946898771563) */
  return numerator / denominator;
}

static inline double	G_GNUC_CONST
bse_approx_qcircle2 (double x)
{
  double numerator = 1.20460124790369468987715633298929*x;
  double denominator = x + 0.20460124790369468987715633298929;
  /* R2(x)=1.2046012479036946898771563*x/(x + 0.2046012479036946898771563) */
  return numerator / denominator;
}

static inline double	G_GNUC_CONST
bse_approx_qcircle3 (double x)
{
  double numerator = 0.20460124790369468987715633298929 - 0.20460124790369468987715633298929 * x;
  double denominator = x + 0.20460124790369468987715633298929;
  /* R3(x)=(0.2046012479036946898771563 - 0.2046012479036946898771563 * x) / (x + 0.2046012479036946898771563) */
  return numerator / denominator;
}

static inline double	G_GNUC_CONST
bse_approx_qcircle4 (double x)
{
  double numerator = -0.20460124790369468987715633298929 * x;
  double denominator = x - 1.20460124790369468987715633298929;
  /* R4(x)=-0.2046012479036946898771563 * x / (x - 1.2046012479036946898771563) */
  return numerator / denominator;
}

static inline double G_GNUC_CONST
bse_approx2_exp2 (float ex)
{
  BseFloatIEEE754 fp = { 0, };
  int i = bse_ftoi (ex);
  fp.mpn.biased_exponent = BSE_FLOAT_BIAS + i;
  double x = ex - i;
  return fp.v_float * (1.0 + x * (0.69314718055994530941723212145818 +
                                  x * (0.24022650695910071233355126316333)));
}

static inline double G_GNUC_CONST
bse_approx3_exp2 (float ex)
{
  BseFloatIEEE754 fp = { 0, };
  int i = bse_ftoi (ex);
  fp.mpn.biased_exponent = BSE_FLOAT_BIAS + i;
  double x = ex - i;
  return fp.v_float * (1.0 + x * (0.69314718055994530941723212145818 +
                                  x * (0.24022650695910071233355126316333 +
                                       x * (0.055504108664821579953142263768622))));
  /* exp2frac(x)=x-ftoi(x)
   * exp2a3(x)=2**ftoi(x)*(1+exp2frac(x)*(0.6931471805599453+exp2frac(x)*(0.2402265069591+exp2frac(x)*0.0555041086648)))
   */
}

static inline double G_GNUC_CONST
bse_approx4_exp2 (float ex)
{
  BseFloatIEEE754 fp = { 0, };
  int i = bse_ftoi (ex);
  fp.mpn.biased_exponent = BSE_FLOAT_BIAS + i;
  double x = ex - i;
  return fp.v_float * (1.0 + x * (0.69314718055994530941723212145818 +
                                  x * (0.24022650695910071233355126316333 +
                                       x * (0.055504108664821579953142263768622 +
                                            x * (0.0096181291076284771619790715736589)))));
  /* ftoi(x)=int(x<-0.0 ? x - 0.5 : x + 0.5)
   * exp2frac(x)=x-ftoi(x)
   * exp2a4(x)=2**ftoi(x)*(1+exp2frac(x)*(0.6931471805599453+exp2frac(x)*(0.2402265069591+exp2frac(x)*(0.0555041086648+exp2frac(x)*0.009618129107628477))))
   */
}

static inline double G_GNUC_CONST
bse_approx5_exp2 (float ex)
{
  BseFloatIEEE754 fp = { 0, };
  int i = bse_ftoi (ex);
  fp.mpn.biased_exponent = BSE_FLOAT_BIAS + i;
  double x = ex - i;
  return fp.v_float * (1.0 + x * (0.69314718055994530941723212145818 +
                                  x * (0.24022650695910071233355126316333 +
                                       x * (0.055504108664821579953142263768622 +
                                            x * (0.0096181291076284771619790715736589 +
                                                 x * (0.0013333558146428443423412221987996))))));
}

static inline double G_GNUC_CONST
bse_approx6_exp2 (float ex)
{
  BseFloatIEEE754 fp = { 0, };
  int i = bse_ftoi (ex);
  fp.mpn.biased_exponent = BSE_FLOAT_BIAS + i;
  double x = ex - i;
  return fp.v_float * (1.0 + x * (0.69314718055994530941723212145818 +
                                  x * (0.24022650695910071233355126316333 +
                                       x * (0.055504108664821579953142263768622 +
                                            x * (0.0096181291076284771619790715736589 +
                                                 x * (0.0013333558146428443423412221987996 +
                                                      x * (0.00015403530393381609954437097332742)))))));
}

static inline double G_GNUC_CONST
bse_approx7_exp2 (float ex)
{
  BseFloatIEEE754 fp = { 0, };
  int i = bse_ftoi (ex);
  fp.mpn.biased_exponent = BSE_FLOAT_BIAS + i;
  double x = ex - i;
  return fp.v_float * (1.0 + x * (0.69314718055994530941723212145818 +
                                  x * (0.24022650695910071233355126316333 +
                                       x * (0.055504108664821579953142263768622 +
                                            x * (0.0096181291076284771619790715736589 +
                                                 x * (0.0013333558146428443423412221987996 +
                                                      x * (0.00015403530393381609954437097332742 +
                                                           x * (0.00001525273380405984028002543901201))))))));
}

static inline double G_GNUC_CONST
bse_approx8_exp2 (float ex)
{
  BseFloatIEEE754 fp = { 0, };
  int i = bse_ftoi (ex);
  fp.mpn.biased_exponent = BSE_FLOAT_BIAS + i;
  double x = ex - i;
  return fp.v_float * (1.0 + x * (0.69314718055994530941723212145818 +
                                  x * (0.24022650695910071233355126316333 +
                                       x * (0.055504108664821579953142263768622 +
                                            x * (0.0096181291076284771619790715736589 +
                                                 x * (0.0013333558146428443423412221987996 +
                                                      x * (0.00015403530393381609954437097332742 +
                                                           x * (0.00001525273380405984028002543901201 +
                                                                x * (0.0000013215486790144309488403758228288)))))))));
}

static inline double G_GNUC_CONST
bse_approx9_exp2 (float ex)
{
  BseFloatIEEE754 fp = { 0, };
  int i = bse_ftoi (ex);
  fp.mpn.biased_exponent = BSE_FLOAT_BIAS + i;
  double x = ex - i;
  return fp.v_float * (1.0 + x * (0.69314718055994530941723212145818 +
                                  x * (0.24022650695910071233355126316333 +
                                       x * (0.055504108664821579953142263768622 +
                                            x * (0.0096181291076284771619790715736589 +
                                                 x * (0.0013333558146428443423412221987996 +
                                                      x * (0.00015403530393381609954437097332742 +
                                                           x * (0.00001525273380405984028002543901201 +
                                                                x * (0.0000013215486790144309488403758228288 +
                                                                     x * 0.00000010178086009239699727490007597745)))))))));
}

static inline double G_GNUC_CONST
bse_approx2_tanh (float x)
{
  if (G_UNLIKELY (x < -20))
    return -1;
  if (G_UNLIKELY (x > 20))
    return 1;
  double bpot = bse_approx2_exp2 (x * BSE_2_DIV_LN2);
  return (bpot - 1) / (bpot + 1);
}

static inline double G_GNUC_CONST
bse_approx3_tanh (float x)
{
  if (G_UNLIKELY (x < -20))
    return -1;
  if (G_UNLIKELY (x > 20))
    return 1;
  double bpot = bse_approx3_exp2 (x * BSE_2_DIV_LN2);
  return (bpot - 1) / (bpot + 1);
}

static inline double G_GNUC_CONST
bse_approx4_tanh (float x)
{
  if (G_UNLIKELY (x < -20))
    return -1;
  if (G_UNLIKELY (x > 20))
    return 1;
  double bpot = bse_approx4_exp2 (x * BSE_2_DIV_LN2);
  return (bpot - 1) / (bpot + 1);
  /* tanha4(x)=x<-20 ? -1 : x>20 ? 1 : (exp2a4(x*2.885390081777926814719849362)-1) / (exp2a4(x*2.885390081777926814719849362)+1) */
}

static inline double G_GNUC_CONST
bse_approx5_tanh (float x)
{
  if (G_UNLIKELY (x < -20))
    return -1;
  if (G_UNLIKELY (x > 20))
    return 1;
  double bpot = bse_approx5_exp2 (x * BSE_2_DIV_LN2);
  return (bpot - 1) / (bpot + 1);
}

static inline double G_GNUC_CONST
bse_approx6_tanh (float x)
{
  if (G_UNLIKELY (x < -20))
    return -1;
  if (G_UNLIKELY (x > 20))
    return 1;
  double bpot = bse_approx6_exp2 (x * BSE_2_DIV_LN2);
  return (bpot - 1) / (bpot + 1);
}

static inline double G_GNUC_CONST
bse_approx7_tanh (float x)
{
  if (G_UNLIKELY (x < -20))
    return -1;
  if (G_UNLIKELY (x > 20))
    return 1;
  double bpot = bse_approx7_exp2 (x * BSE_2_DIV_LN2);
  return (bpot - 1) / (bpot + 1);
}

static inline double G_GNUC_CONST
bse_approx8_tanh (float x)
{
  if (G_UNLIKELY (x < -20))
    return -1;
  if (G_UNLIKELY (x > 20))
    return 1;
  double bpot = bse_approx8_exp2 (x * BSE_2_DIV_LN2);
  return (bpot - 1) / (bpot + 1);
}

static inline double G_GNUC_CONST
bse_approx9_tanh (float x)
{
  if (G_UNLIKELY (x < -20))
    return -1;
  if (G_UNLIKELY (x > 20))
    return 1;
  double bpot = bse_approx9_exp2 (x * BSE_2_DIV_LN2);
  return (bpot - 1) / (bpot + 1);
}

static inline double G_GNUC_CONST
bse_saturate_hard (double value,
                   double limit)
{
  double v1 = fabs (value + limit);
  double v2 = fabs (value - limit);
  return 0.5 * (v1 - v2); /* CLAMP() without branching */
}

static inline double G_GNUC_CONST
bse_saturate_branching (double value,
                        double limit)
{
  if (G_UNLIKELY (value >= limit))
    return limit;
  if (G_UNLIKELY (value <= limit))
    return -limit;
  return value;
}

void    _bse_init_signal (void);

extern const double * const bse_cent_table;

static inline double G_GNUC_CONST
bse_cent_tune_fast (int fine_tune)
{
  return bse_cent_table[CLAMP (fine_tune, -100, 100)];
}

namespace Bse {

/**
 * @param  value positive input value
 * @return y     approximating log2 (value)
 *
 * Template arguments:
 *   ORDER  tradeoff between speed and accuracy of the approximation
 *   T      whether to use double or float for internal calculations
 *
 * This function computes a fast approximation for log2. The first template
 * argument ORDER must be specified. The second selects the floating point
 * type and defaults to double.
 *
 * fast_log2<2> (value)
 *   is the fastest version with the largest approximation error
 * fast_log2<9> (value)
 *   is the slowest version with the smallest approximation error
 *
 * In cases where only a float value is required, the second template argument
 * T can be set to float. Typically this will be a bit faster than using double
 * precision internally. In that case the ORDER can be at most 6, since for
 * higher orders the error introduced by float computations is too high.
 *
 * Maximum approximation error in interval [1;2] for different values ORDER is
 * (first value T = double, second value T = float)
 *
 *   2  0.00493976    0.004940044
 *   3  0.0006371173  0.0006375115
 *   4  8.759192e-05  8.858764e-05
 *   5  1.253874e-05  1.412891e-05
 *   6  1.845687e-06  4.548069e-06
 *   7  2.772895e-07  -
 *   8  4.231443e-08  -
 *   9  6.53724e-09   -
 *
 * The error is computed as: | log2 (value) - fast_log2 (value) |
 */
template<int ORDER, typename T = double>
static inline T G_GNUC_CONST
fast_log2 (float value)
{
  union {
    float f;
    int i;
  } float_u;
  float_u.f = value;
  // compute log_2 using float exponent
  const int log_2 = ((float_u.i >> 23) & 255) - 128;
  // replace float exponent
  float_u.i &= ~(255 << 23);
  float_u.i += BSE_FLOAT_BIAS << 23;

  static_assert (ORDER >= 2 && ORDER <= 9);
  if constexpr (std::is_same<float, T>::value)
    static_assert (ORDER <= 6);

  T x = float_u.f;

  // polynomials designed with: lolremez -d <ORDER> -r 1:2 "log(x)/log(2)+1"
  if constexpr (ORDER == 2)
    {
      T u = -3.4484843300001946e-1;
      u = u * x + T (2.0246657790474698);
      x = u * x + T (-6.748775860711561e-1);
    }
  else if constexpr (ORDER == 3)
    {
      T u = 1.5824870260836496e-1;
      u = u * x + T (-1.0518750217176431);
      u = u * x + T (3.0478841468943746);
      x = u * x + T (-1.1536207104929913);
    }
  else if constexpr (ORDER == 4)
    {
      T u = -8.1615808498122383e-2;
      u = u * x + T (6.4514236358772082e-1);
      u = u * x + T (-2.1206751311142674);
      u = u * x + T (4.0700907918522014);
      x = u * x + T (-1.5128546239033371);
    }
  else if constexpr (ORDER == 5)
    {
      T u = 4.4873610194131727e-2;
      u = u * x + T (-4.1656368651734915e-1);
      u = u * x + T (1.6311487636297217);
      u = u * x + T (-3.5507929249026341);
      u = u * x + T (5.0917108110420042);
      x = u * x + T (-1.8003640347009253);
    }
  else if constexpr (ORDER == 6)
    {
      T u = -2.5691088815846394e-2;
      u = u * x + T (2.7514877034856807e-1);
      u = u * x + T (-1.2669182593669425);
      u = u * x + T (3.2865287704176774);
      u = u * x + T (-5.3419892025067624);
      u = u * x + T (6.1129631283200212);
      x = u * x + T (-2.0400402727100282);
    }
  else if constexpr (ORDER == 7)
    {
      T u = 1.5125359168008401e-2;
      u = u * x + T (-1.8393964232233078e-1);
      u = u * x + T (9.7809484881021081e-1);
      u = u * x + T (-2.9850211023586282);
      u = u * x + T (5.7814391203775074);
      u = u * x + T (-7.494130176731051);
      u = u * x + T (7.1339697617832616);
      x = u * x + T (-2.2455378914374758);
    }
  else if constexpr (ORDER == 8)
    {
      T u = -9.0889081045466651e-3;
      u = u * x + T (1.2384570293556399e-1);
      u = u * x + T (-7.4835742226622081e-1);
      u = u * x + T (2.6387738689074552);
      u = u * x + T (-6.0134326106109641);
      u = u * x + T (9.2859836735642522);
      u = u * x + T (-1.0007135250742891e+1);
      u = u * x + T (8.1548040722981051);
      x = u * x + T (-2.4253930836663341);
    }
  else if constexpr (ORDER == 9)
    {
      T u = 5.547594786690081e-3;
      u = u * x + T (-8.3767270477714422e-2);
      u = u * x + T (5.6751026819556446e-1);
      u = u * x + T (-2.2749487308928086);
      u = u * x + T (5.9910685362749332);
      u = u * x + T (-1.0884909361115433e+1);
      u = u * x + T (1.3970237262129601e+1);
      u = u * x + T (-1.2880952951531813e+1);
      u = u * x + T (9.1755128331255977);
      x = u * x + T (-2.585298173957386);
    }

   return x + log_2;
}

/**
 * @param  ex exponent within [-127..+127]
 * @return y  approximating 2^ex
 *
 * Template arguments:
 *   ORDER  tradeoff between speed and accuracy of the approximation
 *   T      whether to use double or float for internal calculations
 *
 * This function computes a fast approximation for exp2. The first template
 * argument ORDER must be specified. The second selects the floating point
 * type and defaults to double.
 *
 * fast_exp2<2> (ex)
 *   is the fastest version with the largest approximation error
 * fast_exp2<9> (ex)
 *   is the slowest version with the smallest approximation error
 *
 * In cases where only a float value is required, the second template argument
 * T can be set to float. Typically this will be a bit faster than using double
 * precision internally. In that case the ORDER can be at most 6, since for
 * higher orders the error introduced by float computations is too high.
 *
 * Maximum relative approximation error in interval [-1;1] for different
 * values ORDER is (first value T = double, second value T = float)
 *
 *   2  0.001724763   0.001724847
 *   3  7.478144e-05  7.482798e-05
 *   4  2.593371e-06  2.70684e-06
 *   5  7.493647e-08  2.333446e-07
 *   6  1.8558e-09    9.54076e-08
 *   7  4.021119e-11  -
 *   8  7.746751e-13  -
 *   9  1.375958e-14  -
 *
 * The relative error is computed as: | exp2 (x) - fast_exp2 (x) | / exp2 (x)
 */
template<int ORDER, typename T = double>
static inline T G_GNUC_CONST
fast_exp2 (float ex)
{
  BseFloatIEEE754 fp = { 0, };
  int i = bse_ftoi (ex);
  fp.mpn.biased_exponent = BSE_FLOAT_BIAS + i;
  T x = ex - i;

  static_assert (ORDER >= 2 && ORDER <= 9);
  if constexpr (std::is_same<float, T>::value)
    static_assert (ORDER <= 6);

  // polynomials designed with: lolremez -d <ORDER> -r -0.5:0.5 "exp(log(2)*x)" "exp(log(2)*x)"
  if constexpr (ORDER == 2)
    {
      T u = 2.3842893576403885e-1;
      u = u * x + T (7.0344800589128555e-1);
      x = u * x + T (1.0004431419562679);
    }
  else if constexpr (ORDER == 3)
    {
      T u = 5.5171669058037948e-2;
      u = u * x + T (2.4261112219321803e-1);
      u = u * x + T (6.9326098546062362e-1);
      x = u * x + T (9.9992807353939516e-1);
    }
  else if constexpr (ORDER == 4)
    {
      T u = 9.5701019112108269e-3;
      u = u * x + T (5.5917860319386755e-2);
      u = u * x + T (2.4024744827862419e-1);
      u = u * x + T (6.9312181473668853e-1);
      x = u * x + T (9.9999926144571251e-1);
    }
  else if constexpr (ORDER == 5)
    {
      T u = 1.3276471992255869e-3;
      u = u * x + T (9.6755413344448377e-3);
      u = u * x + T (5.5507132734988059e-2);
      u = u * x + T (2.402211972384019e-1);
      u = u * x + T (6.9314696706476011e-1);
      x = u * x + T (1.0000000716546848);
    }
  else if constexpr (ORDER == 6)
    {
      T u = 1.5345812002903348e-4;
      u = u * x + T (1.339993121934089e-3);
      u = u * x + T (9.6184889571150709e-3);
      u = u * x + T (5.5503287769647256e-2);
      u = u * x + T (2.4022646890634088e-1);
      u = u * x + T (6.9314720573726808e-1);
      x = u * x + T (1.0000000005541665);
    }
  else if constexpr (ORDER == 7)
    {
      T u = 1.5201921594573214e-5;
      u = u * x + T (1.5469291141688497e-4);
      u = u * x + T (1.3333922563538754e-3);
      u = u * x + T (9.6180272536684501e-3);
      u = u * x + T (5.5504103534479798e-2);
      u = u * x + T (2.4022651198157586e-1);
      u = u * x + T (6.9314718072844706e-1);
      x = u * x + T (9.9999999996168193e-1);
    }
  else if constexpr (ORDER == 8)
    {
      T u = 1.317585811360802e-6;
      u = u * x + T (1.5309737422244982e-5);
      u = u * x + T (1.5403851748734642e-4);
      u = u * x + T (1.3333452062247541e-3);
      u = u * x + T (9.6181285428623769e-3);
      u = u * x + T (5.5504109393417119e-2);
      u = u * x + T (2.402265069888037e-1);
      u = u * x + T (6.9314718054651416e-1);
      x = u * x + T (9.9999999999976235e-1);
    }
  else if constexpr (ORDER == 9)
    {
      T u = 1.0150336705309648e-7;
      u = u * x + T (1.3259405609345135e-6);
      u = u * x + T (1.5252984838653427e-5);
      u = u * x + T (1.540343494807179e-4);
      u = u * x + T (1.3333557617604443e-3);
      u = u * x + T (9.6181291920672461e-3);
      u = u * x + T (5.5504108668685612e-2);
      u = u * x + T (2.4022650695649653e-1);
      u = u * x + T (6.9314718055987097e-1);
      x = u * x + T (1.0000000000000128);
    }
  return fp.v_float * x;
}

}

#endif /* __BSE_SIGNAL_H__ */
