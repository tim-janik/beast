/* GXK - Gtk+ Extension Kit
 * Copyright (C) 1998-2003 Tim Janik
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
#ifndef __GXK_UTILS_H__
#define __GXK_UTILS_H__

#include        <gxk/gxkglobals.h>

G_BEGIN_DECLS

/* --- generated includes --- */
/* type IDs */
#include <gxk/gxkgentypes.h>
/* marshallers */
#include <gxk/gxkmarshal.h>


/* --- Gtk+ workarounds and amendments --- */
void        gxk_cell_editable_is_focus_handler  (GtkCellEditable *ecell);
gboolean    gxk_cell_editable_canceled		(GtkCellEditable *ecell);
GtkWidget*  gxk_item_factory_sensitize		(GtkItemFactory	 *ifactory,
						 const gchar	 *path,
                                                 gboolean         sensitive);
GtkWidget*  gxk_item_factory_get_item		(GtkItemFactory	 *ifactory,
						 const gchar	 *path);
GtkWidget*  gxk_item_factory_get_widget		(GtkItemFactory	 *ifactory,
						 const gchar	 *path);
void	    gxk_widget_proxy_requisition	(GtkWidget	 *widget);


/* --- GtkFileSelection workarounds --- */
GtkWidget*	gxk_file_selection_heal		(GtkFileSelection	 *fs);
GtkWidget*	gxk_file_selection_split	(GtkFileSelection	 *fs,
						 GtkWidget		**bbox);


/* --- GObject convenience --- */
typedef struct {
  gchar        *type_name;
  GType         parent;
  GType        *type_id;
  gconstpointer type_data;	/* e.g. GEnumValue array */
} GxkTypeGenerated;
void    gxk_type_register_generated	(guint			 n_entries,
					 const GxkTypeGenerated	*entries);
void	g_object_set_double		(gpointer		 object,
					 const gchar		*name,
					 gdouble		 v_double);
gdouble	g_object_get_double		(gpointer		 object,
					 const gchar		*name);
void	g_object_set_long		(gpointer		 object,
					 const gchar		*name,
					 glong			 v_long);
glong	g_object_get_long		(gpointer		 object,
					 const gchar		*name);
#define	g_object_set_int		g_object_set_long	// FIXME
#define	g_object_get_int		g_object_get_long	// FIXME


/* --- UTF8 helpers --- */
gchar*	gxk_convert_latin1_to_utf8	(const gchar	*string);
gchar*	gxk_filename_to_utf8		(const gchar	*string);


/* --- Gtk+ Utilities --- */
void	 gxk_widget_viewable_changed	(GtkWidget		*widget);
gboolean gxk_widget_viewable		(GtkWidget		*widget);


/* --- Gdk convenience --- */
#define	GXK_DEFAULT_CURSOR	GDK_LAST_CURSOR	/* revert to default (inherited) cursor */
void	gxk_window_set_cursor_type	(GdkWindow		*window,
					 GdkCursorType		 cursor);
void	gxk_window_process_next		(GdkWindow		*window,
					 gboolean		 update_children);
void	gxk_color_alloc			(GdkColormap		*colormap,
					 GdkColor		*color);


/* --- Gtk convenience --- */
#define GTK_STYLE_THICKNESS(s,xy)	((s)-> xy##thickness)
void	gxk_widget_make_insensitive	(GtkWidget	*widget);
void	gxk_widget_make_sensitive	(GtkWidget	*widget);
void	gxk_widget_showraise		(GtkWidget	*widget);
void	gxk_idle_show_widget		(GtkWidget	*widget);
void	gxk_idle_unrealize_widget	(GtkWidget	*widget);
void    gxk_notebook_add_page		(GtkNotebook	*notebook,
					 GtkWidget	*child,
					 const gchar	*tab_text,
					 gboolean        expand_fill);
void    gxk_notebook_set_current_page_widget (GtkNotebook *notebook,
					      GtkWidget   *page);
GtkWidget* gtk_notebook_current_widget       (GtkNotebook *notebook);

/* functions to affect a widget tree's toplevel */
void	gxk_toplevel_delete		(GtkWidget	*widget);
void	gxk_toplevel_hide		(GtkWidget	*widget);
void	gxk_toplevel_activate_default	(GtkWidget	*widget);

/* widget utilities */
void	gxk_widget_modify_as_title	(GtkWidget	*widget);
void	gxk_widget_modify_bg_as_base	(GtkWidget	*widget);
void	gxk_widget_modify_base_as_bg	(GtkWidget	*widget);
void	gxk_widget_force_bg_clear	(GtkWidget	*widget);
void	gxk_widget_activate_accel_group	(GtkWidget	*widget,
					 GtkAccelGroup	*accel_group);
