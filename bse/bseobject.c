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
 */
#include "bseobject.h"

#include "bseexports.h"
#include "bsestorage.h"
#include "bseparasite.h"
#include "bsecategories.h"
#include "bsemarshal.h"
#include "bsesource.h"		/* debug hack */
#include <string.h>

enum
{
  PROP_0,
  PROP_UNAME,
  PROP_BLURB
};
enum
{
  SIGNAL_DESTROY,
  SIGNAL_STORE,
  SIGNAL_ICON_CHANGED,
  SIGNAL_LAST
};


/* -- structures --- */
struct _BseObjectParser
{
  gchar			 *token;
  BseObjectParseStatement parser;
  gpointer		  user_data;
};


/* --- prototypes --- */
static void		bse_object_class_base_init	(BseObjectClass	*class);
static void		bse_object_class_base_finalize	(BseObjectClass	*class);
static void		bse_object_class_init		(BseObjectClass	*class);
static void		bse_object_init			(BseObject	*object);
static void		bse_object_do_dispose		(GObject	*gobject);
static void		bse_object_do_destroy		(BseObject	*object);
static void		bse_object_do_set_property	(BseObject	*object,
							 guint           property_id,
							 GValue         *value,
							 GParamSpec     *pspec);
static void		bse_object_do_get_property	(BseObject	*object,
							 guint           property_id,
							 GValue         *value,
							 GParamSpec     *pspec);
static void		bse_object_do_set_uname		(BseObject	*object,
							 const gchar	*uname);
static void		bse_object_do_store_private	(BseObject	*object,
							 BseStorage	*storage);
static void		bse_object_do_store_after	(BseObject	*object,
							 BseStorage	*storage);
static void		bse_object_store_property	(BseObject	*object,
							 BseStorage	*storage,
							 GValue		*value,
							 GParamSpec	*pspec);
static GTokenType	bse_object_restore_property	(BseObject	*object,
							 BseStorage	*storage,
							 GValue		*value,
							 GParamSpec	*pspec);
static BseTokenType	bse_object_do_restore_private	(BseObject     *object,
							 BseStorage    *storage);
static BseTokenType	bse_object_do_try_statement	(BseObject	*object,
							 BseStorage	*storage);
static GTokenType	bse_object_do_restore		(BseObject	*object,
							 BseStorage	*storage);
static BswIcon*		bse_object_do_get_icon		(BseObject	*object);


/* --- variables --- */
static gpointer	   parent_class = NULL;
GQuark		   bse_quark_uname = 0;
static GQuark	   quark_blurb = 0;
static GHashTable *object_unames_ht = NULL;
static GHashTable *object_id_ht = NULL;
static GHashTable *marshaller_ht = NULL;
static GQuark	   quark_property_changed_queue = 0;
static guint       object_signals[SIGNAL_LAST] = { 0, };


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
  class->n_parsers = 0;
  class->parsers = NULL;
}

static void
bse_object_class_base_finalize (BseObjectClass *class)
{
  guint i;
  
  for (i = 0; i < class->n_parsers; i++)
    g_free (class->parsers[i].token);
  g_free (class->parsers);
}

