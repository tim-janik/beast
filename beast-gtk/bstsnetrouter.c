/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2003 Tim Janik
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
#include <string.h>
#include "bstcanvaslink.h"
#include "bstmenus.h"
#include "bstgconfig.h"
#include "bstprocedure.h"
#include <gdk/gdkkeysyms.h>


#define EPSILON 1e-6
#define	ROUTER_TOOL_CREATE_LINK		(G_MAXINT)	/* don't clash with category IDs */


/* --- prototypes --- */
static void	  bst_snet_router_class_init	(BstSNetRouterClass	*klass);
static void	  bst_snet_router_init		(BstSNetRouter		*router,
						 BstSNetRouterClass     *class);
static void	  bst_snet_router_destroy	(GtkObject		*object);
static void	  bst_snet_router_finalize	(GObject		*object);
static void       bst_snet_router_build_tools	(BstSNetRouter		*router);
static void	  bst_snet_router_item_added    (BstSNetRouter          *router,
						 SfiProxy                item,
						 SfiProxy                container);
static void	  bst_snet_router_viewable_changed (GtkWidget		*widget);
static gboolean	  bst_snet_router_event		(GtkWidget		*widget,
						 GdkEvent               *event);
static gboolean	  bst_snet_router_button_press	(GtkWidget		*widget,
						 GdkEventButton         *event);
static gboolean	  bst_snet_router_root_event    (BstSNetRouter          *router,
						 GdkEvent               *event);
static void	  bst_snet_router_reset_tool	(BstSNetRouter		*router);
static void	  bst_snet_router_update_links	(BstSNetRouter		*router,
						 BstCanvasSource        *csource);
static void	  bst_snet_router_adjust_zoom	(BstSNetRouter		*router);
static void	  bst_router_tool_set		(BstSNetRouter		*router);
static void	  bst_router_popup_select       (GtkWidget		*widget,
						 gulong                  category_id,
						 gpointer                popup_data);
static void	  bst_router_run_method         (GtkWidget		*widget,
						 gulong                  category_id,
						 gpointer                popup_data);


/* --- menus --- */
static BstMenuConfigEntry popup_entries[] =
{
  { "/Scripts",		NULL,		NULL,	0,	"<Title>",	0 },
  { "/_Utils",		NULL,		NULL,	0,	"<Branch>",	0 },
  { "/-----",		NULL,		NULL,	0,	"<Separator>",	0 },
  { "/Modules",		NULL,		NULL,	0,	"<Title>",	0 },
  { "/Audio _Sources",	NULL,		NULL,	0,	"<Branch>",	0 },
  { "/_Other Sources",	NULL,		NULL,	0,	"<Branch>",	0 },
  { "/_Routing",	NULL,		NULL,	0,	"<Branch>",	0 },
  { "/_Spatial",	NULL,		NULL,	0,	"<Branch>",	0 },
  { "/_Filters",	NULL,		NULL,	0,	"<Branch>",	0 },
  { "/_Enhance",	NULL,		NULL,	0,	"<Branch>",	0 },
  { "/_Distortion",	NULL,		NULL,	0,	"<Branch>",	0 },
  { "/_Virtualization",	NULL,		NULL,	0,	"<Branch>",	0 },
  { "/_Input & Output",	NULL,		NULL,	0,	"<Branch>",	0 },
};
static const gchar *palette_cats[] = {
  "/Modules/Audio Sources/",    "/Modules/Other Sources/",
  "/Modules/Routing/",          "/Modules/Spatial/",
  "/Modules/Filters/",
  "/Modules/Enhance/",          "/Modules/Distortion/",
  "/Modules/Virtualization/",
  "/Modules/Input & Output/",
};


/* --- static variables --- */
static gpointer            parent_class = NULL;
static BstSNetRouterClass *bst_snet_router_class = NULL;


