/* GSL - Generic Sound Layer
 * Copyright (C) 2001-2002 Stefan Westerfeld and Tim Janik
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
#include "gslfilter.h"

#include "gslfft.h"
#include "gslsignal.h"

#include <string.h>


/* --- common utilities --- */
static inline double
cotan (double x)
{
  return - tan (x + GSL_PI * 0.5);
}

static double
gsl_db_invert (double x)
{
  /* db = 20*log(x)/log(10); */
  return exp (x * log (10) / 20.0);
}

static void
band_filter_common (unsigned int iorder,
		    double       p_freq, /* 0..pi */
		    double       s_freq, /* 0..pi */
		    double       epsilon,
		    GslComplex  *roots,
		    GslComplex  *poles,
		    double      *a,      /* [0..iorder] */
		    double      *b,
		    gboolean     band_pass,
		    gboolean     t1_norm)
{
  unsigned int iorder2 = iorder >> 1;
  GslComplex *poly = g_newa (GslComplex, iorder + 1);
  GslComplex fpoly[2 + 1] = { { 0, }, { 0, }, { 1, 0 } };
  double alpha, norm;
  guint i;
  
  epsilon = gsl_trans_zepsilon2ss (epsilon);
  alpha = cos ((s_freq + p_freq) * 0.5) / cos ((s_freq - p_freq) * 0.5);
  
  fpoly[0] = gsl_complex (1, 0);
  fpoly[1] = gsl_complex (1, 0);
  for (i = 0; i < iorder2; i++)
    {
      fpoly[0] = gsl_complex_mul (fpoly[0], gsl_complex_sub (gsl_complex (1, 0), gsl_complex_reciprocal (roots[i])));
      fpoly[1] = gsl_complex_mul (fpoly[1], gsl_complex_sub (gsl_complex (1, 0), gsl_complex_reciprocal (poles[i])));
    }
  norm = gsl_complex_div (fpoly[1], fpoly[0]).re;
  
  if ((iorder2 & 1) == 0)      /* norm is fluctuation minimum */
    norm *= sqrt (1.0 / (1.0 + epsilon * epsilon));
  
  /* z nominator polynomial */
  poly[0] = gsl_complex (norm, 0);
  for (i = 0; i < iorder2; i++)
    {
      GslComplex t, alphac = gsl_complex (alpha, 0);
      
      t = band_pass ? gsl_complex_inv (roots[i]) : roots[i];
      fpoly[1] = gsl_complex_sub (gsl_complex_div (alphac, t), alphac);
      fpoly[0] = gsl_complex_inv (gsl_complex_reciprocal (t));
      gsl_cpoly_mul (poly, i * 2, poly, 2, fpoly);
    }
  for (i = 0; i <= iorder; i++)
    a[i] = poly[i].re;
  
  /* z denominator polynomial */
  poly[0] = gsl_complex (1, 0);
  for (i = 0; i < iorder2; i++)
    {
      GslComplex t, alphac = gsl_complex (alpha, 0);
      
      t = band_pass ? gsl_complex_inv (poles[i]) : poles[i];
      fpoly[1] = gsl_complex_sub (gsl_complex_div (alphac, t), alphac);
      fpoly[0] = gsl_complex_inv (gsl_complex_reciprocal (t));
      gsl_cpoly_mul (poly, i * 2, poly, 2, fpoly);
    }
  for (i = 0; i <= iorder; i++)
    b[i] = poly[i].re;
  gsl_poly_scale (iorder, a, 1.0 / b[0]);
  gsl_poly_scale (iorder, b, 1.0 / b[0]);
}

static void
filter_rp_to_z (unsigned int iorder,
		GslComplex  *roots, /* [0..iorder-1] */
		GslComplex  *poles,
		double      *a,     /* [0..iorder] */
		double      *b)
{
  GslComplex *poly = g_newa (GslComplex, iorder + 1);
  guint i;
  
  /* z nominator polynomial */
  poly[0] = gsl_complex (1, 0);
  for (i = 0; i < iorder; i++)
    gsl_cpoly_mul_reciprocal (i + 1, poly, roots[i]);
  for (i = 0; i <= iorder; i++)
    a[i] = poly[i].re;

  /* z denominator polynomial */
  poly[0] = gsl_complex (1, 0);
  for (i = 0; i < iorder; i++)
    gsl_cpoly_mul_reciprocal (i + 1, poly, poles[i]);
  for (i = 0; i <= iorder; i++)
    b[i] = poly[i].re;
}

static void
filter_lp_invert (unsigned int iorder,
		  double      *a,     /* [0..iorder] */
		  double      *b)
{
  guint i;

  for (i = 1; i <= iorder; i +=2)
    {
      a[i] = -a[i];
      b[i] = -b[i];
    }
}


/* --- butterworth filter --- */
void
gsl_filter_butter_rp (unsigned int iorder,
		      double       freq, /* 0..pi */
		      double       epsilon,
		      GslComplex  *roots,    /* [0..iorder-1] */
		      GslComplex  *poles)
{
  double pi = GSL_PI, order = iorder;
  double beta_mul = pi / (2.0 * order);
  /* double kappa = gsl_trans_freq2s (freq); */
  double kappa;
  GslComplex root;
  unsigned int i;

  epsilon = gsl_trans_zepsilon2ss (epsilon);
  kappa = gsl_trans_freq2s (freq) * pow (epsilon, -1.0 / order);

  /* construct poles for butterworth filter */
  for (i = 1; i <= iorder; i++)
    {
      double t = (i << 1) + iorder - 1;
      double beta = t * beta_mul;

      root.re = kappa * cos (beta);
      root.im = kappa * sin (beta);
      poles[i - 1] = gsl_trans_s2z (root);
    }

  /* z nominator polynomial */
  for (i = 0; i < iorder; i++)
    roots[i] = gsl_complex (-1, 0);
}


