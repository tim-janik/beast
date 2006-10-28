/* BSE - Bedevilled Sound Engine
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
#ifndef BSE_IIR_FILTER_H__
#define BSE_IIR_FILTER_H__

// FIXME: #include <bse/bsemath.h>
#include <stdbool.h> // FIXME
typedef unsigned int uint; // FIXME

// FIXME: BIRNET_EXTERN_C_BEGIN();

/* --- Complex numeral --- */
typedef struct {
  double r;     // real part
  double i;     // imaginary part
} Complex;


typedef enum {
  BSE_IIR_FILTER_BUTTERWORTH = 1,
  BSE_IIR_FILTER_BESSEL      = 2,
  BSE_IIR_FILTER_CHEBYSHEV1  = 3,
  BSE_IIR_FILTER_CHEBYSHEV2  = 4,
  BSE_IIR_FILTER_ELLIPTIC    = 5,
} BseIIRFilterKind;

typedef enum {
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
  double                passband_edge;          /* Hz, > 0.0 && < nyquist_frequency */
  double                passband_ripple_db;     /* dB, > 0.0, not Butterworth */
  double                passband_edge2;         /* Hz, > 0.0 && < nyquist_frequency, for BAND filters */
  double                stopband_edge;          /* Hz, > 0.0, replaces stopband_db, elliptic only */
  double                stopband_db;            /* dB, < 0.0, elliptic only */
} BseIIRFilterRequest;

#define BSE_IIR_MAX_ORDER               (64)
#define BSE_IIR_CARRAY_SIZE             (4 * BSE_IIR_MAX_ORDER + 2) /* size of arrays used to store coefficients */

typedef struct {
  uint    order;
  double  sampling_frequency;
  /* s-plane output */
  double  spz[BSE_IIR_CARRAY_SIZE];     /* s-plane poles and zeros */
  /* z-plane poles and zeros */
  double  gain;
  Complex zp[BSE_IIR_CARRAY_SIZE / 2];  /* z-plane poles [order] */
  Complex zz[BSE_IIR_CARRAY_SIZE / 2];  /* z-plane zeros [order] */
  /* normalized z-plane transfer function */
  double  zn[BSE_IIR_CARRAY_SIZE];      /* numerator coefficients [order+1] */
  double  zd[BSE_IIR_CARRAY_SIZE];      /* denominator coefficients [order+1] */
} BseIIRFilterDesign;

typedef struct {
  int    n_poles;
  int    n_zeros;
  int    z_counter;	/* incremented as z^N coefficients are found, indexes poles and zeros */
  int    n_solved_poles;
  /* common state */
  double gain_scale;
  double ripple_epsilon;
  double nyquist_frequency;
  double tan_angle_frequency;
  double wc; /* tan_angle_frequency or normalized to 1.0 for elliptic */
  double cgam; /* angle frequency temporary */
  double stopband_edge; /* derived from ifr->stopband_edge or ifr->stopband_db */
  double wr;
  double numerator_accu;
  double denominator_accu;
  /* chebyshev state */
  double chebyshev_phi;
  double chebyshev_band_cbp;
  /* elliptic state */
  double elliptic_phi;
  double elliptic_k;
  double elliptic_u;
  double elliptic_m;
  double elliptic_Kk;  /* complete elliptic integral of the first kind of 1-elliptic_m */
  double elliptic_Kpk; /* complete elliptic integral of the first kind of elliptic_m */
  /* common output */
  double  gain;
  double  spz[BSE_IIR_CARRAY_SIZE];	/* s-plane poles and zeros */
  Complex zcpz[BSE_IIR_CARRAY_SIZE];	/* z-plane poles and zeros */
  /* normalized z-plane transfer function */
  double  zn[BSE_IIR_CARRAY_SIZE];      /* numerator coefficients [order+1] */
  double  zd[BSE_IIR_CARRAY_SIZE];      /* denominator coefficients [order+1] */
} DesignState;

static const DesignState default_design_state = {
  .n_poles = 0,
  .n_zeros = 0,
  .z_counter = 0,
  .n_solved_poles = 0,
  .gain_scale = 0.0,
  .ripple_epsilon = 0.0,
  .nyquist_frequency = 0.0,
  .tan_angle_frequency = 0.0,
  .wc = 0.0,
  .cgam = 0.0,
  .stopband_edge = 2400,
  .wr = 0.0,
  .numerator_accu = 0.0,
  .denominator_accu = 0.0,
  .chebyshev_phi = 0.0,
  .chebyshev_band_cbp = 0.0,
  .elliptic_phi = 0.0,
  .elliptic_k = 0.0,
  .elliptic_u = 0.0,
  .elliptic_m = 0.0,
  .elliptic_Kk = 0.0,
  .elliptic_Kpk = 0.0,
  .gain = 0.0,
  .spz = { 0, },
  .zn = { 0, },
  .zd = { 0, },
  .zcpz = { { 0, }, },
};

// FIXME: BIRNET_EXTERN_C_END();

#endif /* BSE_IIR_FILTER_H__ */	/* vim:set ts=8 sw=2 sts=2: */
