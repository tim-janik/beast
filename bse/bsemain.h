/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997, 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * bsemain.h: general bse routines
 */
#ifndef __BSE_MAIN_H__
#define __BSE_MAIN_H__

#include	<bse/bseglobals.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- variables --- */
extern BseDebugFlags bse_debug_flags;


/* --- prototypes --- */
gboolean	bse_initialized			(void);
void		bse_init			(gint		*argc,
						 gchar	      ***argv);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_MAIN_H__ */
