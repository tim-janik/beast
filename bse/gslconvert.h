/* GSL - Generic Sound Layer
 * Copyright (C) 2001 Tim Janik
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
 */
#ifndef __GSL_CONVERT_H__
#define __GSL_CONVERT_H__

#include <gsl/gsldefs.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GSL_CODESET_8859_1      "ISO-8859-1"	/* LATIN-1 */
#define GSL_CODESET_UTF8        "UTF8"		/* "ISO-10646" */

gchar*	gsl_convert_from_utf8	(const gchar	*codeset,
				 const gchar	*string);
gchar*	gsl_convert_to_utf8	(const gchar	*codeset,
				 const gchar	*string);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_CONVERT_H__ */
