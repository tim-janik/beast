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
#include "sfiparams.h"

#include <string.h>

#define	NULL_CHECKED(x)		((x) && (x)[0] ? x : NULL)


/* --- variables --- */
GType	     *sfi__param_spec_types = NULL;
static GQuark quark_hints = 0;
static GQuark quark_stepping = 0;
static GQuark quark_param_group = 0;


/* --- functions --- */
void
_sfi_init_params (void)
{
  GTypeInfo info = {
    sizeof (GParamSpecClass),	/* class_size */
    NULL,			/* base_init */
    NULL,			/* base_destroy */
    NULL,			/* class_init */
    NULL,			/* class_destroy */
    NULL,			/* class_data */
    0,				/* instance_size */
    0,				/* n_preallocs */
    NULL,			/* instance_init */
  };
  static GType pspec_types[6] = { 0, };
  
  g_assert (sfi__param_spec_types == NULL);

  sfi__param_spec_types = pspec_types;

  quark_hints = g_quark_from_static_string ("sfi-param-spec-hints");
  quark_stepping = g_quark_from_static_string ("sfi-param-spec-stepping");
  quark_param_group = g_quark_from_static_string ("sfi-param-group");

  /* pspec types */
  info.instance_size = sizeof (SfiParamSpecChoice);
  SFI_TYPE_PARAM_CHOICE = g_type_register_static (G_TYPE_PARAM_STRING, "SfiParamSpecChoice", &info, 0);
  info.instance_size = sizeof (SfiParamSpecBBlock);
  SFI_TYPE_PARAM_BBLOCK = g_type_register_static (G_TYPE_PARAM_BOXED, "SfiParamSpecBBlock", &info, 0);
  info.instance_size = sizeof (SfiParamSpecFBlock);
  SFI_TYPE_PARAM_FBLOCK = g_type_register_static (G_TYPE_PARAM_BOXED, "SfiParamSpecFBlock", &info, 0);
  info.instance_size = sizeof (SfiParamSpecSeq);
  SFI_TYPE_PARAM_SEQ = g_type_register_static (G_TYPE_PARAM_BOXED, "SfiParamSpecSeq", &info, 0);
  info.instance_size = sizeof (SfiParamSpecRec);
  SFI_TYPE_PARAM_REC = g_type_register_static (G_TYPE_PARAM_BOXED, "SfiParamSpecRec", &info, 0);
  info.instance_size = sizeof (SfiParamSpecProxy);
  SFI_TYPE_PARAM_PROXY = g_type_register_static (G_TYPE_PARAM_POINTER, "SfiParamSpecProxy", &info, 0);
}


/* --- Sfi GParamSpec  constructors --- */
static guint
pspec_flags (const gchar *hints)
{
  return G_PARAM_READWRITE;
}

GParamSpec*
sfi_param_spec_bool (const gchar    *name,
		     const gchar    *nick,
		     const gchar    *blurb,
		     SfiBool         default_value,
		     const gchar    *hints)
{
  GParamSpec *pspec;
  
  g_return_val_if_fail (default_value == TRUE || default_value == FALSE, NULL);

  pspec = g_param_spec_boolean (name, NULL_CHECKED (nick), NULL_CHECKED (blurb), default_value, pspec_flags (hints));
  g_param_spec_set_qdata (pspec, quark_hints, (gchar*) hints);

  return pspec;
}

GParamSpec*
sfi_param_spec_int (const gchar    *name,
		    const gchar    *nick,
		    const gchar    *blurb,
		    SfiInt          default_value,
		    SfiInt          minimum_value,
		    SfiInt          maximum_value,
		    SfiInt          stepping,
		    const gchar    *hints)
{
  GParamSpec *pspec;

  g_return_val_if_fail (default_value >= minimum_value && default_value <= maximum_value, NULL);
  g_return_val_if_fail (minimum_value <= maximum_value, NULL);
  g_return_val_if_fail (minimum_value + stepping <= maximum_value, NULL);

  pspec = g_param_spec_int (name, NULL_CHECKED (nick), NULL_CHECKED (blurb), minimum_value, maximum_value, default_value, pspec_flags (hints));
  g_param_spec_set_qdata (pspec, quark_hints, (gchar*) hints);
  g_param_spec_set_qdata (pspec, quark_stepping, (gpointer) (glong) stepping);

  return pspec;
}

