/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2002 Tim Janik and Red Hat, Inc.
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
#include        "bstutils.h"

#include        "bstmenus.h"
#include        <fcntl.h>
#include        <errno.h>
#include        <unistd.h>
#include        <string.h>


/* --- generated enums --- */
#include "bstenum_arrays.c"	/* enum string value arrays plus include directives */


/* --- prototypes --- */
static void	_bst_init_idl			(void);
static void	traverse_viewable_changed	(GtkWidget	*widget,
						 gpointer	 data);


/* --- variables --- */
static gulong viewable_changed_id = 0;


/* --- functions --- */
void
_bst_init_utils (void)
{
  static guint initialized = 0;
  
  g_assert (initialized++ == 0);

  /* Gtk+ patchups */
  viewable_changed_id = g_signal_newv ("viewable-changed",
				       G_TYPE_FROM_CLASS (gtk_type_class (GTK_TYPE_WIDGET)),
				       G_SIGNAL_RUN_LAST,
				       g_cclosure_new (G_CALLBACK (traverse_viewable_changed), NULL, NULL),
				       NULL, NULL,
				       gtk_marshal_VOID__VOID,
				       G_TYPE_NONE, 0, NULL);

  /* initialize generated type ids */
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

  /* initialize IDL types */
  _bst_init_idl ();

  /* initialize stock icons (included above) */
  {
    /* generated stock icons */
#include "./icons/bst-stock-gen.c"

    gxk_stock_register_icons (G_N_ELEMENTS (stock_icons), stock_icons);
  }

  /* initialize stock actions */
  {
    static const GxkStockAction stock_actions[] = {
      { BST_STOCK_CLONE,		"_Clone",	GTK_STOCK_COPY,			},
      { BST_STOCK_DEFAULT_REVERT,	"_Defaults",	GTK_STOCK_UNDO,			},
      { BST_STOCK_LOAD,			"_Load",	NULL,				},
      { BST_STOCK_OVERWRITE,		"_Overwrite",	GTK_STOCK_SAVE,			},
      { BST_STOCK_REVERT,		"_Revert",	GTK_STOCK_UNDO,			},
    };
    gxk_stock_register_actions (G_N_ELEMENTS (stock_actions), stock_actions);
  }
}

GtkWidget*
bst_image_from_icon (BseIcon    *icon,
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

  icon = bse_icon_copy_shallow (icon);
  pixbuf = gdk_pixbuf_new_from_data (icon->pixels->bytes, GDK_COLORSPACE_RGB, icon->bytes_per_pixel == 4,
				     8, icon->width, icon->height,
				     icon->width * icon->bytes_per_pixel,
				     NULL, NULL);
  g_object_set_data_full (G_OBJECT (pixbuf), "BseIcon", icon, (GtkDestroyNotify) bse_icon_free);

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


/* --- beast/bsw specific extensions --- */
void
bst_status_eprintf (BseErrorType error,
		    const gchar *message_fmt,
		    ...)
{
  gchar *buffer;
  va_list args;
  
  va_start (args, message_fmt);
  buffer = g_strdup_vprintf (message_fmt, args);
  va_end (args);
  
  if (error)
    gxk_status_set (GXK_STATUS_ERROR, buffer, bse_error_blurb (error));
  else
    gxk_status_set (GXK_STATUS_DONE, buffer, NULL);
  g_free (buffer);
}

typedef struct {
  GtkWindow *window;
  SfiProxy   proxy;
  gchar     *title1;
  gchar     *title2;
} TitleSync;

static void
sync_title (TitleSync *tsync)
{
  const gchar *name = bse_item_get_name (tsync->proxy);
  gchar *s;

  s = g_strconcat (tsync->title1, name ? name : "<NULL>", tsync->title2, NULL);
  g_object_set (tsync->window, "title", s, NULL);
  g_free (s);
}

static void
free_title_sync (gpointer data)
{
  TitleSync *tsync = data;

  bse_proxy_disconnect (tsync->proxy,
			"any_signal", sync_title, tsync,
			NULL);
  g_free (tsync->title1);
  g_free (tsync->title2);
  g_free (tsync);
}

void
bst_window_sync_title_to_proxy (gpointer     window,
				SfiProxy     proxy,
				const gchar *title_format)
{
  g_return_if_fail (GTK_IS_WINDOW (window));
  if (proxy)
    {
      g_return_if_fail (BSE_IS_ITEM (proxy));
      g_return_if_fail (title_format != NULL);
      g_return_if_fail (strstr (title_format, "%s") != NULL);
    }

  if (proxy)
    {
      TitleSync *tsync = g_new0 (TitleSync, 1);
      gchar *p = strstr (title_format, "%s");

      tsync->window = window;
      tsync->proxy = proxy;
      tsync->title1 = g_strndup (title_format, p - title_format);
      tsync->title2 = g_strdup (p + 2);
      bse_proxy_connect (tsync->proxy,
			 "swapped_signal::property-notify::uname", sync_title, tsync,
			 NULL);
      g_object_set_data_full (window, "bst-title-sync", tsync, free_title_sync);
      sync_title (tsync);
    }
  else
    {
      if (g_object_get_data (window, "bst-title-sync"))
	{
	  g_object_set (window, "title", NULL, NULL);
	  g_object_set_data (window, "bst-title-sync", NULL);
	}
    }
}


/* --- Gtk+ Utilities --- */
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
gtk_tree_view_append_text_columns (GtkTreeView *tree_view,
				   guint	n_cols,
				   ...)
{
  va_list var_args;
  
  g_return_if_fail (GTK_IS_TREE_VIEW (tree_view));
  
  va_start (var_args, n_cols);
  while (n_cols--)
    {
      guint col = va_arg (var_args, guint);
      gfloat xalign = va_arg (var_args, gdouble);
      gchar *title = va_arg (var_args, gchar*);
      
      gtk_tree_view_add_column (tree_view, -1,
				g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
					      "title", title,
					      "sizing", GTK_TREE_VIEW_COLUMN_GROW_ONLY,
					      "resizable", TRUE,
					      NULL),
				g_object_new (GTK_TYPE_CELL_RENDERER_TEXT,
					      "xalign", xalign,
					      NULL),
				"text", col,
				NULL);
    }
  va_end (var_args);
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
  guint        gpack : 8;
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
gmask_form (GtkWidget   *parent,
	    GtkWidget   *action,
	    BstGMaskPack gpack)
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
  gpack = CLAMP (gpack, BST_GMASK_FIT, BST_GMASK_BIG);
  gmask->gpack = gpack;
  gmask->tooltips = g_object_get_data (G_OBJECT (parent), "GMask-tooltips");
  if (gmask->tooltips)
    g_object_ref (gmask->tooltips);

  return action;
}

