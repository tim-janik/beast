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
#define _ISOC99_SOURCE  /* round */
#include "bstdbmeter.h"
#include <string.h>
#include <math.h>


#define WIDGET(self)            (GTK_WIDGET (self))
/* accessors */
#define STATE(self)             (WIDGET (self)->state)
#define STYLE(self)             (WIDGET (self)->style)
#define XTHICKNESS(self)        (STYLE (self)->xthickness)
#define YTHICKNESS(self)        (STYLE (self)->ythickness)
#define ALLOCATION(self)        (&WIDGET (self)->allocation)

#define DEFAULT_BORDER          (20)
#define NUMBER_HPADDING         (8)     /* extra spacing to seperate numbers horizontally */


/* --- DB Setup --- */
static BstDBSetup*
bst_db_setup_get_default (void)
{
  static BstDBSetup *dbsetup = NULL;
  if (!dbsetup)
    {
      static const GxkSplinePoint beast_db_points[] = {
        {  +6, 0 },
        {  +3, 1 },
        {  +0, 2 },
        {  -3, 3 },
        {  -6, 4 },
        { -12, 5 },
        { -24, 6 },
        { -48, 7 },
        { -96, 8 },
      };
      const GxkSplinePoint *points;
      guint n_points;
      n_points = G_N_ELEMENTS (beast_db_points);
      points = beast_db_points;
      GxkSpline *spline = gxk_spline_new_natural (n_points, points);
      dbsetup = bst_db_setup_new (spline, points[0].x, points[n_points - 1].x);
      gxk_spline_free (spline);
    }
  return dbsetup;
}

static int
db_color_pixel_cmp (const void *v1,
                    const void *v2)
{
  const BstDBColor *c1 = v1;
  const BstDBColor *c2 = v2;
  return c1->pixel < c2->pixel ? -1 : c1->pixel > c2->pixel;
}

BstDBSetup*
bst_db_setup_new (GxkSpline *db2pixel_spline,
                  double     maxdb,
                  double     mindb)
{
  g_return_val_if_fail (db2pixel_spline != NULL, NULL);
  guint i, zindex = 0;
  double miny = db2pixel_spline->segs[0].y, maxy = miny;
  for (i = 1; i < db2pixel_spline->n_segs; i++)
    {
      miny = MIN (miny, db2pixel_spline->segs[i].y);
      maxy = MAX (maxy, db2pixel_spline->segs[i].y);
      if (!db2pixel_spline->segs[i].x)
        zindex = i;
    }
  g_return_val_if_fail (miny == 0, NULL);
  g_return_val_if_fail (maxy > 0, NULL);
  BstDBSetup *dbsetup = g_new0 (BstDBSetup, 1);
  dbsetup->ref_count = 1;
  dbsetup->spline = gxk_spline_copy (db2pixel_spline);
  dbsetup->offset = dbsetup->length = dbsetup->spzoom = 0;
  dbsetup->zero_index = zindex;
  dbsetup->maxdb = maxdb;
  dbsetup->mindb = mindb;
  /* setup colors */
  BstDBColor default_colors[] = {
    {  +6, 0xff0000 },
    {   0, 0xffff00 },
    {  -6, 0x00ff00 },
    { -96, 0x00c000 },
  };
  dbsetup->n_colors = G_N_ELEMENTS (default_colors);
  dbsetup->colors = g_memdup (default_colors, sizeof (dbsetup->colors[0]) * dbsetup->n_colors);
  /* setup zoom and sort colors */
  bst_db_setup_relocate (dbsetup, 0, 99, FALSE);
  return dbsetup;
}

void
bst_db_setup_relocate (BstDBSetup     *dbsetup,
                       gint            offset,
                       gint            range,
                       gboolean        flipdir)
{
  dbsetup->flipdir = flipdir != FALSE;
  guint i;
  double maxy = dbsetup->spline->segs[0].y;
  for (i = 1; i < dbsetup->spline->n_segs; i++)
    maxy = MAX (maxy, dbsetup->spline->segs[i].y);
  dbsetup->offset = offset;
  dbsetup->length = range + 1;
  dbsetup->spzoom = range / maxy;
  /* sort colors in ascending order */
  for (i = 0; i < dbsetup->n_colors; i++)
    dbsetup->colors[i].pixel = bst_db_setup_get_pixel (dbsetup, dbsetup->colors[i].db);
  qsort (dbsetup->colors, dbsetup->n_colors, sizeof (dbsetup->colors[0]), db_color_pixel_cmp);
}

guint
bst_db_setup_get_color (BstDBSetup *dbsetup,
                        double      pixel,
                        double      saturation)
{
  /* find segment via bisection */
  guint offset = 0, n = dbsetup->n_colors;
  while (offset + 1 < n)
    {
      guint i = (offset + n) >> 1;
      if (pixel < dbsetup->colors[i].pixel)
        n = i;
      else
        offset = i;
    }
  g_assert (offset == 0 || pixel >= dbsetup->colors[offset].pixel);
  if (pixel >= dbsetup->colors[offset].pixel && offset + 1 < dbsetup->n_colors)
    {   /* linear interpolation */
      guint c1 = dbsetup->colors[offset].rgb;
      guint c2 = dbsetup->colors[offset + 1].rgb;
      double delta = pixel - dbsetup->colors[offset].pixel;     /* >= 0, see assertion above */
      double range = dbsetup->colors[offset + 1].pixel -
                     dbsetup->colors[offset].pixel;             /* >= 0, ascending sort */
      double d2 = delta / range;                                /* <= 1, due to bisection */
      double d1 = 1.0 - d2;
      guint8 red = saturation * (((c1 >> 16) & 0xff) * d1 + ((c2 >> 16) & 0xff) * d2);
      guint8 green = saturation * (((c1 >> 8) & 0xff) * d1 + ((c2 >> 8) & 0xff) * d2);
      guint8 blue = saturation * ((c1 & 0xff) * d1 + (c2 & 0xff) * d2);
      return (red << 16) | (green << 8) | blue;
    }
  else  /* pixel is out of range on either boundary */
    {
      guint8 red = saturation * ((dbsetup->colors[offset].rgb >> 16) & 0xff);
      guint8 green = saturation * ((dbsetup->colors[offset].rgb >> 8) & 0xff);
      guint8 blue = saturation * (dbsetup->colors[offset].rgb & 0xff);
      return (red << 16) | (green << 8) | blue;
    }
}

BstDBSetup*
bst_db_setup_copy (BstDBSetup *srcdb)
{
  g_return_val_if_fail (srcdb != NULL, NULL);
  g_return_val_if_fail (srcdb->ref_count > 0, NULL);
  BstDBSetup *dbsetup = g_memdup (srcdb, sizeof (srcdb[0]));
  dbsetup->spline = gxk_spline_copy (srcdb->spline);
  dbsetup->colors = g_memdup (srcdb->colors, sizeof (dbsetup->colors[0]) * dbsetup->n_colors);
  dbsetup->ref_count = 1;
  return dbsetup;
}

BstDBSetup*
bst_db_setup_ref (BstDBSetup *dbsetup)
{
  g_return_val_if_fail (dbsetup != NULL, NULL);
  g_return_val_if_fail (dbsetup->ref_count > 0, NULL);
  dbsetup->ref_count += 1;
  return dbsetup;
}

void
bst_db_setup_unref (BstDBSetup *dbsetup)
{
  g_return_if_fail (dbsetup != NULL);
  g_return_if_fail (dbsetup->ref_count > 0);
  dbsetup->ref_count -= 1;
  if (!dbsetup)
    {
      gxk_spline_free (dbsetup->spline);
      dbsetup->spline = NULL;
      g_free (dbsetup->colors);
      g_free (dbsetup);
    }
}

