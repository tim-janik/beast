/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * bsecategories.h: BSE category related functions
 */
#ifndef __BSE_CATEGORIES_H__
#define __BSE_CATEGORIES_H__

#include        <bse/bsetype.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- structures --- */
struct _BseCategory
{
  gchar	  *category;
  guint    mindex;
  BseType  type;
  BseIcon *icon; /* static icons, no need for reference counting */
};


/* --- prototypes --- */
void                bse_categories_register      (const gchar      *category,
						  BseType           type);
void                bse_categories_register_icon (const gchar      *category,
						  BseType           type,
						  const BsePixdata *pixdata);
BseCategory* /*fr*/ bse_categories_match         (const gchar      *pattern,
						  guint            *n_matches);
BseCategory* /*fr*/ bse_categories_match_typed   (const gchar      *pattern,
						  BseType           base_type,
						  guint            *n_matches);
BseCategory* /*fr*/ bse_categories_from_type     (BseType           type,
						  guint            *n_categories);







#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_CATEGORIES_H__ */
