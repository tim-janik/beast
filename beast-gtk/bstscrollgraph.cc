// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstscrollgraph.hh"
#include "bstsnifferscope.hh" // FIXME: remove include
#include "bstparam.hh"
#include <math.h>

#define N_VALUES(scg)   ((scg)->n_points * (scg)->n_bars)
#define VALUE(scg,b,p)  (BAR (scg, b)[(p)])
#define BAR(scg,nth)    ((scg)->values + (scg)->n_points * (((scg)->bar_offset + (nth)) % (scg)->n_bars))
#define HORIZONTAL(scg) ((scg)->direction == Bst::Direction::DIR_RIGHT || (scg)->direction == Bst::Direction::DIR_LEFT)
#define VERTICAL(scg)   ((scg)->direction == Bst::Direction::DIR_UP || (scg)->direction == Bst::Direction::DIR_DOWN)
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
static void     bst_scrollgraph_resize_values   (BstScrollgraph *self, Bst::Direction direction);
static void     bst_scrollgraph_io_changed      (BstScrollgraph *self);
static guint    signal_resize_values = 0;

/* --- functions --- */
G_DEFINE_TYPE (BstScrollgraph, bst_scrollgraph, GTK_TYPE_BIN);

static void
bst_scrollgraph_destroy (GtkObject *object)
{
  BstScrollgraph *self = BST_SCROLLGRAPH (object);
  bst_scrollgraph_set_source (self, Bse::SourceH(), 0);
  /* chain parent class' handler */
  GTK_OBJECT_CLASS (bst_scrollgraph_parent_class)->destroy (object);
}

static void
bst_scrollgraph_finalize (GObject *object)
{
  BstScrollgraph *self = BST_SCROLLGRAPH (object);
  g_free (self->values);
  self->values = NULL;
  /* chain parent class' handler */
  G_OBJECT_CLASS (bst_scrollgraph_parent_class)->finalize (object);
  using namespace Bse;
  self->source.~SourceH();
}

static void
bst_scrollgraph_resize_values (BstScrollgraph *self, Bst::Direction direction)
{
  g_signal_emit (self, signal_resize_values, 0, CLAMP (direction, Bst::Direction::DIR_UP, Bst::Direction::DIR_DOWN));
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
      bst_scrollgraph_resize_values (self, Aida::enum_value_from_string<Bst::Direction> (sfi_value_get_choice (value)));
      gtk_widget_queue_resize (widget);
      break;
    case PROP_BOOST:
      self->boost = sfi_value_get_real (value);
      gtk_widget_queue_draw (widget);
      break;
    case PROP_WINDOW_SIZE:
      self->window_size = int64 (Aida::enum_value_from_string<Bst::FFTSize> (sfi_value_get_choice (value)));
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
      sfi_value_set_choice (value, Aida::enum_value_to_string<Bst::Direction> (self->direction).c_str());
      break;
    case PROP_BOOST:
      sfi_value_set_real (value, self->boost);
      break;
    case PROP_WINDOW_SIZE:
      sfi_value_set_choice (value, Aida::enum_info<Bst::FFTSize>().value_to_string (self->window_size).c_str());
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

  GtkBin *bin = GTK_BIN (self);
  if (bin->child && GTK_WIDGET_VISIBLE (bin->child))
    {
      GtkRequisition child_requisition;
      gtk_widget_size_request (bin->child, &child_requisition);
    }
}

static void
scrollgraph_resize_values (BstScrollgraph *self, Bst::Direction direction)
{
  GtkWidget *widget = GTK_WIDGET (self);
  gint old_points = self->n_points;
  uint old_win_points = MIN (self->n_points, FFTSZ2POINTS (self->window_size));
  uint old_bars = self->n_bars;
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

  if (self->canvas)
    gdk_window_move_resize (self->canvas, 0, 0, allocation->width, allocation->height);

  bst_scrollgraph_resize_values (self, self->direction);

  GtkBin *bin = GTK_BIN (self);
  if (bin->child && GTK_WIDGET_VISIBLE (bin->child))
    {
      GtkAllocation child_allocation;
      child_allocation.x = 0;
      child_allocation.y = 0;
      child_allocation.width = allocation->width;
      child_allocation.height = allocation->height;
      gtk_widget_size_allocate (bin->child, &child_allocation);
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
  gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);

  attributes.x = 0;
  attributes.y = 0;
  self->canvas = gdk_window_new (widget->window, &attributes, attributes_mask);
  gdk_window_set_user_data (self->canvas, self);
  gdk_window_set_background (self->canvas, &widget->style->black);

  self->pixbuf = gdk_pixbuf_new_from_data (g_new (guint8, self->n_points * 3),
                                           GDK_COLORSPACE_RGB, FALSE, 8,
                                           VERTICAL (self) ? self->n_points : 1,
                                           HORIZONTAL (self) ? self->n_points : 1,
                                           3, (GdkPixbufDestroyNotify) g_free, NULL);

  bst_scrollgraph_io_changed (self); /* show self->canvas if appropriate */
}

