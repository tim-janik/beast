/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999, 2000 Olaf Hoehmann and Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include        "bstprocedure.h"

#include        "bststatusbar.h"
#include        <string.h>


/* --- prototypes --- */
static void     bst_procedure_shell_class_init (BstProcedureShellClass *klass);
static void     bst_procedure_shell_init       (BstProcedureShell      *pe);
static void     bst_procedure_shell_destroy    (GtkObject              *object);


/* --- static variables --- */
static GtkWidget *global_proc_dialog = NULL;
static gpointer   parent_class = NULL;
static GQuark     quark_input_params = 0;
static GQuark     quark_output_params = 0;


/* --- functions --- */
GtkType
bst_procedure_shell_get_type (void)
{
  static GtkType procedure_shell_type = 0;
  
  if (!procedure_shell_type)
    {
      GtkTypeInfo procedure_shell_info =
      {
        "BstProcedureShell",
        sizeof (BstProcedureShell),
        sizeof (BstProcedureShellClass),
        (GtkClassInitFunc) bst_procedure_shell_class_init,
        (GtkObjectInitFunc) bst_procedure_shell_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };
      
      procedure_shell_type = gtk_type_unique (GTK_TYPE_VBOX, &procedure_shell_info);
    }
  
  return procedure_shell_type;
}

static void
bst_procedure_shell_class_init (BstProcedureShellClass *class)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  
  quark_output_params = g_quark_from_static_string ("Output Parameters");
  quark_input_params = g_quark_from_static_string ("Input Parameters");
  
  parent_class = gtk_type_class (GTK_TYPE_VBOX);
  
  object_class->destroy = bst_procedure_shell_destroy;
}

static void
bst_procedure_shell_init (BstProcedureShell *procedure_shell)
{
  procedure_shell->proc = NULL;
  procedure_shell->n_in_params = 0;
  procedure_shell->n_out_params = 0;
  procedure_shell->n_preset_params = 0;
  procedure_shell->bparams = NULL;
  procedure_shell->first_out_bparam = NULL;
  procedure_shell->in_modal_selection = FALSE;
  procedure_shell->in_execution = FALSE;
  procedure_shell->hide_dialog_on_exec = FALSE;
}

static void
bst_procedure_shell_destroy_contents (BstProcedureShell *procedure_shell)
{
  gtk_container_foreach (GTK_CONTAINER (procedure_shell), (GtkCallback) gtk_widget_destroy, NULL);
  g_slist_free (procedure_shell->bparams);
  procedure_shell->n_in_params = 0;
  procedure_shell->n_out_params = 0;
  procedure_shell->n_preset_params = 0;
  procedure_shell->bparams = NULL;
  procedure_shell->first_out_bparam = NULL;
  procedure_shell->in_modal_selection = FALSE;
}

