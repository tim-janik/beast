/* BSW - Bedevilled Sound Engine Wrapper
 * Copyright (C) 2002 Tim Janik
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
 */
#ifndef __BSW_GLUE_H__
#define __BSW_GLUE_H__

#include        <bse/bswcommon.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void	bsw_value_initset_boolean	(GValue		*value,
					 GType		 type,
					 gboolean	 v_bool);
void	bsw_value_initset_char		(GValue		*value,
					 GType		 type,
					 gchar		 v_char);
void	bsw_value_initset_uchar		(GValue		*value,
					 GType		 type,
					 guchar		 v_uchar);
void	bsw_value_initset_int		(GValue		*value,
					 GType		 type,
					 gint		 v_int);
void	bsw_value_initset_uint		(GValue		*value,
					 GType		 type,
					 guint		 v_uint);
void	bsw_value_initset_long		(GValue		*value,
					 GType		 type,
					 glong		 v_long);
void	bsw_value_initset_ulong		(GValue		*value,
					 GType		 type,
					 gulong		 v_ulong);
void	bsw_value_initset_enum		(GValue		*value,
					 GType		 type,
					 gint		 v_enum);
void	bsw_value_initset_flags		(GValue		*value,
					 GType		 type,
					 guint		 v_flags);
void	bsw_value_initset_float		(GValue		*value,
					 GType		 type,
					 gfloat		 v_float);
void	bsw_value_initset_double	(GValue		*value,
					 GType		 type,
					 gdouble	 v_double);
void	bsw_value_initset_string	(GValue		*value,
					 GType		 type,
					 const gchar	*v_string);
void	bsw_value_initset_boxed		(GValue		*value,
					 GType		 type,
					 gpointer	 v_boxed);
void	bsw_value_initset_proxy_notype	(GValue		*value,
					 BswProxy	 v_proxy);
#define	bsw_value_initset_proxy(value, type, proxy)	bsw_value_initset_proxy_notype ((value), (proxy))

gchar*	bsw_type_name_to_cname		(const gchar	*type_name);
gchar*	bsw_type_name_to_sname		(const gchar	*type_name);
gchar*	bsw_type_name_to_cupper		(const gchar	*type_name);
gchar*	bsw_type_name_to_type_macro	(const gchar	*type_name);
gchar*  bsw_cupper_to_sname		(const gchar	*cupper);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSW_GLUE_H__ */
