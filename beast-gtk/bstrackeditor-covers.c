/* BEAST - Bedevilled Audio System
 * Copyright (C) 2001-2002 Tim Janik
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



/* --- functions --- */
static GtkWidget*
rack_cover_create (GtkWidget *rtable)
{
  static const gchar *rivet_xpm[] = {
    "8 8 2 1", "  c None", "# c #a0a0a0",
    "        ",
    "  ####  ",
    " ###### ",
    " # #### ",
    " #  ### ",
    " #   ## ",
    "  ####  ",
    "        ",
  };
  GtkWidget *cover = g_object_new (GTK_TYPE_FRAME,
				   "visible", TRUE,
				   "shadow_type", GTK_SHADOW_ETCHED_OUT,
				   "border_width", 1,
				   NULL);
  GtkTable *table = g_object_new (GTK_TYPE_TABLE,
				  "visible", TRUE,
				  "parent", cover,
				  NULL);
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  GtkWidget *pix;

  pixmap = gdk_pixmap_colormap_create_from_xpm_d (NULL, gtk_widget_get_colormap (rtable), &mask, NULL, (gchar**) rivet_xpm);
  pix = gtk_pixmap_new (pixmap, mask);
  gtk_widget_show (pix);
  gtk_table_attach (table, pix, 0, 1, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);

  pixmap = gdk_pixmap_colormap_create_from_xpm_d (NULL, gtk_widget_get_colormap (rtable), &mask, NULL, (gchar**) rivet_xpm);
  pix = gtk_pixmap_new (pixmap, mask);
  gtk_widget_show (pix);
  gtk_table_attach (table, pix, 2, 3, 2, 3, GTK_SHRINK, GTK_SHRINK, 0, 0);

  pixmap = gdk_pixmap_colormap_create_from_xpm_d (NULL, gtk_widget_get_colormap (rtable), &mask, NULL, (gchar**) rivet_xpm);
  pix = gtk_pixmap_new (pixmap, mask);
  gtk_widget_show (pix);
  gtk_table_attach (table, pix, 0, 1, 2, 3, GTK_SHRINK, GTK_SHRINK, 0, 0);

  pixmap = gdk_pixmap_colormap_create_from_xpm_d (NULL, gtk_widget_get_colormap (rtable), &mask, NULL, (gchar**) rivet_xpm);
  pix = gtk_pixmap_new (pixmap, mask);
  gtk_widget_show (pix);
  gtk_table_attach (table, pix, 2, 3, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);

  gdk_pixmap_unref (pixmap);
  gdk_pixmap_unref (mask);

  pix = g_object_new (GTK_TYPE_ALIGNMENT, "visible", TRUE, NULL);
  gtk_table_attach (table, pix, 1, 2, 1, 2, GTK_EXPAND, GTK_EXPAND, 0, 0);

  return cover;
}

static void
rack_cover_grow_area_horizontal (BstRackTable *rtable,
				 guint         col,
				 guint         row,
				 guint        *hspan,
				 guint        *vspan)
{
  GtkTable *table = GTK_TABLE (rtable);
  guint i, j;

  for (i = 1; col + i < table->ncols; i++)
    if (bst_rack_table_check_cell (rtable, col + i, row))
      break;
  for (j = 0; row + j < table->nrows; j++)
    if (bst_rack_table_check_area (rtable, col, row + j, i, 1))
      break;
  *hspan = i;
  *vspan = j;
}

static void
rack_cover_grow_area_vertical (BstRackTable *rtable,
			       guint         col,
			       guint         row,
			       guint        *hspan,
			       guint        *vspan)
{
  GtkTable *table = GTK_TABLE (rtable);
  guint i, j;

  for (j = 1; row + j < table->nrows; j++)
    if (bst_rack_table_check_cell (rtable, col, row + j))
      break;
  for (i = 0; col + i < table->ncols; i++)
    if (bst_rack_table_check_area (rtable, col + i, row, 1, j))
      break;
  *hspan = i;
  *vspan = j;
}

static GSList*
rack_cover_add_plates (BstRackTable *rtable)
{
  GtkTable *table = GTK_TABLE (rtable);
  GtkWidget *cover;
  guint i, j, max;
  guint k, l, m, n;
  GSList *plate_list = NULL;

#if 0
  for (i = 0; i < table->ncols; i++)
    for (j = 0; j < table->nrows; j++)
      if (!bst_rack_table_check_cell (rtable, i, j))
	{
	  rtable_grow_area_vertical (rtable, i, j, &k, &l);
	  rtable_grow_area_horizontal (rtable, i, j, &m, &n);

	  if (m * n > k * l)
	    {
	      k = m;
	      l = n;
	    }

	  cover = make_cover (GTK_WIDGET (rtable));
	  plate_list = g_slist_prepend (plate_list, cover);
	  bst_rack_child_set_info (cover, i, j, k, l);
	  gtk_container_add (GTK_CONTAINER (rtable), cover);
	}
#else
  do
    {
      gint max_free = 0;
      for (i = 0; i < table->ncols; i++)
	for (j = 0; j < table->nrows; j++)
	  if (!bst_rack_table_check_cell (rtable, i, j))
	    max_free++;
      max = 0; k = 0; l = 0; m = 0; n = 0;
      for (i = 0; i < table->ncols; i++)
	for (j = 0; j < table->nrows; j++)
	  if (!bst_rack_table_check_cell (rtable, i, j))
	    {
	      guint t, u;

	      max_free--;
	      bst_rack_table_expand_rect (rtable, i, j, &t, &u);
	      if (t * u > max)
		{
		  max = t * u;
		  k = i;
		  l = j;
		  m = t;
		  n = u;
		  if (max > max_free / 2)
		    goto max_found;
		}
	    }
    max_found:
      if (max)
	{
	  cover = rack_cover_create (GTK_WIDGET (rtable));
	  plate_list = g_slist_prepend (plate_list, cover);
	  g_assert (k < GTK_TABLE (rtable)->ncols);
	  g_assert (l < GTK_TABLE (rtable)->nrows);
	  g_assert (k + m <= GTK_TABLE (rtable)->ncols);
	  g_assert (l + n <= GTK_TABLE (rtable)->nrows);
	  bst_rack_child_set_info (cover, k, l, m, n);
	  gtk_container_add (GTK_CONTAINER (rtable), cover);
	}
    }
  while (max > 0);
#endif

  return plate_list;
}
