/* BEAST - Bedevilled Audio System
 * Copyright (C) 2000-2001 Tim Janik
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
#include	"bstqsampler.h"

#include	"bstutils.h"
#include	<gsl/gsldatacache.h>
#include	<gsl/gsldatahandle.h>
#include	<string.h>


/* slow machines:
   #define	VREAD_TIMEOUT	(20)
   #define	JOINT_VREADS	(1)
*/
/* medium machines:
   #define	VREAD_TIMEOUT	(10)
   #define	JOINT_VREADS	(4)
*/
/* fast machines:
   #define	VREAD_TIMEOUT	(0)
   #define	JOINT_VREADS	(16)
*/
#define	VREAD_TIMEOUT	(0)
#define	JOINT_VREADS	(8)


#define	FLOOR(x)	((gint) (x))	// FIXME
#define	CTYPE		gint16


/* --- properties --- */
enum {
  PROP_0,
  PROP_ZOOM,
  PROP_VSCALE,
  PROP_DRAW_MODE
};


/* --- prototypes --- */
static void	bst_qsampler_class_init		(BstQSamplerClass	*class);
static void	bst_qsampler_init		(BstQSampler		*qsampler);
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
static void	bst_qsampler_resize		(BstQSampler		*qsampler,
						 gboolean		 view_changed);
static void	bst_qsampler_jump_to		(BstQSampler		*qsampler,
						 guint			 sample_offset);
static void	bst_qsampler_queue_vread	(BstQSampler		*qsampler,
						 guint			 pstart,
						 guint			 pend);
static void	bst_qsampler_queue_expose	(BstQSampler		*qsampler,
						 guint			 pstart,
						 guint			 pend);
static void	bst_qsampler_update_types	(BstQSampler		*qsampler,
						 guint			 offset,
						 guint			 length);
static void	bst_qsampler_update_adjustment	(BstQSampler		*qsampler);
static void	nop_filler			(gpointer       data,
						 guint          voffset,
						 guint          n_values,
						 gint16        *values,
						 BstQSampler   *qsampler);


/* --- static variables --- */
static gpointer	parent_class = NULL;


/* --- functions --- */
GtkType
bst_qsampler_get_type (void)
{
  static GtkType qsampler_type = 0;

  if (!qsampler_type)
    {
      GtkTypeInfo qsampler_info =
      {
	"BstQSampler",
	sizeof (BstQSampler),
	sizeof (BstQSamplerClass),
	(GtkClassInitFunc) bst_qsampler_class_init,
	(GtkObjectInitFunc) bst_qsampler_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };

      qsampler_type = gtk_type_unique (GTK_TYPE_WIDGET, &qsampler_info);
    }

  return qsampler_type;
}

