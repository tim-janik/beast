/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bstprocedure.h"

#include "bststatusbar.h"


/* --- prototypes --- */
static void	bst_procedure_dialog_class_init	(BstProcedureDialogClass *klass);
static void	bst_procedure_dialog_init	(BstProcedureDialog	 *pe);
static void	bst_procedure_dialog_destroy	(GtkObject		 *object);
static void     procedure_dialog_button_execute (BstProcedureDialog      *procedure_dialog);


/* --- static variables --- */
static gpointer                 parent_class = NULL;
static BstProcedureDialogClass *bst_procedure_dialog_class = NULL;
static GtkWidget               *global_procedure_dialog = NULL;


/* --- functions --- */
GtkType
bst_procedure_dialog_get_type (void)
{
  static GtkType procedure_dialog_type = 0;
  
  if (!procedure_dialog_type)
    {
      GtkTypeInfo procedure_dialog_info =
      {
	"BstProcedureDialog",
	sizeof (BstProcedureDialog),
	sizeof (BstProcedureDialogClass),
	(GtkClassInitFunc) bst_procedure_dialog_class_init,
	(GtkObjectInitFunc) bst_procedure_dialog_init,
        /* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      procedure_dialog_type = gtk_type_unique (GTK_TYPE_VBOX, &procedure_dialog_info);
    }
  
  return procedure_dialog_type;
}

static void
bst_procedure_dialog_class_init (BstProcedureDialogClass *class)
{
  GtkObjectClass *object_class;

  object_class = GTK_OBJECT_CLASS (class);

  bst_procedure_dialog_class = class;
  parent_class = gtk_type_class (GTK_TYPE_VBOX);

  object_class->destroy = bst_procedure_dialog_destroy;
}

static void
bst_procedure_dialog_init (BstProcedureDialog *procedure_dialog)
{
  procedure_dialog->one_shot_exec = FALSE;
  procedure_dialog->proc = NULL;
  procedure_dialog->n_in_params = 0;
  procedure_dialog->n_out_params = 0;
  procedure_dialog->bparams = NULL;
  procedure_dialog->_params = NULL;
  procedure_dialog->tooltips = gtk_tooltips_new ();
  gtk_object_ref (GTK_OBJECT (procedure_dialog->tooltips));
  gtk_object_sink (GTK_OBJECT (procedure_dialog->tooltips));
}

static void
bst_procedure_dialog_destroy_contents (BstProcedureDialog *procedure_dialog)
{
  gtk_container_foreach (GTK_CONTAINER (procedure_dialog), (GtkCallback) gtk_widget_destroy, NULL);
  g_slist_free (procedure_dialog->bparams);
  procedure_dialog->n_in_params = 0;
  procedure_dialog->n_out_params = 0;
  procedure_dialog->bparams = NULL;
  procedure_dialog->_params = NULL;
}

static void
bst_procedure_dialog_destroy (GtkObject *object)
{
  BstProcedureDialog *procedure_dialog;

  g_return_if_fail (object != NULL);

  procedure_dialog = BST_PROCEDURE_DIALOG (object);

  bst_procedure_dialog_set_proc (procedure_dialog, NULL);

  gtk_object_unref (GTK_OBJECT (procedure_dialog->tooltips));
  procedure_dialog->tooltips = NULL;

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget*
bst_procedure_dialog_new (BseProcedureClass *proc)
{
  GtkWidget *procedure_dialog;

  procedure_dialog = gtk_widget_new (BST_TYPE_PROCEDURE_DIALOG, NULL);
  bst_procedure_dialog_set_proc (BST_PROCEDURE_DIALOG (procedure_dialog), proc);

  return procedure_dialog;
}

void
bst_procedure_dialog_set_proc (BstProcedureDialog *procedure_dialog,
			       BseProcedureClass  *proc)
{
  g_return_if_fail (BST_IS_PROCEDURE_DIALOG (procedure_dialog));
  if (proc)
    g_return_if_fail (BSE_IS_PROCEDURE_CLASS (proc));

  bst_procedure_dialog_destroy_contents (procedure_dialog);
  if (procedure_dialog->proc)
    bse_procedure_unref (procedure_dialog->proc);
  procedure_dialog->proc = proc;
  if (procedure_dialog->proc)
    bse_procedure_ref (procedure_dialog->proc);

  bst_procedure_dialog_rebuild (procedure_dialog);
}

void
bst_procedure_dialog_rebuild (BstProcedureDialog *procedure_dialog)
{
  BseProcedureClass *proc;
  GtkWidget *param_box, *any;
  GSList *slist, *pspec_array_list = NULL;
  guint is_out_param = 0;

  g_return_if_fail (BST_IS_PROCEDURE_DIALOG (procedure_dialog));

  bst_procedure_dialog_destroy_contents (procedure_dialog);
  
  if (!procedure_dialog->proc)
    return;

  proc = procedure_dialog->proc;
  param_box = gtk_widget_new (GTK_TYPE_VBOX,
			      "parent", procedure_dialog,
			      "homogeneous", FALSE,
			      "spacing", 0,
			      "border_width", 5,
			      NULL);

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
					   param_box,
					   GTK_TOOLTIPS (procedure_dialog->tooltips));
		procedure_dialog->bparams = g_slist_append (procedure_dialog->bparams, bparam);
		(is_out_param ? procedure_dialog->n_out_params : procedure_dialog->n_in_params) += 1;
		bst_param_set_editable (bparam, !is_out_param);
		if (is_out_param && !procedure_dialog->_params)
		  procedure_dialog->_params = g_slist_last (procedure_dialog->bparams);
	      }
	    pspec_p++;
	  }
      is_out_param++;
    }
  g_slist_free (pspec_array_list);
  
  bst_procedure_dialog_update (procedure_dialog);

  any = gtk_widget_new (GTK_TYPE_BUTTON,
			"label", "Execute",
			"visible", TRUE,
			"object_signal::clicked", procedure_dialog_button_execute, procedure_dialog,
			NULL);

  gtk_box_pack_end (GTK_BOX (param_box), any, FALSE, TRUE, 5);

  gtk_widget_show (param_box);
}

void
bst_procedure_dialog_update (BstProcedureDialog *procedure_dialog)
{
  GSList *slist;

  g_return_if_fail (BST_IS_PROCEDURE_DIALOG (procedure_dialog));

  for (slist = procedure_dialog->bparams; slist; slist = slist->next)
    bst_param_get (slist->data);
}

void
bst_procedure_dialog_execute (BstProcedureDialog *procedure_dialog)
{
  GtkWidget *widget;

  g_return_if_fail (BST_IS_PROCEDURE_DIALOG (procedure_dialog));
  g_return_if_fail (GTK_OBJECT_DESTROYED (procedure_dialog) == FALSE);
  g_return_if_fail (procedure_dialog->proc != NULL);

  widget = GTK_WIDGET (procedure_dialog);

  gtk_widget_ref (widget);

  bst_procedure_dialog_update (procedure_dialog);

  /* refresh GUI completely
   */
  gdk_flush ();
  do
    g_main_iteration (FALSE);
  while (g_main_pending ());

  if (!GTK_OBJECT_DESTROYED (widget))
    {
      BseErrorType error;

      error = bse_procedure_execvl (procedure_dialog->proc,
				    procedure_dialog->bparams,
				    procedure_dialog->_params);
      
      /* check for recursion
       */
      if (error == BSE_ERROR_PROC_BUSY)
	gdk_beep ();
      
      bst_procedure_dialog_update (procedure_dialog);
    }

  gtk_widget_unref (widget);
}

static void
procedure_dialog_button_execute (BstProcedureDialog *procedure_dialog)
{
  GtkWidget *window = gtk_widget_get_toplevel (GTK_WIDGET (procedure_dialog));

  gtk_widget_ref (GTK_WIDGET (procedure_dialog));

  if (procedure_dialog->one_shot_exec)
    {
      gtk_widget_ref (window);
      gtk_container_remove (GTK_CONTAINER (GTK_WIDGET (procedure_dialog)->parent), GTK_WIDGET (procedure_dialog));
      gtk_widget_destroy (window);
      gtk_widget_unref (window);
    }

  bst_procedure_dialog_execute (procedure_dialog);

  gtk_widget_unref (GTK_WIDGET (procedure_dialog));
}

GtkWidget*
bst_procedure_dialog_get_global (void)
{
  if (!global_procedure_dialog)
    {
      global_procedure_dialog = bst_subwindow_new (NULL,
						   &global_procedure_dialog,
						   bst_procedure_dialog_new (NULL),
						   BST_SUB_POPUP_POS, NULL);
      bst_status_bar_ensure (GTK_WINDOW (global_procedure_dialog));
    }
  
  return global_procedure_dialog;
}

guint
bst_procedure_dialog_preset (BstProcedureDialog *procedure_dialog,
			     gboolean            lock_presets,
			     GSList             *preset_params)
{
  BseProcedureClass *proc;
  GSList *bparam_slist;
  guint n, u = 0;

  g_return_val_if_fail (BST_IS_PROCEDURE_DIALOG (procedure_dialog), 0);
  g_return_val_if_fail (procedure_dialog->proc != NULL, 0);

  proc = procedure_dialog->proc;
  bparam_slist = procedure_dialog->bparams;
  n = preset_params ? procedure_dialog->n_in_params : 0;

  /* ok, this is the really interesting part!
   * we walk the preset_params list and try to unificate those
   * parameters with the procedure's in parameters. if we find
   * a name match and type conversion is sucessfull, the procedure
   * gets invoced with a predefined parameter
   */

  while (n--)
    {
      BstParam *bparam = bparam_slist->data;
      BseParam *iparam = &bparam->param;
      GSList *slist;
      
      for (slist = preset_params; slist; slist = slist->next)
	{
	  BseParam *pparam = slist->data;

	  if (strcmp (iparam->pspec->any.name, pparam->pspec->any.name) == 0)
	    {
	      // g_print ("preset match %s %s\n", iparam->pspec->any.name, pparam->pspec->any.name);
	      if (bst_param_set_from_other (bparam, pparam))
		{
		  u++;
		  if (lock_presets)
		    bst_param_set_editable (bparam, FALSE);
		  break;
		}
	      else
		g_warning (G_STRLOC "can't convert preset parameter `%s' (`%s') to `%s' (`%s')",
			   pparam->pspec->any.name, bse_type_name (pparam->pspec->type),
			   iparam->pspec->any.name, bse_type_name (iparam->pspec->type));
	    }
	}
      
      bparam_slist = bparam_slist->next;
    }

  return u;
}


/* --- BST procedure stuff --- */
void
bst_procedure_void_execpl_modal (BseProcedureClass *proc,
				 GSList            *preset_params)
{
  GtkWidget *widget;
  BstProcedureDialog *proc_dialog;
  guint n_preset = 0;

  g_return_if_fail (BSE_IS_PROCEDURE_CLASS (proc));

  bse_procedure_ref (proc);

  widget = bst_procedure_dialog_new (proc);
  gtk_widget_ref (widget);
  gtk_object_sink (GTK_OBJECT (widget));
  proc_dialog = BST_PROCEDURE_DIALOG (widget);
  gtk_widget_show (widget);

  if (preset_params)
    n_preset = bst_procedure_dialog_preset (proc_dialog, TRUE, preset_params);

  if (n_preset >= proc->n_in_params)
    bst_procedure_dialog_execute (proc_dialog);
  else
    {
      GtkWidget *dialog = bst_subwindow_new (NULL, &dialog,
					     widget,
					     BST_SUB_DESTROY_ON_HIDE | BST_SUB_POPUP_POS | BST_SUB_MODAL,
					     NULL);
      proc_dialog->one_shot_exec = TRUE;
      gtk_widget_show (dialog);

      do
	g_main_iteration (TRUE);
      while (dialog != NULL);
    }

  gtk_widget_destroy (widget);
  gtk_widget_unref (widget);
  
  bse_procedure_unref (proc);
}
