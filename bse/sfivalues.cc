// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "sfivalues.hh"
#include "sfiprimitives.hh"
#include "sfiparams.hh"
#include "sfimemory.hh"
#include "bse/internal.hh"


/* --- variables --- */
GType	*sfi__value_types = NULL;


/* --- functions --- */
static gpointer
copy_seq (gpointer boxed)
{
  SfiSeq *seq = (SfiSeq*) boxed;
  return seq ? sfi_seq_ref (seq) : NULL;
}

static void
free_seq (gpointer boxed)
{
  SfiSeq *seq = (SfiSeq*) boxed;
  if (seq)
    sfi_seq_unref (seq);
}

static gpointer
copy_rec (gpointer boxed)
{
  SfiRec *rec = (SfiRec*) boxed;
  return rec ? sfi_rec_ref (rec) : NULL;
}

static void
free_rec (gpointer boxed)
{
  SfiRec *rec = (SfiRec*) boxed;
  if (rec)
    sfi_rec_unref (rec);
}

static gpointer
copy_bblock (gpointer boxed)
{
  SfiBBlock *bblock = (SfiBBlock*) boxed;
  return bblock ? sfi_bblock_ref (bblock) : NULL;
}

static void
free_bblock (gpointer boxed)
{
  SfiBBlock *bblock = (SfiBBlock*) boxed;
  if (bblock)
    sfi_bblock_unref (bblock);
}

static gpointer
copy_fblock (gpointer boxed)
{
  SfiFBlock *fblock = (SfiFBlock*) boxed;
  return fblock ? sfi_fblock_ref (fblock) : NULL;
}

static void
free_fblock (gpointer boxed)
{
  SfiFBlock *fblock = (SfiFBlock*) boxed;
  if (fblock)
    sfi_fblock_unref (fblock);
}

void
_sfi_init_values (void)
{
  GTypeInfo info = {
    0,		/* class_size */
    NULL,	/* base_init */
    NULL,	/* base_destroy */
    NULL,	/* class_init */
    NULL,	/* class_destroy */
    NULL,	/* class_data */
    0,		/* instance_size */
    0,		/* n_preallocs */
    NULL,	/* instance_init */
  };
  static GType value_types[6] = { 0, };

  assert_return (sfi__value_types == NULL);

  sfi__value_types = value_types;

  /* value types */
  SFI_TYPE_CHOICE = g_type_register_static (G_TYPE_STRING, "SfiChoice", &info, GTypeFlags (0));
  SFI_TYPE_BBLOCK = g_boxed_type_register_static ("SfiBBlock", copy_bblock, free_bblock);
  SFI_TYPE_FBLOCK = g_boxed_type_register_static ("SfiFBlock", copy_fblock, free_fblock);
  SFI_TYPE_SEQ = g_boxed_type_register_static ("SfiSeq", copy_seq, free_seq);
  SFI_TYPE_REC = g_boxed_type_register_static ("SfiRec", copy_rec, free_rec);
}


/* --- GValue functions --- */
gboolean
sfi_check_value (const GValue *value)
{
  GType vtype, ftype;
  if (!value)
    return FALSE;
  vtype = G_VALUE_TYPE (value);
  if (G_TYPE_IS_FUNDAMENTAL (vtype))
    ftype = vtype;
  else
    ftype = G_TYPE_FUNDAMENTAL (vtype);
  /* checking for fundamental types is good enough to figure most Sfi types */
  switch (ftype)
    {
      /* fundamentals */
    case SFI_TYPE_BOOL:
    case SFI_TYPE_INT:
    case SFI_TYPE_NUM:
    case SFI_TYPE_REAL:
    case SFI_TYPE_SFI_STRING:
    case SFI_TYPE_PSPEC:
      return TRUE;
    }
  /* non fundamentals */
  /* SFI_TYPE_CHOICE is derived from SFI_TYPE_SFI_STRING */
  if (ftype == G_TYPE_BOXED)
    return (vtype == SFI_TYPE_REC ||
	    vtype == SFI_TYPE_SEQ ||
	    vtype == SFI_TYPE_FBLOCK ||
	    vtype == SFI_TYPE_BBLOCK);
  else
    return false;
}

const char*
sfi_value_get_choice (const GValue *value)
{
  assert_return (SFI_VALUE_HOLDS_CHOICE (value), NULL);

  return g_value_get_string (value);
}

void
sfi_value_set_choice (GValue      *value,
		      const gchar *choice_value)
{
  assert_return (SFI_VALUE_HOLDS_CHOICE (value));

  g_value_set_string (value, choice_value);
}

SfiBBlock*
sfi_value_get_bblock (const GValue *value)
{
  assert_return (SFI_VALUE_HOLDS_BBLOCK (value), NULL);

  return (SfiBBlock*) g_value_get_boxed (value);
}

