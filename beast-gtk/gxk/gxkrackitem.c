/* GXK - Gtk+ Extension Kit
 * Copyright (C) 2002-2003 Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "gxkrackitem.h"
#include <string.h>

/* --- prototypes --- */
static void     gxk_rack_item_class_init         (GxkRackItemClass *klass);
static void     gxk_rack_item_init               (GxkRackItem      *self);
static void     gxk_rack_item_destroy            (GtkObject        *object);
static void     gxk_rack_item_parent_set         (GtkWidget        *widget,
                                                  GtkWidget        *previous_parent);
static void     gxk_rack_item_button_press       (GxkRackItem      *self,
                                                  GdkEventButton   *event);


/* --- static variables --- */
static gpointer  parent_class = NULL;


/* --- functions --- */
GType
gxk_rack_item_get_type (void)
{
  static GType type = 0;
  
  if (!type)
    {
      static const GTypeInfo type_info = {
        sizeof (GxkRackItemClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gxk_rack_item_class_init,
        NULL,   /* class_finalize */
        NULL,   /* class_data */
        sizeof (GxkRackItem),
        0,      /* n_preallocs */
        (GInstanceInitFunc) gxk_rack_item_init,
      };
      
      type = g_type_register_static (GTK_TYPE_FRAME,
                                     "GxkRackItem",
                                     &type_info, 0);
    }
  
  return type;
}

static void
gxk_rack_item_class_init (GxkRackItemClass *class)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  object_class->destroy = gxk_rack_item_destroy;
  
  widget_class->parent_set = gxk_rack_item_parent_set;
  
  class->button_press = gxk_rack_item_button_press;
  
  g_signal_new ("button-press",
                G_OBJECT_CLASS_TYPE (class),
                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                G_STRUCT_OFFSET (GxkRackItemClass, button_press),
                NULL, NULL,
                gxk_marshal_NONE__BOXED,
                G_TYPE_NONE, 1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
}

static void
gxk_rack_item_init (GxkRackItem *self)
{
  g_object_set (self,
                "visible", TRUE,
                NULL);
  self->col = self->row = -1;
  self->hspan = self->vspan = 0;
  self->empty_frame = FALSE;
}

static void
gxk_rack_item_destroy (GtkObject *object)
{
  // GxkRackItem *self = GXK_RACK_ITEM (object);
  
  /* chain parent class' handler */
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
update_frame (GxkRackItem *self)
{
  GtkWidget *widget = GTK_WIDGET (self);
  GxkRackTable *rtable = GXK_RACK_TABLE (widget->parent);
  
  g_object_set (self,
                "shadow_type", rtable->editor ? GTK_SHADOW_ETCHED_OUT : GTK_SHADOW_NONE,
                NULL);
}

static void
gxk_rack_item_parent_set (GtkWidget *widget,
                          GtkWidget *previous_parent)
{
  GxkRackItem *self = GXK_RACK_ITEM (widget);
  if (GXK_IS_RACK_TABLE (widget->parent))
    {
      g_object_connect (widget->parent, "swapped_signal::edit-mode-changed", update_frame, self, NULL);
      update_frame (self);
    }
  else if (GXK_IS_RACK_TABLE (previous_parent))
    g_object_disconnect (previous_parent, "any_signal", update_frame, self, NULL);
  
  /* chain parent class' handler */
  if (GTK_WIDGET_CLASS (parent_class)->parent_set)
    GTK_WIDGET_CLASS (parent_class)->parent_set (widget, previous_parent);
}

static void
gxk_rack_item_button_press (GxkRackItem    *self,
                            GdkEventButton *event)
{
  if (event->button == 3)
    {
    }
  g_print ("rack-item, button-%u pressed\n", event->button);
}

void
gxk_rack_item_gui_changed (GxkRackItem *self)
{
  g_return_if_fail (GXK_IS_RACK_ITEM (self));
}

gboolean
gxk_rack_item_set_area (GxkRackItem    *self,
                        gint            col,
                        gint            row,
                        gint            hspan,
                        gint            vspan)
{
  GtkWidget *widget = GTK_WIDGET (self);
  GtkContainer *table;
  gboolean hchanged = FALSE, vchanged = FALSE;
  table = GXK_IS_RACK_TABLE (widget->parent) ? GTK_CONTAINER (widget->parent) : NULL;
  if (col != self->col && col >= 0)
    {
      self->col = col;
      if (table)
        gtk_container_child_set (table, widget, "left-attach", self->col, NULL);
      hchanged = TRUE;
    }
  if (row != self->row && row >= 0)
    {
      self->row = row;
      if (table)
        gtk_container_child_set (table, widget, "top-attach", self->row, NULL);
      vchanged = TRUE;
    }
  if ((hchanged || hspan != self->hspan) && hspan > 0)
    {
      self->hspan = hspan;
      if (table)
        {
          gint base = self->col;
          if (base < 0)
            gtk_container_child_get (table, widget, "left-attach", &base, NULL);
          gtk_container_child_set (table, widget, "right-attach", base + self->hspan, NULL);
        }
      hchanged = TRUE;
    }
  if ((vchanged || vspan != self->vspan) && vspan > 0)
    {
      self->vspan = vspan;
      if (table)
        {
          gint base = self->row;
          if (base < 0)
            gtk_container_child_get (table, widget, "top-attach", &base, NULL);
          gtk_container_child_set (table, widget, "bottom-attach", base + self->vspan, NULL);
        }
      vchanged = TRUE;
    }
  if ((hchanged || vchanged) && table)
    gxk_rack_table_invalidate_child_map (GXK_RACK_TABLE (table));
  return hchanged || vchanged;
}
