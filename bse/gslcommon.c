/* GSL - Generic Sound Layer
 * Copyright (C) 2001 Tim Janik
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
#include "gslcommon.h"

#include "gsldatacache.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <string.h>
#include <sched.h>
#include <errno.h>
#include <sys/poll.h>
#include <sys/stat.h>

/* some systems don't have ERESTART (which is what linux returns for system
 * calls on pipes which are being interrupted). most propably just use EINTR,
 * and maybe some can return both. so we check for both in the below code,
 * and alias ERESTART to EINTR if it's not present. compilers are supposed
 * to catch and optimize the doubled check arising from this.
 */
#ifndef ERESTART
#define ERESTART        EINTR
#endif


#define	SIMPLE_CACHE_SIZE	(1024)
#define	PREALLOC		(8)


/* --- variables --- */
volatile guint64 gsl_externvar_tick_stamp = 0;
static guint     global_tick_stamp_leaps = 0;


/* --- memory allocation --- */
static GslMutex     global_memory = { 0, };
static GTrashStack *simple_cache[SIMPLE_CACHE_SIZE] = { NULL, };
static gulong       memory_allocated = 0;

const guint
gsl_alloc_upper_power2 (const gulong number)
{
  return number ? 1 << g_bit_storage (number - 1) : 0;
}

static inline gpointer
low_alloc (gsize mem_size)
{
  gpointer mem;

  if (mem_size >= sizeof (GTrashStack) && mem_size < SIMPLE_CACHE_SIZE + sizeof (GTrashStack))
    {
      GSL_SPIN_LOCK (&global_memory);
      mem = g_trash_stack_pop (simple_cache + mem_size - sizeof (GTrashStack));
      GSL_SPIN_UNLOCK (&global_memory);
      if (!mem)
	{
	  guint8 *cache_mem = g_malloc (mem_size * PREALLOC);
	  guint i;
	  
	  GSL_SPIN_LOCK (&global_memory);
	  memory_allocated += mem_size * PREALLOC;
	  for (i = 0; i < PREALLOC - 1; i++)
	    {
	      g_trash_stack_push (simple_cache + mem_size - sizeof (GTrashStack), cache_mem);
	      cache_mem += mem_size;
	    }
	  GSL_SPIN_UNLOCK (&global_memory);
	  mem = cache_mem;
	}
    }
  else
    {
      mem = g_malloc (mem_size);
      GSL_SPIN_LOCK (&global_memory);
      memory_allocated += mem_size;
      GSL_SPIN_UNLOCK (&global_memory);
    }
  return mem;
}

static inline void
low_free (gsize    mem_size,
	  gpointer mem)
{
  if (mem_size >= sizeof (GTrashStack) && mem_size < SIMPLE_CACHE_SIZE + sizeof (GTrashStack))
    {
      GSL_SPIN_LOCK (&global_memory);
      g_trash_stack_push (simple_cache + mem_size - sizeof (GTrashStack), mem);
      GSL_SPIN_UNLOCK (&global_memory);
    }
  else
    {
      g_free (mem);
      GSL_SPIN_LOCK (&global_memory);
      memory_allocated -= mem_size;
      GSL_SPIN_UNLOCK (&global_memory);
    }
}

gpointer
gsl_alloc_memblock (gsize block_size)
{
  gpointer mem;
  gsize *debug_size;
  
  g_return_val_if_fail (block_size >= sizeof (gpointer), NULL);	/* cache-link size */

  mem = low_alloc (block_size + sizeof (*debug_size));
  debug_size = mem;
  *debug_size = block_size;
  mem = debug_size + 1;
  
  return mem;
}

void
gsl_free_memblock (gsize    block_size,
		   gpointer mem)
{
  gsize *debug_size;
  
  g_return_if_fail (mem != NULL);
  
  debug_size = mem;
  debug_size -= 1;
  mem = debug_size;
  g_return_if_fail (block_size == *debug_size);

  low_free (block_size + sizeof (*debug_size), mem);
}

void
gsl_alloc_report (void)
{
  g_message ("Gsl-Memory: %lu bytes currently used",
	     memory_allocated);
}

gpointer
gsl_alloc_memblock0 (gsize block_size)
{
  gpointer mem = gsl_alloc_memblock (block_size);
  
  memset (mem, 0, block_size);
  
  return mem;
}

static void
gsl_free_node_list (gpointer mem,
		    gsize    node_size)
{
  struct { gpointer next, data; } *tmp, *node = mem;

  g_return_if_fail (node != NULL);
  g_return_if_fail (node_size >= 2 * sizeof (gpointer));

  /* FIXME: this can be optimized to an O(1) operation with T-style links in mem-caches */
  do
    {
      tmp = node->next;

      gsl_free_memblock (node_size, node);
      node = tmp;
    }
  while (node);
}


