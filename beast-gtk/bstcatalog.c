/* BEAST - Bedevilled Audio System
 * Copyright (C) 2003 Tim Janik
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
#include "bstcatalog.h"


/* -- catalogs --- */
#include "bstmessages.c"


/* --- variables --- */
static BstCatalog *bst_catalog = NULL;


/* --- functions --- */
void
_bst_catalog_init (void)
{
  static BstCatalog cat = { 0, };
  guint i;

  g_assert (bst_catalog == NULL);

  bst_catalog = &cat;
  bst_catalog->n_tools = G_N_ELEMENTS (tools_en);
  bst_catalog->tools = tools_en;
  bst_catalog->tools_ht = g_hash_table_new (g_str_hash, g_str_equal);
  for (i = 0; i < bst_catalog->n_tools; i++)
    g_hash_table_insert (bst_catalog->tools_ht,
			 (gchar*) bst_catalog->tools[i].cat_key,
			 (gchar*) (bst_catalog->tools + i));
}

const BstCatalogTool
bst_catalog_get_tool (const gchar *cat_key)
{
  BstCatalogTool *t;
  BstCatalogTool tool = { 0, };

  g_return_val_if_fail (cat_key != NULL, tool);

  t = g_hash_table_lookup (bst_catalog->tools_ht, cat_key);
  if (t)
    {
      tool = *t;
      tool.name = gettext (tool.name);
      tool.tooltip = gettext (tool.tooltip);
      tool.blurb = gettext (tool.blurb);
    }
  else
    {
      tool.cat_key = g_intern_string (cat_key);
      tool.name = tool.cat_key;
      tool.tooltip = "PROGRAMMING ERROR: category missing from bstmessages.c";
    }
  return tool;
}
