/* BEAST - Better Audio System
 * Copyright (C) 2004 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "bstbusmixer.h"
#include "bstbuseditor.h"


/* --- prototypes --- */
static void     bus_mixer_action_exec           (gpointer                data,
                                                 gulong                  action);
static gboolean bus_mixer_action_check          (gpointer                data,
                                                 gulong                  action,
                                                 guint64                 action_stamp);


/* --- bus actions --- */
enum {
  ACTION_ADD_BUS,
  ACTION_DELETE_BUS,
};
static const GxkStockAction bus_mixer_actions[] = {
  { N_("Add"),          NULL,   N_("Add a new bus to the mixer"),        ACTION_ADD_BUS,        BST_STOCK_BUS_ADD },
  { N_("Delete"),       NULL,   N_("Delete the currently selected bus"), ACTION_DELETE_BUS,     BST_STOCK_TRASHCAN },
};


/* --- functions --- */
G_DEFINE_TYPE (BstBusMixer, bst_bus_mixer, BST_TYPE_ITEM_VIEW);
#define HPAD     (3)
#define HSPACING (1)
#define YPAD     (2 * HPAD)

static gboolean
canvas_box_expose_event (GtkWidget      *widget,
                         GdkEventExpose *event,
                         BstBusMixer    *self)
{
  GtkWidget *child = GTK_CONTAINER (self->hbox)->focus_child;
  if (GTK_IS_ALIGNMENT (child))
    {
      guint tpad, bpad, lpad, rpad;
      gtk_alignment_get_padding (GTK_ALIGNMENT (child), &tpad, &bpad, &lpad, &rpad);
      GtkAllocation area = { 0, 0, 0, 0 };
      gdk_window_get_size (widget->window, &area.width, &area.height);
      area.x = child->allocation.x - HPAD;
      area.width = child->allocation.width + HPAD * 2;
      GdkGC *focus_gc = widget->style->base_gc[GTK_STATE_SELECTED];
      gdk_draw_rectangle (widget->window, focus_gc, TRUE, area.x, area.y, area.width, area.height);
      gdk_draw_vline (widget->window, widget->style->light_gc[GTK_STATE_SELECTED], area.x, area.y, area.height);
      gdk_draw_vline (widget->window, widget->style->dark_gc[GTK_STATE_SELECTED], area.x + area.width - 1, area.y, area.height);
    }
  return FALSE;
}

static void
bus_mixer_add (BstBusMixer *self,
               GtkWidget   *widget)
{
  if (GTK_IS_ALIGNMENT (widget))
    g_object_set (widget, "top-padding", YPAD, "bottom-padding", YPAD, "left-padding", HPAD, "right-padding", HPAD, NULL);
  gtk_box_pack_start (self->hbox, widget, FALSE, TRUE, 0);
  gxk_widget_update_actions (self);
}

static void
bst_bus_mixer_init (BstBusMixer *self)
{
  /* complete GUI */
  GxkRadget *radget = gxk_radget_complete (GTK_WIDGET (self), "beast", "bus-mixer", NULL);
  GtkWidget *canvas = (GtkWidget*) gxk_radget_find (radget, "canvas");
  self->hbox = (GtkBox*) g_object_new (GTK_TYPE_HBOX, "visible", 1, "parent", canvas, "spacing", HSPACING, NULL);
  g_signal_connect_after (self->hbox, "set-focus-child", G_CALLBACK (gtk_widget_queue_draw), NULL);
  g_signal_connect_object (self->hbox, "set-focus-child", G_CALLBACK (gxk_widget_update_actions), self, G_CONNECT_SWAPPED | G_CONNECT_AFTER);
  g_signal_connect_after (self->hbox, "expose-event", G_CALLBACK (canvas_box_expose_event), self);
  /* create tool actions */
  gxk_widget_publish_actions (self, "bus-mixer-actions",
                              G_N_ELEMENTS (bus_mixer_actions), bus_mixer_actions,
                              NULL, bus_mixer_action_check, bus_mixer_action_exec);
  /* add description */
  GxkRadget *bdesc = gxk_radget_create ("beast", "bus-description", NULL);
  bus_mixer_add (self, (GtkWidget*) bdesc);
}

GtkWidget*
bst_bus_mixer_new (SfiProxy song)
{
  GtkWidget *bus_mixer;
  
  g_return_val_if_fail (BSE_IS_SONG (song), NULL);
  
  bus_mixer = gtk_widget_new (BST_TYPE_BUS_MIXER, NULL);
  bst_item_view_set_container (BST_ITEM_VIEW (bus_mixer), song);
  
  return bus_mixer;
}

