/* GSL - Generic Sound Layer
 * Copyright (C) 1999, 2001 Tim Janik
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
#ifndef __GSL_IEEE754_H__
#define __GSL_IEEE754_H__

#include	<gsl/gsldefs.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* IEEE 754 single precision floating point layout:
 *        31 30           23 22            0
 * +--------+---------------+---------------+
 * | s 1bit | e[30:23] 8bit | f[22:0] 23bit |
 * +--------+---------------+---------------+
 * B0------------------->B1------->B2-->B3-->
 *
 * IEEE 754 double precision floating point layout:
 *        63 62            52 51            32   31            0
 * +--------+----------------+----------------+ +---------------+
 * | s 1bit | e[62:52] 11bit | f[51:32] 20bit | | f[31:0] 32bit |
 * +--------+----------------+----------------+ +---------------+
 * B0--------------->B1---------->B2--->B3---->  B4->B5->B6->B7->
 */

/* floating point type related constants */
#define GSL_FLOAT_BIAS		 (127)
#define	GSL_FLOAT_MAX_NORMAL	 (3.40282347e+38)	   /* 7f7fffff */
#define	GSL_FLOAT_MIN_NORMAL	 (1.17549435e-38)	   /* 00800000 */
#define	GSL_FLOAT_MAX_SUBNORMAL	 (1.17549421e-38)	   /* 007fffff */
#define	GSL_FLOAT_MIN_SUBNORMAL	 (1.40129846e-45)	   /* 00000001 */
#define GSL_DOUBLE_BIAS		 (1023)
#define	GSL_DOUBLE_MAX_NORMAL	 (1.7976931348623157e+308) /* 7fefffff ffffffff */
#define	GSL_DOUBLE_MIN_NORMAL	 (2.2250738585072014e-308) /* 00100000 00000000 */
#define	GSL_DOUBLE_MAX_SUBNORMAL (2.2250738585072009e-308) /* 000fffff ffffffff */
#define	GSL_DOUBLE_MIN_SUBNORMAL (4.9406564584124654e-324) /* 00000000 00000001 */
#define	GSL_DOUBLE_INF		 (_gsl_dinf_union.d)
#define	GSL_DOUBLE_NAN		 (_gsl_dnan_union.d)
#define	GSL_FLOAT_INF		 (_gsl_finf_union.f)
#define	GSL_FLOAT_NAN		 (_gsl_fnan_union.f)

/* multiply with base2 exponent to get base10 exponent (for nomal numbers) */
#define GSL_LOG_2_BASE_10         (0.30102999566398119521)

/* the following macros work only on variables
 * and evaluate arguments multiple times
 */

/* single precision value checks */
#define	GSL_FLOAT_IS_ZERO(f)		((f) == 0.0)	/* compiler knows this one */
#define	GSL_FLOAT_IS_NORMAL(f)		(GSL_FLOAT_PARTS (f).mpn.biased_exponent > 0 && \
				         GSL_FLOAT_PARTS (f).mpn.biased_exponent < 255)
#define	GSL_FLOAT_IS_SUBNORMAL(f)	(GSL_FLOAT_PARTS (f).mpn.biased_exponent == 0 && \
					 GSL_FLOAT_PARTS (f).mpn.mantissa != 0)
#define	GSL_FLOAT_IS_NANINF(f)		(GSL_FLOAT_PARTS (f).mpn.biased_exponent == 255)
#define	GSL_FLOAT_IS_NAN(f)		(GSL_FLOAT_IS_NANINF (f) && GSL_FLOAT_PARTS (f).mpn.mantissa != 0)
#define	GSL_FLOAT_IS_INF(f)		(GSL_FLOAT_IS_NANINF (f) && GSL_FLOAT_PARTS (f).mpn.mantissa == 0)
#define	GSL_FLOAT_IS_INF_POSITIVE(f)	(GSL_FLOAT_IS_INF (f) && GSL_FLOAT_PARTS (f).mpn.sign == 0)
#define	GSL_FLOAT_IS_INF_NEGATIVE(f)	(GSL_FLOAT_IS_INF (f) && GSL_FLOAT_PARTS (f).mpn.sign == 1)
#define	GSL_FLOAT_SIGN(f)		(GSL_FLOAT_PARTS (f).mpn.sign)

/* double precision value checks */
#define	GSL_DOUBLE_IS_ZERO(d)		((d) == 0.0)	/* compiler knows this one */
#define	GSL_DOUBLE_IS_NORMAL(d)		(GSL_DOUBLE_PARTS (d).mpn.biased_exponent > 0 && \
					 GSL_DOUBLE_PARTS (d).mpn.biased_exponent < 2047)
#define	GSL_DOUBLE_IS_SUBNORMAL(d)	(GSL_DOUBLE_PARTS (d).mpn.biased_exponent == 0 && \
                                         (GSL_DOUBLE_PARTS (d).mpn.mantissa_low != 0 || \
					  GSL_DOUBLE_PARTS (d).mpn.mantissa_high != 0))
#define	GSL_DOUBLE_IS_NANINF(d)		(GSL_DOUBLE_PARTS (d).mpn.biased_exponent == 2047)
#define	GSL_DOUBLE_IS_NAN(d)		(GSL_DOUBLE_IS_NANINF (d) && \
					 (GSL_DOUBLE_PARTS (d).mpn.mantissa_low != 0 || \
					  GSL_DOUBLE_PARTS (d).mpn.mantissa_high != 0))
