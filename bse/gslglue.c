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
#undef  G_LOG_DOMAIN
#define G_LOG_DOMAIN    "GslGlue"
#include "gslglue.h"

#include "gslcommon.h"
#include "gslgluesignal.c"
#include <string.h>


/* --- variables --- */
static GslGlueValue  zero_value = { 0, };	/* used for assertions */
static GslRing      *context_stack = NULL;


/* --- functions --- */
void
gsl_glue_context_common_init (GslGlueContext            *context,
			      const GslGlueContextTable *vtable)
{
  g_return_if_fail (context->sighash == NULL);

  context->table = *vtable;
  context->sighash = g_hash_table_new (glue_signal_hash, glue_signal_equal);
}

gboolean
gsl_glue_context_pending (GslGlueContext *context)
{
  g_return_val_if_fail (context != NULL, FALSE);

  return context->sigqueue || context->gc_signals;
}

void
gsl_glue_context_dispatch (GslGlueContext *context)
{
  g_return_if_fail (context != NULL);

  gsl_glue_signals_dispatch (context);
}


/* --- VTable API --- */
void
gsl_glue_context_push (GslGlueContext *context)
{
  g_return_if_fail (context != NULL);

  context_stack = gsl_ring_prepend (context_stack, context);
}

GslGlueContext*
gsl_glue_context_current (void)
{
  return context_stack ? context_stack->data : NULL;
}

void
gsl_glue_context_pop (void)
{
  g_return_if_fail (context_stack != NULL);

  context_stack = gsl_ring_remove_node (context_stack, context_stack);
}

static inline gulong
upper_power2 (gulong number)
{
  return number ? 1 << g_bit_storage (number - 1) : 0;
}

GslGlueProc*
gsl_glue_describe_proc (const gchar *proc_name)
{
  GslGlueContext *context = gsl_glue_fetch_context (G_STRLOC);
  GslGlueProc *proc;

  g_return_val_if_fail (proc_name != NULL, NULL);

  proc = context->table.describe_proc (context, proc_name);
  if (proc && !proc->proc_name)
    {
      gsl_glue_free_proc (proc);
      proc = NULL;
    }
  return proc;
}

gchar**
gsl_glue_list_proc_names (void)
{
  GslGlueContext *context = gsl_glue_fetch_context (G_STRLOC);
  gchar **names;

  names = context->table.list_proc_names (context);
  if (!names)
    names = g_new0 (gchar*, 1);
  return names;
}

gchar**
gsl_glue_list_method_names (const gchar *iface_name)
{
  GslGlueContext *context = gsl_glue_fetch_context (G_STRLOC);
  gchar **names;
  
  g_return_val_if_fail (iface_name != NULL, NULL);

  names = context->table.list_method_names (context, iface_name);
  if (!names)
    names = g_new0 (gchar*, 1);
  return names;
}

gchar*
gsl_glue_base_iface (void)
{
  GslGlueContext *context = gsl_glue_fetch_context (G_STRLOC);

  return context->table.base_iface (context);
}

gchar**
gsl_glue_iface_children (const gchar *iface_name)
{
  GslGlueContext *context = gsl_glue_fetch_context (G_STRLOC);
  gchar **names;
  
  g_return_val_if_fail (iface_name != NULL, NULL);

  names = context->table.iface_children (context, iface_name);
  if (!names)
    names = g_new0 (gchar*, 1);
  return names;
}

GslGlueEnum*
gsl_glue_describe_enum (const gchar *enum_name)
{
  GslGlueContext *context = gsl_glue_fetch_context (G_STRLOC);
  GslGlueEnum *e;

  g_return_val_if_fail (enum_name != NULL, NULL);

  e = context->table.describe_enum (context, enum_name);
  if (e && (!e->enum_name || e->n_values == 0))
    {
      gsl_glue_free_enum (e);
      e = NULL;
    }
  return e;
}

gchar*
gsl_glue_proxy_iface (gulong proxy)
{
  GslGlueContext *context = gsl_glue_fetch_context (G_STRLOC);

  return context->table.proxy_iface (context, proxy);
}

GslGlueIFace*
gsl_glue_describe_iface (const gchar *iface_name)
{
  GslGlueContext *context = gsl_glue_fetch_context (G_STRLOC);

  g_return_val_if_fail (iface_name != NULL, NULL);

  return context->table.describe_iface (context, iface_name);
}

GslGlueProp*
gsl_glue_describe_prop (gulong       proxy,
			const gchar *prop_name)
{
  GslGlueContext *context = gsl_glue_fetch_context (G_STRLOC);

  g_return_val_if_fail (prop_name != NULL, NULL);

  return context->table.describe_prop (context, proxy, prop_name);
}

