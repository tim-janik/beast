/* BEAST - Better Audio System
 * Copyright (C) 2004 Tim Janik
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
#include "bstpatternview.h"
#include "bstskinconfig.h"
#include <string.h>


/* --- defines --- */
/* accessors */
#define	STYLE(self)		(GTK_WIDGET (self)->style)
#define	STATE(self)		(GTK_WIDGET (self)->state)
#define	SELECTED_STATE(self)	(GTK_WIDGET_IS_SENSITIVE (self) ? GTK_STATE_SELECTED : GTK_STATE_INSENSITIVE)
#define	ACTIVE_STATE(self)	(GTK_WIDGET_IS_SENSITIVE (self) ? GTK_STATE_ACTIVE : GTK_STATE_INSENSITIVE)
#define	XTHICKNESS(self)	(STYLE (self)->xthickness)
#define	YTHICKNESS(self)	(STYLE (self)->ythickness)
#define	ALLOCATION(self)	(&GTK_WIDGET (self)->allocation)
#define X_OFFSET(self)          (GXK_SCROLL_CANVAS (self)->x_offset)
#define Y_OFFSET(self)          (GXK_SCROLL_CANVAS (self)->y_offset)
#define PLAYOUT(self, i)        (gxk_scroll_canvas_get_pango_layout (GXK_SCROLL_CANVAS (self), i))
#define PLAYOUT_HPANEL(self)    (PLAYOUT (self, 0))
#define PLAYOUT_VPANEL(self)    (PLAYOUT (self, 1))
#define PLAYOUT_CANVAS(self)    (PLAYOUT (self, 2))
#define COLUMN_PLAYOUT_INDEX(i) (i + 3)
#define COLOR_GC(self, i)       (GXK_SCROLL_CANVAS (self)->color_gc[i])
#define COLOR_GC_SHADE1(self)   (COLOR_GC (self, CINDEX_SHADE1))
#define COLOR_GC_SHADE2(self)   (COLOR_GC (self, CINDEX_SHADE2))
#define COLOR_GC_TEXT0(self)    (COLOR_GC (self, CINDEX_TEXT0))
#define COLOR_GC_TEXT1(self)    (COLOR_GC (self, CINDEX_TEXT1))
#define COLOR_GC_VBAR(self)     (COLOR_GC (self, CINDEX_VBAR))
#define CANVAS(self)            (GXK_SCROLL_CANVAS (self)->canvas)
#define HPANEL(self)            (GXK_SCROLL_CANVAS (self)->top_panel)
#define VPANEL(self)            (GXK_SCROLL_CANVAS (self)->left_panel)
#define FOCUS_WIDTH(self)       (bst_pattern_view_get_focus_width (self))
/* layout (allocation) */
#define	CMARK_HEIGHT(self)      (YTHICKNESS (self) * 3 + 1)
#define	PMARK_HEIGHT(self)      (CMARK_HEIGHT (self) + 3 * YTHICKNESS (self))

#define CUSTOM_MARKER_OFFSET    (128)


/* --- prototypes --- */
static void     pattern_view_queue_expose       (BstPatternView *self,
                                                 guint           tick_start,
                                                 guint           tick_end);


/* --- static variables --- */
static guint	signal_drag = 0;
static guint	signal_clicked = 0;


/* --- functions --- */
G_DEFINE_TYPE (BstPatternView, bst_pattern_view, GXK_TYPE_SCROLL_CANVAS);

enum {
  CINDEX_SHADE1,
  CINDEX_SHADE2,
  CINDEX_TEXT0,
  CINDEX_TEXT1,
  CINDEX_VBAR,
  CINDEX_LAST
};

static void
pattern_view_class_setup_skin (BstPatternViewClass *klass)
{
  static GdkColor colors[CINDEX_LAST];
  GxkScrollCanvasClass *scroll_canvas_class = GXK_SCROLL_CANVAS_CLASS (klass);
  scroll_canvas_class->n_colors = G_N_ELEMENTS (colors);
  scroll_canvas_class->colors = colors;
  colors[CINDEX_SHADE1] = gdk_color_from_rgb (BST_SKIN_CONFIG (pattern_scolor1));
  colors[CINDEX_SHADE2] = gdk_color_from_rgb (BST_SKIN_CONFIG (pattern_scolor2));
  colors[CINDEX_TEXT0] = gdk_color_from_rgb (BST_SKIN_CONFIG (pattern_text0));
  colors[CINDEX_TEXT1] = gdk_color_from_rgb (BST_SKIN_CONFIG (pattern_text1));
  colors[CINDEX_VBAR] = gdk_color_from_rgb (BST_SKIN_CONFIG (pattern_vbar1));
  g_free (scroll_canvas_class->image_file_name);
  scroll_canvas_class->image_file_name = BST_SKIN_CONFIG_STRDUP_PATH (pattern_image);
  scroll_canvas_class->image_tint = gdk_color_from_rgb (BST_SKIN_CONFIG (pattern_color));
  scroll_canvas_class->image_saturation = BST_SKIN_CONFIG (pattern_shade) * 0.01;
  gxk_scroll_canvas_class_skin_changed (scroll_canvas_class);
}

static void
bst_pattern_view_init (BstPatternView *self)
{
  GxkScrollCanvas *scc = GXK_SCROLL_CANVAS (self);
  
  self->row_height = 1;
  self->tpt = 384 * 4;
  self->max_ticks = 1;
  self->vticks = 384;
  gxk_scroll_canvas_set_canvas_cursor (scc, GDK_LEFT_PTR);
  gxk_scroll_canvas_set_left_panel_cursor (scc, GDK_HAND2);
  gxk_scroll_canvas_set_top_panel_cursor (scc, GDK_LEFT_PTR);
  bst_pattern_view_vsetup (self, 384, 4, 800 * 384, 384);
  bst_pattern_view_set_layout (self, ("_ "
                                      "offset-0 : note-0 : velocity-0 : fine-tune-0 : length-0 | "
                                      "offset-1 : note-1 : velocity-1 : fine-tune-1 : length-1 | "
                                      "offset-2 : note-2 : velocity-2 : fine-tune-2 : length-2 | "
                                      "offset-3 : note-3 : velocity-3 : fine-tune-3 : length-3 | "
                                      "control-4 | control-7 | control-8"));
}

static void
bst_pattern_view_dispose (GObject *object)
{
  BstPatternView *self = BST_PATTERN_VIEW (object);
  
  bst_pattern_view_set_proxy (self, 0);
  
  G_OBJECT_CLASS (bst_pattern_view_parent_class)->dispose (object);
}

