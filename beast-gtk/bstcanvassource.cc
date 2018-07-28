// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstcanvassource.hh"
#include "bstparamview.hh"
#include "bstgconfig.hh"
#include <string.h>


/* --- defines --- */
#define	ICON_WIDTH(cs)		((gdouble) 64)
#define	ICON_HEIGHT(cs)		((gdouble) 64)
#define CHANNEL_WIDTH(cs)	((gdouble) 10)
#define CHANNEL_HEIGHT(cs)	((gdouble) ICON_HEIGHT (cs))
#define	ICON_X(cs)		((gdouble) CHANNEL_WIDTH (cs))
#define	ICON_Y(cs)		((gdouble) 0)
#define ICHANNEL_realX(cs)	((gdouble) 0)
#define OCHANNEL_realX(cs)	((gdouble) CHANNEL_WIDTH (cs) + ICON_WIDTH (cs))
#define ICHANNEL_X(cs)		(cs->swap_channels ? OCHANNEL_realX (cs) : ICHANNEL_realX (cs))
#define OCHANNEL_X(cs)		(cs->swap_channels ? ICHANNEL_realX (cs) : OCHANNEL_realX (cs))
#define CHANNEL_EAST(cs,isinp)	(cs->swap_channels ^ !isinp)
#define	BORDER_PAD		((gdouble) 1)
#define ICHANNEL_Y(cs)		((gdouble) 0)
#define OCHANNEL_Y(cs)		((gdouble) 0)
#define	TOTAL_WIDTH(cs)		((gdouble) CHANNEL_WIDTH (cs) + ICON_WIDTH (cs) + CHANNEL_WIDTH (cs))
#define	TOTAL_HEIGHT(cs)	((gdouble) ICON_HEIGHT (cs))
#define TEXT_X(cs)		((gdouble) CHANNEL_WIDTH (cs) + ICON_WIDTH (cs) / 2)    /* for anchor: center */
#define TEXT_Y(cs)		((gdouble) ICON_HEIGHT (cs))                            /* for anchor: north */
#define TEXT_HEIGHT		((gdouble) FONT_HEIGHT + 2)
#define	CHANNEL_FONT		("Sans")
#define	TEXT_FONT		("Serif")
#define	FONT_HEIGHT		((gdouble) BST_GCONFIG (snet_font_size))
#define RGBA_BLACK		(0x000000ff)
#define RGBA_INTERNAL           (0x0000ffff)

/* --- signals --- */
enum
{
  SIGNAL_UPDATE_LINKS,
  SIGNAL_LAST
};
typedef void    (*SignalUpdateLinks)            (BstCanvasSource       *source,
						 gpointer         func_data);


/* --- prototypes --- */
static gboolean bst_canvas_source_child_event	(BstCanvasSource	*csource,
						 GdkEvent               *event,
						 GnomeCanvasItem        *child);
static void     bst_canvas_source_changed       (BstCanvasSource        *csource);
static void     bst_canvas_icon_set             (GnomeCanvasItem *item, Bse::Icon &icon2, const char *module_type);
static void	csource_info_update		(BstCanvasSource	*csource);
static void     bst_canvas_source_build         (BstCanvasSource        *csource);


/* --- static variables --- */
static void *parent_class = NULL;
static uint  csource_signals[SIGNAL_LAST] = { 0 };


/* --- functions --- */
G_DEFINE_TYPE (BstCanvasSource, bst_canvas_source, GNOME_TYPE_CANVAS_GROUP);

static void
bst_canvas_source_init (BstCanvasSource *csource)
{
  new (&csource->source) Bse::SourceH();

  csource->params_dialog = NULL;
  csource->source_info = NULL;
  csource->icon_item = NULL;
  csource->text = NULL;
  csource->channel_items = NULL;
  csource->channel_hints = NULL;
  csource->swap_channels = BST_SNET_SWAP_IO_CHANNELS;
  csource->in_move = FALSE;
  csource->show_hints = FALSE;
  csource->move_dx = 0;
  csource->move_dy = 0;
  GtkObject *object = GTK_OBJECT (csource);
  g_object_connect (object,
		    "signal::notify", bst_canvas_source_changed, NULL,
		    NULL);
}

static void
bst_canvas_source_finalize (GObject *object)
{
  BstCanvasSource *self = BST_CANVAS_SOURCE (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);

  using namespace Bse;
  self->source.~SourceH();
}

static void
source_channels_changed (BstCanvasSource *csource)
{
  assert_return (BST_IS_CANVAS_SOURCE (csource));

  GNOME_CANVAS_NOTIFY (csource);
  bst_canvas_source_update_links (csource);
}