static void
bus_mixer_item_added (SfiProxy     container,
                      SfiProxy     item,
                      BstBusMixer *self)
{
  BstItemView *iview = BST_ITEM_VIEW (self);
  if (BSE_IS_ITEM (item) && bse_proxy_is_a (item, BST_ITEM_VIEW_GET_CLASS (self)->item_type))
    {
      self->unlisteners = g_slist_prepend (self->unlisteners, (gpointer) item);
      BST_ITEM_VIEW_GET_CLASS (self)->listen_on (iview, item);
      GtkWidget *be = bst_bus_editor_new (item);
      bus_mixer_add (self, be);
    }
}

static void
bus_mixer_item_removed (SfiProxy     unused1,
                        SfiProxy     item,
                        gint         unused2,
                        BstBusMixer *self)
{
  BstItemView *iview = BST_ITEM_VIEW (self);
  if (BSE_IS_ITEM (item) && bse_proxy_is_a (item, BST_ITEM_VIEW_GET_CLASS (self)->item_type))
    {
      self->unlisteners = g_slist_remove (self->unlisteners, (gpointer) item);
      BST_ITEM_VIEW_GET_CLASS (self)->unlisten_on (iview, item);
      GList *node, *list = gtk_container_get_children (GTK_CONTAINER (self->hbox));
      for (node = list; node; node = node->next)
        if (BST_IS_BUS_EDITOR (node->data))
          {
            BstBusEditor *be = BST_BUS_EDITOR (node->data);
            if (be->item == item)
              {
                gtk_widget_destroy (GTK_WIDGET (be));
                break;
              }
          }
      g_list_free (list);
      gxk_widget_update_actions (self);
    }
}

static void
bus_mixer_set_container (BstItemView *iview,
                         SfiProxy     container)
{
  BstBusMixer *self = BST_BUS_MIXER (iview);
  if (iview->container)
    {
      while (self->unlisteners)
        {
          SfiProxy item = (SfiProxy) g_slist_pop_head (&self->unlisteners);
          bus_mixer_item_removed (0, item, 0, self);
        }
      bse_proxy_disconnect (iview->container,
                            "any-signal", bus_mixer_item_added, self,
                            "any-signal", bus_mixer_item_removed, self,
                            NULL);
    }
  BST_ITEM_VIEW_CLASS (bst_bus_mixer_parent_class)->set_container (iview, container);
  if (iview->container)
    {
      bse_proxy_connect (iview->container,
                         "signal::item_added", bus_mixer_item_added, self,
                         "signal::item_remove", bus_mixer_item_removed, self,
                         NULL);
      BseItemSeq *iseq = bse_container_list_children (iview->container);
      guint i;
      for (i = 0; i < iseq->n_items; i++)
        bus_mixer_item_added (iview->container, iseq->items[i], self);
    }
}

static void
bus_mixer_action_exec (gpointer data,
                       gulong   action)
{
  BstBusMixer *self = BST_BUS_MIXER (data);
  BstItemView *iview = BST_ITEM_VIEW (self);
  SfiProxy song = iview->container;
  switch (action)
    {
      SfiProxy item;
    case ACTION_ADD_BUS:
      bse_item_group_undo (song, "Create Bus");
      item = bse_song_create_bus (song);
      bse_bus_ensure_output (item);
      bse_item_ungroup_undo (song);
      if (item)
        bst_item_view_select (iview, item);
      break;
    case ACTION_DELETE_BUS:
      if (self->hbox && BST_IS_BUS_EDITOR (GTK_CONTAINER (self->hbox)->focus_child))
        {
          BstBusEditor *be = BST_BUS_EDITOR (GTK_CONTAINER (self->hbox)->focus_child);
          if (be->item != bse_song_get_master_bus (song))
            bse_song_remove_bus (song, be->item);
        }
      break;
    }
  gxk_widget_update_actions_downwards (self);
}

static gboolean
bus_mixer_action_check (gpointer data,
                        gulong   action,
                        guint64  action_stamp)
{
  BstBusMixer *self = BST_BUS_MIXER (data);
  BstItemView *iview = BST_ITEM_VIEW (self);
  SfiProxy song = iview->container;
  switch (action)
    {
    case ACTION_ADD_BUS:
      return TRUE;
    case ACTION_DELETE_BUS:
      if (self->hbox && BST_IS_BUS_EDITOR (GTK_CONTAINER (self->hbox)->focus_child))
        {
          BstBusEditor *be = BST_BUS_EDITOR (GTK_CONTAINER (self->hbox)->focus_child);
          if (be->item != bse_song_get_master_bus (song))
            return TRUE;
        }
      return FALSE;
    default:
      return FALSE;
    }
}

static void
bst_bus_mixer_class_init (BstBusMixerClass *klass)
{
  BstItemViewClass *iview_class = BST_ITEM_VIEW_CLASS (klass);

  iview_class->item_type = "BseBus";
  iview_class->set_container = bus_mixer_set_container;
}
