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
#include <string.h>
#include "sfiparams.h"
#include "sfiprimitives.h"


#define	NULL_CHECKED(x)		((x) && (x)[0] ? x : NULL)


typedef struct {
  gint          (*values_cmp)           (GParamSpec   *pspec,
					 const GValue *value1,
					 const GValue *value2);
  gboolean      (*value_validate)       (GParamSpec   *pspec,
					 GValue       *value);
} PSpecClassData;


/* --- prototypes --- */
static gint	param_bblock_values_cmp	(GParamSpec   *pspec,
					 const GValue *value1,
					 const GValue *value2);
static gint	param_fblock_values_cmp	(GParamSpec   *pspec,
					 const GValue *value1,
					 const GValue *value2);
static gint	param_seq_values_cmp	(GParamSpec   *pspec,
					 const GValue *value1,
					 const GValue *value2);
static gint	param_rec_values_cmp	(GParamSpec   *pspec,
					 const GValue *value1,
					 const GValue *value2);
static gboolean	param_seq_validate	(GParamSpec   *pspec,
					 GValue       *value);
static gboolean	param_rec_validate	(GParamSpec   *pspec,
					 GValue       *value);
static void	param_class_init	(gpointer      class,
					 gpointer      class_data);


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
    param_class_init,		/* class_init */
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
  info.instance_size = sizeof (SfiParamSpecProxy);
  SFI_TYPE_PARAM_PROXY = g_type_register_static (G_TYPE_PARAM_POINTER, "SfiParamSpecProxy", &info, 0);
  info.instance_size = sizeof (SfiParamSpecChoice);
  SFI_TYPE_PARAM_CHOICE = g_type_register_static (G_TYPE_PARAM_STRING, "SfiParamSpecChoice", &info, 0);
  {
    static const PSpecClassData cdata = {
      param_bblock_values_cmp, NULL,
    };
    info.class_data = &cdata;
    info.instance_size = sizeof (SfiParamSpecBBlock);
    SFI_TYPE_PARAM_BBLOCK = g_type_register_static (G_TYPE_PARAM_BOXED, "SfiParamSpecBBlock", &info, 0);
  }
  {
    static const PSpecClassData cdata = {
      param_fblock_values_cmp, NULL,
    };
    info.class_data = &cdata;
    info.instance_size = sizeof (SfiParamSpecFBlock);
    SFI_TYPE_PARAM_FBLOCK = g_type_register_static (G_TYPE_PARAM_BOXED, "SfiParamSpecFBlock", &info, 0);
  }
  {
    static const PSpecClassData cdata = {
      param_seq_values_cmp,
      param_seq_validate,
    };
    info.class_data = &cdata;
    info.instance_size = sizeof (SfiParamSpecSeq);
    SFI_TYPE_PARAM_SEQ = g_type_register_static (G_TYPE_PARAM_BOXED, "SfiParamSpecSeq", &info, 0);
  }
  {
    static const PSpecClassData cdata = {
      param_rec_values_cmp,
      param_rec_validate,
    };
    info.class_data = &cdata;
    info.instance_size = sizeof (SfiParamSpecRec);
    SFI_TYPE_PARAM_REC = g_type_register_static (G_TYPE_PARAM_BOXED, "SfiParamSpecRec", &info, 0);
  }
}

static void
param_class_init (gpointer class,
		  gpointer class_data)
{
  PSpecClassData *cdata = class_data;
  if (cdata)
    {
      GParamSpecClass *pclass = G_PARAM_SPEC_CLASS (class);
      if (cdata->values_cmp)
	pclass->values_cmp = cdata->values_cmp;
      if (cdata->value_validate)
	pclass->value_validate = cdata->value_validate;
    }
}