void	gxk_size_group			(GtkSizeGroupMode sgmode,
					 gpointer	  first_widget,
					 ...);

/* tree view convenience */
gint	 gxk_tree_spath_index0			(const gchar		*strpath);
gboolean gxk_tree_path_prev			(GtkTreePath		*path);
guint	 gxk_tree_view_add_column		(GtkTreeView		*tree_view,
						 gint			 position,
						 GtkTreeViewColumn	*column,
						 GtkCellRenderer	*cell,
						 const gchar		*attrib_name,
						 ...);
void	 gxk_tree_view_append_text_columns	(GtkTreeView		*tree_view,
						 guint			 n_cols,
						 ...);
void	 gxk_tree_view_add_text_column		(GtkTreeView  *tree_view,
						 guint	       model_column,
						 const gchar  *column_flags,
						 gdouble       xalign,
						 const gchar  *title,
						 const gchar  *tooltip,
						 gpointer      edited_callback,
						 gpointer      data,
						 GConnectFlags cflags);
void	 gxk_tree_view_add_popup_column		(GtkTreeView  *tree_view,
						 guint	       model_column,
						 const gchar  *column_flags,
						 gdouble       xalign,
						 const gchar  *title,
						 const gchar  *tooltip,
						 gpointer      edited_callback,
						 gpointer      popup_callback,
						 gpointer      data,
						 GConnectFlags cflags);
void	gxk_tree_view_add_toggle_column		(GtkTreeView  *tree_view,
						 guint	       model_column,
						 const gchar  *column_flags,
						 gdouble       xalign,
						 const gchar  *title,
						 const gchar  *tooltip,
						 gpointer      toggled_callback,
						 gpointer      data,
						 GConnectFlags cflags);
void	gxk_tree_view_column_set_tip_title	(GtkTreeViewColumn   *tree_column,
						 const gchar         *title,
						 const gchar	     *tooltip);
void	gxk_tree_view_set_editable      	(GtkTreeView         *tview,
                                                 gboolean             maybe_editable);


/* tree selection convenience */
void   gxk_tree_selection_select_spath   (GtkTreeSelection      *selection,
					  const gchar           *str_path);
void   gxk_tree_selection_unselect_spath (GtkTreeSelection      *selection,
					  const gchar           *str_path);
void   gxk_tree_selection_select_ipath   (GtkTreeSelection      *selection,
					  gint			 first_index,
					  ...);
void   gxk_tree_selection_unselect_ipath (GtkTreeSelection      *selection,
					  gint			 first_index,
					  ...);
void   gxk_tree_selection_force_browse	 (GtkTreeSelection	*selection,
					  GtkTreeModel		*model);
void   gxk_tree_view_get_bin_window_pos	 (GtkTreeView		*tree,
					  gint			*x_p,
					  gint			*y_p);
gboolean gxk_tree_view_get_row_area	 (GtkTreeView		*tree,
					  gint			 row,
					  gint			*y_p,
					  gint			*height_p);
gboolean gxk_tree_view_get_row_from_coord(GtkTreeView		*tree,
					  gint			 y,
					  gint			*row_p);
void     gxk_tree_view_focus_row	 (GtkTreeView		*tree,
					  gint			 row);
gboolean gxk_tree_view_is_row_selected	 (GtkTreeView		*tree,
					  gint			 row);
gint     gxk_tree_view_get_selected_row	 (GtkTreeView		*tree);

/* misc widgets */
void	gxk_notebook_append		(GtkNotebook	*notebook,
					 GtkWidget	*child,
					 const gchar	*label);


/* --- signal convenience --- */
gboolean	gxk_signal_handler_pending	(gpointer	 instance,
						 const gchar	*detailed_signal,
						 GCallback	 callback,
						 gpointer	 data);


/* --- derivation convenience --- */
typedef enum /*< skip >*/
{
  GXK_METHOD_NONE,
  GXK_METHOD_INIT,
  GXK_METHOD_FINALIZE,
  GXK_METHOD_DISPOSE,
  GXK_METHOD_DESTROY,
} GxkMethodType;
GType	gxk_object_derive	(GType		parent_type,
				 const gchar   *name,
				 gpointer      *parent_class_p,
				 guint          instance_size,
				 guint          class_size,
				 GxkMethodType  mtype,
				 ...);


G_END_DECLS

#endif /* __GXK_UTILS_H__ */
