/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002, 2003 Tim Janik
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
#include "sfiglueproxy.h"
#include "sfiustore.h"
#include "sfimsg.h"
#include <string.h>
#include <gobject/gvaluecollector.h>


/* --- prototype --- */
static GHashTable*	glue_gc_hash_table_new	(void);


/* --- variables --- */
static GQuark      quark_context_stack = 0;


/* --- context functions --- */
void
_sfi_init_glue (void)
{
  g_assert (quark_context_stack == 0);
  
  quark_context_stack = g_quark_from_static_string ("sfi-glue-context-stack");
}

void
sfi_glue_context_common_init (SfiGlueContext            *context,
			      const SfiGlueContextTable *vtable)
{
  g_return_if_fail (context->table.base_iface == NULL);
  
  context->table = *vtable;
  context->seq_hook_id = 1;
  context->proxies = sfi_ustore_new ();
  context->pending_events = NULL;
  context->gc_hash = glue_gc_hash_table_new ();
}

void
sfi_glue_context_push (SfiGlueContext *context)
{
  g_return_if_fail (context != NULL);
  g_return_if_fail (context->table.destroy != NULL);
  
  sfi_thread_set_qdata_full (quark_context_stack,
			     sfi_ring_prepend (sfi_thread_steal_qdata (quark_context_stack),
					       context),
			     (GDestroyNotify) sfi_ring_free);
}

SfiGlueContext*
sfi_glue_context_current (void)
{
  SfiRing *context_stack = sfi_thread_get_qdata (quark_context_stack);
  return context_stack ? context_stack->data : NULL;
}

void
sfi_glue_context_pop (void)
{
  SfiRing *context_stack = sfi_thread_steal_qdata (quark_context_stack);
  
  g_return_if_fail (context_stack != NULL);
  
  sfi_thread_set_qdata_full (quark_context_stack,
			     sfi_ring_remove_node (context_stack, context_stack),
			     (GDestroyNotify) sfi_ring_free);
}

SfiRing*
sfi_glue_context_list_poll_fds (void)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  SfiRing *ring;

  /* pfds are owned by the context implementations, the ring is dynamic */
  ring = context->table.list_poll_fds (context);
  if (ring)
    sfi_glue_gc_add (ring, sfi_ring_free);
  return ring;
}

static void
sfi_glue_context_fetch_all_events (SfiGlueContext *context)
{
  context->pending_events = sfi_ring_concat (context->pending_events,
					     context->table.fetch_events (context));
}

gboolean
sfi_glue_context_pending (void)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  
  if (!context->pending_events)
    sfi_glue_context_fetch_all_events (context);
  return context->pending_events != NULL;
}

void
sfi_glue_context_process_fd (void)
{
  sfi_glue_context_pending ();
}

void
sfi_glue_context_dispatch (void)
{
  SfiSeq *seq = sfi_glue_context_fetch_event ();
  if (seq)
    {
      _sfi_glue_proxy_dispatch_event (seq);
      sfi_seq_unref (seq);
    }
}

SfiSeq*
sfi_glue_context_fetch_event (void)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  SfiSeq *seq = NULL;
  sfi_glue_context_fetch_all_events (context);
  seq = sfi_ring_pop_head (&context->pending_events);
  return seq;
}

gboolean
_sfi_glue_proxy_request_notify (SfiProxy        proxy,
				const gchar    *signal,
				gboolean        enable_notify)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  gboolean connected;

  g_return_val_if_fail (proxy != 0, FALSE);
  g_return_val_if_fail (signal != NULL, FALSE);

  connected = context->table.proxy_request_notify (context, proxy, signal, enable_notify);
  if (!enable_notify)					/* filter current event queue */
    {
      SfiRing *ring;
      GQuark signal_quark = sfi_glue_proxy_get_signal_quark (signal);
      sfi_glue_context_fetch_all_events (context);	/* fetch remaining events */
      for (ring = context->pending_events; ring; ring = sfi_ring_walk (ring, context->pending_events))
	sfi_glue_proxy_cancel_matched_event (ring->data, proxy, signal_quark);
    }
  return connected;
}

void
sfi_glue_context_destroy (SfiGlueContext *context)
{
  void (*destroy) (SfiGlueContext *);
  SfiSeq *seq;

  g_return_if_fail (context != NULL);

  sfi_glue_context_push (context);
  sfi_glue_gc_run ();
  _sfi_glue_context_clear_proxies (context);
  g_assert (context->proxies == NULL);
  sfi_glue_gc_run ();
  sfi_glue_context_pop ();
  destroy = context->table.destroy;
  memset (&context->table, 0, sizeof (context->table));
  g_hash_table_destroy (context->gc_hash);
  context->gc_hash = NULL;
  seq = sfi_ring_pop_head (&context->pending_events);
  while (seq)
    {
      sfi_seq_unref (seq);
      seq = sfi_ring_pop_head (&context->pending_events);
    }
  destroy (context);
}


