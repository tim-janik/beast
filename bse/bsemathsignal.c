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
#include "gslsignal.h"

#include "gslcommon.h"


/* --- frequency modulation --- */
void
gsl_frequency_modulator (const GslFrequencyModulator *fm,
			 guint                        n_values,
			 const gfloat                *ifreq,
			 const gfloat                *ifmod,
			 gfloat                      *fm_buffer)
{
  gfloat *bound, fine_tune, fm_strength;
  gboolean with_fine_tune;

  fine_tune = gsl_cent_factor (fm->fine_tune);
  with_fine_tune = fm->fine_tune != 0;
  fm_strength = fm->fm_strength;
  
  bound = fm_buffer + n_values;
  if (ifreq && ifmod)
    {
      if (fm->exponential_fm)
	{
	  if (with_fine_tune)
	    do {
	      *fm_buffer++ = *ifreq++ * gsl_approx_exp2 (fm_strength * *ifmod++) * fine_tune;
	    } while (fm_buffer < bound);
	  else
	    do {
	      *fm_buffer++ = *ifreq++ * gsl_approx_exp2 (fm_strength * *ifmod++);
	    } while (fm_buffer < bound);
	}
      else
	{
	  if (with_fine_tune)
	    do {
	      *fm_buffer++ = *ifreq++ * (1 + fm_strength * *ifmod++) * fine_tune;
	    } while (fm_buffer < bound);
	  else
	    do {
	      *fm_buffer++ = *ifreq++ * (1 + fm_strength * *ifmod++);
	    } while (fm_buffer < bound);
	}
    }
  else if (ifmod)
    {
      gfloat signal_freq = fm->signal_freq * fine_tune;

      if (fm->exponential_fm)
	do {
	  *fm_buffer++ = signal_freq * gsl_approx_exp2 (fm_strength * *ifmod++);
	} while (fm_buffer < bound);
      else
	do {
	  *fm_buffer++ = signal_freq * (1 + fm_strength * *ifmod++);
	} while (fm_buffer < bound);
    }
  else if (ifreq)
    {
      if (with_fine_tune)
	do {
	  *fm_buffer++ = *ifreq++ * fine_tune;
	} while (fm_buffer < bound);
      else
	do {
	  *fm_buffer++ = *ifreq++;
	} while (fm_buffer < bound);
    }
  else
    {
      gfloat signal_freq = fm->signal_freq * fine_tune;

      do {
	*fm_buffer++ = signal_freq;
      } while (fm_buffer < bound);
    }
}


/* --- windows --- */
double
gsl_window_bartlett (double x)	/* triangle */
{
  if (fabs (x) > 1)
    return 0;

  return 1.0 - fabs (x);
}

double
gsl_window_blackman (double x)
{
  if (fabs (x) > 1)
    return 0;

  return 0.42 + 0.5 * cos (GSL_PI * x) + 0.08 * cos (2.0 * GSL_PI * x);
}

double
gsl_window_cos (double x)	/* von Hann window */
{
  if (fabs (x) > 1)
    return 0;

  return 0.5 * cos (x * GSL_PI) + 0.5;
}

double
gsl_window_hamming (double x)	/* sharp (rectangle) cutoffs at boundaries */
{
  if (fabs (x) > 1)
    return 0;

  return 0.54 + 0.46 * cos (GSL_PI * x);
}

double
gsl_window_sinc (double x)	/* noramlied C. Lanczos window */
{
  if (fabs (x) > 1)
    return 0;
  x = x * GSL_PI;
  if (fabs (x) < 1e-12)
    return 1.0;
  else
    return sin (x) / x;
}

double
gsl_window_rect (double x)	/* a square */
{
  if (fabs (x) > 1)
    return 0;
  return 1.0;
}

/*
cos_roll_off(x)= x>fh?0:x<fl?1:cos(pi/2.*((fl-x)/(fh-fl))) 
*/


/* --- cents & init --- */
const gdouble *gsl_cent_table = NULL;
#define GSL_2_RAISED_TO_1_OVER_1200_d     ( /* 2^(1/1200) */ \
              1.0005777895065548488418016859213821589946746826171875)
void
_gsl_init_signal (void)
{
  static gdouble cent_table_space[201];
  gint i;

  /* cent table initialization,
   * allow negative indexing within [-100..+100]
   */
  gsl_cent_table = cent_table_space + 100;
  for (i = -100; i <= 100; i++)
    cent_table_space[100 + i] = pow (GSL_2_RAISED_TO_1_OVER_1200_d, i);
}


/* --- gsl_approx_atan1() --- */
double
gsl_approx_atan1_prescale (double boost_amount)
{
  double max_boost_factor = 100;	/* atan1(x*100) gets pretty close to 1 for x=1 */
  double recip_tan_1_div_0_75 = 0.24202942695518667705824990442766; /* 1/tan(1/0.75) */
  double scale;

  g_return_val_if_fail (boost_amount >= 0 && boost_amount <= 1.0, 1.0);

  /* scale boost_amount from [0..1] to -1..1 */
  boost_amount = boost_amount * 2 - 1.0;

  /* prescale factor for atan1(x*prescale), ranges from 1/max_boost_factor..max_boost_factor */
  scale = pow (max_boost_factor, tan (boost_amount / 0.75) * recip_tan_1_div_0_75);

  return scale;
}


/* --- exp2f() approximation taylor coefficients finder --- */
#if 0
#include <stdio.h>
double
exp2coeff (int n)
{
  double r = 1;
  int i;

  for (i = 1; i <= n; i++)
    {
      r *= GSL_LN2;
      r /= i;
    }
  return r;
}
/* generate taylor coefficients */
int
main (int   argc,
      char *argv[])
{
  int i;

  for (i = 0; i < 20; i++)
    printf ("#define EXP2_TAYLOR_COEFF_%u\t(%.40f)\n", i, exp2coeff (i));

  return 0;
}
/* test/bench program */
#define _GNU_SOURCE
#include <math.h>
int
main (int   argc,
      char *argv[])
{
  double x, dummy = 0, l = 4;

  if (1)	/* print errors */
    for (x = -3; x < 3.01; x += 0.1)
      {
	g_print ("%+f %+1.20f \t (%.20f - %.20f)\n",
		 x, exp (x * GSL_LN2) - gsl_signal_exp2 (x),
		 exp (x * GSL_LN2), gsl_signal_exp2 (x));
      }

  if (0)	/* bench test */
    for (x = -l; x < l; x += 0.000001)
      {
	dummy += gsl_signal_exp2 (x);
	// dummy += exp2f (x);
      }

  g_print ("%f\r                            \n", dummy);

  return 0;
}
#endif  /* coeff generation */

