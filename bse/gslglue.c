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



/* --- prototype --- */
static void		glue_dup_value		(const GslGlueValue	*src_value,
						 GslGlueValue		*dest_value);
static void		glue_copy_value		(const GslGlueValue	*src_value,
						 GslGlueValue		*dest_value);
static void		glue_unset_value	(GslGlueValue		*value);
static GHashTable*	glue_gc_hash_table_new	(void);


/* --- variables --- */
static GslRing    *context_stack = NULL;
static GHashTable *context_gc_hash = NULL;


/* --- functions --- */
void
gsl_glue_context_common_init (GslGlueContext            *context,
			      const GslGlueContextTable *vtable)
{
  g_return_if_fail (context->sighash == NULL);

  if (!context_gc_hash)
    context_gc_hash = glue_gc_hash_table_new ();

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
      gsl_glue_gc_free_now (proc, _gsl_glue_proc_free);
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
  gsl_glue_gc_add (names, g_strfreev);
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
  gsl_glue_gc_add (names, g_strfreev);
  return names;
}

gchar*
gsl_glue_base_iface (void)
{
  GslGlueContext *context = gsl_glue_fetch_context (G_STRLOC);
  gchar *biface = context->table.base_iface (context);

  if (biface)
    gsl_glue_gc_add (biface, g_free);
  return biface;
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
  gsl_glue_gc_add (names, g_strfreev);
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
      gsl_glue_gc_free_now (e, _gsl_glue_enum_free);
      e = NULL;
    }
  return e;
}

gchar*
gsl_glue_proxy_iface (gulong proxy)
{
  GslGlueContext *context = gsl_glue_fetch_context (G_STRLOC);
  gchar *iface = context->table.proxy_iface (context, proxy);

  if (iface)
    gsl_glue_gc_add (iface, g_free);
  return iface;
}

GslGlueIFace*
gsl_glue_describe_iface (const gchar *iface_name)
{
  GslGlueContext *context = gsl_glue_fetch_context (G_STRLOC);
  GslGlueIFace *iface;

  g_return_val_if_fail (iface_name != NULL, NULL);

  iface = context->table.describe_iface (context, iface_name);
  return iface;
}

GslGlueProp*
gsl_glue_describe_prop (gulong       proxy,
			const gchar *prop_name)
{
  GslGlueContext *context = gsl_glue_fetch_context (G_STRLOC);
  GslGlueProp *prop;

  g_return_val_if_fail (prop_name != NULL, NULL);

  prop = context->table.describe_prop (context, proxy, prop_name);
  return prop;
}

GslGlueCall*
gsl_glue_call_proc (const gchar *proc_name)
{
  GslGlueCall *call;

  g_return_val_if_fail (proc_name != NULL, NULL);

  call = g_new0 (GslGlueCall, 1);
  gsl_glue_gc_add (call, _gsl_glue_call_free);
  call->proc_name = g_strdup (proc_name);
  call->params = gsl_glue_seq ();
  gsl_glue_gc_remove (call->params, gsl_glue_seq_unref);
  call->ret_value = NULL; /* special case, filled upon exec */

  return call;
}

GslGlueCall*
_gsl_glue_call_proc_seq (const gchar *proc_name,
			 GslGlueSeq  *params)
{
  GslGlueCall *call;
  
  g_return_val_if_fail (proc_name != NULL, NULL);
  
  call = g_new0 (GslGlueCall, 1);
  gsl_glue_gc_add (call, _gsl_glue_call_free);
  call->proc_name = g_strdup (proc_name);
  call->params = gsl_glue_seq_ref (params);
  call->ret_value = NULL; /* special case, filled upon exec */
  
  return call;
}

void
gsl_glue_call_add_arg (GslGlueCall        *call,
		       const GslGlueValue *value)
{
  g_return_if_fail (call != NULL);
  g_return_if_fail (value != NULL);

  gsl_glue_seq_append (call->params, value);
}

