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


/* --- variables --- */
gchar *bsw_log_domain_bsw = "BSW";


/* --- functions --- */
void
bsw_init (gint               *argc,
	  gchar             **argv[],
	  const BswLockFuncs *lock_funcs)
{
  BseLockFuncs lfuncs;

  if (lock_funcs)
    {
      g_return_if_fail (lock_funcs->lock != NULL);
      g_return_if_fail (lock_funcs->unlock != NULL);

      lfuncs.lock_data = lock_funcs->lock_data;
      lfuncs.lock = lock_funcs->lock;
      lfuncs.unlock = lock_funcs->unlock;
    }
  
  bse_init (argc, argv, lock_funcs ? &lfuncs : NULL);
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

gchar*
bsw_collector_get_string (GValue *value)	// FIXME
{
  return g_value_dup_string (value);
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
	  case BSE_TYPE_TIME: case BSE_TYPE_NOTE: case BSE_TYPE_DOTS:
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
