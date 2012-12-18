/* GXK - Gtk+ Extension Kit
 * Copyright (C) 1998-2006 Tim Janik
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
gboolean        gxk_widget_ancestry_viewable       (GtkWidget       *widget);
void            gxk_cell_editable_is_focus_handler (GtkCellEditable *ecell);
gboolean        gxk_cell_editable_canceled         (GtkCellEditable *ecell);
GtkWidget*      gxk_item_factory_sensitize         (GtkItemFactory  *ifactory,
                                                    const gchar     *path,
                                                    gboolean         sensitive);
GtkWidget*      gxk_item_factory_get_item          (GtkItemFactory  *ifactory,
                                                    const gchar     *path);
GtkWidget*      gxk_item_factory_get_widget        (GtkItemFactory  *ifactory,
                                                    const gchar     *path);
void            gxk_widget_proxy_requisition       (GtkWidget       *widget,
                                                    gdouble          xscale,
                                                    gdouble          yscale);
void            gxk_widget_request_hclient_height  (GtkWidget       *widget,
                                                    GtkWidget       *client);
void            gxk_widget_request_vclient_width   (GtkWidget       *widget,
                                                    GtkWidget       *client);
gboolean        gxk_widget_has_ancestor            (gpointer         widget,
                                                    gpointer         ancestor);
void            gxk_menu_set_active                (GtkMenu         *menu,
                                                    GtkWidget       *child);
void            gxk_widget_regulate                (GtkWidget       *widget,
                                                    gboolean         sensitive,
                                                    gboolean         active);
gboolean        gxk_widget_regulate_uses_active    (GtkWidget       *widget);
gboolean        gxk_menu_check_sensitive           (GtkMenu         *menu);
void            gxk_menu_attach_as_submenu         (GtkMenu         *menu,
                                                    GtkMenuItem     *menu_item);
void            gxk_option_menu_set_menu           (GtkOptionMenu   *option_menu,
                                                    GtkMenu         *menu);
void            gxk_menu_attach_as_popup           (GtkMenu         *menu,
                                                    GtkWidget       *widget);
void            gxk_menu_attach_as_popup_with_func (GtkMenu         *menu,
                                                    GtkWidget       *widget,
                                                    GtkMenuDetachFunc mdfunc);
void            gxk_menu_popup                     (GtkMenu         *menu,
                                                    gint             x,
                                                    gint             y,
                                                    guint            mouse_button,
                                                    guint32          time);
void            gxk_menu_popup_pushable            (GtkMenu         *menu,
                                                    gint             x,
                                                    gint             y,
                                                    gint             pushed_x,
                                                    gint             pushed_y,
                                                    guint            mouse_button,
                                                    guint32          time);
void            gxk_menu_popup_pushed_in           (GtkMenu         *menu,
                                                    gint             pushed_x,
                                                    gint             pushed_y,
                                                    guint            mouse_button,
                                                    guint32          time);
GtkWidget*      gxk_widget_find_level_ordered      (GtkWidget       *toplevel,
                                                    const gchar     *name);
GtkWidget*      gxk_widget_get_attach_toplevel     (GtkWidget       *widget);
void            gxk_widget_add_font_requisition    (GtkWidget       *widget,
                                                    guint            n_chars,
                                                    guint            n_digits);
void            gxk_widget_add_option              (gpointer         widget,
                                                    const gchar     *option,
                                                    const gchar     *value);
gboolean        gxk_widget_check_option            (gpointer         widget,
                                                    const gchar     *option);
const gchar*    gxk_widget_get_options             (gpointer         widget);
void            gxk_window_set_menu_accel_group    (GtkWindow       *window,
                                                    GtkAccelGroup   *agroup);
GtkAccelGroup*  gxk_window_get_menu_accel_group    (GtkWindow       *window);
void            gxk_window_set_geometry_min_width  (GtkWindow       *window,
                                                    guint            min_width);
void            gxk_window_set_geometry_min_height (GtkWindow       *window,
                                                    guint            min_height);
void            gxk_window_set_geometry_width_inc  (GtkWindow       *window,
                                                    guint            width_increment);
void            gxk_window_set_geometry_height_inc (GtkWindow       *window,
                                                    guint            height_increment);
void            gxk_expander_connect_to_widget     (GtkWidget       *expander,
                                                    GtkWidget       *widget);
void            gxk_label_set_attributes           (GtkLabel        *label,
                                                    ...);

guint           gxk_container_get_insertion_slot   (GtkContainer    *container);
void            gxk_container_slot_reorder_child   (GtkContainer    *container,
                                                    GtkWidget       *widget,
                                                    guint            slot);
gboolean        gxk_grab_pointer_and_keyboard      (GdkWindow    *window,
                                                    gboolean      owner_events,
                                                    GdkEventMask  event_mask,
                                                    GdkWindow    *confine_to,
                                                    GdkCursor    *cursor,
                                                    guint32       time);
void            gxk_ungrab_pointer_and_keyboard    (GdkWindow    *window,
                                                    guint32       time);


/* --- GtkFileSelection workarounds --- */
GtkWidget*	gxk_file_selection_heal		(GtkFileSelection	 *fs);
GtkWidget*	gxk_file_selection_split	(GtkFileSelection	 *fs,
						 GtkWidget		**bbox);


