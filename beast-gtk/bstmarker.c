/* BEAST - Bedevilled Audio System
 * Copyright (C) 2003 Tim Janik
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
#include "bstmarker.h"
#include <string.h>

#define	MARK_PIXELS	3
#define	MARK_OFFSET	-1
#define	MARK_POS(self,n)	((self)->marks[(n)].pixoffset + MARK_OFFSET)
#define	MARK_BOUND(self,n)	((self)->marks[(n)].pixoffset + MARK_OFFSET + MARK_PIXELS)


/* --- functions --- */
void
bst_marker_init_vertical (BstMarkerSetup *self)
{
  GdkColor cred   = { 0, 0xe130, 0x0000, 0x0000 };
  GdkColor cgreen = { 0, 0x0000, 0xe130, 0x0000 };
  GdkColor cblue  = { 0, 0x0000, 0x0000, 0xe130 };
  GdkColor clight = { 0, 0xe0e0, 0xe0e0, 0xe0e0 };
  GdkColor cdark  = { 0, 0x0000, 0x0000, 0x0000 };

  g_return_if_fail (self != NULL);

  self->n_marks = 0;
  self->vmarks = TRUE;
  self->marks = NULL;
  self->drawable = NULL;
  self->pixmap = NULL;
  self->red = cred;
  self->green = cgreen;
  self->blue = cblue;
  self->light = clight;
  self->dark = cdark;
  self->red_gc = NULL;
  self->green_gc = NULL;
  self->blue_gc = NULL;
  self->light_gc = NULL;
  self->dark_gc = NULL;
}

void
bst_marker_finalize (BstMarkerSetup *self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->drawable == NULL);	/* !realized */

  g_free (self->marks);
  self->marks = NULL;
  self->n_marks = 0;
}

static void
resize_pixmap (BstMarkerSetup *self)
{
  gint width, height;
  GdkPixmap *pixmap;
  gdk_drawable_get_size (self->drawable, &width, &height);
  if (self->vmarks)
    width = MAX (self->n_marks * MARK_PIXELS, 1);
  else
    height = MAX (self->n_marks * MARK_PIXELS, 1);
  pixmap = gdk_pixmap_new (self->drawable, width, height, -1);
  if (self->pixmap)
    {
      gdk_draw_drawable (pixmap, self->red_gc, self->pixmap,
			 0, 0, 0, 0, width, height);
      g_object_unref (self->pixmap);
    }
  self->pixmap = pixmap;
}

void
bst_marker_realize (BstMarkerSetup *self,
		    GdkDrawable    *drawable)
{
  GdkGCValuesMask gcvm;
  GdkGCValues gcv;

  g_return_if_fail (self != NULL);
  g_return_if_fail (GDK_IS_DRAWABLE (drawable));

  if (self->drawable)
    bst_marker_unrealize (self);

  self->drawable = g_object_ref (drawable);
  resize_pixmap (self);
  gxk_color_alloc (gdk_drawable_get_colormap (self->drawable), &self->red);
  gxk_color_alloc (gdk_drawable_get_colormap (self->drawable), &self->green);
  gxk_color_alloc (gdk_drawable_get_colormap (self->drawable), &self->blue);
  gxk_color_alloc (gdk_drawable_get_colormap (self->drawable), &self->light);
  gxk_color_alloc (gdk_drawable_get_colormap (self->drawable), &self->dark);
  gcvm = GDK_GC_FOREGROUND;
  gcv.foreground = self->red;
  self->red_gc = gtk_gc_get (gdk_drawable_get_depth (self->drawable), gdk_drawable_get_colormap (self->drawable), &gcv, gcvm);
  gcv.foreground = self->green;
  self->green_gc = gtk_gc_get (gdk_drawable_get_depth (self->drawable), gdk_drawable_get_colormap (self->drawable), &gcv, gcvm);
  gcv.foreground = self->blue;
  self->blue_gc = gtk_gc_get (gdk_drawable_get_depth (self->drawable), gdk_drawable_get_colormap (self->drawable), &gcv, gcvm);
  gcv.foreground = self->light;
  self->light_gc = gtk_gc_get (gdk_drawable_get_depth (self->drawable), gdk_drawable_get_colormap (self->drawable), &gcv, gcvm);
  gcv.foreground = self->dark;
  self->dark_gc = gtk_gc_get (gdk_drawable_get_depth (self->drawable), gdk_drawable_get_colormap (self->drawable), &gcv, gcvm);
}

void
bst_marker_resize (BstMarkerSetup *self)
{
  g_return_if_fail (self != NULL);

  if (self->drawable)	/* realized */
    resize_pixmap (self);
}

