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
#define	__GSL_ENGINE_MASTER_C__
#include "gslopmaster.h"

#include "gslcommon.h"
#include "gslopnode.h"
#include "gsloputil.h"
#include "gslopschedule.h"
#include "gslieee754.h"
#include <string.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <errno.h>



/* force public constantness but allow us to change values
 * in these structures.
 * *** sync this with gslengine.h ***
 */
struct _GslJStream
{
  const gfloat      **values;
  /* const */ guint   n_connections;
  guint               user_flags : 16;
};
struct _GslIStream
{
  const gfloat     *values;
  guint             user_flags : 16;
  /* const */ guint connected : 1;
};
struct _GslOStream
{
  gfloat           *values;
  guint             user_flags : 16;
  /* const */ guint connected : 1;
};

#define	NODE_FLAG_RECONNECT(node)	G_STMT_START { (node)->reconnected = (node)->module.klass->reconnect != NULL; } G_STMT_END


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
  GslPollFunc poll_func;
  gpointer    data;
  guint       n_fds;
  GPollFD    *fds;
  GslFreeFunc free_func;
};


/* --- prototypes --- */
static void	master_schedule_discard	(void);


/* --- variables --- */
static gboolean	       master_need_reflow = FALSE;
static gboolean	       master_need_process = FALSE;
static EngineNode     *master_consumer_list = NULL;
const gfloat           gsl_engine_master_zero_block[GSL_STREAM_MAX_VALUES] = { 0, }; /* FIXME */
static Poll	      *master_poll_list = NULL;
static guint           master_n_pollfds = 0;
static guint           master_pollfds_changed = FALSE;
static GPollFD         master_pollfds[GSL_ENGINE_MAX_POLLFDS];
static EngineSchedule *master_schedule = NULL;


/* --- functions --- */
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
  node->module.istreams[istream].connected = FALSE;
  was_consumer = ENGINE_NODE_IS_CONSUMER (src_node);
  src_node->outputs[ostream].n_outputs -= 1;
  src_node->module.ostreams[ostream].connected = src_node->outputs[ostream].n_outputs > 0;
  src_node->output_nodes = gsl_ring_remove (src_node->output_nodes, node);
  NODE_FLAG_RECONNECT (node);
  NODE_FLAG_RECONNECT (src_node);
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
	    node->module.jstreams[jstream].n_connections > 0 &&
	    src_node->outputs[ostream].n_outputs > 0);	/* these checks better pass */
  
  i = --node->module.jstreams[jstream].n_connections;
  node->jinputs[jstream][con] = node->jinputs[jstream][i];
  node->module.jstreams[jstream].values[i] = NULL; /* float**values 0-termination */
  was_consumer = ENGINE_NODE_IS_CONSUMER (src_node);
  src_node->outputs[ostream].n_outputs -= 1;
  src_node->module.ostreams[ostream].connected = src_node->outputs[ostream].n_outputs > 0;
  src_node->output_nodes = gsl_ring_remove (src_node->output_nodes, node);
  NODE_FLAG_RECONNECT (node);
  NODE_FLAG_RECONNECT (src_node);
  /* add to consumer list */
  if (!was_consumer && ENGINE_NODE_IS_CONSUMER (src_node))
    add_consumer (src_node);
}

static void
master_disconnect_node_outputs (EngineNode *src_node,
				EngineNode *dest_node)
{
  gint i, j;
  
  for (i = 0; i < ENGINE_NODE_N_ISTREAMS (dest_node); i++)
    if (dest_node->inputs[i].src_node == src_node)
      master_idisconnect_node (dest_node, i);
  for (j = 0; j < ENGINE_NODE_N_JSTREAMS (dest_node); j++)
    for (i = 0; i < dest_node->module.jstreams[j].n_connections; i++)
      if (dest_node->jinputs[j][i].src_node == src_node)
	master_jdisconnect_node (dest_node, j, i--);
}

