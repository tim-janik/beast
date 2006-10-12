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

/* Type of computer arithmetic */

/* UNKnown arithmetic, invokes coefficients given in
 * normal decimal format.  Beware of range boundary
 * problems (MACHEP, MAXLOG, etc. in const.c) and
 * roundoff problems in pow.c:
 * (Sun SPARCstation)
 */
#define UNK 1

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

#ifdef UNK
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
#endif

#ifdef IBMPC
			/* 2**-53 =  1.11022302462515654042E-16 */
unsigned short MACHEP[4] = {0x0000,0x0000,0x0000,0x3ca0};
unsigned short UFLOWTHRESH[4] = {0x0000,0x0000,0x0000,0x0010};
#ifdef DENORMAL
			/* log(MAXNUM) =  7.09782712893383996732224E2 */
unsigned short MAXLOG[4] = {0x39ef,0xfefa,0x2e42,0x4086};
			/* log(2**-1074) = - -7.44440071921381262314E2 */
/*unsigned short MINLOG[4] = {0x71c3,0x446d,0x4385,0xc087};*/
unsigned short MINLOG[4] = {0x3052,0xd52d,0x4910,0xc087};
#else
			/* log(2**1022) =   7.08396418532264106224E2 */
unsigned short MAXLOG[4] = {0xbcd2,0xdd7a,0x232b,0x4086};
			/* log(2**-1022) = - 7.08396418532264106224E2 */
unsigned short MINLOG[4] = {0xbcd2,0xdd7a,0x232b,0xc086};
#endif
			/* 2**1024*(1-MACHEP) =  1.7976931348623158E308 */
unsigned short MAXNUM[4] = {0xffff,0xffff,0xffff,0x7fef};
unsigned short PI[4]     = {0x2d18,0x5444,0x21fb,0x4009};
unsigned short PIO2[4]   = {0x2d18,0x5444,0x21fb,0x3ff9};
unsigned short PIO4[4]   = {0x2d18,0x5444,0x21fb,0x3fe9};
unsigned short SQRT2[4]  = {0x3bcd,0x667f,0xa09e,0x3ff6};
unsigned short SQRTH[4]  = {0x3bcd,0x667f,0xa09e,0x3fe6};
unsigned short LOG2E[4]  = {0x82fe,0x652b,0x1547,0x3ff7};
unsigned short SQ2OPI[4] = {0x3651,0x33d4,0x8845,0x3fe9};
unsigned short LOGE2[4]  = {0x39ef,0xfefa,0x2e42,0x3fe6};
unsigned short LOGSQ2[4] = {0x39ef,0xfefa,0x2e42,0x3fd6};
unsigned short THPIO4[4] = {0x21d2,0x7f33,0xd97c,0x4002};
unsigned short TWOOPI[4] = {0xc883,0x6dc9,0x5f30,0x3fe4};
#ifdef INFINITIES
unsigned short INFINITY[4] = {0x0000,0x0000,0x0000,0x7ff0};
#else
unsigned short INFINITY[4] = {0xffff,0xffff,0xffff,0x7fef};
#endif
#ifdef NANS
unsigned short NAN[4] = {0x0000,0x0000,0x0000,0x7ffc};
#else
unsigned short NAN[4] = {0x0000,0x0000,0x0000,0x0000};
#endif
#ifdef MINUSZERO
unsigned short NEGZERO[4] = {0x0000,0x0000,0x0000,0x8000};
#else
unsigned short NEGZERO[4] = {0x0000,0x0000,0x0000,0x0000};
#endif
#endif

#ifdef MIEEE
			/* 2**-53 =  1.11022302462515654042E-16 */
unsigned short MACHEP[4] = {0x3ca0,0x0000,0x0000,0x0000};
unsigned short UFLOWTHRESH[4] = {0x0010,0x0000,0x0000,0x0000};
#ifdef DENORMAL
			/* log(2**1024) =   7.09782712893383996843E2 */
unsigned short MAXLOG[4] = {0x4086,0x2e42,0xfefa,0x39ef};
			/* log(2**-1074) = - -7.44440071921381262314E2 */
/* unsigned short MINLOG[4] = {0xc087,0x4385,0x446d,0x71c3}; */
unsigned short MINLOG[4] = {0xc087,0x4910,0xd52d,0x3052};
#else
			/* log(2**1022) =  7.08396418532264106224E2 */