GslGlueCall*
gsl_glue_call_proc (const gchar *proc_name)
{
  GslGlueCall *call;

  g_return_val_if_fail (proc_name != NULL, NULL);

  call = g_new0 (GslGlueCall, 1);
  call->proc_name = g_strdup (proc_name);
  call->params = gsl_glue_rec ();

  return call;
}

void
gsl_glue_call_take_arg (GslGlueCall *call,
			GslGlueValue value)
{
  g_return_if_fail (call != NULL);

  gsl_glue_rec_append (call->params, value);
  gsl_glue_reset_value (&value);
}

void
gsl_glue_call_exec (GslGlueCall *call)
{
  GslGlueContext *context = gsl_glue_fetch_context (G_STRLOC);

  g_return_if_fail (call != NULL);

  if (call->proc_name)
    call->retval = context->table.exec_proc (context, call);
}

GslGlueValue
gsl_glue_client_msg (const gchar *msg,
		     GslGlueValue value)
{
  GslGlueContext *context = gsl_glue_fetch_context (G_STRLOC);

  return context->table.client_msg (context, msg, value);
}


/* --- GlueParam initializers --- */
void
gsl_glue_param_bool (GslGlueParam *param,
		     const gchar  *name,
		     gboolean      dflt)
{
  g_return_if_fail (param != NULL);
  g_return_if_fail (param->glue_type == 0);
  g_return_if_fail (name != NULL);

  param->glue_type = GSL_GLUE_TYPE_BOOL;
  param->pbool.name = g_strdup (name);
  param->pbool.dflt = dflt != FALSE;
}

void
gsl_glue_param_irange (GslGlueParam *param,
		       const gchar  *name,
		       gint          dflt,
		       gint          min,
		       gint          max,
		       gint          stepping)
{
  g_return_if_fail (param != NULL);
  g_return_if_fail (param->glue_type == 0);
  g_return_if_fail (name != NULL);
  g_return_if_fail (dflt >= min && dflt <= max);

  param->glue_type = GSL_GLUE_TYPE_IRANGE;
  param->irange.name = g_strdup (name);
  param->irange.dflt = dflt;
  param->irange.min = min;
  param->irange.max = max;
  param->irange.stepping = stepping;
}

void
gsl_glue_param_frange (GslGlueParam *param,
		       const gchar  *name,
		       gdouble       dflt,
		       gdouble       min,
		       gdouble       max,
		       gdouble       stepping)
{
  g_return_if_fail (param != NULL);
  g_return_if_fail (param->glue_type == 0);
  g_return_if_fail (name != NULL);
  g_return_if_fail (dflt >= min && dflt <= max);

  param->glue_type = GSL_GLUE_TYPE_FRANGE;
  param->frange.name = g_strdup (name);
  param->frange.dflt = dflt;
  param->frange.min = min;
  param->frange.max = max;
  param->frange.stepping = stepping;
}

void
gsl_glue_param_string (GslGlueParam *param,
		       const gchar  *name,
		       const gchar  *dflt)
{
  g_return_if_fail (param != NULL);
  g_return_if_fail (param->glue_type == 0);
  g_return_if_fail (name != NULL);

  param->glue_type = GSL_GLUE_TYPE_STRING;
  param->string.name = g_strdup (name);
  param->string.dflt = g_strdup (dflt);
}

void
gsl_glue_param_enum (GslGlueParam *param,
		     const gchar  *name,
		     const gchar  *enum_name,
		     guint         dflt_index)
{
  g_return_if_fail (param != NULL);
  g_return_if_fail (param->glue_type == 0);
  g_return_if_fail (name != NULL);
  g_return_if_fail (enum_name != NULL);

  param->glue_type = GSL_GLUE_TYPE_ENUM;
  param->penum.name = g_strdup (name);
  param->penum.enum_name = g_strdup (enum_name);
  param->penum.dflt = dflt_index;
}

void
gsl_glue_param_proxy (GslGlueParam *param,
		      const gchar  *name,
		      const gchar  *iface_name)
{
  g_return_if_fail (param != NULL);
  g_return_if_fail (param->glue_type == 0);
  g_return_if_fail (name != NULL);
  g_return_if_fail (iface_name != NULL);

  param->glue_type = GSL_GLUE_TYPE_PROXY;
  param->proxy.name = g_strdup (name);
  param->proxy.iface_name = g_strdup (iface_name);
}

void
gsl_glue_param_rec (GslGlueParam *param,
		    const gchar  *name)
{
  g_return_if_fail (param != NULL);
  g_return_if_fail (param->glue_type == 0);
  g_return_if_fail (name != NULL);

  param->glue_type = GSL_GLUE_TYPE_REC;
  param->rec.name = g_strdup (name);
  param->rec.n_fields = 0;
}


