/* BSE - Better Sound Engine
 * Copyright (C) 2006 Tim Janik
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
#ifndef BSE_FILTER_H__
#define BSE_FILTER_H__

#include <bse/bsemath.h>

BIRNET_EXTERN_C_BEGIN();

typedef enum /*< skip >*/
{
  BSE_IIR_FILTER_BUTTERWORTH = 1,
  BSE_IIR_FILTER_BESSEL      = 2,
  BSE_IIR_FILTER_CHEBYSHEV1  = 3,
  BSE_IIR_FILTER_CHEBYSHEV2  = 4,
  BSE_IIR_FILTER_ELLIPTIC    = 5,
} BseIIRFilterKind;

typedef enum /*< skip >*/
{
  BSE_IIR_FILTER_LOW_PASS    = 1,
  BSE_IIR_FILTER_BAND_PASS   = 2,
  BSE_IIR_FILTER_HIGH_PASS   = 3,
  BSE_IIR_FILTER_BAND_STOP   = 4,
} BseIIRFilterType;

typedef struct {
  BseIIRFilterKind      kind;
  BseIIRFilterType      type;
  uint                  order;                  /*     >= 1 */
  double                sampling_frequency;     /* Hz, > 0.0 && == 2 * nyquist_frequency */
  double                passband_ripple_db;     /* dB, > 0.0, Chebyshev1 or elliptic */
  double                passband_edge;          /* Hz, > 0.0 && < nyquist_frequency */
  double                passband_edge2;         /* Hz, > 0.0 && < nyquist_frequency, for BAND filters */
  double                stopband_edge;          /* Hz, > 0.0, replaces stopband_db, elliptic only */
  double                stopband_db;            /* dB, < 0.0, elliptic only */
} BseIIRFilterRequest;

#define BSE_IIR_MAX_ORDER		(64)
#define BSE_IIR_CARRAY_SIZE		(4 * BSE_IIR_MAX_ORDER + 2) /* size of arrays used to store coefficients */

typedef struct {
  double     sampling_frequency;		/* same as BseIIRFilterRequest.sampling_frequency */
  uint 	     order;
  double     center_frequency;
  /* z-plane poles and zeros */
  double     gain;
  uint       n_zeros;
  BseComplex zz[BSE_IIR_CARRAY_SIZE / 2];	/* z-plane zeros [order] */
  uint       n_poles;
  BseComplex zp[BSE_IIR_CARRAY_SIZE / 2];	/* z-plane poles [order] */
  /* normalized z-plane transfer function */
  // double zn[BSE_IIR_CARRAY_SIZE];	/* numerator coefficients [order+1] */
  // double zd[BSE_IIR_CARRAY_SIZE];	/* denominator coefficients [order+1] */
} BseIIRFilterDesign;

typedef struct {
  double xz2;   // x[i-2] coefficient
  double xz1;   // x[i-1] coefficient
  double yz2;   // y[i-2] coefficient
  double yz1;   // y[i-1] coefficient
} BseIIRStage;

typedef struct {
  uint         order;
  BseIIRStage *stages;
  double      *states;	/* [0..2*order] */
} BseIIRFilter;


bool		bse_iir_filter_design	(const BseIIRFilterRequest  *filter_request,
					 BseIIRFilterDesign         *filter_design);
BseIIRFilter*	bse_iir_filter_new	(const BseIIRFilterDesign   *filter_design);
void          	bse_iir_filter_change	(BseIIRFilter		    *filter,
					 const BseIIRFilterDesign   *filter_design);
void          	bse_iir_filter_eval	(BseIIRFilter		    *filter,
					 uint           	     n_values,
					 const float	   	    *x,
					 float         		    *y);
void          	bse_iir_filter_free	(BseIIRFilter		    *filter);
const gchar*	bse_iir_filter_kind_string	(BseIIRFilterKind       fkind);
const gchar*	bse_iir_filter_type_string	(BseIIRFilterType       ftype);
gchar*	bse_iir_filter_request_string	(const BseIIRFilterRequest  *filter_request);
gchar*	bse_iir_filter_design_string	(const BseIIRFilterDesign   *filter_design);
gchar*	bse_iir_filter_string		(const BseIIRFilter         *filter);


/* --- internal prototypes --- */
bool	_bse_filter_design_ellf 	(const BseIIRFilterRequest      *ifr,
					 BseIIRFilterDesign             *fid);

BIRNET_EXTERN_C_END();

#endif /* BSE_FILTER_H__ */	/* vim:set ts=8 sw=2 sts=2: */
