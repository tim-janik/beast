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
#include <stdio.h>      /* sscanf() */
#include "sfiserial.h"
#include "sfiparams.h"
#include "sfitime.h"
#include "sfinote.h"


/* --- storage helpers --- */
#define	gstring_puts(gstring, string)	g_string_append (gstring, string)
#define	gstring_putc(gstring, vchar)	g_string_append_c (gstring, vchar)
#define	gstring_printf			g_string_append_printf


/* --- parsing aids --- */
#define parse_or_return(scanner, token)  G_STMT_START{ \
  GScanner *__s = (scanner); guint _t = (token); \
  if (g_scanner_get_next_token (__s) != _t) \
    return _t; \
}G_STMT_END
#define peek_or_return(scanner, token)   G_STMT_START{ \
  GScanner *__s = (scanner); guint _t = (token); \
  if (g_scanner_peek_next_token (__s) != _t) { \
    g_scanner_get_next_token (__s); /* advance position for error-handler */ \
    return _t; \
  } \
}G_STMT_END
static inline gboolean
check_nil (GScanner *scanner)
{
  return (scanner->token == G_TOKEN_IDENTIFIER &&
	  strcmp (scanner->value.v_identifier, "nil") == 0);
}


/* --- functions --- */
static void
gstring_handle_break (GString *gstring)
{
  /* better be safe than sorry? */
  gstring_putc (gstring, '\n');
}

static void
store_value_nonstruct (const GValue *value,
		       GString      *gstring,
		       SfiSCategory  cat)
{
  switch (cat)
    {
      SfiNum num;
      SfiFBlock *fblock;
      SfiProxy proxy;
      gchar *string;
      const gchar *cstring;
    case SFI_SCAT_BOOL:
      gstring_puts (gstring, sfi_value_get_bool (value) ? "#t" : "#f");
      break;
    case SFI_SCAT_INT:
      gstring_printf (gstring, "%d", sfi_value_get_int (value));
      break;
    case SFI_SCAT_NUM:
      num = sfi_value_get_num (value);
      if (num <= G_MAXINT && num >= G_MININT)
	gstring_printf (gstring, "%d", (SfiInt) num);
      else
	{
	  guint i1, i2;
	  if (num < 0)
	    {
	      gstring_puts (gstring, "- ");
	      num = -num;
	    }
	  i1 = num >> 32, i2 = num & 0xffffffff;
	  gstring_printf (gstring, "'(0x%x . 0x%08x)", i1, i2);
	}
      break;
    case SFI_SCAT_REAL:
      gstring_printf (gstring, "%.18g", sfi_value_get_real (value));
      break;
    case SFI_SCAT_FBLOCK:
      fblock = sfi_value_get_fblock (value);
      if (!fblock)
	gstring_puts (gstring, "nil");
      else
	{
	  guint i;
	  gstring_printf (gstring, "'(");
	  if (fblock->n_values)
	    gstring_printf (gstring, "%.18g", fblock->values[0]);
	  for (i = 1; i < fblock->n_values; i++)
	    gstring_printf (gstring, " %.18g", fblock->values[i]);
	  gstring_printf (gstring, ")");
	}
      break;
    case SFI_SCAT_STRING:
      cstring = sfi_value_get_string (value);
      if (cstring)
	{
	  string = g_strescape (cstring, NULL);
	  gstring_putc (gstring, '"');
	  gstring_puts (gstring, string);
	  gstring_putc (gstring, '"');
	  g_free (string);
	}
      else
	gstring_puts (gstring, "nil");
      break;
    case SFI_SCAT_CHOICE:
      cstring = sfi_value_get_string (value);
      if (!cstring)
	gstring_puts (gstring, "nil");
      else
	gstring_printf (gstring, "'%s", cstring);
      break;
    case SFI_SCAT_PROXY:
      proxy = sfi_value_get_proxy (value);
      gstring_printf (gstring, "%lu", proxy);
      break;
    case SFI_SCAT_NOTE:
      string = sfi_note_to_string (sfi_value_get_int (value));
      gstring_printf (gstring, "\"%s\"", string);
      g_free (string);
      break;
    case SFI_SCAT_TIME:
      string = sfi_time_to_string (sfi_time_to_utc (sfi_value_get_num (value)));
      gstring_printf (gstring, "\"%s\"", string);
      g_free (string);
      break;
    default:
      g_error ("%s: unable to handle values of category `%c'", G_STRLOC, cat);
      break;
    }
}

