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
#include "gslopmaster.h"

#include "gslcommon.h"
#include "gslopnode.h"
#include "gsloputil.h"
#include "gslopschedule.h"
#include <string.h>
#include <sys/poll.h>
#include <errno.h>



/* --- typedefs & structures --- */
typedef struct _Poll Poll;
struct _Poll
{
  Poll	     *next;
  GslPollFunc poll_func;
  gpointer    data;
  guint       n_fds;
  GslPollFD  *fds;
  GslFreeFunc free_func;
};


/* --- prototypes --- */
static void	master_schedule_discard	(void);


/* --- variables --- */
static gboolean	    master_need_user_wakeups = FALSE;
static gboolean	    master_need_reflow = FALSE;
static gboolean	    master_need_process = FALSE;
static OpNode	   *master_node_list = NULL;
static OpNode	   *master_consumer_list = NULL;
const gfloat        gsl_engine_master_zero_block[GSL_STREAM_MAX_VALUES] = { 0, };
static Poll	   *master_poll_list = NULL;
static guint        master_n_pollfds = 0;
static guint        master_pollfds_changed = FALSE;
static GslPollFD    master_pollfds[GSL_ENGINE_MAX_POLLFDS];


/* --- functions --- */
static void
add_consumer (OpNode *node)
{
  g_return_if_fail (OP_NODE_IS_CONSUMER (node) && node->mcl_next == NULL);

  node->mcl_next = master_consumer_list;
  master_consumer_list = node;
}

static void
remove_consumer (OpNode *node)
{
  OpNode *tmp, *last = NULL;

  g_return_if_fail (OP_NODE_IS_CONSUMER (node));
  
  for (tmp = master_consumer_list; tmp; last = tmp, tmp = last->mcl_next)
    if (tmp == node)
      break;
  g_return_if_fail (tmp != NULL);
  if (last)
    last->mcl_next = node->mcl_next;
  else
    master_consumer_list = node->mcl_next;
  node->mcl_next = NULL;
}

static void
op_node_disconnect (OpNode *node,
		    guint   istream)
{
  OpNode *src_node = node->inputs[istream].src_node;
  guint ostream = node->inputs[istream].src_stream;
  gboolean was_consumer;

  g_assert (ostream < OP_NODE_N_OSTREAMS (src_node) &&
	    src_node->outputs[ostream].n_outputs > 0);	/* these checks better pass */

  node->inputs[istream].src_node = NULL;
  node->inputs[istream].src_stream = ~0;
  node->module.istreams[istream].connected = FALSE;
  was_consumer = OP_NODE_IS_CONSUMER (src_node);
  src_node->outputs[ostream].n_outputs -= 1;
  src_node->module.ostreams[ostream].connected = src_node->outputs[ostream].n_outputs > 0;
  src_node->onodes = gsl_ring_remove (src_node->onodes, node);
  /* add to consumer list */
  if (!was_consumer && OP_NODE_IS_CONSUMER (src_node))
    add_consumer (src_node);
}

static void
op_node_disconnect_outputs (OpNode *src_node,
			    OpNode *dest_node)
{
  guint i;

  for (i = 0; i < OP_NODE_N_ISTREAMS (dest_node); i++)
    if (dest_node->inputs[i].src_node == src_node)
      op_node_disconnect (dest_node, i);
}

