/* BEAST - Better Audio System
 * Copyright (C) 2003 Tim Janik
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
#include "bsttrackroll.h"
#include "bstsnifferscope.h"
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
#define PLAYOUT_HPANEL(self)    (gxk_scroll_canvas_get_pango_layout (GXK_SCROLL_CANVAS (self), 0))
#define PLAYOUT_CANVAS(self)    (gxk_scroll_canvas_get_pango_layout (GXK_SCROLL_CANVAS (self), 1))
#define COLOR_GC(self, i)       (GXK_SCROLL_CANVAS (self)->color_gc[i])
#define COLOR_GC_POS(self)      (COLOR_GC (self, CINDEX_POS))
#define COLOR_GC_LOOP(self)     (COLOR_GC (self, CINDEX_LOOP))
#define COLOR_GC_SELECT(self)   (COLOR_GC (self, CINDEX_SELECT))
#define CANVAS(self)            (GXK_SCROLL_CANVAS (self)->canvas)
#define HPANEL(self)            (GXK_SCROLL_CANVAS (self)->top_panel)
#define VPANEL(self)            (GXK_SCROLL_CANVAS (self)->left_panel)
/* layout (allocation) */
#define	CMARK_WIDTH(self)       (XTHICKNESS (self) * 3 + 1)
#define	PMARK_WIDTH(self)       (CMARK_WIDTH (self) + 3 * XTHICKNESS (self))
/* appearance */
#define TACT_HPIXELS		(50)	/* guideline */


/* --- prototypes --- */
static void	bst_track_roll_hsetup			(BstTrackRoll		*self,
							 guint			 tpt,
							 guint			 max_ticks,
							 gdouble		 hzoom);
static void	bst_track_roll_allocate_ecell		(BstTrackRoll		*self);
static void     bst_track_roll_allocate_scope           (BstTrackRoll           *self,
                                                         GtkWidget              *child,
                                                         guint                   row);


/* --- static variables --- */
static guint	signal_select_row = 0;
static guint	signal_drag = 0;
static guint	signal_clicked = 0;
static guint	signal_stop_edit = 0;


/* --- functions --- */
G_DEFINE_TYPE (BstTrackRoll, bst_track_roll, GXK_TYPE_SCROLL_CANVAS);

enum {
  CINDEX_POS,
  CINDEX_LOOP,
  CINDEX_SELECT,
  CINDEX_COUNT
};

static void
bst_track_roll_init (BstTrackRoll *self)
{
  GxkScrollCanvas *scc = GXK_SCROLL_CANVAS (self);
  
  self->tpt = 384 * 4;
  self->max_ticks = 1;
  self->hzoom = 1;
  self->draw_tact_grid = TRUE;
  self->prelight_row = 0;
  self->hpanel_height = 20;
  gxk_scroll_canvas_set_canvas_cursor (scc, GDK_LEFT_PTR);
  gxk_scroll_canvas_set_left_panel_cursor (scc, GDK_HAND2);
  gxk_scroll_canvas_set_top_panel_cursor (scc, GDK_LEFT_PTR);
  self->area_offset = 20;
  bst_track_roll_hsetup (self, 384 * 4, 800 * 384, 100);
}

static void
bst_track_roll_destroy (GtkObject *object)
{
  BstTrackRoll *self = BST_TRACK_ROLL (object);
  guint i;

  if (self->scope_update)
    {
      g_source_remove (self->scope_update);
      self->scope_update = 0;
    }
  for (i = 0; i < self->n_scopes; i++)
    gtk_widget_unparent (self->scopes[i]);
  g_free (self->scopes);
  self->scopes = NULL;
  self->n_scopes = 0;
  
  GTK_OBJECT_CLASS (bst_track_roll_parent_class)->destroy (object);
}

static void
bst_track_roll_finalize (GObject *object)
{
  BstTrackRoll *self = BST_TRACK_ROLL (object);
  guint i;
  
  bst_track_roll_setup (self, NULL, 0);

  if (self->scope_update)
    {
      g_source_remove (self->scope_update);
      self->scope_update = 0;
    }

  for (i = 0; i < self->n_scopes; i++)
    gtk_widget_unparent (self->scopes[i]);
  g_free (self->scopes);
  
  G_OBJECT_CLASS (bst_track_roll_parent_class)->finalize (object);
}

static void
track_roll_forall (GtkContainer    *container,
                   gboolean         include_internals,
                   GtkCallback      callback,
                   gpointer         callback_data)
{
  BstTrackRoll *self = BST_TRACK_ROLL (container);
  guint i;
  for (i = 0; i < self->n_scopes; i++)
    if (include_internals)
      callback (self->scopes[i], callback_data);
  if (self->ecell && include_internals)
    callback ((GtkWidget*) self->ecell, callback_data);
}

static void
track_roll_update_layout (BstTrackRoll *self,
			  gboolean      queue_resize)
{
  gint old_area_offset, dummy;

  old_area_offset = self->area_offset;

  if (self->tree)
    gxk_tree_view_get_bin_window_pos (self->tree, &dummy, &self->area_offset);
  else
    self->area_offset = 20;

  if (old_area_offset != self->area_offset && queue_resize)
    gtk_widget_queue_resize (GTK_WIDGET (self));
}