static void
bse_object_class_init (BseObjectClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  bse_quark_uname = g_quark_from_static_string ("bse-object-uname");
  quark_property_changed_queue = g_quark_from_static_string ("bse-property-changed-queue");
  quark_blurb = g_quark_from_static_string ("bse-object-blurb");
  object_unames_ht = g_hash_table_new (bse_string_hash, bse_string_equals);
  object_id_ht = g_hash_table_new (NULL, NULL);
  marshaller_ht = g_hash_table_new (NULL, NULL);
  
  gobject_class->get_property = (GObjectGetPropertyFunc) bse_object_do_get_property;
  gobject_class->set_property = (GObjectSetPropertyFunc) bse_object_do_set_property;
  gobject_class->dispose = bse_object_do_dispose;
  
  class->store_property = bse_object_store_property;
  class->restore_property = bse_object_restore_property;
  class->store_after = bse_object_do_store_after;
  class->try_statement = bse_object_do_try_statement;
  class->restore = bse_object_do_restore;
  
  class->set_uname = bse_object_do_set_uname;
  class->store_private = bse_object_do_store_private;
  class->restore_private = bse_object_do_restore_private;
  class->unlocked = NULL;
  class->get_icon = bse_object_do_get_icon;
  class->destroy = bse_object_do_destroy;
  
  bse_object_class_add_param (class, NULL,
			      PROP_UNAME,
			      bse_param_spec_string ("uname", "Name", "Unique name of this object",
						     NULL,
						     BSE_PARAM_GUI | G_PARAM_LAX_VALIDATION
						     /* watch out, unames are specially
						      * treated within the various
						      * objects, specifically BseItem
						      * and BseContainer.
						      */));
  bse_object_class_add_param (class, NULL,
			      PROP_BLURB,
			      bse_param_spec_string ("blurb", "Comment", NULL,
						     NULL,
						     BSE_PARAM_DEFAULT |
						     BSE_PARAM_HINT_CHECK_NULL));
  
  object_signals[SIGNAL_DESTROY] = bse_object_class_add_signal (class, "destroy",
								bse_marshal_VOID__NONE, NULL,
								G_TYPE_NONE, 0);
  object_signals[SIGNAL_STORE] = bse_object_class_add_signal (class, "store",
							      bse_marshal_VOID__POINTER, NULL, // FIXME __OBJECT
							      G_TYPE_NONE, 1, G_TYPE_POINTER); // FIXME: G_TYPE_STORAGE);
  object_signals[SIGNAL_ICON_CHANGED] = bse_object_class_add_signal (class, "icon_changed",
								     bse_marshal_VOID__NONE, NULL,
								     G_TYPE_NONE, 0);
  
  /* feature parasites */
  bse_parasite_install_parsers (class);
}

void
bse_object_debug_leaks (void)
{
  BSE_IF_DEBUG (LEAKS)
    {
      GList *list, *objects = bse_objects_list (BSE_TYPE_OBJECT);

      for (list = objects; list; list = list->next)
	{
	  BseObject *object = list->data;

	  g_message ("[%p] stale %s\t ref_count=%u prepared=%u locked=%u id=%u",
		     object,
		     G_OBJECT_TYPE_NAME (object),
		     G_OBJECT (object)->ref_count,
		     BSE_IS_SOURCE (object) && BSE_SOURCE_PREPARED (object),
		     object->lock_count > 0,
		     BSE_OBJECT_ID (object));
	}
      g_list_free (objects);
    }
}

const gchar*
bse_object_debug_name (gpointer object)
{
  GTypeInstance *instance = object;
  gchar *debug_name;

  if (!instance)
    return "<NULL>";
  if (!instance->g_class)
    return "<NULL-Class>";
  if (!g_type_is_a (instance->g_class->g_type, BSE_TYPE_OBJECT))
    return "<Non-BseObject>";
  debug_name = g_object_get_data (G_OBJECT (instance), "bse-debug-name");
  if (!debug_name)
    {
      const gchar *uname = BSE_OBJECT_UNAME (instance);
      debug_name = g_strdup_printf ("\"%s::%s\"", G_OBJECT_TYPE_NAME (instance), uname ? uname : "");
      g_object_set_data_full (G_OBJECT (instance), "bse-debug-name", debug_name, g_free);
    }
  return debug_name;
}

static inline void
object_unames_ht_insert (BseObject *object)
{
  GSList *object_slist;
  
  object_slist = g_hash_table_lookup (object_unames_ht, BSE_OBJECT_UNAME (object));
  if (object_slist)
    g_hash_table_remove (object_unames_ht, BSE_OBJECT_UNAME (object_slist->data));
  object_slist = g_slist_prepend (object_slist, object);
  g_hash_table_insert (object_unames_ht, BSE_OBJECT_UNAME (object_slist->data), object_slist);
}

