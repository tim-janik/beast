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
#ifndef __BST_UTILS_H__
#define __BST_UTILS_H__

#include        <bsw/bsw.h>
#include        <gtk/gtk.h>
#include        "bstdefs.h"
#include        "bstcluehunter.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* --- generated includes --- */
/* type IDs */
#include "bstgentypes.h"
/* marshallers */
#include "bstmarshal.h"
/* icon stock IDs */
#include "./icons/bst-stock-gen.h"


/* --- stock icon aliases --- */
#define	BST_STOCK_QUESTION		GTK_STOCK_DIALOG_QUESTION
#define	BST_STOCK_WARNING		GTK_STOCK_DIALOG_WARNING
#define	BST_STOCK_ERROR			GTK_STOCK_DIALOG_ERROR


/* --- stock actions and aliases --- */
#define	BST_STOCK_NONE			(NULL)
#define BST_STOCK_APPLY			GTK_STOCK_APPLY
#define BST_STOCK_CANCEL		GTK_STOCK_CANCEL
#define	BST_STOCK_CDROM			GTK_STOCK_CDROM
#define BST_STOCK_CLONE			("bst-stock-clone")
#define BST_STOCK_CLOSE			GTK_STOCK_CLOSE
#define BST_STOCK_DEFAULT_REVERT	("bst-stock-default-revert")
#define	BST_STOCK_DELETE		GTK_STOCK_DELETE
#define BST_STOCK_EXECUTE		GTK_STOCK_EXECUTE
#define BST_STOCK_OK			GTK_STOCK_OK
#define BST_STOCK_OVERWRITE		("bst-stock-overwrite")
#define	BST_STOCK_PROPERTIES		GTK_STOCK_PROPERTIES
#define BST_STOCK_REDO			GTK_STOCK_REDO
#define BST_STOCK_REVERT		("bst-stock-revert")
#define BST_STOCK_UNDO			GTK_STOCK_UNDO
#define	BST_STOCK_ZOOM_100		GTK_STOCK_ZOOM_100
#define	BST_STOCK_ZOOM_FIT		GTK_STOCK_ZOOM_FIT
#define	BST_STOCK_ZOOM_IN		GTK_STOCK_ZOOM_IN
#define	BST_STOCK_ZOOM_OUT		GTK_STOCK_ZOOM_OUT


/* --- stock icon sizes --- */
#define	BST_SIZE_BUTTON			GXK_SIZE_BUTTON
#define	BST_SIZE_BIG_BUTTON		GXK_SIZE_BIG_BUTTON
#define	BST_SIZE_CANVAS			GXK_SIZE_CANVAS
#define	BST_SIZE_TOOLBAR		GXK_SIZE_TOOLBAR
#define	BST_SIZE_MENU			GXK_SIZE_MENU
#define	BST_SIZE_INFO_SIGN		GXK_SIZE_INFO_SIGN
#define	BST_SIZE_PALETTE		GXK_SIZE_PALETTE


/* --- pixbuf shortcuts --- */
#define bst_pixbuf_no_icon()	gxk_stock_fallback_pixbuf (BST_STOCK_NO_ICON)
#define	bst_pixbuf_knob()	gxk_stock_fallback_pixbuf (BST_STOCK_KNOB)


/* retrieve static icons (no reference count needs) */
GtkWidget*	bst_image_from_icon		(BseIcon	*icon,
						 GtkIconSize	 icon_size);


/* --- beast/bsw specific extensions --- */
void		bst_status_eprintf		(BseErrorType	 error,
						 const gchar	*message_fmt,
						 ...) G_GNUC_PRINTF (2, 3);
void		bst_window_sync_title_to_proxy	(gpointer	 window,
						 SfiProxy	 proxy,
						 const gchar	*title_format);


/* --- Gtk+ utilities & workarounds --- */
#define    GTK_TYPE_VPANED               (gtk_vpaned_get_type ())
#define    GTK_TYPE_HPANED               (gtk_hpaned_get_type ())
gboolean   gtk_widget_viewable		 (GtkWidget		*widget);
void	   bst_widget_request_aux_info	 (GtkWidget		*viewport);
void	   gtk_file_selection_heal	 (GtkFileSelection	*fs);
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
void   gtk_tree_view_append_text_columns (GtkTreeView		*tree_view,
					  guint			 n_cols,
					  ...);

#define gtk_notebook_current_widget(n) \
    gtk_notebook_get_nth_page ((n), gtk_notebook_get_current_page ((n)))


/* --- GUI field mask --- */
typedef struct _BstGMask BstGMask;
GtkWidget*   bst_gmask_container_create	(gpointer	tooltips,
					 guint		border_width,
					 gboolean	dislodge_columns);
typedef enum /*< skip >*/
{
  BST_GMASK_FIT,
  BST_GMASK_FILL,
  BST_GMASK_INTERLEAVE, /* stretch */
  BST_GMASK_BIG
} BstGMaskPack;
gpointer	bst_gmask_form		(GtkWidget     *gmask_container,
					 GtkWidget     *action,
					 BstGMaskPack   gpack);
#define		bst_gmask_form_big(c,a)	bst_gmask_form ((c), (a), BST_GMASK_BIG)
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
		       (sensitive) ? gxk_widget_make_sensitive : gxk_widget_make_insensitive, \
		       NULL)
#define	bst_gmask_destroy(mask)				\
    bst_gmask_foreach ((mask), gtk_widget_destroy, NULL)
#define	bst_gmask_ref		g_object_ref
#define	bst_gmask_unref		g_object_unref


/* --- BEAST utilities --- */
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


/* --- internal --- */
void	_bst_init_utils		(void);





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_UTILS_H__ */