static void
track_roll_get_layout (GxkScrollCanvas        *scc,
                       GxkScrollCanvasLayout  *layout)
{
  BstTrackRoll *self = BST_TRACK_ROLL (scc);
  track_roll_update_layout (self, FALSE);
  layout->top_panel_height = MAX (1, self->area_offset - 2 * YTHICKNESS (self));
  layout->left_panel_width = 40 + 4 * XTHICKNESS (self);
  layout->right_panel_width = 0;
  layout->bottom_panel_height = 0;
  layout->canvas_width = 1 + 2 * XTHICKNESS (self) + 1;
  layout->canvas_height = 1;
}

static void
track_roll_release_proxy (BstTrackRoll *self)
{
  bse_proxy_disconnect (self->proxy,
                        "any_signal", track_roll_release_proxy, self,
                        NULL);
  bse_item_unuse (self->proxy);
  self->proxy = 0;
}

void
bst_track_roll_setup (BstTrackRoll   *self,
                      GtkTreeView    *tree,
                      SfiProxy        song)
{
  g_return_if_fail (BST_IS_TRACK_ROLL (self));
  if (tree)
    g_return_if_fail (GTK_IS_TREE_VIEW (tree));
  if (song)
    g_return_if_fail (BSE_IS_SONG (song));

  if (self->tree)
    {
      g_object_disconnect_any (self->tree, gxk_scroll_canvas_reallocate, self);
      g_object_unref (self->tree);
    }
  self->tree = tree;
  if (self->tree)
    {
      g_object_ref (tree);
      g_object_connect (self->tree,
                        "swapped_object_signal_after::size_allocate", gxk_scroll_canvas_reallocate, self,
                        NULL);
    }

  if (self->proxy)
    track_roll_release_proxy (self);
  self->proxy = song;
  if (self->proxy)
    {
      bse_item_use (self->proxy);
      bse_proxy_connect (self->proxy,
                         "swapped_signal::release", track_roll_release_proxy, self,
                         NULL);
    }
  track_roll_update_layout (self, TRUE);
  bst_track_roll_queue_row_change (self, 0);
  gtk_widget_queue_resize (GTK_WIDGET (self));
}

void
bst_track_roll_reselect (BstTrackRoll *self)
{
  g_return_if_fail (BST_IS_TRACK_ROLL (self));

  if (self->tree)
    {
      bst_track_roll_set_prelight_row (self, gxk_tree_view_get_selected_row (self->tree));
      if (GTK_WIDGET_DRAWABLE (self))
        gxk_window_process_next (GTK_WIDGET (self)->window, TRUE);
    }
  else
    bst_track_roll_set_prelight_row (self, 0xffffffff);
}

static gint
ticks_to_pixels (BstTrackRoll *self,
		 gint	       ticks)
{
  gdouble tpt = self->tpt;
  gdouble tpixels = TACT_HPIXELS;

  /* compute pixel span of a tick range */

  tpixels *= self->hzoom / tpt * (gdouble) ticks;
  if (ticks)
    tpixels = MAX (tpixels, 1);
  return MIN (G_MAXINT, tpixels);
}

static gint
pixels_to_ticks_unscrolled (BstTrackRoll *self,
                            gint	       pixels)
{
  gdouble tpt = self->tpt;
  gdouble ticks = 1.0 / (gdouble) TACT_HPIXELS;

  /* compute tick span of a pixel range */

  ticks = ticks * tpt / self->hzoom * (gdouble) pixels;
  if (pixels > 0)
    ticks = MAX (ticks, 1);
  else
    ticks = 0;
  return MIN (G_MAXINT, ticks);
}

static gint
tick_to_coord (BstTrackRoll *self,
	       gint	     tick)
{
  return ticks_to_pixels (self, tick) - X_OFFSET (self);
}

static gint
coord_to_tick (BstTrackRoll *self,
	       gint	     x,
	       gboolean	     right_bound)
{
  guint tick;

  x += X_OFFSET (self);
  tick = pixels_to_ticks_unscrolled (self, x);
  if (right_bound)
    {
      guint tick2 = pixels_to_ticks_unscrolled (self, x + 1);

      if (tick2 > tick)
	tick = tick2 - 1;
    }
  return tick;
}

#define	CROSSING_TACT4		(1)
#define	CROSSING_TACT		(2)

static guint
coord_check_crossing (BstTrackRoll *self,
		      gint	    x,
		      guint	    crossing)
{
  guint ltick = coord_to_tick (self, x, FALSE);
  guint rtick = coord_to_tick (self, x, TRUE);
  guint lq = 0, rq = 0;

  /* catch _at_ tick boundary as well */
  rtick += 1;

  switch (crossing)
    {
    case CROSSING_TACT4:
      lq = ltick / (self->tpt * 4);
      rq = rtick / (self->tpt * 4);
      break;
    case CROSSING_TACT:
      lq = ltick / self->tpt;
      rq = rtick / self->tpt;
      break;
    }

  return lq != rq;
}

static gint
coord_to_row (BstTrackRoll *self,
	      gint          y,
	      gboolean     *is_valid)
{
  gint row;
  if (self->tree && is_valid)
    *is_valid = gxk_tree_view_get_row_from_coord (self->tree, y, &row);
  else if (self->tree)
    gxk_tree_view_get_row_from_coord (self->tree, y, &row);
  else
    row = y / 15; /* uneducated guess */
  return row;
}