GParamSpec*
sfi_param_spec_num (const gchar    *name,
		    const gchar    *nick,
		    const gchar    *blurb,
		    SfiNum          default_value,
		    SfiNum          minimum_value,
		    SfiNum          maximum_value,
		    SfiNum          stepping,
		    const gchar    *hints)
{
  GParamSpec *pspec;
  SfiNum *sdata;

  g_return_val_if_fail (default_value >= minimum_value && default_value <= maximum_value, NULL);
  g_return_val_if_fail (minimum_value <= maximum_value, NULL);
  g_return_val_if_fail (minimum_value + stepping <= maximum_value, NULL);

  pspec = g_param_spec_int64 (name, NULL_CHECKED (nick), NULL_CHECKED (blurb), minimum_value, maximum_value, default_value, pspec_flags (hints));
  g_param_spec_set_qdata (pspec, quark_hints, (gchar*) hints);
  sdata = g_new (SfiNum, 1);
  *sdata = stepping;
  g_param_spec_set_qdata_full (pspec, quark_stepping, sdata, g_free);

  return pspec;
}

GParamSpec*
sfi_param_spec_real (const gchar    *name,
		     const gchar    *nick,
		     const gchar    *blurb,
		     SfiReal         default_value,
		     SfiReal         minimum_value,
		     SfiReal         maximum_value,
		     SfiReal         stepping,
		     const gchar    *hints)
{
  GParamSpec *pspec;
  SfiReal *sdata;

  g_return_val_if_fail (default_value >= minimum_value && default_value <= maximum_value, NULL);
  g_return_val_if_fail (minimum_value <= maximum_value, NULL);
  g_return_val_if_fail (minimum_value + stepping <= maximum_value, NULL);
  
  pspec = g_param_spec_double (name, NULL_CHECKED (nick), NULL_CHECKED (blurb), minimum_value, maximum_value, default_value, pspec_flags (hints));
  g_param_spec_set_qdata (pspec, quark_hints, (gchar*) hints);
  sdata = g_new (SfiReal, 1);
  *sdata = stepping;
  g_param_spec_set_qdata_full (pspec, quark_stepping, sdata, g_free);

  return pspec;
}

GParamSpec*
sfi_param_spec_string (const gchar    *name,
		       const gchar    *nick,
		       const gchar    *blurb,
		       const gchar    *default_value,
		       const gchar    *hints)
{
  GParamSpec *pspec;
  GParamSpecString *sspec;

  pspec = g_param_spec_internal (SFI_TYPE_PARAM_STRING, name, NULL_CHECKED (nick), NULL_CHECKED (blurb), pspec_flags (hints));
  g_param_spec_set_qdata (pspec, quark_hints, (gchar*) hints);
  sspec = G_PARAM_SPEC_STRING (pspec);
  g_free (sspec->default_value);
  sspec->default_value = g_strdup (default_value);

  return pspec;
}

GParamSpec*
sfi_param_spec_choice (const gchar    *name,
		       const gchar    *nick,
		       const gchar    *blurb,
		       const gchar    *default_value,
		       SfiChoiceValues   static_const_cvalues,
		       const gchar    *hints)
{
  GParamSpec *pspec;
  GParamSpecString *sspec;
  SfiParamSpecChoice *cspec;

  g_return_val_if_fail (static_const_cvalues.n_values >= 1, NULL);

  pspec = g_param_spec_internal (SFI_TYPE_PARAM_CHOICE, name, NULL_CHECKED (nick), NULL_CHECKED (blurb), pspec_flags (hints));
  g_param_spec_set_qdata (pspec, quark_hints, (gchar*) hints);
  sspec = G_PARAM_SPEC_STRING (pspec);
  g_free (sspec->default_value);
  sspec->default_value = g_strdup (default_value ? default_value : static_const_cvalues.values[0].value_name);
  cspec = SFI_PARAM_SPEC_CHOICE (pspec);
  cspec->cvalues = static_const_cvalues;
  pspec->value_type = SFI_TYPE_CHOICE;

  return pspec;
}

