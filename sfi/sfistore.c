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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "sfistore.h"
#include "sfiprimitives.h"
#include "sfiserial.h"
#include "sfiparams.h"
#include "sfilog.h"
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>


#define SER_INFO	sfi_info_keyfunc ("serialization")


/* --- structures --- */
typedef struct {
  SfiStoreReadBin reader;
  gpointer	  data;
  GDestroyNotify  destroy;
  off_t		  patch_offset;
  off_t		  offset, length;
} BBlock;


/* --- writable store --- */
SfiWStore*
sfi_wstore_new (void)
{
  SfiWStore *wstore = g_new0 (SfiWStore, 1);
  wstore->text = g_string_new (NULL);
  wstore->indent = 0;
  wstore->bblocks = NULL;
  wstore->comment_start = ';';
  return wstore;
}

void
sfi_wstore_destroy (SfiWStore *wstore)
{
  g_return_if_fail (wstore != NULL);
  
  g_string_free (wstore->text, TRUE);
  wstore->text = NULL;
  while (wstore->bblocks)
    {
      BBlock *bblock = sfi_ring_pop_head (&wstore->bblocks);
      if (bblock->destroy)
        bblock->destroy (bblock->data);
      g_free (bblock);
    }
  g_free (wstore);
}

static inline void
sfi_wstore_text_changed (SfiWStore *wstore)
{
  wstore->needs_break = wstore->text->len && wstore->text->str[wstore->text->len - 1] != '\n';
}

void
sfi_wstore_break (SfiWStore *wstore)
{
  g_return_if_fail (wstore != NULL);
  
  if (wstore->needs_break)
    {
      guint n;
      g_string_append_c (wstore->text, '\n');
      /* don't interpret indentation text as needs-break */
      sfi_wstore_text_changed (wstore);
      for (n = 0; n < wstore->indent; n += 2)
	g_string_append (wstore->text, "  ");
    }
}

void
sfi_wstore_push_level (SfiWStore *wstore)
{
  g_return_if_fail (wstore != NULL);
  
  wstore->indent += 2;
}

void
sfi_wstore_pop_level (SfiWStore *wstore)
{
  g_return_if_fail (wstore != NULL);
  
  if (wstore->indent >= 2)
    wstore->indent -= 2;
}

void
sfi_wstore_puts (SfiWStore   *wstore,
		 const gchar *string)
{
  g_return_if_fail (wstore != NULL);
  
  if (string)
    {
      g_string_append (wstore->text, string);
      if (string[0])
        sfi_wstore_text_changed (wstore);
    }
}

void
sfi_wstore_putc (SfiWStore *wstore,
		 gchar	    character)
{
  g_return_if_fail (wstore != NULL);
  
  g_string_append_c (wstore->text, character);
  sfi_wstore_text_changed (wstore);
}

void
sfi_wstore_printf (SfiWStore   *wstore,
		   const gchar *format,
		   ...)
{
  gchar *buffer;
  va_list args;
  
  g_return_if_fail (wstore != NULL);
  
  va_start (args, format);
  buffer = g_strdup_vprintf (format, args);
  va_end (args);
  
  g_string_append (wstore->text, buffer);
  if (buffer[0])
    sfi_wstore_text_changed (wstore);
  g_free (buffer);
}

void
sfi_wstore_put_value (SfiWStore	   *wstore,
		      const GValue *value)
{
  GString *gstring;
  
  g_return_if_fail (wstore != NULL);
  g_return_if_fail (G_IS_VALUE (value));
  
  gstring = g_string_new (NULL);
  sfi_value_store_typed (value, gstring);
  sfi_wstore_puts (wstore, gstring->str);
  g_string_free (gstring, TRUE);
}

