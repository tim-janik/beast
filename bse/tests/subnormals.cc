/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2006 Tim Janik
 * Copyright (C) 2006 Stefan Westerfeld
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
#include <bse/bsemain.h>
//#define TEST_VERBOSE
#include <birnet/birnettests.h>
#include <stdio.h>

#if 1
inline float  test1f (float v) { return v;     }
inline float  test2f (float v) { return bse_float_zap_denormal (v); }
inline float  test3f (float v) { BSE_FLOAT_FLUSH_with_cond (v); return v; }
inline float  test4f (float v) { BSE_FLOAT_FLUSH_with_if (v); return v; }
inline float  test5f (float v) { BSE_FLOAT_FLUSH_with_threshold (v); return v; }
#else
extern float  test1f (float v);
extern float  test2f (float v);
extern float  test3f (float v);
extern float  test4f (float v);
extern float  test5f (float v);
#endif

inline double test2d (double v) { return bse_double_zap_denormal (v); }
inline double test3d (double v) { BSE_DOUBLE_FLUSH_with_cond (v); return v; }
inline double test4d (double v) { BSE_DOUBLE_FLUSH_with_if (v); return v; }
inline double test5d (double v) { BSE_DOUBLE_FLUSH_with_threshold (v); return v; }

template<float Func (float)> void
test_correct_subnormal_elimination (const char* algo_name)
{
  TSTART ("testing algorithm %s for correctness", algo_name);
  const int n = 1000000;
  for (int i = 1; i < n; i++)
    {
      float value = BSE_FLOAT_MAX_SUBNORMAL * i / n;
      g_assert (BSE_FLOAT_IS_SUBNORMAL (value));

      float normalized_positive_value = Func (value);
      g_assert (!BSE_FLOAT_IS_SUBNORMAL (normalized_positive_value));

      float normalized_negative_value = Func (-value);
      g_assert (!BSE_FLOAT_IS_SUBNORMAL (normalized_negative_value));

      if (i % 100000 == 0)
        TOK();
    }
  TOK();
  TDONE();
}

template<double Func (double)> void
test_correct_subnormal_elimination (const char* algo_name)
{
  TSTART ("testing algorithm %s for correctness", algo_name);
  const int n = 1000000;
  for (int i = 1; i < n; i++)
    {
      double value = BSE_DOUBLE_MAX_SUBNORMAL * i / n;
      g_assert (BSE_DOUBLE_IS_SUBNORMAL (value));

      double normalized_positive_value = Func (value);
      g_assert (!BSE_DOUBLE_IS_SUBNORMAL (normalized_positive_value));

      double normalized_negative_value = Func (-value);
      g_assert (!BSE_DOUBLE_IS_SUBNORMAL (normalized_negative_value));

      if (i % 100000 == 0)
        TOK();
    }
  TOK();
  TDONE();
}

int
main (int   argc,
      char *argv[])
{
  bse_init_test (&argc, &argv, NULL);

  test_correct_subnormal_elimination<test2f> ("zap");
  test_correct_subnormal_elimination<test3f> ("inlined-cond");
  test_correct_subnormal_elimination<test4f> ("if-cond");
  test_correct_subnormal_elimination<test5f> ("arithmetic");

  test_correct_subnormal_elimination<test2d> ("zap-double");
  test_correct_subnormal_elimination<test3d> ("inlined-cond-double");
  test_correct_subnormal_elimination<test4d> ("if-cond-double");
  test_correct_subnormal_elimination<test5d> ("arithmetic-double");

  g_print ("benchmarking...\n");
  const float max_sub = BSE_FLOAT_MAX_SUBNORMAL;

  float n = 10 * 1000000;
  float sum;
  GTimer *timer = g_timer_new();
  volatile double volatile_accu = 0;

  int j;
  const int blen = 4096;
  volatile float buffer[blen];

  sum = j = 0;
  memset ((void*) buffer, 0, sizeof (buffer));
  g_timer_start (timer);
  for (float i = 0; i <= n; i += 1)
    {
      float v = max_sub * i / n;
      buffer[j++] = test1f (v);
      buffer[j++] = test1f (-v);
      j %= blen;
    }
  volatile_accu += sum;
  g_timer_stop (timer);
  float test1_time = g_timer_elapsed (timer, NULL);

  sum = j = 0;
  memset ((void*) buffer, 0, sizeof (buffer));
  g_timer_start (timer);
  for (float i = 0; i <= n; i += 1)
    {
      float v = max_sub * i / n;
      buffer[j++] = test2f (v);
      buffer[j++] = test2f (-v);
      j %= blen;
    }
  volatile_accu += sum;
  g_timer_stop (timer);
  float test2_time = g_timer_elapsed (timer, NULL);

  sum = j = 0;
  memset ((void*) buffer, 0, sizeof (buffer));
  g_timer_start (timer);
  for (float i = 0; i <= n; i += 1)
    {
      float v = max_sub * i / n;
      buffer[j++] = test3f (v);
      buffer[j++] = test3f (-v);
      j %= blen;
    }
  volatile_accu += sum;
  g_timer_stop (timer);
  float test3_time = g_timer_elapsed (timer, NULL);

  sum = j = 0;
  memset ((void*) buffer, 0, sizeof (buffer));
  g_timer_start (timer);
  for (float i = 0; i <= n; i += 1)
    {
      float v = max_sub * i / n;
      buffer[j++] = test4f (v);
      buffer[j++] = test4f (-v);
      j %= blen;
    }
  volatile_accu += sum;
  g_timer_stop (timer);
  float test4_time = g_timer_elapsed (timer, NULL);

  sum = j = 0;
  memset ((void*) buffer, 0, sizeof (buffer));
  g_timer_start (timer);
  for (float i = 0; i <= n; i += 1)
    {
      float v = max_sub * i / n;
      buffer[j++] = test5f (v);
      buffer[j++] = test5f (-v);
      j %= blen;
    }
  volatile_accu += sum;
  g_timer_stop (timer);
  float test5_time = g_timer_elapsed (timer, NULL);

  sum = j = 0;
  memset ((void*) buffer, 0, sizeof (buffer));
  g_timer_start (timer);
  for (float i = 0; i <= n; i += 1)
    {
      float v = max_sub * i / n;
      float f1 = v, f2 = -v;
      BSE_FLOAT_FLUSH (f1);
      buffer[j++] = f1;
      BSE_FLOAT_FLUSH (f2);
      buffer[j++] = f2;
      j %= blen;
    }
  volatile_accu += sum;
  g_timer_stop (timer);
  float test_bse_time = g_timer_elapsed (timer, NULL);

  g_print ("subnormal cancellation times: keep=%fs zap=%fs inlined-cond=%fs if-cond=%fs arithmetic=%f bse=%f\n",
           test1_time, test2_time, test3_time, test4_time, test5_time, test_bse_time);

  return 0;
}
