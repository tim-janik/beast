/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2002 Tim Janik
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
#include "bseserver.h"
#include "bsesong.h"
#include "bseprocedure.h"
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
struct _BseStorageItemLink
{
  BseStorageItemLink   *next;
  BseItem	       *from_item;
  BseStorageRestoreLink restore_link;
  gpointer              data;
  guint			pbackup;
  gchar		       *upath;
  BseItem	       *to_item;
  gchar		       *error;
};


/* --- macros --- */
#define parse_or_return		bse_storage_scanner_parse_or_return
#define peek_or_return		bse_storage_scanner_peek_or_return


/* --- variables --- */
static  GScannerConfig  bse_scanner_config_template = {
  (
   " \t\r\n"
   )                    /* cset_skip_characters */,
  (
   G_CSET_a_2_z
   "_"
   G_CSET_A_2_Z
   )                    /* cset_identifier_first */,
  (
   G_CSET_a_2_z
   ".:-+_0123456789*!?"
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
  FALSE                 /* scan_string_sq */,
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
bse_storage_enable_proxies (BseStorage *storage)
{
  g_return_if_fail (BSE_IS_STORAGE (storage));

  BSE_STORAGE_SET_FLAGS (storage, BSE_STORAGE_FLAG_PROXIES_ENABLED);
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
      bse_storage_resolve_item_links (storage);
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

BseErrorType
bse_storage_input_text (BseStorage     *storage,
                        const gchar    *text)
{
  g_return_val_if_fail (BSE_IS_STORAGE (storage), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (!BSE_STORAGE_WRITABLE (storage), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (!BSE_STORAGE_READABLE (storage), BSE_ERROR_INTERNAL);
  if (!text)
    text = "";

  storage->fd = -1;
  storage->scanner = g_scanner_new (&bse_scanner_config_template);
  g_datalist_set_data (&storage->scanner->qdata, "BseStorage", storage);
  g_scanner_add_symbol (storage->scanner, "nil", GUINT_TO_POINTER (BSE_TOKEN_NIL));
  g_scanner_input_text (storage->scanner, text, strlen (text));
  storage->scanner->input_name = g_strdup ("InstantString");
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

const gchar*
bse_storage_peek_text (BseStorage *storage,
		       guint      *length)
{
  g_return_val_if_fail (BSE_IS_STORAGE (storage), NULL);
  g_return_val_if_fail (BSE_STORAGE_WRITABLE (storage), NULL);

  bse_storage_handle_break (storage);

  if (length)
    *length = storage->gstring->len;
  return storage->gstring->str;
}

void
bse_storage_flush_fd (BseStorage *storage,
                      gint        fd)
{
  gint l;
  
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_WRITABLE (storage));
  g_return_if_fail (fd >= 0);

  bse_storage_handle_break (storage);
  
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
		  g_warning ("failed to retrive data for storage"); /* FIXME */
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

static BseStorageItemLink*
storage_add_item_link (BseStorage           *storage,
		       BseItem              *from_item,
		       BseStorageRestoreLink restore_link,
		       gpointer              data,
		       gchar		    *error)
{
  BseStorageItemLink *ilink = g_new0 (BseStorageItemLink, 1);

  ilink->next = storage->item_links;
  storage->item_links = ilink;
  ilink->from_item = g_object_ref (from_item);
  ilink->restore_link = restore_link;
  ilink->data = data;
  ilink->error = error;

  return ilink;
}

void
bse_storage_resolve_item_links (BseStorage *storage)
{
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_READABLE (storage));

  while (storage->item_links)
    {
      BseStorageItemLink *ilink = storage->item_links;

      storage->item_links = ilink->next;

      if (ilink->error)
	{
	  gchar *error = g_strdup_printf ("unable to resolve upath for item `%s': %s",
					  BSE_OBJECT_UNAME (ilink->from_item),
					  ilink->error);
	  ilink->restore_link (ilink->data, storage, ilink->from_item, NULL, error);
	  g_free (error);
	  if (ilink->to_item)
	    g_object_unref (ilink->to_item);
	  g_free (ilink->error);
	}
      else if (ilink->to_item)
	{
	  ilink->restore_link (ilink->data, storage, ilink->from_item, ilink->to_item, NULL);
	  g_object_unref (ilink->to_item);
	}
      else if (!ilink->upath)
	{
	  ilink->restore_link (ilink->data, storage, ilink->from_item, NULL, NULL);
	}
      else
	{
	  BseItem *child = NULL, *parent = ilink->from_item;
	  guint pbackup = ilink->pbackup;
	  gchar *error = NULL;

	  while (pbackup && parent)
	    {
	      pbackup--;
	      parent = parent->parent;
	    }
	  if (!parent)
	    error = g_strdup_printf ("failed to chain to ancestor of item `%s' (chain length: %u, "
				     "number of parents: %u) while resolving upath: %s",
				     BSE_OBJECT_UNAME (ilink->from_item),
				     ilink->pbackup,
				     ilink->pbackup - pbackup + 1,
				     ilink->upath);
	  else
	    {
	      child = bse_container_resolve_upath (BSE_CONTAINER (parent), ilink->upath);
	      if (!child)
		error = g_strdup_printf ("failed to find object for item `%s' while resolving upath from ancestor `%s': %s",
					 BSE_OBJECT_UNAME (ilink->from_item),
					 BSE_OBJECT_UNAME (parent),
					 ilink->upath);
	    }
	  ilink->restore_link (ilink->data, storage, ilink->from_item, child, error);
	  g_free (error);
	}
      g_object_unref (ilink->from_item);
      g_free (ilink->upath);
      g_free (ilink);
    }
}

static GTokenType
storage_skipc_statement (BseStorage *storage,
			 guint       level)
{
  GScanner *scanner = storage->scanner;

  g_return_val_if_fail (level > 0, G_TOKEN_ERROR);

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
bse_storage_skip_statement (BseStorage *storage)
{
  g_return_val_if_fail (BSE_IS_STORAGE (storage), G_TOKEN_ERROR);
  g_return_val_if_fail (BSE_STORAGE_READABLE (storage), G_TOKEN_ERROR);

  g_scanner_get_next_token (storage->scanner);
  
  return storage_skipc_statement (storage, 1);
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
  
  if (storage->scanner->parse_errors < storage->scanner->max_parse_errors)
    g_scanner_warn (storage->scanner, "%s - skipping...", string);
  
  g_free (string);

  return bse_storage_skip_statement (storage);
}

GTokenType
bse_storage_warn_skipc (BseStorage  *storage,
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
  
  if (storage->scanner->parse_errors < storage->scanner->max_parse_errors)
    g_scanner_warn (storage->scanner, "%s - skipping...", string);
  
  g_free (string);
  
  return storage_skipc_statement (storage, 1);
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
  
  if (storage->scanner->parse_errors < storage->scanner->max_parse_errors)
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
    *data_handle_p = gsl_wave_handle_new (storage->scanner->input_name,
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
  if (scanner->token == G_TOKEN_STRING)
    bse_bbuffer_puts (bbuffer, scanner->value.v_string);
  else
    return G_TOKEN_STRING;

  note = bse_note_from_string (bbuffer);
  
  if (note_p)
    *note_p = note;
  
  return G_TOKEN_NONE;
}

static GTokenType
parse_note (BseStorage *storage,
	    gboolean    maybe_void,
	    gint       *v_note)
{
  gchar bbuffer[BSE_BBUFFER_SIZE];
  GTokenType token = bse_storage_parse_note (storage, v_note, bbuffer);

  if (token != G_TOKEN_NONE)
    return token;

  if (*v_note == BSE_NOTE_UNPARSABLE)
    {
      bse_storage_error (storage,
			 "invalid note definition `%s'",
			 bbuffer);
      return G_TOKEN_STRING;
    }

  if (*v_note == BSE_NOTE_VOID && !maybe_void)
    {
      *v_note = BSE_KAMMER_NOTE;
      bse_storage_warn (storage,
			"note `%s' out of range",
			bbuffer);
    }
  return G_TOKEN_NONE;
}

BseErrorType
bse_storage_store_procedure (gpointer          storage,
			     BseProcedureClass *proc,
			     const GValue      *ivalues,
			     GValue            *ovalues)
{
  g_return_val_if_fail (BSE_IS_STORAGE (storage), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (BSE_STORAGE_WRITABLE (storage), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (BSE_STORAGE_PROXIES_ENABLED (storage), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (BSE_IS_PROCEDURE_CLASS (proc), BSE_ERROR_INTERNAL);

  bse_storage_handle_break (storage);
  bse_storage_printf (storage, "(bse-proc-eval \"%s\" ", proc->name);
  bse_storage_push_level (storage);
  if (proc->n_in_pspecs)
    {
      guint i;

      for (i = 0; i < proc->n_in_pspecs; i++)
	{
	  bse_storage_break (storage);
	  bse_storage_put_value (storage, ivalues + i, proc->in_pspecs[i]);
	}
    }
  bse_storage_pop_level (storage);
  bse_storage_handle_break (storage);
  bse_storage_putc (storage, ')');

  return BSE_ERROR_NONE;
}

void
bse_storage_put_param (BseStorage   *storage,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_WRITABLE (storage));
  g_return_if_fail (G_IS_VALUE (value));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  
  bse_storage_handle_break (storage);
  
  bse_storage_putc (storage, '(');
  bse_storage_puts (storage, pspec->name);
  bse_storage_putc (storage, ' ');

  bse_storage_put_value (storage, value, pspec);

  bse_storage_putc (storage, ')');
}

void
bse_storage_put_value (BseStorage   *storage,
		       const GValue *value,
		       GParamSpec   *pspec)
{
  GType vtype;

  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_WRITABLE (storage));
  g_return_if_fail (G_IS_VALUE (value));
  if (pspec)
    g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  
  bse_storage_handle_break (storage);

  vtype = G_VALUE_TYPE (value);
  switch (G_TYPE_FUNDAMENTAL (vtype))
    {
      gchar *string;
      GEnumClass *eclass;
      GFlagsClass *fclass;
      GEnumValue *ev;
      GFlagsValue *fv;
      
    case G_TYPE_BOOLEAN:
      bse_storage_puts (storage, g_value_get_boolean (value) ? "#t" : "#f");
      break;
    case G_TYPE_INT:
      if (pspec && BSE_IS_PARAM_SPEC_NOTE (pspec))
	{
	  string = bse_note_to_string (bse_value_get_note (value));
	  bse_storage_printf (storage, "\"%s\"", string);
	  g_free (string);
	}
      else
	bse_storage_printf (storage, "%d", g_value_get_int (value));
      break;
    case G_TYPE_UINT:
      bse_storage_printf (storage, "%u", g_value_get_uint (value));
      break;
    case G_TYPE_LONG:
      bse_storage_printf (storage, "%ld", g_value_get_long (value));
      break;
    case G_TYPE_ULONG:
      bse_storage_printf (storage, "%lu", g_value_get_ulong (value));
      break;
    case G_TYPE_FLOAT:
      bse_storage_printf (storage, "%.17g", g_value_get_float (value));
      break;
    case G_TYPE_DOUBLE:
      bse_storage_printf (storage, "%.30g", g_value_get_double (value));
      break;
    case G_TYPE_STRING:
      if (g_value_get_string (value))
        {
          string = g_strescape (g_value_get_string (value), NULL);
          bse_storage_putc (storage, '"');
          bse_storage_puts (storage, string);
          bse_storage_putc (storage, '"');
          g_free (string);
        }
      else
        bse_storage_puts (storage, "nil");
      break;
    case G_TYPE_ENUM:
      eclass = g_type_class_ref (G_VALUE_TYPE (value));
      ev = g_enum_get_value (eclass, g_value_get_enum (value));
      if (ev)
        bse_storage_puts (storage, ev->value_name);
      else
        bse_storage_printf (storage, "%d", g_value_get_enum (value));
      g_type_class_unref (eclass);
      break;
    case G_TYPE_FLAGS:
      fclass = g_type_class_ref (G_VALUE_TYPE (value));
      fv = g_flags_get_first_value (fclass, g_value_get_flags (value));
      if (fv)
        {
          guint v_flags;
          guint i = 0;
          
          v_flags = g_value_get_flags (value) & ~fv->value;
	  if ((v_flags & ~fv->value) == 0)
	    {
	      /* single value flag */
	      bse_storage_puts (storage, fv->value_name);
	    }
	  else
	    {
	      /* list of flag values */
	      bse_storage_puts (storage, " '(");
	      while (fv)
		{
		  v_flags &= ~fv->value;
		  
		  if (i++ > 0)
		    bse_storage_putc (storage, ' ');
		  bse_storage_puts (storage, fv->value_name);
		  
		  fv = g_flags_get_first_value (fclass, v_flags);
		}
	      if (v_flags)
		bse_storage_printf (storage, " %u", v_flags);
	      bse_storage_putc (storage, ')');
	    }
        }
      else
        bse_storage_printf (storage, "%u", g_value_get_flags (value));
      g_type_class_unref (fclass);
      break;
    case BSE_TYPE_TIME:
      string = bse_time_to_str (bse_time_to_gmt (bse_value_get_time (value)));
      bse_storage_putc (storage, '"');
      bse_storage_puts (storage, string);
      bse_storage_putc (storage, '"');
      g_free (string);
      break;
    case G_TYPE_OBJECT:
      if (BSE_STORAGE_PROXIES_ENABLED (storage) && g_type_is_a (vtype, BSE_TYPE_OBJECT))
	{
	  BseObject *object = g_value_get_object (value);
	  gulong proxy = object ? BSE_OBJECT_ID (object) : 0;
	  bse_storage_printf (storage, "(bse-proxy %lu)", proxy);
	  break;
	}
      goto put_unhandled_parameter;
    case G_TYPE_POINTER:
      if (BSE_STORAGE_PROXIES_ENABLED (storage) && vtype == BSW_TYPE_PROXY)
	{
	  gulong proxy = bsw_value_get_proxy (value);
	  bse_storage_printf (storage, "(bse-proxy %lu)", proxy);
	  break;
	}
      goto put_unhandled_parameter;
    case G_TYPE_BOXED:
      if (g_type_is_a (G_VALUE_TYPE (value), BSW_TYPE_NOTE_SEQUENCE))
	{
	  BswNoteSequence *sdata = g_value_get_boxed (value);
	  guint i;

	  bse_storage_printf (storage, "'(");
	  bse_storage_push_level (storage);
	  for (i = 0; i < sdata->n_notes; i++)
	    {
	      if (i % 4)
		bse_storage_putc (storage, ' ');
	      else
		bse_storage_break (storage);
	      string = bse_note_to_string (sdata->notes[i].note);
	      bse_storage_printf (storage, "\"%s\"", string);
	      g_free (string);
	    }
	  bse_storage_pop_level (storage);
	  bse_storage_putc (storage, ')');
	  break;
	}
      goto put_unhandled_parameter;
    default:
    put_unhandled_parameter:
      bse_storage_putc (storage, '?');
      if (pspec)
	g_warning (G_STRLOC ": unhandled parameter \"%s\" type `%s'",
		   pspec->name,
		   g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
      else
	g_warning (G_STRLOC ": unhandled value type `%s'",
		   g_type_name (vtype));
      break;
    }
}

void
bse_storage_put_item_link (BseStorage     *storage,
			   BseItem        *from_item,
			   BseItem        *to_item)
{
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_WRITABLE (storage));
  g_return_if_fail (BSE_IS_ITEM (from_item));
  g_return_if_fail (BSE_IS_ITEM (to_item));

  bse_storage_handle_break (storage);

  if (!to_item)						/* special case (1) */
    {
      bse_storage_puts (storage, "nil");
    }
  else if (to_item == (BseItem*) bse_server_get ())	/* special case (2) */
    {
      bse_storage_printf (storage, "(bse-server)");
    }
  else		/* ordiniary object link within a project or other container */
    {
      BseItem *tmp, *common_ancestor;
      guint pbackup = 0;
      gchar *upath, *epath;

      g_return_if_fail (BSE_IS_ITEM (to_item));
      common_ancestor = bse_item_common_ancestor (from_item, to_item);
      g_return_if_fail (BSE_IS_CONTAINER (common_ancestor));

      /* figure number of parent backup levels to reach common ancestor */
      for (tmp = from_item; tmp != common_ancestor; tmp = tmp->parent)
	pbackup++;

      /* path to reach to_item */
      upath = bse_container_make_upath (BSE_CONTAINER (common_ancestor), to_item);

      /* store path reference */
      epath = g_strescape (upath, NULL);
      bse_storage_printf (storage, "(bse-upath-resolve %u \"%s\")", pbackup, epath);
      g_free (epath);
      g_free (upath);
    }
}

GTokenType
bse_storage_parse_item_link (BseStorage           *storage,
			     BseItem              *from_item,
			     BseStorageRestoreLink restore_link,
			     gpointer              data)
{
  GScanner *scanner;
  BseStorageItemLink *ilink;
  GTokenType expected_token;

  g_return_val_if_fail (BSE_IS_STORAGE (storage), G_TOKEN_ERROR);
  g_return_val_if_fail (BSE_STORAGE_READABLE (storage), G_TOKEN_ERROR);
  g_return_val_if_fail (BSE_IS_ITEM (from_item), G_TOKEN_ERROR);
  g_return_val_if_fail (restore_link != NULL, G_TOKEN_ERROR);

  scanner = storage->scanner;

#define	parse_or_goto(etoken,label) \
  { expected_token = (etoken); if (g_scanner_get_next_token (scanner) != expected_token) goto label; }
#define	peek_or_goto(etoken,label)  \
  { expected_token = (etoken); if (g_scanner_peek_next_token (scanner) != expected_token) \
    { g_scanner_get_next_token (scanner); goto label; } }

  g_scanner_get_next_token (scanner);

  if (scanner->token == BSE_TOKEN_NIL)
    {
      ilink = storage_add_item_link (storage, from_item, restore_link, data, NULL);
    }
  else if (scanner->token == '(')
    {
      parse_or_goto (G_TOKEN_IDENTIFIER, error_parse_link);

      if (strcmp (scanner->value.v_identifier, "bse-server") == 0)
	{
	  peek_or_goto (')', error_parse_link);

	  ilink = storage_add_item_link (storage, from_item, restore_link, data, NULL);
	  ilink->to_item = g_object_ref (bse_server_get ());
	}
      else if (strcmp (scanner->value.v_identifier, "bse-upath-resolve") == 0)
	{
	  guint pbackup = 0;

	  if (g_scanner_peek_next_token (scanner) == G_TOKEN_INT)
	    {
	      g_scanner_get_next_token (scanner);	/* eat int */
	      pbackup = scanner->value.v_int;
	    }

	  parse_or_goto (G_TOKEN_STRING, error_parse_link);
          peek_or_goto (')', error_parse_link);

	  ilink = storage_add_item_link (storage, from_item, restore_link, data, NULL);
	  ilink->upath = g_strdup (scanner->value.v_string);
	  ilink->pbackup = pbackup;
	}
      else
	{
	  expected_token = G_TOKEN_IDENTIFIER;
	  goto error_parse_link;
	}
      parse_or_goto (')', error_parse_link);
    }
  else
    {
      expected_token = '(';
      goto error_parse_link;
    }

  return G_TOKEN_NONE;
  
#undef	parse_or_goto
#undef	peek_or_goto

 error_parse_link:
  ilink = storage_add_item_link (storage, from_item, restore_link, data, g_strdup ("failed to parse upath"));
  return expected_token;
}

static GTokenType	G_GNUC_PRINTF (4,5)
storage_errwarn_skip (BseStorage  *storage,
		      gboolean     close_statement,
		      gboolean     skip_checks_current,
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

  if (close_statement)
    g_scanner_warn (storage->scanner, "%s - skipping...", string);
  else
    g_scanner_error (storage->scanner, "%s", string);

  g_free (string);

  if (close_statement && !skip_checks_current)
    g_scanner_get_next_token (storage->scanner);

  return !close_statement ? G_TOKEN_ERROR : storage_skipc_statement (storage, 1);
}

GTokenType
bse_storage_parse_param_value (BseStorage *storage,
                               GValue     *value,
                               GParamSpec *pspec,
			       gboolean    close_statement)
{
  GScanner *scanner;
  const gchar *pname;
  GType vtype;

  g_return_val_if_fail (BSE_IS_STORAGE (storage), G_TOKEN_ERROR);
  g_return_val_if_fail (BSE_STORAGE_READABLE (storage), G_TOKEN_ERROR);
  g_return_val_if_fail (G_IS_VALUE (value), G_TOKEN_ERROR);
  if (pspec)
    g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), G_TOKEN_ERROR);

  scanner = storage->scanner;
  pname = pspec ? pspec->name : "anonymous value";
  vtype = G_VALUE_TYPE (value);
  if (pspec)
    g_return_val_if_fail (g_type_is_a (vtype, G_PARAM_SPEC_VALUE_TYPE (pspec)), G_TOKEN_ERROR);
  switch (G_TYPE_FUNDAMENTAL (vtype))
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
      else if (scanner->token == '#')
        {
	  g_scanner_get_next_token (scanner);

          if (scanner->token == 't' || scanner->token == 'T')
	    v_bool = TRUE;
	  else if (scanner->token == 'f' || scanner->token == 'F')
	    v_bool = FALSE;
          else if ((scanner->token >= 'a' && scanner->token <= 'z') ||
		   (scanner->token >= 'A' && scanner->token <= 'Z') ||
		   (scanner->token >= '0' && scanner->token <= '9'))
            return storage_errwarn_skip (storage, close_statement, TRUE,
					 "unrecognized boolean value `%c' for parameter `%s'",
					 scanner->token,
					 pname);
	  else
	    return 'f';
        }
      else
        return '#';
      g_value_set_boolean (value, v_bool);
      break;
    case G_TYPE_UINT:
    case G_TYPE_ULONG:
      g_scanner_get_next_token (scanner);
      if (scanner->token != G_TOKEN_INT &&
	  scanner->token != G_TOKEN_FLOAT)
        return G_TOKEN_INT;
      else
	{
	  gulong v_int = scanner->token == G_TOKEN_INT ? scanner->value.v_int : scanner->value.v_float;

	  if (G_TYPE_FUNDAMENTAL (G_VALUE_TYPE (value)) == G_TYPE_UINT)
	    g_value_set_uint (value, v_int);
	  else
	    g_value_set_ulong (value, v_int);
	  if (pspec && g_param_value_validate (pspec, value) && !(pspec->flags & G_PARAM_LAX_VALIDATION))
	    return storage_errwarn_skip (storage, close_statement, FALSE,
					 "parameter value `%lu' out of bounds for parameter `%s'",
					 v_int, pname);
	}
      break;
    case G_TYPE_INT:
      if (pspec && BSE_IS_PARAM_SPEC_NOTE (pspec))
	{
	  token = parse_note (storage, pspec ? BSE_PARAM_SPEC_NOTE (pspec)->allow_void : TRUE, &v_note);
	  if (token != G_TOKEN_NONE)
	    return token;
	  else
	    {
	      bse_value_set_note (value, v_note);
	      if (pspec && g_param_value_validate (pspec, value) && !(pspec->flags & G_PARAM_LAX_VALIDATION))
		{
		  string = bse_note_to_string (v_note);
		  return storage_errwarn_skip (storage, close_statement, FALSE,
					       "note value `%s' out of bounds for parameter `%s'",
					       string,
					       pname);
		  g_free (string);
		}
	    }
	}
      /* fall through */
    case G_TYPE_LONG:
      g_scanner_get_next_token (scanner);
      v_bool = FALSE;
      if (scanner->token == '-')
        {
          v_bool = !v_bool;
          g_scanner_get_next_token (scanner);
        }
      if (scanner->token != G_TOKEN_INT &&
	  scanner->token != G_TOKEN_FLOAT)
        return G_TOKEN_INT;
      else
	{
          glong v_int = scanner->token == G_TOKEN_INT ? scanner->value.v_int : scanner->value.v_float;

	  if (G_TYPE_FUNDAMENTAL (G_VALUE_TYPE (value)) == G_TYPE_INT)
	    g_value_set_int (value, v_bool ? - v_int : v_int);
	  else
	    g_value_set_long (value, v_bool ? - v_int : v_int);
          if (pspec && g_param_value_validate (pspec, value) && !(pspec->flags & G_PARAM_LAX_VALIDATION))
	    return storage_errwarn_skip (storage, close_statement, FALSE,
					 "parameter value `%ld' out of bounds for parameter `%s'",
					 v_bool ? - v_int : v_int,
					 pname);
	}
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
	{
	  v_double = scanner->value.v_int;
	  if (v_bool)
	    v_double = -v_double;
	}
      else
        return G_TOKEN_FLOAT;
      g_value_set_float (value, v_double);
      if (pspec && g_param_value_validate (pspec, value) && !(pspec->flags & G_PARAM_LAX_VALIDATION))
        return storage_errwarn_skip (storage, close_statement, FALSE,
				     "parameter value `%f' out of bounds for parameter `%s'",
				     v_double,
				     pname);
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
	{
	  v_double = scanner->value.v_int;
	  if (v_bool)
	    v_double = -v_double;
	}
      else
        return G_TOKEN_FLOAT;
      g_value_set_double (value, v_double);
      if (pspec && g_param_value_validate (pspec, value) && !(pspec->flags & G_PARAM_LAX_VALIDATION))
        return storage_errwarn_skip (storage, close_statement, FALSE,
				     "parameter value `%f' out of bounds for parameter `%s'",
				     v_double,
				     pname);
      break;
    case G_TYPE_STRING:
      if (g_scanner_get_next_token (scanner) == BSE_TOKEN_NIL)
	g_value_set_string (value, NULL); /* FIXME: check validity */
      else if (scanner->token == G_TOKEN_STRING)
	g_value_set_string (value, scanner->value.v_string);
      else
        return G_TOKEN_STRING;
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
          GEnumClass *eclass = g_type_class_ref (vtype);

          ev = g_enum_get_value_by_nick (eclass,
                                         scanner->value.v_identifier);
          if (!ev)
            ev = g_enum_get_value_by_name (eclass,
                                           scanner->value.v_identifier);
	  v_enum = ev ? ev->value : 0;

	  g_type_class_unref (eclass);
          if (!ev)
            return storage_errwarn_skip (storage, close_statement, FALSE,
					 "unrecognized enum value `%s' for parameter `%s'",
					 scanner->value.v_identifier,
					 pname);
        }
      else
        return G_TOKEN_IDENTIFIER;
      g_value_set_enum (value, v_enum);
      if (pspec && g_param_value_validate (pspec, value) && !(pspec->flags & G_PARAM_LAX_VALIDATION))
        return storage_errwarn_skip (storage, close_statement, FALSE,
				     "parameter value `%d' out of bounds for parameter `%s'",
				     v_enum,
				     pname);
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
	      GFlagsClass *fclass = g_type_class_ref (vtype);
	      
              fv = g_flags_get_value_by_nick (fclass, scanner->value.v_identifier);
              if (!fv)
                g_flags_get_value_by_name (fclass, scanner->value.v_identifier);
	      v_flags |= fv ? fv->value : 0;

	      g_type_class_unref (fclass);
              if (!fv)
                return storage_errwarn_skip (storage, close_statement, FALSE,
					     "unrecognized flags value `%s' for parameter `%s'",
					     scanner->value.v_identifier,
					     pname);
            }
          else
            return G_TOKEN_IDENTIFIER;
        }
      while (g_scanner_peek_next_token (scanner) != ')');
      g_value_set_flags (value, v_flags);
      if (pspec && g_param_value_validate (pspec, value) && !(pspec->flags & G_PARAM_LAX_VALIDATION))
        return storage_errwarn_skip (storage, close_statement, FALSE,
				     "parameter value `%d' out of bounds for parameter `%s'",
				     v_flags,
				     pname);
      break;
    case BSE_TYPE_TIME:
      parse_or_return (scanner, G_TOKEN_STRING);
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
	      bse_storage_warn (storage, "%s out of bounds in date `%s'", "years", scanner->value.v_string);
	      break;
	    case BSE_ERROR_DATE_MONTH_BOUNDS:
	      bse_storage_warn (storage, "%s out of bounds in date `%s'", "months", scanner->value.v_string);
	      break;
	    case BSE_ERROR_DATE_DAY_BOUNDS:
	      bse_storage_warn (storage, "%s out of bounds in date `%s'", "days", scanner->value.v_string);
	      break;
	    case BSE_ERROR_DATE_HOUR_BOUNDS:
	      bse_storage_warn (storage, "%s out of bounds in date `%s'", "hours", scanner->value.v_string);
	      break;
	    case BSE_ERROR_DATE_MINUTE_BOUNDS:
	      bse_storage_warn (storage, "%s out of bounds in date `%s'", "minutes", scanner->value.v_string);
	      break;
	    case BSE_ERROR_DATE_SECOND_BOUNDS:
	      bse_storage_warn (storage, "%s out of bounds in date `%s'", "seconds", scanner->value.v_string);
	      break;
	    case BSE_ERROR_DATE_TOO_OLD:
	      return storage_errwarn_skip (storage, close_statement, FALSE,
					   "invalid date `%s'", scanner->value.v_string);
	    case BSE_ERROR_DATE_INVALID:
	      return storage_errwarn_skip (storage, close_statement, FALSE,
					   "unable to parse date `%s'", scanner->value.v_string);
	    default:
	      return G_TOKEN_STRING;
	    }
	bse_value_set_time (value, bse_time_from_gmt (time));
	if (pspec && g_param_value_validate (pspec, value) && !(pspec->flags & G_PARAM_LAX_VALIDATION))
	  {
	    gchar bbuffer[BSE_BBUFFER_SIZE];
	    
	    return storage_errwarn_skip (storage, close_statement, FALSE,
					 "date `%s' out of bounds for parameter `%s'",
					 bse_time_to_bbuffer (bse_time_from_gmt (time), bbuffer),
					 pname);
	  }
      }
      break;
    case G_TYPE_OBJECT:
      if (BSE_STORAGE_PROXIES_ENABLED (storage) && g_type_is_a (vtype, BSE_TYPE_OBJECT))
	{
	  g_scanner_get_next_token (scanner);
	  if (scanner->token == '(')
	    {
	      gulong proxy;

	      parse_or_return (scanner, G_TOKEN_IDENTIFIER);
	      if (!bse_string_equals ("bse-proxy", scanner->value.v_identifier))
		return G_TOKEN_IDENTIFIER;
	      parse_or_return (scanner, G_TOKEN_INT);
	      proxy = scanner->value.v_int;
	      parse_or_return (scanner, ')');
	      g_value_set_object (value, proxy ? bse_object_from_id (proxy) : 0);
	    }
	  else
	    return '(';
	  break;
	}
      goto parse_unhandled_parameter;
    case G_TYPE_POINTER:
      if (BSE_STORAGE_PROXIES_ENABLED (storage) && vtype == BSW_TYPE_PROXY)
	{
	  g_scanner_get_next_token (scanner);
	  if (scanner->token == '(')
	    {
	      gulong proxy;

	      parse_or_return (scanner, G_TOKEN_IDENTIFIER);
	      if (!bse_string_equals ("bse-proxy", scanner->value.v_identifier))
		return G_TOKEN_IDENTIFIER;
	      parse_or_return (scanner, G_TOKEN_INT);
	      proxy = scanner->value.v_int;
	      parse_or_return (scanner, ')');
	      bsw_value_set_proxy (value, proxy);
	    }
	  else
	    return '(';
	  break;
	}
      goto parse_unhandled_parameter;
    case G_TYPE_BOXED:
      if (g_type_is_a (vtype, BSW_TYPE_NOTE_SEQUENCE))
	{
	  BswNoteSequence *sdata = NULL;
	  guint i = 0;

	  parse_or_return (scanner, '\'');	/* list quote */
	  parse_or_return (scanner, '(');	/* list start */

	  sdata = bsw_note_sequence_new (32);
	  while (g_scanner_peek_next_token (scanner) != ')')
	    {
	      token = parse_note (storage, TRUE, &v_note);
	      if (token != G_TOKEN_NONE)
		{
		  bsw_note_sequence_free (sdata);
		  return token;
		}
	      if (i == sdata->n_notes)
		sdata = bsw_note_sequence_resize (sdata, sdata->n_notes + 32);
	      sdata->notes[i++].note = v_note;
	    }
	  if (g_scanner_get_next_token (scanner) != ')')
	    {
	      bsw_note_sequence_free (sdata);
	      return ')';			/* list end */
	    }
	  g_value_set_boxed_take_ownership (value, bsw_note_sequence_resize (sdata, i));
	  break;
	}
      goto parse_unhandled_parameter;
    default:
    parse_unhandled_parameter:
      return storage_errwarn_skip (storage, close_statement, FALSE,
				   "unhandled parameter type `%s' for parameter `%s': unable to parse `%s' types",
				   g_type_name (vtype),
				   pname,
				   g_type_name (G_TYPE_FUNDAMENTAL (vtype)));
    }

  if (!close_statement)
    return G_TOKEN_NONE;

  g_scanner_get_next_token (scanner);
  return scanner->token == ')' ? G_TOKEN_NONE : ')';
}