GParamSpec*
sfi_param_spec_enum (const gchar    *name,
		     const gchar    *nick,
		     const gchar    *blurb,
		     gint            default_value,
		     GType           enum_type,
		     const gchar    *hints)
{
  GParamSpec *pspec;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);
  g_return_val_if_fail (enum_type != G_TYPE_ENUM, NULL);

  pspec = g_param_spec_enum (name, NULL_CHECKED (nick), NULL_CHECKED (blurb), enum_type, default_value, pspec_flags (hints));
  g_param_spec_set_qdata (pspec, quark_hints, (gchar*) hints);

  return pspec;
}

GParamSpec*
sfi_param_spec_bblock (const gchar    *name,
		       const gchar    *nick,
		       const gchar    *blurb,
		       const gchar    *hints)
{
  GParamSpec *pspec;
  // SfiParamSpecBBlock *bspec;

  pspec = g_param_spec_internal (SFI_TYPE_PARAM_BBLOCK, name, NULL_CHECKED (nick), NULL_CHECKED (blurb), pspec_flags (hints));
  g_param_spec_set_qdata (pspec, quark_hints, (gchar*) hints);
  pspec->value_type = SFI_TYPE_BBLOCK;

  return pspec;
}

GParamSpec*
sfi_param_spec_fblock (const gchar    *name,
		       const gchar    *nick,
		       const gchar    *blurb,
		       const gchar    *hints)
{
  GParamSpec *pspec;
  // SfiParamSpecFBlock *fspec;

  pspec = g_param_spec_internal (SFI_TYPE_PARAM_FBLOCK, name, NULL_CHECKED (nick), NULL_CHECKED (blurb), pspec_flags (hints));
  g_param_spec_set_qdata (pspec, quark_hints, (gchar*) hints);
  pspec->value_type = SFI_TYPE_FBLOCK;

  return pspec;
}

GParamSpec*
sfi_param_spec_seq (const gchar    *name,
		    const gchar    *nick,
		    const gchar    *blurb,
		    GParamSpec     *element_spec,
		    const gchar    *hints)
{
  GParamSpec *pspec;

  if (element_spec)
    g_return_val_if_fail (G_IS_PARAM_SPEC (element_spec), NULL);

  pspec = g_param_spec_internal (SFI_TYPE_PARAM_SEQ, name, NULL_CHECKED (nick), NULL_CHECKED (blurb), pspec_flags (hints));
  g_param_spec_set_qdata (pspec, quark_hints, (gchar*) hints);
  if (element_spec)
    {
      SfiParamSpecSeq *sspec = SFI_PARAM_SPEC_SEQ (pspec);
      sspec->element = g_param_spec_ref (element_spec);
      g_param_spec_sink (element_spec);
    }
  pspec->value_type = SFI_TYPE_SEQ;

  return pspec;
}

GParamSpec*
sfi_param_spec_rec (const gchar    *name,
		    const gchar    *nick,
		    const gchar    *blurb,
		    SfiRecFields    static_const_fields,
		    const gchar    *hints)
{
  GParamSpec *pspec;
  SfiParamSpecRec *rspec;

  g_return_val_if_fail (static_const_fields.n_fields >= 1, NULL);

  pspec = g_param_spec_internal (SFI_TYPE_PARAM_REC, name, NULL_CHECKED (nick), NULL_CHECKED (blurb), pspec_flags (hints));
  g_param_spec_set_qdata (pspec, quark_hints, (gchar*) hints);
  rspec = SFI_PARAM_SPEC_REC (pspec);
  rspec->fields = static_const_fields;
  pspec->value_type = SFI_TYPE_REC;

  return pspec;
}

GParamSpec*
sfi_param_spec_proxy (const gchar    *name,
		      const gchar    *nick,
		      const gchar    *blurb,
		      const gchar    *hints)
{
  GParamSpec *pspec;
  // SfiParamSpecProxy *xspec;

  pspec = g_param_spec_internal (SFI_TYPE_PARAM_PROXY, name, NULL_CHECKED (nick), NULL_CHECKED (blurb), pspec_flags (hints));
  g_param_spec_set_qdata (pspec, quark_hints, (gchar*) hints);
  pspec->value_type = SFI_TYPE_PROXY;

  return pspec;
}

GParamSpec*
sfi_param_spec_object (const gchar    *name,
		       const gchar    *nick,
		       const gchar    *blurb,
		       GType           object_type,
		       const gchar    *hints)
{
  GParamSpec *pspec;

  g_return_val_if_fail (g_type_is_a (object_type, SFI_TYPE_OBJECT), NULL);

  pspec = g_param_spec_object (name, NULL_CHECKED (nick), NULL_CHECKED (blurb), object_type, pspec_flags (hints));
  g_param_spec_set_qdata (pspec, quark_hints, (gchar*) hints);

  return pspec;
}