/* --- tschebyscheff type 1 filter --- */
static double
tschebyscheff_eval (unsigned int degree,
		    double       x)
{
  double td = x, td_m_1 = 1;
  unsigned int d = 1;

  /* eval polynomial for a certain x */
  if (degree == 0)
    return 1;

  while (d < degree)
    {
      double td1 = 2 * x * td - td_m_1;

      td_m_1 = td;
      td = td1;
      d++;
    }
  return td;
}

static double
tschebyscheff_inverse (unsigned int degree,
		       double       x)
{
  /* note, that thebyscheff_eval(degree,x)=cosh(degree*acosh(x)) */
  return cosh (acosh (x) / degree);
}

void
gsl_filter_tscheb1_rp (unsigned int iorder,
		       double       freq,  /* 1..pi */
		       double       epsilon,
		       GslComplex  *roots, /* [0..iorder-1] */
		       GslComplex  *poles)
{
  double pi = GSL_PI, order = iorder;
  double alpha;
  double beta_mul = pi / (2.0 * order);
  double kappa = gsl_trans_freq2s (freq);
  GslComplex root;
  unsigned int i;

  epsilon = gsl_trans_zepsilon2ss (epsilon);
  alpha = asinh (1.0 / epsilon) / order;

  /* construct poles polynomial from tschebyscheff polynomial */
  for (i = 1; i <= iorder; i++)
    {
      double t = (i << 1) + iorder - 1;
      double beta = t * beta_mul;

      root.re = kappa * sinh (alpha) * cos (beta);
      root.im = kappa * cosh (alpha) * sin (beta);
      poles[i - 1] = gsl_trans_s2z (root);
    }

  /* z nominator polynomial */
  for (i = 0; i < iorder; i++)
    roots[i] = gsl_complex (-1, 0);
}


/* --- tschebyscheff type 2 filter --- */
void
gsl_filter_tscheb2_rp (unsigned int iorder,
		       double       c_freq, /* 1..pi */
		       double       steepness,
		       double       epsilon,
		       GslComplex  *roots,  /* [0..iorder-1] */
		       GslComplex  *poles)
{
  double pi = GSL_PI, order = iorder;
  double r_freq = c_freq * steepness;
  double kappa_c = gsl_trans_freq2s (c_freq);
  double kappa_r = gsl_trans_freq2s (r_freq);
  double tepsilon;
  double alpha;
  double beta_mul = pi / (2.0 * order);
  GslComplex root;
  unsigned int i;

#if 0
  /* triggers an internal compiler error with gcc-2.95.4 (and certain
   * combinations of optimization options)
   */
  g_return_if_fail (c_freq * steepness < GSL_PI);
#endif
  g_return_if_fail (steepness > 1.0);

  epsilon = gsl_trans_zepsilon2ss (epsilon);
  tepsilon = epsilon * tschebyscheff_eval (iorder, kappa_r / kappa_c);
  alpha = asinh (tepsilon) / order;
  
  /* construct poles polynomial from tschebyscheff polynomial */
  for (i = 1; i <= iorder; i++)
    {
      double t = (i << 1) + iorder - 1;
      double beta = t * beta_mul;
      
      root.re = sinh (alpha) * cos (beta);
      root.im = cosh (alpha) * sin (beta);
      root = gsl_complex_div (gsl_complex (kappa_r, 0), root);
      root = gsl_trans_s2z (root);
      poles[i - 1] = root;
    }
  
  /* construct roots polynomial from tschebyscheff polynomial */
  for (i = 1; i <= iorder; i++)
    {
      double t = (i << 1) - 1;
      GslComplex root = gsl_complex (0, cos (t * beta_mul));
      
      if (fabs (root.im) > 1e-14)
	{
	  root = gsl_complex_div (gsl_complex (kappa_r, 0), root);
	  root = gsl_trans_s2z (root);
	}
      else
	root = gsl_complex (-1, 0);
      roots[i - 1] = root;
    }
}

/**
 * gsl_filter_tscheb2_steepness_db
 * @iorder:      filter order
 * @c_freq:      passband cutoff frequency (0..pi)
 * @epsilon:     fall off at passband frequency (0..1)
 * @stopband_db: reduction in stopband in dB (>= 0)
 * Calculates the steepness parameter for Tschebyscheff type 2 lowpass filter,
 * based on the ripple residue in the stop band.
 */
double
gsl_filter_tscheb2_steepness_db (unsigned int iorder,
				 double       c_freq,
				 double       epsilon,
				 double       stopband_db)
{
  return gsl_filter_tscheb2_steepness (iorder, c_freq, epsilon, gsl_db_invert (-stopband_db));
}

/**
 * gsl_filter_tscheb2_steepness
 * @iorder:    filter order
 * @c_freq:    passband cutoff frequency (0..pi)
 * @epsilon:   fall off at passband frequency (0..1)
 * @residue:   maximum of transfer function in stopband (0..1)
 * Calculates the steepness parameter for Tschebyscheff type 2 lowpass filter,
 * based on ripple residue in the stop band.
 */
double
gsl_filter_tscheb2_steepness (unsigned int iorder,
			      double       c_freq,
			      double       epsilon,
			      double       residue)
{
  double kappa_c, kappa_r, r_freq;

  epsilon = gsl_trans_zepsilon2ss (epsilon);
  kappa_c = gsl_trans_freq2s (c_freq);
  kappa_r = tschebyscheff_inverse (iorder, sqrt (1.0 / (residue * residue) - 1.0) / epsilon) * kappa_c;
  r_freq = gsl_trans_freq2z (kappa_r);

  return r_freq / c_freq;
}


/* --- lowpass filters --- */
/**
 * gsl_filter_butter_lp
 * @iorder:   filter order
 * @freq:     cutoff frequency (0..pi)
 * @epsilon:  fall off at cutoff frequency (0..1)
 * @a:        root polynomial coefficients a[0..iorder]
 * @b:        pole polynomial coefficients b[0..iorder]
 * Butterworth lowpass filter.
 */
