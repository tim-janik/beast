/* BirnetThread
 * Copyright (C) 2002-2006 Tim Janik
 * Copyright (C) 2002 Stefan Westerfeld
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
#define _GNU_SOURCE     /* syscall() */
#include "birnetconfig.h"
#if	(BIRNET_HAVE_MUTEXATTR_SETTYPE > 0)
#define	_XOPEN_SOURCE   600	/* for full pthread facilities */
#endif	/* defining _XOPEN_SOURCE on random systems can have bad effects */
#include "birnetthread.h"
#include "birnetring.h"
#include <sys/time.h>
#include <sched.h>
#include <unistd.h>     /* sched_yield() */
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/times.h>

#define HAVE_GSLICE     (GLIB_MAJOR_VERSION >= 2 && GLIB_MINOR_VERSION >= 9)

#define FLOATING_FLAG                           (1 << 31)
#define THREAD_REF_COUNT(thread)                (thread->ref_field & ~FLOATING_FLAG)
#define THREAD_CAS(thread, oldval, newval)      g_atomic_int_compare_and_exchange ((int*) &thread->ref_field, oldval, newval)

/* --- structures --- */
struct _BirnetThread
{
  volatile gpointer      threadxx;
  volatile uint32        ref_field;
  gchar		        *name;
  gint8		         aborted;
  gint8		         got_wakeup;
  gint8                  accounting;
  volatile void*         guard_cache;
  BirnetCond	         wakeup_cond;
  BirnetThreadWakeup     wakeup_func;
  gpointer	         wakeup_data;
  GDestroyNotify         wakeup_destroy;
  guint64	         awake_stamp;
  gint                   tid;
  GData		        *qdata;
  /* accounting */
  struct {
    struct timeval stamp;
    gint64         utime, stime;
    gint64         cutime, cstime;
  }                ac;
  struct {
    guint          processor;
    gint           priority;
    BirnetThreadState state;
    gint           utime, stime;
    gint           cutime, cstime;
  }                info;
};

/* --- prototypes --- */
static void             birnet_guard_deregister_all     (BirnetThread *thread);
static void	        birnet_thread_handle_exit	(BirnetThread *thread);
static void             birnet_thread_accounting_L      (BirnetThread *self,
                                                         bool          force_update);
static void             thread_get_tid                  (BirnetThread *thread);
static inline guint     cached_getpid                   (void);


/* --- variables --- */
static BirnetMutex global_thread_mutex = { 0, };
static BirnetMutex global_startup_mutex = { 0, };
static BirnetCond  global_thread_cond = { 0, };
static BirnetRing *global_thread_list = NULL;
static BirnetRing *thread_awaken_list = NULL;
BirnetThreadTable  birnet_thread_table = { NULL, };


/* --- functions --- */
BirnetThread*
birnet_thread_new (const gchar *name)
{
  g_return_val_if_fail (name && name[0], NULL);
  BirnetThread *thread;
#if HAVE_GSLICE
  thread = g_slice_new0 (BirnetThread);
#else
  thread = g_new0 (BirnetThread, 1);
#endif

  g_atomic_pointer_set (&thread->threadxx, NULL);
  thread->ref_field = FLOATING_FLAG + 1;
  thread->name = g_strdup (name);
  thread->aborted = FALSE;
  thread->got_wakeup = FALSE;
  thread->accounting = 0;
  thread->guard_cache = NULL;
  birnet_cond_init (&thread->wakeup_cond);
  thread->wakeup_func = NULL;
  thread->wakeup_destroy = NULL;
  thread->awake_stamp = 0;
  thread->tid = -1;
  g_datalist_init (&thread->qdata);
  return thread;
}

BirnetThread*
birnet_thread_ref (BirnetThread *thread)
{
  g_return_val_if_fail (thread != NULL, NULL);
  BIRNET_ASSERT (THREAD_REF_COUNT (thread) > 0);
  uint32 old_ref, new_ref;
  do {
    old_ref = g_atomic_int_get (&thread->ref_field);
    new_ref = old_ref + 1;
    BIRNET_ASSERT (new_ref & ~FLOATING_FLAG); /* catch overflow */
  } while (!THREAD_CAS (thread, old_ref, new_ref));
  return thread;
}

BirnetThread*
birnet_thread_ref_sink (BirnetThread *thread)
{
  g_return_val_if_fail (thread != NULL, NULL);
  BIRNET_ASSERT (THREAD_REF_COUNT (thread) > 0);
  birnet_thread_ref (thread);
  uint32 old_ref, new_ref;
  do {
    old_ref = g_atomic_int_get (&thread->ref_field);
    new_ref = old_ref & ~FLOATING_FLAG;
  } while (!THREAD_CAS (thread, old_ref, new_ref));
  if (old_ref & FLOATING_FLAG)
    birnet_thread_unref (thread);
  return thread;
}

void
birnet_thread_unref (BirnetThread *thread)
{
  BIRNET_ASSERT (THREAD_REF_COUNT (thread) > 0);
  uint32 old_ref, new_ref;
  do {
    old_ref = g_atomic_int_get (&thread->ref_field);
    BIRNET_ASSERT (old_ref & ~FLOATING_FLAG); /* catch underflow */
    new_ref = old_ref - 1;
  } while (!THREAD_CAS (thread, old_ref, new_ref));
  if (0 == (new_ref & ~FLOATING_FLAG))
    {
      g_datalist_clear (&thread->qdata);
      void *threadcxx = g_atomic_pointer_get (&thread->threadxx);
      while (threadcxx)
        {
          _birnet_thread_cxx_delete (threadcxx);
          g_datalist_clear (&thread->qdata);
          threadcxx = g_atomic_pointer_get (&thread->threadxx);
        }
      birnet_cond_destroy (&thread->wakeup_cond);
      g_free (thread->name);
      thread->name = NULL;
#if HAVE_GSLICE
      g_slice_free (BirnetThread, thread);
#else
      g_free (thread);
#endif
    }
}

static void
filter_priority_warning (const gchar    *log_domain,
                         GLogLevelFlags  log_level,
                         const gchar    *message,
                         gpointer        unused_data)
{
  static const char *fmsg = "Priorities can only be increased by root.";
  if (message && strcmp (message, fmsg) == 0)
    ;   /* ignore warning */
  else
    g_log_default_handler (log_domain, log_level, message, unused_data);
}

