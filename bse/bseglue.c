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
#include "bseglue.h"
#include "bse.h"
#include <string.h>


/* --- prototypes --- */
static GslGlueEnum*     bglue_describe_enum               (GslGlueContext *context,
                                                           const gchar    *enum_name);
static GslGlueIFace*    bglue_describe_iface              (GslGlueContext *context,
                                                           const gchar    *iface);
static GslGlueProp*     bglue_describe_prop               (GslGlueContext *context,
                                                           gulong          proxy,
                                                           const gchar    *prop_name);
static GslGlueProc*     bglue_describe_proc               (GslGlueContext *context,
                                                           const gchar    *proc_name);
static gchar**          bglue_list_proc_names             (GslGlueContext *context);
static gchar**          bglue_list_method_names           (GslGlueContext *context,
                                                           const gchar    *iface_name);
static gchar*           bglue_base_iface                  (GslGlueContext *context);
static gchar**          bglue_iface_children              (GslGlueContext *context,
                                                           const gchar    *iface_name);
static gchar*           bglue_proxy_iface                 (GslGlueContext *context,
                                                           gulong          proxy);
static GslGlueValue     bglue_exec_proc                   (GslGlueContext *context,
                                                           GslGlueCall    *proc_call);
static gboolean         bglue_signal_connection           (GslGlueContext *context,
                                                           const gchar    *signal,
                                                           gulong          proxy,
                                                           gboolean        enable_connection);
static GslGlueValue     bglue_client_msg                  (GslGlueContext *context,
                                                           const gchar    *msg,
                                                           GslGlueValue    value);


/* --- functions --- */
GType
bse_glue_make_rorecord (const gchar      *rec_name,
			GBoxedCopyFunc    copy,
			GBoxedFreeFunc    free,
			BseGlueBoxedToRec to_record)
{
  GType type;

  type = g_boxed_type_register_static (rec_name, copy, free);
  g_type_set_qdata (type, g_quark_from_string ("BseGlueBoxedToRec"), to_record);

  return type;
}

GType
bse_glue_make_rosequence (const gchar      *seq_name,
			  GBoxedCopyFunc    copy,
			  GBoxedFreeFunc    free,
			  BseGlueBoxedToSeq to_sequence)
{
  GType type;

  type = g_boxed_type_register_static (seq_name, copy, free);
  g_type_set_qdata (type, g_quark_from_string ("BseGlueBoxedToSeq"), to_sequence);

  return type;
}

GslGlueValue
bse_glue_boxed_to_value (GType    boxed_type,
			 gpointer boxed)
{
  BseGlueBoxedToRec b2rec;
  BseGlueBoxedToSeq b2seq;
  GslGlueValue zero_value = { 0, };
  
  g_return_val_if_fail (G_TYPE_IS_BOXED (boxed_type) && G_TYPE_IS_DERIVED (boxed_type), zero_value);
  g_return_val_if_fail (boxed != NULL, zero_value);

  b2rec = g_type_get_qdata (boxed_type, g_quark_from_string ("BseGlueBoxedToRec"));
  b2seq = g_type_get_qdata (boxed_type, g_quark_from_string ("BseGlueBoxedToSeq"));
  if (b2rec)
    return gsl_glue_value_take_rec (b2rec (boxed));
  else if (b2seq)
    return gsl_glue_value_take_seq (b2seq (boxed));
  else /* urm, bad */
    {
      g_warning ("unable to convert boxed type `%s' to record or sequence", g_type_name (boxed_type));
      return zero_value;
    }
}

GslGlueContext*
bse_glue_context (void)
{
  static const GslGlueContextTable bse_glue_table = {
    bglue_describe_enum,
    bglue_describe_iface,
    bglue_describe_prop,
    bglue_describe_proc,
    bglue_list_proc_names,
    bglue_list_method_names,
    bglue_base_iface,
    bglue_iface_children,
    bglue_proxy_iface,
    bglue_exec_proc,
    bglue_signal_connection,
    bglue_client_msg,
  };
  static GslGlueContext *bse_context = NULL;
  
  if (!bse_context)
    {
      bse_context = g_new0 (GslGlueContext, 1);
      gsl_glue_context_common_init (bse_context, &bse_glue_table);
    }
  
  return bse_context;
}

