/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2006 Tim Janik
 * Copyright (C) 1984, 1987, 1988, 1989, 1995, 2000 Stephen L. Moshier
 *
 * This software is provided "as is"; redistribution and modification
 * is permitted, provided that the following disclaimer is retained.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */
#include "bseellipticfilter.h"
#define _ISOC99_SOURCE  /* for INFINITY and NAN */
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


#define ARRSIZ          (300)   /* size of arrays used to store coefficients */

static void __attribute__ ((__format__ (__printf__, 1, 2)))
VERBOSE (const char *format,
         ...)
{
  char buffer[4096];
  va_list args;
  va_start (args, format);
  vsnprintf (buffer, sizeof (buffer), format, args);
  va_end (args);
  printf ("# %s", buffer);
  fflush (stdout);
}

#define EVERBOSE VERBOSE
//#define EVERBOSE(...) do{}while (0)

#if 0 // FIXME: increase precision by using:

//#include "bseieee754.h"
#define PI                            (3.141592653589793238462643383279502884197)    // pi
#define BSE_PI_DIV_2                  (1.570796326794896619231321691639751442099)    // pi/2
#define BSE_DOUBLE_EPSILON       (1.1102230246251565404236316680908203125e-16) /* 2^-53, round-off error at 1.0 */
#define BSE_DOUBLE_MAX_NORMAL    (1.7976931348623157e+308) /* 7fefffff ffffffff, 2^1024 * (1 - BSE_DOUBLE_EPSILON) */




#define DECIBELL_FACTOR         (4.3429448190325182765112891891661)     /* 10.0 / ln (10.0) */
#define MACHEP                  (BSE_DOUBLE_EPSILON)                    /* the machine roundoff error */
// #define PI                   /* PI is defined in bseieee754.h */
#define PIO2                    (BSE_PI_DIV_2)                          /* pi/2 */
#define MAXNUM                  (BSE_DOUBLE_MAX_NORMAL)                 /* 2**1024*(1-MACHEP) */
static void init_constants (void) {}

#else

static double DECIBELL_FACTOR = -1;
static void
init_constants (void)
{
  DECIBELL_FACTOR = 10.0 / log (10.0);
}
static const double MAXNUM =  1.79769313486231570815E308;    /* 2**1024*(1-MACHEP) */
static const double PI     =  3.14159265358979323846;       /* pi */
static const double PIO2   =  1.57079632679489661923;       /* pi/2 */
static const double MACHEP =  1.11022302462515654042E-16;   /* 2**-53 */
#endif

static double fixme2local_a, fixme2local_k, fixme2local_m;
static double fixme2local_1; // k
static double fixme2local_2; // u

/* This code calculates design coefficients for
 * digital filters of the Butterworth, Chebyshev, or
 * elliptic varieties.
 *
 * The program displays relevant pass band and stop band edge
 * frequencies and stop band attenuation. The z-plane coefficients
 * are printed in these forms:
 *   Numerator and denominator z polynomial coefficients
 *   Pole and zero locations
 *   Polynomial coefficients of quadratic factors
 * 
 * After giving all the coefficients, the program prints a
 * table of the frequency response of the filter.  You can
 * get a picture by reading the table into gnuplot.
 * 
 * Filter design:
 * 
 * The output coefficients of primary interest are shown as follows:
 * 
 * (z-plane pole location:)
 * pole     3.0050282041410E-001    9.3475816516366E-001
 * (quadratic factors:)
 * q. f.
 * z**2    9.6407477241696E-001
 * z**1   -6.0100564082819E-001
 * (center frequency, gain at f0, and gain at 0 Hz:)
 * f0  2.00496167E+003  gain  2.9238E+001  DC gain  7.3364E-001
 * 
 * zero     1.7886295237392E-001    9.8387399816648E-001
 * q. f.
 * z**2    1.0000000000000E+000
 * z**1   -3.5772590474783E-001
 * f0  2.21379064E+003  gain  0.0000E+000  DC gain  1.6423E+000
 * 
 * To make a biquad filter from this, the equation for the
 * output y(i) at the i-th sample as a function of the input
 * x(i) at the i-th sample is
 * 
 * y(i) + -6.0100564082819E-001 y(i-1) +  9.6407477241696E-001 y(i-2)
 * = x(i) + -3.5772590474783E-001 x(i-1) +  1.0000000000000E+000 x(i-2).
 * 
 * Thus the two coefficients for the pole would normally be
 * negated in a typical implementation of the filter.
 * 
 * References:
 * 
 * A. H. Gray, Jr., and J. D. Markel, "A Computer Program for
 * Designing Digital Elliptic Filters", IEEE Transactions on
 * Acoustics, Speech, and Signal Processing 6, 529-538
 * (December, 1976)
 * 
 * B. Gold and C. M. Rader, Digital Processing of Signals,
 * McGraw-Hill, Inc. 1969, pp 61-90
 * 
 * M. Abramowitz and I. A. Stegun, eds., Handbook of Mathematical
 * Functions, National Bureau of Standards AMS 55, 1964,
 * Chapters 16 and 17
 */

/* --- Complex numeral --- */
typedef struct {
  double r;     // real part
  double i;     // imaginary part
} Complex;

/* --- prototypes --- */
static const Complex COMPLEX_ONE = {1.0, 0.0};
static double Cabs   (const Complex *z);
static void   Cadd   (const Complex *a, const Complex *b, Complex *c);
static void   Cdiv   (const Complex *a, const Complex *b, Complex *c);
static void   Cmov   (const Complex *a,                   Complex *b);
static void   Cmul   (const Complex *a, const Complex *b, Complex *c);
static void   Csqrt  (const Complex *z,                   Complex *w);
static void   Csub   (const Complex *a, const Complex *b, Complex *c);
static double polevl (double x, const double coef[], int N);
static double ellik  (double phi, double m); // incomplete elliptic integral of the first kind
static double ellpk  (double x); // complete elliptic integral of the first kind
static int    ellpj  (double u, double m, double *sn, double *cn, double *dn, double *ph); // Jacobian Elliptic Functions
static int    math_set_error (char *name, int code);

/* --- math errors --- */
static int math_global_error = 0;
#define MATH_ERROR_DOMAIN		1	/* argument domain error */
#define MATH_ERROR_SING		        2	/* argument singularity */
#define MATH_ERROR_OVERFLOW	        3	/* overflow range error */
#define MATH_ERROR_UNDERFLOW	        4	/* underflow range error */
#define MATH_ERROR_TOTAL_LOSS		5	/* total loss of precision */
#define MATH_ERROR_PARTIAL_LOSS		6	/* partial loss of precision */

/* Common error handling routine
 *
 * SYNOPSIS:
 * char *fctnam;
 * int code;
 * int math_set_error();
 *
 * math_set_error(fctnam, code);
 *
 * DESCRIPTION:
 * This routine may be called to report one of the following
 * error conditions (in the include file mconf.h).
 *   Mnemonic        Value          Significance
 *    MATH_ERROR_DOMAIN            1       argument domain error
 *    MATH_ERROR_SING              2       function singularity
 *    MATH_ERROR_OVERFLOW          3       overflow range error
 *    MATH_ERROR_UNDERFLOW         4       underflow range error
 *    MATH_ERROR_TOTAL_LOSS        5       total loss of precision
 *    MATH_ERROR_PARTIAL_LOSS      6       partial loss of precision
 *
 * The default version of the file prints the function name,
 * passed to it by the pointer fctnam, followed by the
 * error condition.  The display is directed to the standard
 * output device.  The routine then returns to the calling
 * program.  Users may wish to modify the program to abort by
 * calling exit() under severe error conditions such as domain
 * errors.
 *
 * Since all error conditions pass control to this function,
 * the display may be easily changed, eliminated, or directed
 * to an error logging device.
 */
