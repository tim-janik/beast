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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include	"bsemath.h"

#include <stdlib.h>


/* --- functions --- */
gboolean
bse_poly2_droots (gdouble roots[2],
		  gdouble a,
		  gdouble b,
		  gdouble c)
{
  gdouble square = b * b - 4.0 * a * c;
  gdouble tmp;

  if (square < 0)
    return FALSE;

  if (b > 0)
    tmp = -b - sqrt (square);
  else
    tmp = -b + sqrt (square);

  roots[0] = tmp / (a + a);
  roots[1] = (c + c) / tmp;

  return TRUE;
}

gint
bse_rand_bool (void)
{
  return rand () & 1;	// FIXME
}
