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


/* --- prototypes --- */
static void     bst_procedure_shell_class_init (BstProcedureShellClass *klass);
static void     bst_procedure_shell_init       (BstProcedureShell      *pe);
static void     bst_procedure_shell_destroy    (GtkObject              *object);


/* --- static variables --- */
static gpointer                parent_class = NULL;
static BstProcedureShellClass *bst_procedure_shell_class = NULL;


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
  GtkObjectClass *object_class;
  
  object_class = GTK_OBJECT_CLASS (class);
  
  bst_procedure_shell_class = class;
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
  procedure_shell->tooltips = gtk_tooltips_new ();
  gtk_object_ref (GTK_OBJECT (procedure_shell->tooltips));
  gtk_object_sink (GTK_OBJECT (procedure_shell->tooltips));
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
  
  gtk_object_unref (GTK_OBJECT (shell->tooltips));
  shell->tooltips = NULL;
  
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
  
  g_return_if_fail (shell->tooltips != NULL);

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
                      "width", 1,
                      "height", 50,
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
      BseParamSpec **pspec_p;
      
      pspec_p = slist->data;
      if (pspec_p)
        while (*pspec_p)
          {
            if ((*pspec_p)->any.flags & BSE_PARAM_SERVE_GUI &&
                (*pspec_p)->any.flags & BSE_PARAM_READABLE)
              {
                BstParam *bparam;
                
                bparam = bst_param_create (proc,
                                           BSE_TYPE_PROCEDURE,
                                           *pspec_p,
                                           (slist->next
                                            ? g_quark_from_static_string ("Input Parameters")
                                            : g_quark_from_static_string ("Output Parameters")),
                                           param_box,
                                           GTK_TOOLTIPS (shell->tooltips));
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
  do
    g_main_iteration (FALSE);
  while (g_main_pending ());
  
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

      bst_param_set_editable (bparam, n-- > 0);
      bst_param_reset (bparam);
    }
  shell->n_preset_params = 0;
}

guint
bst_procedure_shell_preset (BstProcedureShell *shell,
			    gboolean           lock_presets,
			    GSList            *preset_params)
{
  BseProcedureClass *proc;
  GSList *bparam_slist;
  guint n;
  
  g_return_val_if_fail (BST_IS_PROCEDURE_SHELL (shell), 0);
  g_return_val_if_fail (shell->proc != NULL, 0);
  
  proc = shell->proc;
  bparam_slist = shell->bparams;
  n = preset_params ? shell->n_in_params : 0;
  
  /* ok, this is the really interesting part!
   * we walk the preset_params list and try to unificate the
   * parameters with the procedure's in parameters. if we find
   * a name match and type conversion is sucessfull, the procedure
   * gets invoked with a predefined parameter
   */
  
  shell->n_preset_params = 0;
  while (n--)
    {
      BstParam *bparam = bparam_slist->data;
      BseParam *iparam = &bparam->param;
      GSList *slist;
      
      bst_param_set_editable (bparam, TRUE);
      for (slist = preset_params; slist; slist = slist->next)
        {
          BseParam *pparam = slist->data;
	  
          if (strcmp (iparam->pspec->any.name, pparam->pspec->any.name) == 0)
            {
              if (bst_param_set_from_other (bparam, pparam))
                {
		  shell->n_preset_params += 1;
                  if (lock_presets)
                    bst_param_set_editable (bparam, FALSE);
                  break;
                }
              else
                g_warning (G_STRLOC ": can't convert preset parameter `%s' from `%s' to `%s'",
                           pparam->pspec->any.name,
                           bse_type_name (pparam->pspec->type),
                           bse_type_name (iparam->pspec->type));
            }
        }
      
      bparam_slist = bparam_slist->next;
    }

  /* and reset output parameters
   */
  for (; bparam_slist; bparam_slist = bparam_slist->next)
    bst_param_reset (bparam_slist->data);
  
  return shell->n_preset_params;
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
  
  adialog = bst_adialog_new (NULL, dialog_p, GTK_WIDGET (shell),
			     BST_ADIALOG_POPUP_POS | BST_ADIALOG_MODAL,
			     "title", "BEAST: Procedure",
			     "object_signal_after::hide", shell_modal_selection_done, shell,
			     "data_choice::Execute", shell_hide_on_demand, shell,
			     "data_choice::Execute", bst_procedure_shell_execute, shell,
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
        g_main_iteration (TRUE);
      while (shell->in_modal_selection);
      bst_status_window_pop ();
    }
  
  gtk_widget_unref (widget);
}