/* --- Sfi GParamSpec implementations --- */
static gint
param_bblock_values_cmp (GParamSpec   *pspec,
			 const GValue *value1,
			 const GValue *value2)
{
  // SfiParamSpecBBlock *bspec = SFI_PARAM_SPEC_BBLOCK (pspec);
  SfiBBlock *bblock1 = sfi_value_get_bblock (value1);
  SfiBBlock *bblock2 = sfi_value_get_bblock (value2);

  if (!bblock1 || !bblock2)
    return bblock2 ? -1 : bblock1 != bblock2;

  if (bblock1->n_bytes != bblock2->n_bytes)
    return bblock1->n_bytes < bblock2->n_bytes ? -1 : 1;
  else /* bblock1->n_bytes == bblock2->n_bytes */
    {
      guint i;
      for (i = 0; i < bblock1->n_bytes; i++)
	if (bblock1->bytes[i] != bblock2->bytes[i])
	  return bblock1->bytes[i] < bblock2->bytes[i] ? -1 : 1;
      return 0; /* all values equal */
    }
}

static gint
param_fblock_values_cmp (GParamSpec   *pspec,
			 const GValue *value1,
			 const GValue *value2)
{
  // SfiParamSpecFBlock *fspec = SFI_PARAM_SPEC_FBLOCK (pspec);
  SfiFBlock *fblock1 = sfi_value_get_fblock (value1);
  SfiFBlock *fblock2 = sfi_value_get_fblock (value2);

  if (!fblock1 || !fblock2)
    return fblock2 ? -1 : fblock1 != fblock2;

  if (fblock1->n_values != fblock2->n_values)
    return fblock1->n_values < fblock2->n_values ? -1 : 1;
  else /* fblock1->n_values == fblock2->n_values */
    {
      guint i;
      for (i = 0; i < fblock1->n_values; i++)
	if (fblock1->values[i] != fblock2->values[i])
	  return fblock1->values[i] < fblock2->values[i] ? -1 : 1;
      return 0; /* all values equal */
    }
}

static gint
param_seq_values_cmp (GParamSpec   *pspec,
		      const GValue *value1,
		      const GValue *value2)
{
  SfiParamSpecSeq *sspec = SFI_PARAM_SPEC_SEQ (pspec);
  SfiSeq *seq1 = sfi_value_get_seq (value1);
  SfiSeq *seq2 = sfi_value_get_seq (value2);

  if (!seq1 || !seq2)
    return seq2 ? -1 : seq1 != seq2;

  if (seq1->n_elements != seq2->n_elements)
    return seq1->n_elements < seq2->n_elements ? -1 : 1;
  else if (!sspec->element)
    {
      /* we need an element specification for comparisons, so there's not much
       * to compare here, try to at least provide stable lesser/greater result
       */
      return seq1->n_elements < seq2->n_elements ? -1 : seq1->n_elements > seq2->n_elements;
    }
  else /* seq1->n_elements == seq2->n_elements */
    {
      guint i;

      for (i = 0; i < seq1->n_elements; i++)
	{
	  GValue *element1 = seq1->elements + i;
	  GValue *element2 = seq2->elements + i;
	  gint cmp;

	  /* need corresponding element types, provide stable result otherwise */
	  if (G_VALUE_TYPE (element1) != G_VALUE_TYPE (element2))
	    return G_VALUE_TYPE (element1) < G_VALUE_TYPE (element2) ? -1 : 1;
	  /* ignore non conforming elements */
	  if (!G_VALUE_HOLDS (element1, G_PARAM_SPEC_VALUE_TYPE (sspec->element)))
	    continue;
	  cmp = g_param_values_cmp (sspec->element, element1, element2);
	  if (cmp)
	    return cmp;
	}
      return 0;
    }
}

