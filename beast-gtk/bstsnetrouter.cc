// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstsnetrouter.hh"

#include <math.h>
#include <string.h>
#include "bstcanvaslink.hh"
#include "bstmenus.hh"
#include "bstgconfig.hh"
#include "bstscrollgraph.hh"
#include <gdk/gdkkeysyms.h>


#define EPSILON 1e-6
#define ROUTER_TOOL(router)             (router->canvas_tool->action_id)
#define CHANNEL_HINTS(router)           (router->channel_toggle->action_id != FALSE)

enum {
  ROUTER_TOOL_EDIT              = 0,
  ROUTER_TOOL_CREATE_LINK       = G_MAXINT - 1024,      /* don't clash with category IDs / GQuarks */
  ROUTER_TOOL_TOGGLE_PALETTE,
  ROUTER_TOOL_CHANNEL_HINTS
};

/* --- tools & actions --- */
struct ToolBlurb { size_t action_id; const char *blurb; } tool_blurbs[] = {
  { ROUTER_TOOL_EDIT,   N_("Edit tool (mouse buttons 1-3)\n"
                           "Use button1 to create links, button2 "
                           "for movement and button3 to change properties"), },
};
static const GxkStockAction router_canvas_tools[] = {
  { N_("Edit"),         "<ctrl>E",      N_("Edit/Move/Menu (mouse buttons 1-3)"),
    ROUTER_TOOL_EDIT,                   BST_STOCK_MOUSE_TOOL },
};
static const GxkStockAction router_toolbar_actions[] = {
  { N_("Palette"),      "",             N_("Toggle visibility of the tool palette"),
    ROUTER_TOOL_TOGGLE_PALETTE,         BST_STOCK_PALETTE },
};


/* --- static variables --- */
static BstSNetRouterClass *bst_snet_router_class = NULL;


/* --- functions --- */
G_DEFINE_TYPE (BstSNetRouter, bst_snet_router, GNOME_TYPE_CANVAS);