void
bst_marker_unrealize (BstMarkerSetup *self)
{
  GdkColormap *cmap;

  g_return_if_fail (self != NULL);
  g_return_if_fail (self->drawable != NULL);     /* realized */

  gtk_gc_release (self->red_gc);
  gtk_gc_release (self->green_gc);
  gtk_gc_release (self->blue_gc);
  gtk_gc_release (self->light_gc);
  gtk_gc_release (self->dark_gc);
  self->red_gc = NULL;
  self->green_gc = NULL;
  self->blue_gc = NULL;
  self->light_gc = NULL;
  self->dark_gc = NULL;
  cmap = gdk_drawable_get_colormap (self->drawable);
  if (cmap)	/* might be destroyed */
    {
      gdk_colormap_free_colors (cmap, &self->red, 1);
      gdk_colormap_free_colors (cmap, &self->green, 1);
      gdk_colormap_free_colors (cmap, &self->blue, 1);
      gdk_colormap_free_colors (cmap, &self->light, 1);
      gdk_colormap_free_colors (cmap, &self->dark, 1);
    }
  g_object_unref (self->pixmap);
  self->pixmap = NULL;
  g_object_unref (self->drawable);
  self->drawable = NULL;
}

static void
bst_marker_draw (BstMarkerSetup *self,
                 gint            n)
{
  GdkGC *gc = bst_marker_get_gc (self, self->marks + n);
  gint pixoffset = self->marks[n].pixoffset;
  gint width, height;
  gdk_window_get_size (self->drawable, &width, &height);
  gdk_draw_line (self->drawable, self->light_gc,
		 pixoffset + MARK_OFFSET + 0, 0,
		 pixoffset + MARK_OFFSET + 0, height);
  // gdk_gc_set_function (gc, GDK_XOR);
  gdk_draw_line (self->drawable, gc,
		 pixoffset + MARK_OFFSET + 1, 0,
		 pixoffset + MARK_OFFSET + 1, height);
  // gdk_gc_set_function (gc, GDK_COPY);
  gdk_draw_line (self->drawable, self->dark_gc,
		 pixoffset + MARK_OFFSET + 2, 0,
		 pixoffset + MARK_OFFSET + 2, height);
}

static void
bst_marker_begin_paint (BstMarkerSetup *self,
			gint            n,
			GdkRectangle   *dest)
{
  GdkRectangle rect;

  g_return_if_fail (self->drawable != NULL);

  gdk_window_get_size (self->drawable, &rect.width, &rect.height);
  rect.y = 0;
  rect.x = MARK_POS (self, n);
  rect.width = MARK_PIXELS;
  gdk_window_begin_paint_rect (self->drawable, &rect);
  gdk_draw_drawable (self->drawable, self->red_gc, self->pixmap,
		     n * MARK_PIXELS, 0, rect.x, 0, rect.width, rect.height);
  if (dest)
    *dest = rect;
}

void
bst_marker_expose (BstMarkerSetup *self,
		   GdkRectangle   *area)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (area != NULL);

  if (self->drawable)	/* realized */
    {
      gint n, b1, b2;
      if (self->vmarks)
	{
	  b1 = area->x;
	  b2 = b1 + area->width;
	}
      else
	{
	  b1 = area->y;
	  b2 = b1 + area->height;
	}
      for (n = 0; n < self->n_marks; n++)
	if (self->marks[n].type && MARK_POS (self, n) < b2 && MARK_BOUND (self, n) > b1)
	  bst_marker_draw (self, n);
    }
}

void
bst_marker_save_backing (BstMarkerSetup *self,
			 GdkRectangle   *area)
{
  GdkDrawable *drawable;
  gint width, height, b1, b2, n;

  g_return_if_fail (self != NULL);
  g_return_if_fail (self->drawable != NULL);     /* realized */
  g_return_if_fail (area != NULL);

  drawable = self->drawable;
  if (self->vmarks)
    {
      b1 = area->x;
      b2 = b1 + area->width;
    }
  else
    {
      b1 = area->y;
      b2 = b1 + area->height;
    }
  gdk_window_get_size (self->drawable, &width, &height);
  for (n = 0; n < self->n_marks; n++)
    {
      gint p1 = MARK_POS (self, n), p2 = MARK_BOUND (self, n);
      if (p1 < b2 && p2 >= b1)
	{
	  gint z = n * MARK_PIXELS;
	  b1 = MAX (b1, p1);
	  b2 = MIN (b2, p2);
	  gdk_draw_drawable (self->pixmap, self->red_gc, drawable,
			     b1, area->y, z + (b1 - p1), area->y,
			     b2 - b1, area->height);
	}
    }
}