double
bst_db_setup_get_pixel (BstDBSetup *dbsetup,
                        double      dbvalue)
{
  double pixel = gxk_spline_y (dbsetup->spline, dbvalue) * dbsetup->spzoom;
  if (dbsetup->flipdir)
    pixel = (dbsetup->length - 1) - pixel;
  return pixel + dbsetup->offset;
}

double
bst_db_setup_get_dbvalue (BstDBSetup *dbsetup,
                          double      pixel)
{
  pixel -= dbsetup->offset;
  if (dbsetup->flipdir)
    pixel = (dbsetup->length - 1) - pixel;
  return gxk_spline_findx (dbsetup->spline, pixel / dbsetup->spzoom);
}


/* --- comon helpers --- */
static void
db_setup_size_allocate (BstDBSetup     *dbsetup,
                        gint            thickness,
                        gint            border,
                        gint            length,
                        gboolean        vertical)
{
  border = MAX (border, 1);             /* account for the 1 pixel shadow of labeling's max/min lines */
  border = MAX (border, thickness);     /* account for outer shadow of dbbeam */
  gint size = length - 2 * border;
  size -= 1;                            /* account for length = range + 1 pixels */
  bst_db_setup_relocate (dbsetup, border, MAX (size, 0), !vertical);
}

enum {
  PROP_ORIENTATION = 1,
  PROP_DRAW_VALUES,
  PROP_JUSTIFY,
  PROP_N_CHANNELS,
};


/* --- DB Labeling --- */
G_DEFINE_TYPE (BstDBLabeling, bst_db_labeling, GTK_TYPE_WIDGET);
static void
bst_db_labeling_init (BstDBLabeling *self)
{
  GtkWidget *widget = GTK_WIDGET (self);
  GTK_WIDGET_SET_FLAGS (self, GTK_NO_WINDOW);
  gtk_widget_show (widget);
  self->dbsetup = bst_db_setup_copy (bst_db_setup_get_default ());
  self->border = DEFAULT_BORDER;
  self->orientation = GTK_ORIENTATION_VERTICAL;
  self->justify = GTK_JUSTIFY_CENTER;
}

static void
bst_db_labeling_destroy (GtkObject *object)
{
  // BstDBLabeling *self = BST_DB_LABELING (object);
  GTK_OBJECT_CLASS (bst_db_labeling_parent_class)->destroy (object);
}

static void
bst_db_labeling_finalize (GObject *object)
{
  BstDBLabeling *self = BST_DB_LABELING (object);
  bst_db_setup_unref (self->dbsetup);
  G_OBJECT_CLASS (bst_db_labeling_parent_class)->finalize (object);
}

static PangoLayout*
bst_db_labeling_create_layout (BstDBLabeling *self,
                               double         dB)
{
  gchar *buffer = g_strdup_printf ("%u", (int) (0.5 + ABS (dB)));
  PangoLayout *layout = gtk_widget_create_pango_layout (GTK_WIDGET (self), buffer);
  g_free (buffer);
  return layout;
}

static void
bst_db_labeling_size_request (GtkWidget      *widget,
                              GtkRequisition *requisition)
{
  BstDBLabeling *self = BST_DB_LABELING (widget);
  const gboolean vertical = self->orientation == GTK_ORIENTATION_VERTICAL;
  /* always request size based on digit dimensions, regardles of whether
   * numeric values are to be drawn.
   */
  PangoRectangle irect = { 0, }, lrect = { 0 };
  gint i, breadth = 0, length = 0;
  PangoLayout *layout;
  
  for (i = 0; i < self->dbsetup->spline->n_segs; i++)
    {
      double v = self->dbsetup->spline->segs[i].x;
      layout = bst_db_labeling_create_layout (self, v);
      pango_layout_get_pixel_extents (layout, &irect, &lrect);
      if (vertical)
        {
          breadth = MAX (breadth, lrect.width);
          length += lrect.height + 3;
        }
      else
        {
          length += lrect.width + 3 + NUMBER_HPADDING;
          breadth = MAX (breadth, lrect.height);
        }
      g_object_unref (layout);
    }
  if (!self->draw_values)
    {
      /* font width */
      PangoRectangle irect = { 0, }, lrect = { 0 };
      PangoLayout *layout = gtk_widget_create_pango_layout (GTK_WIDGET (self), "9");
      pango_layout_get_pixel_extents (layout, &irect, &lrect);
      g_object_unref (layout);
      guint dash_length = lrect.width;

      if (self->justify == GTK_JUSTIFY_CENTER)
        breadth = 2 * dash_length | 1;  /* always request odd size */
      else
        breadth = dash_length;
    }
  if (vertical)
    {
      requisition->width = breadth;
      requisition->height = length;
    }
  else
    {
      requisition->width = length;
      requisition->height = breadth;
    }
}

static void
bst_db_labeling_size_allocate (GtkWidget     *widget,
                               GtkAllocation *allocation)
{
  BstDBLabeling *self = BST_DB_LABELING (widget);
  const gboolean vertical = self->orientation == GTK_ORIENTATION_VERTICAL;
  widget->allocation = *allocation;
  guint thickness = vertical ? YTHICKNESS (self) : XTHICKNESS (self);
  db_setup_size_allocate (self->dbsetup, thickness, self->border, vertical ? allocation->height : allocation->width, vertical);
}

typedef enum {
  DRAW_SKIP,
  DRAW_ETCHED,
  DRAW_MAJOR,
  DRAW_MINOR,
  DRAW_MICRO,
  DRAW_SUBRO,
  DRAW_NUM
} DrawType;

static void
db_labeling_draw_lateral_line (BstDBLabeling   *self,
                               GdkGC           *gc,
                               gint             x,
                               gint             y,
                               gint             pos,
                               gint             breadth,
                               double           indent)
{
  GtkWidget *widget = GTK_WIDGET (self);
  GdkWindow *drawable = widget->window;
  g_return_if_fail (indent <= 0.5);
  gint pixindent = indent * breadth;
  gint breadth_reduz = 2 * pixindent;
  switch (self->justify)
    {
    case GTK_JUSTIFY_FILL:
      pixindent = 0;
      breadth_reduz = 0;
      break;
    case GTK_JUSTIFY_CENTER:
      pixindent = indent * breadth;
      breadth_reduz = 2 * pixindent;
      break;
    case GTK_JUSTIFY_LEFT:
      breadth_reduz = 2 * pixindent;
      pixindent = 0;
      break;
    case GTK_JUSTIFY_RIGHT:
      breadth_reduz = 2 * pixindent;
      pixindent = breadth_reduz;
      break;
    }
  if (self->orientation == GTK_ORIENTATION_VERTICAL)
    gdk_draw_hline (drawable, gc, x + pixindent, y + pos, breadth - breadth_reduz);
  else  /* horizontal */
    gdk_draw_vline (drawable, gc, x + pos, y + pixindent, breadth - breadth_reduz);
}

static void
db_labeling_draw_vline (BstDBLabeling *self,
                        GdkGC         *gc,
                        gint           x,
                        gint           y,
                        gint           pos,
                        gint           breadth,
                        gint           indent)
{
  GtkWidget *widget = GTK_WIDGET (self);
  GdkWindow *drawable = widget->window;
  if (self->orientation == GTK_ORIENTATION_VERTICAL)
    gdk_draw_vline (drawable, gc, x + indent, y + pos, breadth);
  else  /* horizontal */
    gdk_draw_hline (drawable, gc, x + pos, y + indent, breadth);
}