static void
bse_object_init (BseObject *object)
{
  static guint unique_id = 1;
  
  object->flags = 0;
  object->lock_count = 0;
  object->unique_id = unique_id++;
  if (!unique_id)
    g_error ("object ID overflow");
  g_hash_table_insert (object_id_ht, (gpointer) object->unique_id, object);
  
  object_unames_ht_insert (object);
}

static inline void
object_unames_ht_remove (BseObject *object)
{
  GSList *object_slist, *orig_slist;
  
  object_slist = g_hash_table_lookup (object_unames_ht, BSE_OBJECT_UNAME (object));
  orig_slist = object_slist;
  object_slist = g_slist_remove (object_slist, object);
  if (object_slist != orig_slist)
    {
      g_hash_table_remove (object_unames_ht, BSE_OBJECT_UNAME (object));
      if (object_slist)
	g_hash_table_insert (object_unames_ht, BSE_OBJECT_UNAME (object_slist->data), object_slist);
    }
}

static void
bse_object_do_dispose (GObject *gobject)
{
  BseObject *object = BSE_OBJECT (gobject);
  
  g_return_if_fail (gobject->ref_count == 1);

  BSE_OBJECT_SET_FLAGS (object, BSE_OBJECT_FLAG_DISPOSED);
  
  /* perform destroy notification */
  g_signal_emit (object, object_signals[SIGNAL_DESTROY], 0);
  
  g_return_if_fail (gobject->ref_count == 1);
  
  /* invoke destroy method */
  BSE_OBJECT_GET_CLASS (object)->destroy (object);
  
  g_return_if_fail (gobject->ref_count == 1);

  /* complete shutdown process, by chaining
   * parent class' handler
   */
  G_OBJECT_CLASS (parent_class)->dispose (gobject);
  
  g_return_if_fail (gobject->ref_count == 1);
}

static void
bse_object_do_destroy (BseObject *object)
{
  g_hash_table_remove (object_id_ht, (gpointer) object->unique_id);
  
  /* remove object from hash list *before* clearing data list,
   * since the object uname is kept in the datalist!
   */
  object_unames_ht_remove (object);
}

static void
bse_object_do_set_uname (BseObject   *object,
			 const gchar *uname)
{
  bse_object_set_qdata_full (object, bse_quark_uname, g_strdup (uname), uname ? g_free : NULL);
}

