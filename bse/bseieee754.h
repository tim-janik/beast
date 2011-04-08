/* BSE - Better Sound Engine
 * Copyright (C) 1999-2004 Tim Janik
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
#ifndef __BSE_IEEE754_H__
#define __BSE_IEEE754_H__

#include <bse/bsedefs.h>
#include <math.h> /* signbit */

/* override math.h definition of PI */
#undef PI
#define PI                            (3.141592653589793238462643383279502884197)    // pi

G_BEGIN_DECLS

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
#define BSE_FLOAT_BIAS		 (127)
#define	BSE_FLOAT_MAX_NORMAL	 (3.40282347e+38)	   /* 7f7fffff, 2^128 * (1 - BSE_FLOAT_EPSILON) */
#define	BSE_FLOAT_MIN_NORMAL	 (1.17549435e-38)	   /* 00800000 */
#define	BSE_FLOAT_MAX_SUBNORMAL	 (1.17549421e-38)	   /* 007fffff */
#define	BSE_FLOAT_MIN_SUBNORMAL	 (1.40129846e-45)	   /* 00000001 */
#define	BSE_FLOAT_EPSILON	 (5.9604644775390625e-08)  /* 2^-24, round-off error at 1.0 */
#define BSE_DOUBLE_BIAS		 (1023)
#define	BSE_DOUBLE_MAX_NORMAL	 (1.7976931348623157e+308) /* 7fefffff ffffffff, 2^1024 * (1 - BSE_DOUBLE_EPSILON) */
#define	BSE_DOUBLE_MIN_NORMAL	 (2.2250738585072014e-308) /* 00100000 00000000 */
#define	BSE_DOUBLE_MAX_SUBNORMAL (2.2250738585072009e-308) /* 000fffff ffffffff */
#define	BSE_DOUBLE_MIN_SUBNORMAL (4.9406564584124654e-324) /* 00000000 00000001 */
#define	BSE_DOUBLE_EPSILON	 (1.1102230246251565404236316680908203125e-16) /* 2^-53, round-off error at 1.0 */
#define	BSE_DOUBLE_INF		 (_bse_dinf_union.d)
#define	BSE_DOUBLE_NAN		 (_bse_dnan_union.d)
#define	BSE_FLOAT_INF		 (_bse_finf_union.f)
#define	BSE_FLOAT_NAN		 (_bse_fnan_union.f)

/* multiply with base2 exponent to get base10 exponent (for nomal numbers) */
#define BSE_LOG_2_BASE_10         (0.30102999566398119521)

/* the following macros work only on variables
 * and evaluate arguments multiple times
 */

/* single precision value checks */
#define	BSE_FLOAT_IS_ZERO(f)		((f) == 0.0)	/* compiler knows this one */
#define	BSE_FLOAT_IS_NORMAL(f)		(BSE_FLOAT_PARTS (f).mpn.biased_exponent > 0 && \
				         BSE_FLOAT_PARTS (f).mpn.biased_exponent < 255)
#define	BSE_FLOAT_IS_SUBNORMAL(f)	(BSE_FLOAT_PARTS (f).mpn.biased_exponent == 0 && \
					 BSE_FLOAT_PARTS (f).mpn.mantissa != 0)
#define	BSE_FLOAT_IS_NANINF(f)		(BSE_FLOAT_PARTS (f).mpn.biased_exponent == 255)
#define	BSE_FLOAT_IS_NAN(f)		(BSE_FLOAT_IS_NANINF (f) && BSE_FLOAT_PARTS (f).mpn.mantissa != 0)
#define	BSE_FLOAT_IS_INF(f)		(BSE_FLOAT_IS_NANINF (f) && BSE_FLOAT_PARTS (f).mpn.mantissa == 0)
#define	BSE_FLOAT_IS_INF_POSITIVE(f)	(BSE_FLOAT_IS_INF (f) && BSE_FLOAT_PARTS (f).mpn.sign == 0)
#define	BSE_FLOAT_IS_INF_NEGATIVE(f)	(BSE_FLOAT_IS_INF (f) && BSE_FLOAT_PARTS (f).mpn.sign == 1)
#ifdef signbit
#define BSE_FLOAT_SIGN(f)               (signbit (f))
#else
#define BSE_FLOAT_SIGN(f)               (BSE_FLOAT_PARTS (f).mpn.sign)
#endif

