/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998-1999, 2000-2002 Tim Janik
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
#include        "bseprocedure.h"

#include	<gobject/gvaluecollector.h>
#include	"bseobject.h"
#include	"bseserver.h"
#include	"bsestorage.h"
#include	"bseexports.h"
#include	<string.h>


/* --- macros --- */
#define parse_or_return         bse_storage_scanner_parse_or_return
#define peek_or_return          bse_storage_scanner_peek_or_return


/* --- prototypes --- */
extern void	bse_type_register_procedure_info  (GTypeInfo		    *info);
static void     bse_procedure_base_init		  (BseProcedureClass	    *proc);
static void     bse_procedure_base_finalize	  (BseProcedureClass	    *proc);
static void     bse_procedure_init		  (BseProcedureClass	    *proc,
						   const BseExportProcedure *pspec);


/* --- functions --- */
extern void
bse_type_register_procedure_info (GTypeInfo *info)
{
  static const GTypeInfo proc_info = {
    sizeof (BseProcedureClass),

    (GBaseInitFunc) bse_procedure_base_init,
    (GBaseFinalizeFunc) bse_procedure_base_finalize,
    (GClassInitFunc) NULL,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,

    /* non classed type stuff */
    0, 0, NULL,
  };

  *info = proc_info;
}

static void
bse_procedure_base_init (BseProcedureClass *proc)
{
  proc->name = NULL;
  proc->blurb = NULL;
  proc->private_id = 0;
  proc->help = NULL;
  proc->author = NULL;
  proc->copyright = NULL;
  proc->date = NULL;
  proc->n_in_pspecs = 0;
  proc->in_pspecs = NULL;
  proc->n_out_pspecs = 0;
  proc->out_pspecs = NULL;
  proc->execute = NULL;
}

static void
bse_procedure_base_finalize (BseProcedureClass *proc)
{
  guint i;

  proc->name = NULL;
  proc->blurb = NULL;
  proc->help = NULL;
  proc->author = NULL;
  proc->copyright = NULL;
  proc->date = NULL;

  for (i = 0; i < proc->n_in_pspecs; i++)
    g_param_spec_unref (proc->in_pspecs[i]);
  g_free (proc->in_pspecs);
  for (i = 0; i < proc->n_out_pspecs; i++)
    g_param_spec_unref (proc->out_pspecs[i]);
  g_free (proc->out_pspecs);

  proc->execute = NULL;
}