static gpointer
birnet_thread_exec (gpointer data)
{
  void           **tfdx      = data;
  BirnetThread    *thread    = tfdx[0];
  BirnetThreadFunc func      = tfdx[1];
  gpointer         user_data = tfdx[2];
  birnet_thread_table.thread_set_handle (thread);
  
  BirnetThread *self = birnet_thread_self ();
  g_assert (self == thread);
  
  thread_get_tid (thread);

  birnet_thread_ref (thread);
  
  birnet_mutex_lock (&global_thread_mutex);
  global_thread_list = birnet_ring_append (global_thread_list, self);
  self->accounting = 1;
  birnet_thread_accounting_L (self, TRUE);
  birnet_cond_broadcast (&global_thread_cond);
  birnet_mutex_unlock (&global_thread_mutex);
  /* here, tfdx contents have become invalid */

  birnet_mutex_lock (&global_startup_mutex);
  /* acquiring this mutex waits for birnet_thread_run() to figure inlist (global_thread_list, self) */
  birnet_mutex_unlock (&global_startup_mutex);
  
  func (user_data);
  g_datalist_clear (&thread->qdata);

  /* because func() can be prematurely exited via pthread_exit(),
   * birnet_thread_handle_exit() does unref and final destruction
   */
  return NULL;
}

static void
birnet_thread_handle_exit (BirnetThread *thread)
{
  /* run custom data cleanup handlers */
  g_datalist_clear (&thread->qdata);
  /* cleanup wakeup hook */
  thread->wakeup_func = NULL;
  while (thread->wakeup_destroy)
    {
      GDestroyNotify wakeup_destroy = thread->wakeup_destroy;
      thread->wakeup_destroy = NULL;
      wakeup_destroy (thread->wakeup_data);
    }
  /* cleanup custom data from destruction phase */
  g_datalist_clear (&thread->qdata);

  /* regular cleanup code, all custom hooks have been processed now */
  birnet_guard_deregister_all (thread);
  birnet_mutex_lock (&global_thread_mutex);
  global_thread_list = birnet_ring_remove (global_thread_list, thread);
  if (thread->awake_stamp)
    thread_awaken_list = birnet_ring_remove (thread_awaken_list, thread);
  birnet_cond_broadcast (&global_thread_cond);
  birnet_mutex_unlock (&global_thread_mutex);
  /* free thread structure */
  birnet_thread_unref (thread);
}

/**
 * @param thread        a valid, unstarted BirnetThread
 * @param func	        function to execute in new thread
 * @param user_data     user data to pass into @a func
 * @param returns       FALSE in case of error
 *
 * Create a new thread running @a func.
 */
bool
birnet_thread_start (BirnetThread    *thread,
                     BirnetThreadFunc func,
                     gpointer         user_data)
{
  GThread *gthread = NULL;
  GError *gerror = NULL;
  
  g_return_val_if_fail (thread != NULL, FALSE);
  g_return_val_if_fail (thread->tid == -1, FALSE);
  g_return_val_if_fail (func != NULL, FALSE);
  
  birnet_thread_ref (thread);

  /* silence those stupid priority warnings triggered by glib */
  guint hid = g_log_set_handler ("GLib", G_LOG_LEVEL_WARNING, filter_priority_warning, NULL);

  /* thread creation context, protection by global_startup_mutex */
  birnet_mutex_lock (&global_startup_mutex);
  void **tfdx = g_new0 (void*, 4);
  tfdx[0] = thread;
  tfdx[1] = func;
  tfdx[2] = user_data;
  tfdx[3] = NULL;
  
  thread->tid = cached_getpid();

  /* don't dare setting joinable to TRUE, that prevents the thread's
   * resources from being freed, since we don't offer pthread_join().
   * so we'd just run out of stack at some point.
   */
  const gboolean joinable = FALSE;
  gthread = g_thread_create_full (birnet_thread_exec, tfdx, 0, joinable, FALSE,
                                  G_THREAD_PRIORITY_NORMAL, &gerror);
  if (gthread)
    {
      birnet_mutex_lock (&global_thread_mutex);
      while (!birnet_ring_find (global_thread_list, thread))
	birnet_cond_wait (&global_thread_cond, &global_thread_mutex);
      birnet_mutex_unlock (&global_thread_mutex);
    }
  else
    {
      thread->tid = -1;
      g_message ("failed to create thread \"%s\": %s", thread->name, gerror->message);
      g_error_free (gerror);
    }

  /* let the new thread actually start out */
  birnet_mutex_unlock (&global_startup_mutex);

  /* withdraw warning filter */
  g_free (tfdx);
  g_log_remove_handler ("GLib", hid);

  birnet_thread_unref (thread);

  return gthread != NULL;
}

/**
 * @param name	     thread name
 * @param func	     function to execute in new thread
 * @param user_data  user data to pass into @a func
 * @param returns    new thread handle or NULL in case of error
 *
 * Create a new thread running @a func.
 */
BirnetThread*
birnet_thread_run (const gchar     *name,
                   BirnetThreadFunc func,
                   gpointer         user_data)
{
  g_return_val_if_fail (name && name[0], NULL);

  BirnetThread *thread = birnet_thread_new (name);
  birnet_thread_ref_sink (thread);
  if (birnet_thread_start (thread, func, user_data))
    return thread;
  else
    {
      birnet_thread_unref (thread);
      return NULL;
    }
}

/**
 * Volountarily give up the curren scheduler time slice and let
 * another process or thread run, if any is in the queue.
 * The effect of this funciton is highly system dependent and
 * may simply result in the current thread being continued.
 */
void
birnet_thread_yield (void)
{
#ifdef  _POSIX_PRIORITY_SCHEDULING
  sched_yield();
#else
  g_thread_yield();
#endif
}

/**
 * @return		thread handle
 *
 * Return the thread handle of the currently running thread.
 */
BirnetThread*
birnet_thread_self (void)
{
  BirnetThread *thread = birnet_thread_table.thread_get_handle ();
  if G_UNLIKELY (!thread)
    {
      /* this function is also used during thread initialization,
       * so not all library components are yet usable
       */
      static volatile int anon_count = 1;
      guint id = birnet_atomic_int_swap_and_add (&anon_count, 1);
      gchar name[256];
      g_snprintf (name, 256, "Anon%u", id);
      thread = birnet_thread_new (name);
      birnet_thread_ref_sink (thread);
      thread_get_tid (thread);
      birnet_thread_table.thread_set_handle (thread);
      birnet_mutex_lock (&global_thread_mutex);
      global_thread_list = birnet_ring_append (global_thread_list, thread);
      birnet_mutex_unlock (&global_thread_mutex);
    }
  return thread;
}

