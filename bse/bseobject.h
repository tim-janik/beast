/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998, 1999, 2000 Olaf Hoehmann and Tim Janik
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
 * - to set/get certain parameters, i.e. object members or properties
 * - to aid basic parsing/dumping facilities
 * - for notification upon certain object related events
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
#define BSE_OBJECT_NAME(object)		  ((gchar*) g_datalist_id_get_data (&((GObject*) (object))->qdata, _bse_quark_name))
#define BSE_OBJECT_FLAGS(object)	  (((BseObject*) (object))->flags)
#define BSE_OBJECT_SET_FLAGS(object, f)	  (BSE_OBJECT_FLAGS (object) |= (f))
#define BSE_OBJECT_UNSET_FLAGS(object, f) (BSE_OBJECT_FLAGS (object) &= ~(f))
#define BSE_OBJECT_IS_LOCKED(obj)	  (((BseObject*) (obj))->lock_count > 0)


/* --- bse object flags --- */
typedef enum				/*< skip >*/
{
  BSE_OBJECT_FLAG_CONSTRUCTED		= 1 << 0
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
};
struct _BseObjectClass
{
  GObjectClass		 parent_class;

  /* internal stuff */
  guint			 n_notifiers;
  GQuark		*notifiers;
  guint			 n_parsers;
  BseObjectParser	*parsers;
  void			(*store_after)		(BseObject	*object,
						 BseStorage	*storage);
  BseTokenType		(*try_statement)	(BseObject	*object,
						 BseStorage	*storage);
  GTokenType		(*restore)		(BseObject	*object,
						 BseStorage	*storage);

  /* custom methods for specific object needs, most of them require chaining */
  void			(*set_name)		(BseObject	*object,
						 const gchar	*name);
  void			(*store_private)	(BseObject	*object,
						 BseStorage	*storage);
  BseTokenType		(*restore_private)	(BseObject	*object,
						 BseStorage	*storage);
  void			(*unlocked)		(BseObject	*object);
  BseIcon*		(*get_icon)		(BseObject	*object);
  void			(*destroy)		(BseObject	*object);
};


/* --- object class prototypes ---*/
void		bse_object_class_add_param	(BseObjectClass *oclass,
						 const gchar	*param_group,
						 guint		 param_id,
						 GParamSpec	*pspec /* taken over */);
void		bse_object_class_add_parser	(BseObjectClass *oclass,
						 const gchar	*token,
						 BseObjectParseStatement parse_func,
						 gpointer	 user_data);


/* --- object prototypes --- */
gpointer	bse_object_new			(GType		 type,
						 const gchar	*first_param_name,
						 ...);
gpointer	bse_object_new_valist		(GType		 type,
						 const gchar	*first_param_name,
						 va_list	 var_args);
gpointer	bse_object_ref			(gpointer	 object);
void		bse_object_unref		(gpointer	 object);
void		bse_object_set			(BseObject	*object,
						 const gchar	*first_param_name,
						 ...);
void		bse_object_get			(BseObject	*object,
						 const gchar	*first_param_name,
						 ...);
void		bse_object_param_changed	(BseObject	*object,
						 const gchar	*param_name);
void		bse_object_lock			(BseObject	*object);
void		bse_object_unlock		(BseObject	*object);
void		bse_object_set_name		(BseObject	*object,
						 const gchar	*name);
gchar*		bse_object_get_name_or_type	(BseObject	*object);
gpointer	bse_object_get_data		(BseObject	*object,
						 const gchar	*key);
void		bse_object_set_data		(BseObject	*object,
						 const gchar	*key,
						 gpointer	 data);
void		bse_object_set_data_full	(BseObject	*object,
						 const gchar	*key,
						 gpointer	 data,
						 GDestroyNotify	 destroy);
BseIcon*	bse_object_get_icon		(BseObject	*object);
void		bse_object_notify_icon_changed	(BseObject	*object);
GList*		bse_objects_list		(GType		 type);
GList*		bse_objects_list_by_name	(GType		 type,
						 const gchar	*name);
void		bse_object_store		(BseObject	*object,
						 BseStorage	*storage);
GTokenType	bse_object_restore		(BseObject	*object,
						 BseStorage	*storage);
void		bse_object_remove_notifier	(gpointer	 object,
						 guint		 id);
guint		bse_object_add_notifier		(gpointer	 object,
						 const gchar	*method,
						 gpointer	 func,
						 gpointer	 data);
guint		bse_object_add_data_notifier	(gpointer	 object,
						 const gchar	*method,
						 gpointer	 func,
						 gpointer	 data);
guint		bse_object_add_notifier_full	(gpointer	 object,
						 const gchar	*method,
						 gpointer	 func,
						 gpointer	 data,
						 GDestroyNotify	 destroy);
guint		bse_object_add_data_notifier_full   (gpointer	    object,
						     const gchar   *method,
						     gpointer	    func,
						     gpointer	    data,
						     GDestroyNotify destroy);
void		bse_object_remove_notifiers_by_func (gpointer	    object,
						     gpointer	    func,
						     gpointer	    data);
void		bse_nullify_pointer		    (gpointer	   *pointer_loc);


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
extern GQuark _bse_quark_name;
GHookList*	bse_object_get_hook_list	(BseObject	*object);
struct _BseNotifyHook
{
  GHook	 hook;
  GQuark quark;
};
#define BSE_NOTIFY_FLAG_CALL_DATA	(1 << G_HOOK_FLAG_USER_SHIFT)
/* bse_object_add_notifier_full (song, get_param, my_cb, data, data_destroy);
 * BSE_NOTIFY (song, get_param, NOTIFY (OBJECT, id, param, DATA));
 */
#define BSE_NOTIFY(__obj, __method, __CALL) \
    BSE_NOTIFY_CHECK (__obj, __method, __CALL, if (1 /* !BSE_OBJECT_DESTROYED (__object) */))
#define BSE_NOTIFY_CHECK(__obj, __method, __CALL, __CHECK) \
G_STMT_START { \
  BseObject *__object = (BseObject*) (__obj); GHook *__hook; \
  BseNotify_ ## __method NOTIFY; GHookList *__hook_list; \
  GQuark __hook_quark = g_quark_try_string (G_STRINGIFY (__method)); \
  bse_object_ref (__object); \
  __hook_list = bse_object_get_hook_list (__object); \
  __hook = __hook_list ? g_hook_first_valid (__hook_list, TRUE) : NULL; \
  __CHECK while (__hook) \
    { \
      if (((BseNotifyHook*) __hook)->quark == __hook_quark) \
	{ \
	  gpointer OBJECT, DATA; \
	  gboolean __hook_in_call = G_HOOK_IN_CALL (__hook); \
	  __hook->flags |= G_HOOK_FLAG_IN_CALL; \
	  if (__hook->flags & BSE_NOTIFY_FLAG_CALL_DATA) \
	    { OBJECT = __hook->data; DATA = __object; } \
	  else \
	    { OBJECT = __object; DATA = __hook->data; } \
	  NOTIFY = __hook->func; \
	  { __CALL ; } \
	  if (!__hook_in_call) \
	    __hook->flags &= ~G_HOOK_FLAG_IN_CALL; \
	} \
      __hook = g_hook_next_valid (__hook_list, __hook, TRUE); \
    } \
  bse_object_unref (__object); \
} G_STMT_END

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_OBJECT_H__ */
