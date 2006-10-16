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

typedef enum {
  BSE_IIR_FILTER_BUTTERWORTH = 1,
  BSE_IIR_FILTER_CHEBYSHEV   = 2,
  BSE_IIR_FILTER_ELLIPTIC    = 3,
} BseIIRFilterKind;
typedef enum {
  BSE_IIR_FILTER_LOW_PASS    = 1,
  BSE_IIR_FILTER_BAND_PASS   = 2,
  BSE_IIR_FILTER_HIGH_PASS   = 3,
  BSE_IIR_FILTER_BAND_STOP   = 4,
} BseIIRFilterType;
typedef struct {
  BseIIRFilterKind	kind;
  BseIIRFilterType	type;
  uint                  order;			/* >= 1 */
  double		passband_ripple_db; 	/* dB, not Butterworth */
  double 		sampling_frequency;	/* Hz, > 0.0 */
  double                passband_edge;		/* Hz, > 0.0 && < nyquist_frequency */
  double                passband_edge2;		/* Hz, > 0.0 && < nyquist_frequency, for BAND filters */
  double                stopband_db;		/* dB < 0, elliptic only */
  double                stopband_edge;		/* Hz, > 0.0 && < nyquist_frequency, elliptic only */
} BseIIRFilterRequirements;

typedef struct {
  int    n_poles;
  int    n_zeros;
  double gain_scale;
  double ripple_epsilon;
  double nyquist_frequency;
  double tan_angle_frequency;
  double wc; /* tan_angle_frequency or normalized to 1.0 for elliptic */
  double cgam; /* angle frequency temporary */
  double stopband_edge; /* derived from ifr->stopband_edge or ifr->stopband_db */
  double wr;
  double elliptic_k;
  double elliptic_u;
  double elliptic_m;
  double chebyshev_band_cbp;
} DesignState;

static const DesignState default_design_state = {
  .n_poles = 0.0,
  .n_zeros = 0.0,
  .gain_scale = 0.0,
  .ripple_epsilon = 0.0,
  .nyquist_frequency = 0.0,
  .tan_angle_frequency = 0.0,
  .wc = 0.0,
  .cgam = 0.0,
  .stopband_edge = 2400,
  .wr = 0.0,
  .elliptic_k = 0.0,
  .elliptic_u = 0.0,
  .elliptic_m = 0.0,
  .chebyshev_band_cbp = 0.0,
};

// FIXME: BIRNET_EXTERN_C_END();

#endif /* BSE_IIR_FILTER_H__ */	/* vim:set ts=8 sw=2 sts=2: */