static void
bst_marker_fetch_backing (BstMarkerSetup *self,
			  gint            pixmap_pos,
			  gint            drawable_pos)
{
  GdkDrawable *drawable = self->drawable;
  if (drawable && pixmap_pos >= 0 && drawable_pos >= 0)
    {
      gint n, width, height;
      gdk_window_get_size (self->drawable, &width, &height);
      for (n = 0; n < self->n_marks; n++)
	{
	  gint p1 = MARK_POS (self, n), p2 = MARK_BOUND (self, n);
	  if (self->marks[n].type && p1 <= drawable_pos && p2 > drawable_pos)
	    {
	      gint z = n * MARK_PIXELS;
	      gdk_draw_drawable (self->pixmap, self->red_gc, self->pixmap,
				 z + drawable_pos - p1, 0, pixmap_pos, 0, 1, height);
	      return;
	    }
	}
      gdk_draw_drawable (self->pixmap, self->red_gc, drawable,
			 drawable_pos, 0, pixmap_pos, 0, 1, height);
    }
}

BstMarker*
bst_marker_add (BstMarkerSetup *self,
		guint           index)
{
  gint n;

  g_return_val_if_fail (self != NULL, NULL);

  n = self->n_marks++;
  self->marks = g_renew (BstMarker, self->marks, self->n_marks);
  self->marks[n].index = index;
  self->marks[n].type = BST_MARKER_NONE;
  self->marks[n].pixoffset = -MARK_PIXELS;
  self->marks[n].position = 0;
  self->marks[n].user_data = NULL;
  if (self->pixmap)
    resize_pixmap (self);
  return self->marks + n;
}

BstMarker*
bst_marker_get (BstMarkerSetup *self,
		guint           index)
{
  gint n;

  g_return_val_if_fail (self != NULL, NULL);

  for (n = 0; n < self->n_marks; n++)
    if (self->marks[n].index == index)
      return self->marks + n;
  return NULL;
}

void
bst_marker_delete (BstMarkerSetup *self,
		   BstMarker      *mark)
{
  guint n;

  g_return_if_fail (self != NULL);
  n = mark - self->marks;
  g_return_if_fail (n < self->n_marks);

  if (self->drawable)
    bst_marker_begin_paint (self, n, NULL);
  self->n_marks -= 1;
  g_memmove (self->marks + n, self->marks + n + 1, (self->n_marks - n) * sizeof (self->marks[0]));
  if (self->pixmap)
    {
      gint width, height, z = n * MARK_PIXELS, r = (self->n_marks - n) * MARK_PIXELS;
      gdk_drawable_get_size (self->pixmap, &width, &height);
      if (self->vmarks)
	gdk_draw_drawable (self->pixmap, self->red_gc, self->pixmap,
			   z + MARK_PIXELS, 0, z, 0, r, height);
      else
	gdk_draw_drawable (self->pixmap, self->red_gc, self->pixmap,
			   0, z + MARK_PIXELS, 0, z, width, r);
    }
  if (self->drawable)
    gdk_window_end_paint (self->drawable);
}

void
bst_marker_set (BstMarkerSetup *self,
		BstMarker      *mark,
		BstMarkerType   type,
		gint            pixoffset)
{
  guint n;

  g_return_if_fail (self != NULL);
  n = mark - self->marks;
  g_return_if_fail (n < self->n_marks);

  if (self->marks[n].type != type ||
      self->marks[n].pixoffset != pixoffset)
    {
      GdkRectangle area, area2;
      gint i;
      if (self->drawable)
	bst_marker_begin_paint (self, n, &area);
      self->marks[n].type = type;
      self->marks[n].pixoffset = -MARK_PIXELS;
      for (i = 0; i < MARK_PIXELS; i++)
	bst_marker_fetch_backing (self, n * MARK_PIXELS + i, pixoffset + MARK_OFFSET + i);
      self->marks[n].pixoffset = pixoffset;
      if (self->drawable)
	{
	  bst_marker_begin_paint (self, n, &area2);
	  bst_marker_expose (self, &area2);
	  gdk_window_end_paint (self->drawable);
	  bst_marker_expose (self, &area);
	  gdk_window_end_paint (self->drawable);
	}
    }
}

void
bst_marker_scroll (BstMarkerSetup *self,
		   gint            xdiff,
		   gint            ydiff)
{
  gint n, diff;

  g_return_if_fail (self != NULL);

  diff = self->vmarks ? xdiff : ydiff;
  for (n = 0; n < self->n_marks; n++)
    self->marks[n].pixoffset += diff;
}


GdkGC*
bst_marker_get_gc (BstMarkerSetup *self,
		   BstMarker      *marker)
{
  GdkGC *gc;
  guint n;

  g_return_val_if_fail (self != NULL, NULL);
  n = marker - self->marks;
  g_return_val_if_fail (n < self->n_marks, NULL);

  switch (self->marks[n].type)
    {
    case BST_MARKER_RED:	gc = self->red_gc;	break;
    case BST_MARKER_GREEN:	gc = self->green_gc;	break;
    default:
    case BST_MARKER_BLUE:	gc = self->blue_gc;	break;
    }
  return gc;
}
