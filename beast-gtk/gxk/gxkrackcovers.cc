// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html

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
  GtkWidget *cover = (GtkWidget*) g_object_new (GTK_TYPE_FRAME,
                                                "visible", TRUE,
                                                "shadow_type", GTK_SHADOW_ETCHED_OUT,
                                                "border_width", 1,
                                                NULL);
  GtkTable *table = (GtkTable*) g_object_new (GTK_TYPE_TABLE,
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
  
  pix = (GtkWidget*) g_object_new (GTK_TYPE_ALIGNMENT, "visible", TRUE, NULL);
  gtk_table_attach (table, pix, 1, 2, 1, 2, GTK_EXPAND, GTK_EXPAND, 0, 0);

  g_object_ref (cover);
  gtk_object_sink (GTK_OBJECT (cover));
  
  return cover;
}

static void
rack_cover_grow_area_horizontal (GxkRackTable *rtable,
				 guint         col,
				 guint         row,
				 guint        *hspan,
				 guint        *vspan)
{
  GtkTable *table = GTK_TABLE (rtable);
  guint i, j;
  
  for (i = 1; col + i < table->ncols; i++)
    if (gxk_rack_table_check_cell (rtable, col + i, row))
      break;
  for (j = 0; row + j < table->nrows; j++)
    if (gxk_rack_table_check_area (rtable, FALSE, col, row + j, i, 1, NULL))
      break;
  *hspan = i;
  *vspan = j;
}

static void
rack_cover_grow_area_vertical (GxkRackTable *rtable,
			       guint         col,
			       guint         row,
			       guint        *hspan,
			       guint        *vspan)
{
  GtkTable *table = GTK_TABLE (rtable);
  guint i, j;
  
  for (j = 1; row + j < table->nrows; j++)
    if (gxk_rack_table_check_cell (rtable, col, row + j))
      break;
  for (i = 0; col + i < table->ncols; i++)
    if (gxk_rack_table_check_area (rtable, FALSE, col + i, row, 1, j, NULL))
      break;
  *hspan = i;
  *vspan = j;
}

static GSList*
rack_cover_add_plates (GxkRackTable *rtable)
{
  GtkTable *table = GTK_TABLE (rtable);
  GtkWidget *cover;
  guint i, j;
  guint k, l, m, n;
  GSList *plate_list = NULL;
  
  if (0)
    {
      /* grab the first free cell and start growing a cover from it.
       * first vertically, then horizontally.
       * this approach can lead to a very large number of covers.
       */
      for (i = 0; i < table->ncols; i++)
        for (j = 0; j < table->nrows; j++)
          if (!gxk_rack_table_check_cell (rtable, i, j))
            {
              rack_cover_grow_area_vertical (rtable, i, j, &k, &l);
              rack_cover_grow_area_horizontal (rtable, i, j, &m, &n);
              
              if (m * n > k * l)
                {
                  k = m;
                  l = n;
                }
              
              cover = rack_cover_create (GTK_WIDGET (rtable));
              plate_list = g_slist_prepend (plate_list, cover);
              gxk_rack_table_attach (rtable, cover, i, j, k, l);
            }
    }
  else
    {
      gint max;
      /* try to minimize number of covers by maximizing cover size:
       * - find cover rectangle for each cell, choose the biggest cover.
       * - start over untill all cells are covered.
       * as a slight modification, areas are deemed big enough if
       * they cover more than half of the available space. this tends
       * to avoid lots of small fillers for leftover regions.
       */
      do
        {
          gint max_free = 0;
          for (i = 0; i < table->ncols; i++)
            for (j = 0; j < table->nrows; j++)
              if (!gxk_rack_table_check_cell (rtable, i, j))
                max_free++;
          max = 0; k = 0; l = 0; m = 0; n = 0;
          for (i = 0; i < table->ncols; i++)
            for (j = 0; j < table->nrows; j++)
              if (!gxk_rack_table_check_cell (rtable, i, j))
                {
                  guint t, u;
                  max_free--;
                  gxk_rack_table_expand_rect (rtable, i, j, &t, &u);
                  if (int (t * u) > max)
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
              gxk_rack_table_attach (rtable, cover, k, l, m, n);
            }
        }
      while (max > 0);
    }
  return plate_list;
}
