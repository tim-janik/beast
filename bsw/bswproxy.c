/* BSW - Bedevilled Sound Engine Wrapper
 * Copyright (C) 2000-2002 Tim Janik
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
#include "bswproxy.h"

#include <bse/bse.h>
#include <gobject/gvaluecollector.h>

#include <bse/bswprivate.h>

typedef struct {
  BswProxy  proxy;
  gchar    *name;
  gpointer  data;
} ProxyHashKey;


/* --- variables --- */
gchar *bsw_log_domain_bsw = "BSW";
static GHashTable *bsw_proxy_hash = NULL;


/* --- functions --- */
static guint
proxy_hash (gconstpointer key)
{
  const ProxyHashKey *k = key;
  guint h = k->proxy;

#if	GLIB_SIZEOF_LONG > 4
  h ^= k->proxy >> 32;
#endif
  h ^= g_str_hash (k->name);

  return h;
}

static gboolean
proxy_equal (gconstpointer v1,
	     gconstpointer v2)
{
  const ProxyHashKey *k1 = v1;
  const ProxyHashKey *k2 = v2;

  return k1->proxy == k2->proxy && strcmp (k1->name, k2->name) == 0;
}

void
bsw_init (gint               *argc,
	  gchar             **argv[],
	  const BswLockFuncs *lock_funcs)
{
  BseLockFuncs lfuncs;

  g_return_if_fail (bsw_proxy_hash == NULL);

  bsw_proxy_hash = g_hash_table_new (proxy_hash, proxy_equal);

  if (lock_funcs)
    {
      g_return_if_fail (lock_funcs->lock != NULL);
      g_return_if_fail (lock_funcs->unlock != NULL);

      lfuncs.lock_data = lock_funcs->lock_data;
      lfuncs.lock = lock_funcs->lock;
      lfuncs.unlock = lock_funcs->unlock;
    }
  
  bse_init (argc, argv, lock_funcs ? &lfuncs : NULL);

  g_assert (BSW_MIN_NOTE == BSE_MIN_NOTE &&
	    BSW_MAX_NOTE == BSE_MAX_NOTE &&
	    BSW_NOTE_VOID == BSE_NOTE_VOID &&
	    BSW_NOTE_UNPARSABLE == BSE_NOTE_UNPARSABLE &&
	    BSW_KAMMER_OCTAVE == BSE_KAMMER_OCTAVE &&
	    BSW_MIN_OCTAVE == BSE_MIN_OCTAVE &&
	    BSW_MAX_OCTAVE == BSE_MAX_OCTAVE &&
	    BSW_MIN_FINE_TUNE == BSE_MIN_FINE_TUNE &&
	    BSW_MAX_FINE_TUNE == BSE_MAX_FINE_TUNE);
}

void
bsw_proxy_call_procedure (BswProxyProcedureCall *closure)
{
  GType proc_type;
  BseProcedureClass *proc;
  BseErrorType result = BSE_ERROR_NONE;

  g_return_if_fail (closure != NULL);
  g_return_if_fail (closure->proc_name != NULL);

  proc_type = g_type_from_name (closure->proc_name);
  g_return_if_fail (BSE_TYPE_IS_PROCEDURE (proc_type));

  proc = g_type_class_ref (proc_type);
  if (proc->n_in_pspecs == closure->n_ivalues && proc->n_out_pspecs <= 1)
    {
      guint i;

      for (i = 0; i < closure->n_ivalues; i++)
	{
	  GValue *value = closure->ivalues + i;

	  bsw_value_glue2bse (value, value, proc->in_pspecs[i]->value_type);
	}
      if (proc->n_out_pspecs)
	bsw_value_glue2bse (&closure->ovalue, &closure->ovalue, proc->out_pspecs[0]->value_type);
      result = bse_procedure_marshal (proc_type, closure->ivalues, &closure->ovalue, NULL, NULL);
      for (i = 0; i < closure->n_ivalues; i++)
	g_value_unset (closure->ivalues + i);
      if (proc->n_out_pspecs)
	bsw_value_glue2bsw (&closure->ovalue, &closure->ovalue);
    }
  else
    g_warning ("closure parameters mismatch procedure");

  if (result != BSE_ERROR_NONE)
    g_warning ("procedure \"%s\" execution failed: %s",
	       proc->name,
	       bse_error_blurb (result));

  g_type_class_unref (proc);
}

static BswProxy
bsw_proxy_lookup (gpointer bse_object)
{
  return BSE_IS_OBJECT (bse_object) ? BSE_OBJECT_ID (bse_object) : 0;
}

BswProxy
bsw_proxy_get_server (void)
{
  return bsw_proxy_lookup (bse_server_get ());
}

typedef struct {
  gpointer next;
  gpointer data;
  void (*free_func) (gpointer data);
} GCData;

static GCData *collector_list = NULL;
static guint   collector_id = 0;

static gboolean
garbage_collector (gpointer data)
{
  GCData *list;
  BSE_THREADS_ENTER ();
  list = collector_list;
  collector_list = NULL;
  collector_id = 0;
  while (list)
    {
      GCData *tmp = list->next;
      list->free_func (list->data);
      g_list_free_1 ((GList*) list);
      list = tmp;
    }
  BSE_THREADS_LEAVE ();
  return FALSE;
}

