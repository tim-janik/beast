/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2002 Tim Janik
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
  clink->ochannel = ~0;
  clink->oc_handler = 0;
  clink->icsource = NULL;
  clink->ichannel = ~0;
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

BstCanvasSource*
bst_canvas_link_csource_at (BstCanvasLink *clink,
			    gdouble        world_x,
			    gdouble        world_y)
{
  GnomeCanvasItem *tag;

  g_return_val_if_fail (BST_IS_CANVAS_LINK (clink), NULL);

  tag = gnome_canvas_typed_item_at (GNOME_CANVAS_ITEM (clink)->canvas, GNOME_TYPE_CANVAS_ELLIPSE, world_x, world_y);
  if (tag && tag == clink->tag_start)
    return clink->ocsource;
  else if (tag && tag == clink->tag_end)
    return clink->icsource;
  else
    return NULL;
}

static void
clink_view_update (BstCanvasLink *clink,
		   gboolean       force_update)
{
  GtkWidget *frame = (clink->link_view && (force_update || GTK_WIDGET_VISIBLE (clink->link_view))
		      ? gxk_dialog_get_child (GXK_DIALOG (clink->link_view))
		      : NULL);

  if (frame)
    {
      GtkWidget *text = GTK_BIN (frame)->child;
      const gchar *ic_name, *oc_name, *ic_blurb, *oc_blurb, *iname, *oname;
      gchar *string;

      /* figure appropriate window title
       */
      iname = clink->icsource ? bse_item_get_name_or_type (clink->icsource->source) : "<""???"">";
      oname = clink->ocsource ? bse_item_get_name_or_type (clink->ocsource->source) : "<""???"">";
      string = g_strconcat (_("Module Link: "), iname, " <=> ", oname, NULL);
      gxk_dialog_set_title (GXK_DIALOG (clink->link_view), string);
      g_free (string);

      /* construct actuall information
       */
      oc_name = clink->ocsource ? bse_source_ochannel_name (clink->ocsource->source, clink->ochannel) : NULL;
      oc_blurb = clink->ocsource ? bse_source_ochannel_blurb (clink->ocsource->source, clink->ochannel) : NULL;
      ic_name = clink->icsource ? bse_source_ichannel_name (clink->icsource->source, clink->ichannel) : NULL;
      ic_blurb = clink->icsource ? bse_source_ichannel_blurb (clink->icsource->source, clink->ichannel) : NULL;
      if (!oc_name)
	oc_name = "?";
      if (!ic_name)
	ic_name = "?";

      /* compose new info */
      gxk_scroll_text_clear (text);
      gxk_scroll_text_aprintf (text, "Source Module:\n");
      gxk_scroll_text_push_indent (text);
      if (oc_blurb)
	{
	  gxk_scroll_text_aprintf (text, "%s: %s:\n", oname, oc_name);
	  gxk_scroll_text_push_indent (text);
	  gxk_scroll_text_aprintf (text, "%s\n", oc_blurb);
	  gxk_scroll_text_pop_indent (text);
	}
      else
	gxk_scroll_text_aprintf (text, "%s: %s\n", oname, oc_name);
      gxk_scroll_text_pop_indent (text);
      gxk_scroll_text_aprintf (text, "\nDestination Module:\n");
      gxk_scroll_text_push_indent (text);
      if (ic_blurb)
	{
	  gxk_scroll_text_aprintf (text, "%s: %s:\n", iname, ic_name);
	  gxk_scroll_text_push_indent (text);
	  gxk_scroll_text_aprintf (text, "%s\n", ic_blurb);
	  gxk_scroll_text_pop_indent (text);
	}
      else
	gxk_scroll_text_aprintf (text, "%s: %s\n", iname, ic_name);
    }
}

