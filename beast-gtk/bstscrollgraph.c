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
#include "bstparam.h"
#include <math.h>

#define N_VALUES(scg)   ((scg)->n_points * (scg)->n_bars)
#define VALUE(scg,b,p)  (BAR (scg, b)[(p)])
#define BAR(scg,nth)    ((scg)->values + (scg)->n_points * (((scg)->bar_offset + (nth)) % (scg)->n_bars))
#define HORIZONTAL(scg) ((scg)->direction == BST_RIGHT || (scg)->direction == BST_LEFT)
#define VERTICAL(scg)   ((scg)->direction == BST_UP || (scg)->direction == BST_DOWN)
#define FLIP(scg)       ((scg)->flip ^ HORIZONTAL (scg))
#define FFTSZ2POINTS(k) ((k) / 2 + 1)

enum {
  PROP_0,
  PROP_FLIP,
  PROP_DIRECTION,
  PROP_BOOST,
  PROP_WINDOW_SIZE,
};

/* --- prototypes --- */
static void     bst_scrollgraph_resize_values   (BstScrollgraph *self,
                                                 BstDirection    direction);
static guint signal_resize_values = 0;

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
bst_scrollgraph_resize_values (BstScrollgraph *self,
                               BstDirection    direction)
{
  g_signal_emit (self, signal_resize_values, 0, CLAMP (direction, BST_UP, BST_DOWN));
}