static void
bst_procedure_shell_destroy (GtkObject *object)
{
  BstProcedureShell *shell;
  
  g_return_if_fail (object != NULL);
  
  shell = BST_PROCEDURE_SHELL (object);
  
  if (shell->in_execution)
    g_warning (G_STRLOC ": destroying procedure shell during execution");
  
  bst_procedure_shell_set_proc (shell, NULL);
  
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget*
bst_procedure_shell_new (BseProcedureClass *proc)
{
  GtkWidget *procedure_shell;
  
  procedure_shell = gtk_widget_new (BST_TYPE_PROCEDURE_SHELL, NULL);
  bst_procedure_shell_set_proc (BST_PROCEDURE_SHELL (procedure_shell), proc);
  
  return procedure_shell;
}

void
bst_procedure_shell_set_proc (BstProcedureShell *procedure_shell,
			      BseProcedureClass *proc)
{
  g_return_if_fail (BST_IS_PROCEDURE_SHELL (procedure_shell));
  if (proc)
    g_return_if_fail (BSE_IS_PROCEDURE_CLASS (proc));
  
  if (proc != procedure_shell->proc)
    {
      bst_procedure_shell_destroy_contents (procedure_shell);
      if (procedure_shell->proc)
	bse_procedure_unref (procedure_shell->proc);
      procedure_shell->proc = proc;
      if (procedure_shell->proc)
	bse_procedure_ref (procedure_shell->proc);
      
      bst_procedure_shell_rebuild (procedure_shell);
    }
}

void
bst_procedure_shell_rebuild (BstProcedureShell *shell)
{
  BseProcedureClass *proc;
  GtkWidget *param_box;
  GSList *slist, *pspec_array_list = NULL;
  gchar *string;
  guint is_out_param = 0;
  
  g_return_if_fail (BST_IS_PROCEDURE_SHELL (shell));
  
  bst_procedure_shell_destroy_contents (shell);
  
  if (!shell->proc)
    return;
  
  proc = shell->proc;
  
  /* main container
   */
  param_box = GTK_WIDGET (shell);
  gtk_widget_set (param_box,
		  "homogeneous", FALSE,
		  "spacing", 0,
		  "border_width", 5,
		  NULL);
  
  /* put procedure title
   */
  string = strchr (proc->name, ':');
  if (string && string[1] == ':')
    string +=2;
  else
    string = proc->name;
  gtk_box_pack_start (GTK_BOX (param_box),
                      gtk_widget_new (GTK_TYPE_LABEL,
                                      "visible", TRUE,
                                      "label", string,
                                      NULL),
                      FALSE,
                      TRUE,
                      0);
  
  /* put description
   */
  if (proc->blurb)
    {
      GtkWidget *hbox, *text = bst_wrap_text_create (proc->blurb, TRUE, NULL);
      
      hbox = gtk_widget_new (GTK_TYPE_HBOX,
                             "visible", TRUE,
                             NULL);
      gtk_box_pack_start (GTK_BOX (hbox), text, TRUE, TRUE, 5);
      gtk_widget_new (GTK_TYPE_FRAME,
                      "visible", TRUE,
                      "label", "Description",
                      "label_xalign", 0.0,
                      "width_request", 1,
                      "height_request", 50,
                      "child", hbox,
                      "parent", param_box,
                      NULL);
    }
  
  /* parameter fields
   */
  pspec_array_list = g_slist_prepend (pspec_array_list, proc->out_param_specs);
  pspec_array_list = g_slist_prepend (pspec_array_list, proc->in_param_specs);
  for (slist = pspec_array_list; slist; slist = slist->next)
    {
      GParamSpec **pspec_p;
      
      pspec_p = slist->data;
      if (pspec_p)
        while (*pspec_p)
          {
            if ((*pspec_p)->flags & BSE_PARAM_SERVE_GUI &&
                (*pspec_p)->flags & BSE_PARAM_READABLE)
              {
                BstParam *bparam;
                
                bparam = bst_param_create (proc,
                                           BSE_TYPE_PROCEDURE,
                                           *pspec_p,
                                           (slist->next
					    ? g_quark_to_string (quark_input_params)
					    : g_quark_to_string (quark_output_params)),
                                           param_box,
                                           BST_TOOLTIPS);
                shell->bparams = g_slist_append (shell->bparams, bparam);
                (is_out_param ? shell->n_out_params : shell->n_in_params) += 1;
                if (is_out_param && !shell->first_out_bparam)
                  shell->first_out_bparam = g_slist_last (shell->bparams);
              }
            pspec_p++;
          }
      is_out_param++;
    }
  g_slist_free (pspec_array_list);
  
  /* initialize parameter values
   */
  bst_procedure_shell_reset (shell);
}

void
bst_procedure_shell_execute (BstProcedureShell *shell)
{
  GtkWidget *widget;
  GSList *slist;
  
  g_return_if_fail (BST_IS_PROCEDURE_SHELL (shell));
  g_return_if_fail (GTK_OBJECT_DESTROYED (shell) == FALSE);
  g_return_if_fail (shell->proc != NULL);
  g_return_if_fail (shell->in_execution == FALSE);
  
  widget = GTK_WIDGET (shell);
  
  gtk_widget_ref (widget);
  
  /* process any pending GUI updates
   */
  gdk_flush ();
  GDK_THREADS_LEAVE ();
  do
    g_main_iteration (FALSE);
  while (g_main_pending ());
  GDK_THREADS_ENTER ();
  
  for (slist = shell->first_out_bparam; slist; slist = slist->next)
    bst_param_reset (slist->data);
  bst_procedure_shell_update (shell);
  
  if (!GTK_OBJECT_DESTROYED (widget))
    {
      BseErrorType error;
      
      shell->in_execution = TRUE;
      error = bse_procedure_execvl (shell->proc,
                                    shell->bparams,
                                    shell->first_out_bparam);
      shell->in_execution = FALSE;
      
      /* check for recursion
       */
      if (error == BSE_ERROR_PROC_BUSY)
        gdk_beep ();
      
      bst_procedure_shell_update (shell);
      /* feature procedures with error out parameter */
      if (!error && shell->n_out_params)
	{
	  BstParam *bparam = shell->first_out_bparam->data;

	  /* the execvl error return was set automatically,
	   * so only act on out_param errors
	   */
	  if (g_type_is_a (G_VALUE_TYPE (&bparam->value), BSE_TYPE_ERROR_TYPE))
	    {
	      error = g_value_get_enum (&bparam->value);
	      bst_status_set (error ? 0 : 100, shell->proc->name, bse_error_blurb (error));
	    }
	}
    }
  
  gtk_widget_unref (widget);
}

void
bst_procedure_shell_update (BstProcedureShell *shell)
{
  GSList *slist;
  
  g_return_if_fail (BST_IS_PROCEDURE_SHELL (shell));
  
  for (slist = shell->bparams; slist; slist = slist->next)
    bst_param_get (slist->data);
}

void
bst_procedure_shell_reset (BstProcedureShell *shell)
{
  GSList *slist;
  gint n;
  
  g_return_if_fail (BST_IS_PROCEDURE_SHELL (shell));
  
  n = shell->n_in_params;
  for (slist = shell->bparams; slist; slist = slist->next)
    {
      BstParam *bparam = slist->data;
      
      (n > 0 ? bst_param_set_default : bst_param_reset) (bparam);
      bst_param_set_editable (bparam, n-- > 0);
    }
  shell->n_preset_params = 0;
}

void
bst_procedure_shell_unpreset (BstProcedureShell *shell)
{
  GSList *slist;
  gint n;
  
  g_return_if_fail (BST_IS_PROCEDURE_SHELL (shell));
  
  for (n = shell->n_in_params, slist = shell->bparams; slist && n; n--, slist = slist->next)
    {
      BstParam *bparam = slist->data;
      
      if (!bparam->editable)
	{
	  bst_param_set_default (bparam);
	  bst_param_set_editable (bparam, TRUE);
	}
    }
  shell->n_preset_params = 0;
}

gboolean
bst_procedure_shell_preset (BstProcedureShell *shell,
			    const gchar       *name,
			    const GValue      *value,
			    gboolean           lock_preset)
{
  BseProcedureClass *proc;
  GSList *slist;
  guint n;
  
  g_return_val_if_fail (BST_IS_PROCEDURE_SHELL (shell), 0);
  g_return_val_if_fail (shell->proc != NULL, 0);
  g_return_val_if_fail (name != NULL, 0);
  g_return_val_if_fail (G_IS_VALUE (value), 0);
  
  proc = shell->proc;
  
  /* ok, this is the really interesting part!
   * we try to unificate the preset parameter with the procedure's
   * in parameters. if we find a name match and type conversion
   * is sucessfull, the procedure gets invoked with a
   * predefined parameter.
   */
  
  for (slist = shell->bparams, n = shell->n_in_params; n; slist = slist->next, n--)
    {
      BstParam *bparam = slist->data;
      
      if (strcmp (bparam->pspec->name, name) == 0)
	{
	  if (bst_param_set_value (bparam, value))
	    {
	      if (lock_preset && bparam->editable)
		{
		  shell->n_preset_params += 1;
		  bst_param_set_editable (bparam, FALSE);
		}
	      return TRUE;
	    }
	  else
	    g_warning (G_STRLOC ": cannot convert `%s' value to `%s' for `%s' parameter `%s'",
		       g_type_name (G_VALUE_TYPE (value)),
		       g_type_name (G_PARAM_SPEC_VALUE_TYPE (bparam->pspec)),
		       g_type_name (G_PARAM_SPEC_TYPE (bparam->pspec)),
		       name);
	}
    }
  
  return FALSE;
}

static void
shell_modal_selection_done (GtkWidget *widget)
{
  BstProcedureShell *shell = BST_PROCEDURE_SHELL (widget);
  
  shell->in_modal_selection = FALSE;
}

static void
shell_hide_on_demand (GtkWidget *widget)
{
  BstProcedureShell *shell = BST_PROCEDURE_SHELL (widget);
  
  if (shell->hide_dialog_on_exec)
    gtk_toplevel_hide (widget);
}

GtkWidget*
bst_procedure_dialog_from_shell (BstProcedureShell *shell,
				 GtkWidget        **dialog_p)
{
  GtkWidget *adialog;
  
  g_return_val_if_fail (BST_IS_PROCEDURE_SHELL (shell), NULL);
  g_return_val_if_fail (GTK_OBJECT_DESTROYED (shell) == FALSE, NULL);
  g_return_val_if_fail (GTK_WIDGET (shell)->parent == NULL, NULL);
  
  gtk_widget_show (GTK_WIDGET (shell));
  
  adialog = g_object_connect (bst_adialog_new (NULL, dialog_p, GTK_WIDGET (shell),
					       BST_ADIALOG_POPUP_POS | BST_ADIALOG_MODAL,
					       "title", "BEAST: Procedure",
					       "default_width", 200,
					       NULL),
			      "swapped_signal_after::hide", shell_modal_selection_done, shell,
			      NULL);
  bst_adialog_setup_choices (adialog,
			     "swapped_choice::Execute", shell_hide_on_demand, shell,
			     "swapped_choice::Execute", bst_procedure_shell_execute, shell,
			     "default_choice::Execute", NULL, NULL,
			     "choice::Close", gtk_toplevel_hide, NULL,
			     NULL);
  
  return adialog;
}

BstProcedureShell*
bst_procedure_dialog_get_shell (GtkWidget *dialog)
{
  GtkWidget *child;
  
  g_return_val_if_fail (BST_IS_ADIALOG (dialog), NULL);
  
  child = bst_adialog_get_child (dialog);
  
  return BST_IS_PROCEDURE_SHELL (child) ? BST_PROCEDURE_SHELL (child) : NULL;
}

void
bst_procedure_dialog_exec_modal (GtkWidget *dialog,
				 gboolean   force_display)
{
  BstProcedureShell *shell;
  GtkWidget *widget;
  
  g_return_if_fail (BST_IS_ADIALOG (dialog));
  g_return_if_fail (GTK_WIDGET_VISIBLE (dialog) == FALSE);
  g_return_if_fail (GTK_WINDOW (dialog)->modal == TRUE);
  
  shell = bst_procedure_dialog_get_shell (dialog);
  g_return_if_fail (BST_IS_PROCEDURE_SHELL (shell));
  g_return_if_fail (shell->in_modal_selection == FALSE);
  
  widget = GTK_WIDGET (shell);
  
  gtk_widget_ref (widget);
  
  if (!force_display && shell->n_preset_params == shell->n_in_params && shell->n_out_params == 0)
    bst_procedure_shell_execute (shell);
  else
    {
      if (shell->n_out_params)
	{
	  GtkWidget *sbar = bst_status_bar_ensure (GTK_WINDOW (dialog));
	  
	  shell->hide_dialog_on_exec = FALSE;
	  
	  gtk_widget_show (sbar);
	}
      else
	{
	  GtkWidget *sbar = bst_status_bar_from_window (GTK_WINDOW (dialog));
	  
	  shell->hide_dialog_on_exec = TRUE;
	  
	  if (sbar)
	    gtk_widget_hide (sbar);
	}
      
      shell->in_modal_selection = TRUE;
      gtk_widget_show (dialog);
      
      /* hand control over to user
       */
      bst_status_window_push (dialog);
      do
	{
	  GDK_THREADS_LEAVE ();
	  g_main_iteration (TRUE);
	  GDK_THREADS_ENTER ();
	}
      while (shell->in_modal_selection);
      bst_status_window_pop ();
    }
  
  gtk_widget_unref (widget);
}

void
bst_procedure_user_exec_method (const gchar *proc_path,
				BswProxy     preset_proxy)
{
  BseProcedureClass *procedure;
  BstProcedureShell *pshell;
  GType type;

  g_return_if_fail (proc_path != NULL);

  if (!global_proc_dialog)
    {
      pshell = BST_PROCEDURE_SHELL (bst_procedure_shell_new (NULL));
      global_proc_dialog = bst_procedure_dialog_from_shell (BST_PROCEDURE_SHELL (bst_procedure_shell_new (NULL)),
							    &global_proc_dialog);
    }
  pshell = bst_procedure_dialog_get_shell (global_proc_dialog);

  type = g_type_from_name (proc_path);
  if (!BSE_TYPE_IS_PROCEDURE (type))
    {
      g_warning ("procedure type \"%s\" is not registered", proc_path);
      return;
    }

  procedure = g_type_class_ref (type);
  bst_procedure_shell_set_proc (pshell, procedure);
  g_type_class_unref (procedure);
  bst_procedure_shell_unpreset (pshell);

  if (preset_proxy)
    {
      GValue value = { 0, };

      if (!procedure->n_in_params ||
	  !g_type_is_a (G_OBJECT_TYPE (bse_object_from_id (preset_proxy)),
			G_PARAM_SPEC_VALUE_TYPE (procedure->in_param_specs[0])))
	{
	  g_warning ("proxy %u invalid as preset parameter for procedure type \"%s\"",
		     preset_proxy, proc_path);
	  return;
	}
      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (procedure->in_param_specs[0]));
      g_value_set_object (&value, bse_object_from_id (preset_proxy));
      bst_procedure_shell_preset (pshell, procedure->in_param_specs[0]->name, &value, TRUE);
      g_value_unset (&value);
    }

  bst_status_window_push (GTK_WIDGET (global_proc_dialog));
  bst_procedure_dialog_exec_modal (global_proc_dialog, FALSE);
  bst_status_window_pop ();
}
