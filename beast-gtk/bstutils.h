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
#ifndef __BST_UTILS_H__
#define __BST_UTILS_H__

#include        <bse/bse.h>
#include        <bsw/bsw.h>
#include        <gtk/gtk.h>
#include        <libgnomecanvas/libgnomecanvas.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- generated type ids --- */
#include	"bstgentypes.h"
void		bst_init_gentypes	(void);


/* --- marshallers --- */
#include	"bstmarshal.h"


/* --- stock actions & icons --- */
#define	BST_STOCK_NONE			(NULL)
#define BST_STOCK_APPLY			("bst-stock-apply")
#define BST_STOCK_CANCEL		("bst-stock-cancel")
#define	BST_STOCK_CDROM			("bst-stock-cdrom")
#define BST_STOCK_CLONE			("bst-stock-clone")
#define BST_STOCK_CLOSE			("bst-stock-close")
#define	BST_STOCK_DELETE		("bst-stock-delete")
#define BST_STOCK_DEFAULT_REVERT	("bst-stock-default-revert")
#define BST_STOCK_EDIT_TOOL		("bst-stock-edit-tool")
#define BST_STOCK_EXECUTE		("bst-stock-execute")
#define	BST_STOCK_INFO			("bst-stock-info")
#define	BST_STOCK_INSTRUMENT		("bst-stock-instrument")
#define	BST_STOCK_KNOB			("bst-stock-palette-knob")
#define	BST_STOCK_LOAD			("bst-stock-load")
#define	BST_STOCK_MOUSE_TOOL		("bst-stock-mouse-tool")
#define	BST_STOCK_NOICON		("bst-stock-no-icon")
#define	BST_STOCK_NO_ILINK		("bst-stock-no-ilink")
#define	BST_STOCK_NO_OLINK		("bst-stock-no-olink")
#define BST_STOCK_OK			("bst-stock-ok")
#define BST_STOCK_OVERWRITE		("bst-stock-overwrite")
#define	BST_STOCK_PALETTE		("bst-stock-palette")
#define	BST_STOCK_PART			("bst-stock-part")
#define	BST_STOCK_PART_EDITOR		("bst-stock-part-editor")
#define	BST_STOCK_PART_TOOL		("bst-stock-part-tool")
#define	BST_STOCK_PATTERN		("bst-stock-pattern")
#define	BST_STOCK_PATTERN_GROUP		("bst-stock-pattern-group")
#define	BST_STOCK_PATTERN_TOOL		("bst-stock-pattern-tool")
#define BST_STOCK_PREVIEW_AUDIO		("bst-stock-preview-audio")
#define BST_STOCK_PREVIEW_NOAUDIO	("bst-stock-preview-noaudio")
#define	BST_STOCK_PROPERTIES		("bst-stock-properties")
#define BST_STOCK_RECT_SELECT		("bst-stock-rect-select")
#define BST_STOCK_REDO			("bst-stock-redo")
#define BST_STOCK_REVERT		("bst-stock-revert")
#define	BST_STOCK_TARGET		("bst-stock-target")
#define	BST_STOCK_TRASHCAN		("bst-stock-trashcan")
#define BST_STOCK_UNDO			("bst-stock-undo")
#define BST_STOCK_VERT_SELECT		("bst-stock-vert-select")
#define BST_STOCK_WAVE			("bst-stock-wave")
#define BST_STOCK_WAVE_TOOL		("bst-stock-wave-tool")
#define	BST_STOCK_ZOOM_100		("bst-stock-zoom-100")
#define	BST_STOCK_ZOOM_ANY		("bst-stock-zoom-any")
#define	BST_STOCK_ZOOM_FIT		("bst-stock-zoom-fit")
#define	BST_STOCK_ZOOM_IN		("bst-stock-zoom-in")
#define	BST_STOCK_ZOOM_OUT		("bst-stock-zoom-out")

#define	BST_SIZE_BUTTON		(bst_size_button)
#define	BST_SIZE_BIG_BUTTON	(bst_size_big_button)
#define	BST_SIZE_CANVAS		(bst_size_canvas)
#define	BST_SIZE_TOOLBAR	(bst_size_toolbar)
#define	BST_SIZE_MENU		(bst_size_menu)
#define	BST_SIZE_PALETTE	(BST_SIZE_TOOLBAR)

