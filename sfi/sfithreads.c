/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002 Tim Janik and Stefan Westerfeld
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
#include <sfi/sficonfig.h>
#if	(SFI_HAVE_MUTEXATTR_SETTYPE > 0)
#define	_XOPEN_SOURCE   600	/* for full pthread facilities */
#endif	/* defining _XOPEN_SOURCE on random systems can have bad effects */
#include "sfithreads.h"
#include "sfimemory.h"
#include "sfiprimitives.h"
#include "sfilog.h"
#include "sfitime.h"
#include <sys/time.h>
#include <sched.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/times.h>


/* --- structures --- */
struct _SfiThread
{
  gchar		 *name;
  SfiThreadFunc	  func;
  gpointer	  data;
  gint8		  aborted;
  gint8		  got_wakeup;
  gint8           accounting;
  SfiCond	 *wakeup_cond;
  SfiThreadWakeup wakeup_func;
  gpointer	  wakeup_data;
  GDestroyNotify  wakeup_destroy;
  guint64	  awake_stamp;
  GData		 *qdata;
  gint            tid;
  /* accounting */
  struct {
    struct timeval stamp;
    gint64         utime, stime;
    gint64         cutime, cstime;
  }                ac;
  struct {
    guint          processor;
    gint           priority;
    SfiThreadState state;
    gint           utime, stime;
    gint           cutime, cstime;
  }                info;
};


/* --- variables --- */
static SfiMutex global_thread_mutex = { 0, };
static SfiCond  global_thread_cond = { 0, };
static SfiRing *global_thread_list = NULL;
static SfiRing *thread_awaken_list = NULL;
SfiThreadTable  sfi_thread_table = { NULL, };


/* --- functions --- */
static SfiThread*
sfi_thread_handle_new (const gchar *name)
{
  SfiThread *thread;
  gint error = 0;
  
  thread = sfi_new_struct0 (SfiThread, 1);
  thread->func = NULL;
  thread->data = NULL;
  thread->aborted = FALSE;
  thread->got_wakeup = FALSE;
  thread->wakeup_cond = NULL;
  thread->wakeup_func = NULL;
  thread->wakeup_destroy = NULL;
  thread->tid = -1;
  if (!error)
    {
      if (!name)
	{
	  static guint anon_count = 1;
	  guint id;
	  SFI_SYNC_LOCK (&global_thread_mutex);
	  id = anon_count++;
	  SFI_SYNC_UNLOCK (&global_thread_mutex);
	  thread->name = g_strdup_printf ("Foreign%u", id);
	}
      else
	thread->name = g_strdup (name);
      g_datalist_init (&thread->qdata);
    }
  else
    {
      sfi_delete_struct (SfiThread, thread);
      thread = NULL;
    }
  return thread;
}

#ifdef  __linux__
#include <sys/types.h>
#include <sys/syscall.h>
#include <linux/unistd.h>
#endif
static void
thread_get_tid (SfiThread *thread)
{
  gint ppid = thread->tid;      /* creator process id */
  gint tid = -1;
  
#if     defined (__linux__) && defined (__NR_gettid)    /* present on linux >= 2.4.20 */
  tid = syscall (__NR_gettid);
#endif
  if (tid < 0)
    tid = getpid();
  if (tid != ppid &&            /* thread pid different from creator pid, probably correct */
      tid > 0)
    thread->tid = tid;
  else
    thread->tid = 0;            /* failed to figure id */
}

static inline unsigned long long int
timeval_usecs (const struct timeval *tv)
{
  return tv->tv_usec + tv->tv_sec * (guint64) 1000000;
}

static void
thread_info_from_stat_L (SfiThread *self,
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
      gchar *filename = g_strdup_printf ("/proc/%u/stat", self->tid);
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
  
  if (n >= 3)
    self->info.state = state;
  if (n >= 39)
    self->info.processor = 1 + processor;
}

#define ACCOUNTING_MSECS        (500)