/* --- ring (circular-list) --- */
static inline GslRing*
gsl_ring_prepend_i (GslRing  *head,
		    gpointer data)
{
  GslRing *ring = gsl_new_struct (GslRing, 1);
  
  ring->data = data;
  if (!head)
    {
      ring->prev = ring;
      ring->next = ring;
    }
  else
    {
      ring->prev = head->prev;
      ring->next = head;
      if (head->prev)
	head->prev->next = ring;
      head->prev = ring;
    }
  return ring;
}

GslRing*
gsl_ring_prepend (GslRing  *head,
		  gpointer data)
{
  return gsl_ring_prepend_i (head, data);
}

GslRing*
gsl_ring_prepend_uniq (GslRing  *head,
		       gpointer data)
{
  GslRing *walk;
  
  for (walk = head; walk; walk = gsl_ring_walk (head, walk))
    if (walk->data == data)
      return head;
  return gsl_ring_prepend_i (head, data);
}

GslRing*
gsl_ring_append (GslRing  *head,
		 gpointer data)
{
  GslRing *ring;
  
  ring = gsl_ring_prepend_i (head, data);
  
  return head ? head : ring;
}

GslRing*
gsl_ring_concat (GslRing *head1,
		 GslRing *head2)
{
  GslRing *tail1, *tail2;
  
  if (!head1)
    return head2;
  if (!head2)
    return head1;
  tail1 = head1->prev;
  tail2 = head2->prev;
  head1->prev = tail2;
  tail2->next = head1;
  head2->prev = tail1;
  tail1->next = head2;
  
  return head1;
}

GslRing*
gsl_ring_remove_node (GslRing *head,
		      GslRing *node)
{
  if (!head)
    g_return_val_if_fail (head == NULL && node == NULL, NULL);
  if (!head || !node)
    return NULL;
  
  /* special case one item ring */
  if (head->prev == head)
    {
      g_return_val_if_fail (node == head, head);
      
      gsl_delete_struct (GslRing, node);
      return NULL;
    }
  g_return_val_if_fail (node != node->next, head); /* node can't be a one item ring here */
  
  node->next->prev = node->prev;
  node->prev->next = node->next;
  if (head == node)
    head = node->next;
  gsl_delete_struct (GslRing, node);
  
  return head;
}

GslRing*
gsl_ring_remove (GslRing *head,
		 gpointer data)
{
  GslRing *walk;

  if (!head)
    return NULL;
  
  /* make tail data removal an O(1) operation */
  if (head->prev->data == data)
    return gsl_ring_remove_node (head, head->prev);
  
  for (walk = head; walk; walk = gsl_ring_walk (head, walk))
    if (walk->data == data)
      return gsl_ring_remove_node (head, walk);
  
  g_warning (G_STRLOC ": couldn't find data item (%p) to remove from ring (%p)", data, head);
  
  return head;
}

guint
gsl_ring_length (GslRing *head)
{
  GslRing *ring;
  guint i = 0;
  
  for (ring = head; ring; ring = gsl_ring_walk (head, ring))
    i++;

  return i;
}

GslRing*
gsl_ring_find (GslRing      *head,
	       gconstpointer data)
{
  GslRing *ring;

  for (ring = head; ring; ring = gsl_ring_walk (head, ring))
    if (ring->data == (gpointer) data)
      return ring;

  return NULL;
}

GslRing*
gsl_ring_nth (GslRing *head,
	      guint    n)
{
  GslRing *ring = head;

  while (n-- && ring)
    ring = gsl_ring_walk (head, ring);

  return ring;
}

gpointer
gsl_ring_nth_data (GslRing *head,
		   guint    n)
{
  GslRing *ring = head;

  while (n-- && ring)
    ring = gsl_ring_walk (head, ring);

  return ring ? ring->data : ring;
}

void
gsl_ring_free (GslRing *head)
{
  if (head)
    {
      head->prev->next = NULL;
      gsl_free_node_list (head, sizeof (*head));
    }
}

gpointer
gsl_ring_pop_head (GslRing **head_p)
{
  gpointer data;
  
  g_return_val_if_fail (head_p != NULL, NULL);
  
  if (!*head_p)
    return NULL;
  data = (*head_p)->data;
  *head_p = gsl_ring_remove_node (*head_p, *head_p);
  
  return data;
}