static gboolean
row_to_coords (BstTrackRoll *self,
	       gint          row,
	       gint         *y_p,
	       gint         *height_p)
{
  if (self->tree)
    return gxk_tree_view_get_row_area (self->tree, row, y_p, height_p, FALSE);
  else
    {
      if (y_p)
	*y_p = row * 15;
      if (height_p)
	*height_p = 15;
      return TRUE;
    }
}

static SfiProxy
row_to_track (BstTrackRoll *self,
	      gint          row)
{
  if (self->get_track)
    return self->get_track (self->proxy_data, row);
  else
    return 0;
}

static void
track_roll_allocate_2markers (BstTrackRoll    *self,
                              GxkScrollMarker *marker)
{
  GxkScrollCanvas *scc = GXK_SCROLL_CANVAS (self);
  gint ph = 10, ch = 10, x = tick_to_coord (self, marker[0].coords.x);
  if (CANVAS (self))
    gdk_window_get_size (CANVAS (self), NULL, &ch);
  if (HPANEL (self))
    gdk_window_get_size (HPANEL (self), NULL, &ph);
  gxk_scroll_canvas_setup_marker (scc, &marker[0], &scc->canvas,
                                  x - CMARK_WIDTH (self) / 2, 0,
                                  CMARK_WIDTH (self), ch);
  gxk_scroll_canvas_setup_marker (scc, &marker[1], &scc->top_panel,
                                  x - PMARK_WIDTH (self) / 2, 0,
                                  PMARK_WIDTH (self), ph);
}

static void
track_roll_move_2markers (BstTrackRoll    *self,
                          GxkScrollMarker *marker)
{
  GxkScrollCanvas *scc = GXK_SCROLL_CANVAS (self);
  gint x = tick_to_coord (self, marker[0].coords.x);
  gxk_scroll_canvas_move_marker (scc, &marker[0], x - CMARK_WIDTH (self) / 2, 0);
  gxk_scroll_canvas_move_marker (scc, &marker[1], x - PMARK_WIDTH (self) / 2, 0);
}

static void
track_roll_reallocate_contents (GxkScrollCanvas *scc,
                                gint             xdiff,
                                gint             ydiff)
{
  BstTrackRoll *self = BST_TRACK_ROLL (scc);
  guint i;
  if (xdiff || ydiff)
    {
      /* do delta allocations */
      GtkAllocation allocation;
      GtkWidget *widget;
      if (self->ecell)
        {
          widget = GTK_WIDGET (self->ecell);
          allocation = widget->allocation;
          allocation.x += xdiff;
          allocation.y += ydiff;
          gtk_widget_size_allocate (widget, &allocation);
        }
      for (i = 0; i < self->n_scopes; i++)
        {
          widget = self->scopes[i];
          allocation = widget->allocation;
          allocation.y += ydiff;
          gtk_widget_size_allocate (widget, &allocation);
        }
    }

  for (i = 0; i < scc->n_markers; i += 2)
    if (xdiff || ydiff)
      track_roll_move_2markers (self, scc->markers + i);
    else
      track_roll_allocate_2markers (self, scc->markers + i);

  if (!xdiff && !ydiff)         /* real size-allocate */
    {
      guint i;
      for (i = 0; i < self->n_scopes; i++)
        bst_track_roll_allocate_scope (self, self->scopes[i], i);
      bst_track_roll_allocate_ecell (self);
      bst_track_roll_check_update_scopes (self);
      bst_track_roll_reselect (self);
    }
}

static void
scope_set_track (GtkWidget *scope,
                 SfiProxy   track)
{
  g_object_set_long (scope, "BstTrackRoll-Track", track);
  if (BST_SNIFFER_SCOPE (scope))
    bst_sniffer_scope_set_sniffer (BST_SNIFFER_SCOPE (scope),
                                   bse_track_get_output_source (track));
}

static SfiProxy
scope_get_track (GtkWidget *scope)
{
  return g_object_get_long (scope, "BstTrackRoll-Track");
}

static gboolean
track_roll_idle_update_scopes (gpointer data)
{
  BstTrackRoll *self = BST_TRACK_ROLL (data);
  GSList *scope_list = NULL;
  guint i;

  GDK_THREADS_ENTER ();
  self->scope_update = 0;
  
  /* save existing scopes */
  for (i = 0; i < self->n_scopes; i++)
    scope_list = g_slist_prepend (scope_list, self->scopes[i]);

  /* reset scope list */
  self->n_scopes = 0;

  /* match or create needed scopes */
  if (self->get_track && GTK_WIDGET_REALIZED (self))
    for (i = 0; ; i++)
      {
        SfiProxy track = self->get_track (self->proxy_data, i);
        GtkWidget *scope = NULL;
        GSList *slist;
        if (!track)
          break;
        for (slist = scope_list; slist; slist = slist->next)
          {
            scope = slist->data;
            if (scope_get_track (scope) == track)  /* match existing */
              {
                scope_list = g_slist_remove (scope_list, scope);
                break;
              }
          }
        if (!slist)     /* create new if not matched */
          {
            scope = g_object_new (BST_TYPE_SNIFFER_SCOPE, NULL);
            gtk_widget_show (scope);
            scope_set_track (scope, track);
            gtk_widget_set_parent_window (scope, VPANEL (self));
            gtk_widget_set_parent (scope, GTK_WIDGET (self));
          }
        /* add to scope list */
        self->n_scopes++;
        self->scopes = g_renew (GtkWidget*, self->scopes, self->n_scopes);
        self->scopes[i] = scope;
      }

  /* get rid of unneeded scopes */
  while (scope_list)
    {
      GtkWidget *child = g_slist_pop_head (&scope_list);
      gtk_widget_unparent (child);
    }
  
  /* allocate scopes 'n stuff */
  gxk_scroll_canvas_reallocate (GXK_SCROLL_CANVAS (self));
  /* work around spurious redraw problems */
  gtk_widget_queue_draw (GTK_WIDGET (self));

  GDK_THREADS_LEAVE ();
  return FALSE;
}

