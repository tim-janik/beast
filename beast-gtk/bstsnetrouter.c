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
#include "bstdialog.h"


#define EPSILON 1e-6


/* --- prototypes --- */
static void	  bst_snet_router_class_init	(BstSNetRouterClass	*klass);
static void	  bst_snet_router_init		(BstSNetRouter		*router,
						 BstSNetRouterClass     *class);
static void	  bst_snet_router_destroy	(GtkObject		*object);
static void	  bst_snet_router_finalize	(GObject		*object);
static void       bst_snet_router_build_tools	(BstSNetRouter		*router);
static void	  bst_snet_router_item_added    (BstSNetRouter          *router,
						 BseItem                *item,
						 BseContainer           *container);
static void	  bst_snet_router_viewable_changed (GtkWidget		*widget);
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
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  bst_snet_router_class = class;
  
  gobject_class->finalize = bst_snet_router_finalize;
  object_class->destroy = bst_snet_router_destroy;
  
  widget_class->event = bst_snet_router_event;
}

static void
bst_snet_router_init (BstSNetRouter      *router,
		      BstSNetRouterClass *class)
{
  GnomeCanvas *canvas = GNOME_CANVAS (router);
  
  router->toolbar = NULL;
  router->palette = NULL;
  router->adjustment = NULL;
  router->snet = 0;
  router->world_x = 0;
  router->world_y = 0;
  router->drag_is_input = FALSE;
  router->drag_channel = ~0;
  router->drag_csource = NULL;
  router->tmp_line = NULL;
  router->link_list = NULL;

  router->rtools = bst_radio_tools_new ();
  gtk_object_ref (GTK_OBJECT (router->rtools));
  gtk_object_sink (GTK_OBJECT (router->rtools));
  g_object_connect (router->rtools,
		    "swapped_signal_after::set_tool", bst_router_set_tool, router,
		    NULL);
  
  g_object_connect (GTK_OBJECT (canvas->root),
		    "swapped_signal::event", bst_snet_router_root_event, router,
		    NULL);
  g_object_connect (GTK_OBJECT (router),
		    "signal_after::show", bst_snet_router_reset_tool, NULL,
		    "signal::viewable_changed", bst_snet_router_viewable_changed, NULL,
		    NULL);
  
  router->adjustment = (GtkAdjustment*) gtk_adjustment_new (1.0, 0.20, 5.00, 0.05, 0.50, 0.50);
  g_object_connect (GTK_OBJECT (router->adjustment),
		    "swapped_signal::value_changed", bst_snet_router_adjust_zoom, router,
		    "swapped_signal::destroy", bse_nullify_pointer, &router->adjustment,
		    NULL);

  bst_snet_router_build_tools (router);
}

static void
bst_snet_router_destroy_contents (BstSNetRouter *router)
{
  GnomeCanvas *canvas = GNOME_CANVAS (router);
  GSList *slist;

  if (canvas->root)
    {
      GnomeCanvasGroup *group = GNOME_CANVAS_GROUP (canvas->root);

      while (group->item_list)
	gtk_object_destroy (group->item_list->data);
    }
  for (slist = router->link_list; slist; slist = slist->next)
    {
      BstCanvasLink *link = slist->data;

      gtk_object_destroy (slist->data);
      g_object_unref (link);
    }
  g_slist_free (router->link_list);
  router->link_list = NULL;
}

