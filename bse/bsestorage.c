/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997, 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bsestorage.h"

#include "bseitem.h"
#include "bsebindata.h"
#include "bseproject.h"
#include "bsesong.h"
#include "bsesequence.h"
#include "gsldatahandle.h"
#include "gsldatautils.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>


/* --- typedefs --- */
struct _BseStorageBBlock
{
  BseStorageBBlock *next;
  GslLong	    write_voffset;
  GslLong	    vlength;
  GslDataHandle	   *data_handle;
  guint		    bytes_per_value;
  gulong	    storage_offset;	/* in bytes */
  gulong	    storage_length;	/* in bytes */
};


/* --- macros --- */
#define parse_or_return(scanner, token) { guint _t = (token); \
                                          if (g_scanner_get_next_token (scanner) != _t) \
                                            return _t; \
                                        }
#define peek_or_return(scanner, token) { GScanner *__s = (scanner); guint _t = (token); \
                                          if (g_scanner_peek_next_token (__s) != _t) { \
                                            g_scanner_get_next_token (__s); \
                                            return _t; \
                                        } }


/* --- variables --- */
static  GScannerConfig  bse_scanner_config_template = {
  (
   " \t\r\n"
   )                    /* cset_skip_characters */,
  (
   G_CSET_a_2_z
   "_'"
   G_CSET_A_2_Z
   )                    /* cset_identifier_first */,
  (
   G_CSET_a_2_z
   ".:-+_0123456789"
   G_CSET_A_2_Z
   G_CSET_LATINS
   G_CSET_LATINC
   )                    /* cset_identifier_nth */,
  ( ";\n" )             /* cpair_comment_single */,
  
  FALSE                 /* case_sensitive */,
  
  TRUE                  /* skip_comment_multi */,
  TRUE                  /* skip_comment_single */,
  FALSE                 /* scan_comment_multi */,
  TRUE                  /* scan_identifier */,
  FALSE                 /* scan_identifier_1char */,
  FALSE                 /* scan_identifier_NULL */,
  TRUE                  /* scan_symbols */,
  TRUE                  /* scan_binary */,
  TRUE                  /* scan_octal */,
  TRUE                  /* scan_float */,
  TRUE                  /* scan_hex */,
  FALSE                 /* scan_hex_dollar */,
  TRUE                  /* scan_string_sq */,
  TRUE                  /* scan_string_dq */,
  TRUE                  /* numbers_2_int */,
  FALSE                 /* int_2_float */,
  FALSE                 /* identifier_2_string */,
  TRUE                  /* char_2_token */,
  TRUE                  /* symbol_2_token */,
  FALSE                 /* scope_0_fallback */,
};


/* --- functions --- */
BseStorage*
bse_storage_new (void)
{
  BseStorage *storage;
  
  storage = g_new0 (BseStorage, 1);
  storage->flags = 0;
  storage->indent_width = 2;
  
  storage->scanner = NULL;
  storage->fd = -1;
  storage->bin_offset = 0;
  storage->resolver = NULL;
  storage->resolver_data = NULL;
  
  storage->indent = NULL;
  storage->wblocks = NULL;
  storage->gstring = NULL;
  
  return storage;
}

BseStorage*
bse_storage_from_scanner (GScanner *scanner)
{
  gpointer storage;

  g_return_val_if_fail (scanner != NULL, NULL);
  
  storage = g_datalist_get_data (&scanner->qdata, "BseStorage");

  return BSE_IS_STORAGE (storage) ? storage : NULL;
}

void
bse_storage_destroy (BseStorage *storage)
{
  g_return_if_fail (storage != NULL);
  
  bse_storage_reset (storage);
  
  g_free (storage);
}

void
bse_storage_reset (BseStorage *storage)
{
  g_return_if_fail (BSE_IS_STORAGE (storage));
  
  if (BSE_STORAGE_READABLE (storage))
    {
      g_free ((gchar*) storage->scanner->input_name);
      g_scanner_destroy (storage->scanner);
      if (storage->fd >= 0)
        close (storage->fd);
      storage->scanner = NULL;
      storage->fd = -1;
      storage->bin_offset = 0;
      storage->resolver = NULL;
      storage->resolver_data = NULL;
    }
  
  if (BSE_STORAGE_WRITABLE (storage))
    {
      GSList *slist;
      
      for (slist = storage->indent; slist; slist = slist->next)
        g_free (slist->data);
      g_slist_free (storage->indent);
      storage->indent = NULL;
      
      while (storage->wblocks)
        {
          BseStorageBBlock *bblock = storage->wblocks;
          
          storage->wblocks = bblock->next;
	  gsl_data_handle_unref (bblock->data_handle);
          g_free (bblock);
        }
      
      g_string_free (storage->gstring, TRUE);
      storage->gstring = NULL;
    }
  
  storage->flags = 0;
}

void
bse_storage_prepare_write (BseStorage *storage,
                           gboolean    store_defaults)
{
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (!BSE_STORAGE_WRITABLE (storage));
  g_return_if_fail (!BSE_STORAGE_READABLE (storage));
  
  storage->indent = g_slist_prepend (NULL, g_strdup (""));
  storage->gstring = g_string_sized_new (1024);
  BSE_STORAGE_SET_FLAGS (storage, BSE_STORAGE_FLAG_WRITABLE);
  BSE_STORAGE_SET_FLAGS (storage, BSE_STORAGE_FLAG_AT_BOL);
  if (store_defaults)
    BSE_STORAGE_SET_FLAGS (storage, BSE_STORAGE_FLAG_PUT_DEFAULTS);
}