SfiBBlock*
sfi_value_dup_bblock (const GValue *value)
{
  assert_return (SFI_VALUE_HOLDS_BBLOCK (value), NULL);

  SfiBBlock *bblock = (SfiBBlock*) g_value_get_boxed (value);
  return bblock ? sfi_bblock_ref (bblock) : NULL;
}

void
sfi_value_set_bblock (GValue    *value,
		      SfiBBlock *bblock)
{
  assert_return (SFI_VALUE_HOLDS_BBLOCK (value));

  g_value_set_boxed (value, bblock);
}

void
sfi_value_take_bblock (GValue    *value,
		       SfiBBlock *bblock)
{
  assert_return (SFI_VALUE_HOLDS_BBLOCK (value));

  g_value_take_boxed (value, bblock);
}

SfiFBlock*
sfi_value_get_fblock (const GValue *value)
{
  assert_return (SFI_VALUE_HOLDS_FBLOCK (value), NULL);

  return (SfiFBlock*) g_value_get_boxed (value);
}

SfiFBlock*
sfi_value_dup_fblock (const GValue *value)
{
  assert_return (SFI_VALUE_HOLDS_FBLOCK (value), NULL);

  SfiFBlock *fblock = (SfiFBlock*) g_value_get_boxed (value);
  return fblock ? sfi_fblock_ref (fblock) : NULL;
}

void
sfi_value_set_fblock (GValue    *value,
		      SfiFBlock *fblock)
{
  assert_return (SFI_VALUE_HOLDS_FBLOCK (value));

  g_value_set_boxed (value, fblock);
}

void
sfi_value_take_fblock (GValue    *value,
		       SfiFBlock *fblock)
{
  assert_return (SFI_VALUE_HOLDS_FBLOCK (value));

  g_value_take_boxed (value, fblock);
}

GParamSpec*
sfi_value_get_pspec (const GValue *value)
{
  assert_return (SFI_VALUE_HOLDS_PSPEC (value), NULL);

  return g_value_get_param (value);
}

GParamSpec*
sfi_value_dup_pspec (const GValue *value)
{
  GParamSpec *pspec;

  assert_return (SFI_VALUE_HOLDS_PSPEC (value), NULL);

  pspec = g_value_get_param (value);
  return pspec ? sfi_pspec_ref (pspec) : NULL;
}

void
sfi_value_set_pspec (GValue     *value,
		     GParamSpec *pspec)
{
  assert_return (SFI_VALUE_HOLDS_PSPEC (value));

  g_value_set_param (value, pspec);
}

void
sfi_value_take_pspec (GValue     *value,
		      GParamSpec *pspec)
{
  assert_return (SFI_VALUE_HOLDS_PSPEC (value));

  g_value_take_param (value, pspec);
}

SfiSeq*
sfi_value_get_seq (const GValue *value)
{
  assert_return (SFI_VALUE_HOLDS_SEQ (value), NULL);

  return (SfiSeq*) g_value_get_boxed (value);
}

void
sfi_value_set_seq (GValue *value,
		   SfiSeq *seq)
{
  assert_return (SFI_VALUE_HOLDS_SEQ (value));

  g_value_set_boxed (value, seq);
}

void
sfi_value_take_seq (GValue *value,
		    SfiSeq *seq)
{
  assert_return (SFI_VALUE_HOLDS_SEQ (value));

  g_value_take_boxed (value, seq);
}

SfiRec*
sfi_value_get_rec (const GValue *value)
{
  assert_return (SFI_VALUE_HOLDS_REC (value), NULL);

  return (SfiRec*) g_value_get_boxed (value);
}

SfiRec*
sfi_value_dup_rec (const GValue *value)
{
  assert_return (SFI_VALUE_HOLDS_REC (value), NULL);
  SfiRec *rec = (SfiRec*) g_value_get_boxed (value);
  return rec ? sfi_rec_ref (rec) : NULL;
}

void
sfi_value_set_rec (GValue *value,
		   SfiRec *rec)
{
  assert_return (SFI_VALUE_HOLDS_REC (value));

  g_value_set_boxed (value, rec);
}

void
sfi_value_take_rec (GValue *value,
		    SfiRec *rec)
{
  assert_return (SFI_VALUE_HOLDS_REC (value));

  g_value_take_boxed (value, rec);
}

