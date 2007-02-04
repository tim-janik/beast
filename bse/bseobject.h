/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2003 Tim Janik
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

G_BEGIN_DECLS

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
#define BSE_OBJECT_IN_RESTORE(object)	  ((BSE_OBJECT_FLAGS (object) & BSE_OBJECT_FLAG_IN_RESTORE) > 0)
#define BSE_OBJECT_ID(object)		  (((BseObject*) (object))->unique_id)

/* --- object flags --- */
typedef enum				/*< skip >*/
{
  BSE_OBJECT_FLAG_FIXED_UNAME		= 1 << 0,
  BSE_OBJECT_FLAG_DISPOSING		= 1 << 1,
  BSE_OBJECT_FLAG_IN_RESTORE		= 1 << 2
} BseObjectFlags;
#define BSE_OBJECT_FLAGS_USHIFT	    (3)
#define BSE_OBJECT_FLAGS_MAX_SHIFT  (16)

/* --- typedefs & structures --- */
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

  gboolean              (*editable_property)    (BseObject      *object, /* for set_property/get_property implementations */
                                                 guint           param_id,
                                                 GParamSpec     *pspec);
  /* custom methods for specific object needs, most of them require chaining */
  gboolean              (*check_pspec_editable) (BseObject      *object, /* for containers */
                                                 GParamSpec     *pspec);
  void			(*set_uname)		(BseObject	*object,
						 const gchar	*uname);
  void			(*store_private)	(BseObject	*object,
						 BseStorage	*storage);
  void                  (*restore_start)        (BseObject      *object,
                                                 BseStorage     *storage);
  SfiTokenType		(*restore_private)	(BseObject	*object,
						 BseStorage	*storage,
                                                 GScanner       *scanner);
  void                  (*restore_finish)       (BseObject      *object);
  void			(*unlocked)		(BseObject	*object);
  BseIcon*		(*get_icon)		(BseObject	*object);
};

/* --- object class API ---*/
void	bse_object_class_add_property		(BseObjectClass *oclass,
						 const gchar	*property_group,
						 guint		 property_id,
						 GParamSpec	*pspec);
void	bse_object_class_add_grouped_property	(BseObjectClass *oclass,
						 guint		 property_id,
						 GParamSpec	*pspec);
#define	bse_object_class_add_param	         bse_object_class_add_property
guint	bse_object_class_add_signal	        (BseObjectClass	*oclass,
						 const gchar	*signal_name,
						 GType           return_type,
						 guint           n_params,
						 ...);
guint	bse_object_class_add_asignal    	(BseObjectClass	*oclass,
						 const gchar	*signal_name,
						 GType           return_type,
						 guint           n_params,
						 ...);
guint	bse_object_class_add_dsignal    	(BseObjectClass	*oclass,
						 const gchar	*signal_name,
						 GType           return_type,
						 guint           n_params,
						 ...);


/* --- object API --- */
void		bse_object_lock			(gpointer	 object);
void		bse_object_unlock		(gpointer	 object);
gboolean        bse_object_editable_property	(gpointer	 object,
                                                 const gchar    *property);
BseIcon*	bse_object_get_icon		(BseObject	*object);
void		bse_object_notify_icon_changed	(BseObject	*object);
gpointer	bse_object_from_id		(guint		 unique_id);
GList*		bse_objects_list		(GType		 type);
GList*		bse_objects_list_by_uname	(GType		 type,
						 const gchar	*uname);
void		bse_object_debug_leaks		(void);
const gchar*	bse_object_debug_name		(gpointer	 object);
gchar*	        bse_object_strdup_debug_handle 	(gpointer	 object);
void            bse_object_restore_start        (BseObject      *object,
                                                 BseStorage     *storage);
void            bse_object_restore_finish       (BseObject      *object);
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
extern GQuark bse_quark_uname;
void          bse_object_marshal_signal (GClosure       *closure,
                                         GValue /*out*/ *return_value,
                                         guint           n_param_values,
                                         const GValue   *param_values,
                                         gpointer        invocation_hint,
                                         gpointer        marshal_data);

G_END_DECLS

#endif /* __BSE_OBJECT_H__ */
