/* BSW - Bedevilled Sound Engine Wrapper
 * Copyright (C) 2002 Tim Janik
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
#include "bswsignal.h"

#include <bse/bseobject.h>
#include <bse/bsemain.h>
#include <bse/bswprivate.h>
#include <gsl/gslcommon.h>
#include <string.h>


/* --- structures --- */
typedef struct {
  guint     n_values;
  GValue   *values;
  GClosure *closure;
} Event;


/* --- variables --- */
static GslRing *event_list = NULL;


/* --- functions --- */
static void
event_free (Event *event)
{
  guint i;

  for (i = 0; i < event->n_values; i++)
    g_value_unset (event->values + i);
  g_free (event->values);
  g_closure_unref (event->closure);
  g_free (event);
}

static gboolean
handle_event_list (gpointer data)
{
  BSE_THREADS_ENTER ();

  while (event_list)
    {
      Event *event = event_list->data;

      event_list = gsl_ring_remove_node (event_list, event_list);
      g_closure_invoke (event->closure, NULL, event->n_values, event->values, NULL);
      event_free (event);
    }

  BSE_THREADS_LEAVE ();

  return FALSE;
}

static void
proxy_meta_marshal (GClosure     *closure,
		    GValue       *return_value,
		    guint         n_param_values,
		    const GValue *param_values,
		    gpointer      invocation_hint,
		    gpointer      marshal_data)
{
  /* BswProxy proxy = (BswProxy) closure->data; */
  GClosure *user_closure = marshal_data;
  Event *event = g_new0 (Event, 1);
  guint i;

  g_return_if_fail (return_value == NULL);
  g_return_if_fail (n_param_values >= 1);
  g_return_if_fail (g_type_is_a (G_VALUE_TYPE (param_values), BSE_TYPE_OBJECT));

  event->n_values = n_param_values;
  event->values = g_new0 (GValue, event->n_values);
  for (i = 0; i < event->n_values; i++)
    bsw_value_glue2bsw (param_values + i, event->values + i);
  event->closure = g_closure_ref (user_closure);
  if (G_CLOSURE_NEEDS_MARSHAL (event->closure))
    g_closure_set_marshal (event->closure, bse_proxy_marshaller_lookup (closure->marshal));

  if (!event_list)
    g_idle_add_full (BSE_NOTIFY_PRIORITY, handle_event_list, NULL, NULL);
  event_list = gsl_ring_append (event_list, event);
}

static GClosure*
proxy_closure_new (BswProxy  proxy,
		   GObject  *pobject,
		   GClosure *user_closure)
{
  GClosure *closure = g_closure_new_simple (sizeof (GClosure), (gpointer) proxy);

  g_closure_add_invalidate_notifier (closure,
				     g_closure_ref (user_closure),
				     (GClosureNotify) g_closure_unref);
  g_closure_sink (user_closure);
  g_closure_set_meta_marshal (closure, user_closure, proxy_meta_marshal);
  g_object_watch_closure (pobject, closure);
  
  return closure;
}

static void
bsw_proxy_connect_closure (BswProxy     proxy,
			   const gchar *signal,
			   GClosure    *closure)
{
  GObject *object = bse_object_from_id (proxy);
  GClosure *proxy_closure;

  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (object->ref_count > 0);
  g_return_if_fail (signal != NULL);
  g_return_if_fail (closure != NULL);

  proxy_closure = proxy_closure_new (proxy, object, closure);

  g_signal_connect_closure (object,
			    signal,
			    proxy_closure,
			    FALSE);
}

static void
disconnect_proxy_closure (GClosure *closure)
{
  GClosure *user_closure;
  GslRing *ring;

  g_return_if_fail (closure->meta_marshal == 1);

  user_closure = closure->notifiers[0].data;
  g_closure_invalidate (closure);
  ring = event_list;
  while (ring)
    {
      Event *event = ring->data;
      GslRing *old = ring;

      ring = gsl_ring_walk (event_list, ring);
      if (event->closure == user_closure)
	{
	  event_list = gsl_ring_remove_node (event_list, old);
	  event_free (event);
	}
    }
}

static GSList*
g_object_list_watched_closures (GObject *object)
{
#if !GLIB_CHECK_VERSION (2, 0, 0)
#  error at least GLib 2.0.0 is required for this version of BEAST
#endif
  typedef struct {
    GObject  *object;
    guint     n_closures;
    GClosure *closures[1]; /* flexible array */
  } CArray;
  CArray *carray;

  g_return_val_if_fail (G_IS_OBJECT (object), NULL);

  carray = g_object_get_data (object, "GObject-closure-array");
  if (carray)
    {
      GSList *slist = NULL;
      guint i;

      for (i = 0; i < carray->n_closures; i++)
	slist = g_slist_prepend (slist, carray->closures[i]);
      return slist;
    }
  return NULL;
}

