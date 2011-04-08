/* BEAST - Better Audio System
 * Copyright (C) 2002 Tim Janik
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
#include <gtk/gtk.h>
#include "bstracktable.h"
#include "bstdial.h"

static GSList *plate_list = NULL;

static GtkWidget*
make_cover (GtkWidget *rtable)
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
rtable_grow_area_horizontal (BstRackTable *rtable,
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
rtable_grow_area_vertical (BstRackTable *rtable,
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

static void
add_plates (BstRackTable *rtable)
{
  GtkTable *table;
  GtkWidget *cover;
  guint i, j, max;
  guint k, l, m, n;

  table = GTK_TABLE (rtable);

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
	  bst_rack_widget_set_info (cover, i, j, k, l);
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
	      guint t, u, v, w;

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
	  cover = make_cover (GTK_WIDGET (rtable));
	  plate_list = g_slist_prepend (plate_list, cover);
	  g_assert (k < GTK_TABLE (rtable)->ncols);
	  g_assert (l < GTK_TABLE (rtable)->nrows);
	  g_assert (k + m <= GTK_TABLE (rtable)->ncols);
	  g_assert (l + n <= GTK_TABLE (rtable)->nrows);
	  bst_rack_widget_set_info (cover, k, l, m, n);
	  gtk_container_add (GTK_CONTAINER (rtable), cover);
	}
    }
  while (max > 0);
#endif
}

static void
rtable_toggle_edit (BstRackTable *rtable)
{
  bst_rack_table_set_edit_mode (rtable, !rtable->edit_mode);
  if (!rtable->edit_mode)
    add_plates (rtable);
  else
    {
      GSList *slist;

      for (slist = plate_list; slist; slist = slist->next)
	gtk_widget_destroy (slist->data);
      g_slist_free (plate_list);
      plate_list = NULL;
    }
}

int
main (int   argc,
      char *argv[])
{
  GtkWidget *window, *vbox, *hbox, *rtable, *dial, *button, *scale, *frame;
  gpointer adjustment;
  
  gtk_init (&argc, &argv);

  window = g_object_new (GTK_TYPE_WINDOW, NULL);
  g_object_connect (window,
		    "signal::destroy", gtk_main_quit, NULL,
		    NULL);
  vbox = g_object_new (GTK_TYPE_VBOX,
		       "visible", TRUE,
		       "parent", window,
		       NULL);
  hbox = g_object_new (GTK_TYPE_VBOX,
		       "visible", TRUE,
		       "parent", vbox,
		       NULL);
  rtable = g_object_new (BST_TYPE_RACK_TABLE,
			 "visible", TRUE,
			 "parent", vbox,
			 "border_width", 10,
			 NULL);
  gtk_table_resize (GTK_TABLE (rtable), 40, 60);
  
  frame = g_object_new (GTK_TYPE_FRAME,
			"visible", TRUE,
			"child", g_object_new (GTK_TYPE_LABEL,
					       "visible", TRUE,
					       "label", "huhu",
					       NULL),
			NULL);
  bst_rack_widget_set_info (frame, -1, -1, 16, 16);
  gtk_container_add (GTK_CONTAINER (rtable), frame);

  bst_rack_widget_set_info (g_object_new (GTK_TYPE_LABEL,
					  "visible", TRUE,
					  "label", "HAHA21",
					  "parent", rtable,
					  NULL),
			    -1, -1, 5, 3);
  g_object_new (GTK_TYPE_LABEL,
		"visible", TRUE,
		"label", "XXXX-43",
		"parent", rtable,
		NULL);
  g_object_new (GTK_TYPE_LABEL,
		"visible", TRUE,
		"label", "v37____u",
		"parent", rtable,
		NULL);
  
  button = g_object_connect (g_object_new (GTK_TYPE_BUTTON,
					   "label", "Edit",
					   "parent", hbox,
					   "visible", TRUE,
					   NULL),
			     "swapped_signal::clicked", rtable_toggle_edit, rtable,
			     NULL);

  adjustment = gtk_adjustment_new (0, 0, 1, 0.05, 0.25, 0.5);

  dial = g_object_new (BST_TYPE_DIAL,
		       "visible", TRUE,
		       "parent", g_object_new (GTK_TYPE_FRAME,
					       "visible", TRUE,
					       "shadow_type", GTK_SHADOW_OUT,
					       "border_width", 1,
					       "parent", rtable,
					       NULL),
		       NULL);
  bst_dial_set_adjustment (BST_DIAL (dial), adjustment);
  
  frame = g_object_new (GTK_TYPE_FRAME,
			"visible", TRUE,
			"shadow_type", GTK_SHADOW_ETCHED_OUT,
			"border_width", 1,
			NULL);
  scale = g_object_new (GTK_TYPE_HSCALE,
			"visible", TRUE,
			"draw_value", FALSE,
			"adjustment", adjustment,
			"parent", frame,
			NULL);
  bst_rack_widget_set_info (frame, -1, -1, 8, 4);
  gtk_container_add (GTK_CONTAINER (rtable), frame);

  scale = g_object_new (GTK_TYPE_VSCALE,
			"visible", TRUE,
			"draw_value", FALSE,
			"adjustment", adjustment,
			NULL);
  bst_rack_widget_set_info (scale, -1, -1, 4, 16);
  gtk_container_add (GTK_CONTAINER (rtable), scale);

  add_plates (rtable);
  gtk_widget_show (window);
  gtk_main ();
  
  return 0;
}