static GslGlueEnum*
bglue_describe_enum (GslGlueContext *context,
                     const gchar    *enum_name)
{
  GType type = g_type_from_name (enum_name);
  GslGlueEnum *e;
  GEnumClass *eclass;
  guint i;
  
  if (!G_TYPE_IS_ENUM (type))
    return NULL;
  
  eclass = g_type_class_ref (type);
  e = g_new0 (GslGlueEnum, 1);
  e->enum_name = g_strdup (g_type_name (type));
  e->n_values = eclass->n_values;
  e->values = g_new (gchar*, e->n_values + 1);
  e->blurbs = g_new (gchar*, e->n_values + 1);
  for (i = 0; i < e->n_values; i++)
    {
      e->values[i] = g_strdup (eclass->values[i].value_name);
      if (eclass->values[i].value_nick)
        e->blurbs[i] = g_strdup (eclass->values[i].value_nick);
      else
        e->blurbs[i] = g_strdup (eclass->values[i].value_name);
    }
  e->values[i] = NULL;
  e->blurbs[i] = NULL;
  g_type_class_unref (eclass);
  return e;
}

static GslGlueIFace*
bglue_describe_iface (GslGlueContext *context,
                      const gchar    *iface)
{
  GType xtype, type = g_type_from_name (iface);
  GslGlueIFace *f;
  GObjectClass *oclass;
  GParamSpec **pspecs;
  GSList *plist = NULL;
  guint i, n;
  
  if (!G_TYPE_IS_OBJECT (type) || !g_type_is_a (type, BSE_TYPE_ITEM))
    return NULL;
  
  f = g_new0 (GslGlueIFace, 1);
  f->type_name = g_strdup (g_type_name (type));
  f->n_ifaces = g_type_depth (type) - g_type_depth (BSE_TYPE_ITEM) + 1;
  f->ifaces = g_new (gchar*, f->n_ifaces + 1);
  xtype = type;
  for (i = 0; i < f->n_ifaces; i++)
    {
      f->ifaces[i] = g_strdup (g_type_name (xtype));
      xtype = g_type_parent (xtype);
    }
  f->ifaces[i] = NULL;
  
  oclass = g_type_class_ref (type);
  xtype = BSE_TYPE_ITEM;
  pspecs = g_object_class_list_properties (oclass, &n);
  f->n_props = 0;
  for (i = 0; i < n; i++)
    {
      GParamSpec *pspec = pspecs[i];
      
      if (g_type_is_a (pspec->owner_type, xtype))
        {
          plist = g_slist_prepend (plist, g_strdup (pspec->name));
          f->n_props++;
        }
    }
  g_free (pspecs);
  g_type_class_unref (oclass);
  
  i = f->n_props;
  f->props = g_new (gchar*, i + 1);
  f->props[i] = NULL;
  while (i--)
    {
      GSList *tmp = plist->next;
      
      f->props[i] = plist->data;
      g_slist_free_1 (plist);
      plist = tmp;
    }
  
  f->n_signals = f->n_props;
  f->signals = g_new (gchar*, f->n_signals + 1);
  for (i = 0; i < f->n_props; i++)
    {
      gchar *signame = g_strdup_printf ("notify::%s", f->props[i]);
      
      f->signals[i] = g_strdup (signame);
      g_free (signame);
    }
  f->signals[i] = NULL;
  
  return f;
}

guint
bse_glue_enum_index (GType enum_type,
		     gint  enum_value)
{
  GEnumClass *eclass;
  GEnumValue *ev;
  guint index;
  
  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), G_MAXINT);
  g_return_val_if_fail (G_TYPE_IS_DERIVED (enum_type), G_MAXINT);

  eclass = g_type_class_ref (enum_type);
  ev = g_enum_get_value (eclass, enum_value);
  if (!ev)
    g_message ("%s: enum \"%s\" has no value %u", G_STRLOC, g_type_name (enum_type), enum_value);
  index = ev ? ev - eclass->values : G_MAXINT;
  g_type_class_unref (eclass);

  return index;
}