/* --- conversion --- */
GParamSpec*
sfi_param_spec_choice_from_enum (GParamSpec *enum_pspec)
{
  GParamSpec *pspec;
  GParamSpecEnum *espec;
  SfiChoiceValues cvals;
  GEnumValue *default_evalue;
  gchar *hints;

  g_return_val_if_fail (G_IS_PARAM_SPEC_ENUM (enum_pspec), NULL);

  espec = G_PARAM_SPEC_ENUM (enum_pspec);
  cvals.n_values = espec->enum_class->n_values;
  cvals.values = espec->enum_class->values;
  default_evalue = g_enum_get_value (espec->enum_class, espec->default_value);
  hints = g_param_spec_get_qdata (enum_pspec, quark_hints);
  hints = g_strdup (hints);
  pspec = sfi_param_spec_choice (enum_pspec->name,
				 enum_pspec->_nick,
				 enum_pspec->_blurb,
				 default_evalue->value_name,
				 cvals, hints);
  g_param_spec_set_qdata_full (pspec, quark_hints, hints, g_free);
  sfi_param_spec_set_group (pspec, sfi_param_spec_get_group (enum_pspec));
  return pspec;
}

GParamSpec*
sfi_param_spec_proxy_from_object (GParamSpec *object_pspec)
{
  GParamSpec *pspec;
  GParamSpecObject *ospec;
  gchar *hints;

  g_return_val_if_fail (G_IS_PARAM_SPEC_OBJECT (object_pspec), NULL);

  ospec = G_PARAM_SPEC_OBJECT (object_pspec);
  hints = g_param_spec_get_qdata (object_pspec, quark_hints);
  hints = g_strdup (hints);
  pspec = sfi_param_spec_proxy (object_pspec->name,
				object_pspec->_nick,
				object_pspec->_blurb,
				hints);
  g_param_spec_set_qdata_full (pspec, quark_hints, hints, g_free);
  sfi_param_spec_set_group (pspec, sfi_param_spec_get_group (object_pspec));
  return pspec;
}


/* --- pspec accessors --- */
void
sfi_param_spec_set_group (GParamSpec  *pspec,
			  const gchar *group)
{
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  g_param_spec_set_qdata_full (pspec, quark_param_group, g_strdup (group), group ? g_free : NULL);
}

const gchar*
sfi_param_spec_get_group (GParamSpec *pspec)
{
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), NULL);

  return quark_param_group ? g_param_spec_get_qdata (pspec, quark_param_group) : NULL;
}

SfiBool
sfi_pspec_get_bool_default (GParamSpec *pspec)
{
  g_return_val_if_fail (SFI_IS_PARAM_SPEC_BOOL (pspec), FALSE);
  
  return SFI_PARAM_SPEC_BOOL (pspec)->default_value;
}

SfiInt
sfi_pspec_get_int_default (GParamSpec *pspec)
{
  g_return_val_if_fail (SFI_IS_PARAM_SPEC_INT (pspec), 0);
  
  return SFI_PARAM_SPEC_INT (pspec)->default_value;
}

void
sfi_pspec_get_int_range (GParamSpec *pspec,
			 SfiInt     *minimum_value,
			 SfiInt     *maximum_value,
			 SfiInt     *stepping)
{
  SfiParamSpecInt *ispec;
  
  g_return_if_fail (SFI_IS_PARAM_SPEC_INT (pspec));
  
  ispec = SFI_PARAM_SPEC_INT (pspec);
  if (minimum_value)
    *minimum_value = ispec->minimum;
  if (maximum_value)
    *maximum_value = ispec->maximum;
  if (stepping)
    *stepping = (SfiInt) (glong) g_param_spec_get_qdata (pspec, quark_stepping);
}

SfiNum
sfi_pspec_get_num_default (GParamSpec *pspec)
{
  g_return_val_if_fail (SFI_IS_PARAM_SPEC_NUM (pspec), 0);
  
  return SFI_PARAM_SPEC_NUM (pspec)->default_value;
}

