// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_ENGINE_NODE_H__
#define __BSE_ENGINE_NODE_H__

#include <bse/bsedefs.hh>
#include "gslcommon.hh"

// == Typedefs ==
struct EngineSchedule;

namespace Bse {

struct EngineInput;
struct EngineJInput;
struct EngineOutput;
union  EngineTimedJob;

/// DSP Engine Module
class Module { 		  // fields sorted by order of processing access
  std::recursive_mutex   rec_mutex_; // processing lock
public:
  explicit               Module (const BseModuleClass &klass);
  virtual               ~Module ();
  inline void            lock   ()      { rec_mutex_.lock(); }
  inline void            unlock ()      { rec_mutex_.unlock(); }
  const BseModuleClass  &klass;
  gpointer               user_data = NULL;
  BseIStream            *istreams = NULL;	// input streams
  BseJStream            *jstreams = NULL;     	// joint (multiconnect) input streams
  BseOStream            *ostreams = NULL;     	// output streams
  guint64                counter = 0;     // <= Bse::TickStamp::current() */
  EngineInput	        *inputs = NULL;	  // [BSE_MODULE_N_ISTREAMS()] */
  EngineJInput         **jinputs = NULL;  // [BSE_MODULE_N_JSTREAMS()][jstream->jcount] */
  EngineOutput	        *outputs = NULL;  // [BSE_MODULE_N_OSTREAMS()] */
  // timed jobs
  EngineTimedJob        *flow_jobs = NULL;			// active jobs
  EngineTimedJob        *probe_jobs = NULL;		        // probe requests // FIXME: remove?
  EngineTimedJob        *boundary_jobs = NULL;		        // active jobs
  EngineTimedJob        *tjob_head = NULL, *tjob_tail = NULL;	// trash list
  // suspend/activation time
  guint64                next_active = 0;                       // result of suspend state updates
  // master-node-list
  Module                *mnl_next = NULL;
  Module	        *mnl_prev = NULL;
  guint		         integrated : 1;
  guint		         virtual_node : 1;
  guint		         is_consumer : 1;
  // suspension
  guint		         update_suspend : 1;    	// whether suspend state needs updating
  guint		         in_suspend_call : 1;   	// recursion barrier during suspend state updates
  guint		         needs_reset : 1;	        // flagged at resumption
  // scheduler
  guint		         cleared_ostreams : 1;	        // whether ostream[].connected was cleared already
  guint		         sched_tag : 1;	        	// whether this node is contained in the schedule
  guint		         sched_recurse_tag : 1; 	// recursion flag used during scheduling
  guint                  sched_leaf_level = 0;
  guint64                local_active = 0;              // local suspend state stamp
  Module	        *toplevel_next = NULL;	        // master-consumer-list, FIXME: overkill, using a SfiRing is good enough
  SfiRing	        *output_nodes = NULL;	        // EngineNode* ring of nodes in ->outputs[]
};
} // Bse

#endif // __BSE_ENGINE_NODE_H__