static int
math_set_error (char *name, int code)
{
  /* Notice: the order of appearance of the following
   * messages is bound to the error codes defined
   * in mconf.h.
   */
  static const char *ermsg[7] = {
    "unknown",      /* error code 0 */
    "domain",       /* error code 1 */
    "singularity",  /* et seq.      */
    "overflow",
    "underflow",
    "total loss of precision",
    "partial loss of precision"
  };
  
  /* Display string passed by calling program,
   * which is supposed to be the name of the
   * function in which the error occurred:
   */
  printf ("\n%s ", name); // FIXME
  
  /* Set global error message word */
  math_global_error = code;
  
  /* Display error message defined
   * by the code argument.
   */
  if ((code <= 0) || (code >= 7))
    code = 0;
  printf ("%s error\n", ermsg[code]);
  
  /* Return to calling
   * program
   */
  return 0;
}

/* --- complex number arithmetic --- */
/* c = b + a	*/
static void
Cadd (const Complex *a, const Complex *b, Complex *c)
{
  c->r = b->r + a->r;
  c->i = b->i + a->i;
}

/* c = b - a	*/
static void
Csub (const Complex *a, const Complex *b, Complex *c)
{
  c->r = b->r - a->r;
  c->i = b->i - a->i;
}

/* c = b * a */
static void
Cmul (const Complex *a, const Complex *b, Complex *c)
{
  /* Multiplication:
   *    c.r  =  b.r * a.r  -  b.i * a.i
   *    c.i  =  b.r * a.i  +  b.i * a.r
   */
  double y;
  y    = b->r * a->r  -  b->i * a->i;
  c->i = b->r * a->i  +  b->i * a->r;
  c->r = y;
  /* see Cdiv() for accuracy comments */
}

/* c = b / a */
static void
Cdiv (const Complex *a, const Complex *b, Complex *c)
{
  /* Division:
   *    d    =  a.r * a.r  +  a.i * a.i
   *    c.r  = (b.r * a.r  + b.i * a.i)/d
   *    c.i  = (b.i * a.r  -  b.r * a.i)/d
   */
  double y = a->r * a->r  +  a->i * a->i;
  double p = b->r * a->r  +  b->i * a->i;
  double q = b->i * a->r  -  b->r * a->i;
  
  if (y < 1.0)
    {
      double w = MAXNUM * y;
      if ((fabs (p) > w) || (fabs (q) > w) || (y == 0.0))
        {
          c->r = MAXNUM;
          c->i = MAXNUM;
          math_set_error ("Cdiv", MATH_ERROR_OVERFLOW);
          return;
        }
    }
  c->r = p/y;
  c->i = q/y;
  /* ACCURACY:
   * In DEC arithmetic, the test (1/z) * z = 1 had peak relative
   * error 3.1e-17, rms 1.2e-17.  The test (y/z) * (z/y) = 1 had
   * peak relative error 8.3e-17, rms 2.1e-17.
   * Tests in the rectangle {-10,+10}:
   *                      Relative error:
   * arithmetic   function  # trials      peak         rms
   *    DEC        Cadd       10000       1.4e-17     3.4e-18
   *    IEEE       Cadd      100000       1.1e-16     2.7e-17
   *    DEC        Csub       10000       1.4e-17     4.5e-18
   *    IEEE       Csub      100000       1.1e-16     3.4e-17
   *    DEC        Cmul        3000       2.3e-17     8.7e-18
   *    IEEE       Cmul      100000       2.1e-16     6.9e-17
   *    DEC        Cdiv       18000       4.9e-17     1.3e-17
   *    IEEE       Cdiv      100000       3.7e-16     1.1e-16
   */
}

/* b = a */
static void
Cmov (const Complex *a, Complex *b)
{
  *b = *a;
}

/* Cabs() - Complex absolute value
 *
 * SYNOPSIS:
 * double Cabs();
 * Complex z;
 * double a;
 *
 * a = Cabs (&z);
 *
 * DESCRIPTION:
 * If z = x + iy
 * then
 *       a = sqrt (x**2 + y**2).
 * Overflow and underflow are avoided by testing the magnitudes
 * of x and y before squaring.  If either is outside half of
 * the floating point full scale range, both are rescaled.
 *
 * ACCURACY:
 *                      Relative error:
 * arithmetic   domain     # trials      peak         rms
 *    DEC       -30,+30     30000       3.2e-17     9.2e-18
 *    IEEE      -10,+10    100000       2.7e-16     6.9e-17
 */
static double
Cabs (const Complex *z)
{
  /* exponent thresholds for IEEE doubles */
  const double PREC = 27;
  const double MAXEXP = 1024;
  const double MINEXP = -1077;

  double x, y, b, re, im;
  int ex, ey, e;
  
  /* Note, Cabs (INFINITY,NAN) = INFINITY. */
  if (z->r == INFINITY || z->i == INFINITY
      || z->r == -INFINITY || z->i == -INFINITY)
    return INFINITY;
  
  if (isnan (z->r))
    return z->r;
  if (isnan (z->i))
    return z->i;
  
  re = fabs (z->r);
  im = fabs (z->i);
  
  if (re == 0.0)
    return im;
  if (im == 0.0)
    return re;
  
  /* Get the exponents of the numbers */
  x = frexp (re, &ex);
  y = frexp (im, &ey);
  
  /* Check if one number is tiny compared to the other */
  e = ex - ey;
  if (e > PREC)
    return re;
  if (e < -PREC)
    return im;
  
  /* Find approximate exponent e of the geometric mean. */
  e = (ex + ey) >> 1;
  
  /* Rescale so mean is about 1 */
  x = ldexp (re, -e);
  y = ldexp (im, -e);
  
  /* Hypotenuse of the right triangle */
  b = sqrt (x * x  +  y * y);
  
  /* Compute the exponent of the answer. */
  y = frexp (b, &ey);
  ey = e + ey;
  
  /* Check it for overflow and underflow. */
  if (ey > MAXEXP)
    {
      math_set_error ("Cabs", MATH_ERROR_OVERFLOW);
      return INFINITY;
    }
  if (ey < MINEXP)
    return 0.0;
  
  /* Undo the scaling */
  b = ldexp (b, e);
  return b;
}

/* Csqrt() - Complex square root
 *
 * SYNOPSIS:
 * void Csqrt();
 * Complex z, w;
 * Csqrt (&z, &w);
 *
 * DESCRIPTION:
 * If z = x + iy,  r = |z|, then
 *                       1/2
 * Im w  =  [ (r - x)/2 ]   ,
 * Re w  =  y / 2 Im w.
 *
 * Note that -w is also a square root of z.  The root chosen
 * is always in the upper half plane.
 * Because of the potential for cancellation error in r - x,
 * the result is sharpened by doing a Heron iteration
 * (see sqrt.c) in complex arithmetic.
 *
 * ACCURACY:
 *                      Relative error:
 * arithmetic   domain     # trials      peak         rms
 *    DEC       -10,+10     25000       3.2e-17     9.6e-18
 *    IEEE      -10,+10    100000       3.2e-16     7.7e-17
 *                        2
 * Also tested by Csqrt (z) = z, and tested by arguments
 * close to the real axis.
 */
static void
Csqrt (const Complex *z, Complex *w)
{
  Complex q, s;
  double x, y, r, t;
  
  x = z->r;
  y = z->i;
  
  if (y == 0.0)
    {
      if (x < 0.0)
        {
          w->r = 0.0;
          w->i = sqrt (-x);
          return;
        }
      else
        {
          w->r = sqrt (x);
          w->i = 0.0;
          return;
        }
    }
  
  
  if (x == 0.0)
    {
      r = fabs (y);
      r = sqrt (0.5*r);
      if (y > 0)
        w->r = r;
      else
        w->r = -r;
      w->i = r;
      return;
    }
  
  /* Approximate  sqrt (x^2+y^2) - x  =  y^2/2x - y^4/24x^3 + ... .
   * The relative error in the first term is approximately y^2/12x^2 .
   */
  if ((fabs (y) < 2.e-4 * fabs (x))
      && (x > 0))
    {
      t = 0.25*y*(y/x);
    }
  else
    {
      r = Cabs (z);
      t = 0.5*(r - x);
    }
  
  r = sqrt (t);
  q.i = r;
  q.r = y/(2.0*r);
  /* Heron iteration in complex arithmetic */
  Cdiv (&q, z, &s);
  Cadd (&q, &s, w);
  w->r *= 0.5;
  w->i *= 0.5;
}