void
sfi_value_store (const GValue *value,
		 GString      *gstring,
		 GParamSpec   *pspec)
{
  SfiSCategory cat;
  const gchar *indent = "";
  
  g_return_if_fail (G_IS_VALUE (value));
  g_return_if_fail (gstring != NULL);
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  gstring_handle_break (gstring);

  cat = sfi_categorize_pspec (pspec);
  switch (cat)
    {
      SfiSeq *seq;
      SfiRec *rec;
      GParamSpec *espec;
      SfiRecFields rfields;
    case SFI_SCAT_BOOL:
    case SFI_SCAT_INT:
    case SFI_SCAT_NUM:
    case SFI_SCAT_REAL:
    case SFI_SCAT_STRING:
    case SFI_SCAT_CHOICE:
    case SFI_SCAT_PROXY:
    case SFI_SCAT_NOTE:
    case SFI_SCAT_TIME:
    case SFI_SCAT_FBLOCK:
      store_value_nonstruct (value, gstring, cat);
      break;
    case SFI_SCAT_SEQ:
      seq = sfi_value_get_seq (value);
      espec = sfi_pspec_get_seq_element (pspec);
      if (!seq || !sfi_seq_check (seq, G_PARAM_SPEC_VALUE_TYPE (espec)))
        gstring_puts (gstring, "nil");
      else
	{
	  gchar *lbreak = g_strconcat ("\n", indent, "  ", NULL);
	  guint i;
	  gstring_printf (gstring, "'(");
	  for (i = 0; i < seq->n_elements; i++)
	    {
	      GValue *evalue = sfi_seq_get (seq, i);
	      gstring_puts (gstring, lbreak);
	      sfi_value_store (evalue, gstring, espec);
	    }
	  gstring_puts (gstring, ")");
	  g_free (lbreak);
	}
      break;
    case SFI_SCAT_REC:
      rec = sfi_value_get_rec (value);
      rfields = sfi_pspec_get_rec_fields (pspec);
      if (!rec || !sfi_rec_check (rec, rfields))
	gstring_puts (gstring, "nil");
      else
	{
	  gchar *lbreak = g_strconcat ("\n", indent, "  ", NULL);
	  guint i;
	  gstring_printf (gstring, "'(");
	  for (i = 0; i < rfields.n_fields; i++)
	    {
	      GValue *evalue = sfi_rec_get (rec, rfields.fields[i]->name);
	      gstring_puts (gstring, lbreak);
	      gstring_printf (gstring, "'(%s ", rfields.fields[i]->name);
	      sfi_value_store (evalue, gstring, rfields.fields[i]);
	      gstring_puts (gstring, ")");
	    }
	  gstring_puts (gstring, ")");
	  g_free (lbreak);
	}
      break;
    default:
      g_error ("%s: unable to handle values of type `%s' (category: `%c')", G_STRLOC, G_VALUE_TYPE_NAME (value), cat);
      break;
    }
}

static GTokenType
scanner_skip_statement (GScanner *scanner,
			guint     level)
{
  while (level)
    {
      g_scanner_get_next_token (scanner);
      if (scanner->token == G_TOKEN_EOF ||
	  scanner->token == G_TOKEN_ERROR)
	return ')';
      if (scanner->token == '(')
	level++;
      else if (scanner->token == ')')
	level--;
    }
  return G_TOKEN_NONE;
}

static gboolean
g_scanner_peeked_integer (GScanner *scanner)
{
  return (scanner->next_token == G_TOKEN_BINARY ||
	  scanner->next_token == G_TOKEN_OCTAL ||
	  scanner->next_token == G_TOKEN_INT ||
	  scanner->next_token == G_TOKEN_HEX);
}

