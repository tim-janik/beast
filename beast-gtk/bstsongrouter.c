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

#include	<math.h>



/* --- prototypes --- */
static void	bst_song_router_class_init	(BstSongRouterClass	*klass);
static void	bst_song_router_init		(BstSongRouter		*pe);
static void	bst_song_router_destroy		(GtkObject		*object);
static gint	tag_event			(BstSongRouter		*router,
						 GdkEvent		*event,
						 GnomeCanvasItem	*item);
static void	middle_arrow			(GnomeCanvasItem	*item,
						 GnomeCanvasItem	*line);


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
      
      song_router_type = gtk_type_unique (GTK_TYPE_VBOX, &song_router_info);
    }
  
  return song_router_type;
}

static void
bst_song_router_class_init (BstSongRouterClass *class)
{
  extern void gnome_type_init (void);
  GtkObjectClass *object_class;
  
  object_class = GTK_OBJECT_CLASS (class);
  
  bst_song_router_class = class;
  parent_class = gtk_type_class (GTK_TYPE_VBOX);
  
  object_class->destroy = bst_song_router_destroy;
  
  /* FIXME: */ gnome_type_init (); gdk_rgb_init ();
}

static void
bst_song_router_init (BstSongRouter *router)
{
  GtkWidget *widget;
  
  widget = GTK_WIDGET (router);
  
  router->song = NULL;
  router->canvas = NULL;
  router->root = NULL;
  router->line = NULL;
  router->rect = NULL;
  
  gtk_widget_set (widget,
		  "homogeneous", FALSE,
		  "spacing", 0,
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

void
bst_song_router_rebuild (BstSongRouter *router)
{
  GnomeCanvasPoints *points;
  
  g_return_if_fail (BST_IS_SONG_ROUTER (router));
  
  bst_song_router_destroy_contents (router);
  
  router->canvas = (GnomeCanvas*)
    gtk_widget_new (GNOME_TYPE_CANVAS,
		    "visible", TRUE,
		    "signal::destroy", gtk_widget_destroyed, &router->canvas,
		    "parent", gtk_widget_new (GTK_TYPE_SCROLLED_WINDOW,
					      "visible", TRUE,
					      "hscrollbar_policy", GTK_POLICY_ALWAYS,
					      "vscrollbar_policy", GTK_POLICY_ALWAYS,
					      "parent", router,
					      NULL),
		    NULL);
  //  router->canvas->aa = 1;
  router->root = gnome_canvas_root (router->canvas);
  bst_object_set (GTK_OBJECT (router->root),
		  "signal::destroy", gtk_widget_destroyed, &router->root,
		  NULL);
  
  router->line = gnome_canvas_item_new (router->root,
					GNOME_TYPE_CANVAS_LINE,
					"signal::destroy", gtk_widget_destroyed, &router->line,
					"fill_color", "black",
					"first_arrowhead", FALSE,
					"last_arrowhead", FALSE,
					NULL);
  bst_object_set (router->line,
		  "object_signal::args_changed",
		  middle_arrow,
		  gnome_canvas_item_new (router->root,
					 GNOME_TYPE_CANVAS_RECT,
					 "fill_color", "black",
					 "outline_color", "black",
					 NULL),
		  NULL);
  
  points = gnome_canvas_points_new (2);
  points->coords[0] = 0;
  points->coords[1] = 0;
  points->coords[2] = 50;
  points->coords[3] = 50;
  
  bst_object_set (router->line,
		  "points", points,
		  NULL);
  
  router->tag = gnome_canvas_item_new (router->root,
				       GNOME_TYPE_CANVAS_ELLIPSE,
				       "fill_color_rgba", 0x00000000,
				       "outline_color", "blue",
				       "width_pixels", 0,
				       "x1", points->coords[0] - BST_TAG_DIAMETER/2,
				       "y1", points->coords[1] - BST_TAG_DIAMETER/2,
				       "x2", points->coords[0] + BST_TAG_DIAMETER/2,
				       "y2", points->coords[1] + BST_TAG_DIAMETER/2,
				       "object_signal::event", tag_event, router,
				       "signal::destroy", gtk_widget_destroyed, &router->tag,
				       NULL);
  gnome_canvas_points_free (points);
  
  bst_song_router_update (router);
}

void
bst_song_router_update (BstSongRouter *router)
{
  g_return_if_fail (BST_IS_SONG_ROUTER (router));
}

#define	ARROW_LENGTH	(8)
#define	ARROW_WIDTH	(6)

static void
middle_arrow (GnomeCanvasItem *item,
	      GnomeCanvasItem *line)
{
  GnomeCanvasPoints *gpoints;
  gdouble dx, dy, l, x, y, cos_theta, sin_theta, *points;
  
  gtk_object_get (GTK_OBJECT (line), "points", &gpoints, NULL);
  if (!gpoints)
    gpoints = gnome_canvas_points_new0 (2);
  points = gpoints->coords;

  dx = points[2] - points[0];
  dy = points[3] - points[1];
  l = sqrt (dx * dx + dy * dy);
  x = (points[0] + points[2]) / 2;
  y = (points[1] + points[3]) / 2;

  sin_theta = dy / l;
  cos_theta = dx / l;

  gnome_canvas_points_free (gpoints);

  gpoints = gnome_canvas_points_new (2);
  points = gpoints->coords;

  points[0] = x - ARROW_WIDTH * sin_theta;
  points[1] = y - ARROW_WIDTH * cos_theta;
  points[2] = x + ARROW_WIDTH * sin_theta;
  points[3] = y + ARROW_WIDTH * cos_theta;

  // bst_object_set (item, "points", gpoints, NULL);
  bst_object_set (item,
		  "x1", points[0],
		  "y1", points[1],
		  "x2", points[2],
		  "y2", points[3],
		  NULL);
  gnome_canvas_points_free (gpoints);
}

static gint
tag_event (BstSongRouter   *router,
	   GdkEvent        *event,
	   GnomeCanvasItem *tag)
{
  GdkCursor *fleur;
  
  switch (event->type)
    {
    case GDK_ENTER_NOTIFY:
      bst_object_set (tag,
		      "fill_color_rgba", 0xff000080,
		      NULL);
      break;
    case GDK_LEAVE_NOTIFY:
      if (!(event->crossing.state & GDK_BUTTON1_MASK))
	bst_object_set (tag,
			"fill_color_rgba", 0x00000000,
			NULL);
      break;
    case GDK_BUTTON_PRESS:
      fleur = gdk_cursor_new (GDK_FLEUR);
      gnome_canvas_item_grab (tag,
			      GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			      fleur,
			      event->button.time);
      gdk_cursor_destroy (fleur);
      break;
    case GDK_MOTION_NOTIFY:
      if (event->motion.state & GDK_BUTTON1_MASK)
	{
	  gdouble x, y;
	  GnomeCanvasPoints *points;
	  GtkArg arg;
	  
	  x = event->motion.x;
	  y = event->motion.y;
	  bst_object_set (tag,
			  "x1", x - BST_TAG_DIAMETER / 2,
			  "y1", y - BST_TAG_DIAMETER / 2,
			  "x2", x + BST_TAG_DIAMETER / 2,
			  "y2", y + BST_TAG_DIAMETER / 2,
			  NULL);
	  
	  arg.name = "points";
	  gtk_object_getv (GTK_OBJECT (router->line), 1, &arg);
	  points = GTK_VALUE_POINTER (arg);
	  points->coords[0] = x;
	  points->coords[1] = y;
	  bst_object_set (router->line,
			  "points", points,
			  NULL);
	  gnome_canvas_points_free (points);
	}
      break;
    case GDK_BUTTON_RELEASE:
      gnome_canvas_item_ungrab (tag, event->button.time);
      break;
    default:
      break;
    }
  
  return FALSE;
}