unsigned short MAXLOG[4] = {0x4086,0x232b,0xdd7a,0xbcd2};
			/* log(2**-1022) = - 7.08396418532264106224E2 */
unsigned short MINLOG[4] = {0xc086,0x232b,0xdd7a,0xbcd2};
#endif
			/* 2**1024*(1-MACHEP) =  1.7976931348623158E308 */
unsigned short MAXNUM[4] = {0x7fef,0xffff,0xffff,0xffff};
unsigned short PI[4]     = {0x4009,0x21fb,0x5444,0x2d18};
unsigned short PIO2[4]   = {0x3ff9,0x21fb,0x5444,0x2d18};
unsigned short PIO4[4]   = {0x3fe9,0x21fb,0x5444,0x2d18};
unsigned short SQRT2[4]  = {0x3ff6,0xa09e,0x667f,0x3bcd};
unsigned short SQRTH[4]  = {0x3fe6,0xa09e,0x667f,0x3bcd};
unsigned short LOG2E[4]  = {0x3ff7,0x1547,0x652b,0x82fe};
unsigned short SQ2OPI[4] = {0x3fe9,0x8845,0x33d4,0x3651};
unsigned short LOGE2[4]  = {0x3fe6,0x2e42,0xfefa,0x39ef};
unsigned short LOGSQ2[4] = {0x3fd6,0x2e42,0xfefa,0x39ef};
unsigned short THPIO4[4] = {0x4002,0xd97c,0x7f33,0x21d2};
unsigned short TWOOPI[4] = {0x3fe4,0x5f30,0x6dc9,0xc883};
#ifdef INFINITIES
unsigned short INFINITY[4] = {0x7ff0,0x0000,0x0000,0x0000};
#else
unsigned short INFINITY[4] = {0x7fef,0xffff,0xffff,0xffff};
#endif
#ifdef NANS
unsigned short NAN[4] = {0x7ff8,0x0000,0x0000,0x0000};
#else
unsigned short NAN[4] = {0x0000,0x0000,0x0000,0x0000};
#endif
#ifdef MINUSZERO
unsigned short NEGZERO[4] = {0x8000,0x0000,0x0000,0x0000};
#else
unsigned short NEGZERO[4] = {0x0000,0x0000,0x0000,0x0000};
#endif
#endif

#ifdef DEC
			/* 2**-56 =  1.38777878078144567553E-17 */
unsigned short MACHEP[4] = {0022200,0000000,0000000,0000000};
unsigned short UFLOWTHRESH[4] = {0x0080,0x0000,0x0000,0x0000};
			/* log 2**127 = 88.029691931113054295988 */
unsigned short MAXLOG[4] = {041660,007463,0143742,025733,};
			/* log 2**-128 = -88.72283911167299960540 */
unsigned short MINLOG[4] = {0141661,071027,0173721,0147572,};
			/* 2**127 = 1.701411834604692317316873e38 */
unsigned short MAXNUM[4] = {077777,0177777,0177777,0177777,};
unsigned short PI[4]     = {040511,007732,0121041,064302,};
unsigned short PIO2[4]   = {040311,007732,0121041,064302,};
unsigned short PIO4[4]   = {040111,007732,0121041,064302,};
unsigned short SQRT2[4]  = {040265,002363,031771,0157145,};
unsigned short SQRTH[4]  = {040065,002363,031771,0157144,};
unsigned short LOG2E[4]  = {040270,0125073,024534,013761,};
unsigned short SQ2OPI[4] = {040114,041051,0117241,0131204,};
unsigned short LOGE2[4]  = {040061,071027,0173721,0147572,};
unsigned short LOGSQ2[4] = {037661,071027,0173721,0147572,};
unsigned short THPIO4[4] = {040426,0145743,0174631,007222,};
unsigned short TWOOPI[4] = {040042,0174603,067116,042025,};
/* Approximate infinity by MAXNUM.  */
unsigned short INFINITY[4] = {077777,0177777,0177777,0177777,};
unsigned short NAN[4] = {0000000,0000000,0000000,0000000};
#ifdef MINUSZERO
unsigned short NEGZERO[4] = {0000000,0000000,0000000,0100000};
#else
unsigned short NEGZERO[4] = {0000000,0000000,0000000,0000000};
#endif
#endif

