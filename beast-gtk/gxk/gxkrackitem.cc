/* GXK - Gtk+ Extension Kit
 * Copyright (C) 2002-2003 Tim Janik
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
#include "gxkrackitem.hh"
#include <string.h>

/* --- prototypes --- */
static void gxk_rack_item_class_init               (GxkRackItemClass *klass);
static void gxk_rack_item_init                     (GxkRackItem      *self);
static void gxk_rack_item_destroy                  (GtkObject        *object);
static void gxk_rack_item_add                      (GtkContainer     *container,
                                                    GtkWidget        *child);
static void gxk_rack_item_remove                   (GtkContainer     *container,
                                                    GtkWidget        *child);
static void gxk_rack_item_parent_set               (GtkWidget        *widget,
                                                    GtkWidget        *previous_parent);
static void gxk_rack_item_size_allocate            (GtkWidget        *widget,
                                                    GtkAllocation    *allocation);
static void gxk_rack_item_button_press             (GxkRackItem      *self,
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
                                     &type_info, GTypeFlags (0));
    }
  
  return type;
}

static void
gxk_rack_item_class_init (GxkRackItemClass *klass)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);
  
  object_class->destroy = gxk_rack_item_destroy;
  
  widget_class->parent_set = gxk_rack_item_parent_set;
  widget_class->size_allocate = gxk_rack_item_size_allocate;

  container_class->add = gxk_rack_item_add;
  container_class->remove = gxk_rack_item_remove;

  klass->button_press = gxk_rack_item_button_press;
  
  g_signal_new ("button-press",
                G_OBJECT_CLASS_TYPE (klass),
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
  self->empty_frame = TRUE;
  GTK_FRAME (self)->label_xalign = 0.5;
}

static void
gxk_rack_item_destroy (GtkObject *object)
{
  // GxkRackItem *self = GXK_RACK_ITEM (object);
  
  /* chain parent class' handler */
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gxk_rack_item_add (GtkContainer     *container,
                   GtkWidget        *child)
{
  GxkRackItem *self = GXK_RACK_ITEM (container);
  GtkWidget *widget = GTK_WIDGET (self);
  self->empty_frame = FALSE;
  if (GXK_IS_RACK_TABLE (widget->parent))
    gxk_rack_table_invalidate_child_map (GXK_RACK_TABLE (widget->parent));
  GTK_CONTAINER_CLASS (parent_class)->add (container, child);
}

static void
gxk_rack_item_remove (GtkContainer     *container,
                      GtkWidget        *child)
{
  GxkRackItem *self = GXK_RACK_ITEM (container);
  GtkWidget *widget = GTK_WIDGET (self);
  self->empty_frame = TRUE;
  if (GXK_IS_RACK_TABLE (widget->parent))
    gxk_rack_table_invalidate_child_map (GXK_RACK_TABLE (widget->parent));
  GTK_CONTAINER_CLASS (parent_class)->remove (container, child);
}

static void
update_frame (GxkRackItem *self)
{
  GtkWidget *widget = GTK_WIDGET (self);
  GxkRackTable *rtable = GXK_RACK_TABLE (widget->parent);

  if (!self->empty_frame)
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
gxk_rack_item_size_allocate (GtkWidget        *widget,
                             GtkAllocation    *assigned_allocation)
{
  GxkRackItem *self = GXK_RACK_ITEM (widget);
  /* GxkRackItem is overriding this function, because:
   * a) GtkFrame doesn't constrain the width of the label widget to the frame size
   * b) we want the frame to be centered within the rack table cell thickness
   */
  GtkFrame *frame = GTK_FRAME (widget);
  GtkBin *bin = GTK_BIN (widget);
  GtkAllocation new_allocation;
  gint cell_width = GXK_IS_RACK_TABLE (widget->parent) ? GXK_RACK_TABLE (widget->parent)->cell_width : 0;
  gint cell_height = GXK_IS_RACK_TABLE (widget->parent) ? GXK_RACK_TABLE (widget->parent)->cell_height : 0;

  widget->allocation = *assigned_allocation;

  /* center within cell thickness */
  cell_width -= widget->style->xthickness / 2;
  cell_height -= widget->style->ythickness / 2;
  if (self->empty_frame && cell_width > 0 && widget->allocation.width >= 2 * cell_width)
    {
      widget->allocation.x += cell_width / 2;
      widget->allocation.width -= cell_width;
    }
  if (self->empty_frame && cell_height > 0 && widget->allocation.height >= 2 * cell_height)
    {
      widget->allocation.y += frame->label_widget ? 0 : cell_height / 2;
      widget->allocation.height -= frame->label_widget ? cell_height / 2 : cell_height;
    }

  GTK_FRAME_GET_CLASS (frame)->compute_child_allocation (frame, &new_allocation);

  /* If the child allocation changed, that means that the frame is drawn
   * in a new place, so we must redraw the entire widget.
   */
  if (GTK_WIDGET_MAPPED (widget) &&
      (new_allocation.x != frame->child_allocation.x ||
       new_allocation.y != frame->child_allocation.y ||
       new_allocation.width != frame->child_allocation.width ||
       new_allocation.height != frame->child_allocation.height))
    gdk_window_invalidate_rect (widget->window, &widget->allocation, FALSE);

  if (bin->child && GTK_WIDGET_VISIBLE (bin->child))
    gtk_widget_size_allocate (bin->child, &new_allocation);

  frame->child_allocation = new_allocation;

  if (frame->label_widget && GTK_WIDGET_VISIBLE (frame->label_widget))
    {
      GtkRequisition child_requisition;
      GtkAllocation child_allocation;
      const int LABEL_PAD = 1, LABEL_SIDE_PAD = 2;
      gfloat xalign;

      gtk_widget_get_child_requisition (frame->label_widget, &child_requisition);

      if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_LTR)
        xalign = frame->label_xalign;
      else
        xalign = 1.0 - frame->label_xalign;

      child_allocation.x = frame->child_allocation.x + LABEL_SIDE_PAD +
                           xalign * MAX (0, (frame->child_allocation.width -
                                             child_requisition.width -
                                             2 * LABEL_PAD -
                                             2 * LABEL_SIDE_PAD)) +
                           LABEL_PAD;
      child_allocation.width = MIN (child_requisition.width,
                                    frame->child_allocation.width -
                                    // 2 * LABEL_PAD -
                                    2 * LABEL_SIDE_PAD);

      child_allocation.y = frame->child_allocation.y - child_requisition.height;
      child_allocation.height = child_requisition.height;

      gtk_widget_size_allocate (frame->label_widget, &child_allocation);
    }
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
