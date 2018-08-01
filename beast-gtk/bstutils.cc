// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstmarshal.h"
#include "bstmarshal.cc"

#include "bstutils.hh"

#include "bstgconfig.hh"
#include "bstmenus.hh"
#include "bsttrackview.hh"
#include "bstwaveview.hh"
#include "bstsoundfontview.hh"
#include "bstpartview.hh"
#include "bstbusmixer.hh"
#include "bstbuseditor.hh"
#include "bstbusview.hh"
#include "bstpianoroll.hh"
#include "bstpatternview.hh"
#include "bsteventroll.hh"
#include "bstgrowbar.hh"
#include "bstdbmeter.hh"
#include "bstscrollgraph.hh"

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>


namespace Bst {

static const uint16 EVENT_LOOP_RUNNING   = 0xffff;
static uint16       event_loop_quit_code = EVENT_LOOP_RUNNING;

int
event_loop_run ()
{
  // run main event loop until main_quit()
  while (event_loop_quit_code == EVENT_LOOP_RUNNING)
    {
      sfi_glue_gc_run ();
      GDK_THREADS_LEAVE ();
      g_main_iteration (TRUE);
      GDK_THREADS_ENTER ();
    }
  // return exit status and reset state to allow restarts
  const int quit_code = event_loop_quit_code;
  event_loop_quit_code = EVENT_LOOP_RUNNING;
  return quit_code;
}

void
event_loop_quit (uint8 exit_code)
{
  if (event_loop_quit_code == EVENT_LOOP_RUNNING)
    event_loop_quit_code = exit_code;
}

struct LambdaHandler {
  std::function<bool()> lambda;
  uint id = 0;
};

static std::vector<LambdaHandler> frame_handlers;
static size_t frame_handler_cur = 0, frame_handler_max = 0;

static void
remove_handler (uint handler_id, bool warn_missing)
{
  for (size_t i = 0; i < frame_handlers.size(); i++)
    if (frame_handlers[i].id == handler_id)
      {
        frame_handlers.erase (frame_handlers.begin() + i);
        if (i < frame_handler_cur)
          frame_handler_cur--;
        frame_handler_max--;
        return;
      }
  Bse::warning  ("%s: invalid frame handler id: %u", __func__, handler_id);
}

static int
call_frame_handlers (void*)
{
  frame_handler_max = frame_handlers.size(); // store end mark to ignore new handlers during this run
  for (frame_handler_cur = 0; frame_handler_cur < frame_handler_max; frame_handler_cur++)
    {
      const uint current_id = frame_handlers[frame_handler_cur].id;
      // the next two calls may alter frame_handler_cur and frame_handler_max
      if (!frame_handlers[frame_handler_cur].lambda())
        remove_handler (current_id, false);
    }
  return true;
}

void
remove_handler (uint *handler_id)
{
  assert_return (handler_id != NULL);
  if (*handler_id)
    {
      remove_handler (*handler_id, true);
      *handler_id = 0;
    }
}

static void
reinstall_frame_handler()
{
  static uint timeout_id = 0;
  const bool engine_active = bse_server.engine_active();
  if (engine_active && !timeout_id)
    timeout_id = g_timeout_add_full (GTK_PRIORITY_REDRAW, 16, call_frame_handlers, NULL, NULL);
  else if (!engine_active && timeout_id)
    {
      g_source_remove (timeout_id);
      timeout_id = 0;
    }
}

static uint
initialize_frame_handlers()
{
  bse_server.on ("enginechange", [] (const Aida::Event&) { reinstall_frame_handler(); });
  reinstall_frame_handler();
  return 0x200000; // first handler id
}

uint
add_frame_handler (const std::function<bool()> &func)
{
  static uint frame_handler_id = initialize_frame_handlers();
  LambdaHandler lh { func, frame_handler_id++ };
  frame_handlers.push_back (lh); // pushes size() beyond frame_handler_max
  return lh.id;
}

uint
add_frame_handler (const std::function<void()> &func)
{
  std::function<bool()> lambda = [func] () {
    func();
    return true;
  };
  return add_frame_handler (lambda);
}

} // Bst

/* --- variables --- */
/** Global handle to communicate with the remote BSE server instance.
 * Initialize with *new in order to avoid calls to ~ServerS from atexit() handlers,
 * since the dtor would make remote calls and around atexit time the remote end
 * has been shutdown.
 */
Bse::ServerS &bse_server = *new Bse::ServerS(); // avoid ~ServerS from atexit() handlers

static GtkIconFactory *stock_icon_factory = NULL;

/* --- functions --- */
void
_bst_init_utils (void)
{
  assert_return (stock_icon_factory == NULL);
  stock_icon_factory = gtk_icon_factory_new ();
  gtk_icon_factory_add_default (stock_icon_factory);

  /* initialize stock icons (included above) */
  {
    /* generated stock icons */
#include "beast-gtk/icons/bst-stock-gen.cc"

    gxk_stock_register_icons (G_N_ELEMENTS (stock_icons), stock_icons);
  }

  /* initialize stock actions */
  {
    static const GxkStockItem stock_items[] = {
      { BST_STOCK_CLONE,                "_Clone",       GTK_STOCK_COPY,                 },
      { BST_STOCK_DISMISS,              "_Dismiss",     GTK_STOCK_CLOSE,                },
      { BST_STOCK_DEFAULT_REVERT,       "_Defaults",    GTK_STOCK_UNDO,                 },
      { BST_STOCK_LOAD,                 "_Load",        NULL,                           },
      { BST_STOCK_OVERWRITE,            "_Overwrite",   GTK_STOCK_SAVE,                 },
      { BST_STOCK_REVERT,               "_Revert",      GTK_STOCK_UNDO,                 },
    };
    gxk_stock_register_items (G_N_ELEMENTS (stock_items), stock_items);
  }
}

