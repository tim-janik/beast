// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_SIGNALMATH_HH__
#define __BSE_SIGNALMATH_HH__

#include <bse/bseieee754.hh>

namespace Bse {

/// Round float to int, using round-to-nearest
/// Fast version of `f < 0 ? int (f - 0.5) : int (f + 0.5)`.
extern inline G_GNUC_CONST int irintf (float f) { return __builtin_rintf (f); }

/// Force number into double precision floating point format, even with `-ffast-math`.
extern inline double force_double (double d)   { volatile double v = d; return v; }

/// Force number into single precision floating point format, even with `-ffast-math`.
extern inline float  force_float  (float  f)   { volatile float v = f; return v; }

/**
 * @param ex	exponent within [-127..+127]
 * @return	fast approximation of `2^x`
 *
 * Fast approximation of 2 raised to the power of `x`.
 * Within -1..+1, the error stays below 4e-7 which corresponds to a sample
 * precision of 21 bit. For integer values of `x` (i.e. `x - floor (x) -> 0`),
 * the error approaches zero. With FMA instructions and -ffast-math enabled,
 * execution times should be below 10ns on 3GHz machines.
 */
extern inline float fast_exp2   (float x)       G_GNUC_CONST;

/**
 * @param ex	exponent within [1.1e-38..2^127]
 * @return	fast approximation of `log2(x)`
 *
 * Fast approximation of logarithm to base 2.
 * Within 1e-7..+1, the error stays below 3.8e-6 which corresponds to a sample
 * precision of 18 bit. When `x` is an exact power of 2, the error approaches
 * zero. With FMA instructions and -ffast-math enabled, execution times should
 * be below 10ns on 3GHz machines.
 */
extern inline float fast_log2   (float x)       G_GNUC_CONST;

// == Implementations ==
extern inline G_GNUC_CONST float
fast_exp2 (float ex)
{
  BseFloatIEEE754 fp = { 0, };
  // const int i = ex < 0 ? int (ex - 0.5) : int (ex + 0.5);
  const int i = irintf (ex);
  fp.mpn.biased_exponent = BSE_FLOAT_BIAS + i;
  const float x = ex - i;
  float r;

  // f=2^x; remez(1, 5, [-.5;.5], 1/f, 1e-16); // minimized relative error
  r = x *  0.0013276471992255f;
  r = x * (0.0096755413344448f + r);
  r = x * (0.0555071327349880f + r);
  r = x * (0.2402211972384019f + r);
  r = x * (0.6931469670647601f + r);
  r = fp.v_float * (1.0f + r);
  return r;
}

extern inline G_GNUC_CONST float
fast_log2 (float value)
{
  static_assert (127 == BSE_FLOAT_BIAS);

  // log2 (i*x) = log2 (i) + log2 (x)
  BseFloatIEEE754 u { value };                  // v_float = 2^(biased_exponent-127) * mantissa
  const int i = u.mpn.biased_exponent - 127;    // extract exponent without bias
  u.mpn.biased_exponent = 127;                  // reset to 2^0 so v_float is mantissa in [1..2]
  float r, x = u.v_float - 1.0f;                // x=[0..1]; r = log2 (x + 1);

  // h=0.0113916; // offset to reduce error at origin
  // f=(1/log(2)) * log(x+1); dom=[0-h;1+h]; p=remez(f, 6, dom, 1);
  // p = p - p(0); // discard non-0 offset
  // err=p-f; plot(err,[0;1]); plot(f,p,dom); // result in sollya
  r = x *  -0.0259366993544709205147977455165000143561553284592936f;
  r = x * (+0.122047857676447181074792747820717519424533931189428f + r);
  r = x * (-0.27814297685064327713977752916286528359628147166014f + r);
  r = x * (+0.45764712300320092992105460899527194244236573556309f + r);
  r = x * (-0.71816105664624015087225994551041120290062342459945f + r);
  r = x * (+1.44254540258782520489769598315182363877204824648687f + r);
  return i + r; // log2 (i) + log2 (x)
}

/// Logarithmically map (and invert) a range onto 0…+1.
struct Logscale {
  double b2 = 0;         // log2 (begin)
  double r2 = 1, ir = 1; // range
  /// Provide minimum and maximum values to be mapped.
  void
  setup (double min, double max)
  {
    b2 = ::log2l (min);
    auto range = ::log2l (max) - b2;
    ir = 1.0 / range;
    r2 = range;
  }
  /// Calculate scale value within `[min … max]` from normalized `x`.
  double
  scale (double normalized) const
  {
    return ::exp2 (b2 + normalized * r2);
  }
  /// Calculate `normalized` from a `scale()` result within `[min … max]`.
  double
  iscale (double mmvalue) const
  {
    return (::log2 (mmvalue) - b2) * ir;
  }
};

} // Bse

#endif // __BSE_SIGNALMATH_HH__