static void
bst_scrollgraph_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BstScrollgraph *self = BST_SCROLLGRAPH (object);
  GtkWidget *widget = GTK_WIDGET (self);
  switch (prop_id)
    {
    case PROP_FLIP:
      self->flip = sfi_value_get_bool (value);
      bst_scrollgraph_resize_values (self, self->direction);
      gtk_widget_queue_draw (widget);
      break;
    case PROP_DIRECTION:
      bst_scrollgraph_resize_values (self, bst_direction_from_choice (sfi_value_get_choice (value)));
      gtk_widget_queue_resize (widget);
      break;
    case PROP_BOOST:
      self->boost = sfi_value_get_real (value);
      gtk_widget_queue_draw (widget);
      break;
    case PROP_WINDOW_SIZE:
      self->window_size = bst_fft_size_to_int (bst_fft_size_from_choice (sfi_value_get_choice (value)));
      bst_scrollgraph_resize_values (self, self->direction);
      gtk_widget_queue_resize (widget);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bst_scrollgraph_get_property (GObject     *object,
                              guint        prop_id,
                              GValue      *value,
                              GParamSpec  *pspec)
{
  BstScrollgraph *self = BST_SCROLLGRAPH (object);
  switch (prop_id)
    {
    case PROP_FLIP:
      sfi_value_set_bool (value, self->flip);
      break;
    case PROP_DIRECTION:
      sfi_value_set_choice (value, bst_direction_to_choice (self->direction));
      break;
    case PROP_BOOST:
      sfi_value_set_real (value, self->boost);
      break;
    case PROP_WINDOW_SIZE:
      sfi_value_set_choice (value, bst_fft_size_to_choice (bst_fft_size_from_int (self->window_size)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bst_scrollgraph_size_request (GtkWidget      *widget,
                              GtkRequisition *requisition)
{
  BstScrollgraph *self = BST_SCROLLGRAPH (widget);

  /* chain parent class' handler */
  GTK_WIDGET_CLASS (bst_scrollgraph_parent_class)->size_request (widget, requisition);

  guint length = MIN (FFTSZ2POINTS (self->window_size), 480);
  if (HORIZONTAL (self))
    {
      requisition->height = FFTSZ2POINTS (self->window_size);
      requisition->width = length;
    }
  if (VERTICAL (self))
    {
      requisition->height = length;
      requisition->width = FFTSZ2POINTS (self->window_size);
    }
}

static void
scrollgraph_resize_values (BstScrollgraph *self,
                           BstDirection    direction)
{
  GtkWidget *widget = GTK_WIDGET (self);
  gint old_points = self->n_points;
  gint old_win_points = MIN (self->n_points, FFTSZ2POINTS (self->window_size));
  gint old_bars = self->n_bars;
  gint old_offset = self->bar_offset;
  gfloat *old_values = self->values;
  self->direction = direction;
  self->n_points = VERTICAL (self) ? widget->allocation.width : widget->allocation.height;
  self->n_bars = HORIZONTAL (self) ? widget->allocation.width : widget->allocation.height;
  self->bar_offset %= self->n_bars;
  self->values = g_new0 (gfloat, N_VALUES (self));
  guint i, j;
#if 0
  /* fill background */
  for (j = 0; j < self->n_bars; j++)
    for (i = 0; i < self->n_points; i++)
      VALUE (self, j, i) = i / (float) self->n_points;
#endif
  /* copy over relicts */
  for (j = 0; j < MIN (self->n_bars, old_bars); j++)
    for (i = 0; i < MIN (self->n_points, old_win_points); i++)
      VALUE (self, j, i) = (old_values + old_points * ((old_offset + j) % old_bars))[i];
  g_free (old_values);
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
}

static void
bst_scrollgraph_size_allocate (GtkWidget     *widget,
                               GtkAllocation *allocation)
{
  BstScrollgraph *self = BST_SCROLLGRAPH (widget);

  /* chain parent class' handler */
  GTK_WIDGET_CLASS (bst_scrollgraph_parent_class)->size_allocate (widget, allocation);

  bst_scrollgraph_resize_values (self, self->direction);
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
        gdk_window_scroll (drawable, self->direction == BST_LEFT ? -1 : 1, 0);
      else
        gdk_window_scroll (drawable, 0, self->direction == BST_UP ? -1 : 1);
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
  if (self->direction == BST_LEFT || self->direction == BST_UP)
    n = self->n_bars - 1 - (nth % self->n_bars);
  GxkColorDots *cdots = create_color_dots(); // FIXME
  for (i = 0; i < self->n_points; i++)
    {
      double v = FLIP (self) ? VALUE (self, nth, self->n_points - 1 - i) : VALUE (self, nth, i);
      v *= self->boost;
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
  if (n < self->n_bars && (self->direction == BST_LEFT || self->direction == BST_UP))
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
  if (probe && probe->mix_freq != self->mix_freq && probe->mix_freq > 0)
    {
      self->mix_freq = probe->mix_freq;
      bst_scrollgraph_resize_values (self, self->direction);
    }
  if (probe && probe->probe_features->probe_fft && probe->fft_data->n_values)
    {
      gfloat *bar = BAR (self, self->n_bars - 1); /* update last bar */
      SfiFBlock *fft = probe->fft_data;
      if (self->window_size == fft->n_values)
        {
          for (i = 0; i < MIN (self->n_points, FFTSZ2POINTS (fft->n_values)); i++)
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
    }
  bse_probe_seq_free (pseq);
  bse_source_queue_probe_request (self->source, self->ochannel, 0, 0, 0, self->window_size);
}

static void
bst_scrollgraph_release_item (SfiProxy        item,
                              BstScrollgraph *self)
{
  g_assert (self->source == item);
  bst_scrollgraph_set_source (self, 0, 0);
  if (self->delete_toplevel)
    gxk_toplevel_delete (GTK_WIDGET (self));
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
      bse_source_queue_probe_request (self->source, self->ochannel, 0, 0, 0, self->window_size);
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
  gtk_widget_set_double_buffered (widget, FALSE);
  self->direction = BST_LEFT;
  self->mix_freq = 44100;
  self->boost = 1;
  self->window_size = 512;
  self->flip = FALSE;
  self->delete_toplevel = TRUE;
  self->bar_offset = 0;
  self->n_points = 0;
  self->n_bars = 0;
  self->values = NULL;
  widget->allocation.width = 10; widget->allocation.height = 10; // FIXME: initial values for _resize()
  bst_scrollgraph_resize_values (self, BST_LEFT);
}

static void
bst_scrollgraph_class_init (BstScrollgraphClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  gobject_class->set_property = bst_scrollgraph_set_property;
  gobject_class->get_property = bst_scrollgraph_get_property;

  object_class->destroy = bst_scrollgraph_destroy;
  
  widget_class->size_request = bst_scrollgraph_size_request;
  widget_class->size_allocate = bst_scrollgraph_size_allocate;
  widget_class->realize = bst_scrollgraph_realize;
  widget_class->unrealize = bst_scrollgraph_unrealize;
  widget_class->expose_event = bst_scrollgraph_expose;
  widget_class->button_press_event = bst_scrollgraph_button_press;
  widget_class->button_release_event = bst_scrollgraph_button_release;
  widget_class->motion_notify_event = bst_scrollgraph_motion_notify;

  class->resize_values = scrollgraph_resize_values;

  bst_object_class_install_property (gobject_class, _("Spectrograph"), PROP_FLIP,
                                     sfi_pspec_bool ("flip", _("Flip Spectrum"), _("Flip Spectrum display,  interchaging low and high frequencies"),
                                                     FALSE, SFI_PARAM_STANDARD));
  bst_object_class_install_property (gobject_class, _("Spectrograph"), PROP_DIRECTION,
                                     sfi_pspec_choice ("direction", _("Direction"), _("Choose display scrolling direction"),
                                                       "BST_LEFT", bst_direction_get_values(), SFI_PARAM_STANDARD));
  bst_object_class_install_property (gobject_class, _("Spectrograph"), PROP_BOOST,
                                     sfi_pspec_real ("boost", _("Boost"), _("Adjust frequency level threshold"),
                                                     1.0, 1.0 / 256., 256, 0.1, SFI_PARAM_STANDARD ":scale:db-range"));
  bst_object_class_install_property (gobject_class, _("Spectrograph"), PROP_WINDOW_SIZE,
                                     sfi_pspec_choice ("window_size", _("Window Size"), _("Adjust FFT window size"),
                                                       "BST_FFT_SIZE_512", bst_fft_size_get_values(), SFI_PARAM_STANDARD ":db-range"));
  signal_resize_values = g_signal_new ("resize-values", G_OBJECT_CLASS_TYPE (class),
                                       G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BstScrollgraphClass, resize_values),
                                       NULL, NULL,
                                       bst_marshal_NONE__INT, // HACK: should use BstDirection enum type
                                       G_TYPE_NONE, 1, G_TYPE_INT);
}

static GxkParam*
scrollgraph_build_param (BstScrollgraph *self,
                         const gchar    *property)
{
  GParamSpec *pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (self), property);
  GxkParam *param = bst_param_new_object (pspec, G_OBJECT (self));
  return param;
}

static void
scrollgraph_resize_rulers (BstScrollgraph *self,
                           BstDirection    direction,
                           gpointer        data)
{
  GtkWidget *widget = GTK_WIDGET (self);
  GtkWidget *hruler = g_object_get_data (self, "BstScrollgraph-hruler");
  GtkWidget *vruler = g_object_get_data (self, "BstScrollgraph-vruler");
  if (self->source)
    {
      gdouble secs = self->window_size / (gdouble) self->mix_freq;
      if (HORIZONTAL (self))
        {
          gdouble lower = 0, upper = widget->allocation.width * secs;
          gboolean sflip = self->direction == BST_LEFT;
          gtk_ruler_set_range (GTK_RULER (hruler), sflip ? upper : lower, sflip ? lower : upper, 0, MAX (lower, upper));
          lower = 0, upper = self->mix_freq / 2;
          gtk_ruler_set_range (GTK_RULER (vruler), FLIP (self) ? upper : lower, FLIP (self) ? lower : upper, 0, MAX (lower, upper));
        }
      if (VERTICAL (self))
        {
          gdouble lower = 0, upper = widget->allocation.height * secs;
          gboolean sflip = self->direction == BST_UP;
          gtk_ruler_set_range (GTK_RULER (vruler), sflip ? upper : lower, sflip ? lower : upper, 0, MAX (lower, upper));
          lower = 0, upper = self->mix_freq / 2;
          gtk_ruler_set_range (GTK_RULER (hruler),  FLIP (self) ? upper : lower, FLIP (self) ? lower : upper, 0, MAX (lower, upper));
        }
    }
  if (hruler->style)
    {
      gtk_widget_modify_font (hruler, NULL); /* undo previous font alterations */
      PangoFontDescription *pfd = pango_font_description_copy (hruler->style->font_desc);
      if (pango_font_description_get_size (pfd) > 8 * PANGO_SCALE)
        pango_font_description_set_size (pfd, 8 * PANGO_SCALE);
      gtk_widget_modify_font (hruler, pfd);
      pango_font_description_free (pfd);
    }
  if (vruler->style)
    {
      gtk_widget_modify_font (vruler, NULL); /* undo previous font alterations */
      PangoFontDescription *pfd = pango_font_description_copy (vruler->style->font_desc);
      if (pango_font_description_get_size (pfd) > 8 * PANGO_SCALE)
        pango_font_description_set_size (pfd, 8 * PANGO_SCALE);
      gtk_widget_modify_font (vruler, pfd);
      pango_font_description_free (pfd);
    }
}

static void
scrollgraph_resize_alignment (BstScrollgraph *self,
                              BstDirection    direction,
                              gpointer        data)
{
  GtkAlignment *alignment = GTK_ALIGNMENT (data);
  g_object_set (alignment,
                "xscale", HORIZONTAL (self) ? 1.0 : 0.0,
                "yscale", VERTICAL (self) ? 1.0 : 0.0,
                NULL);
}

GtkWidget*
bst_scrollgraph_build_dialog (GtkWidget   *alive_object,
                              SfiProxy     source,
                              guint        ochannel)
{
  g_return_val_if_fail (BSE_IS_SOURCE (source), NULL);

  GxkRadget *radget = gxk_radget_create ("beast", "scrollgraph-dialog", NULL);
  BstScrollgraph *scg = gxk_radget_find (radget, "scrollgraph");
  if (BST_IS_SCROLLGRAPH (scg))
    {
      bst_scrollgraph_set_source (scg, source, ochannel);
      GxkRadget *pbox = gxk_radget_find (radget, "scrollgraph-param-box");
      if (pbox)
        pbox = gxk_radget_find_area (pbox, NULL);       /* find pbox' default child-area */
      if (GTK_IS_CONTAINER (pbox))
        {
          bst_param_create_span_gmask (scrollgraph_build_param (scg, "boost"), NULL, pbox, 2);
          bst_param_create_col_gmask (scrollgraph_build_param (scg, "window-size"), NULL, pbox, 0);
          bst_param_create_col_gmask (scrollgraph_build_param (scg, "direction"), NULL, pbox, 1);
          bst_param_create_col_gmask (scrollgraph_build_param (scg, "flip"), NULL, pbox, 2);
        }
      GtkWidget *hruler = gxk_radget_find (radget, "hruler");
      GtkWidget *vruler = gxk_radget_find (radget, "vruler");
      if (hruler && vruler)
        {
          g_object_set_data_full (scg, "BstScrollgraph-hruler", g_object_ref (hruler), g_object_unref);
          g_object_set_data_full (scg, "BstScrollgraph-vruler", g_object_ref (vruler), g_object_unref);
          g_signal_connect_object (scg, "resize-values", G_CALLBACK (scrollgraph_resize_rulers), NULL, G_CONNECT_AFTER);
          g_signal_connect_swapped (scg, "motion-notify-event", G_CALLBACK (GTK_WIDGET_GET_CLASS (hruler)->motion_notify_event), hruler); // HACK
          g_signal_connect_swapped (scg, "motion-notify-event", G_CALLBACK (GTK_WIDGET_GET_CLASS (vruler)->motion_notify_event), vruler); // HACK
        }
      GtkWidget *alignment = gxk_radget_find (radget, "scrollgraph-alignment");
      if (alignment)
        g_signal_connect_object (scg, "resize-values", G_CALLBACK (scrollgraph_resize_alignment), alignment, G_CONNECT_AFTER);
    }
  GtkWidget *dialog = gxk_dialog_new (NULL, (GtkObject*) alive_object, GXK_DIALOG_HIDE_ON_DELETE, "Scrollgraph", radget);
  gchar *title = g_strdup_printf ("Spectrogram: %%s (%s)", bse_source_ochannel_label (source, ochannel));
  bst_window_sync_title_to_proxy (dialog, source, title);
  g_free (title);
  return dialog;
}