/* --- Sequence --- */
GslGlueSeq*
gsl_glue_seq (GslGlueType element_type)
{
  GslGlueSeq *s;

  g_return_val_if_fail (element_type >= GSL_GLUE_TYPE_FIRST, NULL);
  g_return_val_if_fail (element_type <= GSL_GLUE_TYPE_LAST, NULL);

  s = g_malloc (sizeof (*s));
  s->element_type = element_type;
  s->n_elements = 0;
  s->elements = NULL;

  return s;
}

void
gsl_glue_seq_append (GslGlueSeq  *seq,
		     GslGlueValue value)
{
  g_return_if_fail (seq != NULL);
  g_return_if_fail (seq->element_type == value.glue_type);

  value = gsl_glue_valuedup (value);
  gsl_glue_seq_take_append (seq, &value);
}

void
gsl_glue_seq_take_append (GslGlueSeq   *seq,
			  GslGlueValue *value)
{
  guint i, l, n;

  g_return_if_fail (seq != NULL);
  g_return_if_fail (value != NULL);
  g_return_if_fail (seq->element_type == value->glue_type);

  l = upper_power2 (seq->n_elements);
  i = seq->n_elements++;
  n = upper_power2 (seq->n_elements);
  if (n > l)
    seq->elements = g_realloc (seq->elements, n * sizeof (seq->elements[0]));
  seq->elements[i] = *value;
  memset (value, 0, sizeof (*value));
}

guint
gsl_glue_seq_length (const GslGlueSeq *seq)
{
  return seq ? seq->n_elements : 0;
}

GslGlueValue
gsl_glue_seq_get (const GslGlueSeq *seq,
		  guint             index)
{
  g_return_val_if_fail (seq != NULL, zero_value);
  g_return_val_if_fail (index < seq->n_elements, zero_value);

  return seq->elements[index];
}


/* --- Record --- */
GslGlueRec*
gsl_glue_rec (void)
{
  GslGlueRec *r = g_malloc (sizeof (*r));

  r->n_fields = 0;
  r->fields = NULL;

  return r;
}

void
gsl_glue_rec_take_append (GslGlueRec   *rec,
			  GslGlueValue *value)
{
  guint i, l, n;

  g_return_if_fail (rec != NULL);
  g_return_if_fail (value != NULL);
  g_return_if_fail (value->glue_type >= GSL_GLUE_TYPE_FIRST);
  g_return_if_fail (value->glue_type <= GSL_GLUE_TYPE_LAST);

  l = upper_power2 (rec->n_fields);
  i = rec->n_fields++;
  n = upper_power2 (rec->n_fields);
  if (n > l)
    rec->fields = g_realloc (rec->fields, n * sizeof (rec->fields[0]));
  rec->fields[i] = *value;	/* relocate */
  memset (value, 0, sizeof (*value));
}

void
gsl_glue_rec_append (GslGlueRec  *rec,
		     GslGlueValue value)
{
  g_return_if_fail (rec != NULL);
  g_return_if_fail (value.glue_type >= GSL_GLUE_TYPE_FIRST);
  g_return_if_fail (value.glue_type <= GSL_GLUE_TYPE_LAST);

  value = gsl_glue_valuedup (value);
  gsl_glue_rec_take_append (rec, &value);
}

guint
gsl_glue_rec_n_fields (const GslGlueRec *rec)
{
  return rec ? rec->n_fields : 0;
}

GslGlueValue
gsl_glue_rec_field (const GslGlueRec *rec,
		    guint             index)
{
  g_return_val_if_fail (rec != NULL, zero_value);
  g_return_val_if_fail (index < rec->n_fields, zero_value);

  return rec->fields[index];
}


/* --- Glue utilities --- */
GslGlueValue
gsl_glue_valuedup (const GslGlueValue value)
{
  GslGlueValue dest;

  g_return_val_if_fail (value.glue_type >= GSL_GLUE_TYPE_FIRST, zero_value);
  g_return_val_if_fail (value.glue_type <= GSL_GLUE_TYPE_LAST, zero_value);

  dest = value;
  switch (value.glue_type)
    {
    case GSL_GLUE_TYPE_ENUM:
      dest.value.v_enum.name = g_strdup (value.value.v_enum.name);
      break;
    case GSL_GLUE_TYPE_STRING:
      dest.value.v_string = g_strdup (value.value.v_string);
      break;
    case GSL_GLUE_TYPE_SEQ:
      dest.value.v_seq = gsl_glue_seqdup (value.value.v_seq);
      break;
    case GSL_GLUE_TYPE_REC:
      dest.value.v_rec = gsl_glue_recdup (value.value.v_rec);
      break;
    default:
      break;
    }
  return dest;
}