static gboolean
idle_move_item (gpointer data)
{
  BstCanvasSource *self = (BstCanvasSource*) data;
  GnomeCanvasItem *item = GNOME_CANVAS_ITEM (self);

  GDK_THREADS_ENTER ();
  if (self->source && item->canvas)
    {
      SfiReal x, y;
      bse_proxy_get (self->source.proxy_id(),
                     "pos-x", &x,
                     "pos-y", &y,
                     NULL);
      x *= BST_CANVAS_SOURCE_PIXEL_SCALE;
      y *= -BST_CANVAS_SOURCE_PIXEL_SCALE;
      gnome_canvas_item_w2i (item, &x, &y);
      g_object_freeze_notify (G_OBJECT (self));
      gnome_canvas_item_move (item, x, y);
      /* canvas notification bug workaround */
      g_object_notify ((GObject*) self, "x");
      g_object_notify ((GObject*) self, "y");
      g_object_thaw_notify (G_OBJECT (self));
    }
  self->idle_reposition = FALSE;
  g_object_unref (self);
  GDK_THREADS_LEAVE ();
  return FALSE;
}

static void
source_pos_changed (BstCanvasSource *self)
{
  if (!self->idle_reposition)
    {
      self->idle_reposition = TRUE;
      g_idle_add (idle_move_item, g_object_ref (self));
    }
}

static void
canvas_source_set_position (BstCanvasSource *self)
{
  gboolean idle_reposition = self->idle_reposition;
  GDK_THREADS_LEAVE ();
  idle_move_item (g_object_ref (self));
  GDK_THREADS_ENTER ();
  self->idle_reposition = idle_reposition;
}

static void
source_name_changed (BstCanvasSource *csource)
{
  assert_return (BST_IS_CANVAS_SOURCE (csource));

  String name = csource->source.get_name_or_type();

  if (csource->text)
    g_object_set (csource->text, "text", name.c_str(), NULL);

  if (csource->params_dialog)
    {
      gxk_dialog_set_title (GXK_DIALOG (csource->params_dialog), name.c_str());
      csource_info_update (csource);
    }

  name = "Info: " + name;
  if (csource->source_info)
    gxk_dialog_set_title (GXK_DIALOG (csource->source_info), name.c_str());
}

static void
source_icon_changed (BstCanvasSource *csource)
{
  // update icon in group, revert to a stock icon if none is available
  if (csource->icon_item)
    {
      Bse::Icon icon = csource->source.icon();
      bst_canvas_icon_set (csource->icon_item, icon, csource->source.get_type().c_str());
    }
}

static void
bst_canvas_source_destroy (GtkObject *object)
{
  BstCanvasSource *csource = BST_CANVAS_SOURCE (object);
  GnomeCanvasGroup *group = GNOME_CANVAS_GROUP (object);

  if (csource->in_move)
    {
      csource->in_move = FALSE;
      csource->source.ungroup_undo();
    }

  while (csource->channel_hints)
    gtk_object_destroy ((GtkObject*) csource->channel_hints->data);
  while (group->item_list)
    gtk_object_destroy ((GtkObject*) group->item_list->data);

  if (csource->source)
    {
      bse_proxy_disconnect (csource->source.proxy_id(),
			    "any_signal", gtk_object_destroy, csource,
			    "any_signal", source_channels_changed, csource,
			    "any_signal", source_pos_changed, csource,
			    "any_signal", source_icon_changed, csource,
			    NULL);
      csource->source.unuse();
      csource->source = Bse::SourceH();
    }

  GTK_OBJECT_CLASS (bst_canvas_source_parent_class)->destroy (object);
}

#define EPSILON 1e-6

GnomeCanvasItem*
bst_canvas_source_new (GnomeCanvasGroup *group,
		       SfiProxy		 source)
{
  BstCanvasSource *csource;
  GnomeCanvasItem *item;

  assert_return (GNOME_IS_CANVAS_GROUP (group), NULL);
  assert_return (BSE_IS_SOURCE (source), NULL);

  item = gnome_canvas_item_new (group,
				BST_TYPE_CANVAS_SOURCE,
				NULL);
  csource = BST_CANVAS_SOURCE (item);
  csource->source = Bse::SourceH::down_cast (bse_server.from_proxy (source));
  if (csource->source)
    csource->source.use();
  Bst::scoped_on (&csource->source, "notify:uname", [csource] () { source_name_changed (csource); });
  bse_proxy_connect (csource->source.proxy_id(),
		     "swapped_signal::release", gtk_object_destroy, csource,
		     "swapped_signal::io_changed", source_channels_changed, csource,
		     "swapped_signal::property-notify::pos-x", source_pos_changed, csource,
		     "swapped_signal::property-notify::pos-y", source_pos_changed, csource,
		     "swapped_signal::icon-changed", source_icon_changed, csource,
		     NULL);

  canvas_source_set_position (csource);
  bst_canvas_source_build (csource);

  GNOME_CANVAS_NOTIFY (item);

  return item;
}

