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
#include	"bstcanvassource.h"

#include	"bstparamview.h"
#include	<PKG_config.h>



/* --- BEAST feature ;) --- */
#define	FLIP_CHANNELS(s) (BSE_CHECK_STRUCT_TYPE ((s), flip_channels_type)) /* FIXME: config option */


/* --- defines --- */
#define	ICON_WIDTH	((gdouble) 64)
#define	ICON_HEIGHT	((gdouble) 64)
#define CHANNEL_WIDTH	((gdouble) 10)
#define CHANNEL_HEIGHT	((gdouble) ICON_HEIGHT)
#define	ICON_X		((gdouble) CHANNEL_WIDTH)
#define	ICON_Y		((gdouble) 0)
#define ICHANNEL_realX	((gdouble) 0)
#define OCHANNEL_realX	((gdouble) CHANNEL_WIDTH + ICON_WIDTH)
#define ICHANNEL_X(s)	(FLIP_CHANNELS (s) ? OCHANNEL_realX : ICHANNEL_realX)
#define OCHANNEL_X(s)	(FLIP_CHANNELS (s) ? ICHANNEL_realX : OCHANNEL_realX)
#define ICHANNEL_Y	((gdouble) 0)
#define OCHANNEL_Y	((gdouble) 0)
#define	TOTAL_WIDTH	((gdouble) CHANNEL_WIDTH + ICON_WIDTH + CHANNEL_WIDTH)
#define	TOTAL_HEIGHT	((gdouble) ICON_HEIGHT + TEXT_HEIGHT)
#define TEXT_X		((gdouble) CHANNEL_WIDTH + ICON_WIDTH / 2) /* anchor: north */
#define TEXT_Y		((gdouble) ICON_HEIGHT)
#define TEXT_HEIGHT	((gdouble) 13)
#define RGBA_BLACK	(0x000000ff)


/* --- signals --- */
enum
{
  SIGNAL_UPDATE_LINKS,
  SIGNAL_LAST
};
typedef void    (*SignalUpdateLinks)            (BstCanvasSource       *source,
						 gpointer         func_data);


/* --- prototypes --- */
static void	bst_canvas_source_class_init	(BstCanvasSourceClass	*class);
static void	bst_canvas_source_init		(BstCanvasSource	*csource);
static void	bst_canvas_source_build		(BstCanvasSource	*csource);
static void	bst_canvas_source_destroy	(GtkObject		*object);
static gboolean bst_canvas_source_event		(GnomeCanvasItem        *item,
						 GdkEvent               *event);
static gboolean bst_canvas_source_child_event	(BstCanvasSource	*csource,
						 GdkEvent               *event,
						 GnomeCanvasItem        *child);
static void     bst_canvas_source_changed       (BstCanvasSource        *csource);


/* --- static variables --- */
static gpointer              parent_class = NULL;
static BstCanvasSourceClass *bst_canvas_source_class = NULL;
static guint                 csource_signals[SIGNAL_LAST] = { 0 };
static BseType               flip_channels_type = 0;


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

  class->update_links = NULL;

  csource_signals[SIGNAL_UPDATE_LINKS] =
    gtk_signal_new ("update-links",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (BstCanvasSourceClass, update_links),
		    gtk_signal_default_marshaller,
		    GTK_TYPE_NONE, 0);
  gtk_object_class_add_signals (object_class, csource_signals, SIGNAL_LAST);

  flip_channels_type = bse_type_from_name ("BseLoopback");
}

static void
bst_canvas_source_init (BstCanvasSource *csource)
{
  GtkObject *object = GTK_OBJECT (csource);
  
  csource->source = NULL;
  csource->source_view = NULL;
  csource->text = NULL;
  csource->channel_items = NULL;
  csource->in_move = FALSE;
  csource->move_dx = 0;
  csource->move_dy = 0;
  gtk_signal_connect (object, "args_changed", bst_canvas_source_changed, NULL);
}

static void
source_channels_changed (BstCanvasSource *csource)
{
  g_return_if_fail (BST_IS_CANVAS_SOURCE (csource));

  BST_OBJECT_ARGS_CHANGED (csource);
  bst_canvas_source_update_links (csource);
}

