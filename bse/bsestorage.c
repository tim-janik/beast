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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include	"bsestorage.h"

#include	"bseitem.h"
#include	"bsebindata.h"
#include	"bseproject.h"
#include	"bsesample.h"
#include	"bsesong.h"
#include	<fcntl.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<errno.h>


/* --- variables --- */
static	GScannerConfig	bse_scanner_config_template = {
  (
   " \t\r\n"
   )			/* cset_skip_characters */,
  (
   G_CSET_a_2_z
   "_'"
   G_CSET_A_2_Z
   )			/* cset_identifier_first */,
  (
   G_CSET_a_2_z
   ".:-+_0123456789"
   G_CSET_A_2_Z
   G_CSET_LATINS
   G_CSET_LATINC
   )			/* cset_identifier_nth */,
  ( ";\n" )		/* cpair_comment_single */,
  
  FALSE			/* case_sensitive */,
  
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
  TRUE			/* scan_string_sq */,
  TRUE			/* scan_string_dq */,
  TRUE			/* numbers_2_int */,
  FALSE			/* int_2_float */,
  FALSE			/* identifier_2_string */,
  TRUE			/* char_2_token */,
  TRUE			/* symbol_2_token */,
  FALSE			/* scope_0_fallback */,
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
  storage->rblocks = NULL;
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
  g_return_val_if_fail (scanner != NULL, NULL);
  
  return BSE_IS_STORAGE (scanner->derived_data) ? scanner->derived_data : NULL;
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
      
      while (storage->rblocks)
	{
	  BseStorageBBlock *bblock = storage->rblocks;
	  
	  storage->rblocks = bblock->next;
	  bse_object_unref (BSE_OBJECT (bblock->bdata));
	  g_free (bblock);
	}
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
	  bse_object_unref (BSE_OBJECT (bblock->bdata));
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
  storage->scanner->derived_data = storage;
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
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_WRITABLE (storage));
  
  if (string)
    {
      guint l = strlen (string);
      
      if (storage->gstring)
	g_string_append (storage->gstring, string);
      
      if (string[l - 1] == '\n')
	BSE_STORAGE_SET_FLAGS (storage, BSE_STORAGE_FLAG_AT_BOL);
      else
	BSE_STORAGE_UNSET_FLAGS (storage, BSE_STORAGE_FLAG_AT_BOL);
    }
}

