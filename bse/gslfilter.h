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
#ifndef __GSL_FILTER_H__
#define __GSL_FILTER_H__

#include <bse/gslmath.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- transformations --- */
static inline GslComplex gsl_trans_s2z          (GslComplex     s);
static inline double     gsl_trans_freq2s       (double         w);
static inline double     gsl_trans_zepsilon2ss  (double         epsilon);


/* --- filter roots and poles --- */
void	gsl_filter_butter_rp    (unsigned int iorder,
				 double       freq,   /* 0..pi */
				 double       epsilon,
				 GslComplex  *roots,  /* [0..iorder-1] */
				 GslComplex  *poles);
void	gsl_filter_tscheb1_rp	(unsigned int iorder,
				 double       freq,   /* 0..pi */
				 double       epsilon,
				 GslComplex  *roots,  /* [0..iorder-1] */
				 GslComplex  *poles);
void	gsl_filter_tscheb2_rp	(unsigned int iorder,
				 double       c_freq, /* 0..pi */
				 double       steepness,
				 double       epsilon,
				 GslComplex  *roots,  /* [0..iorder-1] */
				 GslComplex  *poles);


/* --- tschebyscheff type II steepness --- */
double	gsl_filter_tscheb2_steepness_db	(unsigned int iorder,
					 double       c_freq,
					 double       epsilon,
					 double       stopband_db);
double	gsl_filter_tscheb2_steepness	(unsigned int iorder,
					 double       c_freq,
					 double       epsilon,
					 double       residue);


/* --- lowpass filters --- */
void	gsl_filter_butter_lp    (unsigned int iorder,
				 double       freq, /* 0..pi */
				 double       epsilon,
				 double      *a,    /* [0..iorder] */
				 double      *b);
void	gsl_filter_tscheb1_lp	(unsigned int iorder,
				 double       freq, /* 0..pi */
				 double       epsilon,
				 double      *a,    /* [0..iorder] */
				 double      *b);
void	gsl_filter_tscheb2_lp	(unsigned int iorder,
				 double       c_freq, /* 0..pi */
                                 double       steepness,
				 double       epsilon,
				 double      *a,      /* [0..iorder] */
				 double      *b);


/* --- highpass filters --- */
void	gsl_filter_butter_hp	(unsigned int iorder,
				 double       freq,   /* 0..pi */
				 double       epsilon,
				 double      *a,      /* [0..iorder] */
				 double      *b);
void	gsl_filter_tscheb1_hp	(unsigned int iorder,
				 double       freq,   /* 0..pi */
				 double       epsilon,
				 double      *a,      /* [0..iorder] */
				 double      *b);
void	gsl_filter_tscheb2_hp	(unsigned int iorder,
				 double       c_freq, /* 0..pi */
				 double       steepness,
				 double       epsilon,
				 double      *a,      /* [0..iorder] */
				 double      *b);


/* --- bandpass filters --- */
void	gsl_filter_butter_bp	(unsigned int iorder,
				 double       freq1,  /* 0..pi */
				 double       freq2,  /* 0..pi */
				 double       epsilon,
				 double      *a,      /* [0..iorder] */
				 double      *b);
void	gsl_filter_tscheb1_bp	(unsigned int iorder,
				 double       freq1,  /* 0..pi */
				 double       freq2,  /* 0..pi */
				 double       epsilon,
				 double      *a,      /* [0..iorder] */
				 double      *b);
void	gsl_filter_tscheb2_bp	(unsigned int iorder,
				 double       freq1,  /* 0..pi */
				 double       freq2,  /* 0..pi */
				 double       steepness,
				 double       epsilon,
				 double      *a,      /* [0..iorder] */
				 double      *b);


/* --- bandstop filters --- */
void	gsl_filter_butter_bs	(unsigned int iorder,
				 double       freq1,  /* 0..pi */
				 double       freq2,  /* 0..pi */
				 double       epsilon,
				 double      *a,      /* [0..iorder] */
				 double      *b);
void	gsl_filter_tscheb1_bs	(unsigned int iorder,
				 double       freq1,  /* 0..pi */
				 double       freq2,  /* 0..pi */
				 double       epsilon,
				 double      *a,      /* [0..iorder] */
				 double      *b);
void	gsl_filter_tscheb2_bs	(unsigned int iorder,
				 double       freq1,  /* 0..pi */
				 double       freq2,  /* 0..pi */
				 double       steepness,
				 double       epsilon,
				 double      *a,      /* [0..iorder] */
				 double      *b);