/**
 * bst_gmask_container_create
 * @tooltips:         Tooltip widget
 * @border_width:     Border width of this GUI mask
 * @dislodge_columns: Provide expandable space between columns
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
bst_gmask_form (GtkWidget   *gmask_container,
		GtkWidget   *action,
		BstGMaskPack gpack)
{
  return gmask_form (gmask_container, action, gpack);
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
 * FILL: allocate all possible (Pre/Post)Action space to the action widget
 * INTERLEAVE: allow the action widget to facilitate unused Aux2/Aux3 space
 * BIG: allocate maximum (left extendeded) possible space to Action
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

  /* retrieve children and set tips */
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
	  dislodge = g_object_new (GTK_TYPE_ALIGNMENT, "visible", TRUE, NULL);
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
  if (gmask->gpack == BST_GMASK_BIG ||
      gmask->gpack == BST_GMASK_INTERLEAVE)	/* extend action to the left when possible */
    {
      if (!aux3)
	{
	  n--;
	  if (!aux2)
	    {
	      n--;
	      if (gmask->gpack == BST_GMASK_BIG && !aux1)
		{
		  n--;
		  if (!prompt)
		    n--;
		}
	    }
	}
    }
  if (gmask->gpack == BST_GMASK_FIT ||
      gmask->gpack == BST_GMASK_INTERLEAVE) /* align to right without expansion */
    action = gtk_widget_new (GTK_TYPE_ALIGNMENT,
			     "visible", TRUE,
			     "child", action,
			     "xalign", 1.0,
			     "xscale", 0.0,
			     "yscale", 0.0,
			     NULL);
  gtk_table_attach (table, action,
		    n, c + 1, row, row + 1,
		    GTK_SHRINK | GTK_FILL,
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
  gpointer mask = bst_gmask_form (gmask_container, action_widget, BST_GMASK_FILL);
  
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


/* --- generated marshallers --- */
#include "bstmarshal.c"


/* --- IDL pspecs --- */
#define sfidl_pspec_Int(name, nick, blurb, dflt, min, max, step, hints)	\
  sfi_pspec_int (name, nick, blurb, dflt, min, max, step, hints)
#define sfidl_pspec_Int_default(name)	sfi_pspec_int (name, NULL, NULL, 0, G_MININT, G_MAXINT, 256, SFI_PARAM_DEFAULT)
#define sfidl_pspec_UInt(name, nick, blurb, dflt, hints)	\
  sfi_pspec_int (name, nick, blurb, dflt, 0, G_MAXINT, 1, hints)
#define sfidl_pspec_Real(name, nick, blurb, dflt, min, max, step, hints)	\
  sfi_pspec_real (name, nick, blurb, dflt, min, max, step, hints)
#define sfidl_pspec_Real_default(name)	sfi_pspec_real (name, NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 10, SFI_PARAM_DEFAULT)
#define sfidl_pspec_Bool(name, nick, blurb, dflt, hints)			\
  sfi_pspec_bool (name, nick, blurb, dflt, hints)
#define sfidl_pspec_Bool_default(name)	sfi_pspec_bool (name, NULL, NULL, FALSE, SFI_PARAM_DEFAULT)
#define sfidl_pspec_Note(name, nick, blurb, dflt, hints)			\
  sfi_pspec_note (name, nick, blurb, dflt, hints)
#define sfidl_pspec_String(name, nick, blurb, dflt, hints)			\
  sfi_pspec_string (name, nick, blurb, dflt, hints)
#define sfidl_pspec_String_default(name)	sfi_pspec_string (name, NULL, NULL, NULL, SFI_PARAM_DEFAULT)
#define sfidl_pspec_Proxy_default(name)	sfi_pspec_proxy (name, NULL, NULL, SFI_PARAM_DEFAULT)
#define sfidl_pspec_Seq(name, nick, blurb, hints, element_pspec)		\
  sfi_pspec_seq (name, nick, blurb, element_pspec, hints)
#define sfidl_pspec_Rec(name, nick, blurb, hints, fields)			\
  sfi_pspec_rec (name, nick, blurb, fields, hints)
#define sfidl_pspec_Rec_default(name, fields)	sfi_pspec_rec (name, NULL, NULL, fields, SFI_PARAM_DEFAULT)
#define sfidl_pspec_BBlock(name, nick, blurb, hints)				\
  sfi_pspec_bblock (name, nick, blurb, hints)
/* --- generated type IDs and SFIDL types --- */
#include "bstgentypes.c"	/* type id defs */