void
sfi_pspec_get_num_range (GParamSpec *pspec,
			 SfiNum     *minimum_value,
			 SfiNum     *maximum_value,
			 SfiNum     *stepping)
{
  SfiParamSpecNum *nspec;

  g_return_if_fail (SFI_IS_PARAM_SPEC_NUM (pspec));
  
  nspec = SFI_PARAM_SPEC_NUM (pspec);
  if (minimum_value)
    *minimum_value = nspec->minimum;
  if (maximum_value)
    *maximum_value = nspec->maximum;
  if (stepping)
    {
      SfiNum *sdata = g_param_spec_get_qdata (pspec, quark_stepping);
      *stepping = sdata ? *sdata : 0;
    }
}

SfiReal
sfi_pspec_get_real_default (GParamSpec *pspec)
{
  g_return_val_if_fail (SFI_IS_PARAM_SPEC_REAL (pspec), 0);
  
  return SFI_PARAM_SPEC_REAL (pspec)->default_value;
}

void
sfi_pspec_get_real_range (GParamSpec *pspec,
			  SfiReal    *minimum_value,
			  SfiReal    *maximum_value,
			  SfiReal    *stepping)
{
  SfiParamSpecReal *nspec;
  
  g_return_if_fail (SFI_IS_PARAM_SPEC_REAL (pspec));
  
  nspec = SFI_PARAM_SPEC_REAL (pspec);
  if (minimum_value)
    *minimum_value = nspec->minimum;
  if (maximum_value)
    *maximum_value = nspec->maximum;
  if (stepping)
    {
      SfiReal *sdata = g_param_spec_get_qdata (pspec, quark_stepping);
      *stepping = sdata ? *sdata : 0;
    }
}

const gchar*
sfi_pspec_get_string_default (GParamSpec *pspec)
{
  g_return_val_if_fail (SFI_IS_PARAM_SPEC_STRING (pspec), NULL);

  return SFI_PARAM_SPEC_STRING (pspec)->default_value;
}

const gchar*
sfi_pspec_get_choice_default (GParamSpec *pspec)
{
  g_return_val_if_fail (SFI_IS_PARAM_SPEC_CHOICE (pspec), NULL);

  return G_PARAM_SPEC_STRING (pspec)->default_value;
}

SfiChoiceValues
sfi_pspec_get_choice_values (GParamSpec *pspec)
{
  SfiParamSpecChoice *cspec;
  SfiChoiceValues dummy = { 0, };

  g_return_val_if_fail (SFI_IS_PARAM_SPEC_CHOICE (pspec), dummy);

  cspec = SFI_PARAM_SPEC_CHOICE (pspec);
  return cspec->cvalues;
}

GEnumValue*
sfi_pspec_get_choice_value_list (GParamSpec *pspec)
{
  SfiParamSpecChoice *cspec;

  g_return_val_if_fail (SFI_IS_PARAM_SPEC_CHOICE (pspec), NULL);

  cspec = SFI_PARAM_SPEC_CHOICE (pspec);
  return (GEnumValue*) cspec->cvalues.values;
}

gint
sfi_pspec_get_enum_default (GParamSpec *pspec)
{
  g_return_val_if_fail (SFI_IS_PARAM_SPEC_ENUM (pspec), 0);

  return G_PARAM_SPEC_ENUM (pspec)->default_value;
}

GEnumValue*
sfi_pspec_get_enum_value_list (GParamSpec *pspec)
{
  GParamSpecEnum *espec;

  g_return_val_if_fail (SFI_IS_PARAM_SPEC_ENUM (pspec), NULL);

  espec = SFI_PARAM_SPEC_ENUM (pspec);
  return (GEnumValue*) espec->enum_class->values;
}

GParamSpec*
sfi_pspec_get_seq_element (GParamSpec *pspec)
{
  SfiParamSpecSeq *sspec;

  g_return_val_if_fail (SFI_IS_PARAM_SPEC_SEQ (pspec), NULL);

  sspec = SFI_PARAM_SPEC_SEQ (pspec);
  return sspec->element;
}

SfiRecFields
sfi_pspec_get_rec_fields (GParamSpec *pspec)
{
  SfiParamSpecRec *rspec;
  SfiRecFields dummy = { 0, };

  g_return_val_if_fail (SFI_IS_PARAM_SPEC_REC (pspec), dummy);

  rspec = SFI_PARAM_SPEC_REC (pspec);
  return rspec->fields;
}