void
gsl_filter_butter_lp (unsigned int iorder,
		      double       freq, /* 0..pi */
		      double       epsilon,
		      double      *a,    /* [0..iorder] */
		      double      *b)
{
  GslComplex *roots = g_newa (GslComplex, iorder);
  GslComplex *poles = g_newa (GslComplex, iorder);
  double norm;
  
  g_return_if_fail (freq > 0 && freq < GSL_PI);

  gsl_filter_butter_rp (iorder, freq, epsilon, roots, poles);
  filter_rp_to_z (iorder, roots, poles, a, b);

  /* scale maximum to 1.0 */
  norm = gsl_poly_eval (iorder, b, 1) / gsl_poly_eval (iorder, a, 1);
  gsl_poly_scale (iorder, a, norm);
}

/**
 * gsl_filter_tscheb1_lp
 * @iorder:   filter order
 * @freq:     cutoff frequency (0..pi)
 * @epsilon:  fall off at cutoff frequency (0..1)
 * @a:        root polynomial coefficients a[0..iorder]
 * @b:        pole polynomial coefficients b[0..iorder]
 * Tschebyscheff type 1 lowpass filter.
 */
void
gsl_filter_tscheb1_lp (unsigned int iorder,
		       double       freq, /* 0..pi */
		       double       epsilon,
		       double      *a,    /* [0..iorder] */
		       double      *b)
{
  GslComplex *roots = g_newa (GslComplex, iorder);
  GslComplex *poles = g_newa (GslComplex, iorder);
  double norm;

  g_return_if_fail (freq > 0 && freq < GSL_PI);

  gsl_filter_tscheb1_rp (iorder, freq, epsilon, roots, poles);
  filter_rp_to_z (iorder, roots, poles, a, b);

  /* scale maximum to 1.0 */
  norm = gsl_poly_eval (iorder, b, 1) / gsl_poly_eval (iorder, a, 1);
  if ((iorder & 1) == 0)      /* norm is fluctuation minimum */
    {
      epsilon = gsl_trans_zepsilon2ss (epsilon);
      norm *= sqrt (1.0 / (1.0 + epsilon * epsilon));
    }
  gsl_poly_scale (iorder, a, norm);
}

/**
 * gsl_filter_tscheb2_lp
 * @iorder:    filter order
 * @freq:      passband cutoff frequency (0..pi)
 * @steepness: frequency steepness (c_freq * steepness < pi)
 * @epsilon:   fall off at passband frequency (0..1)
 * @a:         root polynomial coefficients a[0..iorder]
 * @b:         pole polynomial coefficients b[0..iorder]
 * Tschebyscheff type 2 lowpass filter.
 * To gain a transition band between freq1 and freq2, pass arguements
 * @freq=freq1 and @steepness=freq2/freq1. To specify the transition
 * band width in fractions of octaves, pass @steepness=2^octave_fraction.
 */
void
gsl_filter_tscheb2_lp (unsigned int iorder,
		       double       freq,   /* 0..pi */
		       double       steepness,
		       double       epsilon,
		       double      *a,      /* [0..iorder] */
		       double      *b)
{
  GslComplex *roots = g_newa (GslComplex, iorder);
  GslComplex *poles = g_newa (GslComplex, iorder);
  double norm;

  g_return_if_fail (freq > 0 && freq < GSL_PI);
  g_return_if_fail (freq * steepness < GSL_PI);
  g_return_if_fail (steepness > 1.0);

  gsl_filter_tscheb2_rp (iorder, freq, steepness, epsilon, roots, poles);
  filter_rp_to_z (iorder, roots, poles, a, b);
  
  /* scale maximum to 1.0 */
  norm = gsl_poly_eval (iorder, b, 1) / gsl_poly_eval (iorder, a, 1); /* H(z=0):=1, e^(j*omega) for omega=0 => e^0==1 */
  gsl_poly_scale (iorder, a, norm);
}


/* --- highpass filters --- */
/**
 * gsl_filter_butter_hp
 * @iorder:   filter order
 * @freq:     passband frequency (0..pi)
 * @epsilon:  fall off at passband frequency (0..1)
 * @a:        root polynomial coefficients a[0..iorder]
 * @b:        pole polynomial coefficients b[0..iorder]
 * Butterworth highpass filter.
 */
void
gsl_filter_butter_hp (unsigned int iorder,
		      double       freq, /* 0..pi */
		      double       epsilon,
		      double      *a,    /* [0..iorder] */
		      double      *b)
{
  g_return_if_fail (freq > 0 && freq < GSL_PI);

  freq = GSL_PI - freq;
  gsl_filter_butter_lp (iorder, freq, epsilon, a, b);
  filter_lp_invert (iorder, a, b);
}

/**
 * gsl_filter_tscheb1_hp
 * @iorder:   filter order
 * @freq:     passband frequency (0..pi)
 * @epsilon:  fall off at passband frequency (0..1)
 * @a:        root polynomial coefficients a[0..iorder]
 * @b:        pole polynomial coefficients b[0..iorder]
 * Tschebyscheff type 1 highpass filter.
 */
void
gsl_filter_tscheb1_hp (unsigned int iorder,
		       double       freq, /* 0..pi */
		       double       epsilon,
		       double      *a,    /* [0..iorder] */
		       double      *b)
{
  g_return_if_fail (freq > 0 && freq < GSL_PI);

  freq = GSL_PI - freq;
  gsl_filter_tscheb1_lp (iorder, freq, epsilon, a, b);
  filter_lp_invert (iorder, a, b);
}

/**
 * gsl_filter_tscheb2_hp
 * @iorder:    filter order
 * @freq:      stopband frequency (0..pi)
 * @steepness: frequency steepness
 * @epsilon:   fall off at passband frequency (0..1)
 * @a:         root polynomial coefficients a[0..iorder]
 * @b:         pole polynomial coefficients b[0..iorder]
 * Tschebyscheff type 2 highpass filter.
 */