void
_bst_init_radgets (void)
{
  gxk_radget_define_widget_type (BST_TYPE_TRACK_VIEW);
  gxk_radget_define_widget_type (BST_TYPE_HGROW_BAR);
  gxk_radget_define_widget_type (BST_TYPE_VGROW_BAR);
  gxk_radget_define_widget_type (BST_TYPE_WAVE_VIEW);
  gxk_radget_define_widget_type (BST_TYPE_SOUND_FONT_VIEW);
  gxk_radget_define_widget_type (BST_TYPE_PART_VIEW);
  gxk_radget_define_widget_type (BST_TYPE_BUS_EDITOR);
  gxk_radget_define_widget_type (BST_TYPE_BUS_MIXER);
  gxk_radget_define_widget_type (BST_TYPE_BUS_VIEW);
  gxk_radget_define_widget_type (BST_TYPE_PIANO_ROLL);
  gxk_radget_define_widget_type (BST_TYPE_EVENT_ROLL);
  gxk_radget_define_widget_type (BST_TYPE_DB_BEAM);
  gxk_radget_define_widget_type (BST_TYPE_DB_LABELING);
  gxk_radget_define_widget_type (BST_TYPE_DB_METER);
  gxk_radget_define_widget_type (BST_TYPE_SCROLLGRAPH);
  gxk_radget_define_widget_type (BST_TYPE_PATTERN_VIEW);
  gxk_radget_define_widget_type (BST_TYPE_ZOOMED_WINDOW);
  Bse::Blob blob;
  blob = Bse::Blob ("res:gxk/radgets-standard.xml");
  gxk_radget_parse_text ("beast", blob.data(), blob.size(), NULL, NULL);
  blob = Bse::Blob ("res:gxk/radgets-beast.xml");
  gxk_radget_parse_text ("beast", blob.data(), blob.size(), NULL, NULL);
}

GtkWidget*
bst_stock_button (const gchar *stock_id)
{
  GtkWidget *w = gtk_button_new_from_stock (stock_id);
  gtk_widget_show_all (w);
  return w;
}

GtkWidget*
bst_stock_dbutton (const gchar *stock_id)
{
  GtkWidget *w = bst_stock_button (stock_id);
  g_object_set (w, "can-default", TRUE, NULL);
  return w;
}

GtkWidget*
bst_stock_icon_button (const gchar *stock_id)
{
  GtkWidget *w = (GtkWidget*) g_object_new (GTK_TYPE_BUTTON,
                               "visible", TRUE,
                               "child", gtk_image_new_from_stock (stock_id, GXK_ICON_SIZE_BUTTON),
                               "can-focus", FALSE,
                               NULL);
  gtk_widget_show_all (w);
  return w;
}

void
bst_stock_register_icon (const String &stock_id, uint bytes_per_pixel, uint width, uint height, uint rowstride, const uint8 *pixels)
{
  assert_return (bytes_per_pixel == 3 || bytes_per_pixel == 4);
  assert_return (width > 0 && height > 0 && rowstride >= width * bytes_per_pixel);

  if (!gtk_icon_factory_lookup (stock_icon_factory, stock_id.c_str()))
    {
      GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data ((guchar*) g_memdup (pixels, rowstride * height),
                                                    GDK_COLORSPACE_RGB, bytes_per_pixel == 4,
                                                    8, width, height,
                                                    width * bytes_per_pixel,
                                                    (GdkPixbufDestroyNotify) g_free, NULL);
      GtkIconSet *iset = gtk_icon_set_new_from_pixbuf (pixbuf);
      g_object_unref (pixbuf);
      gtk_icon_factory_add (stock_icon_factory, stock_id.c_str(), iset);
      gtk_icon_set_unref (iset);
    }
}

/* --- beast/bse specific extensions --- */
void
bst_status_set_error (Bse::Error error, const std::string &message)
{
  if (error != 0)
    gxk_status_set (GXK_STATUS_ERROR, message.c_str(), Bse::error_blurb (error));
  else
    gxk_status_set (GXK_STATUS_DONE, message.c_str(), NULL);
}

void
bst_gui_error_bell (gpointer widget)
{
  assert_return (GTK_IS_WIDGET (widget));

  if (GTK_WIDGET_DRAWABLE (widget) && BST_GUI_ENABLE_ERROR_BELL)
    {
#if GTK_CHECK_VERSION (2, 12, 0)
      gdk_window_beep (GTK_WIDGET (widget)->window);
#else
      gdk_beep();
#endif
    }
}

void
bst_window_sync_title_to_proxy (gpointer     window,
                                SfiProxy     proxy,
                                const gchar *title_format)
{
  assert_return (GTK_IS_WINDOW (window));
  if (proxy)
    {
      assert_return (BSE_IS_ITEM (proxy));
      assert_return (title_format != NULL);
    }
  Bse::ItemH item = Bse::ItemH::down_cast (bse_server.from_proxy (proxy));
  if (item)
    {
      Bse::ItemS *itemp = new Bse::ItemS (item);
      const char *const p = title_format ? strstr (title_format, "%s") : NULL;
      const std::string t1 = p ? std::string (title_format, p - title_format) : title_format;
      const std::string t2 = p ? p + 2 : "";
      auto setter = [t1, t2, itemp, window] () {
        const String name = itemp->get_name_or_type();
        const std::string title = t1 + name + t2;
        g_object_set (window, "title", title.c_str(), NULL);
      };
      itemp->on ("notify:uname", [setter] (const Aida::Event&) { setter(); });
      g_object_set_data_full ((GObject*) window, "bst-title-sync", itemp, [] (void *data) {
          Bse::ItemS *itemp = (Bse::ItemS*) data;
          delete itemp; // deletes notify:uname handler
        });
      setter();
    }
  else
    {
      g_object_set_data ((GObject*) window, "bst-title-sync", NULL);
      g_object_set (window, "title", title_format, NULL);
    }
}