/* double precision value checks */
#define	BSE_DOUBLE_IS_ZERO(d)		((d) == 0.0)	/* compiler knows this one */
#define	BSE_DOUBLE_IS_NORMAL(d)		(BSE_DOUBLE_PARTS (d).mpn.biased_exponent > 0 && \
					 BSE_DOUBLE_PARTS (d).mpn.biased_exponent < 2047)
#define	BSE_DOUBLE_IS_SUBNORMAL(d)	(BSE_DOUBLE_PARTS (d).mpn.biased_exponent == 0 && \
                                         (BSE_DOUBLE_PARTS (d).mpn.mantissa_low != 0 || \
					  BSE_DOUBLE_PARTS (d).mpn.mantissa_high != 0))
#define	BSE_DOUBLE_IS_NANINF(d)		(BSE_DOUBLE_PARTS (d).mpn.biased_exponent == 2047)
#define	BSE_DOUBLE_IS_NAN(d)		(BSE_DOUBLE_IS_NANINF (d) && \
					 (BSE_DOUBLE_PARTS (d).mpn.mantissa_low != 0 || \
					  BSE_DOUBLE_PARTS (d).mpn.mantissa_high != 0))
#define	BSE_DOUBLE_IS_INF(d)		(BSE_DOUBLE_IS_NANINF (d) && \
					 BSE_DOUBLE_PARTS (d).mpn.mantissa_low == 0 && \
					 BSE_DOUBLE_PARTS (d).mpn.mantissa_high == 0)
#define	BSE_DOUBLE_IS_INF_POSITIVE(d)	(BSE_DOUBLE_IS_INF (d) && BSE_DOUBLE_PARTS (d).mpn.sign == 0)
#define	BSE_DOUBLE_IS_INF_NEGATIVE(d)	(BSE_DOUBLE_IS_INF (d) && BSE_DOUBLE_PARTS (d).mpn.sign == 1)
#ifdef signbit
#define BSE_DOUBLE_SIGN(d)              (signbit (d))
#else
#define BSE_DOUBLE_SIGN(d)              (BSE_DOUBLE_PARTS (d).mpn.sign)
#endif

/* --- denormal float handling --- */
static inline float	bse_float_zap_denormal	(register float  fval);	/* slow */
static inline double	bse_double_zap_denormal	(register double dval);	/* slow */

/* --- coarse but fast variants to eliminate denormalized floats --- */
/* pure arithmetic flushing, fastest with -ffast-math */
#define	BSE_FLOAT_FLUSH(mutable_float)		BSE_FLOAT_FLUSH_with_threshold (mutable_float)
#define	BSE_DOUBLE_FLUSH(mutable_double)	BSE_DOUBLE_FLUSH_with_threshold (mutable_double)
#if 0	/* may be slow in non-inlined functions */
#define	BSE_FLOAT_FLUSH(mutable_float)		BSE_FLOAT_FLUSH_with_cond (mutable_float)
#define	BSE_DOUBLE_FLUSH(mutable_double)	BSE_DOUBLE_FLUSH_with_cond (mutable_double)
#endif
#if 0	/* branching may hurt performance in excessively inlined code */
#define	BSE_FLOAT_FLUSH(mutable_float)		BSE_FLOAT_FLUSH_with_if (mutable_float)
#define	BSE_DOUBLE_FLUSH(mutable_double)	BSE_DOUBLE_FLUSH_with_if (mutable_double)
#endif