void
gsl_filter_tscheb2_hp   (unsigned int iorder,
			 double       freq,
			 double       steepness,
			 double       epsilon,
			 double      *a,      /* [0..iorder] */
			 double      *b)
{
  g_return_if_fail (freq > 0 && freq < GSL_PI);

  freq = GSL_PI - freq;
  gsl_filter_tscheb2_lp (iorder, freq, steepness, epsilon, a, b);
  filter_lp_invert (iorder, a, b);
}


/* --- bandpass filters --- */
/**
 * gsl_filter_butter_bp
 * @iorder:   filter order (must be even)
 * @freq1:    stopband end frequency (0..pi)
 * @freq2:    passband end frequency (0..pi)
 * @epsilon:  fall off at passband frequency (0..1)
 * @a:        root polynomial coefficients a[0..iorder]
 * @b:        pole polynomial coefficients b[0..iorder]
 * Butterworth bandpass filter.
 */
void
gsl_filter_butter_bp (unsigned int iorder,
		      double       freq1, /* 0..pi */
		      double       freq2, /* 0..pi */
		      double       epsilon,
		      double      *a,      /* [0..iorder] */
		      double      *b)
{
  unsigned int iorder2 = iorder >> 1;
  GslComplex *roots = g_newa (GslComplex, iorder2);
  GslComplex *poles = g_newa (GslComplex, iorder2);
  double theta;

  g_return_if_fail ((iorder & 0x01) == 0);
  g_return_if_fail (freq1 > 0);
  g_return_if_fail (freq1 < freq2);
  g_return_if_fail (freq2 < GSL_PI);

  theta = 2. * atan2 (1., cotan ((freq2 - freq1) * 0.5));

  gsl_filter_butter_rp (iorder2, theta, epsilon, roots, poles);
  band_filter_common (iorder, freq1, freq2, epsilon, roots, poles, a, b, TRUE, FALSE);
}

/**
 * gsl_filter_tscheb1_bp
 * @iorder:   filter order (must be even)
 * @freq1:    stopband end frequency (0..pi)
 * @freq2:    passband end frequency (0..pi)
 * @epsilon:  fall off at passband frequency (0..1)
 * @a:        root polynomial coefficients a[0..iorder]
 * @b:        pole polynomial coefficients b[0..iorder]
 * Tschebyscheff type 1 bandpass filter.
 */
void
gsl_filter_tscheb1_bp (unsigned int iorder,
		       double       freq1, /* 0..pi */
		       double       freq2, /* 0..pi */
		       double       epsilon,
		       double      *a,      /* [0..iorder] */
		       double      *b)
{
  unsigned int iorder2 = iorder >> 1;
  GslComplex *roots = g_newa (GslComplex, iorder2);
  GslComplex *poles = g_newa (GslComplex, iorder2);
  double theta;

  g_return_if_fail ((iorder & 0x01) == 0);
  g_return_if_fail (freq1 > 0);
  g_return_if_fail (freq1 < freq2);
  g_return_if_fail (freq2 < GSL_PI);
  
  theta = 2. * atan2 (1., cotan ((freq2 - freq1) * 0.5));

  gsl_filter_tscheb1_rp (iorder2, theta, epsilon, roots, poles);
  band_filter_common (iorder, freq1, freq2, epsilon, roots, poles, a, b, TRUE, TRUE);
}

/**
 * gsl_filter_tscheb2_bp
 * @iorder:    filter order (must be even)
 * @freq1:     stopband end frequency (0..pi)
 * @freq2:     passband end frequency (0..pi)
 * @steepness: frequency steepness factor
 * @epsilon:   fall off at passband frequency (0..1)
 * @a:         root polynomial coefficients a[0..iorder]
 * @b:         pole polynomial coefficients b[0..iorder]
 * Tschebyscheff type 2 bandpass filter.
 */
void
gsl_filter_tscheb2_bp (unsigned int iorder,
		       double       freq1, /* 0..pi */
		       double       freq2, /* 0..pi */
		       double       steepness,
		       double       epsilon,
		       double      *a,      /* [0..iorder] */
		       double      *b)
{
  unsigned int iorder2 = iorder >> 1;
  GslComplex *roots = g_newa (GslComplex, iorder2);
  GslComplex *poles = g_newa (GslComplex, iorder2);
  double theta;

  g_return_if_fail ((iorder & 0x01) == 0);
  g_return_if_fail (freq1 > 0);
  g_return_if_fail (freq1 < freq2);
  g_return_if_fail (freq2 < GSL_PI);
  
  theta = 2. * atan2 (1., cotan ((freq2 - freq1) * 0.5));

  gsl_filter_tscheb2_rp (iorder2, theta, steepness, epsilon, roots, poles);
  band_filter_common (iorder, freq1, freq2, epsilon, roots, poles, a, b, TRUE, FALSE);
}


/* --- bandstop filters --- */
/**
 * gsl_filter_butter_bs
 * @iorder:   filter order (must be even)
 * @freq1:    passband end frequency (0..pi)
 * @freq2:    stopband end frequency (0..pi)
 * @epsilon:  fall off at passband frequency (0..1)
 * @a:        root polynomial coefficients a[0..iorder]
 * @b:        pole polynomial coefficients b[0..iorder]
 * Butterworth bandstop filter.
 */
void
gsl_filter_butter_bs (unsigned int iorder,
		      double       freq1, /* 0..pi */
		      double       freq2, /* 0..pi */
		      double       epsilon,
		      double      *a,      /* [0..iorder] */
		      double      *b)
{
  unsigned int iorder2 = iorder >> 1;
  GslComplex *roots = g_newa (GslComplex, iorder2);
  GslComplex *poles = g_newa (GslComplex, iorder2);
  double theta;

  g_return_if_fail ((iorder & 0x01) == 0);
  g_return_if_fail (freq1 > 0);
  g_return_if_fail (freq1 < freq2);
  g_return_if_fail (freq2 < GSL_PI);

  theta = 2. * atan2 (1., tan ((freq2 - freq1) * 0.5));

  gsl_filter_butter_rp (iorder2, theta, epsilon, roots, poles);
  band_filter_common (iorder, freq1, freq2, epsilon, roots, poles, a, b, FALSE, FALSE);
}

