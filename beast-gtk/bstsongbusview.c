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
#include "bstsongbusview.h"


/* --- prototypes --- */
static void     song_bus_view_action_exec           (gpointer                data,
                                                     gulong                  action);
static gboolean song_bus_view_action_check          (gpointer                data,
                                                     gulong                  action);


/* --- song bus actions --- */
enum {
  ACTION_ADD_SONG_BUS,
  ACTION_DELETE_SONG_BUS,
};
static const GxkStockAction song_bus_view_actions[] = {
  { N_("Add"),          NULL,   NULL,   ACTION_ADD_SONG_BUS,        BST_STOCK_NO_ICON },
  { N_("Delete"),       NULL,   NULL,   ACTION_DELETE_SONG_BUS,     BST_STOCK_TRASHCAN },
};


/* --- functions --- */
G_DEFINE_TYPE (BstSongBusView, bst_song_bus_view, BST_TYPE_ITEM_VIEW);

static void
bst_song_bus_view_class_init (BstSongBusViewClass *class)
{
  BstItemViewClass *item_view_class = BST_ITEM_VIEW_CLASS (class);
  
  item_view_class->item_type = "BseSongBus";
}

static void
bst_song_bus_view_init (BstSongBusView *self)
{
  BstItemView *iview = BST_ITEM_VIEW (self);
  /* complete GUI */
  GxkRadget *radget = gxk_radget_complete (GTK_WIDGET (self), "beast", "song-bus-view", NULL);
  /* setup tree view */
  GtkTreeView *tview = gxk_radget_find (radget, "tree-view");
  bst_item_view_complete_tree (iview, tview);
  /* create tool actions */
  gxk_widget_publish_actions (self, "song-bus-view-actions",
                              G_N_ELEMENTS (song_bus_view_actions), song_bus_view_actions,
                              NULL, song_bus_view_action_check, song_bus_view_action_exec);
  /* create property editor */
  bst_item_view_build_param_view (iview, gxk_radget_find (radget, "property-area"));
}

GtkWidget*
bst_song_bus_view_new (SfiProxy song)
{
  GtkWidget *song_bus_view;
  
  g_return_val_if_fail (BSE_IS_SONG (song), NULL);
  
  song_bus_view = gtk_widget_new (BST_TYPE_SONG_BUS_VIEW, NULL);
  bst_item_view_set_container (BST_ITEM_VIEW (song_bus_view), song);
  
  return song_bus_view;
}

static void
song_bus_view_action_exec (gpointer                data,
                           gulong                  action)
{
  BstSongBusView *self = BST_SONG_BUS_VIEW (data);
  BstItemView *item_view = BST_ITEM_VIEW (self);
  SfiProxy song = item_view->container;
  switch (action)
    {
      SfiProxy item;
    case ACTION_ADD_SONG_BUS:
      item = bse_song_create_bus (song);
      bst_item_view_select (item_view, item);
      break;
    case ACTION_DELETE_SONG_BUS:
      item = bst_item_view_get_current (item_view);
      bse_song_remove_bus (song, item);
      break;
    }
  gxk_widget_update_actions_downwards (self);
}

static gboolean
song_bus_view_action_check (gpointer                data,
                            gulong                  action)
{
  BstSongBusView *self = BST_SONG_BUS_VIEW (data);
  BstItemView *item_view = BST_ITEM_VIEW (self);
  switch (action)
    {
      SfiProxy item;
    case ACTION_ADD_SONG_BUS:
      return TRUE;
    case ACTION_DELETE_SONG_BUS:
      item = bst_item_view_get_current (item_view);
      return item != 0;
    default:
      return FALSE;
    }
}