/* --- functions --- */
GType
bst_snet_router_get_type (void)
{
  static GType type = 0;
  if (!type)
    {
      static const GTypeInfo type_info = {
        sizeof (BstSNetRouterClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) bst_snet_router_class_init,
        NULL,   /* class_finalize */
        NULL,   /* class_data */
        sizeof (BstSNetRouter),
        0,      /* n_preallocs */
        (GInstanceInitFunc) bst_snet_router_init,
      };
      type = g_type_register_static (GNOME_TYPE_CANVAS, "BstSNetRouter", &type_info, 0);
    }
  return type;
}

static void
bst_snet_router_class_init (BstSNetRouterClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  BseCategorySeq *cseq;
  BstMenuConfig *m1, *m2, *m3;

  parent_class = g_type_class_peek_parent (class);
  bst_snet_router_class = class;
  
  gobject_class->finalize = bst_snet_router_finalize;
  object_class->destroy = bst_snet_router_destroy;
  
  widget_class->event = bst_snet_router_event;
  widget_class->button_press_event = bst_snet_router_button_press;

  class->popup_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<BstSnetRouter>", NULL);
  gtk_accel_group_lock (class->popup_factory->accel_group);

  /* standard entries */
  m1 = bst_menu_config_from_entries (G_N_ELEMENTS (popup_entries), popup_entries);
  /* module entries */
  cseq = bse_categories_match_typed ("/Modules/*", "BseSource");
  m2 = bst_menu_config_from_cats (cseq, bst_router_popup_select, 1);
  bst_menu_config_sort (m2);
  /* methods */
  cseq = bse_categories_match_method ("/Scripts/Utils/*", "BseSNet");
  m3 = bst_menu_config_from_cats (cseq, bst_router_run_method, 1);
  bst_menu_config_sort (m3);
  /* merge them */
  m1 = bst_menu_config_merge (m1, m2);
  m1 = bst_menu_config_merge (m1, m3);
  /* create menu items */
  bst_menu_config_create_items (m1, class->popup_factory, NULL);
  /* cleanup */
  bst_menu_config_free (m1);
}