static void
bst_pattern_view_destroy_columns (BstPatternView *self)
{
  bst_pattern_view_set_pixmarker (self, 1, BST_PATTERN_VIEW_MARKER_FOCUS, -1, -1, 1, 1);
  bst_pattern_view_set_pixmarker (self, 2, BST_PATTERN_VIEW_MARKER_FOCUS, -1, -1, 1, 1);
  bst_pattern_view_set_pixmarker (self, 3, BST_PATTERN_VIEW_MARKER_FOCUS, -1, -1, 1, 1);
  bst_pattern_view_set_pixmarker (self, 4, BST_PATTERN_VIEW_MARKER_FOCUS, -1, -1, 1, 1);
  bst_pattern_view_set_pixmarker (self, 5, BST_PATTERN_VIEW_MARKER_FOCUS, -1, -1, 1, 1);
  while (self->n_cols)
    {
      self->n_cols--;
      self->cols[self->n_cols]->klass->finalize (self->cols[self->n_cols]);
    }
  self->n_cols = 0;
  g_free (self->cols);
  self->cols = NULL;
  self->n_focus_cols = 0;
  g_free (self->focus_cols);
  self->focus_cols = NULL;
  gtk_widget_queue_resize (GTK_WIDGET (self));
  g_free (self->layout_string);
  self->layout_string = NULL;
  gxk_scroll_canvas_reset_pango_layouts (GXK_SCROLL_CANVAS (self));
}

static void
bst_pattern_view_finalize (GObject *object)
{
  BstPatternView *self = BST_PATTERN_VIEW (object);

  bst_pattern_view_set_proxy (self, 0);
  bst_pattern_view_destroy_columns (self);
  
  G_OBJECT_CLASS (bst_pattern_view_parent_class)->finalize (object);
}

static void
pattern_view_update (BstPatternView *self,
                     guint           tick,
                     guint           duration,
                     gint            min_note,
                     gint            max_note)
{
  duration = MAX (duration, 1);
  if (GTK_WIDGET_DRAWABLE (self))
    pattern_view_queue_expose (self, tick, tick + duration - 1);
}

static void
pattern_view_release_proxy (BstPatternView *self)
{
  gxk_toplevel_delete (GTK_WIDGET (self));
  bst_pattern_view_set_proxy (self, 0);
}

static void
pattern_view_range_changed (BstPatternView *self)
{
  guint max_ticks;
  bse_proxy_get (self->proxy, "last-tick", &max_ticks, NULL);
  bst_pattern_view_vsetup (self, 384, 4, MAX (max_ticks, 1), self->vticks);
}

void
bst_pattern_view_set_proxy (BstPatternView *self,
                            SfiProxy        proxy)
{
  g_return_if_fail (BST_PATTERN_VIEW (self));
  if (proxy)
    {
      g_return_if_fail (BSE_IS_PART (proxy));
      g_return_if_fail (bse_item_get_project (proxy) != 0);
    }
  
  if (self->proxy)
    {
      bse_proxy_disconnect (self->proxy,
                            "any_signal", pattern_view_release_proxy, self,
                            "any_signal", pattern_view_range_changed, self,
                            "any_signal", pattern_view_update, self,
                            NULL);
      bse_item_unuse (self->proxy);
    }
  self->proxy = proxy;
  if (self->proxy)
    {
      bse_item_use (self->proxy);
      bse_proxy_connect (self->proxy,
                         "swapped_signal::release", pattern_view_release_proxy, self,
                         "swapped_signal::property-notify::last-tick", pattern_view_range_changed, self,
                         "swapped_signal::range-changed", pattern_view_update, self,
                         NULL);
      pattern_view_range_changed (self);
    }
  gtk_widget_queue_resize (GTK_WIDGET (self));
}

static PangoLayout*
pattern_view_column_pango_layout (BstPatternView   *self,
                                  BstPatternColumn *col)
{
  GxkScrollCanvas *scc = GXK_SCROLL_CANVAS (self);
  uint i;
  for (i = 0; i < self->n_cols; i++)
    if (self->cols[i] == col)
      break;
  return gxk_scroll_canvas_peek_pango_layout (scc, COLUMN_PLAYOUT_INDEX (i));
}

static void
pattern_view_get_layout (GxkScrollCanvas        *scc,
                         GxkScrollCanvasLayout  *layout)
{
  BstPatternView *self = BST_PATTERN_VIEW (scc);
  PangoFontDescription *fdesc;
  PangoRectangle rect = { 0 };
  gchar buffer[64];
  guint i, accu = 0;

  /* hpanel writings */
  pango_layout_get_pixel_extents (PLAYOUT_HPANEL (self), NULL, &rect);
  layout->top_panel_height = rect.height;

  /* vpanel numbers */
  fdesc = pango_font_description_new ();
  pango_font_description_set_family_static (fdesc, "monospace");
  // pango_font_description_set_weight (fdesc, PANGO_WEIGHT_BOLD);
  pango_font_description_merge (fdesc, STYLE (self)->font_desc, FALSE);
  pango_layout_set_font_description (PLAYOUT_VPANEL (self), fdesc);
  pango_font_description_free (fdesc);
  g_snprintf (buffer, 64, "%05x", self->max_ticks);
  pango_layout_set_text (PLAYOUT_VPANEL (self), buffer, -1);
  pango_layout_get_pixel_extents (PLAYOUT_VPANEL (self), NULL, &rect);
  layout->left_panel_width = rect.width + 2 * XTHICKNESS (self);

  layout->right_panel_width = 0;
  layout->bottom_panel_height = 0;

  /* canvas size */
  pango_layout_get_pixel_extents (PLAYOUT_CANVAS (self), NULL, &rect);
  self->row_height = rect.height + FOCUS_WIDTH (self);
  layout->canvas_height = self->row_height;
  for (i = 0; i < self->n_cols; i++)
    {
      BstPatternColumn *col = self->cols[i];
      PangoLayout *playout = gxk_scroll_canvas_peek_pango_layout (scc, COLUMN_PLAYOUT_INDEX (i));
      if (!playout && col->klass->create_font_desc)
        {
          PangoFontDescription *fdesc = col->klass->create_font_desc (col);
          pango_font_description_merge (fdesc, STYLE (self)->font_desc, FALSE);
          playout = gxk_scroll_canvas_get_pango_layout (scc, COLUMN_PLAYOUT_INDEX (i));
          pango_layout_set_font_description (playout, fdesc);
          pango_font_description_free (fdesc);
        }
      accu += self->cols[i]->width = self->cols[i]->klass->width_request (self->cols[i], self, CANVAS (self), playout, self->vticks);
    }
  layout->canvas_width = accu;
}

