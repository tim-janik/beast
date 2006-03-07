/* BIRNET - Synthesis Fusion Kit Interface
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
#include <birnet/birnetconfig.h>
#if	(BIRNET_HAVE_MUTEXATTR_SETTYPE > 0)
#define	_XOPEN_SOURCE   600	/* for full pthread facilities */
#endif	/* defining _XOPEN_SOURCE on random systems can have bad effects */
#include "birnetthreads.h"
#include "birnetmemory.h"
#include "birnetprimitives.h"
#include "birnetlog.h"
#include "birnettime.h"
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
struct _BirnetThread
{
  gchar		 *name;
  BirnetThreadFunc	  func;
  gpointer	  data;
  gint8		  aborted;
  gint8		  got_wakeup;
  gint8           accounting;
  BirnetCond	 *wakeup_cond;
  BirnetThreadWakeup wakeup_func;
  gpointer	  wakeup_data;
  GDestroyNotify  wakeup_destroy;
  guint64	  awake_stamp;
  GData		 *qdata;
  gint            tid;
  gpointer        guard_cache;
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
static void     birnet_guard_deregister_all        (BirnetThread      *thread);
static void	birnet_thread_handle_deleted	(BirnetThread	*thread);

/* --- variables --- */
static BirnetMutex global_thread_mutex = { 0, };
static BirnetCond  global_thread_cond = { 0, };
static BirnetRing *global_thread_list = NULL;
static BirnetRing *thread_awaken_list = NULL;
BirnetThreadTable  birnet_thread_table = { NULL, };


/* --- functions --- */
static BirnetThread*
birnet_thread_handle_new (const gchar *name)
{
  BirnetThread *thread;
  
  thread = birnet_new_struct0 (BirnetThread, 1);
  thread->func = NULL;
  thread->data = NULL;
  thread->aborted = FALSE;
  thread->got_wakeup = FALSE;
  thread->wakeup_cond = NULL;
  thread->wakeup_func = NULL;
  thread->wakeup_destroy = NULL;
  thread->tid = -1;
  if (!name)
    {
      static guint anon_count = 1;
      guint id;
      BIRNET_SYNC_LOCK (&global_thread_mutex);
      id = anon_count++;
      BIRNET_SYNC_UNLOCK (&global_thread_mutex);
      thread->name = g_strdup_printf ("Foreign%u", id);
    }
  else
    thread->name = g_strdup (name);
  g_datalist_init (&thread->qdata);
  return thread;
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
birnet_thread_accounting_L (BirnetThread *self,
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
birnet_thread_exec (gpointer thread)
{
  BirnetThread *self;
  birnet_thread_table.thread_set_handle (thread);
  
  self = birnet_thread_self ();
  g_assert (self == thread);
  
  thread_get_tid (thread);
  
  BIRNET_SYNC_LOCK (&global_thread_mutex);
  global_thread_list = birnet_ring_append (global_thread_list, self);
  self->accounting = 1;
  birnet_thread_accounting_L (self, TRUE);
  birnet_cond_broadcast (&global_thread_cond);
  BIRNET_SYNC_UNLOCK (&global_thread_mutex);
  
  self->func (self->data);

  /* birnet_thread_handle_deleted() does final destruction */
  return NULL;
}

static void
birnet_thread_handle_deleted (BirnetThread *thread)
{
  thread->wakeup_func = NULL;
  if (thread->wakeup_destroy)
    {
      GDestroyNotify wakeup_destroy = thread->wakeup_destroy;
      thread->wakeup_destroy = NULL;
      wakeup_destroy (thread->wakeup_data);
    }
  
  g_datalist_clear (&thread->qdata);
  
  birnet_guard_deregister_all (thread);

  BIRNET_SYNC_LOCK (&global_thread_mutex);
  global_thread_list = birnet_ring_remove (global_thread_list, thread);
  if (thread->awake_stamp)
    thread_awaken_list = birnet_ring_remove (thread_awaken_list, thread);
  birnet_cond_broadcast (&global_thread_cond);
  BIRNET_SYNC_UNLOCK (&global_thread_mutex);
  
  if (thread->wakeup_cond)
    {
      birnet_cond_destroy (thread->wakeup_cond);
      g_free (thread->wakeup_cond);
      thread->wakeup_cond = NULL;
    }
  g_free (thread->name);
  thread->name = NULL;
  birnet_delete_struct (BirnetThread, thread);
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
 * @param name	thread name
 * @param func	function to execute in new thread
 * @param user_data	user data to pass into @a func
 * @param returns	new thread handle or NULL in case of error
 *
 * Create a new thread running @a func.
 */
BirnetThread*
birnet_thread_run (const gchar  *name,
		BirnetThreadFunc func,
		gpointer      user_data)
{
  GThread *gthread = NULL;
  BirnetThread *thread;
  GError *gerror = NULL;
  guint hid = 0;
  
  g_return_val_if_fail (func != NULL, FALSE);
  
  /* silence those stupid priority warnings triggered by glib */
  hid = g_log_set_handler ("GLib", G_LOG_LEVEL_WARNING, filter_priority_warning, NULL);
  
  /* create thread */
  thread = birnet_thread_handle_new (name);
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
      gthread = g_thread_create_full (birnet_thread_exec, thread, 0, joinable, FALSE,
				      G_THREAD_PRIORITY_NORMAL, &gerror);
    }
  
  if (gthread)
    {
      BIRNET_SYNC_LOCK (&global_thread_mutex);
      while (!birnet_ring_find (global_thread_list, thread))
	birnet_cond_wait (&global_thread_cond, &global_thread_mutex);
      BIRNET_SYNC_UNLOCK (&global_thread_mutex);
    }
  else
    {
      if (thread)
	{
	  birnet_delete_struct (BirnetThread, thread);
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
 * @return		thread handle
 *
 * Return the thread handle of the currently running thread.
 */
BirnetThread*
birnet_thread_self (void)
{
  BirnetThread *thread = birnet_thread_table.thread_get_handle ();
  
  if (!thread)
    {
      thread = birnet_thread_handle_new (NULL);
      thread_get_tid (thread);
      if (!thread)
	g_error ("failed to create thread handle for foreign thread");
      birnet_thread_table.thread_set_handle (thread);
      BIRNET_SYNC_LOCK (&global_thread_mutex);
      global_thread_list = birnet_ring_append (global_thread_list, thread);
      BIRNET_SYNC_UNLOCK (&global_thread_mutex);
    }
  return thread;
}

void
birnet_thread_set_name (const gchar *name)
{
  BirnetThread *thread = birnet_thread_self ();
  if (name)
    {
      BIRNET_SYNC_LOCK (&global_thread_mutex);
      g_free (thread->name);
      thread->name = g_strdup (name);
      BIRNET_SYNC_UNLOCK (&global_thread_mutex);
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
  if (thread->wakeup_cond)
    birnet_cond_signal (thread->wakeup_cond);
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
gboolean
birnet_thread_sleep (glong max_useconds)
{
  BirnetThread *self = birnet_thread_self ();
  gboolean aborted;
  
  BIRNET_SYNC_LOCK (&global_thread_mutex);
  if (!self->wakeup_cond)
    {
      self->wakeup_cond = g_new0 (BirnetCond, 1);
      birnet_cond_init (self->wakeup_cond);
    }
  
  birnet_thread_accounting_L (self, FALSE);
  
  if (!self->got_wakeup && max_useconds != 0)
    {
      if (max_useconds >= 0) /* wait once without time adjustments */
	birnet_cond_wait_timed (self->wakeup_cond, &global_thread_mutex, max_useconds);
      else /* wait forever */
	while (!self->got_wakeup)
	  birnet_cond_wait (self->wakeup_cond, &global_thread_mutex);
    }
  
  self->got_wakeup = FALSE;
  aborted = self->aborted != FALSE;
  BIRNET_SYNC_UNLOCK (&global_thread_mutex);
  
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
		       gpointer        wakeup_data,
		       GDestroyNotify  destroy)
{
  BirnetThread *self = birnet_thread_self ();
  
  g_return_if_fail (wakeup_func != NULL);
  g_return_if_fail (self->wakeup_func == NULL);
  
  BIRNET_SYNC_LOCK (&global_thread_mutex);
  self->wakeup_func = wakeup_func;
  self->wakeup_data = wakeup_data;
  self->wakeup_destroy = destroy;
  BIRNET_SYNC_UNLOCK (&global_thread_mutex);
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
  
  BIRNET_SYNC_LOCK (&global_thread_mutex);
  g_assert (birnet_ring_find (global_thread_list, thread));
  birnet_thread_wakeup_L (thread);
  BIRNET_SYNC_UNLOCK (&global_thread_mutex);
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
  
  BIRNET_SYNC_LOCK (&global_thread_mutex);
  if (!self->awake_stamp)
    {
      thread_awaken_list = birnet_ring_prepend (thread_awaken_list, self);
      self->awake_stamp = stamp;
    }
  else
    self->awake_stamp = MIN (self->awake_stamp, stamp);
  BIRNET_SYNC_UNLOCK (&global_thread_mutex);
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
  
  BIRNET_SYNC_LOCK (&global_thread_mutex);
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
  BIRNET_SYNC_UNLOCK (&global_thread_mutex);
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
  
  BIRNET_SYNC_LOCK (&global_thread_mutex);
  g_assert (birnet_ring_find (global_thread_list, thread));
  thread->aborted = TRUE;
  birnet_thread_wakeup_L (thread);
  while (birnet_ring_find (global_thread_list, thread))
    birnet_cond_wait (&global_thread_cond, &global_thread_mutex);
  BIRNET_SYNC_UNLOCK (&global_thread_mutex);
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
  
  BIRNET_SYNC_LOCK (&global_thread_mutex);
  g_assert (birnet_ring_find (global_thread_list, thread));
  thread->aborted = TRUE;
  birnet_thread_wakeup_L (thread);
  BIRNET_SYNC_UNLOCK (&global_thread_mutex);
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
gboolean
birnet_thread_aborted (void)
{
  BirnetThread *self = birnet_thread_self ();
  gboolean aborted;
  
  BIRNET_SYNC_LOCK (&global_thread_mutex);
  birnet_thread_accounting_L (self, FALSE);
  aborted = self->aborted != FALSE;
  BIRNET_SYNC_UNLOCK (&global_thread_mutex);
  
  return aborted;
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
		     glong     max_useconds)
{
  if (max_useconds < 0)
    birnet_cond_wait (cond, mutex);
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
      
      birnet_thread_table.cond_wait_timed (cond, mutex, limit.tv_sec, limit.tv_usec);
    }
}


/* --- thread info --- */
BirnetThreadInfo*
birnet_thread_info_collect (BirnetThread *thread)
{
  BirnetThreadInfo *info = g_new0 (BirnetThreadInfo, 1);
  struct timeval now;
  gboolean recent = TRUE;
  if (!thread)
    thread = birnet_thread_self ();
  gettimeofday (&now, NULL);
  BIRNET_SYNC_LOCK (&global_thread_mutex);
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
  BIRNET_SYNC_UNLOCK (&global_thread_mutex);
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
  BirnetGuard  *next;       /* global guard list */
  BirnetThread *thread;
  BirnetGuard  *cache_next; /* per thread free list */
  guint      n_values;
  gpointer   values[1];  /* variable length array */
};
static BirnetGuard * volatile guard_list = NULL;
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
BirnetGuard*
birnet_guard_register (guint n_hazards)
{
  BirnetThread *thread = birnet_thread_self();
  BirnetGuard *guard, *last = NULL;
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
      while (!g_atomic_pointer_compare_and_exchange ((gpointer) &guard_list, guard->next, guard));
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
birnet_guard_deregister (BirnetGuard *guard)
{
  guard = values2guard (guard);
  BirnetThread *thread = birnet_thread_self();
  g_return_if_fail (guard->thread == thread);
  memset (guard->values, 0, sizeof (guard->values[0]) * guard->n_values);
  /* must we have a memory barrier here? */
  guard->cache_next = thread->guard_cache;
  thread->guard_cache = guard;
}

static void
birnet_guard_deregister_all (BirnetThread *thread)
{
  BirnetGuard *guard;
  thread->guard_cache = NULL;
  for (guard = g_atomic_pointer_get (&guard_list); guard; guard = guard->next)
    if (guard->thread == thread)
      {
        memset (guard->values, 0, sizeof (guard->values[0]) * guard->n_values);
        guard->cache_next = NULL;
        g_atomic_pointer_compare_and_exchange ((gpointer*) &guard->thread, thread, NULL); /* reset ->thread with memory barrier */
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
void birnet_guard_protect (BirnetGuard *guard,  /* defined in birnetthreads.h */
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
gboolean
birnet_guard_snap_values (guint          *n_values,
                       gpointer       *values)
{
  guint i, n = 0;
  BirnetGuard *guard;
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
gboolean
birnet_guard_is_protected (gpointer value)
{
  if (value)
    {
      BirnetGuard *guard;
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
      BIRNET_SYNC_LOCK (&rec_mutex->mutex);
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
	  BIRNET_SYNC_UNLOCK (&rec_mutex->mutex);
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
                          gulong    abs_secs,
                          gulong    abs_usecs)
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
  fallback_thread_table_key = g_private_new ((GDestroyNotify) birnet_thread_handle_deleted);
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
    birnet_thread_handle_deleted (tmp);
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
		     gulong    abs_secs,
		     gulong    abs_usecs)
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
  (void (*)            (BirnetCond*)) pthread_cond_signal,
  (void (*)            (BirnetCond*)) pthread_cond_broadcast,
  (void (*) (BirnetCond*, BirnetMutex*)) pthread_cond_wait,
  pth_cond_wait_timed,
  (void (*)            (BirnetCond*)) pthread_cond_destroy,
};
static BirnetThreadTable*
get_pth_thread_table (void)
{
  if (pthread_key_create (&pth_thread_table_key, (void(*)(void*)) birnet_thread_handle_deleted) != 0)
    {
      birnet_diag ("failed to create pthread key, falling back to GLib threads");
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
  birnet_cond_init (&global_thread_cond);
  
  _birnet_init_logging ();
  
  _birnet_init_memory ();

  birnet_thread_self ();
}