/**
 * gsl_filter_tscheb1_bs
 * @iorder:   filter order (must be even)
 * @freq1:    passband end frequency (0..pi)
 * @freq2:    stopband end frequency (0..pi)
 * @epsilon:  fall off at passband frequency (0..1)
 * @a:        root polynomial coefficients a[0..iorder]
 * @b:        pole polynomial coefficients b[0..iorder]
 * Tschebyscheff type 1 bandstop filter.
 */
void
gsl_filter_tscheb1_bs (unsigned int iorder,
		       double       freq1, /* 0..pi */
		       double       freq2, /* 0..pi */
		       double       epsilon,
		       double      *a,      /* [0..iorder] */
		       double      *b)
{
  unsigned int iorder2 = iorder >> 1;
  GslComplex *roots = g_newa (GslComplex, iorder2);
  GslComplex *poles = g_newa (GslComplex, iorder2);
  double theta;

  g_return_if_fail ((iorder & 0x01) == 0);
  g_return_if_fail (freq1 > 0);
  g_return_if_fail (freq1 < freq2);
  g_return_if_fail (freq2 < GSL_PI);
  
  theta = 2. * atan2 (1., tan ((freq2 - freq1) * 0.5));

  gsl_filter_tscheb1_rp (iorder2, theta, epsilon, roots, poles);
  band_filter_common (iorder, freq1, freq2, epsilon, roots, poles, a, b, FALSE, TRUE);
}

/**
 * gsl_filter_tscheb2_bs
 * @iorder:    filter order (must be even)
 * @freq1:     passband end frequency (0..pi)
 * @freq2:     stopband end frequency (0..pi)
 * @steepness: frequency steepness factor
 * @epsilon:   fall off at passband frequency (0..1)
 * @a:         root polynomial coefficients a[0..iorder]
 * @b:         pole polynomial coefficients b[0..iorder]
 * Tschebyscheff type 2 bandstop filter.
 */
void
gsl_filter_tscheb2_bs (unsigned int iorder,
		       double       freq1, /* 0..pi */
		       double       freq2, /* 0..pi */
		       double       steepness,
		       double       epsilon,
		       double      *a,      /* [0..iorder] */
		       double      *b)
{
  unsigned int iorder2 = iorder >> 1;
  GslComplex *roots = g_newa (GslComplex, iorder2);
  GslComplex *poles = g_newa (GslComplex, iorder2);
  double theta;

  g_return_if_fail ((iorder & 0x01) == 0);
  g_return_if_fail (freq1 > 0);
  g_return_if_fail (freq1 < freq2);
  g_return_if_fail (freq2 < GSL_PI);
  
  theta = 2. * atan2 (1., tan ((freq2 - freq1) * 0.5));

  gsl_filter_tscheb2_rp (iorder2, theta, steepness, epsilon, roots, poles);
  band_filter_common (iorder, freq1, freq2, epsilon, roots, poles, a, b, FALSE, FALSE);
}


/* --- tschebyscheff type 1 via generic root-finding --- */
#if 0
static void
tschebyscheff_poly (unsigned int degree,
		    double      *v)
{
  /* construct all polynomial koefficients */
  if (degree == 0)
    v[0] = 1;
  else if (degree == 1)
    {
      v[1] = 1; v[0] = 0;
    }
  else
    {
      double *u = g_newa (double, 1 + degree);
      
      u[degree] = 0; u[degree - 1] = 0;
      tschebyscheff_poly (degree - 2, u);
      
      v[0] = 0;
      tschebyscheff_poly (degree - 1, v + 1);
      gsl_poly_scale (degree - 1, v + 1, 2);
      
      gsl_poly_sub (degree, v, u);
    }
}

static void
gsl_filter_tscheb1_test	(unsigned int iorder,
			 double       zomega,
			 double       epsilon,
			 double      *a,    /* [0..iorder] */
			 double      *b)
{
  GslComplex *roots = g_newa (GslComplex, iorder * 2), *r;
  GslComplex *zf = g_newa (GslComplex, 1 + iorder);
  double *vk = g_newa (double, 1 + iorder), norm;
  double *q = g_newa (double, 2 * (1 + iorder));
  double O = gsl_trans_freq2s (zomega);
  unsigned int i;
  
  /* calc Vk() */
  tschebyscheff_poly (iorder, vk);
  
  /* calc q=1+e^2*Vk()^2 */
  gsl_poly_mul (q, iorder >> 1, vk, iorder >> 1, vk);
  iorder *= 2;
  gsl_poly_scale (iorder, q, epsilon * epsilon);
  q[0] += 1;

  /* find roots, fix roots by 1/(jO) */
  gsl_poly_complex_roots (iorder, q, roots);
  for (i = 0; i < iorder; i++)
    roots[i] = gsl_complex_mul (roots[i], gsl_complex (0, O));
  
  /* choose roots from the left half-plane */
  if (0)
    g_print ("zhqr-root:\n%s\n", gsl_complex_list (iorder, roots, "  "));
  r = roots;
  for (i = 0; i < iorder; i++)
    if (roots[i].re < 0)
      {
	r->re = roots[i].re;
	r->im = roots[i].im;
	r++;
      }
  iorder /= 2;
  
  /* assert roots found */
  if (!(r - roots == iorder))
    {
      g_print ("ERROR: n_roots=%u != iorder=%u\n", r - roots, iorder);
      abort ();
    }
  
  /* s => z */
  for (i = 0; i < iorder; i++)
    roots[i] = gsl_trans_s2z (roots[i]);
  
  /* z denominator polynomial */
  gsl_cpoly_from_roots (iorder, zf, roots);
  for (i = 0; i <= iorder; i++)
    b[i] = zf[i].re;
  
  /* z nominator polynomial */
  for (i = 0; i < iorder; i++)
    {
      roots[i].re = -1;
      roots[i].im = 0;
    }
  gsl_cpoly_from_roots (iorder, zf, roots);
  for (i = 0; i <= iorder; i++)
    a[i] = zf[i].re;
  
  /* scale for b[0]==1.0 */
  gsl_poly_scale (iorder, b, 1.0 / b[0]);

  /* scale maximum to 1.0 */
  norm = gsl_poly_eval (iorder, a, 1) / gsl_poly_eval (iorder, b, 1);
  if ((iorder & 0x01) == 0)	/* norm is fluctuation minimum */
    norm /= sqrt (1.0 / (1.0 + epsilon * epsilon));
  gsl_poly_scale (iorder, a, 1.0 / norm);
}
#endif