static gboolean
param_from_pspec (GslGlueParam *param,
                  GParamSpec   *pspec)
{
  switch (G_TYPE_FUNDAMENTAL (G_PARAM_SPEC_VALUE_TYPE (pspec)))
    {
      glong istepping;
      gdouble fstepping;
    case G_TYPE_BOOLEAN:
      gsl_glue_param_bool (param, pspec->name,
                           G_PARAM_SPEC_BOOLEAN (pspec)->default_value);
      break;
    case G_TYPE_CHAR:
      gsl_glue_param_irange (param, pspec->name,
                             G_PARAM_SPEC_CHAR (pspec)->default_value,
                             G_PARAM_SPEC_CHAR (pspec)->minimum,
                             G_PARAM_SPEC_CHAR (pspec)->maximum,
                             1);
      break;
    case G_TYPE_UCHAR:
      gsl_glue_param_irange (param, pspec->name,
                             G_PARAM_SPEC_UCHAR (pspec)->default_value,
                             G_PARAM_SPEC_UCHAR (pspec)->minimum,
                             G_PARAM_SPEC_UCHAR (pspec)->maximum,
                             1);
      break;
    case G_TYPE_INT:
      if (BSE_IS_PARAM_SPEC_INT (pspec))
	gsl_glue_param_irange (param, pspec->name,
			       G_PARAM_SPEC_INT (pspec)->default_value,
			       G_PARAM_SPEC_INT (pspec)->minimum,
			       G_PARAM_SPEC_INT (pspec)->maximum,
			       BSE_PARAM_SPEC_INT (pspec)->stepping_rate);
      else if (BSE_IS_PARAM_SPEC_NOTE (pspec))
	gsl_glue_param_irange (param, pspec->name,
			       BSE_PARAM_SPEC_NOTE (pspec)->default_value,
			       BSE_PARAM_SPEC_NOTE (pspec)->minimum,
			       BSE_PARAM_SPEC_NOTE (pspec)->maximum,
			       BSE_PARAM_SPEC_NOTE (pspec)->stepping_rate);
      else
	gsl_glue_param_irange (param, pspec->name,
			       G_PARAM_SPEC_INT (pspec)->default_value,
			       G_PARAM_SPEC_INT (pspec)->minimum,
			       G_PARAM_SPEC_INT (pspec)->maximum,
			       1);
      break;
    case G_TYPE_UINT:
      istepping = BSE_IS_PARAM_SPEC_UINT (pspec) ? BSE_PARAM_SPEC_UINT (pspec)->stepping_rate : 1;
      gsl_glue_param_irange (param, pspec->name,
                             G_PARAM_SPEC_UINT (pspec)->default_value,
                             G_PARAM_SPEC_UINT (pspec)->minimum,
                             G_PARAM_SPEC_UINT (pspec)->maximum,
                             istepping);
      break;
    case G_TYPE_LONG:
      gsl_glue_param_irange (param, pspec->name,
                             G_PARAM_SPEC_LONG (pspec)->default_value,
                             G_PARAM_SPEC_LONG (pspec)->minimum,
                             G_PARAM_SPEC_LONG (pspec)->maximum,
                             1);
      break;
    case G_TYPE_ULONG:
      gsl_glue_param_irange (param, pspec->name,
                             G_PARAM_SPEC_ULONG (pspec)->default_value,
                             G_PARAM_SPEC_ULONG (pspec)->minimum,
                             G_PARAM_SPEC_ULONG (pspec)->maximum,
                             1);
      break;
    case G_TYPE_FLOAT:
      fstepping = BSE_IS_PARAM_SPEC_FLOAT (pspec) ? BSE_PARAM_SPEC_FLOAT (pspec)->stepping_rate : 1;
      gsl_glue_param_frange (param, pspec->name,
                             G_PARAM_SPEC_FLOAT (pspec)->default_value,
                             G_PARAM_SPEC_FLOAT (pspec)->minimum,
                             G_PARAM_SPEC_FLOAT (pspec)->maximum,
                             fstepping);
      break;
    case G_TYPE_DOUBLE:
      fstepping = BSE_IS_PARAM_SPEC_DOUBLE (pspec) ? BSE_PARAM_SPEC_DOUBLE (pspec)->stepping_rate : 1;
      gsl_glue_param_frange (param, pspec->name,
                             G_PARAM_SPEC_DOUBLE (pspec)->default_value,
                             G_PARAM_SPEC_DOUBLE (pspec)->minimum,
                             G_PARAM_SPEC_DOUBLE (pspec)->maximum,
                             fstepping);
      break;
    case BSE_TYPE_TIME:
      gsl_glue_param_irange (param, pspec->name,
                             BSE_PARAM_SPEC_TIME (pspec)->default_value,
                             0,
                             G_MAXINT,
                             1);
      break;
    case G_TYPE_ENUM:
      gsl_glue_param_enum (param, pspec->name,
                           g_type_name (G_TYPE_FROM_CLASS (G_PARAM_SPEC_ENUM (pspec)->enum_class)),
                           bse_glue_enum_index (G_TYPE_FROM_CLASS (G_PARAM_SPEC_ENUM (pspec)->enum_class),
						G_PARAM_SPEC_ENUM (pspec)->default_value));
      break;
    case G_TYPE_STRING:
      gsl_glue_param_string (param, pspec->name,
                             G_PARAM_SPEC_STRING (pspec)->default_value);
      break;
    case G_TYPE_OBJECT:
      gsl_glue_param_proxy (param, pspec->name,
                            g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
      break;
    case G_TYPE_BOXED:
      if (g_type_get_qdata (G_PARAM_SPEC_VALUE_TYPE (pspec), g_quark_from_string ("BseGlueBoxedToRec")))
	gsl_glue_param_rec (param, pspec->name);
      else if (g_type_get_qdata (G_PARAM_SPEC_VALUE_TYPE (pspec), g_quark_from_string ("BseGlueBoxedToSeq")))
	gsl_glue_param_seq (param, pspec->name);
      else
	{
	  g_warning ("unable to create glue param for boxed type `%s'", g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
	  return FALSE;
	}
      break;
    default:
      return FALSE;
    }
  return TRUE;
}

static GslGlueProp*
bglue_describe_prop (GslGlueContext *context,
                     gulong          proxy,
                     const gchar    *prop_name)
{
  BseObject *object = bse_object_from_id (proxy);
  GParamSpec *pspec;
  GslGlueProp *p;
  
  if (!BSE_IS_ITEM (object))
    {
      g_message ("property lookup: no such object (id=%lu)", proxy);
      return NULL;
    }
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object), prop_name);
  if (!pspec)
    return NULL;
  
  p = g_new0 (GslGlueProp, 1);
  if (!param_from_pspec (&p->param, pspec))
    {
      g_message ("failed to construct glue param description for property \"%s\" of `%s'",
                 prop_name, G_OBJECT_TYPE_NAME (object));
      g_free (p);
      return NULL;
    }
  p->group = g_strdup (bse_param_spec_get_group (pspec));
  p->pretty_name = g_strdup (g_param_spec_get_nick (pspec));
  p->blurb = g_strdup (g_param_spec_get_blurb (pspec));
  
  if (pspec->flags & BSE_PARAM_WRITABLE)
    p->flags |= GSL_GLUE_FLAG_WRITABLE;
  if (pspec->flags & BSE_PARAM_READABLE)
    p->flags |= GSL_GLUE_FLAG_READABLE;
  if (pspec->flags & 0)
    p->flags |= GSL_GLUE_FLAG_DISABLED;
  if (pspec->flags & BSE_PARAM_GUI)
    p->flags |= GSL_GLUE_FLAG_GUI;
  if (pspec->flags & BSE_PARAM_STORAGE)
    p->flags |= GSL_GLUE_FLAG_STORAGE;
  if (pspec->flags & BSE_PARAM_HINT_RADIO)
    p->flags |= GSL_GLUE_FLAG_RADIO;
  if (pspec->flags & BSE_PARAM_HINT_SCALE)
    p->flags |= GSL_GLUE_FLAG_SCALE;
  if (pspec->flags & BSE_PARAM_HINT_DIAL)
    p->flags |= GSL_GLUE_FLAG_DIAL;
  
  return p;
}