typedef struct {
  gboolean       (*handler) (gpointer data);
  gpointer         data;
  void           (*free_func) (gpointer data);
} BackgroundHandler;

static SfiRing *background_handlers1 = NULL;
static SfiRing *background_handlers2 = NULL;

static gboolean
bst_background_handlers_timeout (gpointer timeout_data)
{
  GDK_THREADS_ENTER();
  if (background_handlers1 || background_handlers2)
    {
      gxk_status_set (GXK_STATUS_PROGRESS, _("Updating View"), NULL);
      BackgroundHandler *bgh = (BackgroundHandler*) sfi_ring_pop_head (&background_handlers1);
      gint prio = 1;
      if (!bgh)
        {
          prio = 2;
          bgh = (BackgroundHandler*) sfi_ring_pop_head (&background_handlers2);
        }
      if (bgh->handler (bgh->data))
        {
          if (prio == 1)
            background_handlers1 = sfi_ring_append (background_handlers1, bgh);
          else
            background_handlers2 = sfi_ring_append (background_handlers2, bgh);
        }
      else
        {
          if (bgh->free_func)
            bgh->free_func (bgh->data);
          g_free (bgh);
        }
      if (background_handlers1 || background_handlers2)
        gxk_status_set (GXK_STATUS_PROGRESS, _("Updating View"), NULL);
    }
  if (!background_handlers1 && !background_handlers2)
    gxk_status_set (100, _("Updating View"), NULL); /* done */
  GDK_THREADS_LEAVE();
  /* re-queue instead of returning TRUE to start a new delay cycle */
  if (background_handlers1 || background_handlers2)
    g_timeout_add_full (G_PRIORITY_LOW - 100,
                        30, /* milliseconds */
                        bst_background_handlers_timeout, NULL, NULL);
  return FALSE;
}

static void
bst_background_handler_add (gboolean       (*handler) (gpointer data),
                            gpointer         data,
                            void           (*free_func) (gpointer data),
                            gint             prio)
{
  assert_return (handler != NULL);
  BackgroundHandler *bgh = g_new0 (BackgroundHandler, 1);
  bgh->handler = handler;
  bgh->data = data;
  bgh->free_func = free_func;
  if (!background_handlers1 && !background_handlers2)
    g_timeout_add_full (G_PRIORITY_LOW - 100,
                        20, /* milliseconds */
                        bst_background_handlers_timeout, NULL, NULL);
  if (prio == 1)
    background_handlers1 = sfi_ring_append (background_handlers1, bgh);
  else
    background_handlers2 = sfi_ring_append (background_handlers2, bgh);
}

void
bst_background_handler1_add (gboolean       (*handler) (gpointer data),
                             gpointer         data,
                             void           (*free_func) (gpointer data))
{
  bst_background_handler_add (handler, data, free_func, 1);
}

void
bst_background_handler2_add (gboolean       (*handler) (gpointer data),
                             gpointer         data,
                             void           (*free_func) (gpointer data))
{
  bst_background_handler_add (handler, data, free_func, 2);
}

/* --- packing utilities --- */
#define SPACING 3
static void
bst_util_pack (GtkWidget   *widget,
               const gchar *location,
               guint        spacing,
               va_list      args)
{
  GtkBox *box = GTK_BOX (widget);
  while (location)
    {
      gchar *t, **toks = g_strsplit (location, ":", -1);
      guint border = 0, padding = 0, i = 0;
      gboolean fill = FALSE, expand = FALSE, start = TRUE;
      GtkWidget *child;
      t = toks[i++];
      if (t && t[0] >= '0' && t[0] <= '9')
        {
          border = g_ascii_strtoull (t, NULL, 10);
          t = toks[i++];
        }
      if (t && t[0] == '+')
        {
          expand = TRUE;
          t = toks[i++];
        }
      if (t && t[0] == '*')
        {
          expand = fill = TRUE;
          t = toks[i++];
        }
      if (t && t[0] == 'H')
        {
          gtk_box_set_homogeneous (box, TRUE);
          expand = fill = TRUE;
          t = toks[i++];
        }
      if (t && t[0] == 's')
        {
          start = TRUE;
          t = toks[i++];
        }
      if (t && t[0] == 'e')
        {
          start = FALSE;
          t = toks[i++];
        }
      if (t && t[0] == 'p')
        {
          padding = g_ascii_strtoull (t + 1, NULL, 10);
          t = toks[i++];
        }
      g_strfreev (toks);
      child = va_arg (args, GtkWidget*);
      if (child)
        {
          if (border)
            child = (GtkWidget*) g_object_new (GTK_TYPE_ALIGNMENT,
                                               "child", child,
                                               "border_width", border * spacing,
                                               NULL);
          if (start)
            gtk_box_pack_start (box, child, expand, fill, padding);
          else
            gtk_box_pack_end (box, child, expand, fill, padding);
        }
      location = va_arg (args, gchar*);
    }
}

GtkWidget*
bst_vpack (const gchar *first_location,
           ...)
{
  va_list args;
  GtkWidget *box = (GtkWidget*) g_object_new (GTK_TYPE_VBOX,
                                 "spacing", SPACING,
                                 NULL);
  va_start (args, first_location);
  bst_util_pack (box, first_location, SPACING, args);
  va_end (args);
  gtk_widget_show_all (box);
  return box;
}

GtkWidget*
bst_hpack (const gchar *first_location,
           ...)
{
  va_list args;
  GtkWidget *box = (GtkWidget*) g_object_new (GTK_TYPE_HBOX,
                                 "spacing", SPACING,
                                 NULL);
  va_start (args, first_location);
  bst_util_pack (box, first_location, SPACING, args);
  va_end (args);
  gtk_widget_show_all (box);
  return box;
}