/* --- windowed fir approximation --- */
/* returns a blackman window: x is supposed to be in the interval [0..1] */
static inline double
gsl_blackman_window (double x)
{
  if (x < 0)
    return 0;
  if (x > 1)
    return 0;
  return 0.42 - 0.5 * cos (GSL_PI * x * 2) + 0.08 * cos (4 * GSL_PI * x);
}

/**
 * gsl_filter_fir_approx
 *
 * @iorder: order of the filter (must be oven, >= 2)
 * @freq:   the frequencies of the transfer function
 * @value:  the desired value of the transfer function
 *
 * Approximates a given transfer function with an iorder-coefficient FIR filter.
 * It is recommended to provide enough frequency values, so that
 * @n_points >= @iorder.
 */
void
gsl_filter_fir_approx (unsigned int  iorder,
		       double       *a,	/* [0..iorder] */
		       unsigned int  n_points,
		       const double *freq,
		       const double *value)
{
  /* TODO:
   *
   * a) does fft_size matter for the quality of the approximation? i.e. do
   *    larger fft_sizes produce better filters?
   * b) generalize windowing
   */
  unsigned int fft_size = 8;
  unsigned int point = 0, i;
  double lfreq = -2, lval = 1.0, rfreq = -1, rval = 1.0;
  double *fft_in, *fft_out;
  double ffact;
  
  g_return_if_fail (iorder >= 2);
  g_return_if_fail ((iorder & 1) == 0);

  while (fft_size / 2 <= iorder)
    fft_size *= 2;
  
  fft_in = g_newa (double, fft_size*2);
  fft_out = fft_in+fft_size;
  ffact = 2.0 * GSL_PI / (double)fft_size;
  
  for (i = 0; i <= fft_size / 2; i++)
    {
      double f = (double) i * ffact;
      double pos, val;
      
      while (f > rfreq && point != n_points)
	{
	  lfreq = rfreq;
	  rfreq = freq[point];
	  lval = rval;
	  rval = value[point];
	  point++;
	}
      
      pos = (f - lfreq) / (rfreq - lfreq);
      val = lval * (1.0 - pos) + rval * pos;
      
      if (i != fft_size / 2)
	{
	  fft_in[2 * i] = val;
	  fft_in[2 * i + 1] = 0.0;
	}
      else
	fft_in[1] = val;
    }
  
  gsl_power2_fftsr (fft_size, fft_in, fft_out);
  
  for (i = 0; i <= iorder / 2; i++)
    {
      double c = fft_out[i] * gsl_blackman_window (0.5 + (double) i / (iorder + 2.0));
      a[iorder / 2 - i] = c;
      a[iorder / 2 + i] = c;
    }
}


/* --- filter evaluation --- */
void
gsl_iir_filter_setup (GslIIRFilter  *f,
		      guint          order,
		      const gdouble *a,
		      const gdouble *b,
		      gdouble       *buffer) /* 4*(order+1) */
{
  guint i;

  g_return_if_fail (f != NULL && a != NULL && b != NULL && buffer != NULL);
  g_return_if_fail (order > 0);

  f->order = order;
  f->a = buffer;
  f->b = f->a + order + 1;
  f->w = f->b + order + 1;

  memcpy (f->a, a, sizeof (a[0]) * (order + 1));
  for (i = 0; i <= order; i++)
    f->b[i] = -b[i];
  memset (f->w, 0, sizeof (f->w[0]) * (order + 1) * 2);

  g_return_if_fail (fabs (b[0] - 1.0) < 1e-14);
}

void
gsl_iir_filter_change (GslIIRFilter  *f,
		       guint          order,
		       const gdouble *a,
		       const gdouble *b,
		       gdouble       *buffer)
{
  guint i;

  g_return_if_fail (f != NULL && a != NULL && b != NULL && buffer != NULL);
  g_return_if_fail (order > 0);
  
  /* there's no point in calling this function if f wasn't setup properly
   * and it's only the As and Bs that changed
   */
  g_return_if_fail (f->a == buffer && f->b == f->a + f->order + 1 && f->w == f->b + f->order + 1);

  /* if the order changed there's no chance preserving state */
  if (f->order != order)
    {
      gsl_iir_filter_setup (f, order, a, b, buffer);
      return;
    }

  memcpy (f->a, a, sizeof (a[0]) * (order + 1));
  for (i = 0; i <= order; i++)
    f->b[i] = -b[i];
  /* leaving f->w to preserve state */

  g_return_if_fail (fabs (b[0] - 1.0) < 1e-14);
}

static inline gdouble /* Y */
filter_step_direct_canon_2 (GslIIRFilter *f,
			    gdouble       X)
{
  register guint n = f->order;
  gdouble *a = f->a, *b = f->b, *w = f->w;
  gdouble x, y, v;

  v = w[n];
  x = b[n] * v;
  y = a[n] * v;

  while (--n)
    {
      gdouble t1, t2;

      v = w[n];
      t1 = v * b[n];
      t2 = v * a[n];
      w[n+1] = v;
      x += t1;
      y += t2;
    }

  x += X;
  w[1] = x;
  y += x * a[0];
  /* w[0] unused */

  return y;
}

