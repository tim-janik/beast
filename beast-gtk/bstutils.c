/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999, 2000, 2001 Tim Janik and Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include        "bstutils.h"

#include        "bstmenus.h"
#include        <fcntl.h>
#include        <errno.h>
#include        <unistd.h>
#include        <string.h>


/* compile marshallers */
#include        "bstmarshal.c"




/* --- generated types --- */
#include "bstgentypes.c"	/* type id defs */
#include "bstenum_arrays.c"	/* enum string value arrays plus include directives */
void
bst_init_gentypes (void)
{
  static gboolean initialized = FALSE;

  if (!initialized)
    {
      static struct {
	gchar            *type_name;
	GType             parent;
	GType            *type_id;
	gconstpointer     pointer1;
      } builtin_info[] = {
#include "bstenum_list.c"	/* type entries */
      };
      guint i;

      for (i = 0; i < sizeof (builtin_info) / sizeof (builtin_info[0]); i++)
	{
	  GType type_id = 0;

	  if (builtin_info[i].parent == G_TYPE_ENUM)
	    type_id = g_enum_register_static (builtin_info[i].type_name, builtin_info[i].pointer1);
	  else if (builtin_info[i].parent == G_TYPE_FLAGS)
	    type_id = g_flags_register_static (builtin_info[i].type_name, builtin_info[i].pointer1);
	  else
	    g_assert_not_reached ();
	  g_assert (g_type_name (type_id) != NULL);
	  *builtin_info[i].type_id = type_id;
	}
    }
}


/* --- Icons, Stock setup --- */
#include "./icons/bst-stock-gen.c"
typedef struct
{
  const gchar *stock_id;
  const gchar *data;
} StockIcon;
static StockIcon stock_pixdata_icons[] = {
  { BST_STOCK_EDIT_TOOL,	stock_editor,		},
  { BST_STOCK_INFO,		stock_info,		},
  { BST_STOCK_INSTRUMENT,	stock_instrument,	},
  { BST_STOCK_KNOB,		stock_knob,		},
  { BST_STOCK_LOAD,		stock_cdrom,		},
  { BST_STOCK_MOUSE_TOOL,	stock_mouse_tool,	},
  { BST_STOCK_NOICON,		stock_no_icon,		},
  { BST_STOCK_NO_ILINK,		stock_no_ilink,		},
  { BST_STOCK_NO_OLINK,		stock_no_olink,		},
  { BST_STOCK_PALETTE,		stock_palette,		},
  { BST_STOCK_PART,		stock_part,		},
  { BST_STOCK_PART_EDITOR,	stock_part_editor,	},
  { BST_STOCK_PART_TOOL,	stock_part_tool,	},
  { BST_STOCK_PATTERN,		stock_pattern,		},
  { BST_STOCK_PATTERN_GROUP,	stock_pattern_group,	},
  { BST_STOCK_PATTERN_TOOL,	stock_pattern_tool,	},
  { BST_STOCK_PREVIEW_AUDIO,	stock_small_audio,	},
  { BST_STOCK_PREVIEW_NOAUDIO,	stock_small_noaudio,	},
  { BST_STOCK_RECT_SELECT,	stock_rect_select,	},
  { BST_STOCK_TARGET,		stock_target,		},
  { BST_STOCK_TRASHCAN,		stock_trashcan,		},
  { BST_STOCK_VERT_SELECT,	stock_vert_select,	},
  { BST_STOCK_WAVE,		stock_wave,		},
  { BST_STOCK_WAVE_TOOL,	stock_wave_tool,	},
  { BST_STOCK_ZOOM_ANY,		stock_zoom_any,		},
};
static StockIcon stock_gtk_stock_ids[] = {
  { BST_STOCK_APPLY,		GTK_STOCK_APPLY,	},
  { BST_STOCK_CANCEL,		GTK_STOCK_CANCEL,	},
  { BST_STOCK_CDROM,		GTK_STOCK_CDROM,	},
  { BST_STOCK_CLONE,		GTK_STOCK_COPY,		},
  { BST_STOCK_CLOSE,		GTK_STOCK_CLOSE,	},
  { BST_STOCK_DEFAULT_REVERT,	GTK_STOCK_UNDO,		},
  { BST_STOCK_DELETE,		GTK_STOCK_DELETE,	},
  { BST_STOCK_EXECUTE,		GTK_STOCK_EXECUTE,	},
  { BST_STOCK_OK,		GTK_STOCK_OK,		},
  { BST_STOCK_OVERWRITE,	GTK_STOCK_SAVE,		},
  { BST_STOCK_PROPERTIES,	GTK_STOCK_PROPERTIES,	},
  { BST_STOCK_REDO,		GTK_STOCK_REDO,		},
  { BST_STOCK_REVERT,		GTK_STOCK_UNDO,		},
  { BST_STOCK_UNDO,		GTK_STOCK_UNDO,		},
  { BST_STOCK_ZOOM_100,		GTK_STOCK_ZOOM_100,	},
  { BST_STOCK_ZOOM_FIT,		GTK_STOCK_ZOOM_FIT,	},
  { BST_STOCK_ZOOM_IN,		GTK_STOCK_ZOOM_IN,	},
  { BST_STOCK_ZOOM_OUT,		GTK_STOCK_ZOOM_OUT,	},
};
/* stock icon sizes */
guint    bst_size_button = 0;
guint    bst_size_big_button = 0;
guint    bst_size_canvas = 0;
guint    bst_size_toolbar = 0;
guint    bst_size_menu = 0;
static GtkIconFactory *stock_icon_factory = NULL;
static GdkPixbuf      *stock_pixbuf_no_icon = NULL;
static GdkPixbuf      *stock_pixbuf_knob = NULL;
static GdkPixbuf      *stock_pixbuf_pattern_group = NULL;
static GdkPixbuf      *stock_pixbuf_pattern = NULL;

void
_bst_utils_init (void)
{
  guint i;

  g_assert (stock_icon_factory == NULL);

  stock_icon_factory = gtk_icon_factory_new ();

  /* setup icon sizes */
  bst_size_button = GTK_ICON_SIZE_BUTTON;	/* 20x20 */
  bst_size_big_button = GTK_ICON_SIZE_DND;	/* 32x32 */
  bst_size_menu = GTK_ICON_SIZE_MENU;		/* 16x16 */
  bst_size_toolbar = bst_size_big_button;
  bst_size_canvas = gtk_icon_size_register ("BstIconSizeCanvas", 64, 64);

  /* add beast icons to stock factory */
  for (i = 0; i < G_N_ELEMENTS (stock_pixdata_icons); i++)
    {
      StockIcon *icon = stock_pixdata_icons + i;
      GdkPixbuf *pixbuf = gdk_pixbuf_new_from_inline (-1, icon->data, FALSE, NULL);
      GtkIconSet *iset = gtk_icon_set_new_from_pixbuf (pixbuf);

      if (icon->stock_id == BST_STOCK_NOICON)
	stock_pixbuf_no_icon = pixbuf;
      else if (icon->stock_id == BST_STOCK_KNOB)
        stock_pixbuf_knob = pixbuf;
      else if (icon->stock_id == BST_STOCK_PATTERN_GROUP)
        stock_pixbuf_pattern_group = pixbuf;
      else if (icon->stock_id == BST_STOCK_PATTERN)
        stock_pixbuf_pattern = pixbuf;
      else
	g_object_unref (pixbuf);
      gtk_icon_factory_add (stock_icon_factory, icon->stock_id, iset);
      gtk_icon_set_unref (iset);
    }

  /* add gtk aliases to stock factory */
  for (i = 0; i < G_N_ELEMENTS (stock_gtk_stock_ids); i++)
    {
      StockIcon *icon = stock_gtk_stock_ids + i;

      gtk_icon_factory_add (stock_icon_factory,
			    icon->stock_id,
			    gtk_icon_factory_lookup_default (icon->data));
    }

  /* register globally */
  gtk_icon_factory_add_default (stock_icon_factory);
}