void
sfi_wstore_put_param (SfiWStore	   *wstore,
		      const GValue *value,
		      GParamSpec   *pspec)
{
  GValue svalue = { 0, };
  GParamSpec *spspec;
  
  g_return_if_fail (wstore != NULL);
  g_return_if_fail (G_IS_VALUE (value));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  
  spspec = sfi_pspec_to_serializable (pspec);
  if (!spspec)          /* we really can't do anything here */
    g_error ("unable to (de-)serialize \"%s\" of type `%s'", pspec->name,
             g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
  
  g_value_init (&svalue, G_PARAM_SPEC_VALUE_TYPE (spspec));
  if (sfi_value_transform (value, &svalue))
    {
      GString *gstring = g_string_new (NULL);
      if (g_param_value_validate (spspec, &svalue))
	{
	  if (G_VALUE_TYPE (&svalue) != G_VALUE_TYPE (value))
	    SER_INFO ("fixing up value for \"%s\" of type `%s' (converted from `%s')",
		      pspec->name, g_type_name (G_VALUE_TYPE (&svalue)),
		      g_type_name (G_VALUE_TYPE (value)));
	  else
	    SER_INFO ("fixing up value for \"%s\" of type `%s'",
		      pspec->name, g_type_name (G_VALUE_TYPE (&svalue)));
	}
      sfi_value_store_param (&svalue, gstring, spspec, wstore->indent);
      sfi_wstore_break (wstore);
      sfi_wstore_puts (wstore, gstring->str);
      g_string_free (gstring, TRUE);
    }
  else
    g_warning ("unable to transform \"%s\" of type `%s' to `%s'",
	       pspec->name, g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
	       g_type_name (G_VALUE_TYPE (&svalue)));
  g_value_unset (&svalue);
  g_param_spec_unref (spspec);
}

void
sfi_wstore_put_binary (SfiWStore      *wstore,
		       SfiStoreReadBin reader,
		       gpointer	       data,
		       GDestroyNotify  destroy)
{
  BBlock *bblock;
  
  g_return_if_fail (wstore != NULL);
  g_return_if_fail (wstore->flushed == FALSE);
  g_return_if_fail (reader != NULL);
  
  bblock = g_new0 (BBlock, 1);
  bblock->reader = reader;
  bblock->data = data;
  bblock->destroy = destroy;
  wstore->bblocks = sfi_ring_append (wstore->bblocks, bblock);
  
  sfi_wstore_puts (wstore, "(binary-appendix ");
  bblock->patch_offset = wstore->text->len;
  sfi_wstore_puts (wstore, "0x00000000 0x00000000)");
}

const gchar*
sfi_wstore_peek_text (SfiWStore      *wstore,
                      guint          *length_p)
{
  g_return_val_if_fail (wstore != NULL, NULL);

  if (length_p)
    *length_p = wstore->text->len;

  return wstore->text->str;
}

void
sfi_wstore_flush_fd (SfiWStore *wstore,
		     gint      fd)
{
  guint8 buffer[8192] = { 0, };
  const guint bsize = sizeof (buffer);
  SfiRing *ring;
  off_t text_offset, binary_offset;
  guint l;
  
  g_return_if_fail (wstore != NULL);
  g_return_if_fail (wstore->flushed == FALSE);
  g_return_if_fail (fd >= 0);

  wstore->flushed = TRUE;

  sfi_wstore_break (wstore);
  
  /* save text offset */
  do
    text_offset = lseek (fd, 0, SEEK_CUR);
  while (text_offset < 0 && errno == EINTR);
  
  /* dump text */
  do
    l = write (fd, wstore->text->str, wstore->text->len);
  while (l < 0 && errno == EINTR);
  
  /* binary data header */
  if (wstore->bblocks)
    {
      gchar term[] = "\nX binary appendix:\n";
      guint n = strlen (term) + 1;
      term[1] = wstore->comment_start;
      do
	l = write (fd, term, n);
      while (l < 0 && errno == EINTR);
    }
  
  /* save binary offset */
  do
    binary_offset = lseek (fd, 0, SEEK_CUR);
  while (binary_offset < 0 && errno == EINTR);
  /* binary_offset is position of the first byte *after* \000 */

  /* store binary data */
  for (ring = wstore->bblocks; ring; ring = sfi_ring_walk (ring, wstore->bblocks))
    {
      BBlock *bblock = ring->data;
      gint n;
      
      /* save block offset */
      do
	bblock->offset = lseek (fd, 0, SEEK_CUR);
      while (bblock->offset < 0 && errno == EINTR);
      bblock->length = 0;
      
      /* dump binary */
      do
	{
	  n = bblock->reader (bblock->data, bblock->length, buffer, bsize);
	  if (n < 0)
	    break;	// FIXME: error handling
	  g_assert (n <= bsize);
	  do
	    l = write (fd, buffer, n);
	  while (l < 0 && errno == EINTR);
	  bblock->length += n;
	}
      while (n);
    }
  
  /* patch binary offsets and lengths */
  for (ring = wstore->bblocks; ring; ring = sfi_ring_walk (ring, wstore->bblocks))
    {
      BBlock *bblock = ring->data;
      off_t foff;
      gchar ptext[2 + 8 + 1 + 2 + 8 + 1];
      /*          0x *0* ' '  0x *0* '\0' */
      
      do
	foff = lseek (fd, text_offset + bblock->patch_offset, SEEK_SET);
      while (foff < 0 && errno == EINTR);
      g_snprintf (ptext, sizeof (ptext), "0x%08x 0x%08x",
		  (guint32) (bblock->offset - binary_offset),
		  (guint32) bblock->length);
      do
	l = write (fd, ptext, sizeof (ptext) - 1);
      while (l < 0 && errno == EINTR);
    }
}


/* --- readable store --- */
SfiRStore*
sfi_rstore_new (void)
{
  SfiRStore *rstore;

  rstore = g_new0 (SfiRStore, 1);
  rstore->fd = -1;
  rstore->scanner = g_scanner_new (sfi_storage_scanner_config);
  rstore->scanner->max_parse_errors = 1;
  rstore->fname = NULL;
  rstore->parser_this = rstore;
  rstore->bin_offset = -1;

  return rstore;
}

void
sfi_rstore_destroy (SfiRStore *rstore)
{
  g_return_if_fail (rstore != NULL);

  g_scanner_destroy (rstore->scanner);
  g_free (rstore->fname);
  g_free (rstore);
}

void
sfi_rstore_input_fd (SfiRStore   *rstore,
		     gint         fd,
		     const gchar *fname)
{
  g_return_if_fail (rstore != NULL);
  g_return_if_fail (fd >= 0);

  g_free (rstore->fname);
  rstore->fname = g_strdup (fname ? fname : "<memory>");
  rstore->scanner->input_name = rstore->fname;
  rstore->scanner->parse_errors = 0;
  rstore->fd = fd;
  g_scanner_input_file (rstore->scanner, fd);
}

void
sfi_rstore_input_text (SfiRStore   *rstore,
		       const gchar *text)
{
  g_return_if_fail (rstore != NULL);
  g_return_if_fail (text != NULL);

  g_free (rstore->fname);
  rstore->fname = g_strdup ("<text>");
  rstore->scanner->input_name = rstore->fname;
  rstore->scanner->parse_errors = 0;
  g_scanner_input_text (rstore->scanner, text, strlen (text));
}

gboolean
sfi_rstore_eof (SfiRStore *rstore)
{
  GScanner *scanner;

  g_return_val_if_fail (rstore != NULL, TRUE);

  scanner = rstore->scanner;

  return g_scanner_eof (scanner) || scanner->parse_errors >= scanner->max_parse_errors;
}

void
sfi_rstore_error (SfiRStore   *rstore,
		  const gchar *format,
		  ...)
{
  va_list args;

  g_return_if_fail (rstore);
  g_return_if_fail (format != NULL);

  va_start (args, format);
  if (rstore->scanner->parse_errors < rstore->scanner->max_parse_errors)
    {
      gchar *string = g_strdup_vprintf (format, args);
      g_scanner_error (rstore->scanner, "%s", string);
      g_free (string);
    }
  va_end (args);
}

void
sfi_rstore_unexp_token (SfiRStore *rstore,
			GTokenType expected_token)
{
  GScanner *scanner;

  g_return_if_fail (rstore);

  scanner = rstore->scanner;
  if (scanner->parse_errors < scanner->max_parse_errors)
    {
      gchar *message;

      if (scanner->parse_errors + 1 >= scanner->max_parse_errors)
        message = "aborting...";
      else
        message = NULL;
      g_scanner_unexp_token (scanner, expected_token, NULL, NULL, NULL, message, TRUE);
    }
}

void
sfi_rstore_warn (SfiRStore   *rstore,
		 const gchar *format,
		 ...)
{
  va_list args;

  g_return_if_fail (rstore);
  g_return_if_fail (format != NULL);

  va_start (args, format);
  if (rstore->scanner->parse_errors < rstore->scanner->max_parse_errors)
    {
      gchar *string = g_strdup_vprintf (format, args);
      g_scanner_warn (rstore->scanner, "%s", string);
      g_free (string);
    }
  va_end (args);
}

static GTokenType
scanner_skip_statement (GScanner *scanner,
			guint     level) /* == number of closing parens left to read */
{
  g_return_val_if_fail (scanner != NULL, G_TOKEN_ERROR);
  g_return_val_if_fail (level > 0, G_TOKEN_ERROR);

  do
    {
      g_scanner_get_next_token (scanner);
      switch (scanner->token)
	{
	case G_TOKEN_EOF:
	case G_TOKEN_ERROR:
	  return ')';
	case '(':
	  level++;
	  break;
	case ')':
	  level--;
	  break;
	default:
	  break;
	}
    }
  while (level);
  return G_TOKEN_NONE;
}

GTokenType
sfi_rstore_warn_skip (SfiRStore   *rstore,
		      const gchar *format,
		      ...)
{
  va_list args;

  g_return_val_if_fail (rstore, G_TOKEN_ERROR);
  g_return_val_if_fail (format != NULL, G_TOKEN_ERROR);

  va_start (args, format);
  if (rstore->scanner->parse_errors < rstore->scanner->max_parse_errors)
    {
      gchar *string = g_strdup_vprintf (format, args);
      /* construct warning *before* modifying scanner state */
      g_scanner_warn (rstore->scanner, "%s - skipping...", string);
      g_free (string);
    }
  va_end (args);

  return scanner_skip_statement (rstore->scanner, 1);
}

GTokenType
sfi_rstore_parse_param (SfiRStore  *rstore,
			GValue     *value,
			GParamSpec *pspec)
{
  GParamSpec *spspec;
  GValue pvalue = { 0, };
  GTokenType token;

  g_return_val_if_fail (rstore != NULL, G_TOKEN_ERROR);
  g_return_val_if_fail (G_IS_VALUE (value), G_TOKEN_ERROR);
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), G_TOKEN_ERROR);

  spspec = sfi_pspec_to_serializable (pspec);
  if (!spspec)          /* we really can't do anything here */
    g_error ("unable to (de-)serialize \"%s\" of type `%s'", pspec->name,
             g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));

  token = sfi_value_parse_param_rest (&pvalue, rstore->scanner, spspec);
  if (token == G_TOKEN_NONE)
    {
      if (sfi_value_transform (&pvalue, value))
	{
	  if (g_param_value_validate (pspec, value))
	    {
	      if (G_VALUE_TYPE (&pvalue) != G_VALUE_TYPE (value))
		sfi_rstore_warn (rstore, "fixing up value for \"%s\" of type `%s' (converted from `%s')",
				 pspec->name, g_type_name (G_VALUE_TYPE (value)),
				 g_type_name (G_VALUE_TYPE (&pvalue)));
	      else
		sfi_rstore_warn (rstore, "fixing up value for \"%s\" of type `%s'",
				 pspec->name, g_type_name (G_VALUE_TYPE (value)));
	    }
	}
      else
	{
	  g_warning ("unable to transform \"%s\" of type `%s' to `%s'",
		     pspec->name, g_type_name (G_VALUE_TYPE (&pvalue)),
		     g_type_name (G_VALUE_TYPE (value)));
	  return G_TOKEN_ERROR;
	}
      g_value_unset (&pvalue);
    }
  g_param_spec_unref (spspec);
  return token;
}

