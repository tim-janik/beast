/* BSW-SCM - Bedevilled Sound Engine Scheme Wrapper
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
#define G_LOG_DOMAIN "BswShell"

#include <string.h>
#include <errno.h>
#include "bswscminterp.h"
#include <gsl/gslgluecodec.h>
#include <bse/bseglue.h>
#include <bse/bsecomwire.h>

/* Data types:
 * SCM
 * Constants:
 * SCM_BOOL_T, SCM_BOOL_F
 * Object:
 * SCM_UNSPECIFIED
 * Checks:
 * SCM_IMP()	- is immediate?
 * SCM_NIMP()	- is not immediate?
 *
 * catching exceptions:
 * typedef SCM (*scm_catch_body_t) (void *data);
 * typedef SCM (*scm_catch_handler_t) (void *data,
 * SCM tag = SCM_BOOL_T; means catch-all
 * SCM gh_catch(SCM tag, scm_catch_body_t body, void *body_data,
 *              scm_catch_handler_t handler, void *handler_data);
 */


/* --- SCM GC hook --- */
typedef void (*BswScmFreeFunc) ();
typedef struct {
  gpointer       data;
  BswScmFreeFunc free_func;
  gsize          size_hint;
} BswScmGCCell;
static gulong tc_gc_cell = 0;
static void
bsw_scm_enter_gc (SCM           *scm_gc_list,
		  gpointer       data,
		  BswScmFreeFunc free_func,
		  gsize          size_hint)
{
  BswScmGCCell *gc_cell;
  SCM s_cell = 0;

  g_return_if_fail (scm_gc_list != NULL);
  g_return_if_fail (free_func != NULL);

  gc_cell = g_new (BswScmGCCell, 1);
  gc_cell->data = data;
  gc_cell->free_func = free_func;
  gc_cell->size_hint = size_hint + sizeof (BswScmGCCell);

  SCM_NEWSMOB (s_cell, tc_gc_cell, gc_cell);
  *scm_gc_list = gh_cons (s_cell, *scm_gc_list);
  scm_done_malloc (gc_cell->size_hint);
}

static SCM
bsw_scm_mark_gc_cell (SCM scm_gc_cell)
{
  /* BswScmGCCell *gc_cell = (BswScmGCCell*) SCM_CDR (scm_gc_cell); */

  /* scm_gc_mark (gc_cell->something); */

  return SCM_BOOL_F;
}

static scm_sizet
bsw_scm_free_gc_cell (SCM scm_gc_cell)
{
  BswScmGCCell *gc_cell = (BswScmGCCell*) SCM_CDR (scm_gc_cell);
  scm_sizet size = gc_cell->size_hint;

  // g_printerr ("GCCell freeing %u bytes (%p).\n", size, gc_cell->free_func);

  gc_cell->free_func (gc_cell->data);
  g_free (gc_cell);
  
  return size;
}


/* --- SCM procedures --- */
static gboolean server_enabled = FALSE;

void
bsw_scm_enable_server (gboolean enabled)
{
  server_enabled = enabled != FALSE;
}

SCM
bsw_scm_server_get (void)
{
  BswProxy server;
  SCM s_retval;

  BSW_SCM_DEFER_INTS ();
  server = server_enabled ? BSW_SERVER : 0;
  BSW_SCM_ALLOW_INTS ();
  s_retval = gh_ulong2scm (server);

  return s_retval;
}

static inline gchar
char_tolower (gchar c)
{
  if (c >= '0' && c <= '9')
    return c;
  else if (c >= 'A' && c <= 'Z')
    return c - 'A' + 'a';
  else if (c >= 'a' && c <= 'z')
    return c;
  else
    return '-';
}

static inline gboolean
enum_match (const gchar *str1,
	    const gchar *str2)
{
  while (*str1 && *str2)
    {
      guchar s1 = char_tolower (*str1++);
      guchar s2 = char_tolower (*str2++);
      if (s1 != s2)
	return FALSE;
    }
  return *str1 == 0 && *str2 == 0;
}

