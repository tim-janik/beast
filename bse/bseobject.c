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
 */
#include	"bseobject.h"

#include	"bseexports.h"
#include	"bsestorage.h"
#include	"bseparasite.h"
#include	"bsecategories.h"	/* FIXME */

enum
{
  PARAM_0,
  PARAM_NAME,
  PARAM_BLURB
};


/* -- structures --- */
struct _BseObjectParser
{
  gchar			 *token;
  BseObjectParseStatement parser;
  gpointer		  user_data;
};


/* --- notifiers --- */
static const struct {
  const gchar *notifier;
  const gchar *object;
} bse_notifiers[] = {
#include "bsenotifier_array.c"
  { NULL, NULL, },
};


/* --- prototypes --- */
static void		bse_object_class_base_init	(BseObjectClass	*class);
static void		bse_object_class_base_finalize	(BseObjectClass	*class);
static void		bse_object_class_init		(BseObjectClass	*class);
static void		bse_object_init			(BseObject	*object);
static void	   bse_object_do_dispatch_param_changed (GObject        *object,
							 GParamSpec     *pspec);
static void		bse_object_do_shutdown		(GObject	*gobject);
static void		bse_object_do_destroy		(BseObject	*object);
static void		bse_object_do_set_param		(BseObject	*object,
							 guint           param_id,
							 GValue         *value,
							 GParamSpec     *pspec,
							 const gchar    *trailer);
static void		bse_object_do_get_param		(BseObject	*object,
							 guint           param_id,
							 GValue         *value,
							 GParamSpec     *pspec,
							 const gchar    *trailer);
static void		bse_object_do_set_name		(BseObject	*object,
							 const gchar	*name);
static void		bse_object_do_store_private	(BseObject	*object,
							 BseStorage	*storage);
static void		bse_object_do_store_after	(BseObject	*object,
							 BseStorage	*storage);
static BseTokenType	bse_object_do_restore_private	(BseObject     *object,
							 BseStorage    *storage);
static BseTokenType	bse_object_do_try_statement	(BseObject	*object,
							 BseStorage	*storage);
static GTokenType	bse_object_do_restore		(BseObject	*object,
							 BseStorage	*storage);
static BseIcon*		bse_object_do_get_icon		(BseObject	*object);


/* --- variables --- */
static gpointer	   parent_class = NULL;
GQuark		   _bse_quark_name = 0;
static GQuark	   quark_blurb = 0;
static GQuark	   quark_hook_list = 0;
static GHashTable *object_names_ht = NULL;
static GQuark	   quark_param_changed_queue = 0;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseObject)
{
  static const GTypeInfo object_info = {
    sizeof (BseObjectClass),
    
    (GBaseInitFunc) bse_object_class_base_init,
    (GBaseFinalizeFunc) bse_object_class_base_finalize,
    (GClassInitFunc) bse_object_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseObject),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_object_init,
  };
  
  return bse_type_register_static (G_TYPE_OBJECT,
				   "BseObject",
				   "BSE Object Hierarchy base type",
				   &object_info);
}

void
bse_object_complete_info (const BseExportSpec *spec,
			  GTypeInfo	    *info)
{
  const BseExportObject *ospec = &spec->s_object;
  
  *info = *ospec->object_info;
}

const gchar*
bse_object_type_register (const gchar *name,
			  const gchar *parent_name,
			  const gchar *blurb,
			  BsePlugin   *plugin,
			  GType	      *ret_type)
{
  GType	  type;
  
  g_return_val_if_fail (ret_type != NULL, bse_error_blurb (BSE_ERROR_INTERNAL));
  *ret_type = 0;
  g_return_val_if_fail (name != NULL, bse_error_blurb (BSE_ERROR_INTERNAL));
  g_return_val_if_fail (parent_name != NULL, bse_error_blurb (BSE_ERROR_INTERNAL));
  g_return_val_if_fail (plugin != NULL, bse_error_blurb (BSE_ERROR_INTERNAL));
  
  type = g_type_from_name (name);
  if (type)
    return "Object already registered";
  type = g_type_from_name (parent_name);
  if (!type)
    return "Parent type unknown";
  if (!BSE_TYPE_IS_OBJECT (type))
    return "Parent type is non-object type";
  
  type = bse_type_register_dynamic (type,
				    name,
				    blurb,
				    plugin);
  *ret_type = type;
  
  return NULL;
}

