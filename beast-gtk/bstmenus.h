/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2002 Tim Janik and Red Hat, Inc.
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
#ifndef __BST_MENUS_H__
#define __BST_MENUS_H__

#include        "bstutils.h"

G_BEGIN_DECLS

/* --- BstChoice --- */
/* BstChoice are simple inlined popup menus for modal selections.
 */
typedef struct _BstChoice BstChoice;

GtkWidget* bst_choice_menu_createv        (const gchar		  *menu_path,
					   BstChoice              *first_choice,
					   ...);
void bst_choice_menu_add_choice_and_free  (GtkWidget		  *menu,
					   BstChoice		  *choice);
void bst_choice_menu_set_item_sensitive	  (GtkWidget		  *menu,
					   gulong		   id,
					   gboolean		   sensitive);
GtkWidget* bst_choice_dialog_createv      (BstChoice              *first_choice,
					   ...);
gboolean   bst_choice_selectable          (GtkWidget              *widget);
guint      bst_choice_modal               (GtkWidget              *widget,
					   guint                   mouse_button,
					   guint32                 time);
guint      bst_choice_get_last            (GtkWidget              *widget);
void	   bst_choice_destroy		  (GtkWidget		  *choice);


/* --- BstChoice shortcuts --- */
#define BST_CHOICE_TITLE(name)           (bst_choice_alloc (BST_CHOICE_TYPE_TITLE, \
							    (name), 0, BST_STOCK_NONE, 0))
#define BST_CHOICE(id, name, bst_icon)   (bst_choice_alloc (BST_CHOICE_TYPE_ITEM, \
							    (name), (gpointer) (id), \
                                                            BST_STOCK_ ## bst_icon, 0))
#define BST_CHOICE_D(id, name, bst_icon) (bst_choice_alloc (BST_CHOICE_TYPE_ITEM | \
							    BST_CHOICE_FLAG_DEFAULT, \
                                                            (name), (gpointer) (id), \
							    BST_STOCK_ ## bst_icon, 0))
#define BST_CHOICE_S(id, name, icon, s)  (bst_choice_alloc (BST_CHOICE_TYPE_ITEM | \
							    ((s) ? 0 : BST_CHOICE_FLAG_INSENSITIVE), \
                                                            (name), (gpointer) (id), \
							    BST_STOCK_ ## icon, 0))
#define BST_CHOICE_SUBMENU(nam,menu,icn) (bst_choice_alloc (BST_CHOICE_TYPE_SUBMENU, \
							    (nam), (menu), BST_STOCK_ ## icn, 0))
#define BST_CHOICE_TEXT(name)            (bst_choice_alloc (BST_CHOICE_TYPE_TEXT, \
							    (name), 0, BST_STOCK_NONE, 0))
#define BST_CHOICE_SEPERATOR             (bst_choice_alloc (BST_CHOICE_TYPE_SEPARATOR, \
							    NULL, 0, BST_STOCK_NONE, 0))
#define BST_CHOICE_END                   (NULL)


/* --- private implementation stubs --- */
typedef enum
{
  BST_CHOICE_TYPE_SEPARATOR	= 0,
  BST_CHOICE_TYPE_TITLE		= 1,
  BST_CHOICE_TYPE_TEXT		= 2,
  BST_CHOICE_TYPE_ITEM		= 3,
  BST_CHOICE_TYPE_SUBMENU	= 4,
  BST_CHOICE_TYPE_MASK		= 0xff,
  BST_CHOICE_FLAG_INSENSITIVE	= (1 << 8),
  BST_CHOICE_FLAG_DEFAULT	= (1 << 9),
  BST_CHOICE_FLAG_MASK		= (~BST_CHOICE_TYPE_MASK)
} BstChoiceFlags;
BstChoice* bst_choice_alloc               (BstChoiceFlags          type,
					   const gchar            *choice_name,
					   gpointer                choice_id,
					   const gchar		  *icon_stock_id,
					   BseIcon		  *bse_icon);

G_END_DECLS

#endif  /* __BST_MENUS_H__ */
