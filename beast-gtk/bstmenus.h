/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BST_MENUS_H__
#define __BST_MENUS_H__


#include        "bstdefs.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- defines --- */
#define	BST_CHOICE_TITLE(name)		(bst_choice_alloc (1, BST_ICON_NONE, (name), 0))
#define	BST_CHOICE(id, name, bst_icon)	(bst_choice_alloc (0, BST_ICON_ ## bst_icon, \
							   (name), (id)))
#define	BST_CHOICE_S(id, name, icon, s)	(bst_choice_alloc ((s) ? 0 : 1, BST_ICON_ ## icon, \
							   (name), (id)))
#define	BST_CHOICE_SEPERATOR		(bst_choice_alloc (1, BST_ICON_NONE, NULL, 0))
#define BST_CHOICE_END			(NULL)


/* --- structures --- */
typedef struct _BstMenuEntry BstMenuEntry;
typedef struct _BstChoice    BstChoice;
struct _BstMenuEntry
{
  GtkItemFactoryEntry entry;
  BseIcon            *icon;
};


/* --- prototypes --- */
GSList*	bst_menu_entries_compose	(guint			 n_menu_entries,
					 GtkItemFactoryEntry	*menu_entries,
					 guint			 n_cats,
					 BseCategory		*cats,
					 GtkItemFactoryCallback  cat_activate);
void    bst_menu_entries_create         (GtkItemFactory         *ifactory,
					 GSList                 *bst_menu_entries,
					 gpointer                callback_data);
GtkWidget* bst_choice_createv		(BstChoice		*first_choice,
					 ...);
gboolean   bst_choice_selectable	(GtkWidget		*widget);
guint      bst_choice_modal		(GtkWidget		*widget,
					 guint                   mouse_button,
					 guint32                 time);


/* --- private --- */
BstChoice* bst_choice_alloc		(guint			 type,
					 BstIconId		 icon_id,
					 const gchar		*choice_name,
					 guint			 choice_id);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif  /* __BST_MENUS_H__ */
