// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include <string.h>
#include <stdio.h>      /* sscanf() */
#include "sfiserial.hh"
#include "sfiparams.hh"
#include "sfitime.hh"
#include "sfinote.hh"
#include <stdlib.h>     // FIXME: remove "free"

typedef std::string String; // FIXME

/* --- parsing aids --- */
#define MC(s)   const_cast<char*> (s)
static const GScannerConfig storage_scanner_config = {
  MC (
      " \t\r\n\v"
      )			/* cset_skip_characters */,
  MC (
      G_CSET_a_2_z
      "_"
      G_CSET_A_2_Z
      )			/* cset_identifier_first */,
  MC (
      G_CSET_a_2_z
      ".:-+_0123456789*!?"
      G_CSET_A_2_Z
      )			/* cset_identifier_nth */,
  MC ( ";\n" )		/* cpair_comment_single */,
  TRUE			/* case_sensitive */,
  TRUE			/* skip_comment_multi */,
  TRUE			/* skip_comment_single */,
  FALSE			/* scan_comment_multi */,
  TRUE			/* scan_identifier */,
  FALSE			/* scan_identifier_1char */,
  FALSE			/* scan_identifier_NULL */,
  TRUE			/* scan_symbols */,
  TRUE			/* scan_binary */,
  TRUE			/* scan_octal */,
  TRUE			/* scan_float */,
  TRUE			/* scan_hex */,
  FALSE			/* scan_hex_dollar */,
  FALSE			/* scan_string_sq */,
  TRUE			/* scan_string_dq */,
  TRUE			/* numbers_2_int */,
  FALSE			/* int_2_float */,
  FALSE			/* identifier_2_string */,
  TRUE			/* char_2_token */,
  TRUE			/* symbol_2_token */,
  FALSE			/* scope_0_fallback */,
  TRUE			/* store_int64 */,
};
const GScannerConfig *sfi_storage_scanner_config = &storage_scanner_config;

#define parse_or_return(scanner, token)  G_STMT_START{ \
  GScanner *__s = (scanner); int _t = (token); \
  if (g_scanner_get_next_token (__s) != _t) \
    return GTokenType (_t);                 \
}G_STMT_END
#define peek_or_return(scanner, token)   G_STMT_START{ \
  GScanner *__s = (scanner); guint _t = (token); \
  if (g_scanner_peek_next_token (__s) != _t) { \
    g_scanner_get_next_token (__s); /* advance position for error-handler */ \
    return _t; \
  } \
}G_STMT_END
static GTokenType
scanner_skip_statement (GScanner *scanner,
			guint     level)
{
  while (level)
    {
      g_scanner_get_next_token (scanner);
      if (scanner->token == G_TOKEN_EOF ||
	  scanner->token == G_TOKEN_ERROR)
	return GTokenType (')');
      if (scanner->token == '(')
	level++;
      else if (scanner->token == ')')
	level--;
    }
  return G_TOKEN_NONE;
}

static String
string_vprintf (const char *format, va_list vargs) // FIXME: move
{
  char *str = NULL;
  if (vasprintf (&str, format, vargs) >= 0 && str)
    {
      String s = str;
      free (str);
      return s;
    }
  else
    return format;
}

static String
string_printf (const char *format, ...) // FIXME: move
{
  String str;
  va_list args;
  va_start (args, format);
  str = string_vprintf (format, args);
  va_end (args);
  return str;
}

static String
string_to_cescape (const String &str)   // FIXME: move
{
  String buffer;
  for (String::const_iterator it = str.begin(); it != str.end(); it++)
    {
      uint8 d = *it;
      if (d < 32 || d > 126 || d == '?')
        buffer += string_printf ("\\%03o", d);
      else if (d == '\\')
        buffer += "\\\\";
      else if (d == '"')
        buffer += "\\\"";
      else
        buffer += d;
    }
  return buffer;
}

/* --- storage helpers --- */
#define	gstring_puts(gstring, string)	g_string_append (gstring, string)
#define	gstring_putc(gstring, vchar)	g_string_append_c (gstring, vchar)
#define	gstring_printf			g_string_append_printf
static void
gstring_break (GString  *gstring,
	       gboolean *needs_break,
	       guint     indent)
{
  gchar *s = g_new (gchar, indent + 1);
  memset (s, ' ', indent);
  s[indent] = 0;
  gstring_putc (gstring, '\n');
  gstring_puts (gstring, s);
  g_free (s);
  *needs_break = FALSE;
}