static void
bse_procedure_init (BseProcedureClass        *proc,
		    const BseExportProcedure *pspec)
{
  GParamSpec *in_pspecs[BSE_PROCEDURE_MAX_IN_PARAMS + 8];
  GParamSpec *out_pspecs[BSE_PROCEDURE_MAX_OUT_PARAMS + 8];
  guint i;
  gchar *const_name, *const_blurb;

  memset (in_pspecs, 0, sizeof (in_pspecs));
  memset (out_pspecs, 0, sizeof (out_pspecs));

  proc->name = g_type_name (BSE_PROCEDURE_TYPE (proc));
  proc->blurb = bse_type_blurb (BSE_PROCEDURE_TYPE (proc));
  proc->private_id = pspec->private_id;

  /* init procedure class from plugin,
   * paranoia check certain class members
   */
  const_name = proc->name;
  const_blurb = proc->blurb;
  pspec->init (proc, in_pspecs, out_pspecs);
  if (proc->name != const_name)
    {
      proc->name = const_name;
      g_warning ("procedure \"%s\" redefines procedure name", proc->name);
    }
  if (proc->blurb != const_blurb)
    {
      proc->blurb = const_blurb;
      g_warning ("procedure \"%s\" redefines procedure blurb", proc->name);
    }
  if (proc->n_in_pspecs || proc->in_pspecs ||
      proc->n_out_pspecs || proc->out_pspecs ||
      proc->execute)
    {
      proc->n_in_pspecs = 0;
      proc->in_pspecs = NULL;
      proc->n_out_pspecs = 0;
      proc->out_pspecs = NULL;
      proc->execute = NULL;
      g_warning ("procedure \"%s\" messes with reserved class members", proc->name);
    }

  /* check input parameters and setup specifications
   */
  for (i = 0; i < BSE_PROCEDURE_MAX_IN_PARAMS; i++)
    if (in_pspecs[i])
      {
	if ((in_pspecs[i]->flags & BSE_PARAM_READWRITE) != BSE_PARAM_READWRITE)
	  g_warning ("procedure \"%s\": input parameter \"%s\" has invalid flags",
		     proc->name,
		     in_pspecs[i]->name);
      }
    else
      break;
  if (i == BSE_PROCEDURE_MAX_IN_PARAMS && in_pspecs[i])
    g_warning ("procedure \"%s\" exceeds maximum number of input parameters (%u)",
	       proc->name, BSE_PROCEDURE_MAX_IN_PARAMS);
  proc->n_in_pspecs = i;
  proc->in_pspecs = g_new (GParamSpec*, proc->n_in_pspecs + 1);
  memcpy (proc->in_pspecs, in_pspecs, sizeof (in_pspecs[0]) * proc->n_in_pspecs);
  proc->in_pspecs[proc->n_in_pspecs] = NULL;

  /* check output parameters and setup specifications
   */
  for (i = 0; i < BSE_PROCEDURE_MAX_OUT_PARAMS; i++)
    if (out_pspecs[i])
      {
        if ((out_pspecs[i]->flags & BSE_PARAM_READWRITE) != BSE_PARAM_READWRITE)
	  g_warning ("procedure \"%s\": output parameter \"%s\" has invalid flags",
		     proc->name,
		     out_pspecs[i]->name);
      }
    else
      break;
  if (i == BSE_PROCEDURE_MAX_OUT_PARAMS && out_pspecs[i])
    g_warning ("procedure \"%s\" exceeds maximum number of output parameters (%u)",
	       proc->name, BSE_PROCEDURE_MAX_OUT_PARAMS);
  proc->n_out_pspecs = i;
  proc->out_pspecs = g_new (GParamSpec*, proc->n_out_pspecs + 1);
  memcpy (proc->out_pspecs, out_pspecs, sizeof (out_pspecs[0]) * proc->n_out_pspecs);
  proc->out_pspecs[proc->n_out_pspecs] = NULL;
  
  proc->execute = pspec->exec;
}

void
bse_procedure_complete_info (const BseExportSpec *spec,
			     GTypeInfo         *info)
{
  const BseExportProcedure *pspec = &spec->s_proc;

  info->class_size = sizeof (BseProcedureClass);
  info->class_init = (GClassInitFunc) bse_procedure_init;
  info->class_finalize = (GClassFinalizeFunc) pspec->unload;
  info->class_data = pspec;
}

gboolean
bse_procedure_status (BseProcedureClass *proc,
		      gfloat             progress)
{
  g_return_val_if_fail (BSE_IS_PROCEDURE_CLASS (proc), TRUE);

  return FALSE;
}

const gchar*
bse_procedure_type_register (const gchar *name,
			     const gchar *blurb,
			     BsePlugin   *plugin,
			     GType       *ret_type)
{
  GType   type, base_type = 0;
  gchar *p;

  g_return_val_if_fail (ret_type != NULL, bse_error_blurb (BSE_ERROR_INTERNAL));
  *ret_type = 0;
  g_return_val_if_fail (name != NULL, bse_error_blurb (BSE_ERROR_INTERNAL));
  g_return_val_if_fail (plugin != NULL, bse_error_blurb (BSE_ERROR_INTERNAL));

  type = g_type_from_name (name);
  if (type)
    return "Procedure already registered";

  p = strchr (name, '+');
  if (p)
    {
      /* enforce <OBJECT>+<METHOD> syntax */
      if (!p[1])
	return "Procedure name invalid";

      p = g_strndup (name, p - name);
      base_type = g_type_from_name (p);
      g_free (p);
      if (!g_type_is_a (base_type, BSE_TYPE_OBJECT))
	return "Procedure base type invalid";
    }
  
  type = bse_type_register_dynamic (BSE_TYPE_PROCEDURE,
				    name,
				    blurb,
				    plugin);

  *ret_type = type;

  return NULL;
}