void
gsl_glue_call_exec (GslGlueCall *call)
{
  GslGlueContext *context = gsl_glue_fetch_context (G_STRLOC);

  g_return_if_fail (call != NULL);
  g_return_if_fail (call->ret_value == NULL);

  if (call->proc_name)
    {
      call->ret_value = context->table.exec_proc (context, call);
      if (!call->ret_value)
	call->ret_value = gsl_glue_value_inval ();
      gsl_glue_gc_remove (call->ret_value, _gsl_glue_value_free);
    }
}

GslGlueValue*
gsl_glue_client_msg (const gchar  *msg,
		     GslGlueValue *value)
{
  GslGlueContext *context = gsl_glue_fetch_context (G_STRLOC);

  value = context->table.client_msg (context, msg, value);
  if (!value)
    value = gsl_glue_value_inval ();
  return value;
}


/* --- GlueParam initializers --- */
static inline GslGlueParam*
gc_alloc_param (GslGlueType type)
{
  GslGlueParam *p = g_new0 (GslGlueParam, 1);
  gsl_glue_gc_add (p, _gsl_glue_param_free);
  p->glue_type = type;
  return p;
}

GslGlueParam*
_gsl_glue_param_inval (void)
{
  return gc_alloc_param (GSL_GLUE_TYPE_NONE);
}

GslGlueParam*
gsl_glue_param_bool (const gchar  *name,
		     gboolean      dflt)
{
  GslGlueParam *param;

  g_return_val_if_fail (name != NULL, NULL);

  param = gc_alloc_param (GSL_GLUE_TYPE_BOOL);
  param->pbool.name = g_strdup (name);
  param->pbool.dflt = dflt != FALSE;

  return param;
}

GslGlueParam*
gsl_glue_param_irange (const gchar  *name,
		       gint          dflt,
		       gint          min,
		       gint          max,
		       gint          stepping)
{
  GslGlueParam *param;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (dflt >= min && dflt <= max, NULL);

  param = gc_alloc_param (GSL_GLUE_TYPE_IRANGE);
  param->irange.name = g_strdup (name);
  param->irange.dflt = dflt;
  param->irange.min = min;
  param->irange.max = max;
  param->irange.stepping = stepping;

  return param;
}

GslGlueParam*
gsl_glue_param_frange (const gchar  *name,
		       gdouble       dflt,
		       gdouble       min,
		       gdouble       max,
		       gdouble       stepping)
{
  GslGlueParam *param;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (dflt >= min && dflt <= max, NULL);

  param = gc_alloc_param (GSL_GLUE_TYPE_FRANGE);
  param->frange.name = g_strdup (name);
  param->frange.dflt = dflt;
  param->frange.min = min;
  param->frange.max = max;
  param->frange.stepping = stepping;

  return param;
}

GslGlueParam*
gsl_glue_param_string (const gchar *name,
		       const gchar *dflt)
{
  GslGlueParam *param;

  g_return_val_if_fail (name != NULL, NULL);

  param = gc_alloc_param (GSL_GLUE_TYPE_STRING);
  param->string.name = g_strdup (name);
  param->string.dflt = g_strdup (dflt);

  return param;
}

GslGlueParam*
gsl_glue_param_enum (const gchar *name,
		     const gchar *enum_name,
		     guint        dflt_index)
{
  GslGlueParam *param;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (enum_name != NULL, NULL);

  param = gc_alloc_param (GSL_GLUE_TYPE_ENUM);
  param->penum.name = g_strdup (name);
  param->penum.enum_name = g_strdup (enum_name);
  param->penum.dflt = dflt_index;

  return param;
}

GslGlueParam*
gsl_glue_param_proxy (const gchar  *name,
		      const gchar  *iface_name)
{
  GslGlueParam *param;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (iface_name != NULL, NULL);

  param = gc_alloc_param (GSL_GLUE_TYPE_PROXY);
  param->proxy.name = g_strdup (name);
  param->proxy.iface_name = g_strdup (iface_name);

  return param;
}

