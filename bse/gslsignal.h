/* GslWaveOsc - GSL Wave Oscillator
 * Copyright (C) 2001-2002 Tim Janik and Stefan Westerfeld
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
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

/* convert between literal frequencies and signal values
 */
#if defined (BSE_COMPILATION) || defined (BSE_PLUGIN_FALLBACK)
#include <bse/bseglobals.h>
#  define GSL_SIGNAL_TO_FREQ(value)	(((gfloat) (value)) * BSE_MAX_FREQUENCY_f)
#  define GSL_SIGNAL_FROM_FREQ(freq)	((gfloat) ((freq) * (1.0 / BSE_MAX_FREQUENCY_f)))
#endif

/**
 * gsl_signal_exp2
 * Fast conversion of linear frequency modulation factor to
 * exponential frequency modulation factor. This is essentially
 * an approximation of exp2f(). It can be much faster than the
 * glibc function though, by taking advantage of a limited input
 * range and smaller precision requirements.
 */
static inline float	gsl_signal_exp2 (float x);



/* --- implementation details --- */
static inline float
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
static inline float
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