void*
_birnet_thread_get_cxx (BirnetThread *thread)
{
  void *ptr = g_atomic_pointer_get (&thread->threadxx);
  if (G_UNLIKELY (!ptr))
    {
      _birnet_thread_cxx_wrap (thread);
      ptr = g_atomic_pointer_get (&thread->threadxx);
    }
  return ptr;
}

void*
_birnet_thread_self_cxx (void)
{
  BirnetThread *thread = birnet_thread_table.thread_get_handle ();
  if (G_UNLIKELY (!thread))
    thread = birnet_thread_self();
  return _birnet_thread_get_cxx (thread);
}

bool
_birnet_thread_set_cxx (BirnetThread *thread,
                        void         *xxdata)
{
  birnet_mutex_lock (&global_thread_mutex);
  bool success = false;
  if (!g_atomic_pointer_get (&thread->threadxx) || !xxdata)
    {
      g_atomic_pointer_set (&thread->threadxx, xxdata);
      success = true;
    }
  else
    g_error ("attempt to exchange C++ thread handle");
  birnet_mutex_unlock (&global_thread_mutex);
  return success;
}

void
birnet_thread_set_name (const gchar *name)
{
  BirnetThread *thread = birnet_thread_self ();
  if (name)
    {
      birnet_mutex_lock (&global_thread_mutex);
      g_free (thread->name);
      thread->name = g_strdup (name);
      birnet_mutex_unlock (&global_thread_mutex);
    }
}

/**
 * @return		thread id
 *
 * Return the thread specific id. This function is highly
 * system dependant. The thread id may deviate from the overall
 * process id or not. On linux, threads have their own id,
 * allthough since kernel 2.6, they share the same process id.
 */
gint
birnet_thread_self_pid (void)
{
  return birnet_thread_self ()->tid;
}

/**
 * @param thread	a valid BirnetThread handle
 * @return		thread id
 *
 * Return the specific id for @a thread. This function is highly
 * system dependant. The thread id may deviate from the overall
 * process id or not. On linux, threads have their own id,
 * allthough since kernel 2.6, they share the same process id.
 */
gint
birnet_thread_get_pid (BirnetThread *thread)
{
  thread = thread ? thread : birnet_thread_self();
  return thread->tid;
}

/**
 * @param thread	a valid BirnetThread handle
 * @return		thread name
 *
 * Return the name of @a thread as specified upon invokation of
 * birnet_thread_run() or assigned by birnet_thread_set_name().
 */
const gchar*
birnet_thread_get_name (BirnetThread *thread)
{
  thread = thread ? thread : birnet_thread_self();
  return thread->name;
}

static void
birnet_thread_wakeup_L (BirnetThread *thread)
{
  birnet_cond_signal (&thread->wakeup_cond);
  if (thread->wakeup_func)
    thread->wakeup_func (thread->wakeup_data);
  thread->got_wakeup = TRUE;
}

/**
 * @param max_useconds	maximum amount of micro seconds to sleep (-1 for infinite time)
 * @param returns	TRUE while the thread should continue execution
 *
 * Sleep for the amount of time given.
 * This function may get interrupted by wakeup requests from
 * birnet_thread_wakeup(), abort requests from birnet_thread_queue_abort()
 * or other means. It returns whether the thread is supposed to
 * continue execution after waking up.
 * This function or alternatively birnet_thread_aborted() should be called
 * periodically, to react to thread abortion requests and to update
 * internal accounting information.
 */
bool
birnet_thread_sleep (BirnetInt64 max_useconds)
{
  BirnetThread *self = birnet_thread_self ();
  bool aborted;
  
  birnet_mutex_lock (&global_thread_mutex);
  
  birnet_thread_accounting_L (self, FALSE);
  
  if (!self->got_wakeup && max_useconds != 0)
    {
      if (max_useconds >= 0) /* wait once without time adjustments */
	birnet_cond_wait_timed (&self->wakeup_cond, &global_thread_mutex, max_useconds);
      else /* wait forever */
	while (!self->got_wakeup)
	  birnet_cond_wait (&self->wakeup_cond, &global_thread_mutex);
    }
  
  self->got_wakeup = FALSE;
  aborted = self->aborted != FALSE;
  birnet_mutex_unlock (&global_thread_mutex);
  
  return !aborted;
}

/**
 * @param wakeup_func	wakeup function to be called by birnet_thread_wakeup()
 * @param wakeup_data	data passed into wakeup_func()
 * @param destroy	destroy handler for @a wakeup_data
 *
 * Set the wakeup function for the current thread. This enables
 * the thread to be woken up through birnet_thread_wakeup() even
 * if not sleeping in birnet_thread_sleep(). The wakeup function
 * must be thread-safe, so it may be called from any thread,
 * and it should be fast, because the global thread system lock
 * is held during its invokation.
 * Per thread, the wakeup function may be set only once.
 */
void
birnet_thread_set_wakeup (BirnetThreadWakeup wakeup_func,
                          gpointer           wakeup_data,
                          GDestroyNotify     destroy)
{
  BirnetThread *self = birnet_thread_self ();
  
  g_return_if_fail (wakeup_func != NULL);
  g_return_if_fail (self->wakeup_func == NULL);
  
  birnet_mutex_lock (&global_thread_mutex);
  self->wakeup_func = wakeup_func;
  self->wakeup_data = wakeup_data;
  self->wakeup_destroy = destroy;
  birnet_mutex_unlock (&global_thread_mutex);
}

/**
 * @param thread	thread to wake up
 *
 * Wake up a currently sleeping thread. In practice, this
 * function simply causes the next call to birnet_thread_sleep()
 * within @a thread to last for 0 seconds.
 */
void
birnet_thread_wakeup (BirnetThread *thread)
{
  g_return_if_fail (thread != NULL);
  
  birnet_mutex_lock (&global_thread_mutex);
  g_assert (birnet_ring_find (global_thread_list, thread));
  birnet_thread_wakeup_L (thread);
  birnet_mutex_unlock (&global_thread_mutex);
}

/**
 * @param stamp	stamp to trigger wakeup
 *
 * Wake the current thread up at the next invocation
 * of birnet_thread_emit_wakeups() with a wakup_stamp
 * greater than @a stamp.
 */
void
birnet_thread_awake_after (guint64 stamp)
{
  BirnetThread *self = birnet_thread_self ();
  
  g_return_if_fail (stamp > 0);
  
  birnet_mutex_lock (&global_thread_mutex);
  if (!self->awake_stamp)
    {
      thread_awaken_list = birnet_ring_prepend (thread_awaken_list, self);
      self->awake_stamp = stamp;
    }
  else
    self->awake_stamp = MIN (self->awake_stamp, stamp);
  birnet_mutex_unlock (&global_thread_mutex);
}

