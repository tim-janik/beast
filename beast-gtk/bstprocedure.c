/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2002 Tim Janik
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
#include "bstprocedure.h"

#include "bsttexttools.h"
#include <gobject/gvaluecollector.h>
#include <string.h>


/* --- prototypes --- */
static void     bst_procedure_shell_class_init (BstProcedureShellClass *klass);
static void     bst_procedure_shell_init       (BstProcedureShell      *pe);
static void     bst_procedure_shell_destroy    (GtkObject              *object);


/* --- static variables --- */
static BstProcedureShell *global_proc_shell = NULL;
static gpointer           parent_class = NULL;
static GQuark             quark_input_params = 0;
static GQuark             quark_output_params = 0;


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
	g_type_class_unref (procedure_shell->proc);
      procedure_shell->proc = proc;
      if (procedure_shell->proc)
	g_type_class_ref (BSE_PROCEDURE_TYPE (procedure_shell->proc));
      
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
      GtkWidget *hbox, *text = bst_scroll_text_create (BST_TEXT_VIEW_CENTER, proc->blurb);

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
  pspec_array_list = g_slist_prepend (pspec_array_list, proc->out_pspecs);
  pspec_array_list = g_slist_prepend (pspec_array_list, proc->in_pspecs);
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
  g_return_if_fail (shell->proc != NULL);
  g_return_if_fail (shell->in_execution == FALSE);
  
  widget = GTK_WIDGET (shell);
  
  gtk_widget_ref (widget);
  
  for (slist = shell->first_out_bparam; slist; slist = slist->next)
    bst_param_reset (slist->data);
  bst_procedure_shell_update (shell);
  
  if (widget)
    {
      BseErrorType error;

      shell->in_execution = TRUE;
      error = bse_procedure_execvl (shell->proc,
                                    shell->bparams,
                                    shell->first_out_bparam);
      shell->in_execution = FALSE;

      bst_status_eprintf (error, "Executing `%s'", shell->proc->name);
      
      bst_procedure_shell_update (shell);
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
    gxk_toplevel_hide (widget);
}

BstProcedureShell*
bst_procedure_shell_global (void)
{
  if (!global_proc_shell)
    {
      GtkWidget *dialog;
      
      global_proc_shell = (BstProcedureShell*) bst_procedure_shell_new (NULL);
      g_object_ref (global_proc_shell);
      gtk_object_sink (GTK_OBJECT (global_proc_shell));
      dialog = gxk_dialog_new (NULL, NULL, GXK_DIALOG_STATUS_SHELL | GXK_DIALOG_HIDE_ON_DELETE | GXK_DIALOG_MODAL,
			       "Procedure", NULL);

      gtk_container_add (GTK_CONTAINER (GXK_DIALOG (dialog)->vbox), GTK_WIDGET (global_proc_shell));
      gtk_widget_show (GTK_WIDGET (global_proc_shell));

      /* actions */
      gxk_dialog_default_action_swapped (GXK_DIALOG (dialog),
					 BST_STOCK_EXECUTE, bst_procedure_shell_execute, global_proc_shell);
      gxk_dialog_action (GXK_DIALOG (dialog), BST_STOCK_CLOSE, gxk_toplevel_delete, NULL);
    }
  return global_proc_shell;
}

static void
bst_procedure_exec_internal (GType        procedure_type,
			     const gchar *preset_param,
			     gboolean     modal,
			     gboolean     auto_start,
			     gboolean	  main_loop_recurse,
			     va_list      var_args)
{
  BseProcedureClass *proc = g_type_class_ref (procedure_type);
  BstProcedureShell *shell;
  GtkWidget *dialog;

  /* structure setup */
  shell = bst_procedure_shell_global ();
  dialog = gtk_widget_get_toplevel (GTK_WIDGET (shell));
  bst_procedure_shell_set_proc (shell, proc);

  /* set preset parameters */
  bst_procedure_shell_unpreset (shell);
  while (preset_param)
    {
      GType vtype = va_arg (var_args, GType);
      gchar *error = NULL;
      GValue value = { 0, };

      g_value_init (&value, vtype);
      G_VALUE_COLLECT (&value, var_args, 0, &error);
      if (error)
	{
	  g_warning ("%s: %s", G_STRLOC, error);
	  g_free (error);
	  break;
	}
      bst_procedure_shell_preset (shell, preset_param, &value, TRUE);
      g_value_unset (&value);
      preset_param = va_arg (var_args, const gchar*);
    }

  if (modal)
    gxk_dialog_add_flags (GXK_DIALOG (dialog), GXK_DIALOG_MODAL);
  else
    gxk_dialog_clear_flags (GXK_DIALOG (dialog), GXK_DIALOG_MODAL);

  /* execution */
  gxk_status_window_push (dialog);
  g_object_ref (dialog);
  if (auto_start && shell->n_preset_params == shell->n_in_params && shell->n_out_params == 0)
    bst_procedure_shell_execute (shell);
  else
    {
      /* hand control over to user
       */
      gtk_widget_show_now (dialog);
      gdk_window_raise (dialog->window);
      if (main_loop_recurse)
	{
	  do
	    {
	      GDK_THREADS_LEAVE ();
	      g_main_iteration (TRUE);
	      GDK_THREADS_ENTER ();
	    }
	  while (GTK_WIDGET_DRAWABLE (dialog));
	}
    }
  gxk_status_window_pop ();
  g_object_unref (dialog);

  g_type_class_unref (proc);
}

void
bst_procedure_exec_modal (GType        procedure_type,
			  const gchar *preset_param,
			  ...)
{
  va_list var_args;

  g_return_if_fail (BSE_TYPE_IS_PROCEDURE (procedure_type));

  va_start (var_args, preset_param);
  bst_procedure_exec_internal (procedure_type, preset_param, TRUE, TRUE, TRUE, var_args);
  va_end (var_args);
}

void
bst_procedure_exec (GType        procedure_type,
		    const gchar *preset_param,
		    ...)
{
  va_list var_args;

  g_return_if_fail (BSE_TYPE_IS_PROCEDURE (procedure_type));

  va_start (var_args, preset_param);
  bst_procedure_exec_internal (procedure_type, preset_param, FALSE, FALSE, FALSE, var_args);
  va_end (var_args);
}

void
bst_procedure_exec_auto (GType        procedure_type,
			 const gchar *preset_param,
			 ...)
{
  va_list var_args;

  g_return_if_fail (BSE_TYPE_IS_PROCEDURE (procedure_type));

  va_start (var_args, preset_param);
  bst_procedure_exec_internal (procedure_type, preset_param, FALSE, TRUE, FALSE, var_args);
  va_end (var_args);
}