static void
bst_snet_router_destroy (GtkObject *object)
{
  BstSNetRouter *router = BST_SNET_ROUTER (object);
  
  bst_snet_router_reset_tool (router);
  bst_snet_router_destroy_contents (router);
  bst_snet_router_set_snet (router, 0);

  if (router->toolbar)
    {
      gtk_widget_destroy (router->toolbar);
      router->toolbar = NULL;
    }
  if (router->palette)
    gtk_widget_destroy (router->palette);
  
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bst_snet_router_finalize (GObject *object)
{
  BstSNetRouter *router = BST_SNET_ROUTER (object);
  
  gtk_object_unref (GTK_OBJECT (router->rtools));
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bst_snet_router_viewable_changed (GtkWidget *widget)
{
  BstSNetRouter *router = BST_SNET_ROUTER (widget);

  if (router->palette)
    {
      gboolean show_palette = widget->window && gtk_widget_viewable (widget);

      if (show_palette && router->reshow_palette)
	{
	  gtk_widget_show (router->palette);
	  router->reshow_palette = FALSE;
	}
      else if (GTK_WIDGET_VISIBLE (router->palette))
	{
	  router->reshow_palette = TRUE;
	  gtk_widget_hide (router->palette);
	}
    }
}

GtkWidget*
bst_snet_router_new (BswProxy snet)
{
  GtkWidget *router;
  
  g_return_val_if_fail (BSW_IS_SNET (snet), NULL);
  
  router = gtk_widget_new (BST_TYPE_SNET_ROUTER,
			   "aa", BST_SNET_ANTI_ALIASED,
			   NULL);
  bst_snet_router_set_snet (BST_SNET_ROUTER (router), snet);
  
  return router;
}

void
bst_snet_router_set_snet (BstSNetRouter *router,
			  BswProxy       snet)
{
  g_return_if_fail (BST_IS_SNET_ROUTER (router));
  if (snet)
    g_return_if_fail (BSW_IS_SNET (snet));
  
  if (router->snet)
    {
      bst_snet_router_destroy_contents (router);
      
      g_object_disconnect (bse_object_from_id (router->snet),
			   "any_signal", bst_snet_router_item_added, router,
			   NULL);
      bse_object_unref (bse_object_from_id (router->snet));
      router->snet = 0;
    }
  if (snet)
    {
      gfloat zoom;
      
      router->snet = snet;
      bse_object_ref (bse_object_from_id (router->snet));
      
      g_object_connect (bse_object_from_id (router->snet),
			"swapped_signal::item_added", bst_snet_router_item_added, router,
			NULL);
      
      bst_snet_router_rebuild (BST_SNET_ROUTER (router));
      if (bse_parasite_get_floats (bse_object_from_id (router->snet), "BstRouterZoom", 1, &zoom) == 1)
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
palette_reset (BstSNetRouter *router)
{
  if (router->rtools)
    bst_radio_tools_set_tool (router->rtools, 0);
}

void
bst_snet_router_toggle_palette (BstSNetRouter *router)
{
  g_return_if_fail (BST_IS_SNET_ROUTER (router));

  if (!router->palette || !GTK_WIDGET_VISIBLE (router->palette))
    {
      if (!router->palette)
	router->palette = g_object_connect (bst_dialog_new (&router->palette,
							    GTK_OBJECT (router),
							    BST_DIALOG_HIDE_ON_DELETE,
							    "Palette",
							    bst_radio_tools_build_palette (router->rtools, TRUE, GTK_RELIEF_NORMAL)),
					    "swapped_signal::hide", palette_reset, router,
					    NULL);
      gtk_widget_showraise (router->palette);
    }
  else
    gtk_widget_hide (router->palette);
}

static void
bst_snet_router_item_added (BstSNetRouter *router,
			    BseItem       *item,
			    BseContainer  *container)
{
  GnomeCanvas *canvas = GNOME_CANVAS (router);
  GnomeCanvasItem *csource;
  
  if (!BSE_IS_SOURCE (item))
    return;
  
  csource = bst_canvas_source_new (GNOME_CANVAS_GROUP (canvas->root),
				   BSE_OBJECT_ID (item),
				   router->world_x,
				   router->world_y);
  g_object_connect (csource,
		    "swapped_signal::update_links", bst_snet_router_update_links, router,
		    NULL);
  bst_canvas_source_update_links (BST_CANVAS_SOURCE (csource));
  /* queue update cause ellipse-rect is broken */
  gnome_canvas_FIXME_hard_update (canvas);
}

static gboolean
walk_items (BseItem  *item,
	    gpointer  data_p)
{
  gpointer *data = data_p;
  BstSNetRouter *router = BST_SNET_ROUTER (data[0]);
  GnomeCanvas *canvas = GNOME_CANVAS (router);
  
  if (BSE_IS_SOURCE (item))
    {
      GnomeCanvasItem *csource;
      
      csource = bst_canvas_source_new (GNOME_CANVAS_GROUP (canvas->root),
				       BSE_OBJECT_ID (item),
				       router->world_x,
				       router->world_y);
      g_object_connect (csource,
			"swapped_signal::update_links", bst_snet_router_update_links, router,
			NULL);
      data[1] = g_slist_prepend (data[1], csource);
      /* queue update cause ellipse-rect is broken */
      gnome_canvas_FIXME_hard_update (canvas);
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

  /* walk all child sources */
  data[0] = router;
  data[1] = NULL;
  if (0)
    {
      /* add the snet itself */
      csource = bst_canvas_source_new (GNOME_CANVAS_GROUP (canvas->root),
				       router->snet,
				       0, 0);
      g_object_connect (csource,
			"swapped_signal::update_links", bst_snet_router_update_links, router,
			NULL);
      data[1] = g_slist_prepend (NULL, csource);
    }
  bse_container_forall_items (bse_object_from_id (router->snet),
			      walk_items,
			      data);
  
  /* update all links */
  for (slist = data[1]; slist; slist = slist->next)
    bst_canvas_source_update_links (BST_CANVAS_SOURCE (slist->data));
  g_slist_free (data[1]);
  /* queue update cause ellipse-rect is broken */
  gnome_canvas_FIXME_hard_update (canvas);
}

static gboolean
idle_zoom (gpointer data)
{
  GnomeCanvas *canvas;
  gdouble *d;

  GDK_THREADS_ENTER ();

  canvas = GNOME_CANVAS (data);
  d = gtk_object_get_data (GTK_OBJECT (canvas), "zoom_d");
  
  if (EPSILON < fabs (canvas->pixels_per_unit - *d))
    gnome_canvas_set_pixels_per_unit (canvas, *d);
  
  gtk_object_remove_data (GTK_OBJECT (canvas), "zoom_d");

  GDK_THREADS_LEAVE ();
  
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

      bse_parasite_set_floats (bse_object_from_id (router->snet), "BstRouterZoom", 1, &zoom);
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
  bst_radio_tools_add_stock_tool (router->rtools,
				  0,
				  "Edit",
				  "Edit tool (mouse buttons 1-3)",
				  ("Edit tool (mouse buttons 1-3)\n"
				   "Use button1 to create links, "
				   "button2 for movement and "
				   "button3 to change properties"),
				  BST_STOCK_MOUSE_TOOL,
				  BST_RADIO_TOOLS_EVERYWHERE);

  /* add BseSource types from categories */
  cats = bse_categories_match ("/Source/*", &n_cats);
  for (i = 0; i < n_cats; i++)
    {
      static const gchar *toolbar_categories[] = {
	"BsePcmOutput", "BseMixer", "BseSnooper", "BsePcmInput",
      };
      guint n, n_toolbar_categories = sizeof (toolbar_categories) / sizeof (toolbar_categories[0]);
      gboolean add_to_toolbar = FALSE;

      for (n = 0; n < n_toolbar_categories; n++)
	if (strcmp (toolbar_categories[n], g_type_name (cats[i].type)) == 0)
	  {
	    add_to_toolbar = TRUE;
	    break;
	  }
      bst_radio_tools_add_category (router->rtools,
				    cats[i].type,
				    cats + i,
				    BST_RADIO_TOOLS_PALETTE |
				    (add_to_toolbar ? BST_RADIO_TOOLS_TOOLBAR : 0));
      g_printerr ("FIXME: Module `%s' registered under deprecated `/Source/' category\n", g_type_name (cats[i].type));
    }
  g_free (cats);

  /* add BseSource types from categories */
  cats = bse_categories_match ("/Modules/*", &n_cats);
  for (i = 0; i < n_cats; i++)
    {
      static const gchar *toolbar_categories[] = {
	"BsePcmOutput", "BseMixer", "BseSnooper", "BsePcmInput",
      };
      guint n, n_toolbar_categories = sizeof (toolbar_categories) / sizeof (toolbar_categories[0]);
      gboolean add_to_toolbar = FALSE;

      for (n = 0; n < n_toolbar_categories; n++)
	if (strcmp (toolbar_categories[n], g_type_name (cats[i].type)) == 0)
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
  router->toolbar = g_object_connect (gtk_widget_new (GTK_TYPE_TOOLBAR,
						      "visible", TRUE,
						      "orientation", GTK_ORIENTATION_HORIZONTAL,
						      "toolbar_style", GTK_TOOLBAR_BOTH,
						      // "relief", GTK_RELIEF_NONE,
						      // "space_style", GTK_TOOLBAR_SPACE_LINE,
						      NULL),
				      "swapped_signal::destroy", bse_nullify_pointer, &router->toolbar,
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
				       bst_image_from_stock (BST_STOCK_PALETTE, BST_SIZE_PALETTE),
				       NULL, NULL);
  g_object_connect (button,
		    "swapped_signal::clicked", bst_snet_router_toggle_palette, router,
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

  /* sigh, queue a hard update to get pending bounds recalculated */
  gnome_canvas_FIXME_hard_update (canvas);

  /* recompute everything, now. then we can use the new root bounds
   * to adjust the scrolling region
   */
  gnome_canvas_update_now (canvas);

  /* set new scroll region with fudge */
  gnome_canvas_item_get_bounds (canvas->root, &x1, &y1, &x2, &y2);
  x1 -= 1; y1 -= 1; x2 += 1;  y2 += 1;
  gnome_canvas_set_scroll_region (canvas, x1, y1, x2, y2);
  adjustment = gtk_layout_get_hadjustment (layout);
  gtk_adjustment_set_value (adjustment,
			    (adjustment->upper - adjustment->lower) / 2 -
			    adjustment->page_size / 2);
  adjustment = gtk_layout_get_vadjustment (layout);
  gtk_adjustment_set_value (adjustment,
			    (adjustment->upper - adjustment->lower) / 2 -
			    adjustment->page_size / 2);

  /* the canvas forgets to re-translate and update its items */
  gnome_canvas_FIXME_hard_update (canvas);
}

BstCanvasSource*
bst_snet_router_csource_from_source (BstSNetRouter *router,
				     BswProxy	    source)
{
  GnomeCanvas *canvas;
  GnomeCanvasGroup *root;
  GList *list;

  g_return_val_if_fail (BST_IS_SNET_ROUTER (router), NULL);
  g_return_val_if_fail (BSW_IS_SOURCE (source), NULL);

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
  for (i = 0; i < bsw_source_n_ichannels (csource->source); i++)
    {
      guint j, ochannel, n_joints = bsw_source_ichannel_get_n_joints (csource->source, i);

      for (j = 0; j < n_joints; j++)
	{
	  BswProxy osource = bsw_source_ichannel_get_osource (csource->source, i, j);
	  BstCanvasSource *ocsource;
	  BstCanvasLink *link = NULL;

	  if (!osource)
	    continue;

	  ochannel = bsw_source_ichannel_get_ochannel (csource->source, i, j);
	  ocsource = bst_snet_router_csource_from_source (router, osource);
	  if (!ocsource)
	    {
	      g_warning ("Couldn't figure CanvasSource Item From BseSource (%u)", osource);
	      continue;
	    }

	  /* find corresponding link */
	  for (slist = ilist; slist; slist = slist->next)
	    {
	      link = slist->data;
	      if (!link)
		continue;
	      
	      if (link->ichannel == i &&
		  link->ocsource == ocsource &&
		  link->ochannel == ochannel)
		break;
	    }
	  if (slist) /* cool, found one already */
	    slist->data = NULL;
	  else /* got none, ok, need to create new one */
	    {
	      link = g_object_ref (bst_canvas_link_new (GNOME_CANVAS_GROUP (canvas->root)));
	      bst_canvas_link_set_icsource (link, csource, i);
	      bst_canvas_link_set_ocsource (link, ocsource, ochannel);
	      /* queue update cause ellipse-rect is broken */
	      gnome_canvas_FIXME_hard_update (canvas);
	    }
	  router->link_list = g_slist_prepend (router->link_list, link);
	}
    }

  /* gotta nuke outdated links now */
  for (slist = ilist; slist; slist = slist->next)
    {
      BstCanvasLink *link = slist->data;

      if (link)
	{
	  gtk_object_destroy (slist->data);
	  g_object_unref (link);
	}
    }
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
	  /* queue update cause canvas-line is broken and leaves artefacts */
	  gnome_canvas_FIXME_hard_update (GNOME_CANVAS (router));
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
	      g_object_set (GTK_OBJECT (router->tmp_line), "points", gpoints, NULL);
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

  if (event->type == GDK_BUTTON_PRESS || event->type == GDK_BUTTON_RELEASE)
    {
      BstCanvasSource *csource = bst_canvas_source_at (canvas, event->button.x, event->button.y);
      BstCanvasLink *clink = bst_canvas_link_at (canvas, event->button.x, event->button.y);
      guint ochannel = ~0, ichannel = ~0;
      gboolean at_channel;

      if (!csource && clink)
	csource = bst_canvas_link_csource_at (clink, event->button.x, event->button.y);
      if (csource)
	{
	  ochannel = bst_canvas_source_ochannel_at (csource, event->button.x, event->button.y);
	  ichannel = bst_canvas_source_ichannel_at (csource, event->button.x, event->button.y);
	}
      at_channel = ochannel != ~0 || ichannel != ~0;

      if (event->type == GDK_BUTTON_PRESS && event->button.button == 1 &&
	  router->rtools->tool_id == 0) /* start link (or popup property dialog) */
	{
	  g_return_val_if_fail (router->tmp_line == NULL, FALSE);

	  router->drag_is_input = ichannel != ~0;
	  if (csource && at_channel && router->drag_is_input &&  /* ichannel in use */
	      !bst_canvas_source_ichannel_free (csource, ichannel))
	    {
	      bst_status_set (0, "Input channel in use", NULL);
	      gdk_beep ();
	    }
	  else if (csource && at_channel) /* i/o link */
	    {
	      GnomeCanvasPoints *gpoints = gnome_canvas_points_new (2);
	      
	      router->drag_channel = router->drag_is_input ? ichannel : ochannel;
	      router->drag_csource = csource;
	      (router->drag_is_input ?
	       bst_canvas_source_ichannel_pos :
	       bst_canvas_source_ochannel_pos) (csource,
						router->drag_channel,
						&gpoints->coords[0],
						&gpoints->coords[1]);
	      gpoints->coords[2] = gpoints->coords[0] + 50;
	      gpoints->coords[3] = gpoints->coords[1] + 50;
	      router->tmp_line = g_object_connect (gnome_canvas_item_new (GNOME_CANVAS_GROUP (canvas->root),
									  GNOME_TYPE_CANVAS_LINE,
									  "fill_color", "black",
									  "points", gpoints,
									  NULL),
						   "swapped_signal::destroy", bse_nullify_pointer, &router->tmp_line,
						   NULL);
	      gnome_canvas_points_free (gpoints);
	      router->world_x = event->button.x;
	      router->world_y = event->button.y;
	      bst_radio_tools_set_tool (router->rtools, 1);
	      if (router->drag_is_input)
		bst_status_set (0, "Create Link", "Select output module");
	      else
		bst_status_set (0, "Create Link", "Select input module");
	      handled = TRUE;
	    }
	  else if (csource && csource->source != router->snet)
	    {
	      if (event->button.state & GDK_SHIFT_MASK)
		bst_canvas_source_toggle_info (csource);
	      else
		bst_canvas_source_toggle_view (csource);
	    }
	  else if (clink && !csource)
	    bst_canvas_link_toggle_view (clink);
	}
      else if (event->button.button == 1 && router->rtools->tool_id == 1) /* finish link */
	{
	  if (event->type == GDK_BUTTON_RELEASE && csource == router->drag_csource &&
	      router->drag_channel == (router->drag_is_input ? ichannel : ochannel))
	    {
	      /* don't react to button releases on the point we started from */
	    }
	  else
	    {
	      BseErrorType error;
	      
	      if (!csource || (router->drag_is_input ? ochannel : ichannel) == ~0)
		error = router->drag_is_input ? BSE_ERROR_SOURCE_NO_SUCH_OCHANNEL : BSE_ERROR_SOURCE_NO_SUCH_ICHANNEL;
	      else if (router->drag_is_input)
		error = bsw_source_set_input (router->drag_csource->source, router->drag_channel,
					      csource->source, ochannel);
	      else
		error = bsw_source_set_input (csource->source, ichannel,
					      router->drag_csource->source, router->drag_channel);
	      router->drag_csource = NULL;
	      router->drag_channel = ~0;
	      bst_snet_router_reset_tool (router);
	      bst_status_set (error ? 0 : 100, "Create Link", bsw_error_blurb (error));
	      if (error)
		gdk_beep ();
	    }
	  handled = TRUE;
	}
      else if (event->type == GDK_BUTTON_PRESS && event->button.button == 3)
	{
	  if (csource)
	    {
	      GtkWidget *choice;
	      gchar *source_name = g_strconcat (bsw_item_get_type_name (csource->source),
						": ",
						bsw_item_get_name (csource->source),
						NULL);
	      guint i, has_inputs = 0;
	      
	      for (i = 0; has_inputs == 0 && i < bsw_source_n_ichannels (csource->source); i++)
		has_inputs += bsw_source_ichannel_get_n_joints (csource->source, i);
	      choice = bst_choice_menu_createv ("<BEAST-SNetRouter>/ModulePopup",
						BST_CHOICE_TITLE (source_name),
						BST_CHOICE_SEPERATOR,
						BST_CHOICE_S (2, "Properties", PROPERTIES, csource->source != router->snet),
						BST_CHOICE_S (3, "Delete Inputs", NO_ILINK, has_inputs),
						BST_CHOICE_S (4, "Delete Outputs", NO_OLINK, BSE_SOURCE (bse_object_from_id (csource->source))->outputs),
						BST_CHOICE (5, "Show Info", INFO),
						BST_CHOICE_SEPERATOR,
						BST_CHOICE_S (1, "Delete", DELETE, csource->source != router->snet),
						BST_CHOICE_END);
	      g_free (source_name);
	      bst_status_bar_catch_procedure ();
	      switch (bst_choice_modal (choice, event->button.button, event->button.time))
		{
		  BswErrorType error;
		case 1:
		  error = bsw_snet_remove_source (router->snet, csource->source);
		  bst_status_set (error ? 0 : 100, "Remove Module", bsw_error_blurb (error));
		  if (error)
		    gdk_beep ();
		  break;
		case 2:
		  bst_canvas_source_popup_view (csource);
		  break;
		case 3:
		  bsw_source_clear_inputs (csource->source);
		  break;
		case 4:
		  bsw_source_clear_outputs (csource->source);
		  break;
		case 5:
		  bst_canvas_source_popup_info (csource);
		  break;
		}
	      bst_status_bar_uncatch_procedure ();
	      bst_choice_destroy (choice);
	      /* FIXME: get rid of artifacts left behind removal (mostly rect-ellipse) */
	      gtk_widget_queue_draw (GTK_WIDGET (canvas));
	    }
	  else if (clink && !csource)
	    {
	      GtkWidget *choice;
	      
	      choice = bst_choice_menu_createv ("<BEAST-SNetRouter>/LinkPopup",
						BST_CHOICE_TITLE ("Source link"),
						BST_CHOICE_SEPERATOR,
						BST_CHOICE (2, "Properties", PROPERTIES),
						BST_CHOICE_SEPERATOR,
						BST_CHOICE (1, "Delete", DELETE),
						BST_CHOICE_END);
              bst_status_bar_catch_procedure ();
	      switch (bst_choice_modal (choice, event->button.button, event->button.time))
		{
		  BseErrorType error;
		case 1:
		  error = bsw_source_unset_input (clink->icsource->source, clink->ichannel,
						  clink->ocsource->source, clink->ochannel);
		  bst_status_set (error ? 0 : 100, "Delete Link", bsw_error_blurb (error));
		  if (error)
		    gdk_beep ();
		  break;
		case 2:
		  bst_canvas_link_popup_view (clink);
		  break;
		}
	      bst_status_bar_uncatch_procedure ();
	      bst_choice_destroy (choice);
	      /* FIXME: get rid of artifacts left behind removal (mostly rect-ellipse) */
	      gtk_widget_queue_draw (GTK_WIDGET (canvas));
	    }
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
	  BswErrorType error;
	  const gchar *type = g_type_name (router->rtools->tool_id);

	  handled = TRUE;
	  gnome_canvas_window_to_world (canvas,
					event->button.x, event->button.y,
					&router->world_x, &router->world_y);

	  error = bsw_snet_can_create_source (router->snet, type);
	  if (!error)
	    bsw_snet_create_source (router->snet, type);
	  bst_status_set (error ? 0 : 100, "Insert Module", bsw_error_blurb (error));
	  if (error)
	    gdk_beep ();

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

BstSNetRouter*
bst_snet_router_build_page (BswProxy snet)
{
  static gchar *zoom_xpm[] = {
    "12 12 2 1", "  c None", "# c #000000",
    "            ",
    " ####  #### ",
    " ##      ## ",
    " # #    # # ",
    " #  ####  # ",
    "    #  #    ",
    "    #  #    ",
    " #  ####  # ",
    " # #    # # ",
    " ##      ## ",
    " ####  #### ",
    "            ",
  };
  GtkWidget *router, *zoomed_window, *router_box, *pix;
  GdkPixmap *pixmap;
  GdkBitmap *mask;

  g_return_val_if_fail (BSW_IS_SNET (snet), NULL);

  router = g_object_new (BST_TYPE_SNET_ROUTER,
			 "aa", BST_SNET_ANTI_ALIASED,
			 NULL);
  bst_snet_router_set_snet (BST_SNET_ROUTER (router), snet);

  router_box = g_object_new (GTK_TYPE_VBOX,
			     "visible", TRUE,
			     "homogeneous", FALSE,
			     "spacing", 3,
			     "border_width", 5,
			     NULL);
  gtk_box_pack_start (GTK_BOX (router_box), BST_SNET_ROUTER (router)->toolbar, FALSE, TRUE, 0);
  zoomed_window = g_object_new (BST_TYPE_ZOOMED_WINDOW,
				"visible", TRUE,
				"hscrollbar_policy", GTK_POLICY_ALWAYS,
				"vscrollbar_policy", GTK_POLICY_ALWAYS,
				"parent", router_box,
				NULL);
  g_object_connect (zoomed_window,
		    "swapped_signal::zoom", bst_snet_router_adjust_region, router,
		    "swapped_signal::zoom", gtk_false, NULL,
		    NULL);

  pixmap = gdk_pixmap_colormap_create_from_xpm_d (NULL, gtk_widget_get_colormap (zoomed_window),
						  &mask, NULL, zoom_xpm);
  pix = gtk_pixmap_new (pixmap, mask);
  gdk_pixmap_unref (pixmap);
  gdk_pixmap_unref (mask);
  gtk_widget_set (pix,
		  "visible", TRUE,
		  "parent", BST_ZOOMED_WINDOW (zoomed_window)->toggle_button,
		  NULL);
  
  g_object_set (router,
		"visible", TRUE,
		"parent", zoomed_window,
		NULL);

  return BST_SNET_ROUTER (router);
}
