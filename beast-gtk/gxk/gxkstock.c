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
#include "gxkstock.h"

#include <string.h>

/* --- variables --- */
guint  gxk_size_button = 0;
guint  gxk_size_big_button = 0;
guint  gxk_size_canvas = 0;
guint  gxk_size_toolbar = 0;
guint  gxk_size_menu = 0;
guint  gxk_size_info_sign = 0;
guint  gxk_size_palette = 0;
static GtkIconFactory *stock_icon_factory = NULL;


/* --- functions --- */
void
_gxk_init_stock (void)
{
  stock_icon_factory = gtk_icon_factory_new ();
  gtk_icon_factory_add_default (stock_icon_factory);

  /* setup icon sizes */
  gxk_size_button = GTK_ICON_SIZE_BUTTON;       /* 20x20 */
  gxk_size_big_button = GTK_ICON_SIZE_DND;      /* 32x32 */
  gxk_size_info_sign = GTK_ICON_SIZE_DIALOG;    /* 48x48 */
  gxk_size_menu = GTK_ICON_SIZE_MENU;           /* 16x16 */
  gxk_size_toolbar = gxk_size_big_button;
  gxk_size_canvas = gtk_icon_size_register ("GxkIconSizeCanvas", 64, 64);
  gxk_size_palette = gxk_size_toolbar;
}

/**
 * gxk_size_width
 * @icon_size:     image size id
 * @RETURNS:       image size width
 *
 * Return the width of a specific image size.
 */
guint
gxk_size_width (GtkIconSize icon_size)
{
  gint width = 10;
  gtk_icon_size_lookup (icon_size, &width, NULL);
  return width;
}

/**
 * gxk_size_height
 * @icon_size:     image size id
 * @RETURNS:       image size height
 *
 * Return the height of a specific image size.
 */
guint
gxk_size_height (GtkIconSize icon_size)
{
  gint height = 10;
  gtk_icon_size_lookup (icon_size, NULL, &height);
  return height;
}

/**
 * gxk_stock_image
 * @stock_icon_id: stock name
 * @icon_size:     image size
 * @RETURNS:       a visible GtkImage widget or %NULL
 *
 * Create a GtkImage widget with a stock image of a
 * certain size, or return %NULL if the image doesn't exist.
 * The returned image widget correctly displays the stock
 * icon, honouring prelight and sensitivity state of the widget.
 */
GtkWidget*
gxk_stock_image (const gchar *stock_icon_id,
		 GtkIconSize  icon_size)
{
  if (stock_icon_id && gtk_icon_factory_lookup_default (stock_icon_id))
    {
      GtkWidget *image = gtk_image_new_from_stock (stock_icon_id, icon_size);

      gtk_widget_show (image);
      return image;
    }
  return NULL;
}

/**
 * gxk_stock_button
 * @stock_id: stock name
 * @label:    button text
 * @RETURNS:  a visible GtkButton widget
 *
 * Create a GtkButton widget with a stock image and custom label text.
 */
GtkWidget*
gxk_stock_button (const gchar *stock_id,
		  const gchar *label)
{
  GtkWidget *button, *alignment;

  g_return_val_if_fail (stock_id != NULL, NULL);

  button = g_object_new (GTK_TYPE_BUTTON,
			 "visible", TRUE,
			 NULL);
  alignment = gxk_stock_button_child (stock_id, label);
  gtk_container_add (GTK_CONTAINER (button), alignment);

  return button;
}

/**
 * gxk_stock_button_child
 * @stock_id: stock name
 * @label:    button text
 * @RETURNS:  a visible widget suitable as GtkButton child
 *
 * This function does the same as gxk_stock_button() except
 * for creating the actual button. The button's child is instead
 * returned to the caller, this might e.g. be usefull if a button
 * is created containing multiple children which are shown only
 * selectively during runtime.
 */