GslGlueParam*
gsl_glue_param_seq (const gchar *name)
{
  GslGlueParam *param;

  g_return_val_if_fail (name != NULL, NULL);

  param = gc_alloc_param (GSL_GLUE_TYPE_SEQ);
  param->seq.name = g_strdup (name);

  return param;
}

GslGlueParam*
gsl_glue_param_rec (const gchar  *name)
{
  GslGlueParam *param;

  g_return_val_if_fail (name != NULL, NULL);

  param = gc_alloc_param (GSL_GLUE_TYPE_REC);
  param->rec.name = g_strdup (name);

  return param;
}


/* --- Sequence --- */
GslGlueSeq*
gsl_glue_seq (void)
{
  GslGlueSeq *s;

  s = g_new (GslGlueSeq, 1);
  gsl_glue_gc_add (s, gsl_glue_seq_unref);
  s->ref_count = 1;
  s->n_elements = 0;
  s->elements = NULL;

  return s;
}

GslGlueSeq*
gsl_glue_seq_ref (GslGlueSeq *seq)
{
  g_return_val_if_fail (seq != NULL, NULL);
  g_return_val_if_fail (seq->ref_count > 0, NULL);

  seq->ref_count++;

  return seq;
}

void
gsl_glue_seq_unref (GslGlueSeq *seq)
{
  g_return_if_fail (seq != NULL);
  g_return_if_fail (seq->ref_count > 0);

  seq->ref_count--;
  if (seq->ref_count == 0)
    {
      guint i;
      for (i = 0; i < seq->n_elements; i++)
	glue_unset_value (seq->elements + i);
      g_free (seq->elements);
      g_free (seq);
    }
}

static void
glue_seq_append (GslGlueSeq         *seq,
		 const GslGlueValue *value,
		 gboolean            deep_copy)
{
  guint i, l, n;

  g_return_if_fail (seq != NULL);
  g_return_if_fail (value != NULL);

  l = upper_power2 (seq->n_elements);
  i = seq->n_elements++;
  n = upper_power2 (seq->n_elements);
  if (n > l)
    seq->elements = g_realloc (seq->elements, n * sizeof (seq->elements[0]));
  if (deep_copy)
    glue_copy_value (value, seq->elements + i);
  else
    glue_dup_value (value, seq->elements + i);
}

GslGlueSeq*
gsl_glue_seq_copy (const GslGlueSeq *seq)
{
  GslGlueSeq *s;
  guint i;

  g_return_val_if_fail (seq != NULL, NULL);
  g_return_val_if_fail (seq->ref_count > 0, NULL);

  s = gsl_glue_seq ();
  for (i = 0; i < seq->n_elements; i++)
    glue_seq_append (s, seq->elements + i, TRUE);
  return s;
}

void
gsl_glue_seq_append (GslGlueSeq         *seq,
		     const GslGlueValue *value)
{
  g_return_if_fail (seq != NULL);
  g_return_if_fail (value != NULL);

  glue_seq_append (seq, value, FALSE);
}

guint
gsl_glue_seq_length (const GslGlueSeq *seq)
{
  return seq ? seq->n_elements : 0;
}

GslGlueValue*
gsl_glue_seq_get (const GslGlueSeq *seq,
		  guint             index)
{
  g_return_val_if_fail (seq != NULL, NULL);
  g_return_val_if_fail (index < seq->n_elements, NULL);

  return gsl_glue_value_dup (seq->elements + index);
}

gboolean
gsl_glue_seq_check_elements (GslGlueSeq *seq,
			     GslGlueType element_type)
{
  guint i;

  g_return_val_if_fail (seq != NULL, FALSE);

  for (i = 0; i < seq->n_elements; i++)
    if (seq->elements[i].glue_type != element_type)
      return FALSE;
  return TRUE;
}