static void
bse_object_class_base_init (BseObjectClass *class)
{
  guint i;
  
  class->n_notifiers = 0;
  class->notifiers = NULL;
  for (i = 0; bse_notifiers[i].notifier && bse_notifiers[i].object; i++)
    if (g_type_conforms_to (BSE_CLASS_TYPE (class),
			    g_type_from_name (bse_notifiers[i].object)))
      {
	class->notifiers = g_renew (GQuark, class->notifiers, class->n_notifiers + 1);
	class->notifiers[class->n_notifiers] = g_quark_from_static_string (bse_notifiers[i].notifier);
	class->n_notifiers++;
	
	BSE_IF_DEBUG (NOTIFY)
	  g_message ("%s: + %s::%s",
		     BSE_CLASS_NAME (class),
		     bse_notifiers[i].object,
		     bse_notifiers[i].notifier);
      }
  
  class->n_parsers = 0;
  class->parsers = NULL;
}

static void
bse_object_class_base_finalize (BseObjectClass *class)
{
  guint i;
  
  g_free (class->notifiers);
  
  for (i = 0; i < class->n_parsers; i++)
    g_free (class->parsers[i].token);
  g_free (class->parsers);
}

static void
bse_object_class_init (BseObjectClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  guint i;
  
  parent_class = g_type_class_peek (G_TYPE_OBJECT);
  
  _bse_quark_name = g_quark_from_static_string ("bse-object-name");
  quark_hook_list = g_quark_from_static_string ("bse-hook-list");
  quark_param_changed_queue = g_quark_from_static_string ("bse-param-changed-queue");
  object_names_ht = g_hash_table_new (bse_string_hash, bse_string_equals);
  
  gobject_class->get_param = (GObjectGetParamFunc) bse_object_do_get_param;
  gobject_class->set_param = (GObjectSetParamFunc) bse_object_do_set_param;
  gobject_class->dispatch_param_changed = bse_object_do_dispatch_param_changed;
  gobject_class->shutdown = bse_object_do_shutdown;
  
  class->store_after = bse_object_do_store_after;
  class->try_statement = bse_object_do_try_statement;
  class->restore = bse_object_do_restore;
  
  class->set_name = bse_object_do_set_name;
  class->store_private = bse_object_do_store_private;
  class->restore_private = bse_object_do_restore_private;
  class->unlocked = NULL;
  class->get_icon = bse_object_do_get_icon;
  class->destroy = bse_object_do_destroy;
  
  bse_object_class_add_param (class, NULL,
			      PARAM_NAME,
			      b_param_spec_cstring ("name", "Name", NULL,
						    NULL,
						    B_PARAM_GUI
						    /* this is only half way true,
						     * ->name is *very* specially
						     * treated within the various
						     * objects, especially BseItem
						     * and BseContainer.
						     */));
  bse_object_class_add_param (class, NULL,
			      PARAM_BLURB,
			      b_param_spec_string ("blurb", "Comment", NULL,
						   NULL,
						   B_PARAM_DEFAULT |
						   B_PARAM_HINT_CHECK_NULL));
  
  /* perform neccessary checks for assumptions we make over generated sources
   */
  for (i = 0; bse_notifiers[i].notifier || bse_notifiers[i].object; i++)
    if (!bse_notifiers[i].notifier ||
	!g_type_conforms_to (g_type_from_name (bse_notifiers[i].object), BSE_TYPE_OBJECT))
      g_error ("notifier entry (\"%s\", `%s') refers to invalid (unknown) type or name",
	       bse_notifiers[i].notifier, bse_notifiers[i].object);
  
  /* feature parasites */
  bse_parasite_install_parsers (class);
}

static inline void
object_names_ht_insert (BseObject *object)
{
  GSList *object_slist;
  
  g_hash_table_freeze (object_names_ht);
  object_slist = g_hash_table_lookup (object_names_ht, BSE_OBJECT_NAME (object));
  if (object_slist)
    g_hash_table_remove (object_names_ht, BSE_OBJECT_NAME (object_slist->data));
  object_slist = g_slist_prepend (object_slist, object);
  g_hash_table_insert (object_names_ht, BSE_OBJECT_NAME (object_slist->data), object_slist);
  g_hash_table_thaw (object_names_ht);
}

static void
bse_object_init (BseObject *object)
{
  object->flags = BSE_OBJECT_FLAG_CONSTRUCTED;
  object->lock_count = 0;
  
  object_names_ht_insert (object);
}