#ifndef UNK
extern unsigned short MACHEP[];
extern unsigned short UFLOWTHRESH[];
extern unsigned short MAXLOG[];
extern unsigned short UNDLOG[];
extern unsigned short MINLOG[];
extern unsigned short MAXNUM[];
extern unsigned short PI[];
extern unsigned short PIO2[];
extern unsigned short PIO4[];
extern unsigned short SQRT2[];
extern unsigned short SQRTH[];
extern unsigned short LOG2E[];
extern unsigned short SQ2OPI[];
extern unsigned short LOGE2[];
extern unsigned short LOGSQ2[];
extern unsigned short THPIO4[];
extern unsigned short TWOOPI[];
extern unsigned short INFINITY[];
extern unsigned short NAN[];
extern unsigned short NEGZERO[];
#endif

/* === const.c - end === */
/* === protos.h - start === */
/*
 *   This file was automatically generated by version 1.7 of cextract.
 *   Manual editing not recommended.
 *
 *   Created: Sun Jan  9 15:07:08 2000
 */

extern double cabs ( cmplx *z );
extern void cadd ( cmplx *a, cmplx *b, cmplx *c );
extern double cay ( double q );
extern void cdiv ( cmplx *a, cmplx *b, cmplx *c );
extern void cmov ( void *a, void *b );
extern void cmul ( cmplx *a, cmplx *b, cmplx *c );
extern void cneg ( cmplx *a );
extern void csqrt ( cmplx *z, cmplx *w );
extern void csub ( cmplx *a, cmplx *b, cmplx *c );
extern double ellie ( double phi, double m );
extern double ellik ( double phi, double m );
extern double ellpe ( double x );
extern int ellpj ( double u, double m, double *sn, double *cn, double *dn, double *ph );
extern double ellpk ( double x );
extern int getnum ( char *line, double *val );
extern int lampln ( void );
extern int main ( void );
extern int mtherr ( char *name, int code );
extern double p1evl ( double x, double coef[], int N );
extern double polevl ( double x, double coef[], int N );
extern int quadf ( double x, double y, int pzflg );
extern double response ( double f, double amp );
extern int spln ( void );
extern int xfun ( void );
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
 * cadd( a, b, c );     c = b + a
 * csub( a, b, c );     c = b - a
 * cmul( a, b, c );     c = b * a
 * cdiv( a, b, c );     c = b / a
 * cneg( c );           c = -c
 * cmov( b, c );        c = b
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
 *    DEC        cadd       10000       1.4e-17     3.4e-18
 *    IEEE       cadd      100000       1.1e-16     2.7e-17
 *    DEC        csub       10000       1.4e-17     4.5e-18
 *    IEEE       csub      100000       1.1e-16     3.4e-17
 *    DEC        cmul        3000       2.3e-17     8.7e-18
 *    IEEE       cmul      100000       2.1e-16     6.9e-17
 *    DEC        cdiv       18000       4.9e-17     1.3e-17
 *    IEEE       cdiv      100000       3.7e-16     1.1e-16
 */
/*				cmplx.c
 * complex number arithmetic
 */


extern double fabs ( double );
extern double cabs ( cmplx * );
extern double sqrt ( double );
extern double atan2 ( double, double );
extern double cos ( double );
extern double sin ( double );
extern double sqrt ( double );
extern double frexp ( double, int * );
extern double ldexp ( double, int );
int isnan ( double );
void cdiv ( cmplx *, cmplx *, cmplx * );
void cadd ( cmplx *, cmplx *, cmplx * );

extern double MAXNUM, MACHEP, PI, PIO2, INFINITY, NAN;

/*
typedef struct
	{
	double r;
	double i;
	}cmplx;
*/
cmplx czero = {0.0, 0.0};
extern cmplx czero;
cmplx cone = {1.0, 0.0};
extern cmplx cone;

/*	c = b + a	*/

void cadd( a, b, c )
register cmplx *a, *b;
cmplx *c;
{

c->r = b->r + a->r;
c->i = b->i + a->i;
}


/*	c = b - a	*/

void csub( a, b, c )
register cmplx *a, *b;
cmplx *c;
{

c->r = b->r - a->r;
c->i = b->i - a->i;
}

