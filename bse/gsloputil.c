/* GSL Engine - Flow module operation engine
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
#include "gsloputil.h"

#include "gslcommon.h"
#include "gslopnode.h"
#include "gslopschedule.h"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>


/* some systems don't have ERESTART (which is what linux returns for system
 * calls on pipes which are being interrupted). most propably just use EINTR,
 * and maybe some can return both. so we check for both in the below code,
 * and alias ERESTART to EINTR if it's not present. compilers are supposed
 * to catch and optimize the doubled check arising from this.
 */
#ifndef ERESTART
#define ERESTART        EINTR
#endif


/* --- UserThread --- */
static void
op_free_node (OpNode *node)
{
  g_return_if_fail (node != NULL);
  g_return_if_fail (node->onodes == NULL);
  g_return_if_fail (node->mnl_contained == FALSE);
  g_return_if_fail (node->sched_tag == FALSE);
  g_return_if_fail (node->sched_router_tag == FALSE);
  
  if (node->module.klass->free)
    node->module.klass->free (node->module.user_data, node->module.klass);
  gsl_rec_mutex_destroy (&node->rec_mutex);
  if (OP_NODE_N_OSTREAMS (node))
    gsl_delete_struct (GslOStream, OP_NODE_N_OSTREAMS (node), node->module.ostreams); /* FIXME: free ostream buffers */
  if (OP_NODE_N_ISTREAMS (node))
    gsl_delete_struct (GslIStream, OP_NODE_N_ISTREAMS (node), node->module.istreams);
  if (node->inputs)
    gsl_delete_struct (OpInput, OP_NODE_N_ISTREAMS (node), node->inputs);
  if (node->outputs)
    gsl_delete_struct (OpOutput, OP_NODE_N_OSTREAMS (node), node->outputs);
  gsl_delete_struct (OpNode, 1, node);
}

static void
op_free_job (GslJob *job)
{
  g_return_if_fail (job != NULL);
  
  switch (job->job_id)
    {
    case OP_JOB_ACCESS:
      if (job->data.access.free_func)
	job->data.access.free_func (job->data.access.data);
      break;
    case OP_JOB_DEBUG:
      g_free (job->data.debug);
      break;
    case OP_JOB_ADD_POLL:
    case OP_JOB_REMOVE_POLL:
      g_free (job->data.poll.fds);
      if (job->data.poll.free_func)
	job->data.poll.free_func (job->data.poll.data);
      break;
    case OP_JOB_DISCARD:
      op_free_node (job->data.node);
      break;
    default: ;
    }
  gsl_delete_struct (GslJob, 1, job);
}

void
_op_collect_trans (void)
{
  GslTrans *trans;
  
  for (trans = op_com_collect_trans (); trans; trans = op_com_collect_trans ())
    {
      trans->comitted = FALSE;
      _op_free_trans (trans);
    }
}

GslTrans*
_op_alloc_trans (void)
{
  /* do a bit of garbage collection here */
  _op_collect_trans ();
  
  return gsl_new_struct0 (GslTrans, 1);
}

void
_op_free_trans (GslTrans *trans)
{
  GslJob *job;
  
  g_return_if_fail (trans != NULL);
  g_return_if_fail (trans->comitted == FALSE);
  if (trans->jobs_tail)
    g_return_if_fail (trans->jobs_tail->next == NULL);	/* paranoid */
  
  job = trans->jobs_head;
  while (job)
    {
      GslJob *tmp = job->next;
      
      op_free_job (job);
      job = tmp;
    }
  gsl_delete_struct (GslTrans, 1, trans);
}

GslOStream*
_op_alloc_ostreams (guint n)
{
  GslOStream *streams;
  
  if (n)
    {
      guint i;
      
      streams = gsl_new_struct0 (GslOStream, n);
      
      for (i = 0; i < n; i++)
	streams[i].values = g_new (gfloat, GSL_STREAM_MAX_VALUES); /* FIXME */
    }
  else
    streams = NULL;
  
  return streams;
}


/* --- const value blocks --- */
typedef struct
{
  guint    n_nodes;
  gfloat **nodes;
} ConstValuesArray;
static const gfloat CONST_VALUES_EPSILON = 1e-5;	/* FIXME: assuming 16bit significant bits */