static inline void
object_names_ht_remove (BseObject *object)
{
  GSList *object_slist, *orig_slist;
  
  g_hash_table_freeze (object_names_ht);
  object_slist = g_hash_table_lookup (object_names_ht, BSE_OBJECT_NAME (object));
  orig_slist = object_slist;
  object_slist = g_slist_remove (object_slist, object);
  if (object_slist != orig_slist)
    {
      g_hash_table_remove (object_names_ht, BSE_OBJECT_NAME (object));
      if (object_slist)
	g_hash_table_insert (object_names_ht, BSE_OBJECT_NAME (object_slist->data), object_slist);
    }
  g_hash_table_thaw (object_names_ht);
}

static void
bse_object_do_set_name (BseObject   *object,
			const gchar *name)
{
  bse_object_set_qdata_full (object, _bse_quark_name, g_strdup (name), name ? g_free : NULL);
}

static void
bse_object_do_set_param (BseObject   *object,
			 guint        param_id,
			 GValue      *value,
			 GParamSpec  *pspec,
			 const gchar *trailer)
{
  switch (param_id)
    {
      gchar *string;
      
    case PARAM_NAME:
      object_names_ht_remove (object);
      string = bse_strdup_stripped (b_value_get_string (value));
      BSE_OBJECT_GET_CLASS (object)->set_name (object, string);
      g_free (string);
      object_names_ht_insert (object);
      BSE_NOTIFY (object, name_set, NOTIFY (OBJECT, DATA));
      break;
    case PARAM_BLURB:
      if (!quark_blurb)
	quark_blurb = g_quark_from_static_string ("bse-blurb");
      string = bse_strdup_stripped (b_value_get_string (value));
      if (b_value_get_string (value) && !string) /* preserve NULL vs. "" distinction */
	string = g_strdup ("");
      bse_object_set_qdata_full (object, quark_blurb, string, string ? g_free : NULL);
      break;
    default:
      G_WARN_INVALID_PARAM_ID (object, param_id, pspec);
      break;
    }
}

static void
bse_object_do_get_param (BseObject   *object,
			 guint        param_id,
			 GValue      *value,
			 GParamSpec  *pspec,
			 const gchar *trailer)
{
  switch (param_id)
    {
    case PARAM_NAME:
      g_value_set_string (value, BSE_OBJECT_NAME (object));
      break;
    case PARAM_BLURB:
      g_value_set_string (value, bse_object_get_qdata (object, quark_blurb));
      break;
    default:
      G_WARN_INVALID_PARAM_ID (object, param_id, pspec);
      break;
    }
}

static inline gboolean
bse_object_class_check_notifier (BseObjectClass *class,
				 GQuark		 quark)
{
  guint i;
  
  for (i = 0; i < class->n_notifiers; i++)
    if (class->notifiers[i] == quark)
      return TRUE;
  
  return FALSE;
}

void
bse_object_class_add_param (BseObjectClass *class,
			    const gchar	   *param_group,
			    guint	    param_id,
			    GParamSpec     *pspec)
{
  g_return_if_fail (BSE_IS_OBJECT_CLASS (class));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  g_return_if_fail (b_param_spec_get_group (pspec) == NULL);
  g_return_if_fail (param_id > 0);
  
  b_param_spec_set_group (pspec, param_group);
  g_object_class_install_param (G_OBJECT_CLASS (class), param_id, pspec);
}

gpointer
bse_object_ref (gpointer object)
{
  g_return_val_if_fail (BSE_IS_OBJECT (object), NULL);
  
  g_object_ref (object);
  
  return object;
}

void
bse_object_unref (gpointer object)
{
  g_return_if_fail (BSE_IS_OBJECT (object));
  
  g_object_unref (object);
}

gpointer
bse_object_new_valist (GType	    type,
		       const gchar *first_param_name,
		       va_list	    var_args)
{
  BseObject *object;
  
  g_return_val_if_fail (BSE_TYPE_IS_OBJECT (type), NULL);
  
  object = g_object_new_valist (type, first_param_name, var_args);
  
  return object;
}

gpointer
bse_object_new (GType	     type,
		const gchar *first_param_name,
		...)
{
  BseObject *object;
  va_list var_args;
  
  g_return_val_if_fail (BSE_TYPE_IS_OBJECT (type), NULL);
  
  va_start (var_args, first_param_name);
  object = g_object_new_valist (type, first_param_name, var_args);
  va_end (var_args);
  
  return object;
}

