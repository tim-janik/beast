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
#include "bstscrollgraph.h"

#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <math.h>
#include <stdio.h>


/* --- prototypes --- */

/* --- functions --- */
G_DEFINE_TYPE (BstScrollgraph, bst_scrollgraph, GTK_TYPE_WIDGET);

static void
bst_scrollgraph_destroy (GtkObject *object)
{
  // BstScrollgraph *self = BST_SCROLLGRAPH (object);
  /* chain parent class' handler */
  GTK_OBJECT_CLASS (bst_scrollgraph_parent_class)->destroy (object);
}

static void
bst_scrollgraph_size_request (GtkWidget      *widget,
                              GtkRequisition *requisition)
{
  /* chain parent class' handler */
  GTK_WIDGET_CLASS (bst_scrollgraph_parent_class)->size_request (widget, requisition);
}

static void
bst_scrollgraph_size_allocate (GtkWidget     *widget,
                               GtkAllocation *allocation)
{
  // BstScrollgraph *self = BST_SCROLLGRAPH (object);

  /* chain parent class' handler */
  GTK_WIDGET_CLASS (bst_scrollgraph_parent_class)->size_allocate (widget, allocation);
}

static void
bst_scrollgraph_realize (GtkWidget *widget)
{
  // BstScrollgraph *self = BST_SCROLLGRAPH (object);

  /* chain parent class' handler */
  GTK_WIDGET_CLASS (bst_scrollgraph_parent_class)->realize (widget);
}

static void
bst_scrollgraph_unrealize (GtkWidget *widget)
{
  // BstScrollgraph *self = BST_SCROLLGRAPH (object);

  /* chain parent class' handler */
  GTK_WIDGET_CLASS (bst_scrollgraph_parent_class)->unrealize (widget);
}

static void
bst_scrollgraph_paint (BstScrollgraph *self)
{
#if 0
  GtkWidget *widget = GTK_WIDGET (scrollgraph);
  GdkGC *gc = widget->style->text_gc[GTK_STATE_PRELIGHT];
  gfloat xc, yc, s, c, angle, radius, dw;
  gint pointer_width = 3;
  gint line_width = 1;
  GdkPoint points[5];
  
  /* compute center */
  xc = widget->allocation.width / 2 + widget->allocation.x + scrollgraph->xofs;
  yc = widget->allocation.height / 2 + widget->allocation.y + scrollgraph->yofs;

  /* compute pointer coords */
  angle = scrollgraph->arc_start - scrollgraph->arc_dist * scrollgraph->angle_range;
  s = sin (angle);
  c = cos (angle);

  /* draw the pointer */
  radius = scrollgraph->furrow_radius;
  gdk_gc_set_line_attributes (gc, 2, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
  gdk_draw_line (widget->window,
		 widget->style->dark_gc[GTK_STATE_NORMAL],
		 xc + c * radius, yc - s * radius,
		 xc, yc);
  gdk_gc_set_line_attributes (gc, 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
  gdk_draw_line (widget->window,
		 widget->style->light_gc[GTK_STATE_NORMAL],
		 xc + c * radius, yc - s * radius,
		 xc, yc);
  gdk_gc_set_line_attributes (gc, 0, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);

  radius = scrollgraph->dot_radius;
  dw = (scrollgraph->dot_radius - scrollgraph->furrow_radius) * 0.75;
  gdk_draw_arc (widget->window, gc,
		TRUE,
		xc + c * radius - dw, yc - s * radius - dw,
		2 * dw, 2 * dw,
		0.0 * 64, 360. * 64);
  

  if (0)
    {
      points[0].x = xc + s * pointer_width + 0.5;
      points[0].y = yc + c * pointer_width + 0.5;
      points[1].x = xc - s * pointer_width + 0.5;
      points[1].y = yc - c * pointer_width + 0.5;
      points[2].x = points[1].x + c * radius + 0.5;
      points[2].y = points[1].y - s * radius + 0.5;
      points[3].x = points[0].x + c * radius + 0.5;
      points[3].y = points[0].y - s * radius + 0.5;
      points[4].x = points[0].x;
      points[4].y = points[0].y;
      
      gdk_gc_set_line_attributes (gc, line_width, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
      gdk_draw_polygon (widget->window, gc, TRUE, points, 5);
      gdk_gc_set_line_attributes (gc, 0, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
    }
#endif
}

static gint
bst_scrollgraph_expose (GtkWidget      *widget,
                        GdkEventExpose *event)
{
  // BstScrollgraph *self = BST_SCROLLGRAPH (object);

  return TRUE;
}

static gboolean
bst_scrollgraph_button_press (GtkWidget      *widget,
                              GdkEventButton *event)
{
  // BstScrollgraph *self = BST_SCROLLGRAPH (object);
  return TRUE;
}

static gboolean
bst_scrollgraph_motion_notify (GtkWidget      *widget,
                               GdkEventMotion *event)
{
  // BstScrollgraph *self = BST_SCROLLGRAPH (object);
  return TRUE;
}

static gboolean
bst_scrollgraph_button_release (GtkWidget      *widget,
                                GdkEventButton *event)
{
  // BstScrollgraph *self = BST_SCROLLGRAPH (object);
  return TRUE;
}

void
bst_scrollgraph_clear (BstScrollgraph *self)
{
  g_return_if_fail (BST_IS_SCROLLGRAPH (self));
  GtkWidget *widget = GTK_WIDGET (self);
  gtk_widget_queue_draw (widget);
}

static void
bst_scrollgraph_init (BstScrollgraph *self)
{
  GTK_WIDGET_SET_FLAGS (self, GTK_NO_WINDOW);
}

static void
bst_scrollgraph_class_init (BstScrollgraphClass *class)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  object_class->destroy = bst_scrollgraph_destroy;
  
  widget_class->size_request = bst_scrollgraph_size_request;
  widget_class->size_allocate = bst_scrollgraph_size_allocate;
  widget_class->realize = bst_scrollgraph_realize;
  widget_class->unrealize = bst_scrollgraph_unrealize;
  widget_class->expose_event = bst_scrollgraph_expose;
  widget_class->button_press_event = bst_scrollgraph_button_press;
  widget_class->button_release_event = bst_scrollgraph_button_release;
  widget_class->motion_notify_event = bst_scrollgraph_motion_notify;
}
