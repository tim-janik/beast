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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "testutils.h"
#include <birnet/birnetthread.h>

static volatile guint atomic_count = 0;
static BirnetMutex    atomic_mutex;
static BirnetCond     atomic_cond;

static void
atomic_up_thread (gpointer data)
{
  volatile int *ip = (int*) data;
  for (guint i = 0; i < 25; i++)
    birnet_atomic_int_add (ip, +3);
  birnet_mutex_lock (&atomic_mutex);
  atomic_count -= 1;
  birnet_cond_signal (&atomic_cond);
  birnet_mutex_unlock (&atomic_mutex);
}

static void
atomic_down_thread (gpointer data)
{
  volatile int *ip = (int*) data;
  for (guint i = 0; i < 25; i++)
    birnet_atomic_int_add (ip, -4);
  birnet_mutex_lock (&atomic_mutex);
  atomic_count -= 1;
  birnet_cond_signal (&atomic_cond);
  birnet_mutex_unlock (&atomic_mutex);
}

static void
test_atomic (void)
{
  TMSG ("AtomicThreading:");
  int count = 60;
  BirnetThread *threads[count];
  volatile int atomic_counter = 0;
  birnet_mutex_init (&atomic_mutex);
  birnet_cond_init (&atomic_cond);
  atomic_count = count;
  for (int i = 0; i < count; i++)
    {
      threads[i] = birnet_thread_run (NULL, (i&1) ? atomic_up_thread : atomic_down_thread, (void*) &atomic_counter);
      TASSERT (threads[i]);
    }
  birnet_mutex_lock (&atomic_mutex);
  while (atomic_count > 0)
    {
      TACK();
      birnet_cond_wait (&atomic_cond, &atomic_mutex);
    }
  birnet_mutex_unlock (&atomic_mutex);
  int result = count / 2 * 25 * +3 + count / 2 * 25 * -4;
  // g_printerr ("{ %d ?= %d }", atomic_counter, result);
  TASSERT (atomic_counter == result);
  TDONE ();
}

static void
plus1_thread (gpointer data)
{
  guint *tdata = (guint*) data;
  birnet_thread_sleep (-1);
  *tdata += 1;
  while (!birnet_thread_aborted ())
    birnet_thread_sleep (-1);
}

static void
test_threads (void)
{
  static BirnetMutex test_mutex;
  gboolean locked;
  TMSG ("Threading:");
  birnet_mutex_init (&test_mutex);
  locked = birnet_mutex_trylock (&test_mutex);
  TASSERT (locked);
  locked = birnet_mutex_trylock (&test_mutex);
  TASSERT (!locked);
  birnet_mutex_unlock (&test_mutex);
  birnet_mutex_destroy (&test_mutex);
  guint thread_data1 = 0;
  BirnetThread *thread1 = birnet_thread_run (NULL, plus1_thread, &thread_data1);
  guint thread_data2 = 0;
  BirnetThread *thread2 = birnet_thread_run (NULL, plus1_thread, &thread_data2);
  guint thread_data3 = 0;
  BirnetThread *thread3 = birnet_thread_run (NULL, plus1_thread, &thread_data3);
  TASSERT (thread1 != NULL);
  TASSERT (thread2 != NULL);
  TASSERT (thread3 != NULL);
  TASSERT (thread_data1 == 0);
  TASSERT (thread_data2 == 0);
  TASSERT (thread_data3 == 0);
  birnet_thread_wakeup (thread1);
  birnet_thread_wakeup (thread2);
  birnet_thread_wakeup (thread3);
  birnet_thread_abort (thread1);
  birnet_thread_abort (thread2);
  birnet_thread_abort (thread3);
  TASSERT (thread_data1 > 0);
  TASSERT (thread_data2 > 0);
  TASSERT (thread_data3 > 0);
  TDONE ();
}

int
main (int   argc,
      char *argv[])
{
  birnet_init (argv[0]);
  g_log_set_always_fatal ((GLogLevelFlags) (g_log_set_always_fatal ((GLogLevelFlags) G_LOG_FATAL_MASK) | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL));

  test_threads();
  test_atomic ();

  return 0;
}

/* vim:set ts=8 sts=2 sw=2: */