static void
bst_db_labeling_draw_value (BstDBLabeling *self,
                            GdkRectangle  *expose_area,
                            GdkRectangle  *canvas,
                            double         dB,
                            GdkRectangle  *cover1,
                            GdkRectangle  *cover2,
                            GdkRectangle  *consumed,
                            DrawType       dtype)
{
  GtkWidget *widget = GTK_WIDGET (self);
  GtkAllocation *allocation = ALLOCATION (self);
  const gboolean vertical = self->orientation == GTK_ORIENTATION_VERTICAL;
  BstDBSetup *dbsetup = self->dbsetup;
  gint pos = bst_db_setup_get_pixel (dbsetup, dB);
  if (dtype == DRAW_NUM)
    {
      PangoLayout *layout = bst_db_labeling_create_layout (self, dB);
      PangoRectangle irect = { 0, }, lrect = { 0 };
      pango_layout_get_pixel_extents (layout, &irect, &lrect);
      gint llength = vertical ? lrect.height : lrect.width;
      pos -= llength / 2;               /* center number around position */
      consumed->width = lrect.width;
      consumed->height = lrect.height;
      if (vertical)
        {
          consumed->x = canvas->x + (canvas->width - lrect.width) / 2;
          consumed->y = canvas->y + pos;
          /* keep maxdb, 0db, mindb inside allocation */
          if (dB == dbsetup->mindb || dB == 0 || dB == dbsetup->maxdb)
            {
              if (consumed->y + consumed->height > allocation->y + allocation->height)
                consumed->y = allocation->y + allocation->height - consumed->height;
              consumed->y = MAX (consumed->y, allocation->y);
            }
        }
      else
        {
          consumed->x = canvas->x + pos;
          consumed->y = canvas->y + (canvas->height - lrect.height) / 2;
          /* keep maxdb, 0db, mindb inside allocation */
          if (dB == dbsetup->mindb || dB == 0 || dB == dbsetup->maxdb)
            {
              if (consumed->x + consumed->width > allocation->x + allocation->width)
                consumed->x = allocation->x + allocation->width - consumed->width;
              consumed->x = MAX (consumed->x, allocation->x);
            }
        }
      GdkRectangle dummy;
      if (!gdk_rectangle_intersect (cover1, consumed, &dummy) &&
          !gdk_rectangle_intersect (cover2, consumed, &dummy) &&
          consumed->x >= allocation->x && consumed->x + consumed->width <= allocation->x + allocation->width &&
          consumed->y >= allocation->y && consumed->y + consumed->height <= allocation->y + allocation->height)
        {
          gtk_paint_layout (widget->style, widget->window,
                            GTK_WIDGET_IS_SENSITIVE (self) ? GTK_STATE_NORMAL : GTK_STATE_INSENSITIVE,
                            FALSE, expose_area, widget, NULL, consumed->x, consumed->y, layout);
          if (!vertical)
            {
              consumed->x -= NUMBER_HPADDING / 2;
              consumed->width += NUMBER_HPADDING;
            }
        }
      else
        consumed->width = consumed->height = 0;
      g_object_unref (layout);
    }
  else if (dtype != DRAW_SKIP)
    {
      gint cbreadth = vertical ? canvas->width : canvas->height;
      gint clength = vertical ? canvas->height : canvas->width;
      gboolean draw_minors = clength > 8 * dbsetup->spline->n_segs;
      gboolean draw_micros = clength > 16 * dbsetup->spline->n_segs;
      gboolean draw_subros = clength > 24 * dbsetup->spline->n_segs;
      GdkGC *light_gc = widget->style->light_gc[widget->state];
      GdkGC *line_gc = widget->style->fg_gc[widget->state];
      GdkGC *dark_gc = widget->style->dark_gc[widget->state];
      GdkGC *minor_gc = draw_subros ? line_gc : dark_gc;
      double minor_indent = 0.10;
      double micro_indent = 0.18;
      double subro_indent = 0.30;
      /* db_labeling_draw_lateral_line (self, gc, x, y, pos, breadth, indent) draws a horizontal line if orientation == VERTICAL */
      if (dtype == DRAW_ETCHED)
        db_labeling_draw_lateral_line (self, dark_gc, canvas->x, canvas->y, pos - 1, cbreadth, 0);
      if (dtype == DRAW_ETCHED ||
          dtype == DRAW_MAJOR)
        db_labeling_draw_lateral_line (self, line_gc, canvas->x, canvas->y, pos, cbreadth, 0);
      else if (dtype == DRAW_SUBRO && draw_subros)
        db_labeling_draw_lateral_line (self, dark_gc, canvas->x, canvas->y, pos, cbreadth, subro_indent);
      else if (dtype == DRAW_MICRO && draw_micros)
        db_labeling_draw_lateral_line (self, dark_gc, canvas->x, canvas->y, pos, cbreadth, micro_indent);
      else if (dtype == DRAW_MINOR && draw_minors)
        db_labeling_draw_lateral_line (self, minor_gc, canvas->x, canvas->y, pos, cbreadth, minor_indent);
      if (dtype == DRAW_ETCHED)
        db_labeling_draw_lateral_line (self, light_gc, canvas->x, canvas->y, pos + 1, cbreadth, 0);
    }
}

