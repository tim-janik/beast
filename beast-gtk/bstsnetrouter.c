/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999, 2000 Olaf Hoehmann and Tim Janik
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
#include "bstsnetrouter.h"

#include <math.h>
#include "bstcanvaslink.h"
#include "bststatusbar.h"
#include "bstmenus.h"
#include "bstgconfig.h"


#define EPSILON 1e-6


/* --- prototypes --- */
static void	  bst_snet_router_class_init	(BstSNetRouterClass	*klass);
static void	  bst_snet_router_init		(BstSNetRouter		*router,
						 BstSNetRouterClass     *class);
static void	  bst_snet_router_destroy	(GtkObject		*object);
static void       bst_snet_router_build_tools	(BstSNetRouter		*router);
static void	  bst_snet_router_item_added    (BstSNetRouter          *router,
						 BseItem                *item,
						 BseContainer           *container);
static gboolean	  bst_snet_router_event		(GtkWidget		*widget,
						 GdkEvent               *event);
static gboolean	  bst_snet_router_root_event    (BstSNetRouter          *router,
						 GdkEvent               *event);
static void	  bst_snet_router_reset_tool	(BstSNetRouter		*router);
static void	  bst_snet_router_update_links	(BstSNetRouter		*router,
						 BstCanvasSource        *csource);