/*	c = b * a */

void cmul( a, b, c )
register cmplx *a, *b;
cmplx *c;
{
double y;

y    = b->r * a->r  -  b->i * a->i;
c->i = b->r * a->i  +  b->i * a->r;
c->r = y;
}



/*	c = b / a */

void cdiv( a, b, c )
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
		mtherr( "cdiv", OVERFLOW );
		return;
		}
	}
c->r = p/y;
c->i = q/y;
}


/*	b = a
   Caution, a `short' is assumed to be 16 bits wide.  */

void cmov( a, b )
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


void cneg( a )
register cmplx *a;
{

a->r = -a->r;
a->i = -a->i;
}

/*							cabs()
 *
 *	Complex absolute value
 *
 *
 *
 * SYNOPSIS:
 *
 * double cabs();
 * cmplx z;
 * double a;
 *
 * a = cabs( &z );
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

/*
typedef struct
	{
	double r;
	double i;
	}cmplx;
*/

#ifdef UNK
#define PREC 27
#define MAXEXP 1024
#define MINEXP -1077
#endif
#ifdef DEC
#define PREC 29
#define MAXEXP 128
#define MINEXP -128
#endif
#ifdef IBMPC
#define PREC 27
#define MAXEXP 1024
#define MINEXP -1077
#endif
#ifdef MIEEE
#define PREC 27
#define MAXEXP 1024
#define MINEXP -1077
#endif


double cabs( z )
register cmplx *z;
{
double x, y, b, re, im;
int ex, ey, e;

#ifdef INFINITIES
/* Note, cabs(INFINITY,NAN) = INFINITY. */
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
	mtherr( "cabs", OVERFLOW );
	return( INFINITY );
	}
if( ey < MINEXP )
	return(0.0);

/* Undo the scaling */
b = ldexp( b, e );
return( b );
}
/*							csqrt()
 *
 *	Complex square root
 *
 *
 *
 * SYNOPSIS:
 *
 * void csqrt();
 * cmplx z, w;
 *
 * csqrt( &z, &w );
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
 * Also tested by csqrt( z ) = z, and tested by arguments
 * close to the real axis.
 */


void csqrt( z, w )
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
	r = cabs(z);
	t = 0.5*(r - x);
	}

r = sqrt(t);
q.i = r;
q.r = y/(2.0*r);
/* Heron iteration in complex arithmetic */
cdiv( &q, z, &s );
cadd( &q, &s, w );
w->r *= 0.5;
w->i *= 0.5;
}