static void
bst_qsampler_class_init (BstQSamplerClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  parent_class = gtk_type_class (GTK_TYPE_WIDGET);

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

  qsampler->n_total_samples = 1;
  qsampler->sample_offset = 0;
  qsampler->n_area_samples = 0;
  qsampler->zoom_factor = 1.0;
  qsampler->vscale_factor = 1.0;
  qsampler->offset2peak_factor = 0;
  qsampler->n_peaks = 0;
  qsampler->peaks = NULL;
  qsampler->mpeaks = NULL;
  qsampler->peak_types = NULL;
  qsampler->marks = NULL;
  qsampler->regions = NULL;
  qsampler->red = default_red;
  qsampler->green = default_green;
  qsampler->red_gc = NULL;
  qsampler->green_gc = NULL;
  qsampler->draw_mode = BST_QSAMPLER_DRAW_CRANGE;
  qsampler->join_vreads = JOINT_VREADS;
  qsampler->src_filler = nop_filler;
  qsampler->src_data = NULL;
  qsampler->src_destroy = NULL;
  bst_qsampler_resize (qsampler, TRUE);
  gtk_widget_set_double_buffered (GTK_WIDGET (qsampler), FALSE);
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
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= (GDK_EXPOSURE_MASK |
			    GDK_BUTTON_PRESS_MASK |
			    GDK_BUTTON_RELEASE_MASK);
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
  gdk_window_set_user_data (widget->window, qsampler);
  widget->style = gtk_style_attach (widget->style, widget->window);
  gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
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
bst_qsampler_size_request (GtkWidget      *widget,
			   GtkRequisition *requisition)
{
  BstQSampler *qsampler = BST_QSAMPLER (widget);

  /* can just guess constantly */
  requisition->width = 320;
  requisition->height = 32;
}

static void
bst_qsampler_size_allocate (GtkWidget     *widget,
			    GtkAllocation *allocation)
{
  BstQSampler *qsampler = BST_QSAMPLER (widget);
  
  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  bst_qsampler_resize (qsampler, TRUE);
}

static void
bst_qsampler_style_set (GtkWidget *widget,
			GtkStyle  *previous_style)
{
  BstQSampler *qsampler = BST_QSAMPLER (widget);

  if (GTK_WIDGET_REALIZED (qsampler))
    {
      GdkGCValuesMask gc_values_mask;
      GdkGCValues gc_values;

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
    }
  GTK_WIDGET_CLASS (parent_class)->style_set (widget, previous_style);
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
  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static gint
bst_qsampler_expose (GtkWidget      *widget,
		     GdkEventExpose *event)
{
  BstQSampler *qsampler = BST_QSAMPLER (widget);
  
  if (GTK_WIDGET_DRAWABLE (qsampler))
    {
      gint xthickness = widget->style->xthickness;
      gint ythickness = widget->style->xthickness;

      qsampler->expose_frame |= (event->area.y <= ythickness ||
				 event->area.y + FLOOR (event->area.height) >= FLOOR (widget->allocation.height) - ythickness ||
				 event->area.x <= xthickness ||
				 event->area.x + FLOOR (event->area.width) >= FLOOR (widget->allocation.width) - xthickness);
      bst_qsampler_queue_expose (qsampler,
				 CLAMP (event->area.x - xthickness, 0, ((gint) qsampler->n_peaks) - 1),
				 CLAMP (event->area.x + FLOOR (event->area.width) - xthickness, 0, qsampler->n_peaks));
    }
  return TRUE;
}

void
bst_qsampler_get_bounds (BstQSampler *qsampler,
			 gint        *first_offset,
			 gint        *last_offset)
{
  GtkWidget *widget;
  guint ostart, oend;

  g_return_if_fail (BST_IS_QSAMPLER (qsampler));

  widget = GTK_WIDGET (qsampler);
  ostart = qsampler->sample_offset;
  oend = qsampler->sample_offset + qsampler->n_area_samples;

  if (first_offset)
    *first_offset = ostart;
  if (last_offset)
    *last_offset = oend;
}

gboolean
bst_qsampler_get_offset_at (BstQSampler *qsampler,
			    gint        *x_coord_p)
{
  g_return_val_if_fail (BST_IS_QSAMPLER (qsampler), FALSE);
  g_return_val_if_fail (x_coord_p != NULL, FALSE);

  if (GTK_WIDGET_DRAWABLE (qsampler))
    {
      GtkWidget *widget = GTK_WIDGET (qsampler);
      gint x_coord, xthickness = widget->style->xthickness;

      x_coord = *x_coord_p;
      x_coord -= xthickness;
      if (x_coord < 0)
	{
	  x_coord = ((gdouble) -x_coord) / qsampler->offset2peak_factor;
	  *x_coord_p = x_coord = -MAX (1, x_coord);
	  return FALSE;
	}
      if (x_coord >= qsampler->n_peaks)
	{
	  x_coord -= qsampler->n_peaks;
	  x_coord = floor (((gdouble) x_coord) / qsampler->offset2peak_factor);
	  *x_coord_p = x_coord + 1;
	  return FALSE;
	}
      x_coord = qsampler->sample_offset + floor (((gdouble) x_coord) / qsampler->offset2peak_factor);
      if (x_coord >= qsampler->n_total_samples)
	{
	  x_coord -= qsampler->n_total_samples;
	  *x_coord_p = x_coord + 1;
	  return FALSE;
	}
      *x_coord_p = x_coord;
      return TRUE;
    }
  *x_coord_p = 0;
  return FALSE;
}

static void
bst_qsampler_destroy (GtkObject *object)
{
  BstQSampler *qsampler = BST_QSAMPLER (object);

  bst_qsampler_set_source (qsampler, 0, NULL, NULL, NULL);
  bst_qsampler_set_adjustment (qsampler, NULL);

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bst_qsampler_finalize (GObject *object)
{
  BstQSampler *qsampler = BST_QSAMPLER (object);

  g_free (qsampler->peaks);
  g_free (qsampler->mpeaks);
  g_free (qsampler->peak_types);
  g_free (qsampler->marks);
  g_free (qsampler->regions);
  if (qsampler->vread_handler)
    g_source_remove (qsampler->vread_handler);
  if (qsampler->expose_handler)
    g_source_remove (qsampler->expose_handler);

  G_OBJECT_CLASS (parent_class)->finalize (object);
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
      bst_qsampler_set_draw_mode (qsampler, g_value_get_enum (value));
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
bst_qsampler_find_dirty (BstQSampler *qsampler,
			 guint        current)
{
  gint i;

  for (i = current; i < qsampler->n_peaks; i++)
    if (qsampler->peak_types[i] & BST_QSAMPLER_DIRTY)
      return i;
  for (i = 0; i < MIN (current, qsampler->n_peaks); i++)
    if (qsampler->peak_types[i] & BST_QSAMPLER_DIRTY)
      return i;
  return qsampler->n_peaks;
}

static gboolean
bst_qsampler_read (BstQSampler *qsampler)
{
  guint i, n, join_vreads = qsampler->join_vreads;

  if (qsampler->vread_handler)
    {
      g_source_remove (qsampler->vread_handler);
      qsampler->vread_handler = 0;
    }

  // i = bst_qsampler_find_dirty (qsampler, qsampler->n_peaks / 2); // FIXME: kernel wants incremental seeks
  i = bst_qsampler_find_dirty (qsampler, 0);
  while (i < qsampler->n_peaks && join_vreads--)
    {
      guint ostart = qsampler->sample_offset + floor (((gdouble) i) / qsampler->offset2peak_factor);
      guint oend = qsampler->sample_offset + floor (((gdouble) i + 1) / qsampler->offset2peak_factor);
      gint16 vblock[1024] = { 0, };
      guint filler_n_values, filler_voffset;
      gint32 va = -32767, vi = 32767;
      
      if (oend - ostart > 1024)
	{
	  filler_n_values = 1024;
	  filler_voffset = ostart + (oend - ostart - 1024) / 2;
	}
      else
	{
	  oend = MAX (oend, ostart + 1);
	  filler_n_values = oend - ostart;
	  filler_voffset = ostart;
	}
      if (filler_voffset + filler_n_values <= qsampler->n_total_samples)
	{
	  qsampler->peak_types[i] &= ~BST_QSAMPLER_SKIP;
	  qsampler->src_filler (qsampler->src_data, filler_voffset, filler_n_values, vblock, qsampler);
	}
      else
	qsampler->peak_types[i] |= BST_QSAMPLER_SKIP;
      for (n = 0; n < filler_n_values; n++)
	{
	  va = MAX (va, vblock[n]);
	  vi = MIN (vi, vblock[n]);
	}
      qsampler->peaks[i] = va;
      qsampler->mpeaks[i] = vi;
      qsampler->peak_types[i] &= ~BST_QSAMPLER_DIRTY;
      bst_qsampler_queue_expose (qsampler, MAX (i, 1) - 1, MIN (i + 2, qsampler->n_peaks));
      i = bst_qsampler_find_dirty (qsampler, i);
    }
  if (i < qsampler->n_peaks && !qsampler->vread_handler)
    qsampler->vread_handler = g_timeout_add_full (BST_QSAMPLER_READ_PRIORITY,
						  VREAD_TIMEOUT,
						  (GSourceFunc) bst_qsampler_read,
						  qsampler,
						  NULL);
  return FALSE;
}

static void
bst_qsampler_queue_vread (BstQSampler *qsampler,
			  guint	       pstart,
			  guint	       pend)
{
  pend = MIN (pend, qsampler->n_peaks);
  if (pstart < pend)
    {
      guint i;
      
      for (i = pstart; i < pend; i++)
	{
	  qsampler->peak_types[i] |= BST_QSAMPLER_DIRTY;
	  qsampler->peaks[i] = 0;
	  qsampler->mpeaks[i] = 0;
	}
      if (!qsampler->vread_handler)
	qsampler->vread_handler = g_timeout_add_full (BST_QSAMPLER_READ_PRIORITY,
						      VREAD_TIMEOUT,
						      (GSourceFunc) bst_qsampler_read,
						      qsampler,
						      NULL);
    }
}

static guint
OFS2PEAK (BstQSampler *qsampler,
	  guint        offset)
{
  gdouble start = offset > qsampler->sample_offset ? offset - qsampler->sample_offset : 0;
  guint peak = qsampler->offset2peak_factor * start + .5;

  return MIN (peak, qsampler->n_peaks);
}

static guint
OFS2PEAK_BOUND (BstQSampler *qsampler,
		guint        offset,
		guint	     length)
{
  guint next_peak = OFS2PEAK (qsampler, offset + length);
  guint peak = OFS2PEAK (qsampler, offset);

  peak = MAX (peak + 1, next_peak);

  return MIN (peak, qsampler->n_peaks);
#if 0
  guint pos = offset + length - 1;
  gdouble bound = pos > qsampler->sample_offset ? pos - qsampler->sample_offset : 0;
  guint peak = qsampler->offset2peak_factor * bound;

  return MIN (peak + MAX (1, qsampler->offset2peak_factor), qsampler->n_peaks);
#endif
}

static void
bst_qsampler_intersect (BstQSampler *qsampler,
			guint	    *start_p,
			guint	    *end_p,
			guint	     offset,
			guint	     length)
{
  guint start = OFS2PEAK (qsampler, offset);
  guint end = OFS2PEAK_BOUND (qsampler, offset, length);

  *start_p = MAX (*start_p, start);
  *end_p = MIN (*end_p, end);
}

static void
bst_qsampler_update_types (BstQSampler *qsampler,
			   guint	offset,
			   guint	length)
{
  guint start = OFS2PEAK (qsampler, offset);
  guint end = OFS2PEAK_BOUND (qsampler, offset, length);

  g_return_if_fail (end <= qsampler->n_peaks);

  if (start < end)
    {
      guint8 *types = g_new0 (guint8, end - start);
      guint changed = end;
      guint i, n;
      
      for (n = 0; n < qsampler->n_regions; n++)
	{
	  guint s = start, e = end;

	  if (qsampler->regions[n].offset + qsampler->regions[n].length < qsampler->sample_offset)
	    continue;
	  bst_qsampler_intersect (qsampler, &s, &e,
				  qsampler->regions[n].offset,
				  qsampler->regions[n].length);
	  for (i = s; i < e; i++)
	    types[i - start] |= qsampler->regions[n].type;
	}
      for (n = 0; n < qsampler->n_marks; n++)
	{
	  guint s = start, e = end;
	  
	  if (qsampler->marks[n].offset < qsampler->sample_offset)
	    continue;
	  bst_qsampler_intersect (qsampler, &s, &e,
				  qsampler->marks[n].offset, 1);
	  if (qsampler->marks[n].type & BST_QSAMPLER_PRELIGHT)
	    e = MIN (e, s + 1);
	  for (i = s; i < e; i++)
	    types[i - start] |= qsampler->marks[n].type;
	}
      for (i = start; i < end; i++)
	if ((qsampler->peak_types[i] & BST_QSAMPLER_MASK) != types[i - start])
	  {
	    qsampler->peak_types[i] &= ~BST_QSAMPLER_MASK;
	    qsampler->peak_types[i] |= types[i - start] | BST_QSAMPLER_NEEDS_DRAW;
	    changed = i;
	  }
      g_free (types);
      
      if (changed < end)	/* make sure we have an expose handler */
	bst_qsampler_queue_expose (qsampler, changed, changed + 1);
    }
}

static void
bst_qsampler_resize (BstQSampler *qsampler,
		     gboolean     view_changed)
{
  GtkWidget *widget = GTK_WIDGET (qsampler);
  guint old_peaks, n_peaks = widget->allocation.width;

  if (2 * widget->style->xthickness > n_peaks)
    n_peaks = 3;
  else
    {
      n_peaks -= 2 * widget->style->xthickness;
      n_peaks = MAX (n_peaks, 3);
    }

  old_peaks = qsampler->n_peaks;
  qsampler->n_peaks = n_peaks;
  qsampler->peaks = g_renew (guint16, qsampler->peaks, qsampler->n_peaks);
  qsampler->mpeaks = g_renew (guint16, qsampler->mpeaks, qsampler->n_peaks);
  qsampler->peak_types = g_renew (guint8, qsampler->peak_types, qsampler->n_peaks);
  if (n_peaks > old_peaks)
    memset (qsampler->peak_types + (view_changed ? 0 : old_peaks),
	    BST_QSAMPLER_SKIP,
	    (n_peaks - (view_changed ? 0 : old_peaks)));
  qsampler->n_area_samples = ((gdouble) qsampler->n_peaks) / qsampler->zoom_factor;
  qsampler->n_area_samples = MAX (qsampler->n_area_samples, 1);
  qsampler->sample_offset = MIN (qsampler->sample_offset,
				 qsampler->n_total_samples - qsampler->n_area_samples);
  qsampler->offset2peak_factor = qsampler->n_peaks;
  qsampler->offset2peak_factor /= (gdouble) qsampler->n_area_samples;
  bst_qsampler_update_types (qsampler, 0, qsampler->n_total_samples);
  bst_qsampler_queue_vread (qsampler, view_changed ? 0 : old_peaks, qsampler->n_peaks);
  bst_qsampler_queue_expose (qsampler, 0, qsampler->n_peaks);
  /* zoom changed */
  bst_qsampler_update_adjustment (qsampler);
}

static void
bst_qsampler_jump_to (BstQSampler *qsampler,
		      guint	   sample_offset)
{
  sample_offset = CLAMP (sample_offset, 0, qsampler->n_total_samples - qsampler->n_area_samples);
  if (sample_offset > qsampler->sample_offset)
    {
      gint diff = OFS2PEAK (qsampler, sample_offset);
      
      qsampler->sample_offset = sample_offset;
      g_memmove (qsampler->peaks, qsampler->peaks + diff, (qsampler->n_peaks - diff) * sizeof (gint16));
      g_memmove (qsampler->mpeaks, qsampler->mpeaks + diff, (qsampler->n_peaks - diff) * sizeof (gint16));
      g_memmove (qsampler->peak_types, qsampler->peak_types + diff, qsampler->n_peaks - diff);
      memset (qsampler->peak_types + qsampler->n_peaks - diff, 0, diff);
      bst_qsampler_update_types (qsampler, 0, qsampler->n_total_samples);
      bst_qsampler_queue_vread (qsampler, qsampler->n_peaks - diff, qsampler->n_peaks);
      bst_qsampler_queue_expose (qsampler, 0, qsampler->n_peaks);
    }
  else if (sample_offset < qsampler->sample_offset)
    {
      gint diff = OFS2PEAK (qsampler, qsampler->sample_offset + qsampler->sample_offset - sample_offset);
      
      qsampler->sample_offset = sample_offset;
      g_memmove (qsampler->peaks + diff, qsampler->peaks, (qsampler->n_peaks - diff) * sizeof (gint16));
      g_memmove (qsampler->mpeaks + diff, qsampler->mpeaks, (qsampler->n_peaks - diff) * sizeof (gint16));
      g_memmove (qsampler->peak_types + diff, qsampler->peak_types, qsampler->n_peaks - diff);
      memset (qsampler->peak_types, 0, diff);
      bst_qsampler_update_types (qsampler, 0, qsampler->n_total_samples);
      bst_qsampler_queue_vread (qsampler, 0, diff);
      bst_qsampler_queue_expose (qsampler, 0, qsampler->n_peaks);
    }
}

static gboolean
bst_qsampler_redraw (BstQSampler *qsampler)
{
  if (qsampler->expose_handler)
    {
      g_source_remove (qsampler->expose_handler);
      qsampler->expose_handler = 0;
    }
  if (GTK_WIDGET_DRAWABLE (qsampler))
    {
      GtkWidget *widget = GTK_WIDGET (qsampler);
      GdkWindow *window = widget->window;
      gint xthickness = widget->style->xthickness;
      gint ythickness = widget->style->xthickness;
      gint hi = ythickness, low = MAX (FLOOR (widget->allocation.height) - ythickness, hi);
      gint range = (low - hi) >> 1, zero = hi + range;
      GdkRectangle rectangle = { 0, 0, widget->allocation.width, widget->allocation.height };
      guint i;

      if (GTK_WIDGET_DOUBLE_BUFFERED (qsampler))
	gdk_window_begin_paint_rect (widget->window, &rectangle);
      
      if (qsampler->expose_frame)
	gtk_draw_shadow (widget->style, widget->window,
			 GTK_STATE_NORMAL, GTK_SHADOW_IN,
			 0, 0,
			 widget->allocation.width, widget->allocation.height);
      for (i = 0; i < qsampler->n_peaks; i++)
	{
#define	VSCALE(qs, member)	(CLAMP ((qs)->vscale_factor * (qs)->member, -32768, 32767))
	  gint16 last_peak = VSCALE (qsampler, peaks[i > 0 ? i - 1 : i]);
	  gint16 peak = VSCALE (qsampler, peaks[i]);
	  gint16 next_peak = VSCALE (qsampler, peaks[i + 1 < qsampler->n_peaks ? i + 1 : i]);
	  gint16 last_mpeak = VSCALE (qsampler, mpeaks[i > 0 ? i - 1 : i]);
	  gint16 mpeak = VSCALE (qsampler, mpeaks[i]);
	  gint16 next_mpeak = VSCALE (qsampler, mpeaks[i + 1 < qsampler->n_peaks ? i + 1 : i]);
	  gfloat last_value = ((gfloat) last_peak) / 32768.;
	  gfloat value = ((gfloat) peak) / 32768.;
	  gfloat next_value = ((gfloat) next_peak) / 32768.;
	  gfloat last_mvalue = ((gfloat) last_mpeak) / 32768.;
	  gfloat mvalue = ((gfloat) mpeak) / 32768.;
	  gfloat next_mvalue = ((gfloat) next_mpeak) / 32768.;
	  gfloat last_middle_value = (last_value + last_mvalue) / 2;
	  gfloat middle_value = (value + mvalue) / 2;
	  gfloat next_middle_value = (next_value + next_mvalue) / 2;
	  gint8 peak_type = qsampler->peak_types[i];
	  GdkGC *back_gc, *fore_gc = NULL;
	  gint x = i + xthickness, y;
	  
	  if (!(peak_type & BST_QSAMPLER_NEEDS_DRAW))
	    continue;
	  qsampler->peak_types[i] &= ~BST_QSAMPLER_NEEDS_DRAW;
	  
	  if (GTK_WIDGET_IS_SENSITIVE (qsampler) && !(peak_type & BST_QSAMPLER_SKIP))
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
			fore_gc = widget->style->fg_gc[GTK_STATE_NORMAL];
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
	    }
	  else
	    {
	      back_gc = widget->style->bg_gc[GTK_STATE_INSENSITIVE];
	      if (qsampler->peak_types[i] & BST_QSAMPLER_SKIP)
		fore_gc = widget->style->bg_gc[GTK_STATE_NORMAL];
	      else
		fore_gc = widget->style->dark_gc[GTK_STATE_INSENSITIVE];
	    }
	  if (peak_type & BST_QSAMPLER_SKIP)
	    gdk_draw_line (window, fore_gc, x, hi, x, low);
	  else if (peak_type & BST_QSAMPLER_DIRTY)
	    gdk_draw_line (window, back_gc, x, hi, x, low);
	  else if (!(qsampler->peak_types[i] & BST_QSAMPLER_DIRTY))
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
		gdk_draw_line (window, back_gc, x, hi, x, low);
		yb = (y + yb) / 2;
		yn = (y + yn) / 2;
		gdk_draw_line (window, fore_gc, x, MIN (y, MIN (yb, yn)), x, MAX (y, MAX (yb, yn)));
		break;
	      case BST_QSAMPLER_DRAW_MINIMUM_SHAPE:
		y = low - (mvalue + 1) * range;
		gdk_draw_line (window, back_gc, x,  y, x, low);
		gdk_draw_line (window, fore_gc, x, hi, x, y);
		break;
	      case BST_QSAMPLER_DRAW_MAXIMUM_SHAPE:
		y = low - (value + 1) * range;
		gdk_draw_line (window, back_gc, x, hi, x, y);
		gdk_draw_line (window, fore_gc, x,  y, x, low);
		break;
	      case BST_QSAMPLER_DRAW_CSHAPE:
		y = low - (value + 1) * range;
		yb = low - (mvalue + 1) * range;
		gdk_draw_line (window, back_gc, x, hi, x, y);
		gdk_draw_line (window, back_gc, x, yb, x, low);
		gdk_draw_line (window, fore_gc, x,  y, x, yb);
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
		  gdk_draw_line (window, back_gc, x, hi, x, y1);
		  gdk_draw_line (window, back_gc, x, y2, x, low);
		  gdk_draw_line (window, fore_gc, x, y1, x, y2);
		}
		break;
	      case BST_QSAMPLER_DRAW_ZERO_SHAPE:
		if (middle_value > 0)
		  {
		    y = zero - middle_value * range;
		    gdk_draw_line (window, back_gc, x,   hi, x,    y);
		    gdk_draw_line (window, back_gc, x, zero, x,  low);
		    gdk_draw_line (window, fore_gc, x,    y, x, zero);
		  }
		else if (middle_value < 0)
		  {
		    y = zero - middle_value * range;
		    gdk_draw_line (window, back_gc, x,   hi, x, zero);
		    gdk_draw_line (window, back_gc, x,    y, x,  low);
		    gdk_draw_line (window, fore_gc, x, zero, x,    y);
		  }
		else /* middle_value == 0 */
		  {
		    gdk_draw_line (window, back_gc, x, hi, x, low);
		    gdk_draw_line (window, fore_gc, x, zero, x, zero);
		  }
		break;
	      }
	}
      if (GTK_WIDGET_DOUBLE_BUFFERED (qsampler))
	gdk_window_end_paint (widget->window);
    }
  qsampler->expose_frame = FALSE;
  
  return FALSE;
}

static void
bst_qsampler_queue_expose (BstQSampler *qsampler,
			   guint	pstart,
			   guint	pend)
{
  guint i;
  
  g_return_if_fail (pstart < qsampler->n_peaks);
  g_return_if_fail (pend <= qsampler->n_peaks);
  
  for (i = pstart; i < pend; i++)
    qsampler->peak_types[i] |= BST_QSAMPLER_NEEDS_DRAW;

  /* callers rely on us _always_ installing a handler */
  if (!qsampler->expose_handler && GTK_WIDGET_DRAWABLE (qsampler))
    qsampler->expose_handler = g_timeout_add_full (GTK_PRIORITY_REDRAW + 1,
						   0,
						   (GSourceFunc) bst_qsampler_redraw,
						   qsampler,
						   NULL);
}

static void
nop_filler (gpointer     data,
	    guint        voffset,
	    guint        n_values,
	    gint16      *values,
	    BstQSampler *qsampler)
{
  memset (values, 0, n_values * sizeof (values[0]));
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
    {
      g_return_if_fail (fill_func == NULL && destroy == NULL);
      fill_func = nop_filler;
      n_total_samples = 1;
    }

  if (qsampler->src_destroy)
    qsampler->src_destroy (qsampler->src_data);
  qsampler->n_marks = 0;
  g_free (qsampler->marks);
  qsampler->marks = NULL;
  qsampler->n_regions = 0;
  g_free (qsampler->regions);
  qsampler->regions = NULL;
  qsampler->n_total_samples = n_total_samples;
  qsampler->src_filler = fill_func;
  qsampler->src_data = data;
  qsampler->src_destroy = destroy;
  bst_qsampler_resize (qsampler, TRUE);
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
  g_return_if_fail (offset < qsampler->n_total_samples);
  g_return_if_fail ((type & ~BST_QSAMPLER_MARK_MASK) == 0);

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
  g_return_if_fail (offset < qsampler->n_total_samples);
  g_return_if_fail (offset + length <= qsampler->n_total_samples);
  g_return_if_fail ((type & ~BST_QSAMPLER_REGION_MASK) == 0);

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
  qsampler->zoom_factor = zoom / 100.;
  bst_qsampler_resize (qsampler, TRUE);

  g_object_notify (G_OBJECT (qsampler), "zoom");
}

void
bst_qsampler_set_vscale (BstQSampler *qsampler,
			 gdouble      vscale)
{
  g_return_if_fail (BST_IS_QSAMPLER (qsampler));

  vscale = CLAMP (vscale, 1e-16, 1e+16);
  qsampler->vscale_factor = vscale / 100.;
  bst_qsampler_queue_expose (qsampler, 0, qsampler->n_peaks);

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

void
bst_qsampler_scroll_show (BstQSampler *qsampler,
			  guint	       offset)
{
  GtkAdjustment *adjustment;

  g_return_if_fail (BST_IS_QSAMPLER (qsampler));
  g_return_if_fail (offset < qsampler->n_total_samples);

  adjustment = qsampler->adjustment;
  if (offset >= qsampler->sample_offset + qsampler->n_area_samples)
    {
      adjustment->value = offset + 1 - qsampler->n_area_samples;
      gtk_adjustment_value_changed (adjustment);
    }
  else if (offset < qsampler->sample_offset)
    {
      adjustment->value = offset;
      gtk_adjustment_value_changed (adjustment);
    }
}

void
bst_qsampler_scroll_to (BstQSampler *qsampler,
			guint	     offset)
{
  GtkAdjustment *adjustment;

  g_return_if_fail (BST_IS_QSAMPLER (qsampler));
  g_return_if_fail (offset < qsampler->n_total_samples);

  adjustment = qsampler->adjustment;
  adjustment->value = offset;
  gtk_adjustment_value_changed (adjustment);
}

static void
bst_qsampler_avalue_changed (BstQSampler *qsampler)
{
  GtkAdjustment *adjustment = qsampler->adjustment;
  gint sample_offset;

  sample_offset = qsampler->n_total_samples * adjustment->value / (adjustment->upper - adjustment->lower);
  bst_qsampler_jump_to (qsampler, sample_offset);
}

static void
bst_qsampler_update_adjustment (BstQSampler *qsampler)
{
  GtkAdjustment *adjustment = qsampler->adjustment;

  if (adjustment)
    {
      adjustment->lower = 0;
      adjustment->upper = qsampler->n_total_samples;
      adjustment->page_size = MIN (qsampler->n_area_samples, qsampler->n_total_samples);
      adjustment->page_increment = adjustment->page_size * .5;
      adjustment->step_increment = adjustment->page_size * .1;
      adjustment->value = qsampler->sample_offset;
      adjustment->value = CLAMP (adjustment->value, adjustment->lower, adjustment->upper - adjustment->page_size);
      gtk_adjustment_changed (adjustment);
      gtk_adjustment_value_changed (adjustment);
    }
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
      g_object_connect (qsampler->adjustment,
			"swapped_signal::value_changed", bst_qsampler_avalue_changed, qsampler,
			NULL);
    }
  bst_qsampler_update_adjustment (qsampler);
}
