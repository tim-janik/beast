/* Birnet
 * Copyright (C) 2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
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