void
bse_object_set (BseObject   *object,
		const gchar *first_param_name,
		...)
{
  va_list var_args;
  
  g_return_if_fail (BSE_IS_OBJECT (object));
  
  va_start (var_args, first_param_name);
  g_object_set_valist (G_OBJECT (object), first_param_name, var_args);
  va_end (var_args);
}

void
bse_object_get (BseObject   *object,
		const gchar *first_param_name,
		...)
{
  va_list var_args;
  
  g_return_if_fail (BSE_IS_OBJECT (object));
  
  va_start (var_args, first_param_name);
  g_object_get_valist (G_OBJECT (object), first_param_name, var_args);
  va_end (var_args);
}

void
bse_object_lock (BseObject *object)
{
  GObject *gobject = (GObject*) object;
  
  g_return_if_fail (BSE_IS_OBJECT (object));
  g_return_if_fail (gobject->ref_count > 0);
  
  g_assert (object->lock_count < 65535);	// if this breaks, we need to fix the guint16
  
  if (!object->lock_count)
    {
      bse_object_ref (object);
      
      /* we also keep the globals locked so we don't need to duplicate
       * this all over the place
       */
      bse_globals_lock ();
    }
  
  object->lock_count += 1;
}

void
bse_object_unlock (BseObject *object)
{
  g_return_if_fail (BSE_IS_OBJECT (object));
  g_return_if_fail (object->lock_count > 0);
  
  object->lock_count -= 1;
  
  if (!object->lock_count)
    {
      /* release global lock */
      bse_globals_unlock ();
      
      if (BSE_OBJECT_GET_CLASS (object)->unlocked)
	BSE_OBJECT_GET_CLASS (object)->unlocked (object);
      
      bse_object_unref (object);
    }
}

static void
bse_object_do_shutdown (GObject *gobject)
{
  BseObject *object = BSE_OBJECT (gobject);
  
  g_return_if_fail (gobject->ref_count == 1);
  
  /* complete shutdown process, by chaining
   * parent class' shutdown handler
   */
  G_OBJECT_CLASS (parent_class)->shutdown (gobject);
  
  g_return_if_fail (gobject->ref_count == 1);
  
  /* perform destroy notification */
  BSE_NOTIFY_CHECK (object, destroy, NOTIFY (OBJECT, DATA), /* always */);
  
  g_return_if_fail (gobject->ref_count == 1);
  
  /* remove all notifiers */
  g_datalist_id_set_data (&gobject->qdata, quark_hook_list, NULL);
  
  g_return_if_fail (gobject->ref_count == 1);
  
  /* invoke destroy method */
  BSE_OBJECT_GET_CLASS (object)->destroy (object);
  
  g_return_if_fail (gobject->ref_count == 1);
}

static void
bse_object_do_destroy (BseObject *object)
{
  /* remove object from hash list *before* clearing data list,
   * since the object name is kept in the datalist!
   */
  object_names_ht_remove (object);
}

static void
bse_object_do_dispatch_param_changed (GObject    *object,
				      GParamSpec *pspec)
{
  g_return_if_fail (BSE_IS_OBJECT (object));
  
  BSE_NOTIFY (object, param_changed, NOTIFY (OBJECT, pspec, DATA));
}

void
bse_object_param_changed (BseObject   *object,
			  const gchar *param_name)
{
  g_return_if_fail (BSE_IS_OBJECT (object));
  g_return_if_fail (param_name != NULL);
  
  g_object_queue_param_changed (G_OBJECT (object), param_name);
}

GList*
bse_objects_list_by_name (GType	       type,
			  const gchar *name)
{
  GList *object_list = NULL;
  
  g_return_val_if_fail (BSE_TYPE_IS_OBJECT (type) == TRUE, NULL);
  
  if (object_names_ht)
    {
      GSList *object_slist, *slist;
      
      object_slist = g_hash_table_lookup (object_names_ht, name);
      
      for (slist = object_slist; slist; slist = slist->next)
	if (g_type_is_a (BSE_OBJECT_TYPE (slist->data), type))
	  object_list = g_list_prepend (object_list, slist->data);
    }
  
  return object_list;
}

static void
list_objects (gpointer key,
	      gpointer value,
	      gpointer user_data)
{
  GSList *slist;
  gpointer *data = user_data;
  
  for (slist = value; slist; slist = slist->next)
    if (g_type_is_a (BSE_OBJECT_TYPE (slist->data), GPOINTER_TO_UINT (data[1])))
      data[0] = g_list_prepend (data[0], slist->data);
}

