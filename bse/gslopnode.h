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
#ifndef __GSL_OP_NODE_H__
#define __GSL_OP_NODE_H__

#include "gslengine.h"
#include "gsloputil.h"
#include "gslcommon.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



#define	OP_NODE(module)			((OpNode*) (module))
#define OP_NODE_N_OSTREAMS(node)	((node)->module.klass->n_ostreams)
#define OP_NODE_N_ISTREAMS(node)	((node)->module.klass->n_istreams)
#define	OP_NODE_IS_CONSUMER(node)	((node)->is_consumer && \
					 (node)->output_nodes == NULL)
#define	OP_NODE_IS_DEFERRED(node)	(FALSE)
#define	OP_NODE_IS_SCHEDULED(node)	(OP_NODE (node)->sched_tag)
#define	OP_NODE_IS_CHEAP(node)		(((node)->module.klass->mflags & GSL_COST_CHEAP) != 0)
#define	OP_NODE_IS_EXPENSIVE(node)	(((node)->module.klass->mflags & GSL_COST_EXPENSIVE) != 0)
#define	OP_NODE_LOCK(node)		gsl_rec_mutex_lock (&(node)->rec_mutex)
#define	OP_NODE_UNLOCK(node)		gsl_rec_mutex_unlock (&(node)->rec_mutex)
#define	OP_NODE_SELF_LOCKED(node)	(gsl_rec_mutex_test_self (&OP_NODE (node)->rec_mutex) > 0)


/* --- transactions --- */
typedef union _GslFlowJob GslFlowJob;
typedef enum {
  OP_JOB_NOP,
  OP_JOB_INTEGRATE,
  OP_JOB_DISCARD,
  OP_JOB_CONNECT,
  OP_JOB_DISCONNECT,
  GSL_JOB_SET_CONSUMER,
  GSL_JOB_UNSET_CONSUMER,
  GSL_JOB_ACCESS,
  OP_JOB_ADD_POLL,
  OP_JOB_REMOVE_POLL,
  GSL_JOB_FLOW_JOB,
  OP_JOB_DEBUG,
  OP_JOB_LAST
} GslJobType;
struct _GslJob
{
  GslJobType          job_id;
  GslJob	     *next;
  union {
    OpNode	     *node;
    struct {
      OpNode	     *dest_node;
      guint	      dest_istream;
      OpNode	     *src_node;
      guint	      src_ostream;
    } connection;
    struct {
      OpNode         *node;
      GslAccessFunc   access_func;
      gpointer	      data;
      GslFreeFunc     free_func;
    } access;
    struct {
      GslPollFunc     poll_func;
      gpointer	      data;
      GslFreeFunc     free_func;
      guint           n_fds;
      GPollFD	     *fds;
    } poll;
    struct {
      OpNode	     *node;
      GslFlowJob     *fjob;
    } flow_job;
    gchar	     *debug;
  } data;
};
struct _GslTrans
{
  GslJob   *jobs_head;
  GslJob   *jobs_tail;
  guint	    comitted : 1;
  GslTrans *cqt_next;	/* com-thread-queue */
};
typedef enum {
  GSL_FLOW_JOB_NOP,
  GSL_FLOW_JOB_SUSPEND,
  GSL_FLOW_JOB_RESUME,
  GSL_FLOW_JOB_ACCESS,
  GSL_FLOW_JOB_LAST
} GslFlowJobType;
typedef struct
{
  GslFlowJobType   fjob_id;
  GslFlowJob      *next;
  guint64          tick_stamp;
} GslFlowJobAny;
typedef struct
{
  GslFlowJobType   fjob_id;
  GslFlowJob	  *next;
  guint64	   tick_stamp;
  GslAccessFunc    access_func;
  gpointer         data;
  GslFreeFunc      free_func;
} GslFlowJobAccess;
union _GslFlowJob
{
  GslFlowJobType   fjob_id;
  GslFlowJobAny	   any;
  GslFlowJobAccess access;
};


/* --- module nodes --- */
typedef struct
{
  OpNode *src_node;
  guint	  src_stream;		/* ostream of src_node */
} OpInput;
typedef struct
{
  gfloat *buffer;
  guint	  n_outputs;
} OpOutput;
struct _OpNode	/* fields sorted by order of processing access */
{
  GslModule	 module;

  GslRecMutex	 rec_mutex;	/* processing lock */
  guint64	 counter;	/* <= GSL_TICK_STAMP */
  OpInput	*inputs;	/* [OP_NODE_N_ISTREAMS()] */
  OpOutput	*outputs;	/* [OP_NODE_N_OSTREAMS()] */

  /* flow jobs */
  GslFlowJob	*flow_jobs;			/* active jobs */
  GslFlowJob	*fjob_first, *fjob_last;	/* trash list */

  /* master-node-list */
  OpNode	*mnl_next;
  OpNode	*mnl_prev;
  guint		 integrated : 1;

  guint		 is_consumer : 1;
  
  /* scheduler */
  guint		 sched_tag : 1;
  guint		 sched_router_tag : 1;
  guint		 sched_leaf_level;
  OpNode	*toplevel_next;	/* master-consumer-list, FIXME: overkill, using a GslRing is good enough */
  GslRing	*output_nodes;	/* OpNode* ring of nodes in ->outputs[] */
};

static void
_gsl_node_insert_flow_job (OpNode     *node,
			   GslFlowJob *fjob)
{
  GslFlowJob *last = NULL, *tmp = node->flow_jobs;

  /* find next position */
  while (tmp && tmp->any.tick_stamp <= fjob->any.tick_stamp)
    {
      last = tmp;
      tmp = last->any.next;
    }
  /* insert before */
  fjob->any.next = tmp;
  if (last)
    last->any.next = fjob;
  else
    node->flow_jobs = fjob;
}

static inline GslFlowJob*
_gsl_node_pop_flow_job (OpNode *node,
			guint64 tick_stamp)
{
  GslFlowJob *fjob = node->flow_jobs;

  if_reject (fjob)
    {
      if (fjob->any.tick_stamp <= tick_stamp)
	{
	  node->flow_jobs = fjob->any.next;
	  
	  fjob->any.next = node->fjob_first;
	  node->fjob_first = fjob;
	  if (!node->fjob_last)
	    node->fjob_last = node->fjob_first;
	}
      else
	fjob = NULL;
    }

  return fjob;
}

static inline guint64
_gsl_node_peek_flow_job_stamp (OpNode *node)
{
  GslFlowJob *fjob = node->flow_jobs;

  if_reject (fjob)
    return fjob->any.tick_stamp;

  return GSL_MAX_TICK_STAMP;
}

#if defined(__GNUC__) || defined(__DECC__)
#define OP_DEBUG	_gsl_op_debug
void _gsl_op_debug    (GslEngineDebugLevel lvl,
		       const gchar        *format,
		       ...) G_GNUC_PRINTF (2,3);
#endif

void	_op_engine_inc_counter	(guint64	delta);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_OP_NODE_H__ */
