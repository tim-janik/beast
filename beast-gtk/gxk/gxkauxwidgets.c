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
#include <gdk/gdkkeysyms.h>

/* --- GxkMenuItem --- */
enum {
  MENU_ITEM_PROP_0,
  MENU_ITEM_PROP_ULINE_LABEL,
  MENU_ITEM_PROP_STOCK_IMAGE,
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
          GtkWidget *image = gtk_image_new_from_stock (string, GTK_ICON_SIZE_MENU);
          if (image)
            {
              gtk_widget_show (image);
              gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (self), image);
            }
        }
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
gxk_menu_item_class_init (GxkMenuItemClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  gobject_class->set_property = gxk_menu_item_set_property;
  gobject_class->get_property = gxk_menu_item_get_property;
  g_object_class_install_property (gobject_class, MENU_ITEM_PROP_ULINE_LABEL,
                                   g_param_spec_string ("uline-label", NULL, NULL, NULL, G_PARAM_WRITABLE));
  g_object_class_install_property (gobject_class, MENU_ITEM_PROP_STOCK_IMAGE,
                                   g_param_spec_string ("stock-image", NULL, NULL, NULL, G_PARAM_WRITABLE));
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
        (GInstanceInitFunc) NULL,
      };
      type = g_type_register_static (GTK_TYPE_IMAGE_MENU_ITEM, "GxkMenuItem", &type_info, 0);
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
  PATCHER_PROP_WIDTH_FROM_HEIGHT,
  PATCHER_PROP_HEIGHT_FROM_WIDTH
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
    case PATCHER_PROP_WIDTH_FROM_HEIGHT:
      self->width_from_height = g_value_get_double (value);
      break;
    case PATCHER_PROP_HEIGHT_FROM_WIDTH:
      self->height_from_width = g_value_get_double (value);
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
  g_object_class_install_property (gobject_class, PATCHER_PROP_WIDTH_FROM_HEIGHT,
                                   g_param_spec_double ("width_from_height", NULL, NULL, -G_MAXDOUBLE, G_MAXDOUBLE, 0, G_PARAM_WRITABLE));
  g_object_class_install_property (gobject_class, PATCHER_PROP_HEIGHT_FROM_WIDTH,
                                   g_param_spec_double ("height_from_width", NULL, NULL, -G_MAXDOUBLE, G_MAXDOUBLE, 0, G_PARAM_WRITABLE));
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
static gboolean
widget_patcher_adopt (GxkGadget          *gadget,
                      GxkGadget          *parent,
                      GxkGadgetData      *gdgdata)
{
  GxkWidgetPatcher *self = GXK_WIDGET_PATCHER (gadget);
  if (self->tooltip)
    {
      g_object_set_data_full (parent, "gxk-widget-patcher-latent-tooltip", g_strdup (self->tooltip), g_free);
      if (self->tooltip_visible)
        gtk_tooltips_set_tip (GXK_TOOLTIPS, parent, self->tooltip, NULL);
    }
  if (self->mute_events &&
      !gxk_signal_handler_pending (parent, "event", G_CALLBACK (widget_mute_events), NULL))
    g_object_connect (parent, "signal::event", widget_mute_events, NULL, NULL);
  if (self->lower_windows &&
      !gxk_signal_handler_pending (parent, "map", G_CALLBACK (widget_lower_windows), NULL))
    g_object_connect (parent, "signal_after::map", widget_lower_windows, NULL, NULL);
  if (self->width_from_height &&
      !gxk_signal_handler_pending (parent, "size-request", G_CALLBACK (widget_patcher_width_from_height), NULL))
    g_object_connect (parent, "signal_after::size-request", widget_patcher_width_from_height, NULL, NULL);
  g_object_set_double (parent, "width-from-height", self->width_from_height);
  if (self->height_from_width &&
      !gxk_signal_handler_pending (parent, "size-request", G_CALLBACK (widget_patcher_height_from_width), NULL))
    g_object_connect (parent, "signal_after::size-request", widget_patcher_height_from_width, NULL, NULL);
  g_object_set_double (parent, "height-from-width", self->height_from_width);
  g_idle_add (widget_patcher_unref, self);      /* takes over initial ref count */
  return FALSE; /* no support for packing options */
}

