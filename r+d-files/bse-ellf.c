/* BSE - Better Sound Engine
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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#if 0
extern double sqrt ( double );
extern double fabs ( double );
extern double log ( double );
extern double tan ( double );
extern double atan ( double );
extern double floor ( double );
extern double fabs ( double );
extern double Cabs ( cmplx * );
extern double sqrt ( double );
extern double atan2 ( double, double );
extern double cos ( double );
extern double sin ( double );
extern double sqrt ( double );
extern double frexp ( double, int * );
extern double ldexp ( double, int );
extern double exp ( double );
extern double log ( double );
extern double cos ( double );
extern double sin ( double );
extern double sqrt ( double );
extern double fabs ( double );
extern double asin ( double );
extern double atan ( double );
extern double atan2 ( double, double );
extern double pow ( double, double );
double sqrt(), fabs(), sin(), cos(), asin(), tanh();
double sinh(), cosh(), atan(), exp();
#else
#include <math.h>
#endif
#define code_printf(...)     fprintf (stdout, __VA_ARGS__)
#define ellf_printf(...)     fprintf (stderr, __VA_ARGS__)
#define ellf_debugf(...)     fprintf (stderr, __VA_ARGS__)
/* === ellf.doc - start === */
/*                        ellf.c
 * This program calculates design coefficients for
 * digital filters of the Butterworth, Chebyshev, or
 * elliptic varieties.
 * 
 * 
 * 
 * Usage:
 * 
 * Inputs are entered by keyboard, or are redirected to come from
 * a command file, as follows:
 * 
 * Kind of filter (1: Butterworth, 2: Chebyshev, 3: Elliptic,
 *                 0: exit to monitor)
 * 
 * Shape of filter (1: low pass, 2: band pass, 3: high pass,
 *                 4: band reject, 0: exit to monitor)
 * 
 * Order of filter (an integer)
 * 
 * Passband ripple (peak to peak decibels)
 * 
 * Sampling frequency (Hz)
 * 
 * Passband edge frequency (Hz)
 * 
 * Second passband edge frequency (for band pass or reject filters)
 * 
 * Stop band edge frequency (Hz)
 *      or stop band attenuation (entered as -decibels)
 * 
 * The "exit to monitor" type 0 may be used to terminate the
 * program when input is redirected to come from a command file.
 * 
 * If your specification is illegal, e.g. the stop band edge
 * is in the middle of the passband, the program will make you
 * start over.  However, it remembers and displays the last
 * value of each parameter entered.  To use the same value, just
 * hit carriage return instead of typing it in again.
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
 * 
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
 * 
 * 
 * Compilation:
 * 
 * This program has been compiled successfully on many different
 * computers.  See the accompanying output listing file ellf.ans,
 * for a set of correct answers.  Use the batch file test.bat to
 * check your executable program. If the low pass and high pass
 * options work but the others don't, then examine your atan2()
 * function carefully for reversed arguments or perhaps an offest of
 * pi.  On most systems, define ANSIC to be 1.  This sets the
 * expected atan2() arguments but does not otherwise imply anything
 * about the ANSI-ness of the program.
 * 
 * 
 * 
 * Files:
 * 
 * mconf.h        system configuration include file
 *                Be sure to define type of computer here!
 * cmplx.c        complex arithmetic subroutine package
 * ellf.ans       right answer file for some elliptic filters
 * ellf.que       elliptic filter questions
 * ellf.c         main program
 * ellf.doc       this file
 * ellf.mak       Microsoft MSDOS makefile
 * ellfu.mak      Unix makefile
 * ellik.c        incomplete elliptic integral of the first kind
 * ellpe.c        complete elliptic integral of the second kind
 * ellpj.c        Jacobian Elliptic Functions
 * ellpk.c        complete elliptic integral of the first kind
 * makefile       Unix makefile
 * mtherr.c       common math function error handler
 * polevl.c       evaluates polynomials
 * test.bat       batch file to run a test
 * descrip.mms    VAX makefile
 * ellf.opt       VAX makefile
 * testvax.bat    VAX test
 * 
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
 * 
 * 
 * - Steve Moshier, December 1986
 * Last rev: November, 1992
 */
/* === ellf.doc - end === */
/* === mconf.h - start === */
/*							mconf.h
 *
 *	Common include file for math routines
 *
 *
 *
 * SYNOPSIS:
 *
 * #include "mconf.h"
 *
 *
 *
 * DESCRIPTION:
 *
 * This file contains definitions for error codes that are
 * passed to the common error handling routine mtherr()
 * (which see).
 *
 * The file also includes a conditional assembly definition
 * for the type of computer arithmetic (IEEE, DEC, Motorola
 * IEEE, or UNKnown).
 * 
 * For Digital Equipment PDP-11 and VAX computers, certain
 * IBM systems, and others that use numbers with a 56-bit
 * significand, the symbol DEC should be defined.  In this
 * mode, most floating point constants are given as arrays
 * of octal integers to eliminate decimal to binary conversion
 * errors that might be introduced by the compiler.
 *
 * For little-endian computers, such as IBM PC, that follow the
 * IEEE Standard for Binary Floating Point Arithmetic (ANSI/IEEE
 * Std 754-1985), the symbol IBMPC should be defined.  These
 * numbers have 53-bit significands.  In this mode, constants
 * are provided as arrays of hexadecimal 16 bit integers.
 *
 * Big-endian IEEE format is denoted MIEEE.  On some RISC
 * systems such as Sun SPARC, double precision constants
 * must be stored on 8-byte address boundaries.  Since integer
 * arrays may be aligned differently, the MIEEE configuration
 * may fail on such machines.
 *
 * To accommodate other types of computer arithmetic, all
 * constants are also provided in a normal decimal radix
 * which one can hope are correctly converted to a suitable
 * format by the available C language compiler.  To invoke
 * this mode, define the symbol UNK.
 *
 * An important difference among these modes is a predefined
 * set of machine arithmetic constants for each.  The numbers
 * MACHEP (the machine roundoff error), MAXNUM (largest number
 * represented), and several other parameters are preset by
 * the configuration symbol.  Check the file const.c to
 * ensure that these values are correct for your computer.
 *
 * Configurations NANS, INFINITIES, MINUSZERO, and DENORMAL
 * may fail on many systems.  Verify that they are supposed
 * to work on your computer.
 */
/* Constant definitions for math error conditions
 */
#define DOMAIN		1	/* argument domain error */
#define SING		2	/* argument singularity */
#define OVERFLOW	3	/* overflow range error */
#define UNDERFLOW	4	/* underflow range error */
#define TLOSS		5	/* total loss of precision */
#define PLOSS		6	/* partial loss of precision */
#define EDOM		33
#define ERANGE		34
/* Complex numeral.  */
typedef struct
{
  double r;
  double i;
} cmplx;
/* Type of computer arithmetic is
 * UNKnown arithmetic, invokes coefficients given in
 * normal decimal format.  Beware of range boundary
 * problems (MACHEP, MAXLOG, etc. in const.c) and
 * roundoff problems in pow.c:
 * (Sun SPARCstation, i386)
 */
/* Define to support tiny denormal numbers, else undefine. */
#define DENORMAL 1
/* Define to ask for infinity support, else undefine. */
/* #define INFINITIES 1 */
/* Define to ask for support of numbers that are Not-a-Number,
   else undefine.  This may automatically define INFINITIES in some files. */
/* #define NANS 1 */
/* Define to distinguish between -0.0 and +0.0.  */
#define MINUSZERO 1
/* Define 1 for ANSI C atan2() function
   See atan.c and clog.c. */
#define ANSIC 1
int mtherr ( char *, int );
/* Variable for error reporting.  See mtherr.c.  */
extern int merror;
/* === mconf.h - end === */
/* === const.c - start === */
/*							const.c
 *
 *	Globally declared constants
 *
 *
 *
 * SYNOPSIS:
 *
 * extern double nameofconstant;
 *
 *
 *
 *
 * DESCRIPTION:
 *
 * This file contains a number of mathematical constants and
 * also some needed size parameters of the computer arithmetic.
 * The values are supplied as arrays of hexadecimal integers
 * for IEEE arithmetic; arrays of octal constants for DEC
 * arithmetic; and in a normal decimal scientific notation for
 * other machines.  The particular notation used is determined
 * by a symbol (DEC, IBMPC, or UNK) defined in the include file
 * mconf.h.
 *
 * The default size parameters are as follows.
 *
 * For DEC and UNK modes:
 * MACHEP =  1.38777878078144567553E-17       2**-56
 * MAXLOG =  8.8029691931113054295988E1       log(2**127)
 * MINLOG = -8.872283911167299960540E1        log(2**-128)
 * MAXNUM =  1.701411834604692317316873e38    2**127
 *
 * For IEEE arithmetic (IBMPC):
 * MACHEP =  1.11022302462515654042E-16       2**-53
 * MAXLOG =  7.09782712893383996843E2         log(2**1024)
 * MINLOG = -7.08396418532264106224E2         log(2**-1022)
 * MAXNUM =  1.7976931348623158E308           2**1024
 *
 * The global symbols for mathematical constants are
 * PI     =  3.14159265358979323846           pi
 * PIO2   =  1.57079632679489661923           pi/2
 * PIO4   =  7.85398163397448309616E-1        pi/4
 * SQRT2  =  1.41421356237309504880           sqrt(2)
 * SQRTH  =  7.07106781186547524401E-1        sqrt(2)/2
 * LOG2E  =  1.4426950408889634073599         1/log(2)
 * SQ2OPI =  7.9788456080286535587989E-1      sqrt( 2/pi )
 * LOGE2  =  6.93147180559945309417E-1        log(2)
 * LOGSQ2 =  3.46573590279972654709E-1        log(2)/2
 * THPIO4 =  2.35619449019234492885           3*pi/4
 * TWOOPI =  6.36619772367581343075535E-1     2/pi
 *
 * These lists are subject to change.
 */
