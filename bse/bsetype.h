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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * bsetype.h: BSE type system
 */
#ifndef __BSE_TYPE_H__
#define __BSE_TYPE_H__

#include	<bse/bsedefs.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* BSE fundamental types.
 */
typedef enum	/*< skip >*/
{
  BSE_TYPE_PROCEDURE		= G_TYPE_BSE_PROCEDURE,

} BseTypeFundamentals;


/* type macros
 */
#define	BSE_TYPE_IS_PROCEDURE(type)	(G_TYPE_FUNDAMENTAL (type) == BSE_TYPE_PROCEDURE)
#define	BSE_CLASS_NAME(class)		(g_type_name (G_TYPE_FROM_CLASS (class)))
#define	BSE_CLASS_TYPE(class)		(G_TYPE_FROM_CLASS (class))
#define	BSE_TYPE_IS_OBJECT(type)	(g_type_is_a ((type), BSE_TYPE_OBJECT))


/* magic macros to define type initialization function within
 * .c files. they identify builtin type functions for magic post
 * processing and help resolving runtime type id retrival.
 */
#define	BSE_TYPE_ID(BseTypeName)	(bse_type_builtin_id_##BseTypeName)
#ifdef BSE_COMPILATION
#  define BSE_BUILTIN_PROTO(BseTypeName) extern GType bse_type_builtin_register_##BseTypeName (void)
#  define BSE_BUILTIN_TYPE(BseTypeName)	 BSE_BUILTIN_PROTO (BseTypeName); \
                                         GType bse_type_builtin_register_##BseTypeName (void)
#  define BSE_DUMMY_TYPE(BseTypeName)	 BSE_BUILTIN_PROTO (BseTypeName); \
                                         GType bse_type_builtin_register_##BseTypeName (void) \
                                         { return 0; } BSE_BUILTIN_PROTO (BseTypeName)
#endif /* BSE_COMPILATION */


/* Here we import the auto generated type ids.
 */
#include        <bse/bsegentypes.h>


/* compatibility functions
 */
#define	bse_type_create_object(type)	((gpointer) g_type_create_instance (type))
#define	bse_type_free_object(object)	(g_type_free_instance ((gpointer) (object)))


/* --- prototypes --- */
void		bse_type_init		  (void);
gchar*		bse_type_blurb		  (GType  	      type);
void		bse_type_set_blurb	  (GType  	      type,
					   const gchar	     *blurb);
GType  		bse_type_register_static  (GType  	      parent_type,
					   const gchar	     *type_name,
					   const gchar	     *type_blurb,
					   const GTypeInfo   *info);
GType  		bse_type_register_dynamic (GType              parent_type,
					   const gchar       *type_name,
					   const gchar       *type_blurb,
					   BsePlugin         *plugin);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_TYPE_H__ */
