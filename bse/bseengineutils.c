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


/* --- UserThread --- */
GslOStream*
_gsl_alloc_ostreams (guint n)
{
  if (n)
    {
      guint i = sizeof (GslOStream) * n + sizeof (gfloat) * gsl_engine_block_size () * n;
      GslOStream *streams = gsl_alloc_memblock0 (i);
      gfloat *buffers = (gfloat*) (streams + n);

      for (i = 0; i < n; i++)
	{
	  streams[i].values = buffers;
	  buffers += gsl_engine_block_size ();
	}
      return streams;
    }
  else
    return NULL;
}

static void
free_node (OpNode *node)
{
  g_return_if_fail (node != NULL);
  g_return_if_fail (node->output_nodes == NULL);
  g_return_if_fail (node->integrated == FALSE);
  g_return_if_fail (node->sched_tag == FALSE);
  g_return_if_fail (node->sched_router_tag == FALSE);
  
  if (node->module.klass->free)
    node->module.klass->free (node->module.user_data, node->module.klass);
  gsl_rec_mutex_destroy (&node->rec_mutex);
  if (OP_NODE_N_OSTREAMS (node))
    {
      guint n = OP_NODE_N_OSTREAMS (node);
      guint i = sizeof (GslOStream) * n + sizeof (gfloat) * gsl_engine_block_size () * n;

      gsl_free_memblock (i, node->module.ostreams);
    }
  if (OP_NODE_N_ISTREAMS (node))
    gsl_delete_structs (GslIStream, OP_NODE_N_ISTREAMS (node), node->module.istreams);
  if (node->inputs)
    gsl_delete_structs (OpInput, OP_NODE_N_ISTREAMS (node), node->inputs);
  if (node->outputs)
    gsl_delete_structs (OpOutput, OP_NODE_N_OSTREAMS (node), node->outputs);
  gsl_delete_struct (OpNode, node);
}

static void
free_job (GslJob *job)
{
  g_return_if_fail (job != NULL);
  
  switch (job->job_id)
    {
    case GSL_JOB_ACCESS:
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
      free_node (job->data.node);
      break;
    default: ;
    }
  gsl_delete_struct (GslJob, job);
}

static void
free_flow_job (GslFlowJob *fjob)
{
  switch (fjob->fjob_id)
    {
    case GSL_FLOW_JOB_SUSPEND:
    case GSL_FLOW_JOB_RESUME:
      gsl_delete_struct (GslFlowJobAny, &fjob->any);
      break;
    case GSL_FLOW_JOB_ACCESS:
      if (fjob->access.free_func)
	fjob->access.free_func (fjob->access.data);
      gsl_delete_struct (GslFlowJobAccess, &fjob->access);
      break;
    default:
      g_assert_not_reached ();
    }
}

void
_gsl_free_trans (GslTrans *trans)
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
      
      free_job (job);
      job = tmp;
    }
  gsl_delete_struct (GslTrans, trans);
}


/* -- master node list --- */
static OpNode      *master_node_list_head = NULL;
static OpNode      *master_node_list_tail = NULL;

OpNode*
_gsl_mnl_head (void)
{
  return master_node_list_head;
}

void
_gsl_mnl_remove (OpNode *node)
{
  g_return_if_fail (node->integrated == TRUE);

  node->integrated = FALSE;
  /* remove */
  if (node->mnl_prev)
    node->mnl_prev->mnl_next = node->mnl_next;
  else
    master_node_list_head = node->mnl_next;
  if (node->mnl_next)
    node->mnl_next->mnl_prev = node->mnl_prev;
  else
    master_node_list_tail = node->mnl_prev;
  node->mnl_prev = NULL;
  node->mnl_next = NULL;
}

void
_gsl_mnl_integrate (OpNode *node)
{
  g_return_if_fail (node->integrated == FALSE);
  g_return_if_fail (node->flow_jobs == NULL);

  node->integrated = TRUE;
  /* append */
  if (master_node_list_tail)
    master_node_list_tail->mnl_next = node;
  node->mnl_prev = master_node_list_tail;
  master_node_list_tail = node;
  if (!master_node_list_head)
    master_node_list_head = master_node_list_tail;
  g_assert (node->mnl_next == NULL);
}

