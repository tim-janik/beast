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
#ifndef __BST_SNET_ROUTER_H__
#define __BST_SNET_ROUTER_H__

#include	"bstdefs.h"
#include	"bstcanvassource.h"
#include	"bstradiotools.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- Gtk+ type macros --- */
#define	BST_TYPE_SNET_ROUTER		(bst_snet_router_get_type ())
#define	BST_SNET_ROUTER(object)		(GTK_CHECK_CAST ((object), BST_TYPE_SNET_ROUTER, BstSNetRouter))
#define	BST_SNET_ROUTER_CLASS(klass)	(GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_SNET_ROUTER, BstSNetRouterClass))
#define	BST_IS_SNET_ROUTER(object)	(GTK_CHECK_TYPE ((object), BST_TYPE_SNET_ROUTER))
#define	BST_IS_SNET_ROUTER_CLASS(klass)	(GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_SNET_ROUTER))
#define BST_SNET_ROUTER_GET_CLASS(obj)	(GTK_CHECK_GET_CLASS ((obj), BST_TYPE_SNET_ROUTER, BstSNetRouterClass))


/* --- structures & typedefs --- */
typedef	struct	_BstSNetRouter		BstSNetRouter;
typedef	struct	_BstSNetRouterClass	BstSNetRouterClass;
struct _BstSNetRouter
{
  GnomeCanvas	    parent_object;

  GtkWidget	   *toolbar;
  GtkWidget	   *palette;
  GtkAdjustment    *adjustment;

  BswProxy	    snet;

  BstRadioTools    *rtools;

  gdouble           world_x, world_y;
  guint		    reshow_palette : 1;
  guint		    drag_is_input : 1;
  guint             drag_channel;
  BstCanvasSource  *drag_csource;
  GnomeCanvasItem  *tmp_line;
  GSList           *link_list;
};
struct _BstSNetRouterClass
{
  GnomeCanvasClass parent_class;
};


/* --- prototypes --- */
GtkType		 bst_snet_router_get_type	     (void);
GtkWidget*	 bst_snet_router_new		     (BswProxy	     snet);
void		 bst_snet_router_set_snet 	     (BstSNetRouter *router,
						      BswProxy       snet);
void		 bst_snet_router_update		     (BstSNetRouter *snet_router);
void		 bst_snet_router_rebuild	     (BstSNetRouter *snet_router);
void		 bst_snet_router_adjust_region	     (BstSNetRouter *snet_router);
BstCanvasSource* bst_snet_router_csource_from_source (BstSNetRouter *snet_router,
						      BswProxy       source);
void		 bst_snet_router_toggle_palette	     (BstSNetRouter *snet_router);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_SNET_ROUTER_H__ */