GtkWidget*
bst_vpack0 (const gchar *first_location,
            ...)
{
  va_list args;
  GtkWidget *box = (GtkWidget*) g_object_new (GTK_TYPE_VBOX,
                                 "spacing", 0,
                                 NULL);
  va_start (args, first_location);
  bst_util_pack (box, first_location, SPACING, args);
  va_end (args);
  gtk_widget_show_all (box);
  return box;
}

GtkWidget*
bst_hpack0 (const gchar *first_location,
            ...)
{
  va_list args;
  GtkWidget *box = (GtkWidget*) g_object_new (GTK_TYPE_HBOX,
                                 "spacing", 0,
                                 NULL);
  va_start (args, first_location);
  bst_util_pack (box, first_location, SPACING, args);
  va_end (args);
  gtk_widget_show_all (box);
  return box;
}

void
bst_action_list_add_cat (GxkActionList *alist, const Bse::Category &cat, uint skip_levels, const char *stock_fallback,
                         GxkActionCheck acheck, GxkActionExec aexec, void *user_data)
{
  const char *p;
  String stock_id;
  if (cat.icon.pixels.size())
    {
      const Bse::Icon &icon = cat.icon;
      assert_return (icon.width * size_t (icon.height) == icon.pixels.size());
      bst_stock_register_icon (cat.category, 4,
                               icon.width, icon.height,
                               icon.width * 4,
                               (uint8*) &icon.pixels[0]);
      stock_id = cat.category;
    }
  else
    stock_id = stock_fallback;

  p = cat.category[0] == '/' ? cat.category.c_str() + 1 : cat.category.c_str();
  while (skip_levels--)
    {
      const char *d = strchr (p, '/');
      p = d ? d + 1 : p;
    }

  gxk_action_list_add_translated (alist, NULL, p, NULL,
                                  gxk_factory_path_get_leaf (cat.category.c_str()),
                                  g_quark_from_string (cat.category.c_str()), stock_id.c_str(),
                                  acheck, aexec, user_data);
}

void
bst_action_list_add_module (GxkActionList *alist, const Bse::AuxData &ad, const Bse::Icon &icon, const char *stock_fallback,
                            GxkActionCheck acheck, GxkActionExec aexec, gpointer user_data)
{
  const char *stock_id;
  if (icon.width && icon.height)
    {
      assert_return (icon.width * icon.height == int (icon.pixels.size()));
      bst_stock_register_icon (ad.entity.c_str(), 4, icon.width, icon.height, icon.width * 4, (const uint8*) icon.pixels.data());
      stock_id = ad.entity.c_str();
    }
  else
    stock_id = stock_fallback;

  String title = Bse::string_vector_find_value (ad.attributes, "title=");
  if (title.empty())
    title = ad.entity;
  Bse::StringVector tags = Bse::string_split_any (Bse::string_vector_find_value (ad.attributes, "tags="), ";:");
  if (tags.size())
    {
      tags.push_back (title);
      title = Bse::string_join ("/", tags);
    }
  gxk_action_list_add_translated (alist, NULL, title.c_str(), NULL,
                                  ad.entity.c_str(), // tooltip
                                  g_quark_from_string (ad.entity.c_str()), stock_id,
                                  acheck, aexec, user_data);
}


/* --- field mask --- */
static GQuark gmask_quark = 0;
typedef struct {
  GtkWidget   *parent;
  GtkWidget   *prompt;
  GtkWidget   *aux1;
  GtkWidget   *aux2;            /* auto-expand */
  GtkWidget   *aux3;
  GtkWidget   *action;
  gchar       *tip;
  guint        column : 16;
  guint        gpack : 8;
} GMask;
#define GMASK_GET(o)    ((GMask*) g_object_get_qdata (G_OBJECT (o), gmask_quark))

static void
gmask_destroy (gpointer data)
{
  GMask *gmask = (GMask*) data;

  if (gmask->parent)
    g_object_unref (gmask->parent);
  if (gmask->prompt)
    g_object_unref (gmask->prompt);
  if (gmask->aux1)
    g_object_unref (gmask->aux1);
  if (gmask->aux2)
    g_object_unref (gmask->aux2);
  if (gmask->aux3)
    g_object_unref (gmask->aux3);
  g_free (gmask->tip);
  g_free (gmask);
}

static gpointer
gmask_form (GtkWidget   *parent,
            GtkWidget   *action,
            BstGMaskPack gpack)
{
  GMask *gmask;

  assert_return (GTK_IS_TABLE (parent), NULL);
  assert_return (GTK_IS_WIDGET (action), NULL);

  if (!gmask_quark)
    gmask_quark = g_quark_from_static_string ("GMask");

  gmask = GMASK_GET (action);
  assert_return (gmask == NULL, NULL);

  gmask = g_new0 (GMask, 1);
  g_object_set_qdata_full (G_OBJECT (action), gmask_quark, gmask, gmask_destroy);
  gmask->parent = (GtkWidget*) g_object_ref (parent);
  gtk_object_sink (GTK_OBJECT (parent));
  gmask->action = action;
  gmask->gpack = gpack;

  return action;
}

/**
 * @param border_width	   Border width of this GUI mask
 * @param dislodge_columns Provide expandable space between columns
 * @return		   GUI field mask container
 *
 * Create a container capable to hold GUI field masks.
 * This is the container to be passed into bst_gmask_form().
 * In case multiple field mask columns are packed into the
 * container (by using bst_gmask_set_column() on the filed
 * masks), @a dislodge_columns specifies whether the field
 * mask columns are to be closely aligned.
 */
GtkWidget*
bst_gmask_container_create (guint    border_width,
                            gboolean dislodge_columns)
{
  GtkWidget *container = gtk_widget_new (GTK_TYPE_TABLE,
                                         "visible", TRUE,
                                         "homogeneous", FALSE,
                                         "n_columns", 2,
                                         "border_width", border_width,
                                         NULL);
  if (dislodge_columns)
    g_object_set_data (G_OBJECT (container), "GMask-dislodge", GUINT_TO_POINTER (TRUE));

  return container;
}

