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



/* --- prototypes --- */
static void	bst_part_view_class_init	(BstPartViewClass	*klass);
static void	bst_part_view_init		(BstPartView		*part_view);
static void     part_view_action_exec           (gpointer                data,
                                                 gulong                  action);
static gboolean part_view_action_check          (gpointer                data,
                                                 gulong                  action);


/* --- part actions --- */
enum {
  ACTION_ADD_PART,
  ACTION_DELETE_PART,
  ACTION_EDIT_PART
};
static const GxkStockAction part_view_actions[] = {
  { N_("Add"),          NULL,   NULL,   ACTION_ADD_PART,        BST_STOCK_PART },
  { N_("Delete"),       NULL,   NULL,   ACTION_DELETE_PART,     BST_STOCK_TRASHCAN },
  { N_("Editor"),       NULL,   NULL,   ACTION_EDIT_PART,       BST_STOCK_PART_EDITOR },
};


/* --- variables --- */
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
    }
  return type;
}

static void
bst_part_view_class_init (BstPartViewClass *class)
{
  // GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  BstItemViewClass *item_view_class = BST_ITEM_VIEW_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);

  item_view_class->item_type = "BsePart";
}

static void
bst_part_view_init (BstPartView *self)
{
  BstItemView *iview = BST_ITEM_VIEW (self);
  /* complete GUI */
  GxkGadget *gadget = gxk_gadget_complete (GTK_WIDGET (self), "beast", "part-view", NULL);
  /* setup tree view */
  GtkTreeView *tview = gxk_gadget_find (gadget, "tree-view");
  bst_item_view_complete_tree (iview, tview);
  /* create tool actions */
  gxk_widget_publish_actions (self, "part-view-actions",
                              G_N_ELEMENTS (part_view_actions), part_view_actions,
                              NULL, part_view_action_check, part_view_action_exec);
  /* create property editor */
  bst_item_view_build_param_view (iview, gxk_gadget_find (gadget, "property-area"));
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
part_view_action_exec (gpointer                data,
                       gulong                  action)
{
  BstPartView *self = BST_PART_VIEW (data);
  BstItemView *item_view = BST_ITEM_VIEW (self);
  SfiProxy song = item_view->container;
  switch (action)
    {
      SfiProxy item;
    case ACTION_ADD_PART:
      item = bse_song_create_part (song);
      bst_item_view_select (item_view, item);
      break;
    case ACTION_DELETE_PART:
      item = bst_item_view_get_current (item_view);
      bse_song_remove_part (song, item);
      break;
    case ACTION_EDIT_PART:
      popup_part_dialog (self);
      break;
    }
  gxk_widget_update_actions_downwards (self);
}

static gboolean
part_view_action_check (gpointer                data,
                        gulong                  action)
{
  BstPartView *self = BST_PART_VIEW (data);
  BstItemView *item_view = BST_ITEM_VIEW (self);
  switch (action)
    {
      SfiProxy item;
    case ACTION_ADD_PART:
      return TRUE;
    case ACTION_DELETE_PART:
      item = bst_item_view_get_current (item_view);
      return item != 0;
    case ACTION_EDIT_PART:
      item = bst_item_view_get_current (item_view);
      return item != 0;
    default:
      return FALSE;
    }
}
