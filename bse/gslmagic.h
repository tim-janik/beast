/* GSL - Generic Sound Layer
 * Copyright (C) 2000-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __GSL_MAGIC_H__
#define __GSL_MAGIC_H__

#include	<gsl/gsldefs.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- structures --- */
struct _GslMagic
{
  gpointer data;
  gchar   *extension;

  /*< private >*/
  gint     priority;
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
GslMagic*	gsl_magic_create		(gpointer	 data,
						 gint		 priority,
						 const gchar	*extension,
						 const gchar	*magic_spec);
GslMagic*	gsl_magic_list_match_file	(GslRing	*magic_list,
						 const gchar    *file_name);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_MAGIC_H__ */