static GTokenType
sfi_scanner_parse_real_num (GScanner *scanner,
			    SfiReal  *real_p,
			    SfiNum   *num_p,
			    gboolean  closing_parenthesis)
{
  gboolean negate = FALSE;
  gchar *cset_identifier_first = scanner->config->cset_identifier_first;
  gchar *cset_identifier_nth = scanner->config->cset_identifier_nth;

  g_return_val_if_fail (g_scanner_peeked_integer (scanner) == FALSE, G_TOKEN_ERROR);

  /* hack around GScanner using strtol() for integer conversions, so we
   * support numbers greater than 2^31-1, up to 2^64-1.
   */

  scanner->config->cset_identifier_first = "0123456789.";
  scanner->config->cset_identifier_nth = "0123456789.+-eE";
  g_scanner_get_next_token (scanner);
  if (scanner->token == ')' && closing_parenthesis)
    {
      /* spcial case for lists of numbers */
      scanner->config->cset_identifier_first = cset_identifier_first;
      scanner->config->cset_identifier_nth = cset_identifier_nth;
      return ')';
    }
  if (scanner->token == '-')
    {
      negate = TRUE;
      g_scanner_get_next_token (scanner);
    }
  scanner->config->cset_identifier_first = cset_identifier_first;
  scanner->config->cset_identifier_nth = cset_identifier_nth;

  if (scanner->token == G_TOKEN_IDENTIFIER)
    {
      guint64 ui64 = 0;
      gdouble vdouble;
      if (strchr (scanner->value.v_identifier, '.') ||
	  strchr (scanner->value.v_identifier, 'e') ||
	  strchr (scanner->value.v_identifier, 'E'))
	{
	  vdouble = g_strtod (scanner->value.v_identifier, NULL);
	  ui64 = vdouble;
	}
      else
	{
	  sscanf (scanner->value.v_identifier, "%llu", &ui64);
	  vdouble = ui64;
	}
      if (num_p)
	{
	  *num_p = ui64;
	  if (negate)
	    *num_p = - *num_p;
	}
      if (real_p)
	*real_p = negate ? -vdouble : vdouble;
    }
  else
    return G_TOKEN_INT;
  return G_TOKEN_NONE;
}

