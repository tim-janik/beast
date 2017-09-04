// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "sfivalues.hh"
#include "sfiprimitives.hh"
#include "sfiparams.hh"
#include "sfimemory.hh"


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
  SFI_TYPE_PROXY = g_pointer_type_register_static ("SfiProxy");
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
    return (vtype == SFI_TYPE_PROXY);
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

SfiProxy
sfi_value_get_proxy (const GValue *value)
{
  assert_return (SFI_VALUE_HOLDS_PROXY (value), 0);

  return (SfiProxy) g_value_get_pointer (value);
}

void
sfi_value_set_proxy (GValue  *value,
		     SfiProxy proxy)
{
  assert_return (SFI_VALUE_HOLDS_PROXY (value));

  g_value_set_pointer (value, (gpointer) proxy);
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

GValue*
sfi_value_proxy (SfiProxy vproxy)
{
  GValue *value = alloc_value (SFI_TYPE_PROXY);
  sfi_value_set_proxy (value, vproxy);
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
