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
#ifndef __GSL_OP_UTIL_H__
#define __GSL_OP_UTIL_H__

#include <gsl/gsldefs.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- typedefs --- */
typedef struct _OpNode     OpNode;
typedef struct _OpSchedule OpSchedule;


/* --- UserThread --- */
GslTrans*       _op_alloc_trans		(void);
void            _op_free_trans		(GslTrans        *trans);
void            _op_collect_trans	(void);
GslOStream*	_op_alloc_ostreams	(guint		 n);


/* --- MasterThread --- */
void	       _gsl_recycle_const_values (void);

/* --- public utils --- */
gfloat*		gsl_engine_const_values	(gfloat		value);


/* communication routines for threads:
 * UserThread   - main application
 * MasterThread - op engine control thread
 * SlaveThread  - op engine calculation threads
 *
 * these functions are for _internal_ use of gslop*.c implementations
 */

/* --- job transactions --- */
/* UserThread */
void		op_com_enqueue_trans	(GslTrans	*trans);
GslTrans*	op_com_collect_trans	(void);
void		op_com_wait_on_trans	(void);
/* MasterThread */
/* GslJob*	op_com_pop_job_timed	(glong		 max_useconds); */
GslJob*		gsl_com_pop_job		(void);
gboolean	op_com_job_pending	(void);


/* --- node processing queue --- */
void	 _gsl_com_set_schedule		(OpSchedule	*schedule);
void	 _gsl_com_unset_schedule	(OpSchedule	*schedule);
OpNode*  _gsl_com_pop_unprocessed_node	(void);
void	 _gsl_com_push_processed_node	(OpNode		*node);
GslRing* _gsl_com_pop_unprocessed_cycle	(void);
void	 _gsl_com_push_processed_cycle	(GslRing	*cycle);
void	 _gsl_com_wait_on_unprocessed	(void);


/* --- thread wakeups --- */
void	_gsl_com_add_master_wakeup	(GslTrans	*trans);
void	_gsl_com_remove_master_wakeup	(GslTrans	*trans);
void	_gsl_com_fire_master_wakeup	(void);
void	_gsl_com_discard_master_wakeups	(void);
gint	_gsl_com_get_user_wakeup	(void);
void	_gsl_com_fire_user_wakeup	(void);
void	_gsl_com_discard_user_wakeups	(void);

       

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_OP_UTIL_H__ */
