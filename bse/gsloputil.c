/* GSL Engine - Flow module operation engine
 * Copyright (C) 2001-2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "gsloputil.h"

#include "gslcommon.h"
#include "gslopnode.h"
#include "gslopschedule.h"
#include "gslsignal.h"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>


/* --- UserThread --- */
GslOStream*
_engine_alloc_ostreams (guint n)
{
  if (n)
    {
      guint i = sizeof (GslOStream) * n + sizeof (gfloat) * gsl_engine_block_size () * n;
      GslOStream *streams = sfi_alloc_memblock0 (i);
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
free_node (EngineNode *node)
{
  const GslClass *klass;
  gpointer user_data;
  guint j;

  g_return_if_fail (node != NULL);
  g_return_if_fail (node->output_nodes == NULL);
  g_return_if_fail (node->integrated == FALSE);
  g_return_if_fail (node->sched_tag == FALSE);
  g_return_if_fail (node->sched_recurse_tag == FALSE);
  g_return_if_fail (node->flow_jobs == NULL && node->fjob_first == NULL);
  
  sfi_rec_mutex_destroy (&node->rec_mutex);
  if (node->module.ostreams)
    {
      guint n = ENGINE_NODE_N_OSTREAMS (node);
      guint i = sizeof (GslOStream) * n + sizeof (gfloat) * gsl_engine_block_size () * n;

      sfi_free_memblock (i, node->module.ostreams);
      sfi_delete_structs (EngineOutput, ENGINE_NODE_N_OSTREAMS (node), node->outputs);
    }
  if (node->module.istreams)
    {
      sfi_delete_structs (GslIStream, ENGINE_NODE_N_ISTREAMS (node), node->module.istreams);
      sfi_delete_structs (EngineInput, ENGINE_NODE_N_ISTREAMS (node), node->inputs);
    }
  for (j = 0; j < ENGINE_NODE_N_JSTREAMS (node); j++)
    {
      g_free (node->jinputs[j]);
      g_free (node->module.jstreams[j].values);
    }
  if (node->module.jstreams)
    {
      sfi_delete_structs (GslJStream, ENGINE_NODE_N_JSTREAMS (node), node->module.jstreams);
      sfi_delete_structs (EngineJInput*, ENGINE_NODE_N_JSTREAMS (node), node->jinputs);
    }
  klass = node->module.klass;
  user_data = node->module.user_data;
  sfi_delete_struct (EngineNode, node);

  /* allow the free function to free the klass as well */
  if (klass->free)
    klass->free (user_data, klass);
}

static void
free_job (GslJob *job)
{
  g_return_if_fail (job != NULL);
  
  switch (job->job_id)
    {
    case ENGINE_JOB_ACCESS:
      if (job->data.access.free_func)
	job->data.access.free_func (job->data.access.data);
      break;
    case ENGINE_JOB_DEBUG:
      g_free (job->data.debug);
      break;
    case ENGINE_JOB_ADD_POLL:
    case ENGINE_JOB_REMOVE_POLL:
      g_free (job->data.poll.fds);
      if (job->data.poll.free_func)
	job->data.poll.free_func (job->data.poll.data);
      break;
    case ENGINE_JOB_ADD_TIMER:
      if (job->data.timer.free_func)
	job->data.timer.free_func (job->data.timer.data);
      break;
    case ENGINE_JOB_DISCARD:
      free_node (job->data.node);
      break;
    default: ;
    }
  sfi_delete_struct (GslJob, job);
}

static void
free_flow_job (EngineFlowJob *fjob)
{
  switch (fjob->fjob_id)
    {
    case ENGINE_FLOW_JOB_RESUME:
      sfi_delete_struct (EngineFlowJobAny, &fjob->any);
      break;
    case ENGINE_FLOW_JOB_ACCESS:
      if (fjob->access.free_func)
	fjob->access.free_func (fjob->access.data);
      sfi_delete_struct (EngineFlowJobAccess, &fjob->access);
      break;
    default:
      g_assert_not_reached ();
    }
}

static void
free_trans (GslTrans *trans)
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
  sfi_delete_struct (GslTrans, trans);
}


/* -- master node list --- */
static EngineNode      *master_node_list_head = NULL;
static EngineNode      *master_node_list_tail = NULL;

EngineNode*
_engine_mnl_head (void)
{
  return master_node_list_head;
}

void
_engine_mnl_remove (EngineNode *node)
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
_engine_mnl_integrate (EngineNode *node)
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
_engine_mnl_reorder (EngineNode *node)
{
  EngineNode *sibling;

  g_return_if_fail (node->integrated == TRUE);

  /* the master node list is partially sorted. that is, all
   * nodes which are not scheduled and have pending flow_jobs
   * are agglomerated at the head.
   */
  sibling = node->mnl_prev ? node->mnl_prev : node->mnl_next;
  if (sibling && GSL_MNL_UNSCHEDULED_FLOW_NODE (node) != GSL_MNL_UNSCHEDULED_FLOW_NODE (sibling))
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
      if (GSL_MNL_UNSCHEDULED_FLOW_NODE (node))	/* move towards head */
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
  guint8  *nodes_used;
} ConstValuesArray;

static const guint8 CONST_VALUES_EXPIRE = 16;           /* expire value after being unused for 16 times */

static inline gfloat**
const_values_lookup_nextmost (ConstValuesArray *array,
		              gfloat	        key_value)
{
  guint n_nodes = array->n_nodes;
  
  if (n_nodes > 0)
    {
      gfloat **nodes = array->nodes;
      gfloat **check;
      
      nodes -= 1;
      do
	{
	  guint i;
	  register gfloat cmp;
	  
	  i = (n_nodes + 1) >> 1;
	  check = nodes + i;
	  cmp = key_value - **check;
	  if (cmp > GSL_SIGNAL_EPSILON)
	    {
	      n_nodes -= i;
	      nodes = check;
	    }
	  else if (cmp < -GSL_SIGNAL_EPSILON)
	    n_nodes = i - 1;
	  else /* cmp ~==~ 0.0 */
	    return check;   /* matched */
	}
      while (n_nodes);

      return check;  /* nextmost */
    }
  
  return NULL;
}

static inline guint
upper_power2 (guint number)
{
  return sfi_alloc_upper_power2 (MAX (number, 8));
}

static inline void
const_values_insert (ConstValuesArray *array,
		     guint             index,
		     gfloat	      *value_block)
{
  if (array->n_nodes == 0)
    {
      guint new_size = upper_power2 (sizeof (gfloat*));
      
      array->nodes = g_realloc (array->nodes, new_size);
      array->nodes_used = g_realloc (array->nodes_used, new_size / sizeof (gfloat*));
      array->n_nodes = 1;

      g_assert (index == 0);
    }
  else
    {
      guint n_nodes = array->n_nodes++;

      if (*array->nodes[index] < *value_block)
	index++;
      
      if (1)
	{
	  guint new_size = upper_power2 (array->n_nodes * sizeof (gfloat*));
	  guint old_size = upper_power2 (n_nodes * sizeof (gfloat*));
	  
	  if (new_size != old_size)
	    {
	      array->nodes = g_realloc (array->nodes, new_size);
	      array->nodes_used = g_realloc (array->nodes_used, new_size / sizeof(gfloat*));
	    }
	}
      g_memmove (array->nodes + index + 1, array->nodes + index, (n_nodes - index) * sizeof (array->nodes[0]));
      g_memmove (array->nodes_used + index + 1, array->nodes_used + index, (n_nodes - index) * sizeof (array->nodes_used[0]));
    }

  array->nodes[index] = value_block;
  array->nodes_used[index] = CONST_VALUES_EXPIRE;
}

static ConstValuesArray cvalue_array = { 0, NULL, NULL };

gfloat*
gsl_engine_const_values (gfloat value)
{
  extern const gfloat gsl_engine_master_zero_block[];
  gfloat **block;
  
  if (fabs (value) < GSL_SIGNAL_EPSILON)
    return (gfloat*) gsl_engine_master_zero_block;

  block = const_values_lookup_nextmost (&cvalue_array, value);

  /* found correct match? */
  if (block && fabs (**block - value) < GSL_SIGNAL_EPSILON)
    {
      cvalue_array.nodes_used[block - cvalue_array.nodes] = CONST_VALUES_EXPIRE;
      return *block;
    }
  else
    {
      /* create new value block */
      gfloat *values = g_new (gfloat, gsl_engine_block_size ());
      guint i;

      for (i = 0; i < gsl_engine_block_size (); i++)
	values[i] = value;
     
      if (block)
	const_values_insert (&cvalue_array, block - cvalue_array.nodes, values);
      else
	const_values_insert (&cvalue_array, 0, values);

      return values;
    }
}

void
_engine_recycle_const_values (void)
{
  gfloat **nodes = cvalue_array.nodes;
  guint8 *used = cvalue_array.nodes_used;
  guint count = cvalue_array.n_nodes, e = 0, i;
  
  for (i = 0; i < count; i++)
    {
      used[i]--;  /* invariant: use counts are never 0 */
      
      if (used[i] == 0)
	g_free (nodes[i]);
      else /* preserve node */
	{
	  if (e < i)
	    {
	      nodes[e] = nodes[i];
	      used[e] = used[i];
	    }
	  e++;
	}
    }
  cvalue_array.n_nodes = e;
}

/* --- job transactions --- */
static SfiMutex       cqueue_trans = { 0, };
static GslTrans      *cqueue_trans_pending_head = NULL;
static GslTrans      *cqueue_trans_pending_tail = NULL;
static SfiCond        cqueue_trans_cond = { 0, };
static GslTrans      *cqueue_trans_trash = NULL;
static GslTrans      *cqueue_trans_active_head = NULL;
static GslTrans      *cqueue_trans_active_tail = NULL;
static EngineFlowJob *cqueue_trash_fjobs = NULL;
static GslJob        *cqueue_trans_job = NULL;

void
_engine_enqueue_trans (GslTrans *trans)
{
  g_return_if_fail (trans != NULL);
  g_return_if_fail (trans->comitted == TRUE);
  g_return_if_fail (trans->jobs_head != NULL);
  
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
  sfi_cond_broadcast (&cqueue_trans_cond);
}

void
_engine_wait_on_trans (void)
{
  GSL_SPIN_LOCK (&cqueue_trans);
  while (cqueue_trans_pending_head || cqueue_trans_active_head)
    sfi_cond_wait (&cqueue_trans_cond, &cqueue_trans);
  GSL_SPIN_UNLOCK (&cqueue_trans);
}

gboolean
_engine_job_pending (void)
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

void
_engine_free_trans (GslTrans *trans)
{
  g_return_if_fail (trans != NULL);
  g_return_if_fail (trans->comitted == FALSE);
  if (trans->jobs_tail)
    g_return_if_fail (trans->jobs_tail->next == NULL);  /* paranoid */

  GSL_SPIN_LOCK (&cqueue_trans);
  trans->cqt_next = cqueue_trans_trash;
  cqueue_trans_trash = trans;
  GSL_SPIN_UNLOCK (&cqueue_trans);
}

GslJob*
_engine_pop_job (void)
{
  /* clean up if necessary and try fetching new jobs
   */
  if (!cqueue_trans_job)	/* no transaction job in communication queue */
    {
      if (cqueue_trans_active_head)	/* currently processing transaction */
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
	  sfi_cond_broadcast (&cqueue_trans_cond);
	}
      else	/* not currently processing a transaction */
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
  
  /* no pending jobs... */
  return NULL;
}


/* --- user thread garbage collection --- */
/**
 * gsl_engine_garbage_collect
 *
 * GSL Engine user thread function. Collects processed jobs
 * and transactions from the engine and frees them. This
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
  EngineFlowJob *fjobs;

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
      if (t->jobs_tail)
	t->jobs_tail->next = NULL;
      t->comitted = FALSE;
      free_trans (t);
    }

  while (fjobs)
    {
      EngineFlowJob *j = fjobs;
      
      fjobs = j->any.next;
      j->any.next = NULL;
      free_flow_job (j);
    }
}

gboolean
gsl_engine_has_garbage (void)
{
  return cqueue_trans_trash || cqueue_trash_fjobs;
}


/* --- node processing queue --- */
static SfiMutex          pqueue_mutex = { 0, };
static EngineSchedule   *pqueue_schedule = NULL;
static guint             pqueue_n_nodes = 0;
static guint             pqueue_n_cycles = 0;
static SfiCond		 pqueue_done_cond = { 0, };
static EngineFlowJob    *pqueue_trash_fjobs_first = NULL;
static EngineFlowJob    *pqueue_trash_fjobs_last = NULL;

void
_engine_set_schedule (EngineSchedule *sched)
{
  g_return_if_fail (sched != NULL);
  g_return_if_fail (sched->secured == TRUE);
  
  GSL_SPIN_LOCK (&pqueue_mutex);
  if_reject (pqueue_schedule != NULL)
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
_engine_unset_schedule (EngineSchedule *sched)
{
  EngineFlowJob *trash_fjobs_first, *trash_fjobs_last;

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

EngineNode*
_engine_pop_unprocessed_node (void)
{
  EngineNode *node;
  
  GSL_SPIN_LOCK (&pqueue_mutex);
  node = pqueue_schedule ? _engine_schedule_pop_node (pqueue_schedule) : NULL;
  if (node)
    pqueue_n_nodes += 1;
  GSL_SPIN_UNLOCK (&pqueue_mutex);
  
  if (node)
    ENGINE_NODE_LOCK (node);
  
  return node;
}

static inline void
collect_trash_flow_jobs_L (EngineNode *node)
{
  if_reject (node->fjob_first != NULL)
    {
      /*move into flow jobs trash queue */
      node->fjob_last->any.next = pqueue_trash_fjobs_first;
      pqueue_trash_fjobs_first = node->fjob_first;
      if (!pqueue_trash_fjobs_last)
	pqueue_trash_fjobs_last = node->fjob_last;
      node->fjob_first = NULL;
      node->fjob_last = NULL;
    }
}

void
_engine_node_collect_flow_jobs (EngineNode *node)
{
  g_return_if_fail (node != NULL);
  g_return_if_fail (!ENGINE_NODE_IS_SCHEDULED (node));

  GSL_SPIN_LOCK (&pqueue_mutex);
  collect_trash_flow_jobs_L (node);
  GSL_SPIN_UNLOCK (&pqueue_mutex);
}

void
_engine_push_processed_node (EngineNode *node)
{
  g_return_if_fail (node != NULL);
  g_return_if_fail (pqueue_n_nodes > 0);
  g_return_if_fail (ENGINE_NODE_IS_SCHEDULED (node));
  
  GSL_SPIN_LOCK (&pqueue_mutex);
  g_assert (pqueue_n_nodes > 0);        /* paranoid */
  collect_trash_flow_jobs_L (node);
  pqueue_n_nodes -= 1;
  ENGINE_NODE_UNLOCK (node);
  if (!pqueue_n_nodes && !pqueue_n_cycles && GSL_SCHEDULE_NONPOPABLE (pqueue_schedule))
    sfi_cond_signal (&pqueue_done_cond);
  GSL_SPIN_UNLOCK (&pqueue_mutex);
}

SfiRing*
_engine_pop_unprocessed_cycle (void)
{
  return NULL;
}

void
_engine_push_processed_cycle (SfiRing *cycle)
{
  g_return_if_fail (cycle != NULL);
  g_return_if_fail (pqueue_n_cycles > 0);
  g_return_if_fail (ENGINE_NODE_IS_SCHEDULED (cycle->data));
}

void
_engine_wait_on_unprocessed (void)
{
  GSL_SPIN_LOCK (&pqueue_mutex);
  while (pqueue_n_nodes || pqueue_n_cycles || !GSL_SCHEDULE_NONPOPABLE (pqueue_schedule))
    sfi_cond_wait (&pqueue_done_cond, &pqueue_mutex);
  GSL_SPIN_UNLOCK (&pqueue_mutex);
}


/* --- initialization --- */
void
_gsl_init_engine_utils (void)
{
  static gboolean initialized = FALSE;

  g_assert (initialized == FALSE); /* single invocation */
  initialized++;

  sfi_mutex_init (&cqueue_trans);
  sfi_cond_init (&cqueue_trans_cond);
  sfi_mutex_init (&pqueue_mutex);
  sfi_cond_init (&pqueue_done_cond);
}


/* vim:set ts=8 sts=2 sw=2: */