gpointer
gsl_ring_pop_tail (GslRing **head_p)
{
  gpointer data;
  
  g_return_val_if_fail (head_p != NULL, NULL);
  
  if (!*head_p)
    return NULL;
  data = (*head_p)->prev->data;
  *head_p = gsl_ring_remove_node (*head_p, (*head_p)->prev);
  
  return data;
}


/* --- GslThread --- */
static GslMutex global_thread;
static GslRing *global_thread_list = NULL;
static GslCond *global_thread_cond = NULL;
static GslRing *awake_tdata_list = NULL;

typedef struct
{
  GslThreadFunc func;
  gpointer      data;
  gint		wpipe[2];
  volatile gint abort;
  guint64       awake_stamp;
} ThreadData;

static inline ThreadData*
thread_data_from_gsl_thread (GslThread *thread)
{
  GThread *gthread = (GThread*) thread;

  return gthread->data;
}

static gpointer
thread_wrapper (gpointer arg)
{
  GslThread *self = gsl_thread_self ();
  ThreadData *tdata = arg;

  g_assert (tdata == thread_data_from_gsl_thread (gsl_thread_self ()));

  GSL_SYNC_LOCK (&global_thread);
  global_thread_list = gsl_ring_prepend (global_thread_list, self);
  gsl_cond_broadcast (global_thread_cond);
  GSL_SYNC_UNLOCK (&global_thread);

  tdata->func (tdata->data);

  GSL_SYNC_LOCK (&global_thread);
  global_thread_list = gsl_ring_remove (global_thread_list, self);
  if (tdata->awake_stamp)
    awake_tdata_list = gsl_ring_remove (awake_tdata_list, tdata);
  gsl_cond_broadcast (global_thread_cond);
  GSL_SYNC_UNLOCK (&global_thread);

  close (tdata->wpipe[0]);
  tdata->wpipe[0] = -1;
  close (tdata->wpipe[1]);
  tdata->wpipe[1] = -1;
  gsl_delete_struct (ThreadData, tdata);
  
  return NULL;
}

GslThread*
gsl_thread_new (GslThreadFunc func,
		gpointer      user_data)
{
  const gboolean joinable = TRUE;
  gpointer gthread = NULL;
  ThreadData *tdata;
  glong d_long;
  GError *gerror = NULL;
  gint error;

  g_return_val_if_fail (func != NULL, FALSE);

  tdata = gsl_new_struct0 (ThreadData, 1);
  tdata->func = func;
  tdata->data = user_data;
  tdata->wpipe[0] = -1;
  tdata->wpipe[1] = -1;
  tdata->abort = FALSE;
  error = pipe (tdata->wpipe);
  if (error == 0)
    {
      d_long = fcntl (tdata->wpipe[0], F_GETFL, 0);
      g_print ("pipe-readfd, blocking=%ld\n", d_long & O_NONBLOCK);
      d_long |= O_NONBLOCK;
      error = fcntl (tdata->wpipe[0], F_SETFL, d_long);
    }
  if (error == 0)
    {
      d_long = fcntl (tdata->wpipe[1], F_GETFL, 0);
      g_print ("pipe-writefd, blocking=%ld\n", d_long & O_NONBLOCK);
      d_long |= O_NONBLOCK;
      error = fcntl (tdata->wpipe[1], F_SETFL, d_long);
    }

  if (error == 0)
    gthread = g_thread_create_full (thread_wrapper, tdata, 0, joinable, FALSE,
				    G_THREAD_PRIORITY_NORMAL, &gerror);
  
  if (gthread)
    {
      GSL_SYNC_LOCK (&global_thread);
      while (!gsl_ring_find (global_thread_list, gthread))
	gsl_cond_wait (global_thread_cond, &global_thread);
      GSL_SYNC_UNLOCK (&global_thread);
    }
  else
    {
      close (tdata->wpipe[0]);
      close (tdata->wpipe[1]);
      gsl_delete_struct (ThreadData, tdata);
      g_warning ("Failed to create thread: %s", gerror->message);
      g_error_free (gerror);
    }

  return gthread;
}

GslThread*
gsl_thread_self (void)
{
  gpointer gthread = g_thread_self ();

  if (!gthread)
    g_error ("gsl_thread_self() failed");

  return gthread;
}

guint
gsl_threads_get_count (void)
{
  guint count;

  GSL_SYNC_LOCK (&global_thread);
  count = gsl_ring_length (global_thread_list);
  GSL_SYNC_UNLOCK (&global_thread);

  return count;
}

static void
thread_wakeup_I (ThreadData *tdata)
{
  guint8 data = 'W';
  gint r;

  do
    r = write (tdata->wpipe[1], &data, 1);
  while (r < 0 && (errno == EINTR || errno == ERESTART));
}