/**
 * @param gmask_container    container created with bst_gmask_container_create()
 * @param action             valid GtkWidget
 * @param gpack	BstGMaskPack packing type
 * @return		     a new GUI field mask
 *
 * Create a new GUI field mask with @a action as action widget.
 * Each GUI field mask consists of an action widget which may
 * be neighboured by pre and post action widgets, the action
 * widget is usually something like a GtkEntry input widget.
 * Also, most field masks have a prompt widget, usually a
 * GtkLabel, labeling the field mask with a name.
 * Optionally, up to three auxillary widgets are supported
 * per field mask, layed out between the prompt and the
 * action widgets.
 * The second auxillary widget will expand if additional
 * space is available. Other layout details are configured
 * through the @a gpack packing type:
 * @li @c BST_GMASK_FIT - the action widget is not expanded,
 * @li @c BST_GMASK_INTERLEAVE - allow the action widget to expand across auxillary
 * columns if it requests that much space,
 * @li @c BST_GMASK_BIG - force expansion of the action widget across all possible
 * columns up to the prompt,
 * @li @c BST_GMASK_CENTER - center the action widget within space across all possible
 * columns up to the prompt.
 * @li @c BST_GMASK_MULTI_SPAN - span aux2 widget across multiple gmask columns.
 */
BstGMask*
bst_gmask_form (GtkWidget   *gmask_container,
                GtkWidget   *action,
                BstGMaskPack gpack)
{
  return (BstGMask*) gmask_form (gmask_container, action, gpack);
}

/**
 * @param mask	   valid BstGMask
 * @param tip_text tooltip text
 *
 * Set the tooltip text of this GUI field @a mask.
 */
void
bst_gmask_set_tip (BstGMask    *mask,
                   const gchar *tip_text)
{
  GMask *gmask;

  assert_return (GTK_IS_WIDGET (mask));
  gmask = GMASK_GET (mask);
  assert_return (gmask != NULL);

  g_free (gmask->tip);
  gmask->tip = g_strdup (tip_text);
}

/**
 * @param mask	 valid BstGMask
 * @param widget valid GtkWidget
 *
 * Set the prompt widget of this GUI field @a mask.
 */
void
bst_gmask_set_prompt (BstGMask *mask,
                      gpointer  widget)
{
  GMask *gmask;

  assert_return (GTK_IS_WIDGET (mask));
  gmask = GMASK_GET (mask);
  assert_return (gmask != NULL);
  assert_return (GTK_IS_WIDGET (widget));

  if (gmask->prompt)
    g_object_unref (gmask->prompt);
  gmask->prompt = (GtkWidget*) g_object_ref (widget);
  gtk_object_sink (GTK_OBJECT (widget));
}

/**
 * @param mask	 valid BstGMask
 * @param widget valid GtkWidget
 *
 * Set the first auxillary widget of this GUI field @a mask.
 */
void
bst_gmask_set_aux1 (BstGMask *mask,
                    gpointer  widget)
{
  GMask *gmask;

  assert_return (GTK_IS_WIDGET (mask));
  gmask = GMASK_GET (mask);
  assert_return (gmask != NULL);
  assert_return (GTK_IS_WIDGET (widget));

  if (gmask->aux1)
    g_object_unref (gmask->aux1);
  gmask->aux1 = (GtkWidget*) g_object_ref (widget);
  gtk_object_sink (GTK_OBJECT (widget));
}

/**
 * @param mask	 valid BstGMask
 * @param widget valid GtkWidget
 *
 * Set the second auxillary widget of this GUI field @a mask.
 * In contrast to the first and third auxillary widget, this
 * one is expanded if extra space is available.
 */
void
bst_gmask_set_aux2 (BstGMask *mask,
                    gpointer  widget)
{
  GMask *gmask;

  assert_return (GTK_IS_WIDGET (mask));
  gmask = GMASK_GET (mask);
  assert_return (gmask != NULL);
  assert_return (GTK_IS_WIDGET (widget));

  if (gmask->aux2)
    g_object_unref (gmask->aux2);
  gmask->aux2 = (GtkWidget*) g_object_ref (widget);
  gtk_object_sink (GTK_OBJECT (widget));
}

/**
 * @param mask	 valid BstGMask
 * @param widget valid GtkWidget
 *
 * Set the third auxillary widget of this GUI field @a mask.
 */
void
bst_gmask_set_aux3 (BstGMask *mask,
                    gpointer  widget)
{
  GMask *gmask;

  assert_return (GTK_IS_WIDGET (mask));
  gmask = GMASK_GET (mask);
  assert_return (gmask != NULL);
  assert_return (GTK_IS_WIDGET (widget));

  if (gmask->aux3)
    g_object_unref (gmask->aux3);
  gmask->aux3 = (GtkWidget*) g_object_ref (widget);
  gtk_object_sink (GTK_OBJECT (widget));
}

/**
 * @param mask	 valid BstGMask
 * @param column column number
 *
 * Set the field mask column. By default all field masks are
 * packed into column 0, so that only vertical packing occours.
 */
void
bst_gmask_set_column (BstGMask *mask,
                      guint     column)
{
  GMask *gmask;

  assert_return (GTK_IS_WIDGET (mask));
  gmask = GMASK_GET (mask);
  assert_return (gmask != NULL);

  gmask->column = column;
}

/**
 * @param mask	valid BstGMask
 * @return	the requested GtkWidget or NULL
 *
 * Retrieve the prompt widget of this GUI field @a mask.
 */
GtkWidget*
bst_gmask_get_prompt (BstGMask *mask)
{
  GMask *gmask;

  assert_return (GTK_IS_WIDGET (mask), NULL);
  gmask = GMASK_GET (mask);
  assert_return (gmask != NULL, NULL);

  return gmask->prompt;
}