/**
 * @param wakeup_stamp	wakeup stamp to trigger wakeups
 *
 * Wake all currently sleeping threads up which queued
 * a wakeup through birnet_thread_awake_after() with a
 * stamp smaller than @a wakeup_stamp.
 */
void
birnet_thread_emit_wakeups (guint64 wakeup_stamp)
{
  BirnetRing *ring, *next;
  
  g_return_if_fail (wakeup_stamp > 0);
  
  birnet_mutex_lock (&global_thread_mutex);
  for (ring = thread_awaken_list; ring; ring = next)
    {
      BirnetThread *thread = ring->data;
      next = birnet_ring_walk (ring, thread_awaken_list);
      if (thread->awake_stamp <= wakeup_stamp)
	{
	  thread->awake_stamp = 0;
	  thread_awaken_list = birnet_ring_remove (thread_awaken_list, thread);
	  birnet_thread_wakeup_L (thread);
	}
    }
  birnet_mutex_unlock (&global_thread_mutex);
}

/**
 * @param thread	thread to abort
 *
 * Abort a currently running thread. This function does not
 * return until the thread in question terminated execution.
 * Note that the thread handle gets invalidated with invocation
 * of birnet_thread_abort() or birnet_thread_queue_abort().
 */
void
birnet_thread_abort (BirnetThread *thread)
{
  g_return_if_fail (thread != NULL);
  g_return_if_fail (thread != birnet_thread_self ());
  
  birnet_mutex_lock (&global_thread_mutex);
  g_assert (birnet_ring_find (global_thread_list, thread));
  thread->aborted = TRUE;
  birnet_thread_wakeup_L (thread);
  while (birnet_ring_find (global_thread_list, thread))
    birnet_cond_wait (&global_thread_cond, &global_thread_mutex);
  birnet_mutex_unlock (&global_thread_mutex);
}

/**
 * @param thread	thread to abort
 *
 * Same as birnet_thread_abort(), but returns as soon as possible,
 * even if thread hasn't stopped execution yet.
 * Note that the thread handle gets invalidated with invocation
 * of birnet_thread_abort() or birnet_thread_queue_abort().
 */
void
birnet_thread_queue_abort (BirnetThread *thread)
{
  g_return_if_fail (thread != NULL);
  
  birnet_mutex_lock (&global_thread_mutex);
  g_assert (birnet_ring_find (global_thread_list, thread));
  thread->aborted = TRUE;
  birnet_thread_wakeup_L (thread);
  birnet_mutex_unlock (&global_thread_mutex);
}

/**
 * @param returns	TRUE if the thread should abort execution
 *
 * Find out if the currently running thread should be aborted (the thread is
 * supposed to return from its main thread function). This function or
 * alternatively birnet_thread_sleep() should be called periodically, to
 * react to thread abortion requests and to update internal accounting
 * information.
 */
bool
birnet_thread_aborted (void)
{
  BirnetThread *self = birnet_thread_self ();
  birnet_mutex_lock (&global_thread_mutex);
  birnet_thread_accounting_L (self, FALSE);
  bool aborted = self->aborted != FALSE;
  birnet_mutex_unlock (&global_thread_mutex);
  return aborted;
}

/**
 * @param thread	thread to abort
 * @param returns	TRUE if the thread should abort execution
 *
 * Find out if the currently running thread should be aborted (the thread is
 * supposed to return from its main thread function). This function or
 * alternatively birnet_thread_sleep() should be called periodically, to
 * react to thread abortion requests and to update internal accounting
 * information.
 */
bool
birnet_thread_get_aborted (BirnetThread *thread)
{
  birnet_mutex_lock (&global_thread_mutex);
  bool aborted = thread->aborted != FALSE;
  birnet_mutex_unlock (&global_thread_mutex);
  return aborted;
}

bool
birnet_thread_get_running (BirnetThread *thread)
{
  birnet_mutex_lock (&global_thread_mutex);
  bool running = birnet_ring_find (global_thread_list, thread);
  birnet_mutex_unlock (&global_thread_mutex);
  return running;
}

void
birnet_thread_wait_for_exit (BirnetThread *thread)
{
  birnet_mutex_lock (&global_thread_mutex);
  while (birnet_ring_find (global_thread_list, thread))
    birnet_cond_wait (&global_thread_cond, &global_thread_mutex);
  birnet_mutex_unlock (&global_thread_mutex);
}

gpointer
birnet_thread_get_qdata (GQuark quark)
{
  BirnetThread *self = birnet_thread_self ();
  return quark ? g_datalist_id_get_data (&self->qdata, quark) : NULL;
}

void
birnet_thread_set_qdata_full (GQuark         quark,
                              gpointer       data,
                              GDestroyNotify destroy)
{
  BirnetThread *self = birnet_thread_self ();
  g_return_if_fail (quark > 0);
  g_datalist_id_set_data_full (&self->qdata, quark, data,
			       data ? destroy : (GDestroyNotify) NULL);
}

gpointer
birnet_thread_steal_qdata (GQuark quark)
{
  BirnetThread *self = birnet_thread_self ();
  return quark ? g_datalist_id_remove_no_notify (&self->qdata, quark) : NULL;
}

void
birnet_cond_wait_timed (BirnetCond  *cond,
                        BirnetMutex *mutex,
                        BirnetInt64  max_useconds)
{
  if (max_useconds < 0)
    birnet_cond_wait (cond, mutex);
  else if (max_useconds > 0)
    {
      struct timeval now, limit;
      gettimeofday (&now, NULL);
      BirnetInt64 secs = max_useconds / 1000000;
      limit.tv_sec = now.tv_sec + secs;
      max_useconds -= secs * 1000000;
      limit.tv_usec = now.tv_usec + max_useconds;
      if (limit.tv_usec >= 1000000)
        {
          limit.tv_usec -= 1000000;
          limit.tv_sec += 1;
        }
      birnet_thread_table.cond_wait_timed (cond, mutex, limit.tv_sec, limit.tv_usec);
    }
}

static inline guint
cached_getpid (void)
{
  static pid_t cached_pid = 0;
  if (G_UNLIKELY (!cached_pid))
    cached_pid = getpid();
  return cached_pid;
}

#ifdef  __linux__
#include <sys/types.h>
#include <sys/syscall.h>
#include <linux/unistd.h>
#endif
static void
thread_get_tid (BirnetThread *thread)
{
  gint ppid = thread->tid;      /* creator process id */
  gint tid = -1;
  
#if     defined (__linux__) && defined (__NR_gettid)    /* present on linux >= 2.4.20 */
  tid = syscall (__NR_gettid);
#endif
  if (tid < 0)
    tid = cached_getpid();
  if (tid != ppid &&            /* thread pid different from creator pid, probably correct */
      tid > 0)
    thread->tid = tid;
  else
    thread->tid = 0;            /* failed to figure id */
}