GList*
bse_objects_list (GType	  type)
{
  g_return_val_if_fail (BSE_TYPE_IS_OBJECT (type) == TRUE, NULL);
  
  if (object_names_ht)
    {
      gpointer data[2] = { NULL, GUINT_TO_POINTER (type), };
      
      g_hash_table_foreach (object_names_ht, list_objects, data);
      
      return data[0];
    }
  
  return NULL;
}

void
bse_object_set_name (BseObject	 *object,
		     const gchar *name)
{
  g_return_if_fail (BSE_IS_OBJECT (object));
  g_return_if_fail (name != NULL);
  
  bse_object_set (object,
		  "name", name,
		  NULL);
}

gchar*
bse_object_get_name_or_type (BseObject *object)
{
  gchar *name;
  
  g_return_val_if_fail (BSE_IS_OBJECT (object), NULL);
  
  name = BSE_OBJECT_NAME (object);
  
  return name ? name : BSE_OBJECT_TYPE_NAME (object);
}

void
bse_object_set_data (BseObject	 *object,
		     const gchar *key,
		     gpointer	  data)
{
  GObject *gobject = (GObject*) object;
  
  g_return_if_fail (BSE_IS_OBJECT (object));
  
  g_datalist_set_data (&gobject->qdata, key, data);
}

void
bse_object_set_data_full (BseObject	*object,
			  const gchar	*key,
			  gpointer	 data,
			  GDestroyNotify destroy)
{
  GObject *gobject = (GObject*) object;
  
  g_return_if_fail (BSE_IS_OBJECT (object));
  
  g_datalist_set_data_full (&gobject->qdata, key, data, data ? destroy : NULL);
}

gpointer
bse_object_get_data (BseObject	 *object,
		     const gchar *key)
{
  GObject *gobject = (GObject*) object;
  
  g_return_val_if_fail (BSE_IS_OBJECT (object), NULL);
  
  return g_datalist_get_data (&gobject->qdata, key);
}

static void
hook_list_destroy (gpointer data)
{
  GHookList *hook_list = data;
  
  g_hook_list_clear (hook_list);
  /* FIXME hook_list needs destruction notifier */
  if (hook_list->hooks || hook_list->hook_memchunk)
    g_warning (G_STRLOC ": hook_list destruction failed, leaking memory...");
  else
    g_free (hook_list);
}

GHookList*
bse_object_get_hook_list (BseObject *object)
{
  GObject *gobject = (GObject*) object;
  
  g_return_val_if_fail (BSE_IS_OBJECT (object), NULL);
  
  return g_datalist_id_get_data (&gobject->qdata, quark_hook_list);
}

static inline guint
bse_object_add_notifier_i (BseObject	 *object,
			   const gchar	 *method,
			   BseFunc	  func,
			   gpointer	  data,
			   GDestroyNotify destroy,
			   gboolean	  is_data)
{
  GObject *gobject = (GObject*) object;
  static guint seq_hook_id = 1;
  GHookList *hook_list;
  GHook *hook;
  GQuark quark;
  
  quark = g_quark_try_string (method);
  if (!quark || !bse_object_class_check_notifier (BSE_OBJECT_GET_CLASS (object), quark))
    {
      gchar *name;
      
      name = g_strdup (method);
      g_strcanon (name, G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "_", '_');
      quark = g_quark_try_string (name);
      g_free (name);
      
      if (!quark || !bse_object_class_check_notifier (BSE_OBJECT_GET_CLASS (object), quark))
	{
	  g_warning (G_STRLOC ": unable to find notify method \"%s\" in the `%s' class",
		     method,
		     BSE_OBJECT_TYPE_NAME (object));
	  return 0;
	}
    }
  
  hook_list = g_datalist_id_get_data (&gobject->qdata, quark_hook_list);
  if (!hook_list)
    {
      hook_list = g_new0 (GHookList, 1);
      g_hook_list_init (hook_list, sizeof (BseNotifyHook));
      g_datalist_id_set_data_full (&gobject->qdata, quark_hook_list, hook_list, hook_list_destroy);
    }
  
  hook = g_hook_alloc (hook_list);
  hook->func = func;
  hook->data = data;
  hook->destroy = destroy;
  if (is_data)
    hook->flags |= BSE_NOTIFY_FLAG_CALL_DATA;
  ((BseNotifyHook*) hook)->quark = quark;
  
  hook_list->seq_id = seq_hook_id;
  g_hook_append (hook_list, hook);
  seq_hook_id = hook_list->seq_id;
  
  return hook->hook_id;
}

