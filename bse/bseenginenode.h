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
#ifndef __GSL_ENGINE_NODE_H__
#define __GSL_ENGINE_NODE_H__

#include "gslengine.h"
#include "gsloputil.h"
#include "gslcommon.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



#define	ENGINE_NODE(module)		((EngineNode*) (module))
#define ENGINE_NODE_N_OSTREAMS(node)	((node)->module.klass->n_ostreams)
#define ENGINE_NODE_N_ISTREAMS(node)	((node)->module.klass->n_istreams)
#define ENGINE_NODE_N_JSTREAMS(node)	((node)->module.klass->n_jstreams)
#define	ENGINE_NODE_IS_CONSUMER(node)	((node)->is_consumer && \
					 (node)->output_nodes == NULL)
#define	ENGINE_NODE_IS_DEFERRED(node)	(FALSE)
#define	ENGINE_NODE_IS_SCHEDULED(node)	(ENGINE_NODE (node)->sched_tag)
#define	ENGINE_NODE_IS_CHEAP(node)	(((node)->module.klass->mflags & GSL_COST_CHEAP) != 0)
#define	ENGINE_NODE_IS_EXPENSIVE(node)	(((node)->module.klass->mflags & GSL_COST_EXPENSIVE) != 0)
#define	ENGINE_NODE_LOCK(node)		gsl_rec_mutex_lock (&(node)->rec_mutex)
#define	ENGINE_NODE_UNLOCK(node)	gsl_rec_mutex_unlock (&(node)->rec_mutex)


/* --- debugging and messages --- */
#define ENG_DEBUG		GSL_DEBUG_FUNCTION (GSL_MSG_ENGINE, NULL)
#define MAS_DEBUG		GSL_DEBUG_FUNCTION (GSL_MSG_MASTER, NULL)
#define JOB_DEBUG		GSL_DEBUG_FUNCTION (GSL_MSG_JOBS, NULL)
#define SCHED_DEBUG		GSL_DEBUG_FUNCTION (GSL_MSG_SCHED, NULL)


/* --- transactions --- */
typedef union _EngineFlowJob EngineFlowJob;
typedef enum {
  ENGINE_JOB_NOP,
  ENGINE_JOB_INTEGRATE,
  ENGINE_JOB_DISCARD,
  ENGINE_JOB_ICONNECT,
  ENGINE_JOB_JCONNECT,
  ENGINE_JOB_IDISCONNECT,
  ENGINE_JOB_JDISCONNECT,
  ENGINE_JOB_SET_CONSUMER,
  ENGINE_JOB_UNSET_CONSUMER,
  ENGINE_JOB_ACCESS,
  ENGINE_JOB_ADD_POLL,
  ENGINE_JOB_REMOVE_POLL,
  ENGINE_JOB_FLOW_JOB,
  ENGINE_JOB_DEBUG,
  ENGINE_JOB_LAST
} EngineJobType;
struct _GslJob
{
  EngineJobType       job_id;
  GslJob	     *next;
  union {
    EngineNode	     *node;
    struct {
      EngineNode     *dest_node;
      guint	      dest_ijstream;
      EngineNode     *src_node;
      guint	      src_ostream;
    } connection;
    struct {
      EngineNode     *node;
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
      EngineNode     *node;
      EngineFlowJob  *fjob;
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
  ENGINE_FLOW_JOB_NOP,
  ENGINE_FLOW_JOB_SUSPEND,
  ENGINE_FLOW_JOB_RESUME,
  ENGINE_FLOW_JOB_ACCESS,
  ENGINE_FLOW_JOB_LAST
} EngineFlowJobType;
typedef struct
{
  EngineFlowJobType fjob_id;
  EngineFlowJob    *next;
  guint64           tick_stamp;
} EngineFlowJobAny;
typedef struct
{
  EngineFlowJobType fjob_id;
  EngineFlowJob	   *next;
  guint64	    tick_stamp;
  GslAccessFunc     access_func;
  gpointer          data;
  GslFreeFunc       free_func;
} EngineFlowJobAccess;
union _EngineFlowJob
{
  EngineFlowJobType   fjob_id;
  EngineFlowJobAny    any;
  EngineFlowJobAccess access;
};


/* --- module nodes --- */
typedef struct
{
  EngineNode *src_node;
  guint	      src_stream;	/* ostream of src_node */
} EngineInput;
typedef struct
{
  EngineNode *src_node;
  guint	      src_stream;	/* ostream of src_node */
} EngineJInput;
typedef struct
{
  gfloat *buffer;
  guint	  n_outputs;
} EngineOutput;
struct _EngineNode		/* fields sorted by order of processing access */
{
  GslModule	 module;

  GslRecMutex	 rec_mutex;	/* processing lock */
  guint64	 counter;	/* <= GSL_TICK_STAMP */
  EngineInput	*inputs;	/* [ENGINE_NODE_N_ISTREAMS()] */
  EngineJInput **jinputs;	/* [ENGINE_NODE_N_JSTREAMS()] */
  EngineOutput	*outputs;	/* [ENGINE_NODE_N_OSTREAMS()] */

  /* flow jobs */
  EngineFlowJob	*flow_jobs;			/* active jobs */
  EngineFlowJob	*fjob_first, *fjob_last;	/* trash list */

  /* master-node-list */
  EngineNode	*mnl_next;
  EngineNode	*mnl_prev;
  guint		 integrated : 1;
  guint		 reconnected : 1;

  guint		 is_consumer : 1;
  
  /* scheduler */
  guint		 sched_tag : 1;
  guint		 sched_router_tag : 1;
  guint		 sched_leaf_level;
  EngineNode	*toplevel_next;	/* master-consumer-list, FIXME: overkill, using a GslRing is good enough */
  GslRing	*output_nodes;	/* EngineNode* ring of nodes in ->outputs[] */
};

static void
_engine_node_insert_flow_job (EngineNode    *node,
			      EngineFlowJob *fjob)
{
  EngineFlowJob *last = NULL, *tmp = node->flow_jobs;

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

static inline EngineFlowJob*
_engine_node_pop_flow_job (EngineNode *node,
			   guint64     tick_stamp)
{
  EngineFlowJob *fjob = node->flow_jobs;

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
_engine_node_peek_flow_job_stamp (EngineNode *node)
{
  EngineFlowJob *fjob = node->flow_jobs;

  if_reject (fjob)
    return fjob->any.tick_stamp;

  return GSL_MAX_TICK_STAMP;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_ENGINE_NODE_H__ */
