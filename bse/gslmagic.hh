/* GSL - Generic Sound Layer
 * Copyright (C) 2000-2002 Tim Janik
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
#ifndef __GSL_MAGIC_H__
#define __GSL_MAGIC_H__

#include <bse/gsldefs.hh>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- structures --- */
typedef struct _GslRealMagic GslRealMagic;
struct _GslMagic
{
  gpointer data;
  gchar   *extension;

  /*< private >*/
  gint          priority;
  GslRealMagic *match_list;
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
GslMagic*	gsl_magic_list_match_file	(SfiRing	*magic_list,
						 const gchar    *file_name);
GslMagic*	gsl_magic_list_match_file_skip	(SfiRing	*magic_list,
						 const gchar    *file_name,
						 guint           skip_bytes);
void		gsl_magic_list_brute_match	(SfiRing	*magic_list,
						 const gchar	*file_name,
						 guint		 skip_bytes,
						 GslMagic	*skip_magic,
						 SfiRing       **ext_matches,
						 SfiRing       **other_matches);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_MAGIC_H__ */