/*							const.c */
#if 1
double MACHEP =  1.11022302462515654042E-16;   /* 2**-53 */
#else
double MACHEP =  1.38777878078144567553E-17;   /* 2**-56 */
#endif
double UFLOWTHRESH =  2.22507385850720138309E-308; /* 2**-1022 */
#ifdef DENORMAL
double MAXLOG =  7.09782712893383996732E2;     /* log(MAXNUM) */
/* double MINLOG = -7.44440071921381262314E2; */     /* log(2**-1074) */
double MINLOG = -7.451332191019412076235E2;     /* log(2**-1075) */
#else
double MAXLOG =  7.08396418532264106224E2;     /* log 2**1022 */
double MINLOG = -7.08396418532264106224E2;     /* log 2**-1022 */
#endif
double MAXNUM =  1.79769313486231570815E308;    /* 2**1024*(1-MACHEP) */
double PI     =  3.14159265358979323846;       /* pi */
double PIO2   =  1.57079632679489661923;       /* pi/2 */
double PIO4   =  7.85398163397448309616E-1;    /* pi/4 */
double SQRT2  =  1.41421356237309504880;       /* sqrt(2) */
double SQRTH  =  7.07106781186547524401E-1;    /* sqrt(2)/2 */
double LOG2E  =  1.4426950408889634073599;     /* 1/log(2) */
double SQ2OPI =  7.9788456080286535587989E-1;  /* sqrt( 2/pi ) */
double LOGE2  =  6.93147180559945309417E-1;    /* log(2) */
double LOGSQ2 =  3.46573590279972654709E-1;    /* log(2)/2 */
double THPIO4 =  2.35619449019234492885;       /* 3*pi/4 */
double TWOOPI =  6.36619772367581343075535E-1; /* 2/pi */
#ifdef INFINITIES
double INFINITY = 1.0/0.0;  /* 99e999; */
#else
double INFINITY =  1.79769313486231570815E308;    /* 2**1024*(1-MACHEP) */
#endif
#ifdef NANS
double NAN = 1.0/0.0 - 1.0/0.0;
#else
double NAN = 0.0;
#endif
#ifdef MINUSZERO
double NEGZERO = -0.0;
#else
double NEGZERO = 0.0;
#endif
/* === const.c - end === */
/* === protos.h - start === */
/*
 *   This file was automatically generated by version 1.7 of cextract.
 *   Manual editing not recommended.
 *
 *   Created: Sun Jan  9 15:07:08 2000
 */
extern double Cabs ( cmplx *z );
extern void Cadd ( cmplx *a, cmplx *b, cmplx *c );
extern double cay ( double q );
extern void Cdiv ( cmplx *a, cmplx *b, cmplx *c );
extern void Cmov ( void *a, void *b );
extern void Cmul ( cmplx *a, cmplx *b, cmplx *c );
extern void Cneg ( cmplx *a );
extern void Csqrt ( cmplx *z, cmplx *w );
extern void Csub ( cmplx *a, cmplx *b, cmplx *c );
extern double ellie ( double phi, double m );
extern double ellik ( double phi, double m );
extern double ellpe ( double x );
extern int ellpj ( double u, double m, double *sn, double *cn, double *dn, double *ph );
extern double ellpk ( double x );
extern int lampln ( void );
extern int mtherr ( char *name, int code );
extern double p1evl ( double x, double coef[], int N );
extern double polevl ( double x, double coef[], int N );
extern int quadf ( double x, double y, int pzflg );
extern double response ( double f, double amp );
extern int spln ( void );
extern int zplna ( void );
extern int zplnb ( void );
extern int zplnc ( void );
/* === protos.h - end === */
/* === cmplx.c - start === */
/*							cmplx.c
 *
 *	Complex number arithmetic
 *
 *
 *
 * SYNOPSIS:
 *
 * typedef struct {
 *      double r;     real part
 *      double i;     imaginary part
 *     }cmplx;
 *
 * cmplx *a, *b, *c;
 *
 * Cadd( a, b, c );     c = b + a
 * Csub( a, b, c );     c = b - a
 * Cmul( a, b, c );     c = b * a
 * Cdiv( a, b, c );     c = b / a
 * Cneg( c );           c = -c
 * Cmov( b, c );        c = b
 *
 *
 *
 * DESCRIPTION:
 *
 * Addition:
 *    c.r  =  b.r + a.r
 *    c.i  =  b.i + a.i
 *
 * Subtraction:
 *    c.r  =  b.r - a.r
 *    c.i  =  b.i - a.i
 *
 * Multiplication:
 *    c.r  =  b.r * a.r  -  b.i * a.i
 *    c.i  =  b.r * a.i  +  b.i * a.r
 *
 * Division:
 *    d    =  a.r * a.r  +  a.i * a.i
 *    c.r  = (b.r * a.r  + b.i * a.i)/d
 *    c.i  = (b.i * a.r  -  b.r * a.i)/d
 * ACCURACY:
 *
 * In DEC arithmetic, the test (1/z) * z = 1 had peak relative
 * error 3.1e-17, rms 1.2e-17.  The test (y/z) * (z/y) = 1 had
 * peak relative error 8.3e-17, rms 2.1e-17.
 *
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
/*				cmplx.c
 * complex number arithmetic
 */
void Cdiv ( cmplx *, cmplx *, cmplx * );
void Cadd ( cmplx *, cmplx *, cmplx * );
extern double MAXNUM, MACHEP, PI, PIO2, INFINITY, NAN;
cmplx czero = {0.0, 0.0};
extern cmplx czero;
cmplx cone = {1.0, 0.0};
extern cmplx cone;
/*	c = b + a	*/
void Cadd( a, b, c )
     register cmplx *a, *b;
     cmplx *c;
{
  c->r = b->r + a->r;
  c->i = b->i + a->i;
}
/*	c = b - a	*/
void Csub( a, b, c )
     register cmplx *a, *b;
     cmplx *c;
{
  c->r = b->r - a->r;
  c->i = b->i - a->i;
}
/*	c = b * a */
void Cmul( a, b, c )
     register cmplx *a, *b;
     cmplx *c;
{
  double y;
  y    = b->r * a->r  -  b->i * a->i;
  c->i = b->r * a->i  +  b->i * a->r;
  c->r = y;
}
/*	c = b / a */
void Cdiv( a, b, c )
     register cmplx *a, *b;
     cmplx *c;
{
  double y, p, q, w;
  y = a->r * a->r  +  a->i * a->i;
  p = b->r * a->r  +  b->i * a->i;
  q = b->i * a->r  -  b->r * a->i;
  if( y < 1.0 )
    {
      w = MAXNUM * y;
      if( (fabs(p) > w) || (fabs(q) > w) || (y == 0.0) )
        {
          c->r = MAXNUM;
          c->i = MAXNUM;
          mtherr( "Cdiv", OVERFLOW );
          return;
        }
    }
  c->r = p/y;
  c->i = q/y;
}
/*	b = a
        Caution, a `short' is assumed to be 16 bits wide.  */
void Cmov( a, b )
     void *a, *b;
{
  register short *pa, *pb;
  int i;
  pa = (short *) a;
  pb = (short *) b;
  i = 8;
  do
    *pb++ = *pa++;
  while( --i );
}
void Cneg( a )
     register cmplx *a;
{
  a->r = -a->r;
  a->i = -a->i;
}
/*							Cabs()
 *
 *	Complex absolute value
 *
 *
 *
 * SYNOPSIS:
 *
 * double Cabs();
 * cmplx z;
 * double a;
 *
 * a = Cabs( &z );
 *
 *
 *
 * DESCRIPTION:
 *
 *
 * If z = x + iy
 *
 * then
 *
 *       a = sqrt( x**2 + y**2 ).
 * 
 * Overflow and underflow are avoided by testing the magnitudes
 * of x and y before squaring.  If either is outside half of
 * the floating point full scale range, both are rescaled.
 *
 *
 * ACCURACY:
 *
 *                      Relative error:
 * arithmetic   domain     # trials      peak         rms
 *    DEC       -30,+30     30000       3.2e-17     9.2e-18
 *    IEEE      -10,+10    100000       2.7e-16     6.9e-17
 */
#define PREC 27
#define MAXEXP 1024
#define MINEXP -1077
double Cabs( z )
     register cmplx *z;
{
  double x, y, b, re, im;
  int ex, ey, e;
#ifdef INFINITIES
  /* Note, Cabs(INFINITY,NAN) = INFINITY. */
  if( z->r == INFINITY || z->i == INFINITY
      || z->r == -INFINITY || z->i == -INFINITY )
    return( INFINITY );
#endif
#ifdef NANS
  if( isnan(z->r) )
    return(z->r);
  if( isnan(z->i) )
    return(z->i);
#endif
  re = fabs( z->r );
  im = fabs( z->i );
  if( re == 0.0 )
    return( im );
  if( im == 0.0 )
    return( re );
  /* Get the exponents of the numbers */
  x = frexp( re, &ex );
  y = frexp( im, &ey );
  /* Check if one number is tiny compared to the other */
  e = ex - ey;
  if( e > PREC )
    return( re );
  if( e < -PREC )
    return( im );
  /* Find approximate exponent e of the geometric mean. */
  e = (ex + ey) >> 1;
  /* Rescale so mean is about 1 */
  x = ldexp( re, -e );
  y = ldexp( im, -e );
  /* Hypotenuse of the right triangle */
  b = sqrt( x * x  +  y * y );
  /* Compute the exponent of the answer. */
  y = frexp( b, &ey );
  ey = e + ey;
  /* Check it for overflow and underflow. */
  if( ey > MAXEXP )
    {
      mtherr( "Cabs", OVERFLOW );
      return( INFINITY );
    }
  if( ey < MINEXP )
    return(0.0);
  /* Undo the scaling */
  b = ldexp( b, e );
  return( b );
}
/*							Csqrt()
 *
 *	Complex square root
 *
 *
 *
 * SYNOPSIS:
 *
 * void Csqrt();
 * cmplx z, w;
 *
 * Csqrt( &z, &w );
 *
 *
 *
 * DESCRIPTION:
 *
 *
 * If z = x + iy,  r = |z|, then
 *
 *                       1/2
 * Im w  =  [ (r - x)/2 ]   ,
 *
 * Re w  =  y / 2 Im w.
 *
 *
 * Note that -w is also a square root of z.  The root chosen
 * is always in the upper half plane.
 *
 * Because of the potential for cancellation error in r - x,
 * the result is sharpened by doing a Heron iteration
 * (see sqrt.c) in complex arithmetic.
 *
 *
 *
 * ACCURACY:
 *
 *                      Relative error:
 * arithmetic   domain     # trials      peak         rms
 *    DEC       -10,+10     25000       3.2e-17     9.6e-18
 *    IEEE      -10,+10    100000       3.2e-16     7.7e-17
 *
 *                        2
 * Also tested by Csqrt( z ) = z, and tested by arguments
 * close to the real axis.
 */