void
bst_canvas_source_update_links (BstCanvasSource *csource)
{
  assert_return (BST_IS_CANVAS_SOURCE (csource));

  if (csource->source)
    gtk_signal_emit (GTK_OBJECT (csource), csource_signals[SIGNAL_UPDATE_LINKS]);
}

static inline void
canvas_source_create_params (BstCanvasSource *csource)
{
  if (!csource->params_dialog)
    {
      GtkWidget *param_view;

      param_view = bst_param_view_new (csource->source.proxy_id());
      csource->params_dialog = (GtkWidget*) gxk_dialog_new (&csource->params_dialog,
                                                            GTK_OBJECT (csource),
                                                            GXK_DIALOG_POPUP_POS,
                                                            csource->source.get_name_or_type().c_str(),
                                                            param_view);
      source_name_changed (csource);
    }
}

void
bst_canvas_source_reset_params (BstCanvasSource *csource)
{
  GtkWidget *param_view;

  assert_return (BST_IS_CANVAS_SOURCE (csource));

  canvas_source_create_params (csource);
  param_view = gxk_dialog_get_child (GXK_DIALOG (csource->params_dialog));
  bst_param_view_apply_defaults (BST_PARAM_VIEW (param_view));
}

void
bst_canvas_source_popup_params (BstCanvasSource *csource)
{
  assert_return (BST_IS_CANVAS_SOURCE (csource));

  canvas_source_create_params (csource);
  gxk_widget_showraise (csource->params_dialog);
}

void
bst_canvas_source_toggle_params (BstCanvasSource *csource)
{
  assert_return (BST_IS_CANVAS_SOURCE (csource));

  if (!csource->params_dialog || !GTK_WIDGET_VISIBLE (csource->params_dialog))
    bst_canvas_source_popup_params (csource);
  else
    gtk_widget_hide (csource->params_dialog);
}

void
bst_canvas_source_set_channel_hints (BstCanvasSource *csource,
				     gboolean         on_off)
{
  GSList *slist;

  assert_return (BST_IS_CANVAS_SOURCE (csource));

  csource->show_hints = !!on_off;
  if (csource->show_hints)
    for (slist = csource->channel_hints; slist; slist = slist->next)
      g_object_set (slist->data, "text", g_object_get_data ((GObject*) slist->data, "hint_text"), NULL);
  else
    for (slist = csource->channel_hints; slist; slist = slist->next)
      g_object_set (slist->data, "text", "", NULL);
}

static void
csource_info_update (BstCanvasSource *csource)
{
  GtkWidget *text = (csource->source_info // && (force_update || GTK_WIDGET_VISIBLE (csource->source_info))
                     ? gxk_dialog_get_child (GXK_DIALOG (csource->source_info))
                     : NULL);
  if (text)
    {
      /* construct information */
      gxk_scroll_text_clear (text);
      gxk_scroll_text_aprintf (text, "%s:\n", csource->source.get_name_or_type());

      /* type & category */
      gxk_scroll_text_push_indent (text);
      gxk_scroll_text_aprintf (text, "Type: %s\n", csource->source.get_type_name());
      Bse::CategorySeq cseq = bse_server.category_match_typed ("*", csource->source.get_type_name());
      if (cseq.size())
        gxk_scroll_text_aprintf (text, "Category: %s\n", cseq[0].category);
      gxk_scroll_text_pop_indent (text);

      /* input channels */
      const size_t csource_source_n_ichannels = csource->source.n_ichannels();
      if (csource_source_n_ichannels)
	{
	  gxk_scroll_text_aprintf (text, "\nInput Channels:\n");
	  gxk_scroll_text_push_indent (text);
	}
      for (size_t i = 0; i < csource_source_n_ichannels; i++)
	{
          const String string = csource->source.ichannel_blurb (i);
	  gxk_scroll_text_aprintf (text, "%s[%s]%s\n",
				   csource->source.ichannel_label (i).c_str(),
				   csource->source.ichannel_ident (i).c_str(),
				   string.empty() ? "" : ":");
	  if (!string.empty())
	    {
	      gxk_scroll_text_push_indent (text);
	      gxk_scroll_text_aprintf (text, "%s\n", string.c_str());
	      gxk_scroll_text_pop_indent (text);
	    }
	}
      if (csource_source_n_ichannels)
	gxk_scroll_text_pop_indent (text);

      /* output channels */
      const size_t csource_source_n_ochannels = csource->source.n_ochannels();
      if (csource_source_n_ochannels)
	{
	  gxk_scroll_text_aprintf (text, "\nOutput Channels:\n");
	  gxk_scroll_text_push_indent (text);
	}
      for (size_t i = 0; i < csource_source_n_ochannels; i++)
	{
	  const String string = csource->source.ochannel_blurb (i);
	  gxk_scroll_text_aprintf (text, "%s[%s]%s\n",
				   csource->source.ochannel_label (i).c_str(),
				   csource->source.ochannel_ident (i).c_str(),
				   string.empty() ? "" : ":");
          if (!string.empty())
	    {
	      gxk_scroll_text_push_indent (text);
	      gxk_scroll_text_aprintf (text, "%s\n", string.c_str());
	      gxk_scroll_text_pop_indent (text);
	    }
	}
      if (csource_source_n_ochannels)
	gxk_scroll_text_pop_indent (text);

      // description
      String string = csource->source.get_type_blurb();
      if (!string.empty())
	{
	  gxk_scroll_text_aprintf (text, "\nDescription:\n");
	  gxk_scroll_text_push_indent (text);
	  gxk_scroll_text_aprintf (text, "%s\n", string);
	  gxk_scroll_text_pop_indent (text);
	}

      // authors
      string = csource->source.get_type_authors();
      if (!string.empty())
        gxk_scroll_text_aprintf (text, "\nAuthors: %s\n", string);

      // license
      string = csource->source.get_type_license();
      if (!string.empty())
        gxk_scroll_text_aprintf (text, "\nLicense: %s\n", string);
    }
}

