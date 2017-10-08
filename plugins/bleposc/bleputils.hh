// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html

#ifndef __BSE_BLEPUTILS_HH__
#define __BSE_BLEPUTILS_HH__

#include <assert.h>

namespace Bse {
namespace BlepUtils {

inline double
bessel_i0 (double x)
{
  /* http://www.vibrationdata.com/Bessel.htm */

  /* 1 + (x/2)^2/(1!^2)
   *   + (x/2)^4/(2!^2)
   *   + (x/2)^6/(3!^2)   ... */

  double delta = 1;
  double result = 1;
  const double sqr_x_2 = (x/2)*(x/2);

  for (int i = 1; i < 500; i++)
    {
      delta *= sqr_x_2 / (i * i);
      result += delta;

      if (delta < 1e-14 * result)
        break;
    }
  return result;
}

inline double
window_kaiser (double x, double beta)
{
  // https://en.wikipedia.org/wiki/Kaiser_window
  const double pos = std::max (1.0 - x * x, 0.0); // avoid problems for |x| > 1

  return bessel_i0 (M_PI * beta * sqrt (pos)) / bessel_i0 (M_PI * beta);
}

}
}

#endif
