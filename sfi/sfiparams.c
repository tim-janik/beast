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
#include "sfinote.h"
#include "sfitime.h"


#define NULL_CHECKED(x)         ((x) && (x)[0] ? x : NULL)


typedef struct {
  gint          (*values_cmp)           (GParamSpec   *pspec,
                                         const GValue *value1,
                                         const GValue *value2);
  gboolean      (*value_validate)       (GParamSpec   *pspec,
                                         GValue       *value);
} PSpecClassData;


/* --- prototypes --- */
static gint     param_bblock_values_cmp (GParamSpec   *pspec,
                                         const GValue *value1,
                                         const GValue *value2);
static gint     param_fblock_values_cmp (GParamSpec   *pspec,
                                         const GValue *value1,
                                         const GValue *value2);
static gint     param_seq_values_cmp    (GParamSpec   *pspec,
                                         const GValue *value1,
                                         const GValue *value2);
static gint     param_rec_values_cmp    (GParamSpec   *pspec,
                                         const GValue *value1,
                                         const GValue *value2);
static gboolean param_seq_validate      (GParamSpec   *pspec,
                                         GValue       *value);
static gboolean param_rec_validate      (GParamSpec   *pspec,
                                         GValue       *value);
static gboolean param_note_validate     (GParamSpec   *pspec,
                                         GValue       *value);
static void     param_class_init        (gpointer      class,
                                         gpointer      class_data);
static void	sfi_pspec_copy_commons	(GParamSpec   *src_pspec,
					 GParamSpec   *dest_pspec);


/* --- variables --- */
GType        *sfi__param_spec_types = NULL;
static GQuark quark_hints = 0;
static GQuark quark_stepping = 0;
static GQuark quark_log_scale = 0;
static GQuark quark_param_group = 0;
static GQuark quark_param_owner = 0;
static GQuark quark_tmp_choice_values = 0;
static GQuark quark_tmp_record_fields = 0;


/* --- functions --- */
void
_sfi_init_params (void)
{
  GTypeInfo info = {
    sizeof (GParamSpecClass),   /* class_size */
    NULL,                       /* base_init */
    NULL,                       /* base_destroy */
    param_class_init,           /* class_init */
    NULL,                       /* class_destroy */
    NULL,                       /* class_data */
    0,                          /* instance_size */
    0,                          /* n_preallocs */
    NULL,                       /* instance_init */
  };
  static GType pspec_types[7] = { 0, };
  
  g_assert (sfi__param_spec_types == NULL);
  
  sfi__param_spec_types = pspec_types;
  
  quark_hints = g_quark_from_static_string ("sfi-pspec-hints");
  quark_stepping = g_quark_from_static_string ("sfi-pspec-stepping");
  quark_log_scale = g_quark_from_static_string ("sfi-pspec-log-scale");
  quark_param_group = g_quark_from_static_string ("sfi-pspec-group");
  quark_param_owner = g_quark_from_static_string ("sfi-pspec-owner");
  quark_tmp_choice_values = g_quark_from_static_string ("sfi-tmp-choice-values");
  quark_tmp_record_fields = g_quark_from_static_string ("sfi-tmp-choice-values");
  
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
  {
    static const PSpecClassData cdata = {
      NULL,
      param_note_validate,
    };
    info.class_data = &cdata;
    info.instance_size = sizeof (SfiParamSpecNote);
    SFI_TYPE_PARAM_NOTE = g_type_register_static (SFI_TYPE_PARAM_INT, "SfiParamSpecNote", &info, 0);
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
  // SfiParamSpecBBlock *bspec = SFI_PSPEC_BBLOCK (pspec);
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
  // SfiParamSpecFBlock *fspec = SFI_PSPEC_FBLOCK (pspec);
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
  SfiParamSpecSeq *sspec = SFI_PSPEC_SEQ (pspec);
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
  // SfiParamSpecRec *rspec = SFI_PSPEC_REC (pspec);
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
          /* ignore fields without or non conforming param specs */
          if (fspec && G_VALUE_HOLDS (field1, G_PARAM_SPEC_VALUE_TYPE (fspec)))
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
  SfiParamSpecSeq *sspec = SFI_PSPEC_SEQ (pspec);
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
              sfi_value_type_transformable (G_VALUE_TYPE (element), G_PARAM_SPEC_VALUE_TYPE (element_spec)))
            {
              GValue dummy = { 0, };
              g_value_init (&dummy, G_PARAM_SPEC_VALUE_TYPE (element_spec));
              sfi_value_transform (element, &dummy);
              g_value_unset (element);
              memcpy (element, &dummy, sizeof (dummy)); /* relocate value */
              changed++;
            }
	  
          /* need to fixup value type, or ensure that the element is initialized at all */
          if (!sfi_value_type_compatible (G_VALUE_TYPE (element), G_PARAM_SPEC_VALUE_TYPE (element_spec)))
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
              field = sfi_rec_get (rec, fspec->name);
              changed++;
            }
	  
          /* support conversion of wrongly typed fields */
          if (G_VALUE_TYPE (field) != G_PARAM_SPEC_VALUE_TYPE (fspec) &&
              sfi_value_type_transformable (G_VALUE_TYPE (field), G_PARAM_SPEC_VALUE_TYPE (fspec)))
            {
              GValue dummy = { 0, };
              g_value_init (&dummy, G_PARAM_SPEC_VALUE_TYPE (fspec));
              sfi_value_transform (field, &dummy);
              g_value_unset (field);
              memcpy (field, &dummy, sizeof (dummy)); /* relocate value */
              changed++;
            }
	  
          /* need to fixup value type, or ensure that the field is initialized at all */
          if (!sfi_value_type_compatible (G_VALUE_TYPE (field), G_PARAM_SPEC_VALUE_TYPE (fspec)))
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

static gboolean
param_note_validate (GParamSpec *pspec,
		     GValue     *value)
{
  SfiNote note = sfi_value_get_note (value);
  SfiInt min, max;
  gboolean allow_void;
  guint changed = 0;

  sfi_pspec_get_note_range (pspec, &min, &max, NULL);
  allow_void = sfi_pspec_allows_void_note (pspec);
  if (allow_void && note == SFI_NOTE_VOID)
    ;
  else if (note < min || note > max)
    {
      sfi_value_set_note (value, allow_void ? SFI_NOTE_VOID : sfi_pspec_get_note_default (pspec));
      changed++;
    }
  return changed;
}


