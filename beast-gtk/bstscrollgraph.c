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
#include "bstsnifferscope.h" // FIXME: remove include
#include <math.h>

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
  BstScrollgraph *self = BST_SCROLLGRAPH (object);
  bst_scrollgraph_set_source (self, 0, 0);
  // FIXME: delete buffers
  /* chain parent class' handler */
  GTK_OBJECT_CLASS (bst_scrollgraph_parent_class)->destroy (object);
}

static void
bst_scrollgraph_size_request (GtkWidget      *widget,
                              GtkRequisition *requisition)
{
  BstScrollgraph *self = BST_SCROLLGRAPH (widget);

  /* chain parent class' handler */
  GTK_WIDGET_CLASS (bst_scrollgraph_parent_class)->size_request (widget, requisition);

  guint length = 480;
  if (HORIZONTAL (self))
    {
      requisition->height = MAX (requisition->height, self->n_points);
      requisition->width = length;
    }
  if (VERTICAL (self))
    {
      requisition->height = length;
      requisition->width = MAX (requisition->width, self->n_points);
    }
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
          VALUE (self, i, j) = j / (float) self->n_points;
    }
}

static void
bst_scrollgraph_realize (GtkWidget *widget)
{
  BstScrollgraph *self = BST_SCROLLGRAPH (widget);
  GdkWindowAttr attributes;
  gint attributes_mask;

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= (GDK_EXPOSURE_MASK |
                            GDK_BUTTON_PRESS_MASK |
                            GDK_BUTTON_RELEASE_MASK |
                            GDK_POINTER_MOTION_MASK |
                            GDK_POINTER_MOTION_HINT_MASK);
  attributes_mask = GDK_WA_X | GDK_WA_Y;
  widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
  gdk_window_set_user_data (widget->window, self);
  widget->style = gtk_style_attach (widget->style, widget->window);

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
bst_scrollgraph_scroll_bars (BstScrollgraph *self)
{
  GtkWidget *widget = GTK_WIDGET (self);
  self->bar_offset = (self->bar_offset + self->n_bars - 1) % self->n_bars;
  if (GTK_WIDGET_DRAWABLE (widget))
    {
      GdkWindow *drawable = widget->window;
      if (HORIZONTAL (self))
        gdk_window_scroll (drawable, self->update_position == GTK_POS_RIGHT ? -1 : 1, 0);
      else
        gdk_window_scroll (drawable, 0, self->update_position == GTK_POS_BOTTOM ? -1 : 1);
    }
}

static GxkColorDots*
create_color_dots (void)
{
  static const GxkColorDot dots[] = {
    { -120, 0x000000 },
    {  -96, 0x000030 },
    {  -24, 0x00ffff },
    {    0, 0xffffff },
  };
  return gxk_color_dots_new (G_N_ELEMENTS (dots), dots);
}

static void
bst_scrollgraph_draw_bar (BstScrollgraph *self,
                          guint           nth)
{
  GtkWidget *widget = GTK_WIDGET (self);
  GdkWindow *drawable = widget->window;
  guint8 *rgb = gdk_pixbuf_get_pixels (self->pixbuf);
  guint i, n = nth;
  if (self->update_position == GTK_POS_RIGHT || self->update_position == GTK_POS_BOTTOM)
    n = self->n_bars - 1 - (nth % self->n_bars);
  GxkColorDots *cdots = create_color_dots(); // FIXME
  for (i = 0; i < self->n_points; i++)
    {
      double v = VALUE (self, nth, i);
      v = v > 0.0 ? log10 (v) : -999;
      v = MAX (v * 20, -120);
      guint col = gxk_color_dots_interpolate (cdots, v, 1.0);
      rgb[i * 3 + 0] = col >> 16;
      rgb[i * 3 + 1] = (col >> 8) & 0xff;
      rgb[i * 3 + 2] = col & 0xff;
    }
  gxk_color_dots_destroy (cdots);
  gdk_pixbuf_render_to_drawable (self->pixbuf, drawable, widget->style->white_gc, 0, 0,
                                 HORIZONTAL (self) ? n : 0,
                                 HORIZONTAL (self) ? 0 : n,
                                 gdk_pixbuf_get_width (self->pixbuf),
                                 gdk_pixbuf_get_height (self->pixbuf),
                                 GDK_RGB_DITHER_MAX, 0, 0);
}

static gint
bst_scrollgraph_bar_from_coord (BstScrollgraph *self,
                                gint            x,
                                gint            y)
{
  GtkWidget *widget = GTK_WIDGET (self);
  if (0)        /* relative to widget->window */
    {
      x -= widget->allocation.x;
      y -= widget->allocation.y;
    }
  gint n = HORIZONTAL (self) ? x : y;
  if (n < self->n_bars && (self->update_position == GTK_POS_RIGHT || self->update_position == GTK_POS_BOTTOM))
    n = ((gint) self->n_bars) - 1 - n;
  return n;
}

static gint
bst_scrollgraph_expose (GtkWidget      *widget,
                        GdkEventExpose *event)
{
  BstScrollgraph *self = BST_SCROLLGRAPH (widget);
  if (event->window == widget->window)
    {
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
              bst_scrollgraph_draw_bar (self, bst_scrollgraph_bar_from_coord (self, i, area->y));
          if (VERTICAL (self))
            for (i = area->y; i < area->y + area->height; i++)
              bst_scrollgraph_draw_bar (self, bst_scrollgraph_bar_from_coord (self, area->x, i));
        }
      g_free (areas);
    }
  return FALSE;
}

