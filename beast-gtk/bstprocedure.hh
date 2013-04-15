// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_PROCEDURE_H__
#define __BST_PROCEDURE_H__

#include	"bstparamview.hh"

G_BEGIN_DECLS

/* --- Gtk+ type macros --- */
#define BST_TYPE_PROCEDURE_SHELL	    (bst_procedure_shell_get_type ())
#define BST_PROCEDURE_SHELL(object)	    (GTK_CHECK_CAST ((object), BST_TYPE_PROCEDURE_SHELL, BstProcedureShell))
#define BST_PROCEDURE_SHELL_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_PROCEDURE_SHELL, BstProcedureShellClass))
#define BST_IS_PROCEDURE_SHELL(object)	    (GTK_CHECK_TYPE ((object), BST_TYPE_PROCEDURE_SHELL))
#define BST_IS_PROCEDURE_SHELL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_PROCEDURE_SHELL))
#define BST_PROCEDURE_SHELL_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), BST_TYPE_PROCEDURE_SHELL, BstProcedureShellClass))


/* --- structures & typedefs --- */
typedef struct	_BstProcedureShell	       BstProcedureShell;
typedef struct	_BstProcedureShellClass	       BstProcedureShellClass;
struct _BstProcedureShell
{
  GtkVBox	     parent_object;

  SfiGlueProc       *proc;
  SfiRec	    *prec;

  guint		     n_preset_params;
  SfiRing	    *params; /* n_in_params + n_out_params params */

  guint		     in_modal_selection : 1;
  guint		     in_execution : 1;
  guint		     hide_dialog_on_exec : 1;
};
struct _BstProcedureShellClass
{
  GtkVBoxClass	     parent_class;
};


/* --- prototypes --- */
GtkType		   bst_procedure_shell_get_type	   (void);
GtkWidget*	   bst_procedure_shell_new	   (SfiGlueProc       *proc);
void		   bst_procedure_shell_update	   (BstProcedureShell *procedure_shell);
void		   bst_procedure_shell_rebuild	   (BstProcedureShell *procedure_shell);
void		   bst_procedure_shell_execute	   (BstProcedureShell *procedure_shell);
void		   bst_procedure_shell_set_proc	   (BstProcedureShell *procedure_shell,
						    SfiGlueProc       *proc);
void		   bst_procedure_shell_reset	   (BstProcedureShell *procedure_shell);
void		   bst_procedure_shell_unpreset	   (BstProcedureShell *procedure_shell);
gboolean	   bst_procedure_shell_preset	   (BstProcedureShell *procedure_shell,
						    const gchar       *name,
						    const GValue      *value,
						    gboolean	       lock_preset);


/* --- convenience --- */
BstProcedureShell* bst_procedure_shell_global	(void);
void		   bst_procedure_exec		(const gchar	*procedure_name,
						 const gchar	*preset_param,
						 ...) G_GNUC_NULL_TERMINATED;
void		   bst_procedure_exec_auto	(const gchar    *procedure_name,
						 const gchar	*preset_param,
						 ...) G_GNUC_NULL_TERMINATED;
void		   bst_procedure_exec_modal	(const gchar    *procedure_name,
						 const gchar	*preset_param,
						 ...) G_GNUC_NULL_TERMINATED;
GParamSpec*        bst_procedure_ref_pspec      (const gchar    *procedure_name,
                                                 const gchar    *parameter);

G_END_DECLS

#endif /* __BST_PROCEDURE_H__ */