#define	GSL_DOUBLE_IS_INF(d)		(GSL_DOUBLE_IS_NANINF (d) && \
					 GSL_DOUBLE_PARTS (d).mpn.mantissa_low == 0 && \
					 GSL_DOUBLE_PARTS (d).mpn.mantissa_high == 0)
#define	GSL_DOUBLE_IS_INF_POSITIVE(d)	(GSL_DOUBLE_IS_INF (d) && GSL_DOUBLE_PARTS (d).mpn.sign == 0)
#define	GSL_DOUBLE_IS_INF_NEGATIVE(d)	(GSL_DOUBLE_IS_INF (d) && GSL_DOUBLE_PARTS (d).mpn.sign == 1)
#define	GSL_DOUBLE_SIGN(d)		(GSL_DOUBLE_PARTS (d).mpn.sign)

/* get structured parts of floating point numbers */
#define	GSL_FLOAT_PARTS(f)		(*((GslFloatIEEE754*) &(f)))
#define	GSL_DOUBLE_PARTS(d)		(*((GslDoubleIEEE754*) &(d)))

/* --- rounding --- */
typedef	unsigned short int	GslFpuState;
#if defined (__i386__) && defined (__GNUC__)
/* setting/restoring rounding mode shouldn't actually
 * be necessary as round-to-nearest is the hardware
 * default (can be checked with gsl_fpu_okround()).
 */
static inline void	gsl_fpu_setround	(GslFpuState		*cw);
static inline int	gsl_fpu_okround		(void);
static inline void	gsl_fpu_restore		(GslFpuState		 cv);
static inline int	gsl_ftoi		(register double	 f)  G_GNUC_CONST;
/* fallbacks for the !386 case are below */
#endif


/* --- implementation bits --- */
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
typedef union
{
  float		v_float;
  struct {
    unsigned int mantissa : 23;
    unsigned int biased_exponent : 8;
    unsigned int sign : 1;
  } mpn;
} GslFloatIEEE754;
typedef union
{
  double	v_double;
  struct {
    unsigned int mantissa_low : 32;
    unsigned int mantissa_high : 20;
    unsigned int biased_exponent : 11;
    unsigned int sign : 1;
  } mpn;
} GslDoubleIEEE754;
#define	_GSL_DOUBLE_INF_BYTES	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x7f }
#define	_GSL_DOUBLE_NAN_BYTES	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x7f }
#define	_GSL_FLOAT_INF_BYTES	{ 0x00, 0x00, 0x80, 0x7f }
#define	_GSL_FLOAT_NAN_BYTES	{ 0x00, 0x00, 0xc0, 0x7f }
#elif G_BYTE_ORDER == G_BIG_ENDIAN
typedef union
{
  float         v_float;
  struct {
    unsigned int sign : 1;
    unsigned int biased_exponent : 8;
    unsigned int mantissa : 23;
  } mpn;
} GslFloatIEEE754;
typedef union
{
  double        v_double;
  struct {
    unsigned int sign : 1;
    unsigned int biased_exponent : 11;
    unsigned int mantissa_high : 20;
    unsigned int mantissa_low : 32;
  } mpn;
} GslDoubleIEEE754;
#define	_GSL_DOUBLE_INF_BYTES	{ 0x7f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define	_GSL_DOUBLE_NAN_BYTES	{ 0x7f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define	_GSL_FLOAT_INF_BYTES	{ 0x7f, 0x80, 0x00, 0x00 }
#define	_GSL_FLOAT_NAN_BYTES	{ 0x7f, 0xc0, 0x00, 0x00 }
#else /* !G_LITTLE_ENDIAN && !G_BIG_ENDIAN */
#error unknown ENDIAN type
#endif /* !G_LITTLE_ENDIAN && !G_BIG_ENDIAN */

static const union { unsigned char c[8]; double d; } _gsl_dnan_union = { _GSL_DOUBLE_NAN_BYTES };
static const union { unsigned char c[8]; double d; } _gsl_dinf_union = { _GSL_DOUBLE_INF_BYTES };
static const union { unsigned char c[4]; float f; }  _gsl_fnan_union = { _GSL_FLOAT_NAN_BYTES };
static const union { unsigned char c[4]; float f; }  _gsl_finf_union = { _GSL_FLOAT_INF_BYTES };

#if defined (__i386__) && defined (__GNUC__)
static inline void
gsl_fpu_setround (GslFpuState *cw)
{
  GslFpuState cv;

  asm ("fnstcw %0"
       : "=m" (*&cv));
  *cw = cv;
  cv &= ~0x0c00;
  asm ("fldcw %0"
       :
       : "m" (*&cv));
}
static inline int
gsl_fpu_okround (void)
{
  GslFpuState cv;

  asm ("fnstcw %0"
       : "=m" (*&cv));
  return !(cv & 0x0c00);
}
static inline void
gsl_fpu_restore (GslFpuState cv)
{
  asm ("fldcw %0"
       :
       : "m" (*&cv));
}
static inline int G_GNUC_CONST
gsl_ftoi (register double f)
{
  int r;

  asm ("fistl %0"
       : "=m" (r)
       : "t" (f));
  return r;
}
#else   /* !386 */
#  define gsl_fpu_setround(p)   ((void) (p));
#  define gsl_fpu_okround()     (1)
#  define gsl_fpu_restore(x)    /* nop */
static inline int G_GNUC_CONST
gsl_ftoi (register double v)
{
  return v < -0.0 ? v - 0.5 : v + 0.5;
}
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_IEEE754_H__ */		/* vim: set ts=8 sw=2 sts=2: */