guint
bst_size_width (GtkIconSize bst_size)
{
  gint width = 10;

  gtk_icon_size_lookup (bst_size, &width, NULL);
  return width;
}

guint
bst_size_height (GtkIconSize bst_size)
{
  gint height = 10;

  gtk_icon_size_lookup (bst_size, NULL, &height);
  return height;
}

GtkWidget*
bst_image_from_stock (const gchar *stock_icon_id,
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

const gchar*
bst_stock_action (const gchar *stock_id)
{
  const gchar *action = NULL;
  GtkStockItem item;

  g_return_val_if_fail (stock_id != NULL, NULL);

  /* keep the lookup stupid and simple for the moment */
  if (strcmp (stock_id, BST_STOCK_CLONE) == 0)
    action = "_Clone";
  else if (strcmp (stock_id, BST_STOCK_REVERT) == 0)
    action = "_Revert";
  else if (strcmp (stock_id, BST_STOCK_DEFAULT_REVERT) == 0)
    action = "_Defaults";
  else if (strcmp (stock_id, BST_STOCK_OVERWRITE) == 0)
    action = "_Overwrite";
  else if (strcmp (stock_id, BST_STOCK_LOAD) == 0)
    action = "_Load";

  if (!action)
    {
      guint i;

      /* find gtk alias */
      for (i = 0; i < G_N_ELEMENTS (stock_gtk_stock_ids); i++)
	{
	  StockIcon *icon = stock_gtk_stock_ids + i;

	  if (strcmp (stock_id, icon->stock_id) == 0)
	    {
	      action = icon->data;
	      break;
	    }
	}
      if (gtk_stock_lookup (action, &item))
	action = item.label;
      else
	action = NULL;
    }
  if (!action)
    action = stock_id;

  return action;
}

GdkPixbuf*
bst_pixbuf_no_icon (void)
{
  return stock_pixbuf_no_icon;
}

GdkPixbuf*
bst_pixbuf_knob (void)
{
  return stock_pixbuf_knob;
}

GtkWidget*
bst_image_from_icon (BswIcon    *icon,
		     GtkIconSize icon_size)
{
  GdkPixbuf *pixbuf;
  GtkWidget *image;
  gint width, height, pwidth, pheight;

  if (!icon)
    return NULL;
  g_return_val_if_fail (icon->bytes_per_pixel == 3 || icon->bytes_per_pixel == 4, NULL);

  if (!gtk_icon_size_lookup (icon_size, &width, &height))
    return NULL;

  bsw_icon_ref (icon);
  pixbuf = gdk_pixbuf_new_from_data (icon->pixels, GDK_COLORSPACE_RGB, icon->bytes_per_pixel == 4,
				     8, icon->width, icon->height,
				     icon->width * icon->bytes_per_pixel,
				     NULL, NULL);
  g_object_set_data_full (G_OBJECT (pixbuf), "BswIcon", icon, (GtkDestroyNotify) bsw_icon_unref);

  pwidth = gdk_pixbuf_get_width (pixbuf);
  pheight = gdk_pixbuf_get_height (pixbuf);
  if (width != pwidth || height != pheight)
    {
      GdkPixbuf *tmp = pixbuf;

      pixbuf = gdk_pixbuf_scale_simple (pixbuf, width, height, GDK_INTERP_HYPER);
      g_object_unref (tmp);
    }

  image = gtk_image_new_from_pixbuf (pixbuf);
  g_object_unref (pixbuf);
  gtk_widget_show (image);

  return image;
}

GtkWidget*
bst_stock_button_child (const gchar *stock_id,
			const gchar *label)
{
  GtkWidget *alignment, *hbox, *image;

  g_return_val_if_fail (stock_id != NULL, NULL);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (alignment), hbox);
  image = bst_image_from_stock (stock_id, BST_SIZE_BUTTON);
  if (image)
    gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox),
		      g_object_new (GTK_TYPE_LABEL,
				    "label", label ? label : bst_stock_action (stock_id),
				    "use_underline", TRUE,
				    NULL),
		      FALSE, TRUE, 0);
  gtk_widget_show_all (alignment);

  return alignment;
}

GtkWidget*
bst_stock_button (const gchar *stock_id,
		  const gchar *label)
{
  GtkWidget *button, *alignment;

  g_return_val_if_fail (stock_id != NULL, NULL);

  button = g_object_new (GTK_TYPE_BUTTON,
			 "visible", TRUE,
			 NULL);
  alignment = bst_stock_button_child (stock_id, label);
  gtk_container_add (GTK_CONTAINER (button), alignment);

  return button;
}