/* --- VTable API --- */
static inline gulong
upper_power2 (gulong number)
{
  return number ? 1 << g_bit_storage (number - 1) : 0;
}

SfiGlueProc*
sfi_glue_describe_proc (const gchar *proc_name)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  SfiGlueProc *proc;
  
  g_return_val_if_fail (proc_name != NULL, NULL);
  
  proc = context->table.describe_proc (context, proc_name);
  if (proc && !proc->name)
    {
      sfi_glue_proc_unref (proc);
      proc = NULL;
    }
  else if (proc)
    sfi_glue_gc_add (proc, sfi_glue_proc_unref);
  return proc;
}

const gchar**
sfi_glue_list_proc_names (void)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  gchar **names;
  
  names = context->table.list_proc_names (context);
  if (!names)
    names = g_new0 (gchar*, 1);
  sfi_glue_gc_add (names, g_strfreev);
  return (const gchar**) names;
}

const gchar**
sfi_glue_list_method_names (const gchar *iface_name)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  gchar **names;
  
  g_return_val_if_fail (iface_name != NULL, NULL);
  
  names = context->table.list_method_names (context, iface_name);
  if (!names)
    names = g_new0 (gchar*, 1);
  sfi_glue_gc_add (names, g_strfreev);
  return (const gchar**) names;
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

const gchar**
sfi_glue_iface_children (const gchar *iface_name)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  gchar **names;
  
  g_return_val_if_fail (iface_name != NULL, NULL);
  
  names = context->table.iface_children (context, iface_name);
  if (!names)
    names = g_new0 (gchar*, 1);
  sfi_glue_gc_add (names, g_strfreev);
  return (const gchar**) names;
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
	  GValue *value = sfi_seq_append_empty (seq, collect_type);
	  G_VALUE_COLLECT (value, var_args, 0, &error);
	}
      if (error)
	{
	  sfi_diag ("%s: %s", G_STRLOC, error);
	  g_free (error);
	  sfi_seq_unref (seq);
	  return NULL;
	}
      arg_type = va_arg (var_args, guint);
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
    sfi_glue_gc_free_now (rvalue, sfi_value_free);
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
    sfi_glue_gc_free_now (rvalue, sfi_value_free);
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
    sfi_glue_gc_free_now (rvalue, sfi_value_free);
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
    sfi_glue_gc_free_now (rvalue, sfi_value_free);
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
    sfi_glue_gc_free_now (rvalue, sfi_value_free);
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
  if (rvalue)
    {
      if (SFI_VALUE_HOLDS_STRING (rvalue))
	retv = sfi_value_get_string (rvalue);
    }
  return retv ? retv : "";
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
  if (rvalue)
    {
      if (SFI_VALUE_HOLDS_CHOICE (rvalue))
	retv = sfi_value_get_choice (rvalue);
    }
  return retv ? retv : "";
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
    sfi_glue_gc_free_now (rvalue, sfi_value_free);
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
  if (rvalue)
    {
      if (SFI_VALUE_HOLDS_SEQ (rvalue))
	retv = sfi_value_get_seq (rvalue);
    }
  if (!retv)
    {
      retv = sfi_seq_new ();
      sfi_glue_gc_add (retv, sfi_seq_unref);
    }
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
  if (rvalue)
    {
      if (SFI_VALUE_HOLDS_REC (rvalue))
	retv = sfi_value_get_rec (rvalue);
    }
  return retv;
}

SfiFBlock*
sfi_glue_vcall_fblock (const gchar *proc_name,
		       guint8       first_arg_type,
		       ...)
{
  va_list var_args;
  GValue *rvalue;
  
  g_return_val_if_fail (proc_name != NULL, NULL);
  
  va_start (var_args, first_arg_type);
  rvalue = sfi_glue_call_valist (proc_name, first_arg_type, var_args);
  va_end (var_args);
  if (rvalue)
    {
      if (SFI_VALUE_HOLDS_FBLOCK (rvalue))
	return sfi_value_get_fblock (rvalue);
    }
  return NULL;
}