void
gsl_glue_reset_value (GslGlueValue *value)
{
  switch (value->glue_type)
    {
    case GSL_GLUE_TYPE_ENUM:
      g_free (value->value.v_enum.name);
      break;
    case GSL_GLUE_TYPE_STRING:
      g_free (value->value.v_string);
      break;
    case GSL_GLUE_TYPE_SEQ:
      if (value->value.v_seq)
	gsl_glue_free_seq (value->value.v_seq);
      break;
    case GSL_GLUE_TYPE_REC:
      if (value->value.v_rec)
	gsl_glue_free_rec (value->value.v_rec);
      break;
    default:
      break;
    }
  memset (value, 0, sizeof (*value));
}

GslGlueSeq*
gsl_glue_seqdup (const GslGlueSeq *seq)
{
  if (seq)
    {
      GslGlueSeq *s = gsl_glue_seq (seq->element_type);
      guint i;

      for (i = 0; i < seq->n_elements; i++)
	gsl_glue_seq_append (s, seq->elements[i]);

      return s;
    }
  else
    return NULL;
}

GslGlueRec*
gsl_glue_recdup (const GslGlueRec *rec)
{
  if (rec)
    {
      GslGlueRec *r = gsl_glue_rec ();
      guint i;

      for (i = 0; i < rec->n_fields; i++)
	gsl_glue_rec_append (r, rec->fields[i]);

      return r;
    }
  else
    return NULL;
}

void
gsl_glue_free_enum (GslGlueEnum *e)
{
  g_return_if_fail (e != NULL);

  g_free (e->enum_name);
  g_strfreev (e->values);
  g_strfreev (e->blurbs);
  g_free (e);
}

void
gsl_glue_free_prop (GslGlueProp *prop)
{
  g_return_if_fail (prop != NULL);

  gsl_glue_reset_param (&prop->param);
  g_free (prop->group);
  g_free (prop->pretty_name);
  g_free (prop->blurb);
  g_free (prop);
}

void
gsl_glue_free_proc (GslGlueProc *proc)
{
  guint i;

  g_return_if_fail (proc != NULL);

  if (proc->ret_param)
    {
      gsl_glue_reset_param (proc->ret_param);
      g_free (proc->ret_param);
    }
  for (i = 0; i < proc->n_params; i++)
    gsl_glue_reset_param (proc->params + i);
  g_free (proc->params);
  g_free (proc->proc_name);
  g_free (proc);
}

void
gsl_glue_free_iface (GslGlueIFace *iface)
{
  g_return_if_fail (iface != NULL);

  g_free (iface->type_name);
  g_strfreev (iface->ifaces);
  g_strfreev (iface->props);
  g_strfreev (iface->signals);
  g_free (iface);
}

void
gsl_glue_free_call (GslGlueCall *call)
{
  g_return_if_fail (call != NULL);

  g_free (call->proc_name);
  if (call->params)
    gsl_glue_free_rec (call->params);
  gsl_glue_reset_value (&call->retval);
  g_free (call);
}

void
gsl_glue_free_seq (GslGlueSeq *seq)
{
  guint i;

  g_return_if_fail (seq != NULL);

  for (i = 0; i < seq->n_elements; i++)
    gsl_glue_reset_value (seq->elements + i);
  g_free (seq->elements);
  g_free (seq);
}

void
gsl_glue_free_rec (GslGlueRec *rec)
{
  guint i;

  g_return_if_fail (rec != NULL);

  for (i = 0; i < rec->n_fields; i++)
    gsl_glue_reset_value (rec->fields + i);
  g_free (rec->fields);
  g_free (rec);
}

void
gsl_glue_reset_param (GslGlueParam *param)
{
  g_return_if_fail (param != NULL);

  g_free (param->any.name);
  switch (param->glue_type)
    {
      guint i;
    case GSL_GLUE_TYPE_NONE:
    case GSL_GLUE_TYPE_BOOL:
    case GSL_GLUE_TYPE_IRANGE:
    case GSL_GLUE_TYPE_FRANGE:
      break;
    case GSL_GLUE_TYPE_STRING:
      g_free (param->string.dflt);
      break;
    case GSL_GLUE_TYPE_ENUM:
      g_free (param->penum.enum_name);
      break;
    case GSL_GLUE_TYPE_PROXY:
      g_free (param->proxy.iface_name);
      break;
    case GSL_GLUE_TYPE_SEQ:
      if (param->seq.elements)
	{
	  gsl_glue_reset_param (param->seq.elements);
	  g_free (param->seq.elements);
	}
      break;
    case GSL_GLUE_TYPE_REC:
      for (i = 0; i < param->rec.n_fields; i++)
	gsl_glue_reset_param (param->rec.fields + i);
      g_free (param->rec.fields);
      break;
    }
  memset (param, 0, sizeof (*param));
}
