/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002-2003 Tim Janik
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
#include "bstactivatable.h"



/* --- prototypes --- */
static void	bst_part_view_class_init	(BstPartViewClass	*klass);
static void	bst_part_view_init		(BstPartView		*part_view);
static void     bst_part_view_activate          (BstActivatable         *activatable,
                                                 gulong                  action);
static gboolean bst_part_view_can_activate      (BstActivatable         *activatable,
                                                 gulong                  action);


/* --- variables --- */
static BstItemViewOp part_view_ops[] = {
  { N_("Add"),		BST_ACTION_ADD_PART,	BST_STOCK_PART,		},
  { N_("Delete"),	BST_ACTION_DELETE_PART,	BST_STOCK_TRASHCAN,	},
  { N_("Editor"),	BST_ACTION_EDIT_PART,	BST_STOCK_PART_EDITOR,	},
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
      type = g_type_register_static (BST_TYPE_ITEM_VIEW, "BstPartView", &type_info, 0);
      bst_type_implement_activatable (type,
                                      bst_part_view_activate,
                                      bst_part_view_can_activate,
                                      NULL);
    }
  return type;
}

static void
bst_part_view_class_init (BstPartViewClass *class)
{
  // GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  BstItemViewClass *item_view_class = BST_ITEM_VIEW_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);

  item_view_class->n_ops = G_N_ELEMENTS (part_view_ops);
  item_view_class->ops = part_view_ops;
  item_view_class->item_type = "BsePart";
}

static void
bst_part_view_init (BstPartView *part_view)
{
}

static void
popup_part_dialog (BstPartView *part_view)
{
  SfiProxy part;
  GtkWidget *pdialog;

  part = bst_item_view_get_current (BST_ITEM_VIEW (part_view));
  pdialog = g_object_new (BST_TYPE_PART_DIALOG, NULL);

  bst_part_dialog_set_proxy (BST_PART_DIALOG (pdialog), part);
  g_signal_connect_object (part_view, "destroy", G_CALLBACK (gtk_widget_destroy), pdialog, G_CONNECT_SWAPPED);
  gtk_widget_show (pdialog);
}

static void
bst_part_view_activate (BstActivatable         *activatable,
                        gulong                  action)
{
  BstPartView *self = BST_PART_VIEW (activatable);
  BstItemView *item_view = BST_ITEM_VIEW (self);
  SfiProxy song = item_view->container;

  switch (action)
    {
      SfiProxy item;
    case BST_ACTION_ADD_PART:
      item = bse_song_create_part (song);
      bst_item_view_select (item_view, item);
      break;
    case BST_ACTION_DELETE_PART:
      item = bst_item_view_get_current (item_view);
      bse_song_remove_part (song, item);
      break;
    case BST_ACTION_EDIT_PART:
      popup_part_dialog (self);
      break;
    }
  bst_widget_update_activatable (activatable);
}

static gboolean
bst_part_view_can_activate (BstActivatable *activatable,
                            gulong          action)
{
  BstPartView *self = BST_PART_VIEW (activatable);
  BstItemView *item_view = BST_ITEM_VIEW (self);

  switch (action)
    {
      SfiProxy item;
    case BST_ACTION_ADD_PART:
      return TRUE;
    case BST_ACTION_DELETE_PART:
      item = bst_item_view_get_current (item_view);
      return item != 0;
    case BST_ACTION_EDIT_PART:
      item = bst_item_view_get_current (item_view);
      return item != 0;
    default:
      return FALSE;
    }
}
