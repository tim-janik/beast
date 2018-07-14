// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_ENGINE_PRIVATE_H__
#define __BSE_ENGINE_PRIVATE_H__

#include "bseengine.hh"
#include "gslcommon.hh"

#define	ENGINE_NODE(module)		(module)
#define ENGINE_NODE_N_OSTREAMS(node)	((node)->module().klass->n_ostreams)
#define ENGINE_NODE_N_ISTREAMS(node)	((node)->module().klass->n_istreams)
#define ENGINE_NODE_N_JSTREAMS(node)	((node)->module().klass->n_jstreams)
#define	ENGINE_NODE_IS_CONSUMER(node)	((node)->is_consumer && \
					 (node)->output_nodes == NULL)
#define	ENGINE_NODE_IS_INTEGRATED(node)	((node)->integrated)
#define	ENGINE_NODE_IS_VIRTUAL(node)	((node)->virtual_node)
#define	ENGINE_NODE_IS_SUSPENDED(nod,s) ((s) < (nod)->next_active)
#define	ENGINE_NODE_IS_DEFERRED(node)	(false)
#define	ENGINE_NODE_IS_SCHEDULED(node)	(node->sched_tag)
#define	ENGINE_NODE_IS_CHEAP(node)	(((node)->module().klass->mflags & BSE_COST_CHEAP) != 0)
#define	ENGINE_NODE_IS_EXPENSIVE(node)	(((node)->module().klass->mflags & BSE_COST_EXPENSIVE) != 0)
#define	ENGINE_NODE_LOCK(node)		(node)->rec_mutex.lock()
#define	ENGINE_NODE_UNLOCK(node)	(node)->rec_mutex.unlock()
#define	ENGINE_MODULE_IS_VIRTUAL(mod)	(ENGINE_NODE_IS_VIRTUAL (ENGINE_NODE (mod)))


/* --- transactions --- */
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
struct BseJob {
  EngineJobType       job_id;
  BseJob	     *next;
  union {
    struct {
      EngineNode     *node;
      gboolean        free_with_job;
      gchar	     *message;
    } data;
    struct {
      std::mutex              *lock_mutex;
      std::condition_variable *lock_cond;
      gboolean                *lock_p;
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
struct BseTrans {
  BseJob   *jobs_head;
  BseJob   *jobs_tail;
  guint	    comitted : 1;
  BseTrans *cqt_next;	/* com-thread-queue */
};
union EngineTimedJob {
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
struct EngineInput {
  EngineNode *src_node;
  guint	      src_stream;	/* ostream of src_node */
  /* valid if istream[].connected, setup by scheduler */
  EngineNode *real_node;	/* set to NULL if !connected */
  guint	      real_stream;	/* ostream of real_node */
};
struct EngineJInput {
  EngineNode *src_node;
  guint	      src_stream;	/* ostream of src_node */
  /* valid if < jstream[].n_connections, setup by scheduler */
  EngineNode *real_node;
  guint       real_stream;	/* ostream of real_node */
};
struct EngineOutput {
  gfloat *buffer;
  guint	  n_outputs;
};

#endif // __BSE_ENGINE_PRIVATE_H__
