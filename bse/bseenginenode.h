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
#ifndef __GSL_ENGINE_NODE_H__
#define __GSL_ENGINE_NODE_H__

#include "gslengine.h"
#include "gsloputil.h"
#include "gslcommon.h"

G_BEGIN_DECLS

#define	ENGINE_NODE(module)		((EngineNode*) (module))
#define ENGINE_NODE_N_OSTREAMS(node)	((node)->module.klass->n_ostreams)
#define ENGINE_NODE_N_ISTREAMS(node)	((node)->module.klass->n_istreams)
#define ENGINE_NODE_N_JSTREAMS(node)	((node)->module.klass->n_jstreams)
#define	ENGINE_NODE_IS_CONSUMER(node)	((node)->is_consumer && \
					 (node)->output_nodes == NULL)
#define	ENGINE_NODE_IS_SUSPENDED(nod,s) ((s) < (nod)->next_active)
#define	ENGINE_NODE_IS_VIRTUAL(node)	((node)->virtual_node)
#define	ENGINE_NODE_IS_DEFERRED(node)	(FALSE)
#define	ENGINE_NODE_IS_SCHEDULED(node)	(ENGINE_NODE (node)->sched_tag)
#define	ENGINE_NODE_IS_CHEAP(node)	(((node)->module.klass->mflags & GSL_COST_CHEAP) != 0)
#define	ENGINE_NODE_IS_EXPENSIVE(node)	(((node)->module.klass->mflags & GSL_COST_EXPENSIVE) != 0)
#define	ENGINE_NODE_LOCK(node)		sfi_rec_mutex_lock (&(node)->rec_mutex)
#define	ENGINE_NODE_UNLOCK(node)	sfi_rec_mutex_unlock (&(node)->rec_mutex)
#define	ENGINE_MODULE_IS_VIRTUAL(mod)	(ENGINE_NODE_IS_VIRTUAL (ENGINE_NODE (mod)))


/* --- transactions --- */
typedef struct _EngineUserJob  EngineUserJob;
typedef struct _EngineProbeJob EngineProbeJob;
typedef struct _EngineTimedJob EngineTimedJob;
typedef enum {
  ENGINE_JOB_NOP,
  ENGINE_JOB_SYNC,
  ENGINE_JOB_INTEGRATE,
  ENGINE_JOB_DISCARD,
  ENGINE_JOB_ICONNECT,
  ENGINE_JOB_JCONNECT,
  ENGINE_JOB_IDISCONNECT,
  ENGINE_JOB_JDISCONNECT,
  ENGINE_JOB_KILL_INPUTS,
  ENGINE_JOB_KILL_OUTPUTS,
  ENGINE_JOB_SET_CONSUMER,
  ENGINE_JOB_UNSET_CONSUMER,
  ENGINE_JOB_FORCE_RESET,
  ENGINE_JOB_ACCESS,
  ENGINE_JOB_SUSPEND,
  ENGINE_JOB_RESUME,
  ENGINE_JOB_ADD_POLL,
  ENGINE_JOB_REMOVE_POLL,
  ENGINE_JOB_ADD_TIMER,
  ENGINE_JOB_PROBE_JOB,
  ENGINE_JOB_FLOW_JOB,
  ENGINE_JOB_BOUNDARY_JOB,
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
      SfiMutex  *lock_mutex;
      SfiCond   *lock_cond;
      gboolean  *lock_p;
    } sync;
    struct {
      EngineNode     *node;
      guint64         stamp;
    } tick;
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
      GslEngineTimerFunc timer_func;
      gpointer	         data;
      GslFreeFunc        free_func;
    } timer;
    struct {
      EngineNode     *node;
      EngineTimedJob *tjob;
    } timed_job;
    struct {
      EngineNode     *node;
      EngineProbeJob *pjob;
    } probe_job;
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
struct _EngineUserJob
{
  EngineUserJob    *next;
  EngineJobType     job_type;
};
struct _EngineProbeJob
{
  EngineProbeJob   *next;       /* keep in sync with EngineUserJob */
  EngineJobType     job_type;   /* keep in sync with EngineUserJob */
  GslProbeFunc      probe_func;
  gpointer          data;
  guint64           tick_stamp;
  guint             n_values;
  gfloat           *oblocks[1]; /* [ENGINE_NODE_N_OSTREAMS()] */
};
struct _EngineTimedJob
{
  EngineTimedJob   *next;       /* keep in sync with EngineUserJob */
  EngineJobType     job_type;   /* keep in sync with EngineUserJob */
  GslFreeFunc       free_func;
  gpointer          data;
  guint64	    tick_stamp;
  GslAccessFunc     access_func;
};


/* --- module nodes --- */
typedef struct
{
  EngineNode *src_node;
  guint	      src_stream;	/* ostream of src_node */
  /* valid if istream[].connected, setup by scheduler */
  EngineNode *real_node;	/* set to NULL if !connected */
  guint	      real_stream;	/* ostream of real_node */
} EngineInput;
typedef struct
{
  EngineNode *src_node;
  guint	      src_stream;	/* ostream of src_node */
  /* valid if < jstream[].n_connections, setup by scheduler */
  EngineNode *real_node;
  guint       real_stream;	/* ostream of real_node */
} EngineJInput;
typedef struct
{
  gfloat *buffer;
  guint	  n_outputs;
} EngineOutput;
struct _EngineNode		/* fields sorted by order of processing access */
{
  GslModule	 module;

  SfiRecMutex	 rec_mutex;	/* processing lock */
  guint64	 counter;	/* <= GSL_TICK_STAMP */
  EngineInput	*inputs;	/* [ENGINE_NODE_N_ISTREAMS()] */
  EngineJInput **jinputs;	/* [ENGINE_NODE_N_JSTREAMS()][jstream->jcount] */
  EngineOutput	*outputs;	/* [ENGINE_NODE_N_OSTREAMS()] */

  /* timed jobs */
  EngineTimedJob *flow_jobs;			/* active jobs */
  EngineProbeJob *probe_jobs;		        /* probe requests */
  EngineTimedJob *boundary_jobs;		/* active jobs */
  EngineUserJob  *ujob_first, *ujob_last;	/* trash list */

  /* suspend/activation time */
  guint64        next_active;           /* result of suspend state updates */

  /* master-node-list */
  EngineNode	*mnl_next;
  EngineNode	*mnl_prev;
  guint		 integrated : 1;
  guint		 virtual_node : 1;

  guint		 is_consumer : 1;

  /* suspension */
  guint		 update_suspend : 1;	/* whether suspend state needs updating */
  guint		 in_suspend_call : 1;	/* recursion barrier during suspend state updates */
  guint		 needs_reset : 1;	/* flagged at resumption */

  /* scheduler */
  guint		 cleared_ostreams : 1;	/* whether ostream[].connected was cleared already */
  guint		 sched_tag : 1;		/* whether this node is contained in the schedule */
  guint		 sched_recurse_tag : 1;	/* recursion flag used during scheduling */
  guint		 sched_leaf_level;
  guint64        local_active;          /* local suspend state stamp */
  EngineNode	*toplevel_next;	        /* master-consumer-list, FIXME: overkill, using a SfiRing is good enough */
  SfiRing	*output_nodes;	        /* EngineNode* ring of nodes in ->outputs[] */
};

G_END_DECLS

#endif /* __GSL_ENGINE_NODE_H__ */