/* --- elliptic functions --- */
/* ellik.c - Incomplete elliptic integral of the first kind
 *
 * SYNOPSIS:
 * double phi, m, y, ellik();
 * y = ellik (phi, m);
 *
 * DESCRIPTION:
 * Approximates the integral
 *                phi
 *                 -
 *                | |
 *                |           dt
 * F(phi_\m)  =    |    ------------------
 *                |                   2
 *              | |    sqrt(1 - m sin t)
 *               -
 *                0
 * of amplitude phi and modulus m, using the arithmetic -
 * geometric mean algorithm.
 *
 * ACCURACY:
 * Tested at random points with m in [0, 1] and phi as indicated.
 *                      Relative error:
 * arithmetic   domain     # trials      peak         rms
 *    IEEE     -10,10       200000      7.4e-16     1.0e-16
 */
static double
ellik (double phi, double m)
{
  double a, b, c, e, temp, t, K;
  int d, mod, sign, npio2;
  
  if (m == 0.0)
    return phi;
  a = 1.0 - m;
  if (a == 0.0)
    {
      if (fabs (phi) >= PIO2)
        {
          math_set_error ("ellik", MATH_ERROR_SING);
          return MAXNUM;
        }
      return log ( tan ((PIO2 + phi) / 2.0) );
    }
  npio2 = floor (phi/PIO2);
  if (npio2 & 1)
    npio2 += 1;
  if (npio2)
    {
      K = ellpk (a);
      phi = phi - npio2 * PIO2;
    }
  else
    K = 0.0;
  if (phi < 0.0)
    {
      phi = -phi;
      sign = -1;
    }
  else
    sign = 0;
  b = sqrt (a);
  t = tan (phi);
  if (fabs (t) > 10.0)
    {
      /* Transform the amplitude */
      e = 1.0/(b*t);
      /* ... but avoid multiple recursions.  */
      if (fabs (e) < 10.0)
        {
          e = atan (e);
          if (npio2 == 0)
            K = ellpk (a);
          temp = K - ellik (e, m);
          goto done;
        }
    }
  a = 1.0;
  c = sqrt (m);
  d = 1;
  mod = 0;
  
  while (fabs (c/a) > MACHEP)
    {
      temp = b/a;
      phi = phi + atan (t*temp) + mod * PI;
      mod = (phi + PIO2)/PI;
      t = t * (1.0 + temp)/(1.0 - temp * t * t);
      c = (a - b)/2.0;
      temp = sqrt (a * b);
      a = (a + b)/2.0;
      b = temp;
      d += d;
    }
  
  temp = (atan (t) + mod * PI)/(d * a);
  
 done:
  if (sign < 0)
    temp = -temp;
  temp += npio2 * K;
  return temp;
}

/* ellpj - Jacobian Elliptic Functions
 *
 * SYNOPSIS:
 * double u, m, sn, cn, dn, phi;
 * int ellpj();
 * ellpj (u, m, _&sn, _&cn, _&dn, _&phi);
 *
 * DESCRIPTION:
 * Evaluates the Jacobian elliptic functions sn(u|m), cn(u|m),
 * and dn(u|m) of parameter m between 0 and 1, and real
 * argument u.
 *
 * These functions are periodic, with quarter-period on the
 * real axis equal to the complete elliptic integral
 * ellpk(1.0-m).
 *
 * Relation to incomplete elliptic integral:
 * If u = ellik(phi,m), then sn(u|m) = sin(phi),
 * and cn(u|m) = cos(phi).  Phi is called the amplitude of u.
 *
 * Computation is by means of the arithmetic-geometric mean
 * algorithm, except when m is within 1e-9 of 0 or 1.  In the
 * latter case with m close to 1, the approximation applies
 * only for phi < pi/2.
 *
 * ACCURACY:
 * Tested at random points with u between 0 and 10, m between
 * 0 and 1.
 *            Absolute error (* = relative error):
 * arithmetic   function   # trials      peak         rms
 *    DEC       sn           1800       4.5e-16     8.7e-17
 *    IEEE      phi         10000       9.2e-16*    1.4e-16*
 *    IEEE      sn          50000       4.1e-15     4.6e-16
 *    IEEE      cn          40000       3.6e-15     4.4e-16
 *    IEEE      dn          10000       1.3e-12     1.8e-14
 * Peak error observed in consistency check using addition
 * theorem for sn(u+v) was 4e-16 (absolute).  Also tested by
 * the above relation to the incomplete elliptic integral.
 * Accuracy deteriorates when u is large.
 */
static int
ellpj (double u, double m,
       double *sn, double *cn, double *dn,
       double *ph)
{
  double ai, b, phi, t, twon;
  double a[9], c[9];
  int i;
  /* Check for special cases */
  if (m < 0.0 || m > 1.0)
    {
      math_set_error ("ellpj", MATH_ERROR_DOMAIN);
      *sn = 0.0;
      *cn = 0.0;
      *ph = 0.0;
      *dn = 0.0;
      return -1;
    }
  if (m < 1.0e-9)
    {
      t = sin (u);
      b = cos (u);
      ai = 0.25 * m * (u - t*b);
      *sn = t - ai*b;
      *cn = b + ai*t;
      *ph = u - ai;
      *dn = 1.0 - 0.5*m*t*t;
      return 0;
    }
  
  if (m >= 0.9999999999)
    {
      ai = 0.25 * (1.0-m);
      b = cosh (u);
      t = tanh (u);
      phi = 1.0/b;
      twon = b * sinh (u);
      *sn = t + ai * (twon - u)/(b*b);
      *ph = 2.0*atan (exp (u)) - PIO2 + ai*(twon - u)/b;
      ai *= t * phi;
      *cn = phi - ai * (twon - u);
      *dn = phi + ai * (twon + u);
      return 0;
    }
  /*	A. G. M. scale		*/
  a[0] = 1.0;
  b = sqrt (1.0 - m);
  c[0] = sqrt (m);
  twon = 1.0;
  i = 0;
  
  while (fabs (c[i]/a[i]) > MACHEP)
    {
      if (i > 7)
        {
          math_set_error ("ellpj", MATH_ERROR_OVERFLOW);
          goto done;
        }
      ai = a[i];
      ++i;
      c[i] = (ai - b)/2.0;
      t = sqrt (ai * b);
      a[i] = (ai + b)/2.0;
      b = t;
      twon *= 2.0;
    }
  
 done:
  /* backward recurrence */
  phi = twon * a[i] * u;
  do
    {
      t = c[i] * sin (phi) / a[i];
      b = phi;
      phi = (asin (t) + phi)/2.0;
    }
  while (--i);
  
  *sn = sin (phi);
  t = cos (phi);
  *cn = t;
  *dn = t/cos (phi-b);
  *ph = phi;
  return 0;
}

/* ellpk - Complete elliptic integral of the first kind
 *
 * SYNOPSIS:
 * double m1, y, ellpk();
 * y = ellpk (m1);
 *
 * DESCRIPTION:
 * Approximates the integral
 *            pi/2
 *             -
 *            | |
 *            |           dt
 * K(m)  =    |    ------------------
 *            |                   2
 *          | |    sqrt(1 - m sin t)
 *           -
 *            0
 * where m = 1 - m1, using the approximation
 *     P(x)  -  log x Q(x).
 *
 * The argument m1 is used rather than m so that the logarithmic
 * singularity at m = 1 will be shifted to the origin; this
 * preserves maximum accuracy.
 * K(0) = pi/2.
 *
 * ACCURACY:
 *                      Relative error:
 * arithmetic   domain     # trials      peak         rms
 *    DEC        0,1        16000       3.5e-17     1.1e-17
 *    IEEE       0,1        30000       2.5e-16     6.8e-17
 * ERROR MESSAGES:
 *   message         condition      value returned
 * ellpk domain       x<0, x>1           0.0
 */