GtkWidget*
bst_drag_window_from_stock (const gchar *stock)
{
  GtkWidget *drag_window, *image;
  GdkPixbuf *pixbuf;
  GdkBitmap *mask;
  guint8 *bitmap_data;
  gint width, height;

  g_return_val_if_fail (stock != NULL, NULL);

  if (strcmp (stock, BST_STOCK_PATTERN_GROUP) == 0)
    pixbuf = stock_pixbuf_pattern_group;
  else if (strcmp (stock, BST_STOCK_PATTERN) == 0)
    pixbuf = stock_pixbuf_pattern;
  else
    {
      g_warning ("unhandled stock id: \"%s\"", stock);
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

guint8*
gdk_pixbuf_create_bitmap_data (GdkPixbuf *pixbuf,
			       gint	 *width_p,
			       gint	 *height_p,
			       guint8	  alpha_threshold)
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


/* --- Gtk+ Utilities --- */
static gulong viewable_changed_id = 0;

void
gtk_widget_viewable_changed (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  g_signal_emit (widget, viewable_changed_id, 0);
}

static void
traverse_viewable_changed (GtkWidget *widget,
			   gpointer   data)
{
  if (GTK_IS_CONTAINER (widget))
    gtk_container_forall (GTK_CONTAINER (widget), (GtkCallback) gtk_widget_viewable_changed, NULL);
}

guint
gtk_tree_view_add_column (GtkTreeView       *tree_view,
			  gint               position,
			  GtkTreeViewColumn *column,
			  GtkCellRenderer   *cell,
			  const gchar       *attrib_name,
			  ...)
{
  guint n_cols;
  va_list var_args;

  g_return_val_if_fail (GTK_IS_TREE_VIEW (tree_view), 0);
  g_return_val_if_fail (GTK_IS_TREE_VIEW_COLUMN (column), 0);
  g_return_val_if_fail (column->tree_view == NULL, 0);
  g_return_val_if_fail (GTK_IS_CELL_RENDERER (cell), 0);

  g_object_ref (column);
  g_object_ref (cell);
  gtk_object_sink (GTK_OBJECT (column));
  gtk_object_sink (GTK_OBJECT (cell));
  gtk_tree_view_column_pack_start (column, cell, TRUE);

  va_start (var_args, attrib_name);
  while (attrib_name)
    {
      guint col = va_arg (var_args, guint);

      gtk_tree_view_column_add_attribute (column, cell, attrib_name, col);
      attrib_name = va_arg (var_args, const gchar*);
    }
  va_end (var_args);

  n_cols = gtk_tree_view_insert_column (tree_view, column, position);

  g_object_unref (column);
  g_object_unref (cell);

  return n_cols;
}

void
gtk_tree_selection_select_spath (GtkTreeSelection *tree_selection,
				 const gchar      *str_path)
{
  GtkTreePath *path;

  g_return_if_fail (GTK_IS_TREE_SELECTION (tree_selection));
  g_return_if_fail (str_path != NULL);

  path = gtk_tree_path_new_from_string (str_path);
  gtk_tree_selection_select_path (tree_selection, path);
  gtk_tree_path_free (path);
}

void
gtk_tree_selection_unselect_spath (GtkTreeSelection *tree_selection,
				   const gchar      *str_path)
{
  GtkTreePath *path;

  g_return_if_fail (GTK_IS_TREE_SELECTION (tree_selection));
  g_return_if_fail (str_path != NULL);

  path = gtk_tree_path_new_from_string (str_path);
  gtk_tree_selection_unselect_path (tree_selection, path);
  gtk_tree_path_free (path);
}

void
gtk_post_init_patch_ups (void)
{
  viewable_changed_id =
    g_signal_newv ("viewable-changed",
		   G_TYPE_FROM_CLASS (gtk_type_class (GTK_TYPE_WIDGET)),
		   G_SIGNAL_RUN_LAST,
		   g_cclosure_new (G_CALLBACK (traverse_viewable_changed), NULL, NULL),
		   NULL, NULL,
		   gtk_marshal_VOID__VOID,
		   G_TYPE_NONE, 0, NULL);
}

gboolean
gtk_widget_viewable (GtkWidget *widget)
{
  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

  while (widget)
    {
      if (!GTK_WIDGET_MAPPED (widget))
	return FALSE;
      widget = widget->parent;
    }
  return TRUE;
}

void
gtk_widget_showraise (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  
  gtk_widget_show (widget);
  if (GTK_WIDGET_REALIZED (widget))
    gdk_window_raise (widget->window);
}

void
gtk_widget_make_sensitive (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  
  gtk_widget_set_sensitive (widget, TRUE);
}

void
gtk_widget_make_insensitive (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  
  gtk_widget_set_sensitive (widget, FALSE);
}

void
gtk_toplevel_hide (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  
  widget = gtk_widget_get_toplevel (widget);
  gtk_widget_hide (widget);
}

void
gtk_toplevel_delete (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  /* this function is usefull to produce the exact same effect
   * as if the user caused window manager triggered window
   * deletion.
   */

  widget = gtk_widget_get_toplevel (widget);
  if (GTK_IS_WINDOW (widget) && GTK_WIDGET_DRAWABLE (widget))
    {
      GdkEvent event = { 0, };

      event.any.type = GDK_DELETE;
      event.any.window = widget->window;
      event.any.send_event = TRUE;
      gdk_event_put (&event);
    }
}

void
gtk_toplevel_activate_default (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  
  widget = gtk_widget_get_toplevel (widget);
  if (GTK_IS_WINDOW (widget))
    gtk_window_activate_default (GTK_WINDOW (widget));
}

static void
requisition_to_aux_info (GtkWidget      *widget,
			 GtkRequisition *requisition)
{
  guint width = requisition->width;
  guint height = requisition->height;

  /* we don't want the requisition to get too big */
  width = MIN (width, gdk_screen_width () / 2);
  height = MIN (height, gdk_screen_height () / 2);

  gtk_widget_set_size_request (widget, width, height);
}

void
bst_widget_request_aux_info (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  g_signal_handlers_disconnect_by_func (widget, requisition_to_aux_info, NULL);
  g_signal_connect_after (widget, "size_request", G_CALLBACK (requisition_to_aux_info), NULL);
}

void
gtk_file_selection_heal (GtkFileSelection *fs)
{
  GtkWidget *main_vbox;
  GtkWidget *hbox;
  GtkWidget *any;
  
  g_return_if_fail (fs != NULL);
  g_return_if_fail (GTK_IS_FILE_SELECTION (fs));
  
  /* button placement
   */
  gtk_container_set_border_width (GTK_CONTAINER (fs), 0);
  gtk_file_selection_hide_fileop_buttons (fs);
  gtk_widget_ref (fs->main_vbox);
  gtk_container_remove (GTK_CONTAINER (fs), fs->main_vbox);
  gtk_box_set_spacing (GTK_BOX (fs->main_vbox), 0);
  gtk_container_set_border_width (GTK_CONTAINER (fs->main_vbox), 5);
  main_vbox = gtk_widget_new (GTK_TYPE_VBOX,
			      "homogeneous", FALSE,
			      "spacing", 0,
			      "border_width", 0,
			      "parent", fs,
			      "visible", TRUE,
			      NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), fs->main_vbox, TRUE, TRUE, 0);
  gtk_widget_unref (fs->main_vbox);
  gtk_widget_hide (fs->ok_button->parent);
  hbox = gtk_widget_new (GTK_TYPE_HBOX,
			 "homogeneous", TRUE,
			 "spacing", 0,
			 "border_width", 5,
			 "visible", TRUE,
			 NULL);
  gtk_box_pack_end (GTK_BOX (main_vbox), hbox, FALSE, TRUE, 0);
  gtk_widget_reparent (fs->ok_button, hbox);
  gtk_widget_reparent (fs->cancel_button, hbox);
  gtk_widget_grab_default (fs->ok_button);
  // gtk_label_set_text (GTK_LABEL (GTK_BIN (fs->ok_button)->child), "Ok");
  // gtk_label_set_text (GTK_LABEL (GTK_BIN (fs->cancel_button)->child), "Cancel");
  
  /* heal the action_area packing so we can customize children
   */
  gtk_box_set_child_packing (GTK_BOX (fs->action_area->parent),
			     fs->action_area,
			     FALSE, TRUE,
			     5, GTK_PACK_START);
  
  any = gtk_widget_new (GTK_TYPE_HSEPARATOR,
			"visible", TRUE,
			NULL);
  gtk_box_pack_end (GTK_BOX (main_vbox), any, FALSE, TRUE, 0);
  gtk_widget_grab_focus (fs->selection_entry);
}

static gint
idle_shower (GtkWidget **widget_p)
{
  GDK_THREADS_ENTER ();
  
  if (GTK_IS_WIDGET (*widget_p))
    {
      gtk_signal_disconnect_by_func (GTK_OBJECT (*widget_p),
				     GTK_SIGNAL_FUNC (gtk_widget_destroyed),
				     widget_p);
      gtk_widget_show (*widget_p);
    }
  
  g_free (widget_p);
  
  GDK_THREADS_LEAVE ();
  
  return FALSE;
}

void
gtk_idle_show_widget (GtkWidget *widget)
{
  GtkWidget **widget_p;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  widget_p = g_new (GtkWidget*, 1);

  *widget_p = widget;
  gtk_signal_connect (GTK_OBJECT (widget),
		      "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroyed),
		      widget_p);
  gtk_idle_add_priority (GTK_PRIORITY_RESIZE - 1, (GtkFunction) idle_shower, widget_p);
}

static gboolean
idle_unparent (gpointer data)
{
  GtkWidget *widget;

  GDK_THREADS_ENTER ();

  widget = GTK_WIDGET (data);
  if (widget->parent)
    gtk_container_remove (GTK_CONTAINER (widget->parent), widget);

  gtk_widget_unref (widget);

  GDK_THREADS_LEAVE ();

  return FALSE;
}

void
gtk_idle_unparent (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (widget->parent)
    {
      gtk_widget_ref (widget);
      gtk_idle_add_priority (GTK_PRIORITY_RESIZE - 1, idle_unparent, widget);
    }
}

void
gtk_last_event_coords (gint *x_root,
		       gint *y_root)
{
  GdkEvent *event = gtk_get_current_event ();
  gdouble x = 0, y = 0;

  if (event)
    gdk_event_get_root_coords (event, &x, &y);

  if (x_root)
    *x_root = x;
  if (y_root)
    *y_root = x;
}

void
gtk_clist_moveto_selection (GtkCList *clist)
{
  g_return_if_fail (GTK_IS_CLIST (clist));

  if (GTK_WIDGET_DRAWABLE (clist) && clist->selection)
    {
      gint row = GPOINTER_TO_INT (clist->selection->data);
      
      if (gtk_clist_row_is_visible (clist, row) != GTK_VISIBILITY_FULL)
	gtk_clist_moveto (clist, row, -1, 0.5, 0);
    }
}

gpointer
gtk_clist_get_selection_data (GtkCList *clist,
			      guint     index)
{
  GList *list;
  gint row;

  g_return_val_if_fail (GTK_IS_CLIST (clist), NULL);

  list = g_list_nth (clist->selection, index);
  row = list ? GPOINTER_TO_INT (list->data) : -1;

  return row >= 0 ? gtk_clist_get_row_data (clist, row) : NULL;
}


/* --- field mask --- */
static GQuark gmask_quark = 0;
typedef struct {
  GtkTooltips *tooltips;
  GtkWidget   *parent;
  GtkWidget   *prompt;
  GtkWidget   *aux1;
  GtkWidget   *aux2;		/* auto-expand */
  GtkWidget   *aux3;
  GtkWidget   *ahead;
  GtkWidget   *action;
  GtkWidget   *atail;
  gchar       *tip;
  guint	       column : 16;
  guint        expandable : 1;	/* expand action? */
  guint	       big : 1;		/* extend to left */
} GMask;
#define	GMASK_GET(o)	((GMask*) g_object_get_qdata (G_OBJECT (o), gmask_quark))

static void
gmask_destroy (gpointer data)
{
  GMask *gmask = data;

  if (gmask->tooltips)
    g_object_unref (gmask->tooltips);
  if (gmask->parent)
    g_object_unref (gmask->parent);
  if (gmask->prompt)
    g_object_unref (gmask->prompt);
  if (gmask->aux1)
    g_object_unref (gmask->aux1);
  if (gmask->aux2)
    g_object_unref (gmask->aux2);
  if (gmask->aux3)
    g_object_unref (gmask->aux3);
  if (gmask->ahead)
    g_object_unref (gmask->ahead);
  if (gmask->atail)
    g_object_unref (gmask->atail);
  g_free (gmask->tip);
  g_free (gmask);
}

static gpointer
gmask_form (GtkWidget *parent,
	    GtkWidget *action,
	    gboolean   expandable,
	    gboolean   big)
{
  GMask *gmask;

  g_return_val_if_fail (GTK_IS_TABLE (parent), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (action), NULL);

  if (!gmask_quark)
    gmask_quark = g_quark_from_static_string ("GMask");

  gmask = GMASK_GET (action);
  g_return_val_if_fail (gmask == NULL, NULL);

  gmask = g_new0 (GMask, 1);
  g_object_set_qdata_full (G_OBJECT (action), gmask_quark, gmask, gmask_destroy);
  gmask->parent = g_object_ref (parent);
  gtk_object_sink (GTK_OBJECT (parent));
  gmask->action = action;
  gmask->expandable = expandable != FALSE;
  gmask->big = big != FALSE;
  gmask->tooltips = g_object_get_data (G_OBJECT (parent), "GMask-tooltips");
  if (gmask->tooltips)
    g_object_ref (gmask->tooltips);

  return action;
}

/**
 * bst_gmask_container_create
 * @tooltips:     Tooltip widget
 * @border_width: Border width of this GUI mask
 *
 * Create the container for a new GUI field mask.
 */
GtkWidget*
bst_gmask_container_create (gpointer tooltips,
			    guint    border_width,
			    gboolean dislodge_columns)
{
  GtkWidget *container = gtk_widget_new (GTK_TYPE_TABLE,
					 "visible", TRUE,
					 "homogeneous", FALSE,
					 "n_columns", 2,
					 "border_width", border_width,
					 NULL);
  if (tooltips)
    {
      g_return_val_if_fail (GTK_IS_TOOLTIPS (tooltips), container);

      g_object_set_data_full (G_OBJECT (container), "GMask-tooltips", g_object_ref (tooltips), g_object_unref);
    }
  if (dislodge_columns)
    g_object_set_data (G_OBJECT (container), "GMask-dislodge", GUINT_TO_POINTER (TRUE));

  return container;
}

gpointer
bst_gmask_form (GtkWidget *gmask_container,
		GtkWidget *action,
		gboolean   expandable)
{
  return gmask_form (gmask_container, action, expandable, FALSE);
}

gpointer
bst_gmask_form_big (GtkWidget *gmask_container,
		    GtkWidget *action)
{
  return gmask_form (gmask_container, action, TRUE, TRUE);
}

void
bst_gmask_set_tip (gpointer     mask,
		   const gchar *tip_text)
{
  GMask *gmask;

  g_return_if_fail (GTK_IS_WIDGET (mask));
  gmask = GMASK_GET (mask);
  g_return_if_fail (gmask != NULL);

  g_free (gmask->tip);
  gmask->tip = g_strdup (tip_text);
}

void
bst_gmask_set_prompt (gpointer mask,
		      gpointer widget)
{
  GMask *gmask;

  g_return_if_fail (GTK_IS_WIDGET (mask));
  gmask = GMASK_GET (mask);
  g_return_if_fail (gmask != NULL);
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (gmask->prompt)
    g_object_unref (gmask->prompt);
  gmask->prompt = g_object_ref (widget);
  gtk_object_sink (GTK_OBJECT (widget));
}

void
bst_gmask_set_aux1 (gpointer mask,
		    gpointer widget)
{
  GMask *gmask;

  g_return_if_fail (GTK_IS_WIDGET (mask));
  gmask = GMASK_GET (mask);
  g_return_if_fail (gmask != NULL);
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (gmask->aux1)
    g_object_unref (gmask->aux1);
  gmask->aux1 = g_object_ref (widget);
  gtk_object_sink (GTK_OBJECT (widget));
}

void
bst_gmask_set_aux2 (gpointer mask,
		    gpointer widget)
{
  GMask *gmask;

  g_return_if_fail (GTK_IS_WIDGET (mask));
  gmask = GMASK_GET (mask);
  g_return_if_fail (gmask != NULL);
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (gmask->aux2)
    g_object_unref (gmask->aux2);
  gmask->aux2 = g_object_ref (widget);
  gtk_object_sink (GTK_OBJECT (widget));
}

void
bst_gmask_set_aux3 (gpointer mask,
		    gpointer widget)
{
  GMask *gmask;

  g_return_if_fail (GTK_IS_WIDGET (mask));
  gmask = GMASK_GET (mask);
  g_return_if_fail (gmask != NULL);
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (gmask->aux3)
    g_object_unref (gmask->aux3);
  gmask->aux3 = g_object_ref (widget);
  gtk_object_sink (GTK_OBJECT (widget));
}

void
bst_gmask_set_ahead (gpointer mask,
		     gpointer widget)
{
  GMask *gmask;

  g_return_if_fail (GTK_IS_WIDGET (mask));
  gmask = GMASK_GET (mask);
  g_return_if_fail (gmask != NULL);
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (gmask->ahead)
    g_object_unref (gmask->ahead);
  gmask->ahead = g_object_ref (widget);
  gtk_object_sink (GTK_OBJECT (widget));
}

void
bst_gmask_set_atail (gpointer mask,
		     gpointer widget)
{
  GMask *gmask;

  g_return_if_fail (GTK_IS_WIDGET (mask));
  gmask = GMASK_GET (mask);
  g_return_if_fail (gmask != NULL);
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (gmask->atail)
    g_object_unref (gmask->atail);
  gmask->atail = g_object_ref (widget);
  gtk_object_sink (GTK_OBJECT (widget));
}

void
bst_gmask_set_column (gpointer mask,
		      guint    column)
{
  GMask *gmask;

  g_return_if_fail (GTK_IS_WIDGET (mask));
  gmask = GMASK_GET (mask);
  g_return_if_fail (gmask != NULL);

  gmask->column = column;
}

GtkWidget*
bst_gmask_get_prompt (gpointer mask)
{
  GMask *gmask;

  g_return_val_if_fail (GTK_IS_WIDGET (mask), NULL);
  gmask = GMASK_GET (mask);
  g_return_val_if_fail (gmask != NULL, NULL);

  return gmask->prompt;
}

GtkWidget*
bst_gmask_get_aux1 (gpointer mask)
{
  GMask *gmask;

  g_return_val_if_fail (GTK_IS_WIDGET (mask), NULL);
  gmask = GMASK_GET (mask);
  g_return_val_if_fail (gmask != NULL, NULL);

  return gmask->aux1;
}

GtkWidget*
bst_gmask_get_aux2 (gpointer mask)
{
  GMask *gmask;

  g_return_val_if_fail (GTK_IS_WIDGET (mask), NULL);
  gmask = GMASK_GET (mask);
  g_return_val_if_fail (gmask != NULL, NULL);

  return gmask->aux2;
}

GtkWidget*
bst_gmask_get_aux3 (gpointer mask)
{
  GMask *gmask;

  g_return_val_if_fail (GTK_IS_WIDGET (mask), NULL);
  gmask = GMASK_GET (mask);
  g_return_val_if_fail (gmask != NULL, NULL);

  return gmask->aux3;
}

GtkWidget*
bst_gmask_get_ahead (gpointer mask)
{
  GMask *gmask;

  g_return_val_if_fail (GTK_IS_WIDGET (mask), NULL);
  gmask = GMASK_GET (mask);
  g_return_val_if_fail (gmask != NULL, NULL);

  return gmask->ahead;
}

GtkWidget*
bst_gmask_get_action (gpointer mask)
{
  GMask *gmask;

  g_return_val_if_fail (GTK_IS_WIDGET (mask), NULL);
  gmask = GMASK_GET (mask);
  g_return_val_if_fail (gmask != NULL, NULL);

  return gmask->action;
}

GtkWidget*
bst_gmask_get_atail (gpointer mask)
{
  GMask *gmask;

  g_return_val_if_fail (GTK_IS_WIDGET (mask), NULL);
  gmask = GMASK_GET (mask);
  g_return_val_if_fail (gmask != NULL, NULL);

  return gmask->atail;
}

void
bst_gmask_foreach (gpointer mask,
		   gpointer func,
		   gpointer data)
{
  GMask *gmask;
  GtkCallback callback = func;

  g_return_if_fail (GTK_IS_WIDGET (mask));
  gmask = GMASK_GET (mask);
  g_return_if_fail (gmask != NULL);
  g_return_if_fail (func != NULL);

  if (gmask->prompt)
    callback (gmask->prompt, data);
  if (gmask->aux1)
    callback (gmask->aux1, data);
  if (gmask->aux2)
    callback (gmask->aux2, data);
  if (gmask->aux3)
    callback (gmask->aux3, data);
  if (gmask->ahead)
    callback (gmask->ahead, data);
  if (gmask->atail)
    callback (gmask->atail, data);
  if (gmask->action)
    callback (gmask->action, data);
}

static GtkWidget*
get_toplevel_and_set_tip (GtkWidget   *widget,
			  GtkTooltips *tooltips,
			  const gchar *tip)
{
  GtkWidget *last;

  if (!widget)
    return NULL;
  else if (!tooltips || !tip)
    return gtk_widget_get_toplevel (widget);
  do
    {
      if (!GTK_WIDGET_NO_WINDOW (widget))
	{
	  gtk_tooltips_set_tip (tooltips, widget, tip, NULL);
	  return gtk_widget_get_toplevel (widget);
	}
      last = widget;
      widget = last->parent;
    }
  while (widget);
  /* need to create a tooltips sensitive parent */
  widget = gtk_widget_new (GTK_TYPE_EVENT_BOX,
			   "visible", TRUE,
			   "child", last,
			   NULL);
  gtk_tooltips_set_tip (tooltips, widget, tip, NULL);
  return widget;
}

static guint
table_max_bottom_row (GtkTable *table,
		      guint     min_col,
		      guint	max_col)
{
  guint max_bottom = 0;
  GList *list;

  for (list = table->children; list; list = list->next)
    {
      GtkTableChild *child = list->data;

      if (child->left_attach >= min_col && child->right_attach <= max_col)
	max_bottom = MAX (max_bottom, child->bottom_attach);
    }
  return max_bottom;
}

/* GUI mask layout:
 * row: |Prompt|Aux1| Aux2 |Aux3| PreAction#Action#PostAction|
 * expandable: expand Action to left, up to Aux3
 * big row: expand Action to left as far as possible (if Prompt/Aux? are omitted)
 * Aux2 expands automatically
 */
void
bst_gmask_pack (gpointer mask)
{
  GtkWidget *prompt, *aux1, *aux2, *aux3, *ahead, *action, *atail;
  GtkTable *table;
  gboolean dummy_aux2 = FALSE;
  guint row, n, c, dislodge_columns;
  GMask *gmask;

  g_return_if_fail (GTK_IS_WIDGET (mask));
  gmask = GMASK_GET (mask);
  g_return_if_fail (gmask != NULL);

  /* retrive children and set tips */
  prompt = get_toplevel_and_set_tip (gmask->prompt, gmask->tooltips, gmask->tip);
  aux1 = get_toplevel_and_set_tip (gmask->aux1, gmask->tooltips, gmask->tip);
  aux2 = get_toplevel_and_set_tip (gmask->aux2, gmask->tooltips, gmask->tip);
  aux3 = get_toplevel_and_set_tip (gmask->aux3, gmask->tooltips, gmask->tip);
  ahead = get_toplevel_and_set_tip (gmask->ahead, gmask->tooltips, gmask->tip);
  action = get_toplevel_and_set_tip (gmask->action, gmask->tooltips, gmask->tip);
  atail = get_toplevel_and_set_tip (gmask->atail, gmask->tooltips, gmask->tip);
  dislodge_columns = g_object_get_data (G_OBJECT (gmask->parent), "GMask-dislodge") != NULL;
  table = GTK_TABLE (gmask->parent);

  /* ensure expansion happens outside of columns */
  if (dislodge_columns)
    {
      gchar *dummy_name = g_strdup_printf ("GMask-dummy-dislodge-%u", MAX (gmask->column, 1) - 1);
      GtkWidget *dislodge = g_object_get_data (G_OBJECT (table), dummy_name);

      if (!dislodge)
	{
	  dislodge = gtk_widget_new (GTK_TYPE_ALIGNMENT, "visible", TRUE, NULL);
	  g_object_set_data_full (G_OBJECT (table), dummy_name, g_object_ref (dislodge), g_object_unref);
	  c = MAX (gmask->column, 1) * 6;
	  gtk_table_attach (table, dislodge, c - 1, c, 0, 1, GTK_EXPAND, 0, 0, 0);
	}
      g_free (dummy_name);
    }

  /* pack gmask children, options: GTK_EXPAND, GTK_SHRINK, GTK_FILL */
  c = 6 * gmask->column;
  row = table_max_bottom_row (table, c, c + 5);
  if (prompt)
    {
      gtk_table_attach (table, prompt, c, c + 1, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
      gtk_table_set_col_spacing (table, c, 2); /* seperate prompt from rest */
    }
  c++;
  if (aux1)
    gtk_table_attach (table, aux1, c, c + 1, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
  c++;
  if (!aux2 && !dislodge_columns)
    {
      gchar *dummy_name = g_strdup_printf ("GMask-dummy-aux2-%u", gmask->column);

      aux2 = g_object_get_data (G_OBJECT (table), dummy_name);
      
      /* need to have at least 1 (dummy) aux2-child per table column to eat up
       * expanding space in this column if !dislodge_columns
       */
      if (!aux2)
	{
	  aux2 = gtk_widget_new (GTK_TYPE_ALIGNMENT, "visible", TRUE, NULL);
	  g_object_set_data_full (G_OBJECT (table), dummy_name, g_object_ref (aux2), g_object_unref);
	}
      else
	aux2 = NULL;
      g_free (dummy_name);
      dummy_aux2 = TRUE;
    }
  if (aux2)
    {
      gtk_table_attach (table, aux2,
			c, c + 1,
			row, row + 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);
      if (dummy_aux2)
	aux2 = NULL;
    }
  c++;
  if (aux3)
    gtk_table_attach (table, aux3, c, c + 1, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
  c++;
  /* pack action with head and tail widgets closely together */
  if (ahead || atail)
    {
      action = gtk_widget_new (GTK_TYPE_HBOX,
			       "visible", TRUE,
			       "child", action,
			       NULL);
      if (ahead)
        gtk_container_add_with_properties (GTK_CONTAINER (action), ahead,
					   "position", 0,
					   "expand", FALSE,
					   NULL);
      if (atail)
	gtk_box_pack_end (GTK_BOX (action), atail, FALSE, TRUE, 0);
    }
  n = c;
  if (gmask->big && !aux3) /* extend action to the left when possible */
    {
      n--;
      if (!aux2)
	{
	  n--;
	  if (!aux1)
	    {
	      n--;
	      if (!prompt)
		n--;
	    }
	}
    }
  if (!gmask->expandable) /* align to right without expansion if desired */
    action = gtk_widget_new (GTK_TYPE_ALIGNMENT,
			     "visible", TRUE,
			     "child", action,
			     "xalign", 1.0,
			     "xscale", 0.0,
			     "yscale", 0.0,
			     NULL);
  gtk_table_attach (table, action,
		    n, c + 1, row, row + 1,
		    GTK_SHRINK | GTK_FILL | (gmask->expandable ? 0 /* GTK_EXPAND */ : 0),
		    GTK_FILL,
		    0, 0);
  gtk_table_set_col_spacing (table, c - 1, 2); /* seperate action from rest */
  c = 6 * gmask->column;
  if (c)
    gtk_table_set_col_spacing (table, c - 1, 5); /* spacing between columns */
}

gpointer
bst_gmask_quick (GtkWidget   *gmask_container,
		 guint	      column,
		 const gchar *prompt,
		 gpointer     action_widget,
		 const gchar *tip_text)
{
  gpointer mask = bst_gmask_form (gmask_container, action_widget, TRUE);
  
  if (prompt)
    bst_gmask_set_prompt (mask, g_object_new (GTK_TYPE_LABEL,
					      "visible", TRUE,
					      "label", prompt,
					      NULL));
  if (tip_text)
    bst_gmask_set_tip (mask, tip_text);
  bst_gmask_set_column (mask, column);
  bst_gmask_pack (mask);

  return mask;
}


/* --- BEAST utilities --- */
static void
style_modify_fg_as_sensitive (GtkWidget *widget)
{
  GtkRcStyle *rc_style = gtk_rc_style_new ();
  
  rc_style->color_flags[GTK_STATE_INSENSITIVE] = GTK_RC_FG;
  rc_style->fg[GTK_STATE_INSENSITIVE].red = widget->style->fg[GTK_STATE_NORMAL].red;
  rc_style->fg[GTK_STATE_INSENSITIVE].green = widget->style->fg[GTK_STATE_NORMAL].green;
  rc_style->fg[GTK_STATE_INSENSITIVE].blue = widget->style->fg[GTK_STATE_NORMAL].blue;
  gtk_widget_modify_style (widget, rc_style);
}

void
bst_widget_modify_as_title (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  
  gtk_widget_set_sensitive (widget, FALSE);
  if (GTK_WIDGET_REALIZED (widget))
    style_modify_fg_as_sensitive (widget);
  if (!gtk_signal_handler_pending_by_func (GTK_OBJECT (widget),
					   gtk_signal_lookup ("realize", GTK_TYPE_WIDGET),
					   TRUE,
					   style_modify_fg_as_sensitive,
					   NULL))
    gtk_signal_connect_after (GTK_OBJECT (widget), "realize", GTK_SIGNAL_FUNC (style_modify_fg_as_sensitive), NULL);
  if (!gtk_signal_handler_pending_by_func (GTK_OBJECT (widget),
					   gtk_signal_lookup ("realize", GTK_TYPE_WIDGET),
					   TRUE,
					   gtk_widget_make_insensitive,
					   NULL))
    gtk_signal_connect_after (GTK_OBJECT (widget), "realize", GTK_SIGNAL_FUNC (gtk_widget_make_insensitive), NULL);
}

static void
style_modify_bg_as_base (GtkWidget *widget)
{
  GtkRcStyle *rc_style = gtk_widget_get_modifier_style (widget);
  
  rc_style->color_flags[GTK_STATE_NORMAL] = GTK_RC_BG;
  rc_style->bg[GTK_STATE_NORMAL].red = widget->style->base[GTK_STATE_NORMAL].red;
  rc_style->bg[GTK_STATE_NORMAL].green = widget->style->base[GTK_STATE_NORMAL].green;
  rc_style->bg[GTK_STATE_NORMAL].blue = widget->style->base[GTK_STATE_NORMAL].blue;
  gtk_widget_modify_style (widget, rc_style);
}

void
bst_widget_modify_bg_as_base (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  
  if (GTK_WIDGET_REALIZED (widget))
    style_modify_bg_as_base (widget);
  if (!gtk_signal_handler_pending_by_func (GTK_OBJECT (widget),
					   gtk_signal_lookup ("realize", GTK_TYPE_WIDGET),
					   TRUE,
					   style_modify_bg_as_base,
					   NULL))
    gtk_signal_connect_after (GTK_OBJECT (widget), "realize", GTK_SIGNAL_FUNC (style_modify_bg_as_base), NULL);
}

GtkWidget*
bst_text_view_from (GString     *gstring,
		    const gchar *file_name,
		    const gchar *font_name)
{
  GtkWidget *hbox, *text, *sb;
  
  hbox = gtk_widget_new (GTK_TYPE_HBOX,
			 "visible", TRUE,
			 "homogeneous", FALSE,
			 "spacing", 0,
			 "border_width", 5,
			 NULL);
  sb = gtk_vscrollbar_new (NULL);
  gtk_widget_show (sb);
  gtk_box_pack_end (GTK_BOX (hbox), sb, FALSE, TRUE, 0);
  text = gtk_widget_new (GTK_TYPE_TEXT,
			 "visible", TRUE,
			 "vadjustment", GTK_RANGE (sb)->adjustment,
			 "editable", FALSE,
			 "word_wrap", TRUE,
			 "line_wrap", FALSE,
			 "width_request", 500,
			 "height_request", 500,
			 "parent", hbox,
			 NULL);

  if (font_name)
    {
      PangoFontDescription *pfdesc = pango_font_description_from_string (font_name);

      gtk_widget_modify_font (text, pfdesc);
      pango_font_description_free (pfdesc);
    }

  if (gstring)
    gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL, gstring->str, gstring->len);
  
  if (file_name)
    {
      gint fd;
      
      fd = open (file_name, O_RDONLY, 0);
      if (fd >= 0)
	{
	  gchar buffer[512];
	  guint n;
	  
	  do
	    {
	      do
		n = read (fd, buffer, 512);
	      while (n < 0 && errno == EINTR); /* don't mind signals */
	      
	      gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL, buffer, n);
	    }
	  while (n > 0);
	  close (fd);
	  
	  if (n < 0)
	    fd = -1;
	}
      if (fd < 0)
	{
	  gchar *error;
	  
	  error = g_strconcat ("Failed to load \"", file_name, "\":\n", g_strerror (errno), NULL);
	  gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL, error, strlen (error));
	  g_free (error);
	}
    }
  
  return hbox;
}

static void
style_modify_base_as_bg (GtkWidget *widget)
{
  GtkRcStyle *rc_style = gtk_rc_style_new ();
  
  rc_style->color_flags[GTK_STATE_NORMAL] = GTK_RC_BASE;
  rc_style->base[GTK_STATE_NORMAL].red = widget->style->bg[GTK_STATE_NORMAL].red;
  rc_style->base[GTK_STATE_NORMAL].green = widget->style->bg[GTK_STATE_NORMAL].green;
  rc_style->base[GTK_STATE_NORMAL].blue = widget->style->bg[GTK_STATE_NORMAL].blue;
  gtk_widget_modify_style (widget, rc_style);
}

void
bst_widget_modify_base_as_bg (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (GTK_WIDGET_REALIZED (widget))
    style_modify_base_as_bg (widget);
  if (!gtk_signal_handler_pending_by_func (GTK_OBJECT (widget),
					   gtk_signal_lookup ("realize", GTK_TYPE_WIDGET),
					   TRUE,
					   style_modify_base_as_bg,
					   NULL))
    gtk_signal_connect_after (GTK_OBJECT (widget), "realize", GTK_SIGNAL_FUNC (style_modify_base_as_bg), NULL);
}

#if 0
static void
tweak_text_resize (GtkText *text)
{
  GtkAllocation *allocation = &GTK_WIDGET (text)->allocation;
  
  if (text->text_area)
    gdk_window_move_resize (text->text_area,
			    0, 0,
			    allocation->width,
			    allocation->height);
  gtk_adjustment_set_value (text->vadj, 0);
}

GtkWidget*
old_bst_wrap_text_create (gboolean     duplicate_newlines,
			  const gchar *string)
{
  GtkWidget *text;
  
  text = g_object_connect (g_object_new (GTK_TYPE_TEXT,
					 "visible", TRUE,
					 "editable", FALSE,
					 "word_wrap", TRUE,
					 "line_wrap", TRUE,
					 "can_focus", FALSE,
					 NULL),
			   "signal_after::realize", tweak_text_resize, NULL,
			   "signal_after::size_allocate", tweak_text_resize, NULL,
			   NULL);
  bst_widget_modify_base_as_bg (text);
  if (duplicate_newlines)
    g_object_set_data (G_OBJECT (text), "duplicate_newlines", GUINT_TO_POINTER (TRUE));
  bst_wrap_text_set (text, string);
  
  return text;
}

void
old_bst_wrap_text_clear (GtkWidget *text)
{
  g_return_if_fail (GTK_IS_TEXT (text));

  gtk_editable_delete_text (GTK_EDITABLE (text), 0, -1);
  gtk_adjustment_set_value (GTK_TEXT (text)->vadj, 0);
}

void
old_bst_wrap_text_set (GtkWidget   *text,
		   const gchar *string)
{
  g_return_if_fail (GTK_IS_TEXT (text));

  bst_wrap_text_clear (text);
  if (string)
    {
      GString *gstring = g_string_new (string);
      
      if (g_object_get_data (G_OBJECT (text), "duplicate_newlines") != NULL)
	{
	  gint i;
	  
	  for (i = 0; i < gstring->len; i++)
	    if (gstring->str[i] == '\n')
	      g_string_insert_c (gstring, i++, '\n');
	}
      gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL, gstring->str, gstring->len);
      g_string_free (gstring, TRUE);
    }
  gtk_adjustment_set_value (GTK_TEXT (text)->vadj, 0);
}

