/* BSE - Bedevilled Sound Engine
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
#include "bsescripthelper.h"

#include "../PKG_config.h"
#include "bsecategories.h"
#include "bseserver.h"
#include <string.h>
#include <stdlib.h>


/* --- prototypes --- */
static void		bse_script_procedure_init	(BseScriptProcedureClass *class,
							 BseScriptData		 *sdata);
static BseErrorType	bse_script_procedure_exec	(BseProcedureClass	 *proc,
							 GValue			 *in_values,
							 GValue			 *out_values);
static gboolean		bse_script_dispatcher		(gpointer		  data,
							 guint			  request,
							 const gchar		 *request_msg,
							 BseComWire		 *wire);
static GParamSpec*	bse_script_param_spec		(gchar			 *pspec_desc,
							 const gchar		 *script_name,
							 const gchar		 *func_name,
							 gchar			**free1,
							 gchar			**free2);
static void		bse_script_param_stringify	(GString		 *gstring,
							 GValue			 *value,
							 GParamSpec		 *pspec);


/* --- variables --- */
static GSList *wire_stack = NULL;


/* --- functions --- */
static GSList*
string_list_copy_deep (GSList *xlist)
{
  GSList *slist, *dlist = NULL;
  for (slist = xlist; slist; slist = slist->next)
    dlist = g_slist_prepend (dlist, g_strdup (slist->data));
  return g_slist_reverse (dlist);
}

static void
string_list_free_deep (GSList *slist)
{
  while (slist)
    {
      GSList *tmp = slist->next;
      g_free (slist->data);
      slist = tmp;
    }
}

static void
bse_script_procedure_init (BseScriptProcedureClass *class,
			   BseScriptData           *sdata)
{
  BseProcedureClass *proc = (BseProcedureClass*) class;
  GSList *slist;
  guint n;

  proc->name = g_type_name (G_TYPE_FROM_CLASS (proc));
  proc->blurb = sdata->blurb;
  proc->help = sdata->help;
  proc->author = sdata->author;
  proc->copyright = sdata->copyright;
  proc->date = sdata->date;
  class->sdata = sdata;
  proc->execute = bse_script_procedure_exec;

  /* we support a limited parameter set for scripts */
  n = g_slist_length (sdata->params);
  proc->in_pspecs = g_new (GParamSpec*, n + 1);
  for (slist = sdata->params; slist; slist = slist->next)
    {
      gchar *f1 = NULL, *f2 = NULL;
      GParamSpec *pspec = bse_script_param_spec (slist->data, sdata->script_file, sdata->name, &f1, &f2);
      g_free (f1);
      g_free (f2);
      if (pspec)
	{
	  proc->in_pspecs[proc->n_in_pspecs++] = pspec;
	  g_param_spec_sink (g_param_spec_ref (pspec));
	}
      else
	g_message ("unable to register parameter for function \"%s\" in script \"%s\" from: %s",
		   sdata->name, sdata->script_file, (gchar*) slist->data);
    }
  proc->in_pspecs[proc->n_in_pspecs] = NULL;
}

GType
bse_script_proc_register (const gchar *script_file,
			  const gchar *name,
			  const gchar *category,
			  const gchar *blurb,
			  const gchar *help,
			  const gchar *author,
			  const gchar *copyright,
			  const gchar *date,
			  GSList      *params)
{
  GTypeInfo script_info = {
    sizeof (BseScriptProcedureClass),

    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_script_procedure_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,

    /* non classed type stuff */
    0, 0, NULL,
  };
  BseScriptData *sdata;
  gchar *tname;
  GType type;

  g_return_val_if_fail (script_file != NULL, 0);
  g_return_val_if_fail (name != NULL, 0);
  if (g_slist_length (params) > BSE_PROCEDURE_MAX_IN_PARAMS)
    {
      g_message ("not registering script \"%s\" which needs more than %u parameters",
		  name, BSE_PROCEDURE_MAX_IN_PARAMS);
      return 0;
    }

  sdata = g_new0 (BseScriptData, 1);
  sdata->script_file = g_strdup (script_file);
  sdata->name = g_strdup (name);
  sdata->blurb = g_strdup (blurb);
  sdata->help = g_strdup (help);
  sdata->author = g_strdup (author);
  sdata->copyright = g_strdup (copyright);
  sdata->date = g_strdup (date);
  sdata->params = string_list_copy_deep (params);
  script_info.class_data = sdata;

  tname = g_strconcat ("bse-script-", name, NULL);
  type = g_type_register_static (BSE_TYPE_PROCEDURE, tname, &script_info, 0);
  g_free (tname);
  if (type && category && category[0])
    bse_categories_register (category, type);
  return type;
}

