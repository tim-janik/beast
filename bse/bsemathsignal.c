/* GSL - Generic Sound Layer
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
#include "gslsignal.h"


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
gsl_window_rectangle (double x)	/* a square */
{
  if (fabs (x) > 1)
    return 0;
  return 1.0;
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