static gint
param_rec_values_cmp (GParamSpec   *pspec,
		      const GValue *value1,
		      const GValue *value2)
{
  // SfiParamSpecRec *rspec = SFI_PARAM_SPEC_REC (pspec);
  SfiRec *rec1 = sfi_value_get_rec (value1);
  SfiRec *rec2 = sfi_value_get_rec (value2);

  if (!rec1 || !rec2)
    return rec2 ? -1 : rec1 != rec2;
  // if (rec1->n_fields) return -1;

  if (rec1->n_fields != rec2->n_fields)
    return rec1->n_fields < rec2->n_fields ? -1 : 1;
  else /* rec1->n_fields == rec2->n_fields */
    {
      guint i;

      sfi_rec_sort (rec1);
      sfi_rec_sort (rec2);
      for (i = 0; i < rec1->n_fields; i++)
	{
	  const gchar *field_name1 = rec1->field_names[i];
	  const gchar *field_name2 = rec1->field_names[i];
	  GValue *field1 = rec1->fields + i;
	  GValue *field2 = rec2->fields + i;
	  GParamSpec *fspec;
	  gint cmp;

	  cmp = strcmp (field_name1, field_name2);
	  if (cmp)
	    return cmp;

	  /* need corresponding field types, provide stable result otherwise */
	  if (G_VALUE_TYPE (field1) != G_VALUE_TYPE (field2))
	    return G_VALUE_TYPE (field1) < G_VALUE_TYPE (field2) ? -1 : 1;

	  fspec = sfi_pspec_get_rec_field (pspec, field_name1);
	  if (fspec)	/* ignore fields without param specs */
	    {
	      cmp = g_param_values_cmp (fspec, field1, field2);
	      if (cmp)
		return cmp;
	    }
	}
      return 0;
    }
}

static gboolean
param_seq_validate (GParamSpec *pspec,
		    GValue     *value)
{
  SfiParamSpecSeq *sspec = SFI_PARAM_SPEC_SEQ (pspec);
  SfiSeq *seq = sfi_value_get_seq (value);
  guint changed = 0;

  if (seq && sspec->element)
    {
      GParamSpec *element_spec = sspec->element;
      guint i;

      for (i = 0; i < seq->n_elements; i++)
	{
	  GValue *element = seq->elements + i;

	  /* support conversion of wrongly typed elements */
	  if (G_VALUE_TYPE (element) != G_PARAM_SPEC_VALUE_TYPE (element_spec) &&
	      g_value_type_transformable (G_VALUE_TYPE (element), G_PARAM_SPEC_VALUE_TYPE (element_spec)))
	    {
	      GValue dummy = { 0, };
	      g_value_init (&dummy, G_PARAM_SPEC_VALUE_TYPE (element_spec));
	      g_value_transform (element, &dummy);
	      g_value_unset (element);
	      memcpy (element, &dummy, sizeof (dummy)); /* relocate value */
	      changed++;
	    }

	  /* need to fixup value type, or ensure that the element is initialized at all */
	  if (!g_value_type_compatible (G_VALUE_TYPE (element), G_PARAM_SPEC_VALUE_TYPE (element_spec)))
	    {
	      if (G_VALUE_TYPE (element) != 0)
		g_value_unset (element);
	      g_value_init (element, G_PARAM_SPEC_VALUE_TYPE (element_spec));
	      g_param_value_set_default (element_spec, element);
	      changed++;
	    }

	  /* validate element against element_spec */
	  changed += g_param_value_validate (element_spec, element);
	}
    }
  return changed;
}

static gboolean
param_rec_validate (GParamSpec *pspec,
		    GValue     *value)
{
  SfiRec *rec = sfi_value_get_rec (value);
  guint changed = 0;

  if (rec)
    {
      SfiRecFields fspecs = sfi_pspec_get_rec_fields (pspec);
      guint i;

      for (i = 0; i < fspecs.n_fields; i++)
	{
	  GParamSpec *fspec = fspecs.fields[i];
	  GValue *field = sfi_rec_get (rec, fspec->name);

	  /* ensure field presence */
	  if (!field)
	    {
	      GValue dummy = { 0, };
	      g_value_init (&dummy, G_PARAM_SPEC_VALUE_TYPE (fspec));
	      g_param_value_set_default (fspec, &dummy);
	      sfi_rec_set (rec, fspec->name, &dummy);
	      g_value_unset (&dummy);
	      changed++;
	    }

	  /* support conversion of wrongly typed fields */
	  if (G_VALUE_TYPE (field) != G_PARAM_SPEC_VALUE_TYPE (fspec) &&
	      g_value_type_transformable (G_VALUE_TYPE (field), G_PARAM_SPEC_VALUE_TYPE (fspec)))
	    {
	      GValue dummy = { 0, };
	      g_value_init (&dummy, G_PARAM_SPEC_VALUE_TYPE (fspec));
	      g_value_transform (field, &dummy);
	      g_value_unset (field);
	      memcpy (field, &dummy, sizeof (dummy)); /* relocate value */
	      changed++;
	    }

	  /* need to fixup value type, or ensure that the field is initialized at all */
	  if (!g_value_type_compatible (G_VALUE_TYPE (field), G_PARAM_SPEC_VALUE_TYPE (fspec)))
	    {
	      if (G_VALUE_TYPE (field) != 0)
		g_value_unset (field);
	      g_value_init (field, G_PARAM_SPEC_VALUE_TYPE (fspec));
	      g_param_value_set_default (fspec, field);
	      changed++;
	    }

	  /* validate field against field_spec */
	  changed += g_param_value_validate (fspec, field);
	}
    }
  return changed;
}


