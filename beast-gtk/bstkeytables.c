/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2002 Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bstkeytables.h"

#include <gdk/gdkkeysyms.h>


/* --- structures --- */
struct _BstKeyTableKey
{
  guint16 keyval;
  guint16 modifier;
  guint32 action;
};


/* --- convenience macros for keytable definitions --- */
#define BST_MOD____	BST_MOD_000
#define BST_MOD___A	BST_MOD_00A
#define BST_MOD__C_	BST_MOD_0C0
#define BST_MOD__CA	BST_MOD_0CA
#define BST_MOD_S__	BST_MOD_S00
#define BST_MOD_S_A	BST_MOD_S0A
#define	BST_MOD_SC_	BST_MOD_SC0
#define BST_MOD_SCA	BST_MOD_SCA
#define MOD(sca_mask)   (BST_MOD_ ## sca_mask)
#define	WRAP_BORD	BST_PEA_WRAP_TO_NOTE
#define	WRAP_PAT	BST_PEA_WRAP_TO_PATTERN
#define	WRAP_CONF	BST_PEA_WRAP_AS_CONFIG
#define	WRAP_none	0
#define	MODIFY(note, octave, instrument, wrapping, movement)	( \
    BST_PEA_TYPE_MODIFY_NOTE | \
    BST_PEA_CONCAT (ANY, note, octave, instrument, movement) | \
    WRAP_ ## wrapping \
)
#define	CHANGE(dflt_octave, dflt_instrument)	( \
    BST_PEA_TYPE_CHANGE_DEFAULTS | \
    BST_PEA_CONCAT (ANY, same, dflt_octave, dflt_instrument, none) \
)
#define	ACTIVATE(cell)	( \
    BST_PEA_TYPE_ACTIVATE_CELL | \
    BST_PEA_CONCAT (cell, same, same, same, none) \
)
#define RECT_SELECT(wrap, movement)	( \
    BST_PEA_TYPE_MODIFY_NOTE | \
    WRAP_ ## wrap | \
    BST_PEA_MOVE_ ## movement | \
    BST_PEA_RECTANGLE_SELECT \
)


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
#if 0
  BstPatternEditorClass *class;
  GSList *slist, *patch_slist = NULL;

  g_return_if_fail (patch != NULL);

  g_type_init ();
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
#endif
}
