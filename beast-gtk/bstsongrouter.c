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
#include "bstsongrouter.h"

#include <math.h>
#include "bstzoomedwindow.h"
#include "bstcanvassource.h"
#include "gnomeforest.h"
#include "bstcanvaslink.h"


/* --- prototypes --- */
static void	  bst_song_router_class_init	(BstSongRouterClass	*klass);
static void	  bst_song_router_init		(BstSongRouter		*router);
static void	  bst_song_router_destroy	(GtkObject		*object);
static GtkWidget* bst_song_router_build_toolbar	(BstSongRouter		*router);
static gboolean   bst_song_router_adjust_region	(BstSongRouter		*router,
						 gboolean                zoom_in);


/* --- static variables --- */
static gpointer            parent_class = NULL;
static BstSongRouterClass *bst_song_router_class = NULL;


/* --- functions --- */
GtkType
bst_song_router_get_type (void)
{
  static GtkType song_router_type = 0;
  
  if (!song_router_type)
    {
      GtkTypeInfo song_router_info =
      {
	"BstSongRouter",
	sizeof (BstSongRouter),
	sizeof (BstSongRouterClass),
	(GtkClassInitFunc) bst_song_router_class_init,
	(GtkObjectInitFunc) bst_song_router_init,
        /* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      song_router_type = gtk_type_unique (GTK_TYPE_HBOX, &song_router_info);
    }
  
  return song_router_type;
}

static void
bst_song_router_class_init (BstSongRouterClass *class)
{
  GtkObjectClass *object_class;
  
  object_class = GTK_OBJECT_CLASS (class);
  
  bst_song_router_class = class;
  parent_class = gtk_type_class (GTK_TYPE_HBOX);
  
  object_class->destroy = bst_song_router_destroy;
}

static void
bst_song_router_init (BstSongRouter *router)
{
  GtkWidget *widget;
  
  widget = GTK_WIDGET (router);
  
  router->song = NULL;
  router->canvas = NULL;
  router->root = NULL;
  
  gtk_widget_set (widget,
		  "homogeneous", FALSE,
		  "spacing", 3,
		  "border_width", 5,
		  NULL);
}

static void
bst_song_router_destroy_contents (BstSongRouter *router)
{
  gtk_container_foreach (GTK_CONTAINER (router), (GtkCallback) gtk_widget_destroy, NULL);
}

static void
bst_song_router_destroy (GtkObject *object)
{
  BstSongRouter *router;
  
  g_return_if_fail (object != NULL);
  
  router = BST_SONG_ROUTER (object);
  
  bst_song_router_destroy_contents (router);
  
  router->song = NULL;
  
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget*
bst_song_router_new (BseSong *song)
{
  GtkWidget *router;
  
  g_return_val_if_fail (BSE_IS_SONG (song), NULL);
  
  router = gtk_widget_new (BST_TYPE_SONG_ROUTER, NULL);
  BST_SONG_ROUTER (router)->song = song;
  
  bst_song_router_rebuild (BST_SONG_ROUTER (router));
  
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
					     zoom_xpm);
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
bst_song_router_rebuild (BstSongRouter *router)
{
  GtkWidget *toolbar;
  GnomeCanvasPoints *points;
  GnomeCanvasItem *link;
  GnomeCanvasItem *item;
  
  g_return_if_fail (BST_IS_SONG_ROUTER (router));
  
  bst_song_router_destroy_contents (router);

  toolbar = bst_song_router_build_toolbar (router);
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
					    "object_signal::zoom", bst_song_router_adjust_region, router,
					    "signal::realize", zoomed_add_xpm, NULL,
					    NULL),
		  NULL);
  
  points = gnome_canvas_points_new (2);
  points->coords[0] = 0;
  points->coords[1] = 0;
  points->coords[2] = 50;
  points->coords[3] = 50;
  gnome_canvas_points_free (points);
  
  bst_song_router_update (router);

  link = bst_canvas_link_new (router->root);
  bst_canvas_link_set_start (BST_CANVAS_LINK (link),
			     bst_canvas_source_new (router->root, NULL));
  item = bst_canvas_source_new (router->root, NULL);
  bst_canvas_link_set_end (BST_CANVAS_LINK (link), item);
  link = bst_canvas_link_new (router->root);
  bst_canvas_link_set_start (BST_CANVAS_LINK (link), item);
  bst_canvas_link_set_end (BST_CANVAS_LINK (link),
			   bst_canvas_source_new (router->root, NULL));

  bst_song_router_adjust_region (router, TRUE);
}

void
bst_song_router_update (BstSongRouter *router)
{
  g_return_if_fail (BST_IS_SONG_ROUTER (router));
}

static GtkWidget*
add_icon_button (GtkBox    *box,
		 BseIcon   *icon,
		 GtkWidget *last_button)
{
  GtkWidget *button, *forest;

  button = gtk_widget_new (GTK_TYPE_RADIO_BUTTON,
			   "visible", TRUE,
			   "group", last_button,
			   "can_focus", FALSE,
			   "draw_indicator", FALSE,
			   "relief", GTK_RELIEF_NONE,
			   NULL);
  gtk_box_pack_start (box, button, FALSE, TRUE, 0);
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
bst_song_router_build_toolbar (BstSongRouter *router)
{
  GtkBox *box;
  GtkWidget *bar;
  GtkWidget *radio = NULL;
  
  g_return_val_if_fail (BST_IS_SONG_ROUTER (router), NULL);

  bar = gtk_widget_new (GTK_TYPE_VBOX,
			"visible", TRUE,
			"homogeneous", FALSE,
			"spacing", 5,
			NULL);
  box = GTK_BOX (bar);

  return gtk_widget_new (GTK_TYPE_FRAME,
			 "label", NULL,
			 "visible", TRUE,
			 "child", bar,
			 "shadow", GTK_SHADOW_OUT,
			 NULL);
}

static gboolean
bst_song_router_adjust_region (BstSongRouter *router,
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