void
sfi_value_copy_deep (const GValue *src_value,
		     GValue       *dest_value)
{
  assert_return (G_IS_VALUE (src_value));
  assert_return (G_IS_VALUE (dest_value));

  SfiSCategory scat = SfiSCategory (sfi_categorize_type (G_VALUE_TYPE (src_value)) & SFI_SCAT_TYPE_MASK);
  switch (scat)
    {
      case SFI_SCAT_SEQ:
	{
	  SfiSeq *seq;
	  assert_return (SFI_VALUE_HOLDS_SEQ (dest_value));
	  seq = sfi_value_get_seq (src_value);
	  sfi_value_take_seq (dest_value, seq ? sfi_seq_copy_deep (seq) : NULL);
	}
	break;
      case SFI_SCAT_REC:
	{
	  SfiRec *rec;
	  assert_return (SFI_VALUE_HOLDS_REC (dest_value));
	  rec = sfi_value_get_rec (src_value);
	  sfi_value_take_rec (dest_value, rec ? sfi_rec_copy_deep (rec) : NULL);
	}
	break;
      case SFI_SCAT_BBLOCK:
	{
	  SfiBBlock *bblock;
	  assert_return (SFI_VALUE_HOLDS_BBLOCK (dest_value));
	  bblock = sfi_value_get_bblock (src_value);
	  sfi_value_take_bblock (dest_value, bblock ? sfi_bblock_copy_deep (bblock) : NULL);
	}
	break;
      case SFI_SCAT_FBLOCK:
	{
	  SfiFBlock *fblock;
	  assert_return (SFI_VALUE_HOLDS_FBLOCK (dest_value));
	  fblock = sfi_value_get_fblock (src_value);
	  sfi_value_take_fblock (dest_value, fblock ? sfi_fblock_copy_deep (fblock) : NULL);
	}
	break;
      default:
	g_value_copy (src_value, dest_value);
    }
}


/* --- Sfi value constructors --- */
static GValue*
alloc_value (GType type)
{
  GValue *value = sfi_new_struct0 (GValue, 1);
  if (type)
    g_value_init (value, type);
  return value;
}

void
sfi_value_free (GValue *value)
{
  assert_return (value != NULL);
  if (G_VALUE_TYPE (value))
    g_value_unset (value);
  sfi_delete_struct (GValue, value);
}

GValue*
sfi_value_empty (void)
{
  GValue *value = alloc_value (0);
  return value;
}

GValue*
sfi_value_clone_deep (const GValue *value)
{
  GValue *dest;

  assert_return (value != NULL, NULL);

  dest = sfi_value_empty ();
  if (G_IS_VALUE (value))
    {
      g_value_init (dest, G_VALUE_TYPE (value));
      sfi_value_copy_deep (value, dest);
    }
  return dest;
}

GValue*
sfi_value_clone_shallow (const GValue *value)
{
  GValue *dest;

  assert_return (value != NULL, NULL);

  dest = sfi_value_empty ();
  if (G_IS_VALUE (value))
    {
      g_value_init (dest, G_VALUE_TYPE (value));
      sfi_value_copy_shallow (value, dest);
    }
  return dest;
}

GValue*
sfi_value_bool (SfiBool vbool)
{
  GValue *value = alloc_value (SFI_TYPE_BOOL);
  sfi_value_set_bool (value, vbool);
  return value;
}

GValue*
sfi_value_int (SfiInt vint)
{
  GValue *value = alloc_value (SFI_TYPE_INT);
  sfi_value_set_int (value, vint);
  return value;
}

GValue*
sfi_value_num (SfiNum vnum)
{
  GValue *value = alloc_value (SFI_TYPE_NUM);
  sfi_value_set_num (value, vnum);
  return value;
}

GValue*
sfi_value_real (SfiReal vreal)
{
  GValue *value = alloc_value (SFI_TYPE_REAL);
  sfi_value_set_real (value, vreal);
  return value;
}

GValue*
sfi_value_string (const gchar *vstring)
{
  GValue *value = alloc_value (SFI_TYPE_SFI_STRING);
  sfi_value_set_string (value, vstring);
  return value;
}

GValue*
sfi_value_lstring (const gchar *vstring,
		   guint        length)
{
  GValue *value = alloc_value (SFI_TYPE_SFI_STRING);
  sfi_value_take_string (value, g_strndup (vstring, vstring ? length : 0));
  return value;
}

GValue*
sfi_value_choice (const gchar *vchoice)
{
  GValue *value = alloc_value (SFI_TYPE_CHOICE);
  sfi_value_set_choice (value, vchoice);
  return value;
}

GValue*
sfi_value_lchoice (const gchar *vchoice,
		   guint        length)
{
  GValue *value = alloc_value (SFI_TYPE_CHOICE);
  sfi_value_take_string (value, g_strndup (vchoice, vchoice ? length : 0));
  return value;
}

GValue*
sfi_value_choice_enum (const GValue *enum_value)
{
  assert_return (G_VALUE_HOLDS_ENUM (enum_value), NULL);

  GEnumClass *eclass = (GEnumClass*) g_type_class_ref (G_VALUE_TYPE (enum_value));
  GEnumValue *ev = g_enum_get_value (eclass, g_value_get_enum (enum_value));
  GValue *value = sfi_value_choice (ev ? ev->value_name : NULL);
  g_type_class_unref (eclass);
  return value;
}