static GTokenType
parse_value_nonstruct (GValue       *value,
		       GScanner     *scanner,
		       SfiSCategory  cat,
		       GError      **errorp)
{
  GTokenType token;
  switch (cat)
    {
      SfiNum num;
      SfiReal real;
      SfiTime ustime;
      gboolean v_bool;
    case SFI_SCAT_BOOL:
      v_bool = FALSE;
      g_scanner_get_next_token (scanner);
      if (scanner->token == G_TOKEN_INT && scanner->value.v_int <= 1)
	v_bool = scanner->value.v_int != 0;
      else if (scanner->token == '#')
	{
	  g_scanner_get_next_token (scanner);
	  if (scanner->token == 't' || scanner->token == 'T')
	    v_bool = TRUE;
	  else if (scanner->token == 'f' || scanner->token == 'F')
	    v_bool = FALSE;
	  else
	    return 'f';
	}
      else
	return '#';
      sfi_value_set_bool (value, v_bool);
      break;
    case SFI_SCAT_INT:
      token = sfi_scanner_parse_real_num (scanner, NULL, &num, FALSE);
      if (token != G_TOKEN_NONE)
	return token;
      sfi_value_set_int (value, num);
      break;
    case SFI_SCAT_NUM:
      token = sfi_scanner_parse_real_num (scanner, NULL, &num, FALSE);
      if (token != G_TOKEN_NONE)
	return token;
      sfi_value_set_num (value, num);
      break;
    case SFI_SCAT_REAL:
      g_scanner_get_next_token (scanner);
      if (scanner->token == '-')
	{
	  v_bool = TRUE;
	  g_scanner_get_next_token (scanner);
	}
      else
	v_bool = FALSE;
      if (scanner->token == G_TOKEN_FLOAT)
	real = scanner->value.v_float;
      else if (scanner->token == G_TOKEN_INT)
	real = scanner->value.v_int;
      else
	return G_TOKEN_FLOAT;
      sfi_value_set_real (value, v_bool ? -real : real);
      break;
    case SFI_SCAT_FBLOCK:
      g_scanner_get_next_token (scanner);
      if (check_nil (scanner))
	sfi_value_set_fblock (value, NULL);
      else if (scanner->token == '\'') // float list
	{
	  guint nfield = G_MAXUINT;
	  SfiFBlock *fblock = sfi_fblock_new ();
	  parse_or_return (scanner, '(');
	  while (g_scanner_get_next_token (scanner) != ')')
	    if (scanner->token == '-' && nfield != fblock->n_values)
	      nfield = fblock->n_values;	// negate this field
	    else if (scanner->token == G_TOKEN_FLOAT)
	      sfi_fblock_append1 (fblock, scanner->value.v_float * (nfield == fblock->n_values ? -1 : 1));
	    else if (scanner->token == G_TOKEN_INT)
	      {
		gfloat f = scanner->value.v_int;
		sfi_fblock_append1 (fblock, nfield == fblock->n_values ? -f : f);
	      }
	    else
	      goto WANT_FLOAT;
	  sfi_value_set_fblock (value, fblock);
	  sfi_fblock_unref (fblock);
	  if (nfield == fblock->n_values)	// last element was '-'
	    {
	    WANT_FLOAT:
	      return G_TOKEN_FLOAT;
	    }
	}
      break;
    case SFI_SCAT_STRING:
      g_scanner_get_next_token (scanner);
      if (check_nil (scanner))
	sfi_value_set_string (value, NULL);
      else if (scanner->token == G_TOKEN_STRING)
	sfi_value_set_string (value, scanner->value.v_string);
      else
	return G_TOKEN_STRING;
      break;
    case SFI_SCAT_CHOICE:
      g_scanner_get_next_token (scanner);
      if (check_nil (scanner))
	sfi_value_set_choice (value, NULL);
      else if (scanner->token == '\'')
	{
	  parse_or_return (scanner, G_TOKEN_IDENTIFIER);
	  sfi_value_set_choice (value, scanner->value.v_identifier);
	}
      else
	return '\'';
      break;
    case SFI_SCAT_PROXY:
      parse_or_return (scanner, G_TOKEN_INT);
      sfi_value_set_proxy (value, scanner->value.v_int);
      break;
    case SFI_SCAT_NOTE:
      parse_or_return (scanner, G_TOKEN_STRING);
      num = sfi_note_from_string (scanner->value.v_string);
      if (!SFI_NOTE_IS_VALID (num))
	return G_TOKEN_STRING;
      sfi_value_set_int (value, num);
      break;
    case SFI_SCAT_TIME:
      parse_or_return (scanner, G_TOKEN_STRING);
      ustime = sfi_time_from_string (scanner->value.v_string);
      if (!ustime)
	return G_TOKEN_STRING;
      sfi_value_set_num (value, ustime);
      break;
    default:
      g_error ("%s: unable to handle values of category `%c'", G_STRLOC, cat);
      break;
    }
  return G_TOKEN_NONE;
}

static GTokenType
parse_rec_field (SfiRec      *rec,
		 GScanner    *scanner,
		 GParamSpec  *rspec,
		 GError     **errorp)
{
  GValue evalue = { 0, };
  GTokenType token;
  GParamSpec *fspec;
  parse_or_return (scanner, '(');
  parse_or_return (scanner, G_TOKEN_STRING);
  fspec = sfi_pspec_get_rec_field (rspec, scanner->value.v_string);
  if (!fspec)
    {
      sfi_set_error (errorp, 0, 0, "skipping unknown record field \"%s\"", scanner->value.v_string);
      return scanner_skip_statement (scanner, 1);
    }
  g_value_init (&evalue, G_PARAM_SPEC_VALUE_TYPE (fspec));
  token = sfi_value_parse (&evalue, scanner, fspec, errorp);
  if (token == G_TOKEN_NONE)
    sfi_rec_set (rec, fspec->name, &evalue);
  g_value_unset (&evalue);
  return token;
}

