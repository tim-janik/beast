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
#include <sys/utsname.h>
#include <string.h>
#include <sched.h>


#define	SIMPLE_CACHE_SIZE	(1024)
#define	PREALLOC		(8)


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
  g_return_val_if_fail (data != NULL, head);
  
  return gsl_ring_prepend_i (head, data);
}

GslRing*
gsl_ring_prepend_uniq (GslRing  *head,
		       gpointer data)
{
  GslRing *walk;
  
  g_return_val_if_fail (data != NULL, head);
  
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
  
  g_return_val_if_fail (data != NULL, head);
  
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
  g_return_val_if_fail (node != NULL, NULL);
  /* can't check whether node is really part of head */
  
  /* special case one item ring */
  if (head->prev == head)
    {
      g_return_val_if_fail (node == head, head);
      
      gsl_delete_struct (GslRing, 1, node);
      return NULL;
    }
  g_return_val_if_fail (node != node->next, head); /* node can't be a one item ring here */
  
  node->next->prev = node->prev;
  node->prev->next = node->next;
  if (head == node)
    head = node->next;
  gsl_delete_struct (GslRing, 1, node);
  
  return head;
}

GslRing*
gsl_ring_remove (GslRing  *head,
		 gpointer data)
{
  GslRing *walk;
  
  g_return_val_if_fail (data != NULL, head);
  g_return_val_if_fail (head != NULL, NULL);    /* since we don't allow NULL data, head can't be NULL either */
  
  /* make tail data removal an O(1) operation */
  if (head->prev->data == data)
    return gsl_ring_remove_node (head, head->prev);
  
  for (walk = head; walk; walk = gsl_ring_walk (head, walk))
    if (walk->data == data)
      return gsl_ring_remove_node (head, walk);
  
  g_warning (G_STRLOC ": couldn't find data item (%p) to remove from ring (%p)", data, head);
  
  return head;
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
static guint    global_thread_count = 0;

typedef struct
{
  GslThreadFunc func;
  gpointer      data;
} ThreadFunc;

static gpointer
thread_wrapper (gpointer arg)
{
  ThreadFunc *tfunc = arg;
  GslThreadFunc func = tfunc->func;
  gpointer data = tfunc->data;

  GSL_SYNC_LOCK (&global_thread);
  global_thread_count += 1;
  GSL_SYNC_UNLOCK (&global_thread);

  gsl_delete_struct (ThreadFunc, 1, tfunc);

  func (data);

  GSL_SYNC_LOCK (&global_thread);
  global_thread_count -= 1;
  GSL_SYNC_UNLOCK (&global_thread);

  return NULL;
}

gboolean
gsl_thread_new (GslThreadFunc func,
		gpointer      user_data)
{
  const gboolean joinable = TRUE;
  ThreadFunc *tfunc;
  gpointer gthread;

  g_return_val_if_fail (func != NULL, FALSE);

  tfunc = gsl_new_struct (ThreadFunc, 1);
  tfunc->func = func;
  tfunc->data = user_data;
  gthread = g_thread_create (thread_wrapper, tfunc, joinable, NULL);

  return gthread != NULL;
}

guint
gsl_threads_get_count (void)
{
  guint count;

  GSL_SYNC_LOCK (&global_thread);
  count = global_thread_count;
  GSL_SYNC_UNLOCK (&global_thread);

  return count;
}

gpointer
gsl_thread_self (void)
{
  gpointer gthread = g_thread_self ();

  if (!gthread)
    g_error ("gsl_thread_self() failed");

  return gthread;
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
    case GSL_ERROR_NONE:	return "Everything went well";
    case GSL_ERROR_INTERNAL:	return "Internal error (please report)";
    case GSL_ERROR_UNKNOWN:	return "Unknown error";
    case GSL_ERROR_IO:		return "I/O error";
    case GSL_ERROR_NOT_FOUND:	return "Not found";
    case GSL_ERROR_READ_FAILED:	return "Read failed";
    case GSL_ERROR_SEEK_FAILED:	return "Seek failed";
    default:			return NULL;
    }
}

void
gsl_message_send (GslMsgType   msgtype,
		  GslErrorType error,
		  const gchar *reasonf,
		  ...)
{
  struct {
    GslMsgType   msg_type;
    const gchar	*msg_str;
    GslErrorType error;
    const gchar	*error_str;
    gchar	 reason[1024];
  } tmsg, *msg = &tmsg;
    
  g_return_if_fail (msgtype >= 0);
  g_return_if_fail (msgtype < GSL_MSG_LAST);

  msg->msg_type = msgtype;
  switch (msg->msg_type)
    {
    case GSL_MSG_NOTIFY:		msg->msg_str = "Note";			break;
    case GSL_MSG_DATA_CACHE:		msg->msg_str = "DataCache";		break;
    case GSL_MSG_WAVE_DATA_HANDLE:	msg->msg_str = "WaveDataHandle";	break;
    default:				g_assert_not_reached ();
    }
  msg->error = error;
  msg->error_str = gsl_strerror (msg->error);
  g_assert (msg->error_str != NULL);
  {	/* vsnprintf() replacement */
    gchar *tmp;
    va_list args;

    va_start (args, reasonf);
    tmp = g_strdup_vprintf (reasonf, args);
    va_end (args);
    strncpy (msg->reason, tmp, 1023);
    msg->reason[1023] = 0;
    g_free (tmp);
  }

  /* in current lack of a decent message queue, just puke the message to stderr */
  g_message ("%s: %s: %s", msg->msg_str, msg->reason, msg->error_str);
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
  _gsl_init_data_handles ();
  _gsl_init_data_caches ();
  _gsl_init_wave_dsc ();
  _gsl_init_engine_utils ();
}