static void
bse_object_do_set_property (BseObject   *object,
			    guint        property_id,
			    GValue      *value,
			    GParamSpec  *pspec)
{
  switch (property_id)
    {
      gchar *string;
      
    case PROP_UNAME:
      if (!(object->flags & BSE_OBJECT_FLAG_FIXED_UNAME))
	{
	  object_unames_ht_remove (object);
	  string = bse_strdup_stripped (g_value_get_string (value));
	  if (string)
	    {
	      gchar *p = strchr (string, ':');
	      /* get rid of colons in the string (invalid reserved character) */
	      while (p)
		{
		  *p++ = '?';
		  p = strchr (p, ':');
		}
	    }
	  BSE_OBJECT_GET_CLASS (object)->set_uname (object, string);
	  g_free (string);
	  g_object_set_data (G_OBJECT (object), "bse-debug-name", NULL);
	  object_unames_ht_insert (object);
	}
      break;
    case PROP_BLURB:
      if (!quark_blurb)
	quark_blurb = g_quark_from_static_string ("bse-blurb");
      string = bse_strdup_stripped (g_value_get_string (value));
      if (g_value_get_string (value) && !string) /* preserve NULL vs. "" distinction */
	string = g_strdup ("");
      bse_object_set_qdata_full (object, quark_blurb, string, string ? g_free : NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bse_object_do_get_property (BseObject   *object,
			    guint        property_id,
			    GValue      *value,
			    GParamSpec  *pspec)
{
  switch (property_id)
    {
    case PROP_UNAME:
      g_value_set_string (value, BSE_OBJECT_UNAME (object));
      break;
    case PROP_BLURB:
      g_value_set_string (value, bse_object_get_qdata (object, quark_blurb));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
bse_object_class_add_property (BseObjectClass *class,
			       const gchar    *property_group,
			       guint	       property_id,
			       GParamSpec     *pspec)
{
  g_return_if_fail (BSE_IS_OBJECT_CLASS (class));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  g_return_if_fail (bse_param_spec_get_group (pspec) == NULL);
  g_return_if_fail (property_id > 0);
  
  bse_param_spec_set_group (pspec, property_group);
  g_object_class_install_property (G_OBJECT_CLASS (class), property_id, pspec);
}

void
bse_object_class_set_param_log_scale (BseObjectClass *oclass,
				      const gchar    *pspec_name,
				      gdouble         center,
				      gdouble         base,
				      guint           n_steps)
{
  GParamSpec *pspec;

  g_return_if_fail (BSE_IS_OBJECT_CLASS (oclass));
  g_return_if_fail (pspec_name != NULL);
  g_return_if_fail (n_steps > 0);
  g_return_if_fail (base > 0);

  pspec = g_object_class_find_property (G_OBJECT_CLASS (oclass), pspec_name);
  if (!G_IS_PARAM_SPEC_FLOAT (pspec) || pspec->owner_type != G_OBJECT_CLASS_TYPE (oclass))
    g_warning ("class `%s' has no float property `%s' to set log scale",
	       G_OBJECT_CLASS_NAME (oclass),
	       pspec_name);
  else
    bse_param_spec_set_log_scale (pspec, center, base, n_steps);
}

guint
bse_object_class_add_signal (BseObjectClass    *oclass,
			     const gchar       *signal_name,
			     GSignalCMarshaller c_marshaller,
			     GSignalCMarshaller proxy_marshaller,
			     GType              return_type,
			     guint              n_params,
			     ...)
{
  va_list args;
  guint signal_id;
  gpointer old_proxy_marshaller;

  g_return_val_if_fail (BSE_IS_OBJECT_CLASS (oclass), 0);
  g_return_val_if_fail (signal_name != NULL, 0);
  g_return_val_if_fail (c_marshaller != NULL, 0);
  
  va_start (args, n_params);
  signal_id = g_signal_new_valist (signal_name,
				   G_TYPE_FROM_CLASS (oclass),
				   G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
				   NULL, NULL, NULL,
				   c_marshaller,
				   return_type,
				   n_params, args);
  va_end (args);

  old_proxy_marshaller = g_hash_table_lookup (marshaller_ht, c_marshaller);
  if (old_proxy_marshaller && old_proxy_marshaller != proxy_marshaller)
    g_warning ("proxy marshaller mismatch for signal \"%s::%s\": %p != %p",
	       g_type_name (G_TYPE_FROM_CLASS (oclass)), signal_name,
	       old_proxy_marshaller, proxy_marshaller);
  else
    g_hash_table_insert (marshaller_ht, c_marshaller, proxy_marshaller);
  
  return signal_id;
}

guint
bse_object_class_add_dsignal (BseObjectClass    *oclass,
			      const gchar       *signal_name,
			      GSignalCMarshaller c_marshaller,
			      GSignalCMarshaller proxy_marshaller,
			      GType              return_type,
			      guint              n_params,
			      ...)
{
  va_list args;
  guint signal_id;
  gpointer old_proxy_marshaller;

  g_return_val_if_fail (BSE_IS_OBJECT_CLASS (oclass), 0);
  g_return_val_if_fail (signal_name != NULL, 0);
  g_return_val_if_fail (c_marshaller != NULL, 0);
  
  va_start (args, n_params);
  signal_id = g_signal_new_valist (signal_name,
				   G_TYPE_FROM_CLASS (oclass),
				   G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS | G_SIGNAL_DETAILED,
				   NULL, NULL, NULL,
				   c_marshaller,
				   return_type,
				   n_params, args);
  va_end (args);

  old_proxy_marshaller = g_hash_table_lookup (marshaller_ht, c_marshaller);
  if (old_proxy_marshaller && old_proxy_marshaller != proxy_marshaller)
    g_warning ("proxy marshaller mismatch for signal \"%s::%s\": %p != %p",
	       g_type_name (G_TYPE_FROM_CLASS (oclass)), signal_name,
	       old_proxy_marshaller, proxy_marshaller);
  else
    g_hash_table_insert (marshaller_ht, c_marshaller, proxy_marshaller);
  
  return signal_id;
}

GSignalCMarshaller
bse_proxy_marshaller_lookup (GSignalCMarshaller c_marshaller)
{
  GSignalCMarshaller proxy_marshaller;

  g_return_val_if_fail (c_marshaller != NULL, NULL);

  proxy_marshaller = g_hash_table_lookup (marshaller_ht, c_marshaller);

  return proxy_marshaller ? proxy_marshaller : c_marshaller;
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
		       const gchar *first_property_name,
		       va_list	    var_args)
{
  g_return_val_if_fail (BSE_TYPE_IS_OBJECT (type), NULL);
  
  return g_object_new_valist (type, first_property_name, var_args);
}

gpointer
bse_object_new (GType	     type,
		const gchar *first_property_name,
		...)
{
  gpointer object;
  va_list var_args;
  
  g_return_val_if_fail (BSE_TYPE_IS_OBJECT (type), NULL);
  
  va_start (var_args, first_property_name);
  object = g_object_new_valist (type, first_property_name, var_args);
  va_end (var_args);
  
  return object;
}

void
bse_object_set (BseObject   *object,
		const gchar *first_property_name,
		...)
{
  va_list var_args;
  
  g_return_if_fail (BSE_IS_OBJECT (object));
  
  va_start (var_args, first_property_name);
  g_object_set_valist (G_OBJECT (object), first_property_name, var_args);
  va_end (var_args);
}

void
bse_object_get (BseObject   *object,
		const gchar *first_property_name,
		...)
{
  va_list var_args;
  
  g_return_if_fail (BSE_IS_OBJECT (object));
  
  va_start (var_args, first_property_name);
  g_object_get_valist (G_OBJECT (object), first_property_name, var_args);
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

gpointer
bse_object_from_id (guint unique_id)
{
  gpointer object = g_hash_table_lookup (object_id_ht, (gpointer) unique_id);

  /* we reveal NULL for disposed objects or 0 IDs */

  if (object && BSE_OBJECT_DISPOSED (object))
    object = NULL;

  return object;
}

GList*
bse_objects_list_by_uname (GType	type,
			   const gchar *uname)
{
  GList *object_list = NULL;
  
  g_return_val_if_fail (BSE_TYPE_IS_OBJECT (type) == TRUE, NULL);
  
  if (object_unames_ht)
    {
      GSList *object_slist, *slist;
      
      object_slist = g_hash_table_lookup (object_unames_ht, uname);
      
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
    if (g_type_is_a (BSE_OBJECT_TYPE (slist->data), (GType) data[1]))
      data[0] = g_list_prepend (data[0], slist->data);
}

GList* /* list_free result */
bse_objects_list (GType	  type)
{
  g_return_val_if_fail (BSE_TYPE_IS_OBJECT (type) == TRUE, NULL);
  
  if (object_unames_ht)
    {
      gpointer data[2] = { NULL, (gpointer) type, };
      
      g_hash_table_foreach (object_unames_ht, list_objects, data);
      
      return data[0];
    }
  
  return NULL;
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
  
  g_signal_emit (object, object_signals[SIGNAL_ICON_CHANGED], 0);
}

BswIcon*
bse_object_get_icon (BseObject *object)
{
  BswIcon *icon;
  
  g_return_val_if_fail (BSE_IS_OBJECT (object), NULL);
  
  bse_object_ref (object);
  
  icon = BSE_OBJECT_GET_CLASS (object)->get_icon (object);
  
  bse_object_unref (object);
  
  return icon;
}

static BswIcon*
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
      BswIcon *icon = cats[i].icon;
      
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
  
  g_signal_emit (object, object_signals[SIGNAL_STORE], 0, storage);
  
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
bse_object_store_property (BseObject  *object,
			   BseStorage *storage,
			   GValue     *value,
			   GParamSpec *pspec)
{
  if (g_type_is_a (G_VALUE_TYPE (value), G_TYPE_OBJECT))
    g_warning ("%s: unable to store object property \"%s\" of type `%s'",
	       G_STRLOC, pspec->name, g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
  else
    {
      bse_storage_break (storage);
      bse_storage_put_param (storage, value, pspec);
    }
}

static void
bse_object_do_store_private (BseObject	*object,
			     BseStorage *storage)
{
  GParamSpec **pspecs;
  guint n;

  /* dump the object paramters, starting out
   * at the base class
   */
  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (object), &n);
  while (n--)
    {
      GParamSpec *pspec = pspecs[n];
      
      if (pspec->flags & BSE_PARAM_SERVE_STORAGE)
	{
	  GValue value = { 0, };
	  
	  g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
	  g_object_get_property (G_OBJECT (object), pspec->name, &value);
	  if (!g_param_value_defaults (pspec, &value) || BSE_STORAGE_PUT_DEFAULTS (storage))
	    BSE_OBJECT_GET_CLASS (object)->store_property (object, storage, &value, pspec);
	  g_value_unset (&value);
	}
    }
  g_free (pspecs);
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

static GTokenType
bse_object_restore_property (BseObject  *object,
			     BseStorage *storage,
			     GValue     *value,
			     GParamSpec *pspec)
{
  GTokenType expected_token;
  gboolean fixed_uname;

  if (g_type_is_a (G_VALUE_TYPE (value), G_TYPE_OBJECT))
    return bse_storage_warn_skip (storage, "unable to restore object property \"%s\" of type `%s'",
				  pspec->name, g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));

  /* parse the value for this pspec, including the trailing closing ')' */
  expected_token = bse_storage_parse_param_value (storage, value, pspec, TRUE);
  if (expected_token != G_TOKEN_NONE)
    return expected_token;	/* failed to parse the parameter value */

  /* preserve the uname during restoring */
  fixed_uname = object->flags & BSE_OBJECT_FLAG_FIXED_UNAME;
  BSE_OBJECT_SET_FLAGS (object, BSE_OBJECT_FLAG_FIXED_UNAME);
  g_object_set_property (G_OBJECT (object), pspec->name, value);
  if (!fixed_uname)
    BSE_OBJECT_UNSET_FLAGS (object, BSE_OBJECT_FLAG_FIXED_UNAME);

  return G_TOKEN_NONE;
}

static BseTokenType
bse_object_do_restore_private (BseObject  *object,
			       BseStorage *storage)
{
  GScanner *scanner = storage->scanner;
  GParamSpec *pspec;
  GValue value = { 0, };
  GTokenType expected_token;
  
  /* we only support property parsing here,
   * so lets figure whether there is something to parse for us
   */
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return BSE_TOKEN_UNMATCHED;
  
  /* ok, we got an identifier, try object property lookup
   * we should in theory only get BSE_PARAM_SERVE_STORAGE
   * properties here, but due to version changes or even
   * users editing their files, we will simply parse all
   * kinds of properties (we might want to at least
   * restrict them to BSE_PARAM_SERVE_STORAGE and
   * BSE_PARAM_SERVE_GUI at some point...)
   */
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object),
					scanner->next_value.v_identifier);
  if (!pspec)
    return BSE_TOKEN_UNMATCHED;
  
  /* ok we got a pspec for this, so eat the token */
  g_scanner_get_next_token (scanner);
  
  /* and away with the property parsing... */
  g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
  expected_token = BSE_OBJECT_GET_CLASS (object)->restore_property (object, storage, &value, pspec);
  g_value_unset (&value);

  return expected_token;
}


/* --- compile standard marshallers --- */
#include	"bsemarshal.c"