static GType
bse_script_register_scan (const gchar *script_file,
			  const gchar *bsr_args)
{
  GTokenType token = G_TOKEN_STRING;
  GScanner *scanner;
  GType type = 0;
  gchar *name = NULL, *category = NULL, *blurb = NULL, *help = NULL;
  gchar *author = NULL, *copyright = NULL, *date = NULL;
  GSList *params = NULL;
  
  g_return_val_if_fail (script_file != NULL, 0);
  g_return_val_if_fail (bsr_args != NULL, 0);

  scanner = g_scanner_new (NULL);
  g_scanner_input_text (scanner, bsr_args, strlen (bsr_args));

  if (g_scanner_get_next_token (scanner) != G_TOKEN_STRING)
    goto bail_out;
  name = g_strdup (scanner->value.v_string);
  if (g_scanner_get_next_token (scanner) != G_TOKEN_STRING)
    goto bail_out;
  category = g_strdup (scanner->value.v_string);
  if (g_scanner_get_next_token (scanner) != G_TOKEN_STRING)
    goto bail_out;
  blurb = g_strdup (scanner->value.v_string);
  if (g_scanner_get_next_token (scanner) != G_TOKEN_STRING)
    goto bail_out;
  help = g_strdup (scanner->value.v_string);
  if (g_scanner_get_next_token (scanner) != G_TOKEN_STRING)
    goto bail_out;
  author = g_strdup (scanner->value.v_string);
  if (g_scanner_get_next_token (scanner) != G_TOKEN_STRING)
    goto bail_out;
  copyright = g_strdup (scanner->value.v_string);
  if (g_scanner_get_next_token (scanner) != G_TOKEN_STRING)
    goto bail_out;
  date = g_strdup (scanner->value.v_string);
  while (g_scanner_get_next_token (scanner) == G_TOKEN_STRING)
    params = g_slist_prepend (params, g_strdup (scanner->value.v_string));
  params = g_slist_reverse (params);
  if (scanner->token != ')')
    {
      token = ')';
      goto bail_out;
    }
  type = bse_script_proc_register (script_file, name, category, blurb, help, author, copyright, date, params);
  goto free_data;

 bail_out:
  g_scanner_unexp_token (scanner, token, NULL, NULL, NULL, "not registering script", FALSE);

 free_data:
  g_free (name);
  g_free (category);
  g_free (blurb);
  g_free (help);
  g_free (author);
  g_free (copyright);
  g_free (date);
  string_list_free_deep (params);
  g_scanner_destroy (scanner);

  return type;
}

static BseErrorType
bse_script_procedure_exec (BseProcedureClass *proc,
			   GValue            *in_values,
			   GValue            *out_values)
{
  BseScriptProcedureClass *sproc = (BseScriptProcedureClass*) proc;
  BseScriptData *sdata = sproc->sdata;
  BseServer *server = bse_server_get ();
  GSList *params = NULL;
  GString *gstring = g_string_new ("");
  gchar *error, *shellpath;
  guint i;

  for (i = 0; i < proc->n_in_pspecs; i++)
    bse_script_param_stringify (gstring, in_values + i, proc->in_pspecs[i]);

  params = g_slist_prepend (params, g_strdup_printf ("--bse-enable-server"));
  params = g_slist_prepend (params, g_strdup_printf ("(load \"%s\")"
						     "(%s %s)",
						     sdata->script_file,
						     sdata->name,
						     gstring->str));
  g_string_free (gstring, TRUE);
  shellpath = g_strdup_printf ("%s/%s", BSW_PATH_BINARIES, "bswshell");
  error = bse_server_run_remote (server, sdata->script_file, shellpath,
				 bse_script_dispatcher, sdata->script_file, NULL,
				 params);
  g_free (shellpath);
  string_list_free_deep (params);

  if (error)
    {
      g_message ("failed to start interpreter for \"%s\": %s", sdata->script_file, error);
      g_free (error);
    }
  else
    {
      bse_server_exec_status (server, BSE_EXEC_STATUS_START, sdata->script_file, -1, BSE_ERROR_NONE);
      /* don't let procedure notification override the status we just sent */
      bse_procedure_skip_next_exec_status ();
    }
  
  return error ? BSE_ERROR_SPAWN : BSE_ERROR_NONE;
}

