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
#ifndef __BST_CATALOG_H__
#define __BST_CATALOG_H__

#include "bstutils.h"

G_BEGIN_DECLS

/* --- structures & typedefs --- */
typedef struct {
  gchar *cat_key;
  gchar *name;
  gchar *stock_id;
  gchar *accelerator;
  gchar *tooltip;
  gchar *blurb;
} BstCatalogTool;

typedef struct {
  guint	                n_tools;
  const BstCatalogTool *tools;
  GHashTable	       *tools_ht;
} BstCatalog;


/* --- API --- */
void			_bst_catalog_init	(void);
const BstCatalogTool	bst_catalog_get_tool	(const gchar	*cat_key);

G_END_DECLS

#endif /* __BST_CATALOG_H__ */