GParamSpec*
sfi_pspec_get_rec_field (GParamSpec  *pspec,
			 const gchar *field)
{
  SfiParamSpecRec *rspec;
  guint i;
  
  g_return_val_if_fail (SFI_IS_PARAM_SPEC_REC (pspec), NULL);

  rspec = SFI_PARAM_SPEC_REC (pspec);
  for (i = 0; i < rspec->fields.n_fields; i++)
    if (strcmp (rspec->fields.fields[i]->name, field) == 0)
      return rspec->fields.fields[i];
  return NULL;
}

gboolean
sfi_pspec_test_hint (GParamSpec  *pspec,
		     const gchar *hint)
{
  const gchar *phint;
  gboolean match = FALSE;
  guint hlen;

  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), FALSE);
  g_return_val_if_fail (hint != NULL, FALSE);

  phint = sfi_pspec_get_hints (pspec);
  hlen = strlen (hint);
  if (phint && hlen)
    {
      gchar *haystack = g_strconcat (":", phint, ":", NULL);
      gchar *needle = g_strconcat (hint[0] == ':' ? "" : ":", hint, hint[hlen - 1] == ':' ? "" : ":", NULL);
      match = strstr (haystack, needle) != NULL;
      g_free (haystack);
      g_free (needle);
    }
  return match;
}

const gchar*
sfi_pspec_get_hints (GParamSpec *pspec)
{
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), NULL);

  return g_param_spec_get_qdata (pspec, quark_hints);
}


/* --- pspec categories --- */
GType
sfi_pspec_category_type (SfiPSpecFlags cat_type)
{
  switch (cat_type & SFI_PSPEC_TYPE_MASK)
    {
    case SFI_PSPEC_BOOL:        return SFI_TYPE_BOOL;
    case SFI_PSPEC_INT:         return SFI_TYPE_INT;
    case SFI_PSPEC_NUM:         return SFI_TYPE_NUM;
    case SFI_PSPEC_REAL:        return SFI_TYPE_REAL;
    case SFI_PSPEC_ENUM:        return SFI_TYPE_ENUM;
    case SFI_PSPEC_STRING:      return SFI_TYPE_STRING;
    case SFI_PSPEC_OBJECT:      return SFI_TYPE_OBJECT;
    case SFI_PSPEC_CHOICE:      return SFI_TYPE_CHOICE;
    case SFI_PSPEC_BBLOCK:      return SFI_TYPE_BBLOCK;
    case SFI_PSPEC_FBLOCK:      return SFI_TYPE_FBLOCK;
    case SFI_PSPEC_SEQ:         return SFI_TYPE_SEQ;
    case SFI_PSPEC_REC:         return SFI_TYPE_REC;
    case SFI_PSPEC_PROXY:       return SFI_TYPE_PROXY;
    default:                    return 0;
    }
}

GType
sfi_pspec_category_pspec_type (SfiPSpecFlags cat_type)
{
  switch (cat_type & SFI_PSPEC_TYPE_MASK)
    {
    case SFI_PSPEC_BOOL:        return SFI_TYPE_PARAM_BOOL;
    case SFI_PSPEC_INT:         return SFI_TYPE_PARAM_INT;
    case SFI_PSPEC_NUM:         return SFI_TYPE_PARAM_NUM;
    case SFI_PSPEC_REAL:        return SFI_TYPE_PARAM_REAL;
    case SFI_PSPEC_ENUM:        return SFI_TYPE_PARAM_ENUM;
    case SFI_PSPEC_STRING:      return SFI_TYPE_PARAM_STRING;
    case SFI_PSPEC_OBJECT:      return SFI_TYPE_PARAM_OBJECT;
    case SFI_PSPEC_CHOICE:      return SFI_TYPE_PARAM_CHOICE;
    case SFI_PSPEC_BBLOCK:      return SFI_TYPE_PARAM_BBLOCK;
    case SFI_PSPEC_FBLOCK:      return SFI_TYPE_PARAM_FBLOCK;
    case SFI_PSPEC_SEQ:         return SFI_TYPE_PARAM_SEQ;
    case SFI_PSPEC_REC:         return SFI_TYPE_PARAM_REC;
    case SFI_PSPEC_PROXY:       return SFI_TYPE_PARAM_PROXY;
    default:			return 0;
    }
}