static inline gdouble /* Y */
filter_step_direct_canon_1 (GslIIRFilter *f,
			    gdouble       X)
{
  register guint n = f->order;
  gdouble *a = f->a, *b = f->b, *w = f->w;
  gdouble y, v;

  /* w[n] unused */
  y = X * a[0] + w[0];
  v = X * a[n] + y * b[n];

  while (--n)
    {
      gdouble t = w[n];

      w[n] = v;
      t += X * a[n];
      v = y * b[n];
      v += t;
    }
  w[0] = v;

  return y;
}

#define	filter_step	filter_step_direct_canon_1

void
gsl_iir_filter_eval (GslIIRFilter *f,
		     guint         n_values,
		     const gfloat *x,
		     gfloat       *y)
{
  const gfloat *bound;
  
  g_return_if_fail (f != NULL && x != NULL && y != NULL);
  g_return_if_fail (f->order > 0);

  bound = x + n_values;
  while (x < bound)
    {
      *y = filter_step (f, *x);
      x++;
      y++;
    }
}


/* --- biquad filters --- */
void
gsl_biquad_config_init (GslBiquadConfig   *c,
			GslBiquadType      type,
			GslBiquadNormalize normalize)
{
  g_return_if_fail (c != NULL);

  memset (c, 0, sizeof (*c));
  c->type = type;
  c->normalize = normalize;
  gsl_biquad_config_setup (c, 0.5, 3, 1);
  c->approx_values = TRUE;	/* need _setup() */
}

void
gsl_biquad_config_setup (GslBiquadConfig *c,
			 gfloat           f_fn,
			 gfloat           gain,
			 gfloat           quality)
{
  g_return_if_fail (c != NULL);
  g_return_if_fail (f_fn >= 0 && f_fn <= 1);

  if (c->type == GSL_BIQUAD_RESONANT_HIGHPASS)
    f_fn = 1.0 - f_fn;
  c->f_fn = f_fn;			/* nyquist relative (0=DC, 1=nyquist) */
  c->gain = gain;
  c->quality = quality;			/* FIXME */
  c->k = tan (c->f_fn * GSL_PI / 2.);
  c->v = pow (10, c->gain / 20.);	/* v=10^(gain[dB]/20) */
  c->dirty = TRUE;
  c->approx_values = FALSE;
}

void
gsl_biquad_config_approx_freq (GslBiquadConfig *c,
			       gfloat           f_fn)
{
  g_return_if_fail (f_fn >= 0 && f_fn <= 1);

  if (c->type == GSL_BIQUAD_RESONANT_HIGHPASS)
    f_fn = 1.0 - f_fn;
  c->f_fn = f_fn;                       /* nyquist relative (0=DC, 1=nyquist) */
  c->k = tan (c->f_fn * GSL_PI / 2.);	/* FIXME */
  c->dirty = TRUE;
  c->approx_values = TRUE;
}

void
gsl_biquad_config_approx_gain (GslBiquadConfig *c,
			       gfloat           gain)
{
  c->gain = gain;
  c->v = gsl_approx_exp2 (c->gain * GSL_LOG2POW20_10);
  c->dirty = TRUE;
  c->approx_values = TRUE;
}

static void
biquad_lpreso (GslBiquadConfig *c,
	       GslBiquadFilter *f)
{
  gdouble kk, sqrt2_reso, denominator;
  gdouble r2p_norm = 0;			/* resonance gain to peak gain (pole: -sqrt2_reso+-j) */

  kk = c->k * c->k;
  sqrt2_reso = 1 / c->v;
  denominator = 1 + (c->k + sqrt2_reso) * c->k;

  switch (c->normalize)
    {
    case GSL_BIQUAD_NORMALIZE_PASSBAND:
      r2p_norm = kk;
      break;
    case GSL_BIQUAD_NORMALIZE_RESONANCE_GAIN:
      r2p_norm = kk * sqrt2_reso;
      break;
    case GSL_BIQUAD_NORMALIZE_PEAK_GAIN:
      r2p_norm = (GSL_SQRT2 * sqrt2_reso - 1.0) / (sqrt2_reso * sqrt2_reso - 0.5);
      r2p_norm = r2p_norm > 1 ? kk * sqrt2_reso : kk * r2p_norm * sqrt2_reso;
      break;
    }
  f->xc0 = r2p_norm / denominator;
  f->xc1 = 2 * f->xc0;
  f->xc2 = f->xc0;
  f->yc1 = 2 * (kk - 1) / denominator;
  f->yc2 = (1 + (c->k - sqrt2_reso) * c->k) / denominator;
}

void
gsl_biquad_filter_config (GslBiquadFilter *f,
			  GslBiquadConfig *c,
			  gboolean         reset_state)
{
  g_return_if_fail (f != NULL);
  g_return_if_fail (c != NULL);

  if (c->dirty)
    {
      switch (c->type)
	{
	case GSL_BIQUAD_RESONANT_LOWPASS:
	  biquad_lpreso (c, f);
	  break;
	case GSL_BIQUAD_RESONANT_HIGHPASS:
	  biquad_lpreso (c, f);
	  f->xc1 = -f->xc1;
	  f->yc1 = -f->yc1;
	  break;
	default:
	  g_assert_not_reached ();
	}
      c->dirty = FALSE;
    }

  if (reset_state)
    f->xd1 = f->xd2 = f->yd1 = f->yd2 = 0;
}

void
gsl_biquad_filter_eval (GslBiquadFilter *f,
			guint            n_values,
			const gfloat    *x,
			gfloat          *y)
{
  const gfloat *bound;
  gdouble xc0, xc1, xc2, yc1, yc2, xd1, xd2, yd1, yd2;

  g_return_if_fail (f != NULL && x != NULL && y != NULL);

  xc0 = f->xc0;
  xc1 = f->xc1;
  xc2 = f->xc2;
  yc1 = f->yc1;
  yc2 = f->yc2;
  xd1 = f->xd1;
  xd2 = f->xd2;
  yd1 = f->yd1;
  yd2 = f->yd2;
  bound = x + n_values;
  while (x < bound)
    {
      gdouble k0, k1, k2;

      k2 = xd2 * xc2;
      k1 = xd1 * xc1;
      xd2 = xd1;
      xd1 = *x++;
      k2 -= yd2 * yc2;
      k1 -= yd1 * yc1;
      yd2 = yd1;
      k0 = xd1 * xc0;
      yd1 = k2 + k1;
      *y++ = yd1 += k0;
    }
  f->xd1 = xd1;
  f->xd2 = xd2;
  f->yd1 = yd1;
  f->yd2 = yd2;
}

