/* BSE - Better Sound Engine
 * Copyright (C) 2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include <bse/bse.h>
#include <bse/bseieee754.h>

float
test1f (float v)
{
  return v;
}

float
test2f (float v)
{
  return bse_float_zap_denormal (v);
}

float
test3f (float v)
{
  BSE_FLOAT_FLUSH_with_cond (v);
  return v;
}

float
test4f (float v)
{
  BSE_FLOAT_FLUSH_with_if (v);
  return v;
}

float
test5f (float v)
{
  BSE_FLOAT_FLUSH_with_threshold (v);
  return v;
}
