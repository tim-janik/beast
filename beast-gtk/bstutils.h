/* BEAST - Bedevilled Audio System
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
#ifndef __BST_UTILS_H__
#define __BST_UTILS_H__

#include <bsw/bsw.h>
#include <gtk/gtk.h>
#include "bstdefs.h"
#include "bstcluehunter.h"

G_BEGIN_DECLS

/* --- generated includes --- */
/* type IDs */
#include "bstgentypes.h"
/* marshallers */
#include "bstmarshal.h"
/* icon stock IDs */
#include "beast-gtk/icons/bst-stock-gen.h"


/* --- utility structs --- */
typedef struct {
  const gchar *cat_key;
  guint        tool_id;
  guint        flags;
} BstTool;


/* --- stock icon aliases --- */
#define	BST_STOCK_QUESTION		GTK_STOCK_DIALOG_QUESTION
#define	BST_STOCK_WARNING		GTK_STOCK_DIALOG_WARNING
#define	BST_STOCK_ERROR			GTK_STOCK_DIALOG_ERROR

#define BST_STOCK_NEW	                GTK_STOCK_NEW
#define BST_STOCK_OPEN	                GTK_STOCK_OPEN
#define BST_STOCK_MERGE	                GTK_STOCK_OPEN // FIXME
#define BST_STOCK_SAVE	                GTK_STOCK_SAVE
#define BST_STOCK_SAVE_AS	        GTK_STOCK_SAVE_AS
#define BST_STOCK_PREFERENCES	        GTK_STOCK_PREFERENCES
#define BST_STOCK_QUIT	                GTK_STOCK_QUIT
#define BST_STOCK_CLEAR_UNDO	        GTK_STOCK_DELETE // FIXME
#define BST_STOCK_PREFERENCES	        GTK_STOCK_PREFERENCES
#define BST_STOCK_PLAY	                GTK_STOCK_GO_FORWARD // FIXME
#define BST_STOCK_STOP	                GTK_STOCK_STOP
#define BST_STOCK_NEW_SONG	        GTK_STOCK_NEW // FIXME
#define BST_STOCK_NEW_CSYNTH	        GTK_STOCK_NEW // FIXME
#define BST_STOCK_NEW_MIDI_SYNTH	GTK_STOCK_NEW // FIXME
#define BST_STOCK_REMOVE_SYNTH	        GTK_STOCK_REMOVE // REMOVE
#define BST_STOCK_DOC_NEWS	        GTK_STOCK_NEW // FIXME
#define BST_STOCK_HELP	                GTK_STOCK_HELP
#define BST_STOCK_DOC_FAQ	        GTK_STOCK_NEW // FIXME
#define BST_STOCK_DOC_DEVEL	        GTK_STOCK_NEW // FIXME
#define BST_STOCK_ABOUT	                GTK_STOCK_YES // FIXME


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
#define	BST_PIXDATA_EMPTY1x1	"GdkP\0\0\0\34\1\1\0\2\0\0\0\4\0\0\0\1\0\0\0\1\0\0\0\0"

/* retrieve static icons (no reference count needs) */
GtkWidget*	bst_image_from_icon		(BseIcon	*icon,
						 GtkIconSize	 icon_size);


/* --- beast/bse specific extensions --- */
void		bst_status_eprintf		(BseErrorType	 error,
						 const gchar	*message_fmt,
						 ...) G_GNUC_PRINTF (2, 3);
void		bst_window_sync_title_to_proxy	(gpointer	 window,
						 SfiProxy	 proxy,
						 const gchar	*title_format);
const gchar**	bst_log_scan_keys		(void);


/* --- GUI field mask --- */
typedef struct _BstGMask BstGMask;
GtkWidget*   bst_gmask_container_create	(guint		border_width,
					 gboolean	dislodge_columns);
typedef enum /*< skip >*/
{
  BST_GMASK_FIT,
  BST_GMASK_FILL,
  BST_GMASK_INTERLEAVE, /* stretch */
  BST_GMASK_BIG,
  BST_GMASK_CENTER
} BstGMaskPack;
BstGMask*	bst_gmask_form		(GtkWidget     *gmask_container,
					 GtkWidget     *action,
					 BstGMaskPack   gpack);
#define		bst_gmask_form_big(c,a)	bst_gmask_form ((c), (a), BST_GMASK_BIG)
void		bst_gmask_set_tip	(BstGMask      *mask,
					 const gchar   *tip_text);
void		bst_gmask_set_prompt	(BstGMask      *mask,
					 gpointer	widget);
void		bst_gmask_set_aux1	(BstGMask      *mask,
					 gpointer	widget);
void		bst_gmask_set_aux2	(BstGMask      *mask,
					 gpointer	widget);
void		bst_gmask_set_aux3	(BstGMask      *mask,
					 gpointer	widget);
void		bst_gmask_set_ahead	(BstGMask      *mask,
					 gpointer	widget);
void		bst_gmask_set_atail	(BstGMask      *mask,
					 gpointer	widget);
void		bst_gmask_set_column	(BstGMask      *mask,
					 guint		column);
GtkWidget*	bst_gmask_get_prompt	(BstGMask      *mask);
GtkWidget*	bst_gmask_get_aux1	(BstGMask      *mask);
GtkWidget*	bst_gmask_get_aux2	(BstGMask      *mask);
GtkWidget*	bst_gmask_get_aux3	(BstGMask      *mask);
GtkWidget*	bst_gmask_get_ahead	(BstGMask      *mask);
GtkWidget*	bst_gmask_get_action	(BstGMask      *mask);
GtkWidget*	bst_gmask_get_atail	(BstGMask      *mask);
void		bst_gmask_foreach	(BstGMask      *mask,
					 gpointer	func,
					 gpointer	data);
void		bst_gmask_pack		(BstGMask      *mask);
BstGMask*	bst_gmask_quick		(GtkWidget     *gmask_container,
					 guint		column,
					 const gchar   *prompt,
					 gpointer       action,
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
void		bst_container_set_named_child	(GtkWidget	*container,
						 GQuark		 qname,
						 GtkWidget	*child);
GtkWidget*	bst_container_get_named_child	(GtkWidget	*container,
						 GQuark		 qname);
GtkWidget*	bst_xpm_view_create		(const gchar   **xpm,
						 GtkWidget	*colormap_widget);


/* --- internal --- */
void	_bst_init_utils		(void);

G_END_DECLS

#endif /* __BST_UTILS_H__ */
