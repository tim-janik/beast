/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999, 2000 Olaf Hoehmann and Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BST_CANVAS_SOURCE_H__
#define __BST_CANVAS_SOURCE_H__

#include	"bstdefs.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- Gtk+ type macros --- */
#define BST_TYPE_CANVAS_SOURCE            (bst_canvas_source_get_type ())
#define BST_CANVAS_SOURCE(object)         (GTK_CHECK_CAST ((object), BST_TYPE_CANVAS_SOURCE, BstCanvasSource))
#define BST_CANVAS_SOURCE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_CANVAS_SOURCE, BstCanvasSourceClass))
#define BST_IS_CANVAS_SOURCE(object)      (GTK_CHECK_TYPE ((object), BST_TYPE_CANVAS_SOURCE))
#define BST_IS_CANVAS_SOURCE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_CANVAS_SOURCE))
#define BST_CANVAS_SOURCE_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), BST_TYPE_CANVAS_SOURCE, BstCanvasSourceClass))


/* --- structures & typedefs --- */
typedef	struct	_BstCanvasSource	BstCanvasSource;
typedef	struct	_BstCanvasSourceClass	BstCanvasSourceClass;
struct _BstCanvasSource
{
  GnomeCanvasGroup parent_object;

  BswProxy	   source;

  GtkWidget	  *source_view;
  GtkWidget	  *source_info;

  GnomeCanvasItem *icon_item;
  GnomeCanvasItem *text;
  GSList	  *channel_items;

  guint		   swap_channels : 1;
  guint            in_move : 1;
  gdouble	   move_dx, move_dy;
};
struct _BstCanvasSourceClass
{
  GnomeCanvasGroupClass parent_class;

  void (*update_links)	(BstCanvasSource *source);
};


/* --- prototypes --- */
GtkType		 bst_canvas_source_get_type	(void);
GnomeCanvasItem* bst_canvas_source_new		(GnomeCanvasGroup *group,
						 BswProxy	   source,
						 gdouble           world_x,
						 gdouble           world_y);
void		 bst_canvas_source_update_links	(BstCanvasSource  *csource);
void		 bst_canvas_source_ochannel_pos	(BstCanvasSource  *csource,
						 guint             ochannel,
						 gdouble          *world_x,
						 gdouble          *world_y);
void		 bst_canvas_source_ichannel_pos	(BstCanvasSource  *csource,
						 guint             ichannel,
						 gdouble          *world_x,
						 gdouble          *world_y);
gboolean	 bst_canvas_source_is_jchannel	(BstCanvasSource  *csource,
						 guint             ichannel);
gboolean	 bst_canvas_source_ichannel_free(BstCanvasSource  *csource,
						 guint		   ichannel);
guint		 bst_canvas_source_ichannel_at  (BstCanvasSource  *csource,
						 gdouble           world_x,
						 gdouble           world_y);
guint		 bst_canvas_source_ochannel_at  (BstCanvasSource  *csource,
						 gdouble           world_x,
						 gdouble           world_y);
BstCanvasSource* bst_canvas_source_at  		(GnomeCanvas      *csource,
						 gdouble           world_x,
						 gdouble           world_y);
void		 bst_canvas_source_popup_view	(BstCanvasSource  *csource);
void		 bst_canvas_source_toggle_view	(BstCanvasSource  *canvas);
void		 bst_canvas_source_popup_info	(BstCanvasSource  *csource);
void		 bst_canvas_source_toggle_info	(BstCanvasSource  *canvas);

     
     


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_CANVAS_SOURCE_H__ */
