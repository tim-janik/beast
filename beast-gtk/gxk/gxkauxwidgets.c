/* GXK - Gtk+ Extension Kit
 * Copyright (C) 2003 Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "gxkauxwidgets.h"
#include "gxkstock.h"
#include <gdk/gdkkeysyms.h>

/* --- GxkMenuItem --- */
enum {
  MENU_ITEM_PROP_0,
  MENU_ITEM_PROP_ULINE_LABEL,
  MENU_ITEM_PROP_STOCK_IMAGE,
  MENU_ITEM_PROP_KEEP_MENUBAR_IMAGE,
  MENU_ITEM_PROP_ACCEL_PATH,
  MENU_ITEM_PROP_ACCEL,
  MENU_ITEM_PROP_TITLE_STYLE,
  MENU_ITEM_PROP_RIGHT_JUSTIFY
};

static void     gxk_menu_item_class_init           (GxkMenuItemClass    *class);
static void     gxk_menu_item_set_property         (GObject             *object,
                                                    guint                param_id,
                                                    const GValue        *value,
                                                    GParamSpec          *pspec);
static void     gxk_menu_item_get_property         (GObject             *object,
                                                    guint                param_id,
                                                    GValue              *value,
                                                    GParamSpec          *pspec);
static void
menu_item_keep_menubar_image (GtkImageMenuItem *imitem)
{
  if (imitem->image && GTK_IS_MENU_BAR (GTK_WIDGET (imitem)->parent) &&
      !g_object_get_long (imitem, "gxk-keep-menubar-image"))
    gtk_image_menu_item_set_image (imitem, NULL);
}

