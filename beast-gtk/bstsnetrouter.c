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
						 gboolean                zoom_in,
						 GtkScrolledWindow      *zoomed_window);
static void	  bst_snet_router_item_added    (BstSNetRouter          *router,
						 BseItem                *item,
						 BseContainer           *container);
static gboolean	  bst_snet_router_canvas_event  (BstSNetRouter          *router,
						 GdkEvent               *event);
static gboolean	  bst_snet_router_root_event    (BstSNetRouter          *router,
						 GdkEvent               *event);


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
  router->radio_action = 0;
  router->canvas = NULL;
  router->root = NULL;
  router->world_x = 0;
  router->world_y = 0;
  
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

  bst_snet_router_set_snet (router, NULL);

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
      
      bse_object_remove_notifiers_by_func (BSE_CONTAINER (router->snet),
					   bst_snet_router_item_added,
					   router);

      router->snet = NULL;
      bse_object_unref (object);
    }
  if (snet)
    {
      BseObject *object = BSE_OBJECT (snet);
      
      bse_object_ref (object);
      router->snet = snet;

      bse_object_add_data_notifier (BSE_CONTAINER (snet),
				    "item_added",
				    bst_snet_router_item_added,
				    router);
      
      bst_snet_router_rebuild (BST_SNET_ROUTER (router));
    }
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
					     (gchar**) zoom_xpm);
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
  GtkWidget *toolbar, *zoomed_window;
  GnomeCanvasPoints *points;
  GnomeCanvasItem *link;
  GnomeCanvasItem *item;
  
  g_return_if_fail (BST_IS_SNET_ROUTER (router));
  
  bst_snet_router_destroy_contents (router);

  router->canvas = (GnomeCanvas*) gnome_canvas_new_aa ();
  router->root = gnome_canvas_root (router->canvas);
  bst_object_set (GTK_OBJECT (router->root),
		  "signal::destroy", gtk_widget_destroyed, &router->root,
		  "object_signal::event", bst_snet_router_root_event, router,
		  NULL);

  toolbar = bst_snet_router_build_toolbar (router);
  gtk_box_pack_start (GTK_BOX (router), toolbar, FALSE, TRUE, 0);
  
  zoomed_window = gtk_widget_new (BST_TYPE_ZOOMED_WINDOW,
				  "visible", TRUE,
				  "hscrollbar_policy", GTK_POLICY_ALWAYS,
				  "vscrollbar_policy", GTK_POLICY_ALWAYS,
				  "parent", router,
				  "object_signal::zoom", bst_snet_router_adjust_region, router,
				  "signal::realize", zoomed_add_xpm, NULL,
				  NULL);
  gtk_widget_set (GTK_WIDGET (router->canvas),
		  "visible", TRUE,
		  "signal::destroy", gtk_widget_destroyed, &router->canvas,
		  "object_signal::event", bst_snet_router_canvas_event, router,
		  "parent", zoomed_window,
		  NULL);
  
  points = gnome_canvas_points_new (2);
  points->coords[0] = 0;
  points->coords[1] = 0;
  points->coords[2] = 50;
  points->coords[3] = 50;
  gnome_canvas_points_free (points);
  
  bst_snet_router_update (router);

#if 0 /* FIXME */
  link = bst_canvas_link_new (router->root);
  bst_canvas_link_set_start (BST_CANVAS_LINK (link),
			     bst_canvas_source_new (router->root, bse_object_new (BSE_TYPE_SOURCE,
										  NULL),0,0));
  item = bst_canvas_source_new (router->root, bse_object_new (BSE_TYPE_SOURCE, NULL),0,0);
  bst_canvas_link_set_end (BST_CANVAS_LINK (link), item);
  link = bst_canvas_link_new (router->root);
  bst_canvas_link_set_start (BST_CANVAS_LINK (link), item);
  bst_canvas_link_set_end (BST_CANVAS_LINK (link),
			   bst_canvas_source_new (router->root, bse_object_new (BSE_TYPE_SOURCE,
										NULL),0,0));

#endif
  bst_snet_router_adjust_region (router, TRUE, GTK_SCROLLED_WINDOW (zoomed_window));
}

static void
bst_snet_router_item_added (BstSNetRouter *router,
			    BseItem       *item,
			    BseContainer  *container)
{
  if (!BSE_IS_SOURCE (item))
    {
      g_warning ("Can't handle non-source snet items");
      return;
    }

  bst_canvas_source_new (router->root,
			 BSE_SOURCE (item),
			 router->world_x,
			 router->world_y);
}

static gboolean
bst_snet_router_walk_itmes (BseItem  *item,
			    gpointer  data)
{
  BstSNetRouter *router = BST_SNET_ROUTER (data);

  if (!BSE_IS_SOURCE (item))
    g_warning ("Can't handle non-source snet items");
  else
    bst_canvas_source_new (router->root,
			   BSE_SOURCE (item),
			   router->world_x,
			   router->world_y);
  return TRUE;
}

void
bst_snet_router_update (BstSNetRouter *router)
{
  GnomeCanvasItem *item;
  
  g_return_if_fail (BST_IS_SNET_ROUTER (router));
  g_return_if_fail (GNOME_IS_CANVAS_GROUP (router->root));

  while (router->root->item_list)
    gtk_object_destroy (router->root->item_list->data);

  /* add the snet itself */
  item = bst_canvas_source_new (router->root, BSE_SOURCE (router->snet), 0, 0);

  /* add all child sources */
  bse_container_forall_items (BSE_CONTAINER (router->snet),
			      bst_snet_router_walk_itmes,
			      router);
}

