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
        g_object_new (GTK_TYPE_ACCEL_LABEL,
                      "visible", TRUE,
                      "label", string,
                      "use-underline", TRUE,
                      "xalign", 0.0,
                      "accel-widget", self,
                      "parent", self,
                      NULL);
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
static gpointer gxk_widget_patcher_parent_class = NULL;
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
  PATCHER_PROP_WIDTH_FROM_HEIGHT,
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
    case PATCHER_PROP_WIDTH_FROM_HEIGHT:
      self->width_from_height = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}
static void
gxk_widget_patcher_finlize (GObject *object)
{
  GxkWidgetPatcher *self = GXK_WIDGET_PATCHER (object);
  g_free (self->tooltip);
  G_OBJECT_CLASS (gxk_widget_patcher_parent_class)->finalize (object);
}
static void
gxk_widget_patcher_class_init (GxkWidgetPatcherClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  gxk_widget_patcher_parent_class = g_type_class_peek_parent (class);
  gobject_class->set_property = gxk_widget_patcher_set_property;
  gobject_class->finalize = gxk_widget_patcher_finlize;
  g_object_class_install_property (gobject_class, PATCHER_PROP_TOOLTIP,
                                   g_param_spec_string ("tooltip", NULL, NULL, NULL, G_PARAM_WRITABLE));
  g_object_class_install_property (gobject_class, PATCHER_PROP_WIDTH_FROM_HEIGHT,
                                   g_param_spec_boolean ("width_from_height", NULL, NULL, FALSE, G_PARAM_WRITABLE));
}
static void
widget_patcher_width_from_height (GtkWidget      *widget,
                                  GtkRequisition *requisition)
{
  requisition->width = requisition->height;
}
static gboolean
widget_patcher_adopt (GxkGadget          *gadget,
                      GxkGadget          *parent,
                      GxkGadgetData      *gdgdata)
{
  GxkWidgetPatcher *self = GXK_WIDGET_PATCHER (gadget);
  if (self->tooltip)
    gtk_tooltips_set_tip (GXK_TOOLTIPS, parent, self->tooltip, NULL);
  if (self->width_from_height)
    g_object_connect (parent, "signal_after::size_request", widget_patcher_width_from_height, NULL, NULL);
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
