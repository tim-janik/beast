/* GXK - Gtk+ Extension Kit
 * Copyright (C) 2003-2004 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#ifndef __GXK_MENU_BUTTON_H__
#define __GXK_MENU_BUTTON_H__

#include "gxkutils.hh"
#include "gxkassortment.hh"

G_BEGIN_DECLS

/* --- type macros --- */
#define GXK_TYPE_MENU_BUTTON              (gxk_menu_button_get_type ())
#define GXK_MENU_BUTTON(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GXK_TYPE_MENU_BUTTON, GxkMenuButton))
#define GXK_MENU_BUTTON_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_MENU_BUTTON, GxkMenuButtonClass))
#define GXK_IS_MENU_BUTTON(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GXK_TYPE_MENU_BUTTON))
#define GXK_IS_MENU_BUTTON_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_MENU_BUTTON))
#define GXK_MENU_BUTTON_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GXK_TYPE_MENU_BUTTON, GxkMenuButtonClass))
typedef enum
{
  GXK_MENU_BUTTON_TOOL_MODE = 1,        /* ---image--- [label] | arrow */
  GXK_MENU_BUTTON_COMBO_MODE,           /* [image] label---    | arrow */
  GXK_MENU_BUTTON_OPTION_MODE,          /* ---(image,label)--- | arrow */
  GXK_MENU_BUTTON_POPUP_MODE,           /* right arrow */
} GxkMenuButtonMode;
typedef struct {
  GtkEventBox    parent_instance;
  GtkWidget     *islot, *cslot, *fframe, *button;
  GtkMenu       *menu;
  GtkWidget     *menu_item, *image, *child;
  GdkWindow     *bwindow;
  gchar         *assortment_name;
  GxkAssortment *assortment_object;
  gint           icon_size, old_icon_size;
  gint           width, height;
  GxkMenuButtonMode mode;
  GtkReliefStyle    relief;
} GxkMenuButton;
typedef GtkEventBoxClass GxkMenuButtonClass;
GType   gxk_menu_button_get_type          (void);
void    gxk_menu_button_update            (GxkMenuButton *self);


G_END_DECLS

#endif /* __GXK_MENU_BUTTON_H__ */
