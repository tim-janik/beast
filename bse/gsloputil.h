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
#ifndef __GSL_ENGINE_UTIL_H__
#define __GSL_ENGINE_UTIL_H__

#include <gsl/gsldefs.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- typedefs --- */
typedef struct _EngineNode     EngineNode;
typedef struct _EngineSchedule EngineSchedule;


/* --- UserThread --- */
void		_engine_free_trans		(GslTrans      *trans);
GslOStream*	_engine_alloc_ostreams		(guint		n);
#if 0	/* gslengine.h: */
void            gsl_engine_garbage_collect	(void);
gfloat*		gsl_engine_const_values		(gfloat		value);
#endif


/* --- MasterThread --- */
void		_engine_recycle_const_values	(void);
void		_engine_node_collect_flow_jobs	(EngineNode	*node);
/* master node list */
void		_engine_mnl_remove		(EngineNode	*node);
void		_engine_mnl_reorder		(EngineNode	*node);
void		_engine_mnl_integrate		(EngineNode	*node);
#define	GSL_MNL_UNSCHEDULED_FLOW_NODE(node)	((node)->flow_jobs && !(node)->sched_tag)
EngineNode*	_engine_mnl_head		(void);

/* communication routines for threads:
 * UserThread   - main application
 * MasterThread - op engine control thread
 * SlaveThread  - op engine calculation threads
 *
 * these functions are for _internal_ use of gslop*.c implementations
 */

/* --- job transactions --- */
/* UserThread */
void		_engine_enqueue_trans	(GslTrans	*trans);
GslTrans*	_engine_collect_trans	(void);
void		_engine_wait_on_trans	(void);
/* MasterThread */
/* GslJob*	_engine_pop_job_timed	(glong		 max_useconds); */
GslJob*		_engine_pop_job		(void);
gboolean	_engine_job_pending	(void);


/* --- node processing queue --- */
void	    _engine_set_schedule		(EngineSchedule	*schedule);
void	    _engine_unset_schedule		(EngineSchedule	*schedule);
EngineNode* _engine_pop_unprocessed_node	(void);
void	    _engine_push_processed_node		(EngineNode	*node);
SfiRing*    _engine_pop_unprocessed_cycle	(void);
void	    _engine_push_processed_cycle	(SfiRing	*cycle);
void	    _engine_wait_on_unprocessed		(void);

       

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_ENGINE_UTIL_H__ */
