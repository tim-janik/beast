/* GSL - Generic Sound Layer
 * Copyright (C) 2001-2002 Tim Janik and Stefan Westerfeld
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
#ifndef __GSL_SIGNAL_H__
#define __GSL_SIGNAL_H__

#include <gsl/gsldefs.h>
#include <gsl/gslieee754.h>
#include <gsl/gslmath.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* smallest value of a signal sample, greater than zero
 */
#define GSL_SIGNAL_EPSILON      (1.15e-14)      /* 1.16415321826934814453125e-9 ~= 1/2^33 */

/* maximum value of a signal sample
 */
#define GSL_SIGNAL_KAPPA        (1.5)

/* catch edges in sync signals.
 * sync signals should be constant, do comparing against
 * an epsilon just hurts speed in the common case
 */
#define GSL_SIGNAL_RAISING_EDGE(v1,v2)	((v1) < (v2))
#define GSL_SIGNAL_FALLING_EDGE(v1,v2)	((v1) > (v2))

/* value changes in signals which represent frequencies
 */
#define GSL_SIGNAL_FREQ_CHANGED(v1,v2)	(fabs ((v1) - (v2)) > 1e-7)

/* value changes in signals which represent modulation
 */
#define GSL_SIGNAL_MOD_CHANGED(v1,v2)	(fabs ((v1) - (v2)) > 1e-8)

/* value changes in signals which represent dB ranges
 */
#define GSL_SIGNAL_GAIN_CHANGED(v1,v2)	(fabs ((v1) - (v2)) > 1e-8)

/* convert between literal frequencies and signal values
 */
#if defined (BSE_COMPILATION) || defined (BSE_PLUGIN_FALLBACK)
#include <bse/bseglobals.h>
#  define GSL_SIGNAL_TO_FREQ_FACTOR	(BSE_MAX_FREQUENCY_f)
#  define GSL_SIGNAL_FROM_FREQ_FACTOR	(1.0 / BSE_MAX_FREQUENCY_f)
#  define GSL_SIGNAL_TO_FREQ(value)	(((gfloat) (value)) * GSL_SIGNAL_TO_FREQ_FACTOR)
#  define GSL_SIGNAL_FROM_FREQ(freq)	(((gfloat) (freq)) * GSL_SIGNAL_FROM_FREQ_FACTOR)
#elif defined (GSL_WANT_ARTS_THREADS)  /* must be aRts */
#  define GSL_SIGNAL_TO_FREQ(x)		(x)
#  define GSL_SIGNAL_FROM_FREQ(x)	(x)
#endif


/* --- frequency modulation --- */
typedef struct {
  gfloat	fm_strength;		/* linear: 0..1, exponential: n_octaves */
  guint		exponential_fm : 1;
  gfloat	signal_freq;		/* for ifreq == NULL (as GSL_SIGNAL_FROM_FREQ) */
  gint		fine_tune;		/* -100..+100 */
} GslFrequencyModulator;

void	gsl_frequency_modulator	(const GslFrequencyModulator	*fm,
				 guint				 n_values,
				 const gfloat			*ifreq,
				 const gfloat			*ifmod,
				 gfloat				*fm_buffer);


/* --- function approximations --- */

/**
 * gsl_signal_exp2
 * Deprecated in favour of gsl_approx_exp2().
 */
static inline float	gsl_signal_exp2 (float x)  G_GNUC_CONST;

/**
 * gsl_approx_exp2
 * @ex:      exponent within [-127..127]
 * @RETURNS: y approximating 2^x
 * Fast approximation of 2 raised to the power of x.
 * Multiplicative error stays below 8e-6 and aproaches zero
 * for integer values of x (i.e. x - floor (x) -> 0).
 */
static inline double	gsl_approx_exp2	(float ex)	G_GNUC_CONST;


/**
 * gsl_approx_atan1
 * @x: x as in atan(x)
 * Fast atan(x)/(PI/2) approximation, with maximum error < 0.01 and
 * gsl_approx_atan1(0)==0, according to the formula:
 * n1 = -0.41156875521951602506487246309908;
 * n2 = -1.0091272542790025586079663559158;
 * d1 = 0.81901156857081841441890603235599;
 * d2 = 1.0091272542790025586079663559158;
 * positive_atan1(x) = 1 + (n1 * x + n2) / ((1 + d1 * x) * x + d2);
 */
static inline double	gsl_approx_atan1 	  (register double x)  G_GNUC_CONST;