/* --- Sfi GParamSpec constructors --- */
GParamSpec*
sfi_pspec_bool (const gchar    *name,
		const gchar    *nick,
		const gchar    *blurb,
		SfiBool         default_value,
		const gchar    *hints)
{
  GParamSpec *pspec;
  
  g_return_val_if_fail (default_value == TRUE || default_value == FALSE, NULL);
  
  pspec = g_param_spec_boolean (name, NULL_CHECKED (nick), NULL_CHECKED (blurb), default_value, 0);
  sfi_pspec_set_options (pspec, hints);
  
  return pspec;
}

GParamSpec*
sfi_pspec_int (const gchar    *name,
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
  
  pspec = g_param_spec_int (name, NULL_CHECKED (nick), NULL_CHECKED (blurb), minimum_value, maximum_value, default_value, 0);
  sfi_pspec_set_options (pspec, hints);
  g_param_spec_set_qdata (pspec, quark_stepping, (gpointer) (glong) stepping);
  
  return pspec;
}

GParamSpec*
sfi_pspec_num (const gchar    *name,
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
  
  pspec = g_param_spec_int64 (name, NULL_CHECKED (nick), NULL_CHECKED (blurb), minimum_value, maximum_value, default_value, 0);
  sfi_pspec_set_options (pspec, hints);
  sdata = g_new (SfiNum, 1);
  *sdata = stepping;
  g_param_spec_set_qdata_full (pspec, quark_stepping, sdata, g_free);
  
  return pspec;
}

GParamSpec*
sfi_pspec_real (const gchar    *name,
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
  
  pspec = g_param_spec_double (name, NULL_CHECKED (nick), NULL_CHECKED (blurb), minimum_value, maximum_value, default_value, 0);
  sfi_pspec_set_options (pspec, hints);
  sdata = g_new (SfiReal, 1);
  *sdata = stepping;
  g_param_spec_set_qdata_full (pspec, quark_stepping, sdata, g_free);
  
  return pspec;
}

GParamSpec*
sfi_pspec_log_scale (const gchar    *name,
		     const gchar    *nick,
		     const gchar    *blurb,
		     SfiReal         default_value,
		     SfiReal         minimum_value,
		     SfiReal         maximum_value,
		     SfiReal         stepping,
		     SfiReal         center,
		     SfiReal         base,
		     SfiReal         n_steps,
		     const gchar    *hints)
{
  GParamSpec *pspec;

  g_return_val_if_fail (n_steps > 0, NULL);
  g_return_val_if_fail (base > 0, NULL);

  pspec = sfi_pspec_real (name, nick, blurb, default_value, minimum_value, maximum_value, stepping, hints);
  if (pspec)
    sfi_pspec_set_log_scale (pspec, center, base, n_steps);
  return pspec;
}

GParamSpec*
sfi_pspec_string (const gchar    *name,
		  const gchar    *nick,
		  const gchar    *blurb,
		  const gchar    *default_value,
		  const gchar    *hints)
{
  GParamSpec *pspec;
  GParamSpecString *sspec;
  
  pspec = g_param_spec_internal (SFI_TYPE_PARAM_STRING, name, NULL_CHECKED (nick), NULL_CHECKED (blurb), 0);
  sfi_pspec_set_options (pspec, hints);
  sspec = G_PARAM_SPEC_STRING (pspec);
  g_free (sspec->default_value);
  sspec->default_value = g_strdup (default_value);
  
  return pspec;
}

GParamSpec*
sfi_pspec_choice (const gchar    *name,
		  const gchar    *nick,
		  const gchar    *blurb,
		  const gchar    *default_value,
		  SfiChoiceValues static_const_cvalues,
		  const gchar    *hints)
{
  GParamSpec *pspec;
  GParamSpecString *sspec;
  SfiParamSpecChoice *cspec;
  
  g_return_val_if_fail (static_const_cvalues.n_values >= 1, NULL);
  
  pspec = g_param_spec_internal (SFI_TYPE_PARAM_CHOICE, name, NULL_CHECKED (nick), NULL_CHECKED (blurb), 0);
  sfi_pspec_set_options (pspec, hints);
  sspec = G_PARAM_SPEC_STRING (pspec);
  g_free (sspec->default_value);
  sspec->default_value = g_strdup (default_value ? default_value : static_const_cvalues.values[0].choice_name);
  cspec = SFI_PSPEC_CHOICE (pspec);
  cspec->cvalues = static_const_cvalues;
  pspec->value_type = SFI_TYPE_CHOICE;
  
  return pspec;
}

GParamSpec*
sfi_pspec_bblock (const gchar    *name,
		  const gchar    *nick,
		  const gchar    *blurb,
		  const gchar    *hints)
{
  GParamSpec *pspec;
  // SfiParamSpecBBlock *bspec;
  
  pspec = g_param_spec_internal (SFI_TYPE_PARAM_BBLOCK, name, NULL_CHECKED (nick), NULL_CHECKED (blurb), 0);
  sfi_pspec_set_options (pspec, hints);
  pspec->value_type = SFI_TYPE_BBLOCK;
  
  return pspec;
}

GParamSpec*
sfi_pspec_fblock (const gchar    *name,
		  const gchar    *nick,
		  const gchar    *blurb,
		  const gchar    *hints)
{
  GParamSpec *pspec;
  // SfiParamSpecFBlock *fspec;
  
  pspec = g_param_spec_internal (SFI_TYPE_PARAM_FBLOCK, name, NULL_CHECKED (nick), NULL_CHECKED (blurb), 0);
  sfi_pspec_set_options (pspec, hints);
  pspec->value_type = SFI_TYPE_FBLOCK;
  
  return pspec;
}

GParamSpec*
sfi_pspec_pspec (const gchar    *name,
		 const gchar    *nick,
		 const gchar    *blurb,
		 const gchar    *hints)
{
  GParamSpec *pspec;
  // SfiParamSpecPSpec *pspec;
  
  pspec = g_param_spec_internal (SFI_TYPE_PARAM_PSPEC, name, NULL_CHECKED (nick), NULL_CHECKED (blurb), 0);
  sfi_pspec_set_options (pspec, hints);
  pspec->value_type = G_TYPE_PARAM;
  
  return pspec;
}