static GslGlueProc*
bglue_describe_proc (GslGlueContext *context,
                     const gchar    *proc_name)
{
  GType type = g_type_from_name (proc_name);
  BseProcedureClass *proc;
  GslGlueProc *p = NULL;
  
  if (!BSE_TYPE_IS_PROCEDURE (type))
    return NULL;
  
  proc = g_type_class_ref (type);
  if (proc->n_out_pspecs < 2)
    {
      gboolean bail_out = FALSE;
      guint i;
      
      p = g_new0 (GslGlueProc, 1);
      p->proc_name = g_strdup (g_type_name (type));
      p->ret_param = g_new0 (GslGlueParam, 1);
      p->n_params = proc->n_in_pspecs;
      p->params = g_new0 (GslGlueParam, p->n_params);
      if (proc->n_out_pspecs && !param_from_pspec (p->ret_param, proc->out_pspecs[0]))
        bail_out = TRUE;
      for (i = 0; i < p->n_params && !bail_out; i++)
        if (!param_from_pspec (p->params + i, proc->in_pspecs[i]))
          bail_out = TRUE;
      if (bail_out)
        {
          gsl_glue_free_proc (p);
          p = NULL;
        }
    }
  g_type_class_unref (proc);
  
  return p;
}

static gchar*
bglue_proxy_iface (GslGlueContext *context,
                   gulong          proxy)
{
  BseObject *object = bse_object_from_id (proxy);
  
  if (BSE_IS_ITEM (object))
    return g_strdup (G_OBJECT_TYPE_NAME (object));
  else
    return NULL;
}