SfiBBlock*
sfi_glue_vcall_bblock (const gchar *proc_name,
		       guint8       first_arg_type,
		       ...)
{
  va_list var_args;
  GValue *rvalue;
  
  g_return_val_if_fail (proc_name != NULL, NULL);
  
  va_start (var_args, first_arg_type);
  rvalue = sfi_glue_call_valist (proc_name, first_arg_type, var_args);
  va_end (var_args);
  if (rvalue)
    {
      if (SFI_VALUE_HOLDS_BBLOCK (rvalue))
	return sfi_value_get_bblock (rvalue);
    }
  return NULL;
}


/* --- Glue utilities --- */
SfiGlueIFace*
sfi_glue_iface_new (const gchar *iface_name)
{
  SfiGlueIFace *iface;
  
  iface = g_new0 (SfiGlueIFace, 1);
  iface->type_name = g_strdup (iface_name ? iface_name : "<null>");
  iface->n_ifaces = 0;
  iface->ifaces = NULL;
  iface->n_props = 0;
  iface->props = NULL;
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
      g_free (iface);
    }
}

SfiGlueProc*
sfi_glue_proc_new (const gchar *proc_name)
{
  SfiGlueProc *proc;

  g_return_val_if_fail (proc_name != NULL, NULL);

  proc = g_new0 (SfiGlueProc, 1);
  proc->name = g_strdup (proc_name);
  proc->help = NULL;
  proc->authors = NULL;
  proc->license = NULL;
  proc->ret_param = NULL;
  proc->n_params = 0;
  proc->params = NULL;
  proc->ref_count = 1;
  return proc;
}

void
sfi_glue_proc_add_param (SfiGlueProc *proc,
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
sfi_glue_proc_add_ret_param (SfiGlueProc *proc,
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
      g_free (proc->name);
      g_free (proc->help);
      g_free (proc->authors);
      g_free (proc->license);
      g_free (proc);
    }
}


/* --- garbage collector --- */
typedef struct {
  gpointer data;
  void   (*free_func) (gpointer);
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

static void
glue_gc_entry_destroy (gpointer hvalue)
{
  GcEntry *entry = hvalue;
  entry->free_func (entry->data);
  g_free (entry);
}

static GHashTable*
glue_gc_hash_table_new (void)
{
  return g_hash_table_new_full (glue_gc_entry_hash, glue_gc_entry_equal, NULL, glue_gc_entry_destroy);
}

void
sfi_glue_gc_add (gpointer data,
		 gpointer free_func)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  GcEntry *entry;
  
  g_return_if_fail (free_func != NULL);
  g_return_if_fail (_sfi_glue_gc_test (data, g_free) == FALSE); /* can't catch ref counted objects */
  g_return_if_fail (_sfi_glue_gc_test (data, g_strfreev) == FALSE);
  g_return_if_fail (_sfi_glue_gc_test (data, sfi_value_free) == FALSE);
  
  entry = g_new (GcEntry, 1);
  entry->data = data;
  entry->free_func = free_func;
  g_hash_table_replace (context->gc_hash, entry, entry);
}

gboolean
_sfi_glue_gc_test (gpointer        data,
		   gpointer        free_func)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  GcEntry key;
  key.data = data;
  key.free_func = free_func;
  return g_hash_table_lookup (context->gc_hash, &key) != NULL;
}

void
sfi_glue_gc_remove (gpointer data,
		    gpointer free_func)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  GcEntry key, *gc_entry;
  
  g_return_if_fail (free_func != NULL);
  
  key.data = data;
  key.free_func = free_func;
  gc_entry = g_hash_table_lookup (context->gc_hash, &key);
  g_return_if_fail (gc_entry != NULL);
  g_hash_table_steal (context->gc_hash, gc_entry);
  g_free (gc_entry);
}

void
sfi_glue_gc_free_now (gpointer data,
		      gpointer free_func)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  GcEntry key, *gc_entry;
  
  g_return_if_fail (free_func != NULL);
  
  key.data = data;
  key.free_func = free_func;
  gc_entry = g_hash_table_lookup (context->gc_hash, &key);
  g_return_if_fail (gc_entry != NULL);
  g_hash_table_steal (context->gc_hash, gc_entry);
  g_free (gc_entry);
  key.free_func (key.data);
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
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);

  do
    {
      GSList *slist, *gclist = NULL;
      g_hash_table_foreach_steal (context->gc_hash, slist_entries, &gclist);
      for (slist = gclist; slist; slist = slist->next)
	{
	  GcEntry *entry = slist->data;
	  entry->free_func (entry->data);
	  g_free (entry);
	}
      g_slist_free (gclist);
    }
  while (g_hash_table_size (context->gc_hash));
}

/* vim:set ts=8 sts=2 sw=2: */