static double
ellpk (double x)
{
  static const double P_ellpk[] = {
    1.37982864606273237150E-4,
    2.28025724005875567385E-3,
    7.97404013220415179367E-3,
    9.85821379021226008714E-3,
    6.87489687449949877925E-3,
    6.18901033637687613229E-3,
    8.79078273952743772254E-3,
    1.49380448916805252718E-2,
    3.08851465246711995998E-2,
    9.65735902811690126535E-2,
    1.38629436111989062502E0
  };
  static const double Q_ellpk[] = {
    2.94078955048598507511E-5,
    9.14184723865917226571E-4,
    5.94058303753167793257E-3,
    1.54850516649762399335E-2,
    2.39089602715924892727E-2,
    3.01204715227604046988E-2,
    3.73774314173823228969E-2,
    4.88280347570998239232E-2,
    7.03124996963957469739E-2,
    1.24999999999870820058E-1,
    4.99999999999999999821E-1
  };
  static const double C1_ellpk = 1.3862943611198906188E0; /* log(4) */

  if ((x < 0.0) || (x > 1.0))
    {
      math_set_error ("ellpk", MATH_ERROR_DOMAIN);
      return 0.0;
    }
  
  if (x > MACHEP)
    {
      return polevl (x,P_ellpk,10) - log (x) * polevl (x,Q_ellpk,10);
    }
  else
    {
      if (x == 0.0)
        {
          math_set_error ("ellpk", MATH_ERROR_SING);
          return MAXNUM;
        }
      else
        {
          return C1_ellpk - 0.5 * log (x);
        }
    }
}

/* --- misc utilities --- */
/* polevl - Evaluate polynomial
 *
 * SYNOPSIS:
 * int N;
 * double x, y, coef[N+1], polevl[];
 * y = polevl(x, coef, N);
 *
 * DESCRIPTION:
 * Evaluates polynomial of degree N:
 *                     2          N
 * y  =  C  + C x + C x  +...+ C x
 *        0    1     2          N
 *
 * Coefficients are stored in reverse order:
 * coef[0] = C  , ..., coef[N] = C  .
 *            N                   0
 * SPEED:
 * In the interest of speed, there are no checks for out
 * of bounds arithmetic.  This routine is used by most of
 * the functions in the library.  Depending on available
 * equipment features, the user may wish to rewrite the
 * program in microcode or assembly language.
 */
static double
polevl (double x, const double coef[], int N)
{
  double ans;
  int i;
  const double *p;
  
  p = coef;
  ans = *p++;
  i = N;
  
  do
    ans = ans * x  +  *p++;
  while (--i);
  
  return ans;
}

#if 1
static double aa[ARRSIZ];
static double pp[ARRSIZ];
static double y[ARRSIZ];
static double zs[ARRSIZ];
static Complex z[ARRSIZ];
static double Kk = 0.0;
static double Kpk = 0.0;
static double rho = 0.0;
static double phi = 0.0;
static double sn = 0.0;
static double cn = 0.0;
static double dn = 0.0;
static double sn1 = 0.0;
static double cn1 = 0.0;
static double dn1 = 0.0;
static double phi1 = 0.0;
static double pn = 0.0;
static double an = 0.0;
static double gam = 0.0;
static double gain = 0.0;
#endif

/* --- prototypes --- */
static int    find_elliptic_locations_in_lambda_plane      (const BseIIRFilterRequirements *ifr,
                                                            DesignState                    *ds);
static int    find_s_plane_poles_and_zeros                 (const BseIIRFilterRequirements *ifr,
                                                            DesignState                    *ds);
static int    convert_s_plane_to_z_plane                   (const BseIIRFilterRequirements *ifr,
                                                            DesignState                    *ds);
static double jacobi_theta_by_nome                                (double q);
static int    z_plane_zeros_poles_to_numerator_denomerator (const BseIIRFilterRequirements *ifr,
                                                            DesignState                    *ds);
static int    gainscale_and_print_deno_nume_zeros2_poles2  (const BseIIRFilterRequirements *ifr,
                                                            DesignState                    *ds);
static int    print_quadratic_factors                      (const BseIIRFilterRequirements *ifr,
                                                            DesignState                    *ds,
                                                            double x, double y, int pzflg);
static void   print_filter_table                           (const BseIIRFilterRequirements *ifr,
                                                            DesignState                    *ds);
static double response                                     (const BseIIRFilterRequirements *ifr,
                                                            DesignState                    *ds,
                                                            double f, double amp);

static void
print_z_fraction_before_zplnc (const BseIIRFilterRequirements *ifr,
                               DesignState                    *ds) /* must be called *before* zplnc() */
{
  double zgain;
  if (ifr->kind != 3 && pn == 0)
    zgain = 1.0;
  else
    zgain = an / (pn * ds->gain_scale);
  VERBOSE ("# constant mygain factor %23.13E\n", zgain); // BSE info
  VERBOSE ("# z plane Denominator      Numerator\n"); // BSE info
  int j;
  for (j = 0; j <= ds->n_solved_poles; j++)
    VERBOSE ("%2d %17.9E %17.9E\n", j, aa[j], pp[j] * zgain); // BSE info
}

