// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_RESAMPLER_HH__
#define __BSE_RESAMPLER_HH__

#include <bse/bsecxxutils.hh>
#include <vector>

namespace Bse {

/**
 * Interface for factor 2 resampling classes
 */
class Resampler2 {
  class Impl
  {
  public:
    virtual void   process_block (const float *input, uint n_input_samples, float *output) = 0;
    virtual uint   order() const = 0;
    virtual double delay() const = 0;
    virtual void   reset() = 0;
    virtual bool   sse_enabled() const = 0;
    virtual
    ~Impl()
    {
    }
  };
  std::unique_ptr<Impl> impl;

  template<uint ORDER, bool USE_SSE>
  class Upsampler2;
  template<uint ORDER, bool USE_SSE>
  class Downsampler2;
public:
  enum Mode {
    UP,
    DOWN
  };
  enum Precision {
    PREC_LINEAR = 1,     /* linear interpolation */
    PREC_48DB = 8,
    PREC_72DB = 12,
    PREC_96DB = 16,
    PREC_120DB = 20,
    PREC_144DB = 24
  };
  /**
   * creates a resampler instance fulfilling a given specification
   */
  Resampler2 (Mode      mode,
              Precision precision,
              bool      use_sse_if_available = true);
  /**
   * returns true if an optimized SSE version of the Resampler is available
   */
  static bool        sse_available();
  /**
   * test internal filter implementation
   */
  static bool        test_filter_impl (bool verbose);
  /**
   * finds a precision which is appropriate for at least the specified number of bits
   */
  static Precision   find_precision_for_bits (uint bits);
  /**
   * returns a human-readable name for a given precision
   */
  static const char  *precision_name (Precision precision);
  /**
   * resample a data block
   */
  void
  process_block (const float *input, uint n_input_samples, float *output)
  {
    impl->process_block (input, n_input_samples, output);
  }
  /**
   * return FIR filter order
   */
  uint
  order() const
  {
    return impl->order();
  }
  /**
   * Return the delay introduced by the resampler. This delay is guaranteed to
   * be >= 0.0, and for factor 2 resampling always a multiple of 0.5 (1.0 for
   * upsampling).
   *
   * The return value can also be thought of as index into the output signal,
   * where the first input sample can be found.
   *
   * Beware of fractional delays, for instance for downsampling, a delay() of
   * 10.5 means that the first input sample would be found by interpolating
   * output[10] and output[11], and the second input sample equates output[11].
   */
  double
  delay() const
  {
    return impl->delay();
  }
  /**
   * clear internal history, reset resampler state to zero values
   */
  void
  reset()
  {
    impl->reset();
  }
  /**
   * return whether the resampler is using sse optimized code
   */
  bool
  sse_enabled() const
  {
    return impl->sse_enabled();
  }
protected:
  /* Creates implementation from filter coefficients and Filter implementation class
   *
   * Since up- and downsamplers use different (scaled) coefficients, its possible
   * to specify a scaling factor. Usually 2 for upsampling and 1 for downsampling.
   */
  template<class Filter> static inline Impl*
  create_impl_with_coeffs (const double *d,
	                   uint          order,
	                   double        scaling)
  {
    float taps[order];
    for (uint i = 0; i < order; i++)
      taps[i] = d[i] * scaling;

    Resampler2::Impl *filter = new Filter (taps);
    BSE_ASSERT_RETURN (order == filter->order(), NULL);
    return filter;
  }
  /* creates the actual implementation; specifying USE_SSE=true will use
   * SSE instructions, USE_SSE=false will use FPU instructions
   *
   * Don't use this directly - it's only to be used by
   * bseblockutils.cc's anonymous Impl classes.
   */
  template<bool USE_SSE> static inline Impl*
  create_impl (Mode      mode,
	       Precision precision);
};

} /* namespace Bse */

#endif /* __BSE_RESAMPLER_HH__ */