static void
queue_scope_update (BstTrackRoll *self)
{
  if (!self->scope_update)
    self->scope_update = g_idle_add_full (GTK_PRIORITY_RESIZE - 1, track_roll_idle_update_scopes, self, NULL);
}

void
bst_track_roll_check_update_scopes (BstTrackRoll *self)
{
  guint i;
  g_return_if_fail (BST_IS_TRACK_ROLL (self));

  /* check whether scope update is necessary and schedule one */
  if (!GTK_WIDGET_REALIZED (self) || !self->get_track)
    {
      if (self->n_scopes)
        queue_scope_update (self);
      return;
    }
  for (i = 0; i < self->n_scopes; i++)
    {
      SfiProxy track = self->get_track (self->proxy_data, i);
      if (scope_get_track (self->scopes[i]) != track)
        {
          queue_scope_update (self);
          return;
        }
    }
  if (self->get_track (self->proxy_data, i)) /* one off, shouldn't exist */
    {
      queue_scope_update (self);
      return;
    }
}

static void
bst_track_roll_realize (GtkWidget *widget)
{
  BstTrackRoll *self = BST_TRACK_ROLL (widget);
  guint i;

  GTK_WIDGET_CLASS (bst_track_roll_parent_class)->realize (widget);

  /* update children */
  for (i = 0; i < self->n_scopes; i++)
    gtk_widget_set_parent_window (self->scopes[i], VPANEL (self));
  if (self->ecell)
    gtk_widget_set_parent_window (GTK_WIDGET (self->ecell), CANVAS (self));
}

static void
bst_track_roll_unrealize (GtkWidget *widget)
{
  BstTrackRoll *self = BST_TRACK_ROLL (widget);

  bst_track_roll_abort_edit (self);

  GTK_WIDGET_CLASS (bst_track_roll_parent_class)->unrealize (widget);
}

static void
bst_track_roll_draw_canvas (GxkScrollCanvas *scc,
                            GdkWindow       *drawable,
                            GdkRectangle    *area)
{
  BstTrackRoll *self = BST_TRACK_ROLL (scc);
  GtkWidget *widget = GTK_WIDGET (self);
  GdkGC *fg_gc = widget->style->fg_gc[widget->state];
  GdkGC *bg2_gc = widget->style->bg_gc[widget->state];
  GdkGC *bg1_gc = STYLE (self)->bg_gc[GTK_STATE_ACTIVE];
  GdkGC *bgp_gc = widget->style->bg_gc[GTK_STATE_PRELIGHT];
  GdkGC *dark_gc = widget->style->dark_gc[widget->state];
  GdkGC *light_gc = widget->style->light_gc[widget->state];
  gint row = coord_to_row (self, area->y, NULL);
  gint ry, rheight, validrow, width, height;
  // gint line_width = 0; /* line widths != 0 interfere with dash-settings on some X servers */
  GXK_SCROLL_CANVAS_CLASS (bst_track_roll_parent_class)->draw_canvas (scc, drawable, area);
  gdk_window_get_size (CANVAS (self), &width, &height);

  validrow = row_to_coords (self, row, &ry, &rheight);
  while (validrow && ry < area->y + area->height)
    {
      SfiProxy track = row_to_track (self, row);
      if (row == self->ecell_row)
	bst_track_roll_allocate_ecell (self);
      gdk_draw_rectangle (drawable,
			  row == self->prelight_row ? bgp_gc : bg1_gc, // row & 1 ? bgd_gc : bg_gc,
			  TRUE,
			  0, ry, width, rheight);
      if (track)
	{
	  BseTrackPartSeq *tps = bse_track_list_parts (track);
	  gint i;
	  for (i = 0; i < tps->n_tparts; i++)
	    {
              static const int ENTRY_INNER_BORDER = 2;      /* sigh, no way around this */
              PangoRectangle rect = { 0 };
	      guint tick = tps->tparts[i]->tick;
	      SfiProxy part = tps->tparts[i]->part;
	      guint duration = tps->tparts[i]->duration;
	      const gchar *name = bse_item_get_name (part);
	      GdkRectangle area, carea;
	      carea.x = tick_to_coord (self, tick);
              carea.width = ticks_to_pixels (self, duration);
	      carea.y = ry;
	      carea.height = rheight;
	      gtk_paint_shadow (widget->style, drawable,
				widget->state, // row == self->prelight_row ? GTK_STATE_PRELIGHT : widget->state,
				GTK_SHADOW_OUT, NULL, NULL, NULL,
				carea.x, carea.y, carea.width, carea.height);
	      carea.x += XTHICKNESS (self);
	      carea.width = MAX (0, carea.width - 2 * XTHICKNESS (self));
              carea.y += YTHICKNESS (self);
              carea.height = MAX (0, carea.height - 2 * YTHICKNESS (self));
              area = carea;
              pango_layout_set_text (PLAYOUT_CANVAS (self), name, -1);
              pango_layout_get_pixel_extents (PLAYOUT_CANVAS (self), NULL, &rect);
	      gdk_draw_rectangle (drawable, bg2_gc, // row == self->prelight_row ? bgp_gc : bg2_gc,
				  TRUE, area.x, area.y, area.width, area.height);
              area.y += MAX (0, area.height - rect.height) / 2;
              area.x += ENTRY_INNER_BORDER;
              area.width = MAX (0, area.width - ENTRY_INNER_BORDER - 1);        /* 1 pixel safety margin from right border */
	      gdk_gc_set_clip_rectangle (fg_gc, &area);
              gdk_draw_layout (drawable, fg_gc,
                               area.x, area.y,
                               PLAYOUT_CANVAS (self));
	      gdk_gc_set_clip_rectangle (fg_gc, NULL);
	    }
	}
      gdk_draw_line (drawable, light_gc, 0, ry, width, ry);
      gdk_draw_line (drawable, dark_gc, 0, ry + rheight - 1, width, ry + rheight - 1);
      validrow = row_to_coords (self, ++row, &ry, &rheight);
    }
}