/* --- Sfi GParamSpec constructors --- */
static guint
pspec_flags (const gchar *hints)
{
  return G_PARAM_READWRITE;	// FIXME
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
  sfi_pspec_set_static_hints (pspec, hints);
  
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
  sfi_pspec_set_static_hints (pspec, hints);
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
  sfi_pspec_set_static_hints (pspec, hints);
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
  sfi_pspec_set_static_hints (pspec, hints);
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
  sfi_pspec_set_static_hints (pspec, hints);
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
  sfi_pspec_set_static_hints (pspec, hints);
  sspec = G_PARAM_SPEC_STRING (pspec);
  g_free (sspec->default_value);
  sspec->default_value = g_strdup (default_value ? default_value : static_const_cvalues.values[0].value_name);
  cspec = SFI_PARAM_SPEC_CHOICE (pspec);
  cspec->cvalues = static_const_cvalues;
  pspec->value_type = SFI_TYPE_CHOICE;
  
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
  sfi_pspec_set_static_hints (pspec, hints);
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
  sfi_pspec_set_static_hints (pspec, hints);
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
  sfi_pspec_set_static_hints (pspec, hints);
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
  
  pspec = g_param_spec_internal (SFI_TYPE_PARAM_REC, name, NULL_CHECKED (nick), NULL_CHECKED (blurb), pspec_flags (hints));
  sfi_pspec_set_static_hints (pspec, hints);
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
  sfi_pspec_set_static_hints (pspec, hints);
  pspec->value_type = SFI_TYPE_PROXY;
  
  return pspec;
}


/* --- conversion --- */
static void
sfi_pspec_copy_commons (GParamSpec *src_pspec,
			GParamSpec *dest_pspec)
{
  sfi_pspec_set_hints (dest_pspec, sfi_pspec_get_hints (src_pspec));
  sfi_pspec_set_group (dest_pspec, sfi_pspec_get_group (src_pspec));
}

GParamSpec*
sfi_param_spec_choice_from_enum (GParamSpec *enum_pspec)
{
  GParamSpec *pspec;
  GParamSpecEnum *espec;
  SfiChoiceValues cvals;
  GEnumValue *default_evalue;
  
  g_return_val_if_fail (G_IS_PARAM_SPEC_ENUM (enum_pspec), NULL);
  
  espec = G_PARAM_SPEC_ENUM (enum_pspec);
  cvals.n_values = espec->enum_class->n_values;
  cvals.values = espec->enum_class->values;
  default_evalue = g_enum_get_value (espec->enum_class, espec->default_value);
  pspec = sfi_param_spec_choice (enum_pspec->name,
				 enum_pspec->_nick,
				 enum_pspec->_blurb,
				 default_evalue->value_name,
				 cvals, NULL);
  sfi_pspec_copy_commons (enum_pspec, pspec);
  return pspec;
}

