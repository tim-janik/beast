/* BEAST - Bedevilled Audio System
 * Copyright (C) 2003 Tim Janik
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
#include "bstsnifferscope.h"


/* --- functions --- */
G_DEFINE_TYPE (BstSnifferScope, bst_sniffer_scope, GTK_TYPE_WIDGET);

static void
bst_sniffer_scope_init (BstSnifferScope *self)
{
  GtkWidget *widget = GTK_WIDGET (self);

  GTK_WIDGET_SET_FLAGS (self, GTK_NO_WINDOW);
  gtk_widget_set_double_buffered (widget, FALSE);
  gtk_widget_show (widget);
}

GtkWidget*
bst_sniffer_scope_new (void)
{
  BstSnifferScope *self = g_object_new (BST_TYPE_SNIFFER_SCOPE, NULL);
  return GTK_WIDGET (self);
}

static void
bst_sniffer_scope_destroy (GtkObject *object)
{
  // BstSnifferScope *self = BST_SNIFFER_SCOPE (object);

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bst_sniffer_scope_finalize (GObject *object)
{
  // BstSnifferScope *self = BST_SNIFFER_SCOPE (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bst_sniffer_scope_size_request (GtkWidget      *widget,
                                GtkRequisition *requisition)
{
  // BstSnifferScope *self = BST_SNIFFER_SCOPE (widget);

  requisition->width = 20;
  requisition->height = 20;
}

static void
bst_sniffer_scope_size_allocate (GtkWidget     *widget,
                                 GtkAllocation *allocation)
{
  // BstSnifferScope *self = BST_SNIFFER_SCOPE (widget);
  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);
}

static gboolean
bst_sniffer_scope_expose (GtkWidget      *widget,
                          GdkEventExpose *event)
{
  // BstSnifferScope *self = BST_SNIFFER_SCOPE (widget);
  GdkWindow *window = event->window;
  GdkRectangle area = event->area;
  GdkGC *dark_gc = widget->style->dark_gc[widget->state];
  GtkAllocation *allocation = &widget->allocation;
  if (window != widget->window)
    return FALSE;

  /* with gtk_widget_set_double_buffered (self, FALSE) in init and
   * with gdk_window_begin_paint_region()/gdk_window_end_paint()
   * around our redraw functions, we can decide on our own on what
   * windows we want double buffering.
   */
  // gdk_window_clear_area (widget->window, area.x, area.y, area.width, area.height);
  gdk_window_begin_paint_rect (event->window, &area);
  // bst_sniffer_scope_draw_window (self, area.x, area.y, area.x + area.width, area.y + area.height);
  gdk_draw_line (window, dark_gc,
                 allocation->x,
                 allocation->y,
                 allocation->x + allocation->width,
                 allocation->y + allocation->height);
  gdk_window_end_paint (event->window);
  
  return FALSE;
}

static void
bst_sniffer_scope_class_init (BstSnifferScopeClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  gobject_class->finalize = bst_sniffer_scope_finalize;

  object_class->destroy = bst_sniffer_scope_destroy;

  widget_class->size_request = bst_sniffer_scope_size_request;
  widget_class->size_allocate = bst_sniffer_scope_size_allocate;
  widget_class->expose_event = bst_sniffer_scope_expose;
}