static gchar**
bglue_list_proc_names (GslGlueContext *context)
{
  guint i, n_cats;
  BseCategory *cats = bse_categories_match_typed ("/Proc/""*", BSE_TYPE_PROCEDURE, &n_cats);
  gchar **p;
  
  p = g_new (gchar*, n_cats + 1);
  for (i = 0; i < n_cats; i++)
    p[i] = g_strdup (g_type_name (cats[i].type));
  p[i] = NULL;
  g_free (cats);
  
  return p;
}

static gchar**
bglue_list_method_names (GslGlueContext *context,
                         const gchar    *iface_name)
{
  GType type = g_type_from_name (iface_name);
  BseCategory *cats;
  gchar **p, *prefix;
  guint i, n_cats, l, n_procs;
  
  if (!g_type_is_a (type, BSE_TYPE_ITEM))
    return NULL;
  
  prefix = g_strdup_printf ("%s+", g_type_name (type));
  l = strlen (prefix);
  
  cats = bse_categories_match_typed ("/Method/""*", BSE_TYPE_PROCEDURE, &n_cats);
  p = g_new (gchar*, n_cats + 1);
  n_procs = 0;
  for (i = 0; i < n_cats; i++)
    if (strncmp (g_type_name (cats[i].type), prefix, l) == 0)
      p[n_procs++] = g_strdup (g_type_name (cats[i].type) + l);
  p[n_procs] = NULL;
  g_free (cats);
  g_free (prefix);
  
  return p;
}

static gchar*
bglue_base_iface (GslGlueContext *context)
{
  return g_strdup ("BseItem");
}

static gchar**
bglue_iface_children (GslGlueContext *context,
                      const gchar    *iface_name)
{
  GType type = g_type_from_name (iface_name);
  gchar **childnames = NULL;
  
  if (g_type_is_a (type, BSE_TYPE_ITEM))
    {
      GType *children;
      guint n;
      
      children = g_type_children (type, &n);
      childnames = g_new (gchar*, n + 1);
      childnames[n] = NULL;
      while (n--)
        childnames[n] = g_strdup (g_type_name (children[n]));
      g_free (children);
    }
  return childnames;
}