GParamSpec*
sfi_param_spec_proxy_from_object (GParamSpec *object_pspec)
{
  GParamSpec *pspec;
  GParamSpecObject *ospec;
  
  g_return_val_if_fail (G_IS_PARAM_SPEC_OBJECT (object_pspec), NULL);
  
  ospec = G_PARAM_SPEC_OBJECT (object_pspec);
  pspec = sfi_param_spec_proxy (object_pspec->name,
				object_pspec->_nick,
				object_pspec->_blurb,
				NULL);
  sfi_pspec_copy_commons (object_pspec, pspec);
  return pspec;
}

GParamSpec*
sfi_param_spec_to_serializable (GParamSpec *xpspec)
{
  GParamSpec *pspec = NULL;
  
  g_return_val_if_fail (G_IS_PARAM_SPEC (xpspec), NULL);
  
  if (sfi_categorize_pspec (xpspec))
    pspec = g_param_spec_ref (xpspec);
  else if (G_IS_PARAM_SPEC_BOXED (xpspec))
    {
      const SfiBoxedRecordInfo *rinfo = sfi_boxed_get_record_info (G_PARAM_SPEC_VALUE_TYPE (xpspec));
      const SfiBoxedSequenceInfo *sinfo = sfi_boxed_get_sequence_info (G_PARAM_SPEC_VALUE_TYPE (xpspec));
      
      if (rinfo)
	{
	  pspec = sfi_param_spec_rec (xpspec->name, xpspec->_nick, xpspec->_blurb, rinfo->fields, NULL);
	  sfi_pspec_copy_commons (xpspec, pspec);
	}
      else if (sinfo)
	{
	  pspec = sfi_param_spec_seq (xpspec->name, xpspec->_nick, xpspec->_blurb, sinfo->element, NULL);
	  sfi_pspec_copy_commons (xpspec, pspec);
	}
    }
  else if (G_IS_PARAM_SPEC_ENUM (xpspec))
    pspec = sfi_param_spec_choice_from_enum (xpspec);
  else if (G_IS_PARAM_SPEC_OBJECT (xpspec))
    pspec = sfi_param_spec_proxy_from_object (xpspec);
  
  if (!pspec)
    g_warning ("%s: unable to convert non serializable pspec \"%s\" of type `%s'",
	       G_STRLOC, xpspec->name, g_type_name (G_PARAM_SPEC_VALUE_TYPE (xpspec)));
  
  return pspec;
}


/* --- pspec accessors --- */
void
sfi_pspec_set_group (GParamSpec  *pspec,
		     const gchar *group)
{
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  
  g_param_spec_set_qdata_full (pspec, quark_param_group, g_strdup (group), group ? g_free : NULL);
}

const gchar*
sfi_pspec_get_group (GParamSpec *pspec)
{
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), NULL);
  
  return quark_param_group ? g_param_spec_get_qdata (pspec, quark_param_group) : NULL;
}

void
sfi_pspec_set_hints (GParamSpec  *pspec,
		     const gchar *hints)
{
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  
  g_param_spec_set_qdata_full (pspec, quark_hints, g_strdup (hints), g_free);
  pspec->flags |= pspec_flags (hints);
}

void
sfi_pspec_set_static_hints (GParamSpec  *pspec,
			    const gchar *hints)
{
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  
  g_param_spec_set_qdata (pspec, quark_hints, (gchar*) hints);
  pspec->flags |= pspec_flags (hints);
}

const gchar*
sfi_pspec_get_hints (GParamSpec *pspec)
{
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), NULL);
  
  return g_param_spec_get_qdata (pspec, quark_hints);
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


/* --- pspec categories --- */
GType
sfi_category_type (SfiSCategory cat_type)
{
  switch (cat_type & SFI_SCAT_TYPE_MASK)
    {
    case SFI_SCAT_BOOL:	       return SFI_TYPE_BOOL;
    case SFI_SCAT_INT:	       return SFI_TYPE_INT;
    case SFI_SCAT_NUM:	       return SFI_TYPE_NUM;
    case SFI_SCAT_REAL:	       return SFI_TYPE_REAL;
    case SFI_SCAT_STRING:      return SFI_TYPE_STRING;
    case SFI_SCAT_CHOICE:      return SFI_TYPE_CHOICE;
    case SFI_SCAT_BBLOCK:      return SFI_TYPE_BBLOCK;
    case SFI_SCAT_FBLOCK:      return SFI_TYPE_FBLOCK;
    case SFI_SCAT_SEQ:	       return SFI_TYPE_SEQ;
    case SFI_SCAT_REC:	       return SFI_TYPE_REC;
    case SFI_SCAT_PROXY:       return SFI_TYPE_PROXY;
    default:		       return 0;
    }
}

