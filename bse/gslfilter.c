/* GSL - Generic Sound Layer
 * Copyright (C) 2001 Stefan Westerfeld and Tim Janik
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


/* --- butterworth filter --- */
#if 0
void
gsl_filter_butter (unsigned int iorder,
		   double       freq, /* 0..pi */
		   double      *a,    /* [0..iorder] */
		   double      *b)
{
  double pi = GSL_PI, order = iorder, norm;
  double beta_mul = pi / (2.0 * order);
  double kappa = gsl_trans_freq2s (freq);
  GslComplex root, *poly = g_newa (GslComplex, iorder + 1);
  unsigned int i;

  /* construct poles for butterworth filter */
  poly[0] = gsl_complex (1, 0);
  for (i = 1; i <= iorder; i++)
    {
      double t = (i << 1) + iorder - 1;
      double beta = t * beta_mul;

      root.re = kappa * cos (beta);
      root.im = kappa * sin (beta);
      root = gsl_trans_s2z (root);
      gsl_cpoly_mul_reciprocal (i, poly, root);
    }

  /* z denominator polynomial */
  for (i = 0; i <= iorder; i++)
    b[i] = poly[i].re;

  /* z nominator polynomial */
  poly[0] = gsl_complex (1, 0);
  for (i = 1; i <= iorder; i++)
    gsl_cpoly_mul_reciprocal (i, poly, gsl_complex (-1, 0));
  for (i = 0; i <= iorder; i++)
    a[i] = poly[i].re;

  /* scale maximum to 1.0 */
  norm = gsl_poly_eval (iorder, a, 1) / gsl_poly_eval (iorder, b, 1);
  g_print("#n:%f\n",norm);
  gsl_poly_scale (iorder, a, 1.0 / norm);
}
#endif

void
gsl_filter_butter (unsigned int iorder,
		   double       freq, /* 0..pi */
		   double       epsilon,
		   double      *a,    /* [0..iorder] */
		   double      *b)
{
  double pi = GSL_PI, order = iorder, norm;
  double beta_mul = pi / (2.0 * order);
  //  double kappa = gsl_trans_freq2s (freq);
  double kappa = gsl_trans_freq2s (freq) * pow (epsilon, -1.0 / order);
  GslComplex root, *poly = g_newa (GslComplex, iorder + 1);
  unsigned int i;

  /* construct poles for butterworth filter */
  poly[0] = gsl_complex (1, 0);
  for (i = 1; i <= iorder; i++)
    {
      double t = (i << 1) + iorder - 1;
      double beta = t * beta_mul;

      root.re = kappa * cos (beta);
      root.im = kappa * sin (beta);
      root = gsl_trans_s2z (root);
      gsl_cpoly_mul_reciprocal (i, poly, root);
    }

  /* z denominator polynomial */
  for (i = 0; i <= iorder; i++)
    b[i] = poly[i].re;

  /* z nominator polynomial */
  poly[0] = gsl_complex (1, 0);
  for (i = 1; i <= iorder; i++)
    gsl_cpoly_mul_reciprocal (i, poly, gsl_complex (-1, 0));
  for (i = 0; i <= iorder; i++)
    a[i] = poly[i].re;

  /* scale maximum to 1.0 */
  norm = gsl_poly_eval (iorder, a, 1) / gsl_poly_eval (iorder, b, 1);
  g_print("#n:%f\n",norm);
  gsl_poly_scale (iorder, a, 1.0 / norm);
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

void
gsl_filter_tscheb1 (unsigned int iorder,
		    double       freq, /* 1..pi */
		    double       epsilon,
		    double      *a,    /* [0..iorder] */
		    double      *b)
{
  double pi = GSL_PI, order = iorder, norm;
  double alpha = asinh (1.0 / epsilon) / order;
  double beta_mul = pi / (2.0 * order);
  double kappa = gsl_trans_freq2s (freq);
  GslComplex root, *poly = g_newa (GslComplex, iorder + 1);
  unsigned int i;

  /* construct poles polynomial from tschebyscheff polynomial */
  poly[0] = gsl_complex (1, 0);
  for (i = 1; i <= iorder; i++)
    {
      double t = (i << 1) + iorder - 1;
      double beta = t * beta_mul;

      root.re = kappa * sinh (alpha) * cos (beta);
      root.im = kappa * cosh (alpha) * sin (beta);
      root = gsl_trans_s2z (root);
      gsl_cpoly_mul_reciprocal (i, poly, root);
    }

  /* z denominator polynomial */
  for (i = 0; i <= iorder; i++)
    b[i] = poly[i].re;

  /* z nominator polynomial */
  poly[0] = gsl_complex (1, 0);
  for (i = 1; i <= iorder; i++)
    gsl_cpoly_mul_reciprocal (i, poly, gsl_complex (-1, 0));
  for (i = 0; i <= iorder; i++)
    a[i] = poly[i].re;

  /* scale maximum to 1.0 */
  norm = gsl_poly_eval (iorder, a, 1) / gsl_poly_eval (iorder, b, 1);
  g_print("#n:%f\n",norm);
  if ((iorder & 1) == 0)      /* norm is fluctuation minimum */
    norm /= sqrt (1.0 / (1.0 + epsilon * epsilon));
  g_print("#n:%f\n",norm);
  gsl_poly_scale (iorder, a, 1.0 / norm);
}


/* --- tschebyscheff type 2 filter --- */
void
gsl_filter_tscheb2 (unsigned int iorder,
		    double       c_freq, /* 1..pi */
		    double       r_freq, /* 1..pi */
		    double       epsilon,
		    double      *a,    /* [0..iorder] */
		    double      *b)
{
  double pi = GSL_PI, order = iorder, norm;
  double kappa_c = gsl_trans_freq2s (c_freq);
  double kappa_r = gsl_trans_freq2s (r_freq);
  double tepsilon = epsilon * tschebyscheff_eval (iorder, kappa_r / kappa_c);
  double alpha = asinh (tepsilon) / order;
  double beta_mul = pi / (2.0 * order);
  GslComplex root, *poly = g_newa (GslComplex, iorder + 1);
  unsigned int i;

  /* construct poles polynomial from tschebyscheff polynomial */
  poly[0] = gsl_complex (1, 0);
  for (i = 1; i <= iorder; i++)
    {
      double t = (i << 1) + iorder - 1;
      double beta = t * beta_mul;
      
      root.re = sinh (alpha) * cos (beta);
      root.im = cosh (alpha) * sin (beta);
      root = gsl_complex_div (gsl_complex (kappa_r, 0), root);
      root = gsl_trans_s2z (root);
      gsl_cpoly_mul_reciprocal (i, poly, root);
    }
  
  /* z denominator polynomial */
  for (i = 0; i <= iorder; i++)
    b[i] = poly[i].re;
  
  /* construct roots polynomial from tschebyscheff polynomial */
  poly[0] = gsl_complex (1, 0);
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
      gsl_cpoly_mul_reciprocal (i, poly, root);
    }
  /* z nominator polynomial */
  for (i = 0; i <= iorder; i++)
    a[i] = poly[i].re;
  
  /* scale maximum to 1.0 */
  norm = gsl_poly_eval (iorder, a, 1) / gsl_poly_eval (iorder, b, 1); /* H(z=0):=1, e^(j*omega) for omega=0 => e^0==1 */
  gsl_poly_scale (iorder, a, 1. / norm);
}


/* --- tschebyscheff type 1 via generic root-finding --- */
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

void
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
  gsl_poly_mul (iorder, q, vk, vk);
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