static gboolean
rstore_ensure_bin_offset (SfiRStore *rstore)
{
  if (rstore->bin_offset < 0)
    {
      guint8 sdata[8192], *p;
      off_t sc_offset, zero_offset;
      ssize_t l;
      gboolean seen_zero = FALSE;

      /* save current scanning offset */
      g_scanner_sync_file_offset (rstore->scanner);
      g_scanner_sync_file_offset (rstore->scanner);
      do
	sc_offset = lseek (rstore->fd, 0, SEEK_CUR);
      while (sc_offset < 0 && errno == EINTR);
      if (sc_offset < 0)
	return FALSE;

      /* seek to literal '\0' */
      zero_offset = sc_offset;
      do
	{
	  do
	    l = read (rstore->fd, sdata, sizeof (sdata));
	  while (l < 0 && errno == EINTR);
	  if (l < 0)
	    return FALSE;

	  p = memchr (sdata, 0, l);
	  seen_zero = p != NULL;
	  zero_offset += seen_zero ? p - sdata : l;
	}
      while (!seen_zero && l);
      if (!seen_zero)
	return FALSE;

      /* restore scanning offset */
      rstore->bin_offset = zero_offset + 1;
      do
	l = lseek (rstore->fd, sc_offset, SEEK_SET);
      while (l < 0 && errno == EINTR);
      if (l != sc_offset)
	return FALSE;
    }
  return TRUE;
}