void
bst_canvas_source_popup_info (BstCanvasSource *csource)
{
  assert_return (BST_IS_CANVAS_SOURCE (csource));

  if (!csource->source_info)
    {
      GtkWidget *sctext = gxk_scroll_text_create (GXK_SCROLL_TEXT_WIDGET_LOOK, NULL);
      GtkWidget *frame = gtk_widget_new (GTK_TYPE_FRAME,
                                         "visible", TRUE,
                                         "label", _("Module Info"),
                                         "border_width", 5,
                                         NULL);
      g_object_new (GTK_TYPE_ALIGNMENT,
                    "visible", TRUE,
                    "parent", frame,
                    "border_width", 2,
                    "child", sctext,
                    NULL);
      csource->source_info = (GtkWidget*) gxk_dialog_new (&csource->source_info,
                                                          GTK_OBJECT (csource),
                                                          GXK_DIALOG_POPUP_POS,
                                                          csource->source.get_name_or_type().c_str(),
                                                          sctext);
    }
  csource_info_update (csource);
  source_name_changed (csource);
  gxk_widget_showraise (csource->source_info);
}

void
bst_canvas_source_toggle_info (BstCanvasSource *csource)
{
  assert_return (BST_IS_CANVAS_SOURCE (csource));

  if (!csource->source_info || !GTK_WIDGET_VISIBLE (csource->source_info))
    bst_canvas_source_popup_info (csource);
  else
    gtk_widget_hide (csource->source_info);
}

BstCanvasSource*
bst_canvas_source_at (GnomeCanvas *canvas,
		      gdouble      world_x,
		      gdouble      world_y)
{
  return (BstCanvasSource*) gnome_canvas_typed_item_at (canvas, BST_TYPE_CANVAS_SOURCE, world_x, world_y);
}

gboolean
bst_canvas_source_is_jchannel (BstCanvasSource *csource,
			       guint            ichannel)
{
  assert_return (BST_IS_CANVAS_SOURCE (csource), FALSE);

  if (!csource->source)
    return FALSE;

  return csource->source.is_joint_ichannel_by_id (ichannel);
}

gboolean
bst_canvas_source_ichannel_free (BstCanvasSource *csource, uint ichannel)
{
  assert_return (BST_IS_CANVAS_SOURCE (csource), FALSE);

  if (!csource->source)
    return FALSE;

  if (csource->source.is_joint_ichannel_by_id (ichannel))
    return TRUE;
  else
    {
      Bse::SourceH osource = csource->source.ichannel_get_osource (ichannel, 0);
      return osource == NULL;
    }
}

void
bst_canvas_source_ichannel_pos (BstCanvasSource *csource,
				guint            ochannel,
				gdouble         *x_p,
				gdouble         *y_p)
{
  gdouble x = 0, y = 0;

  assert_return (BST_IS_CANVAS_SOURCE (csource));

  x = ICHANNEL_X (csource) + CHANNEL_WIDTH (csource) / 2;
  if (csource->source)
    y = CHANNEL_HEIGHT (csource) / csource->source.n_ichannels();
  y *= ochannel + 0.5;
  y += ICHANNEL_Y (csource);
  gnome_canvas_item_i2w (GNOME_CANVAS_ITEM (csource), &x, &y);
  if (x_p)
    *x_p = x;
  if (y_p)
    *y_p = y;
}

