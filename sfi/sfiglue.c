/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "sfiglue.h"

#include "sfiparams.h"
#include <string.h>
#include <gobject/gvaluecollector.h>

#include "sfigluesignal.c"


/* --- prototype --- */
static GHashTable*	glue_gc_hash_table_new	(void);


/* --- variables --- */
static SfiRing    *context_stack = NULL;
static GHashTable *context_gc_hash = NULL;


/* --- functions --- */
void
sfi_glue_context_common_init (SfiGlueContext            *context,
			      const SfiGlueContextTable *vtable)
{
  g_return_if_fail (context->sighash == NULL);

  if (!context_gc_hash)
    context_gc_hash = glue_gc_hash_table_new ();

  context->table = *vtable;
  context->sighash = g_hash_table_new (glue_signal_hash, glue_signal_equal);
}

gboolean
sfi_glue_context_pending (SfiGlueContext *context)
{
  g_return_val_if_fail (context != NULL, FALSE);

  return context->sigqueue || context->gc_signals;
}

void
sfi_glue_context_dispatch (SfiGlueContext *context)
{
  g_return_if_fail (context != NULL);

  sfi_glue_signals_dispatch (context);
}


/* --- VTable API --- */
void
sfi_glue_context_push (SfiGlueContext *context)
{
  g_return_if_fail (context != NULL);

  context_stack = sfi_ring_prepend (context_stack, context);
}

SfiGlueContext*
sfi_glue_context_current (void)
{
  return context_stack ? context_stack->data : NULL;
}

void
sfi_glue_context_pop (void)
{
  g_return_if_fail (context_stack != NULL);

  context_stack = sfi_ring_remove_node (context_stack, context_stack);
}

static inline gulong
upper_power2 (gulong number)
{
  return number ? 1 << g_bit_storage (number - 1) : 0;
}

GParamSpec*
sfi_glue_describe_prop (SfiProxy     proxy,
			const gchar *prop_name)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  GParamSpec *pspec;

  g_return_val_if_fail (proxy != 0, NULL);
  g_return_val_if_fail (prop_name != NULL, NULL);

  pspec = context->table.describe_prop (context, proxy, prop_name);
  if (pspec)
    sfi_glue_gc_add (pspec, g_param_spec_unref);
  return pspec;
}

void
sfi_glue_proxy_set_prop (SfiProxy     proxy,
			 const gchar *prop,
			 GValue      *value)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);

  g_return_if_fail (proxy != 0);
  g_return_if_fail (prop != NULL);
  g_return_if_fail (G_IS_VALUE (value));

  context->table.proxy_set_prop (context, proxy, prop, value);
}

GValue*
sfi_glue_proxy_get_prop (SfiProxy     proxy,
			 const gchar *prop)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  GValue *value;

  g_return_val_if_fail (proxy != 0, NULL);
  g_return_val_if_fail (prop != NULL, NULL);

  value = context->table.proxy_get_prop (context, proxy, prop);
  if (value)
    sfi_glue_gc_add (value, sfi_value_free);
  return value;
}

SfiGlueProc*
sfi_glue_describe_proc (const gchar *proc_name)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  SfiGlueProc *proc;

  g_return_val_if_fail (proc_name != NULL, NULL);

  proc = context->table.describe_proc (context, proc_name);
  if (proc && !proc->proc_name)
    {
      sfi_glue_proc_unref (proc);
      proc = NULL;
    }
  else if (proc)
    sfi_glue_gc_add (proc, sfi_glue_proc_unref);
  return proc;
}

gchar**
sfi_glue_list_proc_names (void)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  gchar **names;

  names = context->table.list_proc_names (context);
  if (!names)
    names = g_new0 (gchar*, 1);
  sfi_glue_gc_add (names, g_strfreev);
  return names;
}

gchar**
sfi_glue_list_method_names (const gchar *iface_name)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  gchar **names;
  
  g_return_val_if_fail (iface_name != NULL, NULL);

  names = context->table.list_method_names (context, iface_name);
  if (!names)
    names = g_new0 (gchar*, 1);
  sfi_glue_gc_add (names, g_strfreev);
  return names;
}

gchar*
sfi_glue_base_iface (void)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  gchar *biface = context->table.base_iface (context);

  if (biface)
    sfi_glue_gc_add (biface, g_free);
  return biface;
}

gchar**
sfi_glue_iface_children (const gchar *iface_name)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  gchar **names;
  
  g_return_val_if_fail (iface_name != NULL, NULL);

  names = context->table.iface_children (context, iface_name);
  if (!names)
    names = g_new0 (gchar*, 1);
  sfi_glue_gc_add (names, g_strfreev);
  return names;
}