GParamSpec*
sfi_pspec_seq (const gchar    *name,
	       const gchar    *nick,
	       const gchar    *blurb,
	       GParamSpec     *element_spec,
	       const gchar    *hints)
{
  GParamSpec *pspec;
  
  if (element_spec)
    g_return_val_if_fail (G_IS_PARAM_SPEC (element_spec), NULL);
  
  pspec = g_param_spec_internal (SFI_TYPE_PARAM_SEQ, name, NULL_CHECKED (nick), NULL_CHECKED (blurb), 0);
  sfi_pspec_set_options (pspec, hints);
  if (element_spec)
    {
      SfiParamSpecSeq *sspec = SFI_PSPEC_SEQ (pspec);
      sspec->element = g_param_spec_ref (element_spec);
      g_param_spec_sink (element_spec);
    }
  pspec->value_type = SFI_TYPE_SEQ;
  
  return pspec;
}

GParamSpec*
sfi_pspec_rec (const gchar    *name,
	       const gchar    *nick,
	       const gchar    *blurb,
	       SfiRecFields    static_const_fields,
	       const gchar    *hints)
{
  GParamSpec *pspec;
  SfiParamSpecRec *rspec;
  
  pspec = g_param_spec_internal (SFI_TYPE_PARAM_REC, name, NULL_CHECKED (nick), NULL_CHECKED (blurb), 0);
  sfi_pspec_set_options (pspec, hints);
  rspec = SFI_PSPEC_REC (pspec);
  rspec->fields = static_const_fields;
  pspec->value_type = SFI_TYPE_REC;
  
  return pspec;
}

GParamSpec*
sfi_pspec_proxy (const gchar    *name,
		 const gchar    *nick,
		 const gchar    *blurb,
		 const gchar    *hints)
{
  GParamSpec *pspec;
  // SfiParamSpecProxy *xspec;
  
  pspec = g_param_spec_internal (SFI_TYPE_PARAM_PROXY, name, NULL_CHECKED (nick), NULL_CHECKED (blurb), 0);
  sfi_pspec_set_options (pspec, hints);
  pspec->value_type = SFI_TYPE_PROXY;
  
  return pspec;
}

GParamSpec*
sfi_pspec_note (const gchar *name,
		const gchar *nick,
		const gchar *blurb,
		SfiInt       default_value,
		SfiInt       min_note,
		SfiInt       max_note,
		gboolean     allow_void,
		const gchar *hints)
{
  SfiParamSpecNote *nspec;
  GParamSpecInt *ispec;
  GParamSpec *pspec;
  gchar *thints;

  if (default_value == SFI_NOTE_VOID)
    {
      g_return_val_if_fail (min_note <= max_note, NULL);
      g_return_val_if_fail (default_value == SFI_NOTE_VOID && allow_void == TRUE, NULL);
    }
  else
    g_return_val_if_fail (default_value >= min_note && default_value <= max_note, NULL);

  pspec = g_param_spec_internal (SFI_TYPE_PARAM_NOTE, name, NULL_CHECKED (nick), NULL_CHECKED (blurb), 0);
  nspec = SFI_PSPEC_NOTE (pspec);
  ispec = G_PARAM_SPEC_INT (pspec);
  ispec->minimum = CLAMP (min_note, SFI_MIN_NOTE, SFI_MAX_NOTE);
  ispec->maximum = CLAMP (max_note, SFI_MIN_NOTE, SFI_MAX_NOTE);
  ispec->default_value = default_value;
  nspec->allow_void = allow_void != FALSE;
  g_param_spec_set_qdata (pspec, quark_stepping, (gpointer) (glong) 12);
  thints = g_strconcat ("note:", hints, NULL);
  sfi_pspec_set_options (pspec, thints);
  g_free (thints);
  return pspec;
}


/* --- conversion --- */
static SfiSeq*
choice_values_to_seq (const SfiChoiceValues cvalues)
{
  SfiSeq *seq = sfi_seq_new ();
  guint i;
  for (i = 0; i < cvalues.n_values; i++)
    if (!cvalues.values[i].choice_blurb)
      sfi_seq_append_string (seq, cvalues.values[i].choice_name);
    else
      {
	gchar *str;
	/* wrap up blurb */
	str = g_strconcat (cvalues.values[i].choice_name, ";", cvalues.values[i].choice_blurb, NULL);
	sfi_seq_append_string (seq, str);
	g_free (str);
      }
  return seq;
}

static SfiSeq*
rec_fields_to_seq (SfiRecFields rfields)
{
  SfiSeq *seq = sfi_seq_new ();
  guint i;
  for (i = 0; i < rfields.n_fields; i++)
    sfi_seq_append_pspec (seq, rfields.fields[i]);
  return seq;
}

typedef struct {
  guint        ref_count;
  SfiRecFields rfields;
} TmpRecordFields;

static void
tmp_record_fields_unref (TmpRecordFields *trf)
{
  g_return_if_fail (trf != NULL);
  g_return_if_fail (trf->ref_count > 0);

  trf->ref_count--;
  if (!trf->ref_count)
    {
      guint i;
      for (i = 0; i < trf->rfields.n_fields; i++)
	if (trf->rfields.fields[i])
	  g_param_spec_unref (trf->rfields.fields[i]);
      g_free ((gpointer) trf->rfields.fields);
      g_free (trf);
    }
}

static TmpRecordFields*
tmp_record_fields_from_seq (SfiSeq *seq)
{
  if (seq)
    {
      guint l = sfi_seq_length (seq);
      /* check that we got a sequence from rec_fields_to_seq() */
      if (l && sfi_seq_check (seq, SFI_TYPE_PSPEC))
	{
	  TmpRecordFields *trf = g_new0 (TmpRecordFields, 1);
	  GParamSpec **fields;
	  guint i;
	  trf->rfields.n_fields = l;
	  fields = g_new0 (GParamSpec*, trf->rfields.n_fields);
	  trf->rfields.fields = fields;
	  trf->ref_count = 1;
	  for (i = 0; i < trf->rfields.n_fields; i++)
	    {
	      fields[i] = sfi_seq_get_pspec (seq, i);
	      if (fields[i])
		g_param_spec_ref (fields[i]);
	      else /* aparently invalid sequence */
		{
		  tmp_record_fields_unref (trf);
		  return NULL;
		}
	    }
	  return trf;
	}
    }
  return NULL;
}

typedef struct {
  guint           ref_count;
  GEnumClass     *eclass; /* !eclass => free (cvalues[*].values.choice_name) */
  SfiChoiceValues cvalues;
} TmpChoiceValues;