const gchar*
gxk_widget_get_latent_tooltip (GtkWidget *widget)
{
  return g_object_get_data (widget, "gxk-widget-patcher-latent-tooltip");
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

static GxkGadget*
widget_patcher_create (GType               type,
                       const gchar        *name,
                       GxkGadgetData      *gdgdata)
{
  return g_object_new (type, NULL);
}

static GParamSpec*
widget_patcher_find_prop (GxkGadget    *gadget,
                          const gchar  *prop_name)
{
  return g_object_class_find_property (G_OBJECT_GET_CLASS (gadget), prop_name);
}

static const GxkGadgetType widget_patcher_def = {
  widget_patcher_create,
  widget_patcher_find_prop,
  (void(*)(GxkGadget*,const gchar*,const GValue*)) g_object_set_property,
  widget_patcher_adopt,
  NULL,         /* find_pack */
  NULL,         /* set_pack */
};
const GxkGadgetType *_gxk_widget_patcher_def = &widget_patcher_def;


/* --- GxkMenuButton --- */
enum {
  MENU_BUTTON_PROP_0,
  MENU_BUTTON_PROP_MENU,
  MENU_BUTTON_PROP_COMBO_ARROW,
  MENU_BUTTON_PROP_STOCK_SIZE
};

static void     menu_button_remove_contents          (GxkMenuButton       *self);
static void
menu_button_popup (GxkMenuButton *self,
                   guint          button,
                   guint32        time)
{
  GdkEvent *event = gtk_get_current_event ();
  GtkWidget *menu_item, *widget = GTK_WIDGET (self);
  gint x, y;
  gtk_widget_grab_focus (widget);
  /* fixate sizes across removing child */
  g_object_set (self->islot,
                "width-request", self->islot->requisition.width,
                "height-request", self->islot->requisition.height,
                NULL);
  menu_button_remove_contents (self);
  gdk_window_get_origin (widget->window, &x, &y);
  x += widget->allocation.x;
  y += widget->allocation.y;
  gxk_menu_popup (self->menu, x, y, FALSE, button, event ? gdk_event_get_time (event) : 0);
  menu_item = gtk_menu_get_active (self->menu);
  if (menu_item)
    gtk_menu_shell_select_item (GTK_MENU_SHELL (self->menu), menu_item);
}
static gboolean
menu_button_button_press (GtkWidget      *widget,
                          GdkEventButton *event)
{
  GxkMenuButton *self = GXK_MENU_BUTTON (widget);
  gtk_widget_grab_focus (GTK_WIDGET (self));
  if (self->menu && event->type == GDK_BUTTON_PRESS && event->button == 1)
    menu_button_popup (self, event->button, event->time);
  return TRUE;
}
static gboolean
menu_button_key_press (GtkWidget   *widget,
                       GdkEventKey *event)
{
  GxkMenuButton *self = GXK_MENU_BUTTON (widget);
  switch (event->keyval)
    {
    case GDK_KP_Space: case GDK_space:
      menu_button_popup (self, 0, event->time);
      return TRUE;
    }
  return FALSE;
}
static gboolean
menu_button_mnemonic_activate (GtkWidget *widget,
                               gboolean   group_cycling)
{
  gtk_widget_grab_focus (widget);
  return TRUE;
}

static void
menu_button_proxy_state (GxkMenuButton *self)
{
  if (self->child)
    gtk_widget_set_sensitive (self->child, GTK_WIDGET_IS_SENSITIVE (self->menu_item));
  if (self->image)
    gtk_widget_set_sensitive (self->image, GTK_WIDGET_IS_SENSITIVE (self->menu_item));
}
static void
menu_button_max_size (GxkMenuButton *self)
{
  if (self->child)
    {
      GList *list, *children = GTK_MENU_SHELL (self->menu)->children;
      GtkRequisition child_requisition = { 0, };
      guint width = 0, height = 0;
      for (list = children; list; list = list->next)
        {
          GtkWidget *mitem = list->data;
          if (GTK_WIDGET_VISIBLE (mitem))
            {
              GtkWidget *child = GTK_BIN (mitem)->child;
              if (child && GTK_WIDGET_VISIBLE (child))
                {
                  gtk_widget_size_request (child, &child_requisition);
                  width = MAX (width, child_requisition.width);
                  height = MAX (height, child_requisition.height);
                }
            }
        }
      gtk_widget_size_request (self->child, &child_requisition);
      width = MAX (width, child_requisition.width);
      height = MAX (height, child_requisition.height);
      g_object_set (self->cslot,
                    "width-request", width,
                    "height-request", height,
                    NULL);
    }
}
static void
menu_button_remove_contents (GxkMenuButton *self)
{
  if (self->menu_item)
    {
      if (self->child)
        {
          gtk_widget_set_sensitive (self->child, TRUE);
          gtk_widget_reparent (self->child, self->menu_item);
          self->child = NULL;
        }
      if (self->image)
        {
          g_object_ref (self->image);
          g_object_set (self->image, "sensitive", TRUE, "icon-size", self->old_icon_size, NULL);
          gtk_container_remove (GTK_CONTAINER (self->image->parent), self->image);
          gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (self->menu_item), self->image);
          g_object_unref (self->image);
        }
      self->image = NULL;
      g_signal_handlers_disconnect_by_func (self->menu_item, menu_button_proxy_state, self);
      g_object_unref (self->menu_item);
      self->menu_item = NULL;
      gtk_tooltips_set_tip (GXK_TOOLTIPS, GTK_WIDGET (self), NULL, NULL);
    }
}
void
gxk_menu_button_update (GxkMenuButton *self)
{
  if (self->menu)
    {
      menu_button_remove_contents (self);
      self->menu_item = gtk_menu_get_active (self->menu);
      if (self->menu_item)
        {
          GtkTooltipsData *tipdata;
          g_object_ref (self->menu_item);
          self->child = GTK_BIN (self->menu_item)->child;
          if (self->child)
            gtk_widget_reparent (self->child, self->cslot);
          if (GTK_IS_IMAGE_MENU_ITEM (self->menu_item))
            self->image = gtk_image_menu_item_get_image (GTK_IMAGE_MENU_ITEM (self->menu_item));
          if (self->image)
            {
              g_object_get (self->image, "icon-size", &self->old_icon_size, NULL);
              gtk_widget_reparent (self->image, self->islot);
              if (self->icon_size)
                g_object_set (self->image, "icon-size", self->icon_size, NULL);
            }
          g_object_connect (self->menu_item, "swapped_signal::state_changed", menu_button_proxy_state, self, NULL);
          menu_button_proxy_state (self);
          gtk_widget_queue_resize (GTK_WIDGET (self));
          tipdata = gtk_tooltips_data_get (self->menu_item);
          if (tipdata && tipdata->tip_text)
            gtk_tooltips_set_tip (GXK_TOOLTIPS, GTK_WIDGET (self), tipdata->tip_text, tipdata->tip_private);
          else
            gtk_tooltips_set_tip (GXK_TOOLTIPS, GTK_WIDGET (self),
                                  gxk_widget_get_latent_tooltip (self->menu_item), NULL);
          /* restore slot sizes */
          g_object_set (self->islot,
                        "width-request", -1,
                        "height-request", -1,
                        NULL);
          menu_button_max_size (self);
        }
    }
}

