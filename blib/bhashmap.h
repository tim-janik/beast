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
 *
 * bhashmap.h: BLib integer<->integer mapping
 */
#ifndef __B_HASH_MAP_H__
#define __B_HASH_MAP_H__

#include        <blib/btype.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- typedefs --- */
typedef struct _BHashMap BHashMap;


/* --- prototypes --- */
BHashMap*	b_hash_map_new		(guint		 upper_bound_hint);
void		b_hash_map_add		(BHashMap	*hmap,
					 guint		 key,
					 guint		 value);
void		b_hash_map_remove	(BHashMap	*hmap,
					 guint		 key);
guint		b_hash_map_lookup	(BHashMap	*hmap,
					 guint		 key);
void		b_hash_map_destroy	(BHashMap	*hmap);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __B_HASH_MAP_H__ */