/* --- Record --- */
GslGlueRec*
gsl_glue_rec (void)
{
  GslGlueRec *r = g_new (GslGlueRec, 1);
  gsl_glue_gc_add (r, gsl_glue_rec_unref);
  r->n_fields = 0;
  r->ref_count = 1;
  r->fields = NULL;
  r->field_names = NULL;

  return r;
}

GslGlueRec*
gsl_glue_rec_ref (GslGlueRec *rec)
{
  g_return_val_if_fail (rec != NULL, NULL);
  g_return_val_if_fail (rec->ref_count > 0, NULL);

  rec->ref_count++;

  return rec;
}

void
gsl_glue_rec_unref (GslGlueRec *rec)
{
  g_return_if_fail (rec != NULL);
  g_return_if_fail (rec->ref_count > 0);

  rec->ref_count--;
  if (rec->ref_count == 0)
    {
      guint i;
      for (i = 0; i < rec->n_fields; i++)
	{
	  glue_unset_value (rec->fields + i);
	  g_free (rec->field_names[i]);
	}
      g_free (rec->fields);
      g_free (rec->field_names);
      g_free (rec);
    }
}

static inline gchar*
dupcanon (const gchar *field_name)
{
  return g_strcanon (g_strdup (field_name),
		     G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS,
		     '-');
}

static void
glue_rec_set (GslGlueRec         *rec,
	      const gchar        *field_name,
	      const GslGlueValue *value,
	      gboolean            deep_copy)
{
  gchar *name;
  guint i;

  name = dupcanon (field_name);
  for (i = 0; i < rec->n_fields; i++)
    if (strcmp (name, rec->field_names[i]) == 0)
      break;
  if (i >= rec->n_fields)
    {
      i = rec->n_fields++;
      rec->fields = g_realloc (rec->fields, rec->n_fields * sizeof (rec->fields[0]));
      rec->field_names = g_realloc (rec->field_names, rec->n_fields * sizeof (rec->field_names[0]));
      rec->field_names[i] = name;
    }
  else
    {
      glue_unset_value (rec->fields + i);
      g_free (name);
    }
  if (deep_copy)
    glue_copy_value (value, rec->fields + i);
  else
    glue_dup_value (value, rec->fields + i);
}

void
gsl_glue_rec_set (GslGlueRec         *rec,
	          const gchar        *field_name,
	          const GslGlueValue *value)
{
  g_return_if_fail (rec != NULL);
  g_return_if_fail (field_name != NULL);
  g_return_if_fail (value != NULL);
  g_return_if_fail (value->glue_type >= GSL_GLUE_TYPE_FIRST);
  g_return_if_fail (value->glue_type <= GSL_GLUE_TYPE_LAST);

  glue_rec_set (rec, field_name, value, FALSE);
}

guint
gsl_glue_rec_n_fields (const GslGlueRec *rec)
{
  return rec ? rec->n_fields : 0;
}

GslGlueValue*
gsl_glue_rec_field (const GslGlueRec *rec,
		    guint             index)
{
  g_return_val_if_fail (rec != NULL, NULL);
  g_return_val_if_fail (index < rec->n_fields, NULL);

  return gsl_glue_value_dup (rec->fields + index);
}

GslGlueValue*
gsl_glue_rec_get (const GslGlueRec *rec,
		  const gchar      *field_name)
{
  gchar *name;
  guint i;

  g_return_val_if_fail (rec != NULL, NULL);
  g_return_val_if_fail (field_name != NULL, NULL);

  name = dupcanon (field_name);
  for (i = 0; i < rec->n_fields; i++)
    if (strcmp (name, rec->field_names[i]) == 0)
      break;
  if (i < rec->n_fields)
    {
      g_free (name);
      return gsl_glue_value_dup (rec->fields + i);
    }
  g_warning ("%s: record (%p) has no field named \"%s\"", G_STRLOC, rec, name);
  g_free (name);
  return gsl_glue_value_inval ();
}

