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
#ifndef __GSL_MATH_H__
#define __GSL_MATH_H__

#include <gsl/gslieee754.h>
#include <gsl/gsldefs.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- constants --- */
#define GSL_PI                  (3.1415926535897932384626433832795029 /* pi */)
#define GSL_PI_DIV_2            (1.5707963267948966192313216916397514 /* pi/2 */)
#define GSL_PI_DIV_4            (0.7853981633974483096156608458198757 /* pi/4 */)
#define GSL_1_DIV_PI            (0.3183098861837906715377675267450287 /* 1/pi */)
#define GSL_2_DIV_PI            (0.6366197723675813430755350534900574 /* 2/pi */)
#define GSL_2_DIV_SQRT_PI       (1.1283791670955125738961589031215452 /* 2/sqrt(pi) */)
#define GSL_SQRT2               (1.4142135623730950488016887242096981 /* sqrt(2) */)
#define GSL_1_DIV_SQRT2         (0.7071067811865475244008443621048490 /* 1/sqrt(2) */)
#define GSL_E                   (2.7182818284590452353602874713526625 /* e */)
#define GSL_LOG2E               (1.4426950408889634073599246810018922 /* log_2(e) */)
#define GSL_LOG10E              (0.4342944819032518276511289189166051 /* log_10(e) */)
#define GSL_LN2                 (0.6931471805599453094172321214581766 /* ln(2) */)
#define GSL_LN10                (2.3025850929940456840179914546843642 /* ln(10) */)
#define	GSL_2_POW_1_DIV_12	(1.0594630943592952645618252949463417 /* 2^(1/12) */)
#define	GSL_2_POW_1_DIV_72	(1.0096735332285108621925214011186051 /* 2^(1/72) */)
#define	GSL_LN_2_POW_1_DIV_12	(0.0577622650466621091181026767881814 /* ln(2^(1/12)) */)
#define	GSL_LN_2_POW_1_DIV_72	(0.0096270441744436848530171127980302 /* ln(2^(1/72)) */)
#define GSL_LOG2_10             (3.3219280948873623478703194294893902 /* log_2(10) */)
#define GSL_LOG2POW20_10        (0.1660964047443681173935159714744695 /* log_2(10)/20 */)


/* --- structures --- */
struct _GslComplex
{
  double re;
  double im;
};


/* --- float/double signbit extraction --- */
#ifdef signbit
#  define       gsl_float_sign(dblflt)  (signbit (dblflt))
#else
#  define       gsl_float_sign(dblflt)  ((dblflt) < -0.0)       /* good enough for us */
#endif


/* --- complex numbers --- */
static inline GslComplex gsl_complex		(double		re,
						 double		im);
static inline GslComplex gsl_complex_polar	(double		abs,
						 double		arg);
static inline GslComplex gsl_complex_add	(GslComplex	c1,
						 GslComplex	c2);
static inline GslComplex gsl_complex_add3	(GslComplex	c1,
						 GslComplex	c2,
						 GslComplex	c3);
static inline GslComplex gsl_complex_sub	(GslComplex	c1,
						 GslComplex	c2);
static inline GslComplex gsl_complex_sub3	(GslComplex	c1,
						 GslComplex	c2,
						 GslComplex	c3);
static inline GslComplex gsl_complex_scale	(GslComplex	c1,
						 double    	scale);
static inline GslComplex gsl_complex_mul	(GslComplex	c1,
						 GslComplex	c2);
static inline GslComplex gsl_complex_mul3	(GslComplex	c1,
						 GslComplex	c2,
						 GslComplex	c3);
static inline GslComplex gsl_complex_div	(GslComplex	a,
						 GslComplex	b);
static inline GslComplex gsl_complex_reciprocal	(GslComplex	c);
static inline GslComplex gsl_complex_sqrt	(GslComplex	z);
static inline GslComplex gsl_complex_conj	(GslComplex	c); /* {re, -im} */
static inline GslComplex gsl_complex_id		(GslComplex	c);
static inline GslComplex gsl_complex_inv	(GslComplex	c); /* {-re, -im} */
static inline double	 gsl_complex_abs	(GslComplex	c);
static inline double	 gsl_complex_arg	(GslComplex	c);
static inline GslComplex gsl_complex_sin	(GslComplex     c);
static inline GslComplex gsl_complex_cos	(GslComplex     c);
static inline GslComplex gsl_complex_tan	(GslComplex     c);
static inline GslComplex gsl_complex_sinh	(GslComplex     c);
static inline GslComplex gsl_complex_cosh	(GslComplex     c);
static inline GslComplex gsl_complex_tanh	(GslComplex     c);
char*			 gsl_complex_str	(GslComplex	c);
char*			 gsl_complex_list	(unsigned int	n_points,
						 GslComplex    *points,
						 const char    *indent);