BseErrorType
bse_storage_input_file (BseStorage     *storage,
                        const gchar    *file_name)
{
  gint fd;
  
  g_return_val_if_fail (BSE_IS_STORAGE (storage), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (!BSE_STORAGE_WRITABLE (storage), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (!BSE_STORAGE_READABLE (storage), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (file_name != NULL, BSE_ERROR_INTERNAL);
  
  fd = open (file_name, O_RDONLY, 0);
  
  if (fd < 0)
    return (errno == ENOENT || errno == ENOTDIR || errno == ELOOP ?
            BSE_ERROR_FILE_NOT_FOUND : BSE_ERROR_FILE_IO);
  
  storage->fd = fd;
  storage->scanner = g_scanner_new (&bse_scanner_config_template);
  g_datalist_set_data (&storage->scanner->qdata, "BseStorage", storage);
  g_scanner_add_symbol (storage->scanner, "nil", GUINT_TO_POINTER (BSE_TOKEN_NIL));
  g_scanner_input_file (storage->scanner, fd);
  storage->scanner->input_name = g_strdup (file_name);
  storage->scanner->max_parse_errors = 1;
  storage->scanner->parse_errors = 0;
  
  BSE_STORAGE_SET_FLAGS (storage, BSE_STORAGE_FLAG_READABLE);
  
  return BSE_ERROR_NONE;
}

static inline const gchar*
bse_storage_get_indent (BseStorage *storage)
{
  return storage->indent->data;
}

static inline void
bse_storage_indent (BseStorage *storage)
{
  bse_storage_puts (storage, bse_storage_get_indent (storage));
}

void
bse_storage_push_level (BseStorage *storage)
{
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_WRITABLE (storage));
  
  storage->indent = g_slist_prepend (storage->indent,
                                     g_strnfill (storage->indent_width +
                                                 strlen (bse_storage_get_indent (storage)),
                                                 ' '));
}

void
bse_storage_pop_level (BseStorage *storage)
{
  GSList *next;
  
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_WRITABLE (storage));
  
  next = storage->indent->next;
  if (next)
    {
      g_free (storage->indent->data);
      g_slist_free_1 (storage->indent);
      storage->indent = next;
    }
}

void
bse_storage_puts (BseStorage  *storage,
                  const gchar *string)
{
  guint l;

  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_WRITABLE (storage));

  if (!string)
    return;
  l = strlen (string);
  if (!l)
    return;

  if (storage->gstring)
    g_string_append (storage->gstring, string);
  
  if (string[l - 1] == '\n')
    BSE_STORAGE_SET_FLAGS (storage, BSE_STORAGE_FLAG_AT_BOL);
  else
    BSE_STORAGE_UNSET_FLAGS (storage, BSE_STORAGE_FLAG_AT_BOL);
}

void
bse_storage_putc (BseStorage *storage,
                  gchar       character)
{
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_WRITABLE (storage));
  
  if (storage->gstring)
    g_string_append_c (storage->gstring, character);
  
  if (character == '\n')
    BSE_STORAGE_SET_FLAGS (storage, BSE_STORAGE_FLAG_AT_BOL);
  else
    BSE_STORAGE_UNSET_FLAGS (storage, BSE_STORAGE_FLAG_AT_BOL);
}

void
bse_storage_printf (BseStorage  *storage,
                    const gchar *format,
                    ...)
{
  gchar *buffer;
  va_list args;
  
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_WRITABLE (storage));
  g_return_if_fail (format != NULL);
  
  va_start (args, format);
  buffer = g_strdup_vprintf (format, args);
  va_end (args);
  
  bse_storage_puts (storage, buffer);
  
  g_free (buffer);
}

void
bse_storage_break (BseStorage *storage)
{
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_WRITABLE (storage));
  
  bse_storage_putc (storage, '\n');
  BSE_STORAGE_UNSET_FLAGS (storage, BSE_STORAGE_FLAG_NEEDS_BREAK);
  bse_storage_indent (storage);
}

void
bse_storage_handle_break (BseStorage *storage)
{
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_WRITABLE (storage));
  
  if (BSE_STORAGE_NEEDS_BREAK (storage))
    bse_storage_break (storage);
}

void
bse_storage_needs_break (BseStorage *storage)
{
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_WRITABLE (storage));
  
  BSE_STORAGE_SET_FLAGS (storage, BSE_STORAGE_FLAG_NEEDS_BREAK);
}

static BseStorageBBlock*
bse_storage_get_create_wblock (BseStorage    *storage,
			       guint	      bytes_per_value,
			       GslDataHandle *data_handle,
			       GslLong	      voffset,
			       GslLong	      vlength)
{
  BseStorageBBlock *bblock, *last = NULL;

  for (bblock = storage->wblocks; bblock; last = bblock, bblock = last->next)
    if (bblock->data_handle == data_handle && bblock->bytes_per_value == bytes_per_value &&
	bblock->write_voffset == voffset && bblock->vlength == vlength)
      return bblock;

  /* create */
  bblock = g_new0 (BseStorageBBlock, 1);
  bblock->write_voffset = voffset;
  bblock->vlength = vlength;
  bblock->data_handle = gsl_data_handle_ref (data_handle);
  bblock->bytes_per_value = bytes_per_value;
  bblock->storage_offset = last ? last->storage_offset + last->storage_length : 0;
  bblock->storage_length = vlength * bblock->bytes_per_value;
  /* align to 4 bytes */
  bblock->storage_length += 4 - 1;
  bblock->storage_length /= 4;
  bblock->storage_length *= 4;
  if (last)
    last->next = bblock;
  else
    storage->wblocks = bblock;
  
  return bblock;
}

