// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_SNET_ROUTER_H__
#define __BST_SNET_ROUTER_H__

#include	"bstcanvassource.hh"


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
void		 bst_snet_router_adjust_region	      (BstSNetRouter *snet_router);
BstCanvasSource* bst_snet_router_csource_from_source  (BstSNetRouter *snet_router,
						       SfiProxy       source);
BstSNetRouter*	 bst_snet_router_build_page	      (SfiProxy	     snet);


G_END_DECLS

#endif /* __BST_SNET_ROUTER_H__ */