/* --- GObject convenience --- */
typedef struct {
  const char   *type_name;
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
gchar*	     gxk_convert_latin1_to_utf8	     (const gchar    *string);
gchar*	     gxk_filename_to_utf8	     (const gchar    *string);
const gchar* gxk_factory_path_get_leaf       (const gchar    *path);
gchar*       gxk_factory_path_unescape_uline (const gchar    *path);


/* --- Gtk+ Utilities --- */
void	 gxk_widget_viewable_changed	        (GtkWidget   *widget);
gboolean gxk_widget_viewable		        (GtkWidget   *widget);
void     gxk_widget_attached_hierarchy_changed  (GtkWidget   *widget);


/* --- Gdk convenience --- */
#define	GXK_DEFAULT_CURSOR	GDK_LAST_CURSOR	///< Indicates default (inherited) cursor
void	 gxk_window_set_cursor_type	(GdkWindow		*window,
					 GdkCursorType		 cursor);
void	 gxk_window_process_next	(GdkWindow		*window,
					 gboolean		 update_children);
void     gdk_draw_hline                 (GdkDrawable            *drawable,
                                         GdkGC                  *gc,
                                         gint                    x,
                                         gint                    y,
                                         gint                    width);
void     gdk_draw_vline                 (GdkDrawable            *drawable,
                                         GdkGC                  *gc,
                                         gint                    x,
                                         gint                    y,
                                         gint                    height);
void	 gxk_color_alloc		(GdkColormap		*colormap,
					 GdkColor		*color);
GdkColor gdk_color_from_rgb             (guint                   rgb_value);
GdkColor gdk_color_from_argb            (guint                   rgb_value);
GdkColor gdk_color_from_rgba            (guint                   rgb_value);

/* --- Colors --- */
typedef struct {
  double value;
  guint  rgb;
} GxkColorDot;
typedef struct {
  guint        n_colors;
  GxkColorDot *colors;
} GxkColorDots;
GxkColorDots*   gxk_color_dots_new              (guint               n_dots,
                                                 const GxkColorDot  *dots);
guint           gxk_color_dots_interpolate      (GxkColorDots       *cdots,
                                                 double              value,
                                                 double              saturation);
void            gxk_color_dots_destroy          (GxkColorDots       *cdots);

/* --- Gtk convenience --- */
#define GTK_STYLE_THICKNESS(s,xy)	((s)-> xy##thickness)
void         gxk_widget_make_insensitive          (GtkWidget        *widget);
void         gxk_widget_make_sensitive            (GtkWidget        *widget);
void         gxk_widget_showraise                 (GtkWidget        *widget);
void         gxk_idle_showraise                   (GtkWidget        *widget);
void         gxk_idle_show_widget                 (GtkWidget        *widget);
void         gxk_idle_unrealize_widget            (GtkWidget        *widget);
GtkWidget*   gxk_notebook_create_tabulator        (const gchar      *label_text,
                                                   const gchar      *stock_image,
                                                   const gchar      *tooltip);
void         gxk_notebook_change_tabulator        (GtkWidget        *tabulator,
                                                   const gchar      *label_text,
                                                   const gchar      *stock_image,
                                                   const gchar      *tooltip);
void         gxk_notebook_set_current_page_widget (GtkNotebook      *notebook,
                                                   GtkWidget        *page);
GtkWidget*   gxk_vseparator_space_new             (gboolean          draw_seperator);
GtkWidget*   gtk_notebook_current_widget          (GtkNotebook      *notebook);
GtkWidget*   gxk_notebook_descendant_get_page     (GtkWidget        *widget);
GtkWidget*   gxk_notebook_descendant_get_tab      (GtkWidget        *widget);
void	     gxk_notebook_append		  (GtkNotebook	    *notebook,
                                                   GtkWidget	    *child,
                                                   const gchar	    *label,
                                                   gboolean          fillexpand);
GtkWidget*   gtk_box_get_nth_child                (GtkBox           *box,
                                                   gint              pos);
void         gxk_scrolled_window_spare_space      (GtkScrolledWindow*scwin);
void         gxk_scrolled_window_unspare_space    (GtkScrolledWindow*scwin);
GtkWidget*   gxk_scrolled_window_create           (GtkWidget        *child,
                                                   GtkShadowType     shadow_type,
                                                   gdouble           xrequest,
                                                   gdouble           yrequest);

/* functions to affect a widget tree's toplevel */
void         gxk_toplevel_delete                  (GtkWidget        *widget);
void         gxk_toplevel_hide                    (GtkWidget        *widget);
void         gxk_toplevel_activate_default        (GtkWidget        *widget);

/* widget utilities */
GtkWidget*   gxk_parent_find_descendant           (GtkWidget        *parent,
                                                   GType             descendant_type);
void         gxk_widget_modify_as_title           (GtkWidget        *widget);
void         gxk_widget_modify_normal_bg_as_base  (GtkWidget        *widget);
void         gxk_widget_modify_bg_as_base         (GtkWidget        *widget);
void         gxk_widget_modify_base_as_bg         (GtkWidget        *widget);
void         gxk_widget_modify_bg_as_active       (GtkWidget        *widget);
void         gxk_widget_force_bg_clear            (GtkWidget        *widget);
void         gxk_widget_set_tooltip               (gpointer          widget,
                                                   const gchar      *tooltip);
void         gxk_widget_set_latent_tooltip        (GtkWidget        *widget,
                                                   const gchar      *tooltip);
const gchar* gxk_widget_get_latent_tooltip        (GtkWidget        *widget);
void         gxk_widget_activate_accel_group      (GtkWidget        *widget,
                                                   GtkAccelGroup    *accel_group);
void         gxk_size_group                       (GtkSizeGroupMode  sgmode,
                                                   gpointer          first_widget,
                                                   ...) G_GNUC_NULL_TERMINATED;

/* tree view convenience */
gint	           gxk_tree_spath_index0		(const gchar		*strpath);
gboolean           gxk_tree_model_get_iter              (GtkTreeModel           *tree_model,
                                                         GtkTreeIter            *iter,
                                                         GtkTreePath            *path);
gboolean           gxk_tree_path_prev			(GtkTreePath		*path);
guint	           gxk_tree_view_add_column		(GtkTreeView		*tree_view,
                                                         gint			 position,
                                                         GtkTreeViewColumn	*column,
                                                         GtkCellRenderer	*cell,
                                                         const gchar		*attrib_name,
                                                         ...) G_GNUC_NULL_TERMINATED;
void               gxk_tree_view_append_text_columns    (GtkTreeView		*tree_view,
                                                         guint			 n_cols,
                                                         ...);
GtkTreeViewColumn* gxk_tree_view_add_text_column	(GtkTreeView            *tree_view,
                                                         guint	                 model_column,
                                                         const gchar            *column_flags,
                                                         gdouble                 xalign,
                                                         const gchar            *title,
                                                         const gchar            *tooltip,
                                                         gpointer                edited_callback,
                                                         gpointer                data,
                                                         GConnectFlags           cflags);
GtkTreeViewColumn* gxk_tree_view_add_popup_column	(GtkTreeView            *tree_view,
                                                         guint	                 model_column,
                                                         const gchar            *column_flags,
                                                         gdouble                 xalign,
                                                         const gchar            *title,
                                                         const gchar            *tooltip,
                                                         gpointer                edited_callback,
                                                         gpointer                popup_callback,
                                                         gpointer                data,
                                                         GConnectFlags           cflags);
GtkTreeViewColumn* gxk_tree_view_add_toggle_column	(GtkTreeView            *tree_view,
                                                         guint	                 model_column,
                                                         const gchar            *column_flags,
                                                         gdouble                 xalign,
                                                         const gchar            *title,
                                                         const gchar            *tooltip,
                                                         gpointer                toggled_callback,
                                                         gpointer                data,
                                                         GConnectFlags           cflags);
void	           gxk_tree_view_column_set_tip_title	(GtkTreeViewColumn      *tree_column,
                                                         const gchar            *title,
                                                         const gchar	        *tooltip);
void	           gxk_tree_view_set_editable      	(GtkTreeView            *tview,
                                                         gboolean                maybe_editable);

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
void   gxk_tree_view_select_index        (GtkTreeView           *tview,
                                          guint                  index);
void   gxk_tree_selection_force_browse	 (GtkTreeSelection	*selection,
					  GtkTreeModel		*model);
void   gxk_tree_view_get_bin_window_pos	 (GtkTreeView		*tree,
					  gint			*x_p,
					  gint			*y_p);
gboolean gxk_tree_view_get_row_area	 (GtkTreeView		*tree,
					  gint			 row,
					  gint			*y_p,
					  gint			*height_p,
                                          gboolean               content_area);
gboolean gxk_tree_view_get_row_from_coord(GtkTreeView		*tree,
					  gint			 y,
					  gint			*row_p);
void     gxk_tree_view_focus_row	 (GtkTreeView		*tree,
					  gint			 row);
gboolean gxk_tree_view_is_row_selected	 (GtkTreeView		*tree,
					  gint			 row);
gint     gxk_tree_view_get_selected_row	 (GtkTreeView		*tree);

/* --- signal convenience --- */
gboolean	gxk_signal_handler_exists	(gpointer	 instance,
						 const gchar	*detailed_signal,
						 GCallback	 callback,
						 gpointer	 data);
gboolean	gxk_signal_handler_pending	(gpointer	 instance,
						 const gchar	*detailed_signal,
						 GCallback	 callback,
						 gpointer	 data);


/* --- zlib support --- */
gchar*  gxk_zfile_uncompress    (guint                uncompressed_size,
                                 const unsigned char *cdata,
                                 guint                cdata_size);


G_END_DECLS

#endif /* __GXK_UTILS_H__ */