void Csqrt( z, w )
     cmplx *z, *w;
{
  cmplx q, s;
  double x, y, r, t;
  x = z->r;
  y = z->i;
  if( y == 0.0 )
    {
      if( x < 0.0 )
        {
          w->r = 0.0;
          w->i = sqrt(-x);
          return;
        }
      else
        {
          w->r = sqrt(x);
          w->i = 0.0;
          return;
        }
    }
  if( x == 0.0 )
    {
      r = fabs(y);
      r = sqrt(0.5*r);
      if( y > 0 )
        w->r = r;
      else
        w->r = -r;
      w->i = r;
      return;
    }
  /* Approximate  sqrt(x^2+y^2) - x  =  y^2/2x - y^4/24x^3 + ... .
   * The relative error in the first term is approximately y^2/12x^2 .
   */
  if( (fabs(y) < 2.e-4 * fabs(x))
      && (x > 0) )
    {
      t = 0.25*y*(y/x);
    }
  else
    {
      r = Cabs(z);
      t = 0.5*(r - x);
    }
  r = sqrt(t);
  q.i = r;
  q.r = y/(2.0*r);
  /* Heron iteration in complex arithmetic */
  Cdiv( &q, z, &s );
  Cadd( &q, &s, w );
  w->r *= 0.5;
  w->i *= 0.5;
}
double hypot( x, y )
     double x, y;
{
  cmplx z;
  z.r = x;
  z.i = y;
  return( Cabs(&z) );
}
/* === cmplx.c - end === */
/* === ellik.c - start === */
/*							ellik.c
 *
 *	Incomplete elliptic integral of the first kind
 *
 *
 *
 * SYNOPSIS:
 *
 * double phi, m, y, ellik();
 *
 * y = ellik( phi, m );
 *
 *
 *
 * DESCRIPTION:
 *
 * Approximates the integral
 *
 *
 *
 *                phi
 *                 -
 *                | |
 *                |           dt
 * F(phi_\m)  =    |    ------------------
 *                |                   2
 *              | |    sqrt( 1 - m sin t )
 *               -
 *                0
 *
 * of amplitude phi and modulus m, using the arithmetic -
 * geometric mean algorithm.
 *
 *
 *
 *
 * ACCURACY:
 *
 * Tested at random points with m in [0, 1] and phi as indicated.
 *
 *                      Relative error:
 * arithmetic   domain     # trials      peak         rms
 *    IEEE     -10,10       200000      7.4e-16     1.0e-16
 *
 *
 */
/*	Incomplete elliptic integral of first kind	*/
extern double ellpk ( double );
double ellik ( double, double );
extern double PI, PIO2, MACHEP, MAXNUM;
double ellik( phi, m )
     double phi, m;
{
  double a, b, c, e, temp, t, K;
  int d, mod, sign, npio2;
  if( m == 0.0 )
    return( phi );
  a = 1.0 - m;
  if( a == 0.0 )
    {
      if( fabs(phi) >= PIO2 )
        {
          mtherr( "ellik", SING );
          return( MAXNUM );
        }
      return(  log(  tan( (PIO2 + phi)/2.0 )  )   );
    }
  npio2 = floor( phi/PIO2 );
  if( npio2 & 1 )
    npio2 += 1;
  if( npio2 )
    {
      K = ellpk( a );
      phi = phi - npio2 * PIO2;
    }
  else
    K = 0.0;
  if( phi < 0.0 )
    {
      phi = -phi;
      sign = -1;
    }
  else
    sign = 0;
  b = sqrt(a);
  t = tan( phi );
  if( fabs(t) > 10.0 )
    {
      /* Transform the amplitude */
      e = 1.0/(b*t);
      /* ... but avoid multiple recursions.  */
      if( fabs(e) < 10.0 )
        {
          e = atan(e);
          if( npio2 == 0 )
            K = ellpk( a );
          temp = K - ellik( e, m );
          goto done;
        }
    }
  a = 1.0;
  c = sqrt(m);
  d = 1;
  mod = 0;
  while( fabs(c/a) > MACHEP )
    {
      temp = b/a;
      phi = phi + atan(t*temp) + mod * PI;
      mod = (phi + PIO2)/PI;
      t = t * ( 1.0 + temp )/( 1.0 - temp * t * t );
      c = ( a - b )/2.0;
      temp = sqrt( a * b );
      a = ( a + b )/2.0;
      b = temp;
      d += d;
    }
  temp = (atan(t) + mod * PI)/(d * a);
 done:
  if( sign < 0 )
    temp = -temp;
  temp += npio2 * K;
  return( temp );
}
/* === ellik.c - end === */
/* === ellpe.c - start === */
/*							ellpe.c
 *
 *	Complete elliptic integral of the second kind
 *
 *
 *
 * SYNOPSIS:
 *
 * double m1, y, ellpe();
 *
 * y = ellpe( m1 );
 *
 *
 *
 * DESCRIPTION:
 *
 * Approximates the integral
 *
 *
 *            pi/2
 *             -
 *            | |                 2
 * E(m)  =    |    sqrt( 1 - m sin t ) dt
 *          | |    
 *           -
 *            0
 *
 * Where m = 1 - m1, using the approximation
 *
 *      P(x)  -  x log x Q(x).
 *
 * Though there are no singularities, the argument m1 is used
 * rather than m for compatibility with ellpk().
 *
 * E(1) = 1; E(0) = pi/2.
 *
 *
 * ACCURACY:
 *
 *                      Relative error:
 * arithmetic   domain     # trials      peak         rms
 *    DEC        0, 1       13000       3.1e-17     9.4e-18
 *    IEEE       0, 1       10000       2.1e-16     7.3e-17
 *
 *
 * ERROR MESSAGES:
 *
 *   message         condition      value returned
 * ellpe domain      x<0, x>1            0.0
 *
 */
/*							ellpe.c		*/
/* Elliptic integral of second kind */
static double P_ellpe[] = {
  1.53552577301013293365E-4,
  2.50888492163602060990E-3,
  8.68786816565889628429E-3,
  1.07350949056076193403E-2,
  7.77395492516787092951E-3,
  7.58395289413514708519E-3,
  1.15688436810574127319E-2,
  2.18317996015557253103E-2,
  5.68051945617860553470E-2,
  4.43147180560990850618E-1,
  1.00000000000000000299E0
};
static double Q_ellpe[] = {
  3.27954898576485872656E-5,
  1.00962792679356715133E-3,
  6.50609489976927491433E-3,
  1.68862163993311317300E-2,
  2.61769742454493659583E-2,
  3.34833904888224918614E-2,
  4.27180926518931511717E-2,
  5.85936634471101055642E-2,
  9.37499997197644278445E-2,
  2.49999999999888314361E-1
};
extern double polevl ( double, double[], int );
extern double log ( double );
double ellpe(x)
     double x;
{
  if( (x <= 0.0) || (x > 1.0) )
    {
      if( x == 0.0 )
        return( 1.0 );
      mtherr( "ellpe", DOMAIN );
      return( 0.0 );
    }
  return( polevl(x,P_ellpe,10) - log(x) * (x * polevl(x,Q_ellpe,9)) );
}
/* === ellpe.c - end === */
/* === ellpj.c - start === */
/*							ellpj.c
 *
 *	Jacobian Elliptic Functions
 *
 *
 *
 * SYNOPSIS:
 *
 * double u, m, sn, cn, dn, phi;
 * int ellpj();
 *
 * ellpj( u, m, _&sn, _&cn, _&dn, _&phi );
 *
 *
 *
 * DESCRIPTION:
 *
 *
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
 *
 * Tested at random points with u between 0 and 10, m between
 * 0 and 1.
 *
 *            Absolute error (* = relative error):
 * arithmetic   function   # trials      peak         rms
 *    DEC       sn           1800       4.5e-16     8.7e-17
 *    IEEE      phi         10000       9.2e-16*    1.4e-16*
 *    IEEE      sn          50000       4.1e-15     4.6e-16
 *    IEEE      cn          40000       3.6e-15     4.4e-16
 *    IEEE      dn          10000       1.3e-12     1.8e-14
 *
 *  Peak error observed in consistency check using addition
 * theorem for sn(u+v) was 4e-16 (absolute).  Also tested by
 * the above relation to the incomplete elliptic integral.
 * Accuracy deteriorates when u is large.
 *
 */