/* --- FIR Filters --- */
void	gsl_filter_fir_approx	(unsigned int  iorder,
				 double       *a,	/* [0..iorder] */
				 unsigned int  n_points,
				 const double *freq,
				 const double *value);


/* --- IIR Filter Evaluation --- */
typedef struct {
  guint order;
  gdouble *a;   /* [0..order] */
  gdouble *b;   /* [0..order] */
  gdouble *w;   /* [0..2*order] */
} GslIIRFilter;
void	gsl_iir_filter_setup	(GslIIRFilter	*f,
				 guint		 order,
				 const gdouble	*a,
				 const gdouble	*b,
				 gdouble	*buffer); /* 4*(order+1) */
void	gsl_iir_filter_change	(GslIIRFilter	*f,
				 guint		 order,
				 const gdouble	*a,
				 const gdouble	*b,
				 gdouble	*buffer); /* 4*(order+1) */
void	gsl_iir_filter_eval	(GslIIRFilter	*f,
				 guint		 n_values,
				 const gfloat	*x,
				 gfloat		*y);


/* --- Biquad Filters --- */
typedef enum	/*< skip >*/
{
  GSL_BIQUAD_NORMALIZE_PASSBAND,
  GSL_BIQUAD_NORMALIZE_RESONANCE_GAIN,
  GSL_BIQUAD_NORMALIZE_PEAK_GAIN
} GslBiquadNormalize;

typedef enum	/*< skip >*/
{
  GSL_BIQUAD_RESONANT_LOWPASS = 1,
  GSL_BIQUAD_RESONANT_HIGHPASS,
  GSL_BIQUAD_LOWSHELVE,
  GSL_BIQUAD_HIGHSHELVE,
  GSL_BIQUAD_PEAK
} GslBiquadType;

typedef struct {
  GslBiquadType      type;
  GslBiquadNormalize normalize;  	/* high/low pass */
  gfloat             f_fn;
  gfloat             gain;
  gfloat	     quality;		/* peak/notch */
  guint		     dirty : 1;		/* post filter_config() changes? */
  guint		     approx_values : 1;	/* biquad_config_approx_*() called? */
  /*< private >*/
  gdouble	     k, v;
} GslBiquadConfig;

typedef struct {
  gdouble xc0, xc1, xc2;
  gdouble yc1, yc2; /* yc0==1 */
  gdouble xd1, xd2, yd1, yd2; /* history */
} GslBiquadFilter;

void	gsl_biquad_config_init		(GslBiquadConfig	*c,
					 GslBiquadType		 type,
					 GslBiquadNormalize	 normalize);
void	gsl_biquad_config_setup		(GslBiquadConfig	*c,
					 gfloat			 f_fn,
					 gfloat			 gain,
					 gfloat			 quality);
void	gsl_biquad_config_approx_freq	(GslBiquadConfig	*c,
					 gfloat			 f_fn);
void	gsl_biquad_config_approx_gain	(GslBiquadConfig	*c,
					 gfloat			 gain);
void	gsl_biquad_filter_config	(GslBiquadFilter	*f,
					 GslBiquadConfig	*c,
					 gboolean		 reset_state);
void	gsl_biquad_filter_eval		(GslBiquadFilter	*f,
					 guint			 n_values,
					 const gfloat		*x,
					 gfloat			*y);


/* --- filter scanning -- */
gdouble	gsl_filter_sine_scan	(guint		 order,
				 const gdouble	*a,
				 const gdouble	*b,
				 gdouble	 freq,
				 guint		 n_values);


/* --- implementations --- */
static inline GslComplex
gsl_trans_s2z (GslComplex s)
{
  /*       1 + (Td/2) * s
   *  z = ----------------
   *       1 - (Td/2) * s
   */
  GslComplex one = { 1, 0 };
  return gsl_complex_div (gsl_complex_add (one, s), gsl_complex_sub (one, s));
  /* return gsl_complex_div (gsl_complex_sub (s, one), gsl_complex_add (s, one)); */
}
static inline double
gsl_trans_freq2s (double w)
{
  return tan (w / 2.);
}
static inline double
gsl_trans_zepsilon2ss (double zepsilon)
{
  double e2 = (1.0 - zepsilon) * (1.0 - zepsilon);
  /* 1___                                      _________________
   * |   \                                    |       1.0
   * |-----\<---- 1 - zepsilon  zepsilon = \  | ----------------
   * |_______\________________               \|  1 + sepsilon^2
   */
  return sqrt ((1.0 - e2) / e2);
}
static inline double
gsl_trans_freq2z (double w)
{
  return atan (w) * 2.;
}



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_FILTER_H__ */	/* vim:set ts=8 sw=2 sts=2: */