static void
master_process_job (GslJob *job)
{
  switch (job->job_id)
    {
      EngineNode *node, *src_node;
      Poll *poll, *poll_last;
      guint istream, jstream, ostream, con;
      EngineFlowJob *fjob;
      gboolean was_consumer;
    case ENGINE_JOB_INTEGRATE:
      node = job->data.node;
      JOB_DEBUG ("integrate(%p)", node);
      g_return_if_fail (node->integrated == FALSE);
      g_return_if_fail (node->sched_tag == FALSE);
      _engine_mnl_integrate (node);
      if (ENGINE_NODE_IS_CONSUMER (node))
	add_consumer (node);
      node->counter = 0;
      NODE_FLAG_RECONNECT (node);
      master_need_reflow |= TRUE;
      break;
    case ENGINE_JOB_DISCARD:
      /* FIXME: free pending flow jobs */
      node = job->data.node;
      JOB_DEBUG ("discard(%p)", node);
      g_return_if_fail (node->integrated == TRUE);
      /* disconnect inputs */
      for (istream = 0; istream < ENGINE_NODE_N_ISTREAMS (node); istream++)
	if (node->inputs[istream].src_node)
	  master_idisconnect_node (node, istream);
      for (jstream = 0; jstream < ENGINE_NODE_N_JSTREAMS (node); jstream++)
	while (node->module.jstreams[jstream].n_connections)
	  master_jdisconnect_node (node, jstream, node->module.jstreams[jstream].n_connections - 1);
      /* disconnect outputs */
      while (node->output_nodes)
	master_disconnect_node_outputs (node, node->output_nodes->data);
      /* remove from consumer list */
      if (ENGINE_NODE_IS_CONSUMER (node))
	{
	  _engine_mnl_remove (node);
	  remove_consumer (node);
	}
      else
	_engine_mnl_remove (node);
      node->counter = 0;
      master_need_reflow |= TRUE;
      master_schedule_discard ();	/* discard schedule so node may be freed */
      break;
    case ENGINE_JOB_SET_CONSUMER:
    case ENGINE_JOB_UNSET_CONSUMER:
      node = job->data.node;
      JOB_DEBUG ("toggle_consumer(%p)", node);
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
    case ENGINE_JOB_ICONNECT:
      node = job->data.connection.dest_node;
      src_node = job->data.connection.src_node;
      istream = job->data.connection.dest_ijstream;
      ostream = job->data.connection.src_ostream;
      JOB_DEBUG ("connect(%p,%u,%p,%u)", node, istream, src_node, ostream);
      g_return_if_fail (node->integrated == TRUE);
      g_return_if_fail (src_node->integrated == TRUE);
      g_return_if_fail (node->inputs[istream].src_node == NULL);
      node->inputs[istream].src_node = src_node;
      node->inputs[istream].src_stream = ostream;
      node->module.istreams[istream].connected = TRUE;
      /* remove from consumer list */
      was_consumer = ENGINE_NODE_IS_CONSUMER (src_node);
      src_node->outputs[ostream].n_outputs += 1;
      src_node->module.ostreams[ostream].connected = TRUE;
      src_node->output_nodes = gsl_ring_append (src_node->output_nodes, node);
      NODE_FLAG_RECONNECT (node);
      NODE_FLAG_RECONNECT (src_node);
      src_node->counter = 0;	/* FIXME: counter reset? */
      if (was_consumer && !ENGINE_NODE_IS_CONSUMER (src_node))
	remove_consumer (src_node);
      master_need_reflow |= TRUE;
      break;
    case ENGINE_JOB_JCONNECT:
      node = job->data.connection.dest_node;
      src_node = job->data.connection.src_node;
      jstream = job->data.connection.dest_ijstream;
      ostream = job->data.connection.src_ostream;
      JOB_DEBUG ("jconnect(%p,%u,%p,%u)", node, jstream, src_node, ostream);
      g_return_if_fail (node->integrated == TRUE);
      g_return_if_fail (src_node->integrated == TRUE);
      con = node->module.jstreams[jstream].n_connections++;
      node->jinputs[jstream] = g_renew (EngineJInput, node->jinputs[jstream], node->module.jstreams[jstream].n_connections);
      node->module.jstreams[jstream].values = g_renew (const gfloat*, node->module.jstreams[jstream].values, node->module.jstreams[jstream].n_connections + 1);
      node->module.jstreams[jstream].values[node->module.jstreams[jstream].n_connections] = NULL; /* float**values 0-termination */
      node->jinputs[jstream][con].src_node = src_node;
      node->jinputs[jstream][con].src_stream = ostream;
      /* remove from consumer list */
      was_consumer = ENGINE_NODE_IS_CONSUMER (src_node);
      src_node->outputs[ostream].n_outputs += 1;
      src_node->module.ostreams[ostream].connected = TRUE;
      src_node->output_nodes = gsl_ring_append (src_node->output_nodes, node);
      NODE_FLAG_RECONNECT (node);
      NODE_FLAG_RECONNECT (src_node);
      src_node->counter = 0;	/* FIXME: counter reset? */
      if (was_consumer && !ENGINE_NODE_IS_CONSUMER (src_node))
	remove_consumer (src_node);
      master_need_reflow |= TRUE;
      break;
    case ENGINE_JOB_IDISCONNECT:
      node = job->data.connection.dest_node;
      JOB_DEBUG ("idisconnect(%p,%u)", node, job->data.connection.dest_ijstream);
      g_return_if_fail (node->integrated == TRUE);
      g_return_if_fail (node->inputs[job->data.connection.dest_ijstream].src_node != NULL);
      master_idisconnect_node (node, job->data.connection.dest_ijstream);
      master_need_reflow |= TRUE;
      break;
    case ENGINE_JOB_JDISCONNECT:
      node = job->data.connection.dest_node;
      jstream = job->data.connection.dest_ijstream;
      src_node = job->data.connection.src_node;
      ostream = job->data.connection.src_ostream;
      JOB_DEBUG ("jdisconnect(%p,%u,%p,%u)", node, jstream, src_node, ostream);
      g_return_if_fail (node->integrated == TRUE);
      g_return_if_fail (node->module.jstreams[jstream].n_connections > 0);
      for (con = 0; con < node->module.jstreams[jstream].n_connections; con++)
	if (node->jinputs[jstream][con].src_node == src_node &&
	    node->jinputs[jstream][con].src_stream == ostream)
	  break;
      if (con < node->module.jstreams[jstream].n_connections)
	{
	  master_jdisconnect_node (node, jstream, con);
	  master_need_reflow |= TRUE;
	}
      else
	g_warning ("jdisconnect(dest:%p,%u,src:%p,%u): no such connection", node, jstream, src_node, ostream);
      break;
    case ENGINE_JOB_ACCESS:
      node = job->data.access.node;
      JOB_DEBUG ("access node(%p): %p(%p)", node, job->data.access.access_func, job->data.access.data);
      g_return_if_fail (node->integrated == TRUE);
      job->data.access.access_func (&node->module, job->data.access.data);
      break;
    case ENGINE_JOB_FLOW_JOB:
      node = job->data.flow_job.node;
      fjob = job->data.flow_job.fjob;
      JOB_DEBUG ("add flow_job(%p,%p)", node, fjob);
      g_return_if_fail (node->integrated == TRUE);
      job->data.flow_job.fjob = NULL;	/* ownership taken over */
      _engine_node_insert_flow_job (node, fjob);
      _engine_mnl_reorder (node);
      break;
    case ENGINE_JOB_DEBUG:
      JOB_DEBUG ("debug");
      g_printerr ("JOB-DEBUG: %s\n", job->data.debug);
      break;
    case ENGINE_JOB_ADD_POLL:
      JOB_DEBUG ("add poll %p(%p,%u)", job->data.poll.poll_func, job->data.poll.data, job->data.poll.n_fds);
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
    case ENGINE_JOB_REMOVE_POLL:
      JOB_DEBUG ("remove poll %p(%p)", job->data.poll.poll_func, job->data.poll.data);
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
	  gsl_delete_struct (Poll, poll_last);
	}
      else
	g_warning (G_STRLOC ": failed to remove unknown poll function %p(%p)",
		   job->data.poll.poll_func, job->data.poll.data);
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

static inline guint64
master_handle_flow_jobs (EngineNode *node,
			 guint64     max_tick)
{
  EngineFlowJob *fjob = _engine_node_pop_flow_job (node, max_tick);
  
  if_reject (fjob)
    do
      {
	g_printerr ("FJob: at:%lld from:%lld \n", node->counter, fjob->any.tick_stamp);
	switch (fjob->fjob_id)
	  {
	  case ENGINE_FLOW_JOB_ACCESS:
	    fjob->access.access_func (&node->module, fjob->access.data);
	    break;
	  default:
	    g_assert_not_reached (); /* FIXME */
	  }
	fjob = _engine_node_pop_flow_job (node, max_tick);
      }
    while (fjob);
  
  return _engine_node_peek_flow_job_stamp (node);
}

static void
master_process_locked_node (EngineNode *node,
			    guint       n_values)
{
  guint64 final_counter = GSL_TICK_STAMP + n_values;
  
  while (node->counter < final_counter)
    {
      guint64 next_counter = master_handle_flow_jobs (node, node->counter);
      guint64 new_counter = MIN (next_counter, final_counter);
      guint i, j, diff = node->counter - GSL_TICK_STAMP;
      
      for (i = 0; i < ENGINE_NODE_N_ISTREAMS (node); i++)
	{
	  EngineNode *inode = node->inputs[i].src_node;
	  
	  if (inode)
	    {
	      ENGINE_NODE_LOCK (inode);
	      if (inode->counter < final_counter)
		master_process_locked_node (inode, final_counter - node->counter);
	      node->module.istreams[i].values = inode->outputs[node->inputs[i].src_stream].buffer;
	      node->module.istreams[i].values += diff;
	      ENGINE_NODE_UNLOCK (inode);
	    }
	  else
	    node->module.istreams[i].values = gsl_engine_master_zero_block;
	}
      for (j = 0; j < ENGINE_NODE_N_JSTREAMS (node); j++)
	for (i = 0; i < node->module.jstreams[j].n_connections; i++)
	  {
	    EngineNode *inode = node->jinputs[j][i].src_node;
	    
	    ENGINE_NODE_LOCK (inode);
	    if (inode->counter < final_counter)
	      master_process_locked_node (inode, final_counter - node->counter);
	    node->module.jstreams[j].values[i] = inode->outputs[node->jinputs[j][i].src_stream].buffer;
	    node->module.jstreams[j].values[i] += diff;
	    ENGINE_NODE_UNLOCK (inode);
	  }
      for (i = 0; i < ENGINE_NODE_N_OSTREAMS (node); i++)
	node->module.ostreams[i].values = node->outputs[i].buffer + diff;
      if_reject (node->reconnected)
	{
	  node->module.klass->reconnect (&node->module);
	  node->reconnected = FALSE;
	}
      node->module.klass->process (&node->module, new_counter - node->counter);
      for (i = 0; i < ENGINE_NODE_N_OSTREAMS (node); i++)
	{
	  /* FIXME: this takes the worst possible performance hit to support virtualization */
	  if (node->module.ostreams[i].values != node->outputs[i].buffer + diff)
	    memcpy (node->outputs[i].buffer + diff, node->module.ostreams[i].values,
		    (new_counter - node->counter) * sizeof (gfloat));
	}
      node->counter = new_counter;
    }
}

static GslLong gsl_profile_modules = 0;	/* set to 1 in gdb to get profile output */

static void
master_process_flow (void)
{
  guint64 new_counter = GSL_TICK_STAMP + gsl_engine_block_size ();
  GslLong profile_maxtime = 0;
  GslLong profile_modules = gsl_profile_modules;
  EngineNode *profile_node = NULL;
  
  g_return_if_fail (master_need_process == TRUE);
  
  g_assert (gsl_fpu_okround () == TRUE);
  
  MAS_DEBUG ("process_flow");
  if (master_schedule)
    {
      EngineNode *node;
      
      _engine_schedule_restart (master_schedule);
      _engine_set_schedule (master_schedule);
      
      node = _engine_pop_unprocessed_node ();
      while (node)
	{
	  ToyprofStamp profile_stamp1, profile_stamp2;
	  
	  if_reject (profile_modules)
	    toyprof_stamp (profile_stamp1);
	  
	  master_process_locked_node (node, gsl_engine_block_size ());
	  
	  if_reject (profile_modules)
	    {
	      GslLong duration;
	      
	      toyprof_stamp (profile_stamp2);
	      duration = toyprof_elapsed (profile_stamp1, profile_stamp2);
	      if (duration > profile_maxtime)
		{
		  profile_maxtime = duration;
		  profile_node = node;
		}
	    }
	  
	  _engine_push_processed_node (node);
	  node = _engine_pop_unprocessed_node ();
	}
      
      if_reject (profile_modules)
	{
	  if (profile_node)
	    {
	      if (profile_maxtime > profile_modules)
		g_print ("Excess Node: %p  Duration: %lu usecs     ((void(*)())%p)         \n",
			 profile_node, profile_maxtime, profile_node->module.klass->process);
	      else
		g_print ("Slowest Node: %p  Duration: %lu usecs     ((void(*)())%p)         \r",
			 profile_node, profile_maxtime, profile_node->module.klass->process);
	    }
	}
      
      /* walk unscheduled nodes which have flow jobs */
      node = _engine_mnl_head ();
      while (node && GSL_MNL_HEAD_NODE (node))
	{
	  EngineNode *tmp = node->mnl_next;
	  EngineFlowJob *fjob = _engine_node_pop_flow_job (node, new_counter);
	  
	  if (fjob)
	    {
	      while (fjob)
		{
		  g_printerr ("ignoring flow_job %p\n", fjob);
		  fjob = _engine_node_pop_flow_job (node, new_counter);
		}
	      _engine_mnl_reorder (node);
	    }
	  node = tmp;
	}
      
      /* nothing new to process, wait on slaves */
      _engine_wait_on_unprocessed ();
      
      _engine_unset_schedule (master_schedule);
      _gsl_tick_stamp_inc ();
      _engine_recycle_const_values ();
    }
  master_need_process = FALSE;
}

static void
master_reschedule_flow (void)
{
  EngineNode *node;
  
  g_return_if_fail (master_need_reflow == TRUE);
  
  MAS_DEBUG ("flow_reschedule");
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
_engine_master_prepare (GslEngineLoop *loop)
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
  
  MAS_DEBUG ("PREPARE: need_dispatch=%u timeout=%6ld n_fds=%u",
	     need_dispatch,
	     loop->timeout, loop->n_fds);
  
  return need_dispatch;
}

gboolean
_engine_master_check (const GslEngineLoop *loop)
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
  
  MAS_DEBUG ("CHECK: need_dispatch=%u", need_dispatch);
  
  return need_dispatch;
}

