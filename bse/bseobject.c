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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include	"bseobject.h"

#include	"bseexports.h"
#include	"bseparamcol.c"		/* FIXME */
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
  gchar                  *token;
  BseObjectParseStatement parser;
  gpointer                user_data;
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
extern void		bse_type_register_object_info	(BseTypeInfo	*info);
static void		bse_object_class_base_init	(BseObjectClass	*class);
static void		bse_object_class_base_destroy	(BseObjectClass	*class);
static void		bse_object_class_init		(BseObjectClass	*class);
static void		bse_object_init			(BseObject	*object);
static void		bse_object_do_shutdown		(BseObject	*object);
static void		bse_object_do_destroy		(BseObject	*object);
static void     	bse_object_do_set_param		(BseObject      *object,
							 BseParam       *param);
static void     	bse_object_do_get_param		(BseObject      *object,
							 BseParam       *param);
static void     	bse_object_do_set_name		(BseObject      *object,
							 const gchar	*name);
static guint		bse_pspec_hash			(gconstpointer	 key_spec);
static gint		bse_pspec_equals		(gconstpointer	 key_spec_1,
							 gconstpointer	 key_spec_2);
static void		bse_object_do_store_private	(BseObject      *object,
							 BseStorage     *storage);
static void		bse_object_do_store_termination	(BseObject      *object,
							 BseStorage     *storage);
static BseTokenType	bse_object_do_restore_private	(BseObject     *object,
							 BseStorage    *storage);
static BseTokenType	bse_object_do_try_statement	(BseObject      *object,
							 BseStorage     *storage);
static GTokenType	bse_object_do_restore		(BseObject      *object,
							 BseStorage     *storage);
static BseIcon*		bse_object_do_get_icon		(BseObject	*object);


/* --- variables --- */
GQuark		   _bse_quark_name = 0;
static GQuark      quark_blurb = 0;
static GQuark      quark_hook_list = 0;
static GHashTable *bse_object_names_ht = NULL;
static GHashTable *bse_pspec_ht = NULL;
static GQuark      quark_param_changed_queue = 0;


/* --- DEBUG stuff --- */
static guint bse_object_count = 0;
static GHashTable *debug_objects_ht = NULL;
static void
bse_object_debug_foreach (gpointer key,
			  gpointer value,
			  gpointer user_data)
{
  BseObject *object = value;

  BSE_IF_DEBUG (OBJECTS)
    g_message ("[%p] stale %s\tref_count=%d%s",
	       object,
	       BSE_OBJECT_TYPE_NAME (object),
	       object->ref_count,
	       BSE_OBJECT_DESTROYED (object) ? " (destroyed)" : "");
}

static void
bse_object_debug (void)
{
  if (debug_objects_ht)
    {
      BSE_IF_DEBUG (OBJECTS)
	{
	  g_message ("stale BseObjects: %u", bse_object_count);
	  g_hash_table_foreach (debug_objects_ht, bse_object_debug_foreach, NULL);
	}
    }
}


/* --- functions --- */
extern void
bse_type_register_object_info (BseTypeInfo *info)
{
  static const BseTypeInfo object_info = {
    sizeof (BseObjectClass),

    (BseBaseInitFunc) bse_object_class_base_init,
    (BseBaseDestroyFunc) bse_object_class_base_destroy,
    (BseClassInitFunc) bse_object_class_init,
    (BseClassDestroyFunc) NULL,
    NULL /* class_data */,

    sizeof (BseObject),
    0 /* n_preallocs */,
    (BseObjectInitFunc) bse_object_init,
  };

  *info = object_info;
  
  g_atexit (bse_object_debug);
}

void
bse_object_complete_info (const BseExportSpec *spec,
			  BseTypeInfo         *info)
{
  const BseExportObject *ospec = &spec->s_object;

  *info = *ospec->object_info;
}

const gchar*
bse_object_type_register (const gchar *name,
			  const gchar *parent_name,
			  const gchar *blurb,
			  BsePlugin   *plugin,
			  BseType     *ret_type)
{
  BseType type;

  g_return_val_if_fail (ret_type != NULL, bse_error_blurb (BSE_ERROR_INTERNAL));
  *ret_type = 0;
  g_return_val_if_fail (name != NULL, bse_error_blurb (BSE_ERROR_INTERNAL));
  g_return_val_if_fail (parent_name != NULL, bse_error_blurb (BSE_ERROR_INTERNAL));
  g_return_val_if_fail (plugin != NULL, bse_error_blurb (BSE_ERROR_INTERNAL));

  type = bse_type_from_name (name);
  if (type)
    return "Object already registered";
  type = bse_type_from_name (parent_name);
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
  
  class->n_params = 0;
  class->param_specs = NULL;
  
  class->n_notifiers = 0;
  class->notifiers = NULL;
  for (i = 0; bse_notifiers[i].notifier && bse_notifiers[i].object; i++)
    if (bse_type_conforms_to (BSE_CLASS_TYPE (class),
			      bse_type_from_name (bse_notifiers[i].object)))
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
  
  class->set_param = NULL;
  class->get_param = NULL;
}

