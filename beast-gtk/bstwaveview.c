/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2001 Tim Janik
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
#include "bstwaveview.h"

// #include "bstwavedialog.h"
#include "bststatusbar.h"
#include "bstprocedure.h"
#include "bstwaveeditor.h"
#include "bstwavedialog.h"



/* --- prototypes --- */
static void	bst_wave_view_class_init	(BstWaveViewClass	*klass);
static void	bst_wave_view_init		(BstWaveView		*wave_view);
static void	bst_wave_view_operate		(BstItemView		*item_view,
						 BstOps			 op);
static gboolean	bst_wave_view_can_operate	(BstItemView		*item_view,
						 BstOps			 op);


/* --- wave ops --- */
static BstItemViewOp wave_view_ops[] = {
  { "Load...",		BST_OP_WAVE_LOAD,	BST_ICON_CDROM, /* FIXME: WAVE*/	},
  { "Delete",		BST_OP_WAVE_DELETE,	BST_ICON_TRASHCAN,	},
  { "Editor...",	BST_OP_WAVE_EDITOR,	BST_ICON_NONE,	},	// FIXME: need icon
};
static guint n_wave_view_ops = sizeof (wave_view_ops) / sizeof (wave_view_ops[0]);


/* --- static variables --- */
static gpointer		 parent_class = NULL;


/* --- functions --- */
GtkType
bst_wave_view_get_type (void)
{
  static GtkType wave_view_type = 0;
  
  if (!wave_view_type)
    {
      GtkTypeInfo wave_view_info =
      {
	"BstWaveView",
	sizeof (BstWaveView),
	sizeof (BstWaveViewClass),
	(GtkClassInitFunc) bst_wave_view_class_init,
	(GtkObjectInitFunc) bst_wave_view_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      wave_view_type = gtk_type_unique (BST_TYPE_ITEM_VIEW, &wave_view_info);
    }
  
  return wave_view_type;
}

static void
bst_wave_view_class_init (BstWaveViewClass *class)
{
  BstItemViewClass *item_view_class = BST_ITEM_VIEW_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);

  item_view_class->can_operate = bst_wave_view_can_operate;
  item_view_class->operate = bst_wave_view_operate;
  item_view_class->n_ops = n_wave_view_ops;
  item_view_class->ops = wave_view_ops;
}

static void
bst_wave_view_init (BstWaveView *wave_view)
{
  BST_ITEM_VIEW (wave_view)->item_type = BSE_TYPE_WAVE;
  wave_view->load_dialog = bst_wave_dialog_new_load (0, GTK_WIDGET (wave_view));
  g_object_connect (wave_view->load_dialog,
		    "swapped_object_signal_after::hide", bst_update_can_operate, wave_view,
		    NULL);
}

GtkWidget*
bst_wave_view_new (BswProxy wrepo)
{
  GtkWidget *wave_view;
  
  g_return_val_if_fail (BSW_IS_WAVE_REPO (wrepo), NULL);
  
  wave_view = gtk_widget_new (BST_TYPE_WAVE_VIEW, NULL);
  bst_item_view_set_container (BST_ITEM_VIEW (wave_view), wrepo);
  bst_wave_dialog_set_wave_repo (BST_WAVE_DIALOG (BST_WAVE_VIEW (wave_view)->load_dialog), wrepo);

  return wave_view;
}

static void
popup_wave_dialog (BstWaveView *wave_view)
{
  BswProxy wave = bst_item_view_get_current (BST_ITEM_VIEW (wave_view));
  GtkWidget *weditor, *wdialog;

  wdialog = g_object_new (GTK_TYPE_WINDOW,
			  "default_width", 320,
			  "default_height", 200,
			  NULL);
  weditor = g_object_new (BST_TYPE_WAVE_EDITOR,
			  "visible", TRUE,
			  "wave", wave,
			  "parent", wdialog,
			  NULL);
  gtk_signal_connect_object_while_alive (GTK_OBJECT (wave_view),
					 "destroy",
					 G_CALLBACK (gtk_widget_destroy),
					 GTK_OBJECT (wdialog));
  gtk_widget_show (wdialog);
}

void
bst_wave_view_operate (BstItemView *item_view,
		       BstOps       op)
{
  BstWaveView *wave_view = BST_WAVE_VIEW (item_view);
  BswProxy wrepo = item_view->container;
  
  g_return_if_fail (bst_wave_view_can_operate (item_view, op));
  
  switch (op)
    {
      BswProxy item;
    case BST_OP_WAVE_LOAD:
      gtk_widget_show (wave_view->load_dialog);
      // bst_procedure_user_exec_method ("BseWaveRepo+read-file", wrepo);
      break;
    case BST_OP_WAVE_DELETE:
      item = bst_item_view_get_current (BST_ITEM_VIEW (wave_view));
      bsw_wave_repo_remove_wave (wrepo, item);
      break;
    case BST_OP_WAVE_EDITOR:
      popup_wave_dialog (wave_view);
      break;
    default:
      break;
    }
  
  bst_update_can_operate (GTK_WIDGET (wave_view));
}

gboolean
bst_wave_view_can_operate (BstItemView *item_view,
			   BstOps	op)
{
  // BseWaveRepo *wrepo = BSE_WAVE_REPO (item_view->container);

  switch (op)
    {
    case BST_OP_WAVE_LOAD:
      return TRUE;
    case BST_OP_WAVE_DELETE:
      return bst_item_view_get_current (item_view) != 0;
    case BST_OP_WAVE_EDITOR:
      return bst_item_view_get_current (item_view) != 0;
    default:
      return FALSE;
    }
}