static gboolean
glue_initset_gvalue (GValue       *value,
                     GParamSpec   *pspec,
                     GslGlueValue  v)
{
  g_return_val_if_fail (G_VALUE_TYPE (value) == 0, FALSE);
  
  switch (v.glue_type)
    {
      GType type;
    case GSL_GLUE_TYPE_NONE:
      g_value_init (value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      g_param_value_set_default (pspec, value);
      break;
    case GSL_GLUE_TYPE_BOOL:
      g_value_init (value, G_TYPE_BOOLEAN);
      g_value_set_boolean (value, v.value.v_bool);
      break;
    case GSL_GLUE_TYPE_IRANGE:
      g_value_init (value, G_TYPE_INT);
      g_value_set_int (value, v.value.v_int);
      break;
    case GSL_GLUE_TYPE_FRANGE:
      g_value_init (value, G_TYPE_DOUBLE);
      g_value_set_double (value, v.value.v_float);
      break;
    case GSL_GLUE_TYPE_STRING:
      g_value_init (value, G_TYPE_STRING);
      g_value_set_string (value, v.value.v_string);
      break;
    case GSL_GLUE_TYPE_ENUM:
      type = v.value.v_enum.name ? g_type_from_name (v.value.v_enum.name) : 0;
      if (G_TYPE_IS_ENUM (type) && G_TYPE_IS_DERIVED (type))
        {
          GEnumClass *eclass = g_type_class_ref (type);

          if (v.value.v_enum.index < eclass->n_values)
            {
              g_value_init (value, G_TYPE_FROM_CLASS (eclass));
              g_value_set_enum (value, eclass->values[v.value.v_enum.index].value);
            }
	  g_type_class_unref (eclass);
        }
      if (!G_VALUE_TYPE (value))
        {
          g_message ("failed to convert glue value (enum-index: %u) into `%s' (parameter: \"%s\")",
                     v.value.v_enum.index, g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
                     pspec->name);
          return FALSE;
        }
      break;
    case GSL_GLUE_TYPE_PROXY:
      g_value_init (value, BSW_TYPE_PROXY);
      bsw_value_set_proxy (value, v.value.v_proxy);
      break;
    case GSL_GLUE_TYPE_SEQ:
    case GSL_GLUE_TYPE_REC:
    default:
      g_message ("unable to convert glue value (type: %u) into `%s' (parameter: \"%s\")",
                 v.glue_type, g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
                 pspec->name);
      return FALSE;
    }
  return TRUE;
}

static void
glue_value_from_gvalue (const GValue *value,
                        GParamSpec   *pspec,
                        GslGlueValue *v)
{
  g_return_if_fail (v->glue_type == 0);
  
  switch (G_TYPE_FUNDAMENTAL (G_VALUE_TYPE (value)))
    {
      GType type;
      BseObject *obj;
    case G_TYPE_NONE:
      break;
    case G_TYPE_BOOLEAN:
      *v = gsl_glue_value_bool (g_value_get_boolean (value));
      break;
    case G_TYPE_CHAR:
      *v = gsl_glue_value_int (g_value_get_char (value));
      break;
    case G_TYPE_UCHAR:
      *v = gsl_glue_value_int (g_value_get_uchar (value));
      break;
    case G_TYPE_INT:
      *v = gsl_glue_value_int (g_value_get_int (value));
      break;
    case G_TYPE_UINT:
      *v = gsl_glue_value_int (g_value_get_uint (value));
      break;
    case G_TYPE_LONG:
      *v = gsl_glue_value_int (g_value_get_long (value));
      break;
    case G_TYPE_ULONG:
      *v = gsl_glue_value_int (g_value_get_ulong (value));
      break;
    case G_TYPE_INT64:
      *v = gsl_glue_value_int (g_value_get_int64 (value));
      break;
    case G_TYPE_UINT64:
      *v = gsl_glue_value_int (g_value_get_uint64 (value));
      break;
    case G_TYPE_FLOAT:
      *v = gsl_glue_value_float (g_value_get_float (value));
      break;
    case G_TYPE_DOUBLE:
      *v = gsl_glue_value_float (g_value_get_double (value));
      break;
    case G_TYPE_STRING:
      *v = gsl_glue_value_string (g_value_get_string (value));
      break;
    case G_TYPE_ENUM:
      type = G_VALUE_TYPE (value);
      *v = gsl_glue_value_enum (g_type_name (type),
				bse_glue_enum_index (type,
						     g_value_get_enum (value)));
      break;
    case G_TYPE_OBJECT:
      obj = g_value_get_object (value);
      *v = gsl_glue_value_proxy (BSE_IS_ITEM (obj) ? BSE_OBJECT_ID (obj) : 0);
      break;
    case G_TYPE_BOXED:
      *v = bse_glue_boxed_to_value (G_VALUE_TYPE (value), g_value_get_boxed (value));
      break;
    case G_TYPE_POINTER:
      if (G_VALUE_TYPE (value) == BSW_TYPE_PROXY)
        {
          *v = gsl_glue_value_proxy (bsw_value_get_proxy (value));
          break;
        }
      /* fall through */
    default:
      g_message ("unable to convert `%s' (parameter: \"%s\") into glue value",
		 g_type_name (G_VALUE_TYPE (value)),
		 pspec ? pspec->name : "AnonValue");
      break;
    }
}

static BseProcedureClass*
ref_proc (const gchar *name)
{
  GType ptype = bse_procedure_lookup (name);
  
  return BSE_TYPE_IS_PROCEDURE (ptype) && G_TYPE_IS_DERIVED (ptype) ? g_type_class_ref (ptype) : NULL;
}

static BseErrorType
glue_marshal_proc (gpointer           marshal_data,
                   BseProcedureClass *proc,
                   const GValue      *ivalues,
                   GValue            *ovalues)
{
  return proc->execute (proc, ivalues, ovalues);
}

static GslGlueValue
bglue_exec_proc (GslGlueContext *context,
                 GslGlueCall    *call)
{
  GslGlueValue zero_value = { 0, };
  GslGlueValue retval = { 0, };
  BseProcedureClass *proc = ref_proc (call->proc_name);
  
  if (proc)
    {
      GValue *ivalues = g_new0 (GValue, proc->n_in_pspecs);
      GValue *ovalues = g_new0 (GValue, proc->n_out_pspecs);
      guint i, n_elements = call->params ? call->params->n_elements : 0;
      BseErrorType error;
      
      for (i = 0; i < proc->n_in_pspecs; i++)
        glue_initset_gvalue (ivalues + i, proc->in_pspecs[i],
                             i < n_elements ? call->params->elements[i] : zero_value);
      for (i = 0; i < proc->n_out_pspecs; i++)
        g_value_init (ovalues + i, G_PARAM_SPEC_VALUE_TYPE (proc->out_pspecs[i]));
      
      error = bse_procedure_marshal (G_TYPE_FROM_CLASS (proc), ivalues, ovalues, glue_marshal_proc, NULL);
      if (error)
        g_message ("while executing \"%s\": %s\n", proc->name, bse_error_blurb (error));
      
      if (proc->n_out_pspecs)
        glue_value_from_gvalue (ovalues, proc->out_pspecs[0], &retval);
      for (i = 0; i < proc->n_in_pspecs; i++)
        g_value_unset (ivalues + i);
      g_free (ivalues);
      for (i = 0; i < proc->n_out_pspecs; i++)
        g_value_unset (ovalues + i);
      g_free (ovalues);
      g_type_class_unref (proc);
    }
  else
    g_message ("failed to execute \"%s\": no such procedure\n", call->proc_name);
  
  return retval;
}

typedef struct {
  GClosure        closure;
  BseItem        *item;
  gchar          *signal;
  GslGlueContext *context;
} BGlueClosure;

static void
bglue_signal_closure_invalidated (gpointer  data,
				  GClosure *closure)
{
  GQuark quark = (GQuark) data;
  BseItem *item = ((BGlueClosure*) closure)->item;
  gulong handler_id = (gulong) g_object_get_qdata (G_OBJECT (item), quark);
  gchar *signal = ((BGlueClosure*) closure)->signal;
  
  if (handler_id)
    {
      GslGlueSeq *args;
      GslGlueValue val;

      g_object_set_qdata (G_OBJECT (item), quark, 0);
      args = gsl_glue_seq ();
      val = gsl_glue_value_proxy (BSE_OBJECT_ID (item));
      gsl_glue_seq_take_append (args, &val);
      gsl_glue_context_push (((BGlueClosure*) closure)->context);
      gsl_glue_enqueue_signal_event (signal, args, TRUE);
      gsl_glue_free_seq (args);
      gsl_glue_context_pop ();
    }
}

static void
bglue_signal_closure_marshal (GClosure       *closure,
			      GValue         *return_value,
			      guint           n_param_values,
			      const GValue   *param_values,
			      gpointer        invocation_hint,
			      gpointer        marshal_data)
{
  gchar *signal = ((BGlueClosure*) closure)->signal;
  GslGlueSeq *args;
  guint i;
  
  args = gsl_glue_seq ();
  for (i = 0; i < n_param_values; i++)
    {
      GslGlueValue val = { 0, };

      glue_value_from_gvalue (param_values + i, NULL, &val);
      gsl_glue_seq_take_append (args, &val);
    }
  gsl_glue_context_push (((BGlueClosure*) closure)->context);
  gsl_glue_enqueue_signal_event (signal, args, FALSE);
  gsl_glue_free_seq (args);
  gsl_glue_context_pop ();
}

static GClosure*
bglue_signal_closure (BseItem        *item,
		      const gchar    *signal,
		      const gchar    *qdata_name,
		      GslGlueContext *context)
{
  GClosure *closure = g_closure_new_object (sizeof (BGlueClosure), G_OBJECT (item));
  BGlueClosure *bc = (BGlueClosure*) closure;
  GQuark quark;

  bc->item = item;
  bc->signal = g_strdup (signal);
  bc->context = context;
  quark = g_quark_from_string (qdata_name);
  g_closure_add_invalidate_notifier (closure, (gpointer) quark, bglue_signal_closure_invalidated);
  g_closure_add_finalize_notifier (closure, bc->signal, (GClosureNotify) g_free);
  g_closure_set_marshal (closure, bglue_signal_closure_marshal);
  
  return closure;
}

static gboolean
bglue_signal_connection (GslGlueContext *context,
                         const gchar    *signal,
                         gulong          proxy,
                         gboolean        enable_connection)
{
  BseItem *item = bse_object_from_id (proxy);
  gboolean connected = FALSE;

  if (BSE_IS_ITEM (item))
    {
      gchar *qname;
      gulong handler_id;

      // FIXME: first check that signal actually exists on item
      qname = g_strdup_printf ("BseGlue-%s", signal);
      handler_id = (gulong) g_object_get_data (G_OBJECT (item), qname);

      if (!handler_id && !enable_connection)
	g_message ("glue signal to (item: `%s', signal: %s) already disabled",
		   G_OBJECT_TYPE_NAME (item), signal);
      else if (!handler_id && enable_connection)
	{
	  handler_id = g_signal_connect_closure (item, signal, bglue_signal_closure (item, signal, qname, context), TRUE);
	  g_object_set_data (G_OBJECT (item), qname, (gpointer) handler_id);
	  connected = TRUE;
	}
      else if (handler_id && !enable_connection)
	{
	  g_object_set_data (G_OBJECT (item), qname, 0);
	  g_signal_handler_disconnect (item, handler_id);
	}
      else /* handler_id && enable_connection */
	{
	  g_message ("glue signal to (item: `%s', signal: %s) already enabled",
		     G_OBJECT_TYPE_NAME (item), signal);
	  connected = TRUE;
	}
      g_free (qname);
    }

  return enable_connection;
}

static GslGlueValue
bglue_client_msg (GslGlueContext *context,
                  const gchar    *msg,
                  GslGlueValue    value)
{
  GslGlueValue retval = { 0, };

  if (!msg)
    ;
  else if (strcmp (msg, "bse-set-prop") == 0)
    {
      GslGlueSeq *args = value.value.v_seq;

      if (value.glue_type != GSL_GLUE_TYPE_SEQ ||
          !args || args->n_elements != 3 ||
          args->elements[0].glue_type != GSL_GLUE_TYPE_PROXY ||
          args->elements[1].glue_type != GSL_GLUE_TYPE_STRING)
        retval = gsl_glue_value_string ("invalid arguments supplied");
      else
        {
          GValue gvalue = { 0, };
          GObject *object = bse_object_from_id (args->elements[0].value.v_proxy);
          GParamSpec *pspec = NULL;

          if (BSE_IS_ITEM (object))
            pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object), args->elements[1].value.v_string);
          if (pspec && glue_initset_gvalue (&gvalue, pspec, args->elements[2]))
            {
              g_object_set_property (object, pspec->name, &gvalue);
              g_value_unset (&gvalue);
            }
          else
            retval = gsl_glue_value_string ("invalid arguments supplied");
        }
    }
  else
    {
      g_message ("unhandled client message: %s\n", msg);
      retval = gsl_glue_value_string ("Unknown client msg");
    }

  return retval;
}
