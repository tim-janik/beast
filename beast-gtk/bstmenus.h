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


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- typedefs & structures --- */
typedef struct _BstChoice BstChoice;
typedef void  (*BstMenuCatFunc)    (GtkWidget   *owner,
				    gulong       category_id,
				    gpointer	 popup_data);
typedef void  (*BstMenuUserFunc)   (GtkWidget   *owner,
				    gulong       callback_action,
				    gpointer	 popup_data);
typedef struct {
  gchar *path;
  gchar *accelerator;

  BstMenuUserFunc callback;
  gulong          callback_action;

  /* possible values:
   * NULL               -> "<Item>"
   * ""                 -> "<Item>"
   * "<Title>"          -> create a title item
   * "<Item>"           -> create a simple item
   * "<ImageItem>"      -> create an item holding an image
   * "<StockItem>"      -> create an item holding a stock image
   * "<CheckItem>"      -> create a check item
   * "<ToggleItem>"     -> create a toggle item
   * "<RadioItem>"      -> create a radio item
   * <path>             -> path of a radio item to link against
   * "<Separator>"      -> create a separator
   * "<Tearoff>"        -> create a tearoff separator
   * "<Branch>"         -> create an item to hold sub items
   * "<LastBranch>"     -> create a right justified item to hold sub items
   */
  gchar          *item_type;

  /* Extra data for some item types:
   *  ImageItem  -> pointer to inlined pixbuf stream
   *  StockItem  -> name of stock item
   */
  gconstpointer extra_data;
} BstMenuConfigEntry;
typedef struct {
  SfiRing *entries;
  GSList  *gcentries;
  GSList  *gcicons;
} BstMenuConfig;


/* --- GxkAction helpers --- */
void            bst_action_list_add_cat         (GxkActionList          *alist,
                                                 BseCategory            *cat,
                                                 GxkActionCheck          acheck,
                                                 GxkActionExec           aexec,
                                                 gpointer                user_data,
                                                 guint                   skip_levels,
                                                 const gchar            *stock_fallback);
GxkActionList*  bst_action_list_from_cats       (BseCategorySeq         *cseq,
                                                 GxkActionCheck          acheck,
                                                 GxkActionExec           aexec,
                                                 gpointer                user_data,
                                                 guint                   skip_levels,
                                                 const gchar            *stock_fallback);


/* --- item factory helpers --- */
BstMenuConfig*	bst_menu_config_from_entries	(guint			 n_entries,
						 BstMenuConfigEntry	*entries);
BstMenuConfig*	bst_menu_config_append_cat	(BstMenuConfig          *mconfig,
                                                 BseCategory		*cat,
						 BstMenuCatFunc		 callback,
						 guint			 skip_levels,
                                                 const gchar            *new_prefix,
                                                 const gchar            *stock_fallback);
BstMenuConfig*	bst_menu_config_from_cats	(BseCategorySeq		*cseq,
						 BstMenuCatFunc		 callback,
						 guint			 skip_levels,
                                                 const gchar            *new_prefix,
                                                 const gchar            *stock_fallback);
void		bst_menu_config_sort		(BstMenuConfig		*config);
void		bst_menu_config_reverse		(BstMenuConfig		*config);
BstMenuConfig*	bst_menu_config_merge		(BstMenuConfig		*config,
						 BstMenuConfig		*merge_config);
void		bst_menu_config_free		(BstMenuConfig		*config);
void		bst_menu_config_create_items	(BstMenuConfig		*config,
						 GtkItemFactory		*ifactory,
						 GtkWidget		*owner); /* e.g. menubar toplevel */

void		bst_menu_popup			(GtkItemFactory		*ifactory,
						 GtkWidget		*owner,  /* window with popup*/
						 gpointer		 popup_data,
						 GtkDestroyNotify	 popup_data_destroy,
						 guint			 x,
						 guint			 y,
						 guint			 mouse_button,
						 guint32		 time);
void		bst_menu_add_accel_owner	(GtkItemFactory		*ifactory,
						 GtkWidget		*owner); /* key press windows */


/* --- BstChoice --- */
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
#define BST_CHOICE_SUBMENU(name,menu)    (bst_choice_alloc (BST_CHOICE_TYPE_SUBMENU, \
							    (name), (menu), BST_STOCK_NONE, 0))
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


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif  /* __BST_MENUS_H__ */
