// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "sfiparams.hh"
#include "sfiprimitives.hh"
#include "sfinote.hh"
#include "sfitime.hh"
#include "bse/internal.hh"
#include <string.h>
#include <algorithm>
#include <unordered_map>

#define NULL_CHECKED(x)         ((x) && (x)[0] ? x : NULL)


typedef struct {
  gint          (*values_cmp)           (GParamSpec   *pspec,
                                         const GValue *value1,
                                         const GValue *value2);
  gboolean      (*value_validate)       (GParamSpec   *pspec,
                                         GValue       *value);
  void          (*finalize)             (GParamSpec   *pspec);
} PSpecClassData;

static inline GParamSpec*
param_spec_internal (GType param_type, const char *name, const char *nick, const char *blurb, GParamFlags flags)
{
  return (GParamSpec*) g_param_spec_internal (param_type, name, nick, blurb, flags);
}

/* --- prototypes --- */
static gint     param_bblock_values_cmp (GParamSpec   *pspec,
                                         const GValue *value1,
                                         const GValue *value2);
static gint     param_fblock_values_cmp (GParamSpec   *pspec,
                                         const GValue *value1,
                                         const GValue *value2);
static gint     param_rec_values_cmp    (GParamSpec   *pspec,
                                         const GValue *value1,
                                         const GValue *value2);
static gboolean param_rec_validate      (GParamSpec   *pspec,
                                         GValue       *value);
static gint     param_seq_values_cmp    (GParamSpec   *pspec,
                                         const GValue *value1,
                                         const GValue *value2);
static gboolean param_seq_validate      (GParamSpec   *pspec,
                                         GValue       *value);
static void     param_seq_finalize      (GParamSpec   *pspec);
static gboolean param_note_validate     (GParamSpec   *pspec,
                                         GValue       *value);
static void     param_class_init        (gpointer      klass,
                                         gpointer      class_data);
static void	sfi_pspec_copy_commons	(GParamSpec   *src_pspec,
					 GParamSpec   *dest_pspec);


/* --- variables --- */
GType        *sfi__param_spec_types = NULL;
static GQuark quark_hints = 0;
static GQuark quark_param_group = 0;
static GQuark quark_param_owner = 0;
static GQuark quark_enum_choice_value_getter = 0;
static GQuark quark_tmp_choice_values = 0;
static GQuark quark_tmp_record_fields = 0;
static GQuark quark_boxed_info = 0;


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

  assert_return (sfi__param_spec_types == NULL);

  sfi__param_spec_types = pspec_types;

  quark_hints = g_quark_from_static_string ("sfi-pspec-hints");
  quark_param_group = g_quark_from_static_string ("sfi-pspec-group");
  quark_param_owner = g_quark_from_static_string ("sfi-pspec-owner");
  quark_enum_choice_value_getter = g_quark_from_static_string ("sfi-enum-choice-value-getter");
  quark_tmp_choice_values = g_quark_from_static_string ("sfi-tmp-choice-values");
  quark_tmp_record_fields = g_quark_from_static_string ("sfi-tmp-choice-values");
  quark_boxed_info = g_quark_from_static_string ("sfi-boxed-info");

  /* pspec types */
  info.instance_size = sizeof (SfiParamSpecChoice);
  SFI_TYPE_PARAM_CHOICE = g_type_register_static (G_TYPE_PARAM_STRING, "SfiParamSpecChoice", &info, GTypeFlags (0));
  {
    static const PSpecClassData cdata = {
      param_bblock_values_cmp, NULL,
    };
    info.class_data = &cdata;
    info.instance_size = sizeof (SfiParamSpecBBlock);
    SFI_TYPE_PARAM_BBLOCK = g_type_register_static (G_TYPE_PARAM_BOXED, "SfiParamSpecBBlock", &info, GTypeFlags (0));
  }
  {
    static const PSpecClassData cdata = {
      param_fblock_values_cmp, NULL,
    };
    info.class_data = &cdata;
    info.instance_size = sizeof (SfiParamSpecFBlock);
    SFI_TYPE_PARAM_FBLOCK = g_type_register_static (G_TYPE_PARAM_BOXED, "SfiParamSpecFBlock", &info, GTypeFlags (0));
  }
  {
    static const PSpecClassData cdata = {
      param_seq_values_cmp,
      param_seq_validate,
      param_seq_finalize,
    };
    info.class_data = &cdata;
    info.instance_size = sizeof (SfiParamSpecSeq);
    SFI_TYPE_PARAM_SEQ = g_type_register_static (G_TYPE_PARAM_BOXED, "SfiParamSpecSeq", &info, GTypeFlags (0));
  }
  {
    static const PSpecClassData cdata = {
      param_rec_values_cmp,
      param_rec_validate,
    };
    info.class_data = &cdata;
    info.instance_size = sizeof (SfiParamSpecRec);
    SFI_TYPE_PARAM_REC = g_type_register_static (G_TYPE_PARAM_BOXED, "SfiParamSpecRec", &info, GTypeFlags (0));
  }
  {
    static const PSpecClassData cdata = {
      NULL,
      param_note_validate,
    };
    info.class_data = &cdata;
    info.instance_size = sizeof (SfiParamSpecNote);
    SFI_TYPE_PARAM_NOTE = g_type_register_static (SFI_TYPE_PARAM_INT, "SfiParamSpecNote", &info, GTypeFlags (0));
  }
}

