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
#ifndef __GSL_FILTER_H__
#define __GSL_FILTER_H__

#include <gsl/gslmath.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


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
				 double       r_freq, /* 0..pi */
				 double       epsilon,
				 GslComplex  *roots,  /* [0..iorder-1] */
				 GslComplex  *poles);


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


/* --- fir filters --- */
void	gsl_filter_fir_approx	(unsigned int  iorder,
				 double       *a,	/* [0..iorder] */
				 unsigned int  n_points,
				 const double *freq,
				 const double *value);





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_FILTER_H__ */	/* vim:set ts=8 sw=2 sts=2: */
