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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include	"bstmenus.h"

#define	N_TRACKS 2 // FIXME: hack


/* --- prototypes --- */


/* --- variables --- */


/* --- functions --- */
static gint
menu_entries_compare (gconstpointer a,
		      gconstpointer b)
{
  const GtkItemFactoryEntry *entry_a = a;
  const GtkItemFactoryEntry *entry_b = b;

  return strcmp (entry_a->path, entry_b->path);
}

GSList*
bst_menu_entries_compose (guint                  n_menu_entries,
			  GtkItemFactoryEntry   *menu_entries,
			  guint                  n_cats,
			  BseCategory           *cats,
			  GtkItemFactoryCallback cat_activate)
{
  BstMenuEntry *bentry, *last_bentry;
  GtkItemFactoryEntry *last_entry;
  GSList *branch_slist = NULL, *entry_slist = NULL;

  if (n_menu_entries)
    g_return_val_if_fail (menu_entries != NULL, NULL);
  if (n_cats)
    {
      g_return_val_if_fail (cats != NULL, NULL);
      g_return_val_if_fail (cat_activate != NULL, NULL);
    }

  bentry = g_new0 (BstMenuEntry, n_cats);
  last_bentry = bentry + n_cats;
  while (bentry < last_bentry)
    {
      entry_slist = g_slist_prepend (entry_slist, bentry);
      bentry->entry.path = cats->category + cats->mindex;
      bentry->entry.callback = cat_activate;
      bentry->entry.callback_action = cats->type;
      bentry->icon = cats->icon;
      cats++;
      bentry++;
    }

  last_entry = menu_entries + n_menu_entries;
  while (menu_entries < last_entry)
    {
      if (bse_string_equals (menu_entries->item_type, "<Branch>") ||
	  bse_string_equals (menu_entries->item_type, "<LastBranch>"))
	branch_slist = g_slist_prepend (branch_slist, menu_entries++);
      else
	entry_slist = g_slist_prepend (entry_slist, menu_entries++);
    }

  return g_slist_concat (g_slist_reverse (branch_slist),
			 g_slist_sort (entry_slist, menu_entries_compare));
}

static GtkWidget*
create_icon_widget (BseIcon *icon)
{
  GtkWidget *widget;
  const guint size = 16;

  if (icon)
    {
      widget = gtk_widget_new (GNOME_TYPE_FOREST,
			       "visible", TRUE,
			       "width", size,
			       "height", size,
			       NULL);
      gnome_forest_put_sprite (GNOME_FOREST (widget), 1,
			       (icon->bytes_per_pixel > 3
				? art_pixbuf_new_const_rgba
				: art_pixbuf_new_const_rgb) (icon->pixels,
							     icon->width,
							     icon->height,
							     icon->width *
							     icon->bytes_per_pixel));
      gnome_forest_set_sprite_size (GNOME_FOREST (widget), 1, size, size);
    }
  else
    widget = gtk_widget_new (GTK_TYPE_ALIGNMENT,
			     "visible", TRUE,
			     "width", size,
			     NULL);
  
  return widget;
}

void
bst_menu_entries_create (GtkItemFactory *ifactory,
			 GSList         *bst_menu_entries,
			 gpointer        callback_data)
{
  GSList *slist;

  g_return_if_fail (GTK_IS_ITEM_FACTORY (ifactory));

  for (slist = bst_menu_entries; slist; slist = slist->next)
    {
      BstMenuEntry *entry = slist->data;
      GtkWidget *item, *child = NULL;

      gtk_item_factory_create_items (ifactory, 1, &entry->entry, callback_data);
      item = gtk_item_factory_get_item (ifactory, entry->entry.path);
      if (GTK_IS_MENU_ITEM (item))
	child = GTK_BIN (item)->child;
      if (child)
	{
	  GtkWidget *hbox;

	  gtk_widget_ref (child);
	  gtk_container_remove (GTK_CONTAINER (item), child);
	  hbox = gtk_widget_new (GTK_TYPE_HBOX,
				 "parent", item,
				 "visible", TRUE,
				 "child", child,
				 NULL);
	  gtk_container_add_with_args (GTK_CONTAINER (hbox),
				       create_icon_widget (entry->icon),
				       "expand", FALSE,
				       "fill", FALSE,
				       "position", 0,
				       NULL);
	  gtk_widget_unref (child);
	}
    }
}