void
bse_storage_put_wave_handle (BseStorage    *storage,
			     guint	    significant_bits,
			     GslDataHandle *data_handle,
			     GslLong        voffset,
			     GslLong        vlength)
{
  BseStorageBBlock *bblock;
  guint bytes_per_value;

  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_WRITABLE (storage));
  g_return_if_fail (data_handle != NULL);
  g_return_if_fail (voffset >= 0 && vlength > 0);
  g_return_if_fail (voffset < data_handle->n_values);
  g_return_if_fail (voffset + vlength <= data_handle->n_values);

  if (significant_bits <= 8)
    bytes_per_value = 1;
  else if (significant_bits <= 16)
    bytes_per_value = 2;
  else
    bytes_per_value = 4;

  bblock = bse_storage_get_create_wblock (storage, bytes_per_value, data_handle, voffset, vlength);

  bse_storage_handle_break (storage);
  bse_storage_printf (storage,
                      "(BseStorageBinaryV0 %lu %c:%u %lu %lu)",
                      bblock->storage_offset,
                      G_BYTE_ORDER == G_LITTLE_ENDIAN ? 'L' : 'B',
                      bblock->bytes_per_value,
                      bblock->storage_length,
		      bblock->vlength);
}

void
bse_storage_flush_fd (BseStorage *storage,
                      gint        fd)
{
  gint l;
  
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_WRITABLE (storage));
  g_return_if_fail (fd >= 0);
  
  /* dump text storage
   */
  do
    l = write (fd, storage->gstring->str, storage->gstring->len);
  while (l < 0 && errno == EINTR);
  
  do
    l = write (fd, "\n", 1);
  while (l < 0 && errno == EINTR);
  
  /* dump binary data
   */
  if (storage->wblocks)
    {
      BseStorageBBlock *bblock;
      gchar term[] = "\n; binary appendix:\n";
      guint n = strlen (term) + 1;
      
      do
        l = write (fd, term, n);
      while (l < 0 && errno == EINTR);

      for (bblock = storage->wblocks; bblock; bblock = bblock->next)
	{
	  GslLong vlength = bblock->vlength;
	  GslLong voffset = bblock->write_voffset, pad;

	  while (vlength > 0)
	    {
	      gfloat fbuffer[8192];
	      GslLong l = MIN (8192, vlength);
	      guint n_retries = 4;
	      gssize s;
	      
	      do
		l = gsl_data_handle_read (bblock->data_handle, voffset, l, fbuffer);
	      while (l < 1 && n_retries--);
	      if (l < 1)
		{
		  g_warning ("failed to retrive data for storage"); // FIXME
		  l = MIN (8192, vlength);
		  memset (fbuffer, 0, l * sizeof (fbuffer[0]));
		}
	      voffset += l;
	      vlength -= l;
	      if (bblock->bytes_per_value == 1)
		{
		  gsl_conv_from_float_clip (GSL_WAVE_FORMAT_SIGNED_8, G_BYTE_ORDER,
					    fbuffer, fbuffer, l);
		}
	      else if (bblock->bytes_per_value == 2)
		{
                  gsl_conv_from_float_clip (GSL_WAVE_FORMAT_SIGNED_16, G_BYTE_ORDER,
                                            fbuffer, fbuffer, l);
		  l *= 2;
		}
	      else
		l *= 4;
	      do
		s = write (fd, fbuffer, l);
	      while (s < 0 && errno == EINTR);
	    }
	  pad = bblock->storage_length - bblock->vlength * bblock->bytes_per_value;
	  while (pad > 0)
            {
              guint8 buffer[1024] = { 0, };
              guint n = MIN (pad, 1024);
              
              do
                l = write (fd, buffer, n);
              while (l < 0 && errno == EINTR);
              pad -= n;
            }
        }
    }
}

void
bse_storage_set_path_resolver (BseStorage     *storage,
                               BsePathResolver resolver,
                               gpointer        func_data)
{
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_READABLE (storage));
  
  storage->resolver = resolver;
  storage->resolver_data = func_data;
}

GTokenType
bse_storage_skip_statement (BseStorage *storage)
{
  GScanner *scanner;
  guint level = 1;
  
  g_return_val_if_fail (BSE_IS_STORAGE (storage), G_TOKEN_ERROR);
  g_return_val_if_fail (BSE_STORAGE_READABLE (storage), G_TOKEN_ERROR);
  
  scanner = storage->scanner;
  
 loop:
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
  
  if (level)
    {
      g_scanner_get_next_token (scanner);
      goto loop;
    }
  
  return G_TOKEN_NONE;
}

GTokenType
bse_storage_parse_rest (BseStorage     *storage,
                        BseTryStatement try_statement,
                        gpointer        func_data,
                        gpointer        user_data)
{
  GScanner *scanner;
  
  g_return_val_if_fail (BSE_IS_STORAGE (storage), G_TOKEN_ERROR);
  g_return_val_if_fail (BSE_STORAGE_READABLE (storage), G_TOKEN_ERROR);
  
  scanner = storage->scanner;
  
  /* we catch any BSE_TOKEN_UNMATCHED at this level, this is merely
   * a "magic" token value to implement the try_statement() semantics
   */
  while (!bse_storage_input_eof (storage))
    {
      g_scanner_get_next_token (scanner);
      
      if (scanner->token == '(')
        {
          GTokenType expected_token;
          
          /* it is only usefull to feature statements that start
           * out with an identifier (syntactically)
           */
          if (g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER)
            {
              /* eat token and bail out */
              g_scanner_get_next_token (scanner);
              return G_TOKEN_IDENTIFIER;
            }
          
          /* parse a statement */
          if (try_statement)
            expected_token = try_statement (func_data, storage, user_data);
          else
            expected_token = BSE_TOKEN_UNMATCHED;
          
          /* if there are no matches, skip statement */
          if (expected_token == BSE_TOKEN_UNMATCHED)
            {
              if (g_scanner_get_next_token (scanner) != G_TOKEN_IDENTIFIER)
                {
                  g_warning (G_STRLOC ": try_statement() implementation <%p> is broken", try_statement);
                  return G_TOKEN_ERROR;
                }
              expected_token = bse_storage_warn_skip (storage,
                                                      "unable to handle identifier \"%s\"",
                                                      scanner->value.v_identifier);
            }
          
          /* bail out on errors */
          if (expected_token != G_TOKEN_NONE)
            return expected_token;
        }
      else if (scanner->token == ')')
        return G_TOKEN_NONE;
      else
        break;
    }
  
  return ')';
}