static guint
glue_enum_index (GslGlueEnum *e,
		 guint        length,
		 const gchar *namechars,
		 SCM          scmval)
{
  SCM gclist = SCM_EOL;
  gchar *sym, *msg;
  guint i;

  sym = g_strndup (namechars, length);
  for (i = 0; i < e->n_values; i++)
    {
      guint n = strlen (e->values[i]);
      if (n >= length && enum_match (e->values[i] + n - length, sym))
	{
	  g_free (sym);
	  return i;
	}
    }
  msg = g_strdup_printf ("enum `%s' has no such value: `%s'",
			 e->enum_name,
			 sym);
  g_free (sym);
  bsw_scm_enter_gc (&gclist, msg, g_free, 64);
  scm_misc_error ("bsw-enum-from-scm", msg, scmval);
  return G_MAXINT;	/* not reached */
}

SCM
bsw_scm_enum_match (SCM s_ev1,
		    SCM s_ev2)
{
  gchar *v1, *v2;
  gboolean match;

  SCM_ASSERT (SCM_SYMBOLP (s_ev1),  s_ev1,  SCM_ARG1, "bsw-enum-match?");
  SCM_ASSERT (SCM_SYMBOLP (s_ev2),  s_ev2,  SCM_ARG2, "bsw-enum-match?");

  v1 = g_strndup (SCM_ROCHARS (s_ev1), SCM_LENGTH (s_ev1));
  v2 = g_strndup (SCM_ROCHARS (s_ev2), SCM_LENGTH (s_ev2));
  match = enum_match (v1 + SCM_LENGTH (s_ev1) - MIN (SCM_LENGTH (s_ev1), SCM_LENGTH (s_ev2)),
		      v2 + SCM_LENGTH (s_ev2) - MIN (SCM_LENGTH (s_ev1), SCM_LENGTH (s_ev2)));
  g_free (v1);
  g_free (v2);

  return gh_bool2scm (match);
}

SCM
bsw_scm_glue_set_prop (SCM s_proxy,
		       SCM s_prop_name,
		       SCM s_value)
{
  SCM gclist = SCM_EOL;
  GslGlueValue value, tmpval;
  GslGlueProp *pdef;
  GslGlueRec *rec;
  gulong proxy;
  gchar *prop_name;

  SCM_ASSERT (SCM_IMP (s_proxy),  s_proxy,  SCM_ARG1, "bsw-set-prop");
  SCM_ASSERT (SCM_STRINGP (s_prop_name), s_prop_name, SCM_ARG2, "bsw-set-prop");

  BSW_SCM_DEFER_INTS ();

  proxy = gh_scm2long (s_proxy);
  prop_name = g_strndup (SCM_ROCHARS (s_prop_name), SCM_LENGTH (s_prop_name));
  bsw_scm_enter_gc (&gclist, prop_name, g_free, SCM_LENGTH (s_prop_name));
  pdef = gsl_glue_describe_prop (proxy, prop_name);
  bsw_scm_enter_gc (&gclist, pdef, gsl_glue_free_prop, 4096);
  if (!pdef)
    {
      gchar *msg = g_strdup_printf ("proxy %lu has no property \"%s\"", proxy, prop_name);
      bsw_scm_enter_gc (&gclist, msg, g_free, 64);
      scm_misc_error ("bsw-set-prop", msg, SCM_BOOL_F);
    }

  switch (pdef->param.glue_type)
    {
      gchar *str;
    case GSL_GLUE_TYPE_BOOL:
      value = gsl_glue_value_bool (gh_scm2bool (s_value));
      break;
    case GSL_GLUE_TYPE_IRANGE:
      value = gsl_glue_value_int (gh_scm2long (s_value));
      break;
    case GSL_GLUE_TYPE_FRANGE:
      value = gsl_glue_value_float (gh_scm2double (s_value));
      break;
    case GSL_GLUE_TYPE_STRING:
      str = gh_scm2newstr (s_value, NULL);
      value = gsl_glue_value_string (str);
      free (str);
      break;
    case GSL_GLUE_TYPE_PROXY:
      value = gsl_glue_value_proxy (gh_scm2long (s_value));
      break;
    case GSL_GLUE_TYPE_ENUM:
      if (SCM_SYMBOLP (s_value))
	{
	  GslGlueEnum *e = gsl_glue_describe_enum (pdef->param.penum.enum_name);
	  if (e)
	    {
	      guint n;
	      
	      bsw_scm_enter_gc (&gclist, e, gsl_glue_free_enum, 1024);
	      n = glue_enum_index (e, SCM_LENGTH (s_value), SCM_ROCHARS (s_value), s_value);
	      if (n < G_MAXINT)
		{
		  value = gsl_glue_value_enum (e->enum_name, n);
		  break;
		}
	    }
	}
      /* fall through */
    default:
      scm_wrong_type_arg ("bsw-set-prop", SCM_ARG3, s_value);
    }

  rec = gsl_glue_rec ();
  tmpval = gsl_glue_value_proxy (proxy);
  gsl_glue_rec_take_append (rec, &tmpval);
  tmpval = gsl_glue_value_string (prop_name);
  gsl_glue_rec_take_append (rec, &tmpval);
  gsl_glue_rec_take_append (rec, &value);
  value = gsl_glue_value_take_rec (rec);
  tmpval = gsl_glue_client_msg ("bse-set-prop", value);
  gsl_glue_reset_value (&value);
  gsl_glue_reset_value (&tmpval);

  BSW_SCM_ALLOW_INTS ();
  
  return SCM_UNSPECIFIED;
}