static void
gstring_check_break (GString  *gstring,
                     gboolean *needs_break,
                     guint     indent)
{
  if (*needs_break)
    gstring_break (gstring, needs_break, indent);
}


/* --- functions --- */
static GTokenType
sfi_scanner_parse_real_num (GScanner *scanner,
			    SfiReal  *real_p,
			    SfiNum   *num_p)
{
  gboolean negate = FALSE;
  gdouble vdouble;
  guint64 ui64;

  g_scanner_get_next_token (scanner);
  if (scanner->token == '-')
    {
      negate = TRUE;
      g_scanner_get_next_token (scanner);
    }
  if (scanner->token == G_TOKEN_INT)
    {
      ui64 = scanner->value.v_int64;
      vdouble = ui64;
    }
  else if (scanner->token == G_TOKEN_FLOAT)
    {
      vdouble = scanner->value.v_float;
      ui64 = vdouble;
    }
  else
    return G_TOKEN_INT;
  if (num_p)
    {
      *num_p = ui64;
      if (negate)
	*num_p = - *num_p;
    }
  if (real_p)
    *real_p = negate ? -vdouble : vdouble;
  return G_TOKEN_NONE;
}

static void
sfi_serialize_rec_typed (SfiRec  *rec,
			 GString *gstring)
{
  if (!rec)
    gstring_puts (gstring, SFI_SERIAL_NULL_TOKEN);
  else
    {
      guint i;
      gstring_puts (gstring, "(");
      for (i = 0; i < rec->n_fields; i++)
	{
	  if (i)
	    gstring_putc (gstring, ' ');
	  gstring_printf (gstring, "(%s ", rec->field_names[i]);
	  sfi_value_store_typed (rec->fields + i, gstring);
	  gstring_putc (gstring, ')');
	}
      gstring_putc (gstring, ')');
    }
}

static GTokenType
sfi_parse_rec_typed (GScanner *scanner,
		     GValue   *value)
{
  g_scanner_get_next_token (scanner);
  if (sfi_serial_check_parse_null_token (scanner))
    sfi_value_set_rec (value, NULL);
  else if (scanner->token == '(')
    {
      SfiRec *rec = sfi_rec_new ();
      sfi_value_set_rec (value, rec);
      sfi_rec_unref (rec);
      while (g_scanner_peek_next_token (scanner) != ')')
	{
	  GTokenType token;
	  GValue *fvalue;
	  gchar *field_name;
	  parse_or_return (scanner, '(');
	  parse_or_return (scanner, G_TOKEN_IDENTIFIER);
	  field_name = g_strdup (scanner->value.v_identifier);
	  fvalue = sfi_value_empty ();
	  token = sfi_value_parse_typed (fvalue, scanner);
	  if (token != G_TOKEN_NONE || g_scanner_peek_next_token (scanner) != ')')
	    {
	      g_free (field_name);
	      sfi_value_free (fvalue);
	      if (token == G_TOKEN_NONE)
		{
		  g_scanner_get_next_token (scanner);	/* eat ')' */
		  token = GTokenType (')');
		}
	      return token;
	    }
	  g_scanner_get_next_token (scanner);		/* eat ')' */
	  sfi_rec_set (rec, field_name, fvalue);
	  g_free (field_name);
	  sfi_value_free (fvalue);
	}
      parse_or_return (scanner, ')');
    }
  else
    return GTokenType ('(');
  return G_TOKEN_NONE;
}