/*							ellpj.c		*/
extern double PIO2, MACHEP;
int ellpj( u, m, sn, cn, dn, ph )
     double u, m;
     double *sn, *cn, *dn, *ph;
{
  double ai, b, phi, t, twon;
  double a[9], c[9];
  int i;
  /* Check for special cases */
  if( m < 0.0 || m > 1.0 )
    {
      mtherr( "ellpj", DOMAIN );
      *sn = 0.0;
      *cn = 0.0;
      *ph = 0.0;
      *dn = 0.0;
      return(-1);
    }
  if( m < 1.0e-9 )
    {
      t = sin(u);
      b = cos(u);
      ai = 0.25 * m * (u - t*b);
      *sn = t - ai*b;
      *cn = b + ai*t;
      *ph = u - ai;
      *dn = 1.0 - 0.5*m*t*t;
      return(0);
    }
  if( m >= 0.9999999999 )
    {
      ai = 0.25 * (1.0-m);
      b = cosh(u);
      t = tanh(u);
      phi = 1.0/b;
      twon = b * sinh(u);
      *sn = t + ai * (twon - u)/(b*b);
      *ph = 2.0*atan(exp(u)) - PIO2 + ai*(twon - u)/b;
      ai *= t * phi;
      *cn = phi - ai * (twon - u);
      *dn = phi + ai * (twon + u);
      return(0);
    }
  /*	A. G. M. scale		*/
  a[0] = 1.0;
  b = sqrt(1.0 - m);
  c[0] = sqrt(m);
  twon = 1.0;
  i = 0;
  while( fabs(c[i]/a[i]) > MACHEP )
    {
      if( i > 7 )
        {
          mtherr( "ellpj", OVERFLOW );
          goto done;
        }
      ai = a[i];
      ++i;
      c[i] = ( ai - b )/2.0;
      t = sqrt( ai * b );
      a[i] = ( ai + b )/2.0;
      b = t;
      twon *= 2.0;
    }
 done:
  /* backward recurrence */
  phi = twon * a[i] * u;
  do
    {
      t = c[i] * sin(phi) / a[i];
      b = phi;
      phi = (asin(t) + phi)/2.0;
    }
  while( --i );
  *sn = sin(phi);
  t = cos(phi);
  *cn = t;
  *dn = t/cos(phi-b);
  *ph = phi;
  return(0);
}
/* === ellpj.c - end === */
/* === ellpk.c - start === */
/*							ellpk.c
 *
 *	Complete elliptic integral of the first kind
 *
 *
 *
 * SYNOPSIS:
 *
 * double m1, y, ellpk();
 *
 * y = ellpk( m1 );
 *
 *
 *
 * DESCRIPTION:
 *
 * Approximates the integral
 *
 *
 *
 *            pi/2
 *             -
 *            | |
 *            |           dt
 * K(m)  =    |    ------------------
 *            |                   2
 *          | |    sqrt( 1 - m sin t )
 *           -
 *            0
 *
 * where m = 1 - m1, using the approximation
 *
 *     P(x)  -  log x Q(x).
 *
 * The argument m1 is used rather than m so that the logarithmic
 * singularity at m = 1 will be shifted to the origin; this
 * preserves maximum accuracy.
 *
 * K(0) = pi/2.
 *
 * ACCURACY:
 *
 *                      Relative error:
 * arithmetic   domain     # trials      peak         rms
 *    DEC        0,1        16000       3.5e-17     1.1e-17
 *    IEEE       0,1        30000       2.5e-16     6.8e-17
 *
 * ERROR MESSAGES:
 *
 *   message         condition      value returned
 * ellpk domain       x<0, x>1           0.0
 *
 */