static SfiPSpecFlags
pspec_type_categorize (GType value_type)
{
  switch (G_TYPE_FUNDAMENTAL (value_type))
    {
      /* simple aliases */
    case SFI_TYPE_BOOL:		return SFI_PSPEC_BOOL;
    case SFI_TYPE_INT:		return SFI_PSPEC_INT;
    case SFI_TYPE_NUM:		return SFI_PSPEC_NUM;
    case SFI_TYPE_REAL:		return SFI_PSPEC_REAL;
    case SFI_TYPE_ENUM:		return SFI_PSPEC_ENUM;
    case SFI_TYPE_OBJECT:	return SFI_PSPEC_OBJECT;
      /* string types */
    case G_TYPE_STRING:
      if (value_type == SFI_TYPE_CHOICE)
	return SFI_PSPEC_CHOICE;
      else
	return SFI_PSPEC_STRING;
      break;	/* FAIL */
      /* boxed types */
    case G_TYPE_BOXED:
      /* test direct match */
      if (value_type == SFI_TYPE_BBLOCK)
	return SFI_PSPEC_BBLOCK;
      if (value_type == SFI_TYPE_FBLOCK)
	return SFI_PSPEC_FBLOCK;
      if (value_type == SFI_TYPE_SEQ)
	return SFI_PSPEC_SEQ;
      if (value_type == SFI_TYPE_REC)
	return SFI_PSPEC_REC;
      break;	/* FAIL */
      /* pointer types */
    case G_TYPE_POINTER:
      if (value_type == SFI_TYPE_PROXY)
	return SFI_PSPEC_PROXY;
      break;	/* FAIL */
    }
  /* FAILED to determine category */
  return SFI_PSPEC_INVAL;
}

SfiPSpecFlags
sfi_pspec_categorize (GParamSpec   *pspec,
		      const GValue *value)
{
  GType value_type, pspec_type;
  SfiPSpecFlags cat;

  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec) || G_IS_VALUE (value), SFI_PSPEC_INVAL);
  if (pspec)
    {
      value_type = G_PARAM_SPEC_VALUE_TYPE (pspec);
      pspec_type = G_PARAM_SPEC_TYPE (pspec);
      if (value)
	g_return_val_if_fail (g_type_is_a (G_VALUE_TYPE (value), value_type), SFI_PSPEC_INVAL);
    }
  else
    {
      value_type = G_VALUE_TYPE (value);
      pspec_type = 0;
    }

  cat = pspec_type_categorize (value_type);
  if (!pspec)
    return cat;

  if (!g_type_is_a (pspec_type, sfi_pspec_category_pspec_type (cat)))
    return SFI_PSPEC_INVAL;

  switch (cat)
    {
    case SFI_PSPEC_INT:
      if (sfi_pspec_test_hint (pspec, "note"))
	cat = SFI_PSPEC_NOTE;
      break;
    case SFI_PSPEC_NUM:
      if (sfi_pspec_test_hint (pspec, "time"))
	cat = SFI_PSPEC_TIME;
      break;
    default:
      break;
    }

  return cat;
}


/* --- convenience aliases --- */
GParamSpec*
sfi_param_spec_time (const gchar *name,
		     const gchar *nick,
		     const gchar *blurb,
		     const gchar *hints)
{
  return sfi_param_spec_num (name, nick, blurb,
			     631148400 * (SfiNum) 1000000,	/* 1990-01-01 00:00:00 */
			     631148400 * (SfiNum) 1000000,
			     2147483647 * (SfiNum) 1000000,	/* 2038-01-19 04:14:07 */
			     3600 * (SfiNum) 1000000,
			     hints);
}

GParamSpec*
sfi_param_spec_note (const gchar *name,
		     const gchar *nick,
		     const gchar *blurb,
		     SfiInt       default_value,
		     const gchar *hints)
{
  // FIXME: min/max note
  const guint BSE_MIN_NOTE = 0;	/* assumed to be 0 in various places */
  const guint BSE_MAX_NOTE = 131; /* 123 */
  const guint BSE_KAMMER_NOTE = 69;	/* A' */
  
  return sfi_param_spec_int (name, nick, blurb,
			     BSE_KAMMER_NOTE, BSE_MIN_NOTE, BSE_MAX_NOTE, 1,
			     hints);
}