gboolean
bse_storage_input_eof (BseStorage *storage)
{
  g_return_val_if_fail (BSE_IS_STORAGE (storage), FALSE);
  g_return_val_if_fail (BSE_STORAGE_READABLE (storage), FALSE);
  
  return (g_scanner_eof (storage->scanner) ||
          storage->scanner->parse_errors >= storage->scanner->max_parse_errors);
}

void
bse_storage_warn (BseStorage  *storage,
                  const gchar *format,
                  ...)
{
  va_list args;
  gchar *string;
  
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_READABLE (storage));
  
  va_start (args, format);
  string = g_strdup_vprintf (format, args);
  va_end (args);
  
  g_scanner_warn (storage->scanner, "%s", string);
  
  g_free (string);
}

GTokenType
bse_storage_warn_skip (BseStorage  *storage,
                       const gchar *format,
                       ...)
{
  va_list args;
  gchar *string;
  
  g_return_val_if_fail (BSE_IS_STORAGE (storage), G_TOKEN_ERROR);
  g_return_val_if_fail (BSE_STORAGE_READABLE (storage), G_TOKEN_ERROR);
  
  va_start (args, format);
  string = g_strdup_vprintf (format, args);
  va_end (args);
  
  g_scanner_warn (storage->scanner, "%s - skipping...", string);
  
  g_free (string);
  
  return bse_storage_skip_statement (storage);
}

void
bse_storage_error (BseStorage  *storage,
                   const gchar *format,
                   ...)
{
  va_list args;
  gchar *string;
  
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_READABLE (storage));
  
  va_start (args, format);
  string = g_strdup_vprintf (format, args);
  va_end (args);
  
  g_scanner_error (storage->scanner, "%s", string);
  
  g_free (string);
}

void
bse_storage_unexp_token (BseStorage *storage,
                         GTokenType  expected_token)
{
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_READABLE (storage));
  
  if (storage->scanner->parse_errors < storage->scanner->max_parse_errors)
    {
      gchar *message;
      
      if (storage->scanner->parse_errors + 1 >= storage->scanner->max_parse_errors)
        message = "aborting...";
      else
        message = NULL;
      g_scanner_unexp_token (storage->scanner, expected_token, NULL, NULL, NULL, message, TRUE);
    }
}

static BseErrorType
bse_storage_restore_offset (BseStorage *storage,
                            glong       offset)
{
  gint fd = storage->fd;
  
  if (fd < 0)
    return BSE_ERROR_FILE_NOT_FOUND;
  else if (lseek (fd, offset, SEEK_SET) != offset)
    return BSE_ERROR_FILE_IO;
  else
    return BSE_ERROR_NONE;
}

static BseErrorType
bse_storage_ensure_bin_offset (BseStorage *storage,
                               glong      *cur_offs_p)
{
  gint fd = storage->fd;
  glong coffs;
  
  if (fd < 0)
    return BSE_ERROR_FILE_NOT_FOUND;
  
  coffs = lseek (fd, 0, SEEK_CUR);
  if (coffs < 0)
    return BSE_ERROR_FILE_IO;
  
  if (!storage->bin_offset)
    {
      glong bin_offset, l;
      gboolean seen_zero = FALSE;
      
      bin_offset = lseek (fd, 0, SEEK_SET);
      if (bin_offset != 0)
        return BSE_ERROR_FILE_IO;
      
      do
        {
          guint8 data[4096];
          guint i;
          
          do
            l = read (fd, data, 4096);
          while (l < 0 && errno == EINTR);
          
          if (l < 0)
            return BSE_ERROR_FILE_IO;
          
          for (i = 0; i < l; i++)
            if (data[i] == 0)
              {
                seen_zero = TRUE;
                break;
              }
          bin_offset += seen_zero ? i : l;
        }
      while (!seen_zero && l);
      
      if (seen_zero)
        storage->bin_offset = bin_offset;
      else
        return BSE_ERROR_FILE_IO;
    }
  
  if (lseek (fd, coffs, SEEK_SET) != coffs)
    return BSE_ERROR_FILE_IO;
  
  if (cur_offs_p)
    *cur_offs_p = coffs;
  
  return BSE_ERROR_NONE;
}

