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

#define function_type   extern

#if 1
extern float    test1 (float v);
extern float    test2 (float v);
extern float    test3 (float v);
#else
inline float    test1 (float v) { return v;     }
inline float    test2 (float v) { return G_UNLIKELY (BSE_FLOAT_IS_SUBNORMAL (v)) ? (float) 0 : (float) v; }
inline float    test3 (float v) { if G_UNLIKELY (BSE_FLOAT_IS_SUBNORMAL (v)) return 0; else return v; }
#endif

int
main (int   argc,
      char *argv[])
{
  birnet_init (&argc, &argv, NULL);

  float n = 10 * 1000000;
  float sum;
  GTimer *timer = g_timer_new();
  volatile double volatile_accu = 0;

  sum = 0;
  g_timer_start (timer);
  for (float i = 0; i <= n; i += 1)
    {
      float v = BSE_FLOAT_MAX_SUBNORMAL * i / n;
      sum += test1 (v);
      sum += test1 (-v);
    }
  volatile_accu += sum;
  g_timer_stop (timer);
  float test1_time = g_timer_elapsed (timer, NULL);

  sum = 0;
  g_timer_start (timer);
  for (float i = 0; i <= n; i += 1)
    {
      float v = BSE_FLOAT_MAX_SUBNORMAL * i / n;
      sum += test2 (v);
      sum += test2 (-v);
    }
  volatile_accu += sum;
  g_timer_stop (timer);
  float test2_time = g_timer_elapsed (timer, NULL);

  sum = 0;
  g_timer_start (timer);
  for (float i = 0; i <= n; i += 1)
    {
      float v = BSE_FLOAT_MAX_SUBNORMAL * i / n;
      sum += test3 (v);
      sum += test3 (-v);
    }
  volatile_accu += sum;
  g_timer_stop (timer);
  float test3_time = g_timer_elapsed (timer, NULL);

  g_print ("test times in seconds: pass-along=%fs inlined-cond=%fs if-cond=%fs\n",
           test1_time, test2_time, test3_time);

  return 0;
}