static gboolean
bst_db_labeling_expose (GtkWidget      *widget,
                        GdkEventExpose *event)
{
  BstDBLabeling *self = BST_DB_LABELING (widget);
  const gboolean vertical = self->orientation == GTK_ORIENTATION_VERTICAL;
  GdkWindow *drawable = event->window;
  GtkAllocation *allocation = ALLOCATION (self);
  BstDBSetup *dbsetup = self->dbsetup;
  gint breadth = vertical ? allocation->width : allocation->height;
  if (drawable != widget->window)
    return FALSE;
  if (0)
    {
      gdk_draw_line (widget->window, widget->style->black_gc, widget->allocation.x, widget->allocation.y,
                     widget->allocation.x + widget->allocation.width-1, widget->allocation.y + widget->allocation.height-1);
      gdk_draw_line (widget->window, widget->style->black_gc, widget->allocation.x + widget->allocation.width-1, widget->allocation.y,
                     widget->allocation.x, widget->allocation.y + widget->allocation.height-1);
    }
  
  GdkGC *dark_gc = widget->style->light_gc[widget->state];
  GdkGC *line_gc = widget->style->fg_gc[widget->state];
  GdkGC *light_gc = widget->style->dark_gc[widget->state];
  gint longitudinal_pos = 0, draw_longitudinal = !self->draw_values;
  switch (self->justify)
    {
    case GTK_JUSTIFY_FILL:
      draw_longitudinal = FALSE;
      break;
    case GTK_JUSTIFY_CENTER:
      longitudinal_pos = breadth / 2;
      break;
    case GTK_JUSTIFY_LEFT:
      // longitudinal_pos = 0 + 1;
      draw_longitudinal = FALSE;
      break;
    case GTK_JUSTIFY_RIGHT:
      // longitudinal_pos = breadth - 1 - 1;
      draw_longitudinal = FALSE;
      break;
    }
  /* vline shades */
  if (draw_longitudinal && breadth >= 3)
    {
      /* db_labeling_draw_vline (self, gc, x, y, pos, breadth, indent) draws a vertical line if orientation == VERTICAL */
      db_labeling_draw_vline (self, light_gc, allocation->x, allocation->y, dbsetup->offset, dbsetup->length, longitudinal_pos - 1);
      db_labeling_draw_vline (self,  dark_gc, allocation->x, allocation->y, dbsetup->offset, dbsetup->length, longitudinal_pos + 1);
    }
  DrawType draw_etched = self->draw_values ? DRAW_NUM : DRAW_ETCHED;
  DrawType draw_major = self->draw_values ? DRAW_NUM : DRAW_MAJOR;
  DrawType draw_minor = self->draw_values ? DRAW_SKIP : DRAW_MINOR;
  DrawType draw_micro = self->draw_values ? DRAW_SKIP : DRAW_MICRO;
  DrawType draw_subro = self->draw_values ? DRAW_SKIP : DRAW_SUBRO;
  gint i;
  GdkRectangle zero_consumed = { 0, }, consumed = { 0, }, last_consumed = { 0, };
  GdkRectangle canvas = *allocation;
  if (vertical)
    {
      // canvas.y += dbsetup->offset;
      canvas.height = dbsetup->length;
    }
  else
    {
      // canvas.x += dbsetup->offset;
      canvas.width = dbsetup->length;
    }
  /* draw zero */
  bst_db_labeling_draw_value (self, &event->area, &canvas, 0, &consumed, &consumed, &zero_consumed, draw_etched);
  /* draw upper half */
  if (dbsetup->maxdb > 0)
    {
      /* draw max */
      GdkRectangle max_consumed = { 0, };
      bst_db_labeling_draw_value (self, &event->area, &canvas, dbsetup->maxdb, &zero_consumed, &zero_consumed, &max_consumed, draw_etched);
      consumed = zero_consumed;
      /* draw zero..max */
      for (i = dbsetup->zero_index + 1; i < dbsetup->spline->n_segs && dbsetup->spline->segs[i].x < dbsetup->maxdb; i++)
        {
          if (consumed.width && consumed.height)
            last_consumed = consumed;
          bst_db_labeling_draw_value (self, &event->area, &canvas, dbsetup->spline->segs[i].x, &max_consumed, &last_consumed, &consumed, draw_major);
        }
      /* draw minors */
      for (i = dbsetup->zero_index; i < dbsetup->spline->n_segs - 1 && dbsetup->spline->segs[i].x < dbsetup->maxdb; i++)
        {
          double db1 = dbsetup->spline->segs[i].x, db2 = dbsetup->spline->segs[i + 1].x;
          bst_db_labeling_draw_value (self, &event->area, &canvas, db1 + (db2 - db1) * 0.125,
                                      &max_consumed, &zero_consumed, &consumed, draw_subro);
          bst_db_labeling_draw_value (self, &event->area, &canvas, db1 + (db2 - db1) * 0.375,
                                      &max_consumed, &zero_consumed, &consumed, draw_subro);
          bst_db_labeling_draw_value (self, &event->area, &canvas, db1 + (db2 - db1) * 0.625,
                                      &max_consumed, &zero_consumed, &consumed, draw_subro);
          bst_db_labeling_draw_value (self, &event->area, &canvas, db1 + (db2 - db1) * 0.875,
                                      &max_consumed, &zero_consumed, &consumed, draw_subro);
          bst_db_labeling_draw_value (self, &event->area, &canvas, db1 + (db2 - db1) * 0.25,
                                      &max_consumed, &zero_consumed, &consumed, draw_micro);
          bst_db_labeling_draw_value (self, &event->area, &canvas, db1 + (db2 - db1) * 0.75,
                                      &max_consumed, &zero_consumed, &consumed, draw_micro);
          bst_db_labeling_draw_value (self, &event->area, &canvas, (db1 + db2) * 0.5,
                                      &max_consumed, &zero_consumed, &consumed, draw_minor);
        }
      /* redraw max */
      bst_db_labeling_draw_value (self, &event->area, &canvas, dbsetup->maxdb, &zero_consumed, &zero_consumed, &max_consumed, draw_etched);
    }
  /* draw lower half */
  if (dbsetup->mindb < 0)
    {
      /* draw min */
      GdkRectangle min_consumed = { 0, };
      bst_db_labeling_draw_value (self, &event->area, &canvas, dbsetup->mindb, &zero_consumed, &zero_consumed, &min_consumed, draw_etched);
      consumed = zero_consumed;
      /* draw min..zero */
      for (i = dbsetup->zero_index - 1; i >= 0 && dbsetup->spline->segs[i].x > dbsetup->mindb; i--)
        {
          if (consumed.width && consumed.height)
            last_consumed = consumed;
          bst_db_labeling_draw_value (self, &event->area, &canvas, dbsetup->spline->segs[i].x, &min_consumed, &last_consumed, &consumed, draw_major);
        }
      /* draw minors */
      for (i = 0; i < dbsetup->zero_index; i++)
        {
          double db1 = dbsetup->spline->segs[i].x, db2 = dbsetup->spline->segs[i + 1].x;
          bst_db_labeling_draw_value (self, &event->area, &canvas, db1 + (db2 - db1) * 0.125,
                                      &min_consumed, &zero_consumed, &consumed, draw_subro);
          bst_db_labeling_draw_value (self, &event->area, &canvas, db1 + (db2 - db1) * 0.375,
                                      &min_consumed, &zero_consumed, &consumed, draw_subro);
          bst_db_labeling_draw_value (self, &event->area, &canvas, db1 + (db2 - db1) * 0.625,
                                      &min_consumed, &zero_consumed, &consumed, draw_subro);
          bst_db_labeling_draw_value (self, &event->area, &canvas, db1 + (db2 - db1) * 0.875,
                                      &min_consumed, &zero_consumed, &consumed, draw_subro);
          bst_db_labeling_draw_value (self, &event->area, &canvas, db1 + (db2 - db1) * 0.25,
                                      &min_consumed, &zero_consumed, &consumed, draw_micro);
          bst_db_labeling_draw_value (self, &event->area, &canvas, db1 + (db2 - db1) * 0.75,
                                      &min_consumed, &zero_consumed, &consumed, draw_micro);
          bst_db_labeling_draw_value (self, &event->area, &canvas, (db1 + db2) * 0.5,
                                      &min_consumed, &zero_consumed, &consumed, draw_minor);
        }
      /* redraw min */
      bst_db_labeling_draw_value (self, &event->area, &canvas, dbsetup->mindb, &zero_consumed, &zero_consumed, &min_consumed, draw_etched);
    }
  /* redraw zero */
  consumed.width = consumed.height = 0;
  bst_db_labeling_draw_value (self, &event->area, &canvas, 0, &consumed, &consumed, &zero_consumed, draw_etched);
  /* vline bar */
  if (draw_longitudinal)
    db_labeling_draw_vline (self, line_gc, allocation->x, allocation->y, dbsetup->offset, dbsetup->length, longitudinal_pos);
  
  return FALSE;
}

void
bst_db_labeling_setup (BstDBLabeling      *self,
                       BstDBSetup     *db_setup)
{
  bst_db_setup_unref (self->dbsetup);
  self->dbsetup = bst_db_setup_copy (db_setup);
  gtk_widget_queue_resize (GTK_WIDGET (self));
}

void
bst_db_labeling_set_border (BstDBLabeling  *self,
                            guint           border)
{
  if (self->border != border)
    {
      self->border = border;
      gtk_widget_queue_resize (GTK_WIDGET (self));
    }
}