void
old_bst_wrap_text_aprintf (GtkWidget   *text,
		       const gchar *text_fmt,
		       ...)
{
  g_return_if_fail (GTK_IS_TEXT (text));

  if (text_fmt)
    {
      va_list args;
      gchar *buffer;
      
      va_start (args, text_fmt);
      buffer = g_strdup_vprintf (text_fmt, args);
      va_end (args);

      bst_wrap_text_append (text, buffer);
      g_free (buffer);
    }
}

void
old_bst_wrap_text_push_indent (GtkWidget   *text,
			   const gchar *spaces)
{
  bst_wrap_text_append (text, spaces);
}

void
old_bst_wrap_text_pop_indent (GtkWidget *text)
{
}

void
old_bst_wrap_text_append (GtkWidget   *text,
		      const gchar *string)
{
  g_return_if_fail (GTK_IS_TEXT (text));
  
  if (string)
    {
      GString *gstring = g_string_new (string);
      
      if (g_object_get_data (G_OBJECT (text), "duplicate_newlines") != NULL)
	{
	  gint i;
	  
	  for (i = 0; i < gstring->len; i++)
	    if (gstring->str[i] == '\n')
	      g_string_insert_c (gstring, i++, '\n');
	}
      gtk_text_set_point (GTK_TEXT (text), gtk_text_get_length (GTK_TEXT (text)));
      gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL, gstring->str, gstring->len);
      g_string_free (gstring, TRUE);
    }
  gtk_adjustment_set_value (GTK_TEXT (text)->vadj, 0);
}
#endif

