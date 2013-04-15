// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "gxkcanvas.hh"
#include "gxkutils.hh"


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

gboolean
gnome_canvas_item_check_undisposed (GnomeCanvasItem *item)
{
  if (GNOME_IS_CANVAS_ITEM (item))
    {
      GnomeCanvasItem *parent = item;
      if (!item->parent &&
          (!GNOME_IS_CANVAS_GROUP (item) ||
           !GNOME_CANVAS_GROUP (item)->item_list))
        return FALSE;
      while (parent->parent)
        parent = parent->parent;
      if (GNOME_IS_CANVAS (item->canvas) && item->canvas == parent->canvas &&
          item->canvas->root == parent)
        return TRUE;
    }
  return FALSE;
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

/**
 * @param item	 canvas text item
 * @param pixels default font size
 *
 * Set the default font size in pixels of a canvas text
 * item. The size will be adjusted when the canvas zoomed
 * via gnome_canvas_set_zoom().
 */
void
gnome_canvas_text_set_zoom_size (GnomeCanvasText *item,
				 gdouble          pixels)
{
  g_return_if_fail (GNOME_IS_CANVAS_TEXT (item));

  g_object_set (item, "size_points", pixels * GNOME_CANVAS_ITEM(item)->canvas->pixels_per_unit, NULL);
  g_object_set_double (item, "zoom_size", pixels);
}

static void
canvas_adjust_text_zoom (GnomeCanvasGroup *group,
			 gdouble           pixels_per_unit)
{
  GList *list;
  for (list = group->item_list; list; list = list->next)
    {
      GnomeCanvasItem *item = (GnomeCanvasItem*) list->data;
      if (GNOME_IS_CANVAS_TEXT (item))
	{
	  gdouble zoom_size = g_object_get_double (item, "zoom_size");
	  if (zoom_size != 0)
	    g_object_set (item, "size_points", zoom_size * pixels_per_unit, NULL);
	}
      else if (GNOME_IS_CANVAS_GROUP (item))
	canvas_adjust_text_zoom (GNOME_CANVAS_GROUP (item), pixels_per_unit);
    }
}

/**
 * @param canvas          valid GnomeCanvas
 * @param pixels_per_unit zoom factor (defaults to 1.0)
 *
 * This function calls gnome_canvas_set_pixels_per_unit()
 * with its arguments and in addition adjusts the font sizes
 * of all canvas text items where gnome_canvas_text_set_zoom_size()
 * was used before.
 */
void
gnome_canvas_set_zoom (GnomeCanvas *canvas,
		       gdouble      pixels_per_unit)
{
  g_return_if_fail (GNOME_IS_CANVAS (canvas));

  /* adjust all text items */
  canvas_adjust_text_zoom (GNOME_CANVAS_GROUP (canvas->root), pixels_per_unit);
  /* perform the actual zoom */
  gnome_canvas_set_pixels_per_unit (canvas, pixels_per_unit);
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