static inline gfloat*
const_values_lookup (ConstValuesArray *array,
		     gfloat	       key_value)
{
  guint n_nodes = array->n_nodes;
  
  if (n_nodes > 0)
    {
      gfloat **nodes = array->nodes;
      
      nodes -= 1;
      do
	{
	  gfloat **check;
	  guint i;
	  register gfloat cmp;
	  
	  i = (n_nodes + 1) >> 1;
	  check = nodes + i;
	  cmp = key_value - **check;
	  if (cmp > CONST_VALUES_EPSILON)
	    {
	      n_nodes -= i;
	      nodes = check;
	    }
	  else if (cmp < CONST_VALUES_EPSILON)
	    n_nodes = i - 1;
	  else /* cmp ~==~ 0.0 */
	    return *check;
	}
      while (n_nodes);
    }
  
  return NULL;
}

static inline guint
upper_power2 (guint number)
{
  return gsl_alloc_upper_power2 (MAX (number, 8));
}

static inline void
const_values_insert (ConstValuesArray *array,
		     gfloat	      *value_block)
{
  gfloat **check;
  
  if (array->n_nodes == 0)
    {
      guint new_size = upper_power2 (sizeof (gfloat*));
      
      array->nodes = g_realloc (array->nodes, new_size);
      array->n_nodes = 1;
      check = array->nodes;
    }
  else
    {
      guint n_nodes = array->n_nodes;
      gfloat **nodes = array->nodes;
      gfloat cmp;
      guint i;
      
      nodes -= 1;
      do
	{
	  i = (n_nodes + 1) >> 1;
	  check = nodes + i;
	  cmp = *value_block - **check;
	  if (cmp > CONST_VALUES_EPSILON)
	    {
	      n_nodes -= i;
	      nodes = check;
	    }
	  else if (cmp < CONST_VALUES_EPSILON)
	    n_nodes = i - 1;
	  else /* cmp ~==~ 0.0 */
	    g_assert_not_reached ();
	}
      while (n_nodes);
      /* grow */
      if (cmp > 0)
	check += 1;
      i = check - array->nodes;
      n_nodes = array->n_nodes++;
      if (1)
	{
	  guint new_size = upper_power2 (array->n_nodes * sizeof (gfloat*));
	  guint old_size = upper_power2 (n_nodes * sizeof (gfloat*));
	  
	  if (new_size != old_size)
	    array->nodes = g_realloc (array->nodes, new_size);
	}
      check = array->nodes + i;
      g_memmove (check + 1, check, (n_nodes - i) * sizeof (gfloat*));
    }
  *check = value_block;
}

static ConstValuesArray cvalue_array = { 0, NULL, };

gfloat*
gsl_engine_const_values (gfloat value)
{
  extern const gfloat gsl_engine_master_zero_block[];
  gfloat *block;
  
  if (fabs (value) < CONST_VALUES_EPSILON)
    return (gfloat*) gsl_engine_master_zero_block;
  block = const_values_lookup (&cvalue_array, value);
  if (!block)
    {
      guint i;
      
      block = g_new (gfloat, gsl_engine_block_size ());
      for (i = 0; i < gsl_engine_block_size (); i++)
	block[i] = value;
      const_values_insert (&cvalue_array, block);
    }
  return block;
}

void
_gsl_recycle_const_values (void)
{
  while (cvalue_array.n_nodes--)
    g_free (cvalue_array.nodes[cvalue_array.n_nodes]);
  cvalue_array.n_nodes = 0;
}


/* --- job transactions --- */
static GslMutex       cqueue_trans = { 0, };
static GslTrans       *cqueue_trans_pending_head = NULL;
static GslTrans       *cqueue_trans_pending_tail = NULL;
static GslCond       *cqueue_trans_cond = NULL;
static GslTrans       *cqueue_trans_trash = NULL;
static GslTrans      *cqueue_trans_active_head = NULL;
static GslTrans      *cqueue_trans_active_tail = NULL;
static GslJob        *cqueue_trans_job = NULL;