/* really of type GtkIconSize: */
extern guint	bst_size_button;
extern guint	bst_size_big_button;
extern guint	bst_size_canvas;
extern guint	bst_size_toolbar;
extern guint	bst_size_menu;

/* retrive static icons (no reference count needs) */
GtkWidget*	bst_image_from_stock		(const gchar	*stock_icon_id,
						 GtkIconSize	 icon_size);
const gchar*	bst_stock_action		(const gchar	*stock_id);
GdkPixbuf*	bst_pixbuf_no_icon		(void);
GdkPixbuf*	bst_pixbuf_knob			(void);
GtkWidget*	bst_image_from_icon		(BswIcon	*icon,
						 GtkIconSize	 icon_size);
GtkWidget*	bst_stock_button		(const gchar	*stock_id,
						 const gchar	*label);
GtkWidget*	bst_drag_window_from_stock	(const gchar	*stock_id);
void		_bst_utils_init			(void);
guint		bst_size_width			(GtkIconSize	 bst_size);
guint		bst_size_height			(GtkIconSize	 bst_size);


/* --- Gtk+ utilities & workarounds --- */
#define    GTK_TYPE_VPANED               (gtk_vpaned_get_type ())
#define    GTK_TYPE_HPANED               (gtk_hpaned_get_type ())
void	   gtk_post_init_patch_ups	 (void);
gboolean   gtk_widget_viewable		 (GtkWidget		*widget);
void	   gtk_widget_showraise		 (GtkWidget		*widget);
void	   gtk_toplevel_hide		 (GtkWidget		*widget);
void	   gtk_toplevel_delete		 (GtkWidget		*widget);
void	   gtk_toplevel_activate_default (GtkWidget		*widget);
void	   bst_widget_request_aux_info	 (GtkWidget		*viewport);
void	   gtk_widget_make_sensitive	 (GtkWidget		*widget);
void	   gtk_widget_make_insensitive	 (GtkWidget		*widget);
void	   gtk_file_selection_heal	 (GtkFileSelection	*fs);
void	   gtk_idle_show_widget		 (GtkWidget		*widget);
void	   gtk_idle_unparent		 (GtkWidget		*widget);
void	   gtk_last_event_coords	 (gint			*x_root,
					  gint			*y_root);
void	   gtk_clist_moveto_selection	 (GtkCList		*clist);
gpointer   gtk_clist_get_selection_data	 (GtkCList		*clist,
					  guint                  index);
void	   gtk_widget_viewable_changed	 (GtkWidget		*widget);
guint	   gtk_tree_view_add_column	 (GtkTreeView	        *tree_view,
					  gint                   position,
					  GtkTreeViewColumn     *column,
					  GtkCellRenderer       *cell,
					  const gchar           *attrib_name,
					  ...);
void   gtk_tree_selection_select_spath   (GtkTreeSelection	*selection,
					  const gchar		*str_path);
void   gtk_tree_selection_unselect_spath (GtkTreeSelection	*selection,
					  const gchar		*str_path);
guint8*	gdk_pixbuf_create_bitmap_data	 (GdkPixbuf		*pixbuf,
					  gint			*width_p,
					  gint			*height_p,
					  guint8		 alpha_threshold);

#define gtk_notebook_current_widget(n) \
    gtk_notebook_get_nth_page ((n), gtk_notebook_get_current_page ((n)))

#define GTK_STYLE_THICKNESS(s,xy)    ((s)-> xy##thickness)
  

/* --- GUI field mask --- */
GtkWidget*   bst_gmask_container_create	(gpointer	tooltips,
					 guint		border_width);
gpointer	bst_gmask_form		(GtkWidget     *gmask_container,
					 GtkWidget     *action,
					 gboolean	expandable);
gpointer	bst_gmask_form_big	(GtkWidget     *gmask_container,
					 GtkWidget     *action);
void		bst_gmask_set_tip	(gpointer	mask,
					 const gchar   *tip_text);
void		bst_gmask_set_prompt	(gpointer	mask,
					 gpointer	widget);
void		bst_gmask_set_aux1	(gpointer	mask,
					 gpointer	widget);
void		bst_gmask_set_aux2	(gpointer	mask,
					 gpointer	widget);
void		bst_gmask_set_aux3	(gpointer	mask,
					 gpointer	widget);
void		bst_gmask_set_ahead	(gpointer	mask,
					 gpointer	widget);
void		bst_gmask_set_atail	(gpointer	mask,
					 gpointer	widget);