static void
source_name_changed (BstCanvasSource *csource)
{
  BseProject *project;
  gchar *pname;
  gchar *name;

  g_return_if_fail (BST_IS_CANVAS_SOURCE (csource));

  name = bse_object_get_name (BSE_OBJECT (csource->source));
  if (!name)
    name = bse_type_name (BSE_OBJECT_TYPE (csource->source));
  project = bse_item_get_project (BSE_ITEM (csource->source));
  pname = bse_object_get_name (BSE_OBJECT (project));
  if (!pname)
    pname = bse_type_name (BSE_OBJECT_TYPE (project));
  pname = g_strconcat (pname, ": ", name, NULL);

  if (csource->text)
    bst_object_set (csource->text, "text", name, NULL);

  if (csource->source_view)
    gtk_window_set_title (GTK_WINDOW (csource->source_view), pname);

  g_free (pname);
}

static void
bst_canvas_source_destroy (GtkObject *object)
{
  BstCanvasSource *csource = BST_CANVAS_SOURCE (object);
  GnomeCanvasGroup *group = GNOME_CANVAS_GROUP (object);

  if (csource->source_view)
    gtk_widget_destroy (csource->source_view);
  
  while (group->item_list)
    gtk_object_destroy (group->item_list->data);

  bse_object_remove_notifiers_by_func (BSE_OBJECT (csource->source),
				       gtk_object_destroy,
				       csource);
  bse_object_remove_notifiers_by_func (BSE_OBJECT (csource->source),
				       source_channels_changed,
				       csource);
  bse_object_remove_notifiers_by_func (BSE_OBJECT (csource->source),
				       source_name_changed,
				       csource);
  csource->source = NULL;

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

#define EPSILON 1e-6

static void
bse_object_set_parasite_coords (BseObject *object,
				gdouble    x,
				gdouble    y)
{
  gfloat coords[2];
  
  g_return_if_fail (BSE_IS_OBJECT (object));
  
  coords[0] = x;
  coords[1] = y;
  
  /* g_print ("set-coords: %p %f %f\n", object, x, y); */
  
  bse_parasite_set_floats (object, "BstRouterCoords", 2, coords);
}

static gboolean
bse_object_get_parasite_coords (BseObject *object,
				gdouble   *x,
				gdouble   *y)
{
  gfloat coords[2];
  
  g_return_val_if_fail (BSE_IS_OBJECT (object), FALSE);
  g_return_val_if_fail (x != NULL && y != NULL, FALSE);
  
  if (bse_parasite_get_floats (object, "BstRouterCoords", 2, coords) == 2)
    {
      *x = coords[0];
      *y = coords[1];
      /* g_print ("get-coords: %p %f %f\n", object, *x, *y); */
      
      return BSE_EPSILON_CMP (coords[0], 0) || BSE_EPSILON_CMP (coords[1], 0);
    }
  
  return FALSE;
}

GnomeCanvasItem*
bst_canvas_source_new (GnomeCanvasGroup *group,
		       BseSource        *source,
		       gdouble           world_x,
		       gdouble           world_y)
{
  BstCanvasSource *csource;
  GnomeCanvasItem *item;
  
  g_return_val_if_fail (GNOME_IS_CANVAS_GROUP (group), NULL);
  g_return_val_if_fail (BSE_IS_SOURCE (source), NULL);

  item = gnome_canvas_item_new (group,
				BST_TYPE_CANVAS_SOURCE,
				NULL);
  csource = BST_CANVAS_SOURCE (item);
  csource->source = source;
  bse_object_add_data_notifier (BSE_OBJECT (csource->source),
				"destroy",
				gtk_object_destroy,
				csource);
  bse_object_add_data_notifier (BSE_OBJECT (csource->source),
				"io_changed",
				source_channels_changed,
				csource);
  bse_object_add_data_notifier (BSE_OBJECT (csource->source),
				"name-set",
				source_name_changed,
				csource);
  
  if (bse_object_get_parasite_coords (BSE_OBJECT (csource->source), &world_x, &world_y))
    {
      world_x = - world_x;
      world_y = - world_y;
      gnome_canvas_item_w2i (item, &world_x, &world_y);
      gnome_canvas_item_move (item, world_x, world_y);
    }
  else
    {
      gnome_canvas_item_w2i (item, &world_x, &world_y);
      gnome_canvas_item_move (item, world_x, world_y);
    }
  
  bst_canvas_source_build (BST_CANVAS_SOURCE (item));
  BST_OBJECT_ARGS_CHANGED (item);
  
  return item;
}

void
bst_canvas_source_update_links (BstCanvasSource *csource)
{
  g_return_if_fail (BST_IS_CANVAS_SOURCE (csource));

  if (!GTK_OBJECT_DESTROYED (csource) && csource->source)
    gtk_signal_emit (GTK_OBJECT (csource), csource_signals[SIGNAL_UPDATE_LINKS]);
}

void
bst_canvas_source_popup_view (BstCanvasSource *csource)
{
  g_return_if_fail (BST_IS_CANVAS_SOURCE (csource));

  if (!csource->source_view)
    {
      GtkWidget *param_view;

      csource->source_view = gtk_widget_new (GTK_TYPE_WINDOW,
					     "auto_shrink", FALSE,
					     "allow_shrink", FALSE,
					     "allow_grow", TRUE,
					     "object_signal::destroy", bse_nullify_pointer, &csource->source_view,
					     "signal::delete_event", gtk_widget_hide, NULL,
					     "signal::delete_event", gtk_true, NULL,
					     NULL);
      param_view = bst_param_view_new (BSE_OBJECT (csource->source));
      gtk_widget_show (param_view);
      gtk_container_add (GTK_CONTAINER (csource->source_view), param_view);
      source_name_changed (csource);
    }

  gtk_widget_show (csource->source_view);
  gdk_window_raise (csource->source_view->window);
}

void
bst_canvas_source_toggle_view (BstCanvasSource *csource)
{
  g_return_if_fail (BST_IS_CANVAS_SOURCE (csource));

  if (!csource->source_view || !GTK_WIDGET_VISIBLE (csource->source_view))
    bst_canvas_source_popup_view (csource);
  else
    gtk_widget_hide (csource->source_view);
}

BstCanvasSource*
bst_canvas_source_at (GnomeCanvas *canvas,
		      gdouble      world_x,
		      gdouble      world_y)
{
  GnomeCanvasItem *item;

  g_return_val_if_fail (GNOME_IS_CANVAS (canvas), NULL);

  item = gnome_canvas_get_item_at (canvas, world_x, world_y);
  while (item && !BST_IS_CANVAS_SOURCE (item))
    item = item->parent;

  return BST_IS_CANVAS_SOURCE (item) ? BST_CANVAS_SOURCE (item) : NULL;
}

void
bst_canvas_source_ichannel_pos (BstCanvasSource *csource,
				guint            ochannel_id,
				gdouble         *x_p,
				gdouble         *y_p)
{
  gdouble x, y;
  
  g_return_if_fail (BST_IS_CANVAS_SOURCE (csource));
  
  x = ICHANNEL_X (csource->source) + CHANNEL_WIDTH / 2;
  y = CHANNEL_HEIGHT / BSE_SOURCE_N_ICHANNELS (csource->source);
  y *= ochannel_id - 0.5;
  y += ICHANNEL_Y;
  gnome_canvas_item_i2w (GNOME_CANVAS_ITEM (csource), &x, &y);
  if (x_p)
    *x_p = x;
  if (y_p)
    *y_p = y;
}

void
bst_canvas_source_ochannel_pos (BstCanvasSource *csource,
				guint            ichannel_id,
				gdouble         *x_p,
				gdouble         *y_p)
{
  gdouble x, y;
  
  g_return_if_fail (BST_IS_CANVAS_SOURCE (csource));
  
  x = OCHANNEL_X (csource->source) + CHANNEL_WIDTH / 2;
  y = CHANNEL_HEIGHT / BSE_SOURCE_N_OCHANNELS (csource->source);
  y *= ichannel_id - 0.5;
  y += OCHANNEL_Y;
  gnome_canvas_item_i2w (GNOME_CANVAS_ITEM (csource), &x, &y);
  if (x_p)
    *x_p = x;
  if (y_p)
    *y_p = y;
}

guint
bst_canvas_source_ichannel_at (BstCanvasSource *csource,
			       gdouble	        x,
			       gdouble	        y)
{
  guint channel_id = 0;

  g_return_val_if_fail (BST_IS_CANVAS_SOURCE (csource), 0);

  gnome_canvas_item_w2i (GNOME_CANVAS_ITEM (csource), &x, &y);

  x -= ICHANNEL_X (csource->source);
  y -= ICHANNEL_Y;
  if (x > 0 && x < CHANNEL_WIDTH &&
      y > 0 && y < CHANNEL_HEIGHT &&
      BSE_SOURCE_N_ICHANNELS (csource->source))
    {
      y /= CHANNEL_HEIGHT / BSE_SOURCE_N_ICHANNELS (csource->source);
      channel_id = 1 + y;
    }

  return channel_id;
}

guint
bst_canvas_source_ochannel_at (BstCanvasSource *csource,
			       gdouble	        x,
			       gdouble	        y)
{
  guint channel_id = 0;

  g_return_val_if_fail (BST_IS_CANVAS_SOURCE (csource), 0);

  gnome_canvas_item_w2i (GNOME_CANVAS_ITEM (csource), &x, &y);

  x -= OCHANNEL_X (csource->source);
  y -= OCHANNEL_Y;
  if (x > 0 && x < CHANNEL_WIDTH &&
      y > 0 && y < CHANNEL_HEIGHT &&
      BSE_SOURCE_N_OCHANNELS (csource->source))
    {
      y /= CHANNEL_HEIGHT / BSE_SOURCE_N_OCHANNELS (csource->source);
      channel_id = 1 + y;
    }

  return channel_id;
}

static void
channel_item_remove (BstCanvasSource *csource,
		     GnomeCanvasItem *item)
{
  csource->channel_items = g_slist_remove (csource->channel_items, item);
}

static void
bst_canvas_source_build_channels (BstCanvasSource *csource,
				  gboolean         is_input,
				  gint             color,
				  gint		   color_fade)
{
  GnomeCanvasGroup *group = GNOME_CANVAS_GROUP (csource);
  const guint alpha = 0x20;
  gint n_channels, color_delta = 0;
  gdouble x1, x2, y1, y2;
  gdouble d_y;
  guint i;

  if (is_input)
    {
      n_channels = BSE_SOURCE_N_ICHANNELS (csource->source);
      x1 = ICHANNEL_X (csource->source);
      y1 = ICHANNEL_Y;
    }
  else
    {
      n_channels = BSE_SOURCE_N_OCHANNELS (csource->source);
      x1 = OCHANNEL_X (csource->source);
      y1 = OCHANNEL_Y;
    }
  x2 = x1 + CHANNEL_WIDTH;
  y2 = y1 + CHANNEL_HEIGHT;
  d_y = y2 - y1;
  d_y /= n_channels;
  if (n_channels > 1)
    {
      gint cd_red, cd_blue, cd_green;

      cd_red = ((color_fade & 0xff0000) - (color & 0xff0000)) / (n_channels - 1);
      cd_green = ((color_fade & 0x00ff00) - (color & 0x00ff00)) / (n_channels - 1);
      cd_blue = ((color_fade & 0x0000ff) - (color & 0x0000ff)) / (n_channels - 1);
      color_delta = (cd_red & ~0xffff) + (cd_green & ~0xff) + cd_blue;
    }
  else if (n_channels == 0)
    {
      GnomeCanvasItem *item;

      item = gnome_canvas_item_new (group,
				    GNOME_TYPE_CANVAS_RECT,
				    "fill_color_rgba", (0xc3c3c3 << 8) | alpha,
				    "outline_color_rgba", RGBA_BLACK,
				    "x1", x1,
				    "y1", y1,
				    "x2", x2,
				    "y2", y2,
				    "object_signal::destroy", channel_item_remove, csource,
				    "object_signal::event", bst_canvas_source_child_event, csource,
				    NULL);
      csource->channel_items = g_slist_prepend (csource->channel_items, item);
    }

  for (i = 0; i < n_channels; i++)
    {
      GnomeCanvasItem *item;

      y2 = y1 + d_y;
      item = gnome_canvas_item_new (group,
				    GNOME_TYPE_CANVAS_RECT,
				    "fill_color_rgba", (color << 8) | alpha,
				    "outline_color_rgba", RGBA_BLACK,
				    "x1", x1,
				    "y1", y1,
				    "x2", x2,
				    "y2", y2,
				    "object_signal::destroy", channel_item_remove, csource,
				    "object_signal::event", bst_canvas_source_child_event, csource,
				    NULL);
      csource->channel_items = g_slist_prepend (csource->channel_items, item);
      color += color_delta;
      y1 = y2;
    }
}

#ifdef	BST_WITH_GDK_PIXBUF
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gnome-canvas-pixbuf.h>
static GnomeCanvasItem*
create_canvas_icon (GnomeCanvasGroup *group,
		    BseIcon          *icon)
{
  ArtPixBuf *apixbuf;
  GdkPixbuf *pixbuf;
  GnomeCanvasItem *item;
  
  bse_icon_ref (icon);
  apixbuf = (icon->bytes_per_pixel > 3
	     ? art_pixbuf_new_const_rgba
	     : art_pixbuf_new_const_rgb) (icon->pixels,
					  icon->width,
					  icon->height,
					  icon->width *
					  icon->bytes_per_pixel);
  pixbuf = gdk_pixbuf_new_from_art_pixbuf (apixbuf);
  item = gnome_canvas_item_new (group, GNOME_TYPE_CANVAS_PIXBUF,
				"pixbuf", pixbuf,
				"x_set", TRUE,
				"y_set", TRUE,
				NULL);
  gtk_object_set_data_full (GTK_OBJECT (item),
			    "BseIcon",
			    icon,
			    (GtkDestroyNotify) bse_icon_unref);
  gdk_pixbuf_unref (pixbuf);
  
  return item;
}
#else	/* !BST_WITH_GDK_PIXBUF */
static GnomeCanvasItem*
create_canvas_icon (GnomeCanvasGroup *group,
		    BseIcon          *icon)
{
  ArtPixBuf *apixbuf;
  GnomeCanvasItem *item;
  
  bse_icon_ref (icon);
  apixbuf = (icon->bytes_per_pixel > 3
	     ? art_pixbuf_new_const_rgba
	     : art_pixbuf_new_const_rgb) (icon->pixels,
					  icon->width,
					  icon->height,
					  icon->width *
					  icon->bytes_per_pixel);
  item = gnome_canvas_item_new (group, GNOME_TYPE_CANVAS_IMAGE,
				"anchor", GTK_ANCHOR_NORTH_WEST,
				"pixbuf", apixbuf,
				NULL);
  gtk_object_set_data_full (GTK_OBJECT (item),
			    "BseIcon",
			    icon,
			    (GtkDestroyNotify) bse_icon_unref);
  return item;
}
#endif	/* !BST_WITH_GDK_PIXBUF */

static void
bst_canvas_source_build (BstCanvasSource *csource)
{
  GnomeCanvasGroup *group = GNOME_CANVAS_GROUP (csource);
  GnomeCanvasPoints *gpoints;
  GnomeCanvasItem *item;
  BseIcon *icon;

  /* order of creation is important to enforce stacking */

  /* add icon to group, revert to a stock icon if none is available
   */
  icon = bse_object_get_icon (BSE_OBJECT (csource->source));
  if (!icon)
    icon = bst_icon_from_stock (BST_ICON_NOICON);
  item = create_canvas_icon (group, icon);
  bst_object_set (GTK_OBJECT (item),
		  "x", ICON_X,
		  "y", ICON_Y,
		  "width", ICON_WIDTH,
		  "height", ICON_HEIGHT,
		  "object_signal::event", bst_canvas_source_child_event, csource,
		  NULL);

  /* add text item, invoke name_changed callback to setup the text value
   */
  csource->text = gnome_canvas_item_new (group,
					 GNOME_TYPE_CANVAS_TEXT,
					 "fill_color", "black",
					 "anchor", GTK_ANCHOR_NORTH,
					 "justification", GTK_JUSTIFY_CENTER,
					 "x", TEXT_X,
					 "y", TEXT_Y,
					 "font", "-adobe-helvetica-medium-r-normal--12-*-72-72-p-*-iso8859-1",
					 "signal::destroy", gtk_widget_destroyed, &csource->text,
					 "object_signal::event", bst_canvas_source_child_event, csource,
					 NULL);
  source_name_changed (csource);

  /* add input and output channel items
   */
  while (csource->channel_items)
    gtk_object_destroy (csource->channel_items->data);
  bst_canvas_source_build_channels (csource,
				    TRUE, /* input channels */
				    0xffff00, 0x808000);
  bst_canvas_source_build_channels (csource,
				    FALSE, /* output channels */
				    0xff0000, 0x800000);

  /* put lines above and below the icon
   */
  gpoints = gnome_canvas_points_newv (2, ICON_X, ICON_Y, ICON_X + ICON_WIDTH, ICON_Y);
  item = gnome_canvas_item_new (group,
				GNOME_TYPE_CANVAS_LINE,
				"fill_color_rgba", RGBA_BLACK,
				"points", gpoints,
				"object_signal::event", bst_canvas_source_child_event, csource,
				NULL);
  gpoints->coords[1] += ICON_HEIGHT;
  gpoints->coords[3] += ICON_HEIGHT;
  item = gnome_canvas_item_new (group,
				GNOME_TYPE_CANVAS_LINE,
				"fill_color_rgba", RGBA_BLACK,
				"points", gpoints,
				"object_signal::event", bst_canvas_source_child_event, csource,
				NULL);
  gnome_canvas_points_free (gpoints);

  /* put a rectangle at the bottm (around the text)
   */
  /*
  item = gnome_canvas_item_new (group,
				GNOME_TYPE_CANVAS_RECT,
				"outline_color_rgba", RGBA_BLACK,
				"x1", 0.0,
				"y1", TEXT_Y,
				"x2", TOTAL_WIDTH,
				"y2", TEXT_Y + TEXT_HEIGHT,
				"object_signal::event", bst_canvas_source_child_event, csource,
				NULL);
  */
  gpoints = gnome_canvas_points_newv (4,
				      0.0, TEXT_Y,
				      0.0, TEXT_Y + TEXT_HEIGHT,
				      TOTAL_WIDTH, TEXT_Y + TEXT_HEIGHT,
				      TOTAL_WIDTH, TEXT_Y);
  item = gnome_canvas_item_new (group,
				GNOME_TYPE_CANVAS_LINE,
				"fill_color_rgba", RGBA_BLACK,
				"points", gpoints,
				"object_signal::event", bst_canvas_source_child_event, csource,
				NULL);
  gnome_canvas_points_free (gpoints);
  
  /* put an outer rectangle, make it transparent in aa mode,
   * so we can receive mouse events everywhere
   */
  item = gnome_canvas_item_new (group,
				GNOME_TYPE_CANVAS_RECT,
				"outline_color_rgba", RGBA_BLACK, /* covers buggy canvas lines */
				"outline_color_rgba", 0x00000000,
				"x1", 0.0,
				"y1", 0.0,
				"x2", TOTAL_WIDTH,
				"y2", TOTAL_HEIGHT,
				"object_signal::event", bst_canvas_source_child_event, csource,
				(GNOME_CANVAS_ITEM (csource)->canvas->aa
				 ? "fill_color_rgba"
				 : NULL), 0x00000000,
				NULL);
}

static void
bst_canvas_source_changed (BstCanvasSource *csource)
{
  if (csource->source)
    {
      GnomeCanvasItem *item = GNOME_CANVAS_ITEM (csource);
      gdouble x = 0, y = 0;

      gnome_canvas_item_w2i (item, &x, &y);
      bse_object_set_parasite_coords (BSE_OBJECT (csource->source), x, y);
    }
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
	  handled = TRUE;
	}
      break;
    default:
      break;
    }
  
  if (!handled && GNOME_CANVAS_ITEM_CLASS (parent_class)->event)
    handled = GNOME_CANVAS_ITEM_CLASS (parent_class)->event (item, event);
  
  return handled;
}

static gboolean
bst_canvas_source_child_event (BstCanvasSource *csource,
			       GdkEvent        *event,
			       GnomeCanvasItem *child)
{
  GnomeCanvasItem *item = GNOME_CANVAS_ITEM (csource);
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
	case 'l':
	  gnome_canvas_item_lower_to_bottom (item);
	  BST_OBJECT_ARGS_CHANGED (item);
	  break;
	case 'R':
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