GType
bse_procedure_lookup (const gchar *proc_name)
{
  GType type;

  g_return_val_if_fail (proc_name != NULL, 0);

  type = g_type_from_name (proc_name);
  return BSE_TYPE_IS_PROCEDURE (type) ? type : 0;
}

static BseErrorType
call_proc (BseProcedureClass  *proc,
	   GValue             *ivalues,
	   GValue             *ovalues,
	   BseProcedureMarshal marshal,
	   gpointer            marshal_data)
{
  guint i, bail_out = FALSE;
  BseErrorType error;

  for (i = 0; i < proc->n_in_pspecs; i++)
    {
      GParamSpec *pspec = proc->in_pspecs[i];

      if (g_param_value_validate (pspec, ivalues + i) && !(pspec->flags & G_PARAM_LAX_VALIDATION))
	{
	  g_warning ("%s: input arg `%s' contains invalid value",
		     proc->name,
		     pspec->name);
	  bail_out = TRUE;
	}
    }

  if (bail_out)
    error = BSE_ERROR_PROC_PARAM_INVAL;
  else
    {
      if (marshal)
	error = marshal (marshal_data, proc, ivalues, ovalues);
      else
	error = proc->execute (proc, ivalues, ovalues);
      bse_server_script_status (bse_server_get (), BSE_SCRIPT_STATUS_PROC_END, proc->name, error ? 0 : 1, error);
    }
  
  for (i = 0; i < proc->n_in_pspecs; i++)
    {
      GParamSpec *pspec = proc->in_pspecs[i];
      
      if (g_param_value_validate (pspec, ivalues + i) && !(pspec->flags & G_PARAM_LAX_VALIDATION))
	g_warning ("%s: internal procedure error: output arg `%s' had invalid value",
		   proc->name,
		   pspec->name);
    }
  
  return error;
}

