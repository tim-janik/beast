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
      
      procedure_dialog_type = gtk_type_unique (GTK_TYPE_WINDOW, &procedure_dialog_info);
    }
  
  return procedure_dialog_type;
}

static void
bst_procedure_dialog_class_init (BstProcedureDialogClass *class)
{
  GtkObjectClass *object_class;

  object_class = GTK_OBJECT_CLASS (class);

  bst_procedure_dialog_class = class;
  parent_class = gtk_type_class (GTK_TYPE_WINDOW);

  object_class->destroy = bst_procedure_dialog_destroy;
}

static void
bst_procedure_dialog_init (BstProcedureDialog *procedure_dialog)
{
  procedure_dialog->proc = NULL;
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
  GtkWidget *param_box, *any, *sbar;
  GSList *slist, *pspec_array_list = NULL;
  guint hackval = 0;

  g_return_if_fail (BST_IS_PROCEDURE_DIALOG (procedure_dialog));

  bst_procedure_dialog_destroy_contents (procedure_dialog);
  
  if (!procedure_dialog->proc)
    return;

  bst_status_bar_ensure (GTK_WINDOW (procedure_dialog));
  sbar = bst_status_bar_from_window (GTK_WINDOW (procedure_dialog));

  proc = procedure_dialog->proc;
  param_box = gtk_widget_new (GTK_TYPE_VBOX,
			      "parent", GTK_BIN (procedure_dialog)->child,
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
		bst_param_set_editable (bparam, !hackval);
		if (hackval && !procedure_dialog->_params)
		  procedure_dialog->_params = g_slist_last (procedure_dialog->bparams);
	      }
	    pspec_p++;
	  }
      hackval++;
    }
  g_slist_free (pspec_array_list);
  
  bst_procedure_dialog_update (procedure_dialog);

  any = gtk_widget_new (GTK_TYPE_BUTTON,
			"label", "Execute",
			"visible", TRUE,
			"object_signal::clicked", bst_procedure_dialog_execute, procedure_dialog,
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

GtkWidget*
bst_procedure_dialog_get_global (void)
{
  if (!global_procedure_dialog)
    {
      global_procedure_dialog = bst_procedure_dialog_new (NULL);
      gtk_widget_set (global_procedure_dialog,
		      "signal::destroy", gtk_widget_destroyed, &global_procedure_dialog,
		      "window_position", GTK_WIN_POS_MOUSE,
		      NULL);
    }
  
  return global_procedure_dialog;
}

guint
bst_procedure_dialog_preset (BstProcedureDialog *procedure_dialog,
			     gboolean            lock_presets,
			     GSList             *preset_params)
{
  BseProcedureClass *proc;
  GSList *slist;
  guint n = 0;

  g_return_val_if_fail (BST_IS_PROCEDURE_DIALOG (procedure_dialog), 0);
  g_return_val_if_fail (procedure_dialog->proc != NULL, 0);

  proc = procedure_dialog->proc;
  slist = procedure_dialog->bparams;

  while (n < proc->n_in_params && preset_params)
    {
      if (!bst_param_set_from_other (preset_params->data, slist->data))
	break;

      n++;
      if (lock_presets)
	bst_param_set_editable (preset_params->data, FALSE);
      preset_params = preset_params->next;
      slist = slist->next;
    }

  return n;
}


/* --- BST procedure stuff --- */
void
bst_procedure_void_execpl (BseProcedureClass *proc,
			   GSList            *preset_params)
{
  GtkWidget *widget;
  BstProcedureDialog *proc_dialog;
  guint n_preset = 0;

  g_return_if_fail (BSE_IS_PROCEDURE_CLASS (proc));

  bse_procedure_ref (proc);

  widget = bst_procedure_dialog_get_global ();
  proc_dialog = BST_PROCEDURE_DIALOG (widget);

  bst_procedure_dialog_set_proc (proc_dialog, proc);

  if (preset_params)
    n_preset = bst_procedure_dialog_preset (proc_dialog, TRUE, preset_params);

  if (n_preset >= proc->n_in_params)
    bst_procedure_dialog_execute (proc_dialog);
  else
    gtk_widget_show (widget);
  
  bse_procedure_unref (proc);
}