static void
bst_track_roll_overlap_grow_hpanel_area (BstTrackRoll *self,
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

static void
bst_track_roll_draw_hpanel (GxkScrollCanvas *scc,
                            GdkWindow       *drawable,
                            GdkRectangle    *area)
{
  BstTrackRoll *self = BST_TRACK_ROLL (scc);
  GdkGC *draw_gc = STYLE (self)->fg_gc[STATE (self)];
  PangoRectangle rect = { 0 };
  gchar buffer[64];
  gint i, width, height;
  gdk_window_get_size (drawable, &width, &height);
  bst_track_roll_overlap_grow_hpanel_area (self, area);
  
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

	  g_snprintf (buffer, 64, "%u", tact4 * 4 + 1);
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

	  g_snprintf (buffer, 64, "%u", tact + 1);
          pango_layout_set_text (PLAYOUT_HPANEL (self), buffer, -1);
          pango_layout_get_pixel_extents (PLAYOUT_HPANEL (self), NULL, &rect);
          
	  /* draw this tact if there's enough space */
	  if (i + rect.width < (i + next_pixel) / 2)		/* don't half width, leave some more space */
            gdk_draw_layout (drawable, draw_gc,
                             i - rect.width / 2, (height - rect.height) / 2,
                             PLAYOUT_HPANEL (self));
	}
    }
}

static void
bst_track_roll_allocate_scope (BstTrackRoll *self,
                               GtkWidget    *child,
                               guint         row)
{
  GtkAllocation allocation;
  gint ry, rheight, validrow;
  gtk_widget_size_request (child, NULL);
  validrow = GTK_WIDGET_REALIZED (self) ? row_to_coords (self, row, &ry, &rheight) : FALSE;
  if (!validrow)
    {
      allocation.x = allocation.y = -10;
      allocation.width = allocation.height = 1;
    }
  else
    {
      gint width, height;
      gdk_window_get_size (VPANEL (self), &width, &height);
      allocation.x = 2 * XTHICKNESS (self);
      allocation.y = ry + YTHICKNESS (self) + 1;
      allocation.width = width - 4 * XTHICKNESS (self);
      allocation.height = rheight - 2 * YTHICKNESS (self) - 2;
    }
  gtk_widget_size_allocate (child, &allocation);
}

static void
bst_track_roll_draw_vpanel (GxkScrollCanvas *scc,
                            GdkWindow       *drawable,
                            GdkRectangle    *area)
{
  BstTrackRoll *self = BST_TRACK_ROLL (scc);
  GtkWidget *widget = GTK_WIDGET (self);
  gint row = coord_to_row (self, area->y, NULL);
  gint ry, rheight, validrow, width, height;
  gdk_window_get_size (VPANEL (self), &width, &height);
  validrow = row_to_coords (self, row, &ry, &rheight);

  while (validrow && ry < area->y + area->height)
    {
      gdk_draw_rectangle (drawable,
                          STYLE (self)->base_gc[row == self->prelight_row ? ACTIVE_STATE (self) : STATE (self)],
			  TRUE,
			  2 * XTHICKNESS (self),
			  ry + YTHICKNESS (self) + 1,
			  width - 4 * XTHICKNESS (self),
			  rheight - 2 * YTHICKNESS (self) - 2);
      gtk_paint_shadow (widget->style, drawable,
			widget->state, GTK_SHADOW_IN,
			NULL, NULL, NULL,
			XTHICKNESS (self),
			ry + 1,
			width - 2 * XTHICKNESS (self),
			rheight - 2);
      validrow = row_to_coords (self, ++row, &ry, &rheight);
    }
}