static SCM
bsw_scm_from_glue_value (GslGlueValue value)
{
  SCM gclist = SCM_EOL;
  SCM s_ret;

  BSW_SCM_DEFER_INTS ();

  switch (value.glue_type)
    {
    case GSL_GLUE_TYPE_NONE:
      s_ret = SCM_UNSPECIFIED;
      break;
    case GSL_GLUE_TYPE_BOOL:
      s_ret = gh_bool2scm (value.value.v_bool);
      break;
    case GSL_GLUE_TYPE_IRANGE:
      s_ret = gh_long2scm (value.value.v_int);
      break;
    case GSL_GLUE_TYPE_FRANGE:
      s_ret = gh_double2scm (value.value.v_float);
      break;
    case GSL_GLUE_TYPE_STRING:
      s_ret = gh_str02scm (value.value.v_string);
      break;
    case GSL_GLUE_TYPE_PROXY:
      s_ret = gh_long2scm (value.value.v_proxy);
      break;
    case GSL_GLUE_TYPE_REC:
      s_ret = SCM_EOL;
      if (value.value.v_rec)
	{
	  GslGlueRec *rec = value.value.v_rec;
	  guint i = rec->n_fields;

	  while (i--)
	    s_ret = scm_cons (bsw_scm_from_glue_value (rec->fields[i]), s_ret);
	}
      break;
    case GSL_GLUE_TYPE_ENUM:
      if (value.value.v_enum.name)
	{
	  GslGlueEnum *e = gsl_glue_describe_enum (value.value.v_enum.name);
	  if (e)
	    {
	      guint n = value.value.v_enum.index;
	      
	      bsw_scm_enter_gc (&gclist, e, gsl_glue_free_enum, 1024);
	      if (n < e->n_values)
		{
		  s_ret = SCM_CAR (scm_intern0 (e->values[n]));
		  break;
		}
	      else
		g_message ("invalid enum index in conversion: %u (type=%s)", n, value.value.v_enum.name);
	    }
	}
      /* fall through */
    default:
      g_message ("unable to convert glue value to scm (type=%u)", value.glue_type);
      s_ret = SCM_UNSPECIFIED;
    }

  BSW_SCM_ALLOW_INTS ();
  
  return s_ret;
}

