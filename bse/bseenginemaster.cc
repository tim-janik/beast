// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseenginemaster.hh"
#include "bseblockutils.hh"
#include "gslcommon.hh"
#include "bsemain.hh" /* bse_log_handler */
#include "bseenginenode.hh"
#include "bseengineutils.hh"
#include "bseengineschedule.hh"
#include "bseieee754.hh"
#include <string.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <errno.h>

#define JOB_DEBUG(...)  BSE_KEY_DEBUG ("job", __VA_ARGS__)
#define TJOB_DEBUG(...) BSE_KEY_DEBUG ("tjob", __VA_ARGS__)

#define	NODE_FLAG_RECONNECT(node)  G_STMT_START { /*(node)->needs_reset = TRUE*/; } G_STMT_END
/* --- time stamping (debugging) --- */
#define	ToyprofStamp		struct timeval
#define	toyprof_clock_name()	("Glibc gettimeofday(2)")
#define toyprof_stampinit()	/* nothing */
#define	toyprof_stamp(st)	gettimeofday (&(st), 0)
#define	toyprof_stamp_ticks()	(1000000)
static inline guint64
toyprof_elapsed (ToyprofStamp fstamp,
		 ToyprofStamp lstamp)
{
  guint64 first = fstamp.tv_sec * toyprof_stamp_ticks () + fstamp.tv_usec;
  guint64 last  = lstamp.tv_sec * toyprof_stamp_ticks () + lstamp.tv_usec;
  return last - first;
}
/* --- typedefs & structures --- */
typedef struct _Poll Poll;
struct _Poll
{
  Poll	     *next;
  BseEnginePollFunc poll_func;
  gpointer    data;
  guint       n_fds;
  GPollFD    *fds;
  BseFreeFunc free_func;
};
typedef struct _Timer Timer;
struct _Timer
{
  Timer             *next;
  BseEngineTimerFunc timer_func;
  gpointer           data;
  BseFreeFunc        free_func;
};
/* --- prototypes --- */
static void	master_schedule_discard	(void);
/* --- variables --- */
static gboolean	       master_need_reflow = FALSE;
static gboolean	       master_need_process = FALSE;
static EngineNode     *master_consumer_list = NULL;
static Timer	      *master_timer_list = NULL;
static Poll	      *master_poll_list = NULL;
static guint           master_n_pollfds = 0;
static guint           master_pollfds_changed = FALSE;
static GPollFD         master_pollfds[BSE_ENGINE_MAX_POLLFDS];
static EngineSchedule *master_schedule = NULL;
static SfiRing        *boundary_node_list = NULL;
static gboolean        master_new_boundary_jobs = FALSE;
static SfiRing        *probe_node_list = NULL;
/* --- node state functions --- */
static void
add_consumer (EngineNode *node)
{
  g_return_if_fail (ENGINE_NODE_IS_CONSUMER (node) && node->toplevel_next == NULL && node->integrated);
  node->toplevel_next = master_consumer_list;
  master_consumer_list = node;
}
static void
remove_consumer (EngineNode *node)
{
  EngineNode *tmp, *last = NULL;
  g_return_if_fail (!ENGINE_NODE_IS_CONSUMER (node) || !node->integrated);
  for (tmp = master_consumer_list; tmp; last = tmp, tmp = last->toplevel_next)
    if (tmp == node)
      break;
  g_return_if_fail (tmp != NULL);
  if (last)
    last->toplevel_next = node->toplevel_next;
  else
    master_consumer_list = node->toplevel_next;
  node->toplevel_next = NULL;
}
static void
propagate_update_suspend (EngineNode *node)
{
  if (!node->update_suspend)
    {
      guint i, j;
      node->update_suspend = TRUE;
      for (i = 0; i < ENGINE_NODE_N_ISTREAMS (node); i++)
	if (node->inputs[i].src_node)
	  propagate_update_suspend (node->inputs[i].src_node);
      for (j = 0; j < ENGINE_NODE_N_JSTREAMS (node); j++)
	for (i = 0; i < node->module.jstreams[j].jcount; i++)
	  propagate_update_suspend (node->jinputs[j][i].src_node);
    }
}
static void
master_idisconnect_node (EngineNode *node,
			 guint       istream)
{
  EngineNode *src_node = node->inputs[istream].src_node;
  guint ostream = node->inputs[istream].src_stream;
  gboolean was_consumer;
  g_assert (ostream < ENGINE_NODE_N_OSTREAMS (src_node) &&
	    src_node->outputs[ostream].n_outputs > 0);	/* these checks better pass */
  node->inputs[istream].src_node = NULL;
  node->inputs[istream].src_stream = ~0;
  node->module.istreams[istream].connected = 0;	/* scheduler update */
  was_consumer = ENGINE_NODE_IS_CONSUMER (src_node);
  src_node->outputs[ostream].n_outputs -= 1;
  src_node->module.ostreams[ostream].connected = 0; /* scheduler update */
  src_node->output_nodes = sfi_ring_remove (src_node->output_nodes, node);
  NODE_FLAG_RECONNECT (node);
  NODE_FLAG_RECONNECT (src_node);
  /* update suspension state of input */
  propagate_update_suspend (src_node);
  /* add to consumer list */
  if (!was_consumer && ENGINE_NODE_IS_CONSUMER (src_node))
    add_consumer (src_node);
}
static void
master_jdisconnect_node (EngineNode *node,
			 guint       jstream,
			 guint       con)
{
  EngineNode *src_node = node->jinputs[jstream][con].src_node;
  guint i, ostream = node->jinputs[jstream][con].src_stream;
  gboolean was_consumer;
  g_assert (ostream < ENGINE_NODE_N_OSTREAMS (src_node) &&
	    node->module.jstreams[jstream].jcount > 0 &&
	    src_node->outputs[ostream].n_outputs > 0);	/* these checks better pass */
  i = --node->module.jstreams[jstream].jcount;
  node->jinputs[jstream][con] = node->jinputs[jstream][i];
  node->module.jstreams[jstream].values[i] = NULL; /* float**values 0-termination */
  was_consumer = ENGINE_NODE_IS_CONSUMER (src_node);
  src_node->outputs[ostream].n_outputs -= 1;
  src_node->module.ostreams[ostream].connected = 0; /* scheduler update */
  src_node->output_nodes = sfi_ring_remove (src_node->output_nodes, node);
  NODE_FLAG_RECONNECT (node);
  NODE_FLAG_RECONNECT (src_node);
  /* update suspension state of input */
  propagate_update_suspend (src_node);
  /* add to consumer list */
  if (!was_consumer && ENGINE_NODE_IS_CONSUMER (src_node))
    add_consumer (src_node);
}
static void
master_disconnect_node_outputs (EngineNode *src_node, EngineNode *dest_node)
{
  for (uint i = 0; i < ENGINE_NODE_N_ISTREAMS (dest_node); i++)
    if (dest_node->inputs[i].src_node == src_node)
      master_idisconnect_node (dest_node, i);
  for (uint j = 0; j < ENGINE_NODE_N_JSTREAMS (dest_node); j++)
    for (uint i = 0; i < dest_node->module.jstreams[j].jcount; i++)
      if (dest_node->jinputs[j][i].src_node == src_node)
	master_jdisconnect_node (dest_node, j, i--);
}
/* --- timed job handling --- */
static inline void
insert_trash_job (EngineNode     *node,
                  EngineTimedJob *tjob)
{
  tjob->next = NULL;
  if (node->tjob_tail)
    node->tjob_tail->next = tjob;
  else
    node->tjob_head = tjob;
  node->tjob_tail = tjob;
}
static inline EngineTimedJob*
node_pop_flow_job (EngineNode  *node,
                   guint64      tick_stamp)
{
  EngineTimedJob *tjob = node->flow_jobs;
  if (UNLIKELY (tjob != NULL))
    {
      if (tjob->tick_stamp <= tick_stamp)
        {
          node->flow_jobs = tjob->next;
          insert_trash_job (node, tjob);
        }
      else
        tjob = NULL;
    }
  return tjob;
}
static inline EngineTimedJob*
node_pop_boundary_job (EngineNode  *node,
                       guint64      tick_stamp,
                       SfiRing     *blist_node)
{
  EngineTimedJob *tjob = node->boundary_jobs;
  if (tjob != NULL)
    {
      if (tjob->tick_stamp <= tick_stamp)
        {
          node->boundary_jobs = tjob->next;
          insert_trash_job (node, tjob);
          if (!node->boundary_jobs)
            boundary_node_list = sfi_ring_remove_node (boundary_node_list, blist_node);
        }
      else
        tjob = NULL;
    }
  return tjob;
}
static inline EngineTimedJob*
insert_timed_job (EngineTimedJob *head,
                  EngineTimedJob *tjob)
{
  EngineTimedJob *last = NULL, *tmp = head;
  /* find next position */
  while (tmp && tmp->tick_stamp <= tjob->tick_stamp)
    {
      last = tmp;
      tmp = last->next;
    }
  /* insert before */
  tjob->next = tmp;
  if (last)
    last->next = tjob;
  else
    head = tjob;
  return head;
}
static inline guint64
node_peek_flow_job_stamp (EngineNode *node)
{
  EngineTimedJob *tjob = node->flow_jobs;
  if (UNLIKELY (tjob != NULL))
    return tjob->tick_stamp;
  return Bse::TickStamp::max_stamp();
}
static inline guint64
node_peek_boundary_job_stamp (EngineNode *node)
{
  EngineTimedJob *tjob = node->boundary_jobs;
  if (UNLIKELY (tjob != NULL))
    return tjob->tick_stamp;
  return Bse::TickStamp::max_stamp();
}
/* --- job processing --- */
static void
master_process_job (BseJob *job)
{
  switch (job->job_id)
    {
      EngineNode *node, *src_node;
      Poll *poll, *poll_last;
      Timer *timer;
      guint64 stamp;
      guint istream, jstream, ostream, con;
      EngineTimedJob *tjob;
      gboolean was_consumer;
    case ENGINE_JOB_SYNC:
      JOB_DEBUG ("sync");
      master_need_reflow |= TRUE;
      master_schedule_discard();
      job->sync.lock_mutex->lock();
      *job->sync.lock_p = TRUE;
      job->sync.lock_cond->signal();
      while (*job->sync.lock_p)
        job->sync.lock_cond->wait (*job->sync.lock_mutex);
      job->sync.lock_mutex->unlock();
      break;
    case ENGINE_JOB_INTEGRATE:
      node = job->data.node;
      JOB_DEBUG ("integrate(%p)", node);
      g_return_if_fail (node->integrated == FALSE);
      g_return_if_fail (node->sched_tag == FALSE);
      job->data.free_with_job = FALSE;  /* ownership taken over */
      _engine_mnl_integrate (node);
      if (ENGINE_NODE_IS_CONSUMER (node))
	add_consumer (node);
      node->counter = Bse::TickStamp::current();
      NODE_FLAG_RECONNECT (node);
      node->local_active = 0;   /* by default not suspended */
      node->update_suspend = TRUE;
      node->needs_reset = TRUE;
      master_need_reflow |= TRUE;
      break;
    case ENGINE_JOB_KILL_INPUTS:
      node = job->data.node;
      JOB_DEBUG ("kill_inputs(%p)", node);
      g_return_if_fail (node->integrated == TRUE);
      for (istream = 0; istream < ENGINE_NODE_N_ISTREAMS (node); istream++)
	if (node->inputs[istream].src_node)
	  master_idisconnect_node (node, istream);
      for (jstream = 0; jstream < ENGINE_NODE_N_JSTREAMS (node); jstream++)
	while (node->module.jstreams[jstream].jcount)
	  master_jdisconnect_node (node, jstream, node->module.jstreams[jstream].jcount - 1);
      master_need_reflow |= TRUE;
      break;
    case ENGINE_JOB_KILL_OUTPUTS:
      node = job->data.node;
      JOB_DEBUG ("kill_outputs(%p)", node);
      g_return_if_fail (node->integrated == TRUE);
      while (node->output_nodes)
	master_disconnect_node_outputs (node, (EngineNode*) node->output_nodes->data);
      master_need_reflow |= TRUE;
      break;
    case ENGINE_JOB_DISCARD:
      node = job->data.node;
      JOB_DEBUG ("discard(%p, %p)", node, node->module.klass);
      g_return_if_fail (node->integrated == TRUE);
      job->data.free_with_job = TRUE;  /* ownership passed on to cause destruction in UserThread */
      /* discard schedule so node may be freed */
      master_need_reflow |= TRUE;
      master_schedule_discard ();
      /* kill inputs */
      for (istream = 0; istream < ENGINE_NODE_N_ISTREAMS (node); istream++)
	if (node->inputs[istream].src_node)
	  master_idisconnect_node (node, istream);
      for (jstream = 0; jstream < ENGINE_NODE_N_JSTREAMS (node); jstream++)
	while (node->module.jstreams[jstream].jcount)
	  master_jdisconnect_node (node, jstream, node->module.jstreams[jstream].jcount - 1);
      /* kill outputs */
      while (node->output_nodes)
	master_disconnect_node_outputs (node, (EngineNode*) node->output_nodes->data);
      /* remove from consumer list */
      if (ENGINE_NODE_IS_CONSUMER (node))
	{
	  _engine_mnl_remove (node);
	  remove_consumer (node);
	}
      else
	_engine_mnl_remove (node);
      node->counter = Bse::TickStamp::max_stamp();
      /* nuke pending timed jobs */
      do
        tjob = node_pop_flow_job (node, Bse::TickStamp::max_stamp());
      while (tjob);
      /* nuke probe jobs */
      if (node->probe_jobs)
        {
          while (node->probe_jobs)
            {
              tjob = node->probe_jobs;
              node->probe_jobs = tjob->next;
              insert_trash_job (node,  tjob);
            }
          probe_node_list = sfi_ring_remove (probe_node_list, node);
        }
      /* nuke boundary jobs */
      if (node->boundary_jobs)
        do
          tjob = node_pop_boundary_job (node, Bse::TickStamp::max_stamp(), sfi_ring_find (boundary_node_list, node));
        while (tjob);
      _engine_node_collect_jobs (node);
      break;
    case ENGINE_JOB_SET_CONSUMER:
    case ENGINE_JOB_UNSET_CONSUMER:
      node = job->data.node;
      JOB_DEBUG ("toggle_consumer(%p)", node);
      g_return_if_fail (node->integrated == TRUE);
      was_consumer = ENGINE_NODE_IS_CONSUMER (node);
      node->is_consumer = job->job_id == ENGINE_JOB_SET_CONSUMER;
      if (was_consumer != ENGINE_NODE_IS_CONSUMER (node))
	{
	  if (ENGINE_NODE_IS_CONSUMER (node))
	    add_consumer (node);
	  else
	    remove_consumer (node);
	  master_need_reflow |= TRUE;
	}
      break;
    case ENGINE_JOB_SUSPEND:
      node = job->tick.node;
      stamp = job->tick.stamp;
      JOB_DEBUG ("suspend(%p,%llu)", node, (long long unsigned int) stamp);
      g_return_if_fail (node->integrated == TRUE);
      if (node->local_active < stamp)
	{
	  propagate_update_suspend (node);
	  node->local_active = stamp;
	  node->needs_reset = TRUE;
	  master_need_reflow |= TRUE;
	}
      break;
    case ENGINE_JOB_RESUME:
      node = job->tick.node;
      stamp = job->tick.stamp;
      JOB_DEBUG ("resume(%p,%llu)", node, (long long unsigned int) stamp);
      g_return_if_fail (node->integrated == TRUE);
      if (node->local_active > stamp)
	{
	  propagate_update_suspend (node);
	  node->local_active = stamp;
	  node->needs_reset = TRUE;
	  master_need_reflow |= TRUE;
	}
      break;
    case ENGINE_JOB_ICONNECT:
      node = job->connection.dest_node;
      src_node = job->connection.src_node;
      istream = job->connection.dest_ijstream;
      ostream = job->connection.src_ostream;
      JOB_DEBUG ("connect(%p,%u,%p,%u)", node, istream, src_node, ostream);
      g_return_if_fail (node->integrated == TRUE);
      g_return_if_fail (src_node->integrated == TRUE);
      g_return_if_fail (node->inputs[istream].src_node == NULL);
      node->inputs[istream].src_node = src_node;
      node->inputs[istream].src_stream = ostream;
      node->module.istreams[istream].connected = 0;	/* scheduler update */
      /* remove from consumer list */
      was_consumer = ENGINE_NODE_IS_CONSUMER (src_node);
      src_node->outputs[ostream].n_outputs += 1;
      src_node->module.ostreams[ostream].connected = 0; /* scheduler update */
      src_node->output_nodes = sfi_ring_append (src_node->output_nodes, node);
      NODE_FLAG_RECONNECT (node);
      NODE_FLAG_RECONNECT (src_node);
      /* update suspension state of input */
      propagate_update_suspend (src_node);
      if (was_consumer && !ENGINE_NODE_IS_CONSUMER (src_node))
	remove_consumer (src_node);
      master_need_reflow |= TRUE;
      break;
    case ENGINE_JOB_JCONNECT:
      node = job->connection.dest_node;
      src_node = job->connection.src_node;
      jstream = job->connection.dest_ijstream;
      ostream = job->connection.src_ostream;
      JOB_DEBUG ("jconnect(%p,%u,%p,%u)", node, jstream, src_node, ostream);
      g_return_if_fail (node->integrated == TRUE);
      g_return_if_fail (src_node->integrated == TRUE);
      con = node->module.jstreams[jstream].jcount++;
      node->jinputs[jstream] = g_renew (EngineJInput, node->jinputs[jstream], node->module.jstreams[jstream].jcount);
      node->module.jstreams[jstream].values = g_renew (const gfloat*, node->module.jstreams[jstream].values, node->module.jstreams[jstream].jcount + 1);
      node->module.jstreams[jstream].values[node->module.jstreams[jstream].jcount] = NULL; /* float**values 0-termination */
      node->jinputs[jstream][con].src_node = src_node;
      node->jinputs[jstream][con].src_stream = ostream;
      /* remove from consumer list */
      was_consumer = ENGINE_NODE_IS_CONSUMER (src_node);
      src_node->outputs[ostream].n_outputs += 1;
      src_node->module.ostreams[ostream].connected = 0; /* scheduler update */
      src_node->output_nodes = sfi_ring_append (src_node->output_nodes, node);
      NODE_FLAG_RECONNECT (node);
      NODE_FLAG_RECONNECT (src_node);
      /* update suspension state of input */
      propagate_update_suspend (src_node);
      if (was_consumer && !ENGINE_NODE_IS_CONSUMER (src_node))
	remove_consumer (src_node);
      master_need_reflow |= TRUE;
      break;
    case ENGINE_JOB_IDISCONNECT:
      node = job->connection.dest_node;
      JOB_DEBUG ("idisconnect(%p,%u)", node, job->connection.dest_ijstream);
      g_return_if_fail (node->integrated == TRUE);
      g_return_if_fail (node->inputs[job->connection.dest_ijstream].src_node != NULL);
      master_idisconnect_node (node, job->connection.dest_ijstream);
      master_need_reflow |= TRUE;
      break;
    case ENGINE_JOB_JDISCONNECT:
      node = job->connection.dest_node;
      jstream = job->connection.dest_ijstream;
      src_node = job->connection.src_node;
      ostream = job->connection.src_ostream;
      JOB_DEBUG ("jdisconnect(%p,%u,%p,%u)", node, jstream, src_node, ostream);
      g_return_if_fail (node->integrated == TRUE);
      g_return_if_fail (node->module.jstreams[jstream].jcount > 0);
      for (con = 0; con < node->module.jstreams[jstream].jcount; con++)
	if (node->jinputs[jstream][con].src_node == src_node &&
	    node->jinputs[jstream][con].src_stream == ostream)
	  break;
      if (con < node->module.jstreams[jstream].jcount)
	{
	  master_jdisconnect_node (node, jstream, con);
	  master_need_reflow |= TRUE;
	}
      else
	g_warning ("jdisconnect(dest:%p,%u,src:%p,%u): no such connection", node, jstream, src_node, ostream);
      break;
    case ENGINE_JOB_FORCE_RESET:
      node = job->data.node;
      JOB_DEBUG ("reset(%p)", node);
      g_return_if_fail (node->integrated == TRUE);
      node->counter = Bse::TickStamp::current();
      node->needs_reset = TRUE;
      break;
    case ENGINE_JOB_ACCESS:
      node = job->access.node;
      JOB_DEBUG ("access node(%p): %p(%p)", node, job->access.access_func, job->access.data);
      g_return_if_fail (node->integrated == TRUE);
      node->counter = Bse::TickStamp::current();
      job->access.access_func (&node->module, job->access.data);
      break;
    case ENGINE_JOB_PROBE_JOB:
      node = job->timed_job.node;
      tjob = job->timed_job.tjob;
      JOB_DEBUG ("add probe_job(%p,%p)", node, tjob);
      g_return_if_fail (node->integrated == TRUE);
      g_return_if_fail (tjob->next == NULL);
      job->timed_job.tjob = NULL;       /* ownership taken over */
      if (!node->probe_jobs)
        probe_node_list = sfi_ring_append (probe_node_list, node);
      tjob->next = node->probe_jobs;
      node->probe_jobs = tjob;
      break;
    case ENGINE_JOB_FLOW_JOB:
      node = job->timed_job.node;
      tjob = job->timed_job.tjob;
      JOB_DEBUG ("add flow_job(%p,%p)", node, tjob);
      g_return_if_fail (node->integrated == TRUE);
      job->timed_job.tjob = NULL;	/* ownership taken over */
      node->flow_jobs = insert_timed_job (node->flow_jobs, tjob);
      _engine_mnl_node_changed (node);
      break;
    case ENGINE_JOB_BOUNDARY_JOB:
      node = job->timed_job.node;
      tjob = job->timed_job.tjob;
      JOB_DEBUG ("add boundary_job(%p,%p)", node, tjob);
      g_return_if_fail (node->integrated == TRUE);
      job->timed_job.tjob = NULL;	/* ownership taken over */
      master_new_boundary_jobs = TRUE;
      if (!node->boundary_jobs)
        boundary_node_list = sfi_ring_append (boundary_node_list, node);
      node->boundary_jobs = insert_timed_job (node->boundary_jobs, tjob);
      break;
    case ENGINE_JOB_MESSAGE:
      if (job->data.message)
        {
          JOB_DEBUG ("debug");
          g_printerr ("BSE-ENGINE: %s\n", job->data.message);
        }
      else
        JOB_DEBUG ("nop");
      break;
    case ENGINE_JOB_ADD_POLL:
      JOB_DEBUG ("add poll %p(%p,%u)", job->poll.poll_func, job->poll.data, job->poll.n_fds);
      if (job->poll.n_fds + master_n_pollfds > BSE_ENGINE_MAX_POLLFDS)
	g_error ("adding poll job exceeds maximum number of poll-fds (%u > %u)",
		 job->poll.n_fds + master_n_pollfds, BSE_ENGINE_MAX_POLLFDS);
      poll = sfi_new_struct0 (Poll, 1);
      poll->poll_func = job->poll.poll_func;
      poll->data = job->poll.data;
      poll->free_func = job->poll.free_func;
      job->poll.free_func = NULL;		/* don't free data this round */
      poll->n_fds = job->poll.n_fds;
      poll->fds = poll->n_fds ? master_pollfds + master_n_pollfds : master_pollfds;
      master_n_pollfds += poll->n_fds;
      if (poll->n_fds)
	master_pollfds_changed = TRUE;
      memcpy (poll->fds, job->poll.fds, sizeof (poll->fds[0]) * poll->n_fds);
      poll->next = master_poll_list;
      master_poll_list = poll;
      break;
    case ENGINE_JOB_REMOVE_POLL:
      JOB_DEBUG ("remove poll %p(%p)", job->poll.poll_func, job->poll.data);
      for (poll = master_poll_list, poll_last = NULL; poll; poll_last = poll, poll = poll_last->next)
	if (poll->poll_func == job->poll.poll_func && poll->data == job->poll.data)
	  {
	    if (poll_last)
	      poll_last->next = poll->next;
	    else
	      master_poll_list = poll->next;
	    break;
	  }
      if (poll)
	{
	  job->poll.free_func = poll->free_func;	/* free data with job */
	  poll_last = poll;
	  if (poll_last->n_fds)
	    {
	      for (poll = master_poll_list; poll; poll = poll->next)
		if (poll->fds > poll_last->fds)
		  poll->fds -= poll_last->n_fds;
	      g_memmove (poll_last->fds, poll_last->fds + poll_last->n_fds,
			 ((guint8*) (master_pollfds + master_n_pollfds)) -
			 ((guint8*) (poll_last->fds + poll_last->n_fds)));
	      master_n_pollfds -= poll_last->n_fds;
	      master_pollfds_changed = TRUE;
	    }
	  sfi_delete_struct (Poll, poll_last);
	}
      else
	g_warning (G_STRLOC ": failed to remove unknown poll function %p(%p)",
		   job->poll.poll_func, job->poll.data);
      break;
    case ENGINE_JOB_ADD_TIMER:
      JOB_DEBUG ("add timer %p(%p)", job->timer.timer_func, job->timer.data);
      timer = sfi_new_struct0 (Timer, 1);
      timer->timer_func = job->timer.timer_func;
      timer->data = job->timer.data;
      timer->free_func = job->timer.free_func;
      job->timer.free_func = NULL;		/* don't free data this round */
      timer->next = master_timer_list;
      master_timer_list = timer;
      break;
    default:
      g_assert_not_reached ();
    }
  JOB_DEBUG ("done");
}
static void
master_poll_check (glong   *timeout_p,
		   gboolean check_with_revents)
{
  gboolean need_processing = FALSE;
  Poll *poll;
  if (master_need_process || *timeout_p == 0)
    {
      master_need_process = TRUE;
      return;
    }
  for (poll = master_poll_list; poll; poll = poll->next)
    {
      glong timeout = -1;
      if (poll->poll_func (poll->data, bse_engine_block_size (), &timeout,
			   poll->n_fds, poll->n_fds ? poll->fds : NULL, check_with_revents)
	  || timeout == 0)
	{
	  need_processing |= TRUE;
	  *timeout_p = 0;
	  break;
	}
      else if (timeout > 0)
	*timeout_p = *timeout_p < 0 ? timeout : MIN (*timeout_p, timeout);
    }
  master_need_process = need_processing;
}
static void
master_tick_stamp_inc (void)
{
  Timer *timer, *last = NULL;
  guint64 new_stamp;
  Bse::TickStamp::_increment();
  new_stamp = Bse::TickStamp::current();
  timer = master_timer_list;
  while (timer)
    {
      Timer *next = timer->next;
      if (!timer->timer_func (timer->data, new_stamp))
	{
	  BseTrans *trans = bse_trans_open ();
	  if (last)
	    last->next = next;
	  else
	    master_timer_list = next;
	  /* free timer data in user thread */
	  bse_trans_add (trans, bse_job_add_timer (timer->timer_func, timer->data, timer->free_func));
	  bse_trans_dismiss (trans);
	  sfi_delete_struct (Timer, timer);
	}
      else
	last = timer;
      timer = next;
    }
}
typedef enum {
  PROBE_UNSCHEDULED,
  PROBE_SCHEDULED,
  PROBE_VIRTUAL /* scheduled */
} ProbeType;
static inline void
master_take_probes (EngineNode   *node,
                    const guint64 current_stamp,
                    guint         n_values,
                    ProbeType     ptype)
{
  if (G_LIKELY (!node->probe_jobs))
    return;
  /* peek probe job */
  EngineTimedJob *tjob = node->probe_jobs;
  /* probe the output stream data */
  tjob->probe.tick_stamp = current_stamp;
  if (ptype == PROBE_SCHEDULED)
    {
      uint i;
      g_assert (tjob->probe.n_ostreams == ENGINE_NODE_N_OSTREAMS (node));
      /* swap output buffers with probe buffers */
      BseOStream *ostreams = node->module.ostreams;
      node->module.ostreams = tjob->probe.ostreams;
      tjob->probe.ostreams = ostreams;
      for (i = 0; i < ENGINE_NODE_N_OSTREAMS (node); i++)
        {
          /* restore real ostream buffer pointers */
          ostreams[i].values = node->outputs[i].buffer;
          /* store real ostream buffer pointers */
          node->outputs[i].buffer = node->module.ostreams[i].values;
          /* preserve connection flags */
          node->module.ostreams[i].connected = ostreams[i].connected;
        }
    }
  else if (ptype == PROBE_VIRTUAL)
    {
      uint i;
      /* copy output buffers to probe buffers */
      for (i = 0; i < ENGINE_NODE_N_OSTREAMS (node); i++)
        {
          EngineInput *input = node->inputs + i;
          if (input->real_node && input->real_node->module.ostreams[input->real_stream].connected)
            {
              tjob->probe.ostreams[i].connected = true;
              bse_block_copy_float (n_values, tjob->probe.ostreams[i].values, input->real_node->outputs[input->real_stream].buffer);
            }
        }
    }
  /* pop probe job */
  node->probe_jobs = tjob->next;
  if (!node->probe_jobs)
    probe_node_list = sfi_ring_remove (probe_node_list, node); //FIXME: protect by lock
  insert_trash_job (node, tjob);
  _engine_node_collect_jobs (node);
}
static inline guint64
master_update_node_state (EngineNode *node,
                          guint64     max_tick)
{
  /* the node is not necessarily scheduled */
  EngineTimedJob *tjob;
  /* if a reset is pending, it needs to be handled *before*
   * flow jobs change state.
   */
  if (UNLIKELY (node->needs_reset && !ENGINE_NODE_IS_SUSPENDED (node, node->counter)))
    {
      /* for suspended nodes, reset() occours later */
      if (node->module.klass->reset)
        node->module.klass->reset (&node->module);
      node->needs_reset = FALSE;
    }
  tjob = node_pop_flow_job (node, max_tick);
  if (UNLIKELY (tjob != NULL))
    do
      {
        TJOB_DEBUG ("flow-access for (%p:s=%u) at:%lld current:%lld\n",
                    node, node->sched_tag, (long long unsigned int) tjob->tick_stamp, (long long unsigned int) node->counter);
        tjob->access.access_func (&node->module, tjob->access.data);
        tjob = node_pop_flow_job (node, max_tick);
      }
    while (tjob);
  return node_peek_flow_job_stamp (node);
}
static void
master_process_locked_node (EngineNode *node,
			    guint       n_values)
{
  const guint64 current_stamp = Bse::TickStamp::current();
  guint64 next_counter, new_counter, final_counter = current_stamp + n_values;
  guint i, j, diff;
  bool needs_probe_reset = node->probe_jobs != NULL;
  g_return_if_fail (node->integrated && node->sched_tag);
  while (node->counter < final_counter)
    {
      /* call reset() and exec flow jobs */
      /* FIXME: decide whether flow jobs can rely on valid ostream[].values pointers or not */
      next_counter = master_update_node_state (node, node->counter);
      /* figure n_values to process */
      new_counter = MIN (next_counter, final_counter);
      if (node->next_active > node->counter)
        new_counter = MIN (node->next_active, new_counter);
      diff = node->counter - current_stamp;
      /* ensure all istream inputs have n_values available */
      for (i = 0; i < ENGINE_NODE_N_ISTREAMS (node); i++)
	{
	  EngineNode *inode = node->inputs[i].real_node;
	  if (inode)
	    {
	      ENGINE_NODE_LOCK (inode);
	      if (inode->counter < final_counter)
		master_process_locked_node (inode, final_counter - node->counter);
	      node->module.istreams[i].values = inode->outputs[node->inputs[i].real_stream].buffer;
	      node->module.istreams[i].values += diff;
	      ENGINE_NODE_UNLOCK (inode);
	    }
	  else
	    node->module.istreams[i].values = bse_engine_const_zeros (BSE_STREAM_MAX_VALUES);
	}
      /* ensure all jstream inputs have n_values available */
      for (j = 0; j < ENGINE_NODE_N_JSTREAMS (node); j++)
	for (i = 0; i < node->module.jstreams[j].n_connections; i++) /* assumes scheduled node */
	  {
	    EngineNode *inode = node->jinputs[j][i].real_node;
	    ENGINE_NODE_LOCK (inode);
	    if (inode->counter < final_counter)
	      master_process_locked_node (inode, final_counter - node->counter);
	    node->module.jstreams[j].values[i] = inode->outputs[node->jinputs[j][i].real_stream].buffer;
	    node->module.jstreams[j].values[i] += diff;
	    ENGINE_NODE_UNLOCK (inode);
	  }
      /* update obuffer pointer (FIXME: need this before flow job callbacks?) */
      for (i = 0; i < ENGINE_NODE_N_OSTREAMS (node); i++)
        node->module.ostreams[i].values = node->outputs[i].buffer + diff;
      if (diff && needs_probe_reset)
        for (i = 0; i < ENGINE_NODE_N_OSTREAMS (node); i++)
          bse_block_fill_float (diff, node->outputs[i].buffer, 0.0);
      needs_probe_reset = false;
      /* process() node */
      if (UNLIKELY (ENGINE_NODE_IS_SUSPENDED (node, node->counter)))
	{
	  /* suspended node processing behaviour */
	  for (i = 0; i < ENGINE_NODE_N_OSTREAMS (node); i++)
	    if (node->module.ostreams[i].connected)
	      node->module.ostreams[i].values = bse_engine_const_zeros (BSE_STREAM_MAX_VALUES);
          node->needs_reset = TRUE;
	}
      else
        node->module.klass->process (&node->module, new_counter - node->counter);
      /* catch obuffer pointer changes */
      for (i = 0; i < ENGINE_NODE_N_OSTREAMS (node); i++)
	{
	  /* FIXME: this takes the worst possible performance hit to support obuffer pointer virtualization */
	  if (node->module.ostreams[i].connected &&
              node->module.ostreams[i].values != node->outputs[i].buffer + diff)
            bse_block_copy_float (new_counter - node->counter, node->outputs[i].buffer + diff, node->module.ostreams[i].values);
	}
      /* update node counter */
      node->counter = new_counter;
    }
}
static gboolean gsl_profile_modules = 0;	/* set to 1 in gdb to get profile output */
static void
master_process_flow (void)
{
  const guint64 current_stamp = Bse::TickStamp::current();
  guint n_values = bse_engine_block_size();
  guint64 final_counter = current_stamp + n_values;
  guint64 profile_maxtime = 0;
  gboolean profile_modules = gsl_profile_modules;
  EngineNode *profile_node = NULL;
  g_return_if_fail (master_need_process == TRUE);
  g_assert (bse_fpu_okround () == TRUE);
  if (master_schedule)
    {
      _engine_schedule_restart (master_schedule);
      _engine_set_schedule (master_schedule);
      EngineNode *node = _engine_pop_unprocessed_node ();
      while (node)
	{
	  ToyprofStamp profile_stamp1, profile_stamp2;
	  if (UNLIKELY (profile_modules))
	    toyprof_stamp (profile_stamp1);
	  master_process_locked_node (node, n_values);
	  if (UNLIKELY (profile_modules))
	    {
	      toyprof_stamp (profile_stamp2);
	      guint64 duration = toyprof_elapsed (profile_stamp1, profile_stamp2);
	      if (duration > profile_maxtime)
		{
		  profile_maxtime = duration;
		  profile_node = node;
		}
	    }
	  _engine_push_processed_node (node);
	  node = _engine_pop_unprocessed_node ();
	}
      /* walk unscheduled nodes with flow jobs */
      node = _engine_mnl_head ();
      while (node && BSE_ENGINE_MNL_UNSCHEDULED_TJOB_NODE (node))
	{
	  EngineNode *tmp = node->mnl_next;
          node->counter = final_counter;
          master_update_node_state (node, node->counter - 1);
          // master_take_probes (node, current_stamp, n_values, PROBE_UNSCHEDULED);
	  _engine_mnl_node_changed (node);      /* collects trash jobs and reorders node */
	  node = tmp;
	}
      /* nothing new to process, wait for slaves */
      _engine_wait_on_unprocessed ();
      /* take remaining probes */
      SfiRing *ring = probe_node_list;
      while (ring)
        {
          node = (EngineNode*) ring->data; /* current ring may be removed during master_take_probes() */
          ring = sfi_ring_walk (ring, probe_node_list);
          if (!ENGINE_NODE_IS_SCHEDULED (node))
            master_take_probes (node, current_stamp, n_values, PROBE_UNSCHEDULED);
          else if (ENGINE_NODE_IS_VIRTUAL (node)) /* scheduled && virtual */
            master_take_probes (node, current_stamp, n_values, PROBE_VIRTUAL);
          else
            master_take_probes (node, current_stamp, n_values, PROBE_SCHEDULED);
        }
      if (UNLIKELY (profile_modules))
	{
	  if (profile_node)
	    {
	      if (profile_maxtime > guint64 (profile_modules))
		g_print ("Excess Node: %p  Duration: %llu usecs     ((void(*)())%p)         \n",
			 profile_node, (long long unsigned int) profile_maxtime, profile_node->module.klass->process);
	      else
		g_print ("Slowest Node: %p  Duration: %llu usecs     ((void(*)())%p)         \r",
			 profile_node, (long long unsigned int) profile_maxtime, profile_node->module.klass->process);
	    }
	}
      _engine_unset_schedule (master_schedule);
      master_tick_stamp_inc ();
      _engine_recycle_const_values (FALSE);
    }
  master_need_process = FALSE;
}
static void
master_reschedule_flow (void)
{
  EngineNode *node;
  g_return_if_fail (master_need_reflow == TRUE);
  if (!master_schedule)
    master_schedule = _engine_schedule_new ();
  else
    {
      _engine_schedule_unsecure (master_schedule);
      _engine_schedule_clear (master_schedule);
    }
  for (node = master_consumer_list; node; node = node->toplevel_next)
    _engine_schedule_consumer_node (master_schedule, node);
  _engine_schedule_secure (master_schedule);
  master_need_reflow = FALSE;
}
static void
master_schedule_discard (void)
{
  g_return_if_fail (master_need_reflow == TRUE);
  if (master_schedule)
    {
      _engine_schedule_unsecure (master_schedule);
      _engine_schedule_destroy (master_schedule);
      master_schedule = NULL;
    }
}
/* --- MasterThread main loop --- */
gboolean
_engine_master_prepare (BseEngineLoop *loop)
{
  gboolean need_dispatch;
  guint i;
  g_return_val_if_fail (loop != NULL, FALSE);
  /* setup and clear pollfds here already, so master_poll_check() gets no junk (and IRIX can't handle non-0 revents) */
  loop->fds_changed = master_pollfds_changed;
  master_pollfds_changed = FALSE;
  loop->n_fds = master_n_pollfds;
  loop->fds = master_pollfds;
  for (i = 0; i < loop->n_fds; i++)
    loop->fds[i].revents = 0;
  loop->revents_filled = FALSE;
  loop->timeout = -1;
  /* cached checks first */
  need_dispatch = master_need_reflow || master_need_process;
  /* lengthy query */
  if (!need_dispatch)
    need_dispatch = _engine_job_pending ();
  /* invoke custom poll checks */
  if (!need_dispatch)
    {
      master_poll_check (&loop->timeout, FALSE);
      need_dispatch = master_need_process;
    }
  if (need_dispatch)
    loop->timeout = 0;
  return need_dispatch;
}
gboolean
_engine_master_check (const BseEngineLoop *loop)
{
  gboolean need_dispatch;
  g_return_val_if_fail (loop != NULL, FALSE);
  g_return_val_if_fail (loop->n_fds == master_n_pollfds, FALSE);
  g_return_val_if_fail (loop->fds == master_pollfds, FALSE);
  if (loop->n_fds)
    g_return_val_if_fail (loop->revents_filled == TRUE, FALSE);
  /* cached checks first */
  need_dispatch = master_need_reflow || master_need_process;
  /* lengthy query */
  if (!need_dispatch)
    need_dispatch = _engine_job_pending ();
  /* invoke custom poll checks */
  if (!need_dispatch)
    {
      glong dummy = -1;
      master_poll_check (&dummy, TRUE);
      need_dispatch = master_need_process;
    }
  return need_dispatch;
}
void
_engine_master_dispatch_jobs (void)
{
  const guint64 current_stamp = Bse::TickStamp::current();
  guint64 last_block_tick = current_stamp + bse_engine_block_size() - 1;
  BseJob *job = _engine_pop_job (boundary_node_list == NULL);
  /* here, we have to process _all_ pending jobs in a row. a popped job
   * stays valid until the next call to _engine_pop_job().
   */
  while (job)
    {
      master_process_job (job);
      job = _engine_pop_job (boundary_node_list == NULL);
    }
  /* process boundary jobs and possibly newly queued jobs after that. */
  if (UNLIKELY (boundary_node_list != NULL))
    do
      {
        SfiRing *ring = boundary_node_list;
        master_new_boundary_jobs = FALSE;       /* to catch new boundary jobs */
        while (ring)
          {
            SfiRing *current = ring;
            EngineNode *node = (EngineNode*) ring->data;
            ring = sfi_ring_walk (ring, boundary_node_list);
            EngineTimedJob *tjob = node_pop_boundary_job (node, last_block_tick, current);
            if (tjob)
              node->counter = current_stamp;
            while (tjob)
              {
                TJOB_DEBUG ("boundary-access for (%p:s=%u) at:%lld current:%lld\n",
                            node, node->sched_tag, (long long int) tjob->tick_stamp, (long long int) node->counter);
                tjob->access.access_func (&node->module, tjob->access.data);
                tjob = node_pop_boundary_job (node, last_block_tick, current);
              }
          }
        /* process newly queued jobs if any */
        job = _engine_pop_job (!master_new_boundary_jobs);
        while (job)
          {
            master_process_job (job);
            job = _engine_pop_job (!master_new_boundary_jobs);
          }
        /* need to repeat if master_process_job() just queued a new boundary job */
      }
    while (master_new_boundary_jobs);   /* new boundary jobs arrived */
}
void
_engine_master_dispatch (void)
{
  /* processing has prime priority, but we can't process the
   * network, until all jobs have been handled and if necessary
   * rescheduled the network.
   * that's why we have to handle everything at once and can't
   * preliminarily return after just handling jobs or rescheduling.
   */
  _engine_master_dispatch_jobs ();
  if (master_need_reflow)
    master_reschedule_flow ();
  if (master_need_process)
    master_process_flow ();
}