static void	  bst_snet_router_adjust_zoom	(BstSNetRouter		*router);
static void	  bst_router_set_tool		(BstSNetRouter		*router);
     

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
  
  canvas->aa = BST_SNET_ANTI_ALIASED;
  router->toolbar = NULL;
  router->palette = NULL;
  router->adjustment = NULL;
  router->snet = NULL;
  router->world_x = 0;
  router->world_y = 0;
  router->ochannel_id = 0;
  router->ocsource = NULL;
  router->tmp_line = NULL;
  router->link_list = NULL;

  router->rtools = bst_radio_tools_new ();
  gtk_object_ref (GTK_OBJECT (router->rtools));
  gtk_object_sink (GTK_OBJECT (router->rtools));
  gtk_object_set (GTK_OBJECT (router->rtools),
		  "object_signal_after::set_tool", bst_router_set_tool, router,
		  NULL);
  
  bst_object_set (GTK_OBJECT (canvas->root),
		  "object_signal::event", bst_snet_router_root_event, router,
		  NULL);
  bst_object_set (GTK_OBJECT (router),
		  "signal_after::show", bst_snet_router_reset_tool, NULL,
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

  bst_snet_router_build_tools (router);
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
  
  bst_snet_router_reset_tool (router);
  
  bst_snet_router_destroy_contents (router);

  bst_snet_router_set_snet (router, NULL);

  if (router->toolbar)
    gtk_widget_destroy (router->toolbar);
  if (router->palette)
    gtk_widget_destroy (router->palette);
  
  gtk_object_unref (GTK_OBJECT (router->rtools));
  router->rtools = NULL;

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
palette_destroyed (BstSNetRouter *router)
{
  if (!GTK_OBJECT_DESTROYED (router) && router->rtools)
    bst_radio_tools_set_tool (router->rtools, 0);
}

void
bst_snet_router_toggle_palette (BstSNetRouter *router)
{
  g_return_if_fail (BST_IS_SNET_ROUTER (router));

  if (!router->palette || !GTK_WIDGET_VISIBLE (router->palette))
    {
      if (!router->palette)
	router->palette = bst_adialog_new (GTK_OBJECT (router), &router->palette,
					   bst_radio_tools_build_palette (router->rtools, TRUE, GTK_RELIEF_NORMAL),
					   BST_ADIALOG_DESTROY_ON_HIDE,
					   "object_signal::destroy", palette_destroyed, router,
					   "title", "BEAST: Palette",
					   NULL);
      gtk_widget_showraise (router->palette);
    }
  else
    {
      gtk_widget_hide (router->palette);
      bst_radio_tools_set_tool (router->rtools, 0);
    }
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

static void
bst_snet_router_build_tools (BstSNetRouter *router)
{
  GtkWidget *button;
  BseCategory *cats;
  guint i, n_cats;
  
  g_return_if_fail (BST_IS_SNET_ROUTER (router));

  if (router->toolbar)
    gtk_widget_destroy (router->toolbar);
  if (router->palette)
    gtk_widget_destroy (router->palette);

  /* setup radio tools
   */
  bst_radio_tools_clear_tools (router->rtools);
  /* add link/move/property edit tool */
  bst_radio_tools_add_tool (router->rtools,
			    0,
			    "Edit",
			    "Edit tool (mouse buttons 1-3)",
			    ("Edit tool (mouse buttons 1-3)\n"
			     "Use button1 to create links, "
			     "button2 for movement and "
			     "button3 to change properties"),
			    bst_icon_from_stock (BST_ICON_MOUSE_TOOL),
			    BST_RADIO_TOOLS_EVERYWHERE);
  /* add BseSource types from categories */
  cats = bse_categories_match ("/Source/*", &n_cats);
  for (i = 0; i < n_cats; i++)
    {
      static const gchar *toolbar_categories[] = {
	"BseCapture", "BseMixer", "BseSInstrument",
      };
      guint n, n_toolbar_categories = sizeof (toolbar_categories) / sizeof (toolbar_categories[0]);
      gboolean add_to_toolbar = FALSE;

      for (n = 0; n < n_toolbar_categories; n++)
	if (strcmp (toolbar_categories[n], bse_type_name (cats[i].type)) == 0)
	  {
	    add_to_toolbar = TRUE;
	    break;
	  }
      bst_radio_tools_add_category (router->rtools,
				    cats[i].type,
				    cats + i,
				    BST_RADIO_TOOLS_PALETTE |
				    (add_to_toolbar ? BST_RADIO_TOOLS_TOOLBAR : 0));
    }
  g_free (cats);

  /* create toolbar
   */
  router->toolbar = gtk_widget_new (GTK_TYPE_TOOLBAR,
				    "visible", TRUE,
				    "orientation", GTK_ORIENTATION_HORIZONTAL,
				    "toolbar_style", (gnome_preferences_get_toolbar_labels ()
						      ? GTK_TOOLBAR_BOTH
						      : GTK_TOOLBAR_ICONS),
				    "relief", (gnome_preferences_get_toolbar_relief_btn ()
					       ? GTK_RELIEF_NONE
					       : GTK_RELIEF_NORMAL),
				    "space_style", GTK_TOOLBAR_SPACE_LINE,
				    "object_signal::destroy", bse_nullify_pointer, &router->toolbar,
				    NULL);
  
  /* add radios to toolbar
   */
  bst_radio_tools_build_toolbar (router->rtools, GTK_TOOLBAR (router->toolbar));

  /* seperate radios
   */
  gtk_toolbar_append_space (GTK_TOOLBAR (router->toolbar));

  /* add `Palette' button to toolbar
   */
  button = gtk_toolbar_append_element (GTK_TOOLBAR (router->toolbar), GTK_TOOLBAR_CHILD_BUTTON, NULL,
				       "Palette", "Toggle visibility of the tool palette", NULL,
				       bst_forest_from_bse_icon (bst_icon_from_stock (BST_ICON_PALETTE_TOOL), 32, 32),
				       NULL, NULL);
  gtk_widget_set (button,
		  "object_signal::clicked", bst_snet_router_toggle_palette, router,
		  NULL);

  /* add `Zoom' spinner
   */
  button = gtk_spin_button_new (router->adjustment, 0.0, 2);
  gtk_widget_set_usize (button, 50, 0);
  gtk_widget_show (button);
  gtk_toolbar_append_element (GTK_TOOLBAR (router->toolbar), GTK_TOOLBAR_CHILD_WIDGET, button,
			      "Zoom Factor", "Adjust the zoom factor of the router display", NULL,
			      NULL, NULL, NULL);

  /* set default tool
   */
  bst_radio_tools_set_tool (router->rtools, 0);
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
  if (router->tmp_line)
    {
      if (router->rtools->tool_id != 1)
	{
	  gtk_object_destroy (GTK_OBJECT (router->tmp_line));
	  bst_status_set (0, NULL, NULL);
	}
      else
	{
	  GnomeCanvasPoints *gpoints = NULL;

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
    }
}

static void
bst_router_set_tool (BstSNetRouter *router)
{
  GnomeCanvas *canvas = GNOME_CANVAS (router);

  update_tmp_line (router);

  if (GTK_WIDGET_REALIZED (canvas))
    {
      GdkCursor *cursor;
      
      if (router->rtools->tool_id > 1)
	cursor = gdk_cursor_new (GDK_UL_ANGLE);
      else if (router->rtools->tool_id == 1)
	cursor = gdk_cursor_new (GDK_TCROSS);
      else
	cursor = NULL;
      
      gdk_window_set_cursor (GTK_WIDGET (canvas)->window, cursor);
      
      if (cursor)
	gdk_cursor_destroy (cursor);
    }
}

static void
bst_snet_router_reset_tool (BstSNetRouter *router)
{
  if (router->rtools->tool_id == 1)
    bst_radio_tools_set_tool (router->rtools, 0);
}

static gboolean
bst_snet_router_root_event (BstSNetRouter   *router,
			    GdkEvent        *event)
{
  GnomeCanvas *canvas = GNOME_CANVAS (router);
  gboolean handled = FALSE;

  if (event->type == GDK_BUTTON_PRESS &&
      event->button.button == 1 &&
      router->rtools->tool_id == 0) /* start link (or popup source view) */
    {
      BstCanvasSource *csource;
      BstCanvasLink *clink;
      guint ochannel_id, ichannel_id;

      g_return_val_if_fail (router->tmp_line == NULL, FALSE);
      
      csource = bst_canvas_source_at (canvas, event->button.x, event->button.y);
      clink = csource ? NULL : bst_canvas_link_at (canvas, event->button.x, event->button.y);
      if (clink && !csource)
	{
	  csource = bst_canvas_link_has_canvas_source_at (clink, event->button.x, event->button.y);
	  if (csource)
	    clink = NULL;
	}
      ochannel_id = (csource
		     ? bst_canvas_source_ochannel_at (csource, event->button.x, event->button.y)
		     : 0);
      ichannel_id = (csource
		     ? bst_canvas_source_ichannel_at (csource, event->button.x, event->button.y)
		     : 0);
      
      if (csource && ochannel_id)
	{
	  GnomeCanvasPoints *gpoints = gnome_canvas_points_new (2);

	  router->ochannel_id = ochannel_id;
	  router->ocsource = csource;
	  bst_canvas_source_ochannel_pos (csource,
					  router->ochannel_id,
					  &gpoints->coords[0],
					  &gpoints->coords[1]);
	  gpoints->coords[2] = gpoints->coords[0] + 50;
	  gpoints->coords[3] = gpoints->coords[1] + 50;
	  router->tmp_line = gnome_canvas_item_new (GNOME_CANVAS_GROUP (canvas->root),
						    GNOME_TYPE_CANVAS_LINE,
						    "fill_color", "black",
						    "object_signal::destroy", bse_nullify_pointer, &router->tmp_line,
						    "points", gpoints,
						    NULL);
	  gnome_canvas_points_free (gpoints);
	  router->world_x = event->button.x;
	  router->world_y = event->button.y;
          bst_status_set (0, "Create Link", "Select input source");
	  bst_radio_tools_set_tool (router->rtools, 1);
	  handled = TRUE;
	}
      else if (csource &&
	       csource->source != (BseSource*) router->snet &&
	       ichannel_id == 0)
	bst_canvas_source_toggle_view (csource);
      else if (clink)
	bst_canvas_link_toggle_view (clink);
    }
  else if ((event->type == GDK_BUTTON_PRESS || event->type == GDK_BUTTON_RELEASE) &&
	   event->button.button == 1 && router->rtools->tool_id == 1) /* finish link */
    {
      BstCanvasSource *csource;
      BseErrorType error;
      guint ichannel_id;
      
      csource = bst_canvas_source_at (canvas, event->button.x, event->button.y);
      if (!csource)
	{
	  BstCanvasLink *clink;

	  clink = bst_canvas_link_at (canvas, event->button.x, event->button.y);
	  if (clink)
	    csource = bst_canvas_link_has_canvas_source_at (clink, event->button.x, event->button.y);
	}
      ichannel_id = (csource
		     ? bst_canvas_source_ichannel_at (csource, event->button.x, event->button.y)
		     : 0);
      /* don't react to button releases on the point we started from */
      if (!(event->type == GDK_BUTTON_RELEASE && csource == router->ocsource &&
	    bst_canvas_source_ochannel_at (csource, event->button.x, event->button.y) == router->ochannel_id))
	{
	  if (csource)
	    error = bse_source_set_input (csource->source, ichannel_id,
					  router->ocsource->source,
					  router->ochannel_id);
	  else
	    error = BSE_ERROR_SOURCE_NO_SUCH_ICHANNEL;
	  bst_snet_router_reset_tool (router);
	  bst_status_set (error ? 0 : 100, "Create Link", bse_error_blurb (error));
	}
      handled = TRUE;
    }
  else if (event->type == GDK_BUTTON_PRESS &&
	   event->button.button == 3)
    {
      BstCanvasSource *csource;
      BstCanvasLink *clink;

      csource = bst_canvas_source_at (canvas, event->button.x, event->button.y);
      clink = csource ? NULL : bst_canvas_link_at (canvas, event->button.x, event->button.y);
      if (clink && !csource)
	{
	  csource = bst_canvas_link_has_canvas_source_at (clink, event->button.x, event->button.y);
	  if (csource)
	    clink = NULL;
	}

      if (csource)
	{
	  GtkWidget *choice;
	  BseSource *source = csource->source;
	  BseSource *rsource = BSE_SOURCE (router->snet);
	  gchar *source_name = g_strconcat (BSE_OBJECT_TYPE_NAME (source),
					    ": ",
					    BSE_OBJECT_NAME (source),
					    NULL);

	  choice = bst_choice_menu_createv (BST_CHOICE_TITLE (source_name),
					    BST_CHOICE_SEPERATOR,
					    BST_CHOICE_S (2, "Properties", PROPERTIES, source != rsource),
					    BST_CHOICE_S (3, "Delete Inputs", NO_ILINK, source->n_inputs),
					    BST_CHOICE_S (4, "Delete Outputs", NO_OLINK, source->outputs),
					    BST_CHOICE_SEPERATOR,
					    BST_CHOICE_S (1, "Delete", DELETE, source != rsource),
					    BST_CHOICE_END);
	  g_free (source_name);
	  switch (bst_choice_modal (choice, event->button.button, event->button.time))
	    {
	    case 1:
	      bse_snet_remove_source (router->snet, source);
	      break;
	    case 2:
	      bst_canvas_source_popup_view (csource);
	      break;
	    case 3:
	      bse_source_clear_ichannels (source);
	      break;
	    case 4:
	      bse_source_clear_ochannels (source);
	      break;
	    }
	  bst_choice_destroy (choice);
	}
      else if (clink)
	{
	  GtkWidget *choice;

	  choice = bst_choice_menu_createv (BST_CHOICE_TITLE ("Source link"),
					    BST_CHOICE_SEPERATOR,
					    BST_CHOICE (2, "Properties", PROPERTIES),
					    BST_CHOICE_SEPERATOR,
					    BST_CHOICE (1, "Delete", DELETE),
					    BST_CHOICE_END);
	  switch (bst_choice_modal (choice, event->button.button, event->button.time))
	    {
	    case 1:
	      bse_source_clear_ichannel (clink->icsource->source, clink->ichannel_id);
	      break;
	    case 2:
	      bst_canvas_link_popup_view (clink);
	      break;
	    }
	  bst_choice_destroy (choice);
	}
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
      if (event->button.button == 1 && router->rtools->tool_id > 1) /* add new source */
	{
	  handled = TRUE;
	  gnome_canvas_window_to_world (canvas,
					event->button.x, event->button.y,
					&router->world_x, &router->world_y);
	  bse_snet_new_source (router->snet,
			       router->rtools->tool_id,
			       NULL);
	  router->world_x = 0;
	  router->world_y = 0;
	  if (BST_SNET_EDIT_FALLBACK)
	    bst_radio_tools_set_tool (router->rtools, 0);
	}
      else if (router->rtools->tool_id > 1 || (router->rtools->tool_id == 1 && event->button.button != 1))
	{
	  /* disturbing button presses, reset stuff */
	  handled = TRUE;
	  bst_snet_router_reset_tool (router);
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
