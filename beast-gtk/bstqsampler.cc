// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstqsampler.hh"

#include "bstutils.hh"
#include <string.h>


/* --- properties --- */
enum {
  PROP_0,
  PROP_ZOOM,
  PROP_VSCALE,
  PROP_DRAW_MODE
};


/* --- prototypes --- */
static void	bst_qsampler_destroy		(GtkObject		*object);
static void	bst_qsampler_finalize		(GObject		*object);
static void	bst_qsampler_set_property	(GObject		*object,
						 guint			 prop_id,
						 const GValue		*value,
						 GParamSpec		*pspec);
static void	bst_qsampler_get_property	(GObject		*object,
						 guint			 prop_id,
						 GValue			*value,
						 GParamSpec		*pspec);
static void	bst_qsampler_realize		(GtkWidget		*widget);
static void	bst_qsampler_size_request	(GtkWidget		*widget,
						 GtkRequisition		*requisition);
static void	bst_qsampler_size_allocate	(GtkWidget		*widget,
						 GtkAllocation		*allocation);
static void	bst_qsampler_style_set		(GtkWidget		*widget,
						 GtkStyle		*previous_style);
static void	bst_qsampler_unrealize		(GtkWidget		*widget);
static gint	bst_qsampler_expose		(GtkWidget		*widget,
						 GdkEventExpose		*event);
static void	bst_qsampler_resize		(BstQSampler		*qsampler);
static void	bst_qsampler_move_to		(BstQSampler		*qsampler,
						 guint			 peak_offset);
static void	bst_qsampler_update_types	(BstQSampler		*qsampler,
						 guint			 pcm_offset,
						 guint			 pcm_length);
static void	bst_qsampler_update_adjustment	(BstQSampler		*qsampler);
static void	bst_qsampler_invalidate		(BstQSampler		*qsampler,
						 guint			 offset,
						 guint			 length,
						 gboolean		 reset);
static void	bst_qsampler_redraw		(BstQSampler		*qsampler,
						 gboolean		 draw_dirty);
static void	bst_qsampler_queue_refresh	(BstQSampler		*qsampler);


/* --- static variables --- */
static GSList  *refresh_widgets = NULL;
static GSList  *tmp_refresh_widgets = NULL;
static gint     refresh_handler = 0;


/* --- functions --- */
G_DEFINE_TYPE (BstQSampler, bst_qsampler, GTK_TYPE_WIDGET);

static void
bst_qsampler_class_init (BstQSamplerClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gobject_class->finalize = bst_qsampler_finalize;
  gobject_class->set_property = bst_qsampler_set_property;
  gobject_class->get_property = bst_qsampler_get_property;

  object_class->destroy = bst_qsampler_destroy;

  widget_class->realize = bst_qsampler_realize;
  widget_class->size_request = bst_qsampler_size_request;
  widget_class->size_allocate = bst_qsampler_size_allocate;
  widget_class->style_set = bst_qsampler_style_set;
  widget_class->unrealize = bst_qsampler_unrealize;
  widget_class->expose_event = bst_qsampler_expose;

  g_object_class_install_property (G_OBJECT_CLASS (object_class),
				   PROP_ZOOM,
				   g_param_spec_double ("zoom", "Zoom [%]", NULL,
							0, G_MAXINT, 100,
							G_PARAM_READWRITE));
  g_object_class_install_property (G_OBJECT_CLASS (object_class),
				   PROP_VSCALE,
				   g_param_spec_double ("vscale", "VScale [%]", NULL,
							0, G_MAXINT, 100,
							G_PARAM_READWRITE));
  g_object_class_install_property (G_OBJECT_CLASS (object_class),
				   PROP_DRAW_MODE,
				   g_param_spec_enum ("draw_mode", "Draw Mode", NULL,
						      BST_TYPE_QSAMPLER_DRAW_MODE,
						      BST_QSAMPLER_DRAW_CRANGE,
						      G_PARAM_READWRITE));
}

static void
bst_qsampler_init (BstQSampler *qsampler)
{
  GdkColor default_red   = { 0, 0xe130,      0,      0 };
  GdkColor default_green = { 0,      0, 0xe130,      0 };

  qsampler->peak_length = 0;
  qsampler->peak_offset = 0;
  qsampler->n_peaks = 0;
  qsampler->peaks = NULL;
  qsampler->n_pixels = 0;

  qsampler->zoom_factor = 1.0;
  qsampler->vscale_factor = 1.0;
  qsampler->marks = NULL;
  qsampler->regions = NULL;
  qsampler->red = default_red;
  qsampler->green = default_green;
  qsampler->red_gc = NULL;
  qsampler->green_gc = NULL;
  qsampler->draw_mode = BST_QSAMPLER_DRAW_CRANGE;
  qsampler->src_filler = NULL;
  qsampler->src_data = NULL;
  qsampler->src_destroy = NULL;
  qsampler->expose_frame = FALSE;
  qsampler->refresh_queued = FALSE;
  bst_qsampler_resize (qsampler);
  gtk_widget_set_double_buffered (GTK_WIDGET (qsampler), FALSE);
}

static void
bst_qsampler_size_request (GtkWidget      *widget,
			   GtkRequisition *requisition)
{
  /* BstQSampler *qsampler = BST_QSAMPLER (widget); */

  /* can just guess constantly */
  requisition->width = 32;
  requisition->height = widget->style->ythickness * 2;
}

static void
bst_qsampler_size_allocate (GtkWidget     *widget,
			    GtkAllocation *allocation)
{
  BstQSampler *qsampler = BST_QSAMPLER (widget);

  GTK_WIDGET_CLASS (bst_qsampler_parent_class)->size_allocate (widget, allocation);

  if (GTK_WIDGET_REALIZED (widget))
    gdk_window_move_resize (qsampler->canvas,
			    widget->style->xthickness,
			    widget->style->ythickness,
			    widget->allocation.width - 2 * widget->style->xthickness,
			    widget->allocation.height - 2 * widget->style->ythickness);

  qsampler->n_pixels = widget->allocation.width - 2 * widget->style->xthickness;
  bst_qsampler_resize (qsampler);
}

