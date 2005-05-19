/* BSE-SCM - Bedevilled Sound Engine Scheme Wrapper
 * Copyright (C) 2002-2003 Tim Janik
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
#ifndef __BSE_SCM_INTERP_H__
#define __BSE_SCM_INTERP_H__

#include <bse/bse.h>
#include <guile/gh.h>

G_BEGIN_DECLS

/* guard around GC-protected code portions,
 * with incremental int-blocking. guile recovers
 * from unbalanced defer/allow pairs.
 */
#define	BSE_SCM_DEFER_INTS()	SCM_REDEFER_INTS
#define	BSE_SCM_ALLOW_INTS()	SCM_REALLOW_INTS

typedef struct _BseSCMWire   BseSCMWire;


/* --- prototypes --- */
void	bse_scm_interp_init		(void);
void	bse_scm_interp_exec_script	(const gchar	*file_name,
					 const gchar	*call_expr,
					 GValue		*value);
void	bse_scm_enable_script_register	(gboolean	 enabled);
void	bse_scm_enable_server		(gboolean	 enabled);


/* --- SCM procedures --- */
SCM	bse_scm_server_get		(void);
SCM	bse_scm_choice_match		(SCM		 s_ev1,
					 SCM		 s_ev2);
SCM	bse_scm_glue_set_prop		(SCM		 s_proxy,
					 SCM		 s_prop_name,
					 SCM		 s_value);
SCM	bse_scm_glue_get_prop		(SCM		 s_proxy,
					 SCM		 s_prop_name);
SCM	bse_scm_glue_call		(SCM		 s_proc_name,
					 SCM		 s_arg_list);
SCM	bse_scm_signal_connect		(SCM		 s_proxy,
					 SCM		 s_signal,
					 SCM		 s_lambda);
SCM     bse_scm_signal_disconnect       (SCM             s_proxy,
                                         SCM             s_handler_id);
SCM     bse_scm_script_message          (SCM             s_type,
                                         SCM             s_bits);
SCM	bse_scm_script_register		(SCM             s_name,
                                         SCM             s_options,
                                         SCM             s_category,
                                         SCM             s_blurb,
                                         SCM             s_author,
                                         SCM             s_license,
                                         SCM             s_params);
SCM	bse_scm_context_pending		(void);
SCM	bse_scm_context_iteration	(SCM		 s_may_block);
SCM	bse_scm_glue_rec_get		(SCM		  scm_rec,
					 SCM		  s_field);
SCM     bse_scm_glue_rec_set            (SCM              scm_rec,
                                         SCM              s_field,
                                         SCM              s_value);
SCM	bse_scm_glue_rec_print		(SCM		  scm_rec);
SCM	bse_scm_make_gc_plateau		(guint		  size_hint);
void	bse_scm_destroy_gc_plateau	(SCM		  s_gcplateau);

G_END_DECLS


#endif /* __BSE_SCM_INTERP_H__ */
