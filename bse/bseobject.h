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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
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

#include        <bse/bseparam.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- BSE type macros --- */
/* since BSE_TYPE_OBJECT is a fundamental type, we can use
 * a number of short hands for the structure checks here.
 * look at bseitem.h to see how the standard type macros are
 * normally implemented.
 */
#define BSE_OBJECT(object)           (BSE_IS_OBJECT (object) ? ((BseObject*) (object)) : \
                                      BSE_CHECK_STRUCT_CAST ((object), BSE_TYPE_OBJECT, BseObject))
#define BSE_OBJECT_CLASS(class)      (BSE_IS_OBJECT_CLASS (class) ? ((BseObjectClass*) (class)) : \
                                      BSE_CHECK_CLASS_CAST ((class), BSE_TYPE_OBJECT, BseObjectClass))
#define BSE_IS_OBJECT(object)        (((BseObject*) (object)) != NULL && \
                                      BSE_IS_OBJECT_CLASS (((BseTypeStruct*) (object))->bse_class))
#define BSE_IS_OBJECT_CLASS(class)   (((BseTypeClass*) (class)) != NULL && \
                                      BSE_TYPE_IS_OBJECT (((BseTypeClass*) (class))->bse_type))
#define BSE_OBJECT_GET_CLASS(object) ((BseObjectClass*) (((BseObject*) (object))->bse_struct.bse_class))


/* --- object member/convenience macros --- */
#define BSE_OBJECT_TYPE(object)           (BSE_STRUCT_TYPE (object))
#define BSE_OBJECT_TYPE_NAME(object)      (bse_type_name (BSE_OBJECT_TYPE (object)))
#define BSE_OBJECT_NAME(object)           ((gchar*) g_datalist_id_get_data (&((BseObject*) (object))->datalist, _bse_quark_name))
#define BSE_OBJECT_FLAGS(object)          (((BseObject*) (object))->flags)
#define BSE_OBJECT_SET_FLAGS(object, f)   (BSE_OBJECT_FLAGS (object) |= (f))
#define BSE_OBJECT_UNSET_FLAGS(object, f) (BSE_OBJECT_FLAGS (object) &= ~(f))
#define BSE_OBJECT_DESTROYED(object)      ((BSE_OBJECT_FLAGS (object) & BSE_OBJECT_FLAG_DESTROYED) != 0)
#define BSE_OBJECT_IN_PARAM_CHANGED(obj)  ((BSE_OBJECT_FLAGS (obj) & BSE_OBJECT_FLAG_IN_PARAM_CHANGED) != 0)
#define BSE_OBJECT_IS_LOCKED(obj)         (((BseObject*) (obj))->lock_count > 0)


/* --- bse object flags --- */
typedef enum                            /*< skip >*/
{
  BSE_OBJECT_FLAG_CONSTRUCTED           = 1 << 0,
  BSE_OBJECT_FLAG_DESTROYED             = 1 << 1,
  BSE_OBJECT_FLAG_IN_PARAM_CHANGED      = 1 << 2
} BseObjectFlags;
#define BSE_OBJECT_FLAGS_USHIFT     (3)
#define BSE_OBJECT_FLAGS_MAX_SHIFT  (16)

typedef struct _BseObjectParser BseObjectParser;


/* --- typedefs & structures --- */
typedef gpointer   (*BseInterfaceDataNew)     (BseObject     *object);
typedef void       (*BseObjectGetParamFunc)   (BseObject     *object,
					       BseParam      *param);
typedef void       (*BseObjectSetParamFunc)   (BseObject     *object,
					       BseParam      *param);
typedef GTokenType (*BseObjectParseStatement) (BseObject     *object,
					       BseStorage    *storage,
					       gpointer       user_data);
struct _BseObject
{
  BseTypeStruct          bse_struct;
  
  /* pack into one guint */
  guint16                flags;
  guint16                lock_count;
  
  guint                  ref_count;

  /* we don't feature gchar *name; directly here, since not all
   * objects really need names, and BseObject needs to be a fairly
   * lightweight structure under certain circumstances.
   * the object name is kept in the datalist instead.
   */
  GData                  *datalist;
};
struct _BseObjectClass
{
  BseTypeClass           bse_class;

  guint                  n_params;
  BseParamSpec         **param_specs;

  /* internal stuff */
  guint                  n_notifiers;
  GQuark                *notifiers;
  guint                  n_parsers;
  BseObjectParser       *parsers;
  void                  (*store_termination)    (BseObject      *object,
                                                 BseStorage     *storage);
  BseTokenType          (*try_statement)        (BseObject      *object,
                                                 BseStorage     *storage);
  GTokenType            (*restore)              (BseObject      *object,
                                                 BseStorage     *storage);

  /* methods to be implemented per-instance */
  BseObjectSetParamFunc  set_param;
  BseObjectGetParamFunc  get_param;

  /* custom methods for specific object needs, most of them require chaining */
  void                  (*set_name)             (BseObject      *object,
                                                 const gchar    *name);
  void                  (*store_private)        (BseObject      *object,
                                                 BseStorage     *storage);
  BseTokenType          (*restore_private)      (BseObject      *object,
                                                 BseStorage     *storage);
  void                  (*unlocked)             (BseObject      *object);
  BseIcon*		(*get_icon)             (BseObject      *object);
  void                  (*shutdown)             (BseObject      *object);
  void                  (*destroy)              (BseObject      *object);
};


/* --- object class prototypes ---*/
void            bse_object_class_add_param      (BseObjectClass *oclass,
                                                 const gchar    *param_group,
                                                 guint           param_id,
                                                 BseParamSpec   *pspec);
