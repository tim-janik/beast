/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998-2002 Tim Janik
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
 * bseparasite.h: BSE Parasite implementation (foreign data storage)
 */
#ifndef __BSE_PARASITE_H__
#define __BSE_PARASITE_H__

#include        <bse/bseobject.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- prototypes --- */
void	   bse_parasite_set_floats	(BseObject      *object,
					 const gchar	*name,
					 guint		 n_values,
					 gfloat		*float_values);
SfiFBlock* bse_parasite_get_floats	(BseObject      *object,
					 const gchar	*name);


/* --- internal --- */
void	bse_parasite_install_parsers	(BseObjectClass	*oclass);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_PARASITE_H__ */
