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
 * bsemagic.h: identify files through extension or magic entry
 */
#ifndef __BSE_MAGIC_H__
#define __BSE_MAGIC_H__

#include	<bse/bseenums.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- defines --- */
typedef enum
{
  BSE_MAGIC_BSE_BIN_EXTENSION	= 1 << 0,
  BSE_MAGIC_BSE_SAMPLE		= 1 << 1,
  BSE_MAGIC_BSE_SONG		= 1 << 2
} BseMagicFlags;


/* --- typedefs --- */
typedef struct _BseMagic BseMagic;


/* --- structures --- */
struct _BseMagic
{
  GType    proc_type;
  GQuark   prefix;
  GQuark   qextension;

  /*< private >*/
  gpointer match_list;
};


/* match entity with:
 * prefix,
 * extension,
 * magic_spec
 *
 * where prefix has absolute preference, and extension is just
 * a _hint_ for magic_spec match order, unless magic_spec==NULL
 *
 * no prefix for save handlers. (?) just extension matches.
 *
 * need pre-parse functionality, to figure name and type of a
 * file's contents.
 */


/* --- prototypes --- */
BseMagic*	bse_magic_new			(GType		 proc_type,
						 GQuark          qextension,
						 const gchar	*magic_spec);
BseMagic*	bse_magic_match_file		(const gchar	*file_name,
						 GSList         *magic_list);
void		bse_magic_list_append		(BseMagic	*magic);
BseMagic*	bse_magic_list_match_file	(const gchar    *file_name);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_MAGIC_H__ */