/* --- rounding --- */
typedef	unsigned short int	BseFpuState;
#if defined (__i386__) && defined (__GNUC__)
/* setting/restoring rounding mode shouldn't actually
 * be necessary as round-to-nearest is the hardware
 * default (can be checked with bse_fpu_okround()).
 */
static inline void	bse_fpu_setround	(BseFpuState		*cw);
static inline int	bse_fpu_okround		(void);
static inline void	bse_fpu_restore		(BseFpuState		 cv);
static inline int	bse_ftoi /* nearest */	(register float		 f)  G_GNUC_CONST;
static inline int	bse_dtoi /* nearest */	(register double	 f)  G_GNUC_CONST;
/* fallbacks for the !386 case are below */
#endif
static inline guint64   bse_dtoull              (const double            v);
static inline gint64    bse_dtoll               (const double            v);

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
} BseFloatIEEE754;
typedef union
{
  double	v_double;
  struct {
    unsigned int mantissa_low : 32;
    unsigned int mantissa_high : 20;
    unsigned int biased_exponent : 11;
    unsigned int sign : 1;
  } mpn;
} BseDoubleIEEE754;
#define	_BSE_DOUBLE_INF_BYTES	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x7f }
#define	_BSE_DOUBLE_NAN_BYTES	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x7f }
#define	_BSE_FLOAT_INF_BYTES	{ 0x00, 0x00, 0x80, 0x7f }
#define	_BSE_FLOAT_NAN_BYTES	{ 0x00, 0x00, 0xc0, 0x7f }
#elif G_BYTE_ORDER == G_BIG_ENDIAN
typedef union
{
  float         v_float;
  struct {
    unsigned int sign : 1;
    unsigned int biased_exponent : 8;
    unsigned int mantissa : 23;
  } mpn;
} BseFloatIEEE754;
typedef union
{
  double        v_double;
  struct {
    unsigned int sign : 1;
    unsigned int biased_exponent : 11;
    unsigned int mantissa_high : 20;
    unsigned int mantissa_low : 32;
  } mpn;
} BseDoubleIEEE754;
#define	_BSE_DOUBLE_INF_BYTES	{ 0x7f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define	_BSE_DOUBLE_NAN_BYTES	{ 0x7f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define	_BSE_FLOAT_INF_BYTES	{ 0x7f, 0x80, 0x00, 0x00 }
#define	_BSE_FLOAT_NAN_BYTES	{ 0x7f, 0xc0, 0x00, 0x00 }
#else /* !G_LITTLE_ENDIAN && !G_BIG_ENDIAN */
#error unknown ENDIAN type
#endif /* !G_LITTLE_ENDIAN && !G_BIG_ENDIAN */

static const union { unsigned char c[8]; double d; } _bse_dnan_union = { _BSE_DOUBLE_NAN_BYTES };
static const union { unsigned char c[8]; double d; } _bse_dinf_union = { _BSE_DOUBLE_INF_BYTES };
static const union { unsigned char c[4]; float f; }  _bse_fnan_union = { _BSE_FLOAT_NAN_BYTES };
static const union { unsigned char c[4]; float f; }  _bse_finf_union = { _BSE_FLOAT_INF_BYTES };

/* get structured parts of floating point numbers */
#if __cplusplus
extern inline BseFloatIEEE754  BSE_FLOAT_PARTS  (register float  fvalue) { BseFloatIEEE754  fret = { fvalue }; return fret; }
extern inline BseDoubleIEEE754 BSE_DOUBLE_PARTS (register double dvalue) { BseDoubleIEEE754 dret = { dvalue }; return dret; }
#else
#define	BSE_FLOAT_PARTS(f)		(((BseFloatIEEE754) (f)))
#define	BSE_DOUBLE_PARTS(d)		(((BseDoubleIEEE754) (d)))
#endif

/* --- implementation details --- */
static inline float
bse_float_zap_denormal (register float  fval)
{
  if (G_UNLIKELY (BSE_FLOAT_IS_SUBNORMAL (fval)))
    return 0;
  else
    return fval;
}