#if 0
void
gsl_biquad_lphp_reso (GslBiquadFilter   *c,
		      gfloat             f_fn,	/* nyquist relative (0=DC, 1=nyquist) */
		      float              gain,
		      gboolean		 design_highpass,
		      GslBiquadNormalize normalize)
{
  double k, kk, v;
  double sqrt2_reso;
  double denominator;
  double r2p_norm = 0;			/* resonance gain to peak gain (pole: -sqrt2_reso+-j) */

  g_return_if_fail (c != NULL);
  g_return_if_fail (f_fn >= 0 && f_fn <= 1);

  if (design_highpass)
    f_fn = 1.0 - f_fn;

  v = pow (10, gain / 20.);		/* v=10^(gain[dB]/20) */
  k = tan (f_fn * GSL_PI / 2.);
  kk = k * k;
  sqrt2_reso = 1 / v;
  denominator = 1 + (k + sqrt2_reso) * k;

  if (0)
    g_printerr ("BIQUAD-lp: R=%f\n", GSL_SQRT2 * sqrt2_reso);

  switch (normalize)
    {
    case GSL_BIQUAD_NORMALIZE_PASSBAND:
      r2p_norm = kk;
      break;
    case GSL_BIQUAD_NORMALIZE_RESONANCE_GAIN:
      r2p_norm = kk * sqrt2_reso;
      break;
    case GSL_BIQUAD_NORMALIZE_PEAK_GAIN:
      r2p_norm = (GSL_SQRT2 * sqrt2_reso - 1.0) / (sqrt2_reso * sqrt2_reso - 0.5);
      g_print ("BIQUAD-lp: (peak-gain) r2p_norm = %f \n", r2p_norm);
      r2p_norm = r2p_norm > 1 ? kk * sqrt2_reso : kk * r2p_norm * sqrt2_reso;
      break;
    }
  c->xc0 = r2p_norm / denominator;
  c->xc1 = 2 * c->xc0;
  c->xc2 = c->xc0;
  c->yc1 = 2 * (kk - 1) / denominator;
  c->yc2 = (1 + (k - sqrt2_reso) * k) / denominator;

  if (design_highpass)
    {
      c->xc1 = -c->xc1;
      c->yc1 = -c->yc1;
    }
  /* normalization notes:
   * pole: -sqrt2_reso+-j
   * freq=0.5: reso->peak gain=8adjust:0.9799887, 9adjust:0.98415
   * resonance gain = 1/(1-R)=sqrt2_reso
   * sqrt2_reso*(1-R)=1
   * 1-R=1/sqrt2_reso
   * R= 1-1/sqrt2_reso
   * peak gain = 2/(1-R^2)
   * = 2 * (1 - (1 - 1 / sqrt2_reso) * (1 - 1 / sqrt2_reso))
   * = 2 - 2 * (1 - 1 / sqrt2_reso)^2
   */
}
#endif


/* --- filter scanning -- */
#define SINE_SCAN_SIZE 1024

/**
 * gsl_filter_sine_scan
 *
 * @order:    order of the iir filter
 * @a:        root polynomial coefficients of the filter a[0..order]
 * @b:        pole polynomial coefficients of the filter b[0..order]
 * @freq:     frequency to test
 * @n_values: number of samples
 *
 * This function sends a sine signal of the desired frequency through an IIR
 * filter, to test the value of the transfer function at a given point. It uses
 * gsl_iir_filter_eval to do so.
 *
 * Compared to a "mathematical approach" of finding the transfer function,
 * this function makes it possible to see the effects of finite arithmetic
 * during filter evaluation.
 * 
 * The first half of the output signal is not considered, since a lot of IIR
 * filters have a transient phase where also overshoot is possible.
 * 
 * For n_values, you should specify a reasonable large value. It should be
 * a lot larger than the filter order, and large enough to let the input
 * signal become (close to) 1.0 multiple times.
 */
gdouble
gsl_filter_sine_scan (guint order,
                      const gdouble *a,
		      const gdouble *b,
		      gdouble freq,
		      guint n_values)
{
  gfloat x[SINE_SCAN_SIZE], y[SINE_SCAN_SIZE];
  gdouble pos = 0.0;
  gdouble result = 0.0;
  GslIIRFilter filter;
  gdouble *filter_state;
  guint scan_start = n_values / 2;
  
  g_return_val_if_fail (order > 0, 0.0);
  g_return_val_if_fail (a != NULL, 0.0);
  g_return_val_if_fail (b != NULL, 0.0);
  g_return_val_if_fail (freq > 0 && freq < GSL_PI, 0.0);
  g_return_val_if_fail (n_values > 0, 0.0);
  
  filter_state = g_newa (double, (order + 1) * 4);
  gsl_iir_filter_setup (&filter, order, a, b, filter_state);
  
  while (n_values)
    {
      guint todo = MIN (n_values, SINE_SCAN_SIZE);
      guint i;
      
      for (i = 0; i < todo; i++)
	{
	  x[i] = sin (pos);
	  pos += freq;
	}
      
      gsl_iir_filter_eval (&filter, SINE_SCAN_SIZE, x, y);
      
      for (i = 0; i < todo; i++)
        if (n_values - i < scan_start)
	  result = MAX (y[i], result);
      
      n_values -= todo;
    }
  return result;
}





/* vim:set ts=8 sts=2 sw=2: */