BseParamSpec*   bse_object_class_get_param_spec (BseObjectClass *oclass,
                                                 const gchar    *param_name);
void            bse_object_class_add_parser     (BseObjectClass *oclass,
                                                 const gchar    *token,
                                                 BseObjectParseStatement parse_func,
                                                 gpointer        user_data);


/* --- object prototypes --- */
gpointer        bse_object_new                  (BseType         type,
                                                 const gchar    *first_param_name,
                                                 ...);
gpointer        bse_object_new_valist           (BseType         type,
                                                 const gchar    *first_param_name,
                                                 va_list         var_args);
void            bse_object_set                  (BseObject      *object,
                                                 const gchar    *first_param_name,
                                                 ...);
void            bse_object_set_valist           (BseObject      *object,
                                                 const gchar    *first_param_name,
                                                 va_list         var_args);
void            bse_object_param_changed        (BseObject      *object,
                                                 const gchar    *param_name);
void            bse_object_ref                  (BseObject      *object);
void            bse_object_unref                (BseObject      *object);
void            bse_object_lock                 (BseObject      *object);
void            bse_object_unlock               (BseObject      *object);
void            bse_object_set_name             (BseObject      *object,
                                                 const gchar    *name);
gchar*          bse_object_get_name             (BseObject      *object);
gchar*          bse_object_get_name_or_type     (BseObject      *object);
void            bse_object_set_blurb            (BseObject      *object,
                                                 const gchar    *blurb);
gchar*          bse_object_get_blurb            (BseObject      *object);
gpointer        bse_object_get_data             (BseObject      *object,
                                                 const gchar    *key);
gpointer        bse_object_get_qdata            (BseObject      *object,
                                                 GQuark          quark);
void            bse_object_set_data             (BseObject      *object,
                                                 const gchar    *key,
                                                 gpointer        data);
void            bse_object_set_data_full        (BseObject      *object,
                                                 const gchar    *key,
                                                 gpointer        data,
                                                 GDestroyNotify  destroy);
void            bse_object_set_qdata            (BseObject      *object,
                                                 GQuark          quark,
                                                 gpointer        data);
void            bse_object_set_qdata_full       (BseObject      *object,
                                                 GQuark          quark,
                                                 gpointer        data,
                                                 GDestroyNotify  destroy);
gpointer        bse_object_steal_qdata          (BseObject      *object,
                                                 GQuark          quark);
BseIcon*        bse_object_get_icon             (BseObject      *object);
void		bse_object_notify_icon_changed  (BseObject      *object);
gpointer        bse_object_ensure_interface_data(BseObject      *object,
                                                 BseType         interface_type,
                                                 BseInterfaceDataNew new_func,
                                                 GDestroyNotify  destroy_func);
gpointer        bse_object_get_interface_data   (BseObject      *object,
                                                 BseType         interface_type);
gpointer        bse_object_get_interface        (BseObject      *object,
                                                 BseType         interface_type);
void            bse_object_get_param            (BseObject      *object,
                                                 BseParam       *param);
void            bse_object_set_param            (BseObject      *object,
                                                 BseParam       *param);
GList*          bse_objects_list                (BseType         type);
GList*          bse_objects_list_by_name        (BseType         type,
                                                 const gchar    *name);
void            bse_object_store                (BseObject      *object,
                                                 BseStorage     *storage);
GTokenType      bse_object_restore              (BseObject      *object,
                                                 BseStorage     *storage);
void            bse_object_remove_notifier      (gpointer        object,
                                                 guint           id);
guint           bse_object_add_notifier         (gpointer        object,
                                                 const gchar    *method,
                                                 gpointer        func,
                                                 gpointer        data);
guint           bse_object_add_data_notifier    (gpointer        object,
                                                 const gchar    *method,
                                                 gpointer        func,
                                                 gpointer        data);
guint           bse_object_add_notifier_full    (gpointer        object,
                                                 const gchar    *method,
                                                 gpointer        func,
                                                 gpointer        data,
                                                 GDestroyNotify  destroy);
guint           bse_object_add_data_notifier_full   (gpointer       object,
                                                     const gchar   *method,
                                                     gpointer       func,
                                                     gpointer       data,
                                                     GDestroyNotify destroy);
void            bse_object_remove_notifiers_by_func (gpointer       object,
                                                     gpointer       func,
                                                     gpointer       data);
void            bse_nullify_pointer                 (gpointer      *pointer_loc);


/* --- implementation details --- */
const gchar*    bse_object_type_register        (const gchar *name,
                                                 const gchar *parent_name,
                                                 const gchar *blurb,
                                                 BsePlugin   *plugin,
                                                 BseType     *ret_type);
extern GQuark _bse_quark_name;
#define BSE_OBJECT_GET_INTERFACE(op, type_id, type) \
    ((type*) bse_object_get_interface ((BseObject*) (op), (type_id)))
GHookList*      bse_object_get_hook_list        (BseObject      *object);
struct _BseNotifyHook
{
  GHook  hook;
  GQuark quark;
};
#define BSE_NOTIFY_FLAG_CALL_DATA       (1 << G_HOOK_FLAG_USER_SHIFT)
/* bse_object_add_notifier_full (song, get_param, my_cb, data, data_destroy);
 * BSE_NOTIFY (song, get_param, NOTIFY (OBJECT, id, param, DATA));
 */
#define BSE_NOTIFY(__obj, __method, __CALL) \
    BSE_NOTIFY_CHECK (__obj, __method, __CALL, if (!BSE_OBJECT_DESTROYED (__object)))
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
