/* GSL - Generic Sound Layer
 * Copyright (C) 2001-2002 Tim Janik and Stefan Westerfeld
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
#include <sys/time.h>

/* some systems don't have ERESTART (which is what linux returns for system
 * calls on pipes which are being interrupted). most propably just use EINTR,
 * and maybe some can return both. so we check for both in the below code,
 * and alias ERESTART to EINTR if it's not present. compilers are supposed
 * to catch and optimize the doubled check arising from this.
 */
#ifndef ERESTART
#define ERESTART        EINTR
#endif


#define	PREALLOC		(8)
#define	SIMPLE_CACHE_SIZE	(64)
#define	TS8_SIZE		(MAX (sizeof (GTrashStack), 8))
#define	DBG8_SIZE		(MAX (sizeof (gsize), 8))


/* --- variables --- */
volatile guint64     gsl_externvar_tick_stamp = 0;
static guint64	     tick_stamp_system_time = 0;
static guint         global_tick_stamp_leaps = 0;
static GslDebugFlags gsl_debug_flags = 0;


/* --- memory allocation --- */
static GslMutex     global_memory = { 0, };
static GTrashStack *simple_cache[SIMPLE_CACHE_SIZE] = { 0, 0, 0, /* ... */ };
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

  if (mem_size >= TS8_SIZE && mem_size / 8 < SIMPLE_CACHE_SIZE)
    {
      guint cell;

      mem_size = (mem_size + 7) & ~0x7;
      cell = (mem_size >> 3) - 1;
      GSL_SPIN_LOCK (&global_memory);
      mem = g_trash_stack_pop (simple_cache + cell);
      GSL_SPIN_UNLOCK (&global_memory);
      if (!mem)
	{
	  guint8 *cache_mem = g_malloc (mem_size * PREALLOC);
	  guint i;
	  
	  GSL_SPIN_LOCK (&global_memory);
	  memory_allocated += mem_size * PREALLOC;
	  for (i = 0; i < PREALLOC - 1; i++)
	    {
	      g_trash_stack_push (simple_cache + cell, cache_mem);
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
  if (mem_size >= TS8_SIZE && mem_size / 8 < SIMPLE_CACHE_SIZE)
    {
      guint cell;

      mem_size = (mem_size + 7) & ~0x7;
      cell = (mem_size >> 3) - 1;
      GSL_SPIN_LOCK (&global_memory);
      g_trash_stack_push (simple_cache + cell, mem);
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
  guint8 *cmem;
  gsize *debug_size;
  
  g_return_val_if_fail (block_size >= sizeof (gpointer), NULL);	/* cache-link size */

  cmem = low_alloc (block_size + DBG8_SIZE);
  debug_size = (gsize*) cmem;
  *debug_size = block_size;
  cmem += DBG8_SIZE;

  return cmem;
}

void
gsl_free_memblock (gsize    block_size,
		   gpointer mem)
{
  gsize *debug_size;
  guint8 *cmem;

  g_return_if_fail (mem != NULL);
  
  cmem = mem;
  cmem -= DBG8_SIZE;
  debug_size = (gsize*) cmem;
  g_return_if_fail (block_size == *debug_size);

  low_free (block_size + DBG8_SIZE, cmem);
}

void
gsl_alloc_report (void)
{
  guint cell, cached = 0;

  GSL_SPIN_LOCK (&global_memory);
  for (cell = 0; cell < SIMPLE_CACHE_SIZE; cell++)
    {
      GTrashStack *trash = simple_cache[cell];
      guint memsize, n = 0;

      while (trash)
	{
	  n++;
	  trash = trash->next;
	}

      if (n)
	{
	  memsize = (cell + 1) << 3;
	  g_message ("cell %4u): %u bytes in %u nodes", memsize, memsize * n, n);
	  cached += memsize * n;
	}
    }
  g_message ("%lu bytes allocated from system, %u bytes unused in cache", memory_allocated, cached);
  GSL_SPIN_UNLOCK (&global_memory);
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

GslRing*
gsl_ring_insert_sorted (GslRing	    *head,
			gpointer     data,
			GCompareFunc func)
{
  gint cmp;

  g_return_val_if_fail (func != NULL, head);

  if (!head)
    return gsl_ring_prepend (head, data);

  /* typedef gint (*GCompareFunc) (gconstpointer a,
   *                               gconstpointer b);
   */
  cmp = func (data, head->data);

  if (cmp >= 0)	/* insert after head */
    {
      GslRing *tmp, *tail = head->prev;
      
      /* make appending an O(1) operation */
      if (head == tail || func (data, tail->data) >= 0)
	return gsl_ring_append (head, data);

      /* walk forward while data >= tmp (skipping equal nodes) */
      for (tmp = head->next; tmp != tail; tmp = tmp->next)
	if (func (data, tmp->data) < 0)
	  break;

      /* insert before sibling which is greater than data */
      gsl_ring_prepend (tmp, data);	/* keep current head */
      return head;
    }
  else /* cmp < 0 */
    return gsl_ring_prepend (head, data);
}


/* --- GslThread --- */
typedef struct
{
  GslThreadFunc func;
  gpointer      data;
  gint		wpipe[2];
  volatile gint abort;
  guint64       awake_stamp;
  GslDebugFlags auxlog_reporter;
  const gchar  *auxlog_section;
} ThreadData;
static GslMutex    global_thread = { 0, };
static GslRing    *global_thread_list = NULL;
static GslCond     global_thread_cond = { 0, };
static GslRing    *awake_tdata_list = NULL;
static ThreadData *main_thread_tdata = NULL;
static GslThread  *main_thread = NULL;

static inline ThreadData*
thread_data_from_gsl_thread (GslThread *thread)
{
  GThread *gthread = (GThread*) thread;

  /* if gthread->data==NULL, we assume this is the main thread */

  return gthread->data ? gthread->data : main_thread_tdata;
}

static gpointer
thread_wrapper (gpointer arg)
{
  GslThread *self = gsl_thread_self ();
  ThreadData *tdata = arg;

  g_assert (tdata == thread_data_from_gsl_thread (gsl_thread_self ()));

  GSL_SYNC_LOCK (&global_thread);
  global_thread_list = gsl_ring_prepend (global_thread_list, self);
  gsl_cond_broadcast (&global_thread_cond);
  GSL_SYNC_UNLOCK (&global_thread);

  tdata->func (tdata->data);

  GSL_SYNC_LOCK (&global_thread);
  global_thread_list = gsl_ring_remove (global_thread_list, self);
  if (tdata->awake_stamp)
    awake_tdata_list = gsl_ring_remove (awake_tdata_list, tdata);
  gsl_cond_broadcast (&global_thread_cond);
  GSL_SYNC_UNLOCK (&global_thread);

  close (tdata->wpipe[0]);
  tdata->wpipe[0] = -1;
  close (tdata->wpipe[1]);
  tdata->wpipe[1] = -1;
  gsl_delete_struct (ThreadData, tdata);
  
  return NULL;
}

static ThreadData*
create_tdata (void)
{
  ThreadData *tdata;
  glong d_long;
  gint error;

  tdata = gsl_new_struct0 (ThreadData, 1);
  tdata->func = NULL;
  tdata->data = NULL;
  tdata->wpipe[0] = -1;
  tdata->wpipe[1] = -1;
  tdata->abort = FALSE;
  tdata->auxlog_reporter = 0;
  tdata->auxlog_section = NULL;
  error = pipe (tdata->wpipe);
  if (error == 0)
    {
      d_long = fcntl (tdata->wpipe[0], F_GETFL, 0);
      /* g_printerr ("pipe-readfd, blocking=%ld\n", d_long & O_NONBLOCK); */
      d_long |= O_NONBLOCK;
      error = fcntl (tdata->wpipe[0], F_SETFL, d_long);
    }
  if (error == 0)
    {
      d_long = fcntl (tdata->wpipe[1], F_GETFL, 0);
      /* g_printerr ("pipe-writefd, blocking=%ld\n", d_long & O_NONBLOCK); */
      d_long |= O_NONBLOCK;
      error = fcntl (tdata->wpipe[1], F_SETFL, d_long);
    }
  if (error)
    {
      close (tdata->wpipe[0]);
      close (tdata->wpipe[1]);
      gsl_delete_struct (ThreadData, tdata);
      tdata = NULL;
    }
  return tdata;
}

GslThread*
gsl_thread_new (GslThreadFunc func,
		gpointer      user_data)
{
  gpointer gthread = NULL;
  ThreadData *tdata;
  GError *gerror = NULL;

  g_return_val_if_fail (func != NULL, FALSE);

  tdata = create_tdata ();

  if (tdata)
    {
      const gboolean joinable = FALSE;

      /* don't dare setting joinable to TRUE, that prevents the thread's
       * resources from being freed, since we don't offer pthread_join().
       * so we'd just rn out of stack at some point.
       */
      tdata->func = func;
      tdata->data = user_data;
      gthread = g_thread_create_full (thread_wrapper, tdata, 0, joinable, FALSE,
				      G_THREAD_PRIORITY_NORMAL, &gerror);
    }

  if (gthread)
    {
      GSL_SYNC_LOCK (&global_thread);
      while (!gsl_ring_find (global_thread_list, gthread))
	gsl_cond_wait (&global_thread_cond, &global_thread);
      GSL_SYNC_UNLOCK (&global_thread);
    }
  else
    {
      if (tdata)
	{
	  close (tdata->wpipe[0]);
	  close (tdata->wpipe[1]);
	  gsl_delete_struct (ThreadData, tdata);
	}
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

GslThread*
gsl_thread_main (void)
{
  return main_thread;
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
  g_return_if_fail (thread != main_thread);
  
  GSL_SYNC_LOCK (&global_thread);
  g_assert (gsl_ring_find (global_thread_list, thread));
  GSL_SYNC_UNLOCK (&global_thread);

  tdata = thread_data_from_gsl_thread (thread);

  GSL_SYNC_LOCK (&global_thread);
  tdata->abort = TRUE;
  thread_wakeup_I (tdata);

  while (gsl_ring_find (global_thread_list, thread))
    gsl_cond_wait (&global_thread_cond, &global_thread);
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
  g_return_if_fail (thread != main_thread);
  
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
 * to continue execution after waking up. This function also processes
 * remaining data from the thread's poll fd.
 */
gboolean
gsl_thread_sleep (glong max_msec)
{
  ThreadData *tdata = thread_data_from_gsl_thread (gsl_thread_self ());
  struct pollfd pfd;
  gint r, aborted;

  pfd.fd = tdata->wpipe[0];
  pfd.events = G_IO_IN;
  pfd.revents = 0;

  r = poll (&pfd, 1, max_msec);

  if (r < 0 && errno != EINTR)
    g_message (G_STRLOC ": poll() error: %s\n", g_strerror (errno));
  else if (pfd.revents & G_IO_IN)
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

/**
 * gsl_thread_awake_after
 * RETURNS: GPollFD for the current thread
 * Get the GPollfd for the current thread which is used
 * to signal thread wakeups (e.g. due to
 * gsl_thread_abort() or gsl_thread_wakeup()).
 */
void
gsl_thread_get_pollfd (GPollFD *pfd)
{
  ThreadData *tdata = thread_data_from_gsl_thread (gsl_thread_self ());

  pfd->fd = tdata->wpipe[0];
  pfd->events = G_IO_IN;
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
 *
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
 * This function is MT-safe and may be called from any thread.
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

/**
 * gsl_time_system
 * @RETURNS: Current system time in micro seconds
 *
 * Get the current system time in micro seconds.
 * Subsequent calls to this function do not necessarily
 * return growing values. In fact, a second call may return
 * a value smaller than the first call under certainsystem
 * conditions.
 * This function is MT-safe and may be called from any thread.
 */
guint64
gsl_time_system (void)
{
  struct timeval tv;
  guint64 csys_time;
  gint error;

  error = gettimeofday (&tv, NULL);
  if (error)
    g_error ("gettimeofday() failed: %s", g_strerror (errno));
  csys_time = tv.tv_sec;
  csys_time = csys_time * 1000000 + tv.tv_usec;

  return csys_time;
}

/**
 * gsl_tick_stamp_last
 * @RETURNS: Current tick stamp and system time in micro seconds
 *
 * Get the system time of the last GSL global tick stamp update.
 * This function is MT-safe and may be called from any thread.
 */
GslTickStampUpdate
gsl_tick_stamp_last (void)
{
  GslTickStampUpdate ustamp;

  GSL_SYNC_LOCK (&global_thread);
  ustamp.tick_stamp = gsl_externvar_tick_stamp;
  ustamp.system_time = tick_stamp_system_time;
  GSL_SYNC_UNLOCK (&global_thread);

  return ustamp;
}

void
_gsl_tick_stamp_inc (void)
{
  volatile guint64 newstamp;
  GslRing *ring;
  guint64 systime;

  g_return_if_fail (global_tick_stamp_leaps > 0);

  systime = gsl_time_system ();
  newstamp = gsl_externvar_tick_stamp + global_tick_stamp_leaps;

  GSL_SYNC_LOCK (&global_thread);
  gsl_externvar_tick_stamp = newstamp;
  tick_stamp_system_time = systime;
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

static void
default_mutex_init (GslMutex *mutex)
{
  g_return_if_fail (mutex != NULL);
  
  mutex->mutex_pointer = g_mutex_new ();
}

static int
default_mutex_trylock (GslMutex *mutex)
{
  return g_mutex_trylock (mutex->mutex_pointer) ? 0 : -1;
}

static void
default_mutex_lock (GslMutex *mutex)
{
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

static void
default_mutex_unlock (GslMutex *mutex)
{
  g_mutex_unlock (mutex->mutex_pointer);
}

static void
default_mutex_destroy (GslMutex *mutex)
{
  g_mutex_free (mutex->mutex_pointer);
  memset (mutex, 0, sizeof (*mutex));
}

static void
default_rec_mutex_init (GslRecMutex *rec_mutex)
{
  rec_mutex->depth = 0;
  rec_mutex->owner = NULL;
  gsl_mutex_init (&rec_mutex->sync_mutex);
}

static int
default_rec_mutex_trylock (GslRecMutex *rec_mutex)
{
  gpointer self = gsl_thread_self ();

  if (rec_mutex->owner == self)
    {
      g_assert (rec_mutex->depth > 0);  /* paranoid */
      rec_mutex->depth += 1;
      return 0;
    }
  else
    {
      if (gsl_mutex_trylock (&rec_mutex->sync_mutex))
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
default_rec_mutex_lock (GslRecMutex *rec_mutex)
{
  gpointer self = gsl_thread_self ();

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

static void
default_rec_mutex_unlock (GslRecMutex *rec_mutex)
{
  gpointer self = gsl_thread_self ();

  if (rec_mutex->owner == self && rec_mutex->depth > 0)
    {
      rec_mutex->depth -= 1;
      if (!rec_mutex->depth)
	{
	  rec_mutex->owner = NULL;
	  GSL_SYNC_UNLOCK (&rec_mutex->sync_mutex);
	}
    }
  else
    g_warning ("unable to unlock recursive mutex with self %p != %p or depth %u < 1",
	       rec_mutex->owner, self, rec_mutex->depth);
}

static void
default_rec_mutex_destroy (GslRecMutex *rec_mutex)
{
  if (rec_mutex->owner || rec_mutex->depth)
    {
      g_warning (G_STRLOC ": recursive mutex still locked during destruction");
      return;
    }
  gsl_mutex_destroy (&rec_mutex->sync_mutex);
  g_assert (rec_mutex->owner == NULL && rec_mutex->depth == 0);
}

static void
default_cond_init (GslCond *cond)
{
  cond->cond_pointer = g_cond_new ();
}

static void
default_cond_wait (GslCond  *cond,
		   GslMutex *mutex)
{
  /* infinite wait */
  g_cond_wait (cond->cond_pointer, mutex->mutex_pointer);
}

static void
default_cond_signal (GslCond *cond)
{
  g_cond_signal (cond->cond_pointer);
}

static void
default_cond_broadcast (GslCond *cond)
{
  g_cond_broadcast (cond->cond_pointer);
}

static void
default_cond_destroy (GslCond *cond)
{
  g_cond_free (cond->cond_pointer);
}

static void
default_cond_wait_timed (GslCond  *cond,
			 GslMutex *mutex,
			 gulong    abs_secs,
			 gulong    abs_usecs)
{
  GTimeVal gtime;

  gtime.tv_sec = abs_secs;
  gtime.tv_usec = abs_usecs;
  g_cond_timed_wait (cond->cond_pointer, mutex->mutex_pointer, &gtime);
}

GslMutexTable gsl_mutex_table = {
  default_mutex_init,
  default_mutex_lock,
  default_mutex_trylock,
  default_mutex_unlock,
  default_mutex_destroy,
  default_rec_mutex_init,
  default_rec_mutex_lock,
  default_rec_mutex_trylock,
  default_rec_mutex_unlock,
  default_rec_mutex_destroy,
  default_cond_init,
  default_cond_signal,
  default_cond_broadcast,
  default_cond_wait,
  default_cond_wait_timed,
  default_cond_destroy,
};

void
gsl_cond_wait_timed (GslCond  *cond,
		     GslMutex *mutex,
		     glong     max_useconds)
{
  if (max_useconds < 0)
    gsl_cond_wait (cond, mutex);
  else
    {
      struct timeval now;
      glong secs;

      gettimeofday (&now, NULL);
      secs = max_useconds / 1000000;
      now.tv_sec += secs;
      max_useconds -= secs * 1000000;
      now.tv_usec += max_useconds;
      if (now.tv_usec >= 1000000)
	{
	  now.tv_usec -= 1000000;
	  now.tv_sec += 1;
	}

      /* linux on x86 with pthread has actually 10ms resolution */
      gsl_mutex_table.cond_wait_timed (cond, mutex, now.tv_sec, now.tv_usec);
    }
}


/* --- GslMessage --- */
const gchar*
gsl_strerror (GslErrorType error)
{
  switch (error)
    {
    case GSL_ERROR_NONE:		return "Everything went well";
    case GSL_ERROR_INTERNAL:		return "Internal error (please report)";
    case GSL_ERROR_UNKNOWN:		return "Unknown error";
    case GSL_ERROR_IO:			return "I/O error";
    case GSL_ERROR_PERMS:		return "Insufficient permission";
    case GSL_ERROR_BUSY:		return "Resource currently busy";
    case GSL_ERROR_EXISTS:		return "Resource exists already";
    case GSL_ERROR_TEMP:		return "Temporary error";
    case GSL_ERROR_EOF:			return "File empty or premature EOF";
    case GSL_ERROR_NOT_FOUND:		return "Resource not found";
    case GSL_ERROR_OPEN_FAILED:		return "Open failed";
    case GSL_ERROR_SEEK_FAILED:		return "Seek failed";
    case GSL_ERROR_READ_FAILED:		return "Read failed";
    case GSL_ERROR_WRITE_FAILED:	return "Write failed";
    case GSL_ERROR_FORMAT_INVALID:	return "Invalid format";
    case GSL_ERROR_FORMAT_UNKNOWN:	return "Unknown format";
    case GSL_ERROR_DATA_CORRUPT:        return "Data corrupt";
    case GSL_ERROR_CONTENT_GLITCH:      return "Data glitch (junk) detected";
    case GSL_ERROR_NO_RESOURCE:		return "Out of memory, disk space or similar resource";
    case GSL_ERROR_CODEC_FAILURE:	return "CODEC failure";
    default:				return NULL;
    }
}

static const GDebugKey gsl_static_debug_keys[] = {
  { "notify",         GSL_MSG_NOTIFY },
  { "dcache",         GSL_MSG_DATA_CACHE },
  { "dhandle",        GSL_MSG_DATA_HANDLE },
  { "loader",         GSL_MSG_LOADER },
  { "osc",	      GSL_MSG_OSC },
  { "engine",         GSL_MSG_ENGINE },
  { "jobs",           GSL_MSG_JOBS },
  { "fjobs",          GSL_MSG_FJOBS },
  { "sched",          GSL_MSG_SCHED },
  { "master",         GSL_MSG_MASTER },
  { "slave",          GSL_MSG_SLAVE },
};

static const gchar*
reporter_name (GslDebugFlags reporter)
{
  switch (reporter)
    {
    case GSL_MSG_NOTIFY:	return "Notify";
    case GSL_MSG_DATA_CACHE:	return "DataCache";
    case GSL_MSG_DATA_HANDLE:	return "DataHandle";
    case GSL_MSG_LOADER:	return "Loader";
    case GSL_MSG_OSC:		return "Oscillator";
    case GSL_MSG_ENGINE:	return "Engine";	/* Engine */
    case GSL_MSG_JOBS:		return "Jobs";		/* Engine */
    case GSL_MSG_FJOBS:		return "FlowJobs";	/* Engine */
    case GSL_MSG_SCHED:		return "Sched";		/* Engine */
    case GSL_MSG_MASTER:	return "Master";	/* Engine */
    case GSL_MSG_SLAVE:		return "Slave";		/* Engine */
    default:			return "Custom";
    }
}

const GDebugKey *gsl_debug_keys = gsl_static_debug_keys;
const guint      gsl_n_debug_keys = G_N_ELEMENTS (gsl_static_debug_keys);

void
gsl_message_send (GslDebugFlags reporter,
		  const gchar  *section,
		  GslErrorType  error,
		  const gchar  *messagef,
		  ...)
{
  struct {
    GslDebugFlags reporter;
    gchar         reporter_name[64];
    gchar         section[64];	/* auxillary information about reporter code portion */
    GslErrorType  error;
    const gchar	 *error_str;	/* gsl_strerror() of error */
    gchar	  message[1024];
  } tmsg, *msg = &tmsg;
  gchar *string;
  va_list args;
    
  g_return_if_fail (messagef != NULL);

  /* create message */
  memset (msg, 0, sizeof (*msg));
  msg->reporter = reporter;
  strncpy (msg->reporter_name, reporter_name (msg->reporter), 63);
  if (section)
    strncpy (msg->section, section, 63);
  msg->error = error;
  msg->error_str = error ? gsl_strerror (msg->error) : NULL;

  /* vsnprintf() replacement */
  va_start (args, messagef);
  string = g_strdup_vprintf (messagef, args);
  va_end (args);
  strncpy (msg->message, string, 1023);
  g_free (string);

  /* in current lack of a decent message queue, puke the message to stderr */
  g_printerr ("GSL-%s%s%s: %s%s%s\n",
	      msg->reporter_name,
	      msg->section ? ":" : "",
	      msg->section ? msg->section : "",
	      msg->message,
	      msg->error_str ? ": " : "",
	      msg->error_str ? msg->error_str : "");
}

void
gsl_debug_enable (GslDebugFlags dbg_flags)
{
  gsl_debug_flags |= dbg_flags;
}

void
gsl_debug_disable (GslDebugFlags dbg_flags)
{
  gsl_debug_flags &= dbg_flags;
}

gboolean
gsl_debug_check (GslDebugFlags dbg_flags)
{
  return (gsl_debug_flags & dbg_flags) != 0;
}

void
gsl_debug (GslDebugFlags reporter,
	   const gchar  *section,
	   const gchar  *format,
	   ...)
{
  g_return_if_fail (format != NULL);

  if (reporter & gsl_debug_flags)
    {
      va_list args;
      gchar *string;

      va_start (args, format);
      string = g_strdup_vprintf (format, args);
      va_end (args);
      g_printerr ("DEBUG:GSL-%s%s%s: %s\n",
		  reporter_name (reporter),
		  section ? ":" : "",
		  section ? section : "",
		  string);
      g_free (string);
    }
}

void
gsl_auxlog_push (GslDebugFlags reporter,
		 const gchar  *section)
{
  ThreadData *tdata = thread_data_from_gsl_thread (gsl_thread_self ());

  if (tdata)
    {
      tdata->auxlog_reporter = reporter;
      tdata->auxlog_section = section;
    }
}

void
gsl_auxlog_debug (const gchar *format,
		  ...)
{
  ThreadData *tdata = thread_data_from_gsl_thread (gsl_thread_self ());
  GslDebugFlags reporter = GSL_MSG_NOTIFY;
  const gchar *section = NULL;
  va_list args;
  gchar *string;

  if (tdata)
    {
      reporter = tdata->auxlog_reporter;
      section = tdata->auxlog_section;
      tdata->auxlog_reporter = 0;
      tdata->auxlog_section = NULL;
    }

  g_return_if_fail (format != NULL);

  va_start (args, format);
  string = g_strdup_vprintf (format, args);
  va_end (args);
  gsl_debug (reporter, section, "%s", string);
  g_free (string);
}

void
gsl_auxlog_message (GslErrorType error,
		    const gchar *format,
		    ...)
{
  ThreadData *tdata = thread_data_from_gsl_thread (gsl_thread_self ());
  GslDebugFlags reporter = GSL_MSG_NOTIFY;
  const gchar *section = NULL;
  va_list args;
  gchar *string;

  if (tdata)
    {
      reporter = tdata->auxlog_reporter;
      section = tdata->auxlog_section;
      tdata->auxlog_reporter = 0;
      tdata->auxlog_section = NULL;
    }

  g_return_if_fail (format != NULL);

  va_start (args, format);
  string = g_strdup_vprintf (format, args);
  va_end (args);
  gsl_message_send (reporter, section, error, "%s", string);
  g_free (string);
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
      
      if (check_link)
	{
	  if (lstat (file_name, &st) < 0)
	    goto have_errno;
	}
      else if (stat (file_name, &st) < 0)
	goto have_errno;

      if ((check_file && !S_ISREG (st.st_mode)) ||
	  (check_dir && !S_ISDIR (st.st_mode)) ||
	  (check_link && !S_ISLNK (st.st_mode)))
	return GSL_ERROR_OPEN_FAILED;
    }

  return GSL_ERROR_NONE;
  
 have_errno:
  return gsl_error_from_errno (errno, GSL_ERROR_OPEN_FAILED);
}

GslErrorType
gsl_error_from_errno (gint         sys_errno,
		      GslErrorType fallback)
{
  switch (sys_errno)
    {
    case ELOOP:
    case ENAMETOOLONG:
    case ENOTDIR:
    case ENOENT:        return GSL_ERROR_NOT_FOUND;
    case EROFS:
    case EPERM:
    case EACCES:        return GSL_ERROR_PERMS;
    case ENOMEM:
    case ENOSPC:
    case EFBIG:
    case ENFILE:
    case EMFILE:	return GSL_ERROR_NO_RESOURCE;
    case EISDIR:
    case ESPIPE:
    case EIO:           return GSL_ERROR_IO;
    case EEXIST:        return GSL_ERROR_EXISTS;
    case ETXTBSY:
    case EBUSY:         return GSL_ERROR_BUSY;
    case EAGAIN:
    case EINTR:		return GSL_ERROR_TEMP;
    case EINVAL:
    case EFAULT:
    case EBADF:         return GSL_ERROR_INTERNAL;
    default:            return fallback;
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
gsl_init (const GslConfigValue values[],
	  GslMutexTable       *mtable)
{
  const GslConfigValue *config = values;
  static GslConfig pconfig = {	/* DEFAULTS */
    1,				/* n_processors */
    2,				/* wave_chunk_padding */
    4,				/* wave_chunk_big_pad */
    512,			/* dcache_block_size */
    1024 * 1024,		/* dcache_cache_memory */
    69,				/* midi_kammer_note */
    440,			/* kammer_freq */
  };

  g_return_if_fail (gsl_config == NULL);	/* assert single initialization */

  /* get mutexes going first */
  if (mtable)
    gsl_mutex_table = *mtable;

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
  gsl_cond_init (&global_thread_cond);
  main_thread_tdata = create_tdata ();
  g_assert (main_thread_tdata != NULL);
  main_thread = gsl_thread_self ();
  global_thread_list = gsl_ring_prepend (global_thread_list, main_thread);
  _gsl_init_signal ();
  _gsl_init_fd_pool ();
  _gsl_init_data_caches ();
  _gsl_init_engine_utils ();
  _gsl_init_loader_gslwave ();
  _gsl_init_loader_wav ();
  _gsl_init_loader_oggvorbis ();
  _gsl_init_loader_mad ();
}