GslGlueRec*
gsl_glue_rec_copy (const GslGlueRec *rec)
{
  GslGlueRec *r;
  guint i;

  g_return_val_if_fail (rec != NULL, NULL);
  g_return_val_if_fail (rec->ref_count > 0, NULL);

  r = gsl_glue_rec ();
  for (i = 0; i < rec->n_fields; i++)
    glue_rec_set (r, rec->field_names[i], &rec->fields[i], TRUE);
  return r;
}


/* --- values --- */
static inline GslGlueValue*
gc_alloc_value (GslGlueType type)
{
  GslGlueValue *v = g_new0 (GslGlueValue, 1);
  gsl_glue_gc_add (v, _gsl_glue_value_free);
  v->glue_type = type;
  return v;
}

GslGlueValue*
gsl_glue_value_inval (void)
{
  return gc_alloc_value (GSL_GLUE_TYPE_NONE);
}

GslGlueValue*
gsl_glue_value_bool (gboolean bool_value)
{
  GslGlueValue *v = gc_alloc_value (GSL_GLUE_TYPE_BOOL);
  v->value.v_bool = bool_value != FALSE;
  return v;
}

GslGlueValue*
gsl_glue_value_int (gint int_value)
{
  GslGlueValue *v = gc_alloc_value (GSL_GLUE_TYPE_IRANGE);
  v->value.v_int = int_value;
  return v;
}

GslGlueValue*
gsl_glue_value_float (gdouble float_value)
{
  GslGlueValue *v = gc_alloc_value (GSL_GLUE_TYPE_FRANGE);
  v->value.v_float = float_value;
  return v;
}

GslGlueValue*
gsl_glue_value_string (const gchar *string_value)
{
  GslGlueValue *v = gc_alloc_value (GSL_GLUE_TYPE_STRING);
  v->value.v_string = g_strdup (string_value);
  return v;
}

GslGlueValue*
gsl_glue_value_take_string (gchar *string_value)
{
  GslGlueValue *v = gc_alloc_value (GSL_GLUE_TYPE_STRING);
  v->value.v_string = string_value;
  return v;
}

GslGlueValue*
gsl_glue_value_stringl (const gchar *string_value,
			guint        string_length)
{
  GslGlueValue *v = gc_alloc_value (GSL_GLUE_TYPE_STRING);
  v->value.v_string = g_strndup (string_value, string_length);
  return v;
}

GslGlueValue*
gsl_glue_value_enum (const gchar *enum_name,
		     gint         enum_index)
{
  GslGlueValue *v = gc_alloc_value (GSL_GLUE_TYPE_ENUM);
  v->value.v_enum.name = g_strdup (enum_name);
  v->value.v_enum.index = enum_index;
  return v;
}

GslGlueValue*
gsl_glue_value_proxy (gulong proxy)
{
  GslGlueValue *v = gc_alloc_value (GSL_GLUE_TYPE_PROXY);
  v->value.v_proxy = proxy;
  return v;
}

GslGlueValue*
gsl_glue_value_seq (GslGlueSeq *seq)
{
  GslGlueValue *v = gc_alloc_value (GSL_GLUE_TYPE_SEQ);
  v->value.v_seq = gsl_glue_seq_ref (seq);
  return v;
}

GslGlueValue*
gsl_glue_value_rec (GslGlueRec *rec)
{
  GslGlueValue *v = gc_alloc_value (GSL_GLUE_TYPE_REC);
  v->value.v_rec = gsl_glue_rec_ref (rec);
  return v;
}

static void
glue_dup_value (const GslGlueValue *src_value,
		GslGlueValue       *dest_value)
{
  g_return_if_fail (src_value->glue_type >= GSL_GLUE_TYPE_FIRST);
  g_return_if_fail (src_value->glue_type <= GSL_GLUE_TYPE_LAST);

  *dest_value = *src_value;
  switch (src_value->glue_type)
    {
    case GSL_GLUE_TYPE_ENUM:
      dest_value->value.v_enum.name = g_strdup (src_value->value.v_enum.name);
      break;
    case GSL_GLUE_TYPE_STRING:
      dest_value->value.v_string = g_strdup (src_value->value.v_string);
      break;
    case GSL_GLUE_TYPE_SEQ:
      dest_value->value.v_seq = gsl_glue_seq_ref (src_value->value.v_seq);
      break;
    case GSL_GLUE_TYPE_REC:
      dest_value->value.v_rec = gsl_glue_rec_ref (src_value->value.v_rec);
      break;
    default:
      break;
    }
}

