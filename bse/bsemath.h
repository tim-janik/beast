/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2001 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BSE_MATH_H__
#define __BSE_MATH_H__

#include        <bse/bsedefs.h>
#include        <math.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- constants --- */
#define BSE_2_POW_1_DIV_12_d          ( /* 2^(1/12) */ \
              1.0594630943592952645618252949463417008)
#define BSE_LN_2_POW_1_DIV_12_d       ( /* ln(2^(1/12)) */ \
              5.776226504666210911810267678818138067296e-2)
#define BSE_LN_2_POW_1_DIV_1200_d     ( /* ln(2^(1/1200)) */ \
              5.776226504666210911810267678818138067296e-4)


/* --- functions --- */
gboolean	bse_poly2_droots	(gdouble roots[2],
					 gdouble a,
					 gdouble b,
					 gdouble c);
gint		bse_rand_int		(void); /* +-G_MAXINT */
gfloat		bse_rand_float		(void); /* -1.0..1.0 */
gint		bse_rand_bool		(void); /* random bit */


/* --- float/double/math utilities and consts --- */
#define BSE_EPSILON                       (1e-8 /* threshold, coined for 24 bit */)
#define BSE_EPSILON_CMP(double1, double2) (_bse_epsilon_cmp ((double1), (double2)))
#undef PI
#if   defined (M_PIl)
#  define PI    (M_PIl)
#else /* !math.h M_PIl */
#  define PI    (3.1415926535897932384626433832795029)
#endif


/* --- implementation details --- */
static inline gint
_bse_epsilon_cmp (gdouble double1,
                  gdouble double2)
{
  register gfloat diff = double1 - double2;

  return diff > BSE_EPSILON ? 1 : diff < - BSE_EPSILON ? -1 : 0;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_MATH_H__ */