void
op_com_enqueue_trans (GslTrans *trans)
{
  g_return_if_fail (trans != NULL);
  g_return_if_fail (trans->comitted == TRUE);
  g_return_if_fail (trans->jobs_head != NULL);
  g_return_if_fail (trans->cqt_next == NULL);
  
  GSL_SPIN_LOCK (&cqueue_trans);
  if (cqueue_trans_pending_tail)
    {
      cqueue_trans_pending_tail->cqt_next = trans;
      cqueue_trans_pending_tail->jobs_tail->next = trans->jobs_head;
    }
  else
    cqueue_trans_pending_head = trans;
  cqueue_trans_pending_tail = trans;
  GSL_SPIN_UNLOCK (&cqueue_trans);
  gsl_cond_signal (cqueue_trans_cond);
}

GslTrans*
op_com_collect_trans (void)
{
  GslTrans *trans;
  
  GSL_SPIN_LOCK (&cqueue_trans);
  trans = cqueue_trans_trash;
  if (trans)
    cqueue_trans_trash = trans->cqt_next;
  GSL_SPIN_UNLOCK (&cqueue_trans);
  if (trans)
    {
      trans->cqt_next = NULL;
      trans->jobs_tail->next = NULL;
    }
  
  return trans;
}

void
op_com_wait_on_trans (void)
{
  GSL_SPIN_LOCK (&cqueue_trans);
  while (cqueue_trans_pending_head || cqueue_trans_active_head)
    gsl_cond_wait (cqueue_trans_cond, &cqueue_trans);
  GSL_SPIN_UNLOCK (&cqueue_trans);
}

gboolean
op_com_job_pending (void)
{
  gboolean pending = cqueue_trans_job != NULL;
  
  if (!pending)
    {
      GSL_SPIN_LOCK (&cqueue_trans);
      pending = cqueue_trans_pending_head != NULL;
      GSL_SPIN_UNLOCK (&cqueue_trans);
    }
  return pending;
}

GslJob*
gsl_com_pop_job (void)	/* (glong max_useconds) */
{
  /* clean up if necessary and try fetching new jobs */
  if (!cqueue_trans_job)
    {
      if (cqueue_trans_active_head)
	{
	  GSL_SPIN_LOCK (&cqueue_trans);
	  /* get rid of processed transaction and
	   * signal UserThread which might be in
	   * op_com_wait_on_trans()
	   */
	  cqueue_trans_active_tail->cqt_next = cqueue_trans_trash;
	  cqueue_trans_trash = cqueue_trans_active_head;
	  /* fetch new transaction */
	  cqueue_trans_active_head = cqueue_trans_pending_head;
	  cqueue_trans_active_tail = cqueue_trans_pending_tail;
	  cqueue_trans_pending_head = NULL;
	  cqueue_trans_pending_tail = NULL;
	  GSL_SPIN_UNLOCK (&cqueue_trans);
	  gsl_cond_signal (cqueue_trans_cond);
	}
      else
	{
	  GSL_SPIN_LOCK (&cqueue_trans);
	  /* fetch new transaction */
	  cqueue_trans_active_head = cqueue_trans_pending_head;
	  cqueue_trans_active_tail = cqueue_trans_pending_tail;
	  cqueue_trans_pending_head = NULL;
	  cqueue_trans_pending_tail = NULL;
	  GSL_SPIN_UNLOCK (&cqueue_trans);
	}
      cqueue_trans_job = cqueue_trans_active_head ? cqueue_trans_active_head->jobs_head : NULL;
    }
  
  /* pick new job and out of here */
  if (cqueue_trans_job)
    {
      GslJob *job = cqueue_trans_job;
      
      cqueue_trans_job = job->next;
      return job;
    }
  
#if 0
  /* wait until jobs are present */
  if (max_useconds != 0)
    {
      GSL_SPIN_LOCK (&cqueue_trans);
      if (!cqueue_trans_pending_head)
	gsl_cond_wait_timed (cqueue_trans_cond,
			     &cqueue_trans,
			     max_useconds);
      GSL_SPIN_UNLOCK (&cqueue_trans);
      
      /* there may be jobs now, start from scratch */
      return op_com_pop_job_timed (max_useconds < 0 ? -1 : 0);
    }
#endif
  
  /* time expired, no jobs... */
  return NULL;
}

