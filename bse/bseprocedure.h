/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
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
 *
 * bseprocedure.h: dynamic procedure implementation
 */
#ifndef __BSE_PROCEDURE_H__
#define __BSE_PROCEDURE_H__

#include	<bse/bseparam.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- BSE type macros --- */
#define	BSE_PROCEDURE_TYPE(proc)	(BSE_CLASS_TYPE (proc))
#define	BSE_IS_PROCEDURE_CLASS(proc)	(BSE_CHECK_CLASS_TYPE ((proc), BSE_TYPE_PROCEDURE))


/* --- limits --- */
#define	BSE_PROCEDURE_MAX_IN_PARAMS	(16)
#define	BSE_PROCEDURE_MAX_OUT_PARAMS	(16)


/* --- BseProcedureClass --- */
typedef BseErrorType  (*BseProcedureExec)    (BseProcedureClass *procedure,
					      BseParam          *iparams,
					      BseParam          *oparams);
struct _BseProcedureClass
{
  BseTypeClass    bse_class;
  gchar          *name;
  gchar          *blurb;

  /* setup upon init */
  gchar          *help;
  gchar          *author;
  gchar          *copyright;
  gchar          *date; /* copyright date */
  
  /* implementation */
  guint           n_in_params;
  BseParamSpec	**in_param_specs;
  guint           n_out_params;
  BseParamSpec  **out_param_specs;

  BseProcedureExec execute;
};


/* --- notifiers --- */
typedef gboolean (*BseProcedureNotify) (gpointer     func_data,
					const gchar *proc_name,
					BseErrorType exit_status);


/* --- prototypes --- */
BseProcedureClass* bse_procedure_find_ref (const gchar		*name);
void		   bse_procedure_ref	  (BseProcedureClass	*proc);
void		   bse_procedure_unref	  (BseProcedureClass	*proc);
/* execute procedure, passing n_in_params param values for in
 * values and n_out_params param value locations for out values
 */
BseErrorType	bse_procedure_exec	  (const gchar		*name,
					   ...);
BseErrorType	bse_procedure_void_exec	  (const gchar		*name,
					   ...);
BseErrorType	bse_procedure_execvl	  (BseProcedureClass	*proc,
					   GSList		*iparam_list,
					   GSList		*oparam_list);
/* functions to call from very time consuming procedures to keep the
 * main program (and playback) alive.
 * "progress"    - value in the range from 0...1 to indicate how far
 *                 the procedure has proceeded yet (*100 = %)
 * return value  - if the return value is TRUE, the procedure is requested
 *                 to abort, and should return BSE_ERROR_PROC_ABORT
 * (a procedure should not intermix calls to both functions during
 *  its execution).
 */
gboolean bse_procedure_share		(BseProcedureClass	*proc);
gboolean bse_procedure_update		(BseProcedureClass	*proc,
					 gfloat			 progress);
void	 bse_procedure_push_share_hook	(BseProcedureShare 	 share_func,
					 gpointer	   	 func_data);
void	 bse_procedure_pop_share_hook	(void);
/* procedure notifiers, executed after a procedure finished execution */
guint	bse_procedure_notifier_add	(BseProcedureNotify	 notifier,
					 gpointer		 func_data);
void	bse_procedure_notifier_remove	(guint			 notifier_id);


/* --- internal --- */
BseErrorType bse_procedure_execva_item	(BseProcedureClass	*proc,
					 BseItem		*item,
					 va_list		 var_args,
					 gboolean		 skip_oparams);
const gchar* bse_procedure_type_register (const gchar		*name,
					  const gchar		*blurb,
					  BsePlugin		*plugin,
					  BseType		*ret_type);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_PROCEDURE_H__ */
