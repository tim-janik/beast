/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2001 Tim Janik
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
 * bseobject.h: basic object definition for the BSE object heirarchy
 * a bse object implements means
 * - of setting/retriving an object name and blurb
 * - to set generic keyed data
 * - to set/get certain properties, i.e. object members or properties
 * - to aid basic parsing/dumping facilities
 */
#ifndef __BSE_OBJECT_H__
#define __BSE_OBJECT_H__

#include	<bse/bseparam.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- BSE type macros --- */
#define BSE_TYPE_OBJECT              (BSE_TYPE_ID (BseObject))
#define BSE_OBJECT(object)	     (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_OBJECT, BseObject))
#define BSE_OBJECT_CLASS(class)	     (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_OBJECT, BseObjectClass))
#define BSE_IS_OBJECT(object)	     (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_OBJECT))
#define BSE_IS_OBJECT_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_OBJECT))
#define BSE_OBJECT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_OBJECT, BseObjectClass))


/* --- object member/convenience macros --- */
#define BSE_OBJECT_TYPE(object)		  (G_TYPE_FROM_INSTANCE (object))
#define BSE_OBJECT_TYPE_NAME(object)	  (g_type_name (BSE_OBJECT_TYPE (object)))
#define BSE_OBJECT_UNAME(object)	  ((gchar*) g_datalist_id_get_data (&((GObject*) (object))->qdata, bse_quark_uname))
#define BSE_OBJECT_FLAGS(object)	  (((BseObject*) (object))->flags)
#define BSE_OBJECT_SET_FLAGS(object, f)	  (BSE_OBJECT_FLAGS (object) |= (f))
#define BSE_OBJECT_UNSET_FLAGS(object, f) (BSE_OBJECT_FLAGS (object) &= ~(f))
#define BSE_OBJECT_IS_LOCKED(object)	  (((BseObject*) (object))->lock_count > 0)
#define BSE_OBJECT_ID(object)		  (((BseObject*) (object))->unique_id)


/* --- bse object flags --- */
typedef enum				/*< skip >*/
{
  BSE_OBJECT_FLAG_FIXED_UNAME		= 1 << 0
} BseObjectFlags;
#define BSE_OBJECT_FLAGS_USHIFT	    (2)
#define BSE_OBJECT_FLAGS_MAX_SHIFT  (16)

typedef struct _BseObjectParser	   BseObjectParser;


/* --- typedefs & structures --- */
typedef GTokenType (*BseObjectParseStatement) (BseObject     *object,
					       BseStorage    *storage,
					       gpointer	      user_data);
struct _BseObject
{
  GObject		 parent_instance;
  
  /* pack into one guint */
  guint16		 flags;
  guint16		 lock_count;
  guint			 unique_id;
};
struct _BseObjectClass
{
  GObjectClass		 parent_class;

  /* internal stuff */
  guint			 n_parsers;
  BseObjectParser	*parsers;
  void			(*store_property)	(BseObject	*object,
						 BseStorage	*storage,
						 GValue		*value,
						 GParamSpec	*pspec);
  GTokenType		(*restore_property)	(BseObject	*object,
						 BseStorage	*storage,
						 GValue		*value,
						 GParamSpec	*pspec);
  void			(*store_after)		(BseObject	*object,
						 BseStorage	*storage);
  BseTokenType		(*try_statement)	(BseObject	*object,
						 BseStorage	*storage);
  GTokenType		(*restore)		(BseObject	*object,
						 BseStorage	*storage);

  /* custom methods for specific object needs, most of them require chaining */
  void			(*set_uname)		(BseObject	*object,
						 const gchar	*uname);
  void			(*store_private)	(BseObject	*object,
						 BseStorage	*storage);
  BseTokenType		(*restore_private)	(BseObject	*object,
						 BseStorage	*storage);
  void			(*unlocked)		(BseObject	*object);
  BswIcon*		(*get_icon)		(BseObject	*object);
  void			(*destroy)		(BseObject	*object);
};


/* --- object class prototypes ---*/
void	bse_object_class_set_param_log_scale	(BseObjectClass	*oclass,
						 const gchar	*pspec_name,
						 gdouble	 center,
						 gdouble	 base,
						 guint		 n_steps);
void	bse_object_class_add_property		(BseObjectClass *oclass,
						 const gchar	*property_group,
						 guint		 property_id,
						 GParamSpec	*pspec);
#define	bse_object_class_add_param	bse_object_class_add_property
void		bse_object_class_add_parser	(BseObjectClass *oclass,
						 const gchar	*token,
						 BseObjectParseStatement parse_func,
						 gpointer	 user_data);
guint		bse_object_class_add_signal	(BseObjectClass	*oclass,
						 const gchar	*signal_name,
						 GSignalCMarshaller c_marshaller,
						 GSignalCMarshaller proxy_marshaller,
						 GType           return_type,
						 guint           n_params,
						 ...);
GSignalCMarshaller bse_proxy_marshaller_lookup	(GSignalCMarshaller c_marshaller);


/* --- object prototypes --- */
gpointer	bse_object_new			(GType		 type,
						 const gchar	*first_property_name,
						 ...);
gpointer	bse_object_new_valist		(GType		 type,
						 const gchar	*first_property_name,
						 va_list	 var_args);
gpointer	bse_object_ref			(gpointer	 object);
void		bse_object_unref		(gpointer	 object);
void		bse_object_set			(BseObject	*object,
						 const gchar	*first_property_name,
						 ...);
void		bse_object_get			(BseObject	*object,
						 const gchar	*first_property_name,
						 ...);
#define 	bse_object_param_changed(o,pn)	g_object_notify ((GObject*) (o), (pn))
void		bse_object_lock			(BseObject	*object);
void		bse_object_unlock		(BseObject	*object);
gpointer	bse_object_get_data		(BseObject	*object,
						 const gchar	*key);
void		bse_object_set_data		(BseObject	*object,
						 const gchar	*key,
						 gpointer	 data);
void		bse_object_set_data_full	(BseObject	*object,
						 const gchar	*key,
						 gpointer	 data,
						 GDestroyNotify	 destroy);
BswIcon*	bse_object_get_icon		(BseObject	*object);
void		bse_object_notify_icon_changed	(BseObject	*object);
gpointer	bse_object_from_id		(guint		 unique_id);
GList*		bse_objects_list		(GType		 type);
GList*		bse_objects_list_by_uname	(GType		 type,
						 const gchar	*uname);
void		bse_object_store		(BseObject	*object,
						 BseStorage	*storage);
GTokenType	bse_object_restore		(BseObject	*object,
						 BseStorage	*storage);
void		bse_object_debug_leaks		(void);


/* --- implementation details --- */
const gchar*	bse_object_type_register	(const gchar *name,
						 const gchar *parent_name,
						 const gchar *blurb,
						 BsePlugin   *plugin,
						 GType	     *ret_type);
#define	bse_object_get_qdata(obj, quark)	(g_object_get_qdata ((gpointer) (obj), (quark)))
#define	bse_object_set_qdata(obj, quark, data)	(g_object_set_qdata ((gpointer) (obj), (quark), (data)))
#define	bse_object_set_qdata_full(o, q, d, y)	(g_object_set_qdata_full ((gpointer) (o), (q), (d), (y)))
#define	bse_object_steal_qdata(obj, quark)	(g_object_steal_qdata ((gpointer) (obj), (quark)))
extern GQuark bse_quark_uname;


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_OBJECT_H__ */
