/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2002 Tim Janik
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
#define BSE_OBJECT_DISPOSING(object)	  ((BSE_OBJECT_FLAGS (object) & BSE_OBJECT_FLAG_DISPOSING) > 0)
#define BSE_OBJECT_ID(object)		  (((BseObject*) (object))->unique_id)


/* --- bse object flags --- */
typedef enum				/*< skip >*/
{
  BSE_OBJECT_FLAG_FIXED_UNAME		= 1 << 0,
  BSE_OBJECT_FLAG_DISPOSING		= 1 << 1
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
  BseIcon*		(*get_icon)		(BseObject	*object);
};


/* --- object class prototypes ---*/
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
						 GType           return_type,
						 guint           n_params,
						 ...);
guint		bse_object_class_add_asignal	(BseObjectClass	*oclass,
						 const gchar	*signal_name,
						 GType           return_type,
						 guint           n_params,
						 ...);
guint		bse_object_class_add_dsignal	(BseObjectClass	*oclass,
						 const gchar	*signal_name,
						 GType           return_type,
						 guint           n_params,
						 ...);


/* --- object prototypes --- */
void		bse_object_lock			(BseObject	*object);
void		bse_object_unlock		(BseObject	*object);
BseIcon*	bse_object_get_icon		(BseObject	*object);
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
const gchar*	bse_object_debug_name		(gpointer	 object);
void		bse_object_reemit_signal	(gpointer	 src_object,
						 const gchar	*src_signal,
						 gpointer	 dest_obejct,
						 const gchar	*dest_signal);
void		bse_object_remove_reemit	(gpointer	 src_object,
						 const gchar	*src_signal,
						 gpointer	 dest_object,
						 const gchar	*dest_signal);
static inline void
bse_object_proxy_notifies	(gpointer	 src_object,
				 gpointer	 dest_object,
				 const gchar	*dest_signal)
{
  bse_object_reemit_signal (src_object, "notify::uname", dest_object, dest_signal);
  bse_object_reemit_signal (src_object, "icon-changed", dest_object, dest_signal);
}
static inline void
bse_object_unproxy_notifies	(gpointer	 src_object,
				 gpointer	 dest_object,
				 const gchar	*dest_signal)
{
  bse_object_remove_reemit (src_object, "notify::uname", dest_object, dest_signal);
  bse_object_remove_reemit (src_object, "icon-changed", dest_object, dest_signal);
}


/* --- implementation details --- */
const gchar*	bse_object_type_register	(const gchar *name,
						 const gchar *parent_name,
						 const gchar *blurb,
						 BsePlugin   *plugin,
						 GType	     *ret_type);
extern GQuark bse_quark_uname;


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_OBJECT_H__ */
