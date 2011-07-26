/* BSE - Better Sound Engine
 * Copyright (C) 1997-2004 Tim Janik
 * Copyright (C) 2001 Stefan Westerfeld
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#ifndef __BSE_MATH_H__
#define __BSE_MATH_H__

#include <bse/bsedefs.h>
#include <bse/bseieee754.h> 	/* provides math.h */

G_BEGIN_DECLS

/* --- constants --- */
/* PI is defined in bseieee754.h */
#define BSE_1_DIV_PI                  (0.3183098861837906715377675267450287240689)   // 1/pi
#define BSE_PI_DIV_2                  (1.570796326794896619231321691639751442099)    // pi/2
#define BSE_2_DIV_PI                  (0.6366197723675813430755350534900574481378)   // 2/pi
#define BSE_2_DIV_SQRT_PI             (1.128379167095512573896158903121545171688)    // 2/sqrt(pi)
#define BSE_2_DIV_LN2                 (2.88539008177792681471984936200378427485329)  // 2/ln(2)
#define BSE_PI_DIV_4                  (0.7853981633974483096156608458198757210493)   // pi/4
#define BSE_E                         (2.718281828459045235360287471352662497757)    // e^1
#define BSE_LOG2E                     (1.442695040888963407359924681001892137427)    // log_2(e^1)
#define BSE_LOG10E                    (0.4342944819032518276511289189166050822944)   // log_10(e^1)
#define BSE_LN2                       (0.6931471805599453094172321214581765680755)   // ln(2)
#define BSE_SQRT2                     (1.41421356237309504880168872420969807857)     // sqrt(2)
#define BSE_1_DIV_SQRT2               (0.7071067811865475244008443621048490392848)   // 1/sqrt(2)
#define BSE_LN4                       (1.386294361119890618834464242916353136151)    // ln(4)
#define BSE_LN10                      (2.302585092994045684017991454684364207601)    // ln(10)
#define BSE_LOG2_10                   (3.321928094887362347870319429489390175865)    // log_2(10)
#define BSE_LOG2POW20_10              (0.1660964047443681173935159714744695087932)   // log_2(10)/20
#define BSE_2_POW_1_DIV_12            (1.059463094359295264561825294946341700779)    // 2^(1/12)
#define BSE_LN_2_POW_1_DIV_12         (5.776226504666210911810267678818138067296e-2) // ln(2^(1/12))
#define BSE_LN_2_POW_1_DIV_1200_d     (5.776226504666210911810267678818138067296e-4) // ln(2^(1/1200))
#define BSE_2_POW_1_DIV_72            (1.009673533228510862192521401118605073603)    // 2^(1/72)
#define BSE_LN_2_POW_1_DIV_72         (9.62704417444368485301711279803023011216e-3)  // ln(2^(1/72))
#define BSE_DECIBEL20_FACTOR          (8.68588963806503655302257837833210164588794)  // 20.0 / ln (10.0)
#define BSE_DECIBEL10_FACTOR          (4.34294481903251827651128918916605082294397)  // 10.0 / ln (10.0)
#define BSE_1_DIV_DECIBEL20_FACTOR    (0.1151292546497022842008995727342182103801)   // ln (10) / 20
#define BSE_COMPLEX_ONE               (bse_complex (1, 0))

/* --- structures --- */
typedef struct {
  double re;
  double im;
} BseComplex;

/* --- complex numbers --- */
static inline BseComplex bse_complex            (double         re,
                                                 double         im);
static inline BseComplex bse_complex_polar      (double         abs,
                                                 double         arg);
static inline BseComplex bse_complex_add        (BseComplex     c1,
                                                 BseComplex     c2);
static inline BseComplex bse_complex_add3       (BseComplex     c1,
                                                 BseComplex     c2,
                                                 BseComplex     c3);
static inline BseComplex bse_complex_sub        (BseComplex     c1,
                                                 BseComplex     c2);
static inline BseComplex bse_complex_sub3       (BseComplex     c1,
                                                 BseComplex     c2,
                                                 BseComplex     c3);
static inline BseComplex bse_complex_scale      (BseComplex     c1,
                                                 double         scale);
