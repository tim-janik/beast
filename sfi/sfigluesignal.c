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


typedef struct {
  gulong    proxy;
  gchar    *signal;
  GHookList hooks;
} GlueSignal;

typedef struct {
  gchar    *signal;
  SfiSeq   *args;
  gboolean  destroyed;
} GlueEvent;


/* --- prototypes --- */
static guint    glue_signal_hash        (gconstpointer  key);
static gboolean glue_signal_equal       (gconstpointer  key1,
					 gconstpointer  key2);


/* --- functions --- */
static guint
glue_signal_hash (gconstpointer key)
{
  const GlueSignal *s = key;
  guint h = s->proxy;

  h ^= g_str_hash (s->signal);

  return h;
}

static gboolean
glue_signal_equal (gconstpointer key1,
		   gconstpointer key2)
{
  const GlueSignal *s1 = key1;
  const GlueSignal *s2 = key2;

  if (s1->proxy != s2->proxy)
    return FALSE;
  return strcmp (s1->signal, s2->signal) == 0;
}

static GlueSignal*
glue_signal_lookup (SfiGlueContext *context,
		    SfiProxy        proxy,
		    const gchar    *signal)
{
  GlueSignal key;
  key.proxy = proxy;
  key.signal = (gchar*) signal;

  return g_hash_table_lookup (context->sighash, &key);
}

static void
sfi_glue_signals_dispatch (SfiGlueContext *context)
{
  static gboolean in_glue_signals_dispatch = FALSE;

  g_return_if_fail (in_glue_signals_dispatch == FALSE);

  in_glue_signals_dispatch = TRUE;
  
  while (context->sigqueue)
    {
      SfiRing *node = context->sigqueue;
      GlueEvent *sevent = node->data;
      GlueSignal *sig;
      SfiProxy proxy;
      GHook *hook;

      g_return_if_fail (SFI_VALUE_HOLDS_PROXY (sevent->args->elements + 0));

      context->sigqueue = sfi_ring_remove_node (context->sigqueue, node);
      proxy = sfi_value_get_proxy (sevent->args->elements + 0);
      sig = glue_signal_lookup (context, proxy, sevent->signal);

      hook = g_hook_first_valid (&sig->hooks, FALSE);
      while (hook)
	{
	  SfiGlueSignalFunc func = (SfiGlueSignalFunc) hook->func;
	  gboolean was_in_call = G_HOOK_IN_CALL (hook);

	  hook->flags |= G_HOOK_FLAG_IN_CALL;
	  func (hook->data, sevent->signal, sevent->args);
	  if (!was_in_call)
	    hook->flags &= ~G_HOOK_FLAG_IN_CALL;
	  hook = g_hook_next_valid (&sig->hooks, hook, FALSE);
	}
      
      sfi_seq_unref (sevent->args);
      g_free (sevent->signal);
      g_free (sevent);
    }

  while (context->gc_signals)
    {
      SfiRing *node = context->gc_signals;
      GlueSignal *sig = node->data;
      GHook *hook;

      context->gc_signals = sfi_ring_remove_node (context->gc_signals, node);

      sig = g_hash_table_lookup (context->sighash, sig);
      hook = g_hook_first_valid (&sig->hooks, TRUE);
      if (hook)
	g_hook_unref (&sig->hooks, hook);
      else
	{
	  g_hook_list_clear (&sig->hooks);
	  g_hash_table_remove (context->sighash, sig);
	  context->table.signal_connection (context, sig->signal, sig->proxy, FALSE);
	  g_free (sig->signal);
	  g_free (sig);
	}
    }

  in_glue_signals_dispatch = FALSE;
}

gulong
sfi_glue_signal_connect (const gchar      *signal,
			 SfiProxy          proxy,
			 SfiGlueSignalFunc func,
			 gpointer          sig_data,
			 GDestroyNotify    sig_data_free)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  GlueSignal *sig;
  GHook *hook;
  
  g_return_val_if_fail (proxy > 0, 0);
  g_return_val_if_fail (signal != NULL, 0);
  g_return_val_if_fail (func != NULL, 0);

  sig = glue_signal_lookup (context, proxy, signal);
  if (!sig)
    {
      if (context->table.signal_connection (context, signal, proxy, TRUE))
	{
	  sig = g_new0 (GlueSignal, 1);
	  g_hook_list_init (&sig->hooks, sizeof (GHook));
	  sig->proxy = proxy;
	  sig->signal = g_strdup (signal);
	  g_hash_table_insert (context->sighash, sig, sig);
	}
      else
	{
	  if (sig_data_free)
	    sig_data_free (sig_data);
	  return 0;
	}
    }
  hook = g_hook_alloc (&sig->hooks);
  hook->data = sig_data;
  hook->destroy = sig_data_free;
  hook->func = func;
  g_hook_append (&sig->hooks, hook);

  return hook->hook_id;
}

void
sfi_glue_signal_disconnect (const gchar *signal,
			    SfiProxy     proxy,
			    gulong       connection_id)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  GlueSignal *sig;
  GHook *hook;
  gboolean destroyed = FALSE;

  g_return_if_fail (proxy > 0);
  g_return_if_fail (signal != NULL);
  g_return_if_fail (connection_id > 0);

  sig = glue_signal_lookup (context, proxy, signal);
  if (sig)
    {
      destroyed = g_hook_destroy (&sig->hooks, connection_id);
      hook = g_hook_first_valid (&sig->hooks, TRUE);
      if (hook)
	g_hook_unref (&sig->hooks, hook);
      else if (!sfi_ring_find (context->gc_signals, sig))
	context->gc_signals = sfi_ring_prepend (context->gc_signals, sig);
    }
  if (!destroyed)
    g_message ("no such signal (id=%lu) to disconnect", connection_id);
}

void
sfi_glue_enqueue_signal_event (const gchar *signal,
			       SfiSeq      *args,
			       gboolean     disabled)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  GlueEvent *sevent;

  g_return_if_fail (signal != NULL);
  g_return_if_fail (args != NULL);
  g_return_if_fail (args->n_elements > 0);
  g_return_if_fail (SFI_VALUE_HOLDS_PROXY (args->elements + 0));

  sevent = g_new0 (GlueEvent, 1);
  sevent->signal = g_strdup (signal);
  sevent->args = sfi_seq_copy_deep (args);
  sfi_glue_gc_remove (sevent->args, sfi_seq_unref);
  sevent->destroyed = disabled != FALSE;
  context->sigqueue = sfi_ring_append (context->sigqueue, sevent);
}

/* vim:set ts=8 sts=2 sw=2: */