/* --- thread info --- */
static inline unsigned long long int
timeval_usecs (const struct timeval *tv)
{
  return tv->tv_usec + tv->tv_sec * (guint64) 1000000;
}

static void
thread_info_from_stat_L (BirnetThread *self,
                         double     usec_norm)
{
  int pid = -1, ppid = -1, pgrp = -1, session = -1, tty_nr = -1, tpgid = -1;
  int exit_signal = 0, processor = 0;
  long cutime = 0, cstime = 0, priority = 0, nice = 0, dummyld = 0;
  long itrealvalue = 0, rss = 0;
  unsigned long flags = 0, minflt = 0, cminflt = 0, majflt = 0, cmajflt = 0;
  unsigned long utime = 0, stime = 0, vsize = 0, rlim = 0, startcode = 0;
  unsigned long endcode = 0, startstack = 0, kstkesp = 0, kstkeip = 0;
  unsigned long signal = 0, blocked = 0, sigignore = 0, sigcatch = 0;
  unsigned long wchan = 0, nswap = 0, cnswap = 0, rt_priority = 0, policy = 0;
  unsigned long long starttime = 0;
  char state = 0, command[8192 + 1] = { 0 };
  FILE *file = NULL;
  int n = 0;
  static int have_stat = 1;
  if (have_stat)
    {
      gchar *filename = g_strdup_printf ("/proc/%u/task/%u/stat", cached_getpid(), self->tid);
      file = fopen (filename, "r");
      g_free (filename);
      if (!file)
        have_stat = 0;  /* reading /proc/self/stat should always succeed, so try only once */
    }
  if (file)
    n = fscanf (file,
                "%d %8192s %c "
                "%d %d %d %d %d "
                "%lu %lu %lu %lu %lu %lu %lu "
                "%ld %ld %ld %ld %ld %ld "
                "%llu %lu %ld "
                "%lu %lu %lu %lu %lu "
                "%lu %lu %lu %lu %lu "
                "%lu %lu %lu %d %d "
                "%lu %lu",
                &pid, command, &state, /* n=3 */
                &ppid, &pgrp, &session, &tty_nr, &tpgid, /* n=8 */
                &flags, &minflt, &cminflt, &majflt, &cmajflt, &utime, &stime, /* n=15 */
                &cutime, &cstime, &priority, &nice, &dummyld, &itrealvalue, /* n=21 */
                &starttime, &vsize, &rss, /* n=24 */
                &rlim, &startcode, &endcode, &startstack, &kstkesp, /* n=29 */
                &kstkeip, &signal, &blocked, &sigignore, &sigcatch, /* n=34 */
                &wchan, &nswap, &cnswap, &exit_signal, &processor, /* n=39 */
                &rt_priority, &policy /* n=41 */
                );
  if (file)
    fclose (file);

  if (n >= 15)
    {
      self->ac.utime = utime * 10000;
      self->ac.stime = stime * 10000;
    }
  if (n >= 17)
    {
      self->ac.cutime = cutime * 10000;
      self->ac.cstime = cstime * 10000;
    }
  if (n >= 3)
    self->info.state = state;
  if (n >= 39)
    self->info.processor = 1 + processor;
}

#define ACCOUNTING_MSECS        (500)

static void
birnet_thread_accounting_L (BirnetThread *self,
                            bool          force_update)
{
  struct timeval stamp, ostamp = self->ac.stamp;
  guint diff = 0;
  if (self->accounting)
    {
      gettimeofday (&stamp, NULL);
      diff = timeval_usecs (&stamp) - timeval_usecs (&ostamp);
      diff = MAX (diff, 0);
    }
  if (force_update || diff >= ACCOUNTING_MSECS * 1000)  /* limit accounting to a few times per second */
    {
      gint64 old_utime = self->ac.utime;
      gint64 old_stime = self->ac.stime;
      gint64 old_cutime = self->ac.cutime;
      gint64 old_cstime = self->ac.cstime;
      gdouble dfact = 1000000.0 / MAX (diff, 1);
      self->ac.stamp = stamp;
      if (0)
        {
          struct rusage res = { { 0 } };
          getrusage (RUSAGE_SELF, &res);
          self->ac.utime = timeval_usecs (&res.ru_utime); /* user time used */
          self->ac.stime = timeval_usecs (&res.ru_stime); /* system time used */
          getrusage (RUSAGE_CHILDREN, &res);
          self->ac.cutime = timeval_usecs (&res.ru_utime);
          self->ac.cstime = timeval_usecs (&res.ru_stime);
        }
      thread_info_from_stat_L (self, dfact);
      self->info.priority = getpriority (PRIO_PROCESS, self->tid);
      self->info.utime = MAX (self->ac.utime - old_utime, 0) * dfact;
      self->info.stime = MAX (self->ac.stime - old_stime, 0) * dfact;
      self->info.cutime = MAX (self->ac.cutime - old_cutime, 0) * dfact;
      self->info.cstime = MAX (self->ac.cstime - old_cstime, 0) * dfact;
      self->accounting--;
    }
}

BirnetThreadInfo*
birnet_thread_info_collect (BirnetThread *thread)
{
  BirnetThreadInfo *info = g_new0 (BirnetThreadInfo, 1);
  struct timeval now;
  gboolean recent = TRUE;
  if (!thread)
    thread = birnet_thread_self ();
  gettimeofday (&now, NULL);
  birnet_mutex_lock (&global_thread_mutex);
  info->name = g_strdup (thread->name);
  info->aborted = thread->aborted;
  info->thread_id = thread->tid;
  if (timeval_usecs (&thread->ac.stamp) + ACCOUNTING_MSECS * 1000 < timeval_usecs (&now))
    recent = FALSE;             /* accounting data too old */
  info->state = thread->info.state;
  info->priority = thread->info.priority;
  info->processor = thread->info.processor;
  if (recent)
    {
      info->utime = thread->info.utime;
      info->stime = thread->info.stime;
      info->cutime = thread->info.cutime;
      info->cstime = thread->info.cstime;
    }
  thread->accounting = 5;       /* update accounting info in the future */
  birnet_mutex_unlock (&global_thread_mutex);
  return info;
}

void
birnet_thread_info_free (BirnetThreadInfo  *info)
{
  g_return_if_fail (info != NULL);
  g_free (info->name);
  g_free (info);
}