void
bst_canvas_link_popup_view (BstCanvasLink *clink)
{
  g_return_if_fail (BST_IS_CANVAS_LINK (clink));
  
  if (!clink->link_view)
    clink->link_view = gxk_dialog_new (&clink->link_view,
				       GTK_OBJECT (clink),
				       0,
				       NULL,
				       gtk_widget_new (GTK_TYPE_FRAME,
						       "visible", TRUE,
						       "border_width", 5,
						       "label", _("Module link"),
						       "child", gxk_scroll_text_create (GXK_SCROLL_TEXT_WIDGET_LOOK, NULL),
						       NULL));
  clink_view_update (clink, TRUE);
  gxk_widget_showraise (clink->link_view);
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
			      guint            ochannel)
{
  g_return_if_fail (BST_IS_CANVAS_LINK (clink));
  if (ocsource)
    g_return_if_fail (BST_CANVAS_SOURCE (ocsource));
  
  if (clink->ocsource)
    {
      if (clink->ocsource->source) /* source may be destroyed already */
	bse_proxy_disconnect (clink->ocsource->source,
			      "any_signal", clink_view_check_update, clink,
			      NULL);
      if (g_signal_handler_is_connected (clink->ocsource, clink->oc_handler))
	gtk_signal_disconnect (GTK_OBJECT (clink->ocsource), clink->oc_handler);
      gtk_object_unref (GTK_OBJECT (clink->ocsource));
    }
  clink->ocsource = ocsource;
  clink->ochannel = ochannel;
  if (clink->ocsource)
    {
      gtk_object_ref (GTK_OBJECT (clink->ocsource));
      clink->oc_handler = gtk_signal_connect_object (GTK_OBJECT (clink->ocsource),
						     "notify",
						     G_CALLBACK (bst_canvas_link_update),
						     GTK_OBJECT (clink));
      bse_proxy_connect (clink->ocsource->source,
			 "swapped_signal::property-notify::uname", clink_view_check_update, clink,
			 NULL);
      bst_canvas_link_update (clink);
    }
}

void
bst_canvas_link_set_icsource (BstCanvasLink   *clink,
			      BstCanvasSource *icsource,
			      guint            ichannel)
{
  g_return_if_fail (BST_IS_CANVAS_LINK (clink));
  if (icsource)
    g_return_if_fail (BST_CANVAS_SOURCE (icsource));
  
  if (clink->icsource)
    {
      if (clink->icsource->source) /* source may be destroyed already */
	bse_proxy_disconnect (clink->icsource->source,
			      "any_signal", clink_view_check_update, clink,
			      NULL);
      if (g_signal_handler_is_connected (clink->icsource, clink->ic_handler))
        gtk_signal_disconnect (GTK_OBJECT (clink->icsource), clink->ic_handler);
      gtk_object_unref (GTK_OBJECT (clink->icsource));
    }
  clink->icsource = icsource;
  clink->ichannel = ichannel;
  if (clink->icsource)
    {
      gtk_object_ref (GTK_OBJECT (clink->icsource));
      clink->ic_handler = gtk_signal_connect_object (GTK_OBJECT (clink->icsource),
						     "notify",
						     G_CALLBACK (bst_canvas_link_update),
						     GTK_OBJECT (clink));
      bse_proxy_connect (clink->icsource->source,
			 "swapped_signal::property-notify::uname", clink_view_check_update, clink,
			 NULL);
      bst_canvas_link_update (clink);
    }
}