void
_engine_master_dispatch_jobs (void)
{
  GslJob *job;
  
  job = _engine_pop_job ();
  while (job)
    {
      master_process_job (job);
      job = _engine_pop_job ();	/* have to process _all_ jobs */
    }
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

void
_engine_master_thread (gpointer data)
{
  gboolean run = TRUE;
  
  /* assert sane configuration checks, since we're simply casting structures */
  g_assert (sizeof (struct pollfd) == sizeof (GPollFD) &&
	    G_STRUCT_OFFSET (GPollFD, fd) == G_STRUCT_OFFSET (struct pollfd, fd) &&
	    G_STRUCT_OFFSET (GPollFD, events) == G_STRUCT_OFFSET (struct pollfd, events) &&
	    G_STRUCT_OFFSET (GPollFD, revents) == G_STRUCT_OFFSET (struct pollfd, revents));
  
  /* add the thread wakeup pipe to master pollfds, so we get woken
   * up in time (even though we evaluate the pipe contents later)
   */
  gsl_thread_get_pollfd (master_pollfds);
  master_n_pollfds += 1;
  master_pollfds_changed = TRUE;
  
  toyprof_stampinit ();
  
  while (run)
    {
      GslEngineLoop loop;
      gboolean need_dispatch;
      
      need_dispatch = _engine_master_prepare (&loop);
      
      if (!need_dispatch)
	{
	  gint err;
	  
	  err = poll ((struct pollfd*) loop.fds, loop.n_fds, loop.timeout);
	  
	  if (err >= 0)
	    loop.revents_filled = TRUE;
	  else
	    g_printerr (G_STRLOC ": poll() error: %s\n", g_strerror (errno));
	  
	  if (loop.revents_filled)
	    need_dispatch = _engine_master_check (&loop);
	}
      
      if (need_dispatch)
	_engine_master_dispatch ();
      
      /* handle thread pollfd messages */
      run = gsl_thread_sleep (0);
    }
}
/* vim:set ts=8 sts=2 sw=2: */
