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
#include "bstsnetrouter.h"

#include <math.h>
#include "bstzoomedwindow.h"
#include "bstcanvassource.h"
#include "bstcanvaslink.h"


/* --- prototypes --- */
static void	  bst_snet_router_class_init	(BstSNetRouterClass	*klass);
static void	  bst_snet_router_init		(BstSNetRouter		*router,
						 BstSNetRouterClass     *class);
static void	  bst_snet_router_destroy	(GtkObject		*object);
static GtkWidget* bst_snet_router_build_toolbar	(BstSNetRouter		*router);
static gboolean   bst_snet_router_adjust_region	(BstSNetRouter		*router,
						 gboolean                zoom_in);


/* --- static variables --- */
static gpointer            parent_class = NULL;
static BstSNetRouterClass *bst_snet_router_class = NULL;


/* --- functions --- */
GtkType
bst_snet_router_get_type (void)
{
  static GtkType snet_router_type = 0;
  
  if (!snet_router_type)
    {
      GtkTypeInfo snet_router_info =
      {
	"BstSNetRouter",
	sizeof (BstSNetRouter),
	sizeof (BstSNetRouterClass),
	(GtkClassInitFunc) bst_snet_router_class_init,
	(GtkObjectInitFunc) bst_snet_router_init,
        /* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      snet_router_type = gtk_type_unique (GTK_TYPE_VBOX, &snet_router_info);
    }
  
  return snet_router_type;
}

static void
bst_snet_router_class_init (BstSNetRouterClass *class)
{
  GtkObjectClass *object_class;
  
  object_class = GTK_OBJECT_CLASS (class);
  
  bst_snet_router_class = class;
  parent_class = gtk_type_class (GTK_TYPE_VBOX);
  
  object_class->destroy = bst_snet_router_destroy;
  class->tooltips = NULL;
}

static void
bst_snet_router_init (BstSNetRouter      *router,
		      BstSNetRouterClass *class)
{
  GtkWidget *widget;
  
  widget = GTK_WIDGET (router);
  
  router->snet = NULL;
  router->canvas = NULL;
  router->root = NULL;
  
  gtk_widget_set (widget,
		  "homogeneous", FALSE,
		  "spacing", 3,
		  "border_width", 5,
		  NULL);

  if (!class->tooltips)
    {
      class->tooltips = gtk_tooltips_new ();
      gtk_object_ref (GTK_OBJECT (class->tooltips));
      gtk_object_sink (GTK_OBJECT (class->tooltips));
      gtk_signal_connect (GTK_OBJECT (class->tooltips),
			  "destroy",
			  gtk_widget_destroyed,
			  &class->tooltips);
    }
  else
    gtk_object_ref (GTK_OBJECT (class->tooltips));
}

static void
bst_snet_router_destroy_contents (BstSNetRouter *router)
{
  gtk_container_foreach (GTK_CONTAINER (router), (GtkCallback) gtk_widget_destroy, NULL);
}

static void
bst_snet_router_destroy (GtkObject *object)
{
  BstSNetRouter *router = BST_SNET_ROUTER (object);
  
  bst_snet_router_destroy_contents (router);
  
  router->snet = NULL;

  gtk_object_unref (GTK_OBJECT (BST_SNET_ROUTER_GET_CLASS (router)->tooltips));
  
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget*
bst_snet_router_new (BseSNet *snet)
{
  GtkWidget *router;
  
  g_return_val_if_fail (BSE_IS_SNET (snet), NULL);
  
  router = gtk_widget_new (BST_TYPE_SNET_ROUTER, NULL);
  BST_SNET_ROUTER (router)->snet = snet;
  
  bst_snet_router_rebuild (BST_SNET_ROUTER (router));
  
  return router;
}

static void
zoomed_add_xpm (BstZoomedWindow *zoomed)
{
  if (!GTK_BIN (zoomed->toggle_button)->child)
    {
      GtkWidget *widget = GTK_WIDGET (zoomed);
      GdkPixmap *pixmap;
      GdkBitmap *mask;
      GtkWidget *pix;
      static const gchar *zoom_xpm[] = {
	"12 12 2 1", "  c None", "# c #000000",
	"####  ####  ",
	"##      ##  ",
	"# #    # #  ",
	"#  ####  #  ",
	"   #  #     ",
	"   #  #     ",
	"#  ####  #  ",
	"# #    # #  ",
	"##      ##  ",
	"####  ####  ",
	"            ",
	"            ",
      };
      
      pixmap = gdk_pixmap_create_from_xpm_d (widget->window,
					     &mask,
					     NULL,
					     (gchar*) zoom_xpm);
      pix = gtk_pixmap_new (pixmap, mask);
      gdk_pixmap_unref (pixmap);
      gdk_pixmap_unref (mask);

      gtk_widget_set (pix,
		      "visible", TRUE,
		      "parent", zoomed->toggle_button,
		      NULL);
    }
}

void
bst_snet_router_rebuild (BstSNetRouter *router)
{
  GtkWidget *toolbar;
  GnomeCanvasPoints *points;
  GnomeCanvasItem *link;
  GnomeCanvasItem *item;
  
  g_return_if_fail (BST_IS_SNET_ROUTER (router));
  
  bst_snet_router_destroy_contents (router);

  toolbar = bst_snet_router_build_toolbar (router);
  gtk_box_pack_start (GTK_BOX (router), toolbar, FALSE, TRUE, 0);
  
  router->canvas = (GnomeCanvas*) gnome_canvas_new_aa ();
  router->root = gnome_canvas_root (router->canvas);
  bst_object_set (GTK_OBJECT (router->root),
		  "signal::destroy", gtk_widget_destroyed, &router->root,
		  NULL);

  gtk_widget_set (GTK_WIDGET (router->canvas),
		  "visible", TRUE,
		  "signal::destroy", gtk_widget_destroyed, &router->canvas,
		  "parent", gtk_widget_new (BST_TYPE_ZOOMED_WINDOW,
					    "visible", TRUE,
					    "hscrollbar_policy", GTK_POLICY_ALWAYS,
					    "vscrollbar_policy", GTK_POLICY_ALWAYS,
					    "parent", router,
					    "object_signal::zoom", bst_snet_router_adjust_region, router,
					    "signal::realize", zoomed_add_xpm, NULL,
					    NULL),
		  NULL);
  
  points = gnome_canvas_points_new (2);
  points->coords[0] = 0;
  points->coords[1] = 0;
  points->coords[2] = 50;
  points->coords[3] = 50;
  gnome_canvas_points_free (points);
  
  bst_snet_router_update (router);

  link = bst_canvas_link_new (router->root);
  bst_canvas_link_set_start (BST_CANVAS_LINK (link),
			     bst_canvas_source_new (router->root, NULL));
  item = bst_canvas_source_new (router->root, NULL);
  bst_canvas_link_set_end (BST_CANVAS_LINK (link), item);
  link = bst_canvas_link_new (router->root);
  bst_canvas_link_set_start (BST_CANVAS_LINK (link), item);
  bst_canvas_link_set_end (BST_CANVAS_LINK (link),
			   bst_canvas_source_new (router->root, NULL));

  bst_snet_router_adjust_region (router, TRUE);
}

void
bst_snet_router_update (BstSNetRouter *router)
{
  g_return_if_fail (BST_IS_SNET_ROUTER (router));
}

static GtkWidget*
add_icon_button (GtkWidget     *parent,
		 const BseIcon *icon,
		 GtkWidget     *last_button)
{
  GtkWidget *button, *forest;

  button = gtk_widget_new (GTK_TYPE_RADIO_BUTTON,
			   "visible", TRUE,
			   "group", last_button,
			   "can_focus", FALSE,
			   "draw_indicator", FALSE,
			   "relief", GTK_RELIEF_NONE,
			   NULL);
  gtk_container_add_with_args (GTK_CONTAINER (parent), button,
			       "hexpand", FALSE,
			       NULL);
  forest = gtk_widget_new (GNOME_TYPE_FOREST,
			   "visible", TRUE,
			   "parent", button,
			   "width", 32,
			   "height", 32,
			   NULL);
  gnome_forest_put_sprite (GNOME_FOREST (forest), 1,
			   (icon->bytes_per_pixel > 3
			    ? art_pixbuf_new_const_rgba
			    : art_pixbuf_new_const_rgb) (icon->pixels,
							 icon->width,
							 icon->height,
							 icon->width *
							 icon->bytes_per_pixel));
  gnome_forest_set_sprite_size (GNOME_FOREST (forest), 1, 32, 32);

  return button;
}

static GtkWidget*
add_category_button (GtkWidget	   *parent,
		     GtkTooltips   *tooltips,
		     BseCategory   *category,
		     const BseIcon *icon,
		     GtkWidget     *last_button)
{
  GtkWidget *button, *forest;
  gchar *tip;

  button = gtk_widget_new (GTK_TYPE_RADIO_BUTTON,
			   "visible", TRUE,
			   "group", last_button,
			   "can_focus", FALSE,
			   "draw_indicator", FALSE,
			   "relief", GTK_RELIEF_NONE,
			   NULL);
  gtk_container_add_with_args (GTK_CONTAINER (parent), button,
			       "hexpand", FALSE,
			       NULL);
  forest = gtk_widget_new (GNOME_TYPE_FOREST,
			   "visible", TRUE,
			   "parent", button,
			   "width", 32,
			   "height", 32,
			   NULL);
  gnome_forest_put_sprite (GNOME_FOREST (forest), 1,
			   (icon->bytes_per_pixel > 3
			    ? art_pixbuf_new_const_rgba
			    : art_pixbuf_new_const_rgb) (icon->pixels,
							 icon->width,
							 icon->height,
							 icon->width *
							 icon->bytes_per_pixel));
  gnome_forest_set_sprite_size (GNOME_FOREST (forest), 1, 32, 32);

  tip = g_strconcat (bse_type_name (category->type),
		     " [",
		     category->category + category->mindex + 1,
		     "]\n",
		     bse_type_blurb (category->type),
		     NULL);
  gtk_tooltips_set_tip (tooltips, button, tip, NULL);
  g_free (tip);

  return button;
}

static GtkWidget*
bst_snet_router_build_toolbar (BstSNetRouter *router)
{
  static BseIcon *fallback_icon = NULL;
  GtkWidget *bar;
  GtkWidget *radio = NULL;
  guint i;
  BseCategory *cats;
  guint n_cats;
  
  g_return_val_if_fail (BST_IS_SNET_ROUTER (router), NULL);

  if (!fallback_icon)
    {
#include "./icons/noicon.c"
      static const BsePixdata noicon_pixdata = {
	NOICON_PIXDATA_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
	NOICON_PIXDATA_WIDTH, NOICON_PIXDATA_HEIGHT,
	NOICON_PIXDATA_RLE_PIXEL_DATA,
      };

      fallback_icon = bse_icon_from_pixdata (&noicon_pixdata);
    }

  bar = gtk_widget_new (GTK_TYPE_HWRAP_BOX,
			"visible", TRUE,
			"homogeneous", FALSE,
			//			"spacing", 5,
			NULL);

  if (1)
    {
#include "./icons/mouse_tool.c"
      BsePixdata pd = {
	MOUSE_TOOL_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
	MOUSE_TOOL_IMAGE_WIDTH,
	MOUSE_TOOL_IMAGE_HEIGHT,
	MOUSE_TOOL_IMAGE_RLE_PIXEL_DATA,
      };
      
      radio = add_icon_button (bar, bse_icon_from_pixdata (&pd), radio);
    }

  cats = bse_categories_match ("/Source/*", &n_cats);
  for (i = 0; i < n_cats; i++)
    radio = add_category_button (bar,
				 BST_SNET_ROUTER_GET_CLASS (router)->tooltips,
				 cats + i,
				 cats[i].icon ? cats[i].icon : fallback_icon,
				 radio);
  g_free (cats);


#define X(i) BseIcon icon = { i.bytes_per_pixel, i.width, i.height, (void*) i.pixel_data };
  if (1)
    {
#include "../bse/icons/song.c"
      X (gimp_image); radio = add_icon_button (bar, &icon, radio);
    }

  return gtk_widget_new (GTK_TYPE_FRAME,
			 "label", NULL,
			 "visible", TRUE,
			 "child", bar,
			 "shadow", GTK_SHADOW_OUT,
			 NULL);
}

static gboolean
bst_snet_router_adjust_region (BstSNetRouter *router,
			       gboolean       zoom_in)
{
  GnomeCanvasItem *root = GNOME_CANVAS_ITEM (router->root);
  gdouble x1, y1, x2, y2;

  gnome_canvas_request_full_update (router->canvas);
  gnome_canvas_update_now (router->canvas);
  gnome_canvas_item_get_bounds (root, &x1, &y1, &x2, &y2);

  /* add fudge */
  x1 -= 1; y1 -= 1; x2 += 1;  y2 += 1;

  gnome_canvas_set_scroll_region (router->canvas, x1, y1, x2, y2);
  gnome_canvas_request_full_update (router->canvas);
  
  return FALSE;
}
