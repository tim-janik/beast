// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_SIGNALMATH_HH__
#define __BSE_SIGNALMATH_HH__

#include <bse/bseieee754.hh>

namespace Bse {

/// Round float to int, using round-to-nearest
/// Fast version of `f < 0 ? int (f - 0.5) : int (f + 0.5)`.
extern inline G_GNUC_CONST int irintf (float f) { return __builtin_rintf (f); }

} // Bse

#endif // __BSE_SIGNALMATH_HH__