void
bse_storage_putc (BseStorage *storage,
		  gchar	      character)
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
bse_storage_printf (BseStorage	*storage,
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

void
bse_storage_put_param (BseStorage *storage,
		       BseParam	  *param)
{
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_WRITABLE (storage));
  
  bse_storage_handle_break (storage);
  
  bse_storage_putc (storage, '(');
  bse_storage_puts (storage, param->pspec->any.name);
  bse_storage_putc (storage, ' ');
  
  switch (BSE_FUNDAMENTAL_TYPE (param->pspec->type))
    {
      gchar *string;
      BseEnumValue *ev;
      BseFlagsValue *fv;
      
    case BSE_TYPE_PARAM_BOOL:
      /* bse_storage_puts (storage, 
       *		   (param->value.v_bool ?
       *		    param->pspec->s_bool.true_identifier :
       *		    param->pspec->s_bool.false_identifier));
       */
      bse_storage_puts (storage, param->value.v_bool ? "'t" : "'f");
      break;
    case BSE_TYPE_PARAM_INT:
      bse_storage_printf (storage, "%d", param->value.v_int);
      break;
    case BSE_TYPE_PARAM_UINT:
      bse_storage_printf (storage, "%u", param->value.v_uint);
      break;
    case BSE_TYPE_PARAM_ENUM:
      ev = bse_enum_get_value (param->pspec->s_enum.enum_class, param->value.v_enum);
      if (ev)
	bse_storage_puts (storage, ev->value_name);
      else
	bse_storage_printf (storage, "%d", param->value.v_enum);
      break;
    case BSE_TYPE_PARAM_FLAGS:
      fv = bse_flags_get_first_value (param->pspec->s_flags.flags_class, param->value.v_flags);
      if (fv)
	{
	  guint value;
	  guint i = 0;
	  
	  value = param->value.v_flags & ~fv->value;
	  while (fv)
	    {
	      value &= ~fv->value;
	      
	      if (i++)
		bse_storage_putc (storage, ' ');
	      
	      bse_storage_puts (storage, fv->value_name);
	      
	      fv = bse_flags_get_first_value (param->pspec->s_flags.flags_class, value);
	    }
	  if (value)
	    bse_storage_printf (storage, " %u", value);
	}
      else
	bse_storage_printf (storage, "%u", param->value.v_flags);
      break;
    case BSE_TYPE_PARAM_FLOAT:
      bse_storage_printf (storage, "%f", param->value.v_float);
      break;
    case BSE_TYPE_PARAM_DOUBLE:
      bse_storage_printf (storage, "%e", param->value.v_double);
      break;
    case BSE_TYPE_PARAM_TIME:
      string = bse_time_to_str (bse_time_to_gmt (param->value.v_time));
      bse_storage_putc (storage, '"');
      bse_storage_puts (storage, string);
      bse_storage_putc (storage, '"');
      g_free (string);
      break;
    case BSE_TYPE_PARAM_NOTE:
      string = bse_note_to_string (param->value.v_note);
      bse_storage_putc (storage, '\'');
      bse_storage_puts (storage, string);
      g_free (string);
      break;
    case BSE_TYPE_PARAM_INDEX_2D:
      string = g_strdup_printf ("%u %u",
				BSE_INDEX_2D_HI (param->value.v_index_2d),
				BSE_INDEX_2D_VI (param->value.v_index_2d));
      bse_storage_puts (storage, string);
      g_free (string);
      break;
    case BSE_TYPE_PARAM_STRING:
      if (param->value.v_string)
	{
	  string = g_strdup_quoted (param->value.v_string);
	  bse_storage_putc (storage, '"');
	  bse_storage_puts (storage, string);
	  bse_storage_putc (storage, '"');
	  g_free (string);
	}
      else
	bse_storage_puts (storage, "nil");
      break;
    case BSE_TYPE_PARAM_ITEM:
      if (param->value.v_item)
	{
	  BseProject *project = bse_item_get_project (param->value.v_item);
	  gchar *path = bse_container_make_item_path (BSE_CONTAINER (project),
						      param->value.v_item,
						      TRUE);

	  bse_storage_printf (storage, "(%s)", path);
	  g_free (path);
	}
      else
	bse_storage_puts (storage, "nil");
      break;
    default:
      bse_storage_putc (storage, '?');
      g_warning (G_STRLOC ": unhandled parameter type `%s' (%d)",
		 bse_type_name (param->pspec->type),
		 param->pspec->type);
      break;
    }
  
  bse_storage_putc (storage, ')');
}

static BseStorageBBlock*
bse_storage_get_create_wblock (BseStorage *storage,
			       BseBinData *bdata)
{
  BseStorageBBlock *bblock, *last = NULL;
  
  for (bblock = storage->wblocks; bblock; last = bblock, bblock = last->next)
    if (bblock->bdata == bdata)
      return bblock;

  /* create */
  bblock = g_new0 (BseStorageBBlock, 1);
  bblock->bdata = bdata;
  bse_object_ref (BSE_OBJECT (bdata));
  bblock->offset = last ? last->offset + last->length : 0;
  bblock->length = bdata->n_bytes;
  if (last)
    last->next = bblock;
  else
    storage->wblocks = bblock;
  
  return bblock;
}

void
bse_storage_put_bin_data (BseStorage *storage,
			  BseBinData *bdata)
{
  BseStorageBBlock *bblock;
  
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_WRITABLE (storage));
  g_return_if_fail (BSE_IS_BIN_DATA (bdata));
  
  bblock = bse_storage_get_create_wblock (storage, bdata);
  
  bse_storage_handle_break (storage);
  /* we save only little endian data */
  bse_storage_printf (storage,
		      "(BseBinStorageV0 %u L:%u %u)",
		      bblock->offset,
		      bdata->bits_per_value,
		      bblock->length);
}