static bool
filter_popup_modules (const Bse::AuxData &ad)
{
  String ahints = Bse::string_vector_find_value (ad.attributes, "hints=");
  if (Bse::string_option_check (ahints, "deprecated") && !BST_DVL_HINTS)
    return false;
  if (Bse::string_option_check (ahints, "unstable") && !BST_DVL_HINTS)
    return false;
  return true;
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

static void
bst_snet_router_reset_tool (BstSNetRouter *self)
{
  if (ROUTER_TOOL (self) == ROUTER_TOOL_CREATE_LINK)
    gxk_action_group_select (self->canvas_tool, ROUTER_TOOL_EDIT);
}

static void
bst_snet_router_destroy_contents (BstSNetRouter *self)
{
  GnomeCanvas *canvas = GNOME_CANVAS (self);
  if (canvas->root)
    {
      GnomeCanvasGroup *group = GNOME_CANVAS_GROUP (canvas->root);

      while (group->item_list)
        gtk_object_destroy ((GtkObject*) group->item_list->data);
    }
  while (self->canvas_links)
    {
      BstCanvasLink *link = (BstCanvasLink*) sfi_ring_pop_head (&self->canvas_links);
      gtk_object_destroy (GTK_OBJECT (link));
      g_object_unref (link);
    }
}

static void
bst_snet_router_destroy (GtkObject *object)
{
  BstSNetRouter *self = BST_SNET_ROUTER (object);

  bst_snet_router_reset_tool (self);
  bst_snet_router_destroy_contents (self);
  bst_snet_router_set_snet (self, Bse::SNetH());

  gxk_action_group_dispose (self->canvas_tool);
  gxk_action_group_dispose (self->channel_toggle);

  if (self->palette)
    gtk_widget_destroy (self->palette);

  GTK_OBJECT_CLASS (bst_snet_router_parent_class)->destroy (object);
}

static void
bst_snet_router_finalize (GObject *object)
{
  BstSNetRouter *self = BST_SNET_ROUTER (object);

  g_object_unref (self->canvas_tool);
  g_object_unref (self->channel_toggle);

  G_OBJECT_CLASS (bst_snet_router_parent_class)->finalize (object);
  using namespace Bse;
  self->snet.~SNetH();
}

static void
bst_snet_router_update_links (BstSNetRouter   *self,
                              BstCanvasSource *csource)
{
  GnomeCanvas *canvas = GNOME_CANVAS (self);

  /* sort out input links of this csource */
  SfiRing *node, *iring = NULL, *tmp_ring = self->canvas_links;
  self->canvas_links = NULL;
  while (tmp_ring)
    {
      BstCanvasLink *link = (BstCanvasLink*) sfi_ring_pop_head (&tmp_ring);
      if (link->icsource == csource)
        iring = sfi_ring_append (iring, link);
      else
        self->canvas_links = sfi_ring_append (self->canvas_links, link);
    }

  /* now we walk the (c)source's input channels, keep
   * existing links and create new ones on the fly
   */
  for (int i = 0; i < csource->source.n_ichannels(); i++)
    {
      guint j, n_joints = csource->source.ichannel_get_n_joints (i);
      for (j = 0; j < n_joints; j++)
        {
          Bse::ObjectH obj = csource->source;
          assert_return (obj != NULL);
          Bse::SourceH isource = csource->source;
          assert_return (isource != NULL);
          Bse::SourceH osource = isource.ichannel_get_osource (i, j);
          if (!osource)
            continue;
          guint ochannel = csource->source.ichannel_get_ochannel (i, j);
          BstCanvasSource *ocsource = bst_snet_router_csource_from_source (self, osource.proxy_id());
          if (!ocsource)
            {
              Bse::warning ("Couldn't figure CanvasSource Item from BSE module \"%s\"", osource.get_name_or_type());
              continue;
            }
          /* find corresponding link */
          BstCanvasLink *link = NULL;
          for (node = iring; node; node = sfi_ring_walk (node, iring))
            {
              link = (BstCanvasLink*) iring->data;
              if (link &&
                  link->ichannel == uint (i) &&
                  link->ocsource == ocsource &&
                  link->ochannel == ochannel)
                break;
            }
          if (node) /* cool, found one already */
            node->data = NULL;
          else /* got none, ok, need to create new one */
            {
              link = (BstCanvasLink*) g_object_ref (bst_canvas_link_new (GNOME_CANVAS_GROUP (canvas->root)));
              bst_canvas_link_set_icsource (link, csource, i);
              bst_canvas_link_set_ocsource (link, ocsource, ochannel);
              /* queue update cause ellipse-rect is broken */
              gnome_canvas_FIXME_hard_update (canvas);
            }
          self->canvas_links = sfi_ring_append (self->canvas_links, link);
        }
    }

  /* cleanup iring and left-over link objects */
  while (iring)
    {
      BstCanvasLink *link = (BstCanvasLink*) sfi_ring_pop_head (&iring);
      if (link)
        {
          gtk_object_destroy (GTK_OBJECT (link));
          g_object_unref (link);
        }
    }
}

static SfiRing *queued_canvas_sources = NULL;

static gboolean
bst_snet_router_handle_link_update (gpointer data)
{
  GDK_THREADS_ENTER();
  while (queued_canvas_sources)
    {
      BstCanvasSource *csource = (BstCanvasSource*) sfi_ring_pop_head (&queued_canvas_sources);
      GnomeCanvasItem *citem = GNOME_CANVAS_ITEM (csource);
      GnomeCanvas *canvas = citem->canvas;
      if (BST_IS_SNET_ROUTER (canvas))
        bst_snet_router_update_links (BST_SNET_ROUTER (canvas), csource);
      g_object_unref (csource);
    }
  GDK_THREADS_LEAVE();
  return FALSE;
}

static void
bst_snet_router_queue_link_update (BstSNetRouter   *self,
                                   BstCanvasSource *csource)
{
  if (!sfi_ring_find (queued_canvas_sources, csource))
    {
      g_object_ref (csource);
      /* make sure a handler is executed as soon as possible to
       * update the canvas links (however, it needs to be executed
       * with a priority low enough so that additional item-added
       * signals are processed first)
       */
      if (!queued_canvas_sources)
        g_idle_add_full (G_PRIORITY_HIGH, bst_snet_router_handle_link_update, NULL, NULL);
      queued_canvas_sources = sfi_ring_append (queued_canvas_sources, csource);
    }
}

GtkWidget*
bst_snet_router_new (Bse::SNetH snet)
{
  GtkWidget *router;

  assert_return (snet != NULL, NULL);

  router = gtk_widget_new (BST_TYPE_SNET_ROUTER,
                           "aa", BST_SNET_ANTI_ALIASED,
                           NULL);
  bst_snet_router_set_snet (BST_SNET_ROUTER (router), snet);

  return router;
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
  bst_canvas_source_set_channel_hints (BST_CANVAS_SOURCE (csource), CHANNEL_HINTS (self));
  g_object_connect (csource,
                    "swapped_signal::update_links", bst_snet_router_queue_link_update, self,
                    NULL);
  bst_canvas_source_update_links (BST_CANVAS_SOURCE (csource));  // FIXME: maybe too early if other items are pending (idle link update)
  /* queue update cause ellipse-rect is broken */
  gnome_canvas_FIXME_hard_update (canvas);
}

void
bst_snet_router_set_snet (BstSNetRouter *self, Bse::SNetH snet)
{
  assert_return (BST_IS_SNET_ROUTER (self));

  if (self->snet)
    {
      bst_snet_router_destroy_contents (self);
      bse_proxy_disconnect (self->snet.proxy_id(),
                            "any_signal", bst_snet_router_item_added, self,
                            NULL);
      self->snet = Bse::SNetH();
    }
  self->snet = snet;
  if (self->snet)
    {
      bse_proxy_connect (self->snet.proxy_id(),
                         "swapped_signal::item_added", bst_snet_router_item_added, self,
                         NULL);
      bst_snet_router_update (self);
      bst_snet_router_adjust_region (self);
    }
}

static void
bst_router_popup_select (gpointer user_data, size_t action_id)
{
  BstSNetRouter *self = BST_SNET_ROUTER (user_data);
  gxk_action_group_select (self->canvas_tool, action_id);
}

void
bst_snet_router_update (BstSNetRouter *self)
{
  GnomeCanvas *canvas;
  GSList *slist, *csources = NULL;

  assert_return (BST_IS_SNET_ROUTER (self));

  canvas = GNOME_CANVAS (self);

  /* destroy all canvas sources */
  bst_snet_router_destroy_contents (self);

#if 0
  {
      /* add canvas source for the snet itself */
      csource = bst_canvas_source_new (GNOME_CANVAS_GROUP (canvas->root), self->snet);
      bst_canvas_source_set_channel_hints (BST_CANVAS_SOURCE (csource), CHANNEL_HINTS (self));
      g_object_connect (csource,
                        "swapped_signal::update_links", bst_snet_router_queue_link_update, self,
                        NULL);
      csources = g_slist_prepend (csources, csource);
  }
#endif

  /* walk all child sources */
  Bse::ItemSeq items = self->snet.list_children();
  for (size_t i = 0; i < items.size(); i++)
    {
      SfiProxy item = items[i].proxy_id();

      if (BSE_IS_SOURCE (item))
        {
          GnomeCanvasItem *csource = bst_canvas_source_new (GNOME_CANVAS_GROUP (canvas->root), item);
          bst_canvas_source_set_channel_hints (BST_CANVAS_SOURCE (csource), CHANNEL_HINTS (self));
          g_object_connect (csource,
                            "swapped_signal::update_links", bst_snet_router_queue_link_update, self,
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
  GDK_THREADS_ENTER ();

  GnomeCanvas *canvas = GNOME_CANVAS (data);
  double *d = (double*) gtk_object_get_data (GTK_OBJECT (canvas), "zoom_d");

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
  double *d = (double*) gtk_object_get_data (object, "zoom_d");

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

void
bst_snet_router_adjust_region (BstSNetRouter *router)
{
  GtkAdjustment *adjustment;
  GtkLayout *layout;
  GnomeCanvas *canvas;
  gdouble x1, y1, x2, y2;

  assert_return (BST_IS_SNET_ROUTER (router));

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
                                     SfiProxy       source)
{
  GnomeCanvas *canvas;
  GnomeCanvasGroup *root;
  GList *list;

  assert_return (BST_IS_SNET_ROUTER (router), NULL);
  assert_return (BSE_IS_SOURCE (source), NULL);

  canvas = GNOME_CANVAS (router);
  root = GNOME_CANVAS_GROUP (canvas->root);
  for (list = root->item_list; list; list = list->next)
    {
      BstCanvasSource *csource = (BstCanvasSource*) list->data;

      if (BST_IS_CANVAS_SOURCE (csource) && csource->source.proxy_id() == source)
        return csource;
    }

  return NULL;
}

static void
update_tmp_line (BstSNetRouter *self)
{
  if (self->tmp_line)
    {
      if (ROUTER_TOOL (self) != ROUTER_TOOL_CREATE_LINK)
        {
          gtk_object_destroy (GTK_OBJECT (self->tmp_line));
          gxk_status_clear ();
          /* queue update cause canvas-line is broken and leaves artefacts */
          gnome_canvas_FIXME_hard_update (GNOME_CANVAS (self));
        }
      else
        {
          GnomeCanvasPoints *gpoints = NULL;

          gtk_object_get (GTK_OBJECT (self->tmp_line), "points", &gpoints, NULL);
          if (gpoints)
            {
              gpoints->coords[2] = self->world_x;
              gpoints->coords[3] = self->world_y;
              if (gpoints->coords[0] > gpoints->coords[2])
                gpoints->coords[2] += 0.5;
              else
                gpoints->coords[2] -= 0.5;
              if (gpoints->coords[1] > gpoints->coords[3])
                gpoints->coords[3] += 0.5;
              else
                gpoints->coords[3] -= 0.5;
              g_object_set (GTK_OBJECT (self->tmp_line), "points", gpoints, NULL);
              gnome_canvas_points_free (gpoints);
            }
        }
    }
}

static void
bst_router_tool_changed (BstSNetRouter *self)
{
  GnomeCanvas *canvas = GNOME_CANVAS (self);

  update_tmp_line (self);

  if (GTK_WIDGET_REALIZED (canvas))
    {
      GdkCursor *cursor;

      if (ROUTER_TOOL (self) == ROUTER_TOOL_CREATE_LINK)
        cursor = gdk_cursor_new (GDK_TCROSS);
      else if (ROUTER_TOOL (self))
        cursor = gdk_cursor_new (GDK_UL_ANGLE);
      else
        cursor = NULL;

      gdk_window_set_cursor (GTK_WIDGET (canvas)->window, cursor);

      if (cursor)
        gdk_cursor_destroy (cursor);

      gxk_status_clear ();
    }
}

static gboolean
bst_snet_router_root_event (BstSNetRouter   *self,
                            GdkEvent        *event)
{
  GnomeCanvas *canvas = GNOME_CANVAS (self);
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
      at_channel = ochannel != ~uint (0) || ichannel != ~uint (0);
      if (event->type == GDK_BUTTON_PRESS &&
          bst_mouse_button_activate (event) &&
          ROUTER_TOOL (self) == 0)                                      /* start link (or popup property dialog) */
        {
          assert_return (self->tmp_line == NULL, FALSE);
          self->drag_is_input = ichannel != ~uint (0);
          if (csource && at_channel && self->drag_is_input &&  /* ichannel in use */
              !bst_canvas_source_ichannel_free (csource, ichannel))
            gxk_status_set (GXK_STATUS_ERROR, _("Input channel in use"), NULL);
          else if (csource && at_channel) /* i/o link */
            {
              GnomeCanvasPoints *gpoints = gnome_canvas_points_new (2);

              self->drag_channel = self->drag_is_input ? ichannel : ochannel;
              self->drag_csource = csource;
              (self->drag_is_input ?
               bst_canvas_source_ichannel_pos :
               bst_canvas_source_ochannel_pos) (csource,
                                                self->drag_channel,
                                                &gpoints->coords[0],
                                                &gpoints->coords[1]);
              gpoints->coords[2] = gpoints->coords[0] + 50;
              gpoints->coords[3] = gpoints->coords[1] + 50;
              self->tmp_line = gnome_canvas_item_new (GNOME_CANVAS_GROUP (canvas->root),
                                                      GNOME_TYPE_CANVAS_LINE,
                                                      "fill_color", "black",
                                                      "points", gpoints,
                                                      NULL);
              g_object_connect (self->tmp_line, "swapped_signal::destroy", g_nullify_pointer, &self->tmp_line, NULL);
              gnome_canvas_points_free (gpoints);
              self->world_x = event->button.x;  /* event coords are world already */
              self->world_y = event->button.y;  /* event coords are world already */
              gxk_action_group_select (self->canvas_tool, ROUTER_TOOL_CREATE_LINK);
              if (self->drag_is_input)
                gxk_status_set (GXK_STATUS_WAIT, _("Create Link"), _("Select output module"));
              else
                gxk_status_set (GXK_STATUS_WAIT, _("Create Link"), _("Select input module"));
              handled = TRUE;
            }
          else if (csource && csource->source != self->snet)
            {
              if (bst_mouse_button_activate2 (event))
                bst_canvas_source_toggle_info (csource);
              else
                bst_canvas_source_toggle_params (csource);
            }
          else if (clink && !csource)
            bst_canvas_link_toggle_view (clink);
          handled = TRUE;
        }
      else if (bst_mouse_button_activate (event) && ROUTER_TOOL (self) == ROUTER_TOOL_CREATE_LINK) /* finish link */
        {
          if (event->type == GDK_BUTTON_RELEASE && csource == self->drag_csource &&
              self->drag_channel == (self->drag_is_input ? ichannel : ochannel))
            {
              /* don't react to button releases on the point we started from */
            }
          else
            {
              Bse::Error error;
              if (!csource || (self->drag_is_input ? ochannel : ichannel) == ~uint (0))
                error = self->drag_is_input ? Bse::Error::SOURCE_NO_SUCH_OCHANNEL : Bse::Error::SOURCE_NO_SUCH_ICHANNEL;
              else if (self->drag_is_input)
                error = self->drag_csource->source.set_input_by_id (self->drag_channel, csource->source, ochannel);
              else
                error = csource->source.set_input_by_id (ichannel, self->drag_csource->source, self->drag_channel);
              self->drag_csource = NULL;
              self->drag_channel = ~0;
              bst_snet_router_reset_tool (self);
              bst_status_eprintf (error, _("Create Link"));
            }
          handled = TRUE;
        }
      else if (event->type == GDK_BUTTON_PRESS && bst_mouse_button_context (event))    /* module context menu */
        {
          if (csource)
            {
              GtkWidget *choice;
              gchar *source_name = g_strconcat (csource->source.get_type_name().c_str(),
                                                ": ",
                                                csource->source.get_name().c_str(),
                                                NULL);
              /* create popup sumenu */
              uint has_inputs = 0, monitor_ids = 1000000;
              choice = bst_choice_menu_createv ("<BEAST-SNetRouter>/ModuleChannelPopup", NULL);
              for (int i = 0; i < csource->source.n_ochannels(); i++)
                {
                  gchar *name = g_strdup_format ("%d: %s", i + 1, csource->source.ochannel_label (i));
                  bst_choice_menu_add_choice_and_free (choice, BST_CHOICE (monitor_ids + i, name, NONE));
                  g_free (name);
                }
              /* create popup */
              for (int i = 0; has_inputs == 0 && i < csource->source.n_ichannels(); i++)
                has_inputs += csource->source.ichannel_get_n_joints (i);
              choice = bst_choice_menu_createv ("<BEAST-SNetRouter>/ModulePopup",
                                                BST_CHOICE_TITLE (source_name),
                                                BST_CHOICE_SEPERATOR,
                                                BST_CHOICE (2, _("Properties"), PROPERTIES),
                                                BST_CHOICE (6, _("Reset Properties"), PROPERTIES_RESET),
                                                BST_CHOICE_S (3, _("Disconnect Inputs"), NO_ILINK, has_inputs),
                                                BST_CHOICE_S (4, _("Disconnect Outputs"), NO_OLINK, csource->source.has_outputs()),
                                                BST_CHOICE_SEPERATOR,
                                                BST_CHOICE (5, _("Show Info"), INFO),
                                                BST_CHOICE_SUBMENU (_("Output Signal Monitor"), choice, SIGNAL),
                                                BST_CHOICE_SEPERATOR,
                                                BST_CHOICE_S (1, _("Delete"), DELETE, csource->source != self->snet),
                                                BST_CHOICE_END);
              g_free (source_name);
              int i = bst_choice_modal (choice, event->button.button, event->button.time);
              switch (i)
                {
                  GtkWidget *dialog;
                  Bse::Error error;
                case 2:
                  bst_canvas_source_popup_params (csource);
                  break;
                case 6:
                  bst_canvas_source_reset_params (csource);
                  break;
                case 3:
                  csource->source.clear_inputs();
                  break;
                case 4:
                  csource->source.clear_outputs();
                  break;
                case 5:
                  bst_canvas_source_popup_info (csource);
                  break;
                case 1:
                  error = self->snet.remove_source (csource->source);
                  bst_status_eprintf (error, _("Remove Module"));
                  break;
                case 0: break;
                default:
                  dialog = bst_scrollgraph_build_dialog (GTK_WIDGET (self), csource->source, i - monitor_ids);
                  gtk_widget_show (dialog);
                  break;
                }
              bst_choice_destroy (choice);
              /* FIXME: get rid of artifacts left behind removal (mostly rect-ellipse) */
              gtk_widget_queue_draw (GTK_WIDGET (canvas));
              handled = TRUE;
            }
          else if (clink && !csource)   /* link context menu */
            {
              GtkWidget *choice;
              guint i;

              choice = bst_choice_menu_createv ("<BEAST-SNetRouter>/LinkPopup",
                                                BST_CHOICE_TITLE (_("Module link")),
                                                BST_CHOICE_SEPERATOR,
                                                BST_CHOICE (2, _("Show Info"), INFO),
                                                BST_CHOICE_SEPERATOR,
                                                BST_CHOICE (1, _("Delete"), DELETE),
                                                BST_CHOICE_END);
              i = bst_choice_modal (choice, event->button.button, event->button.time);
              switch (i)
                {
                  Bse::Error error;
                case 1:
                  {
                    error = clink->icsource->source.unset_input_by_id (clink->ichannel, clink->ocsource->source, clink->ochannel);
                    bst_status_eprintf (error, _("Delete Link"));
                  }
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
  BstSNetRouter *self = BST_SNET_ROUTER (widget);
  GnomeCanvas *canvas = GNOME_CANVAS (self);
  gboolean handled = FALSE;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      if (bst_mouse_button_activate1 (event) &&
          ROUTER_TOOL (self) &&
          ROUTER_TOOL (self) != ROUTER_TOOL_CREATE_LINK) /* add new source */
        {
          const Bse::AuxData &ad = bse_server.find_module_type (g_quark_to_string (ROUTER_TOOL (self)));
          assert_return (ad.entity.empty() == false, false);

          gnome_canvas_window_to_world (canvas,
                                        event->button.x, event->button.y,
                                        &self->world_x, &self->world_y);

          Bse::Error error = self->snet.can_create_source (ad.entity);
          if (error == 0)
            {
              self->snet.group_undo ("Create Module");
              SfiProxy module = self->snet.create_source (ad.entity).proxy_id();
              Bse::SourceH sourceh = Bse::SourceH::down_cast (bse_server.from_proxy (module));
              sourceh.set_pos (self->world_x / BST_CANVAS_SOURCE_PIXEL_SCALE, self->world_y / -BST_CANVAS_SOURCE_PIXEL_SCALE);
              self->snet.ungroup_undo();
            }
          if (BST_SNET_EDIT_FALLBACK)
            gxk_action_group_select (self->canvas_tool, ROUTER_TOOL_EDIT);
          self->world_x = 0;
          self->world_y = 0;
          bst_status_eprintf (error, _("Insert Module"));
          handled = TRUE;
        }
      else if (!bst_mouse_button_activate1 (event) && ROUTER_TOOL (self) == ROUTER_TOOL_CREATE_LINK)
        {
          /* disturbing button presses, reset stuff */
          handled = TRUE;
          bst_snet_router_reset_tool (self);
        }
      break;
    case GDK_MOTION_NOTIFY:
      gnome_canvas_window_to_world (canvas,
                                    event->motion.x, event->motion.y,
                                    &self->world_x, &self->world_y);
      update_tmp_line (self);
      break;
    case GDK_KEY_PRESS:
      if (event->key.keyval == GDK_Escape)
        {
          handled = TRUE;
          gxk_status_clear ();
          gxk_action_group_select (self->canvas_tool, ROUTER_TOOL_EDIT);
        }
      break;
    default:
      break;
    }
  if (!handled && GTK_WIDGET_CLASS (bst_snet_router_parent_class)->event)
    handled = GTK_WIDGET_CLASS (bst_snet_router_parent_class)->event (widget, event);
  return handled;
}
static gboolean
bst_snet_router_button_press (GtkWidget      *widget,
                              GdkEventButton *event)
{
  BstSNetRouter *self = BST_SNET_ROUTER (widget);
  gboolean handled;
  /* chain parent class' handler */
  handled = GTK_WIDGET_CLASS (bst_snet_router_parent_class)->button_press_event (widget, event);
  if (!handled && bst_mouse_button_context (event) && self->canvas_popup)
    gxk_menu_popup (self->canvas_popup,
                    event->x_root, event->y_root,
                    event->button, event->time);
  return handled;
}
static void
snet_router_tool2text (BstSNetRouter *self)
{
  GtkLabel *label = (GtkLabel*) gxk_radget_find (self->palette, "type-label");
  const char *router_tool_name = g_quark_to_string (ROUTER_TOOL (self));
  if (!router_tool_name)
    router_tool_name = "";
  const Bse::AuxData &ad = bse_server.find_module_type (router_tool_name);
  const String authors = Bse::string_vector_find_value (ad.attributes, "authors=");
  const String license = Bse::string_vector_find_value (ad.attributes, "license=");
  String blurb = Bse::string_vector_find_value (ad.attributes, "blurb=");
  String title = Bse::string_vector_find_value (ad.attributes, "title=");
  if (title.empty())
    title = ad.entity;
  if (blurb.empty())
    {
      for (uint i = 0; i < G_N_ELEMENTS (tool_blurbs); i++)
        if (tool_blurbs[i].action_id == ROUTER_TOOL (self))
          blurb = tool_blurbs[i].blurb;
      for (uint i = 0; i < G_N_ELEMENTS (router_canvas_tools); i++)
        if (router_canvas_tools[i].action_id == ROUTER_TOOL (self))
          title = router_canvas_tools[i].name;
    }
  gxk_scroll_text_set (self->palette_text, blurb.c_str());
  if (!blurb.empty() && (!authors.empty() || !license.empty()))
    gxk_scroll_text_append (self->palette_text, "\n");
  if (!authors.empty())
    {
      gxk_scroll_text_append (self->palette_text, "\n");
      gxk_scroll_text_append (self->palette_text, "Authors: ");
      gxk_scroll_text_append (self->palette_text, authors.c_str());
      gxk_scroll_text_append (self->palette_text, "\n");
    }
  if (!license.empty())
    {
      gxk_scroll_text_append (self->palette_text, "\n");
      gxk_scroll_text_append (self->palette_text, "License: ");
      gxk_scroll_text_append (self->palette_text, license.c_str());
    }
  g_object_set (label, "label", title.c_str(), NULL);
}

static void
snet_router_action_exec (gpointer        user_data,
                         size_t          action_id)
{
  BstSNetRouter *self = BST_SNET_ROUTER (user_data);
  GnomeCanvas *canvas = GNOME_CANVAS (self);
  GnomeCanvasGroup *root = GNOME_CANVAS_GROUP (canvas->root);

  switch (action_id)
    {
      GList *list;
    case ROUTER_TOOL_TOGGLE_PALETTE:
      if (!self->palette)
        {
          self->palette = (GtkWidget*) gxk_dialog_new (&self->palette,
                                                       GTK_OBJECT (self),
                                                       GXK_DIALOG_HIDE_ON_DELETE,
                                                       _("Palette"),
                                                       (GtkWidget*) gxk_radget_create ("beast", "snet-palette", NULL));
          /* add actions to palette */
          gxk_widget_republish_actions (self->palette, "router-util-actions", self);
          gxk_widget_republish_actions (self->palette, "router-canvas-tools", self);
          gxk_widget_republish_actions (self->palette, "router-module-actions", self);
          gxk_widget_republish_actions (self->palette, "router-palette-modules", self);
          /* add text handling to palette */
          self->palette_text = gxk_scroll_text_create (GXK_SCROLL_TEXT_WIDGET_LOOK, NULL);
          gxk_radget_add (self->palette, "text-area", self->palette_text);
          g_signal_connect_object (self->canvas_tool, "changed", G_CALLBACK (snet_router_tool2text), self, G_CONNECT_SWAPPED);
          snet_router_tool2text (self);
        }
      if (!GTK_WIDGET_VISIBLE (self->palette))
        gxk_idle_showraise (self->palette);
      else
        gtk_widget_hide (self->palette);
      break;
    case ROUTER_TOOL_CHANNEL_HINTS:
      for (list = root->item_list; list; list = list->next)
        {
          BstCanvasSource *csource = (BstCanvasSource*) list->data;
          if (BST_IS_CANVAS_SOURCE (csource))
            bst_canvas_source_set_channel_hints (csource, CHANNEL_HINTS (self));
        }
      break;
    }
  gxk_widget_update_actions_downwards (self);
}

BstSNetRouter*
bst_snet_router_build_page (Bse::SNetH snet)
{
  static const char *zoom_xpm[] = {
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
  GtkWidget *zoomed_window, *pix;
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  GxkRadget *radget;

  assert_return (snet != NULL, NULL);

  /* main radget */
  radget = gxk_radget_create ("beast", "snet-view", NULL);

  /* router */
  self = (BstSNetRouter*) g_object_new (BST_TYPE_SNET_ROUTER,
                                        "aa", BST_SNET_ANTI_ALIASED,
                                        "visible", TRUE,
                                        NULL);
  bst_snet_router_set_snet (self, snet);
  gxk_radget_add (radget, "zoomed-window", self);
  self->canvas_popup = (GtkMenu*) gxk_radget_find (radget, "snet-popup");

  /* setup zoomed window and its toggle pixmap */
  zoomed_window = (GtkWidget*) gxk_radget_find (radget, "zoomed-window");
  g_object_connect (zoomed_window,
                    "swapped_signal::zoom", bst_snet_router_adjust_region, self,
                    "swapped_signal::zoom", gtk_false, NULL,
                    NULL);
  pixmap = gdk_pixmap_colormap_create_from_xpm_d (NULL, gtk_widget_get_colormap (zoomed_window), &mask, NULL, (char**) zoom_xpm);
  pix = gtk_pixmap_new (pixmap, mask);
  gdk_pixmap_unref (pixmap);
  gdk_pixmap_unref (mask);
  gtk_widget_set (pix,
                  "visible", TRUE,
                  "parent", BST_ZOOMED_WINDOW (zoomed_window)->toggle_button,
                  NULL);

  /* add Zoom spinner */
  gxk_radget_add (radget, "zoom-area",
                  g_object_new (GTK_TYPE_SPIN_BUTTON,
                                "visible", TRUE,
                                "adjustment", self->adjustment,
                                "digits", 2,
                                "width_request", 2 * gxk_size_width (GXK_ICON_SIZE_TOOLBAR),
                                NULL));

  return self;
}

static void
bst_snet_router_init (BstSNetRouter      *self)
{
  new (&self->snet) Bse::SNetH();
  GnomeCanvas *canvas = GNOME_CANVAS (self);
  GxkActionList *canvas_modules, *toolbar_modules, *palette_modules;
  uint i, n;

  self->palette = NULL;
  self->adjustment = NULL;
  self->world_x = 0;
  self->world_y = 0;
  self->drag_is_input = FALSE;
  self->drag_channel = ~0;
  self->drag_csource = NULL;
  self->tmp_line = NULL;
  self->canvas_links = NULL;

  /* module selection group */
  self->canvas_tool = gxk_action_group_new ();
  g_object_connect (self->canvas_tool, "swapped_signal::changed", bst_router_tool_changed, self, NULL);
  g_object_connect (self,
                    "signal_after::show", bst_snet_router_reset_tool, NULL,
                    "signal::viewable_changed", bst_snet_router_viewable_changed, NULL,
                    NULL);
  g_object_connect (canvas->root,
                    "swapped_signal::event", bst_snet_router_root_event, self,
                    NULL);

  self->adjustment = (GtkAdjustment*) gtk_adjustment_new (1.0, 0.20, 5.00, 0.05, 0.50, 0 /* 0.50 - spin buttons needs 0 */);
  g_object_connect (self->adjustment,
                    "swapped_signal::value_changed", bst_snet_router_adjust_zoom, self,
                    "swapped_signal::destroy", g_nullify_pointer, &self->adjustment,
                    NULL);

  /* publish canvas toolbar tools & actions */
  gxk_widget_publish_actions_grouped (self, self->canvas_tool, "router-canvas-tools",
                                      G_N_ELEMENTS (router_canvas_tools), router_canvas_tools,
                                      NULL, NULL, bst_router_popup_select);
  gxk_widget_publish_actions (self, "router-toolbar-actions",
                              G_N_ELEMENTS (router_toolbar_actions), router_toolbar_actions,
                              NULL, NULL, snet_router_action_exec);

  // construct module type action lists
  canvas_modules = gxk_action_list_create_grouped (self->canvas_tool);
  palette_modules = gxk_action_list_create_grouped (self->canvas_tool);
  toolbar_modules = gxk_action_list_create_grouped (self->canvas_tool);
  // toolbar module types
  static struct { const char *type, *name, *tip; } toolbar_types[] = {
    { "BsePcmInput",        N_("Input"),      N_("PCM Input module") },
    { "BseStandardOsc",     N_("Oscillator"), N_("Standard oscillator module") },
    { "BseSimpleADSR",      N_("ADSR"),       N_("ADSR Envelope Generator") },
    { "BseAmplifier",       N_("DCA"),        N_("Standard amplifier module") },
    { "BsePcmOutput",       N_("Output"),     N_("PCM Output module") },
  };
  Bse::AuxDataSeq ads = bse_server.list_module_types ();
  const Bse::AuxData *tbmodules[G_N_ELEMENTS (toolbar_types)] = { NULL, };
  for (i = 0; i < ads.size(); i++)
    {
      const char *stock_fallback = NULL;
      const Bse::AuxData &ad = ads[i];
      // filter inappropriate modules
      if (!filter_popup_modules (ad))
        continue;
      // fetch icon, default for LADSPA
      Bse::Icon icon;
      if (strncmp (ad.entity.c_str(), "BseLadspaModule_", 16) == 0)
        stock_fallback = BST_STOCK_LADSPA;
      else
        icon = bse_server.module_type_icon (ad.entity);
      // provide module as canvas tool
      bst_action_list_add_module (canvas_modules, ad, icon, stock_fallback, NULL, bst_router_popup_select, self);
      // provide selected modules in the palette
      if (icon.width > 0 && icon.height > 0)
        bst_action_list_add_module (palette_modules, ad, icon, stock_fallback, NULL, bst_router_popup_select, self);
      // identify modules to be provided in the toolbar
      for (n = 0; n < G_N_ELEMENTS (toolbar_types); n++)
        if (ad.entity == toolbar_types[n].type)
          tbmodules[n] = &ad;
    }
  /* provide certain variants in the toolbar */
  for (n = 0; n < G_N_ELEMENTS (toolbar_types); n++)
    if (tbmodules[n])
      {
        const Bse::AuxData &ad = *tbmodules[n];
        const char *stock_fallback = NULL;
        Bse::Icon icon;
        if (strncmp (ad.entity.c_str(), "BseLadspaModule_", 16) == 0)
          stock_fallback = BST_STOCK_LADSPA;
        else
          icon = bse_server.module_type_icon (ad.entity);

        const char *stock_id;
        if (icon.width && icon.height)
          {
            assert_return (icon.width * icon.height == int (icon.pixels.size()));
            bst_stock_register_icon (ad.entity.c_str(), 4, icon.width, icon.height, icon.width * 4, (const uint8*) icon.pixels.data());
            stock_id = ad.entity.c_str();
          }
        else
          stock_id = stock_fallback;
        gxk_action_list_add_translated (toolbar_modules, ad.entity.c_str(), _(toolbar_types[n].name), NULL, _(toolbar_types[n].tip),
                                        g_quark_from_string (ad.entity.c_str()), stock_id, NULL, bst_router_popup_select, self);
      }
  /* polish and publish */
  gxk_action_list_sort (canvas_modules);
  gxk_widget_publish_action_list (self, "router-module-actions", canvas_modules);
  gxk_action_list_sort (palette_modules);
  gxk_widget_publish_action_list (self, "router-palette-modules", palette_modules);
  gxk_widget_publish_action_list (self, "router-toolbar-modules", toolbar_modules);

  /* channel hints toggle */
  self->channel_toggle = gxk_action_toggle_new ();
  gxk_action_group_select (self->channel_toggle, ROUTER_TOOL_CHANNEL_HINTS);
  gxk_widget_publish_grouped_translated (self, self->channel_toggle, "router-channel-toggle",
                                         NULL, _("Channel Hints"), NULL, _("Toggle channel name hints"),
                                         ROUTER_TOOL_CHANNEL_HINTS, NULL,
                                         NULL, snet_router_action_exec);
  /* set default tool */
  gxk_action_group_select (self->canvas_tool, ROUTER_TOOL_EDIT);
}

static void
bst_snet_router_class_init (BstSNetRouterClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  bst_snet_router_class = klass;

  gobject_class->finalize = bst_snet_router_finalize;
  object_class->destroy = bst_snet_router_destroy;

  widget_class->event = bst_snet_router_event;
  widget_class->button_press_event = bst_snet_router_button_press;

  klass->popup_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<BstSnetRouter>", NULL);
  gtk_accel_group_lock (klass->popup_factory->accel_group);
}
