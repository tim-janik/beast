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
#include "gxkradget.h"

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

/* --- free radio button --- */
#define GXK_TYPE_FREE_RADIO_BUTTON            (gxk_free_radio_button_get_type ())
#define GXK_FREE_RADIO_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GXK_TYPE_FREE_RADIO_BUTTON, GxkFreeRadioButton))
#define GXK_FREE_RADIO_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_FREE_RADIO_BUTTON, GxkFreeRadioButtonClass))
#define GXK_IS_FREE_RADIO_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GXK_TYPE_FREE_RADIO_BUTTON))
#define GXK_IS_FREE_RADIO_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_FREE_RADIO_BUTTON))
#define GXK_FREE_RADIO_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GXK_TYPE_FREE_RADIO_BUTTON, GxkFreeRadioButtonClass))
typedef GtkRadioButton      GxkFreeRadioButton;
typedef GtkRadioButtonClass GxkFreeRadioButtonClass;
GType      gxk_free_radio_button_get_type (void);
GtkWidget* gxk_free_radio_button_new      (void);

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
  guint   hide_insensitive : 1;
  guint   modify_normal_bg_as_base : 1;
  guint   modify_bg_as_base : 1;
  guint   modify_base_as_bg : 1;
  guint   modify_bg_as_active : 1;
  gdouble resize_hsteps, resize_vsteps;
  gdouble resize_hunits, resize_vunits;
  gdouble width_from_height;
  gdouble height_from_width;
} GxkWidgetPatcher;
GType   gxk_widget_patcher_get_type     (void);
extern const GxkRadgetType *_gxk_widget_patcher_def;

/* --- focus frame --- */
#define GXK_TYPE_FOCUS_FRAME              (gxk_focus_frame_get_type ())
#define GXK_FOCUS_FRAME(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GXK_TYPE_FOCUS_FRAME, GxkFocusFrame))
#define GXK_FOCUS_FRAME_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_FOCUS_FRAME, GxkFocusFrameClass))
#define GXK_IS_FOCUS_FRAME(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GXK_TYPE_FOCUS_FRAME))
#define GXK_IS_FOCUS_FRAME_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_FOCUS_FRAME))
#define GXK_FOCUS_FRAME_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GXK_TYPE_FOCUS_FRAME, GxkFocusFrameClass))
typedef GtkFrame      GxkFocusFrame;
typedef GtkFrameClass GxkFocusFrameClass;
GType   gxk_focus_frame_get_type          (void);

/* --- back shade --- */
#define GXK_TYPE_BACK_SHADE              (gxk_back_shade_get_type ())
#define GXK_BACK_SHADE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GXK_TYPE_BACK_SHADE, GxkBackShade))
#define GXK_BACK_SHADE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_BACK_SHADE, GxkBackShadeClass))
#define GXK_IS_BACK_SHADE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GXK_TYPE_BACK_SHADE))
#define GXK_IS_BACK_SHADE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_BACK_SHADE))
#define GXK_BACK_SHADE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GXK_TYPE_BACK_SHADE, GxkBackShadeClass))
typedef GtkAlignment      GxkBackShade;
typedef GtkAlignmentClass GxkBackShadeClass;
GType   gxk_back_shade_get_type          (void);



G_END_DECLS

#endif /* __GXK_AUX_WIDGETS_H__ */