void
bse_storage_flush_fd (BseStorage *storage,
		      gint	  fd)
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
	  guint len = MIN (bblock->length, bblock->bdata->n_bytes);
	  guint pad = bblock->length - len;

	  do
	    l = write (fd, bblock->bdata->values, len);
	  while (l < 0 && errno == EINTR);

	  while (pad)
	    {
	      guint8 buffer[1024] = { 0, };

	      n = MIN (pad, 1024);

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
			       glong      *cur_offs)
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
	  guint8 data[512];
	  guint i;
	  
	  do
	    l = read (fd, data, 512);
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

  *cur_offs = coffs;

  return BSE_ERROR_NONE;
}

static BseErrorType
bse_storage_create_rblock (BseStorage   *storage,
			   guint         offset,
			   guint         length,
			   BseEndianType byte_order,
			   guint         bits_per_value)
{
  BseBinData *bin_data;
  BseStorageBBlock *bblock;
  BseErrorType error;
  gint fd = storage->fd;

  bin_data = bse_object_new (BSE_TYPE_BIN_DATA,
			     "n-bits", bits_per_value,
			     NULL);
  error = bse_bin_data_set_values_from_fd (bin_data,
					   fd,
					   storage->bin_offset + 1 + offset,
					   length,
					   byte_order);
  if (error)
    {
      bse_object_unref (BSE_OBJECT (bin_data));
      return error;
    }

  bblock = g_new0 (BseStorageBBlock, 1);
  bblock->bdata = bin_data;
  bblock->offset = offset;
  bblock->length = bin_data->n_bytes;
  bblock->next = storage->rblocks;
  storage->rblocks = bblock;

  return BSE_ERROR_NONE;
}

static inline BseStorageBBlock*
bse_storage_find_rblock (BseStorage *storage,
			 guint       offset)
{
  BseStorageBBlock *bblock;

  for (bblock = storage->rblocks; bblock; bblock = bblock->next)
    if (bblock->offset == offset)
      return bblock;
  return NULL;
}