static GTokenType
sfi_serialize_primitives (SfiSCategory scat,
			  GValue      *value,
			  GString     *gstring,
			  GScanner    *scanner,
			  const gchar *hints)
{
  switch (scat)
    {
    case SFI_SCAT_BOOL:
      if (gstring)
	{
	  gstring_puts (gstring, sfi_value_get_bool (value) ? "#t" : "#f");
	}
      else
	{
	  gboolean v_bool = FALSE;
	  g_scanner_get_next_token (scanner);
	  if (scanner->token == G_TOKEN_INT)
	    v_bool = scanner->value.v_int64 != 0;
          else if (scanner->token == G_TOKEN_FLOAT)
            v_bool = scanner->value.v_float >= -0.5 && scanner->value.v_float <= 0.5;
	  else if (scanner->token == '#')
	    {
	      g_scanner_get_next_token (scanner);
	      if (scanner->token == 't' || scanner->token == 'T')
		v_bool = TRUE;
	      else if (scanner->token == 'f' || scanner->token == 'F')
		v_bool = FALSE;
	      else
		return GTokenType ('f');
	    }
	  else
	    return GTokenType ('#');
	  sfi_value_set_bool (value, v_bool);
	}
      break;
    case SFI_SCAT_INT:
      if (gstring)
	{
	  SfiInt iv = sfi_value_get_int (value);
          if (g_option_check (hints, "hex") &&  /* hexadecimal hint */
              iv <= G_MAXINT)
            gstring_printf (gstring, "0x%08x", iv);
          else
            gstring_printf (gstring, "%d", iv);
	}
      else
	{
	  SfiNum num;
	  GTokenType token = sfi_scanner_parse_real_num (scanner, NULL, &num);
	  if (token != G_TOKEN_NONE)
	    return token;
	  sfi_value_set_int (value, num);
	}
      break;
    case SFI_SCAT_NUM:
      if (gstring)
	{
	  SfiNum num = sfi_value_get_num (value);
	  gstring_printf (gstring, "%lld", num);
	}
      else
	{
	  SfiNum num;
	  GTokenType token = sfi_scanner_parse_real_num (scanner, NULL, &num);
	  if (token != G_TOKEN_NONE)
	    return token;
	  sfi_value_set_num (value, num);
	}
      break;
    case SFI_SCAT_REAL:
      if (gstring)
	{
	  gchar numbuf[G_ASCII_DTOSTR_BUF_SIZE + 1] = "";

	  if (g_option_check (hints, "f"))      /* float hint */
	    gstring_puts (gstring, g_ascii_formatd (numbuf, G_ASCII_DTOSTR_BUF_SIZE, "%.7g", sfi_value_get_real (value)));
	  else
	    gstring_puts (gstring, g_ascii_formatd (numbuf, G_ASCII_DTOSTR_BUF_SIZE, "%.17g", sfi_value_get_real (value)));
	}
      else
	{
	  SfiReal real;
	  GTokenType token = sfi_scanner_parse_real_num (scanner, &real, NULL);
	  if (token != G_TOKEN_NONE)
	    return token;
	  sfi_value_set_real (value, real);
	}
      break;
    case SFI_SCAT_STRING:
      if (gstring)
	{
	  char *cstring = const_cast<char*> (sfi_value_get_string (value));
	  if (cstring)
            gstring_puts (gstring, String ("\"" + string_to_cescape (cstring) + "\"").c_str());
	  else
	    gstring_puts (gstring, SFI_SERIAL_NULL_TOKEN);
	}
      else
	{
	  g_scanner_get_next_token (scanner);
	  if (sfi_serial_check_parse_null_token (scanner))
	    sfi_value_set_string (value, NULL);
	  else if (scanner->token == G_TOKEN_STRING)
            sfi_value_set_string (value, scanner->value.v_string);
	  else
	    return G_TOKEN_STRING;
	}
      break;
    case SFI_SCAT_CHOICE:
      if (gstring)
	{
	  char *cstring = const_cast<char*> (sfi_value_get_string (value));
	  if (!cstring)
	    gstring_puts (gstring, SFI_SERIAL_NULL_TOKEN);
	  else
	    gstring_printf (gstring, "%s", cstring);
	}
      else
	{
	  g_scanner_get_next_token (scanner);
	  if (sfi_serial_check_parse_null_token (scanner))
	    sfi_value_set_choice (value, NULL);
	  else if (scanner->token == G_TOKEN_IDENTIFIER)
	    sfi_value_set_choice (value, scanner->value.v_identifier);
	  else
	    return G_TOKEN_IDENTIFIER;
	}
      break;
    case SFI_SCAT_PROXY:
      if (gstring)
	{
	  SfiProxy proxy = sfi_value_get_proxy (value);
	  gstring_printf (gstring, "%lu", proxy);
	}
      else
	{
	  SfiNum num;
	  GTokenType token = sfi_scanner_parse_real_num (scanner, NULL, &num);
          if (token != G_TOKEN_NONE)
	    return token;
	  sfi_value_set_proxy (value, num);
	}
      break;
    case SFI_SCAT_BBLOCK:
      if (gstring)
	{
	  SfiBBlock *bblock = sfi_value_get_bblock (value);
	  if (!bblock)
	    gstring_puts (gstring, SFI_SERIAL_NULL_TOKEN);
	  else
	    {
	      guint i;
	      gstring_puts (gstring, "(");
	      if (bblock->n_bytes)
		gstring_printf (gstring, "%u", bblock->bytes[0]);
	      for (i = 1; i < bblock->n_bytes; i++)
		gstring_printf (gstring, " %u", bblock->bytes[i]);
	      gstring_puts (gstring, ")");
	    }
	}
      else
	{
	  g_scanner_get_next_token (scanner);
	  if (sfi_serial_check_parse_null_token (scanner))
	    sfi_value_set_bblock (value, NULL);
	  else if (scanner->token == '(')
	    {
	      SfiBBlock *bblock;
	      bblock = sfi_bblock_new ();
	      sfi_value_set_bblock (value, bblock);
	      sfi_bblock_unref (bblock);
	      while (g_scanner_get_next_token (scanner) != ')')
		if (scanner->token == G_TOKEN_INT)
		  sfi_bblock_append1 (bblock, scanner->value.v_int64);
		else
		  return G_TOKEN_INT;
	    }
          else
	    return GTokenType ('(');
	}
      break;
    case SFI_SCAT_FBLOCK:
      if (gstring)
	{
	  SfiFBlock *fblock = sfi_value_get_fblock (value);
	  if (!fblock)
	    gstring_puts (gstring, SFI_SERIAL_NULL_TOKEN);
	  else
	    {
	      guint i;
	      gstring_puts (gstring, "(");
	      if (fblock->n_values)
                g_string_append (gstring, Bse::string_printf ("%.9g", fblock->values[0]).c_str());
	      for (i = 1; i < fblock->n_values; i++)
                g_string_append (gstring, Bse::string_printf (" %.9g", fblock->values[i]).c_str());
	      gstring_puts (gstring, ")");
	    }
	}
      else
	{
	  g_scanner_get_next_token (scanner);
	  if (sfi_serial_check_parse_null_token (scanner))
	    sfi_value_set_fblock (value, NULL);
	  else if (scanner->token == '(')
	    {
	      SfiFBlock *fblock;
	      GTokenType token;
	      SfiReal real;
	      fblock = sfi_fblock_new ();
	      sfi_value_set_fblock (value, fblock);
	      sfi_fblock_unref (fblock);
              while (g_scanner_peek_next_token (scanner) != ')')
		{
		  token = sfi_scanner_parse_real_num (scanner, &real, NULL);
		  if (token != G_TOKEN_NONE)
		    return G_TOKEN_FLOAT;
		  sfi_fblock_append1 (fblock, real);
		}
	      parse_or_return (scanner, ')');
	    }
	  else
	    return GTokenType ('(');
	}
      break;
    case SFI_SCAT_PSPEC:
      if (gstring)
	{
          GParamSpec *pspec = sfi_value_get_pspec (value);
	  SfiRec *rec = pspec ? sfi_pspec_to_rec (pspec) : NULL;
	  sfi_serialize_rec_typed (rec, gstring);
	  if (rec)
	    sfi_rec_unref (rec);
	}
      else
	{
	  GValue tmpv = { 0, };
	  GTokenType token;
	  g_value_init (&tmpv, SFI_TYPE_REC);
	  token = sfi_parse_rec_typed (scanner, &tmpv);
	  if (token == G_TOKEN_NONE)
	    {
	      SfiRec *rec = sfi_value_get_rec (&tmpv);
	      GParamSpec *pspec = rec ? sfi_pspec_from_rec (rec) : NULL;
	      sfi_value_set_pspec (value, pspec);
	      if (pspec)
		g_param_spec_sink (pspec);
	    }
	  g_value_unset (&tmpv);
          if (token != G_TOKEN_NONE)
	    return token;
	}
      break;
    case SFI_SCAT_NOTE:
      if (gstring)
	{
	  gchar *string = sfi_note_to_string (sfi_value_get_int (value));
	  gstring_printf (gstring, "%s", string);
	  g_free (string);
	}
      else
	{
	  gchar *error = NULL;
	  SfiNum num;
	  if (g_scanner_peek_next_token (scanner) == G_TOKEN_STRING) // FIXME: compat code, deprecated syntax
	    {
	      g_scanner_get_next_token (scanner);
	      g_scanner_warn (scanner, "deprecated syntax: encountered string instead of note symbol");
	    }
	  else
	    {
	      parse_or_return (scanner, G_TOKEN_IDENTIFIER);
	    }
	  num = sfi_note_from_string_err (scanner->value.v_identifier, &error);
	  if (error)
	    g_scanner_warn (scanner, "%s", error);
	  g_free (error);
	  sfi_value_set_int (value, num);
	}
      break;
    case SFI_SCAT_TIME:
      if (gstring)
	{
	  gchar *string = sfi_time_to_string (sfi_time_to_utc (sfi_value_get_num (value)));
	  gstring_printf (gstring, "\"%s\"", string);
	  g_free (string);
	}
      else
	{
	  SfiTime ustime;
	  gchar *error = NULL;
	  parse_or_return (scanner, G_TOKEN_STRING);
	  ustime = sfi_time_from_string_err (scanner->value.v_string, &error);
	  if (error)
	    g_scanner_warn (scanner, "%s", error);
	  g_free (error);
	  if (ustime < 1)
	    ustime = SFI_MIN_TIME;
	  sfi_value_set_num (value, sfi_time_from_utc (ustime));
	}
      break;
    default:
      if (gstring)
	g_error ("%s: unimplemented category (%u)", G_STRLOC, scat);
      else
	{
	  g_scanner_warn (scanner, "unimplemented category (%u)", scat);
	  return G_TOKEN_ERROR;
	}
    }
  return G_TOKEN_NONE;
}