static inline double
bse_double_zap_denormal	(register double dval)
{
  if (G_UNLIKELY (BSE_DOUBLE_IS_SUBNORMAL (dval)))
    return 0;
  else
    return dval;
}

/* use float arithmetic cancellation to eliminate denormals */
#define	BSE_FLOAT_FLUSH_with_threshold(mutable_float)	do { 	\
  volatile float __forced_float = 1e-29 + mutable_float;	\
  mutable_float = __forced_float - 1e-29;			\
} while (0)
#define	BSE_DOUBLE_FLUSH_with_threshold(mutable_double) do {	\
  volatile double __forced_double = 1e-288 + mutable_double;	\
  mutable_double = __forced_double - 1e-288;			\
} while (0)
/* substitute with 0 beyond a certain threashold greater than possible denormals */
#define BSE_FLOAT_FLUSH_with_cond(mutable_float) do {     			 \
  mutable_float = G_UNLIKELY (fabs (mutable_float) < 1e-32) ? 0 : mutable_float; \
} while (0)
#define BSE_DOUBLE_FLUSH_with_cond(mutable_double) do {      			     \
  mutable_double = G_UNLIKELY (fabs (mutable_double) < 1e-290) ? 0 : mutable_double; \
} while (0)
/* set everything to 0 beyond a certain threashold greater than possible denormals */
#define	BSE_FLOAT_FLUSH_with_if(mutable_float) do { 	\
  if (G_UNLIKELY (fabs (mutable_float) < 1e-32))	\
    mutable_float = 0;					\
} while (0)
#define BSE_DOUBLE_FLUSH_with_if(mutable_double) do {	\
  if (G_UNLIKELY (fabs (mutable_double) < 1e-290))	\
    mutable_double = 0;					\
} while (0)

#if defined (__i386__) && defined (__GNUC__)
static inline void
bse_fpu_setround (BseFpuState *cw)
{
  BseFpuState cv;
  
  __asm__ ("fnstcw %0"
	   : "=m" (*&cv));
  *cw = cv;
  cv &= ~0x0c00;
  __asm__ ("fldcw %0"
	   :
	   : "m" (*&cv));
}
static inline int
bse_fpu_okround (void)
{
  BseFpuState cv;
  
  __asm__ ("fnstcw %0"
	   : "=m" (*&cv));
  return !(cv & 0x0c00);
}
static inline void
bse_fpu_restore (BseFpuState cv)
{
  __asm__ ("fldcw %0"
	   :
	   : "m" (*&cv));
}
static inline int G_GNUC_CONST
bse_ftoi (register float f)
{
  int r;
  
  __asm__ ("fistl %0"
	   : "=m" (r)
	   : "t" (f));
  return r;
}
static inline int G_GNUC_CONST
bse_dtoi (register double f)
{
  int r;
  
  __asm__ ("fistl %0"
	   : "=m" (r)
	   : "t" (f));
  return r;
}
#else   /* !386 */
#  define bse_fpu_setround(p)   ((void) (p));
#  define bse_fpu_okround()     (1)
#  define bse_fpu_restore(x)    /* nop */
static inline int G_GNUC_CONST
bse_ftoi (register float v)
{
  return (int) (v < -0.0 ? v - 0.5 : v + 0.5);
}
static inline int G_GNUC_CONST
bse_dtoi (register double v)
{
  return (int) (v < -0.0 ? v - 0.5 : v + 0.5);
}
#endif
static inline guint64
bse_dtoull (const double v)
{
  return v < -0.0 ? (guint64) (v - 0.5) : (guint64) (v + 0.5);
}
static inline gint64
bse_dtoll (const double v)
{
  return v < -0.0 ? (gint64) (v - 0.5) : (gint64) (v + 0.5);
}

G_END_DECLS

#endif /* __BSE_IEEE754_H__ */		/* vim: set ts=8 sw=2 sts=2: */
