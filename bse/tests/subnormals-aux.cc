/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <bse/bse.h>
#include <bse/bseieee754.h>

// #define FLOAT_IS_SUBNORMAL(foo)      BSE_FLOAT_IS_SUBNORMAL (foo)
#define FLOAT_IS_SUBNORMAL(foo)         (fabs (foo) < 1e-32)


float
test1 (float v)
{
  return v;
}

float
test2 (float v)
{
  return G_UNLIKELY (FLOAT_IS_SUBNORMAL (v)) ? (float) 0 : (float) v;
}

float
test3 (float v)
{
  if G_UNLIKELY (FLOAT_IS_SUBNORMAL (v)) return 0; else return v;
}