GTokenType
bse_storage_parse_bin_data (BseStorage  *storage,
			    BseBinData **bdata_p)
{
  GScanner *scanner;
  BseStorageBBlock *bblock;
  guint offset, bits_per_value, length;
  BseEndianType byte_order = BSE_LITTLE_ENDIAN;
  gchar *string;

  if (bdata_p)
    *bdata_p = NULL;
  g_return_val_if_fail (BSE_IS_STORAGE (storage), G_TOKEN_ERROR);
  g_return_val_if_fail (BSE_STORAGE_READABLE (storage), G_TOKEN_ERROR);

  scanner = storage->scanner;
  
  if (g_scanner_get_next_token (scanner) != '(')
    return '(';

  if (g_scanner_get_next_token (scanner) != G_TOKEN_IDENTIFIER ||
      !bse_string_equals ("BseBinStorageV0", scanner->value.v_identifier))
    return G_TOKEN_IDENTIFIER;

  if (g_scanner_get_next_token (scanner) != G_TOKEN_INT)
    return G_TOKEN_INT;
  offset = scanner->value.v_int;

  if (g_scanner_get_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return G_TOKEN_IDENTIFIER;
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

      bits_per_value = strtol (string + 2, &f, 10);
      if ((bits_per_value != 8 &&
	   bits_per_value != 16) ||
	  (f && *f != 0))
	string = NULL;
    }
  if (!string)
    return bse_storage_warn_skip (storage,
				  "unknown bit type `%s' in bin data definition",
				  scanner->value.v_identifier);

  if (g_scanner_get_next_token (scanner) != G_TOKEN_INT)
    return G_TOKEN_INT;
  length = scanner->value.v_int;

  if (g_scanner_get_next_token (scanner) != ')')
    return ')';

  bblock = bse_storage_find_rblock (storage, offset);
  if (!bblock)
    {
      BseErrorType error;
      glong saved_offset;
      
      /* except for BSE_ERROR_FILE_NOT_FOUND, all errors are fatal ones,
       * we can't guarantee that further parsing is possible.
       */
      error = bse_storage_ensure_bin_offset (storage, &saved_offset);
      if (error == BSE_ERROR_FILE_NOT_FOUND)
	{
	  bse_storage_warn (storage, "no device to retrive binary data from");
	  return G_TOKEN_NONE;
	}

      if (!error)
	error = bse_storage_create_rblock (storage, offset, length, byte_order, bits_per_value);

      if (!error)
	error = bse_storage_restore_offset (storage, saved_offset);

      if (error)
	{
	  bse_storage_error (storage, "failed to retrive binary data: %s", bse_error_blurb (error));
	  return G_TOKEN_ERROR;
	}

      bblock = storage->rblocks;
    }
  if (bblock && bdata_p)
    *bdata_p = bblock->bdata;

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

GTokenType
bse_storage_parse_param_value (BseStorage *storage,
			       BseParam	  *param)
{
  GScanner *scanner;
  BseParamSpec *pspec;
  
  g_return_val_if_fail (BSE_IS_STORAGE (storage), G_TOKEN_ERROR);
  g_return_val_if_fail (BSE_STORAGE_READABLE (storage), G_TOKEN_ERROR);
  g_return_val_if_fail (BSE_IS_PARAM (param), G_TOKEN_ERROR);
  
  scanner = storage->scanner;
  pspec = param->pspec;
  
  switch (BSE_FUNDAMENTAL_TYPE (pspec->type))
    {
      gboolean v_bool;
      gint v_enum;
      guint v_flags;
      gint v_note;
      gdouble v_double;
      gchar *string, buffer[2];
      guint hi, vi;
      
    case BSE_TYPE_PARAM_BOOL:
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
	  else if (!strcmp (scanner->value.v_identifier, pspec->s_bool.true_identifier))
	    v_bool = TRUE;
	  else if (!strcmp (scanner->value.v_identifier, pspec->s_bool.false_identifier))
	    v_bool = FALSE;
	  else
	    return bse_storage_warn_skip (storage,
					  "unrecognized boolean value `%s' for parameter `%s'",
					  scanner->value.v_identifier,
					  pspec->any.name);
	}
      else
	return G_TOKEN_IDENTIFIER;
      bse_param_set_bool (param, v_bool);
      break;
    case BSE_TYPE_PARAM_UINT:
      g_scanner_get_next_token (scanner);
      if (scanner->token != G_TOKEN_INT)
	return G_TOKEN_INT;
      else if (bse_param_set_uint (param, scanner->value.v_int))
	return bse_storage_warn_skip (storage,
				      "parameter value `%lu' out of bounds for parameter `%s'",
				      scanner->value.v_int,
				      pspec->any.name);
      break;
    case BSE_TYPE_PARAM_INT:
      g_scanner_get_next_token (scanner);
      v_bool = FALSE;
      if (scanner->token == '-')
	{
	  v_bool = !v_bool;
	  g_scanner_get_next_token (scanner);
	}
      if (scanner->token != G_TOKEN_INT)
	return G_TOKEN_INT;
      else if (bse_param_set_int (param, v_bool ? - scanner->value.v_int : scanner->value.v_int))
	return bse_storage_warn_skip (storage,
				      "parameter value `%ld' out of bounds for parameter `%s'",
				      v_bool ? - scanner->value.v_int : scanner->value.v_int,
				      pspec->any.name);
      break;
    case BSE_TYPE_PARAM_ENUM:
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
	  BseEnumValue *ev;
	  
	  ev = bse_enum_get_value_by_nick (pspec->s_enum.enum_class,
					   scanner->value.v_identifier);
	  if (!ev)
	    ev = bse_enum_get_value_by_name (pspec->s_enum.enum_class,
					     scanner->value.v_identifier);
	  if (ev)
	    v_enum = ev->value;
	  else
	    return bse_storage_warn_skip (storage,
					  "unrecognized enum value `%s' for parameter `%s'",
					  scanner->value.v_identifier,
					  pspec->any.name);
	}
      else
	return G_TOKEN_IDENTIFIER;
      if (bse_param_set_enum (param, v_enum))
	return bse_storage_warn_skip (storage,
				      "parameter value `%d' out of bounds for parameter `%s'",
				      v_enum,
				      pspec->any.name);
      break;
    case BSE_TYPE_PARAM_FLAGS:
      v_flags = 0;
      do
	{
	  g_scanner_get_next_token (scanner);
	  if (scanner->token == G_TOKEN_INT)
	    v_flags |= scanner->value.v_int;
	  else if (scanner->token == G_TOKEN_IDENTIFIER)
	    {
	      BseFlagsValue *fv;
	      
	      fv = bse_flags_get_value_by_nick (param->pspec->s_flags.flags_class,
						scanner->value.v_identifier);
	      if (!fv)
		bse_flags_get_value_by_name (param->pspec->s_flags.flags_class,
					     scanner->value.v_identifier);
	      if (fv)
		v_flags |= fv->value;
	      else
		return bse_storage_warn_skip (storage,
					      "unrecognized flags value `%s' for parameter `%s'",
					      scanner->value.v_identifier,
					      pspec->any.name);
	    }
	  else
	    return G_TOKEN_IDENTIFIER;
	}
      while (g_scanner_peek_next_token (scanner) != ')');
      if (bse_param_set_flags (param, v_flags))
	return bse_storage_warn_skip (storage,
				      "parameter value `%d' out of bounds for parameter `%s'",
				      v_flags,
				      pspec->any.name);
      break;
    case BSE_TYPE_PARAM_FLOAT:
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
      if (bse_param_set_float (param, v_double))
	return bse_storage_warn_skip (storage,
				      "parameter value `%f' out of bounds for parameter `%s'",
				      v_double,
				      pspec->any.name);
      break;
    case BSE_TYPE_PARAM_DOUBLE:
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
      if (bse_param_set_double (param, v_double))
	return bse_storage_warn_skip (storage,
				      "parameter value `%f' out of bounds for parameter `%s'",
				      v_double,
				      pspec->any.name);
      break;
    case BSE_TYPE_PARAM_TIME:
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
	  if (bse_param_set_time (param, bse_time_from_gmt (time)))
	    {
	      gchar bbuffer[BSE_BBUFFER_SIZE];
	      
	      return bse_storage_warn_skip (storage,
					    "date `%s' out of bounds for parameter `%s'",
					    bse_time_to_bbuffer (bse_time_from_gmt (time), bbuffer),
					    pspec->any.name);
	    }
	}
      break;
    case BSE_TYPE_PARAM_NOTE:
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
      v_note = bse_note_from_string (string);
      if (v_note == BSE_NOTE_UNPARSABLE ||
	  (v_note == BSE_NOTE_VOID &&
	   !pspec->s_note.allow_void))
	return bse_storage_warn_skip (storage,
				      "invalid note definition `%s'",
				      string);
      else if (bse_param_set_note (param, v_note))
	return bse_storage_warn_skip (storage,
				      "note value `%s' out of bounds for parameter `%s'",
				      string,
				      pspec->any.name);
      break;
    case BSE_TYPE_PARAM_INDEX_2D:
      g_scanner_get_next_token (scanner);
      if (scanner->token != G_TOKEN_INT)
	return G_TOKEN_INT;
      if (g_scanner_peek_next_token (scanner) != G_TOKEN_INT)
	{
	  g_scanner_get_next_token (scanner);
	  return G_TOKEN_INT;
	}
      hi = scanner->value.v_int;
      g_scanner_get_next_token (scanner);
      vi = scanner->value.v_int;
      if (bse_param_set_index_2d (param, BSE_INDEX_2D (hi, vi)))
	return bse_storage_warn_skip (storage,
				      "parameter values (`%u' `%u') out of bounds for parameter `%s'",
				      hi,
				      vi,
				      pspec->any.name);
      break;
    case BSE_TYPE_PARAM_STRING:
      g_scanner_get_next_token (scanner);
      if (scanner->token == G_TOKEN_STRING)
	bse_param_set_string (param, scanner->value.v_string);
      else if (scanner->token == BSE_TOKEN_NIL)
	bse_param_set_string (param, NULL); /* FIXME: check validity and free param */
      else
	return G_TOKEN_STRING;
      break;
    case BSE_TYPE_PARAM_ITEM:
      if (g_scanner_get_next_token (scanner) == BSE_TOKEN_NIL)
	bse_param_set_item (param, NULL);
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
	  bse_param_set_item (param, item);

	  g_scanner_get_next_token (scanner);
	}
      break;
    default:
      return bse_storage_warn_skip (storage,
				    "unhandled parameter type `%s' for parameter `%s'",
				    bse_type_name (pspec->type),
				    pspec->any.name);
    }
  
  g_scanner_get_next_token (scanner);
  
  return scanner->token == ')' ? G_TOKEN_NONE : ')';
}
