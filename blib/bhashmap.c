/* BLib - BSE/BSI helper library
 * Copyright (C) 1997, 1998, 1999, 2000 Olaf Hoehmann and Tim Janik
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
 */
#include	"bhashmap.h"


#include	<string.h>



/* --- structures --- */
typedef struct {
  guint	key;
  guint value;
} HMapEntry;
struct _BHashMap
{
  guint      upper_bound_hint;
  guint      n_entries;
  HMapEntry *entries;
};


/* --- functions --- */
static inline HMapEntry*
hmap_lookup (BHashMap *hmap,
	     guint     key)
{
  if (hmap->n_entries)
    {
      HMapEntry *entries = hmap->entries - 1;
      guint n_entries = hmap->n_entries;
      
      do                /* FIXME: should optimize lookups for <= 4 */
	{
	  HMapEntry *check;
	  guint i;
	  
	  i = (n_entries + 1) / 2;
	  check = entries + i;
	  if (key == check->key)
	    return check;
	  else if (key > check->key)
	    {
	      n_entries -= i;
	      entries = check;
	    }
	  else /* if (key < check->key) */
	    n_entries = i - 1;
	}
      while (n_entries);
    }
  return NULL;
}

BHashMap*
b_hash_map_new (guint upper_bound_hint)
{
  BHashMap *hmap = g_new (BHashMap, 1);

  hmap->upper_bound_hint = upper_bound_hint;
  hmap->n_entries = 0;
  hmap->entries = NULL;

  return hmap;
}

void
b_hash_map_add (BHashMap *hmap,
		guint     key,
		guint     value)
{
  HMapEntry *entry;

  g_return_if_fail (hmap != NULL);

  entry = hmap_lookup (hmap, key);
  if (entry)
    entry->value = value;
  else
    {
      guint i;

      hmap->n_entries += 1;
      hmap->entries = g_renew (HMapEntry, hmap->entries, hmap->n_entries);
      for (i = 0; i < hmap->n_entries - 1; i++)
	if (hmap->entries[i].key > key)
	  break;
      g_memmove (hmap->entries + i + 1, hmap->entries + i, sizeof (hmap->entries[0]) * (hmap->n_entries - i - 1));
      hmap->entries[i].key = key;
      hmap->entries[i].value = value;
    }
}

void
b_hash_map_remove (BHashMap *hmap,
		   guint     key)
{
  HMapEntry *entry;

  g_return_if_fail (hmap != NULL);

  entry = hmap_lookup (hmap, key);
  if (!entry)
    g_warning (G_STRLOC ": cannot remove invalid hash map key: %d", key);
  else
    {
      guint i = entry - hmap->entries;

      g_memmove (hmap->entries + i, hmap->entries + i + 1, sizeof (hmap->entries[0]) * (hmap->n_entries - i - 1));
      hmap->n_entries -= 1;
    }
}

guint
b_hash_map_lookup (BHashMap *hmap,
		   guint     key)
{
  HMapEntry *entry;

  g_return_val_if_fail (hmap != NULL, 0);

  entry = hmap_lookup (hmap, key);

  return entry ? entry->value : 0;
}

void
b_hash_map_destroy (BHashMap *hmap)
{
  g_return_if_fail (hmap != NULL);

  g_free (hmap->entries);
  g_free (hmap);
}