/* --- main IIR filter design function --- */
static const char*
iir_filter_design (const BseIIRFilterRequirements *ifr,
                   DesignState                    *ds)
{
  double passband_edge1 = ifr->passband_edge;
  double passband_edge0 = ifr->passband_edge2;
  
  if (ifr->kind <= 0 || ifr->kind > 3)
    return "unknown kind";
  if (ifr->type <= 0 || ifr->type > 4)
    return "unknown type";
  if (ifr->order <= 0)
    return "order too small";

  if (ifr->kind > 1) /* not Butterworth */
    {
      if (ifr->passband_ripple_db <= 0.0)
        return "passband_ripple_db too small";
      if (ifr->kind == 2)
        {
          /* For Chebyshev filter, ripples go from 1.0 to 1/sqrt(1+ds->ripple_epsilon^2) */
          phi = exp (0.5 * ifr->passband_ripple_db / DECIBELL_FACTOR);

          if ((ifr->order & 1) == 0)
            ds->gain_scale = phi;
          else
            ds->gain_scale = 1.0;
        }
      else
        { /* elliptic */
          ds->ripple_epsilon = exp (ifr->passband_ripple_db / DECIBELL_FACTOR);
          ds->gain_scale = 1.0;
          if ((ifr->order & 1) == 0)
            ds->gain_scale = sqrt (ds->ripple_epsilon);
          ds->ripple_epsilon = sqrt (ds->ripple_epsilon - 1.0);
        }
    }
  
  if (ifr->sampling_frequency <= 0.0)
    return "sampling_frequency too small";
  
  ds->nyquist_frequency = 0.5 * ifr->sampling_frequency;
  
  if (passband_edge1 <= 0.0)
    return "passband_edge1 too small";
  if (passband_edge1 >= ds->nyquist_frequency)
    return "passband_edge1 too high";
  
  if ((ifr->type & 1) == 0)
    {
      if (passband_edge0 <= 0.0)
        return "passband_edge too small";
      if (passband_edge0 >= ds->nyquist_frequency)
        return "passband_edge too high";
    }
  else
    {
      passband_edge0 = 0.0;
    }
  if (passband_edge1 < passband_edge0)
    {
      double tmp = passband_edge1;
      passband_edge1 = passband_edge0;
      passband_edge0 = tmp;
    }
  double high_edge, band_width;
  if (ifr->type == 3)	/* high pass */
    {
      band_width = passband_edge1;
      high_edge = ds->nyquist_frequency;
    }
  else
    {
      band_width = passband_edge1 - passband_edge0;
      high_edge = passband_edge1;
    }
  /* Frequency correspondence for bilinear transformation
   *
   *  Wanalog = tan(2 pi Fdigital T / 2)
   *
   * where T = 1/ifr->sampling_frequency
   */
  double ang = band_width * PI / ifr->sampling_frequency; /* angle frequency */
  double sang;
  double cang = cos (ang);
  ds->tan_angle_frequency = sin (ang) / cang; /* Wanalog */
  if (ifr->kind != 3)
    {
      ds->wc = ds->tan_angle_frequency;
      /*printf("cos(1/2 (Whigh-Wlow) T) = %.5e, wc = %.5e\n", cang, ds->wc);*/
    }
  
  
  if (ifr->kind == 3)
    { /* elliptic */
      double tmp_cgam = cos ((high_edge + passband_edge0) * PI / ifr->sampling_frequency) / cang;
      ds->cgam = tmp_cgam;
      if (ifr->stopband_edge > 0.0)
        ds->stopband_edge = ifr->stopband_edge;
      else if (ifr->stopband_db >= 0.0)
        return "need stopband_db or stopband_edge";
      else /* stopband_db < 0.0 */
        { /* calculate band edge from db down */
          double a = exp (-ifr->stopband_db / DECIBELL_FACTOR);
          double m1 = ds->ripple_epsilon / sqrt (a - 1.0);
          m1 *= m1;
          double m1p = 1.0 - m1;
          double Kk1 = ellpk (m1p);
          double Kpk1 = ellpk (m1);
          double q = exp (-PI * Kpk1 / (ifr->order * Kk1));
          fixme2local_1 = jacobi_theta_by_nome (q);
          if (ifr->type >= 3)
            ds->wr = fixme2local_1;
          else
            ds->wr = 1.0 / fixme2local_1;
          if (ifr->type & 1)
            {
              ds->stopband_edge = atan (ds->tan_angle_frequency * ds->wr) * ifr->sampling_frequency / PI;
            }
          else
            {
              // FIXME: using tmp_cgam here increases precision
              fixme2local_a = ds->tan_angle_frequency * ds->wr;
              fixme2local_a *= fixme2local_a;
              double b = fixme2local_a * (1.0 - ds->cgam * ds->cgam) + fixme2local_a * fixme2local_a;
              b = (ds->cgam + sqrt (b))/(1.0 + fixme2local_a);
              ds->stopband_edge = (PI / 2.0 - asin (b)) * ifr->sampling_frequency / (2.0 * PI);
            }
        }
      switch (ifr->type)
	{
	case 1:
          if (ds->stopband_edge <= passband_edge1)
            return "need stopband_edge > passband_edge";
          break;
	case 2:
          if (ds->stopband_edge >= passband_edge0 && ds->stopband_edge <= passband_edge1)
            return "need stopband_edge < passband_edge or stopband_edge > passband_edge2";
          break;
	case 3:
          if (ds->stopband_edge >= passband_edge1)
            return "need stopband_edge < passband_edge";
          break;
	case 4:
          if (ds->stopband_edge <= passband_edge0)
            return "need stopband_edge > passband_edge2";
          if (ds->stopband_edge >= passband_edge1)
            return "need stopband_edge < passband_edge";
          break;
	}
      ang = ds->stopband_edge * PI / ifr->sampling_frequency;
      cang = cos (ang);
      sang = sin (ang);

      if (ifr->type & 1)
	{
          ds->wr = sang/(cang * ds->tan_angle_frequency);
	}
      else
	{
          double q = cang * cang  -  sang * sang;
          sang = 2.0 * cang * sang;
          cang = q;
          ds->wr = (ds->cgam - cang)/(sang * ds->tan_angle_frequency);
	}

      if (ifr->type >= 3)
	ds->wr = 1.0 / ds->wr;
      if (ds->wr < 0.0)
	ds->wr = -ds->wr;
      y[0] = 1.0;
      y[1] = ds->wr;
      /* ds->chebyshev_band_cbp = ds->wr; */
      
      if (ifr->type >= 3)
	y[1] = 1.0 / y[1];
      
      if (ifr->type & 1)
	{
          int i;
          for (i = 1; i <= 2; i++)
            {
              aa[i] = atan (ds->tan_angle_frequency * y[i - 1]) * ifr->sampling_frequency / PI ;
            }
          printf ("pass band %.9E\n", aa[1]);
          printf ("stop band %.9E\n", aa[2]);
	}
      else
	{
          int i;
          for (i = 1; i <= 2; i++)
            {
              double a = ds->tan_angle_frequency * y[i - 1];
              double b = atan (a);
              double q = sqrt (1.0 + a * a  -  ds->cgam * ds->cgam);
              q = atan2 (q, ds->cgam);
              aa[i] = (q + b) * ds->nyquist_frequency / PI;
              pp[i] = (q - b) * ds->nyquist_frequency / PI;
            }
          printf ("pass band %.9E %.9E\n", pp[1], aa[1]);
          printf ("stop band %.9E %.9E\n", pp[2], aa[2]);
	}
      ds->wc = 1.0;
      find_elliptic_locations_in_lambda_plane (ifr, ds);	/* find locations in lambda plane */
      if ((2 * ifr->order + 2) > ARRSIZ)
	goto toosml;
    } /* elliptic */
  
  /* Transformation from low-pass to band-pass critical frequencies
   *
   * Center frequency
   *                     cos(1/2 (Whigh+Wlow) T)
   *  cos(Wcenter T) = ----------------------
   *                     cos(1/2 (Whigh-Wlow) T)
   *
   *
   * Band edges
   *            cos(Wcenter T) - cos(Wdigital T)
   *  Wanalog = -----------------------------------
   *                        sin(Wdigital T)
   */
  
  if (ifr->kind == 2)
    { /* Chebyshev */
      double a = PI * (high_edge + passband_edge0) / ifr->sampling_frequency ;
      ds->cgam = cos (a) / cang;
      a = 2.0 * PI * passband_edge1 / ifr->sampling_frequency;
      ds->chebyshev_band_cbp = (ds->cgam - cos (a)) / sin (a);
    }
  if (ifr->kind == 1)
    { /* Butterworth */
      double a = PI * (high_edge + passband_edge0) / ifr->sampling_frequency ;
      ds->cgam = cos (a) / cang;
      a = 2.0 * PI * passband_edge1 / ifr->sampling_frequency;
      /* ds->chebyshev_band_cbp = (ds->cgam - cos (a)) / sin (a); */
      ds->gain_scale = 1.0;
    }

  EVERBOSE ("State: gain_scale=%.20g ripple_epsilon=%.20g nyquist_frequency=%.20g " // BSE info
            "tan_angle_frequency=%.20g stopband_edge=%.20g wc=%.20g wr=%.20g cgam=%.20g\n",
            ds->gain_scale, ds->ripple_epsilon, ds->nyquist_frequency,
            ds->tan_angle_frequency, ds->stopband_edge, ds->wc, ds->wr, ds->cgam);

  find_s_plane_poles_and_zeros (ifr, ds);		/* find s plane poles and zeros */
  
  if (((ifr->type & 1) == 0) && ((4 * ifr->order + 2) > ARRSIZ))
    goto toosml;
  
  convert_s_plane_to_z_plane (ifr, ds);	/* convert s plane to z plane */
  // volatile_sink ("x");
  z_plane_zeros_poles_to_numerator_denomerator (ifr, ds);
  EVERBOSE ("an=%.20g pn=%.20g scale=%.20g\n", an, pn, ds->gain_scale); // BSE info
  print_z_fraction_before_zplnc (ifr, ds);
  gainscale_and_print_deno_nume_zeros2_poles2 (ifr, ds);
  print_filter_table (ifr, ds); /* tabulate transfer function */
  return NULL;

 toosml:
  return "storage arrays too small";
}