/* --- hazard pointer guards --- */
struct BirnetGuard
{
  volatile BirnetGuard *next;       /* global guard list */
  BirnetThread         *thread;
  volatile BirnetGuard *cache_next; /* per thread free list */
  guint                 n_values;
  volatile gpointer     values[1];  /* variable length array */
};
static volatile BirnetGuard * volatile guard_list = NULL;
static gint       volatile guard_list_length = 0;
#define BIRNET_GUARD_ALIGN  (4)
#define guard2values(ptr)       G_STRUCT_MEMBER_P (ptr, +G_STRUCT_OFFSET (BirnetGuard, values[0]))
#define values2guard(ptr)       G_STRUCT_MEMBER_P (ptr, -G_STRUCT_OFFSET (BirnetGuard, values[0]))

/**
 * @param n_hazards	number of required hazard pointers
 * @return		a valid BirnetGuard
 *
 * Retrieve a new guard for node protection of the current thread.
 * The exact mechanism of protection is described in birnet_guard_protect().
 * Note that birnet_guard_snap_values() will walk the hazard pointer
 * array in ascending order, so that pointers may migrate from array
 * positions with a lower index to positions with a higher index while
 * retaining protection, according to condition C2 as described in
 * http://www.research.ibm.com/people/m/michael/podc-2002.pdf.
 * If an equally or bigger sized hazard pointer array was previously
 * deregistered by this thread, registration takes constant time.
 */
volatile BirnetGuard*
birnet_guard_register (guint n_hazards)
{
  BirnetThread *thread = birnet_thread_self();
  volatile BirnetGuard *guard, *last = NULL;
  /* reuse cached guards */
  for (guard = thread->guard_cache; guard; last = guard, guard = last->cache_next)
    if (n_hazards <= guard->n_values)
      {
        if (last)
          last->cache_next = guard->cache_next;
        else
          thread->guard_cache = guard->cache_next;
        guard->cache_next = NULL;
        break;
      }
  /* allocate new guard */
  if (!guard)
    {
      n_hazards = ((MAX (n_hazards, 3) + BIRNET_GUARD_ALIGN - 1) / BIRNET_GUARD_ALIGN) * BIRNET_GUARD_ALIGN;
      g_atomic_int_add ((gint*) &guard_list_length, n_hazards);
      guard = g_malloc0 (sizeof (BirnetGuard) + (n_hazards - 1) * sizeof (guard->values[0]));
      guard->n_values = n_hazards;
      guard->thread = thread;
      do
        guard->next = g_atomic_pointer_get (&guard_list);
      while (!g_atomic_pointer_compare_and_exchange ((gpointer) &guard_list, (gpointer) guard->next, (gpointer) guard));
    }
  return guard2values (guard);
}

/**
 * @param guard	a valid BirnetGuard as returned from birnet_guard_register()
 *
 * Deregister a guard previously registered by a call to birnet_guard_register().
 * Deregistration is performed in constant time.
 */
void
birnet_guard_deregister (volatile BirnetGuard *guard)
{
  guard = values2guard (guard);
  BirnetThread *thread = birnet_thread_self();
  g_return_if_fail (guard->thread == thread);
  memset ((guint8*) guard->values, 0, sizeof (guard->values[0]) * guard->n_values);
  /* FIXME: must we have a memory barrier here? */
  guard->cache_next = thread->guard_cache;
  thread->guard_cache = guard;
}

static void
birnet_guard_deregister_all (BirnetThread *thread)
{
  volatile BirnetGuard *guard;
  thread->guard_cache = NULL;
  for (guard = g_atomic_pointer_get (&guard_list); guard; guard = guard->next)
    if (guard->thread == thread)
      {
        memset ((guint8*) guard->values, 0, sizeof (guard->values[0]) * guard->n_values);
        guard->cache_next = NULL;
        g_atomic_pointer_compare_and_exchange ((void*) &guard->thread, thread, NULL); /* reset ->thread with memory barrier */
      }
}

/**
 * @param guard	a valid BirnetGuard as returned from birnet_guard_register()
 * @param nth_hazard	index of the hazard pointer to use for protection
 * @param value	a hazardous pointer value or NULL to reset protection
 *
 * Protect the node pointed to by @a value from being destroyed by another
 * thread and against the ABA problem caused by premature reuse.
 * For this to work, threads destroying nodes of the type pointed to by
 * @a value need to suspend destruction as long as nodes are protected,
 * which can by checked by calls to birnet_guard_is_protected() or by
 * searching the values returned from birnet_guard_snap_values().
 * Descriptions of safe memory reclamation and ABA problem detection
 * via hazard pointers guards can be found in
 * http://www.research.ibm.com/people/m/michael/podc-2002.pdf,
 * http://www.cs.brown.edu/people/mph/HerlihyLM02/smli_tr-2002-112.pdf,
 * http://research.sun.com/scalable/Papers/CATS2003.pdf and
 * http://www.research.ibm.com/people/m/michael/ieeetpds-2004.pdf.
 * The exact sequence of steps to protect and access a node is as follows:
 * @* 1) Store the adress of a node to be protected in a hazard pointer
 * @* 2) Verify that the hazard pointer points to a valid node
 * @* 3) Dereference the node only as long as it's protected by the hazard pointer.
 * @* For example:
 * @* 0: BirnetGuard *guard = birnet_guard_register (1);
 * @* 1: peek_head_label:
 * @* 2: auto GSList *node = shared_list_head;
 * @* 3: birnet_guard_protect (guard, 0, node);
 * @* 4: if (node != shared_list_head) goto peek_head_label;
 * @* 5: operate_on_protected_node (node);
 * @* 6: birnet_guard_deregister (guard);
 */
#if 0
static inline
void birnet_guard_protect (volatile BirnetGuard *guard,  /* defined in birnetthreads.h */
                           guint     nth_hazard,
                           gpointer  value);
#endif

/**
 * @return		an upper bound on the number of registered hazard pointers
 *
 * Retrieve an upper bound on the number of hazard pointer value slots
 * currently required for a successfull call to birnet_guard_snap_values().
 * Note that a subsequent call to birnet_guard_snap_values() may still fail
 * due to addtional guards being registerted meanwhile. In such a case
 * birnet_guard_n_snap_values() and birnet_guard_snap_values() can simply be
 * called again.
 */
guint
birnet_guard_n_snap_values (void)
{
  return g_atomic_int_get ((gint*) &guard_list_length);
}

