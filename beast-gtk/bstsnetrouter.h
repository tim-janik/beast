/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2003 Tim Janik
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

#include	"bstcanvassource.h"


G_BEGIN_DECLS

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

  GtkMenu          *canvas_popup;
  GtkWidget	   *palette;
  GtkWidget	   *palette_text;
  GtkAdjustment    *adjustment;

  SfiProxy	    snet;

  GxkActionGroup   *canvas_tool;
  GxkActionGroup   *channel_toggle;

  gdouble           world_x, world_y;
  guint		    reshow_palette : 1;
  guint		    drag_is_input : 1;
  guint             drag_channel;
  BstCanvasSource  *drag_csource;
  GnomeCanvasItem  *tmp_line;
  SfiRing          *canvas_links;
};
struct _BstSNetRouterClass
{
  GnomeCanvasClass parent_class;

  GtkItemFactory  *popup_factory;
};


/* --- prototypes --- */
GType		 bst_snet_router_get_type	      (void);
GtkWidget*	 bst_snet_router_new		      (SfiProxy	     snet);
void		 bst_snet_router_set_snet 	      (BstSNetRouter *router,
						       SfiProxy       snet);
void		 bst_snet_router_update		      (BstSNetRouter *snet_router);
void		 bst_snet_router_rebuild	      (BstSNetRouter *snet_router);
void		 bst_snet_router_adjust_region	      (BstSNetRouter *snet_router);
BstCanvasSource* bst_snet_router_csource_from_source  (BstSNetRouter *snet_router,
						       SfiProxy       source);
BstSNetRouter*	 bst_snet_router_build_page	      (SfiProxy	     snet);


G_END_DECLS

#endif /* __BST_SNET_ROUTER_H__ */