static inline BseComplex bse_complex_mul        (BseComplex     c1,
                                                 BseComplex     c2);
static inline BseComplex bse_complex_mul3       (BseComplex     c1,
                                                 BseComplex     c2,
                                                 BseComplex     c3);
static inline BseComplex bse_complex_div        (BseComplex     a,
                                                 BseComplex     b);
static inline BseComplex bse_complex_reciprocal (BseComplex     c);
static inline BseComplex bse_complex_sqrt       (BseComplex     z);
static inline BseComplex bse_complex_conj       (BseComplex     c); /* {re, -im} */
static inline BseComplex bse_complex_id         (BseComplex     c);
static inline BseComplex bse_complex_inv        (BseComplex     c); /* {-re, -im} */
static inline double     bse_complex_abs        (BseComplex     c);
static inline double     bse_complex_arg        (BseComplex     c);
static inline BseComplex bse_complex_sin        (BseComplex     c);
static inline BseComplex bse_complex_cos        (BseComplex     c);
static inline BseComplex bse_complex_tan        (BseComplex     c);
static inline BseComplex bse_complex_sinh       (BseComplex     c);
static inline BseComplex bse_complex_cosh       (BseComplex     c);
static inline BseComplex bse_complex_tanh       (BseComplex     c);
char*                    bse_complex_str        (BseComplex     c);
char*                    bse_complex_list       (uint           n_points,
                                                 BseComplex    *points,
                                                 const char    *indent);
void                     bse_complex_gnuplot    (const char    *file_name,
                                                 uint           n_points,
                                                 BseComplex    *points);

/* --- polynomials --- */
/* example, degree=2: 5+2x+7x^2 => a[0..degree] = { 5, 2, 7 } */
static inline void     bse_poly_add             (uint           degree,
                                                 double        *a, /* a[0..degree] */
                                                 double        *b);
static inline void     bse_poly_sub             (uint           order,
                                                 double        *a, /* [0..degree] */
                                                 double        *b);
static inline void     bse_poly_mul             (double        *p,  /* out:[0..aorder+border] */
                                                 uint           aorder,
                                                 const double  *a,  /* in:[0..aorder] */
                                                 uint           border,
                                                 const double  *b); /* in:[0..border] */
static inline void     bse_poly_scale           (uint           order,
                                                 double        *a, /* [0..degree] */
                                                 double         scale);
static inline void     bse_poly_xscale          (uint           order,
                                                 double        *a, /* [0..degree] */
                                                 double         xscale);
static inline double   bse_poly_eval            (uint           degree,
                                                 double        *a, /* [0..degree] */
                                                 double         x);
void                   bse_poly_complex_roots   (uint           poly_degree,
                                                 double        *a, /* [0..degree] (degree+1 elements) */
                                                 BseComplex    *roots); /* [degree] */
void                   bse_poly_from_re_roots   (uint           poly_degree,
                                                 double        *a, /* [0..degree] */
                                                 BseComplex    *roots);
void                   bse_cpoly_from_roots     (uint           poly_degree,
                                                 BseComplex    *c, /* [0..degree] */
                                                 BseComplex    *roots);
static inline void     bse_cpoly_mul_monomial   (uint           degree, /* _new_ degree */
                                                 BseComplex    *c, /* in:[0..degree-1] out:[0..degree] */
                                                 BseComplex     root); /* c(x) *= (x^1 - root) */
static inline void     bse_cpoly_mul_reciprocal (uint           degree, /* _new_ degree */
                                                 BseComplex    *c, /* in:[0..degree-1] out:[0..degree] */
                                                 BseComplex     root); /* c(x) *= (1 - root * x^-1) */
static inline void     bse_cpoly_mul            (BseComplex    *p,  /* out:[0..aorder+border] */
                                                 uint           aorder,
                                                 BseComplex    *a,  /* in:[0..aorder] */
                                                 uint           border,
                                                 BseComplex    *b); /* in:[0..border] */
gboolean               bse_poly2_droots         (gdouble        roots[2],
                                                 gdouble        a,
                                                 gdouble        b,
                                                 gdouble        c);