void			 gsl_complex_gnuplot	(const char    *file_name,
						 unsigned int   n_points,
						 GslComplex    *points);


/* --- polynomials --- */
/* example, degree=2: 5+2x+7x^2 => a[0..degree] = { 5, 2, 7 } */
static inline void     gsl_poly_add		(unsigned int	degree,
						 double	       *a, /* a[0..degree] */
						 double	       *b);
static inline void     gsl_poly_sub		(unsigned int	order,
						 double	       *a, /* [0..degree] */
						 double	       *b);
static inline void     gsl_poly_mul		(double	       *p,  /* out:[0..aorder+border] */
                                                 unsigned int   aorder,
						 const double  *a,  /* in:[0..aorder] */
						 unsigned int   border,
						 const double  *b); /* in:[0..border] */
static inline void     gsl_poly_scale		(unsigned int	order,
						 double	       *a, /* [0..degree] */
						 double		scale);
static inline void     gsl_poly_xscale		(unsigned int	order,
						 double	       *a, /* [0..degree] */
						 double		xscale);
static inline double   gsl_poly_eval		(unsigned int	degree,
						 double	       *a, /* [0..degree] */
						 double		x);
void		       gsl_poly_complex_roots	(unsigned int	poly_degree,
						 double	       *a, /* [0..degree] (degree+1 elements) */
						 GslComplex    *roots); /* [degree] */
void		       gsl_poly_from_re_roots	(unsigned int	poly_degree,
						 double	       *a, /* [0..degree] */
						 GslComplex    *roots);
void		       gsl_cpoly_from_roots	(unsigned int	poly_degree,
						 GslComplex    *c, /* [0..degree] */
						 GslComplex    *roots);
static inline void     gsl_cpoly_mul_monomial	(unsigned int	degree, /* _new_ degree */
						 GslComplex    *c, /* in:[0..degree-1] out:[0..degree] */
						 GslComplex	root); /* c(x) *= (x^1 - root) */
static inline void     gsl_cpoly_mul_reciprocal	(unsigned int	degree, /* _new_ degree */
						 GslComplex    *c, /* in:[0..degree-1] out:[0..degree] */
						 GslComplex	root); /* c(x) *= (1 - root * x^-1) */
static inline void     gsl_cpoly_mul		(GslComplex    *p,  /* out:[0..aorder+border] */
						 unsigned int   aorder,
						 GslComplex    *a,  /* in:[0..aorder] */
						 unsigned int   border,
						 GslComplex    *b); /* in:[0..border] */

char*		       gsl_poly_str		(unsigned int	degree,
						 double	       *a,
						 const char    *var);
char*		       gsl_poly_str1		(unsigned int	degree,
						 double	       *a,
						 const char    *var);


/* --- transformations --- */
double			 gsl_temp_freq		(double		kammer_freq,
						 int		semitone_delta);


/* --- miscellaneous --- */
double			gsl_bit_depth_epsilon	(guint		n_bits); /* 1..32 */


/* --- ellipses --- */
double			 gsl_ellip_rf		(double 	x,
						 double		y,
						 double		z);
double			 gsl_ellip_F		(double		phi,
						 double		ak);
double			 gsl_ellip_sn		(double 	u,
						 double		emmc);
double			 gsl_ellip_asn		(double		y,
						 double		emmc);
GslComplex		 gsl_complex_ellip_asn	(GslComplex	y,
						 GslComplex	emmc);
GslComplex		 gsl_complex_ellip_sn	(GslComplex 	u,
						 GslComplex	emmc);