static int
find_elliptic_locations_in_lambda_plane (const BseIIRFilterRequirements *ifr,
                                         DesignState                    *ds)
{
  fixme2local_k = ds->wc / ds->wr;
  fixme2local_m = fixme2local_k * fixme2local_k;
  Kk = ellpk (1.0 - fixme2local_m);
  Kpk = ellpk (fixme2local_m);
  EVERBOSE ("check: k=%.20g m=%.20g Kk=%.20g Kpk=%.20g\n", fixme2local_k, fixme2local_m, Kk, Kpk); // BSE info
  double q = exp (-PI * ifr->order * Kpk / Kk);	/* the nome of k1 */
  double m1 = jacobi_theta_by_nome (q); /* see below */
  /* Note m1 = ds->ripple_epsilon / sqrt(A*A - 1.0) */
  double a = ds->ripple_epsilon / m1;
  a =  a * a + 1;
  a = 10.0 * log (a) / log (10.0);
  printf ("dbdown %.9E\n", a);
  a = 180.0 * asin (fixme2local_k) / PI;
  double b = 1.0/(1.0 + ds->ripple_epsilon * ds->ripple_epsilon);
  b = sqrt (1.0 - b);
  printf ("theta %.9E, rho %.9E\n", a, b);
  m1 *= m1;
  double m1p = 1.0 - m1;
  double Kk1 = ellpk (m1p);
  double Kpk1 = ellpk (m1);
  double r = Kpk1 * Kk / (Kk1 * Kpk);
  printf ("consistency check: n= %.14E\n", r);
  EVERBOSE ("consistency check: r=%.20g Kpk1=%.20g Kk1=%.20g m1=%.20g m1p=%.20g\n", r, Kpk1, Kk1, m1, m1p); // BSE info
  /*   -1
   * sn   j/ds->ripple_epsilon\m  =  j ellik(atan(1/ds->ripple_epsilon), m)
   */
  b = 1.0 / ds->ripple_epsilon;
  phi = atan (b);
  fixme2local_2 = ellik (phi, m1p);
  EVERBOSE ("phi=%.20g m=%.20g u=%.20g\n", phi, m1p, fixme2local_2);
  /* consistency check on inverse sn */
  ellpj (fixme2local_2, m1p, &sn, &cn, &dn, &phi);
  a = sn / cn;
  EVERBOSE ("consistency check: sn/cn = %.20g = %.20g = 1/ripple\n", a, b);
  ds->elliptic_k = fixme2local_k;
  ds->elliptic_u = fixme2local_2 * Kk / (ifr->order * Kk1);	/* or, u = u * Kpk / Kpk1 */
  ds->elliptic_m = fixme2local_m;
  return 0;
}

/* calculate s plane poles and zeros, normalized to wc = 1 */
static int
find_s_plane_poles_and_zeros (const BseIIRFilterRequirements *ifr,
                              DesignState                    *ds)
{
  int i, j;
  for (i = 0; i < ARRSIZ; i++)
    zs[i] = 0.0;
  ds->n_poles = (ifr->order + 1) / 2;
  ds->n_zeros = 0;
  if (ifr->kind == 1)
    {
      double m;
      /* Butterworth poles equally spaced around the unit circle */
      if (ifr->order & 1)
        m = 0.0;
      else
        m = PI / (2.0 * ifr->order);
      for (i = 0; i < ds->n_poles; i++)
        {	/* poles */
          int lr = i + i;
          zs[lr] = -cos (m);
          zs[lr + 1] = sin (m);
          m += PI / ifr->order;
        }	
      /* high pass or band reject
       */
      if (ifr->type >= 3)
        {
          int ii = 0;
          /* map s => 1/s */
          for (j = 0; j < ds->n_poles; j++)
            {
              int ir = j + j;
              ii = ir + 1;
              double b = zs[ir]*zs[ir] + zs[ii]*zs[ii];
              zs[ir] = zs[ir] / b;
              zs[ii] = zs[ii] / b;
            }
          /* The zeros at infinity map to the origin.
           */
          ds->n_zeros = ds->n_poles;
          if (ifr->type == 4)
            {
              ds->n_zeros += ifr->order / 2;
            }
          for (j = 0; j < ds->n_zeros; j++)
            {
              int ir = ii + 1;
              ii = ir + 1;
              zs[ir] = 0.0;
              zs[ii] = 0.0;
            }
        }
    }
  if (ifr->kind == 2)
    {
      /* For Chebyshev, find radii of two Butterworth circles
       * See Gold & Rader, page 60
       */
      rho = (phi - 1.0)*(phi + 1);  /* rho = ds->ripple_epsilon^2 = {sqrt(1+ds->ripple_epsilon^2)}^2 - 1 */
      ds->ripple_epsilon = sqrt (rho);
      /* sqrt(1 + 1/ds->ripple_epsilon^2) + 1/ds->ripple_epsilon  = {sqrt(1 + ds->ripple_epsilon^2)  +  1} / ds->ripple_epsilon
       */
      phi = (phi + 1.0) / ds->ripple_epsilon;
      EVERBOSE ("Chebychev: phi-before=%.20g ripple=%.20g\n", phi, ds->ripple_epsilon); // BSE info
      phi = pow (phi, 1.0 / ifr->order);  /* raise to the 1/n power */
      EVERBOSE ("Chebychev: phi-raised=%.20g rn=%.20g\n", phi, ifr->order * 1.0); // BSE info
      double b = 0.5 * (phi + 1.0 / phi); /* y coordinates are on this circle */
      double a = 0.5 * (phi - 1.0 / phi); /* x coordinates are on this circle */
      double m;
      if (ifr->order & 1)
        m = 0.0;
      else
        m = PI / (2.0 * ifr->order);
      for (i = 0; i < ds->n_poles; i++)
        {	/* poles */
          int lr = i + i;
          zs[lr] = -a * cos (m);
          zs[lr + 1] = b * sin (m);
          m += PI / ifr->order;
        }	
      /* high pass or band reject
       */
      if (ifr->type >= 3)
        {
          int ii = 0;
          /* map s => 1/s */
          for (j = 0; j < ds->n_poles; j++)
            {
              int ir = j + j;
              ii = ir + 1;
              b = zs[ir]*zs[ir] + zs[ii]*zs[ii];
              zs[ir] = zs[ir] / b;
              zs[ii] = zs[ii] / b;
            }
          /* The zeros at infinity map to the origin.
           */
          ds->n_zeros = ds->n_poles;
          if (ifr->type == 4)
            {
              ds->n_zeros += ifr->order / 2;
            }
          for (j = 0; j < ds->n_zeros; j++)
            {
              int ir = ii + 1;
              ii = ir + 1;
              zs[ir] = 0.0;
              zs[ii] = 0.0;
            }
        }
    }
  if (ifr->kind == 3)   /* elliptic filter -- stw */
    {
      double m = ds->elliptic_m;
      ds->n_zeros = ifr->order / 2;
      ellpj (ds->elliptic_u, 1.0 - m, &sn1, &cn1, &dn1, &phi1);
      for (i=0; i < ARRSIZ; i++)
        zs[i] = 0.0;
      for (i = 0; i < ds->n_zeros; i++)
        {	/* zeros */
          double a = ifr->order - 1 - i - i;
          double b = (Kk * a) / ifr->order;
          ellpj (b, m, &sn, &cn, &dn, &phi);
          int lr = 2 * ds->n_poles + 2 * i;
          zs[lr] = 0.0;
          a = ds->wc / (ds->elliptic_k * sn);	/* elliptic_k = sqrt(m) */
          zs[lr + 1] = a;
        }
      for (i = 0; i < ds->n_poles; i++)
        {	/* poles */
          double a = ifr->order - 1 - i - i;
          double b = a * Kk / ifr->order;
          ellpj (b, m, &sn, &cn, &dn, &phi);
          double r = ds->elliptic_k * sn * sn1;
          b = cn1 * cn1 + r * r;
          a = -ds->wc * cn * dn * sn1 * cn1 / b;
          int lr = i + i;
          zs[lr] = a;
          b = ds->wc * sn * dn1 / b;
          zs[lr + 1] = b;
        }	
      if (ifr->type >= 3)
        {
          int ii = 0, nt = ds->n_poles + ds->n_zeros;
          for (j = 0; j < nt; j++)
            {
              int ir = j + j;
              ii = ir + 1;
              double b = zs[ir]*zs[ir] + zs[ii]*zs[ii];
              zs[ir] = zs[ir] / b;
              zs[ii] = zs[ii] / b;
            }
          while (ds->n_poles > ds->n_zeros)
            {
              int ir = ii + 1;
              ii = ir + 1;
              ds->n_zeros += 1;
              zs[ir] = 0.0;
              zs[ii] = 0.0;
            }
        }
    }
  printf ("s plane poles:\n");
  j = 0;
  for (i = 0; i < ds->n_poles + ds->n_zeros; i++)
    {
      double a = zs[j];
      ++j;
      double b = zs[j];
      ++j;
      printf ("%.9E %.9E\n", a, b);
      if (i == ds->n_poles - 1)
        printf ("s plane zeros:\n");
    }
  return 0;
}

