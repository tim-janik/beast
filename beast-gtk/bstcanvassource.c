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
#include "bstcanvassource.h"


/* --- defines --- */
#define CONNECTOR_WIDTH	(10)
#define TEXT_HEIGHT	(13)
#define	ICON_WIDTH	(64 + 2)
#define	ICON_HEIGHT	(64 + 2)


/* --- prototypes --- */
static void	bst_canvas_source_class_init	(BstCanvasSourceClass	*class);
static void	bst_canvas_source_init		(BstCanvasSource	*csource);
static void	bst_canvas_source_build		(BstCanvasSource	*csource);
static void	bst_canvas_source_destroy	(GtkObject		*object);
static gboolean bst_canvas_source_event		(GnomeCanvasItem        *item,
						 GdkEvent               *event);
static gboolean bst_canvas_source_child_event	(GnomeCanvasItem        *item,
						 GdkEvent               *event);


/* --- static variables --- */
static gpointer              parent_class = NULL;
static BstCanvasSourceClass *bst_canvas_source_class = NULL;


/* --- functions --- */
GtkType
bst_canvas_source_get_type (void)
{
  static GtkType canvas_source_type = 0;
  
  if (!canvas_source_type)
    {
      GtkTypeInfo canvas_source_info =
      {
	"BstCanvasSource",
	sizeof (BstCanvasSource),
	sizeof (BstCanvasSourceClass),
	(GtkClassInitFunc) bst_canvas_source_class_init,
	(GtkObjectInitFunc) bst_canvas_source_init,
        /* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      canvas_source_type = gtk_type_unique (GNOME_TYPE_CANVAS_GROUP, &canvas_source_info);
      gtk_type_set_chunk_alloc (canvas_source_type, 8);
    }
  
  return canvas_source_type;
}

static void
bst_canvas_source_class_init (BstCanvasSourceClass *class)
{
  GtkObjectClass *object_class;
  GnomeCanvasItemClass *canvas_item_class;
  GnomeCanvasGroupClass *canvas_group_class;
  
  object_class = GTK_OBJECT_CLASS (class);
  canvas_item_class = GNOME_CANVAS_ITEM_CLASS (class);
  canvas_group_class = GNOME_CANVAS_GROUP_CLASS (class);
  
  bst_canvas_source_class = class;
  parent_class = gtk_type_class (GNOME_TYPE_CANVAS_GROUP);
  
  object_class->destroy = bst_canvas_source_destroy;
  canvas_item_class->event = bst_canvas_source_event;
}

static void
bst_canvas_source_init (BstCanvasSource *csource)
{
  // GtkObject *object = GTK_OBJECT (csource);
  
  csource->rect = NULL;
  csource->move_dx = 0;
  csource->move_dy = 0;
}

static void
bst_canvas_source_destroy (GtkObject *object)
{
  // BstCanvasSource *csource = BST_CANVAS_SOURCE (object);
  GnomeCanvasGroup *group = GNOME_CANVAS_GROUP (object);
  
  while (group->item_list)
    gtk_object_destroy (group->item_list->data);
  
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GnomeCanvasItem*
bst_canvas_source_new (GnomeCanvasGroup *group,
		       BseSource        *source)
{
  GnomeCanvasItem *item;
  
  g_return_val_if_fail (GNOME_IS_CANVAS_GROUP (group), NULL);
  
  item = gnome_canvas_item_new (group,
				BST_TYPE_CANVAS_SOURCE,
				NULL);
  bst_canvas_source_build (BST_CANVAS_SOURCE (item));
  
  return item;
}

void
bst_canvas_source_link_start (BstCanvasSource *csource,
			      gdouble         *x_p,
			      gdouble         *y_p)
{
  gdouble x, y;
  
  g_return_if_fail (BST_CANVAS_SOURCE (csource));
  
  x = CONNECTOR_WIDTH / 2;
  y = ICON_HEIGHT / 2;
  gnome_canvas_item_i2w (GNOME_CANVAS_ITEM (csource), &x, &y);
  if (x_p)
    *x_p = x;
  if (y_p)
    *y_p = y;
}

void
bst_canvas_source_link_end (BstCanvasSource *csource,
			    gdouble         *x_p,
			    gdouble         *y_p)
{
  gdouble x, y;
  
  g_return_if_fail (BST_CANVAS_SOURCE (csource));
  
  x = CONNECTOR_WIDTH + ICON_WIDTH + CONNECTOR_WIDTH / 2;
  y = ICON_HEIGHT / 2;
  gnome_canvas_item_i2w (GNOME_CANVAS_ITEM (csource), &x, &y);
  if (x_p)
    *x_p = x;
  if (y_p)
    *y_p = y;
}

static void
bst_canvas_source_build (BstCanvasSource *csource)
{
  GnomeCanvasItem *item;
  GnomeCanvasPoints *gpoints;
  gdouble tmp_x2;
  
  item = GNOME_CANVAS_ITEM (csource);
  
  csource->rect = gnome_canvas_item_new (GNOME_CANVAS_GROUP (csource),
					 GNOME_TYPE_CANVAS_RECT,
					 "outline_color", "black",
					 "x1", 0.0,
					 "y1", 0.0,
					 "x2", 2.0 * CONNECTOR_WIDTH + ICON_WIDTH,
					 "y2", (gdouble) (ICON_HEIGHT + TEXT_HEIGHT),
					 "signal::destroy", gtk_widget_destroyed, &csource->rect,
					 "object_signal::event", bst_canvas_source_child_event, csource,
					 NULL);
  
  {
#include "./icons/noicon.c"
    static const BsePixdata noicon_pixdata = {
      NOICON_PIXDATA_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      NOICON_PIXDATA_WIDTH, NOICON_PIXDATA_HEIGHT,
      NOICON_PIXDATA_RLE_PIXEL_DATA,
    };
    BseIcon *icon;

    icon = bse_icon_from_pixdata (&noicon_pixdata);
    
    gnome_canvas_item_new (GNOME_CANVAS_GROUP (csource),
			   GNOME_TYPE_CANVAS_IMAGE,
			   "x", (gdouble) CONNECTOR_WIDTH + 1,
			   "y", 0.0 + 1,
			   "width", (gdouble) 64,
			   "height", (gdouble) 64,
			   // "width", (gdouble) GIMP_IMAGE_WIDTH,
			   // "height", (gdouble) GIMP_IMAGE_HEIGHT,
			   "anchor", GTK_ANCHOR_NORTH_WEST,
			   "pixbuf", (icon->bytes_per_pixel > 3
				      ? art_pixbuf_new_const_rgba
				      : art_pixbuf_new_const_rgb) (icon->pixels,
								   icon->width,
								   icon->height,
								   icon->width *
								   icon->bytes_per_pixel),
			   "object_signal::event", bst_canvas_source_child_event, csource,
			   NULL);
  }
  csource->text = gnome_canvas_item_new (GNOME_CANVAS_GROUP (csource),
					 GNOME_TYPE_CANVAS_TEXT,
					 "fill_color", "black",
					 "anchor", GTK_ANCHOR_NORTH,
					 "justification", GTK_JUSTIFY_CENTER,
					 "x", (gdouble) (CONNECTOR_WIDTH + ICON_WIDTH / 2),
					 "y", (gdouble) ICON_HEIGHT,
					 "text", "Widget\nhuhu",
					 "font", "-adobe-helvetica-medium-r-normal--12-*-72-72-p-*-iso8859-1",
					 "signal::destroy", gtk_widget_destroyed, &csource->text,
					 NULL);
  gpoints = gnome_canvas_points_new (2);

  gpoints->coords[0] = 0.0;
  gpoints->coords[1] = 0.0;
  gpoints->coords[2] = CONNECTOR_WIDTH;
  gpoints->coords[3] = ICON_HEIGHT;
  gnome_canvas_item_new (GNOME_CANVAS_GROUP (csource),
			 GNOME_TYPE_CANVAS_RECT,
			 "fill_color_rgba", 0xffff0020,
			 "outline_color_rgba", 0x00000000,
			 "x1", gpoints->coords[0],
			 "y1", gpoints->coords[1],
			 "x2", gpoints->coords[2],
			 "y2", gpoints->coords[3],
			 NULL);
  gpoints->coords[0] += CONNECTOR_WIDTH;
  gnome_canvas_item_new (GNOME_CANVAS_GROUP (csource),
			 GNOME_TYPE_CANVAS_LINE,
			 "fill_color", "black",
			 "points", gpoints,
			 NULL);

  gpoints->coords[0] += ICON_WIDTH;
  gpoints->coords[2] += ICON_WIDTH;
  gnome_canvas_item_new (GNOME_CANVAS_GROUP (csource),
			 GNOME_TYPE_CANVAS_LINE,
			 "fill_color", "black",
			 "points", gpoints,
			 NULL);
  tmp_x2 = gpoints->coords[2] + CONNECTOR_WIDTH;
  gnome_canvas_item_new (GNOME_CANVAS_GROUP (csource),
			 GNOME_TYPE_CANVAS_RECT,
			 "fill_color_rgba", 0xff000020,
			 "outline_color_rgba", 0x00000000,
			 "x1", gpoints->coords[0],
			 "y1", gpoints->coords[1],
			 "x2", tmp_x2,
			 "y2", gpoints->coords[3],
			 NULL);
  gpoints->coords[1] = gpoints->coords[3];
  gpoints->coords[0] = 0.0;
  gpoints->coords[2] += CONNECTOR_WIDTH;
  gnome_canvas_item_new (GNOME_CANVAS_GROUP (csource),
			 GNOME_TYPE_CANVAS_LINE,
			 "fill_color", "black",
			 "points", gpoints,
			 NULL);
  gnome_canvas_points_free (gpoints);
}

static gboolean
bst_canvas_source_event (GnomeCanvasItem *item,
			 GdkEvent        *event)
{
  BstCanvasSource *csource = BST_CANVAS_SOURCE (item);
  gboolean handled = FALSE;
  
  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      if (event->button.button == 2)
	{
	  GdkCursor *fleur;
	  gdouble x = event->button.x, y = event->button.y;
	  
	  gnome_canvas_item_w2i (item, &x, &y);
	  csource->move_dx = x;
	  csource->move_dy = y;
	  csource->in_move = TRUE;
	  
	  fleur = gdk_cursor_new (GDK_FLEUR);
	  gnome_canvas_item_grab (item,
				  GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
				  fleur,
				  event->button.time);
	  gdk_cursor_destroy (fleur);
	  handled = TRUE;
	}
      break;
    case GDK_MOTION_NOTIFY:
      if (csource->in_move)
	{
	  gdouble x = event->motion.x, y = event->motion.y;
	  
	  gnome_canvas_item_w2i (item, &x, &y);
	  gnome_canvas_item_move (item, x - csource->move_dx, y - csource->move_dy);
	  BST_OBJECT_ARGS_CHANGED (item);
	  handled = TRUE;
	}
      break;
    case GDK_BUTTON_RELEASE:
      if (event->button.button == 2 && csource->in_move)
	{
	  csource->in_move = FALSE;
	  gnome_canvas_item_ungrab (item, event->button.time);
	}
      handled = TRUE;
      break;
    default:
      break;
    }
  
  if (!handled && GNOME_CANVAS_ITEM_CLASS (parent_class)->event)
    handled |= GNOME_CANVAS_ITEM_CLASS (parent_class)->event (item, event);
  
  return handled;
}

static gboolean
bst_canvas_source_child_event (GnomeCanvasItem *item,
			       GdkEvent        *event)
{
  BstCanvasSource *csource;
  GtkWidget *widget = GTK_WIDGET (item->canvas);
  gboolean handled = FALSE;
  
  csource = BST_CANVAS_SOURCE (item);

  switch (event->type)
    {
    case GDK_ENTER_NOTIFY:
      if (!GTK_WIDGET_HAS_FOCUS (widget))
	gtk_widget_grab_focus (widget);
      handled = TRUE;
      break;
    case GDK_LEAVE_NOTIFY:
      handled = TRUE;
      break;
    case GDK_KEY_PRESS:
      switch (event->key.keyval)
	{
	case 'L':
	  gnome_canvas_item_lower (item, 1);
	  BST_OBJECT_ARGS_CHANGED (item);
	  break;
	case 'l':
	  gnome_canvas_item_lower_to_bottom (item);
	  BST_OBJECT_ARGS_CHANGED (item);
	  break;
	case 'R':
	  gnome_canvas_item_raise (item, 1);
	  BST_OBJECT_ARGS_CHANGED (item);
	  break;
	case 'r':
	  gnome_canvas_item_raise_to_top (item);
	  BST_OBJECT_ARGS_CHANGED (item);
	  break;
	}
      handled = TRUE;
      break;
    case GDK_KEY_RELEASE:
      handled = TRUE;
      break;
    default:
      break;
    }
  
  return handled;
}