/* --- implementations --- */
static inline GslComplex
gsl_complex (double re,
	     double im)
{
  GslComplex r;
  r.re = re;
  r.im = im;
  return r;
}
static inline GslComplex
gsl_complex_polar (double abs,
		   double arg)
{
  return gsl_complex (abs * cos (arg), abs * sin (arg));
}
static inline GslComplex
gsl_complex_add	(GslComplex c1,
		 GslComplex c2)
{
  return gsl_complex (c1.re + c2.re, c1.im + c2.im);
}
static inline GslComplex
gsl_complex_add3 (GslComplex c1,
		  GslComplex c2,
		  GslComplex c3)
{
  return gsl_complex (c1.re + c2.re + c3.re, c1.im + c2.im + c3.im);
}
static inline GslComplex
gsl_complex_sub	(GslComplex c1,
		 GslComplex c2)
{
  return gsl_complex (c1.re - c2.re, c1.im - c2.im);
}
static inline GslComplex
gsl_complex_sub3 (GslComplex c1,
		  GslComplex c2,
		  GslComplex c3)
{
  return gsl_complex (c1.re - c2.re - c3.re, c1.im - c2.im - c3.im);
}
static inline GslComplex
gsl_complex_scale (GslComplex c1,
		   double     scale)
{
  return gsl_complex (c1.re * scale, c1.im * scale);
}
static inline GslComplex
gsl_complex_mul (GslComplex c1,
		 GslComplex c2)
{
  return gsl_complex (c1.re * c2.re - c1.im * c2.im, c1.re * c2.im + c1.im * c2.re);
}
static inline GslComplex
gsl_complex_mul3 (GslComplex c1,
		  GslComplex c2,
		  GslComplex c3)
{
  double aec = c1.re * c2.re * c3.re;
  double bde = c1.im * c2.im * c3.re;
  double adf = c1.re * c2.im * c3.im;
  double bcf = c1.im * c2.re * c3.im;
  double ade = c1.re * c2.im * c3.re;
  double bce = c1.im * c2.re * c3.re;
  double acf = c1.re * c2.re * c3.im;
  double bdf = c1.im * c2.im * c3.im;
  
  return gsl_complex (aec - bde - adf - bcf, ade + bce + acf - bdf);
}
static inline GslComplex
gsl_complex_div	(GslComplex a,
		 GslComplex b)
{
  GslComplex c;
  if (fabs (b.re) >= fabs (b.im))
    {
      double r = b.im / b.re, den = b.re + r * b.im;
      c.re = (a.re + r * a.im) / den;
      c.im = (a.im - r * a.re) / den;
    }
  else
    {
      double r = b.re / b.im, den = b.im + r * b.re;
      c.re = (a.re * r + a.im) / den;
      c.im = (a.im * r - a.re) / den;
    }
  return c;
}
static inline GslComplex
gsl_complex_reciprocal (GslComplex c)
{
  if (fabs (c.re) >= fabs (c.im))
    {
      double r = c.im / c.re, den = c.re + r * c.im;
      c.re = 1. / den;
      c.im = - r / den;
    }
  else
    {
      double r = c.re / c.im, den = c.im + r * c.re;
      c.re = r / den;
      c.im = - 1. / den;
    }
  return c;
}
static inline GslComplex
gsl_complex_sqrt (GslComplex z)
{
  if (z.re == 0.0 && z.im == 0.0)
    return z;
  else
    {
      GslComplex c;
      double w, x = fabs (z.re), y = fabs (z.im);
      if (x >= y)
	{
	  double r = y / x;
	  w = sqrt (x) * sqrt (0.5 * (1.0 + sqrt (1.0 + r * r)));
	}
      else
	{
	  double r = x / y;
	  w = sqrt (y) * sqrt (0.5 * (r + sqrt (1.0 + r * r)));
	}
      if (z.re >= 0.0)
	{
	  c.re = w;
	  c.im = z.im / (2.0 * w);
	}
      else
	{
	  c.im = z.im >= 0 ? w : -w;
	  c.re = z.im / (2.0 * c.im);
	}
      return c;
    }
}
static inline GslComplex
gsl_complex_conj (GslComplex c)
{
  return gsl_complex (c.re, -c.im);
}
static inline GslComplex
gsl_complex_inv (GslComplex c)
{
  return gsl_complex (-c.re, -c.im);
}
static inline GslComplex
gsl_complex_id (GslComplex c)
{
  return c;
}
static inline double
gsl_complex_abs (GslComplex c)
{
  /* compute (a^2 + b^2)^(1/2) without destructive underflow or overflow */
  double absa = fabs (c.re), absb = fabs (c.im);
  return (absa > absb ?
	  absb == 0.0 ? absa :
	  absa * sqrt (1.0 + (absb / absa) * (absb / absa)) :
	  absb == 0.0 ? 0.0 :
	  absb * sqrt (1.0 + (absa / absb) * (absa / absb)));
}
static inline double
gsl_complex_arg (GslComplex c)
{
  double a = atan2 (c.im, c.re);
  return a;
}
static inline GslComplex
gsl_complex_sin (GslComplex c)
{
  return gsl_complex (sin (c.re) * cosh (c.im), cos (c.re) * sinh (c.im));
}
static inline GslComplex
gsl_complex_cos (GslComplex c)
{
  return gsl_complex (cos (c.re) * cosh (c.im), - sin (c.re) * sinh (c.im));
}
static inline GslComplex
gsl_complex_tan (GslComplex c)
{
  return gsl_complex_div (gsl_complex (tan (c.re), tanh (c.im)),
			  gsl_complex (1.0, -tan (c.re) * tanh (c.im)));
}
static inline GslComplex
gsl_complex_sinh (GslComplex c)
{
  return gsl_complex (sinh (c.re) * cos (c.im), cosh (c.re) * sin (c.im));
}
static inline GslComplex
gsl_complex_cosh (GslComplex c)
{
  return gsl_complex (cosh (c.re) * cos (c.im), sinh (c.re) * sin (c.im));
}
static inline GslComplex
gsl_complex_tanh (GslComplex c)
{
  return gsl_complex_div (gsl_complex_sinh (c),
			  gsl_complex_cosh (c));
}
static inline void
gsl_poly_add (unsigned int degree,
	      double      *a,
	      double      *b)
{
  unsigned int i;
  
  for (i = 0; i <= degree; i++)
    a[i] += b[i];
}
static inline void
gsl_poly_sub (unsigned int degree,
	      double      *a,
	      double      *b)
{
  unsigned int i;
  
  for (i = 0; i <= degree; i++)
    a[i] -= b[i];
}
static inline void
gsl_poly_mul (double        *p,  /* out:[0..aorder+border] */
	      unsigned int   aorder,
	      const double  *a,  /* in:[0..aorder] */
	      unsigned int   border,
	      const double  *b)  /* in:[0..border] */
{
  unsigned int i;

  for (i = aorder + border; i > 0; i--)
    {
      unsigned int j;
      double t = 0;

      for (j = i - MIN (border, i); j <= MIN (aorder, i); j++)
	t += a[j] * b[i - j];
      p[i] = t;
    }
  p[0] = a[0] * b[0];
}
static inline void
gsl_cpoly_mul_monomial (unsigned int degree,
			GslComplex  *c,
			GslComplex   root)
{
  unsigned int j;
  
  c[degree] = c[degree - 1];
  for (j = degree - 1; j >= 1; j--)
    c[j] = gsl_complex_sub (c[j - 1], gsl_complex_mul (c[j], root));
  c[0] = gsl_complex_mul (c[0], gsl_complex_inv (root));
}
static inline void
gsl_cpoly_mul_reciprocal (unsigned int degree,
			  GslComplex  *c,
			  GslComplex   root)
{
  unsigned int j;
  
  c[degree] = gsl_complex_mul (c[degree - 1], gsl_complex_inv (root));
  for (j = degree - 1; j >= 1; j--)
    c[j] = gsl_complex_sub (c[j], gsl_complex_mul (c[j - 1], root));
  /* c[0] = c[0]; */
}
static inline void
gsl_cpoly_mul (GslComplex  *p,	/* [0..aorder+border] */
	       unsigned int aorder,
	       GslComplex  *a,
	       unsigned int border,
	       GslComplex  *b)
{
  unsigned int i;

  for (i = aorder + border; i > 0; i--)
    {
      GslComplex t;
      unsigned int j;

      t = gsl_complex (0, 0);
      for (j = i - MIN (i, border); j <= MIN (aorder, i); j++)
	t = gsl_complex_add (t, gsl_complex_mul (a[j], b[i - j]));
      p[i] = t;
    }
  p[0] = gsl_complex_mul (a[0], b[0]);
}
static inline void
gsl_poly_scale (unsigned int degree,
		double      *a,
		double       scale)
{
  unsigned int i;
  
  for (i = 0; i <= degree; i++)
    a[i] *= scale;
}
static inline void
gsl_poly_xscale (unsigned int degree,
		 double      *a,
		 double       xscale)
{
  double scale = xscale;
  unsigned int i;
  
  for (i = 1; i <= degree; i++)
    {
      a[i] *= scale;
      scale *= xscale;
    }
}
static inline double
gsl_poly_eval (unsigned int degree,
	       double	   *a,
	       double	    x)
{
  double sum = a[degree];
  
  while (degree--)
    sum = sum * x + a[degree];
  return sum;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_MATH_H__ */	/* vim: set ts=8 sw=2 sts=2: */
