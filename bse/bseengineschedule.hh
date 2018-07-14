// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_ENGINE_SCHEDULE_H__
#define __BSE_ENGINE_SCHEDULE_H__

#include <bse/bseengineprivate.hh>

struct EngineCycle {
  Bse::Module *last;	/* resolve node */
  SfiRing    *nodes;	/* of type Bse::Module* */
  guint       seen_deferred_node : 1;
};
struct EngineQuery {
  guint    leaf_level;
  SfiRing *cycles;	/* of type Cycle* */
  SfiRing *cycle_nodes;	/* of type Bse::Module* */
};
struct EngineSchedule {
  guint     n_items;
  guint     leaf_levels;
  SfiRing **nodes;	/* Bse::Module* */
  SfiRing **cycles;	/* SfiRing* */
  guint	    secured : 1;
  guint	    in_pqueue : 1;
  guint	    cur_leaf_level;
  SfiRing  *cur_node;
  SfiRing  *cur_cycle;
  SfiRing  *vnodes;	/* virtual modules */
};
#define	BSE_ENGINE_SCHEDULE_NONPOPABLE(schedule)        ((schedule)->cur_leaf_level >= (schedule)->leaf_levels)


/* --- MasterThread --- */
EngineSchedule*	_engine_schedule_new		(void);
void		_engine_schedule_clear		(EngineSchedule	*schedule);
void		_engine_schedule_destroy	(EngineSchedule	*schedule);
void		_engine_schedule_consumer_node	(EngineSchedule	*schedule,
						 Bse::Module	*node);
void		_engine_schedule_secure		(EngineSchedule	*schedule);
Bse::Module*	_engine_schedule_pop_node	(EngineSchedule	*schedule);
SfiRing*	_engine_schedule_pop_cycle	(EngineSchedule	*schedule);
void		_engine_schedule_restart	(EngineSchedule	*schedule);
void		_engine_schedule_unsecure	(EngineSchedule	*schedule);

#endif /* __BSE_ENGINE_SCHEDULE_H__ */
