/* BSW-SCM - Bedevilled Sound Engine Scheme Wrapper
 * Copyright (C) 2002 Tim Janik
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
#ifndef __BSW_SCM_INTERP_H__
#define __BSW_SCM_INTERP_H__

#include <bsw/bsw.h>
#include <guile/gh.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* guard around GC-protected code portions,
 * with incremental int-blocking. guile recovers
 * from unbalanced defer/allow pairs.
 */
#define	BSW_SCM_DEFER_INTS()	SCM_REDEFER_INTS
#define	BSW_SCM_ALLOW_INTS()	SCM_REALLOW_INTS

typedef struct _BswSCMWire   BswSCMWire;


/* --- prototypes --- */
void	bsw_scm_interp_init		(BswSCMWire	*wire);
void	bsw_scm_interp_exec_script	(const gchar	*file_name,
					 const gchar	*call_expr,
					 GValue		*value);
void	bsw_scm_enable_script_register	(gboolean	 enabled);
void	bsw_scm_enable_server		(gboolean	 enabled);


/* --- SCM procedures --- */
SCM	bsw_scm_server_get		(void);
SCM	bsw_scm_enum_match		(SCM		 s_ev1,
					 SCM		 s_ev2);
SCM	bsw_scm_glue_set_prop		(SCM		 s_proxy,
					 SCM		 s_prop_name,
					 SCM		 s_value);
SCM	bsw_scm_glue_call		(SCM		 s_proc_name,
					 SCM		 s_arg_list);
SCM	bsw_scm_signal_connect		(SCM		 s_proxy,
					 SCM		 s_signal,
					 SCM		 s_lambda);
SCM	bsw_scm_script_register		(SCM		 name,
					 SCM		 category,
					 SCM		 blurb,
					 SCM		 help,
					 SCM		 author,
					 SCM		 copyright,
					 SCM		 date,
					 SCM		 params);
SCM	bsw_scm_context_pending		(void);
SCM	bsw_scm_context_iteration	(SCM		 s_may_block);


/* --- SCM-Wire --- */
BswSCMWire*     bsw_scm_wire_from_pipe   (const gchar    *ident,
					  gint            remote_input,
					  gint            remote_output);
gchar*          bsw_scm_wire_do_request  (BswSCMWire     *wire,
					  const gchar    *request_msg);
void            bsw_scm_wire_died        (BswSCMWire     *wire);
void		bsw_scm_wire_dispatch_io (BswSCMWire     *swire,
					  guint           timeout);

					 

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSW_SCM_INTERP_H__ */
