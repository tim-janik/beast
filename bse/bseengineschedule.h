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
#ifndef __GSL_OP_SCHEDULE_H__
#define __GSL_OP_SCHEDULE_H__

#include <gsl/gslopnode.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef struct
{
  OpNode  *last;	/* resolve node */
  GslRing *nodes;	/* of type OpNode* */
  guint    seen_deferred_node : 1;
} OpCycle;
typedef struct
{
  guint    leaf_level;
  GslRing *cycles;	/* of type Cycle* */
  GslRing *cycle_nodes;	/* of type OpNode* */
} OpQuery;
struct _OpSchedule
{
  guint     n_items;
  guint     leaf_levels;
  GslRing **nodes;	/* OpNode* */
  GslRing **cycles;	/* GslRing* */
  guint	    secured : 1;
  guint	    in_pqueue : 1;
  guint	    cur_leaf_level;
  GslRing  *cur_node;
  GslRing  *cur_cycle;
};
#define	GSL_SCHEDULE_NONPOPABLE(schedule)		((schedule)->cur_leaf_level >= (schedule)->leaf_levels)


/* --- MasterThread --- */
OpSchedule*	_op_schedule_new		(void);
void		_op_schedule_clear		(OpSchedule	*schedule);
void		_op_schedule_destroy		(OpSchedule	*schedule);
void		_op_schedule_consumer_node	(OpSchedule	*schedule,
						 OpNode		*node);
void		_op_schedule_node		(OpSchedule	*schedule,
						 OpNode		*node,
						 guint		 leaf_level);
void		_op_schedule_cycle		(OpSchedule	*schedule,
						 GslRing	*cycle_nodes,
						 guint		 leaf_level);
void		_op_schedule_secure		(OpSchedule	*schedule);
OpNode*		_op_schedule_pop_node		(OpSchedule	*schedule);
GslRing*	_op_schedule_pop_cycle		(OpSchedule	*schedule);
void		_op_schedule_restart		(OpSchedule	*schedule);
void		_op_schedule_unsecure		(OpSchedule	*schedule);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_OP_SCHEDULE_H__ */