GTokenType
sfi_rstore_ensure_bin_offset (SfiRStore *rstore)
{
  g_return_val_if_fail (rstore != NULL, G_TOKEN_ERROR);

  if (!rstore_ensure_bin_offset (rstore))
    return G_TOKEN_ERROR;
  return G_TOKEN_NONE;
}

guint64
sfi_rstore_get_bin_offset (SfiRStore *rstore)
{
  g_return_val_if_fail (rstore != NULL, 0);
  g_return_val_if_fail (rstore->bin_offset >= 0, 0);    /* sfi_rstore_ensure_bin_offset() must be called before hand */

  return rstore->bin_offset;
}

GTokenType
sfi_rstore_parse_binary (SfiRStore *rstore,
			 SfiNum    *offset_p,
			 SfiNum    *length_p)
{
  SfiNum offset, length;

  g_return_val_if_fail (rstore != NULL, G_TOKEN_ERROR);
  g_return_val_if_fail (offset_p && length_p, G_TOKEN_ERROR);

  if (g_scanner_get_next_token (rstore->scanner) != '(')
    return '(';
  if (g_scanner_get_next_token (rstore->scanner) != G_TOKEN_IDENTIFIER ||
      strcmp (rstore->scanner->value.v_identifier, "binary-appendix") != 0)
    return G_TOKEN_IDENTIFIER;
  if (g_scanner_get_next_token (rstore->scanner) != G_TOKEN_INT)
    return G_TOKEN_INT;
  offset = rstore->scanner->value.v_int64;
  if (g_scanner_get_next_token (rstore->scanner) != G_TOKEN_INT)
    return G_TOKEN_INT;
  length = rstore->scanner->value.v_int64;
  if (g_scanner_get_next_token (rstore->scanner) != ')')
    return ')';
  if (!rstore_ensure_bin_offset (rstore))
    return G_TOKEN_ERROR;
  *offset_p = rstore->bin_offset + offset;
  *length_p = length;
  return G_TOKEN_NONE;
}

