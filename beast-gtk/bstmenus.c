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
			  guint                  n_procs,
			  BseType               *proc_types,
			  GtkItemFactoryCallback proc_activate)
{
  GtkItemFactoryEntry *entry, *last_entry;
  GSList *branch_slist = NULL, *entry_slist = NULL;

  if (n_menu_entries)
    g_return_val_if_fail (menu_entries != NULL, NULL);
  if (n_procs)
    {
      g_return_val_if_fail (proc_types != NULL, NULL);
      g_return_val_if_fail (proc_activate != NULL, NULL);
    }

  entry = g_new0 (GtkItemFactoryEntry, n_procs);
  last_entry = entry + n_procs;
  while (entry < last_entry)
    {
      entry_slist = g_slist_prepend (entry_slist, entry);
      entry->path = (gchar*) bse_procedure_get_category (*proc_types);
      entry->callback = proc_activate;
      entry->callback_action = *proc_types;
      proc_types++;
      entry++;
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