static inline void
sfi_thread_accounting_L (SfiThread *self,
                         gboolean   force_update)
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
      struct rusage res = { { 0 } };
      gint64 utime = self->ac.utime;
      gint64 stime = self->ac.stime;
      gdouble dfact = 1000000.0 / MAX (diff, 1);
      self->ac.stamp = stamp;
      getrusage (RUSAGE_SELF, &res);
      self->ac.utime = timeval_usecs (&res.ru_utime); /* user time used */
      self->ac.stime = timeval_usecs (&res.ru_stime); /* system time used */
      self->info.utime = MAX (self->ac.utime - utime, 0) * dfact;
      self->info.stime = MAX (self->ac.stime - stime, 0) * dfact;
      utime = self->ac.cutime;
      stime = self->ac.cstime;
      getrusage (RUSAGE_CHILDREN, &res);
      self->ac.cutime = timeval_usecs (&res.ru_utime);
      self->ac.cstime = timeval_usecs (&res.ru_stime);
      self->info.cutime = MAX (self->ac.cutime - utime, 0) * dfact;
      self->info.cstime = MAX (self->ac.cstime - stime, 0) * dfact;
      self->info.priority = getpriority (PRIO_PROCESS, self->tid);
      thread_info_from_stat_L (self, dfact);
      self->accounting--;
    }
}

static gpointer
sfi_thread_exec (gpointer thread)
{
  SfiThread *self;
  sfi_thread_table.thread_set_handle (thread);
  
  self = sfi_thread_self ();
  g_assert (self == thread);
  
  thread_get_tid (thread);
  
  SFI_SYNC_LOCK (&global_thread_mutex);
  global_thread_list = sfi_ring_append (global_thread_list, self);
  self->accounting = 1;
  sfi_thread_accounting_L (self, TRUE);
  sfi_cond_broadcast (&global_thread_cond);
  SFI_SYNC_UNLOCK (&global_thread_mutex);
  
  self->func (self->data);
  
  g_datalist_clear (&self->qdata);
  /* sfi_thread_handle_deleted() does final destruction */
  return NULL;
}