static gint
tick_to_row (BstPatternView *self,
             gint            tick)
{
  double row = tick / (double) self->vticks;
  return MIN (row, G_MAXINT);
}

static gint
last_visible_row (BstPatternView *self)
{
  GxkScrollCanvas *scc = GXK_SCROLL_CANVAS (self);
  gint model_row = tick_to_row (self, self->max_ticks - 1);
  gint view_row = (-1 + (gint) scc->vadjustment->upper) / self->row_height;
  return MAX (model_row, view_row);
}

static void
row_to_ticks (BstPatternView *self,
              gint            row,
              gint           *tick_p,
              gint           *duration_p)
{
  int tick = MIN (G_MAXINT - self->vticks, row * (double) self->vticks);
  if (tick_p)
    *tick_p = tick;
  if (duration_p)
    *duration_p = self->vticks;
}

static gint
pixels_to_row_unscrolled (BstPatternView *self,
                          gint            y)
{
  double row = y / (double) self->row_height;
  return MIN (G_MAXINT, row);
}

static gint
coord_to_row (BstPatternView *self,
              gint            y,
              gboolean       *is_valid)
{
  gint row = pixels_to_row_unscrolled (self, y + Y_OFFSET (self));
  if (is_valid)
    *is_valid = row >= 0 && row <= last_visible_row (self);
  return row;
}

static void
row_to_pixels_unscrolled (BstPatternView *self,
                          gint            row,
                          gint           *y_p,
                          gint           *height_p)
{
  if (y_p)
    *y_p = MIN (G_MAXINT, row * (double) self->row_height);
  if (height_p)
    *height_p = self->row_height;
}

static gboolean
row_to_coords (BstPatternView *self,
	       gint            row,
	       gint           *y_p,
	       gint           *height_p)
{
  row_to_pixels_unscrolled (self, row, y_p, height_p);
  if (y_p)
    *y_p -= Y_OFFSET (self);
  if (height_p)
    *height_p = self->row_height;
  return row >= 0 && row <= last_visible_row (self);
}

static gint
coord_to_focus_col (BstPatternView *self,
                    gint            x,
                    gboolean       *is_valid)
{
  int i, vwidth = 0, valid = 0, focus_col = G_MAXINT, dist = G_MAXINT;
  for (i = 0; i < self->n_focus_cols; i++)
    {
      int ldist = x - self->focus_cols[i]->x;
      int rdist = x - self->focus_cols[i]->x - self->focus_cols[i]->width;
      if (ABS (ldist) < dist || ABS (rdist) < dist)
        {
          focus_col = i;
          dist = MIN (ABS (ldist), ABS (rdist));
        }
      vwidth += self->focus_cols[i]->width;
    }
  if (focus_col < self->n_focus_cols)
    valid = x >= self->focus_cols[focus_col]->x && x < self->focus_cols[focus_col]->x + self->focus_cols[focus_col]->width;
  else if (x < 0 && self->n_focus_cols)
    focus_col = x / (vwidth / self->n_focus_cols);
  if (focus_col >= 0 && focus_col < self->n_focus_cols &&
      self->focus_cols[focus_col]->n_focus_positions > 1)
    {
      BstPatternColumn *col = self->focus_cols[focus_col];
      GdkRectangle rect;
      int tick, duration, fx, fw, pos = 0, cx = x - col->x;
      row_to_ticks (self, self->focus_row, &tick, &duration);
      row_to_coords (self, self->focus_row, &rect.y, &rect.height);
      rect.x = col->x;
      rect.width = col->width;
      dist = G_MAXINT;
      for (i = 0; i < col->n_focus_positions; i++)
        {
          col->klass->get_focus_pos (col, self, CANVAS (self),
                                     pattern_view_column_pango_layout (self, col),
                                     tick, duration, &rect, i, &fx, &fw);
          fx += fw / 2;
          if (ABS (fx - cx) < dist)
            {
              dist = ABS (fx - cx);
              pos = i;
            }
        }
      focus_col += pos;
    }
  if (is_valid)
    *is_valid = valid;
  return focus_col;
}

#if 0
static gint
ticks_to_pixels (BstPatternView *self,
		 gint	         ticks)
{
  gdouble rppt = self->row_height / (double) self->vticks;      /* row-pixels per tick */
  gint pixels = ticks * rppt;
  if (ticks)
    pixels = MAX (pixels, 1);
  return pixels;
}

static gint
pixels_to_ticks (BstPatternView *self,
		 gint	         pixels)
{
  gdouble rtpp = self->vticks / (double) self->row_height;      /* row-ticks per pixel */
  gint ticks = pixels * rtpp;
  if (pixels > 0)
    ticks = MAX (ticks, 1);
  else
    ticks = 0;
  return ticks;
}

static gint
coord_to_tick (BstPatternView *self,
	       gint	       y,
	       gboolean	       extended_bound)
{
  guint tick;
  
  y += Y_OFFSET (self);
  tick = pixels_to_ticks (self, y);
  if (extended_bound)
    {
      guint tick2 = pixels_to_ticks (self, y + 1);
      if (tick2 > tick)
	tick = tick2 - 1;
    }
  return tick;
}

#define	CROSSING_TACT4		(1)
#define	CROSSING_TACT		(2)

static guint
coord_check_crossing (BstPatternView *self,
		      gint	      y,
		      guint	      crossing)
{
  guint ntick = coord_to_tick (self, y, FALSE);
  guint xtick = coord_to_tick (self, y, TRUE);
  guint nq = 0, xq = 0;
  
  /* catch _at_ tick boundary as well */
  xtick += 1;
  
  switch (crossing)
    {
    case CROSSING_TACT4:
      nq = ntick / (self->tpt * 4);
      xq = xtick / (self->tpt * 4);
      break;
    case CROSSING_TACT:
      nq = ntick / self->tpt;
      xq = xtick / self->tpt;
      break;
    }
  
  return nq != xq;
}
#endif

