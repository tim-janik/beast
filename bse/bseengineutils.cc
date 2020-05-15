// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseengineutils.hh"
#include "bseblockutils.hh"
#include "gslcommon.hh"
#include "bseengineprivate.hh"
#include "bseengineschedule.hh"
#include "bsemathsignal.hh"
#include "bse/internal.hh"
#include <unordered_map>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define LOG_INTERN      SfiLogger ("internals", NULL, NULL)

/* --- prototypes --- */
static inline void      engine_fetch_process_queue_trash_jobs_U (Bse::EngineTimedJob **trash_tjobs_head,
                                                                 Bse::EngineTimedJob **trash_tjobs_tail);

/* --- UserThread --- */
BseOStream*
_engine_alloc_ostreams (guint n)
{
  if (n)
    {
      uint i = sizeof (BseOStream) * n + sizeof (gfloat) * BSE_ENGINE_MAX_BLOCK_SIZE * n;
      BseOStream *streams = (BseOStream*) g_malloc0 (i);
      float *buffers = (float*) (streams + n);
      for (i = 0; i < n; i++)
	{
	  streams[i].values = buffers;
	  buffers += BSE_ENGINE_MAX_BLOCK_SIZE;
	}
      return streams;
    }
  else
    return NULL;
}

static void
bse_engine_free_timed_job (Bse::EngineTimedJob *tjob)
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
      Bse::warning ("Engine: invalid user job type: %d", tjob->type);
      break;
    }
}

void
bse_engine_free_ostreams (guint         n_ostreams,
                          BseOStream   *ostreams)
{
  assert_return (n_ostreams > 0);
  /* bse_engine_block_size() may have changed since allocation */
  g_free (ostreams);
}

static void
bse_engine_free_node (Bse::Module *node)
{
  assert_return (node != NULL);
  delete node;
}