static void
text_view_append (GtkTextView *view,
		  guint        indent,
		  const gchar *string)
{
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (view);
  GtkTextMark *mark = gtk_text_buffer_get_mark (buffer, "imark");
  GtkTextIter iter;

  gtk_text_buffer_get_end_iter (buffer, &iter);
  gtk_text_buffer_move_mark (buffer, mark, &iter);
  gtk_text_buffer_insert (buffer, &iter, string, strlen (string));
  if (TRUE)
    {
      GtkTextTagTable *table = gtk_text_buffer_get_tag_table (buffer);
      gchar *name = g_strdup_printf ("indent-%u", indent);
      GtkTextTag *tag = gtk_text_tag_table_lookup (table, name);
      GtkTextIter miter;
      const guint left_margin = 3, right_margin = 3;

      if (!tag)
	{
	  tag = g_object_new (GTK_TYPE_TEXT_TAG,
			      "name", name,
			      "left_margin", left_margin + indent * 8,
			      "right_margin", right_margin,
			      NULL);
	  gtk_text_tag_table_add (table, tag);
	  g_object_unref (tag);
	}
      g_free (name);
      gtk_text_buffer_get_iter_at_mark (buffer, &miter, mark);
      gtk_text_buffer_apply_tag (buffer, tag, &miter, &iter);
    }

  // gtk_text_buffer_get_start_iter (buffer, &iter);
  gtk_text_view_get_iter_at_location (view, &iter, 0, 0);
  gtk_text_buffer_move_mark (buffer, mark, &iter);

  gtk_text_view_scroll_to_mark (view, mark, 0, TRUE, 0, 0);
}

