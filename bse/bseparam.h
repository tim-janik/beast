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
 */
#ifndef __BSE_PARAM_H__
#define __BSE_PARAM_H__

#include        <bse/bsetype.h>
#include        <bse/bseutils.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- enumeration params and values --- */
#define	    BSE_TYPE_PARAM_ENUM			(G_TYPE_PARAM_ENUM)
#define	    BSE_IS_PARAM_SPEC_ENUM(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), BSE_TYPE_PARAM_ENUM))
#define	    BSE_PARAM_SPEC_ENUM(pspec)		(G_TYPE_CHECK_INSTANCE_CAST ((pspec), BSE_TYPE_PARAM_ENUM, BseParamSpecEnum))
typedef	    GParamSpecEnum			 BseParamSpecEnum;
GParamSpec* bse_param_spec_enum			(const gchar	*name,
						 const gchar	*nick,
						 const gchar	*blurb,
						 gint		 default_value,
						 GType		 enum_type,
						 const gchar	*hints);
gint	    bse_pspec_get_enum_default		(GParamSpec	*pspec);
GEnumValue* bse_pspec_get_enum_value_list	(GParamSpec	*pspec);

#define	    BSE_VALUE_HOLDS_ENUM(value)		(G_TYPE_CHECK_VALUE_TYPE ((value), BSE_TYPE_ENUM))
#define	    bse_value_get_enum			 g_value_get_enum
#define	    bse_value_set_enum			 g_value_set_enum
GValue*	    bse_value_enum			(GType		 enum_type,
						 gint		 evalue);


/* --- object param specs --- */
#define	    BSE_TYPE_PARAM_OBJECT		(G_TYPE_PARAM_OBJECT)
#define	    BSE_IS_PARAM_SPEC_OBJECT(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), BSE_TYPE_PARAM_OBJECT))
#define	    BSE_PARAM_SPEC_OBJECT(pspec)	(G_TYPE_CHECK_INSTANCE_CAST ((pspec), BSE_TYPE_PARAM_OBJECT, BseParamSpecObject))
typedef	    GParamSpecObject			 BseParamSpecObject;
GParamSpec* bse_param_spec_object		(const gchar	*name,
						 const gchar	*nick,
						 const gchar	*blurb,
						 GType		 object_type,
						 const gchar	*hints);

#define	    BSE_VALUE_HOLDS_OBJECT(value)	(G_TYPE_CHECK_VALUE_TYPE ((value), BSE_TYPE_OBJECT))
#define	    bse_value_get_object		 g_value_get_object
#define	    bse_value_set_object		 g_value_set_object
#define	    bse_value_take_object		 g_value_take_object
GValue*	    bse_value_object			(gpointer	 vobject);


/* --- boxed parameters --- */
typedef GParamSpecBoxed			 BseParamSpecBoxed;
#define BSE_TYPE_PARAM_BOXED		(G_TYPE_PARAM_BOXED)
#define BSE_IS_PARAM_SPEC_BOXED(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), BSE_TYPE_PARAM_BOXED))
#define BSE_PARAM_SPEC_BOXED(pspec)	(G_TYPE_CHECK_INSTANCE_CAST ((pspec), BSE_TYPE_PARAM_BOXED, BseParamSpecBoxed))
#define BSE_VALUE_HOLDS_BOXED(value)	(G_TYPE_CHECK_VALUE_TYPE ((value), G_TYPE_BOXED))
GParamSpec* bse_param_spec_boxed	(const gchar  *name,
					 const gchar  *nick,
					 const gchar  *blurb,
					 GType	       boxed_type,
					 const gchar  *hints);
#define     bse_value_get_boxed          g_value_get_boxed
#define     bse_value_set_boxed          g_value_set_boxed
#define     bse_value_dup_boxed          g_value_dup_boxed
#define     bse_value_take_boxed         g_value_set_boxed_take_ownership


/* --- convenience pspec constructors --- */
GParamSpec* bse_param_spec_freq         (const gchar  *name,
					 const gchar  *nick,
					 const gchar  *blurb,
					 SfiReal       default_freq,
					 const gchar  *hints);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_PARAM_H__ */