/**
 * @param n_values	location of n_values variable
 * @param values	value array to fill in
 * @return		TRUE if @a values provided enough space and is filled
 *
 * Make a snapshot of all non-NULL hazard pointer values.
 * TRUE is returned if the number of non-NULL hazard pointer
 * values didn't exceed the size of the input value array provided
 * by @a n_values, and all values are returned in the array pointed
 * to by @a values.
 * The number of values filled in is returned in @a n_values.
 * FALSE is returned if not enough space was available to return
 * all non-NULL values. birnet_guard_n_snap_values() may be used to
 * retrieve the current upper bound on the number of registered
 * guards. Note that a successive call to birnet_guard_snap_values() with
 * the requested number of value slots supplied may still fail,
 * because additional guards may have been registered meanwhile.
 * In such a case birnet_guard_n_snap_values() and birnet_guard_snap_values()
 * can simply be called again.
 * This funciton will always walk the hazard pointer arrays supplied
 * by birnet_guard_register() in ascending order, to allow pointer migration
 * from lower to higher array indieces while retaining protection.
 * The returned pointer values are unordered, so in order to perform
 * multiple pointer lookups, we recommend sorting the returned array
 * and then doing binary lookups. However if only a single pointer
 * is to be looked up, calling birnet_guard_is_protected() should be
 * considered.
 */
bool
birnet_guard_snap_values (guint          *n_values,
                          gpointer       *values)
{
  guint i, n = 0;
  volatile BirnetGuard *guard;
  for (guard = g_atomic_pointer_get (&guard_list); guard; guard = guard->next)
    if (guard->thread)
      for (i = 0; i < guard->n_values; i++)
        {
          gpointer v = guard->values[i];
          if (v)
            {
              n++;
              if (n > *n_values)
                return FALSE;   /* not enough space provided */
              *values++ = v;
            }
        }
  *n_values = n;                /* number of values used */
  return TRUE;
}

/**
 * @param value	hazard pointer value
 * @return		TRUE if a hazard pointer protecting @a value has been found
 *
 * Check whether @a value is protected by a hazard pointer guard.
 * If multiple pointer values are to be checked, use birnet_guard_snap_values()
 * instead, as this function has O(n_hazard_pointers) time complexity.
 * If only one pointer value needs to be looked up though,
 * calling birnet_guard_is_protected() will provide a result faster than
 * calling birnet_guard_snap_values() and looking up the pointer in the
 * filled-in array.
 * Lookup within hazard pointer arrays will always occour in ascending
 * order to allow pointer migration as described in birnet_guard_snap_values()
 * and birnet_guard_register().
 */
bool
birnet_guard_is_protected (gpointer value)
{
  if (value)
    {
      volatile BirnetGuard *guard;
      guint i;
      for (guard = g_atomic_pointer_get (&guard_list); guard; guard = guard->next)
        if (guard->thread)
          for (i = 0; i < guard->n_values; i++)
            if (guard->values[i] == value)
              return TRUE;
    }
  return FALSE;
}

/* --- fallback (GLib) BirnetThreadTable --- */
static GPrivate *fallback_thread_table_key = NULL;

static void
fallback_thread_set_handle (BirnetThread *handle)
{
  g_private_set (fallback_thread_table_key, handle);
}

static BirnetThread*
fallback_thread_get_handle (void)
{
  return g_private_get (fallback_thread_table_key);
}

static void
fallback_mutex_init (BirnetMutex *mutex)
{
  g_return_if_fail (mutex != NULL);
  mutex->mutex_pointer = g_mutex_new ();
}

static int
fallback_mutex_trylock (BirnetMutex *mutex)
{
  return g_mutex_trylock (mutex->mutex_pointer) ? 0 : -1;
}

static void
fallback_mutex_lock (BirnetMutex *mutex)
{
  static gboolean is_smp_system = FALSE; // FIXME
  
  /* spin locks should be held only very short times,
   * so usually, we should succeed here.
   */
  if (g_mutex_trylock (mutex->mutex_pointer))
    return;
  
  if (!is_smp_system)
    {
      /* on uni processor systems, there's no point in busy spinning */
      do
	{
	  birnet_thread_yield ();
	  if (g_mutex_trylock (mutex->mutex_pointer))
	    return;
	}
      while (TRUE);
    }
  else
    {
      /* for multi processor systems, mutex_lock() is hopefully implemented
       * via spinning. note that we can't implement spinning ourselves with
       * mutex_trylock(), since on some architectures that'd block memory
       * bandwith due to constant bus locks
       */
      g_mutex_lock (mutex->mutex_pointer);
    }
}

static void
fallback_mutex_unlock (BirnetMutex *mutex)
{
  g_mutex_unlock (mutex->mutex_pointer);
}

static void
fallback_mutex_destroy (BirnetMutex *mutex)
{
  g_mutex_free (mutex->mutex_pointer);
  memset (mutex, 0, sizeof (*mutex));
}

static void
fallback_rec_mutex_init (BirnetRecMutex *rec_mutex)
{
  rec_mutex->owner = NULL;
  birnet_mutex_init (&rec_mutex->mutex);
  rec_mutex->depth = 0;
}

static int
fallback_rec_mutex_trylock (BirnetRecMutex *rec_mutex)
{
  BirnetThread *self = birnet_thread_self ();
  
  if (rec_mutex->owner == self)
    {
      g_assert (rec_mutex->depth > 0);  /* paranoid */
      rec_mutex->depth += 1;
      return 0;
    }
  else
    {
      if (birnet_mutex_trylock (&rec_mutex->mutex))
	{
	  g_assert (rec_mutex->owner == NULL && rec_mutex->depth == 0); /* paranoid */
	  rec_mutex->owner = self;
	  rec_mutex->depth = 1;
	  return 0;
	}
    }
  return -1;
}

static void
fallback_rec_mutex_lock (BirnetRecMutex *rec_mutex)
{
  BirnetThread *self = birnet_thread_self ();
  
  if (rec_mutex->owner == self)
    {
      g_assert (rec_mutex->depth > 0);  /* paranoid */
      rec_mutex->depth += 1;
    }
  else
    {
      birnet_mutex_lock (&rec_mutex->mutex);
      g_assert (rec_mutex->owner == NULL && rec_mutex->depth == 0); /* paranoid */
      rec_mutex->owner = self;
      rec_mutex->depth = 1;
    }
}

static void
fallback_rec_mutex_unlock (BirnetRecMutex *rec_mutex)
{
  BirnetThread *self = birnet_thread_self ();
  
  if (rec_mutex->owner == self && rec_mutex->depth > 0)
    {
      rec_mutex->depth -= 1;
      if (!rec_mutex->depth)
	{
	  rec_mutex->owner = NULL;
	  birnet_mutex_unlock (&rec_mutex->mutex);
	}
    }
  else
    g_warning ("unable to unlock recursive mutex with self %p != %p or depth %u < 1",
	       rec_mutex->owner, self, rec_mutex->depth);
}