GTokenType
bse_storage_parse_wave_handle (BseStorage     *storage,
			       GslDataHandle **data_handle_p)
{
  GScanner *scanner;
  BseStorageBBlock bblock = { 0, };
  BseEndianType byte_order = BSE_LITTLE_ENDIAN;
  BseErrorType error;
  gchar *string;

  if (data_handle_p) *data_handle_p = NULL;
  g_return_val_if_fail (BSE_IS_STORAGE (storage), G_TOKEN_ERROR);
  g_return_val_if_fail (BSE_STORAGE_READABLE (storage), G_TOKEN_ERROR);

  /*
    bse_storage_printf (storage,
    "(BseStorageBinaryV0 %lu %c:%u %lu %lu)",
    bblock->storage_offset,
    G_BYTE_ORDER == G_LITTLE_ENDIAN ? 'L' : 'B',
    bblock->bytes_per_value,
    bblock->storage_length,
    bblock->storage_vlength);
  */

  scanner = storage->scanner;

  parse_or_return (scanner, '(');
  parse_or_return (scanner, G_TOKEN_IDENTIFIER);
  if (!bse_string_equals ("BseStorageBinaryV0", scanner->value.v_identifier))
    return G_TOKEN_IDENTIFIER;
  parse_or_return (scanner, G_TOKEN_INT);
  bblock.storage_offset = scanner->value.v_int;

  parse_or_return (scanner, G_TOKEN_IDENTIFIER);
  string = scanner->value.v_identifier;
  if (string[0] == 'L' || string[0] == 'l')
    byte_order = BSE_LITTLE_ENDIAN;
  else if (string[0] == 'B' || string[0] == 'b')
    byte_order = BSE_BIG_ENDIAN;
  else
    string = NULL;
  if (string && string[1] != ':')
    string = NULL;
  if (string)
    {
      gchar *f = NULL;
      
      bblock.bytes_per_value = strtol (string + 2, &f, 10);
      if ((bblock.bytes_per_value != 1 && bblock.bytes_per_value != 2 &&
           bblock.bytes_per_value != 4) ||
          (f && *f != 0))
        string = NULL;
    }
  if (!string)
    return bse_storage_warn_skip (storage,
                                  "unknown value type `%s' in binary data definition",
                                  scanner->value.v_identifier);
  
  parse_or_return (scanner, G_TOKEN_INT);
  bblock.storage_length = scanner->value.v_int;
  if (bblock.storage_length < bblock.bytes_per_value)
    return G_TOKEN_INT;
  
  if (g_scanner_peek_next_token (scanner) == G_TOKEN_INT)
    {
      g_scanner_get_next_token (scanner);
      bblock.vlength = scanner->value.v_int;
      if (bblock.vlength < 1 || bblock.vlength * bblock.bytes_per_value > bblock.storage_length)
	return G_TOKEN_INT;
    }
  else
    bblock.vlength = bblock.storage_length / bblock.bytes_per_value;
  
  parse_or_return (scanner, ')');
  
  /* except for BSE_ERROR_FILE_NOT_FOUND, all errors are fatal ones,
   * we can't guarantee that further parsing is possible.
   */
  error = bse_storage_ensure_bin_offset (storage, NULL);
  if (error)
    {
      if (error == BSE_ERROR_FILE_NOT_FOUND)
	bse_storage_warn (storage, "no device to retrive binary data from");
      else
	bse_storage_error (storage, "failed to retrive binary data: %s", bse_error_blurb (error));
      return G_TOKEN_NONE;
    }

  if (data_handle_p)
    *data_handle_p = gsl_wave_handle_new (storage->scanner->input_name, 0,
					  bblock.bytes_per_value == 1 ? GSL_WAVE_FORMAT_SIGNED_8 :
					  bblock.bytes_per_value == 2 ? GSL_WAVE_FORMAT_SIGNED_16 :
					  GSL_WAVE_FORMAT_FLOAT,
					  byte_order,
					  storage->bin_offset + 1 + bblock.storage_offset,
					  bblock.vlength);
  
  return G_TOKEN_NONE;
}

GTokenType
bse_storage_parse_note (BseStorage *storage,
                        gint       *note_p,
                        gchar       bbuffer[BSE_BBUFFER_SIZE])
{
  GScanner *scanner;
  gint note;
  gchar ibuffer[BSE_BBUFFER_SIZE];
  
  if (note_p)
    *note_p = BSE_NOTE_UNPARSABLE;
  g_return_val_if_fail (BSE_IS_STORAGE (storage), G_TOKEN_ERROR);
  g_return_val_if_fail (BSE_STORAGE_READABLE (storage), G_TOKEN_ERROR);
  
  if (!bbuffer)
    bbuffer = ibuffer;
  
  scanner = storage->scanner;
  
  g_scanner_get_next_token (scanner);
  if (scanner->token == G_TOKEN_IDENTIFIER)
    {
      if (scanner->value.v_identifier[0] == '\'')
        bse_bbuffer_puts (bbuffer, scanner->value.v_identifier + 1);
      else
        bse_bbuffer_puts (bbuffer, scanner->value.v_identifier);
    }
  else if ((scanner->token >= 'A' && scanner->token <= 'Z') ||
           (scanner->token >= 'a' && scanner->token <= 'z'))
    bse_bbuffer_putc (bbuffer, scanner->token);
  else
    return G_TOKEN_IDENTIFIER;
  
  note = bse_note_from_string (bbuffer);
  
  if (note_p)
    *note_p = note;
  
  return G_TOKEN_NONE;
}