/**
 * gsl_approx_atan1_prescale
 * @boost_amount: boost amount between [0..1]
 * @RETURNS:      prescale factor for gsl_approx_atan1()
 * Calculate the prescale factor for gsl_approx_atan1(x*prescale) from
 * a linear boost factor, where 0.5 amounts to prescale=1.0, 1.0 results
 * in maximum boost and 0.0 results in maximum attenuation.
 */
double			gsl_approx_atan1_prescale (double	   boost_amount);

/**
 * gsl_approx_qcircle1
 * @x:       x within [0..1]
 * @RETURNS: y for circle approximation within [0..1]
 * Fast approximation of the upper right quadrant of a circle.
 * Errors at x=0 and x=1 are zero, for the rest of the curve, the error
 * wasn't minimized, but distributed to best fit the curverture of a
 * quarter circle. The maximum error is below 0.092.
 */
static inline double	gsl_approx_qcircle1	  (register double x)  G_GNUC_CONST;

/**
 * gsl_approx_qcircle2
 * @x:       x within [0..1]
 * @RETURNS: y for circle approximation within [0..1]
 * Fast approximation of the upper left quadrant of a circle.
 * Errors at x=0 and x=1 are zero, for the rest of the curve, the error
 * wasn't minimized, but distributed to best fit the curverture of a
 * quarter circle. The maximum error is below 0.092.
 */
static inline double	gsl_approx_qcircle2	  (register double x)  G_GNUC_CONST;

/**
 * gsl_approx_qcircle3
 * @x:       x within [0..1]
 * @RETURNS: y for circle approximation within [0..1]
 * Fast approximation of the lower left quadrant of a circle.
 * Errors at x=0 and x=1 are zero, for the rest of the curve, the error
 * wasn't minimized, but distributed to best fit the curverture of a
 * quarter circle. The maximum error is below 0.092.
 */
static inline double	gsl_approx_qcircle3	  (register double x)  G_GNUC_CONST;

/**
 * gsl_approx_qcircle4
 * @x:       x within [0..1]
 * @RETURNS: y for circle approximation within [0..1]
 * Fast approximation of the lower right quadrant of a circle.
 * Errors at x=0 and x=1 are zero, for the rest of the curve, the error
 * wasn't minimized, but distributed to best fit the curverture of a
 * quarter circle. The maximum error is below 0.092.
 */
static inline double	gsl_approx_qcircle4	  (register double x)  G_GNUC_CONST;


/* --- windows --- */
double	gsl_window_bartlett	(double x);	/* narrowest */
double	gsl_window_blackman	(double x);
double	gsl_window_cos		(double x);
double	gsl_window_hamming	(double x);
double	gsl_window_sinc		(double x);
double	gsl_window_rect		(double x);	/* widest */


/* --- cents (1/100th of a semitone) --- */
#define	gsl_cent_factor(index /* -100..100 */)	(gsl_cent_table[index])
extern const gdouble *gsl_cent_table;


/* --- implementation details --- */
static inline double  G_GNUC_CONST
gsl_approx_atan1 (register double x)
{
  if (x < 0)	/* make use of -atan(-x)==atan(x) */
    {
      register double numerator, denominator = -1.0;

      denominator += x * 0.81901156857081841441890603235599; /* d1 */
      numerator = x * 0.41156875521951602506487246309908; /* -n1 */
      denominator *= x;
      numerator += -1.0091272542790025586079663559158; /* n2 */
      denominator += 1.0091272542790025586079663559158; /* d2 */

      return -1.0 - numerator / denominator;
    }
  else
    {
      register double numerator, denominator = 1.0;

      denominator += x * 0.81901156857081841441890603235599; /* d1 */
      numerator = x * -0.41156875521951602506487246309908; /* n1 */
      denominator *= x;
      numerator += -1.0091272542790025586079663559158; /* n2 */
      denominator += 1.0091272542790025586079663559158; /* d2 */

      return 1.0 + numerator / denominator;
    }
}

static inline double	G_GNUC_CONST
gsl_approx_qcircle1 (register double x)
{
  double numerator = 1.20460124790369468987715633298929 * x - 1.20460124790369468987715633298929;
  double denominator = x - 1.20460124790369468987715633298929;
  /* R1(x)=(1.2046012479036946898771563 * x - 1.2046012479036946898771563) / (x - 1.2046012479036946898771563) */
  return numerator / denominator;
}