static void
tmp_choice_values_unref (TmpChoiceValues *tcv)
{
  g_return_if_fail (tcv != NULL);
  g_return_if_fail (tcv->ref_count > 0);

  tcv->ref_count--;
  if (!tcv->ref_count)
    {
      guint i;
      if (tcv->eclass)	/* indicates from eclass or sequence */
	{
	  g_type_set_qdata (G_TYPE_FROM_CLASS (tcv->eclass), quark_tmp_choice_values, NULL);
	  g_type_class_unref (tcv->eclass);
	}
      else
	for (i = 0; i < tcv->cvalues.n_values; i++)
	  g_free (tcv->cvalues.values[i].choice_name);
      g_free ((gpointer) tcv->cvalues.values);
      g_free (tcv);
    }
}

static TmpChoiceValues*
tmp_choice_values_from_seq (SfiSeq *seq)
{
  if (seq)
    {
      guint l = sfi_seq_length (seq);
      /* check that we got a sequence from choice_values_to_seq() */
      if (l && sfi_seq_check (seq, SFI_TYPE_STRING))
	{
	  TmpChoiceValues *tcv = g_new0 (TmpChoiceValues, 1);
	  SfiChoiceValue *cvalues;
	  guint i;
	  tcv->eclass = NULL;
	  tcv->cvalues.n_values = l;
	  cvalues = g_new0 (SfiChoiceValue, tcv->cvalues.n_values);
	  tcv->cvalues.values = cvalues;
	  tcv->ref_count = 1;
	  for (i = 0; i < tcv->cvalues.n_values; i++)
	    {
	      gchar *p;
	      cvalues[i].choice_name = g_strdup (sfi_seq_get_string (seq, i));
	      if (!cvalues[i].choice_name)	/* aparently invalid sequence */
		{
		  tmp_choice_values_unref (tcv);
		  return NULL;
		}
	      /* extract blurb */
	      p = strchr (cvalues[i].choice_name, ';');
	      if (p)
		{
		  *p++ = 0;
		  cvalues[i].choice_blurb = p;
		}
	      else
		cvalues[i].choice_blurb = NULL;
	    }
	  return tcv;
	}
    }
  return NULL;
}

static TmpChoiceValues*
tmp_choice_values_from_enum (GEnumClass *eclass)
{
  TmpChoiceValues *tcv = g_type_get_qdata (G_TYPE_FROM_CLASS (eclass), quark_tmp_choice_values);
  if (!tcv)
    {
      SfiChoiceValue *cvalues;
      guint i;
      tcv = g_new0 (TmpChoiceValues, 1);
      tcv->eclass = g_type_class_ref (G_TYPE_FROM_CLASS (eclass));
      tcv->cvalues.n_values = eclass->n_values;
      cvalues = g_new (SfiChoiceValue, tcv->cvalues.n_values);
      tcv->cvalues.values = cvalues;
      for (i = 0; i < tcv->cvalues.n_values; i++)
	{
	  cvalues[i].choice_name = eclass->values[i].value_name;
	  cvalues[i].choice_blurb = eclass->values[i].value_nick;
	}
      g_type_set_qdata (G_TYPE_FROM_CLASS (eclass), quark_tmp_choice_values, tcv);
    }
  tcv->ref_count++;
  return tcv;
}

GParamSpec*
sfi_pspec_choice_from_enum (GParamSpec *enum_pspec)
{
  GParamSpec *pspec;
  GParamSpecEnum *espec;
  TmpChoiceValues *tcv;
  GEnumValue *default_evalue;

  g_return_val_if_fail (G_IS_PARAM_SPEC_ENUM (enum_pspec), NULL);
  
  espec = G_PARAM_SPEC_ENUM (enum_pspec);
  tcv = tmp_choice_values_from_enum (espec->enum_class);
  default_evalue = g_enum_get_value (espec->enum_class, espec->default_value);
  pspec = sfi_pspec_choice (enum_pspec->name,
			    enum_pspec->_nick,
			    enum_pspec->_blurb,
			    default_evalue->value_name,
			    tcv->cvalues, NULL);
  g_param_spec_set_qdata_full (pspec, quark_tmp_choice_values, tcv, (GDestroyNotify) tmp_choice_values_unref);
  sfi_pspec_copy_commons (enum_pspec, pspec);
  return pspec;
}

GParamSpec*
sfi_pspec_proxy_from_object (GParamSpec *object_pspec)
{
  GParamSpec *pspec;
  GParamSpecObject *ospec;
  
  g_return_val_if_fail (G_IS_PARAM_SPEC_OBJECT (object_pspec), NULL);
  
  ospec = G_PARAM_SPEC_OBJECT (object_pspec);
  pspec = sfi_pspec_proxy (object_pspec->name,
			   object_pspec->_nick,
			   object_pspec->_blurb,
			   NULL);
  sfi_pspec_copy_commons (object_pspec, pspec);
  return pspec;
}

GParamSpec*
sfi_pspec_to_serializable (GParamSpec *xpspec)
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
          pspec = sfi_pspec_rec (xpspec->name, xpspec->_nick, xpspec->_blurb, rinfo->fields, NULL);
          sfi_pspec_copy_commons (xpspec, pspec);
        }
      else if (sinfo)
        {
          pspec = sfi_pspec_seq (xpspec->name, xpspec->_nick, xpspec->_blurb, sinfo->element, NULL);
          sfi_pspec_copy_commons (xpspec, pspec);
        }
    }
  else if (G_IS_PARAM_SPEC_ENUM (xpspec))
    pspec = sfi_pspec_choice_from_enum (xpspec);
  else if (G_IS_PARAM_SPEC_OBJECT (xpspec))
    pspec = sfi_pspec_proxy_from_object (xpspec);
  
  if (!pspec)
    g_warning ("%s: unable to convert non serializable pspec \"%s\" of type `%s'",
               G_STRLOC, xpspec->name, g_type_name (G_PARAM_SPEC_VALUE_TYPE (xpspec)));
  
  return pspec;
}


/* --- pspec accessors --- */
GParamSpec *
sfi_pspec_set_group (GParamSpec  *pspec,
                     const gchar *group)
{
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), pspec);
  
  g_param_spec_set_qdata_full (pspec, quark_param_group, g_strdup (group), group ? g_free : NULL);
  return pspec;
}

const gchar*
sfi_pspec_get_group (GParamSpec *pspec)
{
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), NULL);
  
  return g_param_spec_get_qdata (pspec, quark_param_group);
}

