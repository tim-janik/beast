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

/* --- typedefs --- */
#define BSE_TYPE_PROCEDURE	G_TYPE_MAKE_FUNDAMENTAL (G_TYPE_RESERVED_BSE_FIRST + 0)
#define BSE_TYPE_TIME		G_TYPE_MAKE_FUNDAMENTAL (G_TYPE_RESERVED_BSE_FIRST + 1)
#define	BSE_TYPE_NOTE		G_TYPE_MAKE_FUNDAMENTAL (G_TYPE_RESERVED_BSE_FIRST + 2)
#define	BSE_TYPE_DOTS		G_TYPE_MAKE_FUNDAMENTAL (G_TYPE_RESERVED_BSE_FIRST + 3)

typedef enum
{
  BSE_PARAM_READABLE		= G_PARAM_READABLE,
  BSE_PARAM_WRITABLE		= G_PARAM_WRITABLE,
  BSE_PARAM_MASK		= G_PARAM_MASK,

  /* intention */
  BSE_PARAM_SERVE_GUI		= 1 << 8,
  BSE_PARAM_SERVE_STORAGE	= 1 << 9,
  BSE_PARAM_SERVE_MASK		= 0x00000f00,

  /* GUI hints */
  BSE_PARAM_HINT_CHECK_NULL	= 1 << 16,
  BSE_PARAM_HINT_RDONLY		= 1 << 17,
  BSE_PARAM_HINT_RADIO		= 1 << 18,
  BSE_PARAM_HINT_DIAL		= 1 << 19,
  BSE_PARAM_HINT_SCALE		= 1 << 20,
  BSE_PARAM_HINT_MASK		= 0xffff0000,

  /* aliases */
  BSE_PARAM_READWRITE		= BSE_PARAM_READABLE | BSE_PARAM_WRITABLE,
  BSE_PARAM_GUI			= BSE_PARAM_READWRITE | BSE_PARAM_SERVE_GUI,
  BSE_PARAM_STORAGE		= BSE_PARAM_READWRITE | BSE_PARAM_SERVE_STORAGE,
  BSE_PARAM_DEFAULT		= BSE_PARAM_GUI | BSE_PARAM_STORAGE
} BseParamFlags;


/* type macros
 */
#define	BSE_TYPE_IS_PROCEDURE(type)	(G_TYPE_FUNDAMENTAL (type) == BSE_TYPE_PROCEDURE)
#define	BSE_CLASS_NAME(class)		(g_type_name (G_TYPE_FROM_CLASS (class)))
#define	BSE_CLASS_TYPE(class)		(G_TYPE_FROM_CLASS (class))
#define	BSE_TYPE_IS_OBJECT(type)	(g_type_is_a ((type), BSE_TYPE_OBJECT))


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


/* --- implementation details --- */
extern GType BSE_TYPE_PARAM_INT;
extern GType BSE_TYPE_PARAM_UINT;
extern GType BSE_TYPE_PARAM_FLOAT;
extern GType BSE_TYPE_PARAM_DOUBLE;
extern GType BSE_TYPE_PARAM_TIME;
extern GType BSE_TYPE_PARAM_NOTE;
extern GType BSE_TYPE_PARAM_DOTS;

/* compatibility functions
 */
#define	bse_type_create_object(type)	((gpointer) g_type_create_instance (type))
#define	bse_type_free_object(object)	(g_type_free_instance ((gpointer) (object)))

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


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_TYPE_H__ */