GtkWidget*
gxk_stock_button_child (const gchar *stock_id,
			const gchar *label)
{
  GtkWidget *alignment, *hbox, *image;

  g_return_val_if_fail (stock_id != NULL, NULL);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (alignment), hbox);
  image = gxk_stock_image (stock_id, GXK_SIZE_BUTTON);
  if (image)
    gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox),
		      g_object_new (GTK_TYPE_LABEL,
				    "label", label ? label : gxk_stock_action (stock_id),
				    "use_underline", TRUE,
				    NULL),
		      FALSE, TRUE, 0);
  gtk_widget_show_all (alignment);

  return alignment;
}

/**
 * gxk_stock_register_icon
 * @icon: a validly filled out GxkStockIcon
 *
 * Register a new stock icon from an inlined pixbuf.
 * The inlined pixbuf pixels are not copied, so the
 * caller must make sure for the data to remain valid.
 */
void
gxk_stock_register_icon (const GxkStockIcon *icon)
{
  GdkPixbuf *pixbuf;
  GtkIconSet *iset;

  g_return_if_fail (icon != NULL);

  pixbuf = gdk_pixbuf_new_from_inline (-1, icon->inlined_pixbuf, FALSE, NULL);
  iset = gtk_icon_set_new_from_pixbuf (pixbuf);
  g_object_unref (pixbuf);
  gtk_icon_factory_add (stock_icon_factory, icon->stock_id, iset);
  gtk_icon_set_unref (iset);
}

/**
 * gxk_stock_register_icons
 * @n_icons: number of icons to register
 * @icons:   a validly filled out array of GxkStockIcon
 *
 * For all @n_icons contained in @icons, call gxk_stock_register_icon().
 */
void
gxk_stock_register_icons (guint               n_icons,
			  const GxkStockIcon *icons)
{
  if (n_icons)
    {
      guint i;

      g_return_if_fail (icons != NULL);

      for (i = 0; i < n_icons; i++)
	gxk_stock_register_icon (icons + i);
    }
}

/**
 * gxk_stock_register_action
 * @action: a validly filled out GxkStockAction
 *
 * Register a new stock action. The new stock action
 * @action->stock_id is registered with an action label
 * of @action->label and the stock icon @action->stock_fallback.
 * If @action->label is %NULL, @action->stock_fallback must
 * not be %NULL, and @action->stock_id becomes merely an
 * alias of @action->stock_fallback.
 */
void
gxk_stock_register_action (const GxkStockAction *action)
{
  GtkStockItem item = { NULL, };

  g_return_if_fail (action != NULL);
  g_return_if_fail (action->stock_id != NULL);
  g_return_if_fail (action->label != NULL || action->stock_fallback != NULL);

  item.stock_id = (gchar*) action->stock_id;
  if (action->label)
    item.label = (gchar*) action->label;
  else
    item.label = (gchar*) gxk_stock_action (action->stock_fallback);
  item.modifier = 0;
  item.keyval = 0;
  item.translation_domain = NULL;
  gtk_stock_add (&item, 1);

  if (action->stock_fallback)
    {
      GtkIconSet *iset = gtk_icon_factory_lookup_default (action->stock_fallback);
      if (iset)
	gtk_icon_factory_add (stock_icon_factory, action->stock_id, iset);
    }
}

/**
 * gxk_stock_register_actions
 * @n_actions: number of actions to register
 * @actions:   a validly filled out array of GxkStockAction
 *
 * For all @n_actions contained in @actions, call gxk_stock_register_action().
 */
void
gxk_stock_register_actions (guint                 n_actions,
			    const GxkStockAction *actions)
{
  if (n_actions)
    {
      guint i;

      g_return_if_fail (actions != NULL);

      for (i = 0; i < n_actions; i++)
	gxk_stock_register_action (actions + i);
    }
}

const gchar*
gxk_stock_action (const gchar *stock_id)
{
  const gchar *action = NULL;
  GtkStockItem item;

  g_return_val_if_fail (stock_id != NULL, NULL);

  if (gtk_stock_lookup (stock_id, &item))
    action = item.label;
  else
    action = stock_id;

  return action;
}