SCM
bsw_scm_glue_call (SCM s_proc_name,
		   SCM s_arg_list)
{
  gchar *proc_name;
  SCM gclist = SCM_EOL;
  SCM node, s_ret;
  GslGlueCall *pcall;
  GslGlueProc *pdef;
  guint i;
  
  SCM_ASSERT (SCM_STRINGP (s_proc_name),  s_proc_name,  SCM_ARG1, "bsw-glue-call");
  SCM_ASSERT (SCM_CONSP (s_arg_list) || s_arg_list == SCM_EOL,  s_arg_list,  SCM_ARG2, "bsw-glue-call");

  BSW_SCM_DEFER_INTS ();

  proc_name = g_strndup (SCM_ROCHARS (s_proc_name), SCM_LENGTH (s_proc_name));
  bsw_scm_enter_gc (&gclist, proc_name, g_free, SCM_LENGTH (s_proc_name));

  pdef = gsl_glue_describe_proc (proc_name);
  if (!pdef)
    scm_misc_error ("bsw-glue-call", "failed to retrive proc description", SCM_BOOL_F); // s_proc_name);
  bsw_scm_enter_gc (&gclist, pdef, gsl_glue_free_proc, 4096);

  pcall = gsl_glue_call_proc (proc_name);
  bsw_scm_enter_gc (&gclist, pcall, gsl_glue_free_call, 4096);

  i = 0;
  for (node = s_arg_list; SCM_CONSP (node); node = SCM_CDR (node), i++)
    {
      SCM arg = SCM_CAR (node);

      if (i >= pdef->n_params)
	scm_wrong_num_args (s_proc_name);
      switch (pdef->params[i].glue_type)
	{
	  gchar *str;
	case GSL_GLUE_TYPE_BOOL:
	  gsl_glue_call_arg_bool (pcall, gh_scm2bool (arg));
	  break;
	case GSL_GLUE_TYPE_IRANGE:
	  gsl_glue_call_arg_int (pcall, gh_scm2long (arg));
	  break;
	case GSL_GLUE_TYPE_FRANGE:
	  gsl_glue_call_arg_float (pcall, gh_scm2double (arg));
	  break;
	case GSL_GLUE_TYPE_STRING:
	  str = gh_scm2newstr (arg, NULL);
	  gsl_glue_call_arg_string (pcall, str);
	  free (str);
	  break;
	case GSL_GLUE_TYPE_PROXY:
	  gsl_glue_call_arg_proxy (pcall, gh_scm2long (arg));
	  break;
	case GSL_GLUE_TYPE_ENUM:
	  if (SCM_SYMBOLP (arg))
	    {
	      GslGlueEnum *e = gsl_glue_describe_enum (pdef->params[i].penum.enum_name);
	      if (e)
		{
		  guint n;
		  
		  bsw_scm_enter_gc (&gclist, e, gsl_glue_free_enum, 1024);
		  n = glue_enum_index (e, SCM_LENGTH (arg), SCM_ROCHARS (arg), arg);
		  if (n < G_MAXINT)
		    {
		      gsl_glue_call_arg_enum (pcall, pdef->params[i].penum.enum_name, n);
		      break;
		    }
		}
	    }
	  /* fall through */
	default:
	  scm_wrong_type_arg (proc_name, i, arg);	/* bit unfair, but what the heck ;) */
	}
    }
  /* FIXME: setup defauls here */

  gsl_glue_call_exec (pcall);

  s_ret = bsw_scm_from_glue_value (pcall->retval);

  BSW_SCM_ALLOW_INTS ();
  
  return s_ret;
}

typedef struct {
  gulong proxy;
  gchar *signal;
  SCM s_lambda;
  GslGlueRec *tmp_args;
} SigData;

static void
signal_handler_destroyed (gpointer data)
{
  SigData *sdata = data;

  scm_unprotect_object (sdata->s_lambda);
  sdata->s_lambda = 0;
  g_free (sdata->signal);
  g_free (sdata);
}

static SCM
marshal_sproc (void *data)
{
  SigData *sdata = data;
  SCM s_ret, args = SCM_EOL;
  GslGlueRec *rec = sdata->tmp_args;
  guint i;

  sdata->tmp_args = NULL;

  g_return_val_if_fail (rec != NULL && rec->n_fields > 0, SCM_UNSPECIFIED);

  i = rec->n_fields;
  while (i--)
    {
      SCM arg = bsw_scm_from_glue_value (rec->fields[i]);
      args = gh_cons (arg, args);
    }

  s_ret = scm_apply (sdata->s_lambda, args, SCM_EOL);

  return SCM_UNSPECIFIED;
}

static void
signal_handler (gpointer     sig_data,
		const gchar *signal,
		GslGlueRec  *args)
{
  SCM_STACKITEM stack_item;
  SigData *sdata = sig_data;

  sdata->tmp_args = args;
  scm_internal_cwdr ((scm_catch_body_t) marshal_sproc, sdata,
		     scm_handle_by_message_noexit, "BSW", &stack_item);
}