void
sfi_pspec_set_owner (GParamSpec  *pspec,
		     const gchar *owner)
{
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  g_return_if_fail (owner != NULL);

  g_param_spec_set_qdata_full (pspec, quark_param_owner, g_strdup (owner), g_free);
}

const gchar*
sfi_pspec_get_owner (GParamSpec *pspec)
{
  gchar *owner;

  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), NULL);

  owner = g_param_spec_get_qdata (pspec, quark_param_owner);
  if (!owner && pspec->owner_type)
    {
      owner = g_type_name (pspec->owner_type);
      g_param_spec_set_qdata (pspec, quark_param_owner, owner);
    }
  return owner;
}

static guint
pspec_flags (const gchar *phints)
{
  guint flags = 0;
  if (phints)
    {
      if (g_option_check (phints, "r"))
	flags |= G_PARAM_READABLE;
      if (g_option_check (phints, "w"))
	flags |= G_PARAM_WRITABLE;
      if (g_option_check (phints, "construct"))
        flags |= G_PARAM_CONSTRUCT;
      if (g_option_check (phints, "construct-only"))
        flags |= G_PARAM_CONSTRUCT_ONLY;
      if (g_option_check (phints, "lax-validation"))
        flags |= G_PARAM_LAX_VALIDATION;
    }
  return flags;
}

void
sfi_pspec_set_options (GParamSpec  *pspec,
                     const gchar *hints)
{
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  if (hints)
    g_param_spec_set_qdata (pspec, quark_hints,
                            g_quark_to_string (g_quark_from_string (hints)));
  pspec->flags = pspec_flags (hints);
}

gboolean
sfi_pspec_check_option (GParamSpec  *pspec,
                        const gchar *hint)
{
  const gchar *phints;
  
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), FALSE);

  phints = sfi_pspec_get_options (pspec);
  return g_option_check (phints, hint);
}

void
sfi_pspec_add_option (GParamSpec  *pspec,
                      const gchar *option,
                      const gchar *value)
{
  const gchar *options;
  guint append = 0;

  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  g_return_if_fail (option != NULL && !strchr (option, ':'));
  g_return_if_fail (value == NULL || !strcmp (value, "-") || !strcmp (value, "+"));

  options = sfi_pspec_get_options (pspec);
  if (!options)
    options = "";
  if (value && strcmp (value, "-") == 0 &&
      g_option_check (options, option))
    append = 2;
  else if ((!value || strcmp (value, "+") == 0) &&
           !g_option_check (options, option))
    append = 1;
  if (append)
    {
      guint l = strlen (options);
      gchar *s = g_strconcat (options,
                              options[l] == ':' ? "" : ":",
                              option, /* append >= 1 */
                              append >= 2 ? value : "",
                              NULL);
      sfi_pspec_set_options (pspec, s);
      g_free (s);
    }
}

gboolean
sfi_pspec_require_options (GParamSpec  *pspec,
                           const gchar *hints)
{
  const gchar *p, *options;

  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), FALSE);
  g_return_val_if_fail (hints != NULL, FALSE);

  options = sfi_pspec_get_options (pspec);
 recurse:
  while (hints[0] == ':')
    hints++;
  if (!hints[0])
    return TRUE;
  p = strchr (hints, ':');
  if (p)
    {
      gchar *h = g_strndup (hints, p - hints);
      gboolean match = g_option_check (options, h);
      g_free (h);
      if (!match)
	return FALSE;
      hints = p + 1;
      goto recurse;
    }
  else
    return g_option_check (options, hints);
}

const gchar*
sfi_pspec_get_options (GParamSpec *pspec)
{
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), NULL);
  
  return g_param_spec_get_qdata (pspec, quark_hints);
}

#if 0
static void
sfi_pspec_set_void_note (GParamSpec *pspec,
			 gboolean    allow_void)
{
  SfiParamSpecNote *nspec;

  g_return_if_fail (SFI_IS_PSPEC_NOTE (pspec));

  nspec = SFI_PSPEC_NOTE (pspec);
  nspec->allow_void = allow_void != FALSE;
}
#endif

gboolean
sfi_pspec_allows_void_note (GParamSpec *pspec)
{
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), FALSE);

  return SFI_IS_PSPEC_NOTE (pspec) && SFI_PSPEC_NOTE (pspec)->allow_void;
}

typedef struct {
  SfiReal center;
  SfiReal base;
  SfiReal n_steps;
} LogScale;

void
sfi_pspec_set_log_scale (GParamSpec *pspec,
			 SfiReal     center,
			 SfiReal     base,
			 SfiReal     n_steps)
{
  LogScale *lscale;

  g_return_if_fail (SFI_IS_PSPEC_REAL (pspec));
  g_return_if_fail (n_steps > 0);
  g_return_if_fail (base > 0);
  
  lscale = g_new0 (LogScale, 1);
  lscale->center = center;
  lscale->base = base;
  lscale->n_steps = n_steps;
  g_param_spec_set_qdata_full (pspec, quark_log_scale, lscale, g_free);
  sfi_pspec_add_option (pspec, "log-scale", "+");
}

gboolean
sfi_pspec_get_log_scale (GParamSpec *pspec,
			 SfiReal    *center,
			 SfiReal    *base,
			 SfiReal    *n_steps)
{
  if (SFI_IS_PSPEC_REAL (pspec))
    {
      LogScale *lscale = g_param_spec_get_qdata (pspec, quark_log_scale);
      if (lscale)
	{
	  if (center)
	    *center = lscale->center;
	  if (base)
	    *base = lscale->base;
	  if (n_steps)
	    *n_steps = lscale->n_steps;
	  return TRUE;
	}
    }
  return FALSE;
}

SfiBool
sfi_pspec_get_bool_default (GParamSpec *pspec)
{
  g_return_val_if_fail (SFI_IS_PSPEC_BOOL (pspec), FALSE);
  
  return SFI_PSPEC_BOOL (pspec)->default_value;
}

SfiInt
sfi_pspec_get_int_default (GParamSpec *pspec)
{
  g_return_val_if_fail (SFI_IS_PSPEC_INT (pspec), 0);
  
  return SFI_PSPEC_INT (pspec)->default_value;
}

void
sfi_pspec_get_int_range (GParamSpec *pspec,
                         SfiInt     *minimum_value,
                         SfiInt     *maximum_value,
                         SfiInt     *stepping)
{
  SfiParamSpecInt *ispec;
  
  g_return_if_fail (SFI_IS_PSPEC_INT (pspec));
  
  ispec = SFI_PSPEC_INT (pspec);
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
  g_return_val_if_fail (SFI_IS_PSPEC_NUM (pspec), 0);
  
  return SFI_PSPEC_NUM (pspec)->default_value;
}

