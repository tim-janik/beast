/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BST_CANVAS_LINK_H__
#define __BST_CANVAS_LINK_H__

#include	"bstdefs.h"
#include	"bstcanvassource.h"


#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */


/* --- Gtk+ type macros --- */
#define BST_TYPE_CANVAS_LINK            (bst_canvas_link_get_type ())
#define BST_CANVAS_LINK(object)         (GTK_CHECK_CAST ((object), BST_TYPE_CANVAS_LINK, BstCanvasLink))
#define BST_CANVAS_LINK_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_CANVAS_LINK, BstCanvasLinkClass))
#define BST_IS_CANVAS_LINK(object)      (GTK_CHECK_TYPE ((object), BST_TYPE_CANVAS_LINK))
#define BST_IS_CANVAS_LINK_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_CANVAS_LINK))
#define BST_CANVAS_LINK_GET_CLASS(obj)  ((BstCanvasLinkClass*) (((GtkObject*) (obj))->klass))


/* --- structures & typedefs --- */
typedef	struct	_BstCanvasLink		BstCanvasLink;
typedef	struct	_BstCanvasLinkClass	BstCanvasLinkClass;
struct _BstCanvasLink
{
  GnomeCanvasGroup parent_object;

  GnomeCanvasItem *line;
  GnomeCanvasItem *arrow;
  GnomeCanvasItem *tag_start, *tag_end;

  BstCanvasSource *ocsource;
  guint            ochannel_id;
  guint            oc_handler;
  BstCanvasSource *icsource;
  guint            ichannel_id;
  guint            ic_handler;

  guint		   in_move : 1;
  gdouble          start_move_dx, start_move_dy;
  gdouble          end_move_dx, end_move_dy;

  GtkWidget	  *link_view;
};
struct _BstCanvasLinkClass
{
  GnomeCanvasGroupClass parent_class;
};


/* --- prototypes --- */
GtkType		 bst_canvas_link_get_type		(void);
GnomeCanvasItem* bst_canvas_link_new			(GnomeCanvasGroup *group);
void		 bst_canvas_link_set_ocsource		(BstCanvasLink    *clink,
							 BstCanvasSource  *ocsource,
							 guint             ochannel_id);
void		 bst_canvas_link_set_icsource		(BstCanvasLink    *clink,
							 BstCanvasSource  *icsource,
							 guint             ichannel_id);
void             bst_canvas_link_popup_view		(BstCanvasLink	  *clink);
void             bst_canvas_link_toggle_view		(BstCanvasLink	  *clink);
BstCanvasLink*	 bst_canvas_link_at			(GnomeCanvas      *canvas,
							 gdouble           world_x,
							 gdouble           world_y);
BstCanvasSource* bst_canvas_link_has_canvas_source_at	(BstCanvasLink	  *clink,
							 gdouble           world_x,
							 gdouble           world_y);




#ifdef __cplusplus
#pragma {
}
#endif /* __cplusplus */

#endif /* __BST_CANVAS_LINK_H__ */