static void
param_class_init (gpointer klass,
                  gpointer class_data)
{
  PSpecClassData *cdata = (PSpecClassData*) class_data;
  if (cdata)
    {
      GParamSpecClass *pclass = G_PARAM_SPEC_CLASS (klass);
      if (cdata->values_cmp)
        pclass->values_cmp = cdata->values_cmp;
      if (cdata->value_validate)
        pclass->value_validate = cdata->value_validate;
      if (cdata->finalize)
        pclass->finalize = cdata->finalize;
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

static void
param_seq_finalize (GParamSpec *pspec)
{
  SfiParamSpecSeq *sspec = SFI_PSPEC_SEQ (pspec);
  if (sspec->element)
    {
      g_param_spec_unref (sspec->element);
      sspec->element = NULL;
    }
  /* chain to parent class' handler */
  GParamSpecClass *klass = (GParamSpecClass*) g_type_class_peek (SFI_TYPE_PARAM_SEQ);
  klass = (GParamSpecClass*) g_type_class_peek_parent (klass);
  klass->finalize (pspec);
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
  assert_return (default_value == TRUE || default_value == FALSE, NULL);

  GParamSpec *pspec = g_param_spec_boolean (name, NULL_CHECKED (nick), NULL_CHECKED (blurb), default_value, GParamFlags (0));
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
  assert_return (default_value >= minimum_value && default_value <= maximum_value, NULL);
  assert_return (minimum_value <= maximum_value, NULL);
  assert_return (minimum_value + stepping <= maximum_value, NULL);

  GParamSpec *pspec = g_param_spec_int (name, NULL_CHECKED (nick), NULL_CHECKED (blurb), minimum_value, maximum_value, default_value, GParamFlags (0));
  sfi_pspec_set_options (pspec, hints);
  g_param_spec_set_istepping (pspec, stepping);

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
  assert_return (default_value >= minimum_value && default_value <= maximum_value, NULL);
  assert_return (minimum_value <= maximum_value, NULL);
  assert_return (minimum_value + stepping <= maximum_value, NULL);

  GParamSpec *pspec = g_param_spec_int64 (name, NULL_CHECKED (nick), NULL_CHECKED (blurb), minimum_value, maximum_value, default_value, GParamFlags (0));
  sfi_pspec_set_options (pspec, hints);
  g_param_spec_set_istepping (pspec, stepping);

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
  assert_return (default_value >= minimum_value && default_value <= maximum_value, NULL);
  assert_return (minimum_value <= maximum_value, NULL);
  assert_return (minimum_value + stepping <= maximum_value, NULL);

  GParamSpec *pspec = g_param_spec_double (name, NULL_CHECKED (nick), NULL_CHECKED (blurb), minimum_value, maximum_value, default_value, GParamFlags (0));
  sfi_pspec_set_options (pspec, hints);
  g_param_spec_set_fstepping (pspec, stepping);
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

  assert_return (n_steps > 0, NULL);
  assert_return (base > 0, NULL);

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
  GParamSpec *pspec = param_spec_internal (SFI_TYPE_PARAM_STRING, name, NULL_CHECKED (nick), NULL_CHECKED (blurb), GParamFlags (0));
  sfi_pspec_set_options (pspec, hints);
  GParamSpecString *sspec = (GParamSpecString*) G_PARAM_SPEC_STRING (pspec);
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
  return sfi_pspec_enum_choice (name, nick, blurb, default_value, "", static_const_cvalues, hints);
}

static GQuark quark_pspec_enum_typename = g_quark_from_static_string ("SfiParamSpec-enum_typename");

GParamSpec*
sfi_pspec_enum_choice (const char *name, const char *nick, const char *blurb, const char *default_value,
                       const std::string &enum_typename, SfiChoiceValues static_const_cvalues, const char *hints)
{
  assert_return (static_const_cvalues.n_values >= 1, NULL);

  GParamSpec *pspec = param_spec_internal (SFI_TYPE_PARAM_CHOICE, name, NULL_CHECKED (nick), NULL_CHECKED (blurb), GParamFlags (0));
  sfi_pspec_set_options (pspec, hints);
  GParamSpecString *sspec = G_PARAM_SPEC_STRING (pspec);
  g_free (sspec->default_value);
  sspec->default_value = g_strdup (default_value ? default_value : static_const_cvalues.values[0].choice_ident);
  SfiParamSpecChoice *cspec = SFI_PSPEC_CHOICE (pspec);
  cspec->cvalues = static_const_cvalues;
  pspec->value_type = SFI_TYPE_CHOICE;
  if (!enum_typename.empty())
    g_param_spec_set_qdata (pspec, quark_pspec_enum_typename, (gchar*) g_intern_string (enum_typename.c_str()));

  return pspec;
}

const char*
sfi_pspec_get_enum_typename (GParamSpec *pspec)
{
  void *data = g_param_spec_get_qdata (pspec, quark_pspec_enum_typename);
  return data ? (char*) data : "";
}

GParamSpec*
sfi_pspec_bblock (const gchar    *name,
		  const gchar    *nick,
		  const gchar    *blurb,
		  const gchar    *hints)
{
  // SfiParamSpecBBlock *bspec;
  GParamSpec *pspec = param_spec_internal (SFI_TYPE_PARAM_BBLOCK, name, NULL_CHECKED (nick), NULL_CHECKED (blurb), GParamFlags (0));
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
  // SfiParamSpecFBlock *fspec;
  GParamSpec *pspec = param_spec_internal (SFI_TYPE_PARAM_FBLOCK, name, NULL_CHECKED (nick), NULL_CHECKED (blurb), GParamFlags (0));
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
  // SfiParamSpecPSpec *pspec;
  GParamSpec *pspec = param_spec_internal (SFI_TYPE_PARAM_PSPEC, name, NULL_CHECKED (nick), NULL_CHECKED (blurb), GParamFlags (0));
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
  if (element_spec)
    assert_return (G_IS_PARAM_SPEC (element_spec), NULL);

  GParamSpec *pspec = param_spec_internal (SFI_TYPE_PARAM_SEQ, name, NULL_CHECKED (nick), NULL_CHECKED (blurb), GParamFlags (0));
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
  GParamSpec *pspec = param_spec_internal (SFI_TYPE_PARAM_REC, name, NULL_CHECKED (nick), NULL_CHECKED (blurb), GParamFlags (0));
  sfi_pspec_set_options (pspec, hints);
  SfiParamSpecRec *rspec = SFI_PSPEC_REC (pspec);
  rspec->fields = static_const_fields;
  pspec->value_type = SFI_TYPE_REC;

  return pspec;
}

GParamSpec*
sfi_pspec_rec_generic (const gchar    *name,
                       const gchar    *nick,
                       const gchar    *blurb,
                       const gchar    *hints)
{
  static const SfiRecFields empty_fields = { 0, NULL };
  return sfi_pspec_rec (name, nick, blurb, empty_fields, hints);
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
      assert_return (min_note <= max_note, NULL);
      assert_return (default_value == SFI_NOTE_VOID && allow_void == TRUE, NULL);
    }
  else
    assert_return (default_value >= min_note && default_value <= max_note, NULL);

  pspec = param_spec_internal (SFI_TYPE_PARAM_NOTE, name, NULL_CHECKED (nick), NULL_CHECKED (blurb), GParamFlags (0));
  nspec = SFI_PSPEC_NOTE (pspec);
  ispec = G_PARAM_SPEC_INT (pspec);
  ispec->minimum = CLAMP (min_note, SFI_MIN_NOTE, SFI_MAX_NOTE);
  ispec->maximum = CLAMP (max_note, SFI_MIN_NOTE, SFI_MAX_NOTE);
  ispec->default_value = default_value;
  nspec->allow_void = allow_void != FALSE;
  g_param_spec_set_istepping (pspec, 12);
  thints = g_strconcat ("note:", hints, NULL);
  sfi_pspec_set_options (pspec, thints);
  g_free (thints);
  return pspec;
}


/* --- conversion --- */
enum {
  BOXED_RECORD = 1,
  BOXED_SEQUENCE
};
typedef struct {
  guint       n_fields : 24;
  guint       boxed_kind : 8;
  GParamSpec *fields[1]; /* variable length array */
} BoxedInfo;

void
sfi_boxed_type_set_rec_fields (GType               boxed_type,
                               const SfiRecFields  static_const_fields)
{
  BoxedInfo *binfo = (BoxedInfo*) g_type_get_qdata (boxed_type, quark_boxed_info);
  assert_return (G_TYPE_IS_BOXED (boxed_type));
  if (static_const_fields.n_fields)
    {
      binfo = (BoxedInfo*) g_realloc (binfo, (sizeof (BoxedInfo) +
                                              sizeof (binfo->fields[0]) * (static_const_fields.n_fields - 1)));
      binfo->n_fields = static_const_fields.n_fields;
      memcpy (binfo->fields, static_const_fields.fields, sizeof (binfo->fields[0]) * binfo->n_fields);
      binfo->boxed_kind = BOXED_RECORD;
    }
  else
    {
      g_free (binfo);
      binfo = NULL;
    }
  g_type_set_qdata (boxed_type, quark_boxed_info, binfo);
}

SfiRecFields
sfi_boxed_type_get_rec_fields (GType boxed_type)
{
  BoxedInfo *binfo = (BoxedInfo*) g_type_get_qdata (boxed_type, quark_boxed_info);
  SfiRecFields rfields = { 0, NULL };
  assert_return (G_TYPE_IS_BOXED (boxed_type), rfields);
  if (binfo && binfo->boxed_kind == BOXED_RECORD)
    {
      rfields.n_fields = binfo->n_fields;
      rfields.fields = binfo->fields;
    }
  return rfields;
}

void
sfi_boxed_type_set_seq_element (GType               boxed_type,
                                GParamSpec         *element)
{
  BoxedInfo *binfo = (BoxedInfo*) g_type_get_qdata (boxed_type, quark_boxed_info);
  assert_return (G_TYPE_IS_BOXED (boxed_type));
  guint i;
  for (i = 0; i < (binfo ? binfo->n_fields : 0); i++)
    if (binfo->fields[i])
      g_param_spec_unref (binfo->fields[i]);
  if (element)
    {
      binfo = (BoxedInfo*) g_realloc (binfo, sizeof (BoxedInfo));
      binfo->n_fields = 1;
      binfo->fields[0] = g_param_spec_ref (element);
      g_param_spec_sink (element);
      binfo->boxed_kind = BOXED_SEQUENCE;
    }
  else
    {
      g_free (binfo);
      binfo = NULL;
    }
  g_type_set_qdata (boxed_type, quark_boxed_info, binfo);
}

GParamSpec*
sfi_boxed_type_get_seq_element (GType               boxed_type)
{
  BoxedInfo *binfo = (BoxedInfo*) g_type_get_qdata (boxed_type, quark_boxed_info);
  GParamSpec *pspec = NULL;
  assert_return (G_TYPE_IS_BOXED (boxed_type), NULL);
  if (binfo && binfo->boxed_kind == BOXED_SEQUENCE)
    pspec = binfo->fields[0];
  return pspec;
}

void
sfi_enum_type_set_choice_value_getter (GType                 gtype,
                                       SfiChoiceValueGetter  cvgetter)
{
  assert_return (G_TYPE_IS_ENUM (gtype));
  if (g_type_get_qdata (gtype, quark_tmp_choice_values) != NULL)
    Bse::warning ("%s: unsetting choice value getter of type `%s' while keeping old choice value references", __func__, g_type_name (gtype));
  g_type_set_qdata (gtype, quark_enum_choice_value_getter, (void*) cvgetter);
}

static SfiSeq*
choice_values_to_seq (const SfiChoiceValues cvalues)
{
  SfiSeq *seq = sfi_seq_new ();
  guint i;
  for (i = 0; i < cvalues.n_values; i++)
    {
      sfi_seq_append_string (seq, cvalues.values[i].choice_ident);
      sfi_seq_append_string (seq, cvalues.values[i].choice_label);
      sfi_seq_append_string (seq, cvalues.values[i].choice_blurb);
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
  assert_return (trf != NULL);
  assert_return (trf->ref_count > 0);

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
  guint           free_values : 1;
  GEnumClass     *eclass; /* !eclass => free (cvalues[*].values.choice_ident) */
  SfiChoiceValues cvalues;
} TmpChoiceValues;

static void
tmp_choice_values_unref (TmpChoiceValues *tcv)
{
  assert_return (tcv != NULL);
  assert_return (tcv->ref_count > 0);

  tcv->ref_count--;
  if (!tcv->ref_count)
    {
      if (tcv->free_values)
        {
          guint i;
          for (i = 0; i < tcv->cvalues.n_values; i++)
            {
              g_free ((char*) tcv->cvalues.values[i].choice_ident);
              g_free ((char*) tcv->cvalues.values[i].choice_label);
              g_free ((char*) tcv->cvalues.values[i].choice_blurb);
            }
        }
      g_free ((gpointer) tcv->cvalues.values);
      if (tcv->eclass)	/* indicates from eclass or sequence */
	{
	  g_type_set_qdata (G_TYPE_FROM_CLASS (tcv->eclass), quark_tmp_choice_values, NULL);
	  g_type_class_unref (tcv->eclass);
	}
      g_free (tcv);
    }
}

static TmpChoiceValues*
tmp_choice_values_from_seq (SfiSeq *seq)
{
  if (seq)
    {
      guint i, l = sfi_seq_length (seq), n = l / 3;
      /* check that we got a sequence from choice_values_to_seq() */
      if (n && l == n * 3 && sfi_seq_check (seq, SFI_TYPE_SFI_STRING))
	{
	  TmpChoiceValues *tcv = g_new0 (TmpChoiceValues, 1);
	  tcv->ref_count = 1;
          tcv->free_values = TRUE;
	  tcv->eclass = NULL;
	  tcv->cvalues.n_values = n;
	  SfiChoiceValue *cvalues = g_new0 (SfiChoiceValue, tcv->cvalues.n_values);
	  tcv->cvalues.values = cvalues;
	  for (i = 0; i < tcv->cvalues.n_values; i++)
	    {
	      cvalues[i].choice_ident = g_strdup (sfi_seq_get_string (seq, 3 * i + 0));
	      cvalues[i].choice_label = g_strdup (sfi_seq_get_string (seq, 3 * i + 1));
	      cvalues[i].choice_blurb = g_strdup (sfi_seq_get_string (seq, 3 * i + 2));
	      if (!cvalues[i].choice_ident)	/* aparently invalid sequence */
		{
		  tmp_choice_values_unref (tcv);
		  return NULL;
		}
	    }
	  return tcv;
	}
    }
  return NULL;
}

static TmpChoiceValues*
tmp_choice_values_from_enum (GEnumClass *eclass)
{
  TmpChoiceValues *tcv = (TmpChoiceValues*) g_type_get_qdata (G_TYPE_FROM_CLASS (eclass), quark_tmp_choice_values);
  if (!tcv)
    {
      guint i;
      tcv = g_new0 (TmpChoiceValues, 1);
      tcv->ref_count = 1;
      tcv->free_values = FALSE;
      tcv->eclass = (GEnumClass*) g_type_class_ref (G_TYPE_FROM_CLASS (eclass));
      SfiChoiceValues ccvalues = { 0, };
      SfiChoiceValueGetter cvgetter = (SfiChoiceValueGetter) g_type_get_qdata (G_TYPE_FROM_CLASS (eclass), quark_enum_choice_value_getter);
      if (cvgetter)
        ccvalues = cvgetter (G_TYPE_FROM_CLASS (eclass));
      tcv->cvalues.n_values = ccvalues.n_values ? MIN (eclass->n_values, ccvalues.n_values) : eclass->n_values;
      SfiChoiceValue *cvalues = g_new0 (SfiChoiceValue, tcv->cvalues.n_values);
      tcv->cvalues.values = cvalues;
      for (i = 0; i < tcv->cvalues.n_values; i++)
        if (ccvalues.n_values)
          {
            cvalues[i].choice_ident = ccvalues.values[i].choice_ident;
            cvalues[i].choice_label = ccvalues.values[i].choice_label;
            cvalues[i].choice_blurb = ccvalues.values[i].choice_blurb;
          }
        else
          {
            cvalues[i].choice_ident = eclass->values[i].value_name;
            cvalues[i].choice_label = eclass->values[i].value_nick;
          }
      g_type_set_qdata (G_TYPE_FROM_CLASS (eclass), quark_tmp_choice_values, tcv);
    }
  else
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

  assert_return (G_IS_PARAM_SPEC_ENUM (enum_pspec), NULL);

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
sfi_pspec_to_serializable (GParamSpec *xpspec)
{
  GParamSpec *pspec = NULL;

  assert_return (G_IS_PARAM_SPEC (xpspec), NULL);

  if (sfi_categorize_pspec (xpspec))
    pspec = g_param_spec_ref (xpspec);
  else if (G_IS_PARAM_SPEC_BOXED (xpspec))
    {
      SfiRecFields rfields = sfi_boxed_type_get_rec_fields (G_PARAM_SPEC_VALUE_TYPE (xpspec));
      GParamSpec *element = sfi_boxed_type_get_seq_element (G_PARAM_SPEC_VALUE_TYPE (xpspec));
      if (rfields.n_fields)
        {
          pspec = sfi_pspec_rec (xpspec->name, xpspec->_nick, xpspec->_blurb, rfields, NULL);
          sfi_pspec_copy_commons (xpspec, pspec);
        }
      else if (element)
        {
          pspec = sfi_pspec_seq (xpspec->name, xpspec->_nick, xpspec->_blurb, element, NULL);
          sfi_pspec_copy_commons (xpspec, pspec);
        }
    }
  else if (G_IS_PARAM_SPEC_ENUM (xpspec))
    pspec = sfi_pspec_choice_from_enum (xpspec);

  if (!pspec)
    Bse::warning ("%s: unable to convert non serializable pspec \"%s\" of type `%s'",
                  G_STRLOC, xpspec->name, g_type_name (G_PARAM_SPEC_VALUE_TYPE (xpspec)));

  return pspec;
}


/* --- pspec accessors --- */
GParamSpec *
sfi_pspec_set_group (GParamSpec  *pspec,
                     const gchar *group)
{
  assert_return (G_IS_PARAM_SPEC (pspec), pspec);

  g_param_spec_set_qdata_full (pspec, quark_param_group, g_strdup (group), group ? g_free : NULL);
  return pspec;
}

const gchar*
sfi_pspec_get_group (GParamSpec *pspec)
{
  assert_return (G_IS_PARAM_SPEC (pspec), NULL);
  return (const char*) g_param_spec_get_qdata (pspec, quark_param_group);
}

void
sfi_pspec_set_owner (GParamSpec  *pspec,
		     const gchar *owner)
{
  assert_return (G_IS_PARAM_SPEC (pspec));
  assert_return (owner != NULL);

  g_param_spec_set_qdata_full (pspec, quark_param_owner, g_strdup (owner), g_free);
}

const gchar*
sfi_pspec_get_owner (GParamSpec *pspec)
{
  assert_return (G_IS_PARAM_SPEC (pspec), NULL);

  const char *owner = (char*) g_param_spec_get_qdata (pspec, quark_param_owner);
  if (!owner && pspec->owner_type)
    {
      owner = g_type_name (pspec->owner_type);
      g_param_spec_set_qdata (pspec, quark_param_owner, (void*) owner);
    }
  return owner;
}

#if 0
static void
sfi_pspec_set_void_note (GParamSpec *pspec,
			 gboolean    allow_void)
{
  SfiParamSpecNote *nspec;

  assert_return (SFI_IS_PSPEC_NOTE (pspec));

  nspec = SFI_PSPEC_NOTE (pspec);
  nspec->allow_void = allow_void != FALSE;
}
#endif

gboolean
sfi_pspec_allows_void_note (GParamSpec *pspec)
{
  assert_return (G_IS_PARAM_SPEC (pspec), FALSE);

  return SFI_IS_PSPEC_NOTE (pspec) && SFI_PSPEC_NOTE (pspec)->allow_void;
}

typedef struct {
  SfiReal center;
  SfiReal base;
  SfiReal n_steps;
} LogScale;

SfiBool
sfi_pspec_get_bool_default (GParamSpec *pspec)
{
  assert_return (SFI_IS_PSPEC_BOOL (pspec), FALSE);

  return SFI_PSPEC_BOOL (pspec)->default_value;
}

SfiInt
sfi_pspec_get_int_default (GParamSpec *pspec)
{
  assert_return (SFI_IS_PSPEC_INT (pspec), 0);

  return SFI_PSPEC_INT (pspec)->default_value;
}

void
sfi_pspec_get_int_range (GParamSpec *pspec,
                         SfiInt     *minimum_value,
                         SfiInt     *maximum_value,
                         SfiInt     *stepping)
{
  SfiParamSpecInt *ispec;

  assert_return (SFI_IS_PSPEC_INT (pspec));

  ispec = SFI_PSPEC_INT (pspec);
  if (minimum_value)
    *minimum_value = ispec->minimum;
  if (maximum_value)
    *maximum_value = ispec->maximum;
  if (stepping)
    *stepping = g_param_spec_get_istepping (pspec);
}

SfiNum
sfi_pspec_get_num_default (GParamSpec *pspec)
{
  assert_return (SFI_IS_PSPEC_NUM (pspec), 0);

  return SFI_PSPEC_NUM (pspec)->default_value;
}

void
sfi_pspec_get_num_range (GParamSpec *pspec,
                         SfiNum     *minimum_value,
                         SfiNum     *maximum_value,
                         SfiNum     *stepping)
{
  SfiParamSpecNum *nspec;

  assert_return (SFI_IS_PSPEC_NUM (pspec));

  nspec = SFI_PSPEC_NUM (pspec);
  if (minimum_value)
    *minimum_value = nspec->minimum;
  if (maximum_value)
    *maximum_value = nspec->maximum;
  if (stepping)
    *stepping = g_param_spec_get_istepping (pspec);
}

SfiReal
sfi_pspec_get_real_default (GParamSpec *pspec)
{
  assert_return (SFI_IS_PSPEC_REAL (pspec), 0);

  return SFI_PSPEC_REAL (pspec)->default_value;
}

void
sfi_pspec_get_real_range (GParamSpec *pspec,
                          SfiReal    *minimum_value,
                          SfiReal    *maximum_value,
                          SfiReal    *stepping)
{
  SfiParamSpecReal *nspec;

  assert_return (SFI_IS_PSPEC_REAL (pspec));

  nspec = SFI_PSPEC_REAL (pspec);
  if (minimum_value)
    *minimum_value = nspec->minimum;
  if (maximum_value)
    *maximum_value = nspec->maximum;
  if (stepping)
    *stepping = g_param_spec_get_fstepping (pspec);
}

const gchar*
sfi_pspec_get_string_default (GParamSpec *pspec)
{
  assert_return (SFI_IS_PSPEC_STRING (pspec), NULL);

  return SFI_PSPEC_STRING (pspec)->default_value;
}

const gchar*
sfi_pspec_get_choice_default (GParamSpec *pspec)
{
  assert_return (SFI_IS_PSPEC_CHOICE (pspec), NULL);

  return G_PARAM_SPEC_STRING (pspec)->default_value;
}

SfiChoiceValues
sfi_pspec_get_choice_values (GParamSpec *pspec)
{
  SfiParamSpecChoice *cspec;
  SfiChoiceValues dummy = { 0, };

  assert_return (SFI_IS_PSPEC_CHOICE (pspec), dummy);

  cspec = SFI_PSPEC_CHOICE (pspec);
  return cspec->cvalues;
}

guint64
sfi_pspec_get_choice_hash (GParamSpec *pspec)
{
  SfiParamSpecChoice *cspec;
  guint64 hash;
  guint i;
  assert_return (SFI_IS_PSPEC_CHOICE (pspec), 0);
  cspec = SFI_PSPEC_CHOICE (pspec);
  /* choices are not registered with the type system,
   * so have no unique identifier. for some purposes
   * an approximation of "unique" is good enough though,
   * so we offer a simple hashing function here.
   */
  hash = cspec->cvalues.n_values << 30;
  for (i = 0; i < cspec->cvalues.n_values; i++)
    hash = (hash << 7) - hash + g_str_hash (cspec->cvalues.values[i].choice_ident);
  return hash;
}

GParamSpec*
sfi_pspec_get_seq_element (GParamSpec *pspec)
{
  SfiParamSpecSeq *sspec;

  assert_return (SFI_IS_PSPEC_SEQ (pspec), NULL);

  sspec = SFI_PSPEC_SEQ (pspec);
  return sspec->element;
}

SfiRecFields
sfi_pspec_get_rec_fields (GParamSpec *pspec)
{
  SfiParamSpecRec *rspec;
  SfiRecFields dummy = { 0, };

  assert_return (SFI_IS_PSPEC_REC (pspec), dummy);

  rspec = SFI_PSPEC_REC (pspec);
  return rspec->fields;
}

GParamSpec*
sfi_pspec_get_rec_field (GParamSpec  *pspec,
                         const gchar *field)
{
  SfiParamSpecRec *rspec;
  guint i;

  assert_return (SFI_IS_PSPEC_REC (pspec), NULL);

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
    case SFI_SCAT_STRING:      return SFI_TYPE_SFI_STRING;
    case SFI_SCAT_CHOICE:      return SFI_TYPE_CHOICE;
    case SFI_SCAT_BBLOCK:      return SFI_TYPE_BBLOCK;
    case SFI_SCAT_FBLOCK:      return SFI_TYPE_FBLOCK;
    case SFI_SCAT_PSPEC:       return SFI_TYPE_PSPEC;
    case SFI_SCAT_SEQ:         return SFI_TYPE_SEQ;
    case SFI_SCAT_REC:         return SFI_TYPE_REC;
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
    case SFI_SCAT_NOTE:         return SFI_TYPE_PARAM_NOTE;
    default:
      if (cat_type & ~SFI_SCAT_TYPE_MASK)
	return sfi_category_param_type (SfiSCategory (cat_type & SFI_SCAT_TYPE_MASK));
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
    }
  /* FAILED to determine category */
  return SFI_SCAT_INVAL;
}

SfiSCategory
sfi_categorize_pspec (GParamSpec *pspec)
{
  GType value_type, pspec_type;
  SfiSCategory cat;

  assert_return (G_IS_PARAM_SPEC (pspec), SFI_SCAT_INVAL);

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

  assert_return (pspec != NULL, NULL);

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
  SfiRecFields zero_rfields = { 0, 0, };
  GType ptype;

  assert_return (prec != NULL, NULL);

  SfiSCategory scat = (SfiSCategory) sfi_rec_get_int (prec, "sfi_scategory");
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
	  scat = SfiSCategory (scat & SFI_SCAT_TYPE_MASK);
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

namespace Bse { // bsecore

GParamSpec*
pspec_from_key_value_list (const std::string &name, const Aida::StringVector &introspection)
{
  return introspection_field_to_param_spec (name, introspection, "");
}

static std::map<String, SfiChoiceValues> aida_enum_choice_map;

SfiChoiceValues
choice_values_from_enum (const String &enum_typename)
{
  SfiChoiceValues &cv = aida_enum_choice_map[enum_typename];
  if (!cv.values)
    {
      const StringVector &kvlist = Aida::Introspection::find_type (enum_typename);
      const String enumerator_list = Aida::Introspection::find_value ("enumerators", kvlist);
      const StringVector enumerators = Bse::string_split (enumerator_list, ",");
      if (enumerators.empty())
        return cv;
      SfiChoiceValue *vv = new SfiChoiceValue[enumerators.size()];
      for (size_t i = 0; i < enumerators.size(); i++)
        {
          const String enumerator = enumerators[i];
          const String label = Aida::Introspection::find_value (enumerator + ".label", kvlist);
          const String blurb = Aida::Introspection::find_value (enumerator + ".blurb", kvlist);
          vv[i].choice_ident = g_strdup (enumerator.c_str());
          vv[i].choice_label = g_strdup (label.c_str());
          vv[i].choice_blurb = g_strdup (blurb.c_str());
        }
      cv.n_values = enumerators.size();
      cv.values = vv;
    }
  return cv;
}

SfiChoiceValues
introspection_enum_to_choice_values (const Aida::StringVector &introspection, const String &enumname)
{
  SfiChoiceValues &cv = aida_enum_choice_map[enumname];
  if (!cv.values && !introspection.empty())
    {
      std::vector<SfiChoiceValue> vv;
      for (const String &string : introspection)
        {
          const char *s = string.c_str();
          const char *eq = strchr (s, '=');
          if (eq && eq - s > 7 && strncmp (eq - 6, ".value=", 7) == 0)
            {
              const String ident = string.substr (0, eq - 6 - s);
              vv.resize (vv.size() + 1);
              SfiChoiceValue &v = vv.back();
              v.choice_ident = g_strdup (ident.c_str());
              const String label = Aida::aux_vector_find (introspection, ident, "label");
              const String blurb = Aida::aux_vector_find (introspection, ident, "blurb");
              if (!label.empty())
                v.choice_label = g_strdup (label.c_str());
              if (!blurb.empty())
                v.choice_blurb = g_strdup (blurb.c_str());
            }
        }
      cv.n_values = vv.size();
      SfiChoiceValue *cvalues = new SfiChoiceValue[cv.n_values];
      std::copy_n (vv.begin(), cv.n_values, cvalues);
      cv.values = cvalues;
    }
  return cv;
}

static bool
sfi_pspecs_rec_fields_cache (const String &rec_typename, SfiRecFields *rf, bool assign)
{
  static std::unordered_map<String, SfiRecFields> rec_fields_map;
  if (assign)
    {
      rec_fields_map[rec_typename] = *rf;
      return true;
    }
  auto it = rec_fields_map.find (rec_typename);
  if (it == rec_fields_map.end())
    return false;
  *rf = it->second;
  return true;
}

GParamSpec*
introspection_field_to_param_spec (const std::string &fieldname, const Aida::StringVector &introspection, const String &subname)
{
  using Aida::StringVector;
  const String type = Aida::aux_vector_find (introspection, subname, "type");
  const String fundamental = Aida::Introspection::find_type_kind (type);
  const String label = Aida::aux_vector_find (introspection, subname, "label");
  const String blurb = Aida::aux_vector_find (introspection, subname, "blurb");
  const String hints = Aida::aux_vector_find (introspection, subname, "hints");
  const String group = Aida::aux_vector_find (introspection, subname, "group");
  GParamSpec *pspec = NULL;
  if (fundamental == "BOOL")
    {
      const String dflt = Aida::aux_vector_find (introspection, subname, "default");
      pspec = sfi_pspec_bool (fieldname.c_str(), label.c_str(), blurb.c_str(), Bse::string_to_bool (dflt), hints.c_str());
    }
  else if (fundamental == "STRING")
    {
      const String dflt = Aida::aux_vector_find (introspection, subname, "default");
      pspec = sfi_pspec_string (fieldname.c_str(), label.c_str(), blurb.c_str(), dflt.c_str(), hints.c_str());
    }
  else if (fundamental == "INT32" || fundamental == "INT64")
    {
      const String dflt = Aida::aux_vector_find (introspection, subname, "default");
      const int64 df = Bse::string_to_int (dflt);
      const int64 st = Bse::string_to_int (Aida::aux_vector_find (introspection, subname, "step"));
      const int64 mi = Bse::string_to_int (Aida::aux_vector_find (introspection, subname, "min", "-9223372036854775808"));
      const int64 ma = Bse::string_to_int (Aida::aux_vector_find (introspection, subname, "max", "+9223372036854775807"));
      pspec = sfi_pspec_num (fieldname.c_str(), label.c_str(), blurb.c_str(), df, mi, ma, st, hints.c_str());
    }
  else if (fundamental == "FLOAT64")
    {
      const String dflt = Aida::aux_vector_find (introspection, subname, "default");
      const double df = Bse::string_to_double (dflt);
      const double st = Bse::string_to_double (Aida::aux_vector_find (introspection, subname, "step"));
      const double mi = Bse::string_to_double (Aida::aux_vector_find (introspection, subname, "min", "-1.79769313486231570815e+308"));
      const double ma = Bse::string_to_double (Aida::aux_vector_find (introspection, subname, "max", "+1.79769313486231570815e+308"));
      pspec = sfi_pspec_real (fieldname.c_str(), label.c_str(), blurb.c_str(), df, mi, ma, st, hints.c_str());
    }
  else if (fundamental == "ENUM")
    {
      const String dflt = Aida::aux_vector_find (introspection, subname, "default");
      const String enum_typename = type;
      const StringVector &enum_introspection = Aida::Introspection::find_type (enum_typename);
      SfiChoiceValues cvalues = introspection_enum_to_choice_values (enum_introspection, enum_typename);
      pspec = sfi_pspec_enum_choice (fieldname.c_str(), label.c_str(), blurb.c_str(), dflt.c_str(), enum_typename, cvalues, hints.c_str());
    }
  else if (fundamental == "SEQUENCE")
    {
      const String seq_typename = type;
      const StringVector &seq_introspection = Aida::Introspection::find_type (seq_typename);
      const StringVector seq_fieldnames = introspection_list_field_names (seq_introspection);
      if (seq_fieldnames.size() == 1)
        {
          GParamSpec *field_pspec = introspection_field_to_param_spec (seq_fieldnames[0], seq_introspection, seq_fieldnames[0]);
          if (field_pspec)
            pspec = sfi_pspec_seq (fieldname.c_str(), label.c_str(), blurb.c_str(), field_pspec, hints.c_str());
        }
    }
  else if (fundamental == "RECORD")
    {
      const String rec_typename = type;
      const StringVector &rec_introspection = Aida::Introspection::find_type (rec_typename);
      SfiRecFields rec_fields;
      if (!sfi_pspecs_rec_fields_cache (rec_typename, &rec_fields, false))
        {
          std::vector<GParamSpec*> pspecs = introspection_fields_to_param_list (rec_introspection);
          if (pspecs.size())
            {
              rec_fields.n_fields = pspecs.size();
              rec_fields.fields = g_new0 (GParamSpec*, rec_fields.n_fields);
              for (size_t i = 0; i < rec_fields.n_fields; i++)
                {
                  g_param_spec_ref (pspecs[i]);
                  g_param_spec_sink (pspecs[i]);
                  rec_fields.fields[i] = pspecs[i];
                }
              sfi_pspecs_rec_fields_cache (rec_typename, &rec_fields, true);
            }
        }
      if (rec_fields.n_fields)
        pspec = sfi_pspec_rec (fieldname.c_str(), label.c_str(), blurb.c_str(), rec_fields, hints.c_str());
    }
  if (pspec)
    {
      if (!group.empty())
        sfi_pspec_set_group (pspec, group.c_str());
      g_param_spec_ref (pspec);
      g_param_spec_sink (pspec);
    }
  return pspec;
}

std::vector<GParamSpec*>
introspection_fields_to_param_list (const Aida::StringVector &introspection)
{
  using Aida::StringVector;
  std::vector<GParamSpec*> pspecs;
  for (const String &fieldname : introspection_list_field_names (introspection))
    {
      GParamSpec *pspec = introspection_field_to_param_spec (fieldname, introspection, fieldname);
      if (pspec)
        pspecs.push_back (pspec);
    }
  return pspecs;
}

Aida::StringVector
introspection_list_field_names (const Aida::StringVector &introspection)
{
  Aida::StringVector fieldnames;
  for (const String &string : introspection)
    {
      const char *s = string.c_str();
      const char *eq = strchr (s, '=');
      if (eq && eq - s > 6 && strncmp (eq - 5, ".type=", 6) == 0)
        {
          const String ident = string.substr (0, eq - 5 - s);
          fieldnames.push_back (ident);
        }
    }
  return fieldnames;
}

} // Bse

// == Testing ==
#include "testing.hh"
namespace { // Anon
using namespace Bse;

#define print_vector(v, fmt)       ({ printf ("{"); for (const auto e : v) printf (" " fmt, e); printf (" }\n"); })

BSE_INTEGRITY_TEST (sfi_vector_tests);
static void
sfi_vector_tests()
{
  std::vector<int> ref;
  std::vector<int> v1 = { +17, +7, +5, +3, +1, -1, -3, -5, -7, -19 };
  std::vector<int> v2 = { +27, -7, +4, -2, 0, -1, +2, -6, +7, -12, +66 };
  std::vector<int> rv;
  copy_reordered (v1.begin(), v1.end(), v2.begin(), v2.end(), std::back_inserter (rv));
  ref = { -7, -1, +7, +17, +5, +3, +1, -3, -5, -19 };
  TASSERT (rv == ref); // print_vector (rv, "%d");
  vector_erase_element (v1, -3);    ref = { +17, +7, +5, +3, +1, -1, -5, -7, -19 };   TASSERT (v1 == ref);
  vector_erase_element (v1, +5);    ref = { +17, +7, +3, +1, -1, -5, -7, -19 };       TASSERT (v1 == ref);
  vector_erase_element (v1, -5);    ref = { +17, +7, +3, +1, -1, -7, -19 };           TASSERT (v1 == ref);
  vector_erase_element (v1, -19);   ref = { +17, +7, +3, +1, -1, -7 };                TASSERT (v1 == ref);
  vector_erase_element (v1, +17);   ref = { +7, +3, +1, -1, -7 };                     TASSERT (v1 == ref);
  vector_erase_element (v1, +77);   ref = { +7, +3, +1, -1, -7 };                     TASSERT (v1 == ref);
  vector_erase_element (v1, 0);     ref = { +7, +3, +1, -1, -7 };                     TASSERT (v1 == ref);
  rv.clear(); copy_reordered (v1.begin(), v1.end(), v2.begin(), v2.end(), std::back_inserter (rv));
  ref = { -7, -1, +7, +3, +1 };                                 TASSERT (rv == ref);
  v1.push_back (0);
  rv.clear(); copy_reordered (v1.begin(), v1.end(), v2.begin(), v2.end(), std::back_inserter (rv));
  ref = { -7, 0, -1, +7, +3, +1 };                              TASSERT (rv == ref);
  v1.push_back (0);
  rv.clear(); copy_reordered (v1.begin(), v1.end(), v2.begin(), v2.end(), std::back_inserter (rv));
  ref = { -7, 0, -1, +7, +3, +1, 0 };                           TASSERT (rv == ref);
  v1.push_back (+2);
  rv.clear(); copy_reordered (v1.begin(), v1.end(), v2.begin(), v2.end(), std::back_inserter (rv));
  ref = { -7, 0, -1, +2, +7, +3, +1, 0 };                       TASSERT (rv == ref);
  v1.resize (201);
  std::iota (v1.begin(), v1.end(), -100); // v1 = -100 .. +100
  rv.clear(); copy_reordered (v2.begin(), v2.end(), v1.begin(), v1.end(), std::back_inserter (rv));
  ref = { -12, -7, -6, -2, -1, 0, +2, +4, +7, +27, +66 };       TASSERT (rv == ref);
}

} // Anon
