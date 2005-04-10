/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2003 Tim Janik
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
#include "bsegconfig.h"
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
  SIGNAL_RELEASE,
  SIGNAL_ICON_CHANGED,
  SIGNAL_LAST
};


/* --- prototypes --- */
static guint		eclosure_hash			(gconstpointer	 c);
static gint		eclosure_equals			(gconstpointer	 c1,
							 gconstpointer	 c2);


/* --- variables --- */
static gpointer	   parent_class = NULL;
GQuark		   bse_quark_uname = 0;
GQuark		   bse_quark_icon = 0;
static GQuark	   quark_blurb = 0;
static GHashTable *object_unames_ht = NULL;
static GHashTable *eclosures_ht = NULL;
static SfiUStore  *object_id_ustore = NULL;
static GQuark	   quark_property_changed_queue = 0;
static guint       object_signals[SIGNAL_LAST] = { 0, };


/* --- functions --- */
void
bse_object_debug_leaks (void)
{
  if (sfi_debug_check ("leaks"))
    {
      GList *list, *objects = bse_objects_list (BSE_TYPE_OBJECT);
      
      for (list = objects; list; list = list->next)
	{
	  BseObject *object = list->data;
	  
	  sfi_debug ("leaks", "stale %s:\t prepared=%u locked=%u ref_count=%u id=%u ((BseObject*)%p)",
		     G_OBJECT_TYPE_NAME (object),
		     BSE_IS_SOURCE (object) && BSE_SOURCE_PREPARED (object),
		     object->lock_count > 0,
		     G_OBJECT (object)->ref_count,
		     BSE_OBJECT_ID (object),
                     object);
	}
      g_list_free (objects);
    }
}

/**
 * bse_object_strdup_debug_handle
 * @object:  supposedly valid #GObject pointer
 * @RETURNS: newly allocated string
 * Construct a debugging identifier for @object. No mutable
 * object members are accessed, so as long as the caller
 * keeps @object alive for the duration of the function call,
 * this function is MT-safe and may be called from any thread.
 */
gchar*
bse_object_strdup_debug_handle (gpointer object)
{
  GTypeInstance *instance = object;
  if (!instance)
    return g_strdup ("<NULL>");
  if (!instance->g_class)
    return g_strdup ("<NULL-Class>");
  if (!g_type_is_a (instance->g_class->g_type, G_TYPE_OBJECT))
    return g_strdup ("<Non-GObject>");
  /* we may not access GObject.data (includes BSE_OBJECT_UNAME()) */
  return g_strdup_printf ("%s(%p)\"", G_OBJECT_TYPE_NAME (instance), object);
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
  object->flags = 0;
  object->lock_count = 0;
  object->unique_id = bse_id_alloc ();
  sfi_ustore_insert (object_id_ustore, object->unique_id, object);
  
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
  
  BSE_OBJECT_SET_FLAGS (object, BSE_OBJECT_FLAG_DISPOSING);

  if (BSE_OBJECT_IN_RESTORE (object))
    g_warning ("%s: object in restore state while disposing: %s", G_STRLOC, bse_object_debug_name (object));

  /* perform release notification */
  g_signal_emit (object, object_signals[SIGNAL_RELEASE], 0);
  
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (gobject);

  BSE_OBJECT_UNSET_FLAGS (object, BSE_OBJECT_FLAG_DISPOSING);
}

