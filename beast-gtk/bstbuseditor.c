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
#include "bstbuseditor.h"


/* --- prototypes --- */
static void     bus_editor_action_exec           (gpointer                data,
                                                  gulong                  action);
static gboolean bus_editor_action_check          (gpointer                data,
                                                  gulong                  action);


/* --- bus actions --- */
enum {
  ACTION_ADD_BUS,
  ACTION_DELETE_BUS,
  ACTION_EDIT_BUS
};
static const GxkStockAction bus_editor_actions[] = {
  { N_("Add"),          NULL,   NULL,   ACTION_ADD_BUS,        BST_STOCK_PART },
  { N_("Delete"),       NULL,   NULL,   ACTION_DELETE_BUS,     BST_STOCK_TRASHCAN },
  { N_("Editor"),       NULL,   NULL,   ACTION_EDIT_BUS,       BST_STOCK_PART_EDITOR },
};


/* --- functions --- */
G_DEFINE_TYPE (BstBusEditor, bst_bus_editor, GTK_TYPE_ALIGNMENT);

static void
bst_bus_editor_init (BstBusEditor *self)
{
  /* complete GUI */
  GxkRadget *radget = gxk_radget_complete (GTK_WIDGET (self), "beast", "bus-editor", NULL);
  /* create tool actions */
  gxk_widget_publish_actions (self, "bus-editor-actions",
                              G_N_ELEMENTS (bus_editor_actions), bus_editor_actions,
                              NULL, bus_editor_action_check, bus_editor_action_exec);
}

static void
bst_bus_editor_destroy (GtkObject *object)
{
  BstBusEditor *self = BST_BUS_EDITOR (object);
  bst_bus_editor_set_bus (self, 0);
  GTK_OBJECT_CLASS (bst_bus_editor_parent_class)->destroy (object);
}

static void
bst_bus_editor_finalize (GObject *object)
{
  BstBusEditor *self = BST_BUS_EDITOR (object);
  bst_bus_editor_set_bus (self, 0);
  G_OBJECT_CLASS (bst_bus_editor_parent_class)->finalize (object);
}

GtkWidget*
bst_bus_editor_new (SfiProxy bus)
{
  g_return_val_if_fail (BSE_IS_BUS (bus), NULL);
  GtkWidget *widget = g_object_new (BST_TYPE_BUS_EDITOR, NULL);
  BstBusEditor *self = BST_BUS_EDITOR (widget);
  bst_bus_editor_set_bus (self, bus);
  return widget;
}

static void
bus_editor_release_item (SfiProxy      item,
                         BstBusEditor *self)
{
  g_assert (self->item == item);
  bst_bus_editor_set_bus (self, 0);
}

void
bst_bus_editor_set_bus (BstBusEditor *self,
                        SfiProxy      item)
{
  if (item)
    g_return_if_fail (BSE_IS_BUS (item));
  if (self->item)
    {
      bse_proxy_disconnect (self->item,
                            "any-signal", bus_editor_release_item, self,
                            NULL);
    }
  self->item = item;
  if (self->item)
    {
      bse_proxy_connect (self->item,
                         "signal::release", bus_editor_release_item, self,
                         NULL);
    }
}

static void
bus_editor_action_exec (gpointer data,
                        gulong   action)
{
  BstBusEditor *self = BST_BUS_EDITOR (data);
  switch (action)
    {
    case ACTION_ADD_BUS:
      break;
    case ACTION_DELETE_BUS:
      break;
    case ACTION_EDIT_BUS:
      break;
    }
  gxk_widget_update_actions_downwards (self);
}

static gboolean
bus_editor_action_check (gpointer data,
                        gulong   action)
{
  // BstBusEditor *self = BST_BUS_EDITOR (data);
  switch (action)
    {
    case ACTION_ADD_BUS:
    case ACTION_DELETE_BUS:
    case ACTION_EDIT_BUS:
      return TRUE;
    default:
      return FALSE;
    }
}

static void
bst_bus_editor_class_init (BstBusEditorClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);

  gobject_class->finalize = bst_bus_editor_finalize;
  object_class->destroy = bst_bus_editor_destroy;
}
