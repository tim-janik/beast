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
#include "bstcanvaslink.h"
#include "bststatusbar.h"


/* --- prototypes --- */
static void	  bst_snet_router_class_init	(BstSNetRouterClass	*klass);
static void	  bst_snet_router_init		(BstSNetRouter		*router,
						 BstSNetRouterClass     *class);
static void	  bst_snet_router_destroy	(GtkObject		*object);
static GtkWidget* bst_snet_router_build_toolbar	(BstSNetRouter		*router);
static void	  bst_snet_router_item_added    (BstSNetRouter          *router,
						 BseItem                *item,
						 BseContainer           *container);
static gboolean	  bst_snet_router_event		(GtkWidget		*widget,
						 GdkEvent               *event);
static gboolean	  bst_snet_router_root_event    (BstSNetRouter          *router,
						 GdkEvent               *event);
static void	  bst_snet_router_reset_mode	(BstSNetRouter		*router);
static void	  bst_snet_router_update_links	(BstSNetRouter		*router,
						 BstCanvasSource        *csource);
static void	  bst_snet_router_adjust_zoom	(BstSNetRouter		*router);


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
      
      snet_router_type = gtk_type_unique (GNOME_TYPE_CANVAS, &snet_router_info);
    }
  
  return snet_router_type;
}

static void
bst_snet_router_class_init (BstSNetRouterClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  
  parent_class = gtk_type_class (GNOME_TYPE_CANVAS);
  object_class = GTK_OBJECT_CLASS (class);
  widget_class = GTK_WIDGET_CLASS (class);
  bst_snet_router_class = class;
  
  object_class->destroy = bst_snet_router_destroy;
  
  widget_class->event = bst_snet_router_event;
  
  class->tooltips = NULL;
}