GValue*
sfi_value_choice_genum (gint enum_value, GType enum_type)
{
  const gchar *choice;

  choice = sfi_enum2choice (enum_value, enum_type);
  return sfi_value_choice (choice);
}


GValue*
sfi_value_bblock (SfiBBlock *vbblock)
{
  GValue *value = alloc_value (SFI_TYPE_BBLOCK);
  sfi_value_set_bblock (value, vbblock);
  return value;
}

GValue*
sfi_value_fblock (SfiFBlock *vfblock)
{
  GValue *value = alloc_value (SFI_TYPE_FBLOCK);
  sfi_value_set_fblock (value, vfblock);
  return value;
}

GValue*
sfi_value_pspec (GParamSpec *pspec)
{
  GValue *value = alloc_value (SFI_TYPE_PSPEC);
  sfi_value_set_pspec (value, pspec);
  return value;
}

GValue*
sfi_value_seq (SfiSeq *vseq)
{
  GValue *value = alloc_value (SFI_TYPE_SEQ);
  sfi_value_set_seq (value, vseq);
  return value;
}

GValue*
sfi_value_new_take_seq (SfiSeq *vseq)
{
  GValue *value = alloc_value (SFI_TYPE_SEQ);
  sfi_value_set_seq (value, vseq);
  if (vseq)
    sfi_seq_unref (vseq);
  return value;
}

GValue*
sfi_value_rec (SfiRec *vrec)
{
  GValue *value = alloc_value (SFI_TYPE_REC);
  sfi_value_set_rec (value, vrec);
  return value;
}

GValue*
sfi_value_new_take_rec (SfiRec *vrec)
{
  GValue *value = alloc_value (SFI_TYPE_REC);
  sfi_value_set_rec (value, vrec);
  if (vrec)
    sfi_rec_unref (vrec);
  return value;
}

/* --- transformation --- */
void
sfi_value_choice2enum_simple (const GValue *choice_value,
			      GValue       *enum_value)
{
  return sfi_value_choice2enum (choice_value, enum_value, NULL);
}

void
sfi_value_choice2enum (const GValue *choice_value,
		       GValue       *enum_value,
		       GParamSpec   *fallback_param)
{
  GEnumValue *ev = NULL;
  const char *eval;
  uint i;

  assert_return (SFI_VALUE_HOLDS_CHOICE (choice_value));
  assert_return (G_VALUE_HOLDS_ENUM (enum_value));
  if (fallback_param)
    {
      assert_return (G_IS_PARAM_SPEC_ENUM (fallback_param));
      assert_return (G_VALUE_HOLDS (enum_value, G_PARAM_SPEC_VALUE_TYPE (fallback_param)));
    }

  GEnumClass *eclass = (GEnumClass*) g_type_class_ref (G_VALUE_TYPE (enum_value));
  eval = sfi_value_get_choice (choice_value);
  if (eval)
    for (i = 0; i < eclass->n_values; i++)
      if (sfi_choice_match_detailed (eclass->values[i].value_name, eval, TRUE))
        /* || sfi_choice_match_detailed (eclass->values[i].value_nick, eval, TRUE) */
	{
	  ev = eclass->values + i;
	  break;
	}
  if (ev || fallback_param)
    {
      if (!ev)
	ev = g_enum_get_value (eclass, G_PARAM_SPEC_ENUM (fallback_param)->default_value);
      if (!ev)	/* pspec is broken */
	ev = eclass->values;
      g_value_set_enum (enum_value, ev->value);
    }
  else
    g_value_set_enum (enum_value, 0);
  g_type_class_unref (eclass);
}

static inline gchar*
to_sname (gchar *str)
{
  gchar *s;
  for (s = str; *s; s++)
    if (*s >= 'A' && *s <= 'Z')
      *s += 'a' - 'A';
    else if (*s >= 'a' && *s <= 'z')
      ;
    else if (*s >= '0' && *s <= '9')
      ;
    else
      *s = '-';
  return str;
}

void
sfi_value_enum2choice (const GValue *enum_value,
		       GValue       *choice_value)
{
  assert_return (SFI_VALUE_HOLDS_CHOICE (choice_value));
  assert_return (G_VALUE_HOLDS_ENUM (enum_value));

  GEnumClass *eclass = (GEnumClass*) g_type_class_ref (G_VALUE_TYPE (enum_value));
  GEnumValue *ev = g_enum_get_value (eclass, g_value_get_enum (enum_value));
  if (!ev)
    ev = eclass->values;
  char *sname = to_sname (g_strdup (ev->value_name));
  sfi_value_set_choice (choice_value, sname);
  g_free (sname);
  g_type_class_unref (eclass);
}

