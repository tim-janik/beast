/* GXK - Gtk+ Extension Kit
 * Copyright (C) 2003-2004 Tim Janik
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
#ifndef __GXK_MENU_BUTTON_H__
#define __GXK_MENU_BUTTON_H__

#include "gxkutils.h"

G_BEGIN_DECLS

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
  GtkWidget *child;
  gint       icon_size, old_icon_size;
  gint       width, height;
  guint      show_selection : 1;
  guint      combo_arrow : 1;
  guint      push_in : 1;
} GxkMenuButton;
typedef GtkButtonClass GxkMenuButtonClass;
GType   gxk_menu_button_get_type          (void);
void    gxk_menu_button_update            (GxkMenuButton *self);


G_END_DECLS

#endif /* __GXK_MENU_BUTTON_H__ */