void
bse_storage_put_param (BseStorage *storage,
                       GValue     *value,
                       GParamSpec *pspec)
{
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_WRITABLE (storage));
  g_return_if_fail (G_IS_VALUE (value));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  
  bse_storage_handle_break (storage);
  
  bse_storage_putc (storage, '(');
  bse_storage_puts (storage, pspec->name);
  bse_storage_putc (storage, ' ');
  
  switch (G_TYPE_FUNDAMENTAL (G_PARAM_SPEC_VALUE_TYPE (pspec)))
    {
      gchar *string;
      GEnumValue *ev;
      GFlagsValue *fv;
      
    case G_TYPE_BOOLEAN:
      bse_storage_puts (storage, g_value_get_boolean (value) ? "'t" : "'f");
      break;
    case G_TYPE_INT:
      bse_storage_printf (storage, "%d", g_value_get_int (value));
      break;
    case G_TYPE_UINT:
      bse_storage_printf (storage, "%u", g_value_get_uint (value));
      break;
    case G_TYPE_ENUM:
      ev = g_enum_get_value (G_PARAM_SPEC_ENUM (pspec)->enum_class, g_value_get_enum (value));
      if (ev)
        bse_storage_puts (storage, ev->value_name);
      else
        bse_storage_printf (storage, "%d", g_value_get_enum (value));
      break;
    case G_TYPE_FLAGS:
      fv = g_flags_get_first_value (G_PARAM_SPEC_FLAGS (pspec)->flags_class, g_value_get_flags (value));
      if (fv)
        {
          guint v_flags;
          guint i = 0;
          
          v_flags = g_value_get_flags (value) & ~fv->value;
          while (fv)
            {
              v_flags &= ~fv->value;
              
              if (i++)
                bse_storage_putc (storage, ' ');
              
              bse_storage_puts (storage, fv->value_name);
              
              fv = g_flags_get_first_value (G_PARAM_SPEC_FLAGS (pspec)->flags_class, v_flags);
            }
          if (v_flags)
            bse_storage_printf (storage, " %u", v_flags);
        }
      else
        bse_storage_printf (storage, "%u", g_value_get_flags (value));
      break;
    case G_TYPE_FLOAT:
      bse_storage_printf (storage, "%f", g_value_get_float (value));
      break;
    case G_TYPE_DOUBLE:
      bse_storage_printf (storage, "%e", g_value_get_double (value));
      break;
    case G_TYPE_STRING:
      if (g_value_get_string (value))
        {
          string = g_strdup_quoted (g_value_get_string (value));
          bse_storage_putc (storage, '"');
          bse_storage_puts (storage, string);
          bse_storage_putc (storage, '"');
          g_free (string);
        }
      else
        bse_storage_puts (storage, "nil");
      break;
    case G_TYPE_OBJECT:
      if (BSE_IS_ITEM (g_value_get_object (value)))
        {
          BseProject *project = bse_item_get_project (BSE_ITEM (g_value_get_object (value)));
          gchar *path = bse_container_make_item_path (BSE_CONTAINER (project),
                                                      BSE_ITEM (g_value_get_object (value)),
                                                      TRUE);
          
          bse_storage_printf (storage, "(%s)", path);
          g_free (path);
        }
      else
        bse_storage_puts (storage, "nil");
      break;
    case BSE_TYPE_TIME:
      string = bse_time_to_str (bse_time_to_gmt (bse_value_get_time (value)));
      bse_storage_putc (storage, '"');
      bse_storage_puts (storage, string);
      bse_storage_putc (storage, '"');
      g_free (string);
      break;
    case BSE_TYPE_NOTE:
      string = bse_note_to_string (bse_value_get_note (value));
      bse_storage_putc (storage, '\'');
      bse_storage_puts (storage, string);
      g_free (string);
      break;
    case G_TYPE_BOXED:
      if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (pspec), BSE_TYPE_SEQUENCE))
	{
	  BseSequence *sdata = g_value_get_boxed (value);
	  guint i;

	  if (sdata->offset != BSE_KAMMER_NOTE)
	    g_warning ("%s: unable to handle detuned (%u!=%u) note sequence",
		       G_STRLOC, sdata->offset, BSE_KAMMER_NOTE);

	  bse_storage_printf (storage, "%u", sdata->n_notes);
	  bse_storage_push_level (storage);
	  for (i = 0; i < sdata->n_notes; i++)
	    {
	      if (i % 4)
		bse_storage_putc (storage, ' ');
	      else
		bse_storage_break (storage);
	      string = bse_note_to_string (sdata->notes[i].note);
	      bse_storage_printf (storage, "(%s)", string);
	      g_free (string);
	    }
	  bse_storage_pop_level (storage);
	  break;
	}
      /* fall through */
    default:
      bse_storage_putc (storage, '?');
      g_warning (G_STRLOC ": unhandled parameter \"%s\" type `%s'",
                 pspec->name,
                 g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
      break;
    }
  
  bse_storage_putc (storage, ')');
}

static GTokenType
parse_note (BseStorage *storage,
	    gboolean    maybe_void,
	    gint       *v_note)
{
  GScanner *scanner = storage->scanner;
  gchar *string, buffer[2];
  
  g_scanner_get_next_token (scanner);
  if (scanner->token == G_TOKEN_IDENTIFIER)
    {
      string = scanner->value.v_identifier;
      if (string[0] == '\'')
	string++;
    }
  else if ((scanner->token >= 'A' && scanner->token <= 'Z') ||
	   (scanner->token >= 'a' && scanner->token <= 'z'))
    {
      buffer[0] = scanner->token;
      buffer[1] = 0;
      string = buffer;
    }
  else
    return G_TOKEN_IDENTIFIER;

  *v_note = bse_note_from_string (string);

  if (*v_note == BSE_NOTE_UNPARSABLE || (*v_note == BSE_NOTE_VOID && !maybe_void))
    {
      *v_note = maybe_void ? BSE_NOTE_VOID : BSE_KAMMER_NOTE;
      return bse_storage_warn_skip (storage,
				    "invalid note definition `%s'",
				    string);
    }

  return G_TOKEN_NONE;
}