static void
master_process_job (GslJob *job)
{
  switch (job->job_id)
    {
      OpNode *node, *src_node;
      Poll *poll, *poll_last;
      guint istream, ostream;
    case OP_JOB_INTEGRATE:
      node = job->data.node;
      OP_DEBUG (JOBS, "integrate(%p)", node);
      g_return_if_fail (node->mnl_contained == FALSE);
      g_return_if_fail (node->mnl_prev == NULL && node->mnl_next == NULL);
      node->mnl_contained = TRUE;
      if (master_node_list)
	master_node_list->mnl_prev = node;
      node->mnl_next = master_node_list;
      master_node_list = node;
      if (OP_NODE_IS_CONSUMER (node))
	add_consumer (node);
      node->counter = 0;
      master_need_reflow |= TRUE;
      break;
    case OP_JOB_DISCARD:
      node = job->data.node;
      OP_DEBUG (JOBS, "discard(%p)", node);
      g_return_if_fail (node->mnl_contained == TRUE);
      /* disconnect inputs */
      for (istream = 0; istream < OP_NODE_N_ISTREAMS (node); istream++)
	if (node->inputs[istream].src_node)
	  op_node_disconnect (node, istream);
      /* disconnect outputs */
      while (node->onodes)
	op_node_disconnect_outputs (node, node->onodes->data);
      /* remove from consumer list */
      if (OP_NODE_IS_CONSUMER (node))
	remove_consumer (node);
      /* remove from master node list */
      if (node->mnl_prev)
	node->mnl_prev->mnl_next = node->mnl_next;
      else
	master_node_list = node->mnl_next;
      if (node->mnl_next)
	node->mnl_next->mnl_prev = node->mnl_prev;
      node->mnl_prev = NULL;
      node->mnl_next = NULL;
      node->mnl_contained = FALSE;
      node->counter = 0;
      master_need_reflow |= TRUE;
      master_schedule_discard ();	/* discard schedule so node may be freed */
      break;
    case OP_JOB_CONNECT:
      node = job->data.connection.dest_node;
      src_node = job->data.connection.src_node;
      istream = job->data.connection.dest_istream;
      ostream = job->data.connection.src_ostream;
      OP_DEBUG (JOBS, "connect(%p,%u,%p,%u)", node, istream, src_node, ostream);
      g_return_if_fail (node->mnl_contained == TRUE);
      g_return_if_fail (src_node->mnl_contained == TRUE);
      g_return_if_fail (node->inputs[istream].src_node == NULL);
      node->inputs[istream].src_node = src_node;
      node->inputs[istream].src_stream = ostream;
      node->module.istreams[istream].connected = TRUE;
      /* remove from consumer list */
      if (OP_NODE_IS_CONSUMER (src_node))
	remove_consumer (src_node);
      src_node->outputs[ostream].n_outputs += 1;
      src_node->module.ostreams[ostream].connected = TRUE;
      src_node->onodes = gsl_ring_append (src_node->onodes, node);
      src_node->counter = 0;
      master_need_reflow |= TRUE;
      break;
    case OP_JOB_DISCONNECT:
      node = job->data.connection.dest_node;
      OP_DEBUG (JOBS, "disconnect(%p,%u)", node, job->data.connection.dest_istream);
      g_return_if_fail (node->mnl_contained == TRUE);
      g_return_if_fail (node->inputs[job->data.connection.dest_istream].src_node != NULL);
      op_node_disconnect (node, job->data.connection.dest_istream);
      master_need_reflow |= TRUE;
      break;
    case OP_JOB_DEBUG:
      OP_DEBUG (JOBS, "debug");
      g_print ("JOB-DEBUG: %s\n", job->data.debug); /* FIXME: stderr */
      break;
    case OP_JOB_ADD_POLL:
      OP_DEBUG (JOBS, "add poll %p(%p,%u)", job->data.poll.poll_func, job->data.poll.data, job->data.poll.n_fds);
      if (job->data.poll.n_fds + master_n_pollfds > GSL_ENGINE_MAX_POLLFDS)
	g_error ("adding poll job exceeds maximum number of poll-fds (%u > %u)",
		 job->data.poll.n_fds + master_n_pollfds, GSL_ENGINE_MAX_POLLFDS);
      poll = gsl_new_struct0 (Poll, 1);
      poll->poll_func = job->data.poll.poll_func;
      poll->data = job->data.poll.data;
      poll->free_func = job->data.poll.free_func;
      job->data.poll.free_func = NULL;		/* don't free data this round */
      poll->n_fds = job->data.poll.n_fds;
      poll->fds = poll->n_fds ? master_pollfds + master_n_pollfds : master_pollfds;
      master_n_pollfds += poll->n_fds;
      if (poll->n_fds)
	master_pollfds_changed = TRUE;
      memcpy (poll->fds, job->data.poll.fds, sizeof (poll->fds[0]) * poll->n_fds);
      poll->next = master_poll_list;
      master_poll_list = poll;
      break;
    case OP_JOB_REMOVE_POLL:
      OP_DEBUG (JOBS, "remove poll %p(%p)", job->data.poll.poll_func, job->data.poll.data);
      for (poll = master_poll_list, poll_last = NULL; poll; poll_last = poll, poll = poll_last->next)
	if (poll->poll_func == job->data.poll.poll_func && poll->data == job->data.poll.data)
	  {
	    if (poll_last)
	      poll_last->next = poll->next;
	    else
	      master_poll_list = poll->next;
	    break;
	  }
      if (poll)
	{
	  job->data.poll.free_func = poll->free_func;	/* free data with job */
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
	  gsl_delete_struct (Poll, 1, poll_last);
	}
      else
	g_warning (G_STRLOC ": failed to remove unknown poll function %p(%p)",
		   job->data.poll.poll_func, job->data.poll.data);
      break;
    case OP_JOB_ACCESS:
      node = job->data.access.node;
      OP_DEBUG (JOBS, "access (%p)", node);
      g_return_if_fail (node->mnl_contained == TRUE);
      job->data.access.access_func (&node->module, job->data.access.data);
      break;
    default:
      g_assert_not_reached ();
    }
  OP_DEBUG (JOBS, "done");
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

      if (poll->poll_func (poll->data, gsl_engine_block_size (), &timeout,
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
master_process_locked_node (OpNode *node,
			    guint   n_values)
{
  guint i;
  guint64 new_counter = gsl_engine_last_counter () + n_values;

  for (i = 0; i < OP_NODE_N_ISTREAMS (node); i++)
    {
      OpNode *inode = node->inputs[i].src_node;

      if (inode)
	{
	  OP_NODE_LOCK (inode);
	  if (inode->counter < new_counter)
	    master_process_locked_node (inode, new_counter - node->counter);
	  node->module.istreams[i].values = inode->module.ostreams[node->inputs[i].src_stream].values;
	  OP_NODE_UNLOCK (inode);
	}
      else
	node->module.istreams[i].values = gsl_engine_master_zero_block;
    }
  for (i = 0; i < OP_NODE_N_OSTREAMS (node); i++)
    {
      node->module.ostreams[i].values = node->outputs[i].buffer;
      if (node->module.ostreams[i].zero_initialize)
	memset (node->module.ostreams[i].values, 0, GSL_STREAM_MAX_VALUES * sizeof (gfloat));
    }
  node->module.klass->process (&node->module, n_values);
  node->counter += n_values;
}

static OpSchedule *master_schedule = NULL;

static void
master_process_flow (void)
{
  g_return_if_fail (master_need_process == TRUE);

  OP_DEBUG (MASTER, "process_flow");
  if (master_schedule)
    {
      OpNode *node;

      _op_schedule_restart (master_schedule);
      _gsl_com_set_schedule (master_schedule);
      if (master_need_user_wakeups)
	_gsl_com_fire_user_wakeup ();
      
      node = _gsl_com_pop_unprocessed_node ();
      while (node)
	{
	  master_process_locked_node (node, gsl_engine_block_size ());
	  _gsl_com_push_processed_node (node);
	  node = _gsl_com_pop_unprocessed_node ();
	}
      /* nothing new to process, wait on slaves */
      _gsl_com_wait_on_unprocessed ();

      _gsl_com_unset_schedule (master_schedule);
      _op_engine_inc_counter (gsl_engine_block_size ());
      _gsl_recycle_const_values ();
    }
  master_need_process = FALSE;
}

static void
master_reschedule_flow (void)
{
  OpNode *node;

  g_return_if_fail (master_need_reflow == TRUE);

  OP_DEBUG (MASTER, "flow_reschedule");
  if (!master_schedule)
    master_schedule = _op_schedule_new ();
  else
    {
      _op_schedule_unsecure (master_schedule);
      _op_schedule_clear (master_schedule);
    }
  for (node = master_consumer_list; node; node = node->mcl_next)
    _op_schedule_consumer_node (master_schedule, node);
  _op_schedule_secure (master_schedule);
  master_need_reflow = FALSE;
}

static void
master_schedule_discard (void)
{
  g_return_if_fail (master_need_reflow == TRUE);

  if (master_schedule)
    {
      _op_schedule_unsecure (master_schedule);
      _op_schedule_destroy (master_schedule);
      master_schedule = NULL;
    }
}


/* --- MasterThread main loop --- */
gboolean
_gsl_master_prepare (GslEngineLoop *loop)
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
    need_dispatch = op_com_job_pending ();
  /* invoke custom poll checks */
  if (!need_dispatch)
    {
      master_poll_check (&loop->timeout, FALSE);
      need_dispatch = master_need_process;
    }
  if (need_dispatch)
    loop->timeout = 0;

  OP_DEBUG (MASTER, "PREPARE: need_dispatch=%u timeout=%6ld n_fds=%u",
	    need_dispatch,
	    loop->timeout, loop->n_fds);

  return need_dispatch;
}

gboolean
_gsl_master_check (const GslEngineLoop *loop)
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
    need_dispatch = op_com_job_pending ();
  /* invoke custom poll checks */
  if (!need_dispatch)
    {
      glong dummy = -1;

      master_poll_check (&dummy, TRUE);
      need_dispatch = master_need_process;
    }

  OP_DEBUG (MASTER, "CHECK: need_dispatch=%u", need_dispatch);

  return need_dispatch;
}

void
_gsl_master_dispatch_jobs (void)
{
  GslJob *job;

  _gsl_com_discard_master_wakeups ();

  job = gsl_com_pop_job ();
  while (job)
    {
      master_process_job (job);
      job = gsl_com_pop_job ();	/* have to process _all_ jobs */
    }
}

void
_gsl_master_dispatch (void)
{
  /* processing has prime priority, but we can't process the
   * network, until all jobs have been handled and if necessary
   * rescheduled the network.
   * that's why we have to handle everything at once and can't
   * preliminarily return after just handling jobs or rescheduling.
   */
  _gsl_master_dispatch_jobs ();
  if (master_need_reflow)
    master_reschedule_flow ();
  if (master_need_process)
    master_process_flow ();
}

void
_gsl_master_thread (gpointer data)
{
  gboolean run = TRUE;
  
  g_assert (sizeof (struct pollfd) == sizeof (GslPollFD) &&
	    G_STRUCT_OFFSET (GslPollFD, fd) == G_STRUCT_OFFSET (struct pollfd, fd) &&
	    G_STRUCT_OFFSET (GslPollFD, events) == G_STRUCT_OFFSET (struct pollfd, events) &&
	    G_STRUCT_OFFSET (GslPollFD, revents) == G_STRUCT_OFFSET (struct pollfd, revents));
  
  master_need_user_wakeups = TRUE;
  while (run)
    {
      GslEngineLoop loop;
      gboolean need_dispatch;

      need_dispatch = _gsl_master_prepare (&loop);

      if (!need_dispatch)
	{
	  gint r;

	  if (poll ((struct pollfd*) loop.fds, loop.n_fds, loop.timeout) >= 0)
	    loop.revents_filled = TRUE;

	  if (loop.revents_filled)
	    need_dispatch = _gsl_master_check (&loop);
	}

      if (need_dispatch)
	_gsl_master_dispatch ();
    }
  master_need_user_wakeups = FALSE;
}