static void
pattern_view_allocate_2markers (BstPatternView    *self,
                                GxkScrollMarker *marker)
{
  GxkScrollCanvas *scc = GXK_SCROLL_CANVAS (self);
  gint pw = 10, cw = 10, ry, rh, row = tick_to_row (self, marker[0].coords.y);
  if (CANVAS (self))
    gdk_window_get_size (CANVAS (self), &cw, NULL);
  if (VPANEL (self))
    gdk_window_get_size (HPANEL (self), &pw, NULL);
  row_to_coords (self, row, &ry, &rh);
  gxk_scroll_canvas_setup_marker (scc, &marker[0], &scc->canvas,
                                  0, ry + rh / 2 - CMARK_HEIGHT (self) / 2,
                                  cw, CMARK_HEIGHT (self));
  gxk_scroll_canvas_setup_marker (scc, &marker[1], &scc->left_panel,
                                  0, ry + rh / 2 - PMARK_HEIGHT (self) / 2,
                                  pw, PMARK_HEIGHT (self));
}

static void
pattern_view_move_2markers (BstPatternView    *self,
                            GxkScrollMarker *marker)
{
  GxkScrollCanvas *scc = GXK_SCROLL_CANVAS (self);
  gint ry, rh, row = tick_to_row (self, marker[0].coords.y);
  row_to_coords (self, row, &ry, &rh);
  gxk_scroll_canvas_move_marker (scc, &marker[0], 0, ry + rh / 2 - CMARK_HEIGHT (self) / 2);
  gxk_scroll_canvas_move_marker (scc, &marker[1], 0, ry + rh / 2 - PMARK_HEIGHT (self) / 2);
}

static void
pattern_view_reallocate_contents (GxkScrollCanvas *scc,
                                  gint             xdiff,
                                  gint             ydiff)
{
  BstPatternView *self = BST_PATTERN_VIEW (scc);
  guint i;
  if (!xdiff && !ydiff)         /* real size-allocate */
    {
      guint accu = 0;
      for (i = 0; i < self->n_cols; i++)
        {
          self->cols[i]->x = scc->x_offset + accu;
          accu += self->cols[i]->width;
        }
    }
  if (xdiff)
    for (i = 0; i < self->n_cols; i++)
      self->cols[i]->x += xdiff;
  for (i = 0; i < scc->n_markers; i++)
    {
      GxkScrollMarker *marker1 = scc->markers + i;
      GxkScrollMarker *marker2 = i + 1 < scc->n_markers ? scc->markers + i + 1 : NULL;
      if (marker2 && marker1->index == marker2->index)
        {
          if (xdiff || ydiff)
            pattern_view_move_2markers (self, marker1);
          else
            pattern_view_allocate_2markers (self, marker1);
          i += 1;
        }
    }
}

static void
pattern_view_realize (GtkWidget *widget)
{
  BstPatternView *self = BST_PATTERN_VIEW (widget);
  GTK_WIDGET_CLASS (bst_pattern_view_parent_class)->realize (widget);
  bst_pattern_view_set_focus (self, 0, 0);
}

static void
draw_row_shading (BstPatternView *self,
                  GdkWindow      *drawable,
                  gint            row,
                  gint            drawable_width,
                  gint            y,
                  gint            height)
{
  if (self->srow1 && row % self->srow1 == 0)
    gdk_draw_rectangle (drawable, COLOR_GC_SHADE1 (self), TRUE,
                        0, y, drawable_width, height);
  else if (self->srow2 && row % self->srow2 == 0)
    gdk_draw_rectangle (drawable, COLOR_GC_SHADE2 (self), TRUE,
                        0, y, drawable_width, height);
}

static void
bst_pattern_view_draw_canvas (GxkScrollCanvas *scc,
                              GdkWindow       *drawable,
                              GdkRectangle    *area)
{
  BstPatternView *self = BST_PATTERN_VIEW (scc);
  gint row = coord_to_row (self, area->y, NULL);
  gint validrow, width, height;
  GdkRectangle rect;
  // gint line_width = 0; /* line widths != 0 interfere with dash-settings on some X servers */
  GXK_SCROLL_CANVAS_CLASS (bst_pattern_view_parent_class)->draw_canvas (scc, drawable, area);
  gdk_window_get_size (CANVAS (self), &width, &height);
  
  GdkGC *gcs[BST_PATTERN_COLUMN_GC_LAST] = { NULL, };
  gcs[BST_PATTERN_COLUMN_GC_TEXT0] = COLOR_GC_TEXT0 (self);
  gcs[BST_PATTERN_COLUMN_GC_TEXT1] = COLOR_GC_TEXT1 (self);
  gcs[BST_PATTERN_COLUMN_GC_VBAR] = COLOR_GC_VBAR (self);

  validrow = row_to_coords (self, row, &rect.y, &rect.height);
  while (validrow && rect.y < area->y + area->height)
    {
      gint tick, duration;
      guint i;
      draw_row_shading (self, drawable, row, width, rect.y, rect.height);
      row_to_ticks (self, row, &tick, &duration);
      for (i = 0; i < self->n_cols; i++)
        {
          rect.x = self->cols[i]->x;
          rect.width = self->cols[i]->width;
          self->cols[i]->klass->draw_cell (self->cols[i], self, drawable,
                                           gxk_scroll_canvas_peek_pango_layout (scc, COLUMN_PLAYOUT_INDEX (i)),
                                           tick, duration, &rect, area, gcs);
        }
      validrow = row_to_coords (self, ++row, &rect.y, &rect.height);
    }
}

#if 0   // FIXME
static void
bst_pattern_view_overlap_grow_hpanel_area (BstPatternView *self,
                                           GdkRectangle *area)
{
  gint i, x = area->x, xbound = x + area->width;
  
  /* grow hpanel exposes by surrounding tacts */
  i = coord_to_tick (self, x, FALSE);
  i /= self->tpt;
  if (i > 0)
    i -= 1;		/* fudge 1 tact to the left */
  i *= self->tpt;
  x = tick_to_coord (self, i);
  i = coord_to_tick (self, xbound + 1, TRUE);
  i /= self->tpt;
  i += 2;               /* fudge 1 tact to the right (+1 for round-off) */
  i *= self->tpt;
  xbound = tick_to_coord (self, i);
  
  area->x = x;
  area->width = xbound - area->x;
}
#endif