static void
bst_qsampler_realize (GtkWidget *widget)
{
  BstQSampler *qsampler = BST_QSAMPLER (widget);
  GdkGCValuesMask gc_values_mask;
  GdkGCValues gc_values;
  GdkWindowAttr attributes;
  gint attributes_mask;

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes.event_mask = GDK_EXPOSURE_MASK;
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
  gdk_window_set_user_data (widget->window, qsampler);
  widget->style = gtk_style_attach (widget->style, widget->window);
  gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
  attributes.x = widget->style->xthickness;
  attributes.y = widget->style->ythickness;
  attributes.width = widget->allocation.width - 2 * widget->style->xthickness;
  attributes.height = widget->allocation.height - 2 * widget->style->ythickness;
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= (GDK_EXPOSURE_MASK |
			    GDK_BUTTON_PRESS_MASK |
			    GDK_BUTTON_RELEASE_MASK);
  qsampler->canvas = gdk_window_new (widget->window, &attributes, attributes_mask);
  gdk_window_set_user_data (qsampler->canvas, qsampler);
  widget->style = gtk_style_attach (widget->style, qsampler->canvas);
  gtk_style_set_background (widget->style, qsampler->canvas, GTK_STATE_NORMAL);
  gdk_window_show (qsampler->canvas);

  if (!gdk_color_alloc (widget->style->colormap, &qsampler->red))
    g_warning ("unable to allocate color: { %d, %d, %d }",
	       qsampler->red.red, qsampler->red.green, qsampler->red.blue);
  if (!gdk_color_alloc (widget->style->colormap, &qsampler->green))
    g_warning ("unable to allocate color: { %d, %d, %d }",
	       qsampler->green.red, qsampler->green.green, qsampler->green.blue);
  gc_values_mask = GDK_GC_FOREGROUND;
  gc_values.foreground = qsampler->red;
  qsampler->red_gc = gtk_gc_get (widget->style->depth, widget->style->colormap, &gc_values, gc_values_mask);
  gc_values.foreground = qsampler->green;
  qsampler->green_gc = gtk_gc_get (widget->style->depth, widget->style->colormap, &gc_values, gc_values_mask);
}

static void
bst_qsampler_style_set (GtkWidget *widget,
			GtkStyle  *previous_style)
{
  BstQSampler *qsampler = BST_QSAMPLER (widget);

  GTK_WIDGET_CLASS (bst_qsampler_parent_class)->style_set (widget, previous_style);

  if (GTK_WIDGET_REALIZED (qsampler))
    {
      GdkGCValuesMask gc_values_mask;
      GdkGCValues gc_values;

      gdk_window_move_resize (qsampler->canvas,
			      widget->style->xthickness,
			      widget->style->ythickness,
			      widget->allocation.width - 2 * widget->style->xthickness,
			      widget->allocation.height - 2 * widget->style->ythickness);
      gdk_colormap_free_colors (previous_style->colormap, &qsampler->red, 1);
      gdk_colormap_free_colors (previous_style->colormap, &qsampler->green, 1);
      gtk_gc_release (qsampler->red_gc);
      gtk_gc_release (qsampler->green_gc);
      if (!gdk_color_alloc (widget->style->colormap, &qsampler->red))
	g_warning ("unable to allocate color: { %d, %d, %d }",
		   qsampler->red.red, qsampler->red.green, qsampler->red.blue);
      if (!gdk_color_alloc (widget->style->colormap, &qsampler->green))
	g_warning ("unable to allocate color: { %d, %d, %d }",
		   qsampler->green.red, qsampler->green.green, qsampler->green.blue);
      gc_values_mask = GDK_GC_FOREGROUND;
      gc_values.foreground = qsampler->red;
      qsampler->red_gc = gtk_gc_get (widget->style->depth, widget->style->colormap, &gc_values, gc_values_mask);
      gc_values.foreground = qsampler->green;
      qsampler->green_gc = gtk_gc_get (widget->style->depth, widget->style->colormap, &gc_values, gc_values_mask);
      gc_values_mask = GDK_GC_EXPOSURES;
      gc_values.graphics_exposures = TRUE;
    }
  qsampler->n_pixels = widget->allocation.width - 2 * widget->style->xthickness;
}

static void
bst_qsampler_unrealize (GtkWidget *widget)
{
  BstQSampler *qsampler = BST_QSAMPLER (widget);

  gdk_colormap_free_colors (widget->style->colormap, &qsampler->red, 1);
  gdk_colormap_free_colors (widget->style->colormap, &qsampler->green, 1);
  gtk_gc_release (qsampler->red_gc);
  gtk_gc_release (qsampler->green_gc);
  qsampler->red_gc = NULL;
  qsampler->green_gc = NULL;
  gdk_window_set_user_data (qsampler->canvas, NULL);
  gdk_window_destroy (qsampler->canvas);
  qsampler->canvas = NULL;
  GTK_WIDGET_CLASS (bst_qsampler_parent_class)->unrealize (widget);
}

static gint
bst_qsampler_expose (GtkWidget      *widget,
		     GdkEventExpose *event)
{
  BstQSampler *qsampler = BST_QSAMPLER (widget);

  if (GTK_WIDGET_DRAWABLE (qsampler))
    {
      qsampler->expose_frame |= event->window == widget->window;
      if (event->window == qsampler->canvas)
	bst_qsampler_invalidate (qsampler, event->area.x, event->area.width, FALSE);
      bst_qsampler_queue_refresh (qsampler);
    }
  return TRUE;
}

void
bst_qsampler_get_bounds (BstQSampler *qsampler,
			 gint        *first_offset,
			 gint        *last_offset)
{
  guint ostart, oend;

  g_return_if_fail (BST_IS_QSAMPLER (qsampler));

  // GtkWidget *widget = GTK_WIDGET (qsampler);

  ostart = qsampler->peak_offset;
  oend = ostart + qsampler->n_pixels - 1;

  /* return visible pcm area */

  if (first_offset)
    *first_offset = ostart * qsampler->zoom_factor;
  if (last_offset)
    *last_offset = oend * qsampler->zoom_factor;
}

gboolean
bst_qsampler_get_offset_at (BstQSampler *qsampler,
			    gint        *x_coord_p)
{
  gboolean fits_visible = FALSE;
  gint x_coord = 0;

  g_return_val_if_fail (BST_IS_QSAMPLER (qsampler), FALSE);
  g_return_val_if_fail (x_coord_p != NULL, FALSE);

  if (GTK_WIDGET_DRAWABLE (qsampler))
    {
      x_coord = *x_coord_p;

      /* translate to peaks */
      fits_visible = x_coord >= 0 && uint (x_coord) < qsampler->n_pixels;

      /* translate to pcm offset */
      x_coord += qsampler->peak_offset;
      x_coord *= qsampler->zoom_factor;
    }
  *x_coord_p = x_coord;

  return fits_visible;
}

