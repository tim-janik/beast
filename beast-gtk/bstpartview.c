/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002 Tim Janik
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
#include "bstpartview.h"

#include "bstpartdialog.h"
#include "bstpianoroll.h"



/* --- prototypes --- */
static void	bst_part_view_class_init	(BstPartViewClass	*klass);
static void	bst_part_view_init		(BstPartView		*part_view);
static void	bst_part_view_operate		(BstItemView		*item_view,
						 BstOps			 op);
static gboolean	bst_part_view_can_operate	(BstItemView		*item_view,
						 BstOps			 op);


/* --- variables --- */
static BstItemViewOp part_view_ops[] = {
  { "Add",		BST_OP_PART_ADD,	BST_STOCK_PART,		},
  { "Delete",		BST_OP_PART_DELETE,	BST_STOCK_TRASHCAN,	},
  { "Editor...",	BST_OP_PART_EDITOR,	BST_STOCK_PART_EDITOR,	},
};
static gpointer	parent_class = NULL;


/* --- functions --- */
GType
bst_part_view_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info = {
	sizeof (BstPartViewClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) bst_part_view_class_init,
	NULL,   /* class_finalize */
	NULL,   /* class_data */
	sizeof (BstPartView),
	0,      /* n_preallocs */
	(GInstanceInitFunc) bst_part_view_init,
      };

      type = g_type_register_static (BST_TYPE_ITEM_VIEW,
				     "BstPartView",
				     &type_info, 0);
    }

  return type;
}

static void
bst_part_view_class_init (BstPartViewClass *class)
{
  // GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  BstItemViewClass *item_view_class = BST_ITEM_VIEW_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);

  item_view_class->can_operate = bst_part_view_can_operate;
  item_view_class->operate = bst_part_view_operate;
  item_view_class->n_ops = G_N_ELEMENTS (part_view_ops);
  item_view_class->ops = part_view_ops;
}

static void
bst_part_view_init (BstPartView *part_view)
{
  BST_ITEM_VIEW (part_view)->item_type = g_type_from_name ("BsePart");	// FIXME
}

static void
popup_part_dialog (BstPartView *part_view)
{
  BswProxy part;
  GtkWidget *pdialog;

  part = bst_item_view_get_current (BST_ITEM_VIEW (part_view));
  pdialog = g_object_new (BST_TYPE_PART_DIALOG, NULL);

  bst_part_dialog_set_proxy (BST_PART_DIALOG (pdialog), part);
  g_signal_connect_object (part_view, "destroy", G_CALLBACK (gtk_widget_destroy), pdialog, G_CONNECT_SWAPPED);
  gtk_widget_show (pdialog);
}

void
bst_part_view_operate (BstItemView *item_view,
		       BstOps       op)
{
  BstPartView *part_view = BST_PART_VIEW (item_view);
  BswProxy song;
  
  g_return_if_fail (bst_part_view_can_operate (item_view, op));

  song = item_view->container;
  
  switch (op)
    {
      BswProxy item;
    case BST_OP_PART_ADD:
      item = bsw_song_create_part (song);
      bst_item_view_select (item_view, item);
      break;
    case BST_OP_PART_DELETE:
      item = bst_item_view_get_current (BST_ITEM_VIEW (part_view));
      bsw_song_remove_part (song, item);
      break;
    case BST_OP_PART_EDITOR:
      popup_part_dialog (part_view);
      break;
    default:
      break;
    }
  
  bst_update_can_operate (GTK_WIDGET (part_view));
}

gboolean
bst_part_view_can_operate (BstItemView *item_view,
			   BstOps	op)
{
  BstPartView *part_view = BST_PART_VIEW (item_view);
  BswProxy song;

  g_return_val_if_fail (BST_IS_PART_VIEW (part_view), FALSE);
  
  song = item_view->container;

  switch (op)
    {
      BswProxy item;
    case BST_OP_PART_ADD:
      return TRUE;
    case BST_OP_PART_DELETE:
      item = bst_item_view_get_current (BST_ITEM_VIEW (part_view));
      return item != 0;
    case BST_OP_PART_EDITOR:
      item = bst_item_view_get_current (BST_ITEM_VIEW (part_view));
      return item != 0;
    default:
      return FALSE;
    }
}