static void
bst_snet_router_init (BstSNetRouter      *router,
		      BstSNetRouterClass *class)
{
  GnomeCanvas *canvas = GNOME_CANVAS (router);
  
  canvas->aa = TRUE;
  router->toolbar = NULL;
  router->adjustment = NULL;
  router->snet = NULL;
  router->mode = 0;
  router->edit_radio = NULL;
  router->world_x = 0;
  router->world_y = 0;
  router->ochannel_id = 0;
  router->ocsource = NULL;
  router->tmp_line = NULL;
  router->link_list = NULL;
  
  bst_object_set (GTK_OBJECT (canvas->root),
		  "object_signal::event", bst_snet_router_root_event, router,
		  NULL);
  bst_object_set (GTK_OBJECT (router),
		  "signal_after::show", bst_snet_router_reset_mode, NULL,
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
  
  router->adjustment = (GtkAdjustment*) gtk_adjustment_new (1.0, 0.20, 5.00, 0.05, 0.50, 0.50);
  gtk_object_set (GTK_OBJECT (router->adjustment),
		  "object_signal::value_changed", bst_snet_router_adjust_zoom, router,
		  "object_signal::destroy", bse_nullify_pointer, &router->adjustment,
		  NULL);
  router->toolbar = bst_snet_router_build_toolbar (router);
  gtk_widget_set (router->toolbar,
		  "object_signal::destroy", bse_nullify_pointer, &router->toolbar,
		  NULL);
}

static void
bst_snet_router_destroy_contents (BstSNetRouter *router)
{
  GnomeCanvas *canvas = GNOME_CANVAS (router);
  GnomeCanvasGroup *group = GNOME_CANVAS_GROUP (canvas->root);
  GSList *slist;

  for (slist = router->link_list; slist; slist = slist->next)
    gtk_object_destroy (slist->data);
  g_slist_free (router->link_list);
  router->link_list = NULL;
  
  while (group->item_list)
    gtk_object_destroy (group->item_list->data);
}

static void
bst_snet_router_destroy (GtkObject *object)
{
  BstSNetRouter *router = BST_SNET_ROUTER (object);
  
  bst_snet_router_reset_mode (router);
  
  bst_snet_router_destroy_contents (router);

  bst_snet_router_set_snet (router, NULL);

  if (router->toolbar)
    gtk_widget_destroy (router->toolbar);
  
  gtk_object_unref (GTK_OBJECT (BST_SNET_ROUTER_GET_CLASS (router)->tooltips));
  
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget*
bst_snet_router_new (BseSNet *snet)
{
  GtkWidget *router;
  
  g_return_val_if_fail (BSE_IS_SNET (snet), NULL);
  
  router = gtk_widget_new (BST_TYPE_SNET_ROUTER, NULL);
  bst_snet_router_set_snet (BST_SNET_ROUTER (router), snet);
  
  return router;
}

void
bst_snet_router_set_snet (BstSNetRouter *router,
			  BseSNet       *snet)
{
  g_return_if_fail (BST_IS_SNET_ROUTER (router));
  if (snet)
    g_return_if_fail (BSE_IS_SNET (snet));
  
  if (router->snet)
    {
      BseObject *object = BSE_OBJECT (router->snet);
      
      bst_snet_router_destroy_contents (router);
      
      bse_object_remove_notifiers_by_func (object,
					   bst_snet_router_item_added,
					   router);
      
      router->snet = NULL;
      bse_object_unref (object);
    }
  if (snet)
    {
      BseObject *object = BSE_OBJECT (snet);
      gfloat zoom;
      
      bse_object_ref (object);
      router->snet = snet;
      
      bse_object_add_data_notifier (BSE_CONTAINER (snet),
				    "item_added",
				    bst_snet_router_item_added,
				    router);
      
      bst_snet_router_rebuild (BST_SNET_ROUTER (router));
      if (bse_parasite_get_floats (BSE_OBJECT (router->snet), "BstRouterZoom", 1, &zoom) == 1)
	gtk_adjustment_set_value (router->adjustment, zoom);
    }
}

void
bst_snet_router_rebuild (BstSNetRouter *router)
{
  g_return_if_fail (BST_IS_SNET_ROUTER (router));
  
  bst_snet_router_update (router);

  bst_snet_router_adjust_region (router);
}

static void
bst_snet_router_item_added (BstSNetRouter *router,
			    BseItem       *item,
			    BseContainer  *container)
{
  GnomeCanvas *canvas = GNOME_CANVAS (router);
  GnomeCanvasItem *csource;
  
  if (!BSE_IS_SOURCE (item))
    {
      g_warning ("Can't handle non-source snet items");
      return;
    }
  
  csource = bst_canvas_source_new (GNOME_CANVAS_GROUP (canvas->root),
				   BSE_SOURCE (item),
				   router->world_x,
				   router->world_y);
  gtk_object_set (GTK_OBJECT (csource),
		  "object_signal::update_links", bst_snet_router_update_links, router,
		  NULL);
  bst_canvas_source_update_links (BST_CANVAS_SOURCE (csource));
}

static gboolean
walk_itmes (BseItem  *item,
	    gpointer  data_p)
{
  gpointer *data = data_p;
  BstSNetRouter *router = BST_SNET_ROUTER (data[0]);
  GnomeCanvas *canvas = GNOME_CANVAS (router);
  
  if (!BSE_IS_SOURCE (item))
    g_warning ("Can't handle non-source snet items");
  else
    {
      GnomeCanvasItem *csource;
      
      csource = bst_canvas_source_new (GNOME_CANVAS_GROUP (canvas->root),
				       BSE_SOURCE (item),
				       router->world_x,
				       router->world_y);
      gtk_object_set (GTK_OBJECT (csource),
		      "object_signal::update_links", bst_snet_router_update_links, router,
		      NULL);
      data[1] = g_slist_prepend (data[1], csource);
    }
  
  return TRUE;
}

void
bst_snet_router_update (BstSNetRouter *router)
{
  GnomeCanvasItem *csource;
  GnomeCanvas *canvas;
  GSList *slist;
  gpointer data[2];
  
  g_return_if_fail (BST_IS_SNET_ROUTER (router));
  
  canvas = GNOME_CANVAS (router);
  
  bst_snet_router_destroy_contents (router);

  /* add the snet itself */
  csource = bst_canvas_source_new (GNOME_CANVAS_GROUP (canvas->root),
				   BSE_SOURCE (router->snet),
				   0, 0);
  gtk_object_set (GTK_OBJECT (csource),
		  "object_signal::update_links", bst_snet_router_update_links, router,
		  NULL);
  
  /* add all child sources */
  data[0] = router;
  data[1] = g_slist_prepend (NULL, csource);
  bse_container_forall_items (BSE_CONTAINER (router->snet),
			      walk_itmes,
			      data);

  /* update all links */
  for (slist = data[1]; slist; slist = slist->next)
    bst_canvas_source_update_links (BST_CANVAS_SOURCE (slist->data));
  g_slist_free (data[1]);
}

static void
toolbar_radio_toggle (GtkWidget *radio,
		      guint     *radio_active)
{
  if (GTK_TOGGLE_BUTTON (radio)->active)
    {
      *radio_active = GPOINTER_TO_UINT (gtk_object_get_user_data (GTK_OBJECT (radio)));
      if (*radio_active)
	bst_status_set (0, bse_type_name (*radio_active), "Select Position");
      else
	bst_status_set (0, "Edit", NULL);
    }
}

#define EPSILON 1e-6

static gboolean
idle_zoom (gpointer data)
{
  GnomeCanvas *canvas = GNOME_CANVAS (data);
  gdouble *d = gtk_object_get_data (GTK_OBJECT (canvas), "zoom_d");
  
  if (!GTK_OBJECT_DESTROYED (canvas) &&
      EPSILON < fabs (canvas->pixels_per_unit - *d))
    gnome_canvas_set_pixels_per_unit (canvas, *d);
  
  gtk_object_remove_data (GTK_OBJECT (canvas), "zoom_d");
  
  return FALSE;
}

static void
bst_snet_router_adjust_zoom (BstSNetRouter *router)
{
  GtkObject *object = GTK_OBJECT (router);
  gdouble *d = gtk_object_get_data (object, "zoom_d");

  if (router->snet)
    {
      gfloat zoom = router->adjustment->value;

      bse_parasite_set_floats (BSE_OBJECT (router->snet), "BstRouterZoom", 1, &zoom);
    }
  
  if (!d)
    {
      d = g_new (gdouble, 1);
      gtk_object_set_data_full (object, "zoom_d", d, g_free);
      gtk_object_ref (object);
      g_timeout_add_full (G_PRIORITY_LOW, 250,
			  idle_zoom,
			  object,
			  (GDestroyNotify) gtk_object_unref);
    }
  *d = router->adjustment->value;
}

static GtkWidget*
toolbar_add_radio (BstSNetRouter *router,
		   GtkWidget	 *parent,
		   GtkWidget     *last_radio,
		   guint          activation_id,
		   const gchar   *name,
		   const gchar   *tip,
		   const BseIcon *icon,
		   GtkTooltips   *tooltips,
		   guint         *radio_active)
{
  GtkWidget *button, *forest;
  
  if (!icon)
    icon = bst_icon_from_stock (BST_ICON_NOICON);
  
  if (name[0] == 'B' && name[1] == 's' && name[2] == 'e')
    name += 3;
  
  forest = gtk_widget_new (GNOME_TYPE_FOREST,
			   "visible", TRUE,
			   "width", 32,
			   "height", 32,
			   "expand_forest", FALSE,
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
  
  button = gtk_toolbar_append_element (GTK_TOOLBAR (parent),
				       GTK_TOOLBAR_CHILD_RADIOBUTTON, NULL,
				       name,
				       tip, NULL,
				       forest,
				       NULL, NULL);
  gtk_widget_set (button,
		  "group", last_radio,
		  "user_data", GUINT_TO_POINTER (activation_id),
		  "signal::toggled", toolbar_radio_toggle, radio_active,
		  "object_signal::toggled", bst_snet_router_reset_mode, router,
		  NULL);
  toolbar_radio_toggle (button, radio_active);
  
  return button;
}

static GtkWidget*
toolbar_add_category (BstSNetRouter *router,
		      GtkWidget	    *parent,
		      GtkWidget     *last_radio,
		      BseCategory   *category,
		      GtkTooltips   *tooltips,
		      guint         *radio_active)
{
  GtkWidget *radio;
  gchar *tip;
  
  if (bse_type_is_a (category->type, BSE_TYPE_SUPER))
    return last_radio;
  
  tip = g_strconcat (bse_type_name (category->type),
		     " [",
		     category->category + category->mindex + 1,
		     "]\n",
		     bse_type_blurb (category->type),
		     NULL);
  radio = toolbar_add_radio (router, parent, last_radio,
			     category->type,
			     bse_type_name (category->type),
			     tip,
			     category->icon,
			     tooltips,
			     radio_active);
  g_free (tip);
  
  return radio;
}

static GtkWidget*
bst_snet_router_build_toolbar (BstSNetRouter *router)
{
  GtkWidget *bar;
  GtkWidget *radio = NULL;
  BseCategory *cats;
  guint i, n_cats;
  
  g_return_val_if_fail (BST_IS_SNET_ROUTER (router), NULL);
  
  bar = gtk_widget_new (GTK_TYPE_TOOLBAR,
			"visible", TRUE,
			"orientation", GTK_ORIENTATION_HORIZONTAL,
			"toolbar_style", GTK_TOOLBAR_BOTH,
			NULL);
  
  /* add link/move/property edit tool
   */
  radio = toolbar_add_radio (router, bar, radio, 0,
			     "Edit",
			     "Edit\n"
			     "Use button1 to create links, "
			     "button2 for movement and "
			     "button3 to change properties",
			     bst_icon_from_stock (BST_ICON_MOUSE_TOOL),
			     BST_SNET_ROUTER_GET_CLASS (router)->tooltips,
			     &router->mode);
  router->edit_radio = radio;
  gtk_widget_set (router->edit_radio,
		  "object_signal::destroy", bse_nullify_pointer, &router->edit_radio,
		  NULL);
  
  /* add BseSource types from categories
   */
  cats = bse_categories_match ("/Source/*", &n_cats);
  for (i = 0; i < n_cats; i++)
    radio = toolbar_add_category (router, bar, radio, cats + i,
				  BST_SNET_ROUTER_GET_CLASS (router)->tooltips,
				  &router->mode);
  g_free (cats);
  
  /* FIXME: test hackery
   */
  if (1)
    {
#include "../bse/icons/song.c"
      BseIcon icon = { gimp_image.bytes_per_pixel, ~0,
		       gimp_image.width, gimp_image.height,
		       (void*) gimp_image.pixel_data };
      radio = toolbar_add_radio (router, bar, radio, 0, "Song", NULL,
				 &icon,
				 BST_SNET_ROUTER_GET_CLASS (router)->tooltips,
				 &router->mode);
    }
  
  /* add Zoom: spinner */
  radio = gtk_spin_button_new (router->adjustment, 0.0, 2);
  gtk_widget_set_usize (radio, 50, 0);
  gtk_widget_show (radio);
  gtk_toolbar_append_widget (GTK_TOOLBAR (bar), radio, "Zoom Factor", NULL);
  
  return bar;
}

void
bst_snet_router_adjust_region (BstSNetRouter *router)
{
  GtkAdjustment *adjustment;
  GtkLayout *layout;
  GnomeCanvas *canvas;
  gdouble x1, y1, x2, y2;
  
  g_return_if_fail (BST_IS_SNET_ROUTER (router));
  
  canvas = GNOME_CANVAS (router);
  layout = GTK_LAYOUT (router);
  
  gnome_canvas_request_full_update (canvas);
  gnome_canvas_update_now (canvas);
  gnome_canvas_item_get_bounds (canvas->root, &x1, &y1, &x2, &y2);
  
  /* add fudge */
  x1 -= 1; y1 -= 1; x2 += 1;  y2 += 1;
  
  gnome_canvas_set_scroll_region (canvas, x1, y1, x2, y2);
  gnome_canvas_request_full_update (canvas);
  
  adjustment = gtk_layout_get_hadjustment (layout);
  gtk_adjustment_set_value (adjustment,
			    (adjustment->upper - adjustment->lower) / 2 -
			    adjustment->page_size / 2);
  adjustment = gtk_layout_get_vadjustment (layout);
  gtk_adjustment_set_value (adjustment,
			    (adjustment->upper - adjustment->lower) / 2 -
			    adjustment->page_size / 2);
}

BstCanvasSource*
bst_snet_router_csource_from_source (BstSNetRouter *router,
				     BseSource     *source)
{
  GnomeCanvas *canvas;
  GnomeCanvasGroup *root;
  GList *list;

  g_return_val_if_fail (BST_IS_SNET_ROUTER (router), NULL);
  g_return_val_if_fail (BSE_IS_SOURCE (source), NULL);

  canvas = GNOME_CANVAS (router);
  root = GNOME_CANVAS_GROUP (canvas->root);
  for (list = root->item_list; list; list = list->next)
    {
      BstCanvasSource *csource = list->data;

      if (BST_IS_CANVAS_SOURCE (csource) && csource->source == source)
	return csource;
    }

  return NULL;
}

static void
bst_snet_router_update_links (BstSNetRouter   *router,
			      BstCanvasSource *csource)
{
  GnomeCanvas *canvas = GNOME_CANVAS (router);
  GSList *slist, *tmp_list, *ilist;
  BseSource *source = csource->source;
  guint i;

  /* sort out input links for csource */
  ilist = NULL;
  tmp_list = router->link_list;
  router->link_list = NULL;
  while (tmp_list)
    {
      BstCanvasLink *link = tmp_list->data;

      slist = tmp_list;
      tmp_list = tmp_list->next;
      if (link->icsource == csource)
	{
	  slist->next = ilist;
	  ilist = slist;
	}
      else
	{
	  slist->next = router->link_list;
	  router->link_list = slist;
	}
    }

  /* ok, we now walk the (c)source's input channels, keep
   * existing links and create new ones on the fly
   */
  for (i = 0; i < source->n_inputs; i++)
    {
      BseSourceInput *input = source->inputs + i;
      BstCanvasSource *ocsource = bst_snet_router_csource_from_source (router, input->osource);
      BstCanvasLink *link = NULL;

      if (!ocsource)
	{
	  g_warning ("Couldn't figure CanvasSource Item From BseSource");
	  continue;
	}
      
      /* find corresponding link */
      for (slist = ilist; slist; slist = slist->next)
	{
	  link = slist->data;
	  if (!link)
	    continue;

	  if (link->ichannel_id == input->ichannel_id &&
	      link->ocsource == ocsource &&
	      link->ochannel_id == input->ochannel_id)
	    break;
	}
      if (slist) /* cool, found one already */
	slist->data = NULL;
      else /* got none, ok, need to create new one */
	{
	  link = (BstCanvasLink*) bst_canvas_link_new (GNOME_CANVAS_GROUP (canvas->root));
	  bst_canvas_link_set_icsource (link, csource, input->ichannel_id);
	  bst_canvas_link_set_ocsource (link, ocsource, input->ochannel_id);
	}
      router->link_list = g_slist_prepend (router->link_list, link);
    }

  /* gotta nuke outdated links now */
  for (slist = ilist; slist; slist = slist->next)
    if (slist->data)
      gtk_object_destroy (slist->data);
  g_slist_free (ilist);
}

static void
update_tmp_line (BstSNetRouter *router)
{
  GnomeCanvasPoints *gpoints = NULL;

  if (router->tmp_line)
    gtk_object_get (GTK_OBJECT (router->tmp_line), "points", &gpoints, NULL);
  if (gpoints)
    {
      gpoints->coords[2] = router->world_x;
      gpoints->coords[3] = router->world_y;
      if (gpoints->coords[0] > gpoints->coords[2])
	gpoints->coords[2] += 0.5;
      else
	gpoints->coords[2] -= 0.5;
      if (gpoints->coords[1] > gpoints->coords[3])
	gpoints->coords[3] += 0.5;
      else
	gpoints->coords[3] -= 0.5;
      bst_object_set (GTK_OBJECT (router->tmp_line), "points", gpoints, NULL);
      gnome_canvas_points_free (gpoints);
    }
}

static void
update_mode (BstSNetRouter *router)
{
  GnomeCanvas *canvas = GNOME_CANVAS (router);

  update_tmp_line (router);

  if (GTK_WIDGET_REALIZED (canvas))
    {
      GdkCursor *cursor;
      
      if (router->mode > 1)
	cursor = gdk_cursor_new (GDK_UL_ANGLE);
      else if (router->mode == 1)
	cursor = gdk_cursor_new (GDK_TCROSS);
      else
	cursor = NULL;
      
      gdk_window_set_cursor (GTK_WIDGET (canvas)->window, cursor);
      
      if (cursor)
	gdk_cursor_destroy (cursor);
    }
}

static void
bst_snet_router_reset_mode (BstSNetRouter *router)
{
  if (router->mode == 1)
    router->mode = 0;
  if (router->tmp_line)
    {
      gtk_object_destroy (GTK_OBJECT (router->tmp_line));
      bst_status_set (0, NULL, NULL);
    }

  update_mode (router);
}

static gboolean
bst_snet_router_root_event (BstSNetRouter   *router,
			    GdkEvent        *event)
{
  GnomeCanvas *canvas = GNOME_CANVAS (router);
  gboolean handled = FALSE;

  if (event->type == GDK_BUTTON_PRESS  &&
      event->button.button == 1 &&
      router->mode == 0) /* start link */
    {
      GnomeCanvasItem *item;
      BstCanvasSource *csource = NULL;

      g_return_val_if_fail (router->tmp_line == NULL, FALSE);
      
      item = gnome_canvas_get_item_at (canvas, event->button.x, event->button.y);
      if (item)
	csource = gtk_object_get_data (GTK_OBJECT (item), "csource_oconnector");
      
      if (csource)
	{
	  GnomeCanvasPoints *gpoints = gnome_canvas_points_new (2);

	  router->mode = 1;
	  router->ochannel_id = 1;
	  router->ocsource = csource;
	  bst_canvas_source_ochannel_pos (csource,
					  router->ochannel_id,
					  &gpoints->coords[0],
					  &gpoints->coords[1]);
	  gpoints->coords[2] = gpoints->coords[0] + 50;
	  gpoints->coords[3] = gpoints->coords[1] + 50;
	  router->tmp_line =
	    gnome_canvas_item_new (GNOME_CANVAS_GROUP (canvas->root),
				   GNOME_TYPE_CANVAS_LINE,
				   "fill_color", "black",
				   "object_signal::destroy", bse_nullify_pointer, &router->tmp_line,
				   "points", gpoints,
				   NULL);
	  gnome_canvas_points_free (gpoints);
	  router->world_x = event->button.x;
	  router->world_y = event->button.y;
          bst_status_set (0, "Create Link", "Select input source");
	  update_mode (router);
	  handled = TRUE;
	}
    }
  else if ((event->type == GDK_BUTTON_PRESS ||
	    event->type == GDK_BUTTON_RELEASE) &&
	   event->button.button == 1 &&
	   router->mode == 1) /* finish link */
    {
      GnomeCanvasItem *item;
      BstCanvasSource *csource = NULL;
      
      item = gnome_canvas_get_item_at (canvas, event->button.x, event->button.y);
      if (item)
	csource = gtk_object_get_data (GTK_OBJECT (item), "csource_iconnector");
      
      if (csource)
	{
	  guint ichannel_id = 1;
	  BseErrorType error;

	  error = bse_source_set_input (csource->source, ichannel_id,
					router->ocsource->source,
					router->ochannel_id);
	  bst_snet_router_reset_mode (router);
	  bst_status_set (error ? 0 : 100, "Create Link", bse_error_blurb (error));
	}
      handled = TRUE;
    }
  else if (event->type == GDK_BUTTON_PRESS &&
	   event->button.button == 3)
    {
      GnomeCanvasItem *item;
      BstCanvasSource *csource = NULL;

      item = gnome_canvas_get_item_at (canvas, event->button.x, event->button.y);
      if (item)
	csource = gtk_object_get_data (GTK_OBJECT (item), "csource");

      if (csource && csource->source != (BseSource*) router->snet)
	bst_canvas_source_popup_view (csource);
    }
  
  return handled;
}

static gboolean
bst_snet_router_event (GtkWidget *widget,
		       GdkEvent  *event)
{
  BstSNetRouter *router = BST_SNET_ROUTER (widget);
  GnomeCanvas *canvas = GNOME_CANVAS (router);
  gboolean handled = FALSE;
  
  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      if (event->button.button == 1 && router->mode > 1) /* add new source */
	{
	  handled = TRUE;
	  gnome_canvas_window_to_world (canvas,
					event->button.x, event->button.y,
					&router->world_x, &router->world_y);
	  bse_snet_new_source (router->snet,
			       router->mode,
			       NULL);
	  router->world_x = 0;
	  router->world_y = 0;
	  if (1 && router->edit_radio) /* FIXME: need preference settings for this */
	    gtk_button_clicked (GTK_BUTTON (router->edit_radio));
	}
      else if (router->mode > 1 || (router->mode == 1 && event->button.button != 1))
	{
	  /* disturbing button presses, reset stuff */
	  handled = TRUE;
	  bst_snet_router_reset_mode (router);
	}
      break;
    case GDK_MOTION_NOTIFY:
      gnome_canvas_window_to_world (canvas,
				    event->motion.x, event->motion.y,
				    &router->world_x, &router->world_y);
      update_tmp_line (router);
      break;
    default:
      break;
    }
  
  if (!handled && GTK_WIDGET_CLASS (parent_class)->event)
    handled = GTK_WIDGET_CLASS (parent_class)->event (widget, event);
  
  return handled;
}