gchar*
sfi_glue_proxy_iface (SfiProxy proxy)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  gchar *iface = context->table.proxy_iface (context, proxy);

  if (iface)
    sfi_glue_gc_add (iface, g_free);
  return iface;
}

SfiGlueIFace*
sfi_glue_describe_iface (const gchar *iface_name)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  SfiGlueIFace *iface;

  g_return_val_if_fail (iface_name != NULL, NULL);

  iface = context->table.describe_iface (context, iface_name);
  if (iface)
    sfi_glue_gc_add (iface, sfi_glue_iface_unref);
  return iface;
}

GValue*
sfi_glue_client_msg (const gchar *msg,
		     GValue      *value)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  GValue *rvalue;

  rvalue = context->table.client_msg (context, msg, value);
  if (rvalue)
    sfi_glue_gc_add (rvalue, sfi_value_free);
  return rvalue;
}


/* --- procedure calls --- */
GValue*
sfi_glue_call_seq (const gchar *proc_name,
		   SfiSeq      *params)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  GValue *value;

  g_return_val_if_fail (proc_name != NULL, NULL);
  g_return_val_if_fail (params != NULL, NULL);

  value = context->table.exec_proc (context, proc_name, params);
  if (value)
    sfi_glue_gc_add (value, sfi_value_free);
  return value;
}

GValue*
sfi_glue_call_valist (const gchar *proc_name,
		      guint8       first_arg_type,
		      va_list      var_args)
{
  guint8 arg_type = first_arg_type;
  SfiSeq *seq;

  g_return_val_if_fail (proc_name != NULL, NULL);

  seq = sfi_seq_new ();
  while (arg_type)
    {
      gchar *error = NULL;
      GType collect_type = sfi_category_type (arg_type);
      if (!collect_type)
	error = g_strdup_printf ("%s: invalid category_type (%u)", G_STRLOC, arg_type);
      else
	{
	  GValue value = { 0, };
	  g_value_init (&value, collect_type);
	  G_VALUE_COLLECT (&value, var_args, 0, &error);
	  if (!error)
	    {
	      sfi_seq_append (seq, &value);
	      g_value_unset (&value);
	    }
	}
      if (error)
	{
	  g_warning ("%s: %s", G_STRLOC, error);
	  g_free (error);
	  sfi_seq_unref (seq);
	  return NULL;
	}
      arg_type = va_arg (var_args, guint8);
    }
  if (seq)
    {
      GValue *retval = sfi_glue_call_seq (proc_name, seq);
      sfi_seq_unref (seq);
      return retval;	/* already GC owned */
    }
  return NULL;
}

void
sfi_glue_vcall_void (const gchar    *proc_name,
		     guint8          first_arg_type,
		     ...)
{
  va_list var_args;
  GValue *rvalue;

  g_return_if_fail (proc_name != NULL);

  va_start (var_args, first_arg_type);
  rvalue = sfi_glue_call_valist (proc_name, first_arg_type, var_args);
  va_end (var_args);
  if (rvalue)
    sfi_glue_gc_collect_value (rvalue);
}

SfiBool
sfi_glue_vcall_bool (const gchar *proc_name,
		     guint8       first_arg_type,
		     ...)
{
  va_list var_args;
  GValue *rvalue;
  SfiBool retv = FALSE;

  g_return_val_if_fail (proc_name != NULL, FALSE);

  va_start (var_args, first_arg_type);
  rvalue = sfi_glue_call_valist (proc_name, first_arg_type, var_args);
  va_end (var_args);
  if (SFI_VALUE_HOLDS_BOOL (rvalue))
    retv = sfi_value_get_bool (rvalue);
  if (rvalue)
    sfi_glue_gc_collect_value (rvalue);
  return retv;
}

SfiInt
sfi_glue_vcall_int (const gchar *proc_name,
		    guint8       first_arg_type,
		    ...)
{
  va_list var_args;
  GValue *rvalue;
  SfiInt retv = 0;

  g_return_val_if_fail (proc_name != NULL, 0);

  va_start (var_args, first_arg_type);
  rvalue = sfi_glue_call_valist (proc_name, first_arg_type, var_args);
  va_end (var_args);
  if (SFI_VALUE_HOLDS_INT (rvalue))
    retv = sfi_value_get_int (rvalue);
  if (rvalue)
    sfi_glue_gc_collect_value (rvalue);
  return retv;
}

