/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999, 2000, 2001 Tim Janik and Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * bstmenus.h: menu utilities for BEAST
 */
#ifndef __BST_MENUS_H__
#define __BST_MENUS_H__


#include        "bstdefs.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- defines --- */
#define BST_CHOICE_TITLE(name)           (bst_choice_alloc (BST_CHOICE_TYPE_TITLE, \
							    BST_ICON_NONE, (name), 0))
#define BST_CHOICE(id, name, bst_icon)   (bst_choice_alloc (BST_CHOICE_TYPE_ITEM, \
							    BST_ICON_ ## bst_icon, \
                                                            (name), (gpointer) (id)))
#define BST_CHOICE_D(id, name, bst_icon) (bst_choice_alloc (BST_CHOICE_TYPE_ITEM | \
							    BST_CHOICE_FLAG_DEFAULT, \
							    BST_ICON_ ## bst_icon, \
                                                            (name), (gpointer) (id)))
#define BST_CHOICE_S(id, name, icon, s)  (bst_choice_alloc (BST_CHOICE_TYPE_ITEM | \
							    ((s) ? 0 : BST_CHOICE_FLAG_INSENSITIVE), \
							     BST_ICON_ ## icon, \
                                                            (name), (gpointer) (id)))
#define BST_CHOICE_SUBMENU(name,menu)    (bst_choice_alloc (BST_CHOICE_TYPE_SUBMENU, \
							    BST_ICON_NONE, (name), (menu)))
#define BST_CHOICE_TEXT(name)            (bst_choice_alloc (BST_CHOICE_TYPE_TEXT, \
							    BST_ICON_NONE, (name), 0))
#define BST_CHOICE_SEPERATOR             (bst_choice_alloc (BST_CHOICE_TYPE_SEPARATOR, \
							    BST_ICON_NONE, NULL, 0))
#define BST_CHOICE_END                   (NULL)


/* --- enums --- */
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
  

/* --- structures --- */
typedef struct _BstMenuEntry   BstMenuEntry;
typedef struct _BstFactoryItem BstFactoryItem;
typedef struct _BstChoice      BstChoice;
struct _BstMenuEntry
{
  gchar                 *path;
  gchar                 *accelerator;
  GtkItemFactoryCallback callback;
  guint                  callback_action;
  gchar                 *item_type;
  BstIconId              stock_icon;
};
struct _BstFactoryItem
{
  GtkItemFactoryEntry entry;
  BswIcon            *icon;
};


/* --- prototypes --- */
GSList* bst_menu_entries_add_categories   (GSList		  *entry_slist,
					   guint                   n_cats,
					   BseCategory            *cats,
					   GtkItemFactoryCallback  cat_activate);
GSList* bst_menu_entries_add_item_entries (GSList		  *entry_slist,
					   guint                   n_menu_entries,
                                           GtkItemFactoryEntry    *menu_entries);
GSList* bst_menu_entries_add_bentries 	  (GSList		  *entry_slist,
					   guint                   n_menu_entries,
                                           BstMenuEntry           *menu_entries);
GSList* bst_menu_entries_sort		  (GSList		  *entry_slist);
void    bst_menu_entries_create_list      (GtkItemFactory         *ifactory,
					   GSList                 *bst_menu_entries,
					   gpointer                callback_data);
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
     

/* --- private --- */
BstChoice* bst_choice_alloc               (BstChoiceFlags          type,
					   BstIconId               icon_id,
					   const gchar            *choice_name,
					   gpointer                choice_id);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif  /* __BST_MENUS_H__ */
