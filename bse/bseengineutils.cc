// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseengineutils.hh"
#include "bseblockutils.hh"
#include "gslcommon.hh"
#include "bseenginenode.hh"
#include "bseengineschedule.hh"
#include "bsemathsignal.hh"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define LOG_INTERN      SfiLogger ("internals", NULL, NULL)

/* --- prototypes --- */
static inline void      engine_fetch_process_queue_trash_jobs_U (EngineTimedJob **trash_tjobs_head,
                                                                 EngineTimedJob **trash_tjobs_tail);

/* --- UserThread --- */
BseOStream*
_engine_alloc_ostreams (guint n)
{
  if (n)
    {
      guint i = sizeof (BseOStream) * n + sizeof (gfloat) * bse_engine_block_size () * n;
      BseOStream *streams = (BseOStream*) g_malloc0 (i);
      float *buffers = (float*) (streams + n);
      for (i = 0; i < n; i++)
	{
	  streams[i].values = buffers;
	  buffers += bse_engine_block_size ();
	}
      return streams;
    }
  else
    return NULL;
}

static void
bse_engine_free_timed_job (EngineTimedJob *tjob)
{
  switch (tjob->type)
    {
      BseOStream *ostreams;
    case ENGINE_JOB_PROBE_JOB:
      ostreams = tjob->probe.ostreams;
      if (tjob->probe.probe_func)
        tjob->probe.probe_func (tjob->probe.data, bse_engine_block_size(), tjob->tick_stamp,
                                tjob->probe.n_ostreams, &ostreams);
      if (ostreams)
        bse_engine_free_ostreams (tjob->probe.n_ostreams, ostreams);
      g_free (tjob);
      break;
    case ENGINE_JOB_FLOW_JOB:
    case ENGINE_JOB_BOUNDARY_JOB:
      if (tjob->access.free_func)
        tjob->access.free_func (tjob->access.data);
      g_free (tjob);
      break;
    default:
      g_warning ("Engine: invalid user job type: %d", tjob->type);
      break;
    }
}

void
bse_engine_free_ostreams (guint         n_ostreams,
                          BseOStream   *ostreams)
{
  g_assert (n_ostreams > 0);
  /* bse_engine_block_size() may have changed since allocation */
  g_free (ostreams);
}

