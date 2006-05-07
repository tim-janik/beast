/* BSE Engine - Flow module operation engine
 * Copyright (C) 2001, 2002, 2003, 2004 Tim Janik
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
#ifndef __BSE_ENGINE_NODE_H__
#define __BSE_ENGINE_NODE_H__

#include "bseengine.h"
#include "gslcommon.h"

G_BEGIN_DECLS

#define	ENGINE_NODE(module)		((EngineNode*) (module))
#define ENGINE_NODE_N_OSTREAMS(node)	((node)->module.klass->n_ostreams)
#define ENGINE_NODE_N_ISTREAMS(node)	((node)->module.klass->n_istreams)
#define ENGINE_NODE_N_JSTREAMS(node)	((node)->module.klass->n_jstreams)
#define	ENGINE_NODE_IS_CONSUMER(node)	((node)->is_consumer && \
					 (node)->output_nodes == NULL)
#define	ENGINE_NODE_IS_INTEGRATED(node)	((node)->integrated)
#define	ENGINE_NODE_IS_VIRTUAL(node)	((node)->virtual_node)
#define	ENGINE_NODE_IS_SUSPENDED(nod,s) ((s) < (nod)->next_active)
#define	ENGINE_NODE_IS_DEFERRED(node)	(FALSE)
#define	ENGINE_NODE_IS_SCHEDULED(node)	(ENGINE_NODE (node)->sched_tag)
#define	ENGINE_NODE_IS_CHEAP(node)	(((node)->module.klass->mflags & BSE_COST_CHEAP) != 0)
#define	ENGINE_NODE_IS_EXPENSIVE(node)	(((node)->module.klass->mflags & BSE_COST_EXPENSIVE) != 0)
#define	ENGINE_NODE_LOCK(node)		birnet_rec_mutex_lock (&(node)->rec_mutex)
#define	ENGINE_NODE_UNLOCK(node)	birnet_rec_mutex_unlock (&(node)->rec_mutex)
#define	ENGINE_MODULE_IS_VIRTUAL(mod)	(ENGINE_NODE_IS_VIRTUAL (ENGINE_NODE (mod)))


/* --- typedefs --- */
typedef struct _EngineNode     EngineNode;
typedef struct _EngineSchedule EngineSchedule;

/* --- transactions --- */
typedef union  _EngineTimedJob EngineTimedJob;
typedef enum /*< skip >*/
{
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
  ENGINE_JOB_MESSAGE,
  ENGINE_JOB_LAST
} EngineJobType;
struct _BseJob
{
  EngineJobType       job_id;
  BseJob	     *next;
  union {
    struct {
      EngineNode     *node;
      gboolean        free_with_job;
      gchar	     *message;
    } data;
    struct {
      BirnetMutex  *lock_mutex;
      BirnetCond   *lock_cond;
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
      BseEngineAccessFunc   access_func;
      gpointer	      data;
      BseFreeFunc     free_func;
    } access;
    struct {
      BseEnginePollFunc     poll_func;
      gpointer	      data;
      BseFreeFunc     free_func;
      guint           n_fds;
      GPollFD	     *fds;
    } poll;
    struct {
      BseEngineTimerFunc timer_func;
      gpointer	         data;
      BseFreeFunc        free_func;
    } timer;
    struct {
      EngineNode     *node;
      EngineTimedJob *tjob;
    } timed_job;
  };
};
struct _BseTrans
{
  BseJob   *jobs_head;
  BseJob   *jobs_tail;
  guint	    comitted : 1;
  BseTrans *cqt_next;	/* com-thread-queue */
};
union _EngineTimedJob
{
  struct {
    EngineJobType       type;           /* common */
    EngineTimedJob     *next;           /* common */
    guint64             tick_stamp;     /* common */
  };
  struct {
    EngineJobType       type;           /* common */
    EngineTimedJob     *next;           /* common */
    guint64             tick_stamp;     /* common */
    gpointer            data;
    BseEngineProbeFunc  probe_func;
    BseOStream         *ostreams;
    guint               n_ostreams;
  }                     probe;
  struct {
    EngineJobType       type;           /* common */
    EngineTimedJob     *next;           /* common */
    guint64             tick_stamp;     /* common */
    gpointer            data;
    BseFreeFunc         free_func;
    BseEngineAccessFunc access_func;
  }                     access;
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
  BseModule	 module;
  
  BirnetRecMutex	 rec_mutex;	/* processing lock */
  guint64	 counter;	/* <= GSL_TICK_STAMP */
  EngineInput	*inputs;	/* [ENGINE_NODE_N_ISTREAMS()] */
  EngineJInput **jinputs;	/* [ENGINE_NODE_N_JSTREAMS()][jstream->jcount] */
  EngineOutput	*outputs;	/* [ENGINE_NODE_N_OSTREAMS()] */
  
  /* timed jobs */
  EngineTimedJob *flow_jobs;			/* active jobs */
  EngineTimedJob *probe_jobs;		        /* probe requests */
  EngineTimedJob *boundary_jobs;		/* active jobs */
  EngineTimedJob *tjob_head, *tjob_tail;	/* trash list */
  
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

#endif /* __BSE_ENGINE_NODE_H__ */