BseErrorType
bse_procedure_marshal (GType               proc_type,
		       const GValue       *ivalues,
		       GValue             *ovalues,
		       BseProcedureMarshal marshal,
		       gpointer            marshal_data)
{
  BseProcedureClass *proc;
  GValue tmp_ivalues[BSE_PROCEDURE_MAX_IN_PARAMS];
  GValue tmp_ovalues[BSE_PROCEDURE_MAX_OUT_PARAMS];
  guint i, bail_out = FALSE;
  BseErrorType error;

  g_return_val_if_fail (BSE_TYPE_IS_PROCEDURE (proc_type), BSE_ERROR_INTERNAL);

  proc = g_type_class_ref (proc_type);
  for (i = 0; i < proc->n_in_pspecs; i++)
    {
      GParamSpec *pspec = proc->in_pspecs[i];

      tmp_ivalues[i].g_type = 0;
      g_value_init (tmp_ivalues + i, G_PARAM_SPEC_VALUE_TYPE (pspec));
      if (!g_value_transform (ivalues + i, tmp_ivalues + i))
	{
	  g_warning ("%s: input arg `%s' has invalid type `%s' (expected `%s')",
		     proc->name,
		     pspec->name,
		     G_VALUE_TYPE_NAME (ivalues + i),
		     g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
	  bail_out = TRUE;
	}
    }
  for (i = 0; i < proc->n_out_pspecs; i++)
    {
      tmp_ovalues[i].g_type = 0;
      g_value_init (tmp_ovalues + i, G_PARAM_SPEC_VALUE_TYPE (proc->out_pspecs[i]));
    }

  if (bail_out)
    error = BSE_ERROR_PROC_PARAM_INVAL;
  else
    error = call_proc (proc, tmp_ivalues, tmp_ovalues, marshal, marshal_data);

  for (i = 0; i < proc->n_in_pspecs; i++)
    g_value_unset (tmp_ivalues + i);
  for (i = 0; i < proc->n_out_pspecs; i++)
    {
      GParamSpec *pspec = proc->out_pspecs[i];

      if (!g_value_transform (tmp_ovalues + i, ovalues + i))
	g_warning ("%s: output arg `%s' of type `%s' cannot be converted into `%s'",
		   proc->name,
		   pspec->name,
		   g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
		   G_VALUE_TYPE_NAME (ovalues + i));
      g_value_unset (tmp_ovalues + i);
    }
  g_type_class_unref (proc);

  return error;
}

BseErrorType
bse_procedure_marshal_valist (GType               proc_type,
			      const GValue       *first_value,
			      BseProcedureMarshal marshal,
			      gpointer            marshal_data,
			      gboolean		  skip_ovalues,
			      va_list             var_args)
{
  BseProcedureClass *proc;
  GValue tmp_ivalues[BSE_PROCEDURE_MAX_IN_PARAMS];
  GValue tmp_ovalues[BSE_PROCEDURE_MAX_OUT_PARAMS];
  guint i, bail_out = FALSE;
  BseErrorType error;

  g_return_val_if_fail (BSE_TYPE_IS_PROCEDURE (proc_type), BSE_ERROR_INTERNAL);

  proc = g_type_class_ref (proc_type);
  if (first_value)
    {
      if (proc->n_in_pspecs < 1)
	g_warning ("%s: input arg supplied for procedure taking `void'",
		   proc->name);
      else
	{
	  GParamSpec *pspec = proc->in_pspecs[0];

	  tmp_ivalues[0].g_type = 0;
	  g_value_init (tmp_ivalues + 0, G_PARAM_SPEC_VALUE_TYPE (pspec));
	  if (!g_value_transform (first_value, tmp_ivalues + 0))
	    {
	      g_warning ("%s: input arg `%s' has invalid type `%s' (expected `%s')",
			 proc->name,
			 pspec->name,
			 G_VALUE_TYPE_NAME (first_value),
			 g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
	      bail_out = TRUE;
	    }
	}
    }
  for (i = first_value ? 1 : 0; i < proc->n_in_pspecs; i++)
    {
      GParamSpec *pspec = proc->in_pspecs[i];
      gchar *error_msg = NULL;

      tmp_ivalues[i].g_type = 0;
      g_value_init (tmp_ivalues + i, G_PARAM_SPEC_VALUE_TYPE (pspec));
      if (!bail_out)
	G_VALUE_COLLECT (tmp_ivalues + i, var_args, 0, &error_msg);
      if (error_msg)
	{
	  g_warning ("%s: failed to collect arg `%s' of type `%s': %s",
		     proc->name,
		     pspec->name,
		     G_VALUE_TYPE_NAME (tmp_ivalues + i),
		     error_msg);
	  g_free (error_msg);
	  bail_out = TRUE;
	}
    }
  for (i = 0; i < proc->n_out_pspecs; i++)
    {
      tmp_ovalues[i].g_type = 0;
      g_value_init (tmp_ovalues + i, G_PARAM_SPEC_VALUE_TYPE (proc->out_pspecs[i]));
    }

  if (bail_out)
    error = BSE_ERROR_PROC_PARAM_INVAL;
  else
    error = call_proc (proc, tmp_ivalues, tmp_ovalues, marshal, marshal_data);

  for (i = 0; i < proc->n_in_pspecs; i++)
    g_value_unset (tmp_ivalues + i);
  for (i = 0; i < proc->n_out_pspecs; i++)
    {
      GParamSpec *pspec = proc->out_pspecs[i];
      gchar *error_msg = NULL;

      if (!skip_ovalues)
	G_VALUE_LCOPY (tmp_ovalues + i, var_args, 0, &error_msg);
      if (error_msg)
	{
	  g_warning ("%s: failed to return arg `%s' of type `%s': %s",
		     proc->name,
		     pspec->name,
		     G_VALUE_TYPE_NAME (tmp_ovalues + i),
		     error_msg);
	  g_free (error_msg);
	  skip_ovalues = TRUE;
	}
      g_value_unset (tmp_ovalues + i);
    }
  g_type_class_unref (proc);

  return error;
}

BseErrorType
bse_procedure_exec (const gchar *proc_name,
		    ...)
{
  GType proc_type;

  g_return_val_if_fail (proc_name != NULL, BSE_ERROR_INTERNAL);

  proc_type = bse_procedure_lookup (proc_name);
  if (!proc_type)
    {
      g_warning ("%s: no such procedure", proc_name);
      return BSE_ERROR_NOT_FOUND;
    }
  else
    {
      BseErrorType error;
      va_list var_args;

      va_start (var_args, proc_name);
      error = bse_procedure_marshal_valist (proc_type, NULL, NULL, NULL, FALSE, var_args);
      va_end (var_args);
      return error;
    }
}

BseErrorType
bse_procedure_exec_void (const gchar *proc_name,
			 ...)
{
  GType proc_type;

  g_return_val_if_fail (proc_name != NULL, BSE_ERROR_INTERNAL);

  proc_type = bse_procedure_lookup (proc_name);
  if (!proc_type)
    {
      g_warning ("%s: no such procedure", proc_name);
      return BSE_ERROR_NOT_FOUND;
    }
  else
    {
      BseErrorType error;
      va_list var_args;

      va_start (var_args, proc_name);
      error = bse_procedure_marshal_valist (proc_type, NULL, NULL, NULL, TRUE, var_args);
      va_end (var_args);
      return error;
    }
}

BseErrorType
bse_procedure_store (const gchar *proc_name,
		     BseStorage  *storage,
		     ...)
{
  GType proc_type;

  g_return_val_if_fail (proc_name != NULL, BSE_ERROR_INTERNAL);

  proc_type = bse_procedure_lookup (proc_name);
  if (!proc_type)
    {
      g_warning ("%s: no such procedure", proc_name);
      return BSE_ERROR_NOT_FOUND;
    }
  else
    {
      BseErrorType error;
      va_list var_args;

      va_start (var_args, storage);
      error = bse_procedure_marshal_valist (proc_type, NULL,
					    bse_storage_store_procedure, storage,
					    TRUE, var_args);
      va_end (var_args);
      return error;
    }
}

BseErrorType
bse_procedure_execvl (BseProcedureClass *proc,
		      GSList            *in_value_list,
		      GSList            *out_value_list)
{
  GValue tmp_ivalues[BSE_PROCEDURE_MAX_IN_PARAMS];
  GValue tmp_ovalues[BSE_PROCEDURE_MAX_OUT_PARAMS];
  BseErrorType error;
  GSList *slist;
  guint i;

  /* FIXME: bad, bad compat: bse_procedure_execvl() */

  for (i = 0, slist = in_value_list; slist && i < proc->n_in_pspecs; i++, slist = slist->next)
    memcpy (tmp_ivalues + i, slist->data, sizeof (tmp_ivalues[0]));
  for (i = 0, slist = out_value_list; slist && i < proc->n_out_pspecs; i++, slist = slist->next)
    memcpy (tmp_ovalues + i, slist->data, sizeof (tmp_ovalues[0]));
  error = bse_procedure_marshal (BSE_PROCEDURE_TYPE (proc), tmp_ivalues, tmp_ovalues, NULL, NULL);
  for (i = 0, slist = out_value_list; slist && i < proc->n_out_pspecs; i++, slist = slist->next)
    memcpy (slist->data, tmp_ovalues + i, sizeof (tmp_ivalues[0]));
  return error;
}

static GTokenType
bse_procedure_eval_storage (BseStorage   *storage,
			    BseErrorType *error_p,
			    GValue       *retval)
{
  GValue ivalues[BSE_PROCEDURE_MAX_IN_PARAMS];
  BseProcedureClass *proc;
  GTokenType token = G_TOKEN_NONE;
  GScanner *scanner = storage->scanner;
  GType proc_type;
  guint i;

  parse_or_return (scanner, '(');
  parse_or_return (scanner, G_TOKEN_IDENTIFIER);
  if (strcmp ("bse-proc-call", scanner->value.v_identifier) != 0)
    return G_TOKEN_IDENTIFIER;

  /* fetch and check procedure */
  parse_or_return (scanner, G_TOKEN_STRING);
  proc_type = bse_procedure_lookup (scanner->value.v_string);
  if (!proc_type)
    {
      bse_storage_error (storage, "proc-eval: no such procedure \"%s\"", scanner->value.v_identifier);
      return G_TOKEN_STRING;
    }
  proc = g_type_class_ref (proc_type);
  if (proc->n_out_pspecs > 1)
    {
      bse_storage_error (storage, "proc-eval: procedure \"%s\" has more than 1 (%u) output paremeters",
			 proc->name, proc->n_out_pspecs);
      g_type_class_unref (proc);
      return G_TOKEN_STRING;
    }

  /* parse input values */
  for (i = 0; i < proc->n_in_pspecs; i++)
    {
      ivalues[i].g_type = 0;
      g_value_init (ivalues + i, G_PARAM_SPEC_VALUE_TYPE (proc->in_pspecs[i]));
      token = bse_storage_parse_param_value (storage, ivalues + i, proc->in_pspecs[i], FALSE);
      if (token != G_TOKEN_NONE)
	{
	  g_value_unset (ivalues + i);
	  break;
	}
    }

  /* close statement */
  if (token == G_TOKEN_NONE && g_scanner_get_next_token (scanner) != ')')
    token = ')';

  /* call procedure */
  if (token == G_TOKEN_NONE)
    {
      if (proc->n_out_pspecs)
	g_value_init (retval, G_PARAM_SPEC_VALUE_TYPE (proc->out_pspecs[0]));

      *error_p = bse_procedure_marshal (BSE_PROCEDURE_TYPE (proc),
					ivalues, retval,
					NULL, NULL);

      if (proc->n_out_pspecs && g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (proc->out_pspecs[0]), BSE_TYPE_OBJECT))
	{
	  GValue pvalue = { 0, };
	  g_value_init (&pvalue, BSW_TYPE_PROXY);
	  g_value_transform (retval, &pvalue);
	  g_value_unset (retval);
	  memcpy (retval, &pvalue, sizeof (pvalue));    /* values are relocatable */
	}
      if (*error_p)
	{
	  bse_storage_error (storage, "proc-eval: error during execution of procedure \"%s\": %s",
			     proc->name, bse_error_blurb (*error_p));
	  token = G_TOKEN_ERROR;
	}
    }
  while (i--)
    g_value_unset (ivalues + i);
  g_type_class_unref (proc);

  return token;
}

gchar*
bse_procedure_eval (const gchar  *expr,
		    BseErrorType *error_p,
		    GValue       *value)
{
  BseErrorType error = BSE_ERROR_DATA_CORRUPT;
  BseStorage *storage;
  GTokenType token;
  gchar *warnings = NULL;

  g_return_val_if_fail (expr != NULL, NULL);
  g_return_val_if_fail (G_VALUE_TYPE (value) == 0, NULL);

  storage = bse_storage_new ();
  bse_storage_enable_proxies (storage);
  bse_storage_input_text (storage, expr);

  token = bse_procedure_eval_storage (storage, &error, value);
  if (token != G_TOKEN_NONE)
    bse_storage_unexp_token (storage, token);

  if (G_VALUE_TYPE (value) == 0)
    {
      g_value_init (value, BSE_TYPE_ERROR_TYPE);
      g_value_set_enum (value, error);
    }
  bse_storage_destroy (storage);

  if (error_p)
    *error_p = error;

  return warnings;
}

gchar*
bse_procedure_marshal_retval (BseErrorType error,
			      GValue     *value,
			      const gchar *warnings)
{
  BseStorage *storage;
  gchar *str;

  g_return_val_if_fail (G_IS_VALUE (value), NULL);

  storage = bse_storage_new ();
  bse_storage_enable_proxies (storage);
  bse_storage_prepare_write (storage, TRUE);

  bse_storage_puts (storage, "(bse-proc-return ");
  /* error code, marshalled as uint */
  bse_storage_printf (storage, "%u ", error);
  /* return value type */
  if (G_TYPE_IS_OBJECT (G_VALUE_TYPE (value)))
    bse_storage_puts (storage, "\"BswProxy\" ");
  else
    bse_storage_printf (storage, "\"%s\" ", g_type_name (G_VALUE_TYPE (value)));
  /* return value */
  bse_storage_push_level (storage);
  bse_storage_put_value (storage, value, NULL);
  /* errors and warnings */
  if (warnings)
    {
      gchar *esc = g_strescape (warnings, NULL);

      bse_storage_break (storage);
      bse_storage_printf (storage, " \"%s\"", esc);
      g_free (esc);
    }
  bse_storage_pop_level (storage);
  bse_storage_putc (storage, ')');

  /* done, return string */
  str = g_strdup (bse_storage_peek_text (storage, NULL));
  bse_storage_destroy (storage);

  return str;
}

gchar*
bse_procedure_unmarshal_retval (const gchar  *string,
				BseErrorType *error_p,
				GValue       *value)
{
  BseStorage *storage;
  GScanner *scanner;
  BseErrorType error;
  GType rtype;
  gchar *warnings = NULL;

  g_return_val_if_fail (string != NULL, NULL);
  g_return_val_if_fail (G_VALUE_TYPE (value) == 0, NULL);

  storage = bse_storage_new ();
  bse_storage_enable_proxies (storage);
  bse_storage_input_text (storage, string);
  scanner = storage->scanner;

  /* parse boilerplate */
  if (g_scanner_get_next_token (scanner) != '(' ||
      g_scanner_get_next_token (scanner) != G_TOKEN_IDENTIFIER ||
      strcmp (scanner->value.v_identifier, "bse-proc-return") != 0)
    goto data_corrupt;

  /* parse error code */
  if (g_scanner_get_next_token (scanner) != G_TOKEN_INT)
    goto data_corrupt;
  error = scanner->value.v_int;

  /* return value type */
  if (g_scanner_get_next_token (scanner) != G_TOKEN_STRING)
    goto data_corrupt;
  rtype = g_type_from_name (scanner->value.v_string);
  if (!rtype)
    goto data_corrupt;

  /* return value */
  g_value_init (value, rtype);
  if (bse_storage_parse_param_value (storage, value, NULL, FALSE) != G_TOKEN_NONE)
    goto data_corrupt;

  /* errors and warnings */
  if (g_scanner_peek_next_token (scanner) == G_TOKEN_STRING)
    {
      g_scanner_get_next_token (scanner);
      warnings = g_strdup (scanner->value.v_string);
    }

  /* close statement */
  if (g_scanner_get_next_token (scanner) != ')')
    goto data_corrupt;
  bse_storage_destroy (storage);

  if (error_p)
    *error_p = error;

  return warnings;

 data_corrupt:
  if (G_VALUE_TYPE (value))
    g_value_unset (value);
  if (warnings)
    g_free (warnings);

  if (error_p)
    *error_p = BSE_ERROR_DATA_CORRUPT;

  return g_strdup ("invalid format of bse-error statement");
}