gint
sfi_choice2enum_checked (const gchar    *choice_value,
                         GType           enum_type,
                         GError        **errorp)
{
  GEnumValue *ev = NULL;
  guint i;
  gint enum_value;

  GEnumClass *eclass = (GEnumClass*) g_type_class_ref (enum_type);
  if (choice_value)
    for (i = 0; i < eclass->n_values; i++)
      if (sfi_choice_match_detailed (eclass->values[i].value_name, choice_value, TRUE))
        /* || sfi_choice_match_detailed (eclass->values[i].value_nick, choice_value, TRUE) */
	{
	  ev = eclass->values + i;
	  break;
	}
  if (!ev)
    g_set_error (errorp, SFI_CHOICE_ERROR_QUARK, 1,
                 "%s contains no value: %s",
                 g_type_name (enum_type), choice_value ? choice_value : "<NULL>");
  enum_value = ev ? ev->value : 0;
  g_type_class_unref (eclass);

  return enum_value;
}

gint
sfi_choice2enum (const gchar    *choice_value,
                 GType           enum_type)
{
  return sfi_choice2enum_checked (choice_value, enum_type, NULL);
}

const gchar*
sfi_enum2choice (gint            enum_value,
                 GType           enum_type)
{
  GEnumValue *ev;
  const gchar *choice;
  gchar *cident;

  GEnumClass *eclass = (GEnumClass*) g_type_class_ref (enum_type);
  ev = g_enum_get_value (eclass, enum_value);
  if (!ev)
    ev = eclass->values;
  cident = sfi_strdup_canon (ev->value_name);
  choice = g_intern_string (cident);
  g_free (cident);
  g_type_class_unref (eclass);

  return choice;
}

gint
sfi_value_get_enum_auto (GType         enum_type,
                         const GValue *value)
{
  if (SFI_VALUE_HOLDS_CHOICE (value))
    return sfi_choice2enum (sfi_value_get_choice (value), enum_type);
  else
    return g_value_get_enum (value);
}

void
sfi_value_set_enum_auto (GType       enum_type,
                         GValue     *value,
                         gint        enum_value)
{
  if (SFI_VALUE_HOLDS_CHOICE (value))
    sfi_value_set_choice (value, sfi_enum2choice (enum_value, enum_type));
  else
    g_value_set_enum (value, enum_value);
}

/* transform function to work around glib bugs */
gboolean
sfi_value_type_compatible (GType           src_type,
			   GType           dest_type)
{
  return g_value_type_compatible (src_type, dest_type);
}

gboolean
sfi_value_type_transformable (GType           src_type,
			      GType           dest_type)
{
  if (g_value_type_transformable (src_type, dest_type))
    return TRUE;
  if (src_type == SFI_TYPE_CHOICE && G_TYPE_IS_ENUM (dest_type) && dest_type != G_TYPE_ENUM)
    return TRUE;
  if (dest_type == SFI_TYPE_CHOICE && G_TYPE_IS_ENUM (src_type) && src_type != G_TYPE_ENUM)
    return TRUE;
  return FALSE;
}

gboolean
sfi_value_transform (const GValue   *src_value,
		     GValue         *dest_value)
{
  GType src_type, dest_type;
  if (g_value_transform (src_value, dest_value))
    return TRUE;
  src_type = G_VALUE_TYPE (src_value);
  dest_type = G_VALUE_TYPE (dest_value);
  if (src_type == SFI_TYPE_CHOICE && G_TYPE_IS_ENUM (dest_type) && dest_type != G_TYPE_ENUM)
    {
      sfi_value_choice2enum_simple (src_value, dest_value);
      return TRUE;
    }
  if (dest_type == SFI_TYPE_CHOICE && G_TYPE_IS_ENUM (src_type) && src_type != G_TYPE_ENUM)
    {
      sfi_value_enum2choice (src_value, dest_value);
      return TRUE;
    }
  return FALSE;
}