/**
 * gsl_thread_wakeup
 * @thread: thread to wake up
 * Wake up a currently sleeping thread. In practice, this
 * function simply causes the next call to gsl_thread_sleep()
 * within @thread to last for 0 seconds.
 */
void
gsl_thread_wakeup (GslThread *thread)
{
  ThreadData *tdata;

  g_return_if_fail (thread != NULL);

  GSL_SYNC_LOCK (&global_thread);
  g_assert (gsl_ring_find (global_thread_list, thread));
  GSL_SYNC_UNLOCK (&global_thread);

  tdata = thread_data_from_gsl_thread (thread);
  thread_wakeup_I (tdata);
}

/**
 * gsl_thread_abort
 * @thread: thread to abort
 * Abort a currently running thread. This function does not
 * return until the thread in question terminated execution.
 * Note that the thread handle gets invalidated with invocation
 * of gsl_thread_abort() or gsl_thread_queue_abort().
 */
void
gsl_thread_abort (GslThread *thread)
{
  ThreadData *tdata;
  
  g_return_if_fail (thread != NULL);
  
  GSL_SYNC_LOCK (&global_thread);
  g_assert (gsl_ring_find (global_thread_list, thread));
  GSL_SYNC_UNLOCK (&global_thread);

  tdata = thread_data_from_gsl_thread (thread);

  GSL_SYNC_LOCK (&global_thread);
  tdata->abort = TRUE;
  thread_wakeup_I (tdata);

  while (gsl_ring_find (global_thread_list, thread))
    gsl_cond_wait (global_thread_cond, &global_thread);
  GSL_SYNC_UNLOCK (&global_thread);
}

/**
 * gsl_thread_queue_abort
 * @thread: thread to abort
 * Same as gsl_thread_abort(), but returns as soon as possible,
 * even if thread hasn't stopped execution yet.
 * Note that the thread handle gets invalidated with invocation
 * of gsl_thread_abort() or gsl_thread_queue_abort().
 */
void
gsl_thread_queue_abort (GslThread *thread)
{
  ThreadData *tdata;
  
  g_return_if_fail (thread != NULL);

  GSL_SYNC_LOCK (&global_thread);
  g_assert (gsl_ring_find (global_thread_list, thread));
  GSL_SYNC_UNLOCK (&global_thread);

  tdata = thread_data_from_gsl_thread (thread);

  GSL_SYNC_LOCK (&global_thread);
  tdata->abort = TRUE;
  thread_wakeup_I (tdata);
  GSL_SYNC_UNLOCK (&global_thread);
}

/**
 * gsl_thread_aborted
 * @returns: %TRUE if the thread should abort execution
 * Find out if the currently running thread should be aborted (the thread is
 * supposed to return from its main thread function).
 */
gboolean
gsl_thread_aborted (void)
{
  ThreadData *tdata = thread_data_from_gsl_thread (gsl_thread_self ());
  gboolean aborted;

  GSL_SYNC_LOCK (&global_thread);
  aborted = tdata->abort != FALSE;
  GSL_SYNC_UNLOCK (&global_thread);

  return aborted;
}

/**
 * gsl_thread_sleep
 * @max_msec: maximum amount of milli seconds to sleep (-1 for infinite time)
 * @returns:  %TRUE if the thread should continue execution
 * Sleep for the amount of time given. This function may get interrupted
 * by wakeup or abort requests, it returns whether the thread is supposed
 * to continue execution after waking up.
 */
gboolean
gsl_thread_sleep (glong max_msec)
{
  ThreadData *tdata = thread_data_from_gsl_thread (gsl_thread_self ());
  struct pollfd pfd;
  gint r, aborted;

  pfd.fd = tdata->wpipe[0];
  pfd.events = GSL_POLLIN;
  pfd.revents = 0;

  r = poll (&pfd, 1, max_msec);

  if (r < 0 && errno != EINTR)
    g_message (G_STRLOC ": poll() error: %s\n", g_strerror (errno));
  else if (pfd.revents & GSL_POLLIN)
    {
      guint8 data[64];

      do
	r = read (tdata->wpipe[0], data, sizeof (data));
      while ((r < 0 && (errno == EINTR || errno == ERESTART)) || r == sizeof (data));
    }

  GSL_SYNC_LOCK (&global_thread);
  aborted = tdata->abort != FALSE;
  GSL_SYNC_UNLOCK (&global_thread);

  return !aborted;
}

void
gsl_thread_get_pollfd (GslPollFD *pfd)
{
  ThreadData *tdata = thread_data_from_gsl_thread (gsl_thread_self ());

  pfd->fd = tdata->wpipe[0];
  pfd->events = GSL_POLLIN;
  pfd->revents = 0;
}

