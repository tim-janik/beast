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


/* BSE's fundamental types.
 * the order has to be kept in sync with several other source files,
 * so just don't change it! ;)
 */
typedef enum
{
  BSE_TYPE_INVALID,
  BSE_TYPE_NONE,
  
  /* param types, synced with bseparams.c */
  BSE_TYPE_PARAM_BOOL,
  BSE_TYPE_PARAM_INT,
  BSE_TYPE_PARAM_UINT,
  BSE_TYPE_PARAM_ENUM,
  BSE_TYPE_PARAM_FLAGS,
  BSE_TYPE_PARAM_FLOAT,
  BSE_TYPE_PARAM_DOUBLE,
  BSE_TYPE_PARAM_TIME,
  BSE_TYPE_PARAM_NOTE,
  BSE_TYPE_PARAM_INDEX_2D,
  BSE_TYPE_PARAM_STRING,
  BSE_TYPE_PARAM_DOTS,
  BSE_TYPE_PARAM_ITEM,

  BSE_TYPE_INTERFACE,
  
  BSE_TYPE_PROCEDURE,
  
  BSE_TYPE_ENUM,
  BSE_TYPE_FLAGS,
  
  BSE_TYPE_OBJECT
} BseFundamentalType;
#define	BSE_TYPE_PARAM_FIRST		(BSE_TYPE_PARAM_BOOL)
#define	BSE_TYPE_PARAM_LAST		(BSE_TYPE_PARAM_ITEM)
#define	BSE_TYPE_CLASSED_FIRST		(BSE_TYPE_PROCEDURE)
#define	BSE_TYPE_CLASSED_LAST		(BSE_TYPE_OBJECT)
#define	BSE_TYPE_FUNDAMENTAL_LAST	(BSE_TYPE_CLASSED_LAST)


/* type macros
 */
#define	BSE_FUNDAMENTAL_TYPE(type)	((type) & 0xff)
#define	BSE_TYPE_SEQNO(type)		((type) > 0xff ? (type) >> 8 : (type))
#define BSE_TYPE_IS_PARAM(type)		(BSE_FUNDAMENTAL_TYPE (type) >= BSE_TYPE_PARAM_FIRST && \
					 BSE_FUNDAMENTAL_TYPE (type) <= BSE_TYPE_PARAM_LAST)
#define BSE_TYPE_IS_CLASSED(type)	(BSE_FUNDAMENTAL_TYPE (type) >= BSE_TYPE_CLASSED_FIRST && \
					 BSE_FUNDAMENTAL_TYPE (type) <= BSE_TYPE_CLASSED_LAST)
#define	BSE_TYPE_IS_INTERFACE(type)	(BSE_FUNDAMENTAL_TYPE (type) == BSE_TYPE_INTERFACE)
#define	BSE_TYPE_IS_PROCEDURE(type)	(BSE_FUNDAMENTAL_TYPE (type) == BSE_TYPE_PROCEDURE)
#define	BSE_TYPE_IS_OBJECT(type)	(BSE_FUNDAMENTAL_TYPE (type) == BSE_TYPE_OBJECT)

/* casts, checks and convenience macros for structured types
 */
#ifndef BSE_DISABLE_CAST_CHECKS
#  define BSE_CHECK_STRUCT_CAST(sp, bse_type, c_type)	\
    ((c_type*) bse_type_check_struct_cast ((BseTypeStruct*) (sp), (bse_type)))
#  define BSE_CHECK_CLASS_CAST(cp, bse_type, c_type)	\
    ((c_type*) bse_type_check_class_cast ((BseTypeClass*) (cp), (bse_type)))
# else /* BSE_DISABLE_CAST_CHECKS */
#  define BSE_CHECK_STRUCT_CAST(sp, bse_type, c_type) ((c_type*) (sp))
#  define BSE_CHECK_CLASS_CAST(cp, bse_type, c_type)  ((c_type*) (cp))
#endif /* BSE_DISABLE_CAST_CHECKS */
#define	BSE_CHECK_STRUCT_TYPE(sp, bse_type)	( \
    bse_type_struct_conforms_to ((BseTypeStruct*) (sp), (bse_type)) \
)
#define	BSE_CHECK_CLASS_TYPE(cp, bse_type)	( \
    bse_type_class_is_a ((BseTypeClass*) (cp), (bse_type)) \
)
#define	BSE_STRUCT_TYPE(sp)	(BSE_CLASS_TYPE (((BseTypeStruct*) (sp))->bse_class))
#define	BSE_CLASS_TYPE(cp)	(((BseTypeClass*) (cp))->bse_type)
#define	BSE_CLASS_NAME(cp)	(bse_type_name (BSE_CLASS_TYPE (cp)))


/* magic macros to define type initialization function within
 * .c files. they identify builtin type functions for magic post
 * processing and help resolving runtime type id retrival.
 */