/*							ellpk.c */
static double P_ellpk[] =
{
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
static double Q_ellpk[] =
{
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
static double C1 = 1.3862943611198906188E0; /* log(4) */
extern double polevl ( double, double[], int );
extern double p1evl ( double, double[], int );
extern double log ( double );
extern double MACHEP, MAXNUM;
double ellpk(x)
     double x;
{
  if( (x < 0.0) || (x > 1.0) )
    {
      mtherr( "ellpk", DOMAIN );
      return( 0.0 );
    }
  if( x > MACHEP )
    {
      return( polevl(x,P_ellpk,10) - log(x) * polevl(x,Q_ellpk,10) );
    }
  else
    {
      if( x == 0.0 )
        {
          mtherr( "ellpk", SING );
          return( MAXNUM );
        }
      else
        {
          return( C1 - 0.5 * log(x) );
        }
    }
}
/* === ellpk.c - end === */
/* === mtherr.c - start === */
/*							mtherr.c
 *
 *	Library common error handling routine
 *
 *
 *
 * SYNOPSIS:
 *
 * char *fctnam;
 * int code;
 * int mtherr();
 *
 * mtherr( fctnam, code );
 *
 *
 *
 * DESCRIPTION:
 *
 * This routine may be called to report one of the following
 * error conditions (in the include file mconf.h).
 *  
 *   Mnemonic        Value          Significance
 *
 *    DOMAIN            1       argument domain error
 *    SING              2       function singularity
 *    OVERFLOW          3       overflow range error
 *    UNDERFLOW         4       underflow range error
 *    TLOSS             5       total loss of precision
 *    PLOSS             6       partial loss of precision
 *    EDOM             33       Unix domain error code
 *    ERANGE           34       Unix range error code
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
 *
 * SEE ALSO:
 *
 * mconf.h
 *
 */
#include <stdio.h>
int merror = 0;
/* Notice: the order of appearance of the following
 * messages is bound to the error codes defined
 * in mconf.h.
 */
static char *ermsg[7] = {
  "unknown",      /* error code 0 */
  "domain",       /* error code 1 */
  "singularity",  /* et seq.      */
  "overflow",
  "underflow",
  "total loss of precision",
  "partial loss of precision"
};
int mtherr( name, code )
     char *name;
     int code;
{
  /* Display string passed by calling program,
   * which is supposed to be the name of the
   * function in which the error occurred:
   */
  ellf_debugf ( "\n%s ", name );
  /* Set global error message word */
  merror = code;
  /* Display error message defined
   * by the code argument.
   */
  if( (code <= 0) || (code >= 7) )
    code = 0;
  ellf_debugf ( "%s error\n", ermsg[code] );
  /* Return to calling
   * program
   */
  return( 0 );
}
/* === mtherr.c - end === */
/* === polevl.c - start === */
/*							polevl.c
 *							p1evl.c
 *
 *	Evaluate polynomial
 *
 *
 *
 * SYNOPSIS:
 *
 * int N;
 * double x, y, coef[N+1], polevl[];
 *
 * y = polevl( x, coef, N );
 *
 *
 *
 * DESCRIPTION:
 *
 * Evaluates polynomial of degree N:
 *
 *                     2          N
 * y  =  C  + C x + C x  +...+ C x
 *        0    1     2          N
 *
 * Coefficients are stored in reverse order:
 *
 * coef[0] = C  , ..., coef[N] = C  .
 *            N                   0
 *
 *  The function p1evl() assumes that coef[N] = 1.0 and is
 * omitted from the array.  Its calling arguments are
 * otherwise the same as polevl().
 *
 *
 * SPEED:
 *
 * In the interest of speed, there are no checks for out
 * of bounds arithmetic.  This routine is used by most of
 * the functions in the library.  Depending on available
 * equipment features, the user may wish to rewrite the
 * program in microcode or assembly language.
 *
 */
double polevl( x, coef, N )
     double x;
     double coef[];
     int N;
{
  double ans;
  int i;
  double *p;
  p = coef;
  ans = *p++;
  i = N;
  do
    ans = ans * x  +  *p++;
  while( --i );
  return( ans );
}
/*							p1evl()	*/
/*                                          N
 * Evaluate polynomial when coefficient of x  is 1.0.
 * Otherwise same as polevl.
 */
double p1evl( x, coef, N )
     double x;
     double coef[];
     int N;
{
  double ans;
  double *p;
  int i;
  p = coef;
  ans = x + *p++;
  i = N-1;
  do
    ans = ans * x  + *p++;
  while( --i );
  return( ans );
}
/* === polevl.c - end === */
/* === ellf.c - start === */
/* ellf.c
 * 
 * Read ellf.doc before attempting to compile this program.
 */
#include <stdio.h>
#include <stdlib.h>
/* size of arrays: */
#define ARRSIZ 300
/* System configurations */
extern double PI, PIO2, MACHEP, MAXNUM;
static double aa[ARRSIZ];
static double pp[ARRSIZ];
static double y[ARRSIZ];
static double zs[ARRSIZ];
cmplx z[ARRSIZ];
static double wr = 0.0;
static double cbp = 0.0;
static double wc = 0.0;
static double rn = 8.0;
static double c = 0.0;
static double cgam = 0.0;
static double scale = 0.0;
double fs = 1.0e4;	      /* sampling frequency  -- stw */
static double dbr = 0.5;
static double dbd = -40.0;
static double f1 = 1.5e3;
static double f2 = 2.0e3;
static double f3 = 2.4e3;
double dbfac = 0.0;
static double a = 0.0;
static double b = 0.0;
static double q = 0.0;
static double r = 0.0;
static double u = 0.0;
static double k = 0.0;
static double m = 0.0;
static double Kk = 0.0;
static double Kk1 = 0.0;
static double Kpk = 0.0;
static double Kpk1 = 0.0;
static double eps = 0.0;
static double rho = 0.0;
static double phi = 0.0;
static double sn = 0.0;
static double cn = 0.0;
static double dn = 0.0;
static double sn1 = 0.0;
static double cn1 = 0.0;
static double dn1 = 0.0;
static double phi1 = 0.0;
static double m1 = 0.0;
static double m1p = 0.0;
static double cang = 0.0;
static double sang = 0.0;
static double bw = 0.0;
static double ang = 0.0;
double fnyq = 0.0;	      /* nyquist frequency  -- stw */
static double ai = 0.0;
static double pn = 0.0;
static double an = 0.0;
static double gam = 0.0;
static double cng = 0.0;
double gain = 0.0;
static int lr = 0;
static int nt = 0;
static int i = 0;
static int j = 0;
static int jt = 0;
static int nc = 0;
static int ii = 0;
static int ir = 0;
int zord = 0;
static int icnt = 0;
static int mh = 0;
static int jj = 0;
static int jh = 0;
static int jl = 0;
static int n = 8;
static int np = 0;
static int nz = 0;
static int type = 1;
static int kind = 1;
static char wkind[] =
{"Filter kind:\n1 Butterworth\n2 Chebyshev\n3 Elliptic\n"};
static char salut[] =
{"Filter shape:\n1 low pass\n2 band pass\n3 high pass\n4 band stop\n"};
enum {
  BUTTERWORTH = 1,
  CHEBYSHEV = 2,
  ELLIPTIC = 3
};
enum {
  LOW_PASS = 1,
  BAND_PASS = 2,
  HIGH_PASS = 3,
  BAND_STOP = 4
};
extern double Cabs ( cmplx *z );
extern void Cadd ( cmplx *a, cmplx *b, cmplx *c );
extern void Cdiv ( cmplx *a, cmplx *b, cmplx *c );
extern void Cmov ( void *a, void *b );
extern void Cmul ( cmplx *a, cmplx *b, cmplx *c );
extern void Cneg ( cmplx *a );
extern void Csqrt ( cmplx *z, cmplx *w );
extern void Csub ( cmplx *a, cmplx *b, cmplx *c );
extern double ellie ( double phi, double m );
extern double ellik ( double phi, double m );
extern double ellpe ( double x );
extern int ellpj ( double, double, double *, double *, double *, double * );
extern double ellpk ( double x );
double cay ( double q );
int lampln ( void );
int spln ( void );
void print_filter_table (void);
int zplna ( void );
int zplnb ( void );
int zplnc ( void );
int quadf ( double, double, int );
double response ( double, double );
#define MIN(a,b) ((a) <= (b) ? (a) : (b))
#define MAX(a,b) ((a) >= (b) ? (a) : (b))
static void
print_z_fraction_before_zplnc (void) /* must be called *before* zplnc() */
{
  double zgain;
  if (kind != 3 && pn == 0)
    zgain = 1.0;
  else
    zgain = an / (pn * scale);
  const char *kind_string = kind == 1 ? "BSE_IIR_FILTER_BUTTERWORTH" : kind == 2 ? "BSE_IIR_FILTER_CHEBYSHEV1" : "BSE_IIR_FILTER_ELLIPTIC";
  const char *type_string = (type == 1 ? "BSE_IIR_FILTER_LOW_PASS" : type == 2 ? "BSE_IIR_FILTER_BAND_PASS" :
                             type == 3 ? "BSE_IIR_FILTER_HIGH_PASS" : "BSE_IIR_FILTER_BAND_STOP");
  code_printf ("  {\n");
  code_printf ("    static const BseIIRFilterRequest filter_request = {\n");
  code_printf ("      /* kind = */               %s,\n", kind_string);
  code_printf ("      /* type = */               %s,\n", type_string);
  code_printf ("      /* order = */              %u,\n", (int) rn);
  code_printf ("      /* sampling_frequency = */ %.4f,\n", fs);
  code_printf ("      /* passband_ripple_db = */ %.17g,\n", kind > 1 ? dbr : 0);
  if ((type & 1) == 0)
    {
      code_printf ("      /* passband_edge = */      %.17g,\n", MIN (f1, f2));
      code_printf ("      /* passband_edge2 = */     %.17g,\n", MAX (f1, f2));
    }
  else
    {
      code_printf ("      /* passband_edge = */      %.17g,\n", f2);
      code_printf ("      /* passband_edge2 = */     %.17g,\n", 0.);
    }
  code_printf ("      /* stopband_edge = */      %.17g,\n", kind == 3 && dbd > 0.0 ? dbd : 0);
  code_printf ("      /* stopband_db = */        %.17g,\n", kind == 3 && dbd < 0.0 ? dbd : 0);
  code_printf ("    };\n");
  int j;
  code_printf ("    static const double zeros[] = {\n");
  for (j = 0; j < zord; j++)
    if (z[j + zord].i >= 0.0)
      code_printf ("      %+.17e, %+.17e,\n", z[j + zord].r, z[j + zord].i);
  code_printf ("    };\n");
  code_printf ("    static const double poles[] = {\n");
  for (j = 0; j < zord; j++)
    if (z[j].i >= 0.0)
      code_printf ("      %+.17e, %+.17e, /* pole */\n", z[j].r, z[j].i);
  code_printf ("    };\n");
  code_printf ("    filters[index].filter_request = &filter_request;\n");
  code_printf ("    filters[index].gain = %+.17e;\n", zgain);
  code_printf ("    filters[index].n_zeros = BIRNET_ARRAY_SIZE (zeros) / 2;\n");
  code_printf ("    filters[index].zeros = zeros;\n");
  code_printf ("    filters[index].n_poles = BIRNET_ARRAY_SIZE (poles) / 2;\n");
  code_printf ("    filters[index].poles = poles;\n");
  code_printf ("    index++;\n");
  code_printf ("  }\n");
}
static inline unsigned int
quick_rand32 (void)
{
  static unsigned int accu = 2147483563;
  accu = 1664525 * accu + 1013904223;
  return accu + lrand48();
}
static inline double
quick_rand_range (double s, double e)
{
  double rand_double1;
  do
    {
      rand_double1 = quick_rand32() * 2.32830643708079737531e-10; // [0..2^32-1] -> [0..1]
      rand_double1 = (rand_double1 + quick_rand32()) * 2.32830643708079737531e-10;
    }
  while (rand_double1 > 1); // fix rounding errors
  return s + rand_double1 * (e - s);
}
typedef struct {
  int    kind, type;
  double order;
  double sampling_frequency;
  double passband_ripple_db;
  double passband_edge;
  double passband_edge2;
  double stopband_edge_or_db;
} EllfInput;
static void main_ellf (const EllfInput *einput);
static void
generate_filters_brute_force (void)
{
  EllfInput einput = { 0, };
  const double sampling_frequencies[/* prime! */] = {
    100, 1000, 2000, 3333, 4000, 8000, 12000, 16000, 20050, 32000, 44100, 48000, 56000, 64000,
    72000, 88000, 96000, 128000, 192000, 256000, 512000, 1024 * 1024, 1024 * 1024 * 1024,
  };
  const int n_sampling_frequencies = sizeof (sampling_frequencies) / sizeof (sampling_frequencies[0]);
  const int filter_orders[] = {
    1, 2, 3, 4, 5,
    0, 0, 0, 0, 0, /* used for random orders */
  };
  const int n_orders = sizeof (filter_orders) / sizeof (filter_orders[0]);
  int oix, max_order_index = 0;
  for (oix = 0; oix < n_orders; oix++)
    max_order_index = filter_orders[oix] ? oix : max_order_index;
  int rand_order_width = (32 - filter_orders[max_order_index]);
  double pbe1;
#if 1
  einput.kind = BUTTERWORTH;
  for (oix = 0; oix < n_orders; oix++)
    for (pbe1 = 0.15; pbe1 <= 0.46; pbe1 += 0.15)
      {
        /* low/high */
        einput.order = oix <= max_order_index ? filter_orders[oix] : filter_orders[max_order_index] + quick_rand32() % rand_order_width;
        einput.sampling_frequency = sampling_frequencies[quick_rand32() % n_sampling_frequencies];
        einput.passband_edge = pbe1 * einput.sampling_frequency;
        einput.type = LOW_PASS;
        main_ellf (&einput);
        einput.type = HIGH_PASS;
        main_ellf (&einput);
        /* band filters */
        if (pbe1 + 0.1 < 0.48)
          {
            einput.passband_edge2 = quick_rand_range (pbe1 + 0.1, 0.48) * einput.sampling_frequency;
            einput.type = BAND_PASS; main_ellf (&einput); einput.type = BAND_STOP; main_ellf (&einput);
          }
      }
#endif
#if 1
  einput.kind = CHEBYSHEV;
  for (oix = 0; oix < n_orders; oix++)
    for (pbe1 = 0.15; pbe1 <= 0.46; pbe1 += 0.15)
      {
        einput.order = oix <= max_order_index ? filter_orders[oix] : filter_orders[max_order_index] + quick_rand32() % rand_order_width;
        /* low/high */
        einput.sampling_frequency = sampling_frequencies[quick_rand32() % n_sampling_frequencies];
        einput.passband_ripple_db = quick_rand_range (0, 0.001) * pow (9, quick_rand32() % 5); /* 0.001 .. 6.6 */
        einput.passband_edge = pbe1 * einput.sampling_frequency;
        einput.type = LOW_PASS; main_ellf (&einput);
        einput.passband_ripple_db = quick_rand_range (0, 0.001) * pow (9, quick_rand32() % 5); /* 0.001 .. 6.6 */
        einput.type = HIGH_PASS; main_ellf (&einput);
        /* band filters */
        if (pbe1 + 0.1 < 0.48)
          {
            einput.passband_edge2 = quick_rand_range (pbe1 + 0.1, 0.48) * einput.sampling_frequency;
            einput.type = BAND_PASS; main_ellf (&einput);
            einput.passband_ripple_db = quick_rand_range (0, 0.001) * pow (9, quick_rand32() % 5); /* 0.001 .. 6.6 */
            einput.type = BAND_STOP; main_ellf (&einput);
          }
      }
#endif
#if 1
  einput.kind = ELLIPTIC;
  for (oix = 0; oix < n_orders; oix++)
    for (pbe1 = 0.15; pbe1 <= 0.46; pbe1 += 0.15)
      {
        einput.order = oix <= max_order_index ? filter_orders[oix] : filter_orders[max_order_index] + quick_rand32() % rand_order_width;
        /* low/high */
        einput.sampling_frequency = sampling_frequencies[quick_rand32() % n_sampling_frequencies];
        einput.passband_ripple_db = quick_rand_range (0, 0.001) * pow (9, quick_rand32() % 5); /* 0.001 .. 6.6 */
        einput.passband_edge = pbe1 * einput.sampling_frequency;
        einput.stopband_edge_or_db = quick_rand_range (-9, -150) - 2 * einput.order;
        einput.type = LOW_PASS; main_ellf (&einput);
        einput.passband_ripple_db = quick_rand_range (0, 0.001) * pow (9, quick_rand32() % 5); /* 0.001 .. 6.6 */
        einput.type = HIGH_PASS; main_ellf (&einput);
        /* band filters */
        if (pbe1 + 0.1 < 0.48)
          {
            einput.passband_edge2 = quick_rand_range (pbe1 + 0.1, 0.48) * einput.sampling_frequency;
            fprintf (stderr, "PBE: %f %f\n", pbe1, einput.passband_edge2 / einput.sampling_frequency);
            einput.stopband_edge_or_db = quick_rand_range (-16, -150) - 2 * einput.order;
            einput.type = BAND_PASS; main_ellf (&einput);
            einput.passband_ripple_db = quick_rand_range (0, 0.001) * pow (9, quick_rand32() % 5); /* 0.001 .. 6.6 */
            einput.type = BAND_STOP; main_ellf (&einput);
          }
      }
#endif
}
int
main (int   argc,
      char *argv[])
{
  /* initialize random numbers */
  {
    struct timeval tv;
    gettimeofday (&tv, NULL);
    srand48 (tv.tv_usec + (tv.tv_sec << 16));
    srand (lrand48());
  }
  if (argc >= 2 && strcmp (argv[1], "--test-code") == 0)
    {
      generate_filters_brute_force();
    }
  else
    main_ellf (NULL);
  return 0;
}
/* Get a number from keyboard.
 * Display previous value and keep it if user just hits <CR>.
 */
int
getnum (char *line, double *val, const double *einput)
{
  char s[40];
  ellf_debugf ( "%s = %.9E ? ", line, *val );
  if (einput)
    {
      *val = *einput;
      ellf_debugf ( "%.9E\n", *val );
      return 0;
    }
  if (!fgets (s, sizeof (s), stdin))
    exit (0);
  if( s[0] != '\0' )
    {
      sscanf( s, "%lf", val );
      ellf_debugf ( "%.9E\n", *val );
    }
  return 0;
}
static void
main_ellf (const EllfInput *einput)
{
  char str[80];
  dbfac = 10.0/log(10.0);
 top:
  ellf_debugf ( "%s ? ", wkind );	/* ask for filter kind */
  if (einput)
    kind = einput->kind;
  else
    {
      if (!fgets (str, sizeof (str), stdin))
        exit (0);
      sscanf( str, "%d", &kind );
    }
  ellf_debugf ( "%d\n", kind );
  if( (kind <= 0) || (kind > 3) )
    exit(0);
  ellf_debugf ( "%s ? ", salut );	/* ask for filter type */
  if (einput)
    type = einput->type;
  else
    {
      if (!fgets (str, sizeof (str), stdin))
        exit (0);
      sscanf( str, "%d", &type );
    }
  ellf_debugf ( "%d\n", type );
  if( (type <= 0) || (type > 4) )
    exit(0);
  getnum( "Order of filter", &rn, einput ? &einput->order : NULL); /* see below for getnum() */
  n = rn;
  if( n <= 0 )
    {
          ellf_debugf ("? Need order > 0\n");
    specerr:
      ellf_debugf ( "? Specification error\n" );
      if (einput)
        exit (255);
      else
        goto top;
    }
  rn = n;	/* ensure it is an integer */
  if( kind > 1 ) /* not Butterworth */
    {
      getnum( "Passband ripple, db", &dbr, einput ? &einput->passband_ripple_db : NULL);
      if( dbr <= 0.0 )
        {
          ellf_debugf ("? Need passband-ripple > 0\n");
          goto specerr;
        }
      if( kind == 2 )
        {
          /* For Chebyshev filter, ripples go from 1.0 to 1/sqrt(1+eps^2) */
          phi = exp( 0.5*dbr/dbfac );
          if( (n & 1) == 0 )
            scale = phi;
          else
            scale = 1.0;
        }
      else
        { /* elliptic */
          eps = exp( dbr/dbfac );
          scale = 1.0;
          if( (n & 1) == 0 )
            scale = sqrt( eps );
          eps = sqrt( eps - 1.0 );
        }
    }
  getnum( "Sampling frequency", &fs, einput ? &einput->sampling_frequency : NULL);
  if( fs <= 0.0 )
    {
      ellf_debugf ("? Need sampling-frequency > 0\n");
      goto specerr;
    }
  fnyq = 0.5 * fs;
  getnum( "Passband edge", &f2, einput ? &einput->passband_edge : NULL);
  if( (f2 <= 0.0))
    {
      ellf_debugf ("? Need passband-edge > 0\n");
      goto specerr;
    }
  if( (f2 >= fnyq) )
    {
      ellf_debugf ("? Need passband-edge < nyquist frequency (%f)\n", fnyq);
      goto specerr;
    }
  if( (type & 1) == 0 )
    {
      getnum( "Other passband edge", &f1, einput ? &einput->passband_edge2 : NULL);
      if( (f1 <= 0.0) )
        {
          ellf_debugf ("? Need passband-edge2 > 0\n");
          goto specerr;
        }
      if( (f1 >= fnyq) )
        {
          ellf_debugf ("? Need passband-edge2 < nyquist frequency (%f)\n", fnyq);
          goto specerr;
        }
    }
  else
    {
      f1 = 0.0;
    }
  if( f2 < f1 )
    {
      a = f2;
      f2 = f1;
      f1 = a;
    }
  if( type == 3 )	/* high pass */
    {
      bw = f2;
      a = fnyq;
    }
  else
    {
      bw = f2 - f1;
      a = f2;
    }
  /* Frequency correspondence for bilinear transformation
   *
   *  Wanalog = tan( 2 pi Fdigital T / 2 )
   *
   * where T = 1/fs
   */
  ang = bw * PI / fs;
  cang = cos( ang );
  c = sin(ang) / cang; /* Wanalog */
  if( kind != 3 )
    {
      wc = c;
      /*ellf_debugf( "cos( 1/2 (Whigh-Wlow) T ) = %.5e, wc = %.5e\n", cang, wc );*/
    }
  if( kind == 3 )
    { /* elliptic */
      cgam = cos( (a+f1) * PI / fs ) / cang;
      getnum( "Stop band edge or -(db down)", &dbd, einput ? &einput->stopband_edge_or_db : NULL);
      if( dbd > 0.0 )
        f3 = dbd;
      else
        { /* calculate band edge from db down */
          a = exp( -dbd/dbfac );
          m1 = eps/sqrt( a - 1.0 );
          m1 *= m1;
          m1p = 1.0 - m1;
          Kk1 = ellpk( m1p );
          Kpk1 = ellpk( m1 );
          q = exp( -PI * Kpk1 / (rn * Kk1) );
          k = cay(q);
          if( type >= 3 )
            wr = k;
          else
            wr = 1.0/k;
          if( type & 1 )
            {
              f3 = atan( c * wr ) * fs / PI;
            }
          else
            {
              a = c * wr;
              a *= a;
              b = a * (1.0 - cgam * cgam) + a * a;
              b = (cgam + sqrt(b))/(1.0 + a);
              f3 = (PI/2.0 - asin(b)) * fs / (2.0*PI);
            }
        }
      switch( type )
	{
	case 1:
          if( f3 <= f2 )
            {
              ellf_debugf ("? Need stopband_edge > passband_edge\n");
              goto specerr;
            }
          break;
	case 2:
          if( (f3 > f2) || (f3 < f1) )
            break;
          ellf_debugf ("? Need stopband_edge < passband_edge or stopband_edge > passband_edge2\n");
          goto specerr;
	case 3:
          if( f3 >= f2 )
            {
              ellf_debugf ("? Need stopband_edge < passband_edge\n");
              goto specerr;
            }
          break;
	case 4:
          if( (f3 <= f1) || (f3 >= f2) )
            {
              ellf_debugf ("? Need stopband_edge > passband_edge2 and stopband_edge < passband_edge\n");
              goto specerr;
            }
          break;
	}
      ang = f3 * PI / fs;
      cang = cos(ang);
      sang = sin(ang);
      if( type & 1 )
	{
          wr = sang/(cang*c);
	}
      else
	{
          q = cang * cang  -  sang * sang;
          sang = 2.0 * cang * sang;
          cang = q;
          wr = (cgam - cang)/(sang * c);
	}
      if( type >= 3 )
	wr = 1.0/wr;
      if( wr < 0.0 )
	wr = -wr;
      y[0] = 1.0;
      y[1] = wr;
      cbp = wr;
      if( type >= 3 )
	y[1] = 1.0/y[1];
      if( type & 1 )
	{
          for( i=1; i<=2; i++ )
            {
              aa[i] = atan( c * y[i-1] ) * fs / PI ;
            }
          ellf_debugf ( "pass band %.9E\n", aa[1] );
          ellf_debugf ( "stop band %.9E\n", aa[2] );
	}
      else
	{
          for( i=1; i<=2; i++ )
            {
              a = c * y[i-1];
              b = atan(a);
              q = sqrt( 1.0 + a * a  -  cgam * cgam );
#ifdef ANSIC
              q = atan2( q, cgam );
#else
              q = atan2( cgam, q );
#endif
              aa[i] = (q + b) * fnyq / PI;
              pp[i] = (q - b) * fnyq / PI;
            }
          ellf_debugf ( "pass band %.9E %.9E\n", pp[1], aa[1] );
          ellf_debugf ( "stop band %.9E %.9E\n", pp[2], aa[2] );
	}
      lampln();	/* find locations in lambda plane */
      if( (2*n+2) > ARRSIZ )
	goto toosml;
    }
  /* Transformation from low-pass to band-pass critical frequencies
   *
   * Center frequency
   *                     cos( 1/2 (Whigh+Wlow) T )
   *  cos( Wcenter T ) = ----------------------
   *                     cos( 1/2 (Whigh-Wlow) T )
   *
   *
   * Band edges
   *            cos( Wcenter T) - cos( Wdigital T )
   *  Wanalog = -----------------------------------
   *                        sin( Wdigital T )
   */
  if( kind == 2 )
    { /* Chebyshev */
      a = PI * (a+f1) / fs ;
      cgam = cos(a) / cang;
      a = 2.0 * PI * f2 / fs;
      cbp = (cgam - cos(a))/sin(a);
    }
  if( kind == 1 )
    { /* Butterworth */
      a = PI * (a+f1) / fs ;
      cgam = cos(a) / cang;
      a = 2.0 * PI * f2 / fs;
      cbp = (cgam - cos(a))/sin(a);
      scale = 1.0;
    }
  ellf_debugf ("State: gain_scale=%.20g ripple_epsilon=%.20g nyquist_frequency=%.20g " // BSE info 
               "tan_angle_frequency=%.20g stopband_edge=%.20g wc=%.20g wr=%.20g cgam=%.20g\n",
               scale, eps, fnyq,
               c, f3, wc, wr, cgam);
  spln();		/* find s plane poles and zeros */
  if( ((type & 1) == 0) && ((4*n+2) > ARRSIZ) )
    goto toosml;
  zplna();	/* convert s plane to z plane */
  zplnb();
  ellf_debugf ("an=%.20g pn=%.20g scale=%.20g\n", an, pn, scale); // BSE info
  print_z_fraction_before_zplnc ();
  zplnc();
  print_filter_table(); /* tabulate transfer function */
  if (einput)
    return;
  else
    goto top;
 toosml:
  ellf_debugf ( "Cannot continue, storage arrays too small\n" );
  if (einput)
    exit (255);
  else
    goto top;
}
int lampln()
{
  wc = 1.0;
  k = wc/wr;
  m = k * k;
  Kk = ellpk( 1.0 - m );
  Kpk = ellpk( m );
  ellf_debugf ("check: k=%.20g m=%.20g Kk=%.20g Kpk=%.20g\n", k, m, Kk, Kpk); // BSE info
  q = exp( -PI * rn * Kpk / Kk );	/* the nome of k1 */
  m1 = cay(q); /* see below */
  /* Note m1 = eps / sqrt( A*A - 1.0 ) */
  a = eps/m1;
  a =  a * a + 1;
  a = 10.0 * log(a) / log(10.0);
  ellf_debugf ( "dbdown %.9E\n", a );
  a = 180.0 * asin( k ) / PI;
  b = 1.0/(1.0 + eps*eps);
  b = sqrt( 1.0 - b );
  ellf_debugf ( "theta %.9E, rho %.9E\n", a, b );
  m1 *= m1;
  m1p = 1.0 - m1;
  Kk1 = ellpk( m1p );
  Kpk1 = ellpk( m1 );
  r = Kpk1 * Kk / (Kk1 * Kpk);
  ellf_debugf ("consistency check: r=%.20g Kpk1=%.20g Kk1=%.20g m1=%.20g m1p=%.20g\n", r, Kpk1, Kk1, m1, m1p); // BSE info
  /*   -1
   * sn   j/eps\m  =  j ellik( atan(1/eps), m )
   */
  b = 1.0/eps;
  phi = atan( b );
  u = ellik( phi, m1p );
  ellf_debugf ("phi=%.20g m=%.20g u=%.20g\n", phi, m1p, u);
  /* consistency check on inverse sn */
  ellpj( u, m1p, &sn, &cn, &dn, &phi );
  a = sn/cn;
  ellf_debugf ("consistency check: sn/cn = %.20g = %.20g = 1/ripple\n", a, b);
  u = u * Kk / (rn * Kk1);	/* or, u = u * Kpk / Kpk1 */
  return 0;
}
/* calculate s plane poles and zeros, normalized to wc = 1 */
int spln()
{
  for( i=0; i<ARRSIZ; i++ )
    zs[i] = 0.0;
  np = (n+1)/2;
  nz = 0;
  if( kind == 1 )
    {
      /* Butterworth poles equally spaced around the unit circle
       */
      if( n & 1 )
        m = 0.0;
      else
        m = PI / (2.0*n);
      for( i=0; i<np; i++ )
        {	/* poles */
          lr = i + i;
          zs[lr] = -cos(m);
          zs[lr+1] = sin(m);
          m += PI / n;
        }	
      /* high pass or band reject
       */
      if( type >= 3 )
        {
          /* map s => 1/s
           */
          for( j=0; j<np; j++ )
            {
              ir = j + j;
              ii = ir + 1;
              b = zs[ir]*zs[ir] + zs[ii]*zs[ii];
              zs[ir] = zs[ir] / b;
              zs[ii] = zs[ii] / b;
            }
          /* The zeros at infinity map to the origin.
           */
          nz = np;
          if( type == 4 )
            {
              nz += n/2;
            }
          for( j=0; j<nz; j++ )
            {
              ir = ii + 1;
              ii = ir + 1;
              zs[ir] = 0.0;
              zs[ii] = 0.0;
            }
        }
    }
  if( kind == 2 )
    {
      /* For Chebyshev, find radii of two Butterworth circles
       * See Gold & Rader, page 60
       */
      rho = (phi - 1.0)*(phi+1);  /* rho = eps^2 = {sqrt(1+eps^2)}^2 - 1 */
      eps = sqrt(rho);
      /* sqrt( 1 + 1/eps^2 ) + 1/eps  = {sqrt(1 + eps^2)  +  1} / eps
       */
      phi = (phi + 1.0) / eps;
      ellf_debugf ("Chebychev: phi-before=%.20g ripple=%.20g\n", phi, eps); // BSE info
      phi = pow( phi, 1.0/rn );  /* raise to the 1/n power */
      ellf_debugf ("Chebychev: phi-raised=%.20g rn=%.20g\n", phi, rn); // BSE info
      b = 0.5 * (phi + 1.0/phi); /* y coordinates are on this circle */
      a = 0.5 * (phi - 1.0/phi); /* x coordinates are on this circle */
      if( n & 1 )
        m = 0.0;
      else
        m = PI / (2.0*n);
      for( i=0; i<np; i++ )
        {	/* poles */
          lr = i + i;
          zs[lr] = -a * cos(m);
          zs[lr+1] = b * sin(m);
          m += PI / n;
        }	
      /* high pass or band reject
       */
      if( type >= 3 )
        {
          /* map s => 1/s
           */
          for( j=0; j<np; j++ )
            {
              ir = j + j;
              ii = ir + 1;
              b = zs[ir]*zs[ir] + zs[ii]*zs[ii];
              zs[ir] = zs[ir] / b;
              zs[ii] = zs[ii] / b;
            }
          /* The zeros at infinity map to the origin.
           */
          nz = np;
          if( type == 4 )
            {
              nz += n/2;
            }
          for( j=0; j<nz; j++ )
            {
              ir = ii + 1;
              ii = ir + 1;
              zs[ir] = 0.0;
              zs[ii] = 0.0;
            }
        }
    }
  if( kind == 3 )   /* elliptic filter -- stw */
    {
      nz = n/2;
      ellpj( u, 1.0-m, &sn1, &cn1, &dn1, &phi1 );
      for( i=0; i<ARRSIZ; i++ )
        zs[i] = 0.0;
      for( i=0; i<nz; i++ )
        {	/* zeros */
          a = n - 1 - i - i;
          b = (Kk * a) / rn;
          ellpj( b, m, &sn, &cn, &dn, &phi );
          lr = 2*np + 2*i;
          zs[ lr ] = 0.0;
          a = wc/(k*sn);	/* k = sqrt(m) */
          zs[ lr + 1 ] = a;
        }
      for( i=0; i<np; i++ )
        {	/* poles */
          a = n - 1 - i - i;
          b = a * Kk / rn;		
          ellpj( b, m, &sn, &cn, &dn, &phi );
          r = k * sn * sn1;
          b = cn1*cn1 + r*r;
          a = -wc*cn*dn*sn1*cn1/b;
          lr = i + i;
          zs[lr] = a;
          b = wc*sn*dn1/b;
          zs[lr+1] = b;
        }	
      if( type >= 3 )
        {
          nt = np + nz;
          for( j=0; j<nt; j++ )
            {
              ir = j + j;
              ii = ir + 1;
              b = zs[ir]*zs[ir] + zs[ii]*zs[ii];
              zs[ir] = zs[ir] / b;
              zs[ii] = zs[ii] / b;
            }
          while( np > nz )
            {
              ir = ii + 1;
              ii = ir + 1;
              nz += 1;
              zs[ir] = 0.0;
              zs[ii] = 0.0;
            }
        }
    }
  ellf_printf ( "s plane poles:\n" );
  j = 0;
  for( i=0; i<np+nz; i++ )
    {
      a = zs[j];
      ++j;
      b = zs[j];
      ++j;
      ellf_printf ( "%.9E %.9E\n", a, b );
      if( i == np-1 )
        ellf_printf ( "s plane zeros:\n" );
    }
  return 0;
}
/*		cay()
 *
 * Find parameter corresponding to given nome by expansion
 * in theta functions:
 * AMS55 #16.38.5, 16.38.7
 *
 *       1/2
 * ( 2K )                   4     9
 * ( -- )     =  1 + 2q + 2q  + 2q  + ...  =  Theta (0,q)
 * ( pi )                                          3
 *
 *
 *       1/2
 * ( 2K )     1/4       1/4        2    6    12    20
 * ( -- )    m     =  2q    ( 1 + q  + q  + q   + q   + ...) = Theta (0,q)
 * ( pi )                                                           2
 *
 * The nome q(m) = exp( - pi K(1-m)/K(m) ).
 *
 *                                1/2
 * Given q, this program returns m   .
 */
double cay(q)
     double q;
{
  double a, b, p, r;
  double t1, t2;
  a = 1.0;
  b = 1.0;
  r = 1.0;
  p = q;
  do
    {
      r *= p;
      a += 2.0 * r;
      t1 = fabs( r/a );
      r *= p;
      b += r;
      p *= q;
      t2 = fabs( r/b );
      if( t2 > t1 )
	t1 = t2;
    }
  while( t1 > MACHEP );
  a = b/a;
  a = 4.0 * sqrt(q) * a * a;	/* see above formulas, solved for m */
  return(a);
}
/*		zpln.c
 * Program to convert s plane poles and zeros to the z plane.
 */
extern cmplx cone;
int zplna()
{
  cmplx r, cnum, cden, cwc, ca, cb, b4ac;
  double C;
  if( kind == 3 )
    C = c;
  else
    C = wc;
  for( i=0; i<ARRSIZ; i++ )
    {
      z[i].r = 0.0;
      z[i].i = 0.0;
    }
  nc = np;
  jt = -1;
  ii = -1;
  for( icnt=0; icnt<2; icnt++ )
    {
      /* The maps from s plane to z plane */
      do
	{
          ir = ii + 1;
          ii = ir + 1;
          r.r = zs[ir];
          r.i = zs[ii];
          switch( type )
            {
            case 1:
            case 3:
              /* Substitute  s - r  =  s/wc - r = (1/wc)(z-1)/(z+1) - r
               *
               *     1  1 - r wc (       1 + r wc )
               * =  --- -------- ( z  -  -------- )
               *    z+1    wc    (       1 - r wc )
               *
               * giving the root in the z plane.
               */
              cnum.r = 1 + C * r.r;
              cnum.i = C * r.i;
              cden.r = 1 - C * r.r;
              cden.i = -C * r.i;
              jt += 1;
              Cdiv( &cden, &cnum, &z[jt] );
              if( r.i != 0.0 )
                {
                  /* fill in complex conjugate root */
                  jt += 1;
                  z[jt].r = z[jt-1 ].r;
                  z[jt].i = -z[jt-1 ].i;
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
              if( kind == 2 )
                cwc.r = cbp;
              else
                cwc.r = c;
              cwc.i = 0.0;
              Cmul( &r, &cwc, &cnum );     /* r wc */
              Csub( &cnum, &cone, &ca );   /* a = 1 - r wc */
              Cmul( &cnum, &cnum, &b4ac ); /* 1 - (r wc)^2 */
              Csub( &b4ac, &cone, &b4ac );
              b4ac.r *= 4.0;               /* 4ac */
              b4ac.i *= 4.0;
              cb.r = -2.0 * cgam;          /* b */
              cb.i = 0.0;
              Cmul( &cb, &cb, &cnum );     /* b^2 */
              Csub( &b4ac, &cnum, &b4ac ); /* b^2 - 4 ac */
              Csqrt( &b4ac, &b4ac );
              cb.r = -cb.r;  /* -b */
              cb.i = -cb.i;
              ca.r *= 2.0; /* 2a */
              ca.i *= 2.0;
              Cadd( &b4ac, &cb, &cnum );   /* -b + sqrt( b^2 - 4ac) */
              Cdiv( &ca, &cnum, &cnum );   /* ... /2a */
              jt += 1;
              Cmov( &cnum, &z[jt] );
              if( cnum.i != 0.0 )
                {
                  jt += 1;
                  z[jt].r = cnum.r;
                  z[jt].i = -cnum.i;
                }
              if( (r.i != 0.0) || (cnum.i == 0) )
                {
                  Csub( &b4ac, &cb, &cnum );  /* -b - sqrt( b^2 - 4ac) */
                  Cdiv( &ca, &cnum, &cnum );  /* ... /2a */
                  jt += 1;
                  Cmov( &cnum, &z[jt] );
                  if( cnum.i != 0.0 )
                    {
                      jt += 1;
                      z[jt].r = cnum.r;
                      z[jt].i = -cnum.i;
                    }
                }
            } /* end switch */
	}
      while( --nc > 0 );
      if( icnt == 0 )
	{
          zord = jt+1;
          if( nz <= 0 )
            {
              if( kind != 3 )
                return(0);
              else
                break;
            }
	}
      nc = nz;
    } /* end for() loop */
  return 0;
}
int zplnb()
{
  cmplx lin[2];
  lin[1].r = 1.0;
  lin[1].i = 0.0;
  if( kind != 3 )
    { /* Butterworth or Chebyshev */
      /* generate the remaining zeros */
      while( 2*zord - 1 > jt )
        {
          if( type != 3 )
            {
              ellf_debugf ( "adding zero at Nyquist frequency\n" );
              jt += 1;
              z[jt].r = -1.0; /* zero at Nyquist frequency */
              z[jt].i = 0.0;
            }
          if( (type == 2) || (type == 3) )
            {
              ellf_debugf ( "adding zero at 0 Hz\n" );
              jt += 1;
              z[jt].r = 1.0; /* zero at 0 Hz */
              z[jt].i = 0.0;
            }
        }
    }
  else
    { /* elliptic */
      while( 2*zord - 1 > jt )
        {
          jt += 1;
          z[jt].r = -1.0; /* zero at Nyquist frequency */
          z[jt].i = 0.0;
          if( (type == 2) || (type == 4) )
            {
              jt += 1;
              z[jt].r = 1.0; /* zero at 0 Hz */
              z[jt].i = 0.0;
            }
        }
    }
  ellf_debugf ( "order = %d\n", zord );
  /* Expand the poles and zeros into numerator and
   * denominator polynomials
   */
  for( icnt=0; icnt<2; icnt++ )
    {
      for( j=0; j<ARRSIZ; j++ )
        {
          pp[j] = 0.0;
          y[j] = 0.0;
        }
      pp[0] = 1.0;
      for( j=0; j<zord; j++ )
        {
          jj = j;
          if( icnt )
            jj += zord;
          a = z[jj].r;
          b = z[jj].i;
          for( i=0; i<=j; i++ )
            {
              jh = j - i;
              pp[jh+1] = pp[jh+1] - a * pp[jh] + b * y[jh];
              y[jh+1] =  y[jh+1]  - b * pp[jh] - a * y[jh];
            }
        }
      if( icnt == 0 )
        {
          for( j=0; j<=zord; j++ )
            aa[j] = pp[j];
        }
    }
  /* Scale factors of the pole and zero polynomials */
  a = 1.0;
  switch( type )
    {
    case 3:
      a = -1.0;
    case 1:
    case 4:
      pn = 1.0;
      an = 1.0;
      for( j=1; j<=zord; j++ )
        {
          pn = a * pn + pp[j];
          an = a * an + aa[j];
        }
      break;
    case 2:
      gam = PI/2.0 - asin( cgam );  /* = acos( cgam ) */
      mh = zord/2;
      pn = pp[mh];
      an = aa[mh];
      ai = 0.0;
      if( mh > ((zord/4)*2) )
        {
          ai = 1.0;
          pn = 0.0;
          an = 0.0;
        }
      for( j=1; j<=mh; j++ )
        {
          a = gam * j - ai * PI / 2.0;
          cng = cos(a);
          jh = mh + j;
          jl = mh - j;
          pn = pn + cng * (pp[jh] + (1.0 - 2.0 * ai) * pp[jl]);
          an = an + cng * (aa[jh] + (1.0 - 2.0 * ai) * aa[jl]);
        }
    }
  return 0;
}
int zplnc()
{
  gain = an/(pn*scale);
  if( (kind != 3) && (pn == 0) )
    gain = 1.0;
  ellf_printf( "constant gain factor %23.13E\n", gain );
  for( j=0; j<=zord; j++ )
    pp[j] = gain * pp[j];
  ellf_printf( "z plane Denominator      Numerator\n" );
  for( j=0; j<=zord; j++ )
    {
      ellf_printf( "%2d %17.9E %17.9E\n", j, aa[j], pp[j] );
    }
  /* I /think/ at this point the polynomial is factorized in 2nd order filters,
   * so that it can be implemented without stability problems -- stw
   */
  ellf_printf( "poles and zeros with corresponding quadratic factors\n" );
  for( j=0; j<zord; j++ )
    {
      a = z[j].r;
      b = z[j].i;
      if( b >= 0.0 )
        {
          ellf_printf( "pole  %23.13E %23.13E\n", a, b );
          quadf( a, b, 1 );
        }
      jj = j + zord;
      a = z[jj].r;
      b = z[jj].i;
      if( b >= 0.0 )
        {
          ellf_printf( "zero  %23.13E %23.13E\n", a, b );
          quadf( a, b, 0 );
        }
    }
  return 0;
}
/* display quadratic factors
 */
int quadf( x, y, pzflg )
     double x, y;
     int pzflg;	/* 1 if poles, 0 if zeros */
{
  double a, b, r, f, g, g0;
  if( y > 1.0e-16 )
    {
      a = -2.0 * x;
      b = x*x + y*y;
    }
  else
    {
      a = -x;
      b = 0.0;
    }
  ellf_printf( "q. f.\nz**2 %23.13E\nz**1 %23.13E\n", b, a );
  if( b != 0.0 )
    {
      /* resonant frequency */
      r = sqrt(b);
      f = PI/2.0 - asin( -a/(2.0*r) );
      f = f * fs / (2.0 * PI );
      /* gain at resonance */
      g = 1.0 + r;
      g = g*g - (a*a/r);
      g = (1.0 - r) * sqrt(g);
      g0 = 1.0 + a + b;	/* gain at d.c. */
    }
  else
    {
      /* It is really a first-order network.
       * Give the gain at fnyq and D.C.
       */
      f = fnyq;
      g = 1.0 - a;
      g0 = 1.0 + a;
    }
  if( pzflg )
    {
      if( g != 0.0 )
        g = 1.0/g;
      else
        g = MAXNUM;
      if( g0 != 0.0 )
        g0 = 1.0/g0;
      else
        g = MAXNUM;
    }
  ellf_printf( "f0 %16.8E  gain %12.4E  DC gain %12.4E\n\n", f, g, g0 );
  return 0;
}
/* Print table of filter frequency response
 */
void
print_filter_table (void)
{
  double f, limit = 0.05 * fnyq * 21;
  for (f=0; f < limit; f += limit / 21.)
    {
      double r = response( f, gain );
      if( r <= 0.0 )
        r = -999.99;
      else
        r = 2.0 * dbfac * log( r );
      ellf_debugf ( "%10.1f  %10.2f\n", f, r );
      // f = f + 0.05 * fnyq;
    }
}
/* Calculate frequency response at f Hz
 * mulitplied by amp
 */
double response( f, amp )
     double f, amp;
{
  cmplx x, num, den, w;
  double u;
  int j;
  /* exp( j omega T ) */
  u = 2.0 * PI * f /fs;
  x.r = cos(u);
  x.i = sin(u);
  num.r = 1.0;
  num.i = 0.0;
  den.r = 1.0;
  den.i = 0.0;
  for( j=0; j<zord; j++ )
    {
      Csub( &z[j], &x, &w );
      Cmul( &w, &den, &den );
      Csub( &z[j+zord], &x, &w );
      Cmul( &w, &num, &num );
    }
  Cdiv( &den, &num, &w );
  w.r *= amp;
  w.i *= amp;
  u = Cabs( &w );
  return(u);
}
/* === ellf.c - end === */
/* compile with: gcc -Wall -O2 -g -ffloat-store bse-ellf.c -lm -o bse-ellf */
