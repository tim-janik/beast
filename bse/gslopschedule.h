/* GSL Engine - Flow module operation engine
 * Copyright (C) 2001, 2002 Tim Janik
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
#ifndef __GSL_ENGINE_SCHEDULE_H__
#define __GSL_ENGINE_SCHEDULE_H__

#include <bse/gslopnode.h>

G_BEGIN_DECLS

typedef struct
{
  EngineNode *last;	/* resolve node */
  SfiRing    *nodes;	/* of type EngineNode* */
  guint       seen_deferred_node : 1;
} EngineCycle;
typedef struct
{
  guint    leaf_level;
  SfiRing *cycles;	/* of type Cycle* */
  SfiRing *cycle_nodes;	/* of type EngineNode* */
} EngineQuery;
struct _EngineSchedule
{
  guint     n_items;
  guint     leaf_levels;
  SfiRing **nodes;	/* EngineNode* */
  SfiRing **cycles;	/* SfiRing* */
  guint	    secured : 1;
  guint	    in_pqueue : 1;
  guint	    cur_leaf_level;
  SfiRing  *cur_node;
  SfiRing  *cur_cycle;
  SfiRing  *vnodes;	/* virtual modules */
};
#define	GSL_SCHEDULE_NONPOPABLE(schedule)		((schedule)->cur_leaf_level >= (schedule)->leaf_levels)


/* --- MasterThread --- */
EngineSchedule*	_engine_schedule_new		(void);
void		_engine_schedule_clear		(EngineSchedule	*schedule);
void		_engine_schedule_destroy	(EngineSchedule	*schedule);
void		_engine_schedule_consumer_node	(EngineSchedule	*schedule,
						 EngineNode	*node);
void		_engine_schedule_secure		(EngineSchedule	*schedule);
EngineNode*	_engine_schedule_pop_node	(EngineSchedule	*schedule);
SfiRing*	_engine_schedule_pop_cycle	(EngineSchedule	*schedule);
void		_engine_schedule_restart	(EngineSchedule	*schedule);
void		_engine_schedule_unsecure	(EngineSchedule	*schedule);

G_END_DECLS

#endif /* __GSL_ENGINE_SCHEDULE_H__ */