GTokenType
bse_storage_parse_param_value (BseStorage *storage,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GScanner *scanner;
  
  g_return_val_if_fail (BSE_IS_STORAGE (storage), G_TOKEN_ERROR);
  g_return_val_if_fail (BSE_STORAGE_READABLE (storage), G_TOKEN_ERROR);
  g_return_val_if_fail (G_IS_VALUE (value), G_TOKEN_ERROR);
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), G_TOKEN_ERROR);

  scanner = storage->scanner;
  
  switch (G_TYPE_FUNDAMENTAL (G_PARAM_SPEC_VALUE_TYPE (pspec)))
    {
      gboolean v_bool;
      gint v_enum;
      guint v_flags;
      gint v_note;
      gdouble v_double;
      GTokenType token;
      gchar *string;
      
    case G_TYPE_BOOLEAN:
      g_scanner_get_next_token (scanner);
      v_bool = FALSE;
      if (scanner->token == G_TOKEN_INT &&
          scanner->value.v_int <= 1)
        v_bool = scanner->value.v_int != 0;
      else if (scanner->token == G_TOKEN_IDENTIFIER)
        {
          if (scanner->value.v_identifier[0] == '\'' &&
              scanner->value.v_identifier[1] != 0)
            v_bool = (scanner->value.v_identifier[1] == 't' ||
                      scanner->value.v_identifier[1] == 'T' ||
                      scanner->value.v_identifier[1] == 'j' ||
                      scanner->value.v_identifier[1] == 'J' ||
                      scanner->value.v_identifier[1] == 'y' ||
                      scanner->value.v_identifier[1] == 'Y');
          else
            return bse_storage_warn_skip (storage,
                                          "unrecognized boolean value `%s' for parameter `%s'",
                                          scanner->value.v_identifier,
                                          pspec->name);
        }
      else
        return G_TOKEN_IDENTIFIER;
      g_value_set_boolean (value, v_bool);
      break;
    case G_TYPE_UINT:
      g_scanner_get_next_token (scanner);
      if (scanner->token != G_TOKEN_INT)
        return G_TOKEN_INT;
      else
	{
	  g_value_set_uint (value, scanner->value.v_int);
	  if (g_param_value_validate (pspec, value))
	    return bse_storage_warn_skip (storage,
					  "parameter value `%lu' out of bounds for parameter `%s'",
					  scanner->value.v_int,
					  pspec->name);
	}
      break;
    case G_TYPE_INT:
      g_scanner_get_next_token (scanner);
      v_bool = FALSE;
      if (scanner->token == '-')
        {
          v_bool = !v_bool;
          g_scanner_get_next_token (scanner);
        }
      if (scanner->token != G_TOKEN_INT)
        return G_TOKEN_INT;
      else
	{
	  g_value_set_int (value, v_bool ? - scanner->value.v_int : scanner->value.v_int);
          if (g_param_value_validate (pspec, value))
	    return bse_storage_warn_skip (storage,
					  "parameter value `%ld' out of bounds for parameter `%s'",
					  v_bool ? - scanner->value.v_int : scanner->value.v_int,
					  pspec->name);
	}
      break;
    case G_TYPE_ENUM:
      g_scanner_get_next_token (scanner);
      v_bool = FALSE;
      if (scanner->token == '-')
        {
          v_bool = !v_bool;
          g_scanner_get_next_token (scanner);
        }
      if (scanner->token == G_TOKEN_INT)
        {
          v_enum = v_bool ? - scanner->value.v_int : scanner->value.v_int;
        }
      else if (v_bool)
        return G_TOKEN_INT;
      else if (scanner->token == G_TOKEN_IDENTIFIER)
        {
          GEnumValue *ev;
          
          ev = g_enum_get_value_by_nick (G_PARAM_SPEC_ENUM (pspec)->enum_class,
                                         scanner->value.v_identifier);
          if (!ev)
            ev = g_enum_get_value_by_name (G_PARAM_SPEC_ENUM (pspec)->enum_class,
                                           scanner->value.v_identifier);
          if (ev)
            v_enum = ev->value;
          else
            return bse_storage_warn_skip (storage,
                                          "unrecognized enum value `%s' for parameter `%s'",
                                          scanner->value.v_identifier,
                                          pspec->name);
        }
      else
        return G_TOKEN_IDENTIFIER;
      g_value_set_enum (value, v_enum);
      if (g_param_value_validate (pspec, value))
        return bse_storage_warn_skip (storage,
                                      "parameter value `%d' out of bounds for parameter `%s'",
                                      v_enum,
                                      pspec->name);
      break;
    case G_TYPE_FLAGS:
      v_flags = 0;
      do
        {
          g_scanner_get_next_token (scanner);
          if (scanner->token == G_TOKEN_INT)
            v_flags |= scanner->value.v_int;
          else if (scanner->token == G_TOKEN_IDENTIFIER)
            {
              GFlagsValue *fv;
              
              fv = g_flags_get_value_by_nick (G_PARAM_SPEC_FLAGS (pspec)->flags_class,
                                              scanner->value.v_identifier);
              if (!fv)
                g_flags_get_value_by_name (G_PARAM_SPEC_FLAGS (pspec)->flags_class,
                                           scanner->value.v_identifier);
              if (fv)
                v_flags |= fv->value;
              else
                return bse_storage_warn_skip (storage,
                                              "unrecognized flags value `%s' for parameter `%s'",
                                              scanner->value.v_identifier,
                                              pspec->name);
            }
          else
            return G_TOKEN_IDENTIFIER;
        }
      while (g_scanner_peek_next_token (scanner) != ')');
      g_value_set_flags (value, v_flags);
      if (g_param_value_validate (pspec, value))
        return bse_storage_warn_skip (storage,
                                      "parameter value `%d' out of bounds for parameter `%s'",
                                      v_flags,
                                      pspec->name);
      break;
    case G_TYPE_FLOAT:
      g_scanner_get_next_token (scanner);
      v_bool = FALSE;
      if (scanner->token == '-')
        {
          v_bool = !v_bool;
          g_scanner_get_next_token (scanner);
        }
      if (scanner->token == G_TOKEN_FLOAT)
        v_double = v_bool ? - scanner->value.v_float : scanner->value.v_float;
      else if (scanner->token == G_TOKEN_INT)
        v_double = v_bool ? - scanner->value.v_int : scanner->value.v_int;
      else
        return G_TOKEN_FLOAT;
      g_value_set_float (value, v_double);
      if (g_param_value_validate (pspec, value))
        return bse_storage_warn_skip (storage,
                                      "parameter value `%f' out of bounds for parameter `%s'",
                                      v_double,
                                      pspec->name);
      break;
    case G_TYPE_DOUBLE:
      g_scanner_get_next_token (scanner);
      v_bool = FALSE;
      if (scanner->token == '-')
        {
          v_bool = !v_bool;
          g_scanner_get_next_token (scanner);
        }
      if (scanner->token == G_TOKEN_FLOAT)
        v_double = v_bool ? - scanner->value.v_float : scanner->value.v_float;
      else if (scanner->token == G_TOKEN_INT)
        v_double = v_bool ? - scanner->value.v_int : scanner->value.v_int;
      else
        return G_TOKEN_FLOAT;
      g_value_set_double (value, v_double);
      if (g_param_value_validate (pspec, value))
        return bse_storage_warn_skip (storage,
                                      "parameter value `%f' out of bounds for parameter `%s'",
                                      v_double,
                                      pspec->name);
      break;
    case G_TYPE_STRING:
      g_scanner_get_next_token (scanner);
      if (scanner->token == G_TOKEN_STRING)
        g_value_set_string (value, scanner->value.v_string);
      else if (scanner->token == BSE_TOKEN_NIL)
        g_value_set_string (value, NULL); /* FIXME: check validity and free param */
      else
        return G_TOKEN_STRING;
      break;
    case G_TYPE_OBJECT:
      if (g_scanner_get_next_token (scanner) == BSE_TOKEN_NIL)
        g_value_set_object (value, NULL);
      else
        {
          gpointer item;
          
          if (scanner->token != '(')
            return '(';
          if (g_scanner_get_next_token (scanner) != G_TOKEN_IDENTIFIER)
            return G_TOKEN_IDENTIFIER;
          if (g_scanner_peek_next_token (scanner) != ')')
            {
              g_scanner_get_next_token (scanner);
              return ')';
            }
          
          if (!storage->resolver)
            return bse_storage_warn_skip (storage,
                                          "unable to resolve reference `%s'",
                                          scanner->value.v_identifier);
          item = storage->resolver (storage->resolver_data,
                                    storage,
                                    BSE_TYPE_ITEM,
                                    scanner->value.v_identifier);
          g_value_set_object (value, item);
          
          g_scanner_get_next_token (scanner);
        }
      break;
    case BSE_TYPE_TIME:
      g_scanner_get_next_token (scanner);
      if (scanner->token != G_TOKEN_STRING)
        return G_TOKEN_STRING;
      else
        {
          BseTime time;
          BseErrorType errors[BSE_MAX_DATE_ERRORS];
          guint i;
          
          time = bse_time_from_string (scanner->value.v_string, errors);
          for (i = 0; i < BSE_MAX_DATE_ERRORS && errors[i]; i++)
            switch (errors[i])
              {
              case BSE_ERROR_DATE_CLUTTERED:
                bse_storage_warn (storage, "date string \"%s\" is cluttered", scanner->value.v_string);
                break;
              case BSE_ERROR_DATE_YEAR_BOUNDS:
                bse_storage_warn (storage, "years out of bounds in date `%s'", scanner->value.v_string);
                break;
              case BSE_ERROR_DATE_MONTH_BOUNDS:
                bse_storage_warn (storage, "months out of bounds in date `%s'", scanner->value.v_string);
                break;
              case BSE_ERROR_DATE_DAY_BOUNDS:
                bse_storage_warn (storage, "days out of bounds in date `%s'", scanner->value.v_string);
                break;
              case BSE_ERROR_DATE_HOUR_BOUNDS:
                bse_storage_warn (storage, "hours out of bounds in date `%s'", scanner->value.v_string);
                break;
              case BSE_ERROR_DATE_MINUTE_BOUNDS:
                bse_storage_warn (storage, "minutes out of bounds in date `%s'", scanner->value.v_string);
                break;
              case BSE_ERROR_DATE_SECOND_BOUNDS:
                bse_storage_warn (storage, "seconds out of bounds in date `%s'", scanner->value.v_string);
                break;
              case BSE_ERROR_DATE_TOO_OLD:
                bse_storage_warn_skip (storage, "invalid date `%s'", scanner->value.v_string);
                return G_TOKEN_NONE;
              case BSE_ERROR_DATE_INVALID:
                bse_storage_warn_skip (storage, "unable to match date `%s'", scanner->value.v_string);
                return G_TOKEN_NONE;
              default:
                return G_TOKEN_STRING;
              }
          bse_value_set_time (value, bse_time_from_gmt (time));
          if (g_param_value_validate (pspec, value))
            {
              gchar bbuffer[BSE_BBUFFER_SIZE];
              
              return bse_storage_warn_skip (storage,
                                            "date `%s' out of bounds for parameter `%s'",
                                            bse_time_to_bbuffer (bse_time_from_gmt (time), bbuffer),
                                            pspec->name);
            }
        }
      break;
    case BSE_TYPE_NOTE:
      token = parse_note (storage, BSE_PARAM_SPEC_NOTE (pspec)->allow_void, &v_note);
      if (token != G_TOKEN_NONE)
	return token;
      else
	{
	  bse_value_set_note (value, v_note);
          if (g_param_value_validate (pspec, value))
	    {
	      string = bse_note_to_string (v_note);
	      return bse_storage_warn_skip (storage,
					    "note value `%s' out of bounds for parameter `%s'",
					    string,
					    pspec->name);
	      g_free (string);
	    }
	}
      break;
    case G_TYPE_BOXED:
      if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (pspec), BSE_TYPE_SEQUENCE))
	{
	  BseSequence *sdata = NULL;

	  if (g_scanner_peek_next_token (scanner) != ')')
	    {
	      guint i = 0;

	      parse_or_return (scanner, G_TOKEN_INT);
	      sdata = bse_sequence_new (scanner->value.v_int, BSE_KAMMER_NOTE);

	      while (g_scanner_peek_next_token (scanner) == '(')
		{
		  if (i >= sdata->n_notes)
		    return bse_storage_warn_skip (storage, "too many notes specified");
		  g_scanner_get_next_token (scanner);	/* eat '(' */
		  token = parse_note (storage, TRUE, &v_note);
		  if (token != G_TOKEN_NONE)
		    return token;
		  parse_or_return (scanner, ')');
		  sdata->notes[i++].note = v_note;
		}
	      peek_or_return (scanner, ')');
	    }
	  g_value_set_boxed (value, sdata);
	  break;
	}
      /* fall through */
    default:
      return bse_storage_warn_skip (storage,
                                    "unhandled parameter type `%s' for parameter `%s'",
                                    g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
                                    pspec->name);
    }
  
  g_scanner_get_next_token (scanner);
  
  return scanner->token == ')' ? G_TOKEN_NONE : ')';
}
