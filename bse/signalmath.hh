// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_SIGNALMATH_HH__
#define __BSE_SIGNALMATH_HH__

#include <bse/bseieee754.hh>

namespace Bse {

/// Round float to int, using round-to-nearest
/// Fast version of `f < 0 ? int (f - 0.5) : int (f + 0.5)`.
extern inline G_GNUC_CONST int irintf (float f) { return __builtin_rintf (f); }

/**
 * @param ex	exponent within [-127..+127]
 * @return	fast approximation of `2^ex`
 *
 * Fast approximation of 2 raised to the power of `ex`.
 * Within -1..+1, the error stays below 4e-7 which corresponds to a sample
 * precision of 21 bit. For integer values of `ex` (i.e. `ex - floor (ex) -> 0`),
 * the error approaches zero. With FMA instructions and -ffast-math enabled,
 * execution times should be below 10ns on 3GHz machines.
 */
extern inline float fast_exp2   (float ex)      G_GNUC_CONST;

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

} // Bse

#endif // __BSE_SIGNALMATH_HH__
