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
#ifndef __GSL_ENGINE_SCHEDULE_H__
#define __GSL_ENGINE_SCHEDULE_H__

#include <gsl/gslopnode.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef struct
{
  EngineNode *last;	/* resolve node */
  GslRing    *nodes;	/* of type EngineNode* */
  guint       seen_deferred_node : 1;
} EngineCycle;
typedef struct
{
  guint    leaf_level;
  GslRing *cycles;	/* of type Cycle* */
  GslRing *cycle_nodes;	/* of type EngineNode* */
} EngineQuery;
struct _EngineSchedule
{
  guint     n_items;
  guint     leaf_levels;
  GslRing **nodes;	/* EngineNode* */
  GslRing **cycles;	/* GslRing* */
  guint	    secured : 1;
  guint	    in_pqueue : 1;
  guint	    cur_leaf_level;
  GslRing  *cur_node;
  GslRing  *cur_cycle;
};
#define	GSL_SCHEDULE_NONPOPABLE(schedule)		((schedule)->cur_leaf_level >= (schedule)->leaf_levels)


/* --- MasterThread --- */
EngineSchedule*	_engine_schedule_new		(void);
void		_engine_schedule_clear		(EngineSchedule	*schedule);
void		_engine_schedule_destroy	(EngineSchedule	*schedule);
void		_engine_schedule_consumer_node	(EngineSchedule	*schedule,
						 EngineNode	*node);
void		_engine_schedule_node		(EngineSchedule	*schedule,
						 EngineNode	*node,
						 guint		 leaf_level);
void		_engine_schedule_cycle		(EngineSchedule	*schedule,
						 GslRing	*cycle_nodes,
						 guint		 leaf_level);
void		_engine_schedule_secure		(EngineSchedule	*schedule);
EngineNode*	_engine_schedule_pop_node	(EngineSchedule	*schedule);
GslRing*	_engine_schedule_pop_cycle	(EngineSchedule	*schedule);
void		_engine_schedule_restart	(EngineSchedule	*schedule);
void		_engine_schedule_unsecure	(EngineSchedule	*schedule);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_ENGINE_SCHEDULE_H__ */