void
sfi_pspec_get_num_range (GParamSpec *pspec,
                         SfiNum     *minimum_value,
                         SfiNum     *maximum_value,
                         SfiNum     *stepping)
{
  SfiParamSpecNum *nspec;
  
  g_return_if_fail (SFI_IS_PSPEC_NUM (pspec));
  
  nspec = SFI_PSPEC_NUM (pspec);
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
  g_return_val_if_fail (SFI_IS_PSPEC_REAL (pspec), 0);
  
  return SFI_PSPEC_REAL (pspec)->default_value;
}

void
sfi_pspec_get_real_range (GParamSpec *pspec,
                          SfiReal    *minimum_value,
                          SfiReal    *maximum_value,
                          SfiReal    *stepping)
{
  SfiParamSpecReal *nspec;
  
  g_return_if_fail (SFI_IS_PSPEC_REAL (pspec));
  
  nspec = SFI_PSPEC_REAL (pspec);
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
  g_return_val_if_fail (SFI_IS_PSPEC_STRING (pspec), NULL);
  
  return SFI_PSPEC_STRING (pspec)->default_value;
}

const gchar*
sfi_pspec_get_choice_default (GParamSpec *pspec)
{
  g_return_val_if_fail (SFI_IS_PSPEC_CHOICE (pspec), NULL);
  
  return G_PARAM_SPEC_STRING (pspec)->default_value;
}

SfiChoiceValues
sfi_pspec_get_choice_values (GParamSpec *pspec)
{
  SfiParamSpecChoice *cspec;
  SfiChoiceValues dummy = { 0, };
  
  g_return_val_if_fail (SFI_IS_PSPEC_CHOICE (pspec), dummy);
  
  cspec = SFI_PSPEC_CHOICE (pspec);
  return cspec->cvalues;
}

GParamSpec*
sfi_pspec_get_seq_element (GParamSpec *pspec)
{
  SfiParamSpecSeq *sspec;
  
  g_return_val_if_fail (SFI_IS_PSPEC_SEQ (pspec), NULL);
  
  sspec = SFI_PSPEC_SEQ (pspec);
  return sspec->element;
}

SfiRecFields
sfi_pspec_get_rec_fields (GParamSpec *pspec)
{
  SfiParamSpecRec *rspec;
  SfiRecFields dummy = { 0, };
  
  g_return_val_if_fail (SFI_IS_PSPEC_REC (pspec), dummy);
  
  rspec = SFI_PSPEC_REC (pspec);
  return rspec->fields;
}

GParamSpec*
sfi_pspec_get_rec_field (GParamSpec  *pspec,
                         const gchar *field)
{
  SfiParamSpecRec *rspec;
  guint i;
  
  g_return_val_if_fail (SFI_IS_PSPEC_REC (pspec), NULL);
  
  rspec = SFI_PSPEC_REC (pspec);
  for (i = 0; i < rspec->fields.n_fields; i++)
    if (strcmp (rspec->fields.fields[i]->name, field) == 0)
      return rspec->fields.fields[i];
  return NULL;
}


/* --- pspec categories --- */
GType
sfi_category_type (SfiSCategory cat_type)
{
  switch (cat_type & SFI_SCAT_TYPE_MASK)
    {
    case SFI_SCAT_BOOL:        return SFI_TYPE_BOOL;
    case SFI_SCAT_INT:         return SFI_TYPE_INT;
    case SFI_SCAT_NUM:         return SFI_TYPE_NUM;
    case SFI_SCAT_REAL:        return SFI_TYPE_REAL;
    case SFI_SCAT_STRING:      return SFI_TYPE_STRING;
    case SFI_SCAT_CHOICE:      return SFI_TYPE_CHOICE;
    case SFI_SCAT_BBLOCK:      return SFI_TYPE_BBLOCK;
    case SFI_SCAT_FBLOCK:      return SFI_TYPE_FBLOCK;
    case SFI_SCAT_PSPEC:       return SFI_TYPE_PSPEC;
    case SFI_SCAT_SEQ:         return SFI_TYPE_SEQ;
    case SFI_SCAT_REC:         return SFI_TYPE_REC;
    case SFI_SCAT_PROXY:       return SFI_TYPE_PROXY;
    default:                   return 0;
    }
}

GType
sfi_category_param_type (SfiSCategory cat_type)
{
  switch (cat_type)
    {
    case SFI_SCAT_BOOL:         return SFI_TYPE_PARAM_BOOL;
    case SFI_SCAT_INT:          return SFI_TYPE_PARAM_INT;
    case SFI_SCAT_NUM:          return SFI_TYPE_PARAM_NUM;
    case SFI_SCAT_REAL:         return SFI_TYPE_PARAM_REAL;
    case SFI_SCAT_STRING:       return SFI_TYPE_PARAM_STRING;
    case SFI_SCAT_CHOICE:       return SFI_TYPE_PARAM_CHOICE;
    case SFI_SCAT_BBLOCK:       return SFI_TYPE_PARAM_BBLOCK;
    case SFI_SCAT_FBLOCK:       return SFI_TYPE_PARAM_FBLOCK;
    case SFI_SCAT_PSPEC:        return SFI_TYPE_PARAM_PSPEC;
    case SFI_SCAT_SEQ:          return SFI_TYPE_PARAM_SEQ;
    case SFI_SCAT_REC:          return SFI_TYPE_PARAM_REC;
    case SFI_SCAT_PROXY:        return SFI_TYPE_PARAM_PROXY;
    case SFI_SCAT_NOTE:         return SFI_TYPE_PARAM_NOTE;
    default:
      if (cat_type & ~SFI_SCAT_TYPE_MASK)
	return sfi_category_param_type (cat_type & SFI_SCAT_TYPE_MASK);
      else
	return 0;
    }
}