static void
bst_canvas_link_update (BstCanvasLink *clink)
{
  GnomeCanvasItem *item = GNOME_CANVAS_ITEM (clink);
  GnomeCanvasPoints *gpoints;
  gdouble start_x = 0, start_y = 0, end_x = 10, end_y = 10;
  gboolean is_jchannel = FALSE;

  if (clink->ocsource)
    {
      bst_canvas_source_ochannel_pos (clink->ocsource, clink->ochannel, &start_x, &start_y);
      gnome_canvas_item_w2i (item, &start_x, &start_y);
    }
  if (clink->icsource)
    {
      bst_canvas_source_ichannel_pos (clink->icsource, clink->ichannel, &end_x, &end_y);
      is_jchannel = bst_canvas_source_is_jchannel (clink->icsource, clink->ichannel);
      gnome_canvas_item_w2i (item, &end_x, &end_y);
    }
  if (clink->ocsource && clink->icsource)
    gnome_canvas_item_keep_above (GNOME_CANVAS_ITEM (clink),
				  GNOME_CANVAS_ITEM (clink->ocsource),
				  GNOME_CANVAS_ITEM (clink->icsource));
  
  
  if (!clink->arrow)
    clink->arrow = g_object_connect (gnome_canvas_item_new (GNOME_CANVAS_GROUP (clink),
							    GNOME_TYPE_CANVAS_POLYGON,
							    "outline_color_rgba", 0x0000ffff,
							    "fill_color_rgba", 0xff0000ff,
							    NULL),
				     "signal::destroy", gtk_widget_destroyed, &clink->arrow,
				     "swapped_signal::event", bst_canvas_link_child_event, clink,
				     NULL);
  if (!clink->tag_start)
    clink->tag_start = g_object_connect (gnome_canvas_item_new (GNOME_CANVAS_GROUP (clink),
								GNOME_TYPE_CANVAS_ELLIPSE,
								"outline_color_rgba", 0x000000ff, // xffff00ff,
								"fill_color_rgba", 0xffff00ff,
								NULL),
					 "signal::destroy", gtk_widget_destroyed, &clink->tag_start,
					 "swapped_signal::event", bst_canvas_link_child_event, clink,
					 NULL);
  if (!clink->tag_end)
    clink->tag_end = g_object_connect (gnome_canvas_item_new (GNOME_CANVAS_GROUP (clink),
							      GNOME_TYPE_CANVAS_ELLIPSE,
							      "outline_color_rgba", 0x000000ff, // 0xff0000ff,
							      "fill_color_rgba", is_jchannel ? 0x00ff00ff : 0xff0000ff,
							      NULL),
				       "signal::destroy", gtk_widget_destroyed, &clink->tag_end,
				       "swapped_signal::event", bst_canvas_link_child_event, clink,
				       NULL);
  if (!clink->line)
    clink->line = g_object_connect (gnome_canvas_item_new (GNOME_CANVAS_GROUP (clink),
							   GNOME_TYPE_CANVAS_LINE,
							   "fill_color", "black",
							   NULL),
				    "signal::destroy", gtk_widget_destroyed, &clink->line,
				    "swapped_signal::notify", bst_canvas_link_adjust_arrow, clink,
				    "swapped_signal::notify", bst_canvas_link_adjust_tags, clink,
				    NULL);
  gpoints = gnome_canvas_points_new (2);
  gpoints->coords[0] = start_x;
  gpoints->coords[1] = start_y;
  gpoints->coords[2] = end_x;
  gpoints->coords[3] = end_y;
  g_object_set (clink->line,
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
  g_object_set (clink->tag_start,
		"x1", x1,
		"y1", y1,
		"x2", x2,
		"y2", y2,
		NULL);
  x1 = points[2] - TAG_DIAMETER;
  y1 = points[3] - TAG_DIAMETER;
  x2 = points[2] + TAG_DIAMETER;
  y2 = points[3] + TAG_DIAMETER;
  g_object_set (clink->tag_end,
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
  
  g_object_set (clink->arrow, "points", gpoints, NULL);
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
	  GNOME_CANVAS_NOTIFY (clink->ocsource);
	  handled = TRUE;
	}
      if (clink->in_move && clink->icsource)
	{
	  gdouble x = event->motion.x, y = event->motion.y;
	  
	  gnome_canvas_item_w2i (GNOME_CANVAS_ITEM (clink->icsource), &x, &y);
	  gnome_canvas_item_move (GNOME_CANVAS_ITEM (clink->icsource),
				  x - clink->end_move_dx,
				  y - clink->end_move_dy);
	  GNOME_CANVAS_NOTIFY (clink->icsource);
	  handled = TRUE;
	}
      break;
    case GDK_BUTTON_RELEASE:
      if (event->button.button == 2 && clink->in_move)
	{
	  clink->in_move = FALSE;
	  gnome_canvas_item_ungrab (item, event->button.time);
	  handled = TRUE;
	}
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
	  GNOME_CANVAS_NOTIFY (item);
	  break;
	case 'l':
	  gnome_canvas_item_lower_to_bottom (item);
	  GNOME_CANVAS_NOTIFY (item);
	  break;
	case 'R':
	  gnome_canvas_item_raise (item, 1);
	  GNOME_CANVAS_NOTIFY (item);
	  break;
	case 'r':
	  gnome_canvas_item_raise_to_top (item);
	  GNOME_CANVAS_NOTIFY (item);
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