namespace Bse {

MasterThread::MasterThread (const std::function<void()> &caller_wakeup) :
  caller_wakeup_ (caller_wakeup)
{
  assert (caller_wakeup_ != NULL);
  if (event_fd_.open() != 0)
    g_error ("failed to create engine wake-up pipe: %s", strerror (errno));
  thread_ = std::thread (&MasterThread::master_thread, this); // FIXME: join on exit
}

void
MasterThread::master_thread()
{
  Bse::TaskRegistry::add ("DSP #1", Rapicorn::ThisThread::process_pid(), Rapicorn::ThisThread::thread_pid());

  /* assert pollfd equality, since we're simply casting structures */
  RAPICORN_STATIC_ASSERT (sizeof (struct pollfd) == sizeof (GPollFD));
  RAPICORN_STATIC_ASSERT (G_STRUCT_OFFSET (GPollFD, fd) == G_STRUCT_OFFSET (struct pollfd, fd));
  RAPICORN_STATIC_ASSERT (sizeof (((GPollFD*) 0)->fd) == sizeof (((struct pollfd*) 0)->fd));
  RAPICORN_STATIC_ASSERT (G_STRUCT_OFFSET (GPollFD, events) == G_STRUCT_OFFSET (struct pollfd, events));
  RAPICORN_STATIC_ASSERT (sizeof (((GPollFD*) 0)->events) == sizeof (((struct pollfd*) 0)->events));
  RAPICORN_STATIC_ASSERT (G_STRUCT_OFFSET (GPollFD, revents) == G_STRUCT_OFFSET (struct pollfd, revents));
  RAPICORN_STATIC_ASSERT (sizeof (((GPollFD*) 0)->revents) == sizeof (((struct pollfd*) 0)->revents));

  /* add the thread wakeup pipe to master pollfds,
   * so we get woken  up in time.
   */
  master_pollfds[0].fd = event_fd_.inputfd();
  master_pollfds[0].events = G_IO_IN;
  master_n_pollfds = 1;
  master_pollfds_changed = TRUE;
  toyprof_stampinit ();
  while (1)
    {
      BseEngineLoop loop;
      bool need_dispatch;
      need_dispatch = _engine_master_prepare (&loop);
      master_pollfds[0].revents = 0;
      if (!need_dispatch)
	{
	  int err = poll ((struct pollfd*) loop.fds, loop.n_fds, loop.timeout);
	  if (err >= 0)
	    loop.revents_filled = TRUE;
	  else if (errno != EINTR)
	    g_printerr ("%s: poll() error: %s\n", G_STRFUNC, g_strerror (errno));
	  if (loop.revents_filled)
	    need_dispatch = _engine_master_check (&loop);
	}
      if (need_dispatch)
	_engine_master_dispatch ();
      if (master_pollfds[0].revents)    // need to clear wakeup pipe
        event_fd_.flush();
      // wakeup user thread if necessary
      if (bse_engine_has_garbage ())
	caller_wakeup_();
    }
  Bse::TaskRegistry::remove (Rapicorn::ThisThread::thread_pid());
}

} // Bse
