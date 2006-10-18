/* BseResampler - FPU and SSE optimized FIR Resampling code
 * Copyright (C) 2006 Stefan Westerfeld
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
#ifndef __BSE_RESAMPLER_HH__
#define __BSE_RESAMPLER_HH__

#include <glib.h>

G_BEGIN_DECLS

typedef struct BseResampler2 BseResampler2;

typedef enum /*< skip >*/
{
  BSE_RESAMPLER2_MODE_UPSAMPLE,
  BSE_RESAMPLER2_MODE_DOWNSAMPLE
} BseResampler2Mode;

typedef enum /*< skip >*/
{
  BSE_RESAMPLER2_PREC_48DB = 8,
  BSE_RESAMPLER2_PREC_72DB = 12,
  BSE_RESAMPLER2_PREC_96DB = 16,
  BSE_RESAMPLER2_PREC_120DB = 20,
  BSE_RESAMPLER2_PREC_144DB = 24
} BseResampler2Precision;

BseResampler2* bse_resampler2_create        (BseResampler2Mode      mode,
                                             BseResampler2Precision precision);
void           bse_resampler2_destroy       (BseResampler2         *resampler);
void	       bse_resampler2_process_block (BseResampler2         *resampler,
                                             const float           *input,
                                             unsigned int           n_input_samples,
					     float                 *output);
guint	       bse_resampler2_order         (BseResampler2         *resampler);
double	       bse_resampler2_delay         (BseResampler2         *resampler);

G_END_DECLS

#ifdef __cplusplus
#include <vector>

namespace Bse {
namespace Resampler {

/**
 * Interface for factor 2 resampling classes
 */
class Resampler2 {
public:
  /**
   * creates a resampler instance fulfilling a given specification
   */
  static  Resampler2* create (BseResampler2Mode      mode,
			      BseResampler2Precision precision);
  /**
   * virtual destructor for abstract class
   */
  virtual	      ~Resampler2();
  /**
   * resample a data block
   */
  virtual void	      process_block (const float *input, unsigned int n_input_samples, float *output) = 0;
  /**
   * return FIR filter order
   */
  virtual guint	      order() const = 0;
  /**
   * Return the delay introduced by the resampler. This delay is guaranteed to
   * be >= 0.0, and for factor 2 resampling always a multiple of 0.5 (2.0 for
   * upsampling).
   *
   * The return value can also be thought of as index into the output signal,
   * where the first input sample can be found.
   *
   * Beware of fractional delays, for instance for downsampling, a delay() of
   * 10.5 means that the first input sample would be found by interpolating
   * output[10] and output[11], and the second input sample equates output[11].
   */
  virtual double      delay() const = 0;
protected:
  static const double halfband_fir_48db_coeffs[16];
  static const double halfband_fir_72db_coeffs[24];
  static const double halfband_fir_96db_coeffs[32];
  static const double halfband_fir_120db_coeffs[42];
  static const double halfband_fir_144db_coeffs[52];
  
  /* Creates implementation from filter coefficients and Filter implementation class
   *
   * Since up- and downsamplers use different (scaled) coefficients, its possible
   * to specify a scaling factor. Usually 2 for upsampling and 1 for downsampling.
   */
  template<class Filter> static inline Resampler2*
  create_impl_with_coeffs (const double *d,
	                   guint         order,
	                   double        scaling)
  {
    float taps[order];
    for (guint i = 0; i < order; i++)
      taps[i] = d[i] * scaling;
    
    Resampler2 *filter = new Filter (taps);
    g_assert (order == filter->order());
    return filter;
  }
  /* creates the actual implementation; specifying USE_SSE=true will use
   * SSE instructions, USE_SSE=false will use FPU instructions
   *
   * Don't use this directly - it's only to be used by
   * bseblockutils.cc's anonymous Impl classes.
   */
  template<bool USE_SSE> static inline Resampler2*
  create_impl (BseResampler2Mode      mode,
	       BseResampler2Precision precision);
};

} /* namespace Resampler */

} /* namespace Bse */

#endif /* __cplusplus */

#endif /* __BSE_RESAMPLER_HH__ */