/**
 * @param mask	valid BstGMask
 * @return	the requested GtkWidget or NULL
 *
 * Retrieve the first auxillary widget of this GUI field @a mask.
 */
GtkWidget*
bst_gmask_get_aux1 (BstGMask *mask)
{
  GMask *gmask;

  assert_return (GTK_IS_WIDGET (mask), NULL);
  gmask = GMASK_GET (mask);
  assert_return (gmask != NULL, NULL);

  return gmask->aux1;
}

/**
 * @param mask	valid BstGMask
 * @return	the requested GtkWidget or NULL
 *
 * Retrieve the second auxillary widget of this GUI field @a mask.
 */
GtkWidget*
bst_gmask_get_aux2 (BstGMask *mask)
{
  GMask *gmask;

  assert_return (GTK_IS_WIDGET (mask), NULL);
  gmask = GMASK_GET (mask);
  assert_return (gmask != NULL, NULL);

  return gmask->aux2;
}

/**
 * @param mask	valid BstGMask
 * @return	the requested GtkWidget or NULL
 *
 * Retrieve the third auxillary widget of this GUI field @a mask.
 */
GtkWidget*
bst_gmask_get_aux3 (BstGMask *mask)
{
  GMask *gmask;

  assert_return (GTK_IS_WIDGET (mask), NULL);
  gmask = GMASK_GET (mask);
  assert_return (gmask != NULL, NULL);

  return gmask->aux3;
}

/**
 * @param mask	valid BstGMask
 * @return	the requested GtkWidget or NULL
 *
 * Retrieve the action widget of this GUI field @a mask.
 */
GtkWidget*
bst_gmask_get_action (BstGMask *mask)
{
  GMask *gmask;

  assert_return (GTK_IS_WIDGET (mask), NULL);
  gmask = GMASK_GET (mask);
  assert_return (gmask != NULL, NULL);

  return gmask->action;
}

/**
 * @param mask	valid BstGMask
 * @param func	foreach function as: void func(GtkWidget*, gpointer data);
 * @param data	data passed in to @a func
 *
 * Invoke @a func() with each of the widgets set for this
 * field mask.
 */
void
bst_gmask_foreach (BstGMask *mask,
                   gpointer  func,
                   gpointer  data)
{
  GMask *gmask;
  GtkCallback callback = GtkCallback (func);

  assert_return (GTK_IS_WIDGET (mask));
  gmask = GMASK_GET (mask);
  assert_return (gmask != NULL);
  assert_return (func != NULL);

  if (gmask->prompt)
    callback (gmask->prompt, data);
  if (gmask->aux1)
    callback (gmask->aux1, data);
  if (gmask->aux2)
    callback (gmask->aux2, data);
  if (gmask->aux3)
    callback (gmask->aux3, data);
  if (gmask->action)
    callback (gmask->action, data);
}

static GtkWidget*
get_toplevel_and_set_tip (GtkWidget   *widget,
                          const gchar *tip)
{
  GtkWidget *last;

  if (!widget)
    return NULL;
  else if (!tip)
    return gtk_widget_get_toplevel (widget);
  do
    {
      if (!GTK_WIDGET_NO_WINDOW (widget))
        {
          gxk_widget_set_tooltip (widget, tip);
          return gtk_widget_get_toplevel (widget);
        }
      last = widget;
      widget = last->parent;
    }
  while (widget);
  /* need to create a tooltips sensitive parent */
  widget = gtk_widget_new (GTK_TYPE_EVENT_BOX,
                           "visible", TRUE,
                           "child", last,
                           NULL);
  gxk_widget_set_tooltip (widget, tip);
  return widget;
}

static guint
table_max_bottom_row (GtkTable *table,
                      guint     left_col,
                      guint     right_col)
{
  guint max_bottom = 0;
  GList *list;

  for (list = table->children; list; list = list->next)
    {
      GtkTableChild *child = (GtkTableChild*) list->data;

      if (child->left_attach < right_col && child->right_attach > left_col)
        max_bottom = MAX (max_bottom, child->bottom_attach);
    }
  return max_bottom;
}

/**
 * @param mask	valid BstGMask
 *
 * After the GUI field mask is fully configured, by setting
 * all associated widgets on it, column tooltip text, etc.,
 * this function actually packs it into its container. The
 * field mask setters shouldn't be used after this point.
 */