SCM
bsw_scm_signal_connect (SCM s_proxy,
			SCM s_signal,
			SCM s_lambda)
{
  gulong proxy, id;
  SigData *sdata;
  
  SCM_ASSERT (SCM_IMP (s_proxy), s_proxy,  SCM_ARG1, "bsw-signal-connect");
  SCM_ASSERT (SCM_STRINGP (s_signal), s_signal, SCM_ARG2, "bsw-signal-connect");
  SCM_ASSERT (gh_procedure_p (s_lambda), s_lambda,  SCM_ARG3, "bsw-signal-connect");

  proxy = gh_scm2ulong (s_proxy);

  BSW_SCM_DEFER_INTS ();
  sdata = g_new0 (SigData, 1);
  sdata->proxy = proxy;
  sdata->signal = g_strndup (SCM_ROCHARS (s_signal), SCM_LENGTH (s_signal));
  sdata->s_lambda = s_lambda;
  scm_protect_object (sdata->s_lambda);
  id = gsl_glue_signal_connect (sdata->signal, proxy, signal_handler, sdata, signal_handler_destroyed);
  BSW_SCM_ALLOW_INTS ();
  
  return gh_ulong2scm (id);
}

static gboolean script_register_enabled = FALSE;

void
bsw_scm_enable_script_register (gboolean enabled)
{
  script_register_enabled = enabled != FALSE;
}

SCM
bsw_scm_script_register (SCM s_name,
			 SCM s_category,
			 SCM s_blurb,
			 SCM s_help,
			 SCM s_author,
			 SCM s_copyright,
			 SCM s_date,
			 SCM s_params)
{
  SCM node;
  guint i;

  SCM_ASSERT (SCM_SYMBOLP (s_name),      s_name,      SCM_ARG1, "bsw-script-register");
  SCM_ASSERT (SCM_STRINGP (s_category),  s_category,  SCM_ARG2, "bsw-script-register");
  SCM_ASSERT (SCM_STRINGP (s_blurb),     s_blurb,     SCM_ARG3, "bsw-script-register");
  SCM_ASSERT (SCM_STRINGP (s_help),      s_help,      SCM_ARG4, "bsw-script-register");
  SCM_ASSERT (SCM_STRINGP (s_author),    s_author,    SCM_ARG5, "bsw-script-register");
  SCM_ASSERT (SCM_STRINGP (s_copyright), s_copyright, SCM_ARG6, "bsw-script-register");
  SCM_ASSERT (SCM_STRINGP (s_date),      s_date,      SCM_ARG7, "bsw-script-register");
  for (node = s_params, i = 8; SCM_CONSP (node); node = SCM_CDR (node), i++)
    {
      SCM arg = SCM_CAR (node);
      if (!SCM_STRINGP (arg))
	scm_wrong_type_arg ("bsw-script-register", i, arg);
    }

  BSW_SCM_DEFER_INTS ();
  if (script_register_enabled)
    {
      GslGlueSeq *seq = gsl_glue_seq (GSL_GLUE_TYPE_STRING);
      GslGlueValue sv, rv;

      sv = gsl_glue_value_stringl (SCM_ROCHARS (s_name), SCM_LENGTH (s_name));
      gsl_glue_seq_take_append (seq, &sv);
      sv = gsl_glue_value_stringl (SCM_ROCHARS (s_category), SCM_LENGTH (s_category));
      gsl_glue_seq_take_append (seq, &sv);
      sv = gsl_glue_value_stringl (SCM_ROCHARS (s_blurb), SCM_LENGTH (s_blurb));
      gsl_glue_seq_take_append (seq, &sv);
      sv = gsl_glue_value_stringl (SCM_ROCHARS (s_help), SCM_LENGTH (s_help));
      gsl_glue_seq_take_append (seq, &sv);
      sv = gsl_glue_value_stringl (SCM_ROCHARS (s_author), SCM_LENGTH (s_author));
      gsl_glue_seq_take_append (seq, &sv);
      sv = gsl_glue_value_stringl (SCM_ROCHARS (s_copyright), SCM_LENGTH (s_copyright));
      gsl_glue_seq_take_append (seq, &sv);
      sv = gsl_glue_value_stringl (SCM_ROCHARS (s_date), SCM_LENGTH (s_date));
      gsl_glue_seq_take_append (seq, &sv);
      
      for (node = s_params; SCM_CONSP (node); node = SCM_CDR (node))
	{
	  SCM arg = SCM_CAR (node);
	  sv = gsl_glue_value_stringl (SCM_ROCHARS (arg), SCM_LENGTH (arg));
	  gsl_glue_seq_take_append (seq, &sv);
	}

      sv = gsl_glue_value_take_seq (seq);
      rv = gsl_glue_client_msg ("bse-script-register", sv);
      gsl_glue_reset_value (&sv);
      if (rv.glue_type == GSL_GLUE_TYPE_STRING && rv.value.v_string)
	{
	  gchar *name = g_strndup (SCM_ROCHARS (s_name), SCM_LENGTH (s_name));
	  g_message ("while registering \"%s\": %s", name, rv.value.v_string);
	  g_free (name);
	}
      gsl_glue_reset_value (&rv);
    }
  BSW_SCM_ALLOW_INTS ();

  return SCM_UNSPECIFIED;
}