SfiSCategory
sfi_categorize_type (GType value_type)
{
  switch (G_TYPE_FUNDAMENTAL (value_type))
    {
      /* simple aliases */
    case G_TYPE_BOOLEAN:                        return SFI_SCAT_BOOL;
    case G_TYPE_INT:                            return SFI_SCAT_INT;
    case SFI_TYPE_NUM:                          return SFI_SCAT_NUM;
    case SFI_TYPE_REAL:                         return SFI_SCAT_REAL;
      /* string types */
    case G_TYPE_STRING:
      if (value_type == SFI_TYPE_CHOICE)        return SFI_SCAT_CHOICE;
      else                                      return SFI_SCAT_STRING;
      break;    /* FAIL */
      /* boxed types */
    case SFI_TYPE_PSPEC:			return SFI_SCAT_PSPEC;
    case G_TYPE_BOXED:
      /* test direct match */
      if (value_type == SFI_TYPE_BBLOCK)        return SFI_SCAT_BBLOCK;
      if (value_type == SFI_TYPE_FBLOCK)        return SFI_SCAT_FBLOCK;
      if (value_type == SFI_TYPE_SEQ)           return SFI_SCAT_SEQ;
      if (value_type == SFI_TYPE_REC)           return SFI_SCAT_REC;
      break;    /* FAIL */
      /* pointer types */
    case G_TYPE_POINTER:
      if (value_type == SFI_TYPE_PROXY)         return SFI_SCAT_PROXY;
      break;    /* FAIL */
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
      if (sfi_pspec_check_option (pspec, "note"))
        cat = SFI_SCAT_NOTE;
      break;
    case SFI_SCAT_NUM:
      if (sfi_pspec_check_option (pspec, "time"))
        cat = SFI_SCAT_TIME;
      break;
    default:
      break;
    }
  
  return cat;
}


/* --- convenience aliases --- */
GParamSpec*
sfi_pspec_time (const gchar *name,
		const gchar *nick,
		const gchar *blurb,
		const gchar *hints)
{
  GParamSpec *pspec = sfi_pspec_num (name, nick, blurb, SFI_MIN_TIME, SFI_MIN_TIME, SFI_MAX_TIME,
				     3600 * (SfiNum) 1000000, /* one hour */
				     NULL);
  gchar *thints = g_strconcat ("time:", hints, NULL);
  sfi_pspec_set_options (pspec, thints);
  g_free (thints);
  return pspec;
}


/* --- Record <=> PSpec transforms --- */
SfiRec*
sfi_pspec_to_rec (GParamSpec *pspec)
{
  SfiReal log_center = 0, log_base = 0, log_n_steps = 0;
  SfiSCategory scat;
  SfiRec *prec;
  const gchar *string;

  g_return_val_if_fail (pspec != NULL, NULL);

  scat = sfi_categorize_pspec (pspec);
  if (!scat)
    return NULL;

  prec = sfi_rec_new ();

  /* commons */
  sfi_rec_set_int (prec, "sfi_scategory", scat);
  sfi_rec_set_string (prec, "name", pspec->name);
  string = sfi_pspec_get_owner (pspec);
  if (string)
    sfi_rec_set_string (prec, "owner", string);
  string = g_param_spec_get_nick (pspec);
  if (string)
    sfi_rec_set_string (prec, "nick", string);
  string = g_param_spec_get_blurb (pspec);
  if (string)
    sfi_rec_set_string (prec, "blurb", string);
  string = sfi_pspec_get_group (pspec);
  if (string)
    sfi_rec_set_string (prec, "group", string);
  sfi_rec_set_string (prec, "options", sfi_pspec_get_options (pspec));

  /* type specifics */
  switch (scat & SFI_SCAT_TYPE_MASK)
    {
      SfiSeq *seq;
    case SFI_SCAT_BOOL:
      sfi_rec_set_bool (prec, "default", sfi_pspec_get_bool_default (pspec));
      break;
    case SFI_SCAT_INT:
      {
	SfiInt min = 0, max = 0, stepping = 0;
	sfi_pspec_get_int_range (pspec, &min, &max, &stepping);
	sfi_rec_set_int (prec, "min", min);
	sfi_rec_set_int (prec, "max", max);
	sfi_rec_set_int (prec, "stepping", stepping);
	sfi_rec_set_int (prec, "default", sfi_pspec_get_int_default (pspec));
      }
      break;
    case SFI_SCAT_NUM:
      {
	SfiNum min = 0, max = 0, stepping = 0;
	sfi_pspec_get_num_range (pspec, &min, &max, &stepping);
	sfi_rec_set_num (prec, "min", min);
	sfi_rec_set_num (prec, "max", max);
	sfi_rec_set_num (prec, "stepping", stepping);
	sfi_rec_set_num (prec, "default", sfi_pspec_get_num_default (pspec));
      }
      break;
    case SFI_SCAT_REAL:
      {
	SfiReal min = 0, max = 0, stepping = 0;
	sfi_pspec_get_real_range (pspec, &min, &max, &stepping);
	sfi_rec_set_real (prec, "min", min);
	sfi_rec_set_real (prec, "max", max);
	sfi_rec_set_real (prec, "stepping", stepping);
	sfi_rec_set_real (prec, "default", sfi_pspec_get_real_default (pspec));
      }
      break;
    case SFI_SCAT_STRING:
      string = sfi_pspec_get_string_default (pspec);
      if (string)
	sfi_rec_set_string (prec, "default", string);
      break;
    case SFI_SCAT_CHOICE:
      string = sfi_pspec_get_choice_default (pspec);
      if (string)
	sfi_rec_set_string (prec, "default", string);
      seq = choice_values_to_seq (sfi_pspec_get_choice_values (pspec));
      sfi_rec_set_seq (prec, "choice_values", seq);
      sfi_seq_unref (seq);
      break;
    case SFI_SCAT_SEQ:
      sfi_rec_set_pspec (prec, "element", sfi_pspec_get_seq_element (pspec));
      break;
    case SFI_SCAT_REC:
      seq = rec_fields_to_seq (sfi_pspec_get_rec_fields (pspec));
      sfi_rec_set_seq (prec, "record_fields", seq);
      sfi_seq_unref (seq);
      break;
    case SFI_SCAT_NOTE:
      {
	SfiInt min = 0, max = 0, stepping = 0;
	sfi_pspec_get_int_range (pspec, &min, &max, &stepping);
	sfi_rec_set_int (prec, "min", min);
	sfi_rec_set_int (prec, "max", max);
	sfi_rec_set_int (prec, "default", sfi_pspec_get_int_default (pspec));
	if (sfi_pspec_allows_void_note (pspec))
	  sfi_rec_set_bool (prec, "void-notes", TRUE);
      }
      break;
    default:
      break;
    }
  
  /* log scales */
  if (sfi_pspec_get_log_scale (pspec, &log_center, &log_base, &log_n_steps))
    {
      sfi_rec_set_real (prec, "log_center", log_center);
      sfi_rec_set_real (prec, "log_base", log_base);
      sfi_rec_set_real (prec, "log_n_steps", log_n_steps);
    }
  
  return prec;
}