static void
glue_copy_value (const GslGlueValue *src_value,
		 GslGlueValue       *dest_value)
{
  g_return_if_fail (src_value->glue_type >= GSL_GLUE_TYPE_FIRST);
  g_return_if_fail (src_value->glue_type <= GSL_GLUE_TYPE_LAST);

  switch (src_value->glue_type)
    {
    case GSL_GLUE_TYPE_SEQ:
      *dest_value = *src_value;
      dest_value->value.v_seq = gsl_glue_seq_ref (gsl_glue_seq_copy (src_value->value.v_seq));
      break;
    case GSL_GLUE_TYPE_REC:
      *dest_value = *src_value;
      dest_value->value.v_rec = gsl_glue_rec_ref (gsl_glue_rec_copy (src_value->value.v_rec));
      break;
    default:
      glue_dup_value (src_value, dest_value);
      break;
    }
}

static void
glue_unset_value (GslGlueValue *value)
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
	gsl_glue_seq_unref (value->value.v_seq);
      break;
    case GSL_GLUE_TYPE_REC:
      if (value->value.v_rec)
	gsl_glue_rec_unref (value->value.v_rec);
      break;
    default:
      break;
    }
}

GslGlueValue*
gsl_glue_value_dup (const GslGlueValue *src_value)
{
  GslGlueValue *dest_value;

  g_return_val_if_fail (src_value != NULL, NULL);

  dest_value = gc_alloc_value (GSL_GLUE_TYPE_NONE);
  glue_dup_value (src_value, dest_value);
  return dest_value;
}

GslGlueValue*
gsl_glue_value_copy (const GslGlueValue *src_value)
{
  GslGlueValue *dest_value;

  g_return_val_if_fail (src_value != NULL, NULL);

  dest_value = gc_alloc_value (GSL_GLUE_TYPE_NONE);
  glue_copy_value (src_value, dest_value);
  return dest_value;
}

void
_gsl_glue_value_free (GslGlueValue *value)
{
  g_return_if_fail (value != NULL);
  g_return_if_fail (_gsl_glue_gc_test (value, _gsl_glue_value_free) == FALSE);

  glue_unset_value (value);
  g_free (value);
}


/* --- Glue utilities --- */
GslGlueEnum*
_gsl_glue_enum (const gchar *enum_name)
{
  GslGlueEnum *e;

  g_return_val_if_fail (enum_name != NULL, NULL);

  e = g_new0 (GslGlueEnum, 1);
  gsl_glue_gc_add (e, _gsl_glue_enum_free);
  e->enum_name = g_strdup (enum_name);
  e->n_values = 0;
  e->values = NULL;
  e->blurbs = NULL;

  return e;
}

void
_gsl_glue_enum_free (GslGlueEnum *e)
{
  g_return_if_fail (e != NULL);
  g_return_if_fail (_gsl_glue_gc_test (e, _gsl_glue_enum_free) == FALSE);
  
  g_free (e->enum_name);
  g_strfreev (e->values);
  g_strfreev (e->blurbs);
  g_free (e);
}

GslGlueIFace*
_gsl_glue_iface (const gchar *iface_name)
{
  GslGlueIFace *iface;

  g_return_val_if_fail (iface_name != NULL, NULL);

  iface = g_new0 (GslGlueIFace, 1);
  gsl_glue_gc_add (iface, _gsl_glue_iface_free);
  iface->type_name = g_strdup (iface_name);
  iface->n_ifaces = 0;
  iface->ifaces = NULL;
  iface->n_props = 0;
  iface->props = NULL;
  iface->n_signals = 0;
  iface->signals = NULL;

  return iface;
}

