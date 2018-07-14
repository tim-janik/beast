// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_ENGINE_NODE_H__
#define __BSE_ENGINE_NODE_H__

#include <bse/bsedefs.hh>
#include "gslcommon.hh"

// == Typedefs ==
struct EngineInput;
struct EngineJInput;
struct EngineOutput;
struct EngineSchedule;
union  EngineTimedJob;
struct EngineNode {		  // fields sorted by order of processing access
  virtual ~EngineNode() {}
  std::recursive_mutex rec_mutex; // processing lock
  guint64	 counter = 0;     // <= Bse::TickStamp::current() */
  EngineInput	*inputs = NULL;	  // [ENGINE_NODE_N_ISTREAMS()] */
  EngineJInput **jinputs = NULL;  // [ENGINE_NODE_N_JSTREAMS()][jstream->jcount] */
  EngineOutput	*outputs = NULL;  // [ENGINE_NODE_N_OSTREAMS()] */
  // timed jobs
  EngineTimedJob *flow_jobs = NULL;			// active jobs */
  EngineTimedJob *probe_jobs = NULL;		        // probe requests */
  EngineTimedJob *boundary_jobs = NULL;		        // active jobs */
  EngineTimedJob *tjob_head = NULL, *tjob_tail = NULL;	// trash list */
  // suspend/activation time
  guint64        next_active = 0;                       // result of suspend state updates
  // master-node-list
  EngineNode	*mnl_next = NULL;
  EngineNode	*mnl_prev = NULL;
  guint		 integrated : 1;
  guint		 virtual_node : 1;
  guint		 is_consumer : 1;
  // suspension
  guint		 update_suspend : 1;    	// whether suspend state needs updating
  guint		 in_suspend_call : 1;   	// recursion barrier during suspend state updates
  guint		 needs_reset : 1;	        // flagged at resumption
  // scheduler
  guint		 cleared_ostreams : 1;	        // whether ostream[].connected was cleared already
  guint		 sched_tag : 1;	        	// whether this node is contained in the schedule
  guint		 sched_recurse_tag : 1; 	// recursion flag used during scheduling
  guint		 sched_leaf_level = 0;
  guint64        local_active = 0;              // local suspend state stamp
  EngineNode	*toplevel_next = NULL;	        // master-consumer-list, FIXME: overkill, using a SfiRing is good enough
  SfiRing	*output_nodes = NULL;	        // EngineNode* ring of nodes in ->outputs[]
  EngineNode() :
    integrated (0), virtual_node (0), is_consumer (0), update_suspend (0), in_suspend_call (0), needs_reset (0),
    cleared_ostreams (0), sched_tag (0), sched_recurse_tag (0)
  {}
  BseModule&     module()       { return *(BseModule*) this; }
};

#endif // __BSE_ENGINE_NODE_H__