SfiNum
sfi_glue_vcall_num (const gchar    *proc_name,
		    guint8          first_arg_type,
		    ...)
{
  va_list var_args;
  GValue *rvalue;
  SfiNum retv = 0;

  g_return_val_if_fail (proc_name != NULL, 0);

  va_start (var_args, first_arg_type);
  rvalue = sfi_glue_call_valist (proc_name, first_arg_type, var_args);
  va_end (var_args);
  if (SFI_VALUE_HOLDS_NUM (rvalue))
    retv = sfi_value_get_num (rvalue);
  if (rvalue)
    sfi_glue_gc_collect_value (rvalue);
  return retv;
}

SfiReal
sfi_glue_vcall_real (const gchar    *proc_name,
		     guint8          first_arg_type,
		     ...)
{
  va_list var_args;
  GValue *rvalue;
  SfiReal retv = 0;

  g_return_val_if_fail (proc_name != NULL, 0);

  va_start (var_args, first_arg_type);
  rvalue = sfi_glue_call_valist (proc_name, first_arg_type, var_args);
  va_end (var_args);
  if (SFI_VALUE_HOLDS_REAL (rvalue))
    retv = sfi_value_get_real (rvalue);
  if (rvalue)
    sfi_glue_gc_collect_value (rvalue);
  return retv;
}

const gchar*
sfi_glue_vcall_string (const gchar    *proc_name,
		       guint8          first_arg_type,
		       ...)
{
  va_list var_args;
  GValue *rvalue;
  gchar *retv = NULL;

  g_return_val_if_fail (proc_name != NULL, NULL);

  va_start (var_args, first_arg_type);
  rvalue = sfi_glue_call_valist (proc_name, first_arg_type, var_args);
  va_end (var_args);
  if (SFI_VALUE_HOLDS_STRING (rvalue))
    {
      retv = sfi_value_get_string (rvalue);
      if (retv)
	{
	  retv = g_strdup (retv);
	  sfi_glue_gc_add (retv, g_free);
	}
    }
  if (rvalue)
    sfi_glue_gc_collect_value (rvalue);
  return retv;
}

const gchar*
sfi_glue_vcall_choice (const gchar    *proc_name,
		       guint8          first_arg_type,
		       ...)
{
  va_list var_args;
  GValue *rvalue;
  gchar *retv = NULL;

  g_return_val_if_fail (proc_name != NULL, NULL);

  va_start (var_args, first_arg_type);
  rvalue = sfi_glue_call_valist (proc_name, first_arg_type, var_args);
  va_end (var_args);
  if (SFI_VALUE_HOLDS_CHOICE (rvalue))
    {
      retv = sfi_value_get_choice (rvalue);
      if (retv)
	{
	  retv = g_strdup (retv);
	  sfi_glue_gc_add (retv, g_free);
	}
    }
  if (rvalue)
    sfi_glue_gc_collect_value (rvalue);
  return retv;
}

SfiProxy
sfi_glue_vcall_proxy (const gchar *proc_name,
		      guint8       first_arg_type,
		      ...)
{
  va_list var_args;
  GValue *rvalue;
  SfiProxy retv = 0;

  g_return_val_if_fail (proc_name != NULL, 0);

  va_start (var_args, first_arg_type);
  rvalue = sfi_glue_call_valist (proc_name, first_arg_type, var_args);
  va_end (var_args);
  if (SFI_VALUE_HOLDS_PROXY (rvalue))
    retv = sfi_value_get_proxy (rvalue);
  if (rvalue)
    sfi_glue_gc_collect_value (rvalue);
  return retv;
}

SfiSeq*
sfi_glue_vcall_seq (const gchar *proc_name,
		    guint8       first_arg_type,
		    ...)
{
  va_list var_args;
  GValue *rvalue;
  SfiSeq *retv = NULL;

  g_return_val_if_fail (proc_name != NULL, NULL);

  va_start (var_args, first_arg_type);
  rvalue = sfi_glue_call_valist (proc_name, first_arg_type, var_args);
  va_end (var_args);
  if (SFI_VALUE_HOLDS_SEQ (rvalue))
    {
      retv = sfi_value_get_seq (rvalue);
      if (retv)
	sfi_glue_gc_add (sfi_seq_ref (retv), sfi_seq_unref);
    }
  if (rvalue)
    sfi_glue_gc_collect_value (rvalue);
  return retv;
}

SfiRec*
sfi_glue_vcall_rec (const gchar *proc_name,
		    guint8       first_arg_type,
		    ...)
{
  va_list var_args;
  GValue *rvalue;
  SfiRec *retv = NULL;

  g_return_val_if_fail (proc_name != NULL, NULL);

  va_start (var_args, first_arg_type);
  rvalue = sfi_glue_call_valist (proc_name, first_arg_type, var_args);
  va_end (var_args);
  if (SFI_VALUE_HOLDS_REC (rvalue))
    {
      retv = sfi_value_get_rec (rvalue);
      if (retv)
	sfi_glue_gc_add (sfi_rec_ref (retv), sfi_rec_unref);
    }
  if (rvalue)
    sfi_glue_gc_collect_value (rvalue);
  return retv;
}