void
bst_canvas_source_ochannel_pos (BstCanvasSource *csource,
				guint            ichannel,
				gdouble         *x_p,
				gdouble         *y_p)
{
  gdouble x, y;

  assert_return (BST_IS_CANVAS_SOURCE (csource));

  x = OCHANNEL_X (csource) + CHANNEL_WIDTH (csource) / 2;
  if (csource->source)
    y = CHANNEL_HEIGHT (csource) / csource->source.n_ochannels();
  y *= ichannel + 0.5;
  y += OCHANNEL_Y (csource);
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
  guint channel = ~0;

  assert_return (BST_IS_CANVAS_SOURCE (csource), 0);

  gnome_canvas_item_w2i (GNOME_CANVAS_ITEM (csource), &x, &y);

  x -= ICHANNEL_X (csource);
  y -= ICHANNEL_Y (csource);
  if (x > 0 && x < CHANNEL_WIDTH (csource) &&
      y > 0 && y < CHANNEL_HEIGHT (csource) &&
      csource->source.n_ichannels())
    {
      y /= CHANNEL_HEIGHT (csource) / csource->source.n_ichannels();
      channel = y;
    }

  return channel;
}

guint
bst_canvas_source_ochannel_at (BstCanvasSource *csource,
			       gdouble	        x,
			       gdouble	        y)
{
  guint channel = ~0;

  assert_return (BST_IS_CANVAS_SOURCE (csource), 0);

  gnome_canvas_item_w2i (GNOME_CANVAS_ITEM (csource), &x, &y);

  x -= OCHANNEL_X (csource);
  y -= OCHANNEL_Y (csource);
  if (x > 0 && x < CHANNEL_WIDTH (csource) &&
      y > 0 && y < CHANNEL_HEIGHT (csource) &&
      csource->source.n_ochannels())
    {
      y /= CHANNEL_HEIGHT (csource) / csource->source.n_ochannels();
      channel = y;
    }

  return channel;
}

static void
bst_canvas_icon_set (GnomeCanvasItem *item, Bse::Icon &icon, const char *module_type)
{
  GdkPixbuf *pixbuf;
  gboolean need_unref = FALSE;
  if (icon.width && icon.height && icon.width * icon.height == ssize_t (icon.pixels.size()))
    {
      guchar *pixels = (guchar*) g_memdup (icon.pixels.data(), icon.height * icon.width * 4);
      pixbuf = gdk_pixbuf_new_from_data (pixels, GDK_COLORSPACE_RGB, true,
					 8, icon.width, icon.height, icon.width * 4,
                                         (GdkPixbufDestroyNotify) g_free, NULL);
      need_unref = true;
    }
  else if (module_type && strncmp (module_type, "BseLadspaModule_", 16) == 0)
    pixbuf = bst_pixbuf_ladspa ();
  else
    pixbuf = bst_pixbuf_no_icon ();

  g_object_set (GTK_OBJECT (item),
		"pixbuf", pixbuf,
		"x_in_pixels", FALSE,
		"y_in_pixels", FALSE,
		"anchor", GTK_ANCHOR_NORTH_WEST,
		NULL);
  if (need_unref)
    g_object_unref (pixbuf);
}

static void
channel_item_remove (BstCanvasSource *csource,
		     GnomeCanvasItem *item)
{
  csource->channel_items = g_slist_remove (csource->channel_items, item);
}

static void
channel_name_remove (BstCanvasSource *csource,
		     GnomeCanvasItem *item)
{
  csource->channel_hints = g_slist_remove (csource->channel_hints, item);
}