static void
bse_object_class_base_destroy (BseObjectClass *class)
{
  guint i;

  for (i = 0; i < class->n_params; i++)
    {
      g_hash_table_remove (bse_pspec_ht, class->param_specs[i]);
      bse_param_spec_free (class->param_specs[i]);
    }
  g_free (class->param_specs);

  g_free (class->notifiers);

  for (i = 0; i < class->n_parsers; i++)
    g_free (class->parsers[i].token);
  g_free (class->parsers);
}

static void
bse_object_class_init (BseObjectClass *class)
{
  guint i;

  _bse_quark_name = g_quark_from_static_string ("BseName");
  bse_object_names_ht = g_hash_table_new (bse_string_hash, bse_string_equals);
  bse_pspec_ht = g_hash_table_new (bse_pspec_hash, bse_pspec_equals);

  class->store_termination = bse_object_do_store_termination;
  class->try_statement = bse_object_do_try_statement;
  class->restore = bse_object_do_restore;

  class->get_param = bse_object_do_get_param;
  class->set_param = bse_object_do_set_param;

  class->set_name = bse_object_do_set_name;
  class->store_private = bse_object_do_store_private;
  class->restore_private = bse_object_do_restore_private;
  class->unlocked = NULL;
  class->get_icon = bse_object_do_get_icon;
  class->shutdown = bse_object_do_shutdown;
  class->destroy = bse_object_do_destroy;

  quark_hook_list = g_quark_from_static_string ("bse-hook-list");

  bse_object_class_add_param (class, NULL,
			      PARAM_NAME,
			      bse_param_spec_string ("name", "Name", NULL,
						     NULL,
						     BSE_PARAM_GUI
						     /* this is only half way true,
						      * ->name is *very* specially
						      * treated within the various
						      * objects, especially BseItem
						      * and BseContainer.
						      */));
  bse_object_class_add_param (class, NULL,
			      PARAM_BLURB,
			      bse_param_spec_fstring ("blurb", "Short description", NULL,
						      NULL,
						      BSE_PARAM_DEFAULT |
						      BSE_PARAM_HINT_CHECK_NULL));

  /* perform neccessary checks for assumptions we make over generated sources
   */
  for (i = 0; bse_notifiers[i].notifier || bse_notifiers[i].object; i++)
    if (!bse_notifiers[i].notifier ||
	!bse_type_conforms_to (bse_type_from_name (bse_notifiers[i].object),
			       BSE_TYPE_OBJECT))
      g_error ("notifier entry (\"%s\", `%s') refers to invalid (unknown) type or name",
	       bse_notifiers[i].notifier, bse_notifiers[i].object);

  quark_param_changed_queue = g_quark_from_static_string ("bse-param-changed-queue");

  /* feature parasites */
  bse_parasite_install_parsers (class);
}

static inline void
bse_object_names_ht_insert (BseObject *object)
{
  GSList *object_slist;

  g_hash_table_freeze (bse_object_names_ht);
  object_slist = g_hash_table_lookup (bse_object_names_ht, BSE_OBJECT_NAME (object));
  if (object_slist)
    g_hash_table_remove (bse_object_names_ht, BSE_OBJECT_NAME (object_slist->data));
  object_slist = g_slist_prepend (object_slist, object);
  g_hash_table_insert (bse_object_names_ht, BSE_OBJECT_NAME (object_slist->data), object_slist);
  g_hash_table_thaw (bse_object_names_ht);
}

static void
bse_object_init (BseObject *object)
{
  object->flags = BSE_OBJECT_FLAG_CONSTRUCTED;
  object->ref_count = 1;
  object->lock_count = 0;
  g_datalist_init (&object->datalist);

  bse_object_names_ht_insert (object);

  BSE_IF_DEBUG (OBJECTS)
    {
      if (!debug_objects_ht)
	debug_objects_ht = g_hash_table_new (g_direct_hash, NULL);
      bse_object_count++;
      g_hash_table_insert (debug_objects_ht, object, object);
    }
}

static inline void
bse_object_names_ht_remove (BseObject *object)
{
  GSList *object_slist, *orig_slist;
  
  g_hash_table_freeze (bse_object_names_ht);
  object_slist = g_hash_table_lookup (bse_object_names_ht, BSE_OBJECT_NAME (object));
  orig_slist = object_slist;
  object_slist = g_slist_remove (object_slist, object);
  if (object_slist != orig_slist)
    {
      g_hash_table_remove (bse_object_names_ht, BSE_OBJECT_NAME (object));
      if (object_slist)
	g_hash_table_insert (bse_object_names_ht, BSE_OBJECT_NAME (object_slist->data), object_slist);
    }
  g_hash_table_thaw (bse_object_names_ht);
}

static void
bse_object_do_set_name (BseObject   *object,
			const gchar *name)
{
  g_datalist_id_set_data_full (&object->datalist, _bse_quark_name, g_strdup (name), name ? g_free : NULL);
}