/* --- node processing queue --- */
static GslMutex          pqueue_mutex = { 0, };
static OpSchedule       *pqueue_schedule = NULL;
static guint             pqueue_n_nodes = 0;
static guint             pqueue_n_cycles = 0;
static GslCond		*pqueue_done_cond = NULL;

void
_gsl_com_set_schedule (OpSchedule *sched)
{
  g_return_if_fail (sched != NULL);
  g_return_if_fail (sched->secured == TRUE);
  
  GSL_SPIN_LOCK (&pqueue_mutex);
  if (pqueue_schedule)
    {
      g_warning (G_STRLOC ": schedule already set");
      GSL_SPIN_UNLOCK (&pqueue_mutex);
      return;
    }
  pqueue_schedule = sched;
  sched->in_pqueue = TRUE;
  GSL_SPIN_UNLOCK (&pqueue_mutex);
}

void
_gsl_com_unset_schedule (OpSchedule *sched)
{
  g_return_if_fail (sched != NULL);
  
  GSL_SPIN_LOCK (&pqueue_mutex);
  if (pqueue_schedule != sched)
    {
      g_warning (G_STRLOC ": schedule(%p) not currently set", sched);
      GSL_SPIN_UNLOCK (&pqueue_mutex);
      return;
    }
  if (pqueue_n_nodes || pqueue_n_cycles)
    {
      g_warning (G_STRLOC ": schedule(%p) still busy", sched);
      GSL_SPIN_UNLOCK (&pqueue_mutex);
      return;
    }
  sched->in_pqueue = FALSE;
  pqueue_schedule = NULL;
  GSL_SPIN_UNLOCK (&pqueue_mutex);
}

OpNode*
_gsl_com_pop_unprocessed_node (void)
{
  OpNode *node;
  
  GSL_SPIN_LOCK (&pqueue_mutex);
  node = pqueue_schedule ? _op_schedule_pop_node (pqueue_schedule) : NULL;
  if (node)
    pqueue_n_nodes += 1;
  GSL_SPIN_UNLOCK (&pqueue_mutex);
  
  if (node)
    OP_NODE_LOCK (node);
  
  return node;
}

void
_gsl_com_push_processed_node (OpNode *node)
{
  g_return_if_fail (node != NULL);
  g_return_if_fail (OP_NODE_SELF_LOCKED (node));        /* paranoid */
  g_return_if_fail (pqueue_n_nodes > 0);
  g_return_if_fail (OP_NODE_IS_SCHEDULED (node));
  
  GSL_SPIN_LOCK (&pqueue_mutex);
  g_assert (pqueue_n_nodes > 0);        /* paranoid */
  pqueue_n_nodes -= 1;
  OP_NODE_UNLOCK (node);
  if (!pqueue_n_nodes && !pqueue_n_cycles && GSL_SCHEDULE_NONPOPABLE (pqueue_schedule))
    gsl_cond_signal (pqueue_done_cond);
  GSL_SPIN_UNLOCK (&pqueue_mutex);
}

GslRing*
_gsl_com_pop_unprocessed_cycle (void)
{
  return NULL;
}

void
_gsl_com_push_processed_cycle (GslRing *cycle)
{
  g_return_if_fail (cycle != NULL);
  g_return_if_fail (OP_NODE_SELF_LOCKED (cycle->data));	/* paranoid */
  g_return_if_fail (pqueue_n_cycles > 0);
  g_return_if_fail (OP_NODE_IS_SCHEDULED (cycle->data));
}

void
_gsl_com_wait_on_unprocessed (void)
{
  GSL_SPIN_LOCK (&pqueue_mutex);
  while (pqueue_n_nodes || pqueue_n_cycles || !GSL_SCHEDULE_NONPOPABLE (pqueue_schedule))
    gsl_cond_wait (pqueue_done_cond, &pqueue_mutex);
  GSL_SPIN_UNLOCK (&pqueue_mutex);
}


/* --- thread wakeups --- */
static gint	master_wakeup_pipe[2] = { -1, -1 };
static gint	user_wakeup_pipe[2] = { -1, -1 };