// == Testing ==
#include "testing.hh"
namespace { // Anon
using namespace Bse;

typedef enum /*< skip >*/
{
  SERIAL_TEST_TYPED = 1,
  SERIAL_TEST_PARAM,
  SERIAL_TEST_PSPEC
} SerialTest;

static SerialTest serial_test_type = SerialTest (0);

static void
serial_pspec_check (GParamSpec *pspec,
		    GScanner   *scanner)
{
  GValue *value = sfi_value_pspec (pspec), rvalue = { 0, };
  GString *s1 = g_string_new (NULL);
  GString *s2 = g_string_new (NULL);
  GTokenType token;
  sfi_value_store_typed (value, s1);
  g_scanner_input_text (scanner, s1->str, s1->len);
  token = sfi_value_parse_typed (&rvalue, scanner);
  if (token != G_TOKEN_NONE)
    {
      printout ("{while parsing pspec \"%s\":\n\t%s\n", pspec->name, s1->str);
      g_scanner_unexp_token (scanner, token, NULL, NULL, NULL,
			     g_strdup_format ("failed to serialize pspec \"%s\"", pspec->name), TRUE);
    }
  TASSERT (token == G_TOKEN_NONE);
  sfi_value_store_typed (&rvalue, s2);
  if (strcmp (s1->str, s2->str))
    printout ("{while comparing pspecs \"%s\":\n\t%s\n\t%s\n", pspec->name, s1->str, s2->str);
  TASSERT (strcmp (s1->str, s2->str) == 0);
  g_value_unset (&rvalue);
  sfi_value_free (value);
  g_string_free (s1, TRUE);
  g_string_free (s2, TRUE);
}

// serialize @a value according to @a pspec, deserialize and assert a matching result
static void
serialize_cmp (GValue     *value,
	       GParamSpec *pspec)
{
  GScanner *scanner = g_scanner_new64 (sfi_storage_scanner_config);
  GString *gstring = g_string_new (NULL);
  GValue rvalue = { 0, };
  GTokenType token;
  gint cmp;
  if (serial_test_type == SERIAL_TEST_PSPEC)
    serial_pspec_check (pspec, scanner);
  else
    {
      // if (pspec && strcmp (pspec->name, "real-max") == 0) G_BREAKPOINT ();
      if (serial_test_type == SERIAL_TEST_TYPED)
	sfi_value_store_typed (value, gstring);
      else /* SERIAL_TEST_PARAM */
	sfi_value_store_param (value, gstring, pspec, 2);
      g_scanner_input_text (scanner, gstring->str, gstring->len);
      if (serial_test_type == SERIAL_TEST_TYPED)
	token = sfi_value_parse_typed (&rvalue, scanner);
      else /* SERIAL_TEST_PARAM */
	{
	  if (g_scanner_get_next_token (scanner) == '(')
	    if (g_scanner_get_next_token (scanner) == G_TOKEN_IDENTIFIER &&
		strcmp (scanner->value.v_identifier, pspec->name) == 0)
	      token = sfi_value_parse_param_rest (&rvalue, scanner, pspec);
	    else
	      token = G_TOKEN_IDENTIFIER;
	  else
	    token = GTokenType ('(');
	}
      if (0)
	printout ("{parsing:%s}", gstring->str);
      if (token != G_TOKEN_NONE)
	{
	  printout ("{while parsing \"%s\":\n\t%s\n", pspec->name, gstring->str);
	  g_scanner_unexp_token (scanner, token, NULL, NULL, NULL,
				 g_strdup_format ("failed to serialize \"%s\"", pspec->name), TRUE);
	}
      TASSERT (token == G_TOKEN_NONE);
      cmp = g_param_values_cmp (pspec, value, &rvalue);
      if (cmp)
	{
	  printout ("{after parsing:\n\t%s\n", gstring->str);
	  printout ("while comparing:\n\t%s\nwith:\n\t%s\nresult:\n\t%d\n",
		   g_strdup_value_contents (value),
		   g_strdup_value_contents (&rvalue),
		   cmp);
	  if (0)
	    {
	      G_BREAKPOINT ();
	      g_value_unset (&rvalue);
	      g_scanner_input_text (scanner, gstring->str, gstring->len);
	      token = sfi_value_parse_typed (&rvalue, scanner);
	    }
	}
      TASSERT (cmp == 0);
      if (0) /* generate testoutput */
	printout ("OK=================(%s)=================:\n%s\n", pspec->name, gstring->str);
    }
  g_scanner_destroy (scanner);
  g_string_free (gstring, TRUE);
  if (G_VALUE_TYPE (&rvalue))
    g_value_unset (&rvalue);
  sfi_value_free (value);
  sfi_pspec_sink (pspec);
}

static void
test_typed_serialization (SerialTest test_type)
{
  static const SfiChoiceValue test_choices[] = {
    { "ozo-foo", "Ozo-foo blurb", },
    { "emptyblurb", "", },
    { "noblurb", NULL, },
  };
  static const SfiChoiceValues choice_values = { G_N_ELEMENTS (test_choices), test_choices };
  SfiRecFields rec_fields = { 0, NULL, };
  GParamSpec *pspec_homo_seq;
  SfiFBlock *fblock;
  SfiBBlock *bblock;
  SfiSeq *seq;
  SfiRec *rec;
  GValue *val;
  gchar str256[257];
  guint i;
  serial_test_type = test_type;
  serialize_cmp (sfi_value_bool (FALSE),
		 sfi_pspec_bool ("bool-false", NULL, NULL, FALSE, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_bool (TRUE),
		 sfi_pspec_bool ("bool-true", NULL, NULL, FALSE, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_int (SFI_MAXINT),
		 sfi_pspec_int ("int-max", NULL, NULL, 0, SFI_MININT, SFI_MAXINT, 1, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_int (SFI_MININT),
		 sfi_pspec_int ("int-min", NULL, NULL, 0, SFI_MININT, SFI_MAXINT, 1, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_num (SFI_MAXNUM),
		 sfi_pspec_num ("num-max", NULL, NULL, 0, SFI_MINNUM, SFI_MAXNUM, 1, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_num (SFI_MINNUM),
		 sfi_pspec_num ("num-min", NULL, NULL, 0, SFI_MINNUM, SFI_MAXNUM, 1, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_real (SFI_MINREAL),
		 sfi_pspec_real ("real-min", NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_real (SFI_MAXREAL),
		 sfi_pspec_real ("real-max", NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_real (-SFI_MINREAL),
		 sfi_pspec_real ("real-min-neg", NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_real (-SFI_MAXREAL),
		 sfi_pspec_real ("real-max-neg", NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_real (SFI_MININT),
		 sfi_pspec_real ("real-minint", NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_real (SFI_MINNUM),
		 sfi_pspec_real ("real-minnum", NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_note (SFI_MIN_NOTE),
		 sfi_pspec_note ("vnote-min", NULL, NULL, SFI_KAMMER_NOTE, SFI_MIN_NOTE, SFI_MAX_NOTE, TRUE, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_note (SFI_MAX_NOTE),
		 sfi_pspec_note ("vnote-max", NULL, NULL, SFI_KAMMER_NOTE, SFI_MIN_NOTE, SFI_MAX_NOTE, TRUE, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_note (SFI_NOTE_VOID),
		 sfi_pspec_note ("vnote-void", NULL, NULL, SFI_KAMMER_NOTE, SFI_MIN_NOTE, SFI_MAX_NOTE, TRUE, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_note (SFI_MIN_NOTE),
		 sfi_pspec_note ("note-min", NULL, NULL, SFI_KAMMER_NOTE, SFI_MIN_NOTE, SFI_MAX_NOTE, FALSE, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_note (SFI_MAX_NOTE),
		 sfi_pspec_note ("note-max", NULL, NULL, SFI_KAMMER_NOTE, SFI_MIN_NOTE, SFI_MAX_NOTE, FALSE, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_string (NULL),
		 sfi_pspec_string ("string-nil", NULL, NULL, NULL, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_string ("test\"string'with\\character-?\007rubbish\177H"),
		 sfi_pspec_string ("string", NULL, NULL, NULL, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_string (""),
		 sfi_pspec_string ("string-empty", NULL, NULL, NULL, SFI_PARAM_STANDARD));
  for (i = 0; i < 255; i++)
    str256[i] = i + 1;
  str256[i] = 0;
  serialize_cmp (sfi_value_string (str256),
		 sfi_pspec_string ("string-255", NULL, NULL, NULL, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_choice (NULL),
		 sfi_pspec_choice ("choice-nil", NULL, NULL, NULL, choice_values, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_choice ("test-choice-with-valid-characters_9876543210"),
		 sfi_pspec_choice ("choice", NULL, NULL, NULL, choice_values, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_bblock (NULL),
		 sfi_pspec_bblock ("bblock-nil", NULL, NULL, SFI_PARAM_STANDARD));
  bblock = sfi_bblock_new ();
  serialize_cmp (sfi_value_bblock (bblock),
		 sfi_pspec_bblock ("bblock-empty", NULL, NULL, SFI_PARAM_STANDARD));
  for (i = 0; i < 256; i++)
    sfi_bblock_append1 (bblock, i);
  serialize_cmp (sfi_value_bblock (bblock),
		 sfi_pspec_bblock ("bblock", NULL, NULL, SFI_PARAM_STANDARD));
  sfi_bblock_unref (bblock);
  serialize_cmp (sfi_value_fblock (NULL),
		 sfi_pspec_fblock ("fblock-nil", NULL, NULL, SFI_PARAM_STANDARD));
  fblock = sfi_fblock_new ();
  serialize_cmp (sfi_value_fblock (fblock),
		 sfi_pspec_fblock ("fblock-empty", NULL, NULL, SFI_PARAM_STANDARD));
  sfi_fblock_append1 (fblock, -G_MINFLOAT);
  sfi_fblock_append1 (fblock, G_MINFLOAT);
  sfi_fblock_append1 (fblock, -G_MAXFLOAT);
  sfi_fblock_append1 (fblock, G_MAXFLOAT);
  sfi_fblock_append1 (fblock, SFI_MININT);
  sfi_fblock_append1 (fblock, -SFI_MAXINT);
  sfi_fblock_append1 (fblock, SFI_MAXINT);
  sfi_fblock_append1 (fblock, SFI_MINNUM);
  sfi_fblock_append1 (fblock, -SFI_MAXNUM);
  sfi_fblock_append1 (fblock, SFI_MAXNUM);
  serialize_cmp (sfi_value_fblock (fblock),
		 sfi_pspec_fblock ("fblock", NULL, NULL, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_seq (NULL),
		 sfi_pspec_seq ("seq-nil", NULL, NULL, NULL, SFI_PARAM_STANDARD));
  seq = sfi_seq_new ();
  serialize_cmp (sfi_value_seq (seq),
		 sfi_pspec_seq ("seq-empty", NULL, NULL, NULL, SFI_PARAM_STANDARD));
  val = sfi_value_bool (TRUE);
  sfi_seq_append (seq, val);
  sfi_value_free (val);
  val = sfi_value_int (42);
  sfi_seq_append (seq, val);
  sfi_value_free (val);
  val = sfi_value_string ("huhu");
  sfi_seq_append (seq, val);
  sfi_value_free (val);
  val = sfi_value_fblock (fblock);
  sfi_seq_append (seq, val);
  sfi_value_free (val);
  if (serial_test_type != SERIAL_TEST_PARAM)
    serialize_cmp (sfi_value_seq (seq),
		   sfi_pspec_seq ("seq-heterogeneous", NULL, NULL,
				  sfi_pspec_real ("dummy", NULL, NULL,
						  0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_STANDARD),
				  SFI_PARAM_STANDARD));
  sfi_seq_clear (seq);
  for (i = 0; i < 12; i++)
    {
      val = sfi_value_int (2000 - 3 * i);
      sfi_seq_append (seq, val);
      sfi_value_free (val);
    }
  pspec_homo_seq = sfi_pspec_seq ("seq-homogeneous", NULL, NULL,
				  sfi_pspec_int ("integer", NULL, NULL,
						 1500, 1000, 2000, 1, SFI_PARAM_STANDARD),
				  SFI_PARAM_STANDARD);
  sfi_pspec_ref (pspec_homo_seq);
  serialize_cmp (sfi_value_seq (seq),
		 sfi_pspec_seq ("seq-homogeneous", NULL, NULL,
				sfi_pspec_int ("integer", NULL, NULL,
					       1500, 1000, 2000, 1, SFI_PARAM_STANDARD),
				SFI_PARAM_STANDARD));
  if (serial_test_type == SERIAL_TEST_PSPEC)
    {
      serialize_cmp (sfi_value_pspec (NULL),
		     sfi_pspec_pspec ("pspec-nil", NULL, NULL, SFI_PARAM_STANDARD));
      serialize_cmp (sfi_value_pspec (pspec_homo_seq),
		     sfi_pspec_pspec ("pspec-hseq", NULL, NULL, SFI_PARAM_STANDARD));
    }
  serialize_cmp (sfi_value_rec (NULL),
		 sfi_pspec_rec ("rec-nil", NULL, NULL, rec_fields, SFI_PARAM_STANDARD));
  rec = sfi_rec_new ();
  serialize_cmp (sfi_value_rec (rec),
		 sfi_pspec_rec ("rec-empty", NULL, NULL, rec_fields, SFI_PARAM_STANDARD));
  val = sfi_value_string ("huhu");
  sfi_rec_set (rec, "exo-string", val);
  if (serial_test_type != SERIAL_TEST_PARAM)
    sfi_rec_set (rec, "exo-string2", val);
  sfi_value_free (val);
  val = sfi_value_seq (seq);
  sfi_rec_set (rec, "seq-homogeneous", val);
  sfi_value_free (val);
  rec_fields.fields = g_new (GParamSpec*, 20); /* should be static mem */
  rec_fields.fields[rec_fields.n_fields++] = sfi_pspec_string ("exo-string", NULL, NULL, NULL, SFI_PARAM_STANDARD);
  rec_fields.fields[rec_fields.n_fields++] = pspec_homo_seq;
  serialize_cmp (sfi_value_rec (rec),
		 sfi_pspec_rec ("rec", NULL, NULL, rec_fields, SFI_PARAM_STANDARD));
  sfi_fblock_unref (fblock);
  sfi_seq_unref (seq);
  sfi_pspec_unref (pspec_homo_seq);
  sfi_rec_unref (rec);
}

BSE_INTEGRITY_TEST (test_param_serialization);
static void
test_param_serialization()
{
  test_typed_serialization (SERIAL_TEST_PARAM);
}

BSE_INTEGRITY_TEST (test_typed_serialization);
static void
test_typed_serialization ()
{
  test_typed_serialization (SERIAL_TEST_TYPED);
}

BSE_INTEGRITY_TEST (test_pspec_serialization);
static void
test_pspec_serialization ()
{
  test_typed_serialization (SERIAL_TEST_PSPEC);
}

} // Anon