void
sfi_thread_handle_deleted (SfiThread *thread)
{
  g_datalist_clear (&thread->qdata);
  
  if (thread->wakeup_destroy)
    {
      GDestroyNotify wakeup_destroy = thread->wakeup_destroy;
      thread->wakeup_destroy = NULL;
      wakeup_destroy (thread->wakeup_data);
    }
  
  SFI_SYNC_LOCK (&global_thread_mutex);
  global_thread_list = sfi_ring_remove (global_thread_list, thread);
  if (thread->awake_stamp)
    thread_awaken_list = sfi_ring_remove (thread_awaken_list, thread);
  sfi_cond_broadcast (&global_thread_cond);
  SFI_SYNC_UNLOCK (&global_thread_mutex);
  
  if (thread->wakeup_cond)
    {
      sfi_cond_destroy (thread->wakeup_cond);
      g_free (thread->wakeup_cond);
      thread->wakeup_cond = NULL;
    }
  g_free (thread->name);
  thread->name = NULL;
  sfi_delete_struct (SfiThread, thread);
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

/**
 * sfi_thread_run
 * @name:      thread name
 * @func:      function to execute in new thread
 * @user_data: user data to pass into @func
 * @returns:   new thread handle or %NULL in case of error
 * Create a new thread running @func.
 */
SfiThread*
sfi_thread_run (const gchar  *name,
		SfiThreadFunc func,
		gpointer      user_data)
{
  GThread *gthread = NULL;
  SfiThread *thread;
  GError *gerror = NULL;
  guint hid = 0;
  
  g_return_val_if_fail (func != NULL, FALSE);
  
  /* silence those stupid priority warnings triggered by glib */
  hid = g_log_set_handler ("GLib", G_LOG_LEVEL_WARNING, filter_priority_warning, NULL);
  
  /* create thread */
  thread = sfi_thread_handle_new (name);
  if (thread)
    {
      const gboolean joinable = FALSE;
      
      /* don't dare setting joinable to TRUE, that prevents the thread's
       * resources from being freed, since we don't offer pthread_join().
       * so we'd just run out of stack at some point.
       */
      thread->func = func;
      thread->data = user_data;
      thread->tid = getpid();
      gthread = g_thread_create_full (sfi_thread_exec, thread, 0, joinable, FALSE,
				      G_THREAD_PRIORITY_NORMAL, &gerror);
    }
  
  if (gthread)
    {
      SFI_SYNC_LOCK (&global_thread_mutex);
      while (!sfi_ring_find (global_thread_list, thread))
	sfi_cond_wait (&global_thread_cond, &global_thread_mutex);
      SFI_SYNC_UNLOCK (&global_thread_mutex);
    }
  else
    {
      if (thread)
	{
	  sfi_delete_struct (SfiThread, thread);
	  thread = NULL;
	}
      g_message ("failed to create thread \"%s\": %s", name ? name : "Anon", gerror->message);
      g_error_free (gerror);
    }
  
  /* withdraw warning filter */
  g_log_remove_handler ("GLib", hid);
  
  return thread;
}

/**
 * sfi_thread_self
 * @RETURNS: thread handle
 * Return the thread handle of the currently running thread.
 */
SfiThread*
sfi_thread_self (void)
{
  SfiThread *thread = sfi_thread_table.thread_get_handle ();
  
  if (!thread)
    {
      thread = sfi_thread_handle_new (NULL);
      thread_get_tid (thread);
      if (!thread)
	g_error ("failed to create thread handle for foreign thread");
      sfi_thread_table.thread_set_handle (thread);
      SFI_SYNC_LOCK (&global_thread_mutex);
      global_thread_list = sfi_ring_append (global_thread_list, thread);
      SFI_SYNC_UNLOCK (&global_thread_mutex);
    }
  return thread;
}

void
sfi_thread_set_name (const gchar *name)
{
  SfiThread *thread = sfi_thread_self ();
  if (name)
    {
      SFI_SYNC_LOCK (&global_thread_mutex);
      g_free (thread->name);
      thread->name = g_strdup (name);
      SFI_SYNC_UNLOCK (&global_thread_mutex);
    }
}

/**
 * sfi_thread_self_pid
 * @RETURNS: thread id
 * Return the thread specific id. This function is highly
 * system dependant. The thread id may deviate from the overall
 * process id or not. On linux, threads have their own id,
 * allthough since kernel 2.6, they share the same process id.
 */
gint
sfi_thread_self_pid (void)
{
  return sfi_thread_self ()->tid;
}

static void
sfi_thread_wakeup_L (SfiThread *thread)
{
  if (thread->wakeup_cond)
    sfi_cond_signal (thread->wakeup_cond);
  if (thread->wakeup_func)
    thread->wakeup_func (thread->wakeup_data);
  thread->got_wakeup = TRUE;
}

/**
 * sfi_thread_sleep
 * @max_useconds: maximum amount of micro seconds to sleep (-1 for infinite time)
 * @returns:      %TRUE while the thread should continue execution
 * Sleep for the amount of time given.
 * This function may get interrupted by wakeup requests from
 * sfi_thread_wakeup(), abort requests from sfi_thread_queue_abort()
 * or other means. It returns whether the thread is supposed to
 * continue execution after waking up.
 * This function or alternatively sfi_thread_aborted() should be called
 * periodically, to react to thread abortion requests and to update
 * internal accounting information.
 */
gboolean
sfi_thread_sleep (glong max_useconds)
{
  SfiThread *self = sfi_thread_self ();
  gboolean aborted;
  
  SFI_SYNC_LOCK (&global_thread_mutex);
  if (!self->wakeup_cond)
    {
      self->wakeup_cond = g_new0 (SfiCond, 1);
      sfi_cond_init (self->wakeup_cond);
    }
  
  sfi_thread_accounting_L (self, FALSE);
  
  if (!self->got_wakeup && max_useconds != 0)
    {
      if (max_useconds >= 0) /* wait once without time adjustments */
	sfi_cond_wait_timed (self->wakeup_cond, &global_thread_mutex, max_useconds);
      else /* wait forever */
	while (!self->got_wakeup)
	  sfi_cond_wait (self->wakeup_cond, &global_thread_mutex);
    }
  
  self->got_wakeup = FALSE;
  aborted = self->aborted != FALSE;
  SFI_SYNC_UNLOCK (&global_thread_mutex);
  
  return !aborted;
}

/**
 * sfi_thread_set_wakeup
 * @wakeup_func: wakeup function to be called by sfi_thread_wakeup()
 * @wakeup_data: data passed into wakeup_func()
 * @destroy:     destroy handler for @wakeup_data
 * Set the wakeup function for the current thread. This enables
 * the thread to be woken up through sfi_thread_wakeup() even
 * if not sleeping in sfi_thread_sleep(). The wakeup function
 * must be thread-safe, so it may be called from any thread,
 * and it should be fast, because the global thread system lock
 * is held during its invokation.
 * Per thread, the wakeup function may be set only once.
 */
void
sfi_thread_set_wakeup (SfiThreadWakeup wakeup_func,
		       gpointer        wakeup_data,
		       GDestroyNotify  destroy)
{
  SfiThread *self = sfi_thread_self ();
  
  g_return_if_fail (wakeup_func != NULL);
  g_return_if_fail (self->wakeup_func == NULL);
  
  SFI_SYNC_LOCK (&global_thread_mutex);
  self->wakeup_func = wakeup_func;
  self->wakeup_data = wakeup_data;
  self->wakeup_destroy = destroy;
  SFI_SYNC_UNLOCK (&global_thread_mutex);
}

/**
 * sfi_thread_wakeup
 * @thread: thread to wake up
 * Wake up a currently sleeping thread. In practice, this
 * function simply causes the next call to sfi_thread_sleep()
 * within @thread to last for 0 seconds.
 */
void
sfi_thread_wakeup (SfiThread *thread)
{
  g_return_if_fail (thread != NULL);
  
  SFI_SYNC_LOCK (&global_thread_mutex);
  g_assert (sfi_ring_find (global_thread_list, thread));
  sfi_thread_wakeup_L (thread);
  SFI_SYNC_UNLOCK (&global_thread_mutex);
}

/**
 * sfi_thread_awake_after
 * @stamp: stamp to trigger wakeup
 * Wake the current thread up at the next invocation
 * of sfi_thread_emit_wakeups() with a wakup_stamp
 * greater than @stamp.
 */
void
sfi_thread_awake_after (guint64 stamp)
{
  SfiThread *self = sfi_thread_self ();
  
  g_return_if_fail (stamp > 0);
  
  SFI_SYNC_LOCK (&global_thread_mutex);
  if (!self->awake_stamp)
    {
      thread_awaken_list = sfi_ring_prepend (thread_awaken_list, self);
      self->awake_stamp = stamp;
    }
  else
    self->awake_stamp = MIN (self->awake_stamp, stamp);
  SFI_SYNC_UNLOCK (&global_thread_mutex);
}

/**
 * sfi_thread_emit_wakeups
 * @wakeup_stamp: wakeup stamp to trigger wakeups
 * Wake all currently sleeping threads up which queued
 * a wakeup through sfi_thread_awake_after() with a
 * stamp smaller than @wakeup_stamp.
 */
void
sfi_thread_emit_wakeups (guint64 wakeup_stamp)
{
  SfiRing *ring, *next;
  
  g_return_if_fail (wakeup_stamp > 0);
  
  SFI_SYNC_LOCK (&global_thread_mutex);
  for (ring = thread_awaken_list; ring; ring = next)
    {
      SfiThread *thread = ring->data;
      next = sfi_ring_walk (ring, thread_awaken_list);
      if (thread->awake_stamp <= wakeup_stamp)
	{
	  thread->awake_stamp = 0;
	  thread_awaken_list = sfi_ring_remove (thread_awaken_list, thread);
	  sfi_thread_wakeup_L (thread);
	}
    }
  SFI_SYNC_UNLOCK (&global_thread_mutex);
}

/**
 * sfi_thread_abort
 * @thread: thread to abort
 * Abort a currently running thread. This function does not
 * return until the thread in question terminated execution.
 * Note that the thread handle gets invalidated with invocation
 * of sfi_thread_abort() or sfi_thread_queue_abort().
 */
void
sfi_thread_abort (SfiThread *thread)
{
  g_return_if_fail (thread != NULL);
  g_return_if_fail (thread != sfi_thread_self ());
  
  SFI_SYNC_LOCK (&global_thread_mutex);
  g_assert (sfi_ring_find (global_thread_list, thread));
  thread->aborted = TRUE;
  sfi_thread_wakeup_L (thread);
  while (sfi_ring_find (global_thread_list, thread))
    sfi_cond_wait (&global_thread_cond, &global_thread_mutex);
  SFI_SYNC_UNLOCK (&global_thread_mutex);
}

/**
 * sfi_thread_queue_abort
 * @thread: thread to abort
 * Same as sfi_thread_abort(), but returns as soon as possible,
 * even if thread hasn't stopped execution yet.
 * Note that the thread handle gets invalidated with invocation
 * of sfi_thread_abort() or sfi_thread_queue_abort().
 */
void
sfi_thread_queue_abort (SfiThread *thread)
{
  g_return_if_fail (thread != NULL);
  
  SFI_SYNC_LOCK (&global_thread_mutex);
  g_assert (sfi_ring_find (global_thread_list, thread));
  thread->aborted = TRUE;
  sfi_thread_wakeup_L (thread);
  SFI_SYNC_UNLOCK (&global_thread_mutex);
}

/**
 * sfi_thread_aborted
 * @returns: %TRUE if the thread should abort execution
 * Find out if the currently running thread should be aborted (the thread is
 * supposed to return from its main thread function). This function or
 * alternatively sfi_thread_sleep() should be called periodically, to
 * react to thread abortion requests and to update internal accounting
 * information.
 */
gboolean
sfi_thread_aborted (void)
{
  SfiThread *self = sfi_thread_self ();
  gboolean aborted;
  
  SFI_SYNC_LOCK (&global_thread_mutex);
  sfi_thread_accounting_L (self, FALSE);
  aborted = self->aborted != FALSE;
  SFI_SYNC_UNLOCK (&global_thread_mutex);
  
  return aborted;
}

gpointer
sfi_thread_get_qdata (GQuark quark)
{
  SfiThread *self = sfi_thread_self ();
  return quark ? g_datalist_id_get_data (&self->qdata, quark) : NULL;
}

void
sfi_thread_set_qdata_full (GQuark         quark,
			   gpointer       data,
			   GDestroyNotify destroy)
{
  SfiThread *self = sfi_thread_self ();
  g_return_if_fail (quark > 0);
  g_datalist_id_set_data_full (&self->qdata, quark, data,
			       data ? destroy : (GDestroyNotify) NULL);
}

gpointer
sfi_thread_steal_qdata (GQuark quark)
{
  SfiThread *self = sfi_thread_self ();
  return quark ? g_datalist_id_remove_no_notify (&self->qdata, quark) : NULL;
}

void
sfi_cond_wait_timed (SfiCond  *cond,
		     SfiMutex *mutex,
		     glong     max_useconds)
{
  if (max_useconds < 0)
    sfi_cond_wait (cond, mutex);
  else if (max_useconds > 0)
    {
      struct timeval now, limit;
      glong secs = max_useconds / 1000000;
      gettimeofday (&now, NULL);
      limit.tv_sec = now.tv_sec + secs;
      max_useconds -= secs * 1000000;
      limit.tv_usec = now.tv_usec + max_useconds;
      if (limit.tv_usec >= 1000000)
        {
          limit.tv_usec -= 1000000;
          limit.tv_sec += 1;
        }
      
      sfi_thread_table.cond_wait_timed (cond, mutex, limit.tv_sec, limit.tv_usec);
    }
}


/* --- thread info --- */
SfiThreadInfo*
sfi_thread_info_collect (SfiThread *thread)
{
  SfiThreadInfo *info = g_new0 (SfiThreadInfo, 1);
  struct timeval now;
  gboolean recent = TRUE;
  if (!thread)
    thread = sfi_thread_self ();
  gettimeofday (&now, NULL);
  SFI_SYNC_LOCK (&global_thread_mutex);
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
  SFI_SYNC_UNLOCK (&global_thread_mutex);
  return info;
}

void
sfi_thread_info_free (SfiThreadInfo  *info)
{
  g_return_if_fail (info != NULL);
  g_free (info->name);
  g_free (info);
}


/* --- fallback (GLib) SfiThreadTable --- */
static GPrivate *fallback_thread_table_key = NULL;

static void
fallback_thread_set_handle (SfiThread *handle)
{
  g_private_set (fallback_thread_table_key, handle);
}

static SfiThread*
fallback_thread_get_handle (void)
{
  return g_private_get (fallback_thread_table_key);
}

static void
fallback_mutex_init (SfiMutex *mutex)
{
  g_return_if_fail (mutex != NULL);
  mutex->mutex_pointer = g_mutex_new ();
}

static int
fallback_mutex_trylock (SfiMutex *mutex)
{
  return g_mutex_trylock (mutex->mutex_pointer) ? 0 : -1;
}

static void
fallback_mutex_lock (SfiMutex *mutex)
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
	  g_thread_yield ();
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
fallback_mutex_unlock (SfiMutex *mutex)
{
  g_mutex_unlock (mutex->mutex_pointer);
}

static void
fallback_mutex_destroy (SfiMutex *mutex)
{
  g_mutex_free (mutex->mutex_pointer);
  memset (mutex, 0, sizeof (*mutex));
}

static void
fallback_rec_mutex_init (SfiRecMutex *rec_mutex)
{
  rec_mutex->owner = NULL;
  sfi_mutex_init (&rec_mutex->mutex);
  rec_mutex->depth = 0;
}

static int
fallback_rec_mutex_trylock (SfiRecMutex *rec_mutex)
{
  SfiThread *self = sfi_thread_self ();
  
  if (rec_mutex->owner == self)
    {
      g_assert (rec_mutex->depth > 0);  /* paranoid */
      rec_mutex->depth += 1;
      return 0;
    }
  else
    {
      if (sfi_mutex_trylock (&rec_mutex->mutex))
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
fallback_rec_mutex_lock (SfiRecMutex *rec_mutex)
{
  SfiThread *self = sfi_thread_self ();
  
  if (rec_mutex->owner == self)
    {
      g_assert (rec_mutex->depth > 0);  /* paranoid */
      rec_mutex->depth += 1;
    }
  else
    {
      SFI_SYNC_LOCK (&rec_mutex->mutex);
      g_assert (rec_mutex->owner == NULL && rec_mutex->depth == 0); /* paranoid */
      rec_mutex->owner = self;
      rec_mutex->depth = 1;
    }
}

static void
fallback_rec_mutex_unlock (SfiRecMutex *rec_mutex)
{
  SfiThread *self = sfi_thread_self ();
  
  if (rec_mutex->owner == self && rec_mutex->depth > 0)
    {
      rec_mutex->depth -= 1;
      if (!rec_mutex->depth)
	{
	  rec_mutex->owner = NULL;
	  SFI_SYNC_UNLOCK (&rec_mutex->mutex);
	}
    }
  else
    g_warning ("unable to unlock recursive mutex with self %p != %p or depth %u < 1",
	       rec_mutex->owner, self, rec_mutex->depth);
}

static void
fallback_rec_mutex_destroy (SfiRecMutex *rec_mutex)
{
  if (rec_mutex->owner || rec_mutex->depth)
    g_warning ("recursive mutex still locked during destruction");
  else
    {
      sfi_mutex_destroy (&rec_mutex->mutex);
      g_assert (rec_mutex->owner == NULL && rec_mutex->depth == 0);
    }
}

static void
fallback_cond_init (SfiCond *cond)
{
  cond->cond_pointer = g_cond_new ();
}

static void
fallback_cond_wait (SfiCond  *cond,
                    SfiMutex *mutex)
{
  /* infinite wait */
  g_cond_wait (cond->cond_pointer, mutex->mutex_pointer);
}

static void
fallback_cond_signal (SfiCond *cond)
{
  g_cond_signal (cond->cond_pointer);
}

static void
fallback_cond_broadcast (SfiCond *cond)
{
  g_cond_broadcast (cond->cond_pointer);
}

static void
fallback_cond_destroy (SfiCond *cond)
{
  g_cond_free (cond->cond_pointer);
}

static void
fallback_cond_wait_timed (SfiCond  *cond,
                          SfiMutex *mutex,
                          gulong    abs_secs,
                          gulong    abs_usecs)
{
  GTimeVal gtime;
  
  gtime.tv_sec = abs_secs;
  gtime.tv_usec = abs_usecs;
  g_cond_timed_wait (cond->cond_pointer, mutex->mutex_pointer, &gtime);
}

static SfiThreadTable fallback_thread_table = {
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

static SfiThreadTable*
get_fallback_thread_table (void)
{
  fallback_thread_table_key = g_private_new ((GDestroyNotify) sfi_thread_handle_deleted);
  return &fallback_thread_table;
}


/* --- POSIX threads table --- */
#if	(SFI_HAVE_MUTEXATTR_SETTYPE > 0)
#include <pthread.h>
static pthread_key_t pth_thread_table_key = 0;
static void
pth_thread_set_handle (SfiThread *handle)
{
  SfiThread *tmp = pthread_getspecific (pth_thread_table_key);
  pthread_setspecific (pth_thread_table_key, handle);
  if (tmp)
    sfi_thread_handle_deleted (tmp);
}
static SfiThread*
pth_thread_get_handle (void)
{
  return pthread_getspecific (pth_thread_table_key);
}
static void
pth_mutex_init (SfiMutex *mutex)
{
  /* need NULL attribute here, which is the fast mutex in glibc
   * and cannot be chosen through pthread_mutexattr_settype()
   */
  pthread_mutex_init ((pthread_mutex_t*) mutex, NULL);
}
static void
pth_rec_mutex_init (SfiRecMutex *mutex)
{
  pthread_mutexattr_t attr;
  pthread_mutexattr_init (&attr);
  pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init ((pthread_mutex_t*) mutex, &attr);
  pthread_mutexattr_destroy (&attr);
}
static void
pth_cond_init (SfiCond *cond)
{
  pthread_cond_init ((pthread_cond_t*) cond, NULL);
}
static void
pth_cond_wait_timed (SfiCond  *cond,
		     SfiMutex *mutex,
		     gulong    abs_secs,
		     gulong    abs_usecs)
{
  struct timespec abstime;
  
  abstime.tv_sec = abs_secs;
  abstime.tv_nsec = abs_usecs * 1000;
  pthread_cond_timedwait ((pthread_cond_t*) cond, (pthread_mutex_t*) mutex, &abstime);
}
static SfiThreadTable pth_thread_table = {
  pth_thread_set_handle,
  pth_thread_get_handle,
  pth_mutex_init,
  (void (*) (SfiMutex*)) pthread_mutex_lock,
  (int  (*) (SfiMutex*)) pthread_mutex_trylock,
  (void (*) (SfiMutex*)) pthread_mutex_unlock,
  (void (*) (SfiMutex*)) pthread_mutex_destroy,
  pth_rec_mutex_init,
  (void (*) (SfiRecMutex*)) pthread_mutex_lock,
  (int  (*) (SfiRecMutex*)) pthread_mutex_trylock,
  (void (*) (SfiRecMutex*)) pthread_mutex_unlock,
  (void (*) (SfiRecMutex*)) pthread_mutex_destroy,
  pth_cond_init,
  (void (*)            (SfiCond*)) pthread_cond_signal,
  (void (*)            (SfiCond*)) pthread_cond_broadcast,
  (void (*) (SfiCond*, SfiMutex*)) pthread_cond_wait,
  pth_cond_wait_timed,
  (void (*)            (SfiCond*)) pthread_cond_destroy,
};
static SfiThreadTable*
get_pth_thread_table (void)
{
  if (pthread_key_create (&pth_thread_table_key, (void(*)(void*)) sfi_thread_handle_deleted) != 0)
    {
      sfi_diag ("failed to create pthread key, falling back to GLib threads");
      return NULL;
    }
  return &pth_thread_table;
}
#else	/* !SFI_HAVE_MUTEXATTR_SETTYPE */
#define	get_pth_thread_table()	NULL
#endif	/* !SFI_HAVE_MUTEXATTR_SETTYPE */

void
_sfi_init_threads (void)
{
  SfiThreadTable *table = get_pth_thread_table ();
  if (!table)
    table = get_fallback_thread_table ();
  sfi_thread_table = *table;
  
  sfi_mutex_init (&global_thread_mutex);
  sfi_cond_init (&global_thread_cond);
  
  _sfi_init_memory ();
  
  sfi_thread_self ();
}