void
sfi_value_store_typed (const GValue *value,
		       GString      *gstring)
{
  SfiSCategory scat;

  g_return_if_fail (G_IS_VALUE (value));
  g_return_if_fail (gstring != NULL);

  scat = SfiSCategory (sfi_categorize_type (G_VALUE_TYPE (value)) & SFI_SCAT_TYPE_MASK);
  switch (scat)
    {
      SfiSeq *seq;
      SfiRec *rec;
    case SFI_SCAT_BOOL:
    case SFI_SCAT_INT:
    case SFI_SCAT_NUM:
    case SFI_SCAT_REAL:
    case SFI_SCAT_STRING:
    case SFI_SCAT_CHOICE:
    case SFI_SCAT_PROXY:
    case SFI_SCAT_BBLOCK:
    case SFI_SCAT_FBLOCK:
    case SFI_SCAT_PSPEC:
      gstring_printf (gstring, "(%c ", scat);
      sfi_serialize_primitives (scat, (GValue*) value, gstring, NULL, NULL);
      gstring_putc (gstring, ')');
      break;
    case SFI_SCAT_SEQ:
      gstring_printf (gstring, "(%c", scat);
      seq = sfi_value_get_seq (value);
      if (!seq)
	gstring_puts (gstring, " " SFI_SERIAL_NULL_TOKEN);
      else
	{
	  guint i;
	  gstring_puts (gstring, " (");
	  for (i = 0; i < seq->n_elements; i++)
	    {
	      if (i)
		gstring_putc (gstring, ' ');
	      sfi_value_store_typed (seq->elements + i, gstring);
	    }
	  gstring_putc (gstring, ')');
	}
      gstring_putc (gstring, ')');
      break;
    case SFI_SCAT_REC:
      gstring_printf (gstring, "(%c ", scat);
      rec = sfi_value_get_rec (value);
      if (rec)
	sfi_rec_sort (rec);
      sfi_serialize_rec_typed (rec, gstring);
      gstring_putc (gstring, ')');
      break;
    default:
      g_error ("%s: unimplemented category (%u)", G_STRLOC, scat);
    }
}

