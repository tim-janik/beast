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


/* --- prototypes --- */
BseErrorType	bse_magic_identify_bse_string	(const gchar	*string,
						 BseMagicFlags	*flags);
BseErrorType	bse_magic_identify_bse_fd	(gint		 fd,
						 BseMagicFlags	*flags);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_MAGIC_H__ */