void
bst_gmask_pack (BstGMask *mask)
{
  GtkWidget *prompt, *aux1, *aux2, *aux3, *action;
  GtkTable *table;
  gboolean dummy_aux2 = FALSE;
  guint row, n, c, dislodge_columns;
  GMask *gmask;

  assert_return (GTK_IS_WIDGET (mask));
  gmask = GMASK_GET (mask);
  assert_return (gmask != NULL);

  /* GUI mask layout:
   * row: |Prompt|Aux1| Aux2 |Aux3| PreAction#Action#PostAction|
   * FILL: allocate all possible (Pre/Post)Action space to the action widget
   * INTERLEAVE: allow the action widget to facilitate unused Aux2/Aux3 space
   * BIG: allocate maximum (left extendeded) possible space to Action
   * Aux2 expands automatically
   */

  /* retrieve children and set tips */
  prompt = get_toplevel_and_set_tip (gmask->prompt, gmask->tip);
  aux1 = get_toplevel_and_set_tip (gmask->aux1, gmask->tip);
  aux2 = get_toplevel_and_set_tip (gmask->aux2, gmask->tip);
  aux3 = get_toplevel_and_set_tip (gmask->aux3, gmask->tip);
  action = get_toplevel_and_set_tip (gmask->action, gmask->tip);
  dislodge_columns = g_object_get_data (G_OBJECT (gmask->parent), "GMask-dislodge") != NULL;
  table = GTK_TABLE (gmask->parent);

  /* ensure expansion happens outside of columns */
  if (dislodge_columns)
    {
      gchar *dummy_name = g_strdup_format ("GMask-dummy-dislodge-%u", MAX (gmask->column, 1) - 1);
      GtkWidget *dislodge = (GtkWidget*) g_object_get_data (G_OBJECT (table), dummy_name);

      if (!dislodge)
        {
          dislodge = (GtkWidget*) g_object_new (GTK_TYPE_ALIGNMENT, "visible", TRUE, NULL);
          g_object_set_data_full (G_OBJECT (table), dummy_name, g_object_ref (dislodge), g_object_unref);
          c = MAX (gmask->column, 1) * 6;
          gtk_table_attach (table, dislodge, c - 1, c, 0, 1, GTK_EXPAND, GtkAttachOptions (0), 0, 0);
        }
      g_free (dummy_name);
    }

  /* pack gmask children, options: GTK_EXPAND, GTK_SHRINK, GTK_FILL */
  gboolean span_multi_columns = aux2 && gmask->gpack == BST_GMASK_MULTI_SPAN;
  c = span_multi_columns ? 0 : 6 * gmask->column;
  row = table_max_bottom_row (table, c, 6 * gmask->column + 6);
  if (prompt)
    {
      gtk_table_attach (table, prompt, c, c + 1, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
      gtk_table_set_col_spacing (table, c, 2); /* seperate prompt from rest */
    }
  c++;
  if (aux1)
    {
      gtk_table_attach (table, aux1, c, c + 1, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
      gtk_table_set_col_spacing (table, c, 3); /* aux1 spacing */
    }
  c++;
  if (!aux2 && !dislodge_columns)
    {
      char *dummy_name = g_strdup_format ("GMask-dummy-aux2-%u", gmask->column);
      aux2 = (GtkWidget*) g_object_get_data (G_OBJECT (table), dummy_name);

      /* need to have at least 1 (dummy) aux2-child per table column to eat up
       * expanding space in this column if !dislodge_columns
       */
      if (!aux2)
        {
          aux2 = gtk_widget_new (GTK_TYPE_ALIGNMENT, "visible", TRUE, NULL);
          g_object_set_data_full (G_OBJECT (table), dummy_name, g_object_ref (aux2), g_object_unref);
        }
      else
        aux2 = NULL;
      g_free (dummy_name);
      dummy_aux2 = TRUE;
    }
  if (aux2)
    {
      guint left_col = c;
      if (span_multi_columns)
        c += 6 * gmask->column;
      gtk_table_attach (table, aux2,
                        left_col, c + 1,
                        row, row + 1, GTK_EXPAND | GTK_FILL, GtkAttachOptions (0), 0, 0);
      if (dummy_aux2)
        aux2 = NULL;
      if (aux2)
        gtk_table_set_col_spacing (table, c, 3); /* aux2 spacing */
    }
  c++;
  if (aux3)
    {
      gtk_table_attach (table, aux3, c, c + 1, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
      gtk_table_set_col_spacing (table, c, 3); /* aux3 spacing */
    }
  c++;
  n = c;
  if (gmask->gpack == BST_GMASK_BIG || gmask->gpack == BST_GMASK_CENTER ||
      gmask->gpack == BST_GMASK_INTERLEAVE)     /* extend action to the left when possible */
    {
      if (!aux3)
        {
          n--;
          if (!aux2)
            {
              n--;
              if (!aux1 && (gmask->gpack == BST_GMASK_BIG ||
                            gmask->gpack == BST_GMASK_CENTER))
                {
                  n--;
                  if (!prompt)
                    n--;
                }
            }
        }
    }
  if (gmask->gpack == BST_GMASK_FIT ||
      gmask->gpack == BST_GMASK_INTERLEAVE) /* align to right without expansion */
    action = gtk_widget_new (GTK_TYPE_ALIGNMENT,
                             "visible", TRUE,
                             "child", action,
                             "xalign", 1.0,
                             "xscale", 0.0,
                             "yscale", 0.0,
                             NULL);
  else if (gmask->gpack == BST_GMASK_CENTER)
    action = gtk_widget_new (GTK_TYPE_ALIGNMENT,
                             "visible", TRUE,
                             "child", action,
                             "xalign", 0.5,
                             "yalign", 0.5,
                             "xscale", 0.0,
                             "yscale", 0.0,
                             NULL);
  gtk_table_attach (table, action,
                    n, c + 1, row, row + 1,
                    GTK_SHRINK | GTK_FILL,
                    GTK_FILL,
                    0, 0);
  gtk_table_set_col_spacing (table, c - 1, 3); /* seperate action from rest */
  c = 6 * gmask->column;
  if (c)
    gtk_table_set_col_spacing (table, c - 1, 5); /* spacing between gmask columns */
}

/**
 * @param gmask_container container created with bst_gmask_container_create()
 * @param column	  column number for bst_gmask_set_column()
 * @param prompt	  valid GtkWidget for bst_gmask_set_prompt()
 * @param action	  valid GtkWidget as with bst_gmask_form()
 * @param tip_text	  text for bst_gmask_set_tip()
 * @return		  an already packed GUI field mask
 *
 * Shorthand to form a GUI field mask in @a column of type BST_GMASK_INTERLEAVE,
 * with @a prompt and @a tip_text. Note that this function already calls
 * bst_gmask_pack(), so the returned field mask already can't be modified
 * anymore.
 */
BstGMask*
bst_gmask_quick (GtkWidget   *gmask_container,
                 guint        column,
                 const gchar *prompt,
                 gpointer     action,
                 const gchar *tip_text)
{
  BstGMask *mask = bst_gmask_form (gmask_container, (GtkWidget*) action, BST_GMASK_INTERLEAVE);

  if (prompt)
    bst_gmask_set_prompt (mask, g_object_new (GTK_TYPE_LABEL,
                                              "visible", TRUE,
                                              "label", prompt,
                                              NULL));
  if (tip_text)
    bst_gmask_set_tip (mask, tip_text);
  bst_gmask_set_column (mask, column);
  bst_gmask_pack (mask);

  return mask;
}


/* --- named children --- */
static GQuark quark_container_named_children = 0;
typedef struct {
  GData *qdata;
} NChildren;
static void
nchildren_free (gpointer data)
{
  NChildren *children = (NChildren*) data;

  g_datalist_clear (&children->qdata);
  g_free (children);
}
static void
destroy_nchildren (GtkWidget *container)
{
  g_object_set_qdata (G_OBJECT (container), quark_container_named_children, NULL);
}
void
bst_container_set_named_child (GtkWidget *container,
                               GQuark     qname,
                               GtkWidget *child)
{
  NChildren *children;

  assert_return (GTK_IS_CONTAINER (container));
  assert_return (qname > 0);
  assert_return (GTK_IS_WIDGET (child));
  if (child)
    assert_return (gtk_widget_is_ancestor (child, container));

  if (!quark_container_named_children)
    quark_container_named_children = g_quark_from_static_string ("BstContainer-named_children");

  children = (NChildren*) g_object_get_qdata (G_OBJECT (container), quark_container_named_children);
  if (!children)
    {
      children = g_new (NChildren, 1);
      g_datalist_init (&children->qdata);
      g_object_set_qdata_full (G_OBJECT (container), quark_container_named_children, children, nchildren_free);
      g_object_connect (container,
                        "signal::destroy", destroy_nchildren, NULL,
                        NULL);
    }
  g_object_ref (child);
  g_datalist_id_set_data_full (&children->qdata, qname, child, g_object_unref);
}

GtkWidget*
bst_container_get_named_child (GtkWidget *container,
                               GQuark     qname)
{
  NChildren *children;

  assert_return (GTK_IS_CONTAINER (container), NULL);
  assert_return (qname > 0, NULL);

  children = quark_container_named_children ? (NChildren*) g_object_get_qdata (G_OBJECT (container), quark_container_named_children) : NULL;
  if (children)
    {
      GtkWidget *child = (GtkWidget*) g_datalist_id_get_data (&children->qdata, qname);

      if (child && !gtk_widget_is_ancestor (child, container))
        {
          /* got removed meanwhile */
          g_datalist_id_set_data (&children->qdata, qname, NULL);
          child = NULL;
        }
      return child;
    }
  return NULL;
}

GtkWidget*
bst_xpm_view_create (const gchar **xpm,
                     GtkWidget    *colormap_widget)
{
  GtkWidget *pix;
  GdkPixmap *pixmap;
  GdkBitmap *mask;

  assert_return (xpm != NULL, NULL);
  assert_return (GTK_IS_WIDGET (colormap_widget), NULL);

  pixmap = gdk_pixmap_colormap_create_from_xpm_d (NULL, gtk_widget_get_colormap (colormap_widget),
                                                  &mask, NULL, (gchar**) xpm);
  pix = gtk_pixmap_new (pixmap, mask);
  gdk_pixmap_unref (pixmap);
  gdk_pixmap_unref (mask);
  gtk_widget_set (pix,
                  "visible", TRUE,
                  NULL);
  return pix;
}

#include <sfi/sfistore.hh>

gchar*
bst_file_scan_find_key (const gchar *file,
                        const gchar *key,
                        const gchar *value_prefix)
{
  SfiRStore *rstore;

  assert_return (file != NULL, NULL);

  rstore = sfi_rstore_new_open (file);
  if (rstore)
    {
      guint l = value_prefix ? strlen (value_prefix) : 0;
      gchar *name = NULL;
      while (!g_scanner_eof (rstore->scanner))
        {
          if (g_scanner_get_next_token (rstore->scanner) == '(' &&
              g_scanner_peek_next_token (rstore->scanner) == G_TOKEN_IDENTIFIER)
            {
              g_scanner_get_next_token (rstore->scanner);
              if (strcmp (rstore->scanner->value.v_identifier, key) == 0 &&
                  g_scanner_peek_next_token (rstore->scanner) == G_TOKEN_STRING)
                {
                  g_scanner_get_next_token (rstore->scanner);
                  if (!l || strncmp (rstore->scanner->value.v_string, value_prefix, l) == 0)
                    {
                      name = g_strdup (rstore->scanner->value.v_string + l);
                      break;
                    }
                }
            }
        }
      sfi_rstore_destroy (rstore);
      return name;
    }
  else
    return NULL;
}


// == mouse button checks ==
static bool
shift_event (GdkEvent *event)
{
  return (event->button.state & GDK_SHIFT_MASK) != 0;
}

static bool
alt_event (GdkEvent *event)
{
  return (event->button.state & GDK_MOD1_MASK) != 0;
}

static bool
button_event (GdkEvent *event)
{
  return event->type == GDK_BUTTON_PRESS || event->type == GDK_BUTTON_RELEASE;
}

bool
bst_mouse_button_activate (GdkEvent *event)
{
  return button_event (event) && event->button.button == 1;
}

bool
bst_mouse_button_activate1 (GdkEvent *event)
{
  return !shift_event (event) && button_event (event) && event->button.button == 1;
}

bool
bst_mouse_button_activate2 (GdkEvent *event)
{
  return shift_event (event) && button_event (event) && event->button.button == 1;
}

bool
bst_mouse_button_move (GdkEvent *event)
{
  return button_event (event) && (event->button.button == 2 ||
                                  (alt_event (event) && event->button.button == 1));
}

bool
bst_mouse_button_context (GdkEvent *event)
{
  return button_event (event) && event->button.button == 3;
}

// == bstapi_interfaces.cc ==
#include "bstapi_interfaces.cc"      // compile types and bindings generated for bstapi.idl