#if 0
static void
bsw_proxy_disconnect_closure (BswProxy  proxy,
			      GClosure *user_closure)
{
  GSList *node, *slist;

  g_return_if_fail (bse_object_from_id (proxy) != NULL);
  g_return_if_fail (user_closure != NULL);

  slist = g_object_list_watched_closures (bse_object_from_id (proxy));
  for (node = slist; node; node = node->next)
    {
      GClosure *proxy_closure = node->data;

      if (proxy_closure->meta_marshal &&
	  proxy_closure->notifiers[0].notify == (GClosureNotify) proxy_meta_marshal &&
	  proxy_closure->notifiers[0].data == user_closure)
	{
	  disconnect_proxy_closure (proxy_closure);
	  g_slist_free (slist);
	  return;
	}
    }
  g_slist_free (slist);

  g_warning ("%s: unable to find user_closure %p on proxy %lu", G_STRLOC, user_closure, proxy);
}
#endif

void
bsw_proxy_connect (BswProxy     proxy,
		   const gchar *signal,
		   ...)
{
  GObject *object = bse_object_from_id (proxy);
  va_list var_args;

  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (object->ref_count > 0);

  va_start (var_args, signal);
  while (signal)
    {
      gpointer callback = va_arg (var_args, gpointer);
      gpointer data = va_arg (var_args, gpointer);
      
      if (strncmp (signal, "signal::", 8) == 0)
	bsw_proxy_connect_closure (proxy, signal + 8,
				   g_cclosure_new (callback, data, NULL));
      else if (strncmp (signal, "object_signal::", 15) == 0 ||
	       strncmp (signal, "object-signal::", 15) == 0)
	bsw_proxy_connect_closure (proxy, signal + 15,
				   g_cclosure_new_object (callback, data));
      else if (strncmp (signal, "swapped_signal::", 16) == 0 ||
	       strncmp (signal, "swapped-signal::", 16) == 0)
	bsw_proxy_connect_closure (proxy, signal + 16,
				   g_cclosure_new_swap (callback, data, NULL));
      else if (strncmp (signal, "swapped_object_signal::", 23) == 0 ||
	       strncmp (signal, "swapped-object-signal::", 23) == 0)
	bsw_proxy_connect_closure (proxy, signal + 23,
				   g_cclosure_new_object_swap (callback, data));
      else
	{
	  g_warning ("%s: invalid signal spec \"%s\"", G_STRLOC, signal);
	  break;
	}
      signal = va_arg (var_args, gchar*);
    }
  va_end (var_args);
}

static GSList*
bsw_proxy_list_proxy_closures (GObject  *object,
			       gpointer  func,
			       gpointer  data)
{
  GSList *node, *slist, *rlist = NULL;

  slist = g_object_list_watched_closures (object);
  for (node = slist; node; node = node->next)
    {
      GClosure *proxy_closure = node->data;

      if (proxy_closure->meta_marshal &&
	  proxy_closure->notifiers[0].notify == (GClosureNotify) proxy_meta_marshal)
	{
	  GClosure *user_closure = proxy_closure->notifiers[0].data;

	  if (user_closure->data == data &&
	      (user_closure->marshal == NULL || user_closure->marshal == bse_proxy_marshaller_lookup (proxy_closure->marshal)) &&
	      ((GCClosure*) user_closure)->callback == func)
	    rlist = g_slist_prepend (rlist, proxy_closure);
	}
    }
  g_slist_free (slist);
  return rlist;
}

void
bsw_proxy_disconnect (BswProxy     proxy,
		      const gchar *signal,
		      ...)
{
  GObject *object = bse_object_from_id (proxy);
  va_list var_args;

  if (!object)
    return;	/* we're lax here... */

  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (object->ref_count > 0);

  va_start (var_args, signal);
  while (signal)
    {
      gpointer callback = va_arg (var_args, gpointer);
      gpointer data = va_arg (var_args, gpointer);
      GSList *node, *slist = NULL;

      if (strncmp (signal, "any_signal::", 12) == 0)
	{
	  guint sid = 0, detail = 0, found_one = FALSE;
	  
	  signal += 12;
	  if (!g_signal_parse_name (signal, G_OBJECT_TYPE (object), &sid, &detail, FALSE))
	    g_warning ("%s: invalid signal name \"%s\"", G_STRLOC, signal);
	  else
	    slist = bsw_proxy_list_proxy_closures (object, callback, data);
	  for (node = slist; node; node = node->next)
	    if (g_signal_handler_find (object,
				       G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_CLOSURE | (detail ? G_SIGNAL_MATCH_DETAIL : 0),
				       sid, detail,
				       node->data,
				       NULL, NULL))
	      {
		disconnect_proxy_closure (node->data);
		found_one = TRUE;
	      }
	  g_slist_free (slist);
	  if (!found_one)
	    slist = NULL;
	}
      else if (strcmp (signal, "any_signal") == 0)
	{
	  slist = bsw_proxy_list_proxy_closures (object, callback, data);
	  for (node = slist; node; node = node->next)
	    disconnect_proxy_closure (node->data);
	  g_slist_free (slist);
	}
      else
	{
	  g_warning ("%s: invalid signal spec \"%s\"", G_STRLOC, signal);
	  break;
	}

      if (!slist)
	g_warning ("%s: signal handler %p(%p) is not connected", G_STRLOC, callback, data);
      signal = va_arg (var_args, gchar*);
    }
  va_end (var_args);
}