void
_gsl_glue_iface_free (GslGlueIFace *iface)
{
  g_return_if_fail (iface != NULL);
  g_return_if_fail (_gsl_glue_gc_test (iface, _gsl_glue_iface_free) == FALSE);
  
  g_free (iface->type_name);
  g_strfreev (iface->ifaces);
  g_strfreev (iface->props);
  g_strfreev (iface->signals);
  g_free (iface);
}

GslGlueProp*
_gsl_glue_prop (void)
{
  GslGlueProp *p;

  p = g_new0 (GslGlueProp, 1);
  gsl_glue_gc_add (p, _gsl_glue_prop_free);
  return p;
}

void
_gsl_glue_prop_take_param (GslGlueProp  *prop,
			   GslGlueParam *param)
{
  g_return_if_fail (prop != NULL);
  g_return_if_fail (param != NULL);
  g_return_if_fail (prop->param == NULL);

  prop->param = param;
  gsl_glue_gc_remove (param, _gsl_glue_param_free);
}

void
_gsl_glue_prop_free (GslGlueProp *prop)
{
  g_return_if_fail (prop != NULL);
  g_return_if_fail (_gsl_glue_gc_test (prop, _gsl_glue_prop_free) == FALSE);
  
  _gsl_glue_param_free (prop->param);
  g_free (prop->group);
  g_free (prop->pretty_name);
  g_free (prop->blurb);
  g_free (prop);
}

GslGlueProc*
_gsl_glue_proc (void)
{
  GslGlueProc *p;

  p = g_new0 (GslGlueProc, 1);
  gsl_glue_gc_add (p, _gsl_glue_proc_free);
  p->proc_name = NULL;
  p->ret_param = _gsl_glue_param_inval ();
  gsl_glue_gc_remove (p->ret_param, _gsl_glue_param_free);
  p->n_params = 0;
  p->params = NULL;
  return p;
}

void
_gsl_glue_proc_take_param (GslGlueProc  *proc,
			   GslGlueParam *param)
{
  guint i;

  g_return_if_fail (proc != NULL);
  g_return_if_fail (param != NULL);

  i = proc->n_params++;
  proc->params = g_renew (GslGlueParam*, proc->params, proc->n_params);
  proc->params[i] = param;
  gsl_glue_gc_remove (param, _gsl_glue_param_free);
}

void
_gsl_glue_proc_take_ret_param (GslGlueProc  *proc,
			       GslGlueParam *param)
{
  g_return_if_fail (proc != NULL);
  g_return_if_fail (param != NULL);
  g_return_if_fail (proc->ret_param->glue_type == GSL_GLUE_TYPE_NONE);

  _gsl_glue_param_free (proc->ret_param);
  proc->ret_param = param;
  gsl_glue_gc_remove (param, _gsl_glue_param_free);
}

void
_gsl_glue_proc_free (GslGlueProc *proc)
{
  guint i;

  g_return_if_fail (proc != NULL);
  g_return_if_fail (_gsl_glue_gc_test (proc, _gsl_glue_proc_free) == FALSE);
  
  _gsl_glue_param_free (proc->ret_param);
  for (i = 0; i < proc->n_params; i++)
    _gsl_glue_param_free (proc->params[i]);
  g_free (proc->params);
  g_free (proc->proc_name);
  g_free (proc);
}

void
_gsl_glue_call_free (GslGlueCall *call)
{
  g_return_if_fail (call != NULL);
  g_return_if_fail (_gsl_glue_gc_test (call, _gsl_glue_call_free) == FALSE);
  
  g_free (call->proc_name);
  if (call->params)
    gsl_glue_seq_unref (call->params);
  if (call->ret_value)
    _gsl_glue_value_free (call->ret_value);
  g_free (call);
}