static void
gxk_menu_item_set_property (GObject      *object,
                            guint         param_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  GxkMenuItem *self = GXK_MENU_ITEM (object);
  GtkMenuItem *mitem = GTK_MENU_ITEM (object);
  GtkBin *bin = GTK_BIN (self);
  switch (param_id)
    {
      const gchar *string, *path;
      gchar *accel;
      gboolean vbool;
    case MENU_ITEM_PROP_ULINE_LABEL:
      if (bin->child)
        gtk_container_remove (GTK_CONTAINER (self), bin->child);
      string = g_value_get_string (value);
      if (string)
        {
          GtkWidget *label = g_object_new (GTK_TYPE_ACCEL_LABEL,
                                           "visible", TRUE,
                                           "label", string,
                                           "use-underline", TRUE,
                                           "xalign", 0.0,
                                           "accel-widget", self,
                                           "parent", self,
                                           NULL);
          if (g_object_get_long (self, "gxk-title-style"))
            gxk_widget_modify_as_title (label);
        }
      break;
    case MENU_ITEM_PROP_STOCK_IMAGE:
      string = g_value_get_string (value);
      if (string)
        {
          GtkWidget *image = gtk_image_new_from_stock (string, GXK_ICON_SIZE_MENU);
          if (image)
            {
              gtk_widget_show (image);
              gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (self), image);
            }
        }
      break;
    case MENU_ITEM_PROP_KEEP_MENUBAR_IMAGE:
      vbool = g_value_get_boolean (value);
      g_object_set_long (self, "gxk-keep-menubar-image", vbool);
      menu_item_keep_menubar_image (self);
      break;
    case MENU_ITEM_PROP_RIGHT_JUSTIFY:
      gtk_menu_item_set_right_justified (mitem, g_value_get_boolean (value));
      break;
    case MENU_ITEM_PROP_TITLE_STYLE:
      if (g_value_get_boolean (value))
        {
          gxk_widget_modify_as_title (GTK_WIDGET (self));
          g_object_set_long (self, "gxk-title-style", 1);
          if (bin->child)
            gxk_widget_modify_as_title (bin->child);
        }
      break;
    case MENU_ITEM_PROP_ACCEL_PATH:
      path = g_value_get_string (value);
      gtk_menu_item_set_accel_path (mitem, path);
      accel = g_object_get_data (self, "gxk-menu-item-accel");
      goto setup_accel;
    case MENU_ITEM_PROP_ACCEL:
      accel = g_value_dup_string (value);
      g_object_set_data_full (self, "gxk-menu-item-accel", accel, g_free);
      path = mitem->accel_path;
    setup_accel:
      if (accel && path)
        {
          GdkModifierType mods = 0;
          guint keyval = 0;
          if (accel)
            gtk_accelerator_parse (accel, &keyval, &mods);
          keyval = keyval != GDK_VoidSymbol ? keyval : 0;
          gtk_accel_map_add_entry (path, keyval, mods);
        }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
gxk_menu_item_get_property (GObject    *object,
                            guint       param_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  GxkMenuItem *self = GXK_MENU_ITEM (object);
  switch (param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
gxk_menu_item_init (GxkMenuItem *self)
{
  g_signal_connect (self, "parent-set", G_CALLBACK (menu_item_keep_menubar_image), NULL);
  g_signal_connect (self, "check-resize", G_CALLBACK (menu_item_keep_menubar_image), NULL);         
}

static void
gxk_menu_item_class_init (GxkMenuItemClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  gobject_class->set_property = gxk_menu_item_set_property;
  gobject_class->get_property = gxk_menu_item_get_property;
  g_object_class_install_property (gobject_class, MENU_ITEM_PROP_ULINE_LABEL,
                                   g_param_spec_string ("uline-label", NULL, NULL, NULL, G_PARAM_WRITABLE));
  g_object_class_install_property (gobject_class, MENU_ITEM_PROP_STOCK_IMAGE,
                                   g_param_spec_string ("stock-image", NULL, NULL, NULL, G_PARAM_WRITABLE));
  g_object_class_install_property (gobject_class, MENU_ITEM_PROP_KEEP_MENUBAR_IMAGE,
                                   g_param_spec_boolean ("keep-menubar-image", NULL, NULL, FALSE, G_PARAM_WRITABLE));
  g_object_class_install_property (gobject_class, MENU_ITEM_PROP_ACCEL,
                                   g_param_spec_string ("accel", NULL, NULL, NULL, G_PARAM_WRITABLE));
  g_object_class_install_property (gobject_class, MENU_ITEM_PROP_ACCEL_PATH,
                                   g_param_spec_string ("accel-path", NULL, NULL, NULL, G_PARAM_WRITABLE));
  g_object_class_install_property (gobject_class, MENU_ITEM_PROP_RIGHT_JUSTIFY,
                                   g_param_spec_boolean ("right-justify", NULL, NULL, FALSE, G_PARAM_WRITABLE));
  g_object_class_install_property (gobject_class, MENU_ITEM_PROP_TITLE_STYLE,
                                   g_param_spec_boolean ("title-style", NULL, NULL, FALSE, G_PARAM_WRITABLE));
}

GType
gxk_menu_item_get_type (void)
{
  static GType type = 0;
  if (!type)
    {
      static const GTypeInfo type_info = {
        sizeof (GxkMenuItemClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gxk_menu_item_class_init,
        NULL,   /* class_finalize */
        NULL,   /* class_data */
        sizeof (GxkMenuItem),
        0,      /* n_preallocs */
        (GInstanceInitFunc) gxk_menu_item_init,
      };
      type = g_type_register_static (GTK_TYPE_IMAGE_MENU_ITEM, "GxkMenuItem", &type_info, 0);
    }
  return type;
}


/* --- GxkFreeRadioButton --- */
static void
gxk_free_radio_button_class_init (GxkFreeRadioButtonClass *class)
{
  GtkCheckButtonClass *gtk_check_button_class = g_type_class_ref (GTK_TYPE_CHECK_BUTTON);
  GtkButtonClass *button_class = GTK_BUTTON_CLASS (class);
  button_class->clicked = GTK_BUTTON_CLASS (gtk_check_button_class)->clicked;
  g_type_class_unref (gtk_check_button_class);
}

GType
gxk_free_radio_button_get_type (void)
{
  static GType type = 0;
  if (!type)
    {
      static const GTypeInfo type_info = {
        sizeof (GxkFreeRadioButtonClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gxk_free_radio_button_class_init,
        NULL,   /* class_finalize */
        NULL,   /* class_data */
        sizeof (GxkFreeRadioButton),
        0,      /* n_preallocs */
        (GInstanceInitFunc) NULL,
      };
      type = g_type_register_static (GTK_TYPE_RADIO_BUTTON, "GxkFreeRadioButton", &type_info, 0);
    }
  return type;
}


/* --- GxkImage --- */
enum {
  IMAGE_PROP_0,
  IMAGE_PROP_STOCK_SIZE
};
static void     gxk_image_class_init           (GxkImageClass       *class);
static void     gxk_image_set_property         (GObject             *object,
                                                guint                param_id,
                                                const GValue        *value,
                                                GParamSpec          *pspec);

static void
gxk_image_set_property (GObject      *object,
                        guint         param_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  GxkImage *self = GXK_IMAGE (object);
  switch (param_id)
    {
      const gchar *str;
    case IMAGE_PROP_STOCK_SIZE:
      str = g_value_get_string (value);
      if (str)
        g_object_set (self, "icon-size", gtk_icon_size_from_name (str), NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
gxk_image_class_init (GxkImageClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  gobject_class->set_property = gxk_image_set_property;
  g_object_class_install_property (gobject_class, IMAGE_PROP_STOCK_SIZE,
                                   g_param_spec_string ("stock-size", NULL, NULL, NULL, G_PARAM_WRITABLE));
}

GType
gxk_image_get_type (void)
{
  static GType type = 0;
  if (!type)
    {
      static const GTypeInfo type_info = {
        sizeof (GxkImageClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gxk_image_class_init,
        NULL,   /* class_finalize */
        NULL,   /* class_data */
        sizeof (GxkImage),
        0,      /* n_preallocs */
        (GInstanceInitFunc) NULL,
      };
      type = g_type_register_static (GTK_TYPE_IMAGE, "GxkImage", &type_info, 0);
    }
  return type;
}


/* --- GxkWidgetPatcher --- */
static void   gxk_widget_patcher_class_init     (GxkWidgetPatcherClass  *class);
static void   gxk_widget_patcher_set_property   (GObject                *object,
                                                 guint                   param_id,
                                                 const GValue           *value,
                                                 GParamSpec             *pspec);
static gpointer widget_patcher_parent_class = NULL;
static gboolean
widget_patcher_unref (gpointer data)
{
  GDK_THREADS_ENTER ();
  g_object_unref (data);
  GDK_THREADS_LEAVE ();
  return FALSE;
}

enum {
  PATCHER_PROP_0,
  PATCHER_PROP_TOOLTIP,
  PATCHER_PROP_TOOLTIP_VISIBLE,
  PATCHER_PROP_MUTE_EVENTS,
  PATCHER_PROP_LOWER_WINDOWS,
  PATCHER_PROP_HIDE_INSENSITIVE,
  PATCHER_PROP_WIDTH_FROM_HEIGHT,
  PATCHER_PROP_HEIGHT_FROM_WIDTH,
  PATCHER_PROP_FORCE_RESIZE_HSTEPS,
  PATCHER_PROP_FORCE_RESIZE_VSTEPS,
  PATCHER_PROP_MIN_RESIZE_HUNITS,
  PATCHER_PROP_MIN_RESIZE_VUNITS,
};
static void
gxk_widget_patcher_set_property (GObject      *object,
                                 guint         param_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GxkWidgetPatcher *self = GXK_WIDGET_PATCHER (object);
  switch (param_id)
    {
    case PATCHER_PROP_TOOLTIP:
      self->tooltip = g_value_dup_string (value);
      break;
    case PATCHER_PROP_TOOLTIP_VISIBLE:
      self->tooltip_visible = g_value_get_boolean (value);
      break;
    case PATCHER_PROP_MUTE_EVENTS:
      self->mute_events = g_value_get_boolean (value);
      break;
    case PATCHER_PROP_LOWER_WINDOWS:
      self->lower_windows = g_value_get_boolean (value);
      break;
    case PATCHER_PROP_HIDE_INSENSITIVE:
      self->hide_insensitive = g_value_get_boolean (value);
      break;
    case PATCHER_PROP_WIDTH_FROM_HEIGHT:
      self->width_from_height = g_value_get_double (value);
      break;
    case PATCHER_PROP_HEIGHT_FROM_WIDTH:
      self->height_from_width = g_value_get_double (value);
      break;
    case PATCHER_PROP_FORCE_RESIZE_HSTEPS:
      self->resize_hsteps = g_value_get_double (value);
      break;
    case PATCHER_PROP_FORCE_RESIZE_VSTEPS:
      self->resize_vsteps = g_value_get_double (value);
      break;
    case PATCHER_PROP_MIN_RESIZE_HUNITS:
      self->resize_hunits = g_value_get_double (value);
      break;
    case PATCHER_PROP_MIN_RESIZE_VUNITS:
      self->resize_vunits = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}
static void
gxk_widget_patcher_finalize (GObject *object)
{
  GxkWidgetPatcher *self = GXK_WIDGET_PATCHER (object);
  g_free (self->tooltip);
  G_OBJECT_CLASS (widget_patcher_parent_class)->finalize (object);
}
static void
gxk_widget_patcher_class_init (GxkWidgetPatcherClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  widget_patcher_parent_class = g_type_class_peek_parent (class);
  gobject_class->set_property = gxk_widget_patcher_set_property;
  gobject_class->finalize = gxk_widget_patcher_finalize;
  g_object_class_install_property (gobject_class, PATCHER_PROP_TOOLTIP,
                                   g_param_spec_string ("tooltip", NULL, NULL, NULL, G_PARAM_WRITABLE));
  g_object_class_install_property (gobject_class, PATCHER_PROP_TOOLTIP_VISIBLE,
                                   g_param_spec_boolean ("tooltip_visible", NULL, NULL,
                                                         TRUE, G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (gobject_class, PATCHER_PROP_MUTE_EVENTS,
                                   g_param_spec_boolean ("mute-events", NULL, NULL, FALSE, G_PARAM_WRITABLE));
  g_object_class_install_property (gobject_class, PATCHER_PROP_LOWER_WINDOWS,
                                   g_param_spec_boolean ("lower-windows", NULL, NULL, FALSE, G_PARAM_WRITABLE));
  g_object_class_install_property (gobject_class, PATCHER_PROP_HIDE_INSENSITIVE,
                                   g_param_spec_boolean ("hide-insensitive", NULL, NULL, FALSE, G_PARAM_WRITABLE));
  g_object_class_install_property (gobject_class, PATCHER_PROP_WIDTH_FROM_HEIGHT,
                                   g_param_spec_double ("width_from_height", NULL, NULL, -G_MAXDOUBLE, G_MAXDOUBLE, 0, G_PARAM_WRITABLE));
  g_object_class_install_property (gobject_class, PATCHER_PROP_HEIGHT_FROM_WIDTH,
                                   g_param_spec_double ("height_from_width", NULL, NULL, -G_MAXDOUBLE, G_MAXDOUBLE, 0, G_PARAM_WRITABLE));
  g_object_class_install_property (gobject_class, PATCHER_PROP_FORCE_RESIZE_HSTEPS,
                                   g_param_spec_double ("force-resize-hsteps", NULL, NULL, -G_MAXDOUBLE, G_MAXDOUBLE, 0, G_PARAM_WRITABLE));
  g_object_class_install_property (gobject_class, PATCHER_PROP_FORCE_RESIZE_VSTEPS,
                                   g_param_spec_double ("force-resize-vsteps", NULL, NULL, -G_MAXDOUBLE, G_MAXDOUBLE, 0, G_PARAM_WRITABLE));
  g_object_class_install_property (gobject_class, PATCHER_PROP_MIN_RESIZE_HUNITS,
                                   g_param_spec_double ("min-resize-hunits", NULL, NULL, -G_MAXDOUBLE, G_MAXDOUBLE, 0, G_PARAM_WRITABLE));
  g_object_class_install_property (gobject_class, PATCHER_PROP_MIN_RESIZE_VUNITS,
                                   g_param_spec_double ("min-resize-vunits", NULL, NULL, -G_MAXDOUBLE, G_MAXDOUBLE, 0, G_PARAM_WRITABLE));
}
static void
widget_patcher_height_from_width (GtkWidget      *widget,
                                  GtkRequisition *requisition)
{
  gdouble factor = g_object_get_double (widget, "height-from-width");
  if (factor)
    requisition->height = requisition->width * factor;
}
static void
widget_patcher_width_from_height (GtkWidget      *widget,
                                  GtkRequisition *requisition)
{
  gdouble factor = g_object_get_double (widget, "width-from-height");
  if (factor)
    requisition->width = requisition->height * factor;
}
static gboolean
widget_mute_events (GtkWidget *widget,
                    GdkEvent  *event)
{
  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
    case GDK_2BUTTON_PRESS:
    case GDK_3BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
    case GDK_MOTION_NOTIFY:
    case GDK_KEY_PRESS:
    case GDK_KEY_RELEASE:
    case GDK_ENTER_NOTIFY:
    case GDK_LEAVE_NOTIFY:
      return TRUE;
    default:
      return FALSE;
    }
}
static void
widget_lower_windows (GtkWidget *widget)
{
  GList *list = gdk_window_peek_children (widget->window);
  for (; list; list = list->next)
    {
      gpointer user_data;
      gdk_window_get_user_data (list->data, &user_data);
      if (user_data == (gpointer) widget)
        gdk_window_lower (list->data);
    }
}
static void
widget_hide_insensitive (GtkWidget *widget)
{
  if (!GTK_WIDGET_SENSITIVE (widget) && GTK_WIDGET_VISIBLE (widget))
    gtk_widget_hide (widget);
  else if (GTK_WIDGET_SENSITIVE (widget) && !GTK_WIDGET_VISIBLE (widget))
    gtk_widget_show (widget);
}
static void
widget_patcher_hint_resize_inc (GtkWidget *widget)
{
  GtkWidget *window = gtk_widget_get_toplevel (widget);
  if (GTK_IS_WINDOW (window))
    {
      gdouble resize_hsteps = g_object_get_double (widget, "gxk-resize-hsteps");
      gdouble resize_vsteps = g_object_get_double (widget, "gxk-resize-vsteps");
      gdouble resize_hunits = g_object_get_double (widget, "gxk-resize-hunits");
      gdouble resize_vunits = g_object_get_double (widget, "gxk-resize-vunits");
      gint child_width = widget->requisition.width;
      gint child_height = widget->requisition.height;
      if (resize_hsteps)
        gxk_window_set_geometry_width_inc (GTK_WINDOW (window), resize_hsteps * child_width);
      if (resize_vsteps)
        gxk_window_set_geometry_height_inc (GTK_WINDOW (window), resize_vsteps * child_height);
      if (resize_hunits)
        gxk_window_set_geometry_min_width (GTK_WINDOW (window), resize_hunits * child_width);
      if (resize_vunits)
        gxk_window_set_geometry_min_height (GTK_WINDOW (window), resize_vunits * child_height);
    }
}
static gboolean
widget_patcher_adopt (GxkRadget          *radget,
                      GxkRadget          *parent,
                      GxkRadgetData      *gdgdata)
{
  GxkWidgetPatcher *self = GXK_WIDGET_PATCHER (radget);
  if (self->tooltip)
    {
      gxk_widget_set_latent_tooltip (parent, self->tooltip);
      if (self->tooltip_visible)
        gtk_tooltips_set_tip (GXK_TOOLTIPS, parent, self->tooltip, NULL);
    }
  if (self->mute_events &&
      !gxk_signal_handler_pending (parent, "event", G_CALLBACK (widget_mute_events), NULL))
    g_object_connect (parent, "signal::event", widget_mute_events, NULL, NULL);
  if (self->lower_windows &&
      !gxk_signal_handler_pending (parent, "map", G_CALLBACK (widget_lower_windows), NULL))
    g_object_connect (parent, "signal_after::map", widget_lower_windows, NULL, NULL);
  if (self->hide_insensitive &&
      !gxk_signal_handler_pending (parent, "state-changed", G_CALLBACK (widget_hide_insensitive), NULL))
    g_object_connect (parent, "signal_after::state-changed", widget_hide_insensitive, NULL, NULL);
  if (self->width_from_height &&
      !gxk_signal_handler_pending (parent, "size-request", G_CALLBACK (widget_patcher_width_from_height), NULL))
    g_object_connect (parent, "signal_after::size-request", widget_patcher_width_from_height, NULL, NULL);
  g_object_set_double (parent, "width-from-height", self->width_from_height);
  if (self->height_from_width &&
      !gxk_signal_handler_pending (parent, "size-request", G_CALLBACK (widget_patcher_height_from_width), NULL))
    g_object_connect (parent, "signal_after::size-request", widget_patcher_height_from_width, NULL, NULL);
  g_object_set_double (parent, "height-from-width", self->height_from_width);
  if ((self->resize_hsteps || self->resize_vsteps ||
       self->resize_hunits || self->resize_vunits) && !GTK_IS_WINDOW (parent))
    g_object_connect (parent, "signal_after::size-allocate", widget_patcher_hint_resize_inc, NULL, NULL);
  g_object_set_double (parent, "gxk-resize-hsteps", self->resize_hsteps);
  g_object_set_double (parent, "gxk-resize-vsteps", self->resize_vsteps);
  g_object_set_double (parent, "gxk-resize-hunits", self->resize_hunits);
  g_object_set_double (parent, "gxk-resize-vunits", self->resize_vunits);
  g_idle_add (widget_patcher_unref, self);      /* takes over initial ref count */
  return FALSE; /* no support for packing options */
}

GType
gxk_widget_patcher_get_type (void)
{
  static GType type = 0;
  if (!type)
    {
      static const GTypeInfo type_info = {
        sizeof (GxkWidgetPatcherClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gxk_widget_patcher_class_init,
        NULL,   /* class_finalize */
        NULL,   /* class_data */
        sizeof (GxkWidgetPatcher),
        0,      /* n_preallocs */
        (GInstanceInitFunc) NULL,
      };
      type = g_type_register_static (G_TYPE_OBJECT, "GxkWidgetPatcher", &type_info, 0);
    }
  return type;
}

static GxkRadget*
widget_patcher_create (GType               type,
                       const gchar        *name,
                       GxkRadgetData      *gdgdata)
{
  return g_object_new (type, NULL);
}

static GParamSpec*
widget_patcher_find_prop (GxkRadget    *radget,
                          const gchar  *prop_name)
{
  return g_object_class_find_property (G_OBJECT_GET_CLASS (radget), prop_name);
}

static const GxkRadgetType widget_patcher_def = {
  widget_patcher_create,
  widget_patcher_find_prop,
  (void(*)(GxkRadget*,const gchar*,const GValue*)) g_object_set_property,
  widget_patcher_adopt,
  NULL,         /* find_pack */
  NULL,         /* set_pack */
};
const GxkRadgetType *_gxk_widget_patcher_def = &widget_patcher_def;


/* --- focus frame --- */
G_DEFINE_TYPE (GxkFocusFrame, gxk_focus_frame, GTK_TYPE_FRAME);

static gboolean
focus_frame_expose_event (GtkWidget      *widget,
                          GdkEventExpose *event)
{
  GtkFrame *frame = GTK_FRAME (widget);
  gint x = frame->child_allocation.x - widget->style->xthickness;
  gint y = frame->child_allocation.y - widget->style->ythickness;
  gint width = frame->child_allocation.width + 2 * widget->style->xthickness;
  gint height =  frame->child_allocation.height + 2 * widget->style->ythickness;
  if (frame->shadow_type != GTK_SHADOW_NONE)
    gtk_paint_focus (widget->style, widget->window, GTK_WIDGET_STATE (widget),
                     &event->area, widget, "button", x, y, width, height);
  if (0)
    gtk_paint_shadow (widget->style, widget->window, GTK_STATE_NORMAL, frame->shadow_type,
                      &event->area, widget, "frame", x, y, width, height);
  /* skip normal frame drawing code */
  return GTK_WIDGET_CLASS (g_type_class_peek (g_type_parent (GTK_TYPE_FRAME)))->expose_event (widget, event);
}

static void
gxk_focus_frame_init (GxkFocusFrame *self)
{
  GTK_WIDGET_SET_FLAGS (self, GTK_VISIBLE);
  gtk_container_set_border_width (GTK_CONTAINER (self), 1);
}

static void
gxk_focus_frame_class_init (GxkFocusFrameClass *class)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  widget_class->expose_event = focus_frame_expose_event;
}

/* --- back shade --- */
G_DEFINE_TYPE (GxkBackShade, gxk_back_shade, GTK_TYPE_ALIGNMENT);

/* color converion code stolen from gtkstyle.c */
static void
rgb_to_hls (gdouble  red,
            gdouble  green,
            gdouble  blue,
            gdouble *h_p,	/* 0..360 */
            gdouble *l_p,	/* 0..1 */
            gdouble *s_p)	/* 0..1 */
{
  gdouble max = MAX (MAX (red, green), blue);
  gdouble min = MIN (MIN (red, green), blue);
  gdouble l = (max + min) / 2.;
  gdouble h = 0, s = 0;
  if (max != min)
    {
      gdouble delta = max - min;
      s = delta / (l <= 0.5 ? max + min : 2 - max - min);
      if (delta == 0.0)
        delta = 1.0;
      if (red == max)
        h = (green - blue) / delta;
      else if (green == max)
        h = 2 + (blue - red) / delta;
      else if (blue == max)
        h = 4 + (red - green) / delta;
      h *= 60;
      if (h < 0.0)
        h += 360;
    }
  *h_p = h;
  *l_p = l;
  *s_p = s;
}

static void
hls_to_rgb (gdouble  hue,		/* 0..360 */
            gdouble  lightness,		/* 0..1 */
            gdouble  saturation,	/* 0..1 */
            gdouble *r_p,
            gdouble *g_p,
            gdouble *b_p)
{
  if (saturation == 0)
    {
      *r_p = lightness;
      *g_p = lightness;
      *b_p = lightness;
    }
  else
    {
      gdouble m2, chue;
      if (lightness <= 0.5)
        m2 = lightness * (1 + saturation);
      else
        m2 = lightness + saturation - lightness * saturation;
      gdouble m1 = 2 * lightness - m2;

      chue = hue + 120;
      while (chue > 360)
        chue -= 360;
      while (chue < 0)
        chue += 360;
      if (chue < 60)
        *r_p = m1 + (m2 - m1) * chue / 60;
      else if (chue < 180)
        *r_p = m2;
      else if (chue < 240)
        *r_p = m1 + (m2 - m1) * (240 - chue) / 60;
      else
        *r_p = m1;

      chue = hue;
      while (chue > 360)
        chue -= 360;
      while (chue < 0)
        chue += 360;
      if (chue < 60)
        *g_p = m1 + (m2 - m1) * chue / 60;
      else if (chue < 180)
        *g_p = m2;
      else if (chue < 240)
        *g_p = m1 + (m2 - m1) * (240 - chue) / 60;
      else
        *g_p = m1;

      chue = hue - 120;
      while (chue > 360)
        chue -= 360;
      while (chue < 0)
        chue += 360;
      if (chue < 60)
        *b_p = m1 + (m2 - m1) * chue / 60;
      else if (chue < 180)
        *b_p = m2;
      else if (chue < 240)
        *b_p = m1 + (m2 - m1) * (240 - chue) / 60;
      else
        *b_p = m1;
    }
}

static void
color_shade (GdkColor *color,
             gdouble   k)
{
  gdouble r = color->red / 65535.;
  gdouble g = color->green / 65535.;
  gdouble b = color->blue / 65535.;
  gdouble h, l, s;
  rgb_to_hls (r, g, b, &h, &l, &s);
  l = CLAMP (l * k, 0, 1);
  s = CLAMP (s * k, 0, 1);
  hls_to_rgb (h, l, s, &r, &g, &b);
  color->red = r * 65535.;
  color->green = g * 65535.;
  color->blue = b * 65535.;
}

static GdkGC*
get_darkened_gc (GdkWindow *window,
                 GdkColor  *srccolor,
                 gint       darken_count)
{
  GdkColor color = *srccolor;
  while (darken_count--)
    color_shade (&color, 0.93);
  GdkGC *gc = gdk_gc_new (window);
  gdk_gc_set_rgb_fg_color (gc, &color);
  return gc;
}

static gboolean
back_shade_expose_event (GtkWidget      *widget,
                         GdkEventExpose *event)
{
  GdkWindow *window = widget->window;
  GtkStyle *style = widget->style;
  GdkGC *gc = get_darkened_gc (window, &style->bg[GTK_STATE_NORMAL], 1);
  GdkRectangle *area = &event->area;
  gdk_gc_set_clip_rectangle (gc, area);
  gdk_draw_rectangle (window, gc, TRUE, area->x, area->y, area->width, area->height);
  gdk_gc_set_clip_rectangle (gc, NULL);
  g_object_unref (gc);
  /* chain to parent */
  return GTK_WIDGET_CLASS (gxk_back_shade_parent_class)->expose_event (widget, event);
}

static void
gxk_back_shade_init (GxkBackShade *self)
{
  GTK_WIDGET_SET_FLAGS (self, GTK_VISIBLE);
}

static void
gxk_back_shade_class_init (GxkBackShadeClass *class)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  widget_class->expose_event = back_shade_expose_event;
}

