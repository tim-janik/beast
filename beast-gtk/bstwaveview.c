/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2003 Tim Janik
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
#include "bstprocedure.h"
#include "bstwaveeditor.h"
#include "bstfiledialog.h"
#include "bstsampleeditor.h"



/* --- prototypes --- */
static void	bst_wave_view_class_init	(BstWaveViewClass	*klass);
static void	bst_wave_view_init		(BstWaveView		*wave_view);
static void	bst_wave_view_operate		(BstItemView		*item_view,
						 BstOps			 op);
static gboolean	bst_wave_view_can_operate	(BstItemView		*item_view,
						 BstOps			 op);


/* --- wave ops --- */
static BstItemViewOp wave_view_ops[] = {
  { "Load...",		BST_OP_WAVE_LOAD,	BST_STOCK_LOAD,	},
  { "Lib...",		BST_OP_WAVE_LOAD_LIB,	BST_STOCK_LOAD_LIB,	},
  { "Delete",		BST_OP_WAVE_DELETE,	BST_STOCK_TRASHCAN,	},
  { "Editor",		BST_OP_WAVE_EDITOR,	BST_STOCK_EDIT_TOOL,	},
};
static guint n_wave_view_ops = sizeof (wave_view_ops) / sizeof (wave_view_ops[0]);


/* --- static variables --- */
static gpointer		 parent_class = NULL;


/* --- functions --- */
GType
bst_wave_view_get_type (void)
{
  static GType type = 0;
  if (!type)
    {
      static const GTypeInfo type_info = {
	sizeof (BstWaveViewClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) bst_wave_view_class_init,
	NULL,   /* class_finalize */
	NULL,   /* class_data */
	sizeof (BstWaveView),
	0,      /* n_preallocs */
	(GInstanceInitFunc) bst_wave_view_init,
      };
      type = g_type_register_static (BST_TYPE_ITEM_VIEW, "BstWaveView", &type_info, 0);
    }
  return type;
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
  item_view_class->item_type = "BseWave";
}

static void
bst_wave_view_init (BstWaveView *self)
{
  self->editable = TRUE;
}

GtkWidget*
bst_wave_view_new (SfiProxy wrepo)
{
  GtkWidget *wave_view;
  
  g_return_val_if_fail (BSE_IS_WAVE_REPO (wrepo), NULL);
  
  wave_view = gtk_widget_new (BST_TYPE_WAVE_VIEW, NULL);
  bst_item_view_set_container (BST_ITEM_VIEW (wave_view), wrepo);

  return wave_view;
}

static void
popup_wave_dialog (BstWaveView *wave_view)
{
  SfiProxy wave = bst_item_view_get_current (BST_ITEM_VIEW (wave_view));
  GtkWidget *weditor, *wdialog;

  weditor = bst_wave_editor_new (wave);
  wdialog = gxk_dialog_new (NULL, GTK_OBJECT (wave_view), GXK_DIALOG_DELETE_BUTTON,
			    NULL, weditor);
  bst_window_sync_title_to_proxy (GXK_DIALOG (wdialog), wave, "%s");
  gtk_widget_show (wdialog);
}

#if 0
static void
popup_wave_dialog (BstWaveView *wave_view)
{
  SfiProxy wave = bst_item_view_get_current (BST_ITEM_VIEW (wave_view));
  SfiProxy esample = bse_wave_use_editable (wave, 0);

  if (esample)
    {
      GtkWidget *wdialog, *editor = bst_sample_editor_new (esample);

      wdialog = gxk_dialog_new (NULL, GTK_OBJECT (wave_view), GXK_DIALOG_DELETE_BUTTON,
				NULL,
				editor);
      bst_window_sync_title_to_proxy (GXK_DIALOG (wdialog), esample, "%s");
      gtk_widget_show (editor);
      bse_item_unuse (esample);
      gtk_widget_show (wdialog);
    }
}
#endif

void
bst_wave_view_set_editable (BstWaveView *self,
                            gboolean     enabled)
{
  BstItemView *iview = BST_ITEM_VIEW (self);

  g_return_if_fail (BST_IS_WAVE_VIEW (self));

  self->editable = enabled != FALSE;
  bst_item_view_complete_tree (iview);
  if (iview->tree)
    gxk_tree_view_set_editable (iview->tree, self->editable);

  bst_update_can_operate (GTK_WIDGET (self));
}

void
bst_wave_view_operate (BstItemView *item_view,
		       BstOps       op)
{
  BstWaveView *self = BST_WAVE_VIEW (item_view);
  SfiProxy wrepo = item_view->container;
  
  g_return_if_fail (bst_wave_view_can_operate (item_view, op));
  
  switch (op)
    {
      SfiProxy item;
    case BST_OP_WAVE_LOAD:
      bst_file_dialog_popup_load_wave (item_view, BST_ITEM_VIEW (self)->container, FALSE);
      break;
    case BST_OP_WAVE_LOAD_LIB:
      bst_file_dialog_popup_load_wave (item_view, BST_ITEM_VIEW (self)->container, TRUE);
      break;
    case BST_OP_WAVE_DELETE:
      item = bst_item_view_get_current (BST_ITEM_VIEW (self));
      bse_wave_repo_remove_wave (wrepo, item);
      break;
    case BST_OP_WAVE_EDITOR:
      popup_wave_dialog (self);
      break;
    default:
      break;
    }
  
  bst_update_can_operate (GTK_WIDGET (self));
}

gboolean
bst_wave_view_can_operate (BstItemView *item_view,
			   BstOps	op)
{
  BstWaveView *self = BST_WAVE_VIEW (item_view);
  switch (op)
    {
    case BST_OP_WAVE_LOAD:
    case BST_OP_WAVE_LOAD_LIB:
      return TRUE;
    case BST_OP_WAVE_DELETE:
      return bst_item_view_get_current (item_view) != 0;
    case BST_OP_WAVE_EDITOR:
      return bst_item_view_get_current (item_view) != 0 && self->editable;
    default:
      return FALSE;
    }
}