static void
bse_engine_free_job (BseJob *job)
{
  assert_return (job != NULL);

  switch (job->job_id)
    {
    case ENGINE_JOB_INTEGRATE:
    case ENGINE_JOB_DISCARD:
      if (job->data.node && job->data.free_with_job)
        bse_engine_free_node (job->data.node);
      break;
    case ENGINE_JOB_MESSAGE:
      g_free (job->msg.message);
      break;
    case ENGINE_JOB_ACCESS:
      delete job->access.function;
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
  delete job;
}

static void
bse_engine_free_transaction (BseTrans *trans)
{
  BseJob *job;

  assert_return (trans != NULL);
  assert_return (trans->comitted == FALSE);
  if (trans->jobs_tail)
    assert_return (trans->jobs_tail->next == NULL);	/* paranoid */

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
static std::mutex      cqueue_trans_mutex;
static BseTrans       *cqueue_trans_pending_head = NULL;
static BseTrans       *cqueue_trans_pending_tail = NULL;
static std::condition_variable cqueue_trans_cond;
static BseTrans       *cqueue_trans_trash_head = NULL;
static BseTrans       *cqueue_trans_trash_tail = NULL;
static BseTrans       *cqueue_trans_active_head = NULL;
static BseTrans       *cqueue_trans_active_tail = NULL;
static BseJob         *cqueue_trans_job = NULL;
static Bse::EngineTimedJob *cqueue_tjobs_trash_head = NULL;
static Bse::EngineTimedJob *cqueue_tjobs_trash_tail = NULL;
static guint64         cqueue_commit_base_stamp = 1;
guint64
_engine_enqueue_trans (BseTrans *trans)
{
  assert_return (trans != NULL, 0);
  assert_return (trans->comitted == TRUE, 0);
  assert_return (trans->jobs_head != NULL, 0);
  cqueue_trans_mutex.lock();
  if (cqueue_trans_pending_tail)
    {
      cqueue_trans_pending_tail->cqt_next = trans;
      cqueue_trans_pending_tail->jobs_tail->next = trans->jobs_head;
    }
  else
    cqueue_trans_pending_head = trans;
  cqueue_trans_pending_tail = trans;
  guint64 base_stamp = cqueue_commit_base_stamp;
  cqueue_trans_mutex.unlock();
  cqueue_trans_cond.notify_all();
  return base_stamp + bse_engine_block_size();  /* returns tick_stamp of when this transaction takes effect */
}

void
_engine_wait_on_trans (void)
{
  std::unique_lock<std::mutex> cqueue_trans_guard (cqueue_trans_mutex);
  while (cqueue_trans_pending_head || cqueue_trans_active_head)
    cqueue_trans_cond.wait (cqueue_trans_guard);
}

gboolean
_engine_job_pending (void)
{
  gboolean pending = cqueue_trans_job != NULL;

  if (!pending)
    {
      cqueue_trans_mutex.lock();
      pending = cqueue_trans_pending_head != NULL;
      cqueue_trans_mutex.unlock();
    }
  return pending;
}

void
_engine_free_trans (BseTrans *trans)
{
  assert_return (trans != NULL);
  assert_return (trans->comitted == FALSE);
  if (trans->jobs_tail)
    assert_return (trans->jobs_tail->next == NULL);  /* paranoid */
  cqueue_trans_mutex.lock();
  trans->cqt_next = NULL;
  if (cqueue_trans_trash_tail)
    cqueue_trans_trash_tail->cqt_next = trans;
  else
    cqueue_trans_trash_head = trans;
  cqueue_trans_trash_tail = trans;
  cqueue_trans_mutex.unlock();
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
      Bse::EngineTimedJob *trash_tjobs_head, *trash_tjobs_tail;
      engine_fetch_process_queue_trash_jobs_U (&trash_tjobs_head, &trash_tjobs_tail);
      if (cqueue_trans_active_head)	/* currently processing transaction */
	{
	  cqueue_trans_mutex.lock();
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
            cqueue_commit_base_stamp = Bse::TickStamp::current();        /* last job has been handed out */
	  cqueue_trans_mutex.unlock();
	  cqueue_trans_cond.notify_all();
	}
      else	/* not currently processing a transaction */
	{
	  cqueue_trans_mutex.lock();
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
            cqueue_commit_base_stamp = Bse::TickStamp::current();        /* last job has been handed out */
	  cqueue_trans_mutex.unlock();
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
  Bse::EngineTimedJob *tjobs;
  cqueue_trans_mutex.lock();
  tjobs = cqueue_tjobs_trash_head;
  cqueue_tjobs_trash_head = cqueue_tjobs_trash_tail = NULL;
  trans = cqueue_trans_trash_head;
  cqueue_trans_trash_head = cqueue_trans_trash_tail = NULL;
  cqueue_trans_mutex.unlock();
  while (tjobs)
    {
      Bse::EngineTimedJob *tjob = tjobs;
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
static std::mutex        pqueue_mutex;
static EngineSchedule   *pqueue_schedule = NULL;
static guint             pqueue_n_nodes = 0;
static guint             pqueue_n_cycles = 0;
static std::condition_variable pqueue_done_cond;
static Bse::EngineTimedJob    *pqueue_trash_tjobs_head = NULL;
static Bse::EngineTimedJob    *pqueue_trash_tjobs_tail = NULL;

static inline void
engine_fetch_process_queue_trash_jobs_U (Bse::EngineTimedJob **trash_tjobs_head,
                                         Bse::EngineTimedJob **trash_tjobs_tail)
{
  if (G_UNLIKELY (pqueue_trash_tjobs_head != NULL))
    {
      pqueue_mutex.lock();
      *trash_tjobs_head = pqueue_trash_tjobs_head;
      *trash_tjobs_tail = pqueue_trash_tjobs_tail;
      pqueue_trash_tjobs_head = pqueue_trash_tjobs_tail = NULL;
      /* this function may not be called while nodes are still being processed,
       * because some (probe) jobs may reference ro-data that is still in use
       * during processing. to ensure this, we assert that no flow processing
       * schedule is currently set.
       */
      assert_return (pqueue_schedule == NULL);
      pqueue_mutex.unlock();
    }
  else
    *trash_tjobs_head = *trash_tjobs_tail = NULL;
}
void
_engine_set_schedule (EngineSchedule *sched)
{
  assert_return (sched != NULL);
  assert_return (sched->secured == TRUE);
  pqueue_mutex.lock();
  if (UNLIKELY (pqueue_schedule != NULL))
    {
      pqueue_mutex.unlock();
      Bse::warning ("%s: schedule already set", __func__);
      return;
    }
  pqueue_schedule = sched;
  sched->in_pqueue = TRUE;
  pqueue_mutex.unlock();
}
void
_engine_unset_schedule (EngineSchedule *sched)
{
  Bse::EngineTimedJob *trash_tjobs_head, *trash_tjobs_tail;
  assert_return (sched != NULL);
  pqueue_mutex.lock();
  if (UNLIKELY (pqueue_schedule != sched))
    {
      pqueue_mutex.unlock();
      Bse::warning ("%s: schedule(%p) not currently set", __func__, sched);
      return;
    }
  if (UNLIKELY (pqueue_n_nodes || pqueue_n_cycles))
    Bse::warning ("%s: schedule(%p) still busy", __func__, sched);
  sched->in_pqueue = FALSE;
  pqueue_schedule = NULL;
  /* see engine_fetch_process_queue_trash_jobs_U() on the limitations regarding pqueue trash jobs */
  trash_tjobs_head = pqueue_trash_tjobs_head;
  trash_tjobs_tail = pqueue_trash_tjobs_tail;
  pqueue_trash_tjobs_head = pqueue_trash_tjobs_tail = NULL;
  pqueue_mutex.unlock();
  if (trash_tjobs_head) /* move trash user jobs */
    {
      cqueue_trans_mutex.lock();
      trash_tjobs_tail->next = NULL;
      if (cqueue_tjobs_trash_tail)
        cqueue_tjobs_trash_tail->next = trash_tjobs_head;
      else
        cqueue_tjobs_trash_head = trash_tjobs_head;
      cqueue_tjobs_trash_tail = trash_tjobs_tail;
      cqueue_trans_mutex.unlock();
    }
}
Bse::Module*
_engine_pop_unprocessed_node (void)
{
  Bse::Module *node;
  pqueue_mutex.lock();
  node = pqueue_schedule ? _engine_schedule_pop_node (pqueue_schedule) : NULL;
  if (node)
    {
      pqueue_n_nodes += 1;
      node->lock();
    }
  pqueue_mutex.unlock();
  return node;
}
static inline void
collect_user_jobs_L (Bse::Module *node)
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
_engine_node_collect_jobs (Bse::Module *node)
{
  assert_return (node != NULL);
  pqueue_mutex.lock();
  collect_user_jobs_L (node);
  pqueue_mutex.unlock();
}
void
_engine_push_processed_node (Bse::Module *node)
{
  assert_return (node != NULL);
  assert_return (pqueue_n_nodes > 0);
  assert_return (BSE_MODULE_IS_SCHEDULED (node));
  pqueue_mutex.lock();
  assert_return (pqueue_n_nodes > 0);        /* paranoid */
  collect_user_jobs_L (node);
  pqueue_n_nodes -= 1;
  node->unlock();
  if (!pqueue_n_nodes && !pqueue_n_cycles && BSE_ENGINE_SCHEDULE_NONPOPABLE (pqueue_schedule))
    pqueue_done_cond.notify_one();
  pqueue_mutex.unlock();
}

SfiRing*
_engine_pop_unprocessed_cycle (void)
{
  return NULL;
}

void
_engine_push_processed_cycle (SfiRing *cycle)
{
  assert_return (cycle != NULL);
  assert_return (pqueue_n_cycles > 0);
  Bse::Module *node = (Bse::Module*) cycle->data;
  assert_return (BSE_MODULE_IS_SCHEDULED (node));
}

void
_engine_wait_on_unprocessed (void)
{
  std::unique_lock<std::mutex> pqueue_guard (pqueue_mutex);
  while (pqueue_n_nodes || pqueue_n_cycles || !BSE_ENGINE_SCHEDULE_NONPOPABLE (pqueue_schedule))
    pqueue_done_cond.wait (pqueue_guard);
}


/* -- master node list --- */
static Bse::Module      *master_node_list_head = NULL;
static Bse::Module      *master_node_list_tail = NULL;

Bse::Module*
_engine_mnl_head (void)
{
  return master_node_list_head;
}

void
_engine_mnl_remove (Bse::Module *node)
{
  assert_return (node->integrated == TRUE);

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
_engine_mnl_integrate (Bse::Module *node)
{
  assert_return (node->integrated == FALSE);
  assert_return (node->flow_jobs == NULL);
  assert_return (node->boundary_jobs == NULL);

  node->integrated = TRUE;
  /* append */
  if (master_node_list_tail)
    master_node_list_tail->mnl_next = node;
  node->mnl_prev = master_node_list_tail;
  master_node_list_tail = node;
  if (!master_node_list_head)
    master_node_list_head = master_node_list_tail;
  assert_return (node->mnl_next == NULL);
}

void
_engine_mnl_node_changed (Bse::Module *node)
{
  Bse::Module *sibling;

  assert_return (node->integrated == TRUE);

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
      pqueue_mutex.lock();
      collect_user_jobs_L (node);
      pqueue_mutex.unlock();
    }
}


/* --- const value blocks --- */
typedef std::unordered_map<float, const float*> FloatBlockMap;
static std::mutex    engine_const_value_mutex;
static FloatBlockMap engine_const_value_map;
static const float   engine_const_values_0[BSE_ENGINE_MAX_BLOCK_SIZE + 16] = { 0 }; // 0.0...

float*
bse_engine_const_values (float value)
{
  if (value == 0.0)
    return const_cast<float*> (engine_const_values_0);
  std::lock_guard<std::mutex> guard (engine_const_value_mutex);
  const float *&block = engine_const_value_map[value];
  if (block == NULL)
    {
      float *value_block = new float[BSE_ENGINE_MAX_BLOCK_SIZE + 16];
      bse_block_fill_float (BSE_ENGINE_MAX_BLOCK_SIZE + 16, value_block, value);
      block = value_block;
    }
  return const_cast<float*> (block);
}

float*
bse_engine_const_zeros (uint smaller_than_MAX_BLOCK_SIZE)
{
  assert_return (smaller_than_MAX_BLOCK_SIZE <= BSE_ENGINE_MAX_BLOCK_SIZE, NULL);
  return const_cast<float*> (engine_const_values_0);
}

void
_engine_recycle_const_values (bool remove_all)
{
  if (!remove_all)
    return;
  std::lock_guard<std::mutex> guard (engine_const_value_mutex);
  for (const auto &it : engine_const_value_map)
    delete[] it.second;
  engine_const_value_map.clear();
}