/**
 * gsl_thread_awake_after
 * @tick_stamp: tick stamp update to trigger wakeup
 * Wakeup the currently running thread after the global tick stamp
 * (see gsl_tick_stamp()) has been updated to @tick_stamp.
 * (If the moment of wakeup has already passed by, the thread is
 * woken up at the next global tick stamp update.)
 */
void
gsl_thread_awake_after (guint64 tick_stamp)
{
  ThreadData *tdata = thread_data_from_gsl_thread (gsl_thread_self ());

  g_return_if_fail (tick_stamp > 0);

  GSL_SYNC_LOCK (&global_thread);
  if (!tdata->awake_stamp)
    {
      awake_tdata_list = gsl_ring_prepend (awake_tdata_list, tdata);
      tdata->awake_stamp = tick_stamp;
    }
  else
    tdata->awake_stamp = MIN (tdata->awake_stamp, tick_stamp);
  GSL_SYNC_UNLOCK (&global_thread);
}

/**
 * gsl_thread_awake_before
 * @tick_stamp: tick stamp update to trigger wakeup
 * Wakeup the currently running thread upon the last global tick stamp
 * update (see gsl_tick_stamp()) that happens prior to updating the
 * global tick stamp to @tick_stamp.
 * (If the moment of wakeup has already passed by, the thread is
 * woken up at the next global tick stamp update.)
 */
void
gsl_thread_awake_before (guint64 tick_stamp)
{
  g_return_if_fail (tick_stamp > 0);

  if (tick_stamp > global_tick_stamp_leaps)
    gsl_thread_awake_after (tick_stamp - global_tick_stamp_leaps);
  else
    gsl_thread_awake_after (tick_stamp);
}

/**
 * gsl_tick_stamp
 * @RETURNS: GSL's execution tick stamp as unsigned 64bit integer
 * Retrive the GSL global tick stamp.
 * GSL increments its global tick stamp at certain intervals,
 * by specific amounts (refer to gsl_engine_init() for further
 * details). The tick stamp is a non-wrapping, unsigned 64bit
 * integer greater than 0. Threads can schedule sleep interruptions
 * at certain tick stamps with gsl_thread_awake_after() and
 * gsl_thread_awake_before(). Tick stamp updating occours at
 * GSL engine block processing boundaries, so code that can
 * guarantee to not run across those boundaries (for instance
 * GslProcessFunc() functions) may use the macro %GSL_TICK_STAMP
 * to retrive the current tick in a faster manner (not involving
 * mutex locking). See also gsl_module_tick_stamp().
 */
guint64
gsl_tick_stamp (void)
{
  guint64 stamp;

  GSL_SYNC_LOCK (&global_thread);
  stamp = gsl_externvar_tick_stamp;
  GSL_SYNC_UNLOCK (&global_thread);

  return stamp;
}

void
_gsl_tick_stamp_set_leap (guint ticks)
{
  GSL_SYNC_LOCK (&global_thread);
  global_tick_stamp_leaps = ticks;
  GSL_SYNC_UNLOCK (&global_thread);
}

void
_gsl_tick_stamp_inc (void)
{
  volatile guint64 newstamp;
  GslRing *ring;

  g_return_if_fail (global_tick_stamp_leaps > 0);

  newstamp = gsl_externvar_tick_stamp + global_tick_stamp_leaps;

  GSL_SYNC_LOCK (&global_thread);
  gsl_externvar_tick_stamp = newstamp;
  for (ring = awake_tdata_list; ring; )
    {
      ThreadData *tdata = ring->data;

      if (tdata->awake_stamp <= GSL_TICK_STAMP)
	{
	  GslRing *next = gsl_ring_walk (awake_tdata_list, ring);

	  tdata->awake_stamp = 0;
	  awake_tdata_list = gsl_ring_remove (awake_tdata_list, tdata);

	  thread_wakeup_I (tdata);
	  ring = next;
	}
      else
	ring = gsl_ring_walk (awake_tdata_list, ring);
    }
  GSL_SYNC_UNLOCK (&global_thread);
}


/* --- GslMutex --- */
static gboolean is_smp_system = FALSE;

void
gsl_mutex_init (GslMutex *mutex)
{
  g_return_if_fail (mutex != NULL);
  
  mutex->mutex_pointer = g_mutex_new ();
}