void
bst_scrollgraph_clear (BstScrollgraph *self)
{
  g_return_if_fail (BST_IS_SCROLLGRAPH (self));
  GtkWidget *widget = GTK_WIDGET (self);
  guint i, j;
  for (i = 0; i < self->n_bars; i++)
    for (j = 0; j < self->n_points; j++)
      VALUE (self, i, j) = 0;
  gtk_widget_queue_draw (widget);
}

#define FFT_SIZE 1024

static void
bst_scrollgraph_probes_notify (SfiProxy     source,
                               SfiSeq      *sseq,
                               gpointer     data)
{
  BstScrollgraph *self = BST_SCROLLGRAPH (data);
  BseProbeSeq *pseq = bse_probe_seq_from_seq (sseq);
  BseProbe *probe = NULL;
  guint i;
  for (i = 0; i < pseq->n_probes && !probe; i++)
    if (pseq->probes[i]->channel_id == self->ochannel)
      probe = pseq->probes[i];
  if (probe && probe->probe_features->probe_fft && probe->fft_data->n_values)
    {
      gfloat *bar = BAR (self, self->n_bars - 1); /* update last bar */
      SfiFBlock *fft = probe->fft_data;
      for (i = 0; i < MIN (self->n_points, fft->n_values / 2 + 1); i++)
        {
          gfloat re, im;
          if (i == 0)
            re = fft->values[0], im = 0;
          else if (i == fft->n_values / 2)
            re = fft->values[1], im = 0;
          else
            re = fft->values[i * 2], im = fft->values[i * 2 + 1];
          bar[i] = sqrt (re * re + im * im); // FIXME: speed up
        }
      bst_scrollgraph_scroll_bars (self); /* last bar becomes bar0 */
      if (GTK_WIDGET_DRAWABLE (self))
        bst_scrollgraph_draw_bar (self, 0);
    }
  bse_probe_seq_free (pseq);
  bse_source_queue_probe_request (self->source, self->ochannel, 0, 0, 0, FFT_SIZE);
}

static void
bst_scrollgraph_release_item (SfiProxy        item,
                              BstScrollgraph *self)
{
  g_assert (self->source == item);
  bst_scrollgraph_set_source (self, 0, 0);
}

void
bst_scrollgraph_set_source (BstScrollgraph *self,
                            SfiProxy        source,
                            guint           ochannel)
{
  g_return_if_fail (BST_IS_SCROLLGRAPH (self));
  if (source)
    g_return_if_fail (BSE_IS_SOURCE (source));
  if (self->source)
    {
      bse_proxy_disconnect (self->source,
                            "any_signal::probes", bst_scrollgraph_probes_notify, self,
                            NULL);
      bse_proxy_disconnect (self->source,
                            "any-signal", bst_scrollgraph_release_item, self,
                            NULL);
    }
  self->source = source;
  self->ochannel = ochannel;
  if (self->source)
    {
      bse_proxy_connect (self->source,
                         "signal::release", bst_scrollgraph_release_item, self,
                         NULL);
      /* setup scope */
      bse_proxy_connect (self->source,
                         "signal::probes", bst_scrollgraph_probes_notify, self,
                         NULL);
      bse_source_queue_probe_request (self->source, self->ochannel, 0, 0, 0, FFT_SIZE);
    }
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
  self->update_position = GTK_POS_RIGHT;
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