static void
toolbar_radio_toggle (GtkWidget *radio,
		      guint     *radio_active)
{
  if (GTK_TOGGLE_BUTTON (radio)->active)
    *radio_active = GPOINTER_TO_UINT (gtk_object_get_user_data (GTK_OBJECT (radio)));
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
toolbar_set_zoom (GtkAdjustment *adjustment,
		  BstSNetRouter *router)
{
  GtkObject *object = GTK_OBJECT (router->canvas);
  gdouble *d = gtk_object_get_data (object, "zoom_d");

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
  *d = adjustment->value;
}

static GtkWidget*
toolbar_add_radio (GtkWidget	 *parent,
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
		  NULL);
  toolbar_radio_toggle (button, radio_active);

  return button;
}

static GtkWidget*
toolbar_add_category (GtkWidget	  *parent,
		      GtkWidget   *last_radio,
		      BseCategory *category,
		      GtkTooltips *tooltips,
		      guint       *radio_active)
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
  radio = toolbar_add_radio (parent, last_radio,
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
  GtkAdjustment *adjustment;
  guint i;
  BseCategory *cats;
  guint n_cats;
  
  g_return_val_if_fail (BST_IS_SNET_ROUTER (router), NULL);

  bar = gtk_widget_new (GTK_TYPE_TOOLBAR,
			"visible", TRUE,
			"orientation", GTK_ORIENTATION_HORIZONTAL,
			"toolbar_style", GTK_TOOLBAR_BOTH,
			NULL);

  /* add link/move/property edit tool
   */
  radio = toolbar_add_radio (bar,
			     radio,
			     0,
			     "Edit",
			     "Edit\n"
			     "Use button1 to create links, "
			     "button2 for movement and "
			     "button3 to change properties",
			     bst_icon_from_stock (BST_ICON_MOUSE_TOOL),
			     BST_SNET_ROUTER_GET_CLASS (router)->tooltips,
			     &router->radio_action);

  /* add BseSource types from categories
   */
  cats = bse_categories_match ("/Source/*", &n_cats);
  for (i = 0; i < n_cats; i++)
    radio = toolbar_add_category (bar,
				  radio,
				  cats + i,
				  BST_SNET_ROUTER_GET_CLASS (router)->tooltips,
				  &router->radio_action);
  g_free (cats);

  /* FIXME: test hackery
   */
  if (1)
    {
#include "../bse/icons/song.c"
      BseIcon icon = { gimp_image.bytes_per_pixel, ~0,
		       gimp_image.width, gimp_image.height,
		       (void*) gimp_image.pixel_data };
      radio = toolbar_add_radio (bar,
				 radio,
				 0,
				 "Song",
				 NULL,
				 &icon,
				 BST_SNET_ROUTER_GET_CLASS (router)->tooltips,
				 &router->radio_action);
    }

  /* add Zoom: spinner */
  adjustment = (GtkAdjustment*) gtk_adjustment_new (1.00, 0.20, 5.00, 0.05, 0.50, 0.50);
  gtk_signal_connect (GTK_OBJECT (adjustment),
		      "value_changed",
		      toolbar_set_zoom,
		      router);
  radio = gtk_spin_button_new (adjustment, 0.0, 2);
  gtk_widget_set_usize (radio, 50, 0);
  gtk_widget_show (radio);
  gtk_toolbar_append_widget (GTK_TOOLBAR (bar), radio, "Zoom Factor", NULL);

  return bar;
}

static gboolean
bst_snet_router_adjust_region (BstSNetRouter     *router,
			       gboolean           zoom_in,
			       GtkScrolledWindow *zoomed_window)
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
  
  if (zoomed_window)
    {
      GtkAdjustment *adjustment;
      
      adjustment = gtk_scrolled_window_get_hadjustment (zoomed_window);
      gtk_adjustment_set_value (adjustment,
				(adjustment->upper - adjustment->lower) / 2 -
				adjustment->page_size / 2);
      adjustment = gtk_scrolled_window_get_vadjustment (zoomed_window);
      gtk_adjustment_set_value (adjustment,
				(adjustment->upper - adjustment->lower) / 2 -
				adjustment->page_size / 2);
    }

  return FALSE;
}

static gboolean
bst_snet_router_root_event (BstSNetRouter   *router,
			    GdkEvent        *event)
{
  gboolean handled = FALSE;

  return handled;
}

static gboolean
bst_snet_router_canvas_event (BstSNetRouter   *router,
			      GdkEvent        *event)
{
  gboolean handled = FALSE;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      if (event->button.button == 1)
	{
	  if (bse_type_is_a (router->radio_action, BSE_TYPE_SOURCE))
	    {
	      gnome_canvas_window_to_world (router->canvas,
					    event->button.x, event->button.y,
					    &router->world_x, &router->world_y);
	      bse_snet_new_source (router->snet,
				   router->radio_action,
				   NULL);
	      router->world_x = 0;
	      router->world_y = 0;
	      handled = TRUE;
	    }
	}
      break;
    default:
      break;
    }

  return handled;
}
