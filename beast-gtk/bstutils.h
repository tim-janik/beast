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

#define	GTK_OBJECT_DESTROYED(x)	(0)

/* --- generated type ids --- */
#include	"bstgentypes.h"
void		bst_init_gentypes	(void);


/* --- marshallers --- */
#include	"bstmarshal.h"


/* --- pixmap stock --- */
typedef enum
{
  BST_ICON_NONE,
  BST_ICON_NOICON,
  BST_ICON_MOUSE_TOOL,
  BST_ICON_PALETTE_TOOL,
  BST_ICON_PROPERTIES,
  BST_ICON_DELETE,
  BST_ICON_TRASHCAN,
  BST_ICON_TARGET,
  BST_ICON_CLOSE,
  BST_ICON_NO_ILINK,
  BST_ICON_NO_OLINK,
  BST_ICON_PATTERN,
  BST_ICON_PATTERN_GROUP,
  BST_ICON_PATTERN_TOOL,
  BST_ICON_CDROM,
  BST_ICON_LAST
} BstIconId;

/* retrive static icons (no reference count needs) */
BseIcon* bst_icon_from_stock (BstIconId icon_id);


/* --- Gtk+ utilities & workarounds --- */
#define    GTK_TYPE_VPANED               (gtk_vpaned_get_type ())
#define    GTK_TYPE_HPANED               (gtk_hpaned_get_type ())
void	   gtk_post_init_patch_ups	 (void);
gboolean   gtk_widget_viewable		 (GtkWidget		*widget);
void	   gtk_widget_showraise		 (GtkWidget		*widget);
void	   gtk_toplevel_hide		 (GtkWidget		*widget);
void	   gtk_toplevel_activate_default (GtkWidget		*widget);
void	   gtk_widget_make_sensitive	 (GtkWidget		*widget);
void	   gtk_widget_make_insensitive	 (GtkWidget		*widget);
void	   gtk_file_selection_heal	 (GtkFileSelection	*fs);
void	   gtk_idle_show_widget		 (GtkWidget		*widget);
void	   gtk_idle_unparent		 (GtkWidget		*widget);
void	   gtk_last_event_coords	 (gint			*x_root,
					  gint			*y_root);
void	   gtk_last_event_widget_coords	 (GtkWidget		*widget,
					  gint			*x,
					  gint			*y);
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
void	        bst_widget_modify_as_title	(GtkWidget	*widget);
void	        bst_widget_modify_bg_as_base	(GtkWidget	*widget);
GtkWidget*	bst_forest_from_bse_icon	(BseIcon	*bse_icon,
						 guint           icon_width,
						 guint           icon_height);
GtkWidget*	bst_text_view_from		(GString        *gstring,
						 const gchar    *file_name,
						 const gchar    *font_name,
						 const gchar    *font_fallback);
GtkWidget*	bst_wrap_text_create		(const gchar    *string,
						 gboolean        double_newlines,
						 gpointer        user_data);
void		bst_wrap_text_set		(GtkWidget      *text,
						 const gchar    *string,
						 gboolean        double_newlines,
						 gpointer        user_data);
GtkWidget*	bst_drag_window_from_icon	(BseIcon	*icon);
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


/* --- Auxillary Dialog --- */
#define BST_TYPE_ADIALOG            (bst_adialog_get_type ())
#define BST_ADIALOG(object)         (GTK_CHECK_CAST ((object), BST_TYPE_ADIALOG, BstADialog))
#define BST_ADIALOG_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_ADIALOG, BstADialogClass))
#define BST_IS_ADIALOG(object)      (GTK_CHECK_TYPE ((object), BST_TYPE_ADIALOG))
#define BST_IS_ADIALOG_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_ADIALOG))
#define BST_ADIALOG_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), BST_TYPE_ADIALOG, BstADialogClass))
typedef struct _BstADialog BstADialog;
typedef GtkWindowClass     BstADialogClass;
struct _BstADialog
{
  GtkWindow  parent_object;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *default_widget;
  GtkWidget *child;
};
typedef enum
{
  BST_ADIALOG_DESTROY_ON_HIDE	= (1 << 0),	/* always auto-hides for delete-event */
  BST_ADIALOG_DESTROY_ON_DELETE	= (1 << 1),	/* always auto-hides for delete-event */
  BST_ADIALOG_POPUP_POS		= (1 << 2),
  BST_ADIALOG_MODAL		= (1 << 3),
  BST_ADIALOG_FORCE_HBOX	= (1 << 4)
} BstADialogFlags;

GtkType    bst_adialog_get_type		(void);
GtkWidget* bst_adialog_new       	(GtkObject		*alive_host,
					 GtkWidget	       **adialog_p,
					 GtkWidget		*child,
					 BstADialogFlags	 flags,
					 const gchar		*first_arg_name,
					 ...);
GtkWidget* bst_adialog_get_child	(GtkWidget		*adialog);
void	   bst_adialog_setup_choices	(GtkWidget		*adialog,
					 ...);


/* --- Gdk utilities & workarounds --- */
gboolean gdk_window_translate		(GdkWindow	*src_window,
					 GdkWindow	*dest_window,
					 gint		*x,
					 gint		*y);





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_UTILS_H__ */