static gboolean
bse_script_dispatcher (gpointer        data,
		       guint           request,
		       const gchar    *request_msg,
		       BseComWire     *wire)
{
  const gchar *script_file = data;
  gchar *result;

  /* avoid spurious invocations */
  if (!wire->connected)
    return FALSE;

  /* log current wire */
  wire_stack = g_slist_prepend (wire_stack, wire);

  /* catch registration requests */
  if (strncmp (request_msg, "(bse-script-register", 20) == 0)
    {
      GType type = bse_script_register_scan (script_file, request_msg + 20);
      
      /* marshal response */
      result = bse_procedure_marshal_retval (type ? 0 : BSE_ERROR_DATA_CORRUPT, NULL, NULL);
    }
  else
    {
      GValue value = { 0, };
      BseErrorType error;
      gchar *warnings;

      /* request_msg is a marshalled procedure call (or at least, it better be one ;)
       * we block exec-status, because the various procedure calls that a script makes
       * aren't all that interesting
       */
      bse_procedure_block_exec_status ();
      warnings = bse_procedure_eval (request_msg, &error, &value);
      bse_procedure_unblock_exec_status ();
      
      /* passing on warnings to the interpreter isn't actually usefull,
       * as the interpreter is less likely to present stderr
       */
      if (warnings)
	{
	  gchar *esc = g_strescape (request_msg, NULL);
	  
	  g_printerr ("%s: while evaluating request \"%s\":\n%s\n", wire->ident, esc, warnings);
	  g_free (esc);
	  g_free (warnings);
	}
      
      /* marshal return values */
      result = bse_procedure_marshal_retval (error, &value, NULL);
      g_value_unset (&value);
    }

  /* and send them back through the wire */
  bse_com_wire_send_result (wire, request, result);
  g_free (result);

  /* unlog wire */
  wire_stack = g_slist_remove (wire_stack, wire);

  /* we handled this request_msg */
  return TRUE;
}

BseComWire*
bse_script_peek_current_wire (void)
{
  return wire_stack ? wire_stack->data : NULL;
}

GSList*
bse_script_dir_list_files (const gchar *dir_list)
{
  GSList *slist = bse_search_path_list_files (dir_list, "*.scm", NULL, G_FILE_TEST_IS_REGULAR);

  return g_slist_sort (slist, (GCompareFunc) strcmp);
}

const gchar*
bse_script_file_register (const gchar *file_name)
{
  BseServer *server = bse_server_get ();
  GSList *params = NULL;
  gchar *error, *shellpath, *warning = NULL;

  params = g_slist_prepend (params, g_strdup_printf ("--bse-enable-register"));
  params = g_slist_prepend (params, g_strdup_printf ("(load \"%s\")", file_name));
  shellpath = g_strdup_printf ("%s/%s", BSW_PATH_BINARIES, "bswshell");
  error = bse_server_run_remote (server, file_name, shellpath, bse_script_dispatcher, g_strdup (file_name), g_free, params);
  g_free (shellpath);
  string_list_free_deep (params);

  if (error)
    warning = g_strdup_printf ("failed to start interpreter for \"%s\": %s", file_name, error);
  g_free (error);
  
  return warning;
}

static gchar*
make_sname (const gchar *string)
{
  gchar *p, *cname = g_strdup (string);

  for (p = cname; *p; p++)
    {
      if ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'z'))
	continue;
      else if (*p >= 'A' && *p <= 'Z')
	*p = *p - 'A' + 'a';
      else
	*p = '-';
    }
  return cname;
}

#define PARAM_FLAGS	(BSE_PARAM_DEFAULT | G_PARAM_LAX_VALIDATION)