static void
bst_scrollgraph_unrealize (GtkWidget *widget)
{
  BstScrollgraph *self = BST_SCROLLGRAPH (widget);

  g_object_unref (self->pixbuf);
  self->pixbuf = NULL;

  gdk_window_set_user_data (self->canvas, NULL);
  gdk_window_destroy (self->canvas);
  self->canvas = NULL;

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
      GdkWindow *drawable = self->canvas;
      if (HORIZONTAL (self))
        gdk_window_scroll (drawable, self->direction == Bst::Direction::DIR_LEFT ? -1 : 1, 0);
      else
        gdk_window_scroll (drawable, 0, self->direction == Bst::Direction::DIR_UP ? -1 : 1);
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
  GdkWindow *drawable = self->canvas;
  guint8 *rgb = gdk_pixbuf_get_pixels (self->pixbuf);
  guint i, n = nth;
  if (self->direction == Bst::Direction::DIR_LEFT || self->direction == Bst::Direction::DIR_UP)
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
  if (uint (n) < self->n_bars && (self->direction == Bst::Direction::DIR_LEFT || self->direction == Bst::Direction::DIR_UP))
    n = ((gint) self->n_bars) - 1 - n;
  return n;
}

static gint
bst_scrollgraph_expose (GtkWidget      *widget,
                        GdkEventExpose *event)
{
  BstScrollgraph *self = BST_SCROLLGRAPH (widget);
  if (event->window == self->canvas)
    {
      GdkRectangle *areas;
      gint i, j, n_areas = 1;
      if (event->region)
        gdk_region_get_rectangles (event->region, &areas, &n_areas);
      else
        areas = (GdkRectangle*) g_memdup (&event->area, sizeof (event->area));
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
  return GTK_WIDGET_CLASS (bst_scrollgraph_parent_class)->expose_event (widget, event);
}

void
bst_scrollgraph_clear (BstScrollgraph *self)
{
  assert_return (BST_IS_SCROLLGRAPH (self));
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
    if (uint (pseq->probes[i]->channel_id) == self->ochannel)
      {
        BseProbe *candidate = pseq->probes[i];
        if (candidate->probe_features->probe_fft &&
            candidate->fft_data->n_values == self->window_size)
          probe = candidate;
      }
  if (probe && probe->mix_freq != self->mix_freq && probe->mix_freq > 0)
    {
      self->mix_freq = probe->mix_freq;
      bst_scrollgraph_resize_values (self, self->direction);
    }
  if (probe && probe->probe_features->probe_fft && probe->fft_data->n_values == self->window_size)
    {
      gfloat *bar = BAR (self, self->n_bars - 1); /* update last bar */
      BseFlo4tSeq *fft = probe->fft_data;
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
  bse_probe_seq_free (pseq);
  float mix_freq = bse_source_get_mix_freq (self->source.proxy_id());
  bst_source_queue_probe_request (self->source.proxy_id(), self->ochannel, BST_SOURCE_PROBE_FFT, mix_freq / self->window_size);
}

static void
bst_scrollgraph_io_changed (BstScrollgraph *self)
{
  if (self->canvas)
    {
      if (self->source && self->source.has_output (self->ochannel))
        gdk_window_show (self->canvas);
      else
        gdk_window_hide (self->canvas);
    }
}

static void
bst_scrollgraph_release_item (SfiProxy        item,
                              BstScrollgraph *self)
{
  assert_return (self->source.proxy_id() == item);
  bst_scrollgraph_set_source (self, Bse::SourceH(), 0);
  if (self->delete_toplevel)
    gxk_toplevel_delete (GTK_WIDGET (self));
}

void
bst_scrollgraph_set_source (BstScrollgraph *self, Bse::SourceH source, uint ochannel)
{
  assert_return (BST_IS_SCROLLGRAPH (self));
  if (self->source)
    {
      bse_proxy_disconnect (self->source.proxy_id(),
                            "any-signal", bst_scrollgraph_release_item, self,
                            "any-signal", bst_scrollgraph_probes_notify, self,
                            "any-signal", bst_scrollgraph_io_changed, self,
                            NULL);
    }
  self->source = source;
  self->ochannel = ochannel;
  if (self->source)
    {
      /* setup scope */
      bse_proxy_connect (self->source.proxy_id(),
                         "signal::release", bst_scrollgraph_release_item, self,
                         "signal::probes", bst_scrollgraph_probes_notify, self,
                         "swapped-signal::io_changed", bst_scrollgraph_io_changed, self,
                         NULL);
      float mix_freq = bse_source_get_mix_freq (self->source.proxy_id());
      bst_source_queue_probe_request (self->source.proxy_id(), self->ochannel, BST_SOURCE_PROBE_FFT, mix_freq / self->window_size);
      bst_scrollgraph_io_changed (self);
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
  new (&self->source) Bse::SourceH();
  GtkWidget *widget = GTK_WIDGET (self);
  GTK_WIDGET_UNSET_FLAGS (self, GTK_NO_WINDOW);
  gtk_widget_set_double_buffered (widget, FALSE);
  self->direction = Bst::Direction::DIR_LEFT;
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
  bst_scrollgraph_resize_values (self, Bst::Direction::DIR_LEFT);
}

static void
bst_scrollgraph_class_init (BstScrollgraphClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gobject_class->set_property = bst_scrollgraph_set_property;
  gobject_class->get_property = bst_scrollgraph_get_property;
  gobject_class->finalize = bst_scrollgraph_finalize;

  object_class->destroy = bst_scrollgraph_destroy;

  widget_class->size_request = bst_scrollgraph_size_request;
  widget_class->size_allocate = bst_scrollgraph_size_allocate;
  widget_class->realize = bst_scrollgraph_realize;
  widget_class->unrealize = bst_scrollgraph_unrealize;
  widget_class->expose_event = bst_scrollgraph_expose;
  widget_class->button_press_event = bst_scrollgraph_button_press;
  widget_class->button_release_event = bst_scrollgraph_button_release;
  widget_class->motion_notify_event = bst_scrollgraph_motion_notify;

  klass->resize_values = scrollgraph_resize_values;

  bst_object_class_install_property (gobject_class, _("Spectrograph"), PROP_FLIP,
                                     sfi_pspec_bool ("flip", _("Flip Spectrum"), _("Flip Spectrum display,  interchaging low and high frequencies"),
                                                     FALSE, SFI_PARAM_STANDARD));
  bst_object_class_install_property (gobject_class, _("Spectrograph"), PROP_DIRECTION,
                                     sfi_pspec_choice ("direction", _("Direction"), _("Choose display scrolling direction"),
                                                       "BST_LEFT", Bse::choice_values_from_enum<Bst::Direction>(),
                                                       SFI_PARAM_STANDARD));
  bst_object_class_install_property (gobject_class, _("Spectrograph"), PROP_BOOST,
                                     sfi_pspec_real ("boost", _("Boost"), _("Adjust frequency level threshold"),
                                                     1.0, 1.0 / 256., 256, 0.1, SFI_PARAM_STANDARD ":scale:db-range"));
  bst_object_class_install_property (gobject_class, _("Spectrograph"), PROP_WINDOW_SIZE,
                                     sfi_pspec_choice ("window_size", _("Window Size"), _("Adjust FFT window size"), "FFT_SIZE_512",
                                                       Bse::choice_values_from_enum<Bst::FFTSize>(), SFI_PARAM_STANDARD));
  signal_resize_values = g_signal_new ("resize-values", G_OBJECT_CLASS_TYPE (klass),
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
scrollgraph_resize_rulers (BstScrollgraph *self, Bst::Direction direction, gpointer data)
{
  GtkWidget *widget = GTK_WIDGET (self);
  GtkWidget *hruler = (GtkWidget*) g_object_get_data ((GObject*) self, "BstScrollgraph-hruler");
  GtkWidget *vruler = (GtkWidget*) g_object_get_data ((GObject*) self, "BstScrollgraph-vruler");
  if (self->source.proxy_id())
    {
      gdouble secs = self->window_size / (gdouble) self->mix_freq;
      if (HORIZONTAL (self))
        {
          gdouble lower = 0, upper = widget->allocation.width * secs;
          gboolean sflip = self->direction == Bst::Direction::DIR_LEFT;
          gtk_ruler_set_range (GTK_RULER (hruler), sflip ? upper : lower, sflip ? lower : upper, 0, MAX (lower, upper));
          lower = 0, upper = self->mix_freq / 2;
          gtk_ruler_set_range (GTK_RULER (vruler), FLIP (self) ? upper : lower, FLIP (self) ? lower : upper, 0, MAX (lower, upper));
        }
      if (VERTICAL (self))
        {
          gdouble lower = 0, upper = widget->allocation.height * secs;
          gboolean sflip = self->direction == Bst::Direction::DIR_UP;
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
scrollgraph_resize_alignment (BstScrollgraph *self, Bst::Direction direction, gpointer data)
{
  GtkAlignment *alignment = GTK_ALIGNMENT (data);
  g_object_set (alignment,
                "xscale", HORIZONTAL (self) ? 1.0 : 0.0,
                "yscale", VERTICAL (self) ? 1.0 : 0.0,
                NULL);
}

GtkWidget*
bst_scrollgraph_build_dialog (GtkWidget *alive_object, Bse::SourceH source, uint ochannel)
{
  assert_return (source != NULL, NULL);

  GxkRadget *radget = gxk_radget_create ("beast", "scrollgraph-dialog", NULL);
  BstScrollgraph *scg = (BstScrollgraph*) gxk_radget_find (radget, "scrollgraph");
  if (BST_IS_SCROLLGRAPH (scg))
    {
      if (!GTK_BIN (scg)->child)
        g_object_new (GTK_TYPE_LABEL, "visible", TRUE, "parent", scg, "label", _("Unconnected Output Channel"), NULL);
      bst_scrollgraph_set_source (scg, source, ochannel);
      GxkRadget *pbox = gxk_radget_find (radget, "scrollgraph-param-box");
      if (pbox)
        pbox = gxk_radget_find_area (pbox, NULL);       /* find pbox' default child-area */
      if (GTK_IS_CONTAINER (pbox))
        {
          bst_param_create_span_gmask (scrollgraph_build_param (scg, "boost"), NULL, (GtkWidget*) pbox, 2);
          bst_param_create_col_gmask (scrollgraph_build_param (scg, "window-size"), NULL, (GtkWidget*) pbox, 0);
          bst_param_create_col_gmask (scrollgraph_build_param (scg, "direction"), NULL, (GtkWidget*) pbox, 1);
          bst_param_create_col_gmask (scrollgraph_build_param (scg, "flip"), NULL, (GtkWidget*) pbox, 2);
        }
      GtkWidget *hruler = (GtkWidget*) gxk_radget_find (radget, "hruler");
      GtkWidget *vruler = (GtkWidget*) gxk_radget_find (radget, "vruler");
      if (hruler && vruler)
        {
          g_object_set_data_full ((GObject*) scg, "BstScrollgraph-hruler", g_object_ref (hruler), g_object_unref);
          g_object_set_data_full ((GObject*) scg, "BstScrollgraph-vruler", g_object_ref (vruler), g_object_unref);
          g_signal_connect_object (scg, "resize-values", G_CALLBACK (scrollgraph_resize_rulers), NULL, G_CONNECT_AFTER);
          g_signal_connect_swapped (scg, "motion-notify-event", G_CALLBACK (GTK_WIDGET_GET_CLASS (hruler)->motion_notify_event), hruler); // HACK
          g_signal_connect_swapped (scg, "motion-notify-event", G_CALLBACK (GTK_WIDGET_GET_CLASS (vruler)->motion_notify_event), vruler); // HACK
        }
      GtkWidget *alignment = (GtkWidget*) gxk_radget_find (radget, "scrollgraph-alignment");
      if (alignment)
        g_signal_connect_object (scg, "resize-values", G_CALLBACK (scrollgraph_resize_alignment), alignment, G_CONNECT_AFTER);
    }
  GtkWidget *dialog = (GtkWidget*) gxk_dialog_new (NULL, (GtkObject*) alive_object, GxkDialogFlags (0), "Scrollgraph", (GtkWidget*) radget);
  String title = string_format ("Spectrogram: %%s (%s)", source.ochannel_label (ochannel));
  bst_window_sync_title_to_proxy (dialog, source.proxy_id(), title.c_str());
  return dialog;
}