static void
bst_track_roll_draw_marker (GxkScrollCanvas *scc,
                            GdkWindow       *drawable,
                            GdkRectangle    *area,
                            GxkScrollMarker *marker)
{
  BstTrackRoll *self = BST_TRACK_ROLL (scc);
  BstTrackRollMarkerType mtype = marker->mtype;
  gint x = marker->extends.x, y = marker->extends.y, width = marker->extends.width, height = marker->extends.height;
  GdkGC *draw_gc;
  switch (mtype)
    {
    case BST_TRACK_ROLL_MARKER_POS:     draw_gc = COLOR_GC_POS (self); break;
    case BST_TRACK_ROLL_MARKER_LOOP:    draw_gc = COLOR_GC_LOOP (self); break;
    case BST_TRACK_ROLL_MARKER_SELECT:  draw_gc = COLOR_GC_SELECT (self); break;
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
}

static void
track_roll_adjustment_changed (GxkScrollCanvas *scc,
                               GtkAdjustment   *adj)
{
  BstTrackRoll *self = BST_TRACK_ROLL (scc);
  if (adj == scc->hadjustment)
    {
      double umin = ticks_to_pixels (self, self->max_ticks);                    /* lower bound for adj->upper based on max_ticks */
      double umax = pixels_to_ticks_unscrolled (self, 1e+9);
      umax = ticks_to_pixels (self, MIN (umax, 1e+9));                          /* confine to possible tick range */
      umax = MIN (umax, 1e+9);                                                  /* upper bound for adj->upper based on pixels */
      umin = MIN (umin, umax * 1.5), umax = MAX (umin, umax);                   /* properly confine boundaries */
      /* guard against invalid changes */
      if (adj->lower != 0 || adj->upper != CLAMP (adj->upper, umin, umax))
        {
          scc->hadjustment->lower = 0;
          scc->hadjustment->upper = CLAMP (scc->hadjustment->upper, umin, umax);
          gtk_adjustment_changed (adj);
        }
    }
}

static void
track_roll_update_adjustments (GxkScrollCanvas *scc,
                               gboolean         hadj,
                               gboolean         vadj)
{
  BstTrackRoll *self = BST_TRACK_ROLL (scc);

  if (hadj)
    {
      double umin = ticks_to_pixels (self, self->max_ticks);                    /* lower bound for adj->upper based on max_ticks */
      double umax = pixels_to_ticks_unscrolled (self, 1e+9);
      umax = ticks_to_pixels (self, MIN (umax, 1e+9));                          /* confine to possible tick range */
      umax = MIN (umax, 1e+9);                                                  /* upper bound for adj->upper based on pixels */
      umin = MIN (umin, umax * 1.5), umax = MAX (umin, umax);                   /* properly confine boundaries */
      scc->hadjustment->lower = 0;
      scc->hadjustment->upper = CLAMP (scc->hadjustment->upper, umin, umax);
      scc->hadjustment->step_increment = self->tpt;
      scc->hadjustment->page_increment = self->tpt * 4;
    }
  GXK_SCROLL_CANVAS_CLASS (bst_track_roll_parent_class)->update_adjustments (scc, hadj, vadj);
}

static void
bst_track_roll_hsetup (BstTrackRoll *self,
		       guint	     tpt,
		       guint	     max_ticks,
		       gdouble	     hzoom)
{
  guint old_tpt = self->tpt;
  guint old_max_ticks = self->max_ticks;
  gdouble old_hzoom = self->hzoom;

  /* here, we setup all things necessary to determine our
   * horizontal layout. we have to avoid resizes at
   * least if just max_ticks changes, since the tick range
   * might need to grow during pointer grabs
   */
  self->tpt = MAX (tpt, 1);
  self->max_ticks = MAX (max_ticks, 1);
  self->hzoom = hzoom;
  if (old_tpt != self->tpt ||
      old_max_ticks != self->max_ticks ||
      old_hzoom != self->hzoom)
    {
      self->draw_tact_grid = ticks_to_pixels (self, self->tpt) >= 3;
      gtk_widget_queue_draw (GTK_WIDGET (self));
      X_OFFSET (self) = GXK_SCROLL_CANVAS (self)->hadjustment->value;
      gxk_scroll_canvas_update_adjustments (GXK_SCROLL_CANVAS (self), TRUE, FALSE);
    }
}

gdouble
bst_track_roll_set_hzoom (BstTrackRoll *self,
			  gdouble       hzoom)
{
  GxkScrollCanvas *scc = GXK_SCROLL_CANVAS (self);
  guint i;

  hzoom = CLAMP (hzoom, 0.1, 100);
  bst_track_roll_hsetup (self, self->tpt, self->max_ticks, hzoom / 50);
  /* readjust markers */
  for (i = 0; i < scc->n_markers; i += 2)
    track_roll_allocate_2markers (self, scc->markers + i);
  return self->hzoom * 50;
}

static void
track_roll_handle_drag (GxkScrollCanvas     *scc,
                        GxkScrollCanvasDrag *scc_drag,
                        GdkEvent            *event)
{
  BstTrackRoll *self = BST_TRACK_ROLL (scc);
  BstTrackRollDrag drag_mem = { 0 }, *drag = &drag_mem;
  gint hdrag = scc_drag->canvas_drag || scc_drag->top_panel_drag;
  gint vdrag = scc_drag->canvas_drag || scc_drag->left_panel_drag;
  /* copy over drag setup */
  memcpy (drag, scc_drag, sizeof (*scc_drag));  /* sizeof (*scc_drag) < sizeof (*drag) */
  drag->troll = self;
  /* calculate widget specific drag data */
  if (hdrag)
    drag->current_tick = coord_to_tick (self, MAX (drag->current_x, 0), FALSE);
  if (vdrag)
    {
      drag->current_row = coord_to_row (self, drag->current_y, &drag->current_valid);
      drag->current_track = row_to_track (self, drag->current_row);
    }
  /* sync start-position fields and select row */
  if (drag->type == GXK_DRAG_START)
    {
      drag->start_row = self->start_row = drag->current_row;
      drag->start_track = self->start_track = drag->current_track;
      drag->start_tick = self->start_tick = drag->current_tick;
      drag->start_valid = self->start_valid = drag->current_valid;
      if (drag->current_valid && (drag->canvas_drag || drag->left_panel_drag))
        g_signal_emit (self, signal_select_row, 0, drag->current_row);
    }
  else
    {
      drag->start_row = self->start_row;
      drag->start_track = self->start_track;
      drag->start_tick = self->start_tick;
      drag->start_valid = self->start_valid;
    }
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

static gboolean
bst_track_roll_button_press (GtkWidget	    *widget,
			     GdkEventButton *event)
{
  BstTrackRoll *self = BST_TRACK_ROLL (widget);
  if (self->ecell)
    bst_track_roll_stop_edit (self);
  return GTK_WIDGET_CLASS (bst_track_roll_parent_class)->button_press_event (widget, event);
}

void
bst_track_roll_set_track_callback (BstTrackRoll   *self,
				   gpointer        data,
				   BstTrackRollTrackFunc get_track)
{
  g_return_if_fail (BST_IS_TRACK_ROLL (self));

  self->proxy_data = data;
  self->get_track = get_track;
  gtk_widget_queue_draw (GTK_WIDGET (self));
  bst_track_roll_queue_row_change (self, 0);
}

void
bst_track_roll_queue_row_change (BstTrackRoll *self,
                                 guint         row)
{
  GxkScrollCanvas *scc;
  GdkRectangle rect;

  g_return_if_fail (BST_IS_TRACK_ROLL (self));

  scc = GXK_SCROLL_CANVAS (self);
  gxk_scroll_canvas_get_canvas_size (scc, &rect.width, &rect.height);
  rect.x = 0;
  if (GTK_WIDGET_DRAWABLE (self) &&
      row_to_coords (self, row, &rect.y, &rect.height))
    {
      gdk_window_invalidate_rect (CANVAS (self), &rect, TRUE);
      rect.x = 0;
      gdk_window_get_size (VPANEL (self), &rect.width, NULL);
      gdk_window_invalidate_rect (VPANEL (self), &rect, TRUE);
    }
  guint i, last_tick = 0;
  for (i = 0; ; i++)
    {
      SfiProxy track = row_to_track (self, i);
      if (!track)
        break;
      guint l = bse_track_get_last_tick (track);
      last_tick = MAX (last_tick, l);
    }
  bst_track_roll_hsetup (self, self->tpt, last_tick + 1, self->hzoom);
}

void
bst_track_roll_set_prelight_row (BstTrackRoll *self,
				 guint         row)
{
  g_return_if_fail (BST_IS_TRACK_ROLL (self));

  if (self->prelight_row != row)
    {
      gint clear_row = self->prelight_row;
      self->prelight_row = row;
      bst_track_roll_queue_row_change (self, clear_row);
      bst_track_roll_queue_row_change (self, self->prelight_row);
    }
}

static void
bst_track_roll_allocate_ecell (BstTrackRoll *self)
{
  if (GTK_WIDGET_REALIZED (self) && self->ecell)
    {
      GtkRequisition child_requisition;
      GtkAllocation allocation;
      gint ry, rheight, validrow;
      gtk_widget_size_request (GTK_WIDGET (self->ecell), &child_requisition);
      validrow = row_to_coords (self, self->ecell_row, &ry, &rheight);
      if (!validrow)
	{
	  allocation.x = allocation.y = -10;
	  allocation.width = allocation.height = 1;
	}
      else
	{
	  gint w = ticks_to_pixels (self, self->ecell_duration);
	  allocation.x = tick_to_coord (self, self->ecell_tick) + XTHICKNESS (self);
	  allocation.width = MAX (w - 2 * XTHICKNESS (self) - 1, 1);    /* 1 pixel safety margin from right border */
	  allocation.y = ry + (rheight - child_requisition.height) / 2;
	  allocation.height = child_requisition.height;
	}
      gtk_widget_size_allocate (GTK_WIDGET (self->ecell), &allocation);
    }
}

void
bst_track_roll_start_edit (BstTrackRoll    *self,
			   guint            row,
			   guint            tick,
			   guint            duration,
			   GtkCellEditable *ecell)
{
  gint ry, rheight, validrow;

  g_return_if_fail (BST_IS_TRACK_ROLL (self));
  g_return_if_fail (GTK_WIDGET_REALIZED (self));
  g_return_if_fail (GTK_IS_CELL_EDITABLE (ecell));
  g_return_if_fail (GTK_WIDGET_CAN_FOCUS (ecell));
  g_return_if_fail (GTK_WIDGET (ecell)->parent == NULL);
  g_return_if_fail (self->ecell == NULL);

  validrow = row_to_coords (self, row, &ry, &rheight);
  if (!validrow)
    {
      gtk_object_sink (GTK_OBJECT (ecell));
      return;
    }
  self->ecell = ecell;
  g_object_connect (self->ecell,
		    "signal::notify::is-focus", gxk_cell_editable_is_focus_handler, self,
		    "swapped_signal::editing_done", bst_track_roll_stop_edit, self,
		    NULL);
  self->ecell_row = row;
  self->ecell_tick = tick;
  self->ecell_duration = duration;
  gtk_widget_set_parent_window (GTK_WIDGET (self->ecell), CANVAS (self));
  gtk_widget_set_parent (GTK_WIDGET (self->ecell), GTK_WIDGET (self));
  gtk_cell_editable_start_editing (self->ecell, NULL);
  gtk_widget_grab_focus (GTK_WIDGET (self->ecell));
}

static void
track_roll_stop_edit (BstTrackRoll *self,
		      gboolean      canceled)
{
  g_return_if_fail (BST_IS_TRACK_ROLL (self));
  if (self->ecell)
    {
      g_signal_emit (self, signal_stop_edit, 0,
		     canceled || gxk_cell_editable_canceled (self->ecell),
		     self->ecell);
      g_object_disconnect (self->ecell,
			   "any_signal::notify::is-focus", gxk_cell_editable_is_focus_handler, self,
			   "any_signal::editing_done", bst_track_roll_stop_edit, self,
			   NULL);
      gtk_widget_unparent (GTK_WIDGET (self->ecell));
      self->ecell = NULL;
    }
}

void
bst_track_roll_abort_edit (BstTrackRoll *self)
{
  g_return_if_fail (BST_IS_TRACK_ROLL (self));

  track_roll_stop_edit (self, TRUE);
}

void
bst_track_roll_stop_edit (BstTrackRoll *self)
{
  g_return_if_fail (BST_IS_TRACK_ROLL (self));

  track_roll_stop_edit (self, FALSE);
}

void
bst_track_roll_set_marker (BstTrackRoll          *self,
                           guint                  mark_index,
                           guint                  position,
                           BstTrackRollMarkerType mtype)
{
  GxkScrollCanvas *scc = GXK_SCROLL_CANVAS (self);
  GxkScrollMarker *marker;
  guint count;
  g_return_if_fail (mark_index > 0);

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

  marker[0].coords.x = position;
  marker[1].coords.x = position;
  if (marker[0].mtype != mtype || !marker[0].pixmap)
    {
      marker[0].mtype = mtype;
      marker[1].mtype = mtype;
      track_roll_allocate_2markers (self, marker);
    }
  else
    track_roll_move_2markers (self, marker);
}

static void
bst_track_roll_class_init (BstTrackRollClass *class)
{
  static GdkColor colors[CINDEX_COUNT] = {
    { 0, 0xffff, 0x0000, 0x0000 },  /* red */
    { 0, 0x0000, 0xffff, 0x0000 },  /* green */
    { 0, 0x0000, 0x0000, 0xffff },  /* blue */
  };
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (class);
  GxkScrollCanvasClass *scroll_canvas_class = GXK_SCROLL_CANVAS_CLASS (class);
  
  gobject_class->finalize = bst_track_roll_finalize;

  object_class->destroy = bst_track_roll_destroy;
  
  widget_class->realize = bst_track_roll_realize;
  widget_class->unrealize = bst_track_roll_unrealize;
  widget_class->button_press_event = bst_track_roll_button_press;

  container_class->forall = track_roll_forall;

  scroll_canvas_class->hscrollable = TRUE;
  scroll_canvas_class->vscrollable = TRUE;
  scroll_canvas_class->n_colors = CINDEX_COUNT;
  scroll_canvas_class->colors = colors;
  scroll_canvas_class->get_layout = track_roll_get_layout;
  scroll_canvas_class->update_adjustments = track_roll_update_adjustments;
  scroll_canvas_class->adjustment_changed = track_roll_adjustment_changed;
  scroll_canvas_class->reallocate_contents = track_roll_reallocate_contents;
  scroll_canvas_class->draw_canvas = bst_track_roll_draw_canvas;
  scroll_canvas_class->draw_top_panel = bst_track_roll_draw_hpanel;
  scroll_canvas_class->draw_left_panel = bst_track_roll_draw_vpanel;
  scroll_canvas_class->draw_marker = bst_track_roll_draw_marker;
  scroll_canvas_class->handle_drag = track_roll_handle_drag;
  scroll_canvas_class->image_tint = gdk_color_from_rgb (0x00ffffff);
  scroll_canvas_class->image_saturation = 0;
  
  class->drag = NULL;
  class->clicked = NULL;
  
  signal_select_row = g_signal_new ("select-row", G_OBJECT_CLASS_TYPE (class),
				    G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BstTrackRollClass, select_row),
				    NULL, NULL,
				    bst_marshal_NONE__INT,
				    G_TYPE_NONE, 1, G_TYPE_INT);
  signal_drag = g_signal_new ("drag", G_OBJECT_CLASS_TYPE (class),
			      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BstTrackRollClass, drag),
			      NULL, NULL,
			      bst_marshal_NONE__POINTER,
			      G_TYPE_NONE, 1, G_TYPE_POINTER);
  signal_clicked = g_signal_new ("clicked", G_OBJECT_CLASS_TYPE (class),
				 G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BstTrackRollClass, clicked),
				 NULL, NULL,
				 bst_marshal_NONE__UINT_UINT_INT_BOXED,
				 G_TYPE_NONE, 4, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_INT,
				 GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  signal_stop_edit = g_signal_new ("stop-edit", G_OBJECT_CLASS_TYPE (class),
				   G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BstTrackRollClass, stop_edit),
				   NULL, NULL,
				   bst_marshal_NONE__BOOLEAN_OBJECT,
				   G_TYPE_NONE, 2,
				   G_TYPE_BOOLEAN, G_TYPE_OBJECT);
}