void		bst_gmask_set_column	(gpointer	mask,
					 guint		column);
GtkWidget*	bst_gmask_get_prompt	(gpointer	mask);
GtkWidget*	bst_gmask_get_aux1	(gpointer	mask);
GtkWidget*	bst_gmask_get_aux2	(gpointer	mask);
GtkWidget*	bst_gmask_get_aux3	(gpointer	mask);
GtkWidget*	bst_gmask_get_ahead	(gpointer	mask);
GtkWidget*	bst_gmask_get_action	(gpointer	mask);
GtkWidget*	bst_gmask_get_atail	(gpointer	mask);
void		bst_gmask_foreach	(gpointer	mask,
					 gpointer	func,
					 gpointer	data);
void		bst_gmask_pack		(gpointer	mask);
gpointer	bst_gmask_quick		(GtkWidget     *gmask_container,
					 guint		column,
					 const gchar   *prompt,
					 gpointer       action_widget,
					 const gchar   *tip_text);
#define	bst_gmask_set_sensitive(mask, sensitive)	\
    bst_gmask_foreach ((mask), \
		       (sensitive) ? gtk_widget_make_sensitive : gtk_widget_make_insensitive, \
		       NULL)
#define	bst_gmask_ensure_styles(mask)			\
    bst_gmask_foreach ((mask), gtk_widget_ensure_style, NULL)
#define	bst_gmask_destroy(mask)				\
    bst_gmask_foreach ((mask), gtk_widget_destroy, NULL)


/* --- BEAST utilities --- */
void		g_object_set_int		(gpointer	 object,
						 const gchar	*name,
						 glong		 v_int);
glong		g_object_get_int		(gpointer	 object,
						 const gchar	*name);
void	        bst_widget_modify_as_title	(GtkWidget	*widget);
void	        bst_widget_modify_bg_as_base	(GtkWidget	*widget);
void	        bst_widget_modify_base_as_bg	(GtkWidget	*widget);
GtkWidget*	bst_text_view_from		(GString        *gstring,
						 const gchar    *file_name,
						 const gchar    *font_name);
GtkWidget*	bst_wrap_text_create		(gboolean        duplicate_newlines,
						 const gchar    *string);
void		bst_wrap_text_set		(GtkWidget      *text,
						 const gchar    *string);
void		bst_wrap_text_clear		(GtkWidget      *text);
void		bst_wrap_text_push_indent	(GtkWidget	*text,
						 const gchar	*spaces);
void		bst_wrap_text_append		(GtkWidget      *text,
						 const gchar    *string);
void		bst_wrap_text_aprintf		(GtkWidget      *text,
						 const gchar    *text_fmt,
						 ...) G_GNUC_PRINTF (2, 3);
void		bst_wrap_text_pop_indent	(GtkWidget	*text);
guint      bst_container_get_insertion_position (GtkContainer   *container,
						 gboolean        scan_horizontally,
						 gint            xy,
						 GtkWidget      *ignore_child,
						 gint           *ignore_child_position);
void		bst_container_set_named_child	(GtkWidget	*container,
						 GQuark		 qname,
						 GtkWidget	*child);
GtkWidget*	bst_container_get_named_child	(GtkWidget	*container,
						 GQuark		 qname);
GtkWidget*	bst_xpm_view_create		(const gchar   **xpm,
						 GtkWidget	*colormap_widget);
void		bst_widget_force_bg_clear	(GtkWidget	*widget);


/* --- Canvas utilities & workarounds --- */
GnomeCanvasPoints*	gnome_canvas_points_new0	 (guint            n_points);
GnomeCanvasPoints*	gnome_canvas_points_newv	 (guint            n_points,
							  ...);
GnomeCanvasItem*	gnome_canvas_typed_item_at	 (GnomeCanvas     *canvas,
							  GtkType	   item_type,
							  gdouble          world_x,
							  gdouble          world_y);
guint			gnome_canvas_item_get_stacking   (GnomeCanvasItem *item);
void			gnome_canvas_item_keep_between   (GnomeCanvasItem *between,
							  GnomeCanvasItem *item1,
							  GnomeCanvasItem *item2);
void			gnome_canvas_item_keep_above	 (GnomeCanvasItem *above,
							  GnomeCanvasItem *item1,
							  GnomeCanvasItem *item2);
void			gnome_canvas_FIXME_hard_update	 (GnomeCanvas	  *canvas);





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_UTILS_H__ */
