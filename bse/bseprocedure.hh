// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_PROCEDURE_H__
#define __BSE_PROCEDURE_H__

#include	<bse/bseparam.hh>

G_BEGIN_DECLS

/* --- BSE type macros --- */
#define	BSE_PROCEDURE_TYPE(proc)	(G_TYPE_FROM_CLASS (proc))
#define	BSE_IS_PROCEDURE_CLASS(proc)	(G_TYPE_CHECK_CLASS_TYPE ((proc), BSE_TYPE_PROCEDURE))
#define	BSE_PROCEDURE_NAME(proc)	(g_type_name (BSE_PROCEDURE_TYPE (proc)))


/* --- limits --- */
#define	BSE_PROCEDURE_MAX_IN_PARAMS	(16)
#define	BSE_PROCEDURE_MAX_OUT_PARAMS	(16)


/* --- BseProcedureClass --- */
typedef void          (*BseProcedureInit)   (BseProcedureClass *proc,
                                             GParamSpec       **in_pspecs,
                                             GParamSpec       **out_pspecs);
typedef BseErrorType  (*BseProcedureExec)   (BseProcedureClass *procedure,
                                             const GValue      *in_values,
                                             GValue	       *out_values);
struct _BseProcedureClass
{
  GTypeClass      bse_class;

  /* implementation hint */
  guint           private_id;

  /* in/out parameters */
  guint           n_in_pspecs;
  GParamSpec	**in_pspecs;
  guint           n_out_pspecs;
  GParamSpec	**out_pspecs;
  /* keep type references during class lifetime */
  GTypeClass    **class_refs;
  guint           cache_stamp;
  gpointer        cache_next;
  
  BseProcedureExec execute;
};


/* --- notifiers --- */
typedef gboolean (*BseProcedureNotify) (gpointer     func_data,
					const gchar *proc_name,
					BseErrorType exit_status);
typedef BseErrorType (*BseProcedureMarshal) (gpointer		marshal_data,
					     BseProcedureClass *proc,
					     const GValue      *ivalues,
					     GValue	       *ovalues);


/* --- prototypes --- */
/* execute procedure, passing n_in_pspecs param values for in
 * values and n_out_pspecs param value locations for out values
 */
BseErrorType bse_procedure_exec	  	  (const gchar		*proc_name,
					   ...);
BseErrorType bse_procedure_exec_void  	  (const gchar		*proc_name,
					   ...);
GType	     bse_procedure_lookup	  (const gchar		*proc_name);
BseErrorType bse_procedure_marshal_valist (GType		 proc_type,
					   const GValue		*first_value,
					   BseProcedureMarshal	 marshal,
					   gpointer		 marshal_data,
					   gboolean		 skip_ovalues,
					   va_list		 var_args);
BseErrorType bse_procedure_marshal        (GType		 proc_type,
					   const GValue		*ivalues,
					   GValue		*ovalues,
					   BseProcedureMarshal	 marshal,
					   gpointer		 marshal_data);
BseErrorType bse_procedure_collect_input_args (BseProcedureClass  *proc,
                                               const GValue       *first_value,
                                               va_list             var_args,
                                               GValue              ivalues[BSE_PROCEDURE_MAX_IN_PARAMS]);
BseErrorType bse_procedure_execvl	  (BseProcedureClass	*proc,
					   GSList		*in_value_list,
					   GSList		*out_value_list,
					   BseProcedureMarshal	 marshal,
					   gpointer		 marshal_data);


/* --- internal --- */
const gchar* bse_procedure_type_register (const gchar		*name,
					  BsePlugin		*plugin,
					  GType  		*ret_type);

G_END_DECLS

#endif /* __BSE_PROCEDURE_H__ */
