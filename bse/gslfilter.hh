// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __GSL_FILTER_H__
#define __GSL_FILTER_H__
#include <bse/bsemath.hh>
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
/* --- transformations --- */
static inline BseComplex bse_trans_s2z          (BseComplex     s);
static inline double     bse_trans_freq2s       (double         w);
static inline double     bse_trans_zepsilon2ss  (double         epsilon);
/* --- filter roots and poles --- */
void	gsl_filter_butter_rp    (uint         iorder,
				 double       freq,   /* 0..pi */
				 double       epsilon,
				 BseComplex  *roots,  /* [0..iorder-1] */
				 BseComplex  *poles);
void	gsl_filter_tscheb1_rp	(uint         iorder,
				 double       freq,   /* 0..pi */
				 double       epsilon,
				 BseComplex  *roots,  /* [0..iorder-1] */
				 BseComplex  *poles);
void	gsl_filter_tscheb2_rp	(uint         iorder,
				 double       c_freq, /* 0..pi */
				 double       steepness,
				 double       epsilon,
				 BseComplex  *roots,  /* [0..iorder-1] */
				 BseComplex  *poles);
/* --- tschebyscheff type II steepness --- */
double	gsl_filter_tscheb2_steepness_db	(uint         iorder,
					 double       c_freq,
					 double       epsilon,
					 double       stopband_db);
double	gsl_filter_tscheb2_steepness	(uint         iorder,
					 double       c_freq,
					 double       epsilon,
					 double       residue);
/* --- lowpass filters --- */
void	gsl_filter_butter_lp    (uint         iorder,
				 double       freq, /* 0..pi */
				 double       epsilon,
				 double      *a,    /* [0..iorder] */
				 double      *b);
void	gsl_filter_tscheb1_lp	(uint         iorder,
				 double       freq, /* 0..pi */
				 double       epsilon,
				 double      *a,    /* [0..iorder] */
				 double      *b);
void	gsl_filter_tscheb2_lp	(uint         iorder,
				 double       c_freq, /* 0..pi */
                                 double       steepness,
				 double       epsilon,
				 double      *a,      /* [0..iorder] */
				 double      *b);
/* --- highpass filters --- */
void	gsl_filter_butter_hp	(uint         iorder,
				 double       freq,   /* 0..pi */
				 double       epsilon,
				 double      *a,      /* [0..iorder] */
				 double      *b);
void	gsl_filter_tscheb1_hp	(uint         iorder,
				 double       freq,   /* 0..pi */
				 double       epsilon,
				 double      *a,      /* [0..iorder] */
				 double      *b);
void	gsl_filter_tscheb2_hp	(uint         iorder,
				 double       c_freq, /* 0..pi */
				 double       steepness,
				 double       epsilon,
				 double      *a,      /* [0..iorder] */
				 double      *b);
/* --- bandpass filters --- */
void	gsl_filter_butter_bp	(uint         iorder,
				 double       freq1,  /* 0..pi */
				 double       freq2,  /* 0..pi */
				 double       epsilon,
				 double      *a,      /* [0..iorder] */
				 double      *b);
void	gsl_filter_tscheb1_bp	(uint         iorder,
				 double       freq1,  /* 0..pi */
				 double       freq2,  /* 0..pi */
				 double       epsilon,
				 double      *a,      /* [0..iorder] */
				 double      *b);
void	gsl_filter_tscheb2_bp	(uint         iorder,
				 double       freq1,  /* 0..pi */
				 double       freq2,  /* 0..pi */
				 double       steepness,
				 double       epsilon,
				 double      *a,      /* [0..iorder] */
				 double      *b);
/* --- bandstop filters --- */
void	gsl_filter_butter_bs	(uint         iorder,
				 double       freq1,  /* 0..pi */
				 double       freq2,  /* 0..pi */
				 double       epsilon,
				 double      *a,      /* [0..iorder] */
				 double      *b);
void	gsl_filter_tscheb1_bs	(uint         iorder,
				 double       freq1,  /* 0..pi */
				 double       freq2,  /* 0..pi */
				 double       epsilon,
				 double      *a,      /* [0..iorder] */
				 double      *b);
void	gsl_filter_tscheb2_bs	(uint         iorder,
				 double       freq1,  /* 0..pi */
				 double       freq2,  /* 0..pi */
				 double       steepness,
				 double       epsilon,
				 double      *a,      /* [0..iorder] */
				 double      *b);
/* --- FIR Filters --- */
void	gsl_filter_fir_approx	(uint          iorder,
				 double       *a,	/* [0..iorder] */
				 uint          n_points,
				 const double *freq,
				 const double *value,
				 gboolean      interpolate_db);
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
				 gdouble	 mix_freq);
/* --- implementations --- */
static inline BseComplex
bse_trans_s2z (BseComplex s)
{
  /*       1 + (Td/2) * s
   *  z = ----------------
   *       1 - (Td/2) * s
   */
  BseComplex one = { 1, 0 };
  return bse_complex_div (bse_complex_add (one, s), bse_complex_sub (one, s));
  /* return bse_complex_div (bse_complex_sub (s, one), bse_complex_add (s, one)); */
}
static inline double
bse_trans_freq2s (double w)
{
  return tan (w / 2.);
}
static inline double
bse_trans_zepsilon2ss (double zepsilon)
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
bse_trans_freq2z (double w)
{
  return atan (w) * 2.;
}
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __GSL_FILTER_H__ */	/* vim:set ts=8 sw=2 sts=2: */