double hypot( x, y )
double x, y;
{
cmplx z;

z.r = x;
z.i = y;
return( cabs(&z) );
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

extern double sqrt ( double );
extern double fabs ( double );
extern double log ( double );
extern double tan ( double );
extern double atan ( double );
extern double floor ( double );
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

#ifdef UNK
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
#endif

#ifdef DEC
static unsigned short P[] = {
0035041,0001364,0141572,0117555,
0036044,0066032,0130027,0033404,
0036416,0053617,0064456,0102632,
0036457,0161100,0061177,0122612,
0036376,0136251,0012403,0124162,
0036370,0101316,0151715,0131613,
0036475,0105477,0050317,0133272,
0036662,0154232,0024645,0171552,
0037150,0126220,0047054,0030064,
0037742,0162057,0167645,0165612,
0040200,0000000,0000000,0000000
};
static unsigned short Q[] = {
0034411,0106743,0115771,0055462,
0035604,0052575,0155171,0045540,
0036325,0030424,0064332,0167756,
0036612,0052366,0063006,0115175,
0036726,0070430,0004533,0124654,
0037011,0022741,0030675,0030711,
0037056,0174452,0127062,0132122,
0037157,0177750,0142041,0072523,
0037277,0177777,0173137,0002627,
0037577,0177777,0177777,0101101
};
#endif

#ifdef IBMPC
static unsigned short P[] = {
0x53ee,0x986f,0x205e,0x3f24,
0xe6e0,0x5602,0x8d83,0x3f64,
0xd0b3,0xed25,0xcaf1,0x3f81,
0xf4b1,0x0c4f,0xfc48,0x3f85,
0x750e,0x22a0,0xd795,0x3f7f,
0xb671,0xda79,0x1059,0x3f7f,
0xf6d7,0xea19,0xb167,0x3f87,
0xbe6d,0x4534,0x5b13,0x3f96,
0x8607,0x09c5,0x1592,0x3fad,
0xbd71,0xfdf4,0x5c85,0x3fdc,
0x0000,0x0000,0x0000,0x3ff0
};
static unsigned short Q[] = {
0x2b66,0x737f,0x31bc,0x3f01,
0x296c,0xbb4f,0x8aaf,0x3f50,
0x5dfe,0x8d1b,0xa622,0x3f7a,
0xd350,0xccc0,0x4a9e,0x3f91,
0x7535,0x012b,0xce23,0x3f9a,
0xa639,0x2637,0x24bc,0x3fa1,
0x568a,0x55c6,0xdf25,0x3fa5,
0x2eaa,0x1884,0xfffd,0x3fad,
0xe0b3,0xfecb,0xffff,0x3fb7,
0xf048,0xffff,0xffff,0x3fcf
};
#endif

#ifdef MIEEE
static unsigned short P[] = {
0x3f24,0x205e,0x986f,0x53ee,
0x3f64,0x8d83,0x5602,0xe6e0,
0x3f81,0xcaf1,0xed25,0xd0b3,
0x3f85,0xfc48,0x0c4f,0xf4b1,
0x3f7f,0xd795,0x22a0,0x750e,
0x3f7f,0x1059,0xda79,0xb671,
0x3f87,0xb167,0xea19,0xf6d7,
0x3f96,0x5b13,0x4534,0xbe6d,
0x3fad,0x1592,0x09c5,0x8607,
0x3fdc,0x5c85,0xfdf4,0xbd71,
0x3ff0,0x0000,0x0000,0x0000
};
static unsigned short Q[] = {
0x3f01,0x31bc,0x737f,0x2b66,
0x3f50,0x8aaf,0xbb4f,0x296c,
0x3f7a,0xa622,0x8d1b,0x5dfe,
0x3f91,0x4a9e,0xccc0,0xd350,
0x3f9a,0xce23,0x012b,0x7535,
0x3fa1,0x24bc,0x2637,0xa639,
0x3fa5,0xdf25,0x55c6,0x568a,
0x3fad,0xfffd,0x1884,0x2eaa,
0x3fb7,0xffff,0xfecb,0xe0b3,
0x3fcf,0xffff,0xffff,0xf048
};
#endif

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
double sqrt(), fabs(), sin(), cos(), asin(), tanh();
double sinh(), cosh(), atan(), exp();
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

#ifdef DEC
static unsigned short P[] =
{
0035020,0127576,0040430,0051544,
0036025,0070136,0042703,0153716,
0036402,0122614,0062555,0077777,
0036441,0102130,0072334,0025172,
0036341,0043320,0117242,0172076,
0036312,0146456,0077242,0154141,
0036420,0003467,0013727,0035407,
0036564,0137263,0110651,0020237,
0036775,0001330,0144056,0020305,
0037305,0144137,0157521,0141734,
0040261,0071027,0173721,0147572
};
static unsigned short Q[] =
{
0034366,0130371,0103453,0077633,
0035557,0122745,0173515,0113016,
0036302,0124470,0167304,0074473,
0036575,0132403,0117226,0117576,
0036703,0156271,0047124,0147733,
0036766,0137465,0002053,0157312,
0037031,0014423,0154274,0176515,
0037107,0177747,0143216,0016145,
0037217,0177777,0172621,0074000,
0037377,0177777,0177776,0156435,
0040000,0000000,0000000,0000000
};
static unsigned short ac1[] = {0040261,0071027,0173721,0147572};
#define C1 (*(double *)ac1)
#endif

#ifdef IBMPC
static unsigned short P[] =
{
0x0a6d,0xc823,0x15ef,0x3f22,
0x7afa,0xc8b8,0xae0b,0x3f62,
0xb000,0x8cad,0x54b1,0x3f80,
0x854f,0x0e9b,0x308b,0x3f84,
0x5e88,0x13d4,0x28da,0x3f7c,
0x5b0c,0xcfd4,0x59a5,0x3f79,
0xe761,0xe2fa,0x00e6,0x3f82,
0x2414,0x7235,0x97d6,0x3f8e,
0xc419,0x1905,0xa05b,0x3f9f,
0x387c,0xfbea,0xb90b,0x3fb8,
0x39ef,0xfefa,0x2e42,0x3ff6
};
static unsigned short Q[] =
{
0x6ff3,0x30e5,0xd61f,0x3efe,
0xb2c2,0xbee9,0xf4bc,0x3f4d,
0x8f27,0x1dd8,0x5527,0x3f78,
0xd3f0,0x73d2,0xb6a0,0x3f8f,
0x99fb,0x29ca,0x7b97,0x3f98,
0x7bd9,0xa085,0xd7e6,0x3f9e,
0x9faa,0x7b17,0x2322,0x3fa3,
0xc38d,0xf8d1,0xfffc,0x3fa8,
0x2f00,0xfeb2,0xffff,0x3fb1,
0xdba4,0xffff,0xffff,0x3fbf,
0x0000,0x0000,0x0000,0x3fe0
};
static unsigned short ac1[] = {0x39ef,0xfefa,0x2e42,0x3ff6};
#define C1 (*(double *)ac1)
#endif

#ifdef MIEEE
static unsigned short P[] =
{
0x3f22,0x15ef,0xc823,0x0a6d,
0x3f62,0xae0b,0xc8b8,0x7afa,
0x3f80,0x54b1,0x8cad,0xb000,
0x3f84,0x308b,0x0e9b,0x854f,
0x3f7c,0x28da,0x13d4,0x5e88,
0x3f79,0x59a5,0xcfd4,0x5b0c,
0x3f82,0x00e6,0xe2fa,0xe761,
0x3f8e,0x97d6,0x7235,0x2414,
0x3f9f,0xa05b,0x1905,0xc419,
0x3fb8,0xb90b,0xfbea,0x387c,
0x3ff6,0x2e42,0xfefa,0x39ef
};
static unsigned short Q[] =
{
0x3efe,0xd61f,0x30e5,0x6ff3,
0x3f4d,0xf4bc,0xbee9,0xb2c2,
0x3f78,0x5527,0x1dd8,0x8f27,
0x3f8f,0xb6a0,0x73d2,0xd3f0,
0x3f98,0x7b97,0x29ca,0x99fb,
0x3f9e,0xd7e6,0xa085,0x7bd9,
0x3fa3,0x2322,0x7b17,0x9faa,
0x3fa8,0xfffc,0xf8d1,0xc38d,
0x3fb1,0xffff,0xfeb2,0x2f00,
0x3fbf,0xffff,0xffff,0xdba4,
0x3fe0,0x0000,0x0000,0x0000
};
static unsigned short ac1[] = {
0x3ff6,0x2e42,0xfefa,0x39ef
};
#define C1 (*(double *)ac1)
#endif

#ifdef UNK
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
#endif

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
printf( "\n%s ", name );

/* Set global error message word */
merror = code;

/* Display error message defined
 * by the code argument.
 */
if( (code <= 0) || (code >= 7) )
	code = 0;
printf( "%s error\n", ermsg[code] );

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
#define ARRSIZ 50


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
double fs = 1.0e4;
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
double fnyq = 0.0;
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
extern double cabs ( cmplx *z );
extern void cadd ( cmplx *a, cmplx *b, cmplx *c );
extern void cdiv ( cmplx *a, cmplx *b, cmplx *c );
extern void cmov ( void *a, void *b );
extern void cmul ( cmplx *a, cmplx *b, cmplx *c );
extern void cneg ( cmplx *a );
extern void csqrt ( cmplx *z, cmplx *w );
extern void csub ( cmplx *a, cmplx *b, cmplx *c );
extern double ellie ( double phi, double m );
extern double ellik ( double phi, double m );
extern double ellpe ( double x );
extern int ellpj ( double, double, double *, double *, double *, double * );
extern double ellpk ( double x );
int getnum ( char *line, double *val );
double cay ( double q );
int lampln ( void );
int spln ( void );
int xfun ( void );
int zplna ( void );
int zplnb ( void );
int zplnc ( void );
int quadf ( double, double, int );
double response ( double, double );

int main()
{
char str[80];

dbfac = 10.0/log(10.0);

top:

printf( "%s ? ", wkind );	/* ask for filter kind */
gets( str );
sscanf( str, "%d", &kind );
printf( "%d\n", kind );
if( (kind <= 0) || (kind > 3) )
	exit(0);

printf( "%s ? ", salut );	/* ask for filter type */
gets( str );
sscanf( str, "%d", &type );
printf( "%d\n", type );
if( (type <= 0) || (type > 4) )
	exit(0);

getnum( "Order of filter", &rn ); /* see below for getnum() */
n = rn;
if( n <= 0 )
	{
specerr:
	printf( "? Specification error\n" );
	goto top;
	}
rn = n;	/* ensure it is an integer */
if( kind > 1 ) /* not Butterworth */
	{
	getnum( "Passband ripple, db", &dbr );
	if( dbr <= 0.0 )
		goto specerr;
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

getnum( "Sampling frequency", &fs );
if( fs <= 0.0 )
	goto specerr;

fnyq = 0.5 * fs;

getnum( "Passband edge", &f2 );
if( (f2 <= 0.0) || (f2 >= fnyq) )
	goto specerr;

if( (type & 1) == 0 )
	{
	getnum( "Other passband edge", &f1 );
	if( (f1 <= 0.0) || (f1 >= fnyq) )
		goto specerr;
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
/*printf( "cos( 1/2 (Whigh-Wlow) T ) = %.5e, wc = %.5e\n", cang, wc );*/
	}


if( kind == 3 )
	{ /* elliptic */
	cgam = cos( (a+f1) * PI / fs ) / cang;
	getnum( "Stop band edge or -(db down)", &dbd );
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
			goto specerr;
		break;

	case 2:
		if( (f3 > f2) || (f3 < f1) )
			break;
		goto specerr;

	case 3:
		if( f3 >= f2 )
			goto specerr;
		break;

	case 4:
		if( (f3 <= f1) || (f3 >= f2) )
			goto specerr;
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
	printf( "pass band %.9E\n", aa[1] );
	printf( "stop band %.9E\n", aa[2] );
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
	printf( "pass band %.9E %.9E\n", pp[1], aa[1] );
	printf( "stop band %.9E %.9E\n", pp[2], aa[2] );
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

spln();		/* find s plane poles and zeros */

if( ((type & 1) == 0) && ((4*n+2) > ARRSIZ) )
	goto toosml;

zplna();	/* convert s plane to z plane */
zplnb();
zplnc();
xfun(); /* tabulate transfer function */
goto top;

toosml:
printf( "Cannot continue, storage arrays too small\n" );
goto top;
}


int lampln()
{

wc = 1.0;
k = wc/wr;
m = k * k;
Kk = ellpk( 1.0 - m );
Kpk = ellpk( m );
q = exp( -PI * rn * Kpk / Kk );	/* the nome of k1 */
m1 = cay(q); /* see below */
/* Note m1 = eps / sqrt( A*A - 1.0 ) */
a = eps/m1;
a =  a * a + 1;
a = 10.0 * log(a) / log(10.0);
printf( "dbdown %.9E\n", a );
a = 180.0 * asin( k ) / PI;
b = 1.0/(1.0 + eps*eps);
b = sqrt( 1.0 - b );
printf( "theta %.9E, rho %.9E\n", a, b );
m1 *= m1;
m1p = 1.0 - m1;
Kk1 = ellpk( m1p );
Kpk1 = ellpk( m1 );
r = Kpk1 * Kk / (Kk1 * Kpk);
printf( "consistency check: n= %.14E\n", r );
/*   -1
 * sn   j/eps\m  =  j ellik( atan(1/eps), m )
 */
b = 1.0/eps;
phi = atan( b );
u = ellik( phi, m1p );
printf( "phi %.7e m %.7e u %.7e\n", phi, m1p, u );
/* consistency check on inverse sn */
ellpj( u, m1p, &sn, &cn, &dn, &phi );
a = sn/cn;
printf( "consistency check: sn/cn = %.9E = %.9E = 1/eps\n", a, b );
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
	phi = pow( phi, 1.0/rn );  /* raise to the 1/n power */
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
if( kind == 3 )
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
printf( "s plane poles:\n" );
j = 0;
for( i=0; i<np+nz; i++ )
	{
	a = zs[j];
	++j;
	b = zs[j];
	++j;
	printf( "%.9E %.9E\n", a, b );
	if( i == np-1 )
		printf( "s plane zeros:\n" );
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
		cdiv( &cden, &cnum, &z[jt] );
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
		cmul( &r, &cwc, &cnum );     /* r wc */
		csub( &cnum, &cone, &ca );   /* a = 1 - r wc */
		cmul( &cnum, &cnum, &b4ac ); /* 1 - (r wc)^2 */
		csub( &b4ac, &cone, &b4ac );
		b4ac.r *= 4.0;               /* 4ac */
		b4ac.i *= 4.0;
		cb.r = -2.0 * cgam;          /* b */
		cb.i = 0.0;
		cmul( &cb, &cb, &cnum );     /* b^2 */
		csub( &b4ac, &cnum, &b4ac ); /* b^2 - 4 ac */
		csqrt( &b4ac, &b4ac );
		cb.r = -cb.r;  /* -b */
		cb.i = -cb.i;
		ca.r *= 2.0; /* 2a */
		ca.i *= 2.0;
		cadd( &b4ac, &cb, &cnum );   /* -b + sqrt( b^2 - 4ac) */
		cdiv( &ca, &cnum, &cnum );   /* ... /2a */
		jt += 1;
		cmov( &cnum, &z[jt] );
		if( cnum.i != 0.0 )
			{
			jt += 1;
			z[jt].r = cnum.r;
			z[jt].i = -cnum.i;
			}
		if( (r.i != 0.0) || (cnum.i == 0) )
			{
			csub( &b4ac, &cb, &cnum );  /* -b - sqrt( b^2 - 4ac) */
			cdiv( &ca, &cnum, &cnum );  /* ... /2a */
			jt += 1;
			cmov( &cnum, &z[jt] );
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
	printf( "adding zero at Nyquist frequency\n" );
			jt += 1;
			z[jt].r = -1.0; /* zero at Nyquist frequency */
			z[jt].i = 0.0;
			}
		if( (type == 2) || (type == 3) )
			{
	printf( "adding zero at 0 Hz\n" );
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
printf( "order = %d\n", zord );

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
printf( "constant gain factor %23.13E\n", gain );
for( j=0; j<=zord; j++ )
	pp[j] = gain * pp[j];

printf( "z plane Denominator      Numerator\n" );
for( j=0; j<=zord; j++ )
	{
	printf( "%2d %17.9E %17.9E\n", j, aa[j], pp[j] );
	}
printf( "poles and zeros with corresponding quadratic factors\n" );
for( j=0; j<zord; j++ )
	{
	a = z[j].r;
	b = z[j].i;
	if( b >= 0.0 )
		{
		printf( "pole  %23.13E %23.13E\n", a, b );
		quadf( a, b, 1 );
		}
	jj = j + zord;
	a = z[jj].r;
	b = z[jj].i;
	if( b >= 0.0 )
		{
		printf( "zero  %23.13E %23.13E\n", a, b );
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
printf( "q. f.\nz**2 %23.13E\nz**1 %23.13E\n", b, a );
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
printf( "f0 %16.8E  gain %12.4E  DC gain %12.4E\n\n", f, g, g0 );
return 0;
}



/* Print table of filter frequency response
 */
int xfun()
{
double f, r;
int i;

f = 0.0;

for( i=0; i<=20; i++ )
	{
	r = response( f, gain );
	if( r <= 0.0 )
		r = -999.99;
	else
		r = 2.0 * dbfac * log( r );
	printf( "%10.1f  %10.2f\n", f, r );
	f = f + 0.05 * fnyq;
	}
return 0;
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
	csub( &z[j], &x, &w );
	cmul( &w, &den, &den );
	csub( &z[j+zord], &x, &w );
	cmul( &w, &num, &num );
	}
cdiv( &den, &num, &w );
w.r *= amp;
w.i *= amp;
u = cabs( &w );
return(u);
}




/* Get a number from keyboard.
 * Display previous value and keep it if user just hits <CR>.
 */
int getnum( line, val )
char *line;
double *val;
{
char s[40];

printf( "%s = %.9E ? ", line, *val );
gets( s );
if( s[0] != '\0' )
	{
	sscanf( s, "%lf", val );
	printf( "%.9E\n", *val );
	}
return 0;
}

/* === ellf.c - end === */

/* compile with: gcc -Wall -O2 -g bseiirfilter.c -lm -o ellf */
