/* BEAST - Bedevilled Audio System
 * Copyright (C) 2004 Tim Janik
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
#include "bstbusmixer.h"
#include "bstbuseditor.h"


/* --- prototypes --- */
static void     bus_mixer_action_exec           (gpointer                data,
                                                 gulong                  action);
static gboolean bus_mixer_action_check          (gpointer                data,
                                                 gulong                  action);


/* --- bus actions --- */
enum {
  ACTION_ADD_BUS,
};
static const GxkStockAction bus_mixer_actions[] = {
  { N_("Add"),          NULL,   NULL,   ACTION_ADD_BUS,        BST_STOCK_PART },
};


/* --- functions --- */
G_DEFINE_TYPE (BstBusMixer, bst_bus_mixer, BST_TYPE_ITEM_VIEW);

static void
bst_bus_mixer_init (BstBusMixer *self)
{
  /* complete GUI */
  GxkRadget *radget = gxk_radget_complete (GTK_WIDGET (self), "beast", "bus-mixer", NULL);
  (void) radget;
  /* create tool actions */
  gxk_widget_publish_actions (self, "bus-mixer-actions",
                              G_N_ELEMENTS (bus_mixer_actions), bus_mixer_actions,
                              NULL, bus_mixer_action_check, bus_mixer_action_exec);
  /* add description */
  GxkRadget *bdesc = gxk_radget_create ("beast", "bus-description", NULL);
  gxk_radget_add (self, NULL, bdesc);
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
      gxk_radget_add (self, NULL, be);
      gxk_widget_update_actions (self);
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
      GtkContainer *container = gxk_radget_find_area (self, NULL);      /* find bus-editor container */
      GList *node, *list = gtk_container_get_children (container);
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
      item = bse_song_create_bus (song);
      if (item)
        bst_item_view_select (iview, item);
      break;
    }
  gxk_widget_update_actions_downwards (self);
}

static gboolean
bus_mixer_action_check (gpointer data,
                        gulong   action)
{
  switch (action)
    {
    case ACTION_ADD_BUS:
      return TRUE;
    default:
      return FALSE;
    }
}

static void
bst_bus_mixer_class_init (BstBusMixerClass *class)
{
  BstItemViewClass *iview_class = BST_ITEM_VIEW_CLASS (class);

  iview_class->item_type = "BseBus";
  iview_class->set_container = bus_mixer_set_container;
}