static GParamSpec*
bse_script_param_spec (gchar       *pspec_desc,
		       const gchar *script_name,
		       const gchar *func_name,
		       gchar      **free1,
		       gchar      **free2)
{
  gchar *nick = strchr (pspec_desc, ':');
  gchar *dflt, *cname, *blurb;

  if (!nick)
    return NULL;
  *nick++ = 0;
  dflt = strchr (nick, ':');
  if (!dflt)
    return NULL;
  *dflt++ = 0;
  cname = make_sname (nick);
  *free1 = cname;
  blurb = g_strdup_printf ("Parameter \"%s\" to function <%s> in script \"%s\"",
			   cname, func_name, script_name);
  *free2 = blurb;
  if (strcmp (pspec_desc, "BseParamString") == 0)	/* "BseParamString:Text:Default" */
    return g_param_spec_string (cname, nick, blurb, dflt, PARAM_FLAGS);
  else if (strcmp (pspec_desc, "BseParamBool") == 0)	/* "BseParamBool:Mark-me:0" */
    return g_param_spec_boolean (cname, nick, blurb, strtol (dflt, NULL, 10), PARAM_FLAGS);
  else if (strcmp (pspec_desc, "BseParamIRange") == 0)	/* "BseParamIRange:IntNum:16 -100 100 5" */
    {
      glong val, min, max, step;
      gchar *p;
      val = strtol (dflt, &p, 10);
      min = p ? strtol (p, &p, 10) : -100;
      max = p ? strtol (p, &p, 10) : +100;
      if (max < min)
	{
	  step = min;
	  min = max;
	  max = step;
	}
      step = p ? strtol (p, &p, 10) : (max - min) / 100.0;
      val = CLAMP (val, min, max);
      return bse_param_spec_int (cname, nick, blurb, min, max, val, step, PARAM_FLAGS);
    }
  else if (strcmp (pspec_desc, "BseParamFRange") == 0)	/* "BseParamFRange:FloatNum:42 0 1000 10" */
    {
      double val, min, max, step;
      gchar *p;
      val = g_strtod (dflt, &p);
      min = p ? g_strtod (p, &p) : -100;
      max = p ? g_strtod (p, &p) : +100;
      if (max < min)
	{
	  step = min;
	  min = max;
	  max = step;
	}
      step = p ? g_strtod (p, &p) : (max - min) / 100.0;
      val = CLAMP (val, min, max);
      return bse_param_spec_float (cname, nick, blurb, min, max, val, step, PARAM_FLAGS);
    }
  else if (strcmp (pspec_desc, "BseNote") == 0)		/* "BseNote:Note:C-2" */
    {
      gint dfnote = bse_note_from_string (dflt);
      if (dfnote == BSE_NOTE_UNPARSABLE)
	dfnote = BSE_NOTE_VOID;
      return bse_param_spec_note (cname, nick, blurb, BSE_MIN_NOTE, BSE_MAX_NOTE, dfnote, 1, TRUE, PARAM_FLAGS);
    }
  else
    return NULL;
}

static void
bse_script_param_stringify (GString    *gstring,
			    GValue     *value,
			    GParamSpec *pspec)
{
  switch (G_TYPE_FUNDAMENTAL (G_VALUE_TYPE (value)))
    {
      gchar *str;
    case G_TYPE_STRING:
      str = g_value_get_string (value);
      str = g_strescape (str ? str : "", NULL);
      g_string_printfa (gstring, "\"%s\"", str);
      g_free (str);
      break;
    case G_TYPE_BOOLEAN:
      g_string_printfa (gstring, "#%c", g_value_get_boolean (value) ? 't' : 'f');
      break;
    case G_TYPE_INT:
      g_string_printfa (gstring, "%d", g_value_get_int (value));
      break;
    case G_TYPE_FLOAT:
      g_string_printfa (gstring, "%.17g", g_value_get_float (value));
      break;
    case BSE_TYPE_NOTE:
      str = bse_note_to_string (bse_value_get_note (value));
      g_string_printfa (gstring, "\"%s\"", str);
      g_free (str);
      break;
    default:
      g_assert_not_reached ();
    }
  g_string_append (gstring, " ");
}