static void
bse_engine_free_node (EngineNode *node)
{
  const BseModuleClass *klass;
  gpointer user_data;
  guint j;
  
  g_return_if_fail (node != NULL);
  g_return_if_fail (node->output_nodes == NULL);
  g_return_if_fail (node->integrated == FALSE);
  g_return_if_fail (node->sched_tag == FALSE);
  g_return_if_fail (node->sched_recurse_tag == FALSE);
  g_return_if_fail (node->flow_jobs == NULL);
  g_return_if_fail (node->boundary_jobs == NULL);
  g_return_if_fail (node->tjob_head == NULL);
  g_return_if_fail (node->probe_jobs == NULL);

  sfi_rec_mutex_destroy (&node->rec_mutex);
  if (node->module.ostreams)
    {
      /* bse_engine_block_size() may have changed since allocation */
      bse_engine_free_ostreams (ENGINE_NODE_N_OSTREAMS (node), node->module.ostreams);
      sfi_delete_structs (EngineOutput, ENGINE_NODE_N_OSTREAMS (node), node->outputs);
    }
  if (node->module.istreams)
    {
      sfi_delete_structs (BseIStream, ENGINE_NODE_N_ISTREAMS (node), node->module.istreams);
      sfi_delete_structs (EngineInput, ENGINE_NODE_N_ISTREAMS (node), node->inputs);
    }
  for (j = 0; j < ENGINE_NODE_N_JSTREAMS (node); j++)
    {
      g_free (node->jinputs[j]);
      g_free (node->module.jstreams[j].values);
    }
  if (node->module.jstreams)
    {
      sfi_delete_structs (BseJStream, ENGINE_NODE_N_JSTREAMS (node), node->module.jstreams);
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
bse_engine_free_job (BseJob *job)
{
  g_return_if_fail (job != NULL);
  
  switch (job->job_id)
    {
    case ENGINE_JOB_INTEGRATE:
    case ENGINE_JOB_DISCARD:
    case ENGINE_JOB_MESSAGE:
      if (job->data.node && job->data.free_with_job)
        bse_engine_free_node (job->data.node);
      g_free (job->data.message);
      break;
    case ENGINE_JOB_ACCESS:
      if (job->access.free_func)
	job->access.free_func (job->access.data);
      break;
    case ENGINE_JOB_ADD_POLL:
    case ENGINE_JOB_REMOVE_POLL:
      g_free (job->poll.fds);
      if (job->poll.free_func)
	job->poll.free_func (job->poll.data);
      break;
    case ENGINE_JOB_ADD_TIMER:
      if (job->timer.free_func)
	job->timer.free_func (job->timer.data);
      break;
    case ENGINE_JOB_PROBE_JOB:
    case ENGINE_JOB_FLOW_JOB:
    case ENGINE_JOB_BOUNDARY_JOB:
      if (job->timed_job.tjob)
        bse_engine_free_timed_job (job->timed_job.tjob);
      break;
    default: ;
    }
  sfi_delete_struct (BseJob, job);
}

static void
bse_engine_free_transaction (BseTrans *trans)
{
  BseJob *job;
  
  g_return_if_fail (trans != NULL);
  g_return_if_fail (trans->comitted == FALSE);
  if (trans->jobs_tail)
    g_return_if_fail (trans->jobs_tail->next == NULL);	/* paranoid */
  
  job = trans->jobs_head;
  while (job)
    {
      BseJob *tmp = job->next;
      
      bse_engine_free_job (job);
      job = tmp;
    }
  sfi_delete_struct (BseTrans, trans);
}


/* --- job transactions --- */
static BirnetMutex        cqueue_trans = { 0, };
static BseTrans       *cqueue_trans_pending_head = NULL;
static BseTrans       *cqueue_trans_pending_tail = NULL;
static BirnetCond         cqueue_trans_cond = { 0, };
static BseTrans       *cqueue_trans_trash_head = NULL;
static BseTrans       *cqueue_trans_trash_tail = NULL;
static BseTrans       *cqueue_trans_active_head = NULL;
static BseTrans       *cqueue_trans_active_tail = NULL;
static BseJob         *cqueue_trans_job = NULL;
static EngineTimedJob *cqueue_tjobs_trash_head = NULL;
static EngineTimedJob *cqueue_tjobs_trash_tail = NULL;
static guint64         cqueue_commit_base_stamp = 1;

guint64
_engine_enqueue_trans (BseTrans *trans)
{
  g_return_val_if_fail (trans != NULL, 0);
  g_return_val_if_fail (trans->comitted == TRUE, 0);
  g_return_val_if_fail (trans->jobs_head != NULL, 0);
  
  GSL_SPIN_LOCK (&cqueue_trans);
  if (cqueue_trans_pending_tail)
    {
      cqueue_trans_pending_tail->cqt_next = trans;
      cqueue_trans_pending_tail->jobs_tail->next = trans->jobs_head;
    }
  else
    cqueue_trans_pending_head = trans;
  cqueue_trans_pending_tail = trans;
  guint64 base_stamp = cqueue_commit_base_stamp;
  GSL_SPIN_UNLOCK (&cqueue_trans);
  sfi_cond_broadcast (&cqueue_trans_cond);
  return base_stamp + bse_engine_block_size();  /* returns tick_stamp of when this transaction takes effect */
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
_engine_free_trans (BseTrans *trans)
{
  g_return_if_fail (trans != NULL);
  g_return_if_fail (trans->comitted == FALSE);
  if (trans->jobs_tail)
    g_return_if_fail (trans->jobs_tail->next == NULL);  /* paranoid */
  
  GSL_SPIN_LOCK (&cqueue_trans);
  trans->cqt_next = NULL;
  if (cqueue_trans_trash_tail)
    cqueue_trans_trash_tail->cqt_next = trans;
  else
    cqueue_trans_trash_head = trans;
  cqueue_trans_trash_tail = trans;
  GSL_SPIN_UNLOCK (&cqueue_trans);
}

BseJob*
_engine_pop_job (gboolean update_commit_stamp)
{
  /* update_commit_stamp really means "this is the last time _engine_pop_job()
   * is called during this tick_stamp cycle, unless it returns a job != NULL".
   * based on that, we can update the time_stamp returned by trans_commit() if
   * we actually return a NULL job.
   */
  /* clean up if necessary and try fetching new jobs */
  if (!cqueue_trans_job)	/* no transaction job in communication queue */
    {
      /* before adding trash jobs to the cqueue, make sure pending pqueue trash
       * jobs can be collected (these need to be collected first in the USerThread).
       */
      EngineTimedJob *trash_tjobs_head, *trash_tjobs_tail;
      engine_fetch_process_queue_trash_jobs_U (&trash_tjobs_head, &trash_tjobs_tail);
      if (cqueue_trans_active_head)	/* currently processing transaction */
	{
	  GSL_SPIN_LOCK (&cqueue_trans);
          if (trash_tjobs_head)        /* move trash user jobs */
            {
              trash_tjobs_tail->next = NULL;
              if (cqueue_tjobs_trash_tail)
                cqueue_tjobs_trash_tail->next = trash_tjobs_head;
              else
                cqueue_tjobs_trash_head = trash_tjobs_head;
              cqueue_tjobs_trash_tail = trash_tjobs_tail;
            }
	  /* get rid of processed transaction and
	   * signal UserThread which might be in
	   * op_com_wait_on_trans()
	   */
          cqueue_trans_active_tail->cqt_next = NULL;
          if (cqueue_trans_trash_tail)
            cqueue_trans_trash_tail->cqt_next = cqueue_trans_active_head;
          else
            cqueue_trans_trash_head = cqueue_trans_active_head;
          cqueue_trans_trash_tail = cqueue_trans_active_tail;
	  /* fetch new transaction */
	  cqueue_trans_active_head = cqueue_trans_pending_head;
	  cqueue_trans_active_tail = cqueue_trans_pending_tail;
	  cqueue_trans_pending_head = NULL;
	  cqueue_trans_pending_tail = NULL;
          cqueue_trans_job = cqueue_trans_active_head ? cqueue_trans_active_head->jobs_head : NULL;
          if (!cqueue_trans_job && update_commit_stamp)
            cqueue_commit_base_stamp = gsl_tick_stamp();        /* last job has been handed out */
	  GSL_SPIN_UNLOCK (&cqueue_trans);
	  sfi_cond_broadcast (&cqueue_trans_cond);
	}
      else	/* not currently processing a transaction */
	{
	  GSL_SPIN_LOCK (&cqueue_trans);
          if (trash_tjobs_head)                                 /* move trash user jobs */
            {
              trash_tjobs_tail->next = NULL;
              if (cqueue_tjobs_trash_tail)
                cqueue_tjobs_trash_tail->next = trash_tjobs_head;
              else
                cqueue_tjobs_trash_head = trash_tjobs_head;
              cqueue_tjobs_trash_tail = trash_tjobs_tail;
            }
	  /* fetch new transaction */
	  cqueue_trans_active_head = cqueue_trans_pending_head;
	  cqueue_trans_active_tail = cqueue_trans_pending_tail;
	  cqueue_trans_pending_head = NULL;
	  cqueue_trans_pending_tail = NULL;
          cqueue_trans_job = cqueue_trans_active_head ? cqueue_trans_active_head->jobs_head : NULL;
          if (!cqueue_trans_job && update_commit_stamp)
            cqueue_commit_base_stamp = gsl_tick_stamp();        /* last job has been handed out */
	  GSL_SPIN_UNLOCK (&cqueue_trans);
	}
    }
  
  /* pick new job and out of here */
  if (cqueue_trans_job)
    {
      BseJob *job = cqueue_trans_job;
      cqueue_trans_job = job->next;
      return job;
    }
  
  /* no pending jobs... */
  return NULL;
}


/* --- user thread garbage collection --- */
/**
 * BSE Engine user thread function. Collects processed jobs
 * and transactions from the engine and frees them. This
 * involves callback invocation of BseFreeFunc() functions,
 * e.g. from bse_job_access() or bse_job_flow_access()
 * jobs.
 * This function may only be called from the user thread,
 * as BseFreeFunc() functions are guranteed to be executed
 * in the user thread.
 */
void
bse_engine_user_thread_collect (void)
{
  BseTrans *trans;
  EngineTimedJob *tjobs;
  
  GSL_SPIN_LOCK (&cqueue_trans);
  tjobs = cqueue_tjobs_trash_head;
  cqueue_tjobs_trash_head = cqueue_tjobs_trash_tail = NULL;
  trans = cqueue_trans_trash_head;
  cqueue_trans_trash_head = cqueue_trans_trash_tail = NULL;
  GSL_SPIN_UNLOCK (&cqueue_trans);
  
  while (tjobs)
    {
      EngineTimedJob *tjob = tjobs;
      tjobs = tjob->next;
      tjob->next = NULL;
      bse_engine_free_timed_job (tjob);
    }
  
  while (trans)
    {
      BseTrans *t = trans;
      trans = t->cqt_next;
      t->cqt_next = NULL;
      if (t->jobs_tail)
	t->jobs_tail->next = NULL;
      t->comitted = FALSE;
      bse_engine_free_transaction (t);
    }
}

gboolean
bse_engine_has_garbage (void)
{
  return cqueue_tjobs_trash_head || cqueue_trans_trash_head;
}


/* --- node processing queue --- */
static BirnetMutex       pqueue_mutex = { 0, };
static EngineSchedule   *pqueue_schedule = NULL;
static guint             pqueue_n_nodes = 0;
static guint             pqueue_n_cycles = 0;
static BirnetCond	 pqueue_done_cond = { 0, };
static EngineTimedJob   *pqueue_trash_tjobs_head = NULL;
static EngineTimedJob   *pqueue_trash_tjobs_tail = NULL;

static inline void
engine_fetch_process_queue_trash_jobs_U (EngineTimedJob **trash_tjobs_head,
                                         EngineTimedJob **trash_tjobs_tail)
{
  if (G_UNLIKELY (pqueue_trash_tjobs_head != NULL))
    {
      GSL_SPIN_LOCK (&pqueue_mutex);
      *trash_tjobs_head = pqueue_trash_tjobs_head;
      *trash_tjobs_tail = pqueue_trash_tjobs_tail;
      pqueue_trash_tjobs_head = pqueue_trash_tjobs_tail = NULL;
      /* this function may not be called while nodes are still being processed,
       * because some (probe) jobs may reference ro-data that is still in use
       * during processing. to ensure this, we assert that no flow processing
       * schedule is currently set.
       */
      g_assert (pqueue_schedule == NULL);
      GSL_SPIN_UNLOCK (&pqueue_mutex);
    }
  else
    *trash_tjobs_head = *trash_tjobs_tail = NULL;
}

void
_engine_set_schedule (EngineSchedule *sched)
{
  g_return_if_fail (sched != NULL);
  g_return_if_fail (sched->secured == TRUE);
  
  GSL_SPIN_LOCK (&pqueue_mutex);
  if (UNLIKELY (pqueue_schedule != NULL))
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
  EngineTimedJob *trash_tjobs_head, *trash_tjobs_tail;
  
  g_return_if_fail (sched != NULL);
  
  GSL_SPIN_LOCK (&pqueue_mutex);
  if (UNLIKELY (pqueue_schedule != sched))
    {
      GSL_SPIN_UNLOCK (&pqueue_mutex);
      g_warning (G_STRLOC ": schedule(%p) not currently set", sched);
      return;
    }
  if (UNLIKELY (pqueue_n_nodes || pqueue_n_cycles))
    g_warning (G_STRLOC ": schedule(%p) still busy", sched);
  sched->in_pqueue = FALSE;
  pqueue_schedule = NULL;
  /* see engine_fetch_process_queue_trash_jobs_U() on the limitations regarding pqueue trash jobs */
  trash_tjobs_head = pqueue_trash_tjobs_head;
  trash_tjobs_tail = pqueue_trash_tjobs_tail;
  pqueue_trash_tjobs_head = pqueue_trash_tjobs_tail = NULL;
  GSL_SPIN_UNLOCK (&pqueue_mutex);

  if (trash_tjobs_head) /* move trash user jobs */
    {
      GSL_SPIN_LOCK (&cqueue_trans);
      trash_tjobs_tail->next = NULL;
      if (cqueue_tjobs_trash_tail)
        cqueue_tjobs_trash_tail->next = trash_tjobs_head;
      else
        cqueue_tjobs_trash_head = trash_tjobs_head;
      cqueue_tjobs_trash_tail = trash_tjobs_tail;
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
collect_user_jobs_L (EngineNode *node)
{
  if (UNLIKELY (node->tjob_head != NULL))
    {
      /* move into timed jobs trash queue */
      node->tjob_tail->next = NULL;
      if (pqueue_trash_tjobs_tail)
        pqueue_trash_tjobs_tail->next = node->tjob_head;
      else
        pqueue_trash_tjobs_head = node->tjob_head;
      pqueue_trash_tjobs_tail = node->tjob_tail;
      node->tjob_head = node->tjob_tail = NULL;
    }
}

void
_engine_node_collect_jobs (EngineNode *node)
{
  g_return_if_fail (node != NULL);
  
  GSL_SPIN_LOCK (&pqueue_mutex);
  collect_user_jobs_L (node);
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
  collect_user_jobs_L (node);
  pqueue_n_nodes -= 1;
  ENGINE_NODE_UNLOCK (node);
  if (!pqueue_n_nodes && !pqueue_n_cycles && BSE_ENGINE_SCHEDULE_NONPOPABLE (pqueue_schedule))
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
  while (pqueue_n_nodes || pqueue_n_cycles || !BSE_ENGINE_SCHEDULE_NONPOPABLE (pqueue_schedule))
    sfi_cond_wait (&pqueue_done_cond, &pqueue_mutex);
  GSL_SPIN_UNLOCK (&pqueue_mutex);
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
  g_return_if_fail (node->boundary_jobs == NULL);
  
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
_engine_mnl_node_changed (EngineNode *node)
{
  EngineNode *sibling;
  
  g_return_if_fail (node->integrated == TRUE);
  
  /* the master node list is partially sorted. that is, all
   * nodes which are not scheduled and have pending user jobs
   * are agglomerated at the head.
   */
  sibling = node->mnl_prev ? node->mnl_prev : node->mnl_next;
  if (UNLIKELY (sibling && BSE_ENGINE_MNL_UNSCHEDULED_TJOB_NODE (node) != BSE_ENGINE_MNL_UNSCHEDULED_TJOB_NODE (sibling)))
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
      if (BSE_ENGINE_MNL_UNSCHEDULED_TJOB_NODE (node))	/* move towards head */
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
  if (UNLIKELY (node->tjob_head != NULL))
    {
      GSL_SPIN_LOCK (&pqueue_mutex);
      collect_user_jobs_L (node);
      GSL_SPIN_UNLOCK (&pqueue_mutex);
    }
}


/* --- const value blocks --- */
float*
bse_engine_const_zeros (guint smaller_than_BSE_STREAM_MAX_VALUES)
{
  static const float engine_const_zero_block[BSE_STREAM_MAX_VALUES + 16 /* SIMD alignment */] = { 0, };
  /* this function is callable from any thread */
  g_assert (smaller_than_BSE_STREAM_MAX_VALUES <= BSE_STREAM_MAX_VALUES);
  return (float*) engine_const_zero_block;
}

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
	  if (cmp > BSE_SIGNAL_EPSILON)
	    {
	      n_nodes -= i;
	      nodes = check;
	    }
	  else if (cmp < -BSE_SIGNAL_EPSILON)
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
      uint new_size = upper_power2 (sizeof (float*));
      array->nodes = (float**) g_realloc (array->nodes, new_size);
      array->nodes_used = (guint8*) g_realloc (array->nodes_used, new_size / sizeof (gfloat*));
      array->n_nodes = 1;
      g_assert (index == 0);
    }
  else
    {
      uint n_nodes = array->n_nodes++;
      if (*array->nodes[index] < *value_block)
	index++;
      if (1)
	{
	  uint new_size = upper_power2 (array->n_nodes * sizeof (gfloat*));
	  uint old_size = upper_power2 (n_nodes * sizeof (gfloat*));
	  if (new_size != old_size)
	    {
	      array->nodes = (float**) g_realloc (array->nodes, new_size);
	      array->nodes_used = (guint8*) g_realloc (array->nodes_used, new_size / sizeof (float*));
	    }
	}
      g_memmove (array->nodes + index + 1, array->nodes + index, (n_nodes - index) * sizeof (array->nodes[0]));
      g_memmove (array->nodes_used + index + 1, array->nodes_used + index, (n_nodes - index) * sizeof (array->nodes_used[0]));
    }
  array->nodes[index] = value_block;
  array->nodes_used[index] = CONST_VALUES_EXPIRE;
}

static ConstValuesArray cvalue_array = { 0, NULL, NULL };

float*
bse_engine_const_values (gfloat value)
{
  if (fabs (value) < BSE_SIGNAL_EPSILON)
    return bse_engine_const_zeros (BSE_STREAM_MAX_VALUES);

  float **block = const_values_lookup_nextmost (&cvalue_array, value);
  /* found correct match? */
  if (block && fabs (**block - value) < BSE_SIGNAL_EPSILON)
    {
      cvalue_array.nodes_used[block - cvalue_array.nodes] = CONST_VALUES_EXPIRE;
      return *block;
    }
  else
    {
      /* create new value block */
      gfloat *values = g_new (gfloat, bse_engine_block_size ());
      bse_block_fill_float (bse_engine_block_size(), values, value);
      if (block)
	const_values_insert (&cvalue_array, block - cvalue_array.nodes, values);
      else
	const_values_insert (&cvalue_array, 0, values);
      
      return values;
    }
}

void
_engine_recycle_const_values (gboolean nuke_all)
{
  gfloat **nodes = cvalue_array.nodes;
  guint8 *used = cvalue_array.nodes_used;
  guint count = cvalue_array.n_nodes, e = 0, i;
  
  for (i = 0; i < count; i++)
    {
      if (nuke_all)
        used[i] = 0;
      else
        used[i]--;      /* invariant: use counts are never 0 */
      
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

/* --- initialization --- */
void
bse_engine_reinit_utils (void)
{
  static gboolean initialized = FALSE;
  if (!initialized)
    {
      initialized = TRUE;
      sfi_mutex_init (&cqueue_trans);
      sfi_cond_init (&cqueue_trans_cond);
      sfi_mutex_init (&pqueue_mutex);
      sfi_cond_init (&pqueue_done_cond);
    }
}

/* vim:set ts=8 sts=2 sw=2: */