static void
bst_db_labeling_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BstDBLabeling *self = BST_DB_LABELING (object);
  GtkWidget *widget = GTK_WIDGET (self);
  switch (prop_id)
    {
    case PROP_DRAW_VALUES:
      self->draw_values = g_value_get_boolean (value);
      gtk_widget_queue_resize (widget);
      break;
    case PROP_ORIENTATION:
      self->orientation = g_value_get_enum (value);
      gtk_widget_queue_resize (widget);
      break;
    case PROP_JUSTIFY:
      self->justify = g_value_get_enum (value);
      gtk_widget_queue_resize (widget);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bst_db_labeling_get_property (GObject     *object,
                              guint        prop_id,
                              GValue      *value,
                              GParamSpec  *pspec)
{
  BstDBLabeling *self = BST_DB_LABELING (object);
  switch (prop_id)
    {
    case PROP_DRAW_VALUES:
      g_value_set_boolean (value, self->draw_values);
      break;
    case PROP_ORIENTATION:
      g_value_set_enum (value, self->orientation);
      break;
    case PROP_JUSTIFY:
      g_value_set_enum (value, self->justify);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bst_db_labeling_class_init (BstDBLabelingClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  
  gobject_class->set_property = bst_db_labeling_set_property;
  gobject_class->get_property = bst_db_labeling_get_property;
  gobject_class->finalize = bst_db_labeling_finalize;
  
  object_class->destroy = bst_db_labeling_destroy;
  
  widget_class->size_request = bst_db_labeling_size_request;
  widget_class->size_allocate = bst_db_labeling_size_allocate;
  widget_class->expose_event = bst_db_labeling_expose;
  
  g_object_class_install_property (gobject_class, PROP_DRAW_VALUES,
                                   g_param_spec_boolean ("draw-values", _("Draw Values"), _("Adjust whether to draw dB values instead of lines"),
                                                         FALSE, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_ORIENTATION,
                                   g_param_spec_enum ("orientation", _("Orientation"), _("Choose horizontal or vertical orientation"),
                                                      GTK_TYPE_ORIENTATION, GTK_ORIENTATION_VERTICAL, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_JUSTIFY,
                                   g_param_spec_enum ("justify", _("Justify"), _("Adjust relative alignment of the values or bars to be drawn"),
                                                      GTK_TYPE_JUSTIFICATION, GTK_JUSTIFY_CENTER, G_PARAM_READWRITE));
}


/* --- DB Beam --- */
G_DEFINE_TYPE (BstDBBeam, bst_db_beam, GTK_TYPE_WIDGET);

static void
bst_db_beam_init (BstDBBeam *self)
{
  GtkWidget *widget = GTK_WIDGET (self);
  GTK_WIDGET_SET_FLAGS (self, GTK_NO_WINDOW);
  gtk_widget_show (widget);
  self->dbsetup = bst_db_setup_copy (bst_db_setup_get_default ());
  self->border = DEFAULT_BORDER;
  self->orientation = GTK_ORIENTATION_VERTICAL;
  self->currentdb = 0;
}

static void
bst_db_beam_destroy (GtkObject *object)
{
  // BstDBBeam *self = BST_DB_BEAM (object);
  GTK_OBJECT_CLASS (bst_db_beam_parent_class)->destroy (object);
}

static void
bst_db_beam_finalize (GObject *object)
{
  BstDBBeam *self = BST_DB_BEAM (object);
  bst_db_setup_unref (self->dbsetup);
  G_OBJECT_CLASS (bst_db_beam_parent_class)->finalize (object);
}

static void
bst_db_beam_size_request (GtkWidget      *widget,
                          GtkRequisition *requisition)
{
  BstDBBeam *self = BST_DB_BEAM (widget);
  const gboolean vertical = self->orientation == GTK_ORIENTATION_VERTICAL;
  guint thickness = vertical ? XTHICKNESS (self) : YTHICKNESS (self);

  /* font width */
  PangoRectangle irect = { 0, }, lrect = { 0 };
  PangoLayout *layout = gtk_widget_create_pango_layout (GTK_WIDGET (self), "z");
  pango_layout_get_pixel_extents (layout, &irect, &lrect);
  g_object_unref (layout);
  guint thick_beam = lrect.width;

  if (vertical)
    {
      requisition->width = thickness + thick_beam + thickness;
      requisition->height = 14 + 2 * YTHICKNESS (self);
    }
  else
    {
      requisition->width = 14 + 2 * XTHICKNESS (self);
      requisition->height = thickness + thick_beam + thickness;
    }
}

static void
db_beam_redraw_pixmap (BstDBBeam *self)
{
  GtkWidget *widget = GTK_WIDGET (self);
  GtkAllocation *allocation = &widget->allocation;
  const gboolean vertical = self->orientation == GTK_ORIENTATION_VERTICAL;
  BstDBSetup *dbsetup = self->dbsetup;
  if (self->pixmap)
    g_object_unref (self->pixmap);
  guint i, length = dbsetup->length;
  self->pixmap = gdk_pixmap_new (widget->window,
                                 vertical ? allocation->width : length * 2,
                                 vertical ? length * 2 : allocation->height,
                                 -1);
  GdkGC *lgc = gdk_gc_new (widget->window);
  GdkGC *dgc = gdk_gc_new (widget->window);
  for (i = 0; i < length; i++)
    {
      GdkColor color;
      color = gdk_color_from_rgb (bst_db_setup_get_color (dbsetup, dbsetup->offset + i, 1.0));
      gdk_gc_set_rgb_fg_color (lgc, &color);
      color = gdk_color_from_rgb (bst_db_setup_get_color (dbsetup, dbsetup->offset + i, 0.33));
      gdk_gc_set_rgb_fg_color (dgc, &color);
      if (vertical)
        {
          gdk_draw_line (self->pixmap, lgc, 0, i, allocation->width - 1, i);
          gdk_draw_line (self->pixmap, dgc, 0, length + i, allocation->width - 1, length + i);
        }
      else
        {
          gdk_draw_line (self->pixmap, lgc, i, 0, i, allocation->height - 1);
          gdk_draw_line (self->pixmap, dgc, length + i, 0, length + i, allocation->height - 1);
        }
    }
}

static void
bst_db_beam_size_allocate (GtkWidget     *widget,
                           GtkAllocation *allocation)
{
  BstDBBeam *self = BST_DB_BEAM (widget);
  const gboolean vertical = self->orientation == GTK_ORIENTATION_VERTICAL;
  widget->allocation = *allocation;
  guint thickness = vertical ? YTHICKNESS (self) : XTHICKNESS (self);
  db_setup_size_allocate (self->dbsetup, thickness, self->border, vertical ? allocation->height : allocation->width, vertical);
  if (GTK_WIDGET_REALIZED (self))
    db_beam_redraw_pixmap (self);
}

static void
bst_db_beam_realize (GtkWidget *widget)
{
  BstDBBeam *self = BST_DB_BEAM (widget);
  GTK_WIDGET_CLASS (bst_db_beam_parent_class)->realize (widget);
  db_beam_redraw_pixmap (self);
}

static void
bst_db_beam_unrealize (GtkWidget *widget)
{
  BstDBBeam *self = BST_DB_BEAM (widget);
  g_object_unref (self->pixmap);
  self->pixmap = NULL;
  GTK_WIDGET_CLASS (bst_db_beam_parent_class)->unrealize (widget);
}

static gboolean
bst_db_beam_expose (GtkWidget      *widget,
                    GdkEventExpose *event)
{
  BstDBBeam *self = BST_DB_BEAM (widget);
  GdkWindow *drawable = event->window;
  GtkAllocation *allocation = ALLOCATION (self);
  const gboolean vertical = self->orientation == GTK_ORIENTATION_VERTICAL;
  BstDBSetup *dbsetup = self->dbsetup;
  if (drawable != widget->window)
    return FALSE;
  if (0)
    {
      gdk_draw_line (widget->window, widget->style->black_gc, widget->allocation.x, widget->allocation.y,
                     widget->allocation.x + widget->allocation.width-1, widget->allocation.y + widget->allocation.height-1);
      gdk_draw_line (widget->window, widget->style->black_gc, widget->allocation.x + widget->allocation.width-1, widget->allocation.y,
                     widget->allocation.x, widget->allocation.y + widget->allocation.height-1);
    }

  /* subtract one pixel in length, since it doesn't make sense to always paint mindb (=silence) highlighted */
  if (vertical)
    gtk_paint_shadow (STYLE (self), drawable, STATE (self), GTK_SHADOW_IN, NULL, NULL, NULL,
                      allocation->x, allocation->y + dbsetup->offset - YTHICKNESS (self) + dbsetup->flipdir,
                      allocation->width, dbsetup->length + 2 * YTHICKNESS (self) - 1);
  else
    gtk_paint_shadow (STYLE (self), drawable, STATE (self), GTK_SHADOW_IN, NULL, NULL, NULL,
                      allocation->x + dbsetup->offset - XTHICKNESS (self) + dbsetup->flipdir, allocation->y,
                      dbsetup->length + 2 * XTHICKNESS (self) - 1, allocation->height);

  /* force complete beam redraw */
  double currentdb = self->currentdb;
  self->currentdb = G_MAXDOUBLE;
  bst_db_beam_set_value (self, currentdb);
  self->currentdb = -G_MAXDOUBLE;
  bst_db_beam_set_value (self, currentdb);

  return FALSE;
}

void
bst_db_beam_set_value (BstDBBeam      *self,
                       double          db)
{
  BstDBSetup *dbsetup = self->dbsetup;
  double olddb = CLAMP (self->currentdb, dbsetup->mindb, dbsetup->maxdb);
  self->currentdb = db;
  db = CLAMP (db, dbsetup->mindb, dbsetup->maxdb);
  if (GTK_WIDGET_REALIZED (self))
    {
      GtkWidget *widget = GTK_WIDGET (self);
      GdkWindow *drawable = widget->window;
      GtkAllocation *allocation = ALLOCATION (self);
      const gboolean vertical = self->orientation == GTK_ORIENTATION_VERTICAL;
      gint pold = bst_db_setup_get_pixel (dbsetup, olddb);
      gint pnew = bst_db_setup_get_pixel (dbsetup, db);
      GdkGC *draw_gc = widget->style->fg_gc[widget->state];
      guint thickness = vertical ? XTHICKNESS (self) : YTHICKNESS (self);
      gint pos = 0, offset = 0, length = 0;
      if (pold > pnew)
        {
          pos = pnew - dbsetup->offset;
          if (dbsetup->flipdir)
            pos += dbsetup->length;
          offset = pnew;
          length = pold - pnew;
        }
      else if (pnew > pold)
        {
          pos = pold - dbsetup->offset;
          if (!dbsetup->flipdir)
            pos += dbsetup->length;
          offset = pold;
          length = pnew - pold;
        }
      guint flipshift = dbsetup->flipdir;       /* acount for the 1 pixel mindb we won't paint */
      if (length)
        gdk_draw_drawable (drawable, draw_gc, self->pixmap,
                           vertical ? 0 : pos,
                           vertical ? pos : 0,
                           allocation->x + (vertical ? thickness : offset + flipshift),
                           allocation->y + (vertical ? offset + flipshift: thickness),
                           vertical ? allocation->width - 2 * thickness : length,
                           vertical ? length : allocation->height - 2 * thickness);
    }
}

void
bst_db_beam_set_border (BstDBBeam  *self,
                        guint       border)
{
  if (self->border != border)
    {
      self->border = border;
      gtk_widget_queue_resize (GTK_WIDGET (self));
    }
}

void
bst_db_beam_setup (BstDBBeam      *self,
                   BstDBSetup     *db_setup)
{
  bst_db_setup_unref (self->dbsetup);
  self->dbsetup = bst_db_setup_copy (db_setup);
  gtk_widget_queue_resize (GTK_WIDGET (self));
}

static void
bst_db_beam_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  BstDBBeam *self = BST_DB_BEAM (object);
  GtkWidget *widget = GTK_WIDGET (self);
  switch (prop_id)
    {
    case PROP_ORIENTATION:
      self->orientation = g_value_get_enum (value);
      gtk_widget_queue_resize (widget);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bst_db_beam_get_property (GObject     *object,
                          guint        prop_id,
                          GValue      *value,
                          GParamSpec  *pspec)
{
  BstDBBeam *self = BST_DB_BEAM (object);
  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, self->orientation);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bst_db_beam_class_init (BstDBBeamClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  
  gobject_class->set_property = bst_db_beam_set_property;
  gobject_class->get_property = bst_db_beam_get_property;
  gobject_class->finalize = bst_db_beam_finalize;
  
  object_class->destroy = bst_db_beam_destroy;
  
  widget_class->size_request = bst_db_beam_size_request;
  widget_class->size_allocate = bst_db_beam_size_allocate;
  widget_class->realize = bst_db_beam_realize;
  widget_class->unrealize = bst_db_beam_unrealize;
  widget_class->expose_event = bst_db_beam_expose;
  
  g_object_class_install_property (gobject_class, PROP_ORIENTATION,
                                   g_param_spec_enum ("orientation", _("Orientation"), _("Choose horizontal or vertical orientation"),
                                                      GTK_TYPE_ORIENTATION, GTK_ORIENTATION_VERTICAL, G_PARAM_READWRITE));
}


/* --- DB Meter --- */
G_DEFINE_TYPE (BstDBMeter, bst_db_meter, GTK_TYPE_ALIGNMENT);

static void
bst_db_meter_init (BstDBMeter *self)
{
  self->dbsetup = bst_db_setup_copy (bst_db_setup_get_default ());
  self->border = DEFAULT_BORDER;
  self->orientation = GTK_ORIENTATION_VERTICAL;
  g_object_set (self,
                "visible", TRUE,
                "xalign", 0.5,
                "yalign", 0.5,
                NULL);
}

static void
bst_db_meter_size_allocate (GtkWidget     *widget,
                            GtkAllocation *allocation)
{
  BstDBMeter *self = BST_DB_METER (widget);
  GTK_WIDGET_CLASS (bst_db_meter_parent_class)->size_allocate (widget, allocation);
  const gboolean vertical = self->orientation == GTK_ORIENTATION_VERTICAL;
  guint thickness = vertical ? YTHICKNESS (self) : XTHICKNESS (self);
  db_setup_size_allocate (self->dbsetup, thickness, self->border,
                          vertical ? widget->allocation.height : widget->allocation.width, vertical);
}

static void     db_meter_build_channels (BstDBMeter *self,
                                         guint       n_channels);

static void
bst_db_meter_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  BstDBMeter *self = BST_DB_METER (object);
  switch (prop_id)
    {
    case PROP_ORIENTATION:
      if (!GTK_BIN (self)->child)
        {
          self->orientation = g_value_get_enum (value);
          const gboolean vertical = self->orientation == GTK_ORIENTATION_VERTICAL;
          g_object_new (vertical ? GTK_TYPE_HBOX : GTK_TYPE_VBOX,
                        "visible", TRUE,
                        "parent", self,
                        NULL);
          g_object_set (self,
                        vertical ? "xscale" : "yscale", 0.0,
                        vertical ? "yscale" : "xscale", 1.0,
                        NULL);
        }
      break;
    case PROP_N_CHANNELS:
      db_meter_build_channels (self, g_value_get_uint (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
      break;
    }
}

static void
bst_db_meter_get_property (GObject     *object,
                           guint        prop_id,
                           GValue      *value,
                           GParamSpec  *pspec)
{
  BstDBMeter *self = BST_DB_METER (object);
  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, self->orientation);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
      break;
    }
}

GtkWidget*
bst_db_meter_new (GtkOrientation  orientation,
                  guint           n_channels)
{
  BstDBMeter *self = g_object_new (BST_TYPE_DB_METER, "orientation", orientation, "n-channels", n_channels, NULL);
  return GTK_WIDGET (self);
}

static void
bst_db_meter_destroy (GtkObject *object)
{
  // BstDBMeter *self = BST_DB_METER (object);
  GTK_OBJECT_CLASS (bst_db_meter_parent_class)->destroy (object);
}

static void
bst_db_meter_finalize (GObject *object)
{
  BstDBMeter *self = BST_DB_METER (object);
  bst_db_setup_unref (self->dbsetup);
  self->dbsetup = NULL;
  G_OBJECT_CLASS (bst_db_meter_parent_class)->finalize (object);
}

static void
db_meter_setup_recursive (GtkWidget *widget,
                          gpointer   data)
{
  BstDBSetup *dbsetup = data;
  if (BST_IS_DB_BEAM (widget))
    bst_db_beam_setup (BST_DB_BEAM (widget), dbsetup);
  else if (BST_IS_DB_LABELING (widget))
    bst_db_labeling_setup (BST_DB_LABELING (widget), dbsetup);
  if (GTK_IS_CONTAINER (widget))
    gtk_container_foreach (GTK_CONTAINER (widget), db_meter_setup_recursive, data);
}

void
bst_db_meter_propagate_setup (BstDBMeter     *self,
                              BstDBSetup     *db_setup)
{
  g_return_if_fail (BST_IS_DB_METER (self));
  g_return_if_fail (db_setup != NULL);
  bst_db_setup_unref (self->dbsetup);
  self->dbsetup = bst_db_setup_copy (db_setup);
  db_meter_setup_recursive (GTK_WIDGET (self), self->dbsetup);
}

static void
db_meter_set_border_recursive (GtkWidget *widget,
                               gpointer   data)
{
  guint border = GPOINTER_TO_UINT (data);
  if (BST_IS_DB_BEAM (widget))
    bst_db_beam_set_border (BST_DB_BEAM (widget), border);
  else if (BST_IS_DB_LABELING (widget))
    bst_db_labeling_set_border (BST_DB_LABELING (widget), border);
  if (GTK_IS_CONTAINER (widget))
    gtk_container_foreach (GTK_CONTAINER (widget), db_meter_set_border_recursive, data);
}

void
bst_db_meter_propagate_border (BstDBMeter     *self,
                               guint           border)
{
  g_return_if_fail (BST_IS_DB_METER (self));
  if (self->border != border)
    {
      self->border = border;
      db_meter_set_border_recursive (GTK_WIDGET (self), GUINT_TO_POINTER (self->border));
    }
}

BstDBBeam*
bst_db_meter_create_beam (BstDBMeter     *self,
                          guint           padding)
{
  GtkBox *box = (void*) GTK_BIN (self)->child;
  BstDBBeam *aux = NULL;
  if (GTK_IS_BOX (box))
    {
      aux = g_object_new (BST_TYPE_DB_BEAM,
                          "orientation", self->orientation,
                          NULL);
      bst_db_beam_setup (aux, self->dbsetup);
      bst_db_beam_set_border (aux, self->border);
      gtk_box_pack_start (box, GTK_WIDGET (aux), FALSE, TRUE, padding);
    }
  return aux;
}

BstDBLabeling*
bst_db_meter_create_numbers (BstDBMeter     *self,
                             guint           padding)
{
  GtkBox *box = (void*) GTK_BIN (self)->child;
  BstDBLabeling *aux = NULL;
  if (GTK_IS_BOX (box))
    {
      aux = g_object_new (BST_TYPE_DB_LABELING,
                          "orientation", self->orientation,
                          "draw-values", TRUE,
                          NULL);
      bst_db_labeling_setup (aux, self->dbsetup);
      bst_db_labeling_set_border (aux, self->border);
      gtk_box_pack_start (box, GTK_WIDGET (aux), FALSE, TRUE, padding);
    }
  return aux;
}

BstDBLabeling*
bst_db_meter_create_dashes (BstDBMeter      *self,
                            GtkJustification justify,
                            guint            padding)
{
  GtkBox *box = (void*) GTK_BIN (self)->child;
  BstDBLabeling *aux = NULL;
  if (GTK_IS_BOX (box))
    {
      aux = g_object_new (BST_TYPE_DB_LABELING,
                          "orientation", self->orientation,
                          "draw-values", FALSE,
                          "justify", justify,
                          NULL);
      bst_db_labeling_setup (aux, self->dbsetup);
      bst_db_labeling_set_border (aux, self->border);
      gtk_box_pack_start (box, GTK_WIDGET (aux), FALSE, TRUE, padding);
    }
  return aux;
}

static void
db_scale_pixel_adjustment_value_changed (GtkAdjustment *adjustment,
                                         GxkParam      *param)
{
  if (param->updating)
    return;
  BstDBSetup *dbsetup = g_object_get_data (adjustment, "BstDBSetup");
  double value = bst_db_setup_get_dbvalue (dbsetup, adjustment->value);
  GValue dvalue = { 0, };
  g_value_init (&dvalue, G_TYPE_DOUBLE);
  g_value_set_double (&dvalue, value);
  g_value_transform (&dvalue, &param->value);
  g_value_unset (&dvalue);
  gxk_param_apply_value (param);
}

static void
db_scale_pixel_adjustment_update (GxkParam       *param,
                                  GtkObject      *object)
{
  BstDBSetup *dbsetup = g_object_get_data (object, "BstDBSetup");
  GValue dvalue = { 0, };
  g_value_init (&dvalue, G_TYPE_DOUBLE);
  g_value_transform (&param->value, &dvalue);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (object), bst_db_setup_get_pixel (dbsetup, g_value_get_double (&dvalue)));
  g_value_unset (&dvalue);
}

void
bst_db_scale_hook_up_param (GtkRange     *range,
                            GxkParam     *param)
{
  gchar *tooltip = gxk_param_dup_tooltip (param);
  gtk_tooltips_set_tip (GXK_TOOLTIPS, GTK_WIDGET (range), tooltip, NULL);
  g_free (tooltip);
  gboolean updating = param->updating;
  param->updating = TRUE;       /* protect value from change-notifications during setup */
  // FIXME: update adjustment // range = editor->create_widget (param, tooltip, editor->variant);
  param->updating = updating;
  gxk_param_add_object (param, GTK_OBJECT (range));
  GtkAdjustment *adjustment = gtk_range_get_adjustment (range);
  gxk_object_set_param_callback (GTK_OBJECT (adjustment), db_scale_pixel_adjustment_update);
  gxk_param_add_object (param, GTK_OBJECT (adjustment));
  /* catch notifies *after* the widgets are updated */
  g_object_connect (adjustment, "signal_after::value-changed", db_scale_pixel_adjustment_value_changed, param, NULL);
}

static void
db_scale_size_allocate (GtkRange      *range,
                        GtkAllocation *dummy,
                        BstDBMeter    *dbmeter)
{
  GtkWidget *widget = GTK_WIDGET (range);
  const gboolean vertical = range->orientation == GTK_ORIENTATION_VERTICAL;
  GtkAllocation *allocation = &GTK_WIDGET (range)->allocation;
  gint border = 0;
  /* calculate db meter border */
  gint woffset = vertical ? allocation->y : allocation->x;
  gint wlength = vertical ? allocation->height : allocation->width;
  gint toffset = vertical ? range->range_rect.y : range->range_rect.x;
  gint tlength = vertical ? range->range_rect.height : range->range_rect.width;
  gint slider = range->slider_end - range->slider_start;
  gint range_trough_border = 0, focus_line_width = 0, focus_padding = 0;
  gtk_widget_style_get (widget,
                        "trough-border", &range_trough_border,
                        "focus-line-width", &focus_line_width,
                        "focus-padding", &focus_padding,
                        NULL);
  if (GTK_WIDGET_CAN_FOCUS (range))
    range_trough_border += focus_line_width + focus_padding;
  tlength = CLAMP (tlength, 1, wlength);
  slider = CLAMP (slider, 1, tlength);
  if (0)        // GTKFIX: GtkRange should simply export the slide rectangle through a function call
    g_printerr ("y=%d h=%d ty=%d th=%d trough-borders=%d slider=%d (start=%d)\n",
                woffset, wlength, toffset, tlength, range_trough_border, slider, range->slider_start);
  border = toffset + slider / 2;
  if (range_trough_border && range_trough_border + slider <= tlength / 2)
    border += range_trough_border;
  /* adjust dbsetup */
  BstDBSetup *dbsetup = g_object_get_data (range, "BstDBSetup");
  if (dbsetup)
    {
      guint thickness = vertical ? YTHICKNESS (range) : XTHICKNESS (range);
      db_setup_size_allocate (dbsetup, thickness, border, vertical ? allocation->height : allocation->width, vertical);
    }
  /* adapt adjustment to dbsetup */
  GtkAdjustment *adjustment = gtk_range_get_adjustment (range);
  if (dbsetup && adjustment)
    {
      double pmin = bst_db_setup_get_pixel (dbsetup, dbsetup->mindb);
      double pmax = bst_db_setup_get_pixel (dbsetup, dbsetup->maxdb);
      adjustment->lower = MIN (pmin, pmax);
      adjustment->upper = MAX (pmin, pmax);
      gint pzer = bst_db_setup_get_pixel (dbsetup, 0);
      gint page = bst_db_setup_get_pixel (dbsetup, -6) - pzer;
      adjustment->page_increment = ABS (page);
      adjustment->page_increment = MIN ((adjustment->upper - adjustment->lower) / 4, adjustment->page_increment);
      adjustment->step_increment = adjustment->page_increment / 4;
      // FIXME: adjust adjustment->value
      gtk_adjustment_changed (adjustment);
      // FIXME: junk:
      gtk_adjustment_set_value (adjustment, bst_db_setup_get_pixel (dbsetup, 0));
    }
  /* propagate db meter border */
  bst_db_meter_propagate_border (dbmeter, border);
}

GtkRange*
bst_db_meter_create_scale (BstDBMeter *self,
                           guint       padding)
{
  GtkBox *box = (void*) GTK_BIN (self)->child;
  GtkRange *range = NULL;
  if (GTK_IS_BOX (box))
    {
      BstDBSetup *dbsetup = bst_db_setup_copy (self->dbsetup);
      GtkAdjustment *adjustment = (void*) gtk_adjustment_new (bst_db_setup_get_pixel (dbsetup, 0),
                                                              bst_db_setup_get_pixel (dbsetup, dbsetup->mindb),
                                                              bst_db_setup_get_pixel (dbsetup, dbsetup->maxdb),
                                                              0, 0, 0);
      range = g_object_new (self->orientation == GTK_ORIENTATION_VERTICAL ? GTK_TYPE_VSCALE : GTK_TYPE_HSCALE,
                            "visible", TRUE,
                            "draw_value", FALSE,
                            "adjustment", adjustment,
                            "can-focus", FALSE,
                            NULL);
      gtk_box_pack_start (box, GTK_WIDGET (range), FALSE, TRUE, padding);
      g_object_set_data_full (range, "BstDBSetup", dbsetup, bst_db_setup_unref);
      g_object_set_data_full (adjustment, "BstDBSetup", bst_db_setup_ref (dbsetup), bst_db_setup_unref);
      if (!gxk_signal_handler_pending (range, "size-allocate", G_CALLBACK (db_scale_size_allocate), self))
        g_signal_connect_object (range, "size-allocate", G_CALLBACK (db_scale_size_allocate), self, G_CONNECT_AFTER);
    }
  return range;
}

static gpointer
db_meter_get_child (BstDBMeter     *self,
                    guint           nth,
                    GType           ctype)
{
  GtkBox *box = (void*) GTK_BIN (self)->child;
  GtkWidget *child = NULL;
  if (GTK_IS_BOX (box))
    {
      GList *node, *children = gtk_container_get_children (GTK_CONTAINER (box));
      for (node = children; node; node = node->next)
        if (g_type_is_a (G_OBJECT_TYPE (node->data), ctype))
          {
            if (nth)
              nth--;
            else
              {
                child = node->data;
                break;
              }
          }
      g_list_free (children);
    }
  return child;
}

GtkRange*
bst_db_meter_get_scale (BstDBMeter     *self,
                        guint           nth)
{
  return db_meter_get_child (self, nth, GTK_TYPE_RANGE);
}

BstDBBeam*
bst_db_meter_get_beam (BstDBMeter     *self,
                       guint           nth)
{
  return db_meter_get_child (self, nth, BST_TYPE_DB_BEAM);
}

BstDBLabeling*
bst_db_meter_get_labeling (BstDBMeter     *self,
                           guint           nth)
{
  return db_meter_get_child (self, nth, BST_TYPE_DB_LABELING);
}

static void
db_meter_build_channels (BstDBMeter *self,
                         guint       n_channels)
{
  const gint padding = 2;
  if (n_channels == 1)
    {
      /* scale + dash + number + dash + beam */
      bst_db_meter_create_scale (self, padding);
      bst_db_meter_create_dashes (self, GTK_JUSTIFY_LEFT, padding);
      bst_db_meter_create_numbers (self, padding);
      bst_db_meter_create_dashes (self, GTK_JUSTIFY_RIGHT, padding);
      bst_db_meter_create_beam (self, padding);
    }
  else if (n_channels == 2)
    {
      /* scale + dash + beam */
      bst_db_meter_create_scale (self, padding);
      bst_db_meter_create_dashes (self, GTK_JUSTIFY_FILL, padding);
      bst_db_meter_create_beam (self, padding);
      /* dash + number + dash */
      bst_db_meter_create_dashes (self, GTK_JUSTIFY_LEFT, padding);
      bst_db_meter_create_numbers (self, padding);
      bst_db_meter_create_dashes (self, GTK_JUSTIFY_RIGHT, padding);
      /* beam + dash + scale */
      bst_db_meter_create_beam (self, padding);
      bst_db_meter_create_dashes (self, GTK_JUSTIFY_FILL, padding);
      bst_db_meter_create_scale (self, padding);
    }
}

static void
bst_db_meter_class_init (BstDBMeterClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  
  gobject_class->set_property = bst_db_meter_set_property;
  gobject_class->get_property = bst_db_meter_get_property;
  gobject_class->finalize = bst_db_meter_finalize;
  
  object_class->destroy = bst_db_meter_destroy;

  widget_class->size_allocate = bst_db_meter_size_allocate;

  g_object_class_install_property (gobject_class, PROP_ORIENTATION,
                                   g_param_spec_enum ("orientation", _("Orientation"), _("Choose horizontal or vertical orientation"),
                                                      GTK_TYPE_ORIENTATION, GTK_ORIENTATION_VERTICAL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_N_CHANNELS,
                                   g_param_spec_uint ("n-channels", NULL, NULL, 0, G_MAXINT, 0, G_PARAM_WRITABLE));
}
