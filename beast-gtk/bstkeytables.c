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
#include "bstkeytables.h"


/* --- structures --- */
struct _BstKeyTableKey
{
  guint16 keyval;
  guint16 modifier;
  guint32 action;
};


/* --- convenience macros for keytable definitions --- */
#define Z_WRAP          (0)
#define C_WRAP          (BST_PEA_WRAP_AS_CONFIG)
#define P_WRAP          (BST_PEA_WRAP_TO_PATTERN)
#define SET_INSTR       (BST_PEA_SET_INSTRUMENT_0F | Z_WRAP)
#define SHIFT_OCT       (BST_PEA_AFFECT_BASE_OCTAVE | Z_WRAP)
#define MOD(sca_mask)   (BST_MOD_ ## sca_mask)
#define ACT             BST_PEA


/* --- keytable definitions --- */
#include "bstkeytable-base.c"
#include "bstkeytable-us.c"
#include "bstkeytable-de.c"
static BstKeyTablePatch bst_key_table_patches[] = {
  { "Base",
    "Base Keytable to support keyboard movement, instrument setting "
    "and resetting of notes",
    "Tim Janik <timj@gtk.org>",
    sizeof (bst_key_table_base) / sizeof (bst_key_table_base[0]),
    bst_key_table_base,
    NULL,
  },
  { "US",	      	"Alias",	NULL, 0, NULL, "US-101", },
  { "US-101",
    "US Keypatch for american keyboard layouts (101 keys)",
    "Tim Janik <timj@gtk.org>",
    sizeof (bst_key_table_us_101) / sizeof (bst_key_table_us_101[0]),
    bst_key_table_us_101,
    "Base",
  },
  { "US-104",		"Alias",	NULL, 0, NULL, "US-101", },
  { "US-105",		"Alias",	NULL, 0, NULL, "US-104", },
  { "en_US",		"Alias",	NULL, 0, NULL, "US", },
  { "en_US-101",	"Alias",	NULL, 0, NULL, "US-101", },
  { "en_US-102",	"Alias",	NULL, 0, NULL, "US-101", },
  { "en_US-104",	"Alias",	NULL, 0, NULL, "US-104", },
  { "en_US-105",	"Alias",	NULL, 0, NULL, "US-105", },
  { "DE",		"Alias",	NULL, 0, NULL, "DE-102", },
  { "DE-102",
    "DE Keypatch for german keyboard layouts (102 keys)",
    "Tim Janik <timj@gtk.org>",
    sizeof (bst_key_table_de_102) / sizeof (bst_key_table_de_102[0]),
    bst_key_table_de_102,
    "Base",
  },
  { "DE-105",		"Alias",	NULL, 0, NULL, "DE-102", },
};
static const guint bst_key_table_n_patches = (sizeof (bst_key_table_patches) /
					      sizeof (bst_key_table_patches[0]));


/* --- functions --- */
BstKeyTablePatch*
bst_key_table_patch_find (const gchar *identifier)
{
  guint i;

  g_return_val_if_fail (identifier != NULL, NULL);

  for (i = 0; i < bst_key_table_n_patches; i++)
    if (!g_strcasecmp (bst_key_table_patches[i].identifier, identifier))
      return bst_key_table_patches + i;

  return NULL;
}

GList*
bst_key_table_list_patches (void)
{
  GList *list = NULL;
  gint i;

  for (i = bst_key_table_n_patches; i >= 0; i++)
    list = g_list_prepend (list, bst_key_table_patches + i);

  return list;
}

static BstKeyTablePatch*
bst_key_table_patch_get_base_patch (BstKeyTablePatch *patch)
{
  BstKeyTablePatch *base_patch = NULL;

  if (patch->base_patch)
    {
      base_patch = bst_key_table_patch_find (patch->base_patch);
      if (!base_patch)
	g_warning (G_GNUC_PRETTY_FUNCTION "(): unable to find base patch \"%s\" for \"%s\"",
		   patch->base_patch,
		   patch->identifier);
    }

  return base_patch;
}

void
bst_key_table_install_patch (BstKeyTablePatch *patch)
{
  BstPatternEditorClass *class;
  GSList *slist, *patch_slist = NULL;

  g_return_if_fail (patch != NULL);

  gtk_type_init ();
  class = gtk_type_class (BST_TYPE_PATTERN_EDITOR);

  while (patch)
    {
      patch_slist = g_slist_prepend (patch_slist, patch);
      patch = bst_key_table_patch_get_base_patch (patch);
    }
  
  bst_pattern_editor_class_clear_keys (class);

  for (slist = patch_slist; slist; slist = slist->next)
    {
      BstKeyTablePatch *current_patch = slist->data;
      BstKeyTableKey *key, *last_key = current_patch->keys + current_patch->n_keys;
      
      for (key = current_patch->keys; key < last_key; key++)
	bst_pattern_editor_class_set_key (class,
					  key->keyval,
					  key->modifier,
					  key->action);
    }

  g_slist_free (patch_slist);
}