static void
bse_object_do_set_param (BseObject *object,
			 BseParam  *param)
{
  switch (param->pspec->any.param_id)
    {
      gchar *string;

    case PARAM_NAME:
      bse_object_names_ht_remove (object);
      string = bse_strdup_stripped (param->value.v_string);
      BSE_OBJECT_GET_CLASS (object)->set_name (object, string);
      g_free (string);
      bse_object_names_ht_insert (object);
      BSE_NOTIFY (object, name_set, NOTIFY (OBJECT, DATA));
      break;
    case PARAM_BLURB:
      if (!quark_blurb)
	quark_blurb = g_quark_from_static_string ("bse-blurb");
      string = bse_strdup_stripped (param->value.v_string);
      if (param->value.v_string && !string)
	string = g_strdup ("");
      bse_object_set_qdata_full (object, quark_blurb, string, g_free);
      break;
    default:
      g_warning (G_STRLOC "%s: invalid attempt to set parameter \"%s\" of type `%s'",
		 BSE_OBJECT_TYPE_NAME (object),
		 param->pspec->any.name,
		 bse_type_name (param->pspec->type));
      break;
    }
}

static void
bse_object_do_get_param (BseObject *object,
			 BseParam  *param)
{
  switch (param->pspec->any.param_id)
    {
    case PARAM_NAME:
      g_free (param->value.v_string);
      param->value.v_string = g_strdup (BSE_OBJECT_NAME (object));
      break;
    case PARAM_BLURB:
      g_free (param->value.v_string);
      param->value.v_string = g_strdup (bse_object_get_qdata (object, quark_blurb));
      break;
    default:
      g_warning (G_STRLOC "%s: invalid attempt to get parameter \"%s\" of type `%s'",
		 BSE_OBJECT_TYPE_NAME (object),
		 param->pspec->any.name,
		 bse_type_name (param->pspec->type));
      break;
    }
}

static inline gboolean
bse_object_class_check_notifier (BseObjectClass *class,
				 GQuark	         quark)
{
  guint i;

  for (i = 0; i < class->n_notifiers; i++)
    if (class->notifiers[i] == quark)
      return TRUE;

  return FALSE;
}

gpointer
bse_object_new_valist (BseType      type,
		       const gchar *first_param_name,
		       va_list      var_args)
{
  BseObject *object;
  
  g_return_val_if_fail (bse_type_is_a (type, BSE_TYPE_OBJECT), NULL);
  
  object = bse_type_create_object (type);
  
  if (first_param_name)
    bse_object_set_valist (object, first_param_name, var_args);
  
  return object;
}

gpointer
bse_object_new (BseType      type,
		const gchar *first_param_name,
		...)
{
  BseObject *object;
  va_list var_args;
  
  g_return_val_if_fail (bse_type_is_a (type, BSE_TYPE_OBJECT), NULL);
  
  va_start (var_args, first_param_name);
  object = bse_object_new_valist (type, first_param_name, var_args);
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
  bse_object_set_valist (object, first_param_name, var_args);
  va_end (var_args);
}