/**
 * gxk_stock_fallback_pixbuf
 * @stock_id: stock name
 * @RETURNS:  this @stock_id's pixbuf (or %NULL for invalid stock ids)
 *
 * Return the pixbuf associated with @stock_id, the pixbuf is
 * owned by the stock system and persists throughout runtime.
 * To display stock icons in widgets use gxk_stock_image() and not
 * this pixbuf.
 */
GdkPixbuf*
gxk_stock_fallback_pixbuf (const gchar *stock_id)
{
  static GData *stock_pixbuf_repo = NULL;
  GdkPixbuf *pixbuf;

  g_return_val_if_fail (stock_id != NULL, NULL);

  pixbuf = g_datalist_get_data (&stock_pixbuf_repo, stock_id);
  if (!pixbuf)
    {
      GtkIconSet *iset = gtk_icon_factory_lookup_default (stock_id);

      if (iset)
	{
	  static GtkStyle *style = NULL;	/* FIXME: GTKFIX: gtk_icon_set_render_icon() shouldn't demand a style */
	  if (!style)
	    style = gtk_style_new ();
	  pixbuf = gtk_icon_set_render_icon (iset, style, GTK_TEXT_DIR_NONE,
					     GTK_STATE_NORMAL, -1, NULL, NULL);
	  g_datalist_set_data (&stock_pixbuf_repo, stock_id, pixbuf);
	}
    }
  return pixbuf;
}

static guint8*
gdk_pixbuf_create_bitmap_data (GdkPixbuf *pixbuf,
			       gint      *width_p,
			       gint      *height_p,
			       guint8     alpha_threshold)
{
  guint width, height, rowstride, x, y;
  guint8 *data, *buffer;

  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);

  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  buffer = gdk_pixbuf_get_pixels (pixbuf);

  data = g_new0 (guint8, (width + 7) / 8 * height);
  if (!gdk_pixbuf_get_has_alpha (pixbuf))
    memset (data, 0xff, (width + 7) / 8 * height);
  else
    for (y = 0; y < height; y++)
      for (x = 0; x < width; x++)
	{
	  guint8 *d = data + (width + 7) / 8 * y + x / 8;
	  guint8 *buf = buffer + y * rowstride + x * 4;

	  if (buf[3] >= alpha_threshold)
	    *d |= 1 << (x % 8);
	}
  if (width_p)
    *width_p = width;
  if (height_p)
    *height_p = height;
  return data;
}

/**
 * gxk_stock_icon_window
 * @stock_id: a valid stock icon name
 * @RETURNS:  a window showing the stock icon
 *
 * Create a window displaying a stock icon which is transparent
 * according to the icon's alpha mask. Such windows are mostly
 * usefull to implement drag-and-drop operations with stock icons.
 */
GtkWidget*
gxk_stock_icon_window (const gchar *stock_id)
{
  GtkWidget *drag_window, *image;
  GdkPixbuf *pixbuf;
  GdkBitmap *mask;
  guint8 *bitmap_data;
  gint width, height;

  g_return_val_if_fail (stock_id != NULL, NULL);

  pixbuf = gxk_stock_fallback_pixbuf (stock_id);
  if (!pixbuf)
    {
      g_warning ("%s: unregistered stock id: \"%s\"", G_STRLOC, stock_id);
      return NULL;
    }

  image = gtk_image_new_from_pixbuf (pixbuf);
  gtk_widget_show (image);

  drag_window = gtk_widget_new (GTK_TYPE_WINDOW,
				"type", GTK_WINDOW_POPUP,
				"child", image,
				NULL);
  gtk_widget_set_app_paintable (drag_window, TRUE);
  gtk_widget_realize (drag_window);
  gdk_window_raise (drag_window->window);
  bitmap_data = gdk_pixbuf_create_bitmap_data (pixbuf, &width, &height, 1);
  mask = gdk_bitmap_create_from_data (drag_window->window, bitmap_data, width, height);
  g_free (bitmap_data);
  gtk_widget_shape_combine_mask (drag_window, mask, 0, 0);
  gdk_pixmap_unref (mask);

  return drag_window;
}