/* jacobi_theta_by_nome():
 * Find parameter corresponding to given nome by expansion
 * in theta functions:
 * AMS55 #16.38.5, 16.38.7
 *
 *       1/2
 * (2K)                   4     9
 * (--)     =  1 + 2q + 2q  + 2q  + ...  =  Theta (0,q)
 * (pi)                                          3
 *
 *
 *       1/2
 * (2K)     1/4       1/4        2    6    12    20
 * (--)    m     =  2q    (1 + q  + q  + q   + q   + ...) = Theta (0,q)
 * (pi)                                                           2
 *
 * The nome q(m) = exp(- pi K(1-m)/K(m)).
 *
 *                                1/2
 * Given q, this program returns m   .
 */
static double
jacobi_theta_by_nome (double q)
{
  double t1, a = 1.0, b = 1.0, r = 1.0, p = q;
  do
    {
      r *= p;
      a += 2.0 * r;
      t1 = fabs (r / a);
      
      r *= p;
      b += r;
      p *= q;
      double t2 = fabs (r / b);
      if (t2 > t1)
	t1 = t2;
    }
  while (t1 > MACHEP);
  a = b / a;
  a = 4.0 * sqrt (q) * a * a;	/* see above formulas, solved for m */
  return a;
}

/* convert s plane poles and zeros to the z plane. */
static int
convert_s_plane_to_z_plane (const BseIIRFilterRequirements *ifr,
                            DesignState                    *ds)
{
  Complex r, cnum, cden, cwc, ca, cb, b4ac;
  double C;
  
  if (ifr->kind == 3)
    C = ds->tan_angle_frequency;
  else
    C = ds->wc;
  
  int i;
  for (i = 0; i < ARRSIZ; i++)
    {
      z[i].r = 0.0;
      z[i].i = 0.0;
    }
  
  int nc = ds->n_poles;
  ds->z_counter = -1;

  int icnt, ii = -1;
  for (icnt = 0; icnt < 2; icnt++)
    {
      /* The maps from s plane to z plane */
      do
	{
          int ir = ii + 1;
          ii = ir + 1;
          r.r = zs[ir];
          r.i = zs[ii];
          
          switch (ifr->type)
            {
            case 1:
            case 3:
              /* Substitute  s - r  =  s/wc - r = (1/wc)(z-1)/(z+1) - r
               *
               *     1  1 - r wc (       1 + r wc)
               * =  --- -------- (z  -  --------)
               *    z+1    wc    (       1 - r wc)
               *
               * giving the root in the z plane.
               */
              cnum.r = 1 + C * r.r;
              cnum.i = C * r.i;
              cden.r = 1 - C * r.r;
              cden.i = -C * r.i;
              ds->z_counter += 1;
              Cdiv (&cden, &cnum, &z[ds->z_counter]);
              if (r.i != 0.0)
                {
                  /* fill in complex conjugate root */
                  ds->z_counter += 1;
                  z[ds->z_counter].r = z[ds->z_counter - 1].r;
                  z[ds->z_counter].i = -z[ds->z_counter - 1].i;
                }
              break;
              
            case 2:
            case 4:
              /* Substitute  s - r  =>  s/wc - r
               *
               *     z^2 - 2 z cgam + 1
               * =>  ------------------  -  r
               *         (z^2 + 1) wc  
               *
               *         1
               * =  ------------  [ (1 - r wc) z^2  - 2 cgam z  +  1 + r wc ]
               *    (z^2 + 1) wc  
               *
               * and solve for the roots in the z plane.
               */
              if (ifr->kind == 2)
                cwc.r = ds->chebyshev_band_cbp;
              else
                cwc.r = ds->tan_angle_frequency;
              cwc.i = 0.0;
              Cmul (&r, &cwc, &cnum);     /* r wc */
              Csub (&cnum, &COMPLEX_ONE, &ca);   /* a = 1 - r wc */
              Cmul (&cnum, &cnum, &b4ac); /* 1 - (r wc)^2 */
              Csub (&b4ac, &COMPLEX_ONE, &b4ac);
              b4ac.r *= 4.0;               /* 4ac */
              b4ac.i *= 4.0;
              cb.r = -2.0 * ds->cgam;          /* b */
              cb.i = 0.0;
              Cmul (&cb, &cb, &cnum);     /* b^2 */
              Csub (&b4ac, &cnum, &b4ac); /* b^2 - 4 ac */
              Csqrt (&b4ac, &b4ac);
              cb.r = -cb.r;  /* -b */
              cb.i = -cb.i;
              ca.r *= 2.0; /* 2a */
              ca.i *= 2.0;
              Cadd (&b4ac, &cb, &cnum);   /* -b + sqrt(b^2 - 4ac) */
              Cdiv (&ca, &cnum, &cnum);   /* ... /2a */
              ds->z_counter += 1;
              Cmov (&cnum, &z[ds->z_counter]);
              if (cnum.i != 0.0)
                {
                  ds->z_counter += 1;
                  z[ds->z_counter].r = cnum.r;
                  z[ds->z_counter].i = -cnum.i;
                }
              if ((r.i != 0.0) || (cnum.i == 0))
                {
                  Csub (&b4ac, &cb, &cnum);  /* -b - sqrt(b^2 - 4ac) */
                  Cdiv (&ca, &cnum, &cnum);  /* ... /2a */
                  ds->z_counter += 1;
                  Cmov (&cnum, &z[ds->z_counter]);
                  if (cnum.i != 0.0)
                    {
                      ds->z_counter += 1;
                      z[ds->z_counter].r = cnum.r;
                      z[ds->z_counter].i = -cnum.i;
                    }
                }
            } /* end switch */
	}
      while (--nc > 0);
      
      if (icnt == 0)
	{
          ds->n_solved_poles = ds->z_counter + 1;
          if (ds->n_zeros <= 0)
            {
              if (ifr->kind != 3)
                return 0;
              else
                break;
            }
	}
      nc = ds->n_zeros;
    } /* end for() loop */
  return 0;
}

static int
z_plane_zeros_poles_to_numerator_denomerator (const BseIIRFilterRequirements *ifr,
                                              DesignState                    *ds)
{
  Complex lin[2];
  
  lin[1].r = 1.0;
  lin[1].i = 0.0;
  
  if (ifr->kind != 3)
    { /* Butterworth or Chebyshev */
      /* generate the remaining zeros */
      while (2 * ds->n_solved_poles - 1 > ds->z_counter)
        {
          if (ifr->type != 3)
            {
              printf ("adding zero at Nyquist frequency\n");
              ds->z_counter += 1;
              z[ds->z_counter].r = -1.0; /* zero at Nyquist frequency */
              z[ds->z_counter].i = 0.0;
            }
          if ((ifr->type == 2) || (ifr->type == 3))
            {
              printf ("adding zero at 0 Hz\n");
              ds->z_counter += 1;
              z[ds->z_counter].r = 1.0; /* zero at 0 Hz */
              z[ds->z_counter].i = 0.0;
            }
        }
    }
  else
    { /* elliptic */
      while (2 * ds->n_solved_poles - 1 > ds->z_counter)
        {
          ds->z_counter += 1;
          z[ds->z_counter].r = -1.0; /* zero at Nyquist frequency */
          z[ds->z_counter].i = 0.0;
          if ((ifr->type == 2) || (ifr->type == 4))
            {
              ds->z_counter += 1;
              z[ds->z_counter].r = 1.0; /* zero at 0 Hz */
              z[ds->z_counter].i = 0.0;
            }
        }
    }
  printf ("order = %d\n", ds->n_solved_poles);

  /* Expand the poles and zeros into numerator and
   * denominator polynomials
   */
  int j, icnt;
  for (icnt = 0; icnt < 2; icnt++)
    {
      for (j = 0; j < ARRSIZ; j++)
        {
          pp[j] = 0.0;
          y[j] = 0.0;
        }
      pp[0] = 1.0;
      for (j = 0; j < ds->n_solved_poles; j++)
        {
          int jj = j;
          if (icnt)
            jj += ds->n_solved_poles;
          double a = z[jj].r;
          double b = z[jj].i;
          int i;
          for (i = 0; i <= j; i++)
            {
              int jh = j - i;
              pp[jh + 1] = pp[jh + 1] - a * pp[jh] + b * y[jh];
              y[jh + 1] =  y[jh + 1]  - b * pp[jh] - a * y[jh];
            }
        }
      if (icnt == 0)
        {
          for (j = 0; j <= ds->n_solved_poles; j++)
            aa[j] = pp[j];
        }
    }
  /* Scale factors of the pole and zero polynomials */
  double a = 1.0;
  switch (ifr->type)
    {
    case 3:
      a = -1.0;
      
    case 1:
    case 4:
      
      pn = 1.0;
      an = 1.0;
      for (j=1; j <= ds->n_solved_poles; j++)
        {
          pn = a * pn + pp[j];
          an = a * an + aa[j];
        }
      break;
      
    case 2:
      gam = PI / 2.0 - asin (ds->cgam);  /* = acos(cgam) */
      int mh = ds->n_solved_poles / 2;
      pn = pp[mh];
      an = aa[mh];
      double ai = 0.0;
      if (mh > ((ds->n_solved_poles / 4)*2))
        {
          ai = 1.0;
          pn = 0.0;
          an = 0.0;
        }
      for (j=1; j <= mh; j++)
        {
          a = gam * j - ai * PI / 2.0;
          double cng = cos (a);
          int jh = mh + j;
          int jl = mh - j;
          pn = pn + cng * (pp[jh] + (1.0 - 2.0 * ai) * pp[jl]);
          an = an + cng * (aa[jh] + (1.0 - 2.0 * ai) * aa[jl]);
        }
    }
  return 0;
}