static BswSCMWire *bse_iteration_wire = NULL;

SCM
bsw_scm_context_pending (void)
{
  gboolean pending;

  BSW_SCM_DEFER_INTS ();
  if (bse_iteration_wire)
    bsw_scm_wire_dispatch_io (bse_iteration_wire, 0);
  pending = gsl_glue_context_pending (gsl_glue_fetch_context (G_STRLOC));
  BSW_SCM_ALLOW_INTS ();

  return gh_bool2scm (pending);
}

SCM
bsw_scm_context_iteration (SCM s_may_block)
{
  if (gsl_glue_context_pending (gsl_glue_fetch_context (G_STRLOC)))
    gsl_glue_context_dispatch (gsl_glue_fetch_context (G_STRLOC));
  else if (gh_scm2bool (s_may_block))
    {
      if (bse_iteration_wire)
	bsw_scm_wire_dispatch_io (bse_iteration_wire, 1500);
      else
	g_usleep (1500 * 1000);
    }
  return SCM_UNSPECIFIED;
}


/* --- initialization --- */
static gchar*
send_to_wire (gpointer     user_data,
	      const gchar *message)
{
  gchar *response;

  response = bsw_scm_wire_do_request (user_data, message);

  return response;
}

static void
register_types (gchar **types)
{
  while (*types)
    {
      gchar **names = gsl_glue_list_method_names (*types);
      gchar *sname = bsw_type_name_to_sname (*types);
      gchar *s;
      guint i;
      
      if (strncmp (sname, "bsw-", 4) == 0)
	{
	  s = g_strdup_printf ("(define (bsw-is-%s proxy) (bsw-item-check-is-a proxy \"%s\"))",
			       sname + 4, *types);
	  gh_eval_str (s);
	  g_free (s);
	}
      for (i = 0; names[i]; i++)
	{
	  gchar *s = g_strdup_printf ("(define %s-%s (lambda list (bsw-glue-call \"%s+%s\" list)))",
				      sname, names[i], *types, names[i]);
	  gh_eval_str (s);
	  g_free (s);
	}
      g_strfreev (names);
      g_free (sname);

      names = gsl_glue_iface_children (*types);
      register_types (names);
      g_strfreev (names);

      types++;
    }
}