GtkWidget*
bst_wrap_text_create (gboolean     junk,
		      const gchar *string)
{
  GtkWidget *tview, *text_view;
  GtkTextBuffer *buffer;
  GtkTextIter iter;
  
  tview = g_object_new (GTK_TYPE_SCROLLED_WINDOW,
			"visible", TRUE,
			"hscrollbar_policy", GTK_POLICY_AUTOMATIC,
			"vscrollbar_policy", GTK_POLICY_AUTOMATIC,
			NULL);
  text_view = g_object_new (GTK_TYPE_TEXT_VIEW,
			    "visible", TRUE,
			    "editable", FALSE,
			    "cursor_visible", FALSE,
			    "wrap_mode", GTK_WRAP_WORD,
			    "parent", tview,
			    NULL);
  bst_widget_modify_base_as_bg (text_view);

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
  gtk_text_buffer_get_start_iter (buffer, &iter);
  gtk_text_buffer_create_mark (buffer, "imark", &iter, TRUE);

  if (string)
    bst_wrap_text_append (tview, string);

  return tview;
}

void
bst_wrap_text_clear (GtkWidget *tview)
{
  GtkTextView *view;
  GtkTextBuffer *buffer;
  GtkTextIter iter1, iter2;
  
  g_return_if_fail (GTK_IS_SCROLLED_WINDOW (tview));

  view = GTK_TEXT_VIEW (GTK_BIN (tview)->child);
  buffer = gtk_text_view_get_buffer (view);

  gtk_text_buffer_get_start_iter (buffer, &iter1);
  gtk_text_buffer_get_end_iter (buffer, &iter2);
  gtk_text_buffer_delete (buffer, &iter1, &iter2);
  g_object_set_int (view, "indent", 0);
}