static void
bst_canvas_source_build_channels (BstCanvasSource *csource,
				  gboolean         is_input,
				  gint             color1,
				  gint		   color1_fade,
				  gint		   color2,
				  gint		   color2_fade,
                                  gboolean         build_channel_items,
                                  gboolean         build_channel_hints)
{
  GnomeCanvasGroup *group = GNOME_CANVAS_GROUP (csource);
  const guint alpha = 0xa0;
  gint n_channels, color1_delta = 0, color2_delta = 0;
  gdouble x1, x2, y1, y2;
  gdouble d_y;
  gboolean east_channel = CHANNEL_EAST (csource, is_input);

  if (is_input)
    {
      n_channels = csource->source.n_ichannels();
      x1 = ICHANNEL_X (csource);
      y1 = ICHANNEL_Y (csource);
    }
  else
    {
      n_channels = csource->source.n_ochannels();
      x1 = OCHANNEL_X (csource);
      y1 = OCHANNEL_Y (csource);
    }
  x2 = x1 + CHANNEL_WIDTH (csource);
  y2 = y1 + CHANNEL_HEIGHT (csource);
  d_y = y2 - y1;
  if (n_channels)
    d_y /= n_channels;

  if (n_channels > 1)
    {
      gint cd_red, cd_blue, cd_green;

      cd_red = ((color1_fade & 0xff0000) - (color1 & 0xff0000)) / (n_channels - 1);
      cd_green = ((color1_fade & 0x00ff00) - (color1 & 0x00ff00)) / (n_channels - 1);
      cd_blue = ((color1_fade & 0x0000ff) - (color1 & 0x0000ff)) / (n_channels - 1);
      color1_delta = (cd_red & ~0xffff) + (cd_green & ~0xff) + cd_blue;

      cd_red = ((color2_fade & 0xff0000) - (color2 & 0xff0000)) / (n_channels - 1);
      cd_green = ((color2_fade & 0x00ff00) - (color2 & 0x00ff00)) / (n_channels - 1);
      cd_blue = ((color2_fade & 0x0000ff) - (color2 & 0x0000ff)) / (n_channels - 1);
      color2_delta = (cd_red & ~0xffff) + (cd_green & ~0xff) + cd_blue;
    }
  else if (n_channels == 0)
    {
      GnomeCanvasItem *item;
      if (build_channel_items)
        {
          item = gnome_canvas_item_new (group,
                                        GNOME_TYPE_CANVAS_RECT,
                                        "fill_color_rgba", (0xc3c3c3 << 8) | alpha,
                                        "outline_color_rgba", RGBA_BLACK,
                                        "x1", x1,
                                        "y1", y1,
                                        "x2", x2,
                                        "y2", y2,
                                        NULL);
          g_object_connect (item,
                            "swapped_signal::destroy", channel_item_remove, csource,
                            "swapped_signal::event", bst_canvas_source_child_event, csource,
                            NULL);
          csource->channel_items = g_slist_prepend (csource->channel_items, item);
        }
    }

  for (int i = 0; i < n_channels; i++)
    {
      GnomeCanvasItem *item;
      gboolean is_jchannel = is_input && csource->source.is_joint_ichannel_by_id (i);
      const String label = is_input ? csource->source.ichannel_label (i) : csource->source.ochannel_label (i);
      guint tmp_color = is_jchannel ? color2 : color1;

      y2 = y1 + d_y;
      if (build_channel_items)
        {
          item = gnome_canvas_item_new (group,
                                        GNOME_TYPE_CANVAS_RECT,
                                        "fill_color_rgba", (tmp_color << 8) | alpha,
                                        "outline_color_rgba", RGBA_BLACK,
                                        "x1", x1,
                                        "y1", y1,
                                        "x2", x2,
                                        "y2", y2,
                                        NULL);
          g_object_connect (item,
                            "swapped_signal::destroy", channel_item_remove, csource,
                            "swapped_signal::event", bst_canvas_source_child_event, csource,
                            NULL);
          csource->channel_items = g_slist_prepend (csource->channel_items, item);
        }

      if (build_channel_hints)
        {
          item = gnome_canvas_item_new (group,
                                        GNOME_TYPE_CANVAS_TEXT,
                                        "fill_color_rgba", (0x000000 << 8) | 0x80,
                                        "anchor", east_channel ? GTK_ANCHOR_WEST : GTK_ANCHOR_EAST,
                                        "justification", GTK_JUSTIFY_RIGHT,
                                        "x", east_channel ? TOTAL_WIDTH (csource) + BORDER_PAD * 2. : -BORDER_PAD,
                                        "y", (y1 + y2) / 2.,
                                        "font", CHANNEL_FONT,
                                        "text", csource->show_hints ? label.c_str() : "",
                                        NULL);
          g_object_connect (item,
                            "swapped_signal::destroy", channel_name_remove, csource,
                            NULL);
          gnome_canvas_text_set_zoom_size (GNOME_CANVAS_TEXT (item), FONT_HEIGHT);
          g_object_set_data_full (G_OBJECT (item), "hint_text", g_strdup (label.c_str()), g_free);
          csource->channel_hints = g_slist_prepend (csource->channel_hints, item);
        }

      color1 += color1_delta;
      color2 += color2_delta;
      y1 = y2;
    }
}