static int
gainscale_and_print_deno_nume_zeros2_poles2 (const BseIIRFilterRequirements *ifr, /* zplnc */
                                             DesignState                    *ds)
{
  int j;
  gain = an/(pn * ds->gain_scale);
  if ((ifr->kind != 3) && (pn == 0))
    gain = 1.0;
  printf ("constant gain factor %23.13E\n", gain);
  for (j = 0; j <= ds->n_solved_poles; j++)
    pp[j] = gain * pp[j];
  
  printf ("z plane Denominator      Numerator\n");
  for (j = 0; j <= ds->n_solved_poles; j++)
    {
      printf ("%2d %17.9E %17.9E\n", j, aa[j], pp[j]);
    }

  /* I /think/ at this point the polynomial is factorized in 2nd order filters,
   * so that it can be implemented without stability problems -- stw
   */
  printf ("poles and zeros with corresponding quadratic factors\n");
  for (j = 0; j < ds->n_solved_poles; j++)
    {
      double a = z[j].r;
      double b = z[j].i;
      if (b >= 0.0)
        {
          printf ("pole  %23.13E %23.13E\n", a, b);
          print_quadratic_factors (ifr, ds, a, b, 1);
        }
      int jj = j + ds->n_solved_poles;
      a = z[jj].r;
      b = z[jj].i;
      if (b >= 0.0)
        {
          printf ("zero  %23.13E %23.13E\n", a, b);
          print_quadratic_factors (ifr, ds, a, b, 0);
        }
    }
  return 0;
}

/* display quadratic factors */
static int
print_quadratic_factors (const BseIIRFilterRequirements *ifr,
                         DesignState                    *ds,
                         double x, double y,
                         int pzflg) /* 1 if poles, 0 if zeros */
{
  double a, b, r, f, g, g0;
  
  if (y > 1.0e-16)
    {
      a = -2.0 * x;
      b = x * x + y * y;
    }
  else
    {
      a = -x;
      b = 0.0;
    }
  printf ("q. f.\nz**2 %23.13E\nz**1 %23.13E\n", b, a);
  if (b != 0.0)
    {
      /* resonant frequency */
      r = sqrt (b);
      f = PI / 2.0 - asin (-a/(2.0 * r));
      f = f * ifr->sampling_frequency / (2.0 * PI);
      /* gain at resonance */
      g = 1.0 + r;
      g = g * g - (a * a / r);
      g = (1.0 - r) * sqrt (g);
      g0 = 1.0 + a + b;	/* gain at d.c. */
    }
  else
    {
      /* It is really a first-order network.
       * Give the gain at ds->nyquist_frequency and D.C.
       */
      f = ds->nyquist_frequency;
      g = 1.0 - a;
      g0 = 1.0 + a;
    }
  
  if (pzflg)
    {
      if (g != 0.0)
        g = 1.0 / g;
      else
        g = MAXNUM;
      if (g0 != 0.0)
        g0 = 1.0 / g0;
      else
        g = MAXNUM;
    }
  printf ("f0 %16.8E  gain %12.4E  DC gain %12.4E\n\n", f, g, g0);
  return 0;
}

/* Print table of filter frequency response */
static void
print_filter_table (const BseIIRFilterRequirements *ifr,
                    DesignState                    *ds)
{
  double f, limit = 0.05 * ds->nyquist_frequency * 21;
  
  for (f=0; f < limit; f += limit / 21.)
    {
      double r = response (ifr, ds, f, gain);
      if (r <= 0.0)
        r = -999.99;
      else
        r = 2.0 * DECIBELL_FACTOR * log (r);
      printf ("%10.1f  %10.2f\n", f, r);
      // f = f + 0.05 * ds->nyquist_frequency;
    }
}

/* Calculate frequency response at f Hz mulitplied by amp */
static double
response (const BseIIRFilterRequirements *ifr,
          DesignState                    *ds,
          double f, double amp)
{
  Complex x, num, den, w;
  double u;
  int j;
  
  /* exp(j omega T) */
  u = 2.0 * PI * f /ifr->sampling_frequency;
  x.r = cos (u);
  x.i = sin (u);
  
  num.r = 1.0;
  num.i = 0.0;
  den.r = 1.0;
  den.i = 0.0;
  for (j = 0; j < ds->n_solved_poles; j++)
    {
      Csub (&z[j], &x, &w);
      Cmul (&w, &den, &den);
      Csub (&z[j + ds->n_solved_poles], &x, &w);
      Cmul (&w, &num, &num);
    }
  Cdiv (&den, &num, &w);
  w.r *= amp;
  w.i *= amp;
  u = Cabs (&w);
  return u;
}

static double
my_getnum (const char *text)
{
  printf ("%s ? ", text);
  char s[4096];
  if (!fgets (s, sizeof (s), stdin))
    exit (0);
  double val = 0;
  sscanf (s, "%lf", &val);
  return val;
}


int
main (int   argc,
      char *argv[])
{
  init_constants();
  BseIIRFilterRequirements ifr = { 0 };
  DesignState ds = default_design_state;
  ifr.kind = my_getnum ("kind");
  ifr.type = my_getnum ("type");
  ifr.order = my_getnum ("order");
  if (ifr.kind > BSE_IIR_FILTER_BUTTERWORTH) /* not Butterworth */
    ifr.passband_ripple_db = my_getnum ("passband_ripple_db");
  ifr.sampling_frequency = my_getnum ("sampling_frequency");
  ifr.passband_edge = my_getnum ("passband_edge");
  if (ifr.type == BSE_IIR_FILTER_BAND_PASS ||
      ifr.type == BSE_IIR_FILTER_BAND_STOP)
    ifr.passband_edge2 = my_getnum ("passband_edge2");
  if (ifr.kind == BSE_IIR_FILTER_ELLIPTIC)
    ifr.stopband_db = ifr.stopband_edge = my_getnum ("stopband_edge or stopband_db");
  printf ("\n");
  const char *errmsg = iir_filter_design (&ifr, &ds);
  fflush (stdout);
  fflush (stderr);
  // VERBOSE ("DEBUG: %.20g %.20g %.20g %.20g %.20g\n", a, cos(a), cang, ds->cgam, 0.);
  if (errmsg)
    {
      fprintf (stderr, "Invalid specification: %s\n", errmsg);
      fflush (stderr);
      return 1;
    }
  
  return 0;
}


/* compile with: gcc -Wall -O2 -g bseellipticfilter.c -lm -o bseellipticfilter */
