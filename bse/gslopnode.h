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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



#define	OP_NODE(module)			((OpNode*) (module))
#define OP_NODE_N_OSTREAMS(node)	((node)->module.klass->n_ostreams)
#define OP_NODE_N_ISTREAMS(node)	((node)->module.klass->n_istreams)
#define	OP_NODE_IS_CONSUMER(node)	((node)->module.klass->n_ostreams == 0 || \
					 (((node)->module.klass->mflags & GSL_ALWAYS_PROCESS) && \
					  (node)->onodes == NULL))
#define	OP_NODE_IS_DEFERRED(node)	(FALSE)
#define	OP_NODE_IS_SCHEDULED(node)	(OP_NODE (node)->sched_tag)
#define	OP_NODE_IS_CHEAP(node)		(((node)->module.klass->mflags & GSL_COST_CHEAP) != 0)
#define	OP_NODE_IS_EXPENSIVE(node)	(((node)->module.klass->mflags & GSL_COST_EXPENSIVE) != 0)
#define	OP_NODE_LOCK(node)		gsl_rec_mutex_lock (&(node)->rec_mutex)
#define	OP_NODE_UNLOCK(node)		gsl_rec_mutex_unlock (&(node)->rec_mutex)
#define	OP_NODE_SELF_LOCKED(node)	(gsl_rec_mutex_test_self (&OP_NODE (node)->rec_mutex) > 0)


/* --- transactions --- */
typedef enum {
  OP_JOB_NOP,
  OP_JOB_INTEGRATE,
  OP_JOB_DISCARD,
  OP_JOB_CONNECT,
  OP_JOB_DISCONNECT,
  OP_JOB_ACCESS,
  OP_JOB_ADD_POLL,
  OP_JOB_REMOVE_POLL,
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
struct _OpNode
{
  GslModule	 module;
  OpInput	*inputs;	/* [OP_NODE_N_ISTREAMS()] */
  OpOutput	*outputs;	/* [OP_NODE_N_OSTREAMS()] */
  GslRing	*onodes;	/* OpNode* */
  guint64	 counter;
  
  /* master-node-list */
  guint		 mnl_contained : 1;
  OpNode	*mnl_next;
  OpNode	*mnl_prev;
  OpNode	*mcl_next;	/* master-consumer-list */
  
  /* processing lock */
  GslRecMutex	 rec_mutex;
  
  /* scheduler */
  guint		 sched_tag : 1;
  guint		 sched_router_tag : 1;
  guint		 sched_leaf_level;
};



#ifdef __GNUC__
#define OP_DEBUG(lvl, args...)	_op_debug (OP_DEBUG_ ## lvl, args)
void _op_debug         (OpDebugLevel lvl,
		       const gchar *format,
		       ...) G_GNUC_PRINTF (2,3);
#endif

void	_op_engine_inc_counter	(guint64	delta);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_OP_NODE_H__ */