GTokenType
sfi_value_parse_typed (GValue   *value,
		       GScanner *scanner)
{
  g_return_val_if_fail (value != NULL && G_VALUE_TYPE (value) == 0, G_TOKEN_ERROR);
  g_return_val_if_fail (scanner != NULL, G_TOKEN_ERROR);

  parse_or_return (scanner, '(');
  char scat = g_scanner_get_next_token (scanner);
  if (!((scat >= 'a' && scat <= 'z') ||
	(scat >= 'A' && scat <= 'Z')))
    return G_TOKEN_IDENTIFIER;
  switch (scat)
    {
      GTokenType token;
    case SFI_SCAT_BOOL:
    case SFI_SCAT_INT:
    case SFI_SCAT_NUM:
    case SFI_SCAT_REAL:
    case SFI_SCAT_STRING:
    case SFI_SCAT_CHOICE:
    case SFI_SCAT_PROXY:
    case SFI_SCAT_BBLOCK:
    case SFI_SCAT_FBLOCK:
    case SFI_SCAT_PSPEC:
      g_value_init (value, sfi_category_type (SfiSCategory (scat)));
      token = sfi_serialize_primitives (SfiSCategory (scat), value, NULL, scanner, NULL);
      if (token != G_TOKEN_NONE)
	return token;
      parse_or_return (scanner, ')');
      break;
    case SFI_SCAT_SEQ:
      g_value_init (value, SFI_TYPE_SEQ);
      g_scanner_get_next_token (scanner);
      if (sfi_serial_check_parse_null_token (scanner))
	sfi_value_set_seq (value, NULL);
      else if (scanner->token == '(')
	{
	  SfiSeq *seq;
	  seq = sfi_seq_new ();
	  sfi_value_set_seq (value, seq);
	  sfi_seq_unref (seq);
	  while (g_scanner_peek_next_token (scanner) != ')')
	    {
	      GValue *evalue = sfi_value_empty ();
	      token = sfi_value_parse_typed (evalue, scanner);
	      if (token != G_TOKEN_NONE)
		{
		  sfi_value_free (evalue);
		  return token;
		}
	      sfi_seq_append (seq, evalue);
	      sfi_value_free (evalue);
	    }
	  parse_or_return (scanner, ')');
	}
      else
	return GTokenType ('(');
      parse_or_return (scanner, ')');
      break;
    case SFI_SCAT_REC:
      g_value_init (value, SFI_TYPE_REC);
      token = sfi_parse_rec_typed (scanner, value);
      if (token != G_TOKEN_NONE)
	return token;
      parse_or_return (scanner, ')');
      break;
    default:
      g_scanner_warn (scanner, "skipping value of unknown category `%c'", scat);
      return scanner_skip_statement (scanner, 1);
    }
  return G_TOKEN_NONE;
}

