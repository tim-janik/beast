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

#define N_VALUES(scg)   ((scg)->n_points * (scg)->n_bars)
#define VALUE(scg,b,p)  (BAR (scg, b)[(p)])
#define BAR(scg,nth)    ((scg)->values + (scg)->n_points * (((scg)->bar_offset + (nth)) % (scg)->n_bars))
#define HORIZONTAL(scg) ((scg)->update_position == GTK_POS_LEFT || (scg)->update_position == GTK_POS_RIGHT)
#define VERTICAL(scg)   ((scg)->update_position == GTK_POS_TOP || (scg)->update_position == GTK_POS_BOTTOM)

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
  BstScrollgraph *self = BST_SCROLLGRAPH (widget);

  /* chain parent class' handler */
  GTK_WIDGET_CLASS (bst_scrollgraph_parent_class)->size_allocate (widget, allocation);

  gboolean need_resize = FALSE;
  if (HORIZONTAL (self))
    need_resize = widget->allocation.width != self->n_bars || widget->allocation.height != self->n_points;
  if (VERTICAL (self))
    need_resize = widget->allocation.width != self->n_points || widget->allocation.height != self->n_bars;
  if (need_resize)
    {
      self->n_points = VERTICAL (self) ? widget->allocation.width : widget->allocation.height;
      self->n_bars = HORIZONTAL (self) ? widget->allocation.width : widget->allocation.height;
      self->bar_offset %= self->n_bars;
      g_free (self->values);
      self->values = g_new0 (gfloat, N_VALUES (self));
      if (self->pixbuf)
        {
          g_object_unref (self->pixbuf);
          self->pixbuf = gdk_pixbuf_new_from_data (g_new (guint8, self->n_points * 3),
                                                   GDK_COLORSPACE_RGB, FALSE, 8,
                                                   VERTICAL (self) ? self->n_points : 1,
                                                   HORIZONTAL (self) ? self->n_points : 1,
                                                   3, (GdkPixbufDestroyNotify) g_free, NULL);
        }
      gtk_widget_queue_draw (widget);
      guint i, j;
      for (i = 0; i < self->n_bars; i++)
        for (j = 0; j < self->n_points; j++)
          VALUE (self, i, j) = (i * self->n_points + j) * 1.0 / N_VALUES (self);
    }
}

static void
bst_scrollgraph_realize (GtkWidget *widget)
{
  BstScrollgraph *self = BST_SCROLLGRAPH (widget);
  
  /* chain parent class' handler */
  GTK_WIDGET_CLASS (bst_scrollgraph_parent_class)->realize (widget);
  
  self->pixbuf = gdk_pixbuf_new_from_data (g_new (guint8, self->n_points * 3),
                                           GDK_COLORSPACE_RGB, FALSE, 8,
                                           VERTICAL (self) ? self->n_points : 1,
                                           HORIZONTAL (self) ? self->n_points : 1,
                                           3, (GdkPixbufDestroyNotify) g_free, NULL);
}

static void
bst_scrollgraph_unrealize (GtkWidget *widget)
{
  BstScrollgraph *self = BST_SCROLLGRAPH (widget);

  g_object_unref (self->pixbuf);
  self->pixbuf = NULL;

  /* chain parent class' handler */
  GTK_WIDGET_CLASS (bst_scrollgraph_parent_class)->unrealize (widget);
}

static void
bst_scrollgraph_draw_bar (BstScrollgraph *self,
                          guint           nth)
{
  GtkWidget *widget = GTK_WIDGET (self);
  GdkWindow *drawable = widget->window;
  GdkGC *gc = widget->style->white_gc;
  guint8 *rgb = gdk_pixbuf_get_pixels (self->pixbuf);
  guint i;
  for (i = 0; i < self->n_points; i++)
    {
      gfloat v = VALUE (self, nth, i);
      rgb[i * 3 + 0] = v * 255;
      rgb[i * 3 + 1] = 255 * !(((i / 16) ^ (nth / 16)) & 1);
      rgb[i * 3 + 2] = i * 1.0 / self->n_points * 255.;
    }
  gdk_pixbuf_render_to_drawable (self->pixbuf, drawable, gc, 0, 0,
                                 widget->allocation.x + (HORIZONTAL (self) ? nth : 0),
                                 widget->allocation.y + (HORIZONTAL (self) ? 0 : nth),
                                 gdk_pixbuf_get_width (self->pixbuf),
                                 gdk_pixbuf_get_height (self->pixbuf),
                                 GDK_RGB_DITHER_MAX, 0, 0);
}

static gint
bst_scrollgraph_expose (GtkWidget      *widget,
                        GdkEventExpose *event)
{
  BstScrollgraph *self = BST_SCROLLGRAPH (widget);
  GdkRectangle *areas;
  gint i, j, n_areas = 1;
  if (event->region)
    gdk_region_get_rectangles (event->region, &areas, &n_areas);
  else
    areas = g_memdup (&event->area, sizeof (event->area));
  for (j = 0; j < n_areas; j++)
    {
      const GdkRectangle *area = &areas[j];
      /* double buffering is turned off in init() */
      if (HORIZONTAL (self))
        for (i = area->x; i < area->x + area->width; i++)
          bst_scrollgraph_draw_bar (self, i - widget->allocation.x);
      if (VERTICAL (self))
        for (i = area->y; i < area->y + area->height; i++)
          bst_scrollgraph_draw_bar (self, i - widget->allocation.y);
    }
  g_free (areas);
  return FALSE;
}

void
bst_scrollgraph_clear (BstScrollgraph *self)
{
  g_return_if_fail (BST_IS_SCROLLGRAPH (self));
  GtkWidget *widget = GTK_WIDGET (self);
  gtk_widget_queue_draw (widget);
}

static gboolean
bst_scrollgraph_button_press (GtkWidget      *widget,
                              GdkEventButton *event)
{
  // BstScrollgraph *self = BST_SCROLLGRAPH (widget);
  return TRUE;
}

static gboolean
bst_scrollgraph_motion_notify (GtkWidget      *widget,
                               GdkEventMotion *event)
{
  // BstScrollgraph *self = BST_SCROLLGRAPH (widget);
  return TRUE;
}

static gboolean
bst_scrollgraph_button_release (GtkWidget      *widget,
                                GdkEventButton *event)
{
  // BstScrollgraph *self = BST_SCROLLGRAPH (widget);
  return TRUE;
}

static void
bst_scrollgraph_init (BstScrollgraph *self)
{
  GtkWidget *widget = GTK_WIDGET (self);
  GTK_WIDGET_SET_FLAGS (self, GTK_NO_WINDOW);
  self->update_position = GTK_POS_BOTTOM; // RIGHT;
  self->n_points = 1;
  self->n_bars = 1;
  self->bar_offset = 0;
  self->values = g_new0 (gfloat, N_VALUES (self));
  gtk_widget_set_double_buffered (widget, FALSE);
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
