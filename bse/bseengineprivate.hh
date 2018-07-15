// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_ENGINE_PRIVATE_H__
#define __BSE_ENGINE_PRIVATE_H__

#include "bseengine.hh"
#include "gslcommon.hh"

#define	BSE_MODULE_IS_SUSPENDED(nod,s)  ((s) < (nod)->next_active)
#define	BSE_MODULE_IS_DEFERRED(module)	(false)
#define	BSE_MODULE_IS_CONSUMER(module)	((module)->is_consumer && (module)->output_nodes == NULL)
#define	BSE_MODULE_IS_SCHEDULED(module)	((module)->sched_tag)
#define	BSE_MODULE_IS_VIRTUAL(module)   (0 != (size_t ((module)->klass.mflags) & size_t (Bse::ModuleFlag::VIRTUAL_)))

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

namespace Bse {

struct Job {
  EngineJobType job_id;
  Job          *next;
  union {
    struct {
      Module    *node;
      bool       free_with_job;
      char	*message;
    } data;
    struct {
      std::mutex              *lock_mutex;
      std::condition_variable *lock_cond;
      bool                    *lock_p;
    } sync;
    struct {
      Module    *node;
      uint64     stamp;
    } tick;
    struct {
      Module     *dest_node;
      uint        dest_ijstream;
      Module     *src_node;
      uint        src_ostream;
    } connection;
    struct {
      Module               *node;
      BseEngineAccessFunc   access_func;
      gpointer	            data;
      BseFreeFunc           free_func;
    } access;
    struct {
      BseEnginePollFunc poll_func;
      gpointer	        data;
      BseFreeFunc       free_func;
      uint              n_fds;
      GPollFD	       *fds;
    } poll;
    struct {
      BseEngineTimerFunc timer_func;
      gpointer	         data;
      BseFreeFunc        free_func;
    } timer;
    struct {
      Module         *node;
      EngineTimedJob *tjob;
    } timed_job;
  };
};
struct Trans {
  Job    *jobs_head;
  Job    *jobs_tail;
  uint	  comitted : 1;
  Trans  *cqt_next;	/* com-thread-queue */
};
union EngineTimedJob {
  struct {
    EngineJobType       type;           /* common */
    EngineTimedJob     *next;           /* common */
    uint64              tick_stamp;     /* common */
  };
  struct {
    EngineJobType       type;           /* common */
    EngineTimedJob     *next;           /* common */
    uint64              tick_stamp;     /* common */
    gpointer            data;
    BseEngineProbeFunc  probe_func;
    OStream            *ostreams;
    uint                n_ostreams;
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
  Module *src_node;
  uint    src_stream;	/* ostream of src_node */
  /* valid if istream[].connected, setup by scheduler */
  Module *real_node;	/* set to NULL if !connected */
  uint    real_stream;	/* ostream of real_node */
};
struct EngineJInput {
  Module *src_node;
  uint    src_stream;	/* ostream of src_node */
  /* valid if < jstream[].n_connections, setup by scheduler */
  Module *real_node;
  uint    real_stream;	/* ostream of real_node */
};
struct EngineOutput {
  float *buffer;
  uint	 n_outputs;
};

} // Bse

#endif // __BSE_ENGINE_PRIVATE_H__