static inline double	G_GNUC_CONST
gsl_approx_qcircle2 (register double x)
{
  double numerator = 1.20460124790369468987715633298929*x;
  double denominator = x + 0.20460124790369468987715633298929;
  /* R2(x)=1.2046012479036946898771563*x/(x + 0.2046012479036946898771563) */
  return numerator / denominator;
}

static inline double	G_GNUC_CONST
gsl_approx_qcircle3 (register double x)
{
  double numerator = 0.20460124790369468987715633298929 - 0.20460124790369468987715633298929 * x;
  double denominator = x + 0.20460124790369468987715633298929;
  /* R3(x)=(0.2046012479036946898771563 - 0.2046012479036946898771563 * x) / (x + 0.2046012479036946898771563) */
  return numerator / denominator;
}

static inline double	G_GNUC_CONST
gsl_approx_qcircle4 (register double x)
{
  double numerator = -0.20460124790369468987715633298929 * x;
  double denominator = x - 1.20460124790369468987715633298929;
  /* R4(x)=-0.2046012479036946898771563 * x / (x - 1.2046012479036946898771563) */
  return numerator / denominator;
}

static inline double G_GNUC_CONST
gsl_approx_exp2 (float ex)
{
  register GslFloatIEEE754 fp = { 0, };
  register double numer, denom, x;
  gint i;

  i = gsl_ftoi (ex);
  fp.mpn.biased_exponent = GSL_FLOAT_BIAS + i;
  x = ex - i;
  numer = x * 1.022782938747283388104723674300322141276;
  denom = x - 8.72117024533378044415954808601135282456;
  numer += 8.786902350800703562041965087953613538091;
  denom *= x;
  numer *= x;
  denom += 25.25880955504064143887016455761526606757;
  numer += 25.2588095552441757401874424757283407864;

  return numer / denom * fp.v_float;
}

static inline float  G_GNUC_CONST
_gsl_signal_exp2_fraction (float x)	/* 2^x, -0.5 <= x <= 0.5 */
{
  static const float exp2taylorC0 = 1.0000000000000000000000000000000000000000;
  static const float exp2taylorC1 = 0.6931471805599452862267639829951804131269;
  static const float exp2taylorC2 = 0.2402265069591006940719069007172947749496;
  static const float exp2taylorC3 = 0.0555041086648215761800706502526736585423;
  static const float exp2taylorC4 = 0.0096181291076284768787330037298488605302;
  static const float exp2taylorC5 = 0.0013333558146428443284131626356270317046;
#if 0
  static const float exp2taylorC6 = 0.0001540353039338160877607525334198612654;
  static const float exp2taylorC7 = 0.0000152527338040598393887042200089965149;
  static const float exp2taylorC8 = 0.0000013215486790144307390984122416166535;
  static const float exp2taylorC9 = 0.0000001017808600923969859895309888857262;
#endif
  float r = 0.0;
  
  /* order 5 taylor series aproximation */
  r += exp2taylorC5;
  r *= x;
  r += exp2taylorC4;
  r *= x;
  r += exp2taylorC3;
  r *= x;
  r += exp2taylorC2;
  r *= x;
  r += exp2taylorC1;
  r *= x;
  r += exp2taylorC0;
  
  return r;
}
static inline float  G_GNUC_CONST
gsl_signal_exp2 (float x)		/* 2^x, -3.5 <= x <= 3.5, prec>16bit */
{
  if_reject (x < -0.5)
    {
      if_reject (x < -1.5)
	{
	  if (x < -2.5)
	    return 0.125 * _gsl_signal_exp2_fraction (x + 3);
	  else /* -2.5 <= x < -1.5 */
	    return 0.25 * _gsl_signal_exp2_fraction (x + 2);
	}
      else /* -1.5 <= x < -0.5 */
	return 0.5 * _gsl_signal_exp2_fraction (x + 1);
    }
  else if_reject (x > 0.5)
    {
      if_reject (x > 1.5)
	{
	  if (x > 2.5)
	    return 8 * _gsl_signal_exp2_fraction (x - 3);
	  else /* 1.5 < x <= 2.5 */
	    return 4 * _gsl_signal_exp2_fraction (x - 2);
	}
      else /* 0.5 < x <= 1.5 */
	return 2 * _gsl_signal_exp2_fraction (x - 1);
    }
  else
    return _gsl_signal_exp2_fraction (x);
}



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_SIGNAL_H__ */