static void
bst_pattern_view_draw_hpanel (GxkScrollCanvas *scc,
                              GdkWindow       *drawable,
                              GdkRectangle    *area)
{
#if 0   // FIXME
  BstPatternView *self = BST_PATTERN_VIEW (scc);
  GdkGC *draw_gc = STYLE (self)->fg_gc[STATE (self)];
  PangoRectangle rect = { 0 };
  gchar buffer[64];
  gint i, width, height;
  gdk_window_get_size (drawable, &width, &height);
  bst_pattern_view_overlap_grow_hpanel_area (self, area);
  
  /* tact numbers */
  for (i = area->x; i < area->x + area->width; i++)
    {
      /* drawing tact numbers is not of much use if we can't even draw
       * the tact grid, so we special case draw_tact_grid here
       */
      if (coord_check_crossing (self, i, CROSSING_TACT4))
	{
	  guint next_pixel, tact4 = coord_to_tick (self, i, TRUE) + 1;
          
	  tact4 /= (self->tpt * 4);
	  next_pixel = tick_to_coord (self, (tact4 + 1) * (self->tpt * 4));
          
	  g_snprintf (buffer, 64, "%u", tact4 + 1);
          pango_layout_set_text (PLAYOUT_HPANEL (self), buffer, -1);
          pango_layout_get_pixel_extents (PLAYOUT_HPANEL (self), NULL, &rect);
          
	  /* draw this tact if there's enough space */
	  if (i + rect.width / 2 < (i + next_pixel) / 2)
            gdk_draw_layout (drawable, draw_gc,
                             i - rect.width / 2, (height - rect.height) / 2,
                             PLAYOUT_HPANEL (self));
	}
      else if (self->draw_tact_grid && coord_check_crossing (self, i, CROSSING_TACT))
	{
          guint next_pixel, tact = coord_to_tick (self, i, TRUE) + 1;
          
	  tact /= self->tpt;
	  next_pixel = tick_to_coord (self, (tact + 1) * self->tpt);
          tact = tact % 4 + 1;
          if (tact == 1444)
            continue;   /* would draw on top of tact4 number */
          
	  g_snprintf (buffer, 64, ":%u", tact % 4 + 1);
          pango_layout_set_text (PLAYOUT_HPANEL (self), buffer, -1);
          pango_layout_get_pixel_extents (PLAYOUT_HPANEL (self), NULL, &rect);
          
	  /* draw this tact if there's enough space */
	  if (i + rect.width < (i + next_pixel) / 2)		/* don't half width, leave some more space */
            gdk_draw_layout (drawable, draw_gc,
                             i - rect.width / 2, (height - rect.height) / 2,
                             PLAYOUT_HPANEL (self));
	}
    }
#endif
}

static void
bst_pattern_view_draw_vpanel (GxkScrollCanvas *scc,
                              GdkWindow       *drawable,
                              GdkRectangle    *area)
{
  BstPatternView *self = BST_PATTERN_VIEW (scc);
  GdkGC *draw_gc = STYLE (self)->fg_gc[STATE (self)];
  gint row = coord_to_row (self, area->y, NULL);
  gint validrow, width, height;
  gchar buffer[64];
  GdkRectangle rect;
  gdk_window_get_size (VPANEL (self), &width, &height);

  validrow = row_to_coords (self, row, &rect.y, &rect.height);
  while (validrow && rect.y < area->y + area->height)
    {
      PangoRectangle prect = { 0 };
      gint tick;
      row_to_ticks (self, row, &tick, NULL);
      g_snprintf (buffer, 64, "%05x", tick);
      pango_layout_set_text (PLAYOUT_VPANEL (self), buffer, -1);
      pango_layout_get_pixel_extents (PLAYOUT_VPANEL (self), NULL, &prect);
      gdk_draw_layout (drawable, draw_gc,
                       width - prect.width - XTHICKNESS (self),
                       rect.y + (rect.height - prect.height) / 2,
                       PLAYOUT_VPANEL (self));
      validrow = row_to_coords (self, ++row, &rect.y, &rect.height);
    }
}

static void
bst_pattern_view_draw_marker (GxkScrollCanvas *scc,
                              GdkWindow       *drawable,
                              GdkRectangle    *area,
                              GxkScrollMarker *marker)
{
  BstPatternView *self = BST_PATTERN_VIEW (scc);
  GdkGC *dark_gc = STYLE (self)->dark_gc[STATE (self)];
  GdkGC *slct_gc = STYLE (self)->bg_gc[GTK_STATE_SELECTED];
  GdkGC *draw_gc = GTK_WIDGET_HAS_FOCUS (self) ? slct_gc : dark_gc;
  switch (marker->mtype)
    {
    case BST_PATTERN_VIEW_MARKER_FOCUS:
      gdk_draw_rectangle (drawable, draw_gc, TRUE,
                          marker->extends.x, marker->extends.y,
                          marker->extends.width, marker->extends.height);
      break;
    }
#if 0   // FIXME
  BstPatternView *self = BST_PATTERN_VIEW (scc);
  BstPatternViewMarkerType mtype = marker->mtype;
  gint x = marker->extends.x, y = marker->extends.y, width = marker->extends.width, height = marker->extends.height;
  GdkGC *draw_gc;
  switch (mtype)
    {
    default:                            draw_gc = STYLE (self)->bg_gc[STATE (self)]; break;
    }
  gdk_draw_rectangle (drawable, draw_gc, TRUE,
                      x + XTHICKNESS (self), y + YTHICKNESS (self),
                      width - 2 * XTHICKNESS (self),
                      height - 2 * YTHICKNESS (self));
  if (width == PMARK_WIDTH (self))
    gtk_paint_shadow (STYLE (self), drawable, STATE (self), GTK_SHADOW_OUT, NULL, NULL, NULL,
                      x + XTHICKNESS (self), y + YTHICKNESS (self),
                      width - 2 * XTHICKNESS (self), height - 2 * YTHICKNESS (self));
  gtk_paint_shadow (STYLE (self), drawable, STATE (self),
                    width == PMARK_WIDTH (self) ? GTK_SHADOW_IN : GTK_SHADOW_OUT, NULL, NULL, NULL,
                    x, y, width, height);
#endif
}

static void
pattern_view_queue_expose (BstPatternView *self,
                           guint           tick_start,
                           guint           tick_end)
{
  GdkRectangle area;
  gint y1, y2, height;
  gint row1 = tick_to_row (self, tick_start);
  gint row2 = tick_to_row (self, tick_end);
  gint row_last = bst_pattern_view_get_last_row (self);
  row1 = MAX (row1, 0);
  row2 = MIN (row2, row_last);
  row_to_coords (self, row1, &y1, NULL);
  row_to_coords (self, row2, &y2, &height);
  area.y = y1;
  area.height = y2 + height - y1;
  area.x = 0;
  gdk_window_get_size (VPANEL (self), &area.width, NULL);
  gdk_window_invalidate_rect (VPANEL (self), &area, TRUE);
  gdk_window_get_size (CANVAS (self), &area.width, NULL);
  gdk_window_invalidate_rect (CANVAS (self), &area, TRUE);
}