void
_gsl_glue_param_free (GslGlueParam *param)
{
  g_return_if_fail (param != NULL);
  g_return_if_fail (_gsl_glue_gc_test (param, _gsl_glue_param_free) == FALSE);
  
  g_free (param->any.name);
  switch (param->glue_type)
    {
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
#if 0
      if (param->seq.elements)
	_gsl_glue_param_free (param->seq.elements);
#endif
      break;
    case GSL_GLUE_TYPE_REC:
#if 0
      for (i = 0; i < param->rec.n_fields; i++)
	_gsl_glue_param_free (param->rec.fields + i);
      g_free (param->rec.fields);
#endif
      break;
    }
  g_free (param);
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
gsl_glue_gc_add (gpointer data,
		 gpointer free_func)
{
  GcEntry *entry;

  g_return_if_fail (free_func != NULL);
  g_return_if_fail (_gsl_glue_gc_test (data, free_func) == FALSE);

  entry = g_new (GcEntry, 1);
  entry->data = data;
  entry->free_func = free_func;
  g_hash_table_insert (context_gc_hash, entry, entry);
}

gboolean
_gsl_glue_gc_test (gpointer data,
		   gpointer free_func)
{
  GcEntry key;

  g_return_val_if_fail (free_func != NULL, FALSE);

  key.data = data;
  key.free_func = free_func;

  return g_hash_table_lookup (context_gc_hash, &key) != NULL;
}

void
gsl_glue_gc_remove (gpointer data,
		    gpointer free_func)
{
  GcEntry key;

  g_return_if_fail (free_func != NULL);
  g_return_if_fail (_gsl_glue_gc_test (data, free_func) == TRUE);
  
  key.data = data;
  key.free_func = free_func;
  g_hash_table_remove (context_gc_hash, &key);
}

void
gsl_glue_gc_free_now (gpointer data,
		      gpointer _free_func)
{
  void (*free_func) (gpointer data) = _free_func;

  g_return_if_fail (free_func != NULL);
  g_return_if_fail (_gsl_glue_gc_test (data, free_func) == TRUE);

  gsl_glue_gc_remove (data, free_func);
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
gsl_glue_gc_run (void)
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
gsl_glue_gc_collect_value (GslGlueValue *value)
{
  g_return_if_fail (value != NULL);

  gsl_glue_gc_free_now (value, _gsl_glue_value_free);
}

void
gsl_glue_gc_collect_seq (GslGlueSeq *seq)
{
  g_return_if_fail (seq != NULL);
  
  gsl_glue_gc_free_now (seq, gsl_glue_seq_unref);
}

void
gsl_glue_gc_collect_rec (GslGlueRec *rec)
{
  g_return_if_fail (rec != NULL);
  
  gsl_glue_gc_free_now (rec, gsl_glue_rec_unref);
}

void
gsl_glue_gc_collect_enum (GslGlueEnum *penum)
{
  g_return_if_fail (penum != NULL);
  
  gsl_glue_gc_free_now (penum, _gsl_glue_enum_free);
}

void
gsl_glue_gc_collect_iface (GslGlueIFace *iface)
{
  g_return_if_fail (iface != NULL);
  
  gsl_glue_gc_free_now (iface, _gsl_glue_iface_free);
}

void
gsl_glue_gc_collect_prop (GslGlueProp *prop)
{
  g_return_if_fail (prop != NULL);
  
  gsl_glue_gc_free_now (prop, _gsl_glue_prop_free);
}

void
gsl_glue_gc_collect_call (GslGlueCall *call)
{
  g_return_if_fail (call != NULL);

  gsl_glue_gc_free_now (call, _gsl_glue_call_free);
}

void
gsl_glue_gc_collect_proc (GslGlueProc *proc)
{
  g_return_if_fail (proc != NULL);

  gsl_glue_gc_free_now (proc, _gsl_glue_proc_free);
}

void
gsl_glue_gc_collect_param (GslGlueParam *param)
{
  g_return_if_fail (param != NULL);

  gsl_glue_gc_free_now (param, _gsl_glue_param_free);
}


/* vim:set ts=8 sts=2 sw=2: */