static void
fallback_rec_mutex_destroy (BirnetRecMutex *rec_mutex)
{
  if (rec_mutex->owner || rec_mutex->depth)
    g_warning ("recursive mutex still locked during destruction");
  else
    {
      birnet_mutex_destroy (&rec_mutex->mutex);
      g_assert (rec_mutex->owner == NULL && rec_mutex->depth == 0);
    }
}

static void
fallback_cond_init (BirnetCond *cond)
{
  cond->cond_pointer = g_cond_new ();
}

static void
fallback_cond_wait (BirnetCond  *cond,
                    BirnetMutex *mutex)
{
  /* infinite wait */
  g_cond_wait (cond->cond_pointer, mutex->mutex_pointer);
}

static void
fallback_cond_signal (BirnetCond *cond)
{
  g_cond_signal (cond->cond_pointer);
}

static void
fallback_cond_broadcast (BirnetCond *cond)
{
  g_cond_broadcast (cond->cond_pointer);
}

static void
fallback_cond_destroy (BirnetCond *cond)
{
  g_cond_free (cond->cond_pointer);
}

static void
fallback_cond_wait_timed (BirnetCond  *cond,
                          BirnetMutex *mutex,
                          BirnetUInt64 abs_secs,
                          BirnetUInt64 abs_usecs)
{
  GTimeVal gtime;
  gtime.tv_sec = abs_secs;
  gtime.tv_usec = abs_usecs;
  g_cond_timed_wait (cond->cond_pointer, mutex->mutex_pointer, &gtime);
}

static BirnetThreadTable fallback_thread_table = {
  fallback_thread_set_handle,
  fallback_thread_get_handle,
  fallback_mutex_init,
  fallback_mutex_lock,
  fallback_mutex_trylock,
  fallback_mutex_unlock,
  fallback_mutex_destroy,
  fallback_rec_mutex_init,
  fallback_rec_mutex_lock,
  fallback_rec_mutex_trylock,
  fallback_rec_mutex_unlock,
  fallback_rec_mutex_destroy,
  fallback_cond_init,
  fallback_cond_signal,
  fallback_cond_broadcast,
  fallback_cond_wait,
  fallback_cond_wait_timed,
  fallback_cond_destroy,
};

static BirnetThreadTable*
get_fallback_thread_table (void)
{
  fallback_thread_table_key = g_private_new ((GDestroyNotify) birnet_thread_handle_exit);
  return &fallback_thread_table;
}


/* --- POSIX threads table --- */
#if	(BIRNET_HAVE_MUTEXATTR_SETTYPE > 0)
#include <pthread.h>
static pthread_key_t pth_thread_table_key = 0;
static void
pth_thread_set_handle (BirnetThread *handle)
{
  BirnetThread *tmp = pthread_getspecific (pth_thread_table_key);
  pthread_setspecific (pth_thread_table_key, handle);
  if (tmp)
    birnet_thread_handle_exit (tmp);
}
static BirnetThread*
pth_thread_get_handle (void)
{
  return pthread_getspecific (pth_thread_table_key);
}
static void
pth_mutex_init (BirnetMutex *mutex)
{
  /* need NULL attribute here, which is the fast mutex in glibc
   * and cannot be chosen through pthread_mutexattr_settype()
   */
  pthread_mutex_init ((pthread_mutex_t*) mutex, NULL);
}
static void
pth_rec_mutex_init (BirnetRecMutex *mutex)
{
  pthread_mutexattr_t attr;
  pthread_mutexattr_init (&attr);
  pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init ((pthread_mutex_t*) mutex, &attr);
  pthread_mutexattr_destroy (&attr);
}
static void
pth_cond_init (BirnetCond *cond)
{
  pthread_cond_init ((pthread_cond_t*) cond, NULL);
}
static void
pth_cond_wait_timed (BirnetCond  *cond,
		     BirnetMutex *mutex,
		     BirnetUInt64 abs_secs,
		     BirnetUInt64 abs_usecs)
{
  struct timespec abstime;
  
  abstime.tv_sec = abs_secs;
  abstime.tv_nsec = abs_usecs * 1000;
  pthread_cond_timedwait ((pthread_cond_t*) cond, (pthread_mutex_t*) mutex, &abstime);
}
static BirnetThreadTable pth_thread_table = {
  pth_thread_set_handle,
  pth_thread_get_handle,
  pth_mutex_init,
  (void (*) (BirnetMutex*)) pthread_mutex_lock,
  (int  (*) (BirnetMutex*)) pthread_mutex_trylock,
  (void (*) (BirnetMutex*)) pthread_mutex_unlock,
  (void (*) (BirnetMutex*)) pthread_mutex_destroy,
  pth_rec_mutex_init,
  (void (*) (BirnetRecMutex*)) pthread_mutex_lock,
  (int  (*) (BirnetRecMutex*)) pthread_mutex_trylock,
  (void (*) (BirnetRecMutex*)) pthread_mutex_unlock,
  (void (*) (BirnetRecMutex*)) pthread_mutex_destroy,
  pth_cond_init,
  (void (*) (BirnetCond*))               pthread_cond_signal,
  (void (*) (BirnetCond*))               pthread_cond_broadcast,
  (void (*) (BirnetCond*, BirnetMutex*)) pthread_cond_wait,
  pth_cond_wait_timed,
  (void (*) (BirnetCond*))               pthread_cond_destroy,
};
static BirnetThreadTable*
get_pth_thread_table (void)
{
  if (pthread_key_create (&pth_thread_table_key, (void(*)(void*)) birnet_thread_handle_exit) != 0)
    {
      char buffer[1024];
      snprintf (buffer, 1024, "BirnetThread[%u]: failed to create pthread key, falling back to GLib threads.\n", getpid());
      fputs (buffer, stderr);
      return NULL;
    }
  return &pth_thread_table;
}
#else	/* !BIRNET_HAVE_MUTEXATTR_SETTYPE */
#define	get_pth_thread_table()	NULL
#endif	/* !BIRNET_HAVE_MUTEXATTR_SETTYPE */

void
_birnet_init_threads (void)
{
  BirnetThreadTable *table = get_pth_thread_table ();
  if (!table)
    table = get_fallback_thread_table ();
  birnet_thread_table = *table;
  
  birnet_mutex_init (&global_thread_mutex);
  birnet_mutex_init (&global_startup_mutex);
  birnet_cond_init (&global_thread_cond);
  
  birnet_thread_self ();
}