void
gsl_mutex_spin_lock (GslMutex *mutex)
{
  g_return_if_fail (mutex != NULL);

  /* spin locks should be held only very short times,
   * so frequently we should succeed here
   */
  if (g_mutex_trylock (mutex->mutex_pointer))
    return;

  if (!is_smp_system)
    {
      /* on uni processor systems, there's no point in busy spinning */
      do
	{
	  sched_yield ();
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

void
gsl_mutex_sync_lock (GslMutex *mutex)
{
  g_return_if_fail (mutex != NULL);

  /* syncronization locks should either require us to wait
   * on another thread or are unlocked already.
   */
  if (g_mutex_trylock (mutex->mutex_pointer))
    return;

  /* already locked, share processor */
  sched_yield ();

  /* sharing didn't help, perform lengthy wait in mutex_lock() */
  g_mutex_lock (mutex->mutex_pointer);
}

void
gsl_mutex_unlock (GslMutex *mutex)
{
  g_return_if_fail (mutex != NULL);

  g_mutex_unlock (mutex->mutex_pointer);
}

void
gsl_mutex_destroy (GslMutex *mutex)
{
  g_return_if_fail (mutex != NULL);

  g_mutex_free (mutex->mutex_pointer);
  memset (mutex, 0, sizeof (*mutex));
}


/* --- GslRecMutex --- */
void
gsl_rec_mutex_init (GslRecMutex *rec_mutex)
{
  g_return_if_fail (rec_mutex != NULL);

  rec_mutex->depth = 0;
  rec_mutex->owner = NULL;
  gsl_mutex_init (&rec_mutex->sync_mutex);
}

void
gsl_rec_mutex_destroy (GslRecMutex *rec_mutex)
{
  g_return_if_fail (rec_mutex != NULL);

  if (rec_mutex->owner || rec_mutex->depth)
    {
      g_warning (G_STRLOC ": recursive mutex not unlocked during destruction");
      return;
    }
  gsl_mutex_destroy (&rec_mutex->sync_mutex);
  g_assert (rec_mutex->owner == NULL && rec_mutex->depth == 0);
}

void
gsl_rec_mutex_lock (GslRecMutex *rec_mutex)
{
  gpointer self = gsl_thread_self ();

  g_return_if_fail (rec_mutex != NULL);

  if (rec_mutex->owner == self)
    {
      g_assert (rec_mutex->depth > 0);  /* paranoid */
      rec_mutex->depth += 1;
    }
  else
    {
      GSL_SYNC_LOCK (&rec_mutex->sync_mutex);
      g_assert (rec_mutex->owner == NULL && rec_mutex->depth == 0); /* paranoid */
      rec_mutex->owner = self;
      rec_mutex->depth = 1;
    }
}

void
gsl_rec_mutex_unlock (GslRecMutex *rec_mutex)
{
  gpointer self = gsl_thread_self ();

  g_return_if_fail (rec_mutex != NULL);

  g_assert (rec_mutex->owner == self && rec_mutex->depth > 0);
  rec_mutex->depth -= 1;
  if (!rec_mutex->depth)
    {
      rec_mutex->owner = NULL;
      GSL_SYNC_UNLOCK (&rec_mutex->sync_mutex);
    }
}

gboolean
gsl_rec_mutex_test_self (GslRecMutex *rec_mutex)
{
  gpointer self = gsl_thread_self ();

  g_return_val_if_fail (rec_mutex != NULL, FALSE);

  return rec_mutex->owner == self && rec_mutex->depth > 0;
}


/* --- GslCond --- */
GslCond*
gsl_cond_new (void)
{
  gpointer gcond = g_cond_new ();

  return gcond;
}

void
gsl_cond_destroy (GslCond *cond)
{
  gpointer gcond = cond;

  g_return_if_fail (cond != NULL);

  g_cond_free (gcond);
}

#if 0
void
gsl_cond_wait_timed (GslCond  *cond,
		     GslMutex *mutex,
		     glong     max_useconds)
{
  gpointer gmutex, gcond = cond;

  g_return_if_fail (cond != NULL);
  g_return_if_fail (mutex != NULL);

  gmutex = mutex->mutex_pointer;

  if (max_useconds < 0)
    g_cond_wait (gcond, gmutex);
  else
    {
      glong my_sec;
      GTimeVal gtime;

      /* for some reason, g_cond_timed_wait() (pthread_cond_timedwait) insists
       * on getting absolute timevalues ;(
       */
      g_get_current_time (&gtime);
      my_sec = max_useconds / G_USEC_PER_SEC;
      gtime.tv_sec += my_sec;
      gtime.tv_usec += max_useconds - my_sec * G_USEC_PER_SEC;
      if (gtime.tv_usec >= G_USEC_PER_SEC)
	{
	  gtime.tv_usec -= G_USEC_PER_SEC;
	  gtime.tv_sec += 1;
	}

      /* for linux on x86 with pthread_cond_timedwait(), this has 10ms
       * resolution
       */
      g_cond_timed_wait (gcond, gmutex, &gtime);
    }
}
#endif

void
gsl_cond_wait (GslCond  *cond,
	       GslMutex *mutex)
{
  gpointer gmutex, gcond = cond;

  g_return_if_fail (cond != NULL);
  g_return_if_fail (mutex != NULL);

  gmutex = mutex->mutex_pointer;

  /* infinite wait */
  g_cond_wait (gcond, gmutex);
}

void
gsl_cond_signal (GslCond *cond)
{
  gpointer gcond = cond;

  g_return_if_fail (cond != NULL);

  g_cond_signal (gcond);
}

void
gsl_cond_broadcast (GslCond *cond)
{
  gpointer gcond = cond;

  g_return_if_fail (cond != NULL);

  g_cond_broadcast (gcond);
}


/* --- GslMessage --- */
const gchar*
gsl_strerror (GslErrorType error)
{
  switch (error)
    {
    case GSL_ERROR_NONE:		return "Everything went well";
    case GSL_ERROR_INTERNAL:		return "Internal error (please report)";
    case GSL_ERROR_LAST:
    case GSL_ERROR_UNKNOWN:		return "Unknown error";
    case GSL_ERROR_IO:			return "I/O error";
    case GSL_ERROR_PERMS:		return "Insufficient permission";
    case GSL_ERROR_NOT_FOUND:		return "Not found";
    case GSL_ERROR_OPEN_FAILED:		return "Open failed";
    case GSL_ERROR_SEEK_FAILED:		return "Seek failed";
    case GSL_ERROR_READ_FAILED:		return "Read failed";
    case GSL_ERROR_WRITE_FAILED:	return "Write failed";
    case GSL_ERROR_EOF:			return "File empty or premature EOF";
    case GSL_ERROR_FORMAT_INVALID:	return "Invalid format";
    case GSL_ERROR_FORMAT_UNKNOWN:	return "Unknown format";
    case GSL_ERROR_DATA_CORRUPT:        return "data corrupt";
    case GSL_ERROR_CONTENT_GLITCH:      return "data glitch (junk) detected";
    case GSL_ERROR_CODEC_FAILURE:	return "CODEC failure";
    default:				return NULL;
    }
}

void
gsl_message_send (const gchar *reporter,
		  GslErrorType error,
		  const gchar *messagef,
		  ...)
{
  struct {
    gchar        reporter[1024];
    GslErrorType error;
    const gchar	*error_str;
    gchar	 message[1024];
  } tmsg, *msg = &tmsg;
  gchar *tmp;
  va_list args;
    
  g_return_if_fail (reporter != NULL);
  g_return_if_fail (messagef != NULL);

  strncpy (msg->reporter, reporter, 1023);
  msg->reporter[1023] = 0;

  msg->error = error;
  msg->error_str = error ? gsl_strerror (msg->error) : NULL;

  /* vsnprintf() replacement */
  va_start (args, messagef);
  tmp = g_strdup_vprintf (messagef, args);
  va_end (args);
  strncpy (msg->message, tmp, 1023);
  msg->message[1023] = 0;
  g_free (tmp);

  /* always puke the message to stderr */
  if (msg->error_str)
    g_printerr ("GSL **: %s: %s: %s\n", msg->reporter, msg->message, msg->error_str);
  else
    g_printerr ("GSL **: %s: %s\n", msg->reporter, msg->message);

  /* in current lack of a decent message queue, do nothing here */
  ;
}


/* --- misc --- */
const gchar*
gsl_byte_order_to_string (guint byte_order)
{
  g_return_val_if_fail (byte_order == G_LITTLE_ENDIAN || byte_order == G_BIG_ENDIAN, NULL);

  if (byte_order == G_LITTLE_ENDIAN)
    return "little_endian";
  if (byte_order == G_BIG_ENDIAN)
    return "big_endian";

  return NULL;
}

guint
gsl_byte_order_from_string (const gchar *string)
{
  g_return_val_if_fail (string != NULL, 0);

  while (*string == ' ')
    string++;
  if (strncasecmp (string, "little", 6) == 0)
    return G_LITTLE_ENDIAN;
  if (strncasecmp (string, "big", 3) == 0)
    return G_BIG_ENDIAN;
  return 0;
}

GslErrorType
gsl_check_file (const gchar *file_name,
		const gchar *mode)
{
  guint access_mask = 0;
  guint check_file, check_dir, check_link;
  
  if (strchr (mode, 'r'))	/* readable */
    access_mask |= R_OK;
  if (strchr (mode, 'w'))	/* writable */
    access_mask |= W_OK;
  if (strchr (mode, 'x'))	/* executable */
    access_mask |= X_OK;

  if (access_mask && access (file_name, access_mask) < 0)
    goto have_errno;
  
  check_file = strchr (mode, 'f') != NULL;	/* open as file */
  check_dir  = strchr (mode, 'd') != NULL;	/* open as directory */
  check_link = strchr (mode, 'l') != NULL;	/* open as link */

  if (check_file || check_dir || check_link)
    {
      struct stat st;
      
      if (stat (file_name, &st) < 0)
	goto have_errno;
      
      if ((check_file && !S_ISREG (st.st_mode)) ||
	  (check_dir && !S_ISDIR (st.st_mode)) ||
	  (check_link && !S_ISLNK (st.st_mode)))
	return GSL_ERROR_OPEN_FAILED;
    }

  return GSL_ERROR_NONE;
  
 have_errno:
  switch (errno)
    {
    case ELOOP:
    case ENAMETOOLONG:
    case ENOENT:	return GSL_ERROR_NOT_FOUND;
    case EROFS:
    case EPERM:
    case EACCES:	return GSL_ERROR_PERMS;
    case EIO:		return GSL_ERROR_IO;
    default:		return GSL_ERROR_OPEN_FAILED;
    }
}


/* --- global initialization --- */
static guint
get_n_processors (void)
{
#ifdef _SC_NPROCESSORS_ONLN
  {
    gint n = sysconf (_SC_NPROCESSORS_ONLN);

    if (n > 0)
      return n;
  }
#endif
  return 1;
}

static const GslConfig *gsl_config = NULL;

const GslConfig*
gsl_get_config (void)
{
  return gsl_config;
}

#define	ROUND(dblval)	((GslLong) ((dblval) + .5))

void
gsl_init (const GslConfigValue values[])
{
  const GslConfigValue *config = values;
  static GslConfig pconfig = {	/* DEFAULTS */
    1,				/* n_processors */
    2,				/* wave_chunk_padding */
    4,				/* wave_chunk_big_pad */
    512,			/* dcache_block_size */
    1024 * 1024,		/* dcache_cache_memory */
    57,				/* midi_kammer_note */
    440,			/* kammer_freq */
  };

  if (gsl_config)	/* ignore multiple invocations */
    return;
  g_assert (gsl_config++ == NULL);	/* dumb concurrency prevention */

  gsl_externvar_tick_stamp = 1;

  /* configure permanent config record */
  if (config)
    while (config->value_name)
      {
	if (strcmp ("wave_chunk_padding", config->value_name) == 0)
	  pconfig.wave_chunk_padding = ROUND (config->value);
	else if (strcmp ("wave_chunk_big_pad", config->value_name) == 0)
	  pconfig.wave_chunk_big_pad = ROUND (config->value);
	else if (strcmp ("dcache_cache_memory", config->value_name) == 0)
	  pconfig.dcache_cache_memory = ROUND (config->value);
	else if (strcmp ("dcache_block_size", config->value_name) == 0)
	  pconfig.dcache_block_size = ROUND (config->value);
	else if (strcmp ("midi_kammer_note", config->value_name) == 0)
	  pconfig.midi_kammer_note = ROUND (config->value);
	else if (strcmp ("kammer_freq", config->value_name) == 0)
	  pconfig.kammer_freq = config->value;
	config++;
      }
  
  /* constrain (user) config */
  pconfig.wave_chunk_padding = MAX (1, pconfig.wave_chunk_padding);
  pconfig.wave_chunk_big_pad = MAX (2 * pconfig.wave_chunk_padding, pconfig.wave_chunk_big_pad);
  pconfig.dcache_block_size = MAX (2 * pconfig.wave_chunk_big_pad + sizeof (GslDataType), pconfig.dcache_block_size);
  pconfig.dcache_block_size = gsl_alloc_upper_power2 (pconfig.dcache_block_size - 1);
  /* pconfig.dcache_cache_memory = gsl_alloc_upper_power2 (pconfig.dcache_cache_memory); */

  /* non-configurable config updates */
  pconfig.n_processors = get_n_processors ();

  /* export GSL configuration */
  gsl_config = &pconfig;

  /* initialize subsystems */
  is_smp_system = GSL_CONFIG (n_processors) > 1;
  gsl_mutex_init (&global_memory);
  gsl_mutex_init (&global_thread);
  global_thread_cond = gsl_cond_new ();
  _gsl_init_data_handles ();
  _gsl_init_data_caches ();
  _gsl_init_engine_utils ();
  _gsl_init_loader_gslwave ();
  _gsl_init_loader_wav ();
  _gsl_init_loader_oggvorbis ();
}