void
_gsl_mnl_reorder (OpNode *node)
{
  OpNode *sibling;

  g_return_if_fail (node->integrated == TRUE);

  /* the master node list is partially sorted. that is, all
   * nodes which are not scheduled and have pending flow_jobs
   * are agglomerated at the head.
   */
  sibling = node->mnl_prev ? node->mnl_prev : node->mnl_next;
  if (sibling && GSL_MNL_HEAD_NODE (node) != GSL_MNL_HEAD_NODE (sibling))
    {
      /* remove */
      if (node->mnl_prev)
	node->mnl_prev->mnl_next = node->mnl_next;
      else
	master_node_list_head = node->mnl_next;
      if (node->mnl_next)
	node->mnl_next->mnl_prev = node->mnl_prev;
      else
	master_node_list_tail = node->mnl_prev;
      if (GSL_MNL_HEAD_NODE (node))	/* move towards head */
	{
	  /* prepend to non-NULL list */
	  master_node_list_head->mnl_prev = node;
	  node->mnl_next = master_node_list_head;
	  master_node_list_head = node;
	  node->mnl_prev = NULL;
	}
      else				/* move towards tail */
	{
	  /* append to non-NULL list */
	  master_node_list_tail->mnl_next = node;
	  node->mnl_prev = master_node_list_tail;
	  master_node_list_tail = node;
	  node->mnl_next = NULL;
	}
    }
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
static GslTrans      *cqueue_trans_pending_head = NULL;
static GslTrans      *cqueue_trans_pending_tail = NULL;
static GslCond       *cqueue_trans_cond = NULL;
static GslTrans      *cqueue_trans_trash = NULL;
static GslTrans      *cqueue_trans_active_head = NULL;
static GslTrans      *cqueue_trans_active_tail = NULL;
static GslFlowJob    *cqueue_trash_fjobs = NULL;
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


/* --- user thread garbage collection --- */
/**
 * gsl_engine_garbage_collect
 *
 * GSL Engine user thread function. Collects processed jobs
 * and transactions from the engine and frees them, this
 * involves callback invocation of GslFreeFunc() functions,
 * e.g. from gsl_job_access() or gsl_flow_job_access()
 * jobs.
 * This function may only be called from the user thread,
 * as GslFreeFunc() functions are guranteed to be executed
 * in the user thread.
 */
void
gsl_engine_garbage_collect (void)
{
  GslTrans *trans;
  GslFlowJob *fjobs;

  GSL_SPIN_LOCK (&cqueue_trans);
  trans = cqueue_trans_trash;
  cqueue_trans_trash = NULL;
  fjobs = cqueue_trash_fjobs;
  cqueue_trash_fjobs = NULL;
  GSL_SPIN_UNLOCK (&cqueue_trans);

  while (trans)
    {
      GslTrans *t = trans;

      trans = t->cqt_next;
      t->cqt_next = NULL;
      t->jobs_tail->next = NULL;
      t->comitted = FALSE;
      _gsl_free_trans (t);
    }

  while (fjobs)
    {
      GslFlowJob *j = fjobs;

      fjobs = j->any.next;
      j->any.next = NULL;
      free_flow_job (j);
    }
}


/* --- node processing queue --- */
static GslMutex          pqueue_mutex = { 0, };
static OpSchedule       *pqueue_schedule = NULL;
static guint             pqueue_n_nodes = 0;
static guint             pqueue_n_cycles = 0;
static GslCond		*pqueue_done_cond = NULL;
static GslFlowJob       *pqueue_trash_fjobs_first = NULL;
static GslFlowJob       *pqueue_trash_fjobs_last = NULL;

void
_gsl_com_set_schedule (OpSchedule *sched)
{
  g_return_if_fail (sched != NULL);
  g_return_if_fail (sched->secured == TRUE);
  
  GSL_SPIN_LOCK (&pqueue_mutex);
  if_reject (pqueue_schedule)
    {
      GSL_SPIN_UNLOCK (&pqueue_mutex);
      g_warning (G_STRLOC ": schedule already set");
      return;
    }
  pqueue_schedule = sched;
  sched->in_pqueue = TRUE;
  GSL_SPIN_UNLOCK (&pqueue_mutex);
}

void
_gsl_com_unset_schedule (OpSchedule *sched)
{
  GslFlowJob *trash_fjobs_first, *trash_fjobs_last;

  g_return_if_fail (sched != NULL);
  
  GSL_SPIN_LOCK (&pqueue_mutex);
  if_reject (pqueue_schedule != sched)
    {
      GSL_SPIN_UNLOCK (&pqueue_mutex);
      g_warning (G_STRLOC ": schedule(%p) not currently set", sched);
      return;
    }
  if_reject (pqueue_n_nodes || pqueue_n_cycles)
    g_warning (G_STRLOC ": schedule(%p) still busy", sched);

  sched->in_pqueue = FALSE;
  pqueue_schedule = NULL;
  trash_fjobs_first = pqueue_trash_fjobs_first;
  trash_fjobs_last = pqueue_trash_fjobs_last;
  pqueue_trash_fjobs_first = NULL;
  pqueue_trash_fjobs_last = NULL;
  GSL_SPIN_UNLOCK (&pqueue_mutex);

  if (trash_fjobs_first)	/* move trash flow jobs */
    {
      GSL_SPIN_LOCK (&cqueue_trans);
      trash_fjobs_last->any.next = cqueue_trash_fjobs;
      cqueue_trash_fjobs = trash_fjobs_first;
      GSL_SPIN_UNLOCK (&cqueue_trans);
    }
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
  if (node->fjob_first)	/* collect trash flow jobs */
    {
      node->fjob_last->any.next = pqueue_trash_fjobs_first;
      pqueue_trash_fjobs_first = node->fjob_first;
      if (!pqueue_trash_fjobs_last)
	pqueue_trash_fjobs_last = node->fjob_last;
      node->fjob_first = NULL;
      node->fjob_last = NULL;
    }
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