GType
sfi_category_param_type (SfiSCategory cat_type)
{
  switch (cat_type & SFI_SCAT_TYPE_MASK)
    {
    case SFI_SCAT_BOOL:		return SFI_TYPE_PARAM_BOOL;
    case SFI_SCAT_INT:		return SFI_TYPE_PARAM_INT;
    case SFI_SCAT_NUM:		return SFI_TYPE_PARAM_NUM;
    case SFI_SCAT_REAL:		return SFI_TYPE_PARAM_REAL;
    case SFI_SCAT_STRING:	return SFI_TYPE_PARAM_STRING;
    case SFI_SCAT_CHOICE:	return SFI_TYPE_PARAM_CHOICE;
    case SFI_SCAT_BBLOCK:	return SFI_TYPE_PARAM_BBLOCK;
    case SFI_SCAT_FBLOCK:	return SFI_TYPE_PARAM_FBLOCK;
    case SFI_SCAT_SEQ:		return SFI_TYPE_PARAM_SEQ;
    case SFI_SCAT_REC:		return SFI_TYPE_PARAM_REC;
    case SFI_SCAT_PROXY:	return SFI_TYPE_PARAM_PROXY;
    default:			return 0;
    }
}

SfiSCategory
sfi_categorize_type (GType value_type)
{
  switch (G_TYPE_FUNDAMENTAL (value_type))
    {
      /* simple aliases */
    case G_TYPE_BOOLEAN:			return SFI_SCAT_BOOL;
    case G_TYPE_INT:				return SFI_SCAT_INT;
    case SFI_TYPE_NUM:				return SFI_SCAT_NUM;
    case SFI_TYPE_REAL:				return SFI_SCAT_REAL;
      /* string types */
    case G_TYPE_STRING:
      if (value_type == SFI_TYPE_CHOICE)	return SFI_SCAT_CHOICE;
      else					return SFI_SCAT_STRING;
      break;	/* FAIL */
      /* boxed types */
    case G_TYPE_BOXED:
      /* test direct match */
      if (value_type == SFI_TYPE_BBLOCK)	return SFI_SCAT_BBLOCK;
      if (value_type == SFI_TYPE_FBLOCK)	return SFI_SCAT_FBLOCK;
      if (value_type == SFI_TYPE_SEQ)		return SFI_SCAT_SEQ;
      if (value_type == SFI_TYPE_REC)		return SFI_SCAT_REC;
      break;	/* FAIL */
      /* pointer types */
    case G_TYPE_POINTER:
      if (value_type == SFI_TYPE_PROXY)		return SFI_SCAT_PROXY;
      break;	/* FAIL */
    }
  /* FAILED to determine category */
  return SFI_SCAT_INVAL;
}

SfiSCategory
sfi_categorize_pspec (GParamSpec *pspec)
{
  GType value_type, pspec_type;
  SfiSCategory cat;
  
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), SFI_SCAT_INVAL);

  value_type = G_PARAM_SPEC_VALUE_TYPE (pspec);
  pspec_type = G_PARAM_SPEC_TYPE (pspec);

  cat = sfi_categorize_type (value_type);
  
  if (!g_type_is_a (pspec_type, sfi_category_param_type (cat)))
    return SFI_SCAT_INVAL;
  
  switch (cat)
    {
    case SFI_SCAT_INT:
      if (sfi_pspec_test_hint (pspec, "note"))
	cat = SFI_SCAT_NOTE;
      break;
    case SFI_SCAT_NUM:
      if (sfi_pspec_test_hint (pspec, "time"))
	cat = SFI_SCAT_TIME;
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