static void
pattern_view_adjustment_changed (GxkScrollCanvas *scc,
                                 GtkAdjustment   *adj)
{
  BstPatternView *self = BST_PATTERN_VIEW (scc);
  if (adj == scc->vadjustment)
    {
      gint ry, rh, tick, duration, row = tick_to_row (self, self->max_ticks);
      row_to_pixels_unscrolled (self, row, &ry, &rh);
      double umin = ry + rh;                                            /* lower bound for adj->upper based on max_ticks */
      row = pixels_to_row_unscrolled (self, 1e+9);
      row_to_ticks (self, row, &tick, &duration);
      double umax = MIN (tick + duration, 1e+9);                        /* confine to possible tick range */
      row = tick_to_row (self, umax);
      row_to_pixels_unscrolled (self, row, &ry, &rh);
      umax = MIN (ry + rh, 1e+9);                                       /* upper bound for adj->upper based on pixels */
      umin = MIN (umin, umax * 1.5), umax = MAX (umin, umax);           /* properly confine boundaries */
      /* quantize to row height */
      gint n = CLAMP (adj->upper, umin, umax);
      n = (n + self->row_height - 1) / self->row_height;
      gdouble new_upper = MAX (n, 1) * self->row_height;
      new_upper = CLAMP (new_upper, umin, umax);
      /* guard against invalid changes */
      if (adj->lower != 0 || adj->upper != new_upper)
        {
          scc->vadjustment->lower = 0;
          scc->vadjustment->upper = new_upper;
          gtk_adjustment_changed (adj);
        }
      gtk_widget_queue_draw (GTK_WIDGET (self));
      bst_pattern_view_set_focus (self, self->focus_col, self->focus_row);
    }
}

static void
pattern_view_update_adjustments (GxkScrollCanvas *scc,
                                 gboolean         hadj,
                                 gboolean         vadj)
{
  BstPatternView *self = BST_PATTERN_VIEW (scc);
  
  if (hadj)
    {
      scc->hadjustment->upper = scc->layout.canvas_width;
      scc->hadjustment->step_increment = self->cols[0]->width;
    }
  if (vadj)
    {
      gint ry, rh, tick, duration, row = tick_to_row (self, self->max_ticks);
      row_to_pixels_unscrolled (self, row, &ry, &rh);
      double umin = ry + rh;                                            /* lower bound for adj->upper based on max_ticks */
      row = pixels_to_row_unscrolled (self, 1e+9);
      row_to_ticks (self, row, &tick, &duration);
      double umax = MIN (tick + duration, 1e+9);                        /* confine to possible tick range */
      row = tick_to_row (self, umax);
      row_to_pixels_unscrolled (self, row, &ry, &rh);
      umax = MIN (ry + rh, 1e+9);                                       /* upper bound for adj->upper based on pixels */
      umin = MIN (umin, umax * 1.5), umax = MAX (umin, umax);           /* properly confine boundaries */
      /* quantize to row height */
      gint n = CLAMP (scc->vadjustment->upper, umin, umax);
      n = (n + self->row_height - 1) / self->row_height;
      gdouble new_upper = MAX (n, 1) * self->row_height;
      new_upper = CLAMP (new_upper, umin, umax);
      scc->vadjustment->lower = 0;
      scc->vadjustment->upper = new_upper;
      scc->hadjustment->step_increment = 4 * rh;        // FIXME rows-per-tact
    }
  GXK_SCROLL_CANVAS_CLASS (bst_pattern_view_parent_class)->update_adjustments (scc, hadj, vadj);

  bst_pattern_view_set_focus (self, self->focus_col, self->focus_row);
}

void
bst_pattern_view_vsetup (BstPatternView           *self,
                         guint                     tpqn,
                         guint                     qnpt,
                         guint                     max_ticks,
                         guint                     vticks)
{
  self->tpqn = MAX (tpqn, 1);
  self->tpt = MAX (qnpt, 1) * self->tpqn;
  self->vticks = CLAMP (vticks, 1, self->tpt);
  self->max_ticks = MAX (max_ticks, self->tpt);

  gtk_widget_queue_draw (GTK_WIDGET (self));
  Y_OFFSET (self) = GXK_SCROLL_CANVAS (self)->vadjustment->value;
  gxk_scroll_canvas_update_adjustments (GXK_SCROLL_CANVAS (self), FALSE, TRUE);
}

static void
pattern_view_handle_drag (GxkScrollCanvas     *scc,
                          GxkScrollCanvasDrag *scc_drag,
                          GdkEvent            *event)
{
  BstPatternView *self = BST_PATTERN_VIEW (scc);
  BstPatternViewDrag drag_mem = { 0 }, *drag = &drag_mem;
  gint hvalid = 1, hdrag = scc_drag->canvas_drag || scc_drag->top_panel_drag;
  gint vvalid = 1, vdrag = scc_drag->canvas_drag || scc_drag->left_panel_drag;
  /* copy over drag setup */
  memcpy (drag, scc_drag, sizeof (*scc_drag));  /* sizeof (*scc_drag) < sizeof (*drag) */
  drag->pview = self;
  /* calculate widget specific drag data */
  if (hdrag)
    drag->current_col = coord_to_focus_col (self, drag->current_x, &hvalid);
  if (vdrag)
    {
      drag->current_row = coord_to_row (self, drag->current_y, &vvalid);
      row_to_ticks (self, drag->current_row, &drag->current_tick, &drag->current_duration);
    }
  drag->current_valid = hvalid && vvalid;
  /* sync start-position fields and select row */
  if (drag->type == GXK_DRAG_START)
    {
      drag->start_col = self->start_col = drag->current_col;
      drag->start_row = self->start_row = drag->current_row;
      drag->start_tick = self->start_tick = drag->current_tick;
      drag->start_duration = self->start_duration = drag->current_duration;
      drag->start_valid = self->start_valid = drag->current_valid;
    }
  else
    {
      drag->start_col = self->start_col;
      drag->start_row = self->start_row;
      drag->start_tick = self->start_tick;
      drag->start_duration = self->start_duration;
      drag->start_valid = self->start_valid;
    }
  bst_pattern_view_set_focus (self, drag->current_col, drag->current_row);
  /* handle drag */
  g_signal_emit (self, signal_drag, 0, drag);
  /* copy over drag reply */
  scc_drag->state = drag->state;
  /* resort to clicks for unhandled button presses */
  if (drag->type == GXK_DRAG_START && drag->state == GXK_DRAG_UNHANDLED &&
      event && event->type == GDK_BUTTON_PRESS)
    {
      drag->state = GXK_DRAG_HANDLED;
      g_signal_emit (self, signal_clicked, 0, drag->button, drag->start_row, drag->start_tick, event);
    }
}