void
bse_object_lock (BseObject *object)
{
  g_return_if_fail (BSE_IS_OBJECT (object));
  g_return_if_fail (object->ref_count > 0);

  if (!object->lock_count)
    {
      /* we use a short-hand here, keep in sync with bse_object_ref() */
      object->ref_count += 1;

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
  g_return_if_fail (object->ref_count > 0); /* paranoid */

  object->lock_count -= 1;

  if (!object->lock_count)
    {
      if (BSE_OBJECT_GET_CLASS (object)->unlocked)
	BSE_OBJECT_GET_CLASS (object)->unlocked (object);
	  
      /* release global lock */
      bse_globals_unlock ();
      
      bse_object_unref (object);
    }
}

void
bse_object_ref (BseObject *object)
{
  g_return_if_fail (BSE_IS_OBJECT (object));
  g_return_if_fail (object->ref_count > 0);

  /* NOTE: bse_object_lock/bse_object_unlock modify ->ref_count as well */
  
  object->ref_count += 1;
}

static void
bse_object_do_shutdown (BseObject *object)
{
  BSE_OBJECT_SET_FLAGS (object, BSE_OBJECT_FLAG_DESTROYED);
  BSE_NOTIFY_CHECK (object, destroy, NOTIFY (OBJECT, DATA), /* always */);
}

static void
bse_object_do_destroy (BseObject *object)
{
  /* remove object from hash list *before* clearing data list,
   * since the object name is kept in the datalist!
   */
  bse_object_names_ht_remove (object);

  g_datalist_clear (&object->datalist);
}

void
bse_object_unref (BseObject *object)
{
  g_return_if_fail (BSE_IS_OBJECT (object));
  g_return_if_fail (object->ref_count > 0);
  
  /* NOTE: bse_object_lock/bse_object_unlock modify ->ref_count as well */

  /* "offical" object destruction */
  if (object->ref_count == 1 && !BSE_OBJECT_DESTROYED (object))
    {
      BseObjectClass *class = BSE_OBJECT_GET_CLASS (object);

      if (BSE_OBJECT_IN_PARAM_CHANGED (object))
	g_warning (G_STRLOC "object destructed while in param_changed(), probably ref_count mess");

      /* amongst other things, invoke destroy notifiers and set destroyed flag */
      class->shutdown (object);

      g_return_if_fail (object->ref_count > 0);
    }
  
  object->ref_count -= 1;

  /* finish it off, i.e. internal destruction */
  if (object->ref_count == 0)
    {
      BseObjectClass *class = BSE_OBJECT_GET_CLASS (object);

      class->destroy (object);

      g_return_if_fail (object->ref_count == 0);

      BSE_IF_DEBUG (OBJECTS)
	{
	  g_assert (g_hash_table_lookup (debug_objects_ht, object) == object);
	  
	  g_hash_table_remove (debug_objects_ht, object);
	  bse_object_count--;
	}
      
      bse_type_free_object (object);
    }
}

GList*
bse_objects_list_by_name (BseType      type,
			  const gchar *name)
{
  GList *object_list = NULL;

  g_return_val_if_fail (BSE_TYPE_IS_OBJECT (type) == TRUE, NULL);

  if (bse_object_names_ht)
    {
      GSList *object_slist, *slist;
      
      object_slist = g_hash_table_lookup (bse_object_names_ht, name);
      
      for (slist = object_slist; slist; slist = slist->next)
	if (bse_type_is_a (BSE_OBJECT_TYPE (slist->data), type))
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
    if (bse_type_is_a (BSE_OBJECT_TYPE (slist->data), GPOINTER_TO_UINT (data[1])))
      data[0] = g_list_prepend (data[0], slist->data);
}

GList*
bse_objects_list (BseType type)
{
  g_return_val_if_fail (BSE_TYPE_IS_OBJECT (type) == TRUE, NULL);

  if (bse_object_names_ht)
    {
      gpointer data[2] = { NULL, GUINT_TO_POINTER (type), };

      g_hash_table_foreach (bse_object_names_ht, list_objects, data);

      return data[0];
    }

  return NULL;
}

void
bse_object_set_name (BseObject   *object,
		     const gchar *name)
{
  g_return_if_fail (BSE_IS_OBJECT (object));
  g_return_if_fail (name != NULL);

  bse_object_set (object,
		  "name", name,
		  NULL);
}

gchar*
bse_object_get_name (BseObject *object)
{
  g_return_val_if_fail (BSE_IS_OBJECT (object), NULL);

  return BSE_OBJECT_NAME (object);
}

gchar*
bse_object_get_name_or_type (BseObject *object)
{
  gchar *name;

  g_return_val_if_fail (BSE_IS_OBJECT (object), NULL);

  name = bse_object_get_name (object);

  return name ? name : BSE_OBJECT_TYPE_NAME (object);
}

void
bse_object_set_blurb (BseObject   *object,
		      const gchar *blurb)
{
  g_return_if_fail (BSE_IS_OBJECT (object));

  bse_object_set (object,
		  "blurb", blurb,
		  NULL);
}

gchar*
bse_object_get_blurb (BseObject *object)
{
  g_return_val_if_fail (BSE_IS_OBJECT (object), NULL);

  return bse_object_get_qdata (object, quark_blurb);
}


void
bse_object_set_data (BseObject	 *object,
		     const gchar *key,
		     gpointer	  data)
{
  g_return_if_fail (BSE_IS_OBJECT (object));
  
  g_datalist_set_data (&object->datalist, key, data);
}

void
bse_object_set_data_full (BseObject     *object,
			  const gchar   *key,
			  gpointer       data,
			  GDestroyNotify destroy)
{
  g_return_if_fail (BSE_IS_OBJECT (object));
  
  g_datalist_set_data_full (&object->datalist, key, data, data ? destroy : NULL);
}

gpointer
bse_object_get_data (BseObject   *object,
		     const gchar *key)
{
  g_return_val_if_fail (BSE_IS_OBJECT (object), NULL);
  
  return g_datalist_get_data (&object->datalist, key);
}

void
bse_object_set_qdata (BseObject *object,
		      GQuark	 quark,
		      gpointer	 data)
{
  g_return_if_fail (BSE_IS_OBJECT (object));
  g_return_if_fail (quark > 0);
  
  g_datalist_id_set_data (&object->datalist, quark, data);
}

void
bse_object_set_qdata_full (BseObject     *object,
			   GQuark	  quark,
			   gpointer       data,
			   GDestroyNotify destroy)
{
  g_return_if_fail (BSE_IS_OBJECT (object));
  g_return_if_fail (quark > 0);
  
  g_datalist_id_set_data_full (&object->datalist, quark, data, data ? destroy : NULL);
}

void
bse_object_kill_qdata_no_notify (BseObject *object,
				 GQuark	    quark)
{
  g_return_if_fail (BSE_IS_OBJECT (object));
  g_return_if_fail (quark > 0);
  
  g_datalist_id_remove_no_notify (&object->datalist, quark);
}

gpointer
bse_object_get_qdata (BseObject *object,
		      GQuark     quark)
{
  g_return_val_if_fail (BSE_IS_OBJECT (object), NULL);

  return quark ? g_datalist_id_get_data (&object->datalist, quark) : NULL;
}

static void
bse_hook_list_destroy (gpointer data)
{
  GHookList *hook_list = data;

  g_hook_list_clear (hook_list);
  /* FIXME hook_list needs destruction notifier */
  if (hook_list->hooks || hook_list->hook_memchunk)
    g_warning (G_STRLOC "hook_list destruction failed, leaking memory...");
  else
    g_free (hook_list);
}

GHookList*
bse_object_get_hook_list (BseObject *object)
{
  g_return_val_if_fail (BSE_IS_OBJECT (object), NULL);

  return g_datalist_id_get_data (&object->datalist, quark_hook_list);
}

static inline guint
bse_object_add_notifier_i (BseObject     *object,
			   const gchar	 *method,
			   BseFunc        func,
			   gpointer       data,
			   GDestroyNotify destroy,
			   gboolean	  is_data)
{
  static guint seq_hook_id = 1;
  GHookList *hook_list;
  GHook *hook;
  GQuark quark;
  
  quark = g_quark_try_string (method);
  if (!quark || !bse_object_class_check_notifier (BSE_OBJECT_GET_CLASS (object), quark))
    {
      gchar *name;

      name = g_strdup (method);
      g_strcanon (name, "_", '_');
      quark = g_quark_try_string (name);
      g_free (name);

      if (!quark || !bse_object_class_check_notifier (BSE_OBJECT_GET_CLASS (object), quark))
	{
	  g_warning (G_STRLOC "unable to find notify method \"%s\" in the `%s' class",
		     method,
		     BSE_OBJECT_TYPE_NAME (object));
	  return 0;
	}
    }

  hook_list = g_datalist_id_get_data (&object->datalist, quark_hook_list);
  if (!hook_list)
    {
      hook_list = g_new0 (GHookList, 1);
      g_hook_list_init (hook_list, sizeof (BseNotifyHook));
      g_datalist_id_set_data_full (&object->datalist, quark_hook_list, hook_list, bse_hook_list_destroy);
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
bse_object_add_data_notifier (gpointer     object,
			      const gchar *method,
			      gpointer     func,
			      gpointer     data)
{
  g_return_val_if_fail (BSE_IS_OBJECT (object), 0);
  g_return_val_if_fail (method != NULL, 0);
  g_return_val_if_fail (func != NULL, 0);

  return bse_object_add_notifier_i (object, method, func, data, NULL, 1);
}

guint
bse_object_add_notifier_full (gpointer       object,
			      const gchar   *method,
			      gpointer       func,
			      gpointer       data,
			      GDestroyNotify destroy)
{
  g_return_val_if_fail (BSE_IS_OBJECT (object), 0);
  g_return_val_if_fail (method != NULL, 0);
  g_return_val_if_fail (func != NULL, 0);

  return bse_object_add_notifier_i (object, method, func, data, destroy, 0);
}

guint
bse_object_add_data_notifier_full (gpointer       object,
				   const gchar   *method,
				   gpointer       func,
				   gpointer       data,
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
			       BseFunc      func,
			       gpointer     data,
			       guint        mask)
{
  GHookList *hook_list;
  gboolean found_one = FALSE;
  
  hook_list = g_datalist_id_get_data (&object->datalist, quark_hook_list);
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
    g_warning (G_STRLOC "couldn't remove notifier from %s \"%s\"",
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
  GHookList *hook_list;
  guint oid = id;
  
  g_return_if_fail (BSE_IS_OBJECT (object));
  g_return_if_fail (id > 0);

  hook_list = g_datalist_id_get_data (&object->datalist, quark_hook_list);
  if (hook_list)
    id = !g_hook_destroy (hook_list, id);

  if (id)
    g_warning (G_STRLOC "couldn't remove notifier (%u) from %s \"%s\"",
	       oid,
	       BSE_OBJECT_TYPE_NAME (object),
	       BSE_OBJECT_NAME (object));
}

void
bse_object_class_add_parser (BseObjectClass         *class,
			     const gchar            *token,
			     BseObjectParseStatement parse_func,
			     gpointer                user_data)
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

      class = bse_type_class_peek_parent (class);
    }
  while (class);

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

gpointer
bse_object_ensure_interface_data (BseObject          *object,
				  BseType             interface_type,
				  BseInterfaceDataNew new_func,
				  GDestroyNotify      destroy_func)
{
  gpointer data;
  GQuark quark;

  g_return_val_if_fail (BSE_IS_OBJECT (object), NULL);
  g_return_val_if_fail (BSE_TYPE_IS_INTERFACE (interface_type), NULL);
  g_return_val_if_fail (bse_type_conforms_to (BSE_OBJECT_TYPE (object), interface_type), NULL);
  if (!new_func)
    g_return_val_if_fail (destroy_func == NULL, NULL);

  quark = bse_type_quark (interface_type);

  data = g_datalist_id_get_data (&object->datalist, quark);
  if (!data && new_func)
    {
      data = new_func (object);
      g_datalist_id_set_data_full (&object->datalist, quark, data, destroy_func);
    }

  return data;
}

static void
iface_slist_destroy (gpointer data)
{
  GSList *iface_list = data;
  GSList *slist;

  for (slist = iface_list; slist; slist = slist->next)
    bse_type_interface_unref (slist->data);

  g_slist_free (iface_list);
}

gpointer
bse_object_get_interface (BseObject *object,
			  BseType    interface_type)
{
  static GQuark iface_slist_quark = 0;
  GSList *slist;
  BseTypeInterface *iface_table;

  g_return_val_if_fail (BSE_IS_OBJECT (object), NULL);
  g_return_val_if_fail (BSE_TYPE_IS_INTERFACE (interface_type), NULL);
  
  /* try a fast lookup */
  iface_table = bse_type_interface_peek (BSE_OBJECT_GET_CLASS (object), interface_type);
  if (iface_table)
    return iface_table;

  g_return_val_if_fail (bse_type_conforms_to (BSE_OBJECT_TYPE (object), interface_type), NULL);

  iface_table = bse_type_interface_ref (BSE_OBJECT_GET_CLASS (object), interface_type);

  if (!iface_slist_quark)
    iface_slist_quark = g_quark_from_string ("bse-interface-list");

  slist = g_datalist_id_get_data (&object->datalist, iface_slist_quark);

  if (!slist)
    g_datalist_id_set_data_full (&object->datalist,
				 iface_slist_quark,
				 g_slist_prepend (NULL, iface_table),
				 iface_slist_destroy);
  else
    {
      GSList *tmp;

      tmp = g_slist_alloc ();
      tmp->data = iface_table;
      tmp->next = slist->next;
      slist->next = tmp;
    }

  return iface_table;
}

void
bse_object_class_add_param (BseObjectClass *class,
			    const gchar	   *param_group,
			    guint	    param_id,
			    BseParamSpec   *pspec)
{
  guint i;
  
  g_return_if_fail (BSE_IS_OBJECT_CLASS (class));
  g_return_if_fail (BSE_IS_PARAM_SPEC (pspec));
  g_return_if_fail (pspec->any.parent_type == 0);
  g_return_if_fail (pspec->any.param_group == 0);
  g_return_if_fail (pspec->any.param_id == 0);
  if (pspec->any.flags & BSE_PARAM_WRITABLE)
    g_return_if_fail (class->set_param != NULL);
  if (pspec->any.flags & BSE_PARAM_READABLE)
    g_return_if_fail (class->get_param != NULL);
  
  for (i = 0; i < class->n_params; i++)
    {
      if (class->param_specs[i]->any.param_id == param_id)
	{
	  g_warning (G_STRLOC "class `%s' already contains a parameter with id %u",
		     BSE_CLASS_NAME (class),
		     param_id);
	  return;
	}
    }
  
  pspec->any.parent_type = BSE_CLASS_TYPE (class);
  if (g_hash_table_lookup (bse_pspec_ht, pspec))
    {
      g_warning (G_STRLOC "class `%s' already contains a parameter named `%s'",
		 BSE_CLASS_NAME (class),
		 pspec->any.name);
      pspec->any.parent_type = 0;
      return;
    }
  g_hash_table_insert (bse_pspec_ht, pspec, pspec);
  
  pspec->any.param_group = param_group ? g_quark_from_string (param_group) : 0;
  i = class->n_params++;
  class->param_specs = g_renew (BseParamSpec*, class->param_specs, class->n_params + 1);
  class->param_specs[i] = pspec;
  pspec->any.param_id = param_id;
  i++;
  class->param_specs[i] = NULL;
}

static guint
bse_pspec_hash (gconstpointer key_spec)
{
  const BseParamSpecAny *key = key_spec;
  const gchar *p;
  guint h = BSE_TYPE_SEQNO (key->parent_type);

  for (p = key->name; *p; p++)
    {
      register guint g;

      h = (h << 4) + *p;
      g = h & 0xf0000000;
      if (g)
	h = h ^ (g >> 26);
    }

  return h;
}

static gint
bse_pspec_equals (gconstpointer key_spec_1,
		  gconstpointer key_spec_2)
{
  const BseParamSpecAny *key1 = key_spec_1;
  const BseParamSpecAny *key2 = key_spec_2;

  return (key1->parent_type == key2->parent_type &&
	  strcmp (key1->name, key2->name) == 0);
}

BseParamSpec*
bse_object_class_get_param_spec (BseObjectClass *class,
				 const gchar    *param_name)
{
  BseParamSpec *pspec;
  BseParamSpecAny key;
  
  g_return_val_if_fail (BSE_IS_OBJECT_CLASS (class), NULL);
  g_return_val_if_fail (param_name != NULL, NULL);

  key.parent_type = BSE_CLASS_TYPE (class);
  key.name = g_strdup (param_name);
  g_strcanon (key.name, "-", '-');

  do
    {
      pspec = g_hash_table_lookup (bse_pspec_ht, &key);
      if (pspec)
	break;
      key.parent_type = bse_type_parent (key.parent_type);
    }
  while (key.parent_type);

  g_free (key.name);

  return pspec;
}

static gboolean
notify_param_changed (gpointer data)
{
  BseObject *object = BSE_OBJECT (data);

  /* a reference count is still being held */

  if (BSE_OBJECT_IN_PARAM_CHANGED (object))
    g_warning (G_STRLOC "object `%s' still flagged `IN_PARAM_CHANGED' in idle handler, something's messed",
	       BSE_OBJECT_TYPE_NAME (object));
  else
    {
      GSList *slist = bse_object_get_qdata (object, quark_param_changed_queue);
      
      BSE_OBJECT_SET_FLAGS (object, BSE_OBJECT_FLAG_IN_PARAM_CHANGED);
      
      for (; slist; slist = slist->next)
	if (slist->data)
	  {
	    BseParamSpec *pspec = slist->data;
	    
	    slist->data = NULL;
	    BSE_NOTIFY (object, param_changed, NOTIFY (OBJECT, pspec, DATA));
	  }
      
      BSE_OBJECT_UNSET_FLAGS (object, BSE_OBJECT_FLAG_IN_PARAM_CHANGED);

      bse_object_set_qdata (object, quark_param_changed_queue, NULL);
    }
  
  return FALSE;
}

static inline void
bse_object_queue_param_changed (BseObject    *object,
				BseParamSpec *pspec)
{
  GSList *slist, *last = NULL;

  /* if this is a recursive call on this object, we simply queue further
   * notifications, otherwise we dispatch asyncronously from an idle handler
   * untill the queue is completely empty.
   * we don't queue notifications for params that have yet to be dispatched.
   */
  
  slist = bse_object_get_qdata (object, quark_param_changed_queue);
  for (; slist; last = slist, slist = last->next)
    if (slist->data == pspec)
      return;
  
  if (!last)
    {
      bse_object_ref (object);
      g_idle_add_full (BSE_NOTIFY_PRIORITY,
		       notify_param_changed,
		       object,
		       (GDestroyNotify) bse_object_unref);
      bse_object_set_qdata_full (object,
				 quark_param_changed_queue,
				 g_slist_prepend (NULL, pspec),
				 (GDestroyNotify) g_slist_free);
    }
  else
    last->next = g_slist_append (NULL, pspec);
}

void
bse_object_param_changed (BseObject   *object,
			  const gchar *param_name)
{
  BseParamSpec *pspec;
  
  g_return_if_fail (BSE_IS_OBJECT (object));
  g_return_if_fail (param_name != NULL);

  pspec = bse_object_class_get_param_spec (BSE_OBJECT_GET_CLASS (object), param_name);
  if (!pspec)
    g_warning (G_STRLOC "invalid parameter name `%s'", param_name);
  else
    bse_object_queue_param_changed (object, pspec);
}

void
bse_object_set_valist (BseObject   *object,
		       const gchar *first_param_name,
		       va_list	     var_args)
{
  BseObjectClass *class;
  const gchar *name;
  gboolean in_param_changed;
  
  g_return_if_fail (BSE_IS_OBJECT (object));
  g_return_if_fail (!BSE_OBJECT_DESTROYED (object));
  
  bse_object_ref (object);

  class = BSE_OBJECT_GET_CLASS (object);

  in_param_changed = BSE_OBJECT_IN_PARAM_CHANGED (object);
  BSE_OBJECT_SET_FLAGS (object, BSE_OBJECT_FLAG_IN_PARAM_CHANGED);
  
  name = first_param_name;
  while (name)
    {
      BseParamSpec *pspec;
      BseParam param = { NULL };
      gchar *error;
      
      pspec = bse_object_class_get_param_spec (class, name);
      if (!pspec)
	{
	  g_warning (G_STRLOC "invalid parameter name `%s'", name);
	  goto bail_out;
	}
      
      bse_param_init (&param, pspec);
      BSE_PARAM_COLLECT_VALUE (&param, var_args, error);
      if (error)
	{
	  g_warning (G_STRLOC "%s", error);
	  g_free (error);
          goto bail_out;
	}
      
      bse_object_set_param (object, &param);
      
      bse_param_free_value (&param);
      
      name = va_arg (var_args, gchar*);
    }
  
 bail_out:

  if (!in_param_changed)
    BSE_OBJECT_UNSET_FLAGS (object, BSE_OBJECT_FLAG_IN_PARAM_CHANGED);
  
  bse_object_unref (object);
}

void
bse_object_set_param (BseObject	*object,
		      BseParam	*param)
{
  BseObjectClass *class;
  gboolean in_param_changed;
  
  g_return_if_fail (BSE_IS_OBJECT (object));
  g_return_if_fail (BSE_IS_PARAM (param));
  g_return_if_fail (param->pspec->any.flags & BSE_PARAM_WRITABLE);
  g_return_if_fail (bse_type_is_a (BSE_OBJECT_TYPE (object), param->pspec->any.parent_type));
  g_return_if_fail (!BSE_OBJECT_DESTROYED (object));
  
  class = bse_type_class_peek (param->pspec->any.parent_type);
  g_return_if_fail (class != NULL); /* paranoid */

  bse_object_ref (object);

  in_param_changed = BSE_OBJECT_IN_PARAM_CHANGED (object);
  BSE_OBJECT_SET_FLAGS (object, BSE_OBJECT_FLAG_IN_PARAM_CHANGED);
  class->set_param (object, param);
  if (!in_param_changed)
    BSE_OBJECT_UNSET_FLAGS (object, BSE_OBJECT_FLAG_IN_PARAM_CHANGED);
  bse_object_queue_param_changed (object, param->pspec);

  bse_object_unref (object);
}

void
bse_object_get_param (BseObject *object,
		      BseParam  *param)
{
  BseObjectClass *class;
  
  g_return_if_fail (BSE_IS_OBJECT (object));
  g_return_if_fail (BSE_IS_PARAM (param));
  g_return_if_fail (param->pspec->any.flags & BSE_PARAM_READABLE);
  g_return_if_fail (bse_type_is_a (BSE_OBJECT_TYPE (object), param->pspec->any.parent_type));
  
  class = bse_type_class_peek (param->pspec->any.parent_type);
  g_return_if_fail (class != NULL); /* paranoid */

  bse_object_ref (object);

  /* _get_param functions need to free current parameter values
   * themselves.
   *
   * bse_param_free_value (param);
   */

  class->get_param (object, param);

  bse_object_unref (object);
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

  if (BSE_OBJECT_GET_CLASS (object)->store_termination)
    BSE_OBJECT_GET_CLASS (object)->store_termination (object, storage);

  bse_object_unref (object);
}

static void
bse_object_do_store_termination (BseObject  *object,
				 BseStorage *storage)
{
  bse_storage_handle_break (storage);
  bse_storage_putc (storage, ')');
}

static void
bse_object_do_store_private (BseObject  *object,
			     BseStorage *storage)
{
  BseObjectClass *class;
  GSList *slist, *class_list = NULL;

  /* dump the object paramters, starting out
   * at the base class
   */
  class = BSE_OBJECT_GET_CLASS (object);
  while (class)
    {
      class_list = g_slist_prepend (class_list, class);
      class = bse_type_class_peek_parent (class);
    }
  for (slist = class_list; slist; slist = slist->next)
    {
      guint i;

      class = slist->data;
      
      for (i = 0; i < class->n_params; i++)
	{
	  BseParamSpec *pspec = class->param_specs[i];
	  
	  if (pspec->any.flags & BSE_PARAM_SERVE_STORAGE)
	    {
	      BseParam param = { NULL, };
	      
	      bse_param_init (&param, pspec);
	      bse_object_get_param (object, &param);
	      if (!bse_param_defaults (&param) || BSE_STORAGE_PUT_DEFAULTS (storage))
		{
		  bse_storage_break (storage);
		  bse_storage_put_param (storage, &param);
		}
	      bse_param_free_value (&param);
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
bse_object_do_try_statement (BseObject  *object,
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
   * G_TOKEN_NONE)        statement got parsed, advance
   *                      to next statement
   * BSE_TOKEN_UNMATCHED) couldn't parse statement, try further
   * everything else)     encountered (syntax) error during parsing,
   *                      bail out
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
      g_scanner_get_next_token (scanner); /* read in the identifier */
      
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
  BseParamSpec *pspec;
  BseParam param = { NULL, };
  GTokenType expected_token;

  /* we only feature parameter parsing here,
   * so lets figure whether there is something to parse for us
   */
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return BSE_TOKEN_UNMATCHED;
  
  /* ok, we got an identifier, try object parameter lookup
   * we should in theory only get BSE_PARAM_SERVE_STORAGE
   * parameters here, but due to version changes or even
   * users editing their files, we will simply parse all
   * kinds of parameters here (we might want to at least
   * restrict them to BSE_PARAM_SERVE_STORAGE and
   * BSE_PARAM_SERVE_GUI at some point...)
   */
  pspec = bse_object_class_get_param_spec (BSE_OBJECT_GET_CLASS (object),
					   scanner->next_value.v_identifier);
  if (!pspec)
    return BSE_TOKEN_UNMATCHED;

  /* ok we got a parameter for this, so eat the token */
  g_scanner_get_next_token (scanner);

  /* and away with the parameter parsing... */
  bse_param_init (&param, pspec);
  expected_token = bse_storage_parse_param_value (storage, &param);
  if (expected_token != G_TOKEN_NONE)
    {
      bse_param_free_value (&param);
      /* failed to parse the parameter value */
      return expected_token;
    }
  bse_object_set_param (object, &param);
  bse_param_free_value (&param);

  return G_TOKEN_NONE;
}

void
bse_nullify_pointer (gpointer *pointer_loc)
{
  if (pointer_loc)
    *pointer_loc = NULL;
}