static void
value_store_param (const GValue *value,
		   GString      *gstring,
		   gboolean     *needs_break,
                   gboolean      compound_break,
		   GParamSpec   *pspec,
		   guint         indent)
{
  SfiSCategory scat = sfi_categorize_pspec (pspec);

  gstring_check_break (gstring, needs_break, indent);
  switch (scat)
    {
      SfiSeq *seq;
      SfiRec *rec;
    case SFI_SCAT_BOOL:
    case SFI_SCAT_INT:
    case SFI_SCAT_NUM:
    case SFI_SCAT_REAL:
    case SFI_SCAT_STRING:
    case SFI_SCAT_CHOICE:
    case SFI_SCAT_PROXY:
    case SFI_SCAT_BBLOCK:
    case SFI_SCAT_FBLOCK:
    case SFI_SCAT_PSPEC:
    case SFI_SCAT_NOTE:
    case SFI_SCAT_TIME:
      sfi_serialize_primitives (scat, (GValue*) value, gstring, NULL, sfi_pspec_get_options (pspec));
      break;
    case SFI_SCAT_SEQ:
      *needs_break = *needs_break || compound_break;
      seq = sfi_value_get_seq (value);
      if (!seq)
	gstring_puts (gstring, SFI_SERIAL_NULL_TOKEN);
      else
	{
          GParamSpec *espec = sfi_pspec_get_seq_element (pspec);
	  guint i, nth = 0;
	  /* we ignore non conforming elements */
	  if (espec)
	    {
	      for (i = 0; i < seq->n_elements; i++)
		if (G_VALUE_HOLDS (seq->elements + i, G_PARAM_SPEC_VALUE_TYPE (espec)))
		  {
		    if (nth == 0)
		      {
			gstring_check_break (gstring, needs_break, indent);
			gstring_puts (gstring, "("); /* open sequence */
		      }
		    else if (nth % 5)
		      gstring_putc (gstring, ' ');
		    else
		      *needs_break = TRUE;
		    nth++;
		    value_store_param (seq->elements + i, gstring, needs_break, FALSE, espec, indent + 1);
		  }
	    }
          if (nth == 0)
	    gstring_puts (gstring, "("); /* open sequence */
	  gstring_putc (gstring, ')'); /* close sequence */
	}
      *needs_break = TRUE;
      break;
    case SFI_SCAT_REC:
      *needs_break = *needs_break || compound_break;
      rec = sfi_value_get_rec (value);
      if (!rec)
	gstring_puts (gstring, SFI_SERIAL_NULL_TOKEN);
      else
	{
	  SfiRecFields fspecs = sfi_pspec_get_rec_fields (pspec);
	  guint i, nth = 0;
          /* we ignore non conforming fields */
	  for (i = 0; i < fspecs.n_fields; i++)
	    {
	      GValue *fvalue = sfi_rec_get (rec, fspecs.fields[i]->name);
	      if (fvalue && G_VALUE_HOLDS (fvalue, G_PARAM_SPEC_VALUE_TYPE (fspecs.fields[i])))
		{
		  if (nth++ == 0)
		    {
		      gstring_check_break (gstring, needs_break, indent);
		      gstring_puts (gstring, "("); /* open record */
		    }
		  else
		    gstring_break (gstring, needs_break, indent + 1);
		  gstring_printf (gstring, "(%s ", fspecs.fields[i]->name); /* open field */
		  value_store_param (fvalue, gstring, needs_break, FALSE, fspecs.fields[i], indent + 2 + 1);
		  gstring_putc (gstring, ')'); /* close field */
		}
	    }
	  if (nth == 0)
	    gstring_puts (gstring, "("); /* open record */
          gstring_putc (gstring, ')'); /* close record */
	}
      *needs_break = TRUE;
      break;
    default:
      g_error ("%s: unimplemented category (%u)", G_STRLOC, scat);
    }
}