void
bst_pattern_view_add_column (BstPatternView   *self,
                             BstPatternLType   ltype,
                             gint              num,
                             BstPatternLFlags  lflags)
{
  BstPatternColumn *col;
  g_return_if_fail (BST_PATTERN_VIEW (self));

  self->cols = g_renew (BstPatternColumn*, self->cols, self->n_cols + 1);
  col = bst_pattern_column_create (ltype, num, lflags);
  self->cols[self->n_cols++] = col;
  col->focus_base = self->n_focus_cols;
  for (int i = 0; i < col->n_focus_positions; i++)
    {
      self->focus_cols = g_renew (BstPatternColumn*, self->focus_cols, self->n_focus_cols + 1);
      self->focus_cols[self->n_focus_cols++] = col;
    }
  gtk_widget_queue_resize (GTK_WIDGET (self));
}

gint
bst_pattern_view_get_focus_width (BstPatternView           *self)
{
  g_return_val_if_fail (BST_PATTERN_VIEW (self), 0);
  return (XTHICKNESS (self) + YTHICKNESS (self)) / 2;
}

gint
bst_pattern_view_get_last_row (BstPatternView *self)
{
  g_return_val_if_fail (BST_PATTERN_VIEW (self), 0);
  return last_visible_row (self);
}

BstPatternColumn*
bst_pattern_view_get_focus_cell (BstPatternView *self, int *tick_p, int *duration_p)
{
  int focus_col = self->focus_col;
  int focus_row = self->focus_row;
  if (focus_col < self->n_focus_cols)
    {
      row_to_ticks (self, focus_row, tick_p, duration_p);
      return self->focus_cols[focus_col];
    }
  return NULL;
}

gboolean
bst_pattern_view_dispatch_key (BstPatternView            *self,
                               guint                      keyval,
                               GdkModifierType            modifier,
                               BstPatternFunction         action,
                               gdouble                    param,
                               BstPatternFunction        *movement)
{
  GdkRectangle rect;
  int focus_col = self->focus_col;
  int focus_row = self->focus_row;
  if (focus_col < self->n_focus_cols && row_to_coords (self, focus_row, &rect.y, &rect.height))
    {
      BstPatternColumn *col = self->focus_cols[focus_col];
      gint tick, duration;
      rect.x = col->x;
      rect.width = col->width;
      row_to_ticks (self, focus_row, &tick, &duration);
      if (col->klass->key_event &&
          col->klass->key_event (col, self, CANVAS (self),
                                 pattern_view_column_pango_layout (self, col),
                                 tick, duration, &rect, focus_col - col->focus_base,
                                 keyval, modifier, action, param, movement))
        return TRUE;
    }
  return FALSE;
}

void
bst_pattern_view_set_focus (BstPatternView *self, int focus_col, int focus_row)
{
  GdkRectangle rect;
  gint last_row;
  g_return_if_fail (BST_PATTERN_VIEW (self));

  focus_col = MIN (focus_col + 1, self->n_focus_cols) - 1;
  last_row = bst_pattern_view_get_last_row (self);
  focus_row = MIN (focus_row, last_row);
  if (focus_col >= 0 && focus_col < self->n_focus_cols && row_to_coords (self, focus_row, &rect.y, &rect.height))
    {
      GxkScrollCanvas *scc = GXK_SCROLL_CANVAS (self);
      BstPatternColumn *col = self->focus_cols[focus_col];
      gint fwidth = FOCUS_WIDTH (self);
      rect.x = col->x;
      rect.width = col->width;
      self->focus_row = focus_row;
      self->focus_col = focus_col;
      bst_pattern_view_set_pixmarker (self, 1, BST_PATTERN_VIEW_MARKER_FOCUS,
                                      rect.x, rect.y, rect.width, fwidth);
      bst_pattern_view_set_pixmarker (self, 2, BST_PATTERN_VIEW_MARKER_FOCUS,
                                      rect.x, rect.y + fwidth, fwidth, rect.height - 2 * fwidth);
      bst_pattern_view_set_pixmarker (self, 3, BST_PATTERN_VIEW_MARKER_FOCUS,
                                      rect.x + rect.width - fwidth, rect.y + fwidth, fwidth, rect.height - 2 * fwidth);
      bst_pattern_view_set_pixmarker (self, 4, BST_PATTERN_VIEW_MARKER_FOCUS,
                                      rect.x, rect.y + rect.height - fwidth, rect.width, fwidth);
      if (col->n_focus_positions > 1)
        {
          gint tick, duration, fx, fw;
          row_to_ticks (self, focus_row, &tick, &duration);
          col->klass->get_focus_pos (col, self, CANVAS (self),
                                     pattern_view_column_pango_layout (self, col),
                                     tick, duration, &rect, focus_col - col->focus_base, &fx, &fw);
          bst_pattern_view_set_pixmarker (self, 5, BST_PATTERN_VIEW_MARKER_FOCUS,
                                          rect.x + fx,
                                          rect.y + rect.height - fwidth - fwidth,
                                          fw,
                                          fwidth);
        }
      else
        bst_pattern_view_set_pixmarker (self, 5, BST_PATTERN_VIEW_MARKER_FOCUS, -1, -1, 1, 1);
      gxk_scroll_canvas_make_visible (scc,
                                      X_OFFSET (self) + rect.x,
                                      Y_OFFSET (self) + rect.y,
                                      rect.width, rect.height);
    }
}