SfiFBlock*
sfi_glue_vcall_fblock (const gchar *proc_name,
		       guint8       first_arg_type,
		       ...)
{
  va_list var_args;
  GValue *rvalue;
  SfiFBlock *retv = NULL;

  g_return_val_if_fail (proc_name != NULL, NULL);

  va_start (var_args, first_arg_type);
  rvalue = sfi_glue_call_valist (proc_name, first_arg_type, var_args);
  va_end (var_args);
  if (SFI_VALUE_HOLDS_FBLOCK (rvalue))
    {
      retv = sfi_value_get_fblock (rvalue);
      if (retv)
	sfi_glue_gc_add (sfi_fblock_ref (retv), sfi_fblock_unref);
    }
  if (rvalue)
    sfi_glue_gc_collect_value (rvalue);
  return retv;
}

SfiBBlock*
sfi_glue_vcall_bblock (const gchar *proc_name,
		       guint8       first_arg_type,
		       ...)
{
  va_list var_args;
  GValue *rvalue;
  SfiBBlock *retv = NULL;

  g_return_val_if_fail (proc_name != NULL, NULL);

  va_start (var_args, first_arg_type);
  rvalue = sfi_glue_call_valist (proc_name, first_arg_type, var_args);
  va_end (var_args);
  if (SFI_VALUE_HOLDS_BBLOCK (rvalue))
    {
      retv = sfi_value_get_bblock (rvalue);
      if (retv)
	sfi_glue_gc_add (sfi_bblock_ref (retv), sfi_bblock_unref);
    }
  if (rvalue)
    sfi_glue_gc_collect_value (rvalue);
  return retv;
}


/* --- Glue utilities --- */
SfiGlueIFace*
_sfi_glue_iface_new (const gchar *iface_name)
{
  SfiGlueIFace *iface;

  g_return_val_if_fail (iface_name != NULL, NULL);

  iface = g_new0 (SfiGlueIFace, 1);
  iface->type_name = g_strdup (iface_name);
  iface->n_ifaces = 0;
  iface->ifaces = NULL;
  iface->n_props = 0;
  iface->props = NULL;
  iface->n_signals = 0;
  iface->signals = NULL;
  iface->ref_count = 1;

  return iface;
}

SfiGlueIFace*
sfi_glue_iface_ref (SfiGlueIFace *iface)
{
  g_return_val_if_fail (iface != NULL, NULL);
  g_return_val_if_fail (iface->ref_count > 0, NULL);

  iface->ref_count++;
  return iface;
}

void
sfi_glue_iface_unref (SfiGlueIFace *iface)
{
  g_return_if_fail (iface != NULL);
  g_return_if_fail (iface->ref_count > 0);

  iface->ref_count--;
  if (!iface->ref_count)
    {
      g_return_if_fail (_sfi_glue_gc_test (iface, sfi_glue_iface_unref) == FALSE);

      g_free (iface->type_name);
      g_strfreev (iface->ifaces);
      g_strfreev (iface->props);
      g_strfreev (iface->signals);
      g_free (iface);
    }
}

SfiGlueProc*
_sfi_glue_proc_new (void)
{
  SfiGlueProc *p;

  p = g_new0 (SfiGlueProc, 1);
  p->proc_name = NULL;
  p->ret_param = NULL;
  p->n_params = 0;
  p->params = NULL;
  p->ref_count = 1;
  return p;
}

void
_sfi_glue_proc_add_param (SfiGlueProc *proc,
			  GParamSpec  *param)
{
  guint i;

  g_return_if_fail (proc != NULL);
  g_return_if_fail (param != NULL);

  i = proc->n_params++;
  proc->params = g_renew (GParamSpec*, proc->params, proc->n_params);
  proc->params[i] = g_param_spec_ref (param);
  g_param_spec_sink (param);
}

void
_sfi_glue_proc_add_ret_param (SfiGlueProc *proc,
			      GParamSpec  *param)
{
  g_return_if_fail (proc != NULL);
  g_return_if_fail (param != NULL);
  g_return_if_fail (proc->ret_param == NULL);

  proc->ret_param = g_param_spec_ref (param);
  g_param_spec_sink (param);
}