guint
bse_object_add_notifier (gpointer     object,
			 const gchar *method,
			 gpointer     func,
			 gpointer     data)
{
  g_return_val_if_fail (BSE_IS_OBJECT (object), 0);
  g_return_val_if_fail (method != NULL, 0);
  g_return_val_if_fail (func != NULL, 0);
  
  return bse_object_add_notifier_i (object, method, func, data, NULL, 0);
}

guint
bse_object_add_data_notifier (gpointer	   object,
			      const gchar *method,
			      gpointer	   func,
			      gpointer	   data)
{
  g_return_val_if_fail (BSE_IS_OBJECT (object), 0);
  g_return_val_if_fail (method != NULL, 0);
  g_return_val_if_fail (func != NULL, 0);
  
  return bse_object_add_notifier_i (object, method, func, data, NULL, 1);
}

guint
bse_object_add_notifier_full (gpointer	     object,
			      const gchar   *method,
			      gpointer	     func,
			      gpointer	     data,
			      GDestroyNotify destroy)
{
  g_return_val_if_fail (BSE_IS_OBJECT (object), 0);
  g_return_val_if_fail (method != NULL, 0);
  g_return_val_if_fail (func != NULL, 0);
  
  return bse_object_add_notifier_i (object, method, func, data, destroy, 0);
}

guint
bse_object_add_data_notifier_full (gpointer	  object,
				   const gchar	 *method,
				   gpointer	  func,
				   gpointer	  data,
				   GDestroyNotify destroy)
{
  g_return_val_if_fail (BSE_IS_OBJECT (object), 0);
  g_return_val_if_fail (method != NULL, 0);
  g_return_val_if_fail (func != NULL, 0);
  
  return bse_object_add_notifier_i (object, method, func, data, destroy, 1);
}

static inline void
bse_object_remove_notifiers_i (BseObject   *object,
			       const gchar *method,
			       BseFunc	    func,
			       gpointer	    data,
			       guint	    mask)
{
  GObject *gobject = (GObject*) object;
  GHookList *hook_list;
  gboolean found_one = FALSE;
  
  hook_list = g_datalist_id_get_data (&gobject->qdata, quark_hook_list);
  if (hook_list)
    {
      GHook *hook;
      GQuark quark = (mask & 1) ? g_quark_try_string (method) : 0;
      
      mask = ~mask;
      
      hook = hook_list->hooks;
      while (hook)
	{
	  if ((mask & 1 || ((BseNotifyHook*) hook)->quark == quark) &&
	      (mask & 2 || hook->func == func) &&
	      (mask & 4 || hook->data == data) &&
	      hook->hook_id)
	    {
	      GHook *tmp;
	      
	      tmp = hook;
	      hook = hook->next;
	      
	      g_hook_destroy_link (hook_list, tmp);
	      found_one = TRUE;
	    }
	  else
	    hook = hook->next;
	}
    }
  
  if (!found_one)
    g_warning (G_STRLOC ": couldn't remove notifier from %s \"%s\"",
	       BSE_OBJECT_TYPE_NAME (object),
	       BSE_OBJECT_NAME (object));
}

void
bse_object_remove_notifiers_by_func (gpointer object,
				     gpointer func,
				     gpointer data)
{
  g_return_if_fail (BSE_IS_OBJECT (object));
  g_return_if_fail (func != NULL);
  
  bse_object_remove_notifiers_i (object, NULL, func, data, 0 | 2 | 4);
}

void
bse_object_remove_notifier (gpointer _object,
			    guint     id)
{
  BseObject *object = _object;
  GObject *gobject = (GObject*) object;
  GHookList *hook_list;
  guint oid = id;
  
  g_return_if_fail (BSE_IS_OBJECT (object));
  g_return_if_fail (id > 0);
  
  hook_list = g_datalist_id_get_data (&gobject->qdata, quark_hook_list);
  if (hook_list)
    id = !g_hook_destroy (hook_list, id);
  
  if (id)
    g_warning (G_STRLOC ": couldn't remove notifier (%u) from %s \"%s\"",
	       oid,
	       BSE_OBJECT_TYPE_NAME (object),
	       BSE_OBJECT_NAME (object));
}

