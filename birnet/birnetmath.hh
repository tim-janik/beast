// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BIRNET_MATH_HH__
#define __BIRNET_MATH_HH__

#include <birnet/birnetutils.hh>
#include <math.h>

namespace Birnet {

/* --- double to integer --- */
inline int      dtoi32 (double d) BIRNET_CONST;
inline int64    dtoi64 (double d) BIRNET_CONST;
inline int64    iround (double d) BIRNET_CONST;
inline int64    ifloor (double d) BIRNET_CONST;
inline int64    iceil  (double d) BIRNET_CONST;

/* --- implementation bits --- */
inline int BIRNET_CONST
_dtoi32_generic (double d)
{
  /* this relies on the C++ behaviour of round-to-0 */
  return (int) (d < -0.0 ? d - 0.5 : d + 0.5);
}
inline int BIRNET_CONST
dtoi32 (double d)
{
  /* this relies on the hardware default round-to-nearest */
#if defined __i386__ && defined __GNUC__
  int r;
  __asm__ volatile ("fistl %0"
                    : "=m" (r)
                    : "t" (d));
  return r;
#endif
  return _dtoi32_generic (d);
}
inline int64 BIRNET_CONST
_dtoi64_generic (double d)
{
  /* this relies on the C++ behaviour of round-to-0 */
  return (int64) (d < -0.0 ? d - 0.5 : d + 0.5);
}
inline int64 BIRNET_CONST
dtoi64 (double d)
{
  /* this relies on the hardware default round-to-nearest */
#if defined __i386__ && defined __GNUC__
  int64 r;
  __asm__ volatile ("fistpll %0"
                    : "=m" (r)
                    : "t" (d)
                    : "st");
  return r;
#endif
  return _dtoi64_generic (d);
}
inline int64 BIRNET_CONST iround (double d) { return dtoi64 (round (d)); }
inline int64 BIRNET_CONST ifloor (double d) { return dtoi64 (floor (d)); }
inline int64 BIRNET_CONST iceil  (double d) { return dtoi64 (ceil (d)); }

} // Birnet

#endif /* __BIRNET_MATH_HH__ */
/* vim:set ts=8 sts=2 sw=2: */