static gboolean
bst_canvas_source_build_async (gpointer data)
{
  GnomeCanvasItem *item = GNOME_CANVAS_ITEM (data);
  if (gnome_canvas_item_check_undisposed (item))
    {
      BstCanvasSource *csource = BST_CANVAS_SOURCE (item);
      GnomeCanvasGroup *group = GNOME_CANVAS_GROUP (csource);

      /* keep in mind, that creation order affects stacking */

      /* add input and output channel items */
      if (!csource->built_ichannels)
        {
          csource->built_ichannels = TRUE;
          bst_canvas_source_build_channels (csource,
                                            TRUE,               /* input channels */
                                            0xffff00, 0x808000,	/* ichannels */
                                            0x00afff, 0x005880, /* jchannels */
                                            TRUE, FALSE);
          return TRUE;
        }
      if (!csource->built_ochannels)
        {
          csource->built_ochannels = TRUE;
          bst_canvas_source_build_channels (csource,
                                            FALSE,              /* output channels */
                                            0xff0000, 0x800000, /* ochannels */
                                            0, 0,               /* unused */
                                            TRUE, FALSE);
          return TRUE;
        }

      /* add icon to group */
      if (!csource->icon_item)
        {
          csource->icon_item = gnome_canvas_item_new (group,
                                                      GNOME_TYPE_CANVAS_PIXBUF,
                                                      "x", ICON_X (csource),
                                                      "y", ICON_Y (csource),
                                                      "width", ICON_WIDTH (csource),
                                                      "height", ICON_HEIGHT (csource),
                                                      NULL);
          g_object_connect (csource->icon_item,
                            "signal::destroy", gtk_widget_destroyed, &csource->icon_item,
                            "swapped_signal::event", bst_canvas_source_child_event, csource,
                            NULL);
          source_icon_changed (csource);
          return TRUE;
        }

      if (!csource->text)
        {
          /* add text item, invoke name_changed callback to setup the text value */
          guint ocolor = csource->source && csource->source.internal() ? RGBA_INTERNAL : RGBA_BLACK;
          csource->text = gnome_canvas_item_new (group,
                                                 GNOME_TYPE_CANVAS_TEXT,
                                                 "fill_color_rgba", ocolor,
                                                 "anchor", GTK_ANCHOR_NORTH,
                                                 "justification", GTK_JUSTIFY_CENTER,
                                                 "x", TEXT_X (csource),
                                                 "y", TEXT_Y (csource),
                                                 "font", TEXT_FONT,
                                                 NULL);
          g_object_connect (csource->text,
                            "signal::destroy", gtk_widget_destroyed, &csource->text,
                            "swapped_signal::event", bst_canvas_source_child_event, csource,
                            NULL);
          gnome_canvas_text_set_zoom_size (GNOME_CANVAS_TEXT (csource->text), FONT_HEIGHT);
          source_name_changed (csource);
          return TRUE;
        }

      /* add input and output channel hints */
      if (!csource->built_ihints)
        {
          csource->built_ihints = TRUE;
          bst_canvas_source_build_channels (csource,
                                            TRUE,               /* input channels */
                                            0xffff00, 0x808000,	/* ichannels */
                                            0x00afff, 0x005880, /* jchannels */
                                            FALSE, TRUE);
          return TRUE;
        }
      if (!csource->built_ohints)
        {
          csource->built_ohints = TRUE;
          bst_canvas_source_build_channels (csource,
                                            FALSE,              /* output channels */
                                            0xff0000, 0x800000, /* ochannels */
                                            0, 0,               /* unused */
                                            FALSE, TRUE);
          return TRUE;
        }
    }
  GnomeCanvas *canvas = (GnomeCanvas*) g_object_steal_data ((GObject*) item, "bst-workaround-canvas-ref");
  g_object_unref (item);
  if (canvas)
    g_object_unref (canvas);      /* canvases don't properly protect their items */
  return FALSE;
}