void
bst_wrap_text_set (GtkWidget   *tview,
		   const gchar *string)
{
  g_return_if_fail (GTK_IS_SCROLLED_WINDOW (tview));

  bst_wrap_text_clear (tview);
  if (string)
    bst_wrap_text_append (tview, string);
}

void
bst_wrap_text_append (GtkWidget   *tview,
		      const gchar *string)
{
  GtkTextView *view;

  g_return_if_fail (GTK_IS_SCROLLED_WINDOW (tview));

  view = GTK_TEXT_VIEW (GTK_BIN (tview)->child);
  if (string)
    text_view_append (view, g_object_get_int (view, "indent"), string);
}

void
bst_wrap_text_aprintf (GtkWidget   *tview,
		       const gchar *text_fmt,
		       ...)
{
  g_return_if_fail (GTK_IS_SCROLLED_WINDOW (tview));

  if (text_fmt)
    {
      va_list args;
      gchar *buffer;
      
      va_start (args, text_fmt);
      buffer = g_strdup_vprintf (text_fmt, args);
      va_end (args);

      bst_wrap_text_append (tview, buffer);
      g_free (buffer);
    }
}

void
bst_wrap_text_push_indent (GtkWidget   *tview,
			   const gchar *spaces)
{
  GtkTextView *view;
  guint indent;

  g_return_if_fail (GTK_IS_SCROLLED_WINDOW (tview));

  view = GTK_TEXT_VIEW (GTK_BIN (tview)->child);
  indent = g_object_get_int (view, "indent");
  g_object_set_int (view, "indent", indent + 2);
}