static void
bst_qsampler_destroy (GtkObject *object)
{
  BstQSampler *qsampler = BST_QSAMPLER (object);

  bst_qsampler_set_source (qsampler, 0, NULL, NULL, NULL);
  bst_qsampler_set_adjustment (qsampler, NULL);

  GTK_OBJECT_CLASS (bst_qsampler_parent_class)->destroy (object);
}

static void
bst_qsampler_finalize (GObject *object)
{
  BstQSampler *qsampler = BST_QSAMPLER (object);

  g_free (qsampler->peaks);
  g_free (qsampler->marks);
  g_free (qsampler->regions);
  if (qsampler->refresh_queued)
    {
      refresh_widgets = g_slist_remove (refresh_widgets, qsampler);
      tmp_refresh_widgets = g_slist_remove (tmp_refresh_widgets, qsampler);
    }
  else
    qsampler->refresh_queued = TRUE;

  G_OBJECT_CLASS (bst_qsampler_parent_class)->finalize (object);
}

static void
bst_qsampler_set_property (GObject      *object,
			   guint         prop_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
  BstQSampler *qsampler = BST_QSAMPLER (object);

  switch (prop_id)
    {
    case PROP_ZOOM:
      bst_qsampler_set_zoom (qsampler, g_value_get_double (value));
      break;
    case PROP_VSCALE:
      bst_qsampler_set_vscale (qsampler, g_value_get_double (value));
      break;
    case PROP_DRAW_MODE:
      bst_qsampler_set_draw_mode (qsampler, (BstQSamplerDrawMode) g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bst_qsampler_get_property (GObject    *object,
			   guint       prop_id,
			   GValue     *value,
			   GParamSpec *pspec)
{
  BstQSampler *qsampler = BST_QSAMPLER (object);

  switch (prop_id)
    {
    case PROP_ZOOM:
      g_value_set_double (value, qsampler->zoom_factor * 100.0);
      break;
    case PROP_VSCALE:
      g_value_set_double (value, qsampler->vscale_factor * 100.0);
      break;
    case PROP_DRAW_MODE:
      g_value_set_enum (value, qsampler->draw_mode);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static guint
last_peek_from_pcm_region (BstQSampler *qsampler,
			   guint        pcm_offset,
			   guint	pcm_length)
{
  guint last = (pcm_offset + pcm_length - 1) / qsampler->zoom_factor;
  guint bound = (pcm_offset + pcm_length) / qsampler->zoom_factor;

  /* depending on zoom factor, a pcm_length of 1 spawns multiple peaks */

  return MAX (last, MAX (bound, 1) - 1);
}

static void
bst_qsampler_update_types (BstQSampler *qsampler,
			   guint	pcm_offset,
			   guint	pcm_length)
{
  guint8 *types;
  guint start = pcm_offset / qsampler->zoom_factor; /* first peek from pcm region */
  guint end = last_peek_from_pcm_region (qsampler, pcm_offset, MAX (pcm_length, 1));
  guint i, n, s, e, changed;

  pcm_length = MAX (pcm_length, 1);

  /* intersect with cached area */
  start = MAX (start, qsampler->peak_offset);
  end = MIN (end, qsampler->peak_offset + qsampler->n_peaks - 1);

  if (end < start)
    return;

  types = g_new0 (guint8, end - start + 1);

  /* merge regions */
  for (n = 0; n < qsampler->n_regions; n++)
    {
      BstQSamplerRegion *r = qsampler->regions + n;
      guint rstart = r->offset / qsampler->zoom_factor;
      guint rend = last_peek_from_pcm_region (qsampler, r->offset, r->length);

      g_assert (rstart <= rend);

      /* intersect */
      s = MAX (start, rstart);
      e = MIN (end, rend);

      /* merge region flags */
      for (i = s; i <= e; i++)
	types[i - start] |= r->type;
    }

  /* merge marks */
  for (n = 0; n < qsampler->n_marks; n++)
    {
      BstQSamplerMark *m = qsampler->marks + n;
      guint mstart = m->offset / qsampler->zoom_factor;
      guint mend = last_peek_from_pcm_region (qsampler, m->offset, 1);

      g_assert (mstart <= mend);

      /* intersect */
      s = MAX (start, mstart);
      e = MIN (end, mend);

      /* merge mark flags */
      for (i = s; i <= e; i++)
	types[i - start] |= m->type;
    }

  /* apply changes */
  start -= qsampler->peak_offset;
  end -= qsampler->peak_offset;
  changed = end + 1;
  for (i = start; i <= end; i++)
    if ((qsampler->peaks[i].type & BST_QSAMPLER_MASK) != types[i - start])
      {
	qsampler->peaks[i].type &= ~BST_QSAMPLER_MASK;
	qsampler->peaks[i].type |= types[i - start] | BST_QSAMPLER_NEEDS_DRAW;
	changed = i;
      }
  g_free (types);

  /* make sure we have an expose handler */
  if (changed <= end)
    bst_qsampler_invalidate (qsampler, changed, 1, FALSE);
}

static gboolean
bst_qsampler_reload (BstQSampler *qsampler)
{
  guint span, i;

  for (i = 0; i < qsampler->n_peaks; i++)
    if (qsampler->peaks[i].type & BST_QSAMPLER_DIRTY)
      break;
  for (span = i; span < qsampler->n_peaks; span++)
    if (!(qsampler->peaks[i].type & BST_QSAMPLER_DIRTY))
      break;
  span -= i;
  if (span)
    {
      guint pcm_offset = (qsampler->peak_offset + i) * qsampler->zoom_factor;
      guint block_length = CLAMP (1 * qsampler->zoom_factor, 1, 1024);
      BstQSamplerPeak values[256];
      guint start = i;

      span = MIN (span, 256);
      if (qsampler->src_filler)
	span = qsampler->src_filler (qsampler->src_data,
				     pcm_offset, qsampler->zoom_factor,
				     block_length, span, values,
				     qsampler);
      else
	memset (values, 0, span * sizeof (values[0]));

      for (i = 0; i < span; i++)
	{
	  qsampler->peaks[start + i].max = values[i].max;
	  qsampler->peaks[start + i].min = values[i].min;
	  qsampler->peaks[start + i].type &= ~BST_QSAMPLER_DIRTY;
	  qsampler->peaks[start + i].type |= BST_QSAMPLER_NEEDS_DRAW;
	  if (start + i > 0)
	    qsampler->peaks[start + i - 1].type |= BST_QSAMPLER_NEEDS_DRAW;
	  if (start + i + 1 < qsampler->n_peaks)
	    qsampler->peaks[start + i + 1].type |= BST_QSAMPLER_NEEDS_DRAW;
	}
      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
bst_qsampler_refresh_idler (gpointer data)
{
  GSList *slist;

  GDK_THREADS_ENTER ();
  for (slist = refresh_widgets; slist; slist = refresh_widgets)
    {
      BstQSampler *qsampler = (BstQSampler*) slist->data;

      refresh_widgets = slist->next;
      slist->next = tmp_refresh_widgets;
      tmp_refresh_widgets = slist;
      if (GTK_WIDGET_DRAWABLE (qsampler))
	bst_qsampler_redraw (qsampler, FALSE);
    }
  refresh_widgets = tmp_refresh_widgets;
  tmp_refresh_widgets = NULL;
  for (slist = refresh_widgets; slist; slist = refresh_widgets)
    {
      BstQSampler *qsampler = (BstQSampler*) slist->data;

      refresh_widgets = slist->next;
      slist->next = tmp_refresh_widgets;
      tmp_refresh_widgets = slist;
      qsampler->invalid_remains = bst_qsampler_reload (qsampler);
    }
  refresh_widgets = tmp_refresh_widgets;
  tmp_refresh_widgets = NULL;
  for (slist = refresh_widgets; slist; slist = refresh_widgets)
    {
      BstQSampler *qsampler = (BstQSampler*) slist->data;

      refresh_widgets = slist->next;
      if (qsampler->invalid_remains)
	{
	  slist->next = tmp_refresh_widgets;
	  tmp_refresh_widgets = slist;
	}
      else
	{
	  g_slist_free_1 (slist);
	  qsampler->refresh_queued = FALSE;
	}
      if (GTK_WIDGET_DRAWABLE (qsampler))
	bst_qsampler_redraw (qsampler, TRUE);
    }
  refresh_widgets = tmp_refresh_widgets;
  tmp_refresh_widgets = NULL;
  if (!refresh_widgets)
    refresh_handler = 0;
  GDK_THREADS_LEAVE ();

  return refresh_handler != 0;
}

static void
bst_qsampler_queue_refresh (BstQSampler *qsampler)
{
  if (!qsampler->refresh_queued)
    {
      refresh_widgets = g_slist_prepend (refresh_widgets, qsampler);
      if (!refresh_handler)
	refresh_handler = g_idle_add_full (GTK_PRIORITY_REDRAW,
					   bst_qsampler_refresh_idler,
					   NULL,
					   NULL);
      qsampler->refresh_queued = TRUE;
      qsampler->invalid_remains = TRUE;
    }
}

static void
bst_qsampler_invalidate (BstQSampler *qsampler,
			 guint        offset,
			 guint	      length,
			 gboolean     reset)
{
  guint i, bound;

  if (reset)
    {
      bound = qsampler->peak_length - MIN (qsampler->peak_offset, qsampler->peak_length);
      bound = MIN (bound, offset + length);
      bound = MIN (bound, qsampler->n_peaks);
      for (i = offset; i < bound; i++)
	{
	  qsampler->peaks[i].min = 0;
	  qsampler->peaks[i].max = 0;
	  qsampler->peaks[i].type = BST_QSAMPLER_DIRTY | BST_QSAMPLER_NEEDS_DRAW;
	}
      bound = MIN (qsampler->n_peaks, offset + length);
      for (; i < bound; i++)
	{
	  qsampler->peaks[i].min = 0;
	  qsampler->peaks[i].max = 0;
	  qsampler->peaks[i].type = BST_QSAMPLER_SKIP | BST_QSAMPLER_NEEDS_DRAW;
	}
    }
  else
    {
      bound = MIN (qsampler->n_peaks, offset + length);
      for (i = offset; i < bound; i++)
	qsampler->peaks[i].type |= BST_QSAMPLER_NEEDS_DRAW;
    }
  if (offset < qsampler->n_peaks && length)
    bst_qsampler_queue_refresh (qsampler);
}

static void
bst_qsampler_resize (BstQSampler *qsampler)
{
  GtkWidget *widget = GTK_WIDGET (qsampler);

  qsampler->peak_length = qsampler->pcm_length / qsampler->zoom_factor;
  qsampler->n_peaks = widget->allocation.width + widget->allocation.width;
  qsampler->peaks = g_renew (BstQSamplerTPeak, qsampler->peaks, qsampler->n_peaks);
  bst_qsampler_invalidate (qsampler, 0, qsampler->n_peaks, TRUE);
  bst_qsampler_update_types (qsampler, 0, qsampler->pcm_length);

  /* adjust page size */
  qsampler->ignore_adjustment = TRUE;
  bst_qsampler_update_adjustment (qsampler);
  qsampler->ignore_adjustment = FALSE;
  bst_qsampler_queue_refresh (qsampler);
}

static void
bst_qsampler_copy_area (BstQSampler *qsampler,
			guint        from,
			guint	     to)
{
  guint length = MAX (from, to);

  if (GTK_WIDGET_DRAWABLE (qsampler) && length < qsampler->n_pixels)
    gdk_window_scroll (qsampler->canvas, from - to, 0);
  else
    bst_qsampler_invalidate (qsampler, from, qsampler->n_peaks - length, FALSE);
}

static void
bst_qsampler_move_to (BstQSampler *qsampler,
		      guint        peak_offset)
{
  /* move contents */
  if (peak_offset >= qsampler->peak_offset + qsampler->n_peaks ||
      peak_offset + qsampler->n_peaks <= qsampler->peak_offset)
    {
      qsampler->peak_offset = peak_offset;
      bst_qsampler_invalidate (qsampler, 0, qsampler->n_peaks, TRUE);
      bst_qsampler_update_types (qsampler, 0, qsampler->pcm_length);
    }
  else if (peak_offset > qsampler->peak_offset)
    {
      guint diff = peak_offset - qsampler->peak_offset;

      g_memmove (qsampler->peaks, qsampler->peaks + diff, (qsampler->n_peaks - diff) * sizeof (qsampler->peaks[0]));
      qsampler->peak_offset = peak_offset;
      bst_qsampler_invalidate (qsampler, (qsampler->n_peaks - diff), qsampler->n_peaks, TRUE);
      bst_qsampler_update_types (qsampler,
				 (qsampler->peak_offset + qsampler->n_peaks - diff) * qsampler->zoom_factor,
				 qsampler->pcm_length);
      bst_qsampler_copy_area (qsampler, 0, diff);
    }
  else if (peak_offset < qsampler->peak_offset)
    {
      guint diff = qsampler->peak_offset - peak_offset;

      g_memmove (qsampler->peaks + diff, qsampler->peaks, (qsampler->n_peaks - diff) * sizeof (qsampler->peaks[0]));
      qsampler->peak_offset = peak_offset;
      bst_qsampler_invalidate (qsampler, 0, diff, TRUE);
      bst_qsampler_update_types (qsampler, 0, (qsampler->peak_offset + diff + 1) * qsampler->zoom_factor);
      bst_qsampler_copy_area (qsampler, diff, 0);
    }
  bst_qsampler_queue_refresh (qsampler);
}

static void
bst_qsampler_avalue_changed (BstQSampler *qsampler)
{
  if (!qsampler->ignore_adjustment)
    {
      GtkAdjustment *adjustment = qsampler->adjustment;
      gdouble rel_offset;

      rel_offset = (adjustment->value - adjustment->lower) / (adjustment->upper - adjustment->lower);
      bst_qsampler_move_to (qsampler, rel_offset * qsampler->peak_length);
    }
}

static void
bst_qsampler_update_adjustment (BstQSampler *qsampler)
{
  GtkAdjustment *adjustment = qsampler->adjustment;

  if (adjustment)
    {
      gdouble value = adjustment->value;

      adjustment->lower = 0;
      adjustment->upper = qsampler->pcm_length;
      adjustment->page_size = MIN (qsampler->n_pixels * qsampler->zoom_factor, adjustment->upper - adjustment->lower);
      adjustment->page_increment = adjustment->page_size * .5;
      adjustment->step_increment = adjustment->page_size * .1;
      adjustment->value = CLAMP (adjustment->value, adjustment->lower, adjustment->upper - adjustment->page_size);
      gtk_adjustment_changed (adjustment);
      if (value != adjustment->value)
	gtk_adjustment_value_changed (adjustment);
    }
}

static inline void
bst_qsampler_draw_peak (BstQSampler *qsampler,
			guint        offset,
			GdkGC       *fore_gc)
{
#define	VSCALE(member)	(CLAMP ((qsampler)->vscale_factor * (qsampler)->member, -32768, 32767))
  GtkWidget *widget = (GtkWidget*) qsampler;
  GdkWindow *canvas = qsampler->canvas;
  gint ythickness = widget->style->ythickness;
  gint hi = 0, low = MAX (widget->allocation.height - 2 * ythickness, hi);
  gint range = (low - hi) >> 1, zero = hi + range;
  guint last_i = offset > 0 ? offset - 1 : 0;
  gint16 last_peak  = VSCALE (peaks[last_i].max), last_mpeak = VSCALE (peaks[last_i].min);
  gint16 peak = VSCALE (peaks[offset].max), mpeak = VSCALE (peaks[offset].min);
  gint16 next_peak = VSCALE (peaks[offset + 1].max), next_mpeak = VSCALE (peaks[offset + 1].min);
  gfloat last_value = last_peak / 32768., value = peak / 32768., next_value = next_peak / 32768.;
  gfloat last_mvalue = last_mpeak / 32768., mvalue = mpeak / 32768., next_mvalue = next_mpeak / 32768.;
  gfloat last_middle_value = (last_value + last_mvalue) / 2;
  gfloat middle_value = (value + mvalue) / 2;
  gfloat next_middle_value = (next_value + next_mvalue) / 2;
  gint x = offset, y;


  switch (qsampler->draw_mode)
    {
      gint yb, yn;
    case BST_QSAMPLER_DRAW_MINIMUM_LINE:
      last_middle_value = last_mvalue;
      middle_value = mvalue;
      next_middle_value = next_mvalue;
    case BST_QSAMPLER_DRAW_MIDDLE_LINE:
      last_value = last_middle_value;
      value = middle_value;
      next_value = next_middle_value;
    case BST_QSAMPLER_DRAW_MAXIMUM_LINE:
      y = zero - range * value;
      yb = zero - range * last_value;
      yn = zero - range * next_value;
      yb = (y + yb) / 2;
      yn = (y + yn) / 2;
      gdk_draw_line (canvas, fore_gc, x, MIN (y, MIN (yb, yn)), x, MAX (y, MAX (yb, yn)));
      break;
    case BST_QSAMPLER_DRAW_MINIMUM_SHAPE:
      y = low - (mvalue + 1) * range;
      gdk_draw_line (canvas, fore_gc, x, hi, x, y);
      break;
    case BST_QSAMPLER_DRAW_MAXIMUM_SHAPE:
      y = low - (value + 1) * range;
      gdk_draw_line (canvas, fore_gc, x,  y, x, low);
      break;
    case BST_QSAMPLER_DRAW_CSHAPE:
      y = low - (value + 1) * range;
      yb = low - (mvalue + 1) * range;
      gdk_draw_line (canvas, fore_gc, x,  y, x, yb);
      break;
    case BST_QSAMPLER_DRAW_CRANGE:
      {
	gint last_min = last_mpeak, last_max = last_peak;
	gint next_min = next_mpeak, next_max = next_peak;
	gint cur_min = mpeak, cur_max = peak;
	gint y1, y2;
	gfloat min_value, max_value;

	y1 = cur_max;
	y2 = cur_max;
	if (last_min > cur_max)
	  y1 = (last_min + cur_max) / 2;
	if (next_min > cur_max)
	  y2 = (next_min + cur_max) / 2;
	cur_max = MAX (cur_max, MAX (y1, y2));
	y1 = cur_min;
	y2 = cur_min;
	if (cur_min > last_max)
	  y1 = (last_max + cur_min) / 2;
	if (cur_min > next_max)
	  y2 = (next_max + cur_min) / 2;
	cur_min = MIN (cur_min, MIN (y1, y2));

	min_value = ((gfloat) cur_min) / 32768.;
	max_value = ((gfloat) cur_max) / 32768.;
	y1 = low - (max_value + 1) * range;
	y2 = low - (min_value + 1) * range;
	gdk_draw_line (canvas, fore_gc, x, y1, x, y2);
      }
      break;
    case BST_QSAMPLER_DRAW_ZERO_SHAPE:
      if (middle_value > 0)
	{
	  y = zero - middle_value * range;
	  gdk_draw_line (canvas, fore_gc, x,    y, x, zero);
	}
      else if (middle_value < 0)
	{
	  y = zero - middle_value * range;
	  gdk_draw_line (canvas, fore_gc, x, zero, x,    y);
	}
      else /* middle_value == 0 */
	gdk_draw_line (canvas, fore_gc, x, zero, x, zero);
      break;
    }
}

static inline gboolean
bst_qsampler_fetch_gcs (BstQSampler *qsampler,
			guint 	     peak_type,
			GdkGC      **fore_gc_p,
			GdkGC      **back_gc_p)
{
  GtkWidget *widget = (GtkWidget*) qsampler;
  GdkGC *back_gc, *fore_gc = NULL;

  if (GTK_WIDGET_IS_SENSITIVE (widget) && !(peak_type & BST_QSAMPLER_SKIP))
    {
      back_gc = widget->style->base_gc[GTK_STATE_NORMAL];
      if (peak_type & BST_QSAMPLER_MARK)
	{
	  if (peak_type & BST_QSAMPLER_ACTIVE)
	    {
	      fore_gc = widget->style->fg_gc[GTK_STATE_NORMAL];
	      back_gc = qsampler->green_gc;
	    }
	  if (peak_type & BST_QSAMPLER_SELECTED)
	    {
	      fore_gc = back_gc;
	      back_gc = widget->style->bg_gc[GTK_STATE_SELECTED];
	    }
	  if (peak_type & BST_QSAMPLER_PRELIGHT)
	    {
	      back_gc = qsampler->red_gc;
	      if (peak_type & BST_QSAMPLER_ACTIVE)
		fore_gc = qsampler->green_gc;
	      else if (peak_type & BST_QSAMPLER_SELECTED)
		fore_gc = widget->style->bg_gc[GTK_STATE_SELECTED];
	      else /* normal */
		fore_gc = widget->style->bg_gc[GTK_STATE_NORMAL];
	    }
	}
      else /* regions or untagged peaks */
	{
	  if (peak_type & BST_QSAMPLER_SELECTED)
	    {
	      if (peak_type & BST_QSAMPLER_ACTIVE)
		fore_gc = qsampler->green_gc;
	      else
		fore_gc = back_gc;
	      back_gc = widget->style->bg_gc[GTK_STATE_SELECTED];
	    }
	  else if (peak_type & BST_QSAMPLER_ACTIVE)
	    fore_gc = qsampler->green_gc;
	  else /* normal */
	    fore_gc = widget->style->fg_gc[GTK_STATE_NORMAL];
	}
      if (peak_type & BST_QSAMPLER_DIRTY)
	fore_gc = widget->style->bg_gc[GTK_STATE_NORMAL];
    }
  else if (peak_type & BST_QSAMPLER_SKIP)
    back_gc = widget->style->bg_gc[GTK_STATE_NORMAL];
  else
    {
      back_gc = widget->style->bg_gc[GTK_STATE_INSENSITIVE];
      fore_gc = widget->style->dark_gc[GTK_STATE_INSENSITIVE];
    }
  *fore_gc_p = fore_gc;
  *back_gc_p = back_gc;

  return peak_type & BST_QSAMPLER_DIRTY;
}

static void
bst_qsampler_redraw (BstQSampler *qsampler,
		     gboolean     draw_dirty)
{
  GtkWidget *widget = GTK_WIDGET (qsampler);
  GdkWindow *canvas = qsampler->canvas;
  gint ythickness = widget->style->ythickness;
  gint hi = 0, low = MAX (widget->allocation.height - 2 * ythickness, hi);
  guint peak_type = 0, i, bound;

  if (qsampler->expose_frame)
    gtk_draw_shadow (widget->style, widget->window,
		     GTK_STATE_NORMAL, GTK_SHADOW_IN,
		     0, 0,
		     widget->allocation.width, widget->allocation.height);
  qsampler->expose_frame = FALSE;

  i = 0;
  do
    {
      GdkGC *fore_gc, *back_gc;
      gboolean zero_line;

      /* find first needs_draw peak */
      for (; i < qsampler->n_pixels; i++)
	{
	  peak_type = qsampler->peaks[i].type;
	  if (!(peak_type & BST_QSAMPLER_NEEDS_DRAW) ||
	      (!draw_dirty && (peak_type & BST_QSAMPLER_DIRTY)))
	    continue;
	  break;
	}
      if (i >= qsampler->n_pixels)
	return;
      zero_line = bst_qsampler_fetch_gcs (qsampler, peak_type, &fore_gc, &back_gc);

      /* coalesce common gc setups and clear flags */
      qsampler->peaks[i].type &= ~BST_QSAMPLER_NEEDS_DRAW;
      for (bound = i + 1; bound < qsampler->n_pixels; bound++)
	{
	  GdkGC *tmp_fore_gc, *tmp_back_gc;

	  peak_type = qsampler->peaks[bound].type;
	  if (!(peak_type & BST_QSAMPLER_NEEDS_DRAW) ||
	      (!draw_dirty && (peak_type & BST_QSAMPLER_DIRTY)))
	    break;
	  if (zero_line != bst_qsampler_fetch_gcs (qsampler, peak_type, &tmp_fore_gc, &tmp_back_gc) ||
	      tmp_fore_gc != fore_gc || tmp_back_gc != back_gc)
	    break;
	  qsampler->peaks[bound].type &= ~BST_QSAMPLER_NEEDS_DRAW;
	}

      /* clear background */
      gdk_draw_rectangle (canvas, back_gc, TRUE, i, hi, bound - i, low - hi);

      /* draw peaks */
      if (zero_line)
	gdk_draw_line (canvas, fore_gc, i, (hi + low) >> 1, bound - 1, (hi + low) >> 1);
      else if (fore_gc)
	for (; i < bound; i++)
	  bst_qsampler_draw_peak (qsampler, i, fore_gc);
      i = bound;
    }
  while (i < qsampler->n_pixels);
}

void
bst_qsampler_set_source (BstQSampler    *qsampler,
			 guint           n_total_samples,
			 BstQSamplerFill fill_func,
			 gpointer        data,
			 GDestroyNotify  destroy)
{
  g_return_if_fail (BST_IS_QSAMPLER (qsampler));
  if (n_total_samples > 0)
    g_return_if_fail (fill_func != NULL);
  else
    g_return_if_fail (fill_func == NULL && destroy == NULL);

  if (qsampler->src_destroy)
    {
      GDestroyNotify d = qsampler->src_destroy;

      qsampler->src_destroy = NULL;
      d (qsampler->src_data);
    }
  qsampler->n_marks = 0;
  g_free (qsampler->marks);
  qsampler->marks = NULL;
  qsampler->n_regions = 0;
  g_free (qsampler->regions);
  qsampler->regions = NULL;
  qsampler->pcm_length = n_total_samples;
  qsampler->peak_offset = 0;
  qsampler->src_filler = fill_func;
  qsampler->src_data = data;
  qsampler->src_destroy = destroy;
  //if (qsampler->adjustment)
  // qsampler->adjustment->value = -1;	/* force update */
  bst_qsampler_resize (qsampler);
  if (qsampler->adjustment)
    bst_qsampler_avalue_changed (qsampler);	/* force update */
}

gint
bst_qsampler_get_mark_offset (BstQSampler *qsampler,
			      guint        mark_index)
{
  guint n;

  g_return_val_if_fail (BST_IS_QSAMPLER (qsampler), -1);
  g_return_val_if_fail (mark_index > 0, -1);

  for (n = 0; n < qsampler->n_marks; n++)
    if (qsampler->marks[n].index == mark_index)
      break;
  if (n == qsampler->n_marks)
    return -1;
  else
    return qsampler->marks[n].offset;
}

void
bst_qsampler_set_mark (BstQSampler    *qsampler,
		       guint           mark_index,
		       guint           offset,
		       BstQSamplerType type)
{
  guint n;

  g_return_if_fail (BST_IS_QSAMPLER (qsampler));
  g_return_if_fail (mark_index > 0);
  g_return_if_fail ((type & ~BST_QSAMPLER_MARK_MASK) == 0);
  g_return_if_fail (offset < offset + 1);	/* catch guint wraps */

  if (type)
    type |= BST_QSAMPLER_MARK;
  for (n = 0; n < qsampler->n_marks; n++)
    if (qsampler->marks[n].index == mark_index)
      break;
  if (n == qsampler->n_marks)
    {
      if (!type)
	return;
      qsampler->n_marks++;
      qsampler->marks = g_renew (BstQSamplerMark, qsampler->marks, qsampler->n_marks);
      qsampler->marks[n].index = mark_index;
      qsampler->marks[n].type = type;
      qsampler->marks[n].offset = offset;
    }
  else
    {
      guint old_offset = qsampler->marks[n].offset;

      if (type)
	{
	  qsampler->marks[n].type = type;
	  qsampler->marks[n].offset = offset;
	  bst_qsampler_update_types (qsampler, old_offset, 1);
	}
      else
	{
	  qsampler->n_marks--;
	  if (n < qsampler->n_marks)
	    qsampler->marks[n] = qsampler->marks[qsampler->n_marks];
	  bst_qsampler_update_types (qsampler, old_offset, 1);
	  return;
	}
    }
  bst_qsampler_update_types (qsampler, offset, 1);
}

void
bst_qsampler_set_region (BstQSampler    *qsampler,
			 guint           region_index,
			 guint           offset,
			 guint           length,
			 BstQSamplerType type)
{
  guint n;

  g_return_if_fail (BST_IS_QSAMPLER (qsampler));
  g_return_if_fail (region_index > 0);
  g_return_if_fail ((type & ~BST_QSAMPLER_REGION_MASK) == 0);
  g_return_if_fail (offset < offset + length);	/* catch guint wraps */

  for (n = 0; n < qsampler->n_regions; n++)
    if (qsampler->regions[n].index == region_index)
      break;
  if (n == qsampler->n_regions)
    {
      if (!type || !length)
	return;
      qsampler->n_regions++;
      qsampler->regions = g_renew (BstQSamplerRegion, qsampler->regions, qsampler->n_regions);
      qsampler->regions[n].index = region_index;
      qsampler->regions[n].type = type;
      qsampler->regions[n].offset = offset;
      qsampler->regions[n].length = length;
    }
  else
    {
      guint old_offset = qsampler->regions[n].offset;
      guint old_length = qsampler->regions[n].length;

      if (type && length)
	{
	  qsampler->regions[n].type = type;
	  qsampler->regions[n].offset = offset;
	  qsampler->regions[n].length = length;
	  bst_qsampler_update_types (qsampler, old_offset, old_length);
	}
      else
	{
	  qsampler->n_regions--;
	  if (n < qsampler->n_regions)
	    qsampler->regions[n] = qsampler->regions[qsampler->n_regions];
	  bst_qsampler_update_types (qsampler, old_offset, old_length);
	  return;
	}
    }
  bst_qsampler_update_types (qsampler, offset, length);
}

void
bst_qsampler_set_zoom (BstQSampler *qsampler,
		       gdouble	    zoom)
{
  g_return_if_fail (BST_IS_QSAMPLER (qsampler));

  zoom = CLAMP (zoom, 1e-16, 1e+16);
  qsampler->zoom_factor = 100. / zoom;
  qsampler->peak_offset = qsampler->adjustment->value / qsampler->zoom_factor;
  bst_qsampler_resize (qsampler);

  g_object_notify (G_OBJECT (qsampler), "zoom");
}

void
bst_qsampler_set_vscale (BstQSampler *qsampler,
			 gdouble      vscale)
{
  g_return_if_fail (BST_IS_QSAMPLER (qsampler));

  vscale = CLAMP (vscale, 1e-16, 1e+16);
  qsampler->vscale_factor = vscale / 100.;
  bst_qsampler_invalidate (qsampler, 0, qsampler->n_pixels, FALSE);
  bst_qsampler_queue_refresh (qsampler);

  g_object_notify (G_OBJECT (qsampler), "vscale");
}

void
bst_qsampler_set_draw_mode (BstQSampler        *qsampler,
			    BstQSamplerDrawMode dmode)
{
  g_return_if_fail (BST_IS_QSAMPLER (qsampler));
  g_return_if_fail (dmode < BST_QSAMPLER_DRAW_MODE_LAST);

  qsampler->draw_mode = dmode;
  gtk_widget_queue_draw (GTK_WIDGET (qsampler));
  g_object_notify (G_OBJECT (qsampler), "draw_mode");
}

static void
bst_qsampler_scroll_internal (BstQSampler *qsampler,
			      guint	   pcm_offset,
			      gfloat	   pscale,
			      gfloat       padding,
			      guint        lrflag)
{
  GtkAdjustment *adjustment = qsampler->adjustment;
  gfloat upper = qsampler->n_pixels * pscale;
  gfloat lower = qsampler->n_pixels * pscale;

  if ((lrflag & 1) && pcm_offset >= (qsampler->peak_offset + upper) * qsampler->zoom_factor)
    {
      adjustment->value = adjustment->lower + pcm_offset - (upper - pscale) * qsampler->zoom_factor;
      adjustment->value += padding * qsampler->n_pixels * qsampler->zoom_factor;
      adjustment->value = CLAMP (adjustment->value, adjustment->lower, adjustment->upper - adjustment->page_size);
    }
  else if ((lrflag & 2) && pcm_offset < (qsampler->peak_offset + lower) * qsampler->zoom_factor)
    {
      adjustment->value = adjustment->lower + pcm_offset - lower * qsampler->zoom_factor;
      adjustment->value -= padding * qsampler->n_pixels * qsampler->zoom_factor;
      adjustment->value = CLAMP (adjustment->value, adjustment->lower, adjustment->upper - adjustment->page_size);
    }
}

void
bst_qsampler_scroll_rbounded (BstQSampler *qsampler,
			      guint        pcm_offset,
			      gfloat       boundary_padding,
			      gfloat       padding)
{
  g_return_if_fail (BST_IS_QSAMPLER (qsampler));
  g_return_if_fail (boundary_padding >= 0.1 && boundary_padding <= 1.0);
  g_return_if_fail (padding >= 0 && padding <= 1.0);

  bst_qsampler_scroll_internal (qsampler, pcm_offset, boundary_padding, padding, 1);
  gtk_adjustment_value_changed (qsampler->adjustment);
}

void
bst_qsampler_scroll_lbounded (BstQSampler *qsampler,
			      guint        pcm_offset,
			      gfloat       boundary_padding,
			      gfloat       padding)
{
  g_return_if_fail (BST_IS_QSAMPLER (qsampler));
  g_return_if_fail (boundary_padding >= 0.1 && boundary_padding <= 1.0);
  g_return_if_fail (padding >= 0 && padding <= 1.0);

  bst_qsampler_scroll_internal (qsampler, pcm_offset, 1.0 - boundary_padding, padding, 2);
  gtk_adjustment_value_changed (qsampler->adjustment);
}

void
bst_qsampler_scroll_bounded (BstQSampler *qsampler,
			     guint        pcm_offset,
			     gfloat       boundary_padding,
			     gfloat       padding)
{
  g_return_if_fail (BST_IS_QSAMPLER (qsampler));
  g_return_if_fail (boundary_padding >= 0.1 && boundary_padding <= 1.0);
  g_return_if_fail (padding >= 0 && padding <= 1.0);

  bst_qsampler_scroll_internal (qsampler, pcm_offset, boundary_padding, padding, 1);
  bst_qsampler_scroll_internal (qsampler, pcm_offset, 1.0 - boundary_padding, padding, 2);
  gtk_adjustment_value_changed (qsampler->adjustment);
}

void
bst_qsampler_scroll_show (BstQSampler *qsampler,
			  guint	       pcm_offset)
{
  g_return_if_fail (BST_IS_QSAMPLER (qsampler));

  bst_qsampler_scroll_internal (qsampler, pcm_offset, 1.0, 0, 1);
  bst_qsampler_scroll_internal (qsampler, pcm_offset, 0, 0, 2);
  gtk_adjustment_value_changed (qsampler->adjustment);
}

void
bst_qsampler_scroll_to (BstQSampler *qsampler,
			guint	     pcm_offset)
{
  GtkAdjustment *adjustment;

  g_return_if_fail (BST_IS_QSAMPLER (qsampler));

  adjustment = qsampler->adjustment;
  adjustment->value = pcm_offset;
  adjustment->value = CLAMP (adjustment->value, adjustment->lower, adjustment->upper - adjustment->page_size);
  gtk_adjustment_value_changed (adjustment);
}

void
bst_qsampler_force_refresh (BstQSampler *qsampler)
{
  g_return_if_fail (BST_IS_QSAMPLER (qsampler));

  if (GTK_WIDGET_DRAWABLE (qsampler))
    bst_qsampler_redraw (qsampler, FALSE);
}

void
bst_qsampler_set_adjustment (BstQSampler   *qsampler,
			     GtkAdjustment *adjustment)
{
  g_return_if_fail (BST_IS_QSAMPLER (qsampler));
  if (adjustment)
    g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));

  if (qsampler->adjustment)
    {
      gtk_signal_disconnect_by_data (GTK_OBJECT (qsampler->adjustment), qsampler);
      gtk_object_unref (GTK_OBJECT (qsampler->adjustment));
    }
  qsampler->adjustment = adjustment;
  if (qsampler->adjustment)
    {
      gtk_object_ref (GTK_OBJECT (qsampler->adjustment));
      gtk_object_sink (GTK_OBJECT (qsampler->adjustment));
      g_object_connect (qsampler->adjustment,
			"swapped_signal::value_changed", bst_qsampler_avalue_changed, qsampler,
			NULL);
    }
  bst_qsampler_update_adjustment (qsampler);
}


typedef struct {
  SfiProxy esample;
  guint    nth_channel;
  guint    n_channels;
} ESampleFiller;

static guint
qsampler_esample_filler (gpointer         data,
			 guint            voffset,
			 gdouble          offset_scale,
			 guint            block_size,
			 guint            n_values,
			 BstQSamplerPeak *values,
			 BstQSampler     *qsampler)
{
  ESampleFiller *fill = (ESampleFiller*) data;
  BseFloatSeq *fseq;
  voffset = voffset * fill->n_channels + fill->nth_channel;
  fseq = bse_editable_sample_collect_stats (fill->esample,
                                            voffset,
                                            offset_scale * fill->n_channels,
                                            block_size * fill->n_channels,
                                            fill->n_channels,
                                            n_values);
  uint i;
  for (i = 0; i < fseq->n_values / 2; i++)
    {
      values[i].min = fseq->values[i * 2] * 32767.9;
      values[i].max = fseq->values[i * 2 + 1] * 32767.9;
    }

  return i;
}

static void
free_esample_filler (gpointer data)
{
  ESampleFiller *fill = (ESampleFiller*) data;

  bse_item_unuse (fill->esample);
  g_free (data);
}

void
bst_qsampler_set_source_from_esample (BstQSampler *qsampler,
				      SfiProxy     esample,
				      guint        nth_channel)
{
  ESampleFiller *fill;

  g_return_if_fail (BST_IS_QSAMPLER (qsampler));
  g_return_if_fail (BSE_IS_EDITABLE_SAMPLE (esample));

  fill = g_new (ESampleFiller, 1);
  fill->esample = esample;
  bse_item_use (fill->esample);
  fill->n_channels = bse_editable_sample_get_n_channels (fill->esample);
  fill->nth_channel = nth_channel;

  bst_qsampler_set_source (qsampler,
			   bse_editable_sample_get_length (fill->esample) / fill->n_channels,
			   qsampler_esample_filler,
			   fill, free_esample_filler);
}