static void
bst_canvas_source_build (BstCanvasSource *csource)
{
  GnomeCanvasGroup *group = GNOME_CANVAS_GROUP (csource);
  /* put an outer rectangle, make it transparent in aa mode,
   * so we can receive mouse events everywhere
   */
  GnomeCanvasItem *rect = gnome_canvas_item_new (group,
                                                 GNOME_TYPE_CANVAS_RECT,
                                                 "outline_color_rgba", RGBA_BLACK, /* covers buggy canvas lines */
                                                 "x1", 0.0,
                                                 "y1", 0.0,
                                                 "x2", TOTAL_WIDTH (csource),
                                                 "y2", TOTAL_HEIGHT (csource),
                                                 (GNOME_CANVAS_ITEM (csource)->canvas->aa
                                                  ? "fill_color_rgba"
                                                  : NULL), 0x00000000,
                                                 NULL);
  g_object_connect (rect,
                    "swapped_signal::event", bst_canvas_source_child_event, csource,
                    NULL);
  /* make sure no items are left over */
  while (csource->channel_items)
    gtk_object_destroy ((GtkObject*) csource->channel_items->data);
  while (csource->channel_hints)
    gtk_object_destroy ((GtkObject*) csource->channel_hints->data);
  csource->built_ichannels = FALSE;
  csource->built_ochannels = FALSE;
  csource->built_ihints = FALSE;
  csource->built_ohints = FALSE;
  /* asynchronously rebuild contents */
  GnomeCanvasItem *csource_item = GNOME_CANVAS_ITEM (csource);
  /* work around stale canvas pointers, see #340437 */
  g_object_set_data_full ((GObject*) csource_item, "bst-workaround-canvas-ref", g_object_ref (csource_item->canvas), g_object_unref);
  bst_background_handler2_add (bst_canvas_source_build_async, g_object_ref (csource), NULL);
}

static void
bst_canvas_source_changed (BstCanvasSource *csource)
{
  if (csource->source)
    {
      GnomeCanvasItem *item = GNOME_CANVAS_ITEM (csource);
      gdouble x = 0, y = 0;
      gnome_canvas_item_i2w (item, &x, &y);
      csource->source.set_pos (x / BST_CANVAS_SOURCE_PIXEL_SCALE, y / -BST_CANVAS_SOURCE_PIXEL_SCALE);
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
      if (!csource->in_move && bst_mouse_button_move (event))
	{
	  GdkCursor *fleur = gdk_cursor_new (GDK_FLEUR);
	  if (gnome_canvas_item_grab (item,
                                      GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
                                      fleur,
                                      event->button.time) == 0)
            {
              gdouble x = event->button.x, y = event->button.y;
              gnome_canvas_item_w2i (item, &x, &y);
              csource->move_dx = x;
              csource->move_dy = y;
              csource->in_move = TRUE;
              csource->source.group_undo ("Move");
            }
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
	  GNOME_CANVAS_NOTIFY (item);
	  handled = TRUE;
	}
      else
	{
	  guint channel;
	  const gchar *prefix = NULL;
          String label, ident;

	  /* set i/o channel hints */
	  channel = bst_canvas_source_ichannel_at (csource, event->motion.x, event->motion.y);
	  if (channel != ~uint (0))
	    {
	      label = csource->source.ichannel_label (channel);
	      ident = csource->source.ichannel_ident (channel);
	      prefix = _("Input");
	    }
	  else
	    {
	      channel = bst_canvas_source_ochannel_at (csource, event->motion.x, event->motion.y);
	      if (channel != ~uint (0))
		{
		  label = csource->source.ochannel_label (channel);
		  ident = csource->source.ochannel_ident (channel);
		  prefix = _("Output");
		}
	    }
	  if (!label.empty())
	    gxk_status_printf (GXK_STATUS_IDLE_HINT, _("(Hint)"), "%s[%s]: %s", prefix, ident, label);
	  else
	    gxk_status_set (GXK_STATUS_IDLE_HINT, NULL, NULL);
	}
      break;
    case GDK_BUTTON_RELEASE:
      if (bst_mouse_button_move (event) && csource->in_move)
	{
          csource->source.ungroup_undo();
	  csource->in_move = FALSE;
	  gnome_canvas_item_ungrab (item, event->button.time);
	  handled = TRUE;
	}
      break;
    default:
      break;
    }

  if (!handled && GNOME_CANVAS_ITEM_CLASS (bst_canvas_source_parent_class)->event)
    handled = GNOME_CANVAS_ITEM_CLASS (bst_canvas_source_parent_class)->event (item, event);

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
	  GNOME_CANVAS_NOTIFY (item);
	  break;
	case 'R':
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

static void
bst_canvas_source_class_init (BstCanvasSourceClass *klass)
{
  parent_class = g_type_class_peek_parent (klass);

  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
  object_class->destroy = bst_canvas_source_destroy;

  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = bst_canvas_source_finalize;

  GnomeCanvasItemClass *canvas_item_class = GNOME_CANVAS_ITEM_CLASS (klass);
  canvas_item_class->event = bst_canvas_source_event;
  klass->update_links = NULL;

  csource_signals[SIGNAL_UPDATE_LINKS] =
    gtk_signal_new ("update-links",
		    GTK_RUN_LAST,
		    GTK_CLASS_TYPE (object_class),
		    GTK_SIGNAL_OFFSET (BstCanvasSourceClass, update_links),
		    gtk_signal_default_marshaller,
		    GTK_TYPE_NONE, 0);
}