void
bse_object_class_add_parser (BseObjectClass	    *class,
			     const gchar	    *token,
			     BseObjectParseStatement parse_func,
			     gpointer		     user_data)
{
  guint n;
  
  g_return_if_fail (BSE_IS_OBJECT_CLASS (class));
  g_return_if_fail (token != NULL);
  g_return_if_fail (parse_func != NULL);
  
  n = class->n_parsers++;
  class->parsers = g_renew (BseObjectParser, class->parsers, class->n_parsers);
  class->parsers[n].token = g_strdup (token);
  class->parsers[n].parser = parse_func;
  class->parsers[n].user_data = user_data;
}

static BseObjectParser*
bse_object_class_get_parser (BseObjectClass *class,
			     const gchar    *token)
{
  g_return_val_if_fail (token != NULL, NULL);
  
  do
    {
      guint i;
      
      for (i = 0; i < class->n_parsers; i++)
	if (strcmp (class->parsers[i].token, token) == 0)
	  return class->parsers + i;
      
      class = g_type_class_peek_parent (class);
    }
  while (BSE_IS_OBJECT_CLASS (class));
  
  return NULL;
}

void
bse_object_notify_icon_changed (BseObject *object)
{
  g_return_if_fail (BSE_IS_OBJECT (object));
  
  BSE_NOTIFY (object, icon_changed, NOTIFY (OBJECT, DATA));
}

BseIcon*
bse_object_get_icon (BseObject *object)
{
  BseIcon *icon;
  
  g_return_val_if_fail (BSE_IS_OBJECT (object), NULL);
  
  bse_object_ref (object);
  
  icon = BSE_OBJECT_GET_CLASS (object)->get_icon (object);
  
  bse_object_unref (object);
  
  return icon;
}

static BseIcon*
bse_object_do_get_icon (BseObject *object)
{
  BseCategory *cats;
  guint n_cats, i;
  
  g_return_val_if_fail (BSE_IS_OBJECT (object), NULL);
  
  /* FIXME: this is a gross hack, we should store the first per-type
   * category icon as static type-data and fetch that from here
   */
  
  cats = bse_categories_from_type (BSE_OBJECT_TYPE (object), &n_cats);
  for (i = 0; i < n_cats; i++)
    {
      BseIcon *icon = cats[i].icon;
      
      if (icon)
	{
	  g_free (cats);
	  
	  return icon;
	}
    }
  
  g_free (cats);
  
  return NULL;
}

void
bse_object_store (BseObject  *object,
		  BseStorage *storage)
{
  g_return_if_fail (BSE_IS_OBJECT (object));
  g_return_if_fail (BSE_IS_STORAGE (storage));
  
  bse_object_ref (object);
  
  if (BSE_OBJECT_GET_CLASS (object)->store_private)
    BSE_OBJECT_GET_CLASS (object)->store_private (object, storage);
  
  BSE_NOTIFY (object, store, NOTIFY (OBJECT, storage, DATA));
  
  if (BSE_OBJECT_GET_CLASS (object)->store_after)
    BSE_OBJECT_GET_CLASS (object)->store_after (object, storage);
  
  bse_storage_handle_break (storage);
  bse_storage_putc (storage, ')');
  
  bse_object_unref (object);
}

static void
bse_object_do_store_after (BseObject  *object,
			   BseStorage *storage)
{
}

static void
bse_object_do_store_private (BseObject	*object,
			     BseStorage *storage)
{
  GObjectClass *class;
  GSList *slist, *class_list = NULL;
  
  /* dump the object paramters, starting out
   * at the base class
   */
  class = G_OBJECT_GET_CLASS (object);
  while (class)
    {
      class_list = g_slist_prepend (class_list, class);
      class = g_type_class_peek_parent (class);
    }
  for (slist = class_list; slist; slist = slist->next)
    {
      guint i;
      
      class = slist->data;
      
      for (i = 0; i < class->n_param_specs; i++)
	{
	  GParamSpec *pspec = class->param_specs[i];
	  
	  if (pspec->flags & B_PARAM_SERVE_STORAGE)
	    {
	      GValue value = { 0, };
	      
	      g_value_init (&value, G_PARAM_SPEC_TYPE (pspec));
	      g_object_get_param (G_OBJECT (object), pspec->name, &value);
	      if (!g_value_defaults (&value, pspec) || BSE_STORAGE_PUT_DEFAULTS (storage))
		{
		  bse_storage_break (storage);
		  bse_storage_put_param (storage, &value, pspec);
		}
	      g_value_unset (&value);
	    }
	}
    }
  g_slist_free (class_list);
}

