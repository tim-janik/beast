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
#include "bstcanvaslink.h"

#include <math.h>

#define ARROW_LENGTH    (12.0)
#define ARROW_WIDTH     (6.0)
#define TAG_DIAMETER    (2.0)


/* --- prototypes --- */
static void	bst_canvas_link_class_init	(BstCanvasLinkClass	*class);
static void	bst_canvas_link_init		(BstCanvasLink		*clink);
static void	bst_canvas_link_update		(BstCanvasLink		*clink);
static void	bst_canvas_link_destroy		(GtkObject		*object);
static gboolean bst_canvas_link_event		(GnomeCanvasItem        *item,
						 GdkEvent               *event);
static void	bst_canvas_link_adjust_arrow	(BstCanvasLink		*clink);
static void	bst_canvas_link_adjust_tags	(BstCanvasLink		*clink);
static gboolean	bst_canvas_link_child_event	(GnomeCanvasItem        *item,
						 GdkEvent               *event);


/* --- static variables --- */
static gpointer              parent_class = NULL;
static BstCanvasLinkClass *bst_canvas_link_class = NULL;


/* --- functions --- */
GtkType
bst_canvas_link_get_type (void)
{
  static GtkType canvas_link_type = 0;
  
  if (!canvas_link_type)
    {
      GtkTypeInfo canvas_link_info =
      {
	"BstCanvasLink",
	sizeof (BstCanvasLink),
	sizeof (BstCanvasLinkClass),
	(GtkClassInitFunc) bst_canvas_link_class_init,
	(GtkObjectInitFunc) bst_canvas_link_init,
        /* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      canvas_link_type = gtk_type_unique (GNOME_TYPE_CANVAS_GROUP, &canvas_link_info);
      gtk_type_set_chunk_alloc (canvas_link_type, 16);
    }
  
  return canvas_link_type;
}

static void
bst_canvas_link_class_init (BstCanvasLinkClass *class)
{
  GtkObjectClass *object_class;
  GnomeCanvasItemClass *canvas_item_class;
  GnomeCanvasGroupClass *canvas_group_class;
  
  object_class = GTK_OBJECT_CLASS (class);
  canvas_item_class = GNOME_CANVAS_ITEM_CLASS (class);
  canvas_group_class = GNOME_CANVAS_GROUP_CLASS (class);
  
  bst_canvas_link_class = class;
  parent_class = gtk_type_class (GNOME_TYPE_CANVAS_GROUP);
  
  object_class->destroy = bst_canvas_link_destroy;
  canvas_item_class->event = bst_canvas_link_event;
}

static void
bst_canvas_link_init (BstCanvasLink *clink)
{
  clink->line = NULL;
  clink->arrow = NULL;
  clink->tag_start = NULL;
  clink->tag_end = NULL;
  clink->ocsource = NULL;
  clink->ochannel_id = 0;
  clink->oc_handler = 0;
  clink->icsource = NULL;
  clink->ichannel_id = 0;
  clink->ic_handler = 0;
  clink->link_view = NULL;
}

static void
bst_canvas_link_destroy (GtkObject *object)
{
  BstCanvasLink *clink = BST_CANVAS_LINK (object);
  GnomeCanvasGroup *group = GNOME_CANVAS_GROUP (object);
  
  while (group->item_list)
    gtk_object_destroy (group->item_list->data);
  
  bst_canvas_link_set_ocsource (clink, NULL, 0);
  bst_canvas_link_set_icsource (clink, NULL, 0);
  
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GnomeCanvasItem*
bst_canvas_link_new (GnomeCanvasGroup *group)
{
  GnomeCanvasItem *item;
  
  g_return_val_if_fail (GNOME_IS_CANVAS_GROUP (group), NULL);
  
  item = gnome_canvas_item_new (group,
				BST_TYPE_CANVAS_LINK,
				NULL);
  return item;
}

BstCanvasLink*
bst_canvas_link_at (GnomeCanvas *canvas,
		    gdouble      world_x,
		    gdouble      world_y)
{
  return (BstCanvasLink*) gnome_canvas_typed_item_at (canvas, BST_TYPE_CANVAS_LINK, world_x, world_y);
}

static void
clink_view_update (BstCanvasLink *clink,
		   gboolean       force_update)
{
  GtkWidget *frame = (clink->link_view && (force_update || GTK_WIDGET_VISIBLE (clink->link_view))
		      ? bst_subwindow_get_child (clink->link_view)
		      : NULL);

  if (frame)
    {
      GtkWidget *label = GTK_BIN (frame)->child;
      BseProject *project;
      BseSourceIChannelDef *ic_def;
      BseSourceOChannelDef *oc_def;
      gchar *string, *iname, *oname;

      /* figure appropriate window title
       */
      project = clink->icsource ? bse_item_get_project (BSE_ITEM (clink->icsource->source)) : NULL;
      string = project ? bse_object_get_name_or_type (BSE_OBJECT (project)) : "BEAST";
      string = g_strconcat (string, ": Source Link", NULL);
      gtk_window_set_title (GTK_WINDOW (clink->link_view), string);
      g_free (string);

      /* construct actuall information
       */
      iname = clink->icsource ? bse_object_get_name_or_type (BSE_OBJECT (clink->icsource->source)) : "<???>";
      oname = clink->ocsource ? bse_object_get_name_or_type (BSE_OBJECT (clink->ocsource->source)) : "<???>";
      oc_def = clink->ocsource ? BSE_SOURCE_OCHANNEL_DEF (clink->ocsource->source, clink->ochannel_id) : NULL;
      ic_def = clink->icsource ? BSE_SOURCE_ICHANNEL_DEF (clink->icsource->source, clink->ichannel_id) : NULL;
      string = g_strdup_printf ("Output channel:\n"
				"        %s:%d %s\n"
				"        (%s)\n"
				"Input channel:\n"
				"        %s:%d %s\n"
				"        (%s)\n"
				"\n"
				"Number of tracks: %d\n"
				"History: %d\n",
				oname, clink->ochannel_id, oc_def ? oc_def->name : "?",
				oc_def ? oc_def->blurb : "?",
				iname, clink->ichannel_id, ic_def ? ic_def->name : "?",
				ic_def ? ic_def->blurb : "?",
				oc_def ? oc_def->n_tracks : -1,
				ic_def ? ic_def->history : -1);
      gtk_label_set_text (GTK_LABEL (label), string);
      g_free (string);
    }
}

void
bst_canvas_link_popup_view (BstCanvasLink *clink)
{
  g_return_if_fail (BST_IS_CANVAS_LINK (clink));
  
  if (!clink->link_view)
    clink->link_view = bst_subwindow_new (GTK_OBJECT (clink),
					  &clink->link_view,
					  gtk_widget_new (GTK_TYPE_FRAME,
							  "visible", TRUE,
							  "border_width", 5,
							  "label", "Source link",
							  "child", gtk_widget_new (GTK_TYPE_LABEL,
										   "visible", TRUE,
										   "justify", GTK_JUSTIFY_LEFT,
										   "xpad", 5,
										   NULL),
							  NULL));
  clink_view_update (clink, TRUE);
  gtk_widget_showraise (clink->link_view);
}

void
bst_canvas_link_toggle_view (BstCanvasLink *clink)
{
  g_return_if_fail (BST_IS_CANVAS_LINK (clink));

  if (!clink->link_view || !GTK_WIDGET_VISIBLE (clink->link_view))
    bst_canvas_link_popup_view (clink);
  else
    gtk_widget_hide (clink->link_view);
}

static void
clink_view_check_update (BstCanvasLink *clink)
{
  g_return_if_fail (BST_IS_CANVAS_LINK (clink));

  clink_view_update (clink, FALSE);
}

void
bst_canvas_link_set_ocsource (BstCanvasLink   *clink,
			      BstCanvasSource *ocsource,
			      guint            ochannel_id)
{
  g_return_if_fail (BST_IS_CANVAS_LINK (clink));
  if (ocsource)
    g_return_if_fail (BST_CANVAS_SOURCE (ocsource));
  
  if (clink->ocsource)
    {
      bse_object_remove_notifiers_by_func (BSE_OBJECT (clink->ocsource->source),
					   clink_view_check_update,
					   clink);
      if (!GTK_OBJECT_DESTROYED (clink->ocsource))
	gtk_signal_disconnect (GTK_OBJECT (clink->ocsource), clink->oc_handler);
      gtk_object_unref (GTK_OBJECT (clink->ocsource));
    }
  clink->ocsource = ocsource;
  clink->ochannel_id = ochannel_id;
  if (clink->ocsource)
    {
      gtk_object_ref (GTK_OBJECT (clink->ocsource));
      clink->oc_handler = gtk_signal_connect_object (GTK_OBJECT (clink->ocsource),
						     "args_changed",
						     bst_canvas_link_update,
						     GTK_OBJECT (clink));
      bse_object_add_data_notifier (BSE_OBJECT (clink->ocsource->source),
				    "name-set",
				    clink_view_check_update,
				    clink);
      bst_canvas_link_update (clink);
    }
}

void
bst_canvas_link_set_icsource (BstCanvasLink   *clink,
			      BstCanvasSource *icsource,
			      guint            ichannel_id)
{
  g_return_if_fail (BST_IS_CANVAS_LINK (clink));
  if (icsource)
    g_return_if_fail (BST_CANVAS_SOURCE (icsource));
  
  if (clink->icsource)
    {
      bse_object_remove_notifiers_by_func (BSE_OBJECT (clink->icsource->source),
					   clink_view_check_update,
					   clink);
      if (!GTK_OBJECT_DESTROYED (clink->icsource))
        gtk_signal_disconnect (GTK_OBJECT (clink->icsource), clink->ic_handler);
      gtk_object_unref (GTK_OBJECT (clink->icsource));
    }
  clink->icsource = icsource;
  clink->ichannel_id = ichannel_id;
  if (clink->icsource)
    {
      gtk_object_ref (GTK_OBJECT (clink->icsource));
      clink->ic_handler = gtk_signal_connect_object (GTK_OBJECT (clink->icsource),
						     "args_changed",
						     bst_canvas_link_update,
						     GTK_OBJECT (clink));
      bse_object_add_data_notifier (BSE_OBJECT (clink->icsource->source),
				    "name-set",
				    clink_view_check_update,
				    clink);
      bst_canvas_link_update (clink);
    }
}

static void
bst_canvas_link_update (BstCanvasLink *clink)
{
  GnomeCanvasItem *item = GNOME_CANVAS_ITEM (clink);
  GnomeCanvasPoints *gpoints;
  gdouble start_x = 0, start_y = 0, end_x = 10, end_y = 10;
  
  if (clink->ocsource)
    {
      bst_canvas_source_ochannel_pos (clink->ocsource, clink->ochannel_id, &start_x, &start_y);
      gnome_canvas_item_w2i (item, &start_x, &start_y);
    }
  if (clink->icsource)
    {
      bst_canvas_source_ichannel_pos (clink->icsource, clink->ichannel_id, &end_x, &end_y);
      gnome_canvas_item_w2i (item, &end_x, &end_y);
    }
  if (clink->ocsource && clink->icsource)
    gnome_canvas_item_keep_above (GNOME_CANVAS_ITEM (clink),
				  GNOME_CANVAS_ITEM (clink->ocsource),
				  GNOME_CANVAS_ITEM (clink->icsource));
  
  
  if (!clink->arrow)
    clink->arrow = gnome_canvas_item_new (GNOME_CANVAS_GROUP (clink),
					  GNOME_TYPE_CANVAS_POLYGON,
					  "outline_color_rgba", 0x0000ffff,
					  "fill_color_rgba", 0xff0000ff,
					  "signal::destroy", gtk_widget_destroyed, &clink->arrow,
					  "object_signal::event", bst_canvas_link_child_event, clink,
					  NULL);
  if (!clink->tag_start)
    clink->tag_start = gnome_canvas_item_new (GNOME_CANVAS_GROUP (clink),
					      GNOME_TYPE_CANVAS_ELLIPSE,
					      "outline_color_rgba", 0x000000ff, // xffff00ff,
					      "fill_color_rgba", 0xffff00ff,
					      "signal::destroy", gtk_widget_destroyed, &clink->tag_start,
					      "object_signal::event", bst_canvas_link_child_event, clink,
					      NULL);
  if (!clink->tag_end)
    clink->tag_end = gnome_canvas_item_new (GNOME_CANVAS_GROUP (clink),
					    GNOME_TYPE_CANVAS_ELLIPSE,
					    "outline_color_rgba", 0x000000ff, // 0xff0000ff,
					    "fill_color_rgba", 0xff0000ff,
					    "signal::destroy", gtk_widget_destroyed, &clink->tag_end,
					    "object_signal::event", bst_canvas_link_child_event, clink,
					    NULL);
  if (!clink->line)
    clink->line = gnome_canvas_item_new (GNOME_CANVAS_GROUP (clink),
					 GNOME_TYPE_CANVAS_LINE,
					 "fill_color", "black",
					 "signal::destroy", gtk_widget_destroyed, &clink->line,
					 "object_signal::args_changed", bst_canvas_link_adjust_arrow, clink,
					 "object_signal::args_changed", bst_canvas_link_adjust_tags, clink,
					 NULL);
  gpoints = gnome_canvas_points_new (2);
  gpoints->coords[0] = start_x;
  gpoints->coords[1] = start_y;
  gpoints->coords[2] = end_x;
  gpoints->coords[3] = end_y;
  bst_object_set (clink->line,
		  "points", gpoints,
		  NULL);
  gnome_canvas_points_free (gpoints);
  gnome_canvas_item_raise_to_top (clink->tag_start);
  gnome_canvas_item_raise_to_top (clink->tag_end);
}

static void
bst_canvas_link_adjust_tags (BstCanvasLink *clink)
{
  GnomeCanvasPoints *gpoints;
  gdouble x1, y1, x2, y2, *points;
  
  gtk_object_get (GTK_OBJECT (clink->line), "points", &gpoints, NULL);
  if (!gpoints)
    gpoints = gnome_canvas_points_new0 (2);
  points = gpoints->coords;

  x1 = points[0] - TAG_DIAMETER;
  y1 = points[1] - TAG_DIAMETER;
  x2 = points[0] + TAG_DIAMETER;
  y2 = points[1] + TAG_DIAMETER;
  bst_object_set (clink->tag_start,
		  "x1", x1,
		  "y1", y1,
		  "x2", x2,
		  "y2", y2,
		  NULL);
  x1 = points[2] - TAG_DIAMETER;
  y1 = points[3] - TAG_DIAMETER;
  x2 = points[2] + TAG_DIAMETER;
  y2 = points[3] + TAG_DIAMETER;
  bst_object_set (clink->tag_end,
		  "x1", x1,
		  "y1", y1,
		  "x2", x2,
		  "y2", y2,
		  NULL);
  
  gnome_canvas_points_free (gpoints);
}

static void
bst_canvas_link_adjust_arrow (BstCanvasLink *clink)
{
  GnomeCanvasPoints *gpoints;
  gdouble dx, dy, l, x, y, px, py, cos_theta, sin_theta, *points;
  
  gtk_object_get (GTK_OBJECT (clink->line), "points", &gpoints, NULL);
  if (!gpoints)
    gpoints = gnome_canvas_points_new0 (2);
  points = gpoints->coords;
  
  dx = points[0] - points[2];
  dy = points[1] - points[3];
  l = sqrt (dx * dx + dy * dy);
  x = (points[2] + points[0]) / 2;
  y = (points[3] + points[1]) / 2;
  
  gnome_canvas_points_free (gpoints);
  
  sin_theta = l ? dy / l : 0;
  cos_theta = l ? dx / l : 0;
  px = x - ARROW_LENGTH / 2.0 * cos_theta;
  py = y - ARROW_LENGTH / 2.0 * sin_theta;
  x += ARROW_LENGTH / 2.0 * cos_theta;
  y += ARROW_LENGTH / 2.0 * sin_theta;
  
  gpoints = gnome_canvas_points_new (4);
  points = gpoints->coords;
  
  *(points++) = px;
  *(points++) = py;
  *(points++) = x - ARROW_WIDTH * sin_theta;
  *(points++) = y + ARROW_WIDTH * cos_theta;
  *(points++) = x + ARROW_WIDTH * sin_theta;
  *(points++) = y - ARROW_WIDTH * cos_theta;
  *(points++) = px;
  *(points++) = py;
  
  bst_object_set (clink->arrow, "points", gpoints, NULL);
  gnome_canvas_points_free (gpoints);
}

static gboolean
bst_canvas_link_event (GnomeCanvasItem *item,
		       GdkEvent        *event)
{
  BstCanvasLink *clink = BST_CANVAS_LINK (item);
  gboolean handled = FALSE;
  
  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      if (event->button.button == 2)
	{
	  GdkCursor *fleur;
	  
	  if (clink->ocsource)
	    {
	      clink->start_move_dx = event->button.x;
	      clink->start_move_dy = event->button.y;
	      gnome_canvas_item_w2i (GNOME_CANVAS_ITEM (clink->ocsource),
				     &clink->start_move_dx,
				     &clink->start_move_dy);
	    }
	  if (clink->icsource)
	    {
	      clink->end_move_dx = event->button.x;
	      clink->end_move_dy = event->button.y;
	      gnome_canvas_item_w2i (GNOME_CANVAS_ITEM (clink->icsource),
				     &clink->end_move_dx,
				     &clink->end_move_dy);
	    }
	  clink->in_move = TRUE;
	  
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
      if (clink->in_move && clink->ocsource)
	{
	  gdouble x = event->motion.x, y = event->motion.y;
	  
	  gnome_canvas_item_w2i (GNOME_CANVAS_ITEM (clink->ocsource), &x, &y);
	  gnome_canvas_item_move (GNOME_CANVAS_ITEM (clink->ocsource),
				  x - clink->start_move_dx,
				  y - clink->start_move_dy);
	  BST_OBJECT_ARGS_CHANGED (clink->ocsource);
	  handled = TRUE;
	}
      if (clink->in_move && clink->icsource)
	{
	  gdouble x = event->motion.x, y = event->motion.y;
	  
	  gnome_canvas_item_w2i (GNOME_CANVAS_ITEM (clink->icsource), &x, &y);
	  gnome_canvas_item_move (GNOME_CANVAS_ITEM (clink->icsource),
				  x - clink->end_move_dx,
				  y - clink->end_move_dy);
	  BST_OBJECT_ARGS_CHANGED (clink->icsource);
	  handled = TRUE;
	}
      break;
    case GDK_BUTTON_RELEASE:
      if (event->button.button == 2 && clink->in_move)
	{
	  clink->in_move = FALSE;
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
bst_canvas_link_child_event (GnomeCanvasItem *item,
			     GdkEvent        *event)
{
  BstCanvasLink *clink;
  GtkWidget *widget = GTK_WIDGET (item->canvas);
  gboolean handled = FALSE;
  
  clink = BST_CANVAS_LINK (item);

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
