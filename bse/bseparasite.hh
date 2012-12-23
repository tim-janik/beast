/* BSE - Better Sound Engine
 * Copyright (C) 1998-2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#ifndef __BSE_PARASITE_H__
#define __BSE_PARASITE_H__

#include <bse/bseitem.hh>

G_BEGIN_DECLS

/* --- parasite records --- */
void         bse_item_set_parasite               (BseItem        *item, /* undoable */
                                                  const gchar    *parasite_path,
                                                  SfiRec         *rec);
SfiRec*      bse_item_get_parasite               (BseItem        *item,
                                                  const gchar    *parasite_path);
void         bse_item_backup_parasite            (BseItem        *item,
                                                  const gchar    *parasite_path,
                                                  SfiRec         *rec);
void         bse_item_delete_parasites           (BseItem        *item);
SfiRing*     bse_item_list_parasites             (BseItem        *item,
                                                  const gchar    *parent_path);
const gchar* bse_item_create_parasite_name       (BseItem        *item,
                                                  const gchar    *path_prefix);
/* BseItem signals:
 *   void (*parasites_added)  (BseItem     *item,
 *                             const gchar *parasite_path);
 *   void (*parasite_changed) (BseItem     *item,
 *                             const gchar *parasite_path);
 */
void         bse_item_class_add_parasite_signals (BseItemClass *);


/* --- old prototypes --- */
void	   bse_parasite_set_floats	(BseObject      *object,
					 const gchar	*name,
					 guint		 n_values,
					 gfloat		*float_values);
SfiFBlock* bse_parasite_get_floats	(BseObject      *object,
					 const gchar	*name);
void	   bse_parasite_store		(BseObject	*object,
					 BseStorage	*storage);
GTokenType bse_parasite_restore		(BseObject	*object,
					 BseStorage	*storage);

G_END_DECLS

#endif /* __BSE_PARASITE_H__ */