static void
bse_object_do_finalize (GObject *gobject)
{
  BseObject *object = BSE_OBJECT (gobject);

  bse_id_free (object->unique_id);
  sfi_ustore_remove (object_id_ustore, object->unique_id);
  object->unique_id = 0;

  /* remove object from hash list *before* clearing data list,
   * since the object uname is kept in the datalist!
   */
  object_unames_ht_remove (object);

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

static void
bse_object_do_set_uname (BseObject   *object,
			 const gchar *uname)
{
  g_object_set_qdata_full (object, bse_quark_uname, g_strdup (uname), uname ? g_free : NULL);
}

static void
bse_object_do_set_property (GObject      *gobject,
			    guint         property_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
  BseObject *object = BSE_OBJECT (gobject);
  switch (property_id)
    {
      gchar *string;
      
    case PROP_UNAME:
      if (!(object->flags & BSE_OBJECT_FLAG_FIXED_UNAME))
	{
	  object_unames_ht_remove (object);
	  string = g_strdup_stripped (g_value_get_string (value));
	  if (string)
	    {
	      gchar *p = strchr (string, ':');
	      /* get rid of colons in the string (invalid reserved character) */
	      while (p)
		{
		  *p++ = '?';
		  p = strchr (p, ':');
		}
              /* initial character must be >= \007 */
              if (string[0] > 0 && string[0] < 7)
                string[0] = '_';
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
      string = g_strdup (g_value_get_string (value));
      if (g_value_get_string (value) && !string) /* preserve NULL vs. "" distinction */
	string = g_strdup ("");
      g_object_set_qdata_full (object, quark_blurb, string, string ? g_free : NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bse_object_do_get_property (GObject     *gobject,
			    guint        property_id,
			    GValue      *value,
			    GParamSpec  *pspec)
{
  BseObject *object = BSE_OBJECT (gobject);
  switch (property_id)
    {
    case PROP_UNAME:
      g_value_set_string (value, BSE_OBJECT_UNAME (object));
      break;
    case PROP_BLURB:
      g_value_set_string (value, g_object_get_qdata (object, quark_blurb));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
bse_object_class_add_grouped_property (BseObjectClass *class,
                                       guint	       property_id,
                                       GParamSpec     *pspec)
{
  g_return_if_fail (BSE_IS_OBJECT_CLASS (class));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  g_return_if_fail (property_id > 0);
  
  g_object_class_install_property (G_OBJECT_CLASS (class), property_id, pspec);
}

void
bse_object_class_add_property (BseObjectClass *class,
			       const gchar    *property_group,
			       guint	       property_id,
			       GParamSpec     *pspec)
{
  g_return_if_fail (BSE_IS_OBJECT_CLASS (class));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  g_return_if_fail (sfi_pspec_get_group (pspec) == NULL);
  
  sfi_pspec_set_group (pspec, property_group);
  bse_object_class_add_grouped_property (class, property_id, pspec);
}

void
bse_object_marshal_signal (GClosure       *closure,
                           GValue /*out*/ *return_value,
                           guint           n_param_values,
                           const GValue   *param_values,
                           gpointer        invocation_hint,
                           gpointer        marshal_data)
{
  gpointer arg0, argN;
  
  g_return_if_fail (return_value == NULL);
  g_return_if_fail (n_param_values >= 1 && n_param_values <= 1 + SFI_VMARSHAL_MAX_ARGS);
  g_return_if_fail (G_VALUE_HOLDS_OBJECT (param_values));

  arg0 = g_value_get_object (param_values);
  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      argN = arg0;
      arg0 = closure->data;
    }
  else
    argN = closure->data;
  sfi_vmarshal_void (((GCClosure*) closure)->callback,
		     arg0,
		     n_param_values - 1,
		     param_values + 1,
		     argN);
}

guint
bse_object_class_add_signal (BseObjectClass    *oclass,
			     const gchar       *signal_name,
			     GType              return_type,
			     guint              n_params,
			     ...)
{
  va_list args;
  guint signal_id;
  
  g_return_val_if_fail (BSE_IS_OBJECT_CLASS (oclass), 0);
  g_return_val_if_fail (n_params <= SFI_VMARSHAL_MAX_ARGS, 0);
  g_return_val_if_fail (signal_name != NULL, 0);
  
  va_start (args, n_params);
  signal_id = g_signal_new_valist (signal_name,
				   G_TYPE_FROM_CLASS (oclass),
				   G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
				   NULL, NULL, NULL,
				   bse_object_marshal_signal,
				   return_type,
				   n_params, args);
  va_end (args);
  
  return signal_id;
}

guint
bse_object_class_add_asignal (BseObjectClass    *oclass,
			      const gchar       *signal_name,
			      GType              return_type,
			      guint              n_params,
			      ...)
{
  va_list args;
  guint signal_id;
  
  g_return_val_if_fail (BSE_IS_OBJECT_CLASS (oclass), 0);
  g_return_val_if_fail (n_params <= SFI_VMARSHAL_MAX_ARGS, 0);
  g_return_val_if_fail (signal_name != NULL, 0);
  
  va_start (args, n_params);
  signal_id = g_signal_new_valist (signal_name,
				   G_TYPE_FROM_CLASS (oclass),
				   G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS | G_SIGNAL_ACTION,
				   NULL, NULL, NULL,
				   bse_object_marshal_signal,
				   return_type,
				   n_params, args);
  va_end (args);
  
  return signal_id;
}

guint
bse_object_class_add_dsignal (BseObjectClass    *oclass,
			      const gchar       *signal_name,
			      GType              return_type,
			      guint              n_params,
			      ...)
{
  va_list args;
  guint signal_id;
  
  g_return_val_if_fail (BSE_IS_OBJECT_CLASS (oclass), 0);
  g_return_val_if_fail (n_params <= SFI_VMARSHAL_MAX_ARGS, 0);
  g_return_val_if_fail (signal_name != NULL, 0);
  
  va_start (args, n_params);
  signal_id = g_signal_new_valist (signal_name,
				   G_TYPE_FROM_CLASS (oclass),
				   G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS | G_SIGNAL_DETAILED,
				   NULL, NULL, NULL,
				   bse_object_marshal_signal,
				   return_type,
				   n_params, args);
  va_end (args);
  
  return signal_id;
}

void
bse_object_lock (gpointer _object)
{
  BseObject *object = _object;
  GObject *gobject = _object;
  
  g_return_if_fail (BSE_IS_OBJECT (object));
  g_return_if_fail (gobject->ref_count > 0);
  
  g_assert (object->lock_count < 65535);	// if this breaks, we need to fix the guint16
  
  if (!object->lock_count)
    {
      g_object_ref (object);
      
      /* we also keep the globals locked so we don't need to duplicate
       * this all over the place
       */
      bse_gconfig_lock ();
    }
  
  object->lock_count += 1;
}

void
bse_object_unlock (gpointer _object)
{
  BseObject *object = _object;

  g_return_if_fail (BSE_IS_OBJECT (object));
  g_return_if_fail (object->lock_count > 0);
  
  object->lock_count -= 1;
  
  if (!object->lock_count)
    {
      /* release global lock */
      bse_gconfig_unlock ();
      
      if (BSE_OBJECT_GET_CLASS (object)->unlocked)
	BSE_OBJECT_GET_CLASS (object)->unlocked (object);
      
      g_object_unref (object);
    }
}

gpointer
bse_object_from_id (guint unique_id)
{
  return sfi_ustore_lookup (object_id_ustore, unique_id);
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

static gboolean
object_check_pspec_editable (BseObject      *object,
                             GParamSpec     *pspec)
{
  if (sfi_pspec_check_option (pspec, "ro"))     /* RDONLY option (GUI) */
    return FALSE;
  BseObjectClass *class = g_type_class_peek (pspec->owner_type);
  if (class && class->editable_property)
    return class->editable_property (object, pspec->param_id, pspec) != FALSE;
  else
    return TRUE;
}

gboolean
bse_object_editable_property (gpointer        object,
                              const gchar    *property)
{
  GParamSpec *pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object), property);
  if (!pspec || !(pspec->flags & G_PARAM_WRITABLE))
    return FALSE;
  return BSE_OBJECT_GET_CLASS (object)->check_pspec_editable (object, pspec);
}

void
bse_object_notify_icon_changed (BseObject *object)
{
  g_return_if_fail (BSE_IS_OBJECT (object));
  
  g_signal_emit (object, object_signals[SIGNAL_ICON_CHANGED], 0);
}

BseIcon*
bse_object_get_icon (BseObject *object)
{
  BseIcon *icon;
  
  g_return_val_if_fail (BSE_IS_OBJECT (object), NULL);
  
  g_object_ref (object);
  
  icon = BSE_OBJECT_GET_CLASS (object)->get_icon (object);
  
  g_object_unref (object);
  
  return icon;
}

static BseIcon*
bse_object_do_get_icon (BseObject *object)
{
  BseIcon *icon;
  
  g_return_val_if_fail (BSE_IS_OBJECT (object), NULL);

  icon = g_object_get_qdata (G_OBJECT (object), bse_quark_icon);
  if (!icon)
    {
      BseCategorySeq *cseq;
      guint i;

      /* FIXME: this is a bit of a hack, we could store the first per-type
       * category icon as static type-data and fetch that from here
       */
      cseq = bse_categories_from_type (G_OBJECT_TYPE (object));
      for (i = 0; i < cseq->n_cats; i++)
	if (cseq->cats[i]->icon)
	  {
	    icon = bse_icon_copy_shallow (cseq->cats[i]->icon);
	    g_object_set_qdata_full (G_OBJECT (object), bse_quark_icon, icon, (GDestroyNotify) bse_icon_free);
	    break;
	  }
      bse_category_seq_free (cseq);
    }
  return icon;
}

static void
bse_object_store_private (BseObject	*object,
			  BseStorage *storage)
{
}

void
bse_object_restore_start (BseObject  *object,
                          BseStorage *storage)
{
  g_return_if_fail (BSE_IS_STORAGE (storage));
  if (!BSE_OBJECT_IN_RESTORE (object))
    {
      BSE_OBJECT_SET_FLAGS (object, BSE_OBJECT_FLAG_IN_RESTORE);
      bse_storage_add_restorable (storage, object);
      BSE_OBJECT_GET_CLASS (object)->restore_start (object, storage);
    }
}

static void
object_restore_start (BseObject      *object,
                      BseStorage     *storage)
{
}

static SfiTokenType
object_restore_private (BseObject      *object,
                        BseStorage     *storage,
                        GScanner       *scanner)
{
  return SFI_TOKEN_UNMATCHED;
}

static void
object_restore_finish (BseObject      *object)
{
}

void
bse_object_restore_finish (BseObject *object)
{
  if (BSE_OBJECT_IN_RESTORE (object))
    {
      BSE_OBJECT_GET_CLASS (object)->restore_finish (object);
      BSE_OBJECT_UNSET_FLAGS (object, BSE_OBJECT_FLAG_IN_RESTORE);
    }
}

typedef struct {
  GClosure closure;
  guint    dest_signal;
  GQuark   dest_detail;
  guint    erefs;
  gpointer src_object;
  gulong   handler;
  guint    src_signal;
  guint    src_detail;
} EClosure;

static guint
eclosure_hash (gconstpointer c)
{
  const EClosure *e = c;
  guint h = G_HASH_LONG ((long) e->src_object) >> 2;
  h += G_HASH_LONG ((long) e->closure.data) >> 1;
  h += e->src_detail;
  h += e->dest_detail << 1;
  h += e->src_signal << 12;
  h += e->dest_signal << 13;
  return h;
}

static gint
eclosure_equals (gconstpointer c1,
		 gconstpointer c2)
{
  const EClosure *e1 = c1, *e2 = c2;
  return (e1->src_object   == e2->src_object &&
	  e1->closure.data == e2->closure.data &&
	  e1->src_detail   == e2->src_detail &&
	  e1->dest_detail  == e2->dest_detail &&
	  e1->src_signal   == e2->src_signal &&
	  e1->dest_signal  == e2->dest_signal);
}

static void
eclosure_marshal (GClosure       *closure,
		  GValue /*out*/ *return_value,
		  guint           n_param_values,
		  const GValue   *param_values,
		  gpointer        invocation_hint,
		  gpointer        marshal_data)
{
  EClosure *e = (EClosure*) closure;
  if (e->dest_signal)
    g_signal_emit (e->closure.data, e->dest_signal, e->dest_detail);
  else
    g_object_notify (e->closure.data, g_quark_to_string (e->dest_detail));
}

void
bse_object_reemit_signal (gpointer     src_object,
			  const gchar *src_signal,
			  gpointer     dest_object,
			  const gchar *dest_signal)
{
  EClosure key;
  if (g_signal_parse_name (src_signal, G_OBJECT_TYPE (src_object), &key.src_signal, &key.src_detail, TRUE) &&
      g_signal_parse_name (dest_signal, G_OBJECT_TYPE (dest_object), &key.dest_signal, &key.dest_detail, TRUE))
    {
      EClosure *e;
      key.closure.data = dest_object;
      key.src_object = src_object;
      e = g_hash_table_lookup (eclosures_ht, &key);
      if (!e)
	{
	  GSignalQuery query;
          gboolean property_notify = key.dest_detail && strncmp (dest_signal, "notify", 6) == 0;
	  g_signal_query (key.dest_signal, &query);
	  if (!(query.return_type == G_TYPE_NONE &&
		((query.n_params == 0 &&
                  query.signal_flags & G_SIGNAL_ACTION) ||
		 (property_notify &&
                  g_object_class_find_property (G_OBJECT_GET_CLASS (dest_object),
                                                g_quark_to_string (key.dest_detail))))))
	    {
	      g_warning ("%s: invalid signal for reemission: \"%s\"", G_STRLOC, dest_signal);
	      return;
	    }
	  e = (EClosure*) g_closure_new_simple (sizeof (EClosure), dest_object);
	  e->erefs = 1;
	  e->closure.data = dest_object;
	  e->src_object = src_object;
	  e->dest_signal = property_notify ? 0 : key.dest_signal;
	  e->dest_detail = key.dest_detail;
	  e->src_signal = key.src_signal;
	  e->src_detail = key.src_detail;
	  g_closure_set_marshal (&e->closure, eclosure_marshal);
	  g_closure_ref (&e->closure);
	  g_closure_sink (&e->closure);
	  g_signal_connect_closure_by_id (e->src_object,
					  e->src_signal, e->src_detail,
					  &e->closure, G_CONNECT_AFTER);
	  g_hash_table_insert (eclosures_ht, e, e);
	}
      else
	e->erefs++;
    }
  else
    g_warning ("%s: invalid signal specs: \"%s\", \"%s\"",
	       G_STRLOC, src_signal, dest_signal);
}

void
bse_object_remove_reemit (gpointer     src_object,
			  const gchar *src_signal,
			  gpointer     dest_object,
			  const gchar *dest_signal)
{
  EClosure key;
  if (g_signal_parse_name (dest_signal, G_OBJECT_TYPE (dest_object), &key.dest_signal, &key.dest_detail, TRUE) &&
      g_signal_parse_name (src_signal, G_OBJECT_TYPE (src_object), &key.src_signal, &key.src_detail, TRUE))
    {
      gboolean property_notify = key.dest_detail && strncmp (dest_signal, "notify", 6) == 0;
      EClosure *e;
      key.closure.data = dest_object;
      key.src_object = src_object;
      key.dest_signal = property_notify ? 0 : key.dest_signal;
      e = g_hash_table_lookup (eclosures_ht, &key);
      if (e)
	{
	  g_return_if_fail (e->erefs > 0);

	  e->erefs--;
	  if (!e->erefs)
	    {
	      g_hash_table_remove (eclosures_ht, e);
	      g_signal_handlers_disconnect_matched (e->src_object, G_SIGNAL_MATCH_CLOSURE |
						    G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_DETAIL,
						    e->src_signal, e->src_detail,
						    &e->closure, NULL, NULL);
	      g_closure_invalidate (&e->closure);
	      g_closure_unref (&e->closure);
	    }
	}
      else
	g_warning ("%s: no reemission for object %s signal \"%s\" to object %s signal \"%s\"",
		   G_STRLOC, bse_object_debug_name (src_object), src_signal,
		   bse_object_debug_name (dest_object), dest_signal);
    }
  else
    g_warning ("%s: invalid signal specs: \"%s\", \"%s\"",
	       G_STRLOC, src_signal, dest_signal);
}

static void
bse_object_class_base_init (BseObjectClass *class)
{
  class->editable_property = NULL;
}

static void
bse_object_class_base_finalize (BseObjectClass *class)
{
}

static void
bse_object_class_init (BseObjectClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  bse_quark_uname = g_quark_from_static_string ("bse-object-uname");
  bse_quark_icon = g_quark_from_static_string ("bse-object-icon");
  quark_property_changed_queue = g_quark_from_static_string ("bse-property-changed-queue");
  quark_blurb = g_quark_from_static_string ("bse-object-blurb");
  object_unames_ht = g_hash_table_new (bse_string_hash, bse_string_equals);
  eclosures_ht = g_hash_table_new (eclosure_hash, eclosure_equals);
  object_id_ustore = sfi_ustore_new ();
  
  gobject_class->set_property = bse_object_do_set_property;
  gobject_class->get_property = bse_object_do_get_property;
  gobject_class->dispose = bse_object_do_dispose;
  gobject_class->finalize = bse_object_do_finalize;
  
  class->check_pspec_editable = object_check_pspec_editable;
  class->set_uname = bse_object_do_set_uname;
  class->store_private = bse_object_store_private;
  class->restore_start = object_restore_start;
  class->restore_private = object_restore_private;
  class->restore_finish = object_restore_finish;
  class->unlocked = NULL;
  class->get_icon = bse_object_do_get_icon;
  
  bse_object_class_add_param (class, NULL,
			      PROP_UNAME,
			      sfi_pspec_string ("uname", "Name", "Unique name of this object",
						NULL,
						SFI_PARAM_GUI ":lax-validation"
						/* watch out, unames are specially
						 * treated within the various
						 * objects, specifically BseItem
						 * and BseContainer.
						 */));
  bse_object_class_add_param (class, NULL,
			      PROP_BLURB,
			      sfi_pspec_string ("blurb", "Comment", NULL,
						NULL,
						SFI_PARAM_STANDARD ":skip-default"));
  
  object_signals[SIGNAL_RELEASE] = bse_object_class_add_signal (class, "release",
								G_TYPE_NONE, 0);
  object_signals[SIGNAL_ICON_CHANGED] = bse_object_class_add_signal (class, "icon_changed",
								     G_TYPE_NONE, 0);
}

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
  
  return bse_type_register_abstract (G_TYPE_OBJECT,
                                     "BseObject",
                                     "BSE Object Hierarchy base type",
                                     &object_info);
}