GTokenType
sfi_rstore_parse_until (SfiRStore     *rstore,
                        GTokenType     closing_token,
                        gpointer       context_data,
                        SfiStoreParser try_statement,
                        gpointer       user_data)
{
  GScanner *scanner;

  g_return_val_if_fail (rstore != NULL, G_TOKEN_ERROR);
  g_return_val_if_fail (try_statement != NULL, G_TOKEN_ERROR);
  g_return_val_if_fail (closing_token == G_TOKEN_EOF || closing_token == ')', G_TOKEN_ERROR);

  scanner = rstore->scanner;

  /* we catch all SFI_TOKEN_UNMATCHED at this level. it is merely
   * a "magic" token value to implement the try_statement() semantics
   */
  while (!sfi_rstore_eof (rstore) && g_scanner_get_next_token (scanner) == '(')
    {
      GTokenType expected_token;
      guint saved_line, saved_position;
      
      /* it is only usefull to feature statements which
       * start out with an identifier (syntactically)
       */
      if (g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER)
        {
          /* eat token and bail out */
          g_scanner_get_next_token (scanner);
          return G_TOKEN_IDENTIFIER;
        }
      /* parse a statement (may return SFI_TOKEN_UNMATCHED) */
      saved_line = scanner->line;
      saved_position = scanner->position;
      expected_token = try_statement (context_data, rstore->parser_this, scanner, user_data);
      /* if there are no matches, skip statement */
      if (expected_token == SFI_TOKEN_UNMATCHED)
        {
          if (saved_line != scanner->line || saved_position != scanner->position ||
              scanner->next_token != G_TOKEN_IDENTIFIER)
            {
              g_warning ("((SfiStoreParser)%p) advanced scanner for unmatched token", try_statement);
              return G_TOKEN_ERROR;
            }
          expected_token = sfi_rstore_warn_skip (rstore, "unknown identifier: %s", scanner->next_value.v_identifier);
        }
      /* bail out on errors */
      if (expected_token != G_TOKEN_NONE)
        return expected_token;
    }

  return scanner->token == closing_token ? G_TOKEN_NONE : closing_token;
}

guint
sfi_rstore_parse_all (SfiRStore     *rstore,
		      gpointer       context_data,
		      SfiStoreParser try_statement,
		      gpointer       user_data)
{
  GTokenType expected_token = G_TOKEN_NONE;

  g_return_val_if_fail (rstore != NULL, 1);
  g_return_val_if_fail (try_statement != NULL, 1);

  /* parse all statements */
  expected_token = sfi_rstore_parse_until (rstore, G_TOKEN_EOF, context_data, try_statement, user_data);

  /* report error if any */
  if (expected_token != G_TOKEN_NONE)
    sfi_rstore_unexp_token (rstore, expected_token);

  return rstore->scanner->parse_errors;
}
