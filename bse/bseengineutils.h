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
#ifndef __BSE_ENGINE_UTIL_H__
#define __BSE_ENGINE_UTIL_H__

#include <bse/bseenginenode.h>

G_BEGIN_DECLS

/* --- UserThread --- */
void            bse_engine_reinit_utils         (void);
void		_engine_free_trans		(BseTrans      *trans);
BseOStream*	_engine_alloc_ostreams		(guint		n);
#if 0	/* bseengine.h: */
void            bse_engine_garbage_collect	(void);
gfloat*		bse_engine_const_values		(gfloat		value);
#endif


/* --- MasterThread --- */
void		_engine_recycle_const_values	(gboolean        nuke_all);
void		_engine_node_collect_jobs	(EngineNode	*node);
/* master node list */
void		_engine_mnl_remove		(EngineNode	*node);
void		_engine_mnl_node_changed	(EngineNode	*node);
void		_engine_mnl_integrate		(EngineNode	*node);
#define BSE_ENGINE_MNL_UNSCHEDULED_TJOB_NODE(node)	(!(node)->sched_tag && ((node)->flow_jobs || (node)->probe_jobs))
EngineNode*	_engine_mnl_head		(void);

/* communication routines for threads:
 * UserThread   - main application
 * MasterThread - op engine control thread
 * SlaveThread  - op engine calculation threads
 *
 * these functions are for _internal_ use of bseengine*.c implementations
 */

/* --- job transactions --- */
/* UserThread */
guint64         _engine_enqueue_trans	(BseTrans	*trans);
BseTrans*	_engine_collect_trans	(void);
void		_engine_wait_on_trans	(void);
/* MasterThread */
BseJob*		_engine_pop_job		(gboolean update_commit_stamp);
gboolean	_engine_job_pending	(void);


/* --- node processing queue --- */
void	    _engine_set_schedule		(EngineSchedule	*schedule);
void	    _engine_unset_schedule		(EngineSchedule	*schedule);
EngineNode* _engine_pop_unprocessed_node	(void);
void	    _engine_push_processed_node		(EngineNode	*node);
SfiRing*    _engine_pop_unprocessed_cycle	(void);
void	    _engine_push_processed_cycle	(SfiRing	*cycle);
void	    _engine_wait_on_unprocessed		(void);

G_END_DECLS

#endif /* __BSE_ENGINE_UTIL_H__ */
