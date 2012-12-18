/* BSE - Better Sound Engine
 * Copyright (C) 1998-1999, 2000-2003 Tim Janik
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
#ifndef __BSE_CATEGORIES_H__
#define __BSE_CATEGORIES_H__

#include        <bse/bsetype.h>

G_BEGIN_DECLS


/* --- typedefs --- */
typedef gboolean (BseCategoryCheck) (BseCategory *category,
                                     gpointer     data);


/* --- prototypes --- */
void            bse_categories_register        (const gchar      *category,
                                                const gchar      *i18n_category,
                                                GType             type,
                                                const guint8     *pixstream);
BseCategorySeq* bse_categories_match           (const gchar      *pattern,
                                                GType             base_type,
                                                BseCategoryCheck  check,
                                                gpointer          data);
BseCategorySeq* bse_categories_match_typed     (const gchar      *pattern,
                                                GType             base_type);
BseCategorySeq* bse_categories_from_type       (GType             type);
BseCategory*    bse_category_from_id           (guint             id);
void      bse_categories_register_stock_module (const gchar      *untranslated_category_trunk,
                                                GType             type,
                                                const guint8     *pixstream);


/* --- implementation --- */
void		_bse_init_categories	     (void);

G_END_DECLS

#endif /* __BSE_CATEGORIES_H__ */