void
bst_pattern_view_set_marker (BstPatternView          *self,
                             guint                    mark_index,
                             guint                    position,
                             BstPatternViewMarkerType mtype)
{
  GxkScrollCanvas *scc = GXK_SCROLL_CANVAS (self);
  GxkScrollMarker *marker;
  guint count;
  g_return_if_fail (mark_index > 0);
  mark_index += CUSTOM_MARKER_OFFSET;
  
  marker = gxk_scroll_canvas_lookup_marker (scc, mark_index, &count);
  if (!marker && !mtype)
    return;
  else if (!marker && mtype)
    {
      gxk_scroll_canvas_add_marker (scc, mark_index);
      gxk_scroll_canvas_add_marker (scc, mark_index);
      marker = gxk_scroll_canvas_lookup_marker (scc, mark_index, &count);
    }
  else if (marker && !mtype)
    {
      while (marker)
        {
          gxk_scroll_canvas_remove_marker (scc, marker);
          marker = gxk_scroll_canvas_lookup_marker (scc, mark_index, NULL);
        }
      return;
    }
  
  g_return_if_fail (count == 2);
  
  marker[0].coords.y = position;
  marker[1].coords.y = position;
  if (marker[0].mtype != mtype || !marker[0].pixmap)
    {
      marker[0].mtype = mtype;
      marker[1].mtype = mtype;
      pattern_view_allocate_2markers (self, marker);
    }
  else
    pattern_view_move_2markers (self, marker);
}

void
bst_pattern_view_set_pixmarker (BstPatternView           *self,
                                guint                     mark_index,
                                BstPatternViewMarkerType  mtype,
                                gint                      x,
                                gint                      y,
                                gint                      width,
                                gint                      height)
{
  GxkScrollCanvas *scc = GXK_SCROLL_CANVAS (self);
  GxkScrollMarker *marker;
  guint count;
  g_return_if_fail (mark_index > 0 && mark_index < CUSTOM_MARKER_OFFSET);
  
  marker = gxk_scroll_canvas_lookup_marker (scc, mark_index, &count);
  if (!marker && !mtype)
    return;
  else if (!marker && mtype)
    {
      gxk_scroll_canvas_add_marker (scc, mark_index);
      marker = gxk_scroll_canvas_lookup_marker (scc, mark_index, &count);
    }
  else if (marker && !mtype)
    {
      while (marker)
        {
          gxk_scroll_canvas_remove_marker (scc, marker);
          marker = gxk_scroll_canvas_lookup_marker (scc, mark_index, NULL);
        }
      return;
    }
  
  g_return_if_fail (count == 1);

  if (marker->mtype == mtype && marker->windowp == &CANVAS (self) &&
      marker->extends.width == width && marker->extends.height == height)
    gxk_scroll_canvas_move_marker (scc, marker, x, y);
  else
    {
      marker->mtype = mtype;
      gxk_scroll_canvas_setup_marker (scc, marker, &CANVAS (self), x, y, width, height);
    }
}

const gchar*
bst_pattern_view_get_layout (BstPatternView *self)
{
  return self->layout_string;
}

guint
bst_pattern_view_set_layout (BstPatternView *self,
                             const gchar    *layout)
{
  const gchar *p2, *p = layout;
  struct AuxColumn {
    BstPatternLType  ltype;
    BstPatternLFlags flags;
    int              num;
  };
  uint n = 0;
  while (*p == ' ')
    p++;
  AuxColumn *column = (AuxColumn*) g_malloc (sizeof (column[0]) * (n + 1));
  p2 = bst_pattern_layout_parse_column (p, &column[n].ltype, &column[n].num, &column[n].flags);
  while (p2 > p)
    {
      n++;
      p = p2;
      while (*p == ' ')
        p++;
      column = (AuxColumn*) g_realloc (column, sizeof (column[0]) * (n + 1));
      p2 = bst_pattern_layout_parse_column (p, &column[n].ltype, &column[n].num, &column[n].flags);
    }
  if (!p2[0] && n)
    {
      guint i;
      bst_pattern_view_destroy_columns (self);
      self->layout_string = g_strndup (layout, p2 - layout);
      for (i = 0; i < n; i++)
        bst_pattern_view_add_column (self, column[i].ltype, column[i].num, column[i].flags);
      self->focus_col = self->n_focus_cols ? self->focus_col % self->n_focus_cols : 0;
      bst_pattern_view_set_focus (self, self->focus_col, self->focus_row);
    }
  g_free (column);
  return p2 - layout;
}

void
bst_pattern_view_set_shading (BstPatternView            *self,
                              guint                      row1,
                              guint                      row2,
                              guint                      row3,
                              guint                      row4)
{
  self->srow1 = row1;
  self->srow2 = row2;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
bst_pattern_view_class_init (BstPatternViewClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GxkScrollCanvasClass *scroll_canvas_class = GXK_SCROLL_CANVAS_CLASS (klass);
  
  gobject_class->finalize = bst_pattern_view_finalize;
  gobject_class->dispose = bst_pattern_view_dispose;

  widget_class->realize = pattern_view_realize;

  scroll_canvas_class->hscrollable = TRUE;
  scroll_canvas_class->vscrollable = TRUE;
  scroll_canvas_class->get_layout = pattern_view_get_layout;
  scroll_canvas_class->update_adjustments = pattern_view_update_adjustments;
  scroll_canvas_class->adjustment_changed = pattern_view_adjustment_changed;
  scroll_canvas_class->reallocate_contents = pattern_view_reallocate_contents;
  scroll_canvas_class->draw_canvas = bst_pattern_view_draw_canvas;
  scroll_canvas_class->draw_top_panel = bst_pattern_view_draw_hpanel;
  scroll_canvas_class->draw_left_panel = bst_pattern_view_draw_vpanel;
  scroll_canvas_class->draw_marker = bst_pattern_view_draw_marker;
  scroll_canvas_class->handle_drag = pattern_view_handle_drag;
  scroll_canvas_class->image_tint = gdk_color_from_rgb (0x00ffffff);
  scroll_canvas_class->image_saturation = 0;

  klass->drag = NULL;
  klass->clicked = NULL;
  
  signal_drag = g_signal_new ("drag", G_OBJECT_CLASS_TYPE (klass),
			      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BstPatternViewClass, drag),
			      NULL, NULL,
			      bst_marshal_NONE__POINTER,
			      G_TYPE_NONE, 1, G_TYPE_POINTER);
  signal_clicked = g_signal_new ("clicked", G_OBJECT_CLASS_TYPE (klass),
				 G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BstPatternViewClass, clicked),
				 NULL, NULL,
				 bst_marshal_NONE__UINT_UINT_INT_BOXED,
				 G_TYPE_NONE, 4, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_INT,
				 GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  bst_skin_config_add_notify ((BstSkinConfigNotify) pattern_view_class_setup_skin, klass);
  pattern_view_class_setup_skin (klass);
}
