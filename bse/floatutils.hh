// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_FLOAT_UTILS_HH__
#define __BSE_FLOAT_UTILS_HH__

#include <bse/memory.hh>
#include <cmath>

namespace Bse {

/// Fill `n` values of `dst` with `f`.
extern inline void
floatfill (float *dst, float f, size_t n)
{
  for (size_t i = 0; i < n; i++)
    dst[i] = f;
}

/// Copy `n` values from `src` to `dst`, the buffers must not overlap.
extern inline void
floatcopy (float *__restrict dst, const float *__restrict src, size_t n)
{
  for (size_t i = 0; i < n; i++)
    dst[i] = src[i];
}

/// Add `n` values from `a` pairwise to `b` and store the result in `dst`.
extern inline void
floatadd (float *__restrict dst, const float *__restrict a, const float *__restrict b, size_t n)
{
  for (size_t i = 0; i < n; i++)
    dst[i] = a[i] + b[i];
}

/// For `n` values, multiply `src` and `factors` pairwise and store the result in `dst`.
extern inline void
floatmul (float *__restrict dst, const float *__restrict src, const float *__restrict factors, size_t n)
{
  for (size_t i = 0; i < n; i++)
    dst[i] = src[i] * factors[i];
}

/// Multiply all `n` values from `src` with `scalar` and store the result in `dst`.
extern inline void
floatscale (float *__restrict dst, const float *__restrict src, float scalar, size_t n)
{
  for (size_t i = 0; i < n; i++)
    dst[i] = src[i] * scalar;
}

} // Bse

#endif // __BSE_FLOAT_UTILS_HH__