GTokenType
sfi_value_parse (GValue     *value,
		 GScanner   *scanner,
		 GParamSpec *pspec,
		 GError    **errorp)
{
  SfiSCategory cat;
  GType vtype;

  g_return_val_if_fail (G_IS_VALUE (value), G_TOKEN_ERROR);
  g_return_val_if_fail (scanner != NULL, G_TOKEN_ERROR);
  if (pspec)
    g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), G_TOKEN_ERROR);

  vtype = G_VALUE_TYPE (value);

  cat = sfi_categorize_pspec (pspec);
  switch (cat)
    {
      GParamSpec *espec;
      SfiSeq *seq;
    case SFI_SCAT_BOOL:
    case SFI_SCAT_INT:
    case SFI_SCAT_NUM:
    case SFI_SCAT_REAL:
    case SFI_SCAT_STRING:
    case SFI_SCAT_CHOICE:
    case SFI_SCAT_PROXY:
    case SFI_SCAT_NOTE:
    case SFI_SCAT_TIME:
    case SFI_SCAT_FBLOCK:
      return parse_value_nonstruct (value, scanner, cat, errorp);
      break;
    case SFI_SCAT_SEQ:
      espec = sfi_pspec_get_seq_element (pspec);
      if (check_nil (scanner))
	sfi_value_set_seq (value, NULL);
      else if (scanner->token == '\'')
	{
	  parse_or_return (scanner, '(');
	  seq = sfi_seq_new ();
	  while (g_scanner_peek_next_token (scanner) != ')')
	    {
	      GValue evalue = { 0, };
	      GTokenType token;
	      g_value_init (&evalue, G_PARAM_SPEC_VALUE_TYPE (espec));
	      token = sfi_value_parse (&evalue, scanner, espec, errorp);
	      if (token != G_TOKEN_NONE)
		{
		  g_value_unset (&evalue);
		  sfi_seq_unref (seq);
		  return token;
		}
	      sfi_seq_append (seq, &evalue);
	      g_value_unset (&evalue);
	    }
	  g_scanner_get_next_token (scanner);	/* eat closing ')' */
	  sfi_value_set_seq (value, seq);
	  sfi_seq_unref (seq);
	}
      else
	return '\'';
      break;
    case SFI_SCAT_REC:
      if (check_nil (scanner))
	sfi_value_set_rec (value, NULL);
      else if (scanner->token == '\'')
	{
	  SfiRec *rec;
	  parse_or_return (scanner, '(');
	  rec = sfi_rec_new ();
	  while (g_scanner_peek_next_token (scanner) != ')')
	    {
	      GTokenType token = parse_rec_field (rec, scanner, pspec, errorp);
	      if (token != G_TOKEN_NONE)
		{
		  sfi_rec_unref (rec);
		  return token;
		}
	    }
	  g_scanner_get_next_token (scanner);	/* eat closing ')' */
	  sfi_value_set_rec (value, rec);
	  sfi_rec_unref (rec);
	}
      else
	return '\'';
      break;
    default:
      g_error ("%s: unable to handle values of type `%s' (category: `%c')", G_STRLOC, G_VALUE_TYPE_NAME (value), cat);
      break;
    }
  return G_TOKEN_NONE;
}