SfiGlueProc*
sfi_glue_proc_ref (SfiGlueProc *proc)
{
  g_return_val_if_fail (proc != NULL, NULL);
  g_return_val_if_fail (proc->ref_count > 0, NULL);

  proc->ref_count++;
  return proc;
}

void
sfi_glue_proc_unref (SfiGlueProc *proc)
{
  g_return_if_fail (proc != NULL);
  g_return_if_fail (proc->ref_count > 0);

  proc->ref_count--;
  if (!proc->ref_count)
    {
      guint i;

      g_return_if_fail (_sfi_glue_gc_test (proc, sfi_glue_proc_unref) == FALSE);

      if (proc->ret_param)
	g_param_spec_unref (proc->ret_param);
      for (i = 0; i < proc->n_params; i++)
	g_param_spec_unref (proc->params[i]);
      g_free (proc->params);
      g_free (proc->proc_name);
      g_free (proc);
    }
}


/* --- garbage collector --- */
typedef struct {
  gpointer data;
  gpointer free_func;
} GcEntry;

static guint
glue_gc_entry_hash (gconstpointer key)
{
  const GcEntry *e = key;
  guint h = (glong) e->data;
  h ^= (glong) e->free_func;
  return h;
}

static gboolean
glue_gc_entry_equal (gconstpointer key1,
		     gconstpointer key2)
{
  const GcEntry *e1 = key1;
  const GcEntry *e2 = key2;

  return e1->free_func == e2->free_func && e1->data == e2->data;
}

static GHashTable*
glue_gc_hash_table_new (void)
{
  return g_hash_table_new_full (glue_gc_entry_hash, glue_gc_entry_equal, NULL, g_free);
}

void
sfi_glue_gc_add (gpointer data,
		 gpointer free_func)
{
  GcEntry *entry;

  g_return_if_fail (free_func != NULL);
  g_return_if_fail (_sfi_glue_gc_test (data, free_func) == FALSE);

  entry = g_new (GcEntry, 1);
  entry->data = data;
  entry->free_func = free_func;
  g_hash_table_insert (context_gc_hash, entry, entry);
}

gboolean
_sfi_glue_gc_test (gpointer data,
		   gpointer free_func)
{
  GcEntry key;

  g_return_val_if_fail (free_func != NULL, FALSE);

  key.data = data;
  key.free_func = free_func;

  return g_hash_table_lookup (context_gc_hash, &key) != NULL;
}

void
sfi_glue_gc_remove (gpointer data,
		    gpointer free_func)
{
  GcEntry key;

  g_return_if_fail (free_func != NULL);
  g_return_if_fail (_sfi_glue_gc_test (data, free_func) == TRUE);
  
  key.data = data;
  key.free_func = free_func;
  g_hash_table_remove (context_gc_hash, &key);
}

void
sfi_glue_gc_free_now (gpointer data,
		      gpointer _free_func)
{
  void (*free_func) (gpointer data) = _free_func;

  g_return_if_fail (free_func != NULL);
  g_return_if_fail (_sfi_glue_gc_test (data, free_func) == TRUE);

  sfi_glue_gc_remove (data, free_func);
  free_func (data);
}

static gboolean
slist_entries (gpointer key,
	       gpointer value,
	       gpointer user_data)
{
  GSList **slist_p = user_data;
  *slist_p = g_slist_prepend (*slist_p, value);
  return TRUE;
}

void
sfi_glue_gc_run (void)
{
  GSList *slist, *gclist = NULL;

  g_hash_table_foreach_steal (context_gc_hash, slist_entries, &gclist);
  for (slist = gclist; slist; slist = slist->next)
    {
      GcEntry *entry = slist->data;
      void (*free_func) (gpointer data) = entry->free_func;
      free_func (entry->data);
      g_free (entry);
    }
  g_slist_free (gclist);
}

void
sfi_glue_gc_collect_value (GValue *value)
{
  g_return_if_fail (value != NULL);

  sfi_glue_gc_free_now (value, sfi_value_free);
}

void
sfi_glue_gc_collect_iface (SfiGlueIFace *iface)
{
  g_return_if_fail (iface != NULL);
  
  sfi_glue_gc_free_now (iface, sfi_glue_iface_unref);
}

void
sfi_glue_gc_collect_proc (SfiGlueProc *proc)
{
  g_return_if_fail (proc != NULL);

  sfi_glue_gc_free_now (proc, sfi_glue_proc_unref);
}

void
sfi_glue_gc_collect_pspec (GParamSpec *pspec)
{
  g_return_if_fail (pspec != NULL);

  sfi_glue_gc_free_now (pspec, g_param_spec_unref);
}


/* vim:set ts=8 sts=2 sw=2: */