static void
add_gc (gpointer data,
	gpointer free_func)
{
  GCData *gcdata = (gpointer) g_list_alloc ();	/* abuse GList memchunk for this */
  g_assert (sizeof (*gcdata) == sizeof (GList));

  if (!collector_id)
    collector_id = bse_idle_background (garbage_collector, NULL);
  gcdata->data = data;
  gcdata->free_func = free_func;
  gcdata->next = collector_list;
  collector_list = gcdata;
}

gchar*
bsw_collector_get_string (GValue *value)
{
  gchar *str = g_value_dup_string (value);
  add_gc (str, g_free);
  return str;
}

void
bsw_proxy_set (BswProxy     proxy,
	       const gchar *prop,
	       ...)
{
  va_list var_args;
  GObject *object = bse_object_from_id (proxy);

  g_return_if_fail (BSE_IS_ITEM (object));

  g_object_freeze_notify (object);
  va_start (var_args, prop);
  while (prop)
    {
      GParamSpec *pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object), prop);
      GValue value = { 0, };
      gchar *error = NULL;
      gboolean mask_proxy = FALSE;

      if (pspec)
	switch (G_TYPE_FUNDAMENTAL (G_PARAM_SPEC_VALUE_TYPE (pspec)))
	  {
	  case G_TYPE_CHAR: case G_TYPE_UCHAR: case G_TYPE_BOOLEAN: case G_TYPE_INT:
	  case G_TYPE_UINT: case G_TYPE_LONG: case G_TYPE_ULONG: case G_TYPE_ENUM:
	  case G_TYPE_FLAGS: case G_TYPE_FLOAT: case G_TYPE_DOUBLE: case G_TYPE_STRING:
	  case BSE_TYPE_TIME: case BSE_TYPE_DOTS:
	    break;
	  default:
	    if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (pspec), BSE_TYPE_OBJECT))
	      mask_proxy = TRUE;
	    else
	      pspec = NULL;
	    break;
	  }
      if (!pspec)
	{
	  g_warning (G_STRLOC ": invalid property name `%s'", prop);
	  break;
	}
      g_value_init (&value, bsw_property_type_from_bse (G_PARAM_SPEC_VALUE_TYPE (pspec)));
      G_VALUE_COLLECT (&value, var_args, 0, &error);
      if (error)
	{
	  g_warning ("%s: failed to collect `%s': %s", G_STRLOC, pspec->name, error);
	  g_free (error);

	  /* we purposely leak the value here, it might not be
	   * in a sane state if an error condition occoured
	   */
	  break;
	}
      bsw_value_glue2bse (&value, &value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      g_object_set_property (object, pspec->name, &value);
      g_value_unset (&value);
      prop = va_arg (var_args, gchar*);
    }
  va_end (var_args);
  g_object_thaw_notify (object);
}

GParamSpec*
bsw_proxy_get_pspec (BswProxy     proxy,
		     const gchar *name)
{
  GObject *object;

  object = bse_object_from_id (proxy);

  g_return_val_if_fail (G_IS_OBJECT (object), NULL);

  return g_object_class_find_property (G_OBJECT_GET_CLASS (object), name);
}

GType
bsw_proxy_type (BswProxy proxy)
{
  GObject *object = bse_object_from_id (proxy);

  return BSE_IS_OBJECT (object) ? G_OBJECT_TYPE (object) : 0;
}

gboolean
bsw_proxy_check_is_a (BswProxy proxy,
		      GType    type)
{
  return g_type_check_instance_is_a (bse_object_from_id (proxy), type);
}

void
bsw_proxy_set_data (BswProxy     proxy,
		    const gchar *name,
		    gpointer     data)
{
  ProxyHashKey key, *k;

  g_return_if_fail (proxy > 0);
  g_return_if_fail (name != NULL);

  if (!data)
    bsw_proxy_remove_data (proxy, name);

  key.proxy = proxy;
  key.name = (gchar*) name;
  k = g_hash_table_lookup (bsw_proxy_hash, &key);
  if (!k)
    {
      k = g_new (ProxyHashKey, 1);
      k->proxy = proxy;
      k->name = g_strdup (name);
      g_hash_table_insert (bsw_proxy_hash, k, k);
    }
  k->data = data;
}

gpointer
bsw_proxy_get_data (BswProxy     proxy,
		    const gchar *name)
{
  ProxyHashKey key, *k;

  g_return_val_if_fail (proxy > 0, NULL);
  g_return_val_if_fail (name != NULL, NULL);

  key.proxy = proxy;
  key.name = (gchar*) name;
  k = g_hash_table_lookup (bsw_proxy_hash, &key);

  return k ? k->data : NULL;
}

void
bsw_proxy_remove_data (BswProxy     proxy,
		       const gchar *name)
{
  ProxyHashKey key, *k;

  g_return_if_fail (proxy > 0);
  g_return_if_fail (name != NULL);

  key.proxy = proxy;
  key.name = (gchar*) name;
  k = g_hash_table_lookup (bsw_proxy_hash, &key);
  if (k)
    {
      g_hash_table_remove (bsw_proxy_hash, k);
      g_free (k->name);
      g_free (k);
    }
}