static void
menu_button_detacher (GtkWidget *widget,
                      GtkMenu   *menu)
{
  GxkMenuButton *self = GXK_MENU_BUTTON (widget);
  menu_button_remove_contents (self);
  g_signal_handlers_disconnect_by_func (self->menu, gxk_menu_button_update, self);
  g_signal_handlers_disconnect_by_func (self->menu, menu_button_max_size, self);
  self->menu = NULL;
  g_object_notify (self, "menu");
}

static void
gxk_menu_button_set_property (GObject      *object,
                              guint         param_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GxkMenuButton *self = GXK_MENU_BUTTON (object);
  switch (param_id)
    {
    const gchar *cstr;
    case MENU_BUTTON_PROP_MENU:
      if (self->menu)
        gtk_menu_detach (self->menu);
      self->menu = g_value_get_object (value);
      if (self->menu)
        {
          gxk_menu_attach_as_popup_with_func (self->menu, GTK_WIDGET (self), menu_button_detacher);
          g_object_connect (self->menu,
                            "swapped_signal_after::selection_done", gxk_menu_button_update, self,
                            "swapped_signal_after::size-request", menu_button_max_size, self,
                            NULL);
          gtk_widget_queue_resize (GTK_WIDGET (self));
          gxk_menu_button_update (self);
        }
      break;
    case MENU_BUTTON_PROP_COMBO_ARROW:
      if (g_value_get_boolean (value))
        gtk_box_pack_end (GTK_BOX (GTK_BIN (self)->child),
                          g_object_new (GTK_TYPE_ARROW,
                                        "visible", TRUE,
                                        "arrow-type", GTK_ARROW_DOWN,
                                        NULL),
                          FALSE, TRUE, 0);
      break;
    case MENU_BUTTON_PROP_STOCK_SIZE:
      cstr = g_value_get_string (value);
      if (cstr)
        self->icon_size = gtk_icon_size_from_name (cstr);
      else
        self->icon_size = 0;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}
static void
gxk_menu_button_get_property (GObject    *object,
                              guint       param_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GxkMenuButton *self = GXK_MENU_BUTTON (object);
  switch (param_id)
    {
    case MENU_BUTTON_PROP_MENU:
      g_value_set_object (value, self->menu);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}
static void
gxk_menu_button_init (GxkMenuButton *self)
{
  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  self->islot = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_box_pack_start (GTK_BOX (vbox), self->islot, TRUE, TRUE, 0);
  self->cslot = gtk_alignment_new (0.5, 0.5, 0, 0);
  gtk_box_pack_end (GTK_BOX (vbox), self->cslot, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (self), hbox);
  gtk_widget_show_all (GTK_WIDGET (self));
}
static gpointer menu_button_parent_class = NULL;
static void
menu_button_dispose (GObject *object)
{
  GxkMenuButton *self = GXK_MENU_BUTTON (object);
  if (self->menu)
    gtk_menu_detach (self->menu);
  G_OBJECT_CLASS (menu_button_parent_class)->dispose (object);
}

static void
gxk_menu_button_class_init (GxkMenuButtonClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  menu_button_parent_class = g_type_class_peek_parent (class);
  gobject_class->set_property = gxk_menu_button_set_property;
  gobject_class->get_property = gxk_menu_button_get_property;
  gobject_class->dispose = menu_button_dispose;
  g_object_class_install_property (gobject_class, MENU_BUTTON_PROP_MENU,
                                   g_param_spec_object ("menu", NULL, NULL, GTK_TYPE_MENU, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, MENU_BUTTON_PROP_COMBO_ARROW,
                                   g_param_spec_boolean ("combo-arrow", NULL, NULL, FALSE, G_PARAM_WRITABLE));
  g_object_class_install_property (gobject_class, MENU_BUTTON_PROP_STOCK_SIZE,
                                   g_param_spec_string ("stock-size", NULL, NULL, NULL, G_PARAM_WRITABLE));
  widget_class->button_press_event = menu_button_button_press;
  widget_class->key_press_event = menu_button_key_press;
  widget_class->mnemonic_activate = menu_button_mnemonic_activate;
}

GType
gxk_menu_button_get_type (void)
{
  static GType type = 0;
  if (!type)
    {
      static const GTypeInfo type_info = {
        sizeof (GxkMenuButtonClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gxk_menu_button_class_init,
        NULL,   /* class_finalize */
        NULL,   /* class_data */
        sizeof (GxkMenuButton),
        0,      /* n_preallocs */
        (GInstanceInitFunc) gxk_menu_button_init,
      };
      type = g_type_register_static (GTK_TYPE_BUTTON, "GxkMenuButton", &type_info, 0);
    }
  return type;
}
