/* GSL - Generic Sound Layer
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
  gchar      *signal;
  GslGlueSeq *args;
  gboolean    destroyed;
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
glue_signal_lookup (GslGlueContext *context,
		    gulong          proxy,
		    const gchar    *signal)
{
  GlueSignal key;
  key.proxy = proxy;
  key.signal = (gchar*) signal;

  return g_hash_table_lookup (context->sighash, &key);
}

static void
gsl_glue_signals_dispatch (GslGlueContext *context)
{
  static gboolean in_glue_signals_dispatch = FALSE;

  g_return_if_fail (in_glue_signals_dispatch == FALSE);

  in_glue_signals_dispatch = TRUE;
  
  while (context->sigqueue)
    {
      GslRing *node = context->sigqueue;
      GlueEvent *sevent = node->data;
      GlueSignal *sig;
      gulong proxy;
      GHook *hook;

      g_return_if_fail (sevent->args->elements[0].glue_type == GSL_GLUE_TYPE_PROXY);

      context->sigqueue = gsl_ring_remove_node (context->sigqueue, node);
      proxy = sevent->args->elements[0].value.v_proxy;
      sig = glue_signal_lookup (context, proxy, sevent->signal);

      hook = g_hook_first_valid (&sig->hooks, FALSE);
      while (hook)
	{
	  GslGlueSignalFunc func = (GslGlueSignalFunc) hook->func;
	  gboolean was_in_call = G_HOOK_IN_CALL (hook);

	  hook->flags |= G_HOOK_FLAG_IN_CALL;
	  func (hook->data, sevent->signal, sevent->args);
	  if (!was_in_call)
	    hook->flags &= ~G_HOOK_FLAG_IN_CALL;
	  hook = g_hook_next_valid (&sig->hooks, hook, FALSE);
	}
      
      gsl_glue_free_seq (sevent->args);
      g_free (sevent->signal);
      g_free (sevent);
    }

  while (context->gc_signals)
    {
      GslRing *node = context->gc_signals;
      GlueSignal *sig = node->data;
      GHook *hook;

      context->gc_signals = gsl_ring_remove_node (context->gc_signals, node);

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
gsl_glue_signal_connect (const gchar      *signal,
			 gulong            proxy,
			 GslGlueSignalFunc func,
			 gpointer          sig_data,
			 GDestroyNotify    sig_data_free)
{
  GslGlueContext *context = gsl_glue_fetch_context (G_STRLOC);
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
gsl_glue_signal_disconnect (const gchar *signal,
			    gulong       proxy,
			    gulong       connection_id)
{
  GslGlueContext *context = gsl_glue_fetch_context (G_STRLOC);
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
      else if (!gsl_ring_find (context->gc_signals, sig))
	context->gc_signals = gsl_ring_prepend (context->gc_signals, sig);
    }
  if (!destroyed)
    g_message ("no such signal (id=%lu) to disconnect", connection_id);
}

void
gsl_glue_enqueue_signal_event (const gchar *signal,
			       GslGlueSeq  *args,
			       gboolean     disabled)
{
  GslGlueContext *context = gsl_glue_fetch_context (G_STRLOC);
  GlueEvent *sevent;

  g_return_if_fail (signal != NULL);
  g_return_if_fail (args != NULL);
  g_return_if_fail (args->n_elements > 0);
  g_return_if_fail (args->elements[0].glue_type == GSL_GLUE_TYPE_PROXY);

  sevent = g_new0 (GlueEvent, 1);
  sevent->signal = g_strdup (signal);
  sevent->args = gsl_glue_seqdup (args);
  sevent->destroyed = disabled != FALSE;
  context->sigqueue = gsl_ring_append (context->sigqueue, sevent);
}