void
sfi_value_store_param (const GValue *value,
                       GString      *gstring,
                       GParamSpec   *pspec,
                       guint         indent)
{
  gboolean needs_break = FALSE;

  g_return_if_fail (G_IS_VALUE (value));
  g_return_if_fail (gstring != NULL);
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  g_return_if_fail (G_VALUE_HOLDS (value, G_PARAM_SPEC_VALUE_TYPE (pspec)));

  gstring_check_break (gstring, &needs_break, indent);
  gstring_printf (gstring, "(%s ", pspec->name);
  value_store_param (value, gstring, &needs_break, TRUE, pspec, indent + 2);
  gstring_putc (gstring, ')');
}

static GTokenType
value_parse_param (GValue     *value,
		   GScanner   *scanner,
		   GParamSpec *pspec,
		   gboolean    close_statement)
{
  SfiSCategory scat;

  scat = sfi_categorize_pspec (pspec);
  switch (scat)
    {
      GTokenType token;
    case SFI_SCAT_BOOL:
    case SFI_SCAT_INT:
    case SFI_SCAT_NUM:
    case SFI_SCAT_REAL:
    case SFI_SCAT_STRING:
    case SFI_SCAT_CHOICE:
    case SFI_SCAT_PROXY:
    case SFI_SCAT_BBLOCK:
    case SFI_SCAT_FBLOCK:
    case SFI_SCAT_PSPEC:
    case SFI_SCAT_NOTE:
    case SFI_SCAT_TIME:
      token = sfi_serialize_primitives (scat, value, NULL, scanner, sfi_pspec_get_options (pspec));
      if (token != G_TOKEN_NONE)
	return token;
      break;
    case SFI_SCAT_SEQ:
      g_scanner_get_next_token (scanner);
      if (sfi_serial_check_parse_null_token (scanner))
	sfi_value_set_seq (value, NULL);
      else if (scanner->token == '(')
	{
	  GParamSpec *espec = sfi_pspec_get_seq_element (pspec);
	  SfiSeq *seq;
	  seq = sfi_seq_new ();
	  sfi_value_set_seq (value, seq);
	  sfi_seq_unref (seq);
	  if (espec)
	    {
	      GValue *evalue = sfi_value_empty ();
	      g_value_init (evalue, G_PARAM_SPEC_VALUE_TYPE (espec));
	      while (g_scanner_peek_next_token (scanner) != ')')
		{
		  token = value_parse_param (evalue, scanner, espec, FALSE);
		  if (token != G_TOKEN_NONE)
		    {
		      sfi_value_free (evalue);
		      return token;
		    }
		  sfi_seq_append (seq, evalue);
		}
	      sfi_value_free (evalue);
	    }
	  parse_or_return (scanner, ')');
	}
      else
	return GTokenType ('(');
      break;
    case SFI_SCAT_REC:
      g_scanner_get_next_token (scanner);
      if (sfi_serial_check_parse_null_token (scanner))
	sfi_value_set_rec (value, NULL);
      else if (scanner->token == '(')
	{
	  SfiRec *rec;
	  rec = sfi_rec_new ();
	  sfi_value_set_rec (value, rec);
	  sfi_rec_unref (rec);
	  while (g_scanner_peek_next_token (scanner) != ')')
	    {
	      GParamSpec *fspec;
	      parse_or_return (scanner, '(');
	      parse_or_return (scanner, G_TOKEN_IDENTIFIER);
	      fspec = sfi_pspec_get_rec_field (pspec, scanner->value.v_identifier);
	      if (!fspec)
		{
		  g_scanner_warn (scanner, "skipping unknown record field `%s'", scanner->value.v_identifier);
		  token = scanner_skip_statement (scanner, 1);
		  if (token != G_TOKEN_NONE)
		    return token;
		}
	      else
		{
		  GValue *fvalue = sfi_value_empty ();
                  g_value_init (fvalue, G_PARAM_SPEC_VALUE_TYPE (fspec));
		  token = value_parse_param (fvalue, scanner, fspec, TRUE);
		  if (token != G_TOKEN_NONE)
		    {
		      sfi_value_free (fvalue);
		      return token;
		    }
		  sfi_rec_set (rec, fspec->name, fvalue);
		  sfi_value_free (fvalue);
		}
	    }
	  parse_or_return (scanner, ')');
	}
      else
	return GTokenType ('(');
      break;
    default:
      if (close_statement)
	{
	  g_scanner_warn (scanner, "skipping value of unknown category `%c'", scat);
	  return scanner_skip_statement (scanner, 1);
	}
      else
	{
	  g_scanner_error (scanner, "unable to parse value of unknown category `%c'", scat);
	  return G_TOKEN_ERROR;
	}
    }
  if (close_statement)
    parse_or_return (scanner, ')');
  return G_TOKEN_NONE;
}

