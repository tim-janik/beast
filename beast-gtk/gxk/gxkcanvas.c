/* GXK - Gtk+ Extension Kit
 * Copyright (C) 1998-2002 Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "gxkcanvas.h"


/* --- functions --- */
GnomeCanvasPoints*
gnome_canvas_points_new0 (guint num_points)
{
  GnomeCanvasPoints *points;
  guint i;
  
  g_return_val_if_fail (num_points > 1, NULL);
  
  points = gnome_canvas_points_new (num_points);
  for (i = 0; i < num_points; i++)
    {
      points->coords[i] = 0;
      points->coords[i + num_points] = 0;
    }
  
  return points;
}

GnomeCanvasPoints*
gnome_canvas_points_newv (guint num_points,
			  ...)
{
  GnomeCanvasPoints *points;
  guint i;
  va_list args;
  
  g_return_val_if_fail (num_points > 1, NULL);
  
  va_start (args, num_points);
  points = gnome_canvas_points_new (num_points);
  for (i = 0; i < num_points * 2; i++)
    points->coords[i] = va_arg (args, gdouble);
  va_end (args);
  
  return points;
}

GnomeCanvasItem*
gnome_canvas_typed_item_at (GnomeCanvas *canvas,
			    GtkType      item_type,
			    gdouble      world_x,
			    gdouble      world_y)
{
  GnomeCanvasItem *item;
  
  g_return_val_if_fail (GNOME_IS_CANVAS (canvas), NULL);
  
  item = gnome_canvas_get_item_at (canvas, world_x, world_y);
  while (item && !g_type_is_a (GTK_OBJECT_TYPE (item), item_type))
    item = item->parent;
  
  return item && g_type_is_a (GTK_OBJECT_TYPE (item), item_type) ? item : NULL;
}

guint
gnome_canvas_item_get_stacking (GnomeCanvasItem *item)
{
  g_return_val_if_fail (GNOME_IS_CANVAS_ITEM (item), 0);
  
  if (item->parent)
    {
      GnomeCanvasGroup *parent = GNOME_CANVAS_GROUP (item->parent);
      GList *list;
      guint pos = 0;
      
      for (list = parent->item_list; list; list = list->next)
	{
	  if (list->data == item)
	    return pos;
	  pos++;
	}
    }
  
  return 0;
}

void
gnome_canvas_item_keep_between (GnomeCanvasItem *between,
				GnomeCanvasItem *item1,
				GnomeCanvasItem *item2)
{
  g_return_if_fail (GNOME_IS_CANVAS_ITEM (between));
  g_return_if_fail (GNOME_IS_CANVAS_ITEM (item1));
  g_return_if_fail (GNOME_IS_CANVAS_ITEM (item2));
  
  if (between->parent && item1->parent && item2->parent)
    {
      if (item1->parent == between->parent && item2->parent == between->parent)
	{
	  guint n, i, z;
	  
	  n = gnome_canvas_item_get_stacking (item1);
	  i = gnome_canvas_item_get_stacking (item2);
	  z = gnome_canvas_item_get_stacking (between);
	  n = (n + i + (z > MIN (n, i))) / 2;
	  if (z < n)
	    gnome_canvas_item_raise (between, n - z);
	  else if (n < z)
	    gnome_canvas_item_lower (between, z - n);
	}
      else
	g_warning ("gnome_canvas_item_keep_between() called for non-siblings");
    }
}

void
gnome_canvas_item_keep_above (GnomeCanvasItem *above,
			      GnomeCanvasItem *item1,
			      GnomeCanvasItem *item2)
{
  g_return_if_fail (GNOME_IS_CANVAS_ITEM (above));
  g_return_if_fail (GNOME_IS_CANVAS_ITEM (item1));
  g_return_if_fail (GNOME_IS_CANVAS_ITEM (item2));
  
  if (above->parent && item1->parent && item2->parent)
    {
      if (item1->parent == above->parent && item2->parent == above->parent)
	{
	  guint n, i, z;
	  
	  n = gnome_canvas_item_get_stacking (item1);
	  i = gnome_canvas_item_get_stacking (item2);
	  z = gnome_canvas_item_get_stacking (above);
	  n = MAX (n, i) + 1;
	  if (z < n)
	    gnome_canvas_item_raise (above, n - z);
	  else if (n < z)
	    gnome_canvas_item_lower (above, z - n);
	}
      else
	g_warning ("gnome_canvas_item_keep_above() called for non-siblings");
    }
}

void
gnome_canvas_FIXME_hard_update (GnomeCanvas *canvas)
{
  return;
  g_return_if_fail (GNOME_IS_CANVAS (canvas));
  
  /* _first_ recalc bounds of already queued items */
  gnome_canvas_update_now (canvas);
  
  /* just requeueing an update doesn't suffice for rect-ellipses,
   * re-translating the root-item is good enough though.
   */
  gnome_canvas_item_move (canvas->root, 0, 0);
}