char*                  bse_poly_str             (uint           degree,
                                                 double        *a,
                                                 const char    *var);
char*                  bse_poly_str1            (uint           degree,
                                                 double        *a,
                                                 const char    *var);

/* --- transformations --- */
double                 bse_temp_freq            (double         kammer_freq,
                                                 int            semitone_delta);

/* --- miscellaneous --- */
double                 bse_bit_depth_epsilon    (guint          n_bits);  /* 1..32 */
gint                   bse_rand_int             (void);                   /* +-G_MAXINT */
gfloat                 bse_rand_float           (void);                   /* -1.0..1.0 */
gint                   bse_rand_bool            (void);                   /* random bit */
void                   bse_float_gnuplot        (const char    *file_name,
                                                 double         xstart,
                                                 double         xstep,
                                                 uint           n_ypoints,
                                                 const float   *ypoints);


/* --- implementations --- */
static inline BseComplex
bse_complex (double re,
             double im)
{
  BseComplex r;
  r.re = re;
  r.im = im;
  return r;
}
static inline BseComplex
bse_complex_polar (double abs,
                   double arg)
{
  return bse_complex (abs * cos (arg), abs * sin (arg));
}
static inline BseComplex
bse_complex_add (BseComplex c1,
                 BseComplex c2)
{
  return bse_complex (c1.re + c2.re, c1.im + c2.im);
}
static inline BseComplex
bse_complex_add3 (BseComplex c1,
                  BseComplex c2,
                  BseComplex c3)
{
  return bse_complex (c1.re + c2.re + c3.re, c1.im + c2.im + c3.im);
}
static inline BseComplex
bse_complex_sub (BseComplex c1,
                 BseComplex c2)
{
  return bse_complex (c1.re - c2.re, c1.im - c2.im);
}
static inline BseComplex
bse_complex_sub3 (BseComplex c1,
                  BseComplex c2,
                  BseComplex c3)
{
  return bse_complex (c1.re - c2.re - c3.re, c1.im - c2.im - c3.im);
}
static inline BseComplex
bse_complex_scale (BseComplex c1,
                   double     scale)
{
  return bse_complex (c1.re * scale, c1.im * scale);
}
static inline BseComplex
bse_complex_mul (BseComplex c1,
                 BseComplex c2)
{
  return bse_complex (c1.re * c2.re - c1.im * c2.im, c1.re * c2.im + c1.im * c2.re);
}
static inline BseComplex
bse_complex_mul3 (BseComplex c1,
                  BseComplex c2,
                  BseComplex c3)
{
  double aec = c1.re * c2.re * c3.re;
  double bde = c1.im * c2.im * c3.re;
  double adf = c1.re * c2.im * c3.im;
  double bcf = c1.im * c2.re * c3.im;
  double ade = c1.re * c2.im * c3.re;
  double bce = c1.im * c2.re * c3.re;
  double acf = c1.re * c2.re * c3.im;
  double bdf = c1.im * c2.im * c3.im;
  
  return bse_complex (aec - bde - adf - bcf, ade + bce + acf - bdf);
}
static inline BseComplex
bse_complex_div (BseComplex a,
                 BseComplex b)
{
  BseComplex c;
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
static inline BseComplex
bse_complex_reciprocal (BseComplex c)
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
static inline BseComplex
bse_complex_sqrt (BseComplex z)
{
  if (z.re == 0.0 && z.im == 0.0)
    return z;
  else
    {
      BseComplex c;
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
static inline BseComplex
bse_complex_conj (BseComplex c)
{
  return bse_complex (c.re, -c.im);
}
static inline BseComplex
bse_complex_inv (BseComplex c)
{
  return bse_complex (-c.re, -c.im);
}
static inline BseComplex
bse_complex_id (BseComplex c)
{
  return c;
}
static inline double
bse_complex_abs (BseComplex c)
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
bse_complex_arg (BseComplex c)
{
  double a = atan2 (c.im, c.re);
  return a;
}
static inline BseComplex
bse_complex_sin (BseComplex c)
{
  return bse_complex (sin (c.re) * cosh (c.im), cos (c.re) * sinh (c.im));
}
static inline BseComplex
bse_complex_cos (BseComplex c)
{
  return bse_complex (cos (c.re) * cosh (c.im), - sin (c.re) * sinh (c.im));
}
static inline BseComplex
bse_complex_tan (BseComplex c)
{
  return bse_complex_div (bse_complex (tan (c.re), tanh (c.im)),
                          bse_complex (1.0, -tan (c.re) * tanh (c.im)));
}
static inline BseComplex
bse_complex_sinh (BseComplex c)
{
  return bse_complex (sinh (c.re) * cos (c.im), cosh (c.re) * sin (c.im));
}
static inline BseComplex
bse_complex_cosh (BseComplex c)
{
  return bse_complex (cosh (c.re) * cos (c.im), sinh (c.re) * sin (c.im));
}
static inline BseComplex
bse_complex_tanh (BseComplex c)
{
  return bse_complex_div (bse_complex_sinh (c),
                          bse_complex_cosh (c));
}
static inline void
bse_poly_add (uint         degree,
              double      *a,
              double      *b)
{
  uint         i;
  
  for (i = 0; i <= degree; i++)
    a[i] += b[i];
}
static inline void
bse_poly_sub (uint         degree,
              double      *a,
              double      *b)
{
  uint         i;
  
  for (i = 0; i <= degree; i++)
    a[i] -= b[i];
}
static inline void
bse_poly_mul (double        *p,  /* out:[0..aorder+border] */
              uint           aorder,
              const double  *a,  /* in:[0..aorder] */
              uint           border,
              const double  *b)  /* in:[0..border] */
{
  uint         i;
  
  for (i = aorder + border; i > 0; i--)
    {
      uint         j;
      double t = 0;
      
      for (j = i - MIN (border, i); j <= MIN (aorder, i); j++)
        t += a[j] * b[i - j];
      p[i] = t;
    }
  p[0] = a[0] * b[0];
}
static inline void
bse_cpoly_mul_monomial (uint         degree,
                        BseComplex  *c,
                        BseComplex   root)
{
  uint         j;
  
  c[degree] = c[degree - 1];
  for (j = degree - 1; j >= 1; j--)
    c[j] = bse_complex_sub (c[j - 1], bse_complex_mul (c[j], root));
  c[0] = bse_complex_mul (c[0], bse_complex_inv (root));
}
static inline void
bse_cpoly_mul_reciprocal (uint         degree,
                          BseComplex  *c,
                          BseComplex   root)
{
  uint         j;
  
  c[degree] = bse_complex_mul (c[degree - 1], bse_complex_inv (root));
  for (j = degree - 1; j >= 1; j--)
    c[j] = bse_complex_sub (c[j], bse_complex_mul (c[j - 1], root));
  /* c[0] = c[0]; */
}
static inline void
bse_cpoly_mul (BseComplex  *p,  /* [0..aorder+border] */
               uint         aorder,
               BseComplex  *a,
               uint         border,
               BseComplex  *b)
{
  uint         i;
  
  for (i = aorder + border; i > 0; i--)
    {
      BseComplex t;
      uint         j;
      
      t = bse_complex (0, 0);
      for (j = i - MIN (i, border); j <= MIN (aorder, i); j++)
        t = bse_complex_add (t, bse_complex_mul (a[j], b[i - j]));
      p[i] = t;
    }
  p[0] = bse_complex_mul (a[0], b[0]);
}
static inline void
bse_poly_scale (uint         degree,
                double      *a,
                double       scale)
{
  uint         i;
  
  for (i = 0; i <= degree; i++)
    a[i] *= scale;
}
static inline void
bse_poly_xscale (uint         degree,
                 double      *a,
                 double       xscale)
{
  double scale = xscale;
  uint         i;
  
  for (i = 1; i <= degree; i++)
    {
      a[i] *= scale;
      scale *= xscale;
    }
}
static inline double
bse_poly_eval (uint         degree,
               double      *a,
               double       x)
{
  double sum = a[degree];
  
  while (degree--)
    sum = sum * x + a[degree];
  return sum;
}

G_END_DECLS

#endif /* __BSE_MATH_H__ */     /* vim: set ts=8 sw=2 sts=2: */
