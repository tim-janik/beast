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
#ifndef __GXK_AUX_WIDGETS_H__
#define __GXK_AUX_WIDGETS_H__

#include "gxkutils.h"
#include "gxkgadget.h"

G_BEGIN_DECLS


/* --- menu item --- */
#define GXK_TYPE_MENU_ITEM              (gxk_menu_item_get_type ())
#define GXK_MENU_ITEM(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GXK_TYPE_MENU_ITEM, GxkMenuItem))
#define GXK_MENU_ITEM_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_MENU_ITEM, GxkMenuItemClass))
#define GXK_IS_MENU_ITEM(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GXK_TYPE_MENU_ITEM))
#define GXK_IS_MENU_ITEM_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_MENU_ITEM))
#define GXK_MENU_ITEM_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GXK_TYPE_MENU_ITEM, GxkMenuItemClass))
typedef GtkImageMenuItem      GxkMenuItem;
typedef GtkImageMenuItemClass GxkMenuItemClass;
GType   gxk_menu_item_get_type          (void);


/* --- image --- */
#define GXK_TYPE_IMAGE              (gxk_image_get_type ())
#define GXK_IMAGE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GXK_TYPE_IMAGE, GxkImage))
#define GXK_IMAGE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_IMAGE, GxkImageClass))
#define GXK_IS_IMAGE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GXK_TYPE_IMAGE))
#define GXK_IS_IMAGE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_IMAGE))
#define GXK_IMAGE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GXK_TYPE_IMAGE, GxkImageClass))
typedef GtkImage      GxkImage;
typedef GtkImageClass GxkImageClass;
GType   gxk_image_get_type              (void);


/* --- widget-patcher --- */
#define GXK_TYPE_WIDGET_PATCHER              (gxk_widget_patcher_get_type ())
#define GXK_WIDGET_PATCHER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GXK_TYPE_WIDGET_PATCHER, GxkWidgetPatcher))
#define GXK_WIDGET_PATCHER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_WIDGET_PATCHER, GxkWidgetPatcherClass))
#define GXK_IS_WIDGET_PATCHER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GXK_TYPE_WIDGET_PATCHER))
#define GXK_IS_WIDGET_PATCHER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_WIDGET_PATCHER))
#define GXK_WIDGET_PATCHER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GXK_TYPE_WIDGET_PATCHER, GxkWidgetPatcherClass))
typedef GObjectClass GxkWidgetPatcherClass;
typedef struct {
  GObject parent_instance;
  gchar  *tooltip;
  guint   tooltip_visible : 1;
  guint   mute_events : 1;
  guint   lower_windows : 1;
  gdouble width_from_height;
  gdouble height_from_width;
} GxkWidgetPatcher;
GType   gxk_widget_patcher_get_type     (void);
extern const GxkGadgetType *_gxk_widget_patcher_def;

const gchar*    gxk_widget_get_latent_tooltip (GtkWidget *widget);


/* --- menu button --- */
#define GXK_TYPE_MENU_BUTTON              (gxk_menu_button_get_type ())
#define GXK_MENU_BUTTON(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GXK_TYPE_MENU_BUTTON, GxkMenuButton))
#define GXK_MENU_BUTTON_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_MENU_BUTTON, GxkMenuButtonClass))
#define GXK_IS_MENU_BUTTON(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GXK_TYPE_MENU_BUTTON))
#define GXK_IS_MENU_BUTTON_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_MENU_BUTTON))
#define GXK_MENU_BUTTON_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GXK_TYPE_MENU_BUTTON, GxkMenuButtonClass))
typedef struct {
  GtkButton  parent_instance;
  GtkWidget *cslot, *islot;
  GtkMenu   *menu;
  GtkWidget *menu_item;
  GtkWidget *image;
  gint       icon_size, old_icon_size;
  gint       width, height;
  GtkWidget *child;
} GxkMenuButton;
typedef GtkButtonClass GxkMenuButtonClass;
GType   gxk_menu_button_get_type          (void);
void    gxk_menu_button_update            (GxkMenuButton *self);


G_END_DECLS

#endif /* __GXK_AUX_WIDGETS_H__ */