static inline void
wakeup_pipe_create (gint wakeup_pipe[2])
{
  gint pipe_creation_error;
  glong d_long;

  pipe_creation_error = pipe (wakeup_pipe);
  if (pipe_creation_error == 0)
    {
      d_long = fcntl (wakeup_pipe[0], F_GETFL, 0);
      g_print ("pipe-readfd, blocking=%ld\n", d_long & O_NONBLOCK);
      d_long |= O_NONBLOCK;
      pipe_creation_error = fcntl (wakeup_pipe[0], F_SETFL, d_long);
    }
  if (pipe_creation_error == 0)
    {
      d_long = fcntl (wakeup_pipe[1], F_GETFL, 0);
      g_print ("pipe-writefd, blocking=%ld\n", d_long & O_NONBLOCK);
      d_long |= O_NONBLOCK;
      pipe_creation_error = fcntl (wakeup_pipe[1], F_SETFL, d_long);
    }
  if (pipe_creation_error != 0)		/* hard to handle failures here */
    g_error ("failed to create thread wakeup pipe: %s\n",
	     g_strerror (errno));
}

static inline void
wakeup_pipe_write (gint wakeup_pipe[2])
{
  if (wakeup_pipe[1] != -1)
    {
      guint8 data = 'W';
      gint r;
      
      do
	r = write (wakeup_pipe[1], &data, 1);
      while (r < 0 && (errno == EINTR || errno == ERESTART));
    }
}

static inline void
wakeup_pipe_read_all (gint wakeup_pipe[2])
{
  if (wakeup_pipe[0] != -1)
    {
      guint8 data[64];
      gint r;
      
      do
	r = read (wakeup_pipe[0], data, sizeof (data));
      while ((r < 0 && (errno == EINTR || errno == ERESTART)) || r == sizeof (data));
    }
}

static gboolean
wakeup_check (gpointer         data,
	      guint            n_values,
	      glong           *timeout_p,
	      guint            n_fds,
	      const GslPollFD *fds,
	      gboolean         revents_filled)
{
  if (!revents_filled)	/* we're just here to check poll(2) state */
    return FALSE;
  else
    return fds[0].revents & GSL_POLLIN;
}

static void
wakeup_delete (gpointer data)
{
  gint *wakeup_pipe = data;

  g_return_if_fail (wakeup_pipe[0] != -1);
  
  close (wakeup_pipe[0]);
  wakeup_pipe[0] = -1;
  close (wakeup_pipe[1]);
  wakeup_pipe[1] = -1;
}

void
_gsl_com_add_master_wakeup (GslTrans *trans)
{
  GslPollFD pollfd;

  g_return_if_fail (trans != NULL);
  g_return_if_fail (master_wakeup_pipe[0] == -1);

  wakeup_pipe_create (master_wakeup_pipe);

  pollfd.fd = master_wakeup_pipe[0];
  pollfd.events = GSL_POLLIN;
  gsl_trans_add (trans,
		 gsl_job_add_poll (wakeup_check, master_wakeup_pipe, wakeup_delete,
				   1, &pollfd));
}

void
_gsl_com_remove_master_wakeup (GslTrans *trans)
{
  g_return_if_fail (trans != NULL);
  g_return_if_fail (master_wakeup_pipe[0] != -1);

  gsl_trans_add (trans,
		 gsl_job_remove_poll (wakeup_check, NULL));
}

void
_gsl_com_fire_master_wakeup (void)
{
  wakeup_pipe_write (master_wakeup_pipe);
}

void
_gsl_com_discard_master_wakeups (void)
{
  wakeup_pipe_read_all (master_wakeup_pipe);
}

gint
_gsl_com_get_user_wakeup (void)
{
  if (user_wakeup_pipe[0] == -1)
    wakeup_pipe_create (user_wakeup_pipe);

  return user_wakeup_pipe[0];
}

void
_gsl_com_fire_user_wakeup (void)
{
  wakeup_pipe_write (user_wakeup_pipe);
}

void
_gsl_com_discard_user_wakeups (void)
{
  wakeup_pipe_read_all (user_wakeup_pipe);
}


/* --- initialization --- */
void
_gsl_init_engine_utils (void)
{
  g_assert (cqueue_trans_cond == NULL); /* single invocation */
  
  gsl_mutex_init (&cqueue_trans);
  cqueue_trans_cond = gsl_cond_new ();
  gsl_mutex_init (&pqueue_mutex);
  pqueue_done_cond = gsl_cond_new ();
}