void
bsw_scm_interp_init (BswSCMWire *wire)
{
  gchar **procs;
  guint i;

  if (wire)
    {
      bse_iteration_wire = wire;
      gsl_glue_context_push (gsl_glue_codec_context (send_to_wire, wire, NULL));
    }
  else
    gsl_glue_context_push (bse_glue_context ());

  tc_gc_cell = scm_make_smob_type ("BswScmGCCell", 0);
  scm_set_smob_mark (tc_gc_cell, bsw_scm_mark_gc_cell);
  scm_set_smob_free (tc_gc_cell, bsw_scm_free_gc_cell);

  gh_new_procedure ("bsw-glue-call", bsw_scm_glue_call, 2, 0, 0);
  gh_new_procedure ("bsw-glue-set-prop", bsw_scm_glue_set_prop, 3, 0, 0);

  gh_eval_str ("(define (bsw-is-null proxy) (= proxy 0))");
  
  procs = gsl_glue_list_proc_names ();
  for (i = 0; procs[i]; i++)
    if (strncmp (procs[i], "bse-", 4) == 0)
      {
	gchar *s = g_strdup_printf ("(define bsw-%s (lambda list (bsw-glue-call \"%s\" list)))", procs[i] + 4, procs[i]);
	gh_eval_str (s);
	g_free (s);
      }
  g_strfreev (procs);

  procs = g_new (gchar*, 2);
  procs[0] = gsl_glue_base_iface ();
  procs[1] = NULL;
  register_types (procs);
  g_strfreev (procs);

  gh_new_procedure0_0 ("bsw-server-get", bsw_scm_server_get);
  gh_new_procedure ("bsw-script-register", bsw_scm_script_register, 7, 0, 1);
  gh_new_procedure ("bsw-enum-match?", bsw_scm_enum_match, 2, 0, 0);
  gh_new_procedure ("bsw-signal-connect", bsw_scm_signal_connect, 3, 0, 0);
  gh_new_procedure ("bsw-context-pending", bsw_scm_context_pending, 0, 0, 0);
  gh_new_procedure ("bsw-context-iteration", bsw_scm_context_iteration, 1, 0, 0);
}


/* --- SCM-Wire --- */
struct _BswSCMWire
{
  BseComWire wire;
};

static gboolean
wire_ispatch (gpointer        data,
	      guint           request,
	      const gchar    *request_msg,
	      BseComWire     *wire)
{
  /* avoid spurious invocations */
  if (!wire->connected)
    return FALSE;

  /* dispatch serialized events */
  gsl_glue_codec_enqueue_event (gsl_glue_fetch_context (G_STRLOC), request_msg);

  /* events don't return results */
  bse_com_wire_discard_request (wire, request);

  /* we handled this request_msg */
  return TRUE;
}

BswSCMWire*
bsw_scm_wire_from_pipe (const gchar *ident,
			gint         remote_input,
			gint         remote_output)
{
  BseComWire *wire = bse_com_wire_from_pipe (ident, remote_input, remote_output);

  bse_com_wire_set_dispatcher (wire, wire_ispatch, NULL, NULL);

  return (BswSCMWire*) wire;
}

gchar*
bsw_scm_wire_do_request (BswSCMWire  *swire,
			 const gchar *request_msg)
{
  BseComWire *wire = (BseComWire*) swire;
  guint request_id;

  g_return_val_if_fail (wire != NULL, NULL);
  g_return_val_if_fail (wire->connected != FALSE, NULL);
  g_return_val_if_fail (request_msg != NULL, NULL);

  request_id = bse_com_wire_send_request (wire, request_msg);
  while (wire->connected)
    {
      gchar *result = bse_com_wire_receive_result (wire, request_id);

      /* have result? then we're done */
      if (result)
	return result;
      /* still need to dispatch incoming requests */
      if (!bse_com_wire_receive_dispatch (wire))
	{
	  /* nothing to dispatch, process I/O
	   */
	  /* block until new data is available */
	  bse_com_wire_select (wire, 1000);
	  /* handle new data if any */
	  bse_com_wire_process_io (wire);
	}
    }

  bsw_scm_wire_died (swire);

  return NULL;  /* never reached */
}

void
bsw_scm_wire_dispatch_io (BswSCMWire *swire,
			  guint       timeout)
{
  BseComWire *wire = (BseComWire*) swire;

  g_return_if_fail (wire != NULL);

  if (!wire->connected)
    bsw_scm_wire_died (swire);

  /* dispatch incoming requests */
  if (!bse_com_wire_receive_dispatch (wire))
    {
      /* nothing to dispatch, process I/O
       */
      /* block until new data is available */
      bse_com_wire_select (wire, timeout);
      /* handle new data if any */
      bse_com_wire_process_io (wire);
    }

  if (!wire->connected)
    bsw_scm_wire_died (swire);
}

void
bsw_scm_wire_died (BswSCMWire *swire)
{
  BseComWire *wire = (BseComWire*) swire;

  bse_com_wire_destroy (wire);
  exit (0);
}