GTokenType
bse_object_restore (BseObject  *object,
		    BseStorage *storage)
{
  g_return_val_if_fail (BSE_IS_OBJECT (object), G_TOKEN_ERROR);
  g_return_val_if_fail (BSE_IS_STORAGE (storage), G_TOKEN_ERROR);
  
  if (BSE_OBJECT_GET_CLASS (object)->restore)
    {
      GTokenType expected_token;
      
      bse_object_ref (object);
      expected_token = BSE_OBJECT_GET_CLASS (object)->restore (object, storage);
      bse_object_unref (object);
      
      return expected_token;
    }
  else
    return bse_storage_warn_skip (storage,
				  "`restore' functionality unimplemented for `%s'",
				  BSE_OBJECT_TYPE_NAME (object));
}

static BseTokenType
bse_object_do_try_statement (BseObject	*object,
			     BseStorage *storage)
{
  GScanner *scanner = storage->scanner;
  GTokenType expected_token;
  BseObjectParser *parser;
  
  /* ensure that the statement starts out with an identifier
   */
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER)
    {
      g_scanner_get_next_token (scanner);
      return G_TOKEN_IDENTIFIER;
    }
  
  /* this is pretty much the *only* place where
   * something else than G_TOKEN_NONE may be returned
   * without erroring out.
   *
   * expected_token:
   * G_TOKEN_NONE)	  statement got parsed, advance
   *			  to next statement
   * BSE_TOKEN_UNMATCHED) couldn't parse statement, try further
   * everything else)	  encountered (syntax/semantic) error during parsing,
   *			  bail out
   */
  
  /* ok, lets figure whether the usual object methods can parse
   * this statement
   */
  if (BSE_OBJECT_GET_CLASS (object)->restore_private)
    {
      expected_token = BSE_OBJECT_GET_CLASS (object)->restore_private (object, storage);
      if (expected_token != BSE_TOKEN_UNMATCHED)
	return expected_token;
    }
  
  /* hm, try custom parsing hooks now */
  parser = bse_object_class_get_parser (BSE_OBJECT_GET_CLASS (object),
					scanner->next_value.v_identifier);
  if (parser)
    {
      g_scanner_get_next_token (scanner); /* eat up the identifier */
      
      return parser->parser (object, storage, parser->user_data);
    }
  
  /* no matches
   */
  return BSE_TOKEN_UNMATCHED;
}

static GTokenType
bse_object_do_restore (BseObject  *object,
		       BseStorage *storage)
{
  return bse_storage_parse_rest (storage,
				 (BseTryStatement) BSE_OBJECT_GET_CLASS (object)->try_statement,
				 object,
				 NULL);
}

static BseTokenType
bse_object_do_restore_private (BseObject  *object,
			       BseStorage *storage)
{
  GScanner *scanner = storage->scanner;
  GParamSpec *pspec;
  GValue value = { 0, };
  GTokenType expected_token;
  
  /* we only feature parameter parsing here,
   * so lets figure whether there is something to parse for us
   */
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return BSE_TOKEN_UNMATCHED;
  
  /* ok, we got an identifier, try object parameter lookup
   * we should in theory only get B_PARAM_SERVE_STORAGE
   * parameters here, but due to version changes or even
   * users editing their files, we will simply parse all
   * kinds of parameters here (we might want to at least
   * restrict them to B_PARAM_SERVE_STORAGE and
   * B_PARAM_SERVE_GUI at some point...)
   */
  pspec = g_object_class_find_param_spec (G_OBJECT_GET_CLASS (object),
					  scanner->next_value.v_identifier);
  if (!pspec)
    return BSE_TOKEN_UNMATCHED;
  
  /* ok we got a parameter for this, so eat the token */
  g_scanner_get_next_token (scanner);
  
  /* and away with the parameter parsing... */
  g_value_init (&value, G_PARAM_SPEC_TYPE (pspec));
  expected_token = bse_storage_parse_param_value (storage, &value, pspec);
  if (expected_token != G_TOKEN_NONE)
    {
      g_value_unset (&value);
      /* failed to parse the parameter value */
      return expected_token;
    }
  g_object_set_param (G_OBJECT (object), pspec->name, &value);
  g_value_unset (&value);
  
  return G_TOKEN_NONE;
}

void
bse_nullify_pointer (gpointer *pointer_loc)
{
  if (pointer_loc)
    *pointer_loc = NULL;
}
