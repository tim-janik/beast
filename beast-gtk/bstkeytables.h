/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999, 2000 Olaf Hoehmann and Tim Janik
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
#ifndef __BST_KEY_TABLES_H__
#define __BST_KEY_TABLES_H__

#include "bstpatterneditor.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- structures & typedefs --- */
typedef struct _BstKeyTableKey   BstKeyTableKey;
typedef struct _BstKeyTablePatch BstKeyTablePatch;
struct _BstKeyTablePatch
{
  gchar          *identifier;
  gchar          *description;
  gchar          *maintainer;
  guint           n_keys;
  BstKeyTableKey *keys;
  gchar          *base_patch;
};


/* --- prototypes --- */
GList* /*fr*/	  bst_key_table_list_patches	(void);
BstKeyTablePatch* bst_key_table_patch_find	(const gchar      *identifier);
void		  bst_key_table_install_patch	(BstKeyTablePatch *patch);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_KEY_TABLES_H__ */