/* --- next generation serialization --- */
static GTokenType
sfi_serialize_primitive_value (SfiSCategory scat,
			       GValue      *value,
			       GString     *gstring,
			       GScanner    *scanner)
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
	  if (scanner->token == G_TOKEN_INT && scanner->value.v_int <= 1)
	    v_bool = scanner->value.v_int != 0;
	  else if (scanner->token == '#')
	    {
	      g_scanner_get_next_token (scanner);
	      if (scanner->token == 't' || scanner->token == 'T')
		v_bool = TRUE;
	      else if (scanner->token == 'f' || scanner->token == 'F')
		v_bool = FALSE;
	      else
		return 'f';
	    }
	  else
	    return '#';
	  sfi_value_set_bool (value, v_bool);
	}
      break;
    case SFI_SCAT_INT:
      if (gstring)
	{
	  gstring_printf (gstring, "%d", sfi_value_get_int (value));
	}
      else
	{
	  SfiNum num;
	  GTokenType token = sfi_scanner_parse_real_num (scanner, NULL, &num, FALSE);
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
	  GTokenType token = sfi_scanner_parse_real_num (scanner, NULL, &num, FALSE);
	  if (token != G_TOKEN_NONE)
	    return token;
	  sfi_value_set_num (value, num);
	}
      break;
    case SFI_SCAT_REAL:
      if (gstring)
	{
	  gstring_printf (gstring, "%.18g", sfi_value_get_real (value));
	}
      else
	{
	  SfiReal real;
	  GTokenType token = sfi_scanner_parse_real_num (scanner, &real, NULL, FALSE);
	  if (token != G_TOKEN_NONE)
	    return token;
	  sfi_value_set_real (value, real);
	}
      break;
    case SFI_SCAT_STRING:
      if (gstring)
	{
	  gchar *cstring = sfi_value_get_string (value);
	  if (cstring)
	    {
	      gchar *string = g_strescape (cstring, NULL);
	      gstring_putc (gstring, '"');
	      gstring_puts (gstring, string);
	      gstring_putc (gstring, '"');
	      g_free (string);
	    }
	  else
	    gstring_puts (gstring, "nil");
	}
      else
	{
	  g_scanner_get_next_token (scanner);
	  if (check_nil (scanner))
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
	  gchar *cstring = sfi_value_get_string (value);
	  if (!cstring)
	    gstring_puts (gstring, "nil");
	  else
	    gstring_printf (gstring, "'%s", cstring);
	}
      else
	{
	  g_scanner_get_next_token (scanner);
	  if (check_nil (scanner))
	    sfi_value_set_choice (value, NULL);
	  else if (scanner->token == '\'')
	    {
	      parse_or_return (scanner, G_TOKEN_IDENTIFIER);
	      sfi_value_set_choice (value, scanner->value.v_identifier);
	    }
	  else
	    return '\'';
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
	  GTokenType token = sfi_scanner_parse_real_num (scanner, NULL, &num, FALSE);
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
	    gstring_puts (gstring, "nil");
	  else
	    {
	      guint i;
	      gstring_puts (gstring, "'(");
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
	  if (check_nil (scanner))
	    sfi_value_set_bblock (value, NULL);
	  else if (scanner->token == '\'')
	    {
	      SfiBBlock *bblock;
	      parse_or_return (scanner, '(');
	      bblock = sfi_bblock_new ();
	      sfi_value_set_bblock (value, bblock);
	      sfi_bblock_unref (bblock);
	      while (g_scanner_get_next_token (scanner) != ')')
		if (scanner->token == G_TOKEN_INT)
		  sfi_bblock_append1 (bblock, scanner->value.v_int);
		else
		  return G_TOKEN_INT;
	    }
          else
	    return '\'';
	}
      break;
    case SFI_SCAT_FBLOCK:
      if (gstring)
	{
	  SfiFBlock *fblock = sfi_value_get_fblock (value);
	  if (!fblock)
	    gstring_puts (gstring, "nil");
	  else
	    {
	      guint i;
	      gstring_puts (gstring, "'(");
	      if (fblock->n_values)
		gstring_printf (gstring, "%.9g", fblock->values[0]);
	      for (i = 1; i < fblock->n_values; i++)
		gstring_printf (gstring, " %.9g", fblock->values[i]);
	      gstring_puts (gstring, ")");
	    }
	}
      else
	{
	  g_scanner_get_next_token (scanner);
	  if (check_nil (scanner))
	    sfi_value_set_fblock (value, NULL);
	  else if (scanner->token == '\'')
	    {
	      SfiFBlock *fblock;
	      GTokenType token;
	      SfiReal real;
	      parse_or_return (scanner, '(');
	      fblock = sfi_fblock_new ();
	      sfi_value_set_fblock (value, fblock);
	      sfi_fblock_unref (fblock);
	      token = sfi_scanner_parse_real_num (scanner, &real, NULL, TRUE);
	      while (token != ')')
		{
		  if (token != G_TOKEN_NONE)
		    return G_TOKEN_FLOAT;
		  sfi_fblock_append1 (fblock, real);
		  token = sfi_scanner_parse_real_num (scanner, &real, NULL, TRUE);
		}
	    }
	  else
	    return '\'';
	}
      break;
    case SFI_SCAT_NOTE:
      if (gstring)
	{
	  gchar *string = sfi_note_to_string (sfi_value_get_int (value));
	  gstring_printf (gstring, "'%s", string);
	  g_free (string);
	}
      else
	{
	  gchar *error = NULL;
	  SfiNum num;
	  parse_or_return (scanner, '\'');
	  parse_or_return (scanner, G_TOKEN_IDENTIFIER);
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

  scat = sfi_categorize_type (G_VALUE_TYPE (value)) & SFI_SCAT_TYPE_MASK;
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
      gstring_printf (gstring, "(%c ", scat);
      sfi_serialize_primitive_value (scat, (GValue*) value, gstring, NULL);
      gstring_putc (gstring, ')');
      break;
    case SFI_SCAT_SEQ:
      gstring_printf (gstring, "(%c", scat);
      seq = sfi_value_get_seq (value);
      if (!seq)
	gstring_puts (gstring, " nil");
      else
	{
	  guint i;
	  gstring_puts (gstring, " '(");
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
      gstring_printf (gstring, "(%c", scat);
      rec = sfi_value_get_rec (value);
      if (!rec)
	gstring_puts (gstring, " nil");
      else
	{
	  guint i;
          gstring_puts (gstring, " '(");
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
  SfiSCategory scat;

  g_return_val_if_fail (value != NULL && G_VALUE_TYPE (value) == 0, G_TOKEN_ERROR);
  g_return_val_if_fail (scanner != NULL, G_TOKEN_ERROR);

  parse_or_return (scanner, '(');
  scat = g_scanner_get_next_token (scanner);
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
      g_value_init (value, sfi_category_type (scat));
      token = sfi_serialize_primitive_value (scat, value, NULL, scanner);
      if (token != G_TOKEN_NONE)
	return token;
      parse_or_return (scanner, ')');
      break;
    case SFI_SCAT_SEQ:
      g_value_init (value, SFI_TYPE_SEQ);
      g_scanner_get_next_token (scanner);
      if (check_nil (scanner))
	sfi_value_set_seq (value, NULL);
      else if (scanner->token == '\'')
	{
	  SfiSeq *seq;
	  parse_or_return (scanner, '(');
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
	return '\'';
      parse_or_return (scanner, ')');
      break;
    case SFI_SCAT_REC:
      g_value_init (value, SFI_TYPE_REC);
      g_scanner_get_next_token (scanner);
      if (check_nil (scanner))
	sfi_value_set_rec (value, NULL);
      else if (scanner->token == '\'')
	{
	  SfiRec *rec;
	  parse_or_return (scanner, '(');
	  rec = sfi_rec_new ();
	  sfi_value_set_rec (value, rec);
	  sfi_rec_unref (rec);
	  while (g_scanner_peek_next_token (scanner) != ')')
	    {
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
		      token = ')';
		    }
		  return token;
		}
	      g_scanner_get_next_token (scanner);	/* eat ')' */
	      sfi_rec_set (rec, field_name, fvalue);
	      g_free (field_name);
	      sfi_value_free (fvalue);
	    }
	  parse_or_return (scanner, ')');
	}
      else
	return '\'';
      parse_or_return (scanner, ')');
      break;
      break;
    default:
      g_scanner_warn (scanner, "skipping value of unknown category `%c'", scat);
      return scanner_skip_statement (scanner, 1);
    }
  return G_TOKEN_NONE;
}
