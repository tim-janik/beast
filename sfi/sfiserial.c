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


static GTokenType
parse_num (GScanner *scanner,
	   SfiNum   *num)
{
  SfiNum i1 = 0, i2 = 0;
  gboolean negate = FALSE;
  g_scanner_get_next_token (scanner);
  if (scanner->token == '-')
    {
      negate = TRUE;
      g_scanner_get_next_token (scanner);
    }
  if (scanner->token == G_TOKEN_INT)
    i2 = scanner->value.v_int;
  else if (scanner->token == G_TOKEN_FLOAT)
    i2 = scanner->value.v_float + 0.5;
  else if (scanner->token == '\'')	// 64bit cons
    {
      parse_or_return (scanner, '(');
      parse_or_return (scanner, G_TOKEN_INT);
      i1 = scanner->value.v_int;
      parse_or_return (scanner, '.');
      parse_or_return (scanner, G_TOKEN_INT);
      i2 = scanner->value.v_int;
      parse_or_return (scanner, ')');
    }
  else
    return G_TOKEN_INT;
  i1 <<= 32;
  i1 |= i2;
  *num = negate ? -i1 : i1;
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
      token = parse_num (scanner, &num);
      if (token != G_TOKEN_NONE)
	return token;
      sfi_value_set_int (value, num);
      break;
    case SFI_SCAT_NUM:
      token = parse_num (scanner, &num);
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
      ustime = sfi_time_from_string (scanner->value.v_string, errorp);
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