static void
bst_snet_router_init (BstSNetRouter      *self,
		      BstSNetRouterClass *class)
{
  GnomeCanvas *canvas = GNOME_CANVAS (self);
  
  self->toolbar = NULL;
  self->palette = NULL;
  self->adjustment = NULL;
  self->snet = 0;
  self->world_x = 0;
  self->world_y = 0;
  self->channel_hints = TRUE;
  self->drag_is_input = FALSE;
  self->drag_channel = ~0;
  self->drag_csource = NULL;
  self->tmp_line = NULL;
  self->link_list = NULL;

  self->rtools = bst_radio_tools_new ();
  g_object_connect (self->rtools,
		    "swapped_signal_after::set_tool", bst_router_tool_set, self,
		    NULL);
  
  g_object_connect (GTK_OBJECT (canvas->root),
		    "swapped_signal::event", bst_snet_router_root_event, self,
		    NULL);
  g_object_connect (GTK_OBJECT (self),
		    "signal_after::show", bst_snet_router_reset_tool, NULL,
		    "signal::viewable_changed", bst_snet_router_viewable_changed, NULL,
		    NULL);
  
  self->adjustment = (GtkAdjustment*) gtk_adjustment_new (1.0, 0.20, 5.00, 0.05, 0.50, 0.50);
  g_object_connect (GTK_OBJECT (self->adjustment),
		    "swapped_signal::value_changed", bst_snet_router_adjust_zoom, self,
		    "swapped_signal::destroy", g_nullify_pointer, &self->adjustment,
		    NULL);

  bst_snet_router_build_tools (self);
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
  BstSNetRouter *self = BST_SNET_ROUTER (object);
  
  bst_snet_router_reset_tool (self);
  bst_snet_router_destroy_contents (self);
  bst_snet_router_set_snet (self, 0);

  if (self->toolbar)
    gtk_widget_destroy (GTK_WIDGET (self->toolbar));
  if (self->palette)
    gtk_widget_destroy (self->palette);
  
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bst_snet_router_finalize (GObject *object)
{
  BstSNetRouter *router = BST_SNET_ROUTER (object);
  
  g_object_unref (router->rtools);
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bst_snet_router_viewable_changed (GtkWidget *widget)
{
  BstSNetRouter *router = BST_SNET_ROUTER (widget);

  if (router->palette)
    {
      gboolean show_palette = widget->window && gxk_widget_viewable (widget);

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
bst_snet_router_new (SfiProxy snet)
{
  GtkWidget *router;
  
  g_return_val_if_fail (BSE_IS_SNET (snet), NULL);
  
  router = gtk_widget_new (BST_TYPE_SNET_ROUTER,
			   "aa", BST_SNET_ANTI_ALIASED,
			   NULL);
  bst_snet_router_set_snet (BST_SNET_ROUTER (router), snet);
  
  return router;
}

void
bst_snet_router_set_snet (BstSNetRouter *router,
			  SfiProxy       snet)
{
  g_return_if_fail (BST_IS_SNET_ROUTER (router));
  if (snet)
    g_return_if_fail (BSE_IS_SNET (snet));
  
  if (router->snet)
    {
      bst_snet_router_destroy_contents (router);
      bse_proxy_disconnect (router->snet,
			    "any_signal", bst_snet_router_item_added, router,
			    NULL);
      bse_item_unuse (router->snet);
      router->snet = 0;
    }
  router->snet = snet;
  if (router->snet)
    {
      bse_item_use (router->snet);	// FIXME: should we hold a use-count on the snet?
      bse_proxy_connect (router->snet,
			 "swapped_signal::item_added", bst_snet_router_item_added, router,
			 NULL);
      
      bst_snet_router_rebuild (BST_SNET_ROUTER (router));
#if 0
      gfloat zoom;
      if (bse_parasite_get_floats (router->snet, "BstRouterZoom", 1, &zoom) == 1)
	gtk_adjustment_set_value (router->adjustment, zoom);
#endif
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
bst_router_popup_select (GtkWidget *widget,
			 gulong     category_id,
			 gpointer   popup_data)
{
  BstSNetRouter *self = BST_SNET_ROUTER (widget);

  if (self->rtools)
    bst_radio_tools_set_tool (self->rtools, category_id);
}

static void
bst_router_run_method (GtkWidget *widget,
                       gulong     category_id,
                       gpointer   popup_data)
{
  BstSNetRouter *self = BST_SNET_ROUTER (widget);
  BseCategory *cat = bse_category_from_id (category_id);

  bst_procedure_exec_auto (cat->type,
                           "snet", SFI_TYPE_PROXY, self->snet,
                           NULL);
}

static void
palette_reset (BstSNetRouter *router)
{
  if (router->rtools)
    bst_radio_tools_set_tool (router->rtools, 0);
}

static gboolean
palette_ebox_button (BstSNetRouter  *self,
		     GdkEventButton *event)
{
  GtkItemFactory *popup_factory = BST_SNET_ROUTER_GET_CLASS (self)->popup_factory;

  bst_menu_popup (popup_factory,
		  GTK_WIDGET (self),
		  NULL, NULL,
		  event->x_root, event->y_root,
		  event->button, event->time);
  return TRUE;
}

void
bst_snet_router_toggle_palette (BstSNetRouter *self)
{
  g_return_if_fail (BST_IS_SNET_ROUTER (self));
  
  if (!self->palette || !GTK_WIDGET_VISIBLE (self->palette))
    {
      if (!self->palette)
	{
	  GtkWidget *ebox = g_object_new (GTK_TYPE_EVENT_BOX,
					  "visible", TRUE,
					  "child", g_object_new (GTK_TYPE_FRAME,
								 "visible", TRUE,
								 "shadow_type", GTK_SHADOW_OUT,
								 "child", g_object_new (GTK_TYPE_ARROW,
											"visible", TRUE,
											"arrow_type", GTK_ARROW_RIGHT,
											"shadow_type", GTK_SHADOW_OUT,
											NULL),
								 NULL),
					  NULL);
	  g_object_connect (ebox, "swapped_signal::button_press_event", palette_ebox_button, self, NULL);
	  self->palette = g_object_connect (gxk_dialog_new (&self->palette,
							    GTK_OBJECT (self),
							    GXK_DIALOG_HIDE_ON_DELETE,
							    "Palette",
							    bst_radio_tools_build_palette (self->rtools, ebox,
											   TRUE, GTK_RELIEF_NORMAL)),
					    "swapped_signal::hide", palette_reset, self,
					    NULL);
	}
      gxk_widget_showraise (self->palette);
    }
  else
    gtk_widget_hide (self->palette);
}

static void
bst_snet_router_item_added (BstSNetRouter *self,
			    SfiProxy       item,
			    SfiProxy       container)
{
  GnomeCanvas *canvas = GNOME_CANVAS (self);
  GnomeCanvasItem *csource;

  if (!BSE_IS_SOURCE (item))
    return;
  
  csource = bst_canvas_source_new (GNOME_CANVAS_GROUP (canvas->root), item);
  bst_canvas_source_set_channel_hints (BST_CANVAS_SOURCE (csource), self->channel_hints);
  g_object_connect (csource,
		    "swapped_signal::update_links", bst_snet_router_update_links, self,
		    NULL);
  bst_canvas_source_update_links (BST_CANVAS_SOURCE (csource));
  /* queue update cause ellipse-rect is broken */
  gnome_canvas_FIXME_hard_update (canvas);
}

void
bst_snet_router_update (BstSNetRouter *self)
{
  GnomeCanvasItem *csource;
  GnomeCanvas *canvas;
  BseProxySeq *pseq;
  GSList *slist, *csources = NULL;
  guint i;
  
  g_return_if_fail (BST_IS_SNET_ROUTER (self));
  
  canvas = GNOME_CANVAS (self);

  /* destroy all canvas sources */
  bst_snet_router_destroy_contents (self);
  
  if (0)
    {
      /* add canvas source for the snet itself */
      csource = bst_canvas_source_new (GNOME_CANVAS_GROUP (canvas->root), self->snet);
      bst_canvas_source_set_channel_hints (BST_CANVAS_SOURCE (csource), self->channel_hints);
      g_object_connect (csource,
			"swapped_signal::update_links", bst_snet_router_update_links, self,
			NULL);
      csources = g_slist_prepend (csources, csource);
    }
  
  /* walk all child sources */
  pseq = bse_container_list_items (self->snet);
  for (i = 0; i < pseq->n_proxies; i++)
    {
      SfiProxy item = pseq->proxies[i];
      
      if (BSE_IS_SOURCE (item))
	{
	  GnomeCanvasItem *csource = bst_canvas_source_new (GNOME_CANVAS_GROUP (canvas->root), item);
	  bst_canvas_source_set_channel_hints (BST_CANVAS_SOURCE (csource), self->channel_hints);
	  g_object_connect (csource,
			    "swapped_signal::update_links", bst_snet_router_update_links, self,
			    NULL);
	  csources = g_slist_prepend (csources, csource);
	}
    }
  
  /* update all links */
  for (slist = csources; slist; slist = slist->next)
    bst_canvas_source_update_links (BST_CANVAS_SOURCE (slist->data));
  g_slist_free (csources);
  
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
    gnome_canvas_set_zoom (canvas, *d);
  
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
#if 0
      gfloat zoom = router->adjustment->value;
      bse_parasite_set_floats (router->snet, "BstRouterZoom", 1, &zoom);
#endif
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
bst_snet_router_build_tools (BstSNetRouter *self)
{
  GtkWidget *button;
  BseCategorySeq *cseq;
  guint i;
  
  g_return_if_fail (BST_IS_SNET_ROUTER (self));
  
  if (self->toolbar)
    gtk_widget_destroy (GTK_WIDGET (self->toolbar));
  if (self->palette)
    gtk_widget_destroy (self->palette);
  
  /* setup radio tools
   */
  bst_radio_tools_clear_tools (self->rtools);
  /* add link/move/property edit tool */
  bst_radio_tools_add_stock_tool (self->rtools,
				  0,
				  "Edit",
				  "Edit/Move/Menu (mouse buttons 1-3)",
				  ("Edit tool (mouse buttons 1-3)\n"
				   "Use button1 to create links, "
				   "button2 for movement and "
				   "button3 to change properties"),
				  BST_STOCK_MOUSE_TOOL,
				  BST_RADIO_TOOLS_EVERYWHERE);
  
  /* add BseSource types from categories */
  cseq = bse_categories_match ("/Modules/*");
  for (i = 0; i < cseq->n_cats; i++)
    {
      static struct { gchar *type, *name, *tip; } toolbar_types[] = {
	{ "BsePcmOutput", "Output", "PCM Output" },
	{ "BseAmplifier", "DCA", "Amplifier" },
	{ "BseSnooper", "Snoop", "Signal Debugging Tool" },
	{ "BsePcmInput", "Input", "PCM Input" },
      };
      guint n, is_palette = 0;

      /* decide whether this tool is displayed in the palette */
      for (n = 0; n < G_N_ELEMENTS (palette_cats); n++)
	if (strncmp (palette_cats[n], cseq->cats[i]->category, strlen (palette_cats[n])) == 0)
	  {
	    is_palette = TRUE;
	    break;
	  }
      /* add (all) tools */
      bst_radio_tools_add_category (self->rtools,
				    cseq->cats[i]->category_id,
				    cseq->cats[i],
				    is_palette ? BST_RADIO_TOOLS_PALETTE : BST_RADIO_TOOLS_NONE);
      /* add toolbar variants */
      for (n = 0; n < G_N_ELEMENTS (toolbar_types); n++)
	if (strcmp (toolbar_types[n].type, cseq->cats[i]->type) == 0)
	  {
	    bst_radio_tools_add_tool (self->rtools, cseq->cats[i]->category_id,
				      toolbar_types[n].name,
				      toolbar_types[n].tip, NULL,
				      cseq->cats[i]->icon, BST_RADIO_TOOLS_TOOLBAR);
	    break;
	  }
    }
  
  /* create toolbar
   */
  self->toolbar = gxk_toolbar_new (&self->toolbar);

  /* add radios to toolbar
   */
  bst_radio_tools_build_toolbar (self->rtools, self->toolbar);
  
  /* add `Palette' button to toolbar
   */
  gxk_toolbar_append_space (self->toolbar);
  button = gxk_toolbar_append (self->toolbar, GXK_TOOLBAR_BUTTON,
			       "Palette", "Toggle visibility of the tool palette",
			       gxk_stock_image (BST_STOCK_PALETTE, BST_SIZE_TOOLBAR));
  g_object_connect (button,
		    "swapped_signal::clicked", bst_snet_router_toggle_palette, self,
		    NULL);
  
  /* add `Zoom' spinner
   */
  button = gtk_spin_button_new (self->adjustment, 0.0, 2);
  gtk_widget_set_usize (button, 50, 0);
  gtk_widget_show (button);
  gxk_toolbar_append (self->toolbar, GXK_TOOLBAR_EXTRA_WIDGET,
		      "Zoom", "Adjust the zoom factor of the router display",
		      button);

  /* add channel name toggle
   */
  button = gtk_toggle_button_new_with_mnemonic ("_Channel");
  g_object_set (button,
		"visible", TRUE,
		"active", self->channel_hints,
		"can_focus", FALSE,
		NULL);
  gxk_toolbar_append (self->toolbar, GXK_TOOLBAR_EXTRA_WIDGET,
		      "Hints", "Toggle channel name hints.",
		      button);
  g_object_connect (button,
		    "swapped_signal::clicked", bst_snet_router_toggle_channel_hints, self,
		    NULL);

  /* set default tool
   */
  bst_radio_tools_set_tool (self->rtools, 0);
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
				     SfiProxy	    source)
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

void
bst_snet_router_toggle_channel_hints (BstSNetRouter *self)
{
  GnomeCanvas *canvas;
  GnomeCanvasGroup *root;
  GList *list;

  g_return_if_fail (BST_IS_SNET_ROUTER (self));

  self->channel_hints = !self->channel_hints;
  canvas = GNOME_CANVAS (self);
  root = GNOME_CANVAS_GROUP (canvas->root);
  for (list = root->item_list; list; list = list->next)
    {
      BstCanvasSource *csource = list->data;
      if (BST_IS_CANVAS_SOURCE (csource))
	bst_canvas_source_set_channel_hints (csource, self->channel_hints);
    }
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
  for (i = 0; i < bse_source_n_ichannels (csource->source); i++)
    {
      guint j, ochannel, n_joints = bse_source_ichannel_get_n_joints (csource->source, i);

      for (j = 0; j < n_joints; j++)
	{
	  SfiProxy osource = bse_source_ichannel_get_osource (csource->source, i, j);
	  BstCanvasSource *ocsource;
	  BstCanvasLink *link = NULL;

	  if (!osource)
	    continue;

	  ochannel = bse_source_ichannel_get_ochannel (csource->source, i, j);
	  ocsource = bst_snet_router_csource_from_source (router, osource);
	  if (!ocsource)
	    {
	      g_warning ("Couldn't figure CanvasSource Item from BSE module (%lu)", osource);
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
      if (router->rtools->tool_id != ROUTER_TOOL_CREATE_LINK)
	{
	  gtk_object_destroy (GTK_OBJECT (router->tmp_line));
	  gxk_status_clear ();
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
bst_router_tool_set (BstSNetRouter *router)
{
  GnomeCanvas *canvas = GNOME_CANVAS (router);

  update_tmp_line (router);

  if (GTK_WIDGET_REALIZED (canvas))
    {
      GdkCursor *cursor;
      
      if (router->rtools->tool_id == ROUTER_TOOL_CREATE_LINK)
	cursor = gdk_cursor_new (GDK_TCROSS);
      else if (router->rtools->tool_id)
	cursor = gdk_cursor_new (GDK_UL_ANGLE);
      else
	cursor = NULL;
      
      gdk_window_set_cursor (GTK_WIDGET (canvas)->window, cursor);
      
      if (cursor)
	gdk_cursor_destroy (cursor);

      gxk_status_clear ();
    }
}

static void
bst_snet_router_reset_tool (BstSNetRouter *router)
{
  if (router->rtools->tool_id == ROUTER_TOOL_CREATE_LINK)
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
	    gxk_status_set (GXK_STATUS_ERROR, "Input channel in use", NULL);
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
						   "swapped_signal::destroy", g_nullify_pointer, &router->tmp_line,
						   NULL);
	      gnome_canvas_points_free (gpoints);
	      router->world_x = event->button.x;	/* event coords are world already */
	      router->world_y = event->button.y;	/* event coords are world already */
	      bst_radio_tools_set_tool (router->rtools, ROUTER_TOOL_CREATE_LINK);
	      if (router->drag_is_input)
		gxk_status_set (GXK_STATUS_WAIT, "Create Link", "Select output module");
	      else
		gxk_status_set (GXK_STATUS_WAIT, "Create Link", "Select input module");
	      handled = TRUE;
	    }
	  else if (csource && csource->source != router->snet)
	    {
	      if (event->button.state & GDK_SHIFT_MASK)
		bst_canvas_source_toggle_info (csource);
	      else
		bst_canvas_source_toggle_params (csource);
	    }
	  else if (clink && !csource)
	    bst_canvas_link_toggle_view (clink);
	  handled = TRUE;
	}
      else if (event->button.button == 1 && router->rtools->tool_id == ROUTER_TOOL_CREATE_LINK) /* finish link */
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
		error = bse_source_set_input_by_id (router->drag_csource->source, router->drag_channel,
						    csource->source, ochannel);
	      else
		error = bse_source_set_input_by_id (csource->source, ichannel,
						    router->drag_csource->source, router->drag_channel);
	      router->drag_csource = NULL;
	      router->drag_channel = ~0;
	      bst_snet_router_reset_tool (router);
	      bst_status_eprintf (error, "Create Link");
	    }
	  handled = TRUE;
	}
      else if (event->type == GDK_BUTTON_PRESS && event->button.button == 3)
	{
	  if (csource)
	    {
	      GtkWidget *choice;
	      gchar *source_name = g_strconcat (bse_item_get_type_name (csource->source),
						": ",
						bse_item_get_name (csource->source),
						NULL);
	      guint i, has_inputs = 0;
	      
	      for (i = 0; has_inputs == 0 && i < bse_source_n_ichannels (csource->source); i++)
		has_inputs += bse_source_ichannel_get_n_joints (csource->source, i);
	      choice = bst_choice_menu_createv ("<BEAST-SNetRouter>/ModulePopup",
						BST_CHOICE_TITLE (source_name),
						BST_CHOICE_SEPERATOR,
						BST_CHOICE (2, "Properties", PROPERTIES),
						BST_CHOICE (6, "Reset Properties", PROPERTIES_RESET),
						BST_CHOICE_S (3, "Delete Inputs", NO_ILINK, has_inputs),
						BST_CHOICE_S (4, "Delete Outputs", NO_OLINK, bse_source_has_outputs (csource->source)),
						BST_CHOICE (5, "Show Info", INFO),
						BST_CHOICE_SEPERATOR,
						BST_CHOICE_S (1, "Delete", DELETE, csource->source != router->snet),
						BST_CHOICE_END);
	      g_free (source_name);
	      i = bst_choice_modal (choice, event->button.button, event->button.time);
	      switch (i)
		{
		  BseErrorType error;
		case 2:
		  bst_canvas_source_popup_params (csource);
		  break;
		case 6:
		  bst_canvas_source_reset_params (csource);
		  break;
		case 3:
		  bse_source_clear_inputs (csource->source);
		  break;
		case 4:
		  bse_source_clear_outputs (csource->source);
		  break;
		case 5:
		  bst_canvas_source_popup_info (csource);
		  break;
		case 1:
		  error = bse_snet_remove_source (router->snet, csource->source);
		  bst_status_eprintf (error, "Remove Module");
		  break;
		}
	      bst_choice_destroy (choice);
	      /* FIXME: get rid of artifacts left behind removal (mostly rect-ellipse) */
	      gtk_widget_queue_draw (GTK_WIDGET (canvas));
	      handled = TRUE;
	    }
	  else if (clink && !csource)
	    {
	      GtkWidget *choice;
	      guint i;

	      choice = bst_choice_menu_createv ("<BEAST-SNetRouter>/LinkPopup",
						BST_CHOICE_TITLE ("Module link"),
						BST_CHOICE_SEPERATOR,
                                                BST_CHOICE (2, "Show Info", INFO),
						BST_CHOICE_SEPERATOR,
						BST_CHOICE (1, "Delete", DELETE),
						BST_CHOICE_END);
	      i = bst_choice_modal (choice, event->button.button, event->button.time);
	      switch (i)
		{
		  BseErrorType error;
		case 1:
		  error = bse_source_unset_input_by_id (clink->icsource->source, clink->ichannel,
							clink->ocsource->source, clink->ochannel);
		  bst_status_eprintf (error, "Delete Link");
		  break;
		case 2:
		  bst_canvas_link_popup_view (clink);
		  break;
		}
	      bst_choice_destroy (choice);
	      /* FIXME: get rid of artifacts left behind removal (mostly rect-ellipse) */
	      gtk_widget_queue_draw (GTK_WIDGET (canvas));
	      handled = TRUE;
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
      if (event->button.button == 1 &&
	  router->rtools->tool_id &&
	  router->rtools->tool_id != ROUTER_TOOL_CREATE_LINK) /* add new source */
	{
	  BseErrorType error;
	  BseCategory *cat = bse_category_from_id (router->rtools->tool_id);

	  handled = TRUE;
	  gnome_canvas_window_to_world (canvas,
					event->button.x, event->button.y,
					&router->world_x, &router->world_y);

	  error = bse_snet_can_create_source (router->snet, cat->type);
	  if (!error)
	    {
	      SfiProxy module;
              bse_item_group_undo (router->snet, "Create Module");
              module = bse_snet_create_source (router->snet, cat->type);
              bse_source_set_pos (module,
                                  router->world_x / BST_CANVAS_SOURCE_PIXEL_SCALE,
                                  router->world_y / -BST_CANVAS_SOURCE_PIXEL_SCALE);
              bse_item_ungroup_undo (router->snet);
	    }
	  if (BST_SNET_EDIT_FALLBACK)
	    bst_radio_tools_set_tool (router->rtools, 0);
	  router->world_x = 0;
	  router->world_y = 0;
	  bst_status_eprintf (error, "Insert Module");
	}
      else if (event->button.button != 1 && router->rtools->tool_id == ROUTER_TOOL_CREATE_LINK)
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
    case GDK_KEY_PRESS:
      if (event->key.keyval == GDK_Escape)
	{
	  handled = TRUE;
	  gxk_status_clear ();
	  bst_radio_tools_set_tool (router->rtools, 0);
	}
      break;
    default:
      break;
    }
  
  if (!handled && GTK_WIDGET_CLASS (parent_class)->event)
    handled = GTK_WIDGET_CLASS (parent_class)->event (widget, event);
  
  return handled;
}

static gboolean
bst_snet_router_button_press (GtkWidget      *widget,
			      GdkEventButton *event)
{
  gboolean handled;

  /* chain parent class' handler */
  handled = GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, event);

  if (!handled && event->button == 3)
    {
      GtkItemFactory *popup_factory = BST_SNET_ROUTER_GET_CLASS (widget)->popup_factory;

      /* we get here, if there was no clickable canvas item */
      handled = TRUE;
      bst_menu_popup (popup_factory,
		      widget,
		      NULL, NULL,
		      event->x_root, event->y_root,
		      event->button, event->time);
    }
  
  return handled;
}

BstSNetRouter*
bst_snet_router_build_page (SfiProxy snet)
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
  BstSNetRouter *self;
  GtkWidget *zoomed_window, *router_box, *pix;
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  
  g_return_val_if_fail (BSE_IS_SNET (snet), NULL);
  
  self = g_object_new (BST_TYPE_SNET_ROUTER,
		       "aa", BST_SNET_ANTI_ALIASED,
		       NULL);
  bst_snet_router_set_snet (self, snet);
  
  router_box = g_object_new (GTK_TYPE_VBOX,
			     "visible", TRUE,
			     "homogeneous", FALSE,
			     "spacing", 3,
			     "border_width", 5,
			     NULL);
  gtk_box_pack_start (GTK_BOX (router_box), GTK_WIDGET (self->toolbar), FALSE, TRUE, 0);
  zoomed_window = g_object_new (BST_TYPE_ZOOMED_WINDOW,
				"visible", TRUE,
				"hscrollbar_policy", GTK_POLICY_ALWAYS,
				"vscrollbar_policy", GTK_POLICY_ALWAYS,
				"parent", router_box,
				NULL);
  g_object_connect (zoomed_window,
		    "swapped_signal::zoom", bst_snet_router_adjust_region, self,
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
  
  g_object_set (self,
		"visible", TRUE,
		"parent", zoomed_window,
		NULL);
  
  return BST_SNET_ROUTER (self);
}
