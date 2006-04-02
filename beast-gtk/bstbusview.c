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
#include "bstbusview.h"


/* --- prototypes --- */
static void     bus_view_action_exec           (gpointer                data,
                                                gulong                  action);
static gboolean bus_view_action_check          (gpointer                data,
                                                gulong                  action,
                                                guint64                 action_stamp);


/* --- bus actions --- */
enum {
  ACTION_ADD_BUS,
  ACTION_DELETE_BUS,
};
static const GxkStockAction bus_view_actions[] = {
  { N_("Add"),          NULL,   NULL,   ACTION_ADD_BUS,        BST_STOCK_NO_ICON },
  { N_("Delete"),       NULL,   NULL,   ACTION_DELETE_BUS,     BST_STOCK_TRASHCAN },
};


/* --- functions --- */
G_DEFINE_TYPE (BstBusView, bst_bus_view, BST_TYPE_ITEM_VIEW);

static void
bst_bus_view_class_init (BstBusViewClass *class)
{
  BstItemViewClass *item_view_class = BST_ITEM_VIEW_CLASS (class);
  
  item_view_class->item_type = "BseBus";
}

static void
bst_bus_view_init (BstBusView *self)
{
  BstItemView *iview = BST_ITEM_VIEW (self);
  /* complete GUI */
  GxkRadget *radget = gxk_radget_complete (GTK_WIDGET (self), "beast", "bus-view", NULL);
  /* setup tree view */
  GtkTreeView *tview = gxk_radget_find (radget, "tree-view");
  bst_item_view_complete_tree (iview, tview);
  /* create tool actions */
  gxk_widget_publish_actions (self, "bus-view-actions",
                              G_N_ELEMENTS (bus_view_actions), bus_view_actions,
                              NULL, bus_view_action_check, bus_view_action_exec);
  /* create property editor */
  bst_item_view_build_param_view (iview, gxk_radget_find (radget, "property-area"));
}

GtkWidget*
bst_bus_view_new (SfiProxy song)
{
  GtkWidget *bus_view;
  
  g_return_val_if_fail (BSE_IS_SONG (song), NULL);
  
  bus_view = gtk_widget_new (BST_TYPE_BUS_VIEW, NULL);
  bst_item_view_set_container (BST_ITEM_VIEW (bus_view), song);
  
  return bus_view;
}

static void
bus_view_action_exec (gpointer                data,
                      gulong                  action)
{
  BstBusView *self = BST_BUS_VIEW (data);
  BstItemView *item_view = BST_ITEM_VIEW (self);
  SfiProxy song = item_view->container;
  switch (action)
    {
      SfiProxy item;
    case ACTION_ADD_BUS:
      item = bse_song_create_bus (song);
      bst_item_view_select (item_view, item);
      break;
    case ACTION_DELETE_BUS:
      item = bst_item_view_get_current (item_view);
      bse_song_remove_bus (song, item);
      break;
    }
  gxk_widget_update_actions_downwards (self);
}

static gboolean
bus_view_action_check (gpointer                data,
                       gulong                  action,
                       guint64                 action_stamp)
{
  BstBusView *self = BST_BUS_VIEW (data);
  BstItemView *item_view = BST_ITEM_VIEW (self);
  switch (action)
    {
      SfiProxy item;
    case ACTION_ADD_BUS:
      return TRUE;
    case ACTION_DELETE_BUS:
      item = bst_item_view_get_current (item_view);
      return item != 0;
    default:
      return FALSE;
    }
}