GParamSpec*
sfi_pspec_from_rec (SfiRec *prec)
{
  const gchar *name, *nick, *blurb, *hints, *string;
  GParamSpec *pspec = NULL;
  SfiSCategory scat;
  SfiRecFields zero_rfields = { 0, 0, };
  GType ptype;
  
  g_return_val_if_fail (prec != NULL, NULL);
  
  scat = sfi_rec_get_int (prec, "sfi_scategory");
  name = sfi_rec_get_string (prec, "name");
  ptype = sfi_category_param_type (scat);
  if (!G_TYPE_IS_PARAM (ptype) || !name)
    return NULL;
  nick = sfi_rec_get_string (prec, "nick");
  blurb = sfi_rec_get_string (prec, "blurb");
  hints = sfi_rec_get_string (prec, "options");
  
 reswitch:
  switch (scat)
    {
      TmpChoiceValues *tcv;
      TmpRecordFields *trf;
    case SFI_SCAT_BOOL:
      pspec = sfi_pspec_bool (name, nick, blurb,
			      sfi_rec_get_bool (prec, "default"),
			      hints);
      break;
    case SFI_SCAT_INT:
      pspec = sfi_pspec_int (name, nick, blurb,
			     sfi_rec_get_int (prec, "default"),
			     sfi_rec_get_int (prec, "min"),
			     sfi_rec_get_int (prec, "max"),
			     sfi_rec_get_int (prec, "stepping"),
			     hints);
      break;
    case SFI_SCAT_NUM:
      pspec = sfi_pspec_num (name, nick, blurb,
			     sfi_rec_get_num (prec, "default"),
			     sfi_rec_get_num (prec, "min"),
			     sfi_rec_get_num (prec, "max"),
			     sfi_rec_get_num (prec, "stepping"),
			     hints);
      break;
    case SFI_SCAT_REAL:
      pspec = sfi_pspec_real (name, nick, blurb,
			      sfi_rec_get_real (prec, "default"),
			      sfi_rec_get_real (prec, "min"),
			      sfi_rec_get_real (prec, "max"),
			      sfi_rec_get_real (prec, "stepping"),
			      hints);
      {
	SfiReal log_center = sfi_rec_get_real (prec, "log_center");
	SfiReal log_base = sfi_rec_get_real (prec, "log_base");
	SfiReal log_n_steps = sfi_rec_get_real (prec, "log_n_steps");
	if (log_n_steps >= 1)
	  sfi_pspec_set_log_scale (pspec, log_center, log_base, log_n_steps);
      }
      break;
    case SFI_SCAT_STRING:
      pspec = sfi_pspec_string (name, nick, blurb,
				sfi_rec_get_string (prec, "default"),
				hints);
      break;
    case SFI_SCAT_CHOICE:
      tcv = tmp_choice_values_from_seq (sfi_rec_get_seq (prec, "choice_values"));
      if (tcv)
	{
	  pspec = sfi_pspec_choice (name, nick, blurb,
				    sfi_rec_get_string (prec, "default"),
				    tcv->cvalues, hints);
	  g_param_spec_set_qdata_full (pspec, quark_tmp_choice_values, tcv, (GDestroyNotify) tmp_choice_values_unref);
	}
      break;
    case SFI_SCAT_BBLOCK:
      pspec = sfi_pspec_bblock (name, nick, blurb, hints);
      break;
    case SFI_SCAT_FBLOCK:
      pspec = sfi_pspec_fblock (name, nick, blurb, hints);
      break;
    case SFI_SCAT_PSPEC:
      pspec = sfi_pspec_pspec (name, nick, blurb, hints);
      break;
    case SFI_SCAT_SEQ:
      pspec = sfi_pspec_seq (name, nick, blurb,
			     sfi_rec_get_pspec (prec, "element"),
			     hints);
      break;
    case SFI_SCAT_REC:
      trf = tmp_record_fields_from_seq (sfi_rec_get_seq (prec, "record_fields"));
      if (trf)
	{
	  pspec = sfi_pspec_rec (name, nick, blurb,
				 trf->rfields, hints);
	  g_param_spec_set_qdata_full (pspec, quark_tmp_record_fields, trf, (GDestroyNotify) tmp_record_fields_unref);
	}
      else
	pspec = sfi_pspec_rec (name, nick, blurb,
			       zero_rfields, hints);
      break;
    case SFI_SCAT_PROXY:
      pspec = sfi_pspec_proxy (name, nick, blurb, hints);
      break;
    case SFI_SCAT_NOTE:
      pspec = sfi_pspec_note (name, nick, blurb,
			      sfi_rec_get_int (prec, "default"),
			      sfi_rec_get_int (prec, "min"),
			      sfi_rec_get_int (prec, "max"),
			      sfi_rec_get_bool (prec, "void-notes"),
			      hints);
      break;
    default:
      if (scat & ~SFI_SCAT_TYPE_MASK)
	{
	  scat &= SFI_SCAT_TYPE_MASK;
	  goto reswitch;
	}
      return NULL;
    }

  if (pspec)
    {
      string = sfi_rec_get_string (prec, "owner");
      if (string)
	sfi_pspec_set_owner (pspec, string);
      sfi_pspec_set_group (pspec, sfi_rec_get_string (prec, "group"));
      sfi_pspec_set_options (pspec, sfi_rec_get_string (prec, "options"));
    }

  return pspec;
}

static void
sfi_pspec_copy_commons (GParamSpec *src_pspec,
                        GParamSpec *dest_pspec)
{
  SfiReal log_center, log_base, log_n_steps;
  const gchar *cstring;
  /* skipping name, nick and blurb */
  dest_pspec->flags = src_pspec->flags;
  sfi_pspec_set_options (dest_pspec, sfi_pspec_get_options (src_pspec));
  sfi_pspec_set_group (dest_pspec, sfi_pspec_get_group (src_pspec));
  cstring = sfi_pspec_get_owner (src_pspec);
  if (cstring)
    sfi_pspec_set_owner (dest_pspec, cstring);
  if (sfi_pspec_get_log_scale (src_pspec, &log_center, &log_base, &log_n_steps))
    sfi_pspec_set_log_scale (dest_pspec, log_center, log_base, log_n_steps);
}