#define	BSE_TYPE_ID(BseTypeName)	(bse_type_builtin_id_##BseTypeName)
#ifdef BSE_COMPILATION
#  define BSE_BUILTIN_PROTO(BseTypeName) extern BseType bse_type_builtin_register_##BseTypeName (void)
#  define BSE_BUILTIN_TYPE(BseTypeName)	 BSE_BUILTIN_PROTO (BseTypeName); \
                                         BseType bse_type_builtin_register_##BseTypeName (void)
#  define BSE_DUMMY_TYPE(BseTypeName)	 BSE_BUILTIN_PROTO (BseTypeName); \
                                         BseType bse_type_builtin_register_##BseTypeName (void) \
                                         { return 0; } BSE_BUILTIN_PROTO (BseTypeName)
#endif /* BSE_COMPILATION */


/* Here we import the auto generated type ids.
 */
#include        <bse/bsegentypes.h>


/* asserted portions of structured types
 */
struct _BseTypeClass
{
  BseType	bse_type;
};
struct _BseTypeInterface
{
  BseType	bse_type;
  BseType	object_type;
  guint		ref_count;
};
struct _BseTypeStruct
{
  BseTypeClass *bse_class;
};


/* type informations
 */
typedef void (*BseBaseInitFunc)		(gpointer	 bse_class);
typedef void (*BseBaseDestroyFunc)	(gpointer	 bse_class);
typedef void (*BseClassInitFunc)	(gpointer	 bse_class,
					 gpointer	 class_data);
typedef void (*BseClassDestroyFunc)	(gpointer	 bse_class,
					 gpointer	 class_data);
typedef	void (*BseObjectInitFunc)	(BseObject	*object,
					 gpointer	 object_class);
typedef void (*BseInterfaceInitFunc)	(gpointer	 bse_class,
					 gpointer	 iface_data);
typedef void (*BseInterfaceDestroyFunc)	(gpointer	 bse_class,
					 gpointer	 iface_data);
struct _BseTypeInfo
{
  /* interface types, classed types, object types */
  guint			  class_size;
  
  BseBaseInitFunc	  base_init;
  BseBaseDestroyFunc	  base_destroy;
  
  /* classed types, object types */
  BseClassInitFunc	  class_init;
  BseClassDestroyFunc	  class_destroy;
  gconstpointer		  class_data;
  
  /* object types */
  guint16		  object_size;
  guint16		  n_preallocs;
  BseObjectInitFunc	  object_init;
};
struct _BseInterfaceInfo
{
  BseInterfaceInitFunc	  interface_init;
  BseInterfaceDestroyFunc interface_destroy;
  gpointer		  interface_data;
};


/* --- prototypes --- */
gpointer	bse_type_class_ref		(BseType	 type);
gpointer	bse_type_class_peek		(BseType	 type);
void		bse_type_class_unref		(gpointer	 bse_class);
gpointer	bse_type_interface_ref		(gpointer	 object_class,
						 BseType	 iface_type);
gpointer	bse_type_interface_peek		(gpointer	 object_class,
						 BseType	 iface_type);
void		bse_type_interface_unref	(gpointer	 interface);
gchar*		bse_type_name			(BseType	 type);
GQuark		bse_type_quark			(BseType	 type);
gchar*		bse_type_blurb			(BseType	 type);
BseType		bse_type_from_name		(const gchar	*name);
BseType		bse_type_parent			(BseType	 type);
gpointer	bse_type_class_peek_parent	(gpointer	 type_class);
BseType		bse_type_register_static	(BseType	 parent_type,
						 const gchar	*type_name,
						 const gchar	*type_blurb,
						 const BseTypeInfo *info);
void		bse_type_add_interface		(BseType	 object_type,
						 BseType	 iface_type,
						 BseInterfaceInfo*info);
gboolean	bse_type_is_a			(BseType	 type,
						 BseType	 is_a_type);
gboolean	bse_type_conforms_to		(BseType	 type,
						 BseType	 iface_type);
BseType* /*fr*/	bse_type_children		(BseType	 type,
						 guint		*n_children);
BseType* /*fr*/	bse_type_interfaces		(BseType	 type,
						 guint		*n_interfaces);


/* --- internal --- */
void		bse_type_init			(void);
BseType		bse_type_register_dynamic	(BseType         parent_type,
						 const gchar    *type_name,
						 const gchar    *type_blurb,
						 BsePlugin      *plugin);
gboolean	bse_type_struct_conforms_to	(BseTypeStruct	*type_struct,
						 BseType	 iface_type);
gboolean	bse_type_class_is_a		(BseTypeClass	*type_class,
						 BseType	 is_a_type);
BseTypeStruct*	bse_type_check_struct_cast	(BseTypeStruct	*bse_struct,
						 BseType	 iface_type);
BseTypeClass*	bse_type_check_class_cast	(BseTypeClass	*bse_class,
						 BseType	 is_a_type);
BseObject*	bse_type_create_object		(BseType	 type);
void		bse_type_free_object		(BseObject	*object);
BsePlugin*	bse_type_plugin			(BseType	 type);






#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_TYPE_H__ */