void
bst_wrap_text_pop_indent (GtkWidget *tview)
{
  GtkTextView *view;
  guint indent;

  g_return_if_fail (GTK_IS_SCROLLED_WINDOW (tview));

  view = GTK_TEXT_VIEW (GTK_BIN (tview)->child);
  indent = g_object_get_int (view, "indent");
  if (indent)
    g_object_set_int (view, "indent", indent - 2);
}

void
g_object_set_int (gpointer     object,
		  const gchar *name,
		  glong        v_int)
{
  g_return_if_fail (G_IS_OBJECT (object));

  g_object_set_data (object, name, (gpointer) v_int);
}

glong
g_object_get_int (gpointer     object,
		  const gchar *name)
{
  g_return_val_if_fail (G_IS_OBJECT (object), 0);

  return (glong) g_object_get_data (object, name);
}

guint
bst_container_get_insertion_position (GtkContainer   *container,
				      gboolean        scan_horizontally,
				      gint            xy,	/* relative to container->allocation */
				      GtkWidget      *ignore_child,
				      gint           *ignore_child_position)
{
  GList *list, *children;
  GtkWidget *widget;
  guint position = 0;

  g_return_val_if_fail (GTK_IS_CONTAINER (container), -1);

  widget = GTK_WIDGET (container);

  if (ignore_child_position)
    *ignore_child_position = -1;

  if (GTK_WIDGET_NO_WINDOW (container))
    xy += scan_horizontally ? widget->allocation.x : widget->allocation.y;

  children = gtk_container_children (container);
  for (list = children; list; list = list->next)
    {
      GtkWidget *child = list->data;

      if (child == ignore_child)
	{
	  if (ignore_child_position)
	    *ignore_child_position = position;
	  continue;
	}
#if 0
      if (!GTK_WIDGET_VISIBLE (child))
        continue;
#endif
      if (scan_horizontally && xy < child->allocation.x + child->allocation.width / 2)
	break;
      else if (!scan_horizontally && xy < child->allocation.y + child->allocation.height / 2)
	break;
      position++;
    }
  g_list_free (children);

  return position;
}


/* --- named children --- */
static GQuark quark_container_named_children = 0;
typedef struct {
  GData *qdata;
} NChildren;
static void
nchildren_free (gpointer data)
{
  NChildren *children = data;
  
  g_datalist_clear (&children->qdata);
  g_free (children);
}
static void
destroy_nchildren (GtkWidget *container)
{
  g_object_set_qdata (G_OBJECT (container), quark_container_named_children, NULL);
}
void
bst_container_set_named_child (GtkWidget *container,
			       GQuark     qname,
			       GtkWidget *child)
{
  NChildren *children;

  g_return_if_fail (GTK_IS_CONTAINER (container));
  g_return_if_fail (qname > 0);
  g_return_if_fail (GTK_IS_WIDGET (child));
  if (child)
    g_return_if_fail (gtk_widget_is_ancestor (child, container));

  if (!quark_container_named_children)
    quark_container_named_children = g_quark_from_static_string ("BstContainer-named_children");

  children = g_object_get_qdata (G_OBJECT (container), quark_container_named_children);
  if (!children)
    {
      children = g_new (NChildren, 1);
      g_datalist_init (&children->qdata);
      g_object_set_qdata_full (G_OBJECT (container), quark_container_named_children, children, nchildren_free);
      g_object_connect (container,
			"signal::destroy", destroy_nchildren, NULL,
			NULL);
    }
  g_object_ref (child);
  g_datalist_id_set_data_full (&children->qdata, qname, child, g_object_unref);
}

GtkWidget*
bst_container_get_named_child (GtkWidget *container,
			       GQuark     qname)
{
  NChildren *children;
  
  g_return_val_if_fail (GTK_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (qname > 0, NULL);

  children = quark_container_named_children ? g_object_get_qdata (G_OBJECT (container), quark_container_named_children) : NULL;
  if (children)
    {
      GtkWidget *child = g_datalist_id_get_data (&children->qdata, qname);

      if (child && !gtk_widget_is_ancestor (child, container))
	{
	  /* got removed meanwhile */
	  g_datalist_id_set_data (&children->qdata, qname, NULL);
	  child = NULL;
	}
      return child;
    }
  return NULL;
}

GtkWidget*
bst_xpm_view_create (const gchar **xpm,
		     GtkWidget    *colormap_widget)
{
  GtkWidget *pix;
  GdkPixmap *pixmap;
  GdkBitmap *mask;

  g_return_val_if_fail (xpm != NULL, NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (colormap_widget), NULL);

  pixmap = gdk_pixmap_colormap_create_from_xpm_d (NULL, gtk_widget_get_colormap (colormap_widget),
						  &mask, NULL, (gchar**) xpm);
  pix = gtk_pixmap_new (pixmap, mask);
  gdk_pixmap_unref (pixmap);
  gdk_pixmap_unref (mask);
  gtk_widget_set (pix,
		  "visible", TRUE,
		  NULL);
  return pix;
}

static gboolean
expose_bg_clear (GtkWidget      *widget,
		 GdkEventExpose *event)
{
  gtk_paint_flat_box (widget->style, widget->window, GTK_STATE_NORMAL,
		      GTK_SHADOW_NONE, &event->area, widget, "base", 0, 0, -1, -1);
  
  return FALSE;
}

void
bst_widget_force_bg_clear (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  
  gtk_widget_set_redraw_on_allocate (widget, TRUE);
  if (!g_signal_handler_find (widget, G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL, expose_bg_clear, NULL))
    g_object_connect (widget, "signal::expose_event", expose_bg_clear, NULL, NULL);
}


/* --- Canvas Utilities & Workarounds --- */
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
  while (item && !gtk_type_is_a (GTK_OBJECT_TYPE (item), item_type))
    item = item->parent;
  
  return item && gtk_type_is_a (GTK_OBJECT_TYPE (item), item_type) ? item : NULL;
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