GTokenType
sfi_value_parse_param_rest (GValue     *value,
			    GScanner   *scanner,
			    GParamSpec *pspec)
{
  g_return_val_if_fail (value != NULL && G_VALUE_TYPE (value) == 0, G_TOKEN_ERROR);
  g_return_val_if_fail (scanner != NULL, G_TOKEN_ERROR);
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), G_TOKEN_ERROR);

  /* the scanner better be at the pspec name */
  g_return_val_if_fail (scanner->token == G_TOKEN_IDENTIFIER, G_TOKEN_ERROR);
  g_return_val_if_fail (strcmp (scanner->value.v_identifier, pspec->name) == 0, G_TOKEN_ERROR);

  g_value_init (value, G_PARAM_SPEC_VALUE_TYPE (pspec));

  return value_parse_param (value, scanner, pspec, TRUE);
}

gboolean
sfi_serial_check_parse_null_token (GScanner *scanner)
{
  g_return_val_if_fail (scanner != NULL, FALSE);

  if (scanner->token == '#' && g_scanner_peek_next_token (scanner) == 'f')
    {
      g_scanner_get_next_token (scanner);
      return TRUE;
    }
  else
    return FALSE;
}

void
sfi_value_store_stderr (const GValue *value)
{
  GString *gstring = g_string_new ("");
  sfi_value_store_typed (value, gstring);
  g_printerr ("((GValue*)%p)=%s\n", value, gstring->str);
  g_string_free (gstring, TRUE);
}
