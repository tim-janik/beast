/* scankeys - simplistic key scanner
 * Copyright (C) 2003 Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include <glib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


/* --- job definitions --- */
enum {
  JOB_SKIP_KEYS,
  JOB_SCAN_KEYS,
};


/* --- variables and global setup --- */
static GHashTable  *match_key_ht = NULL;
static GScanner    *scanner = NULL;
static guint        job = JOB_SCAN_KEYS;
static GHashTable  *need_key_ht = NULL;
static GHashTable  *skip_key_ht = NULL;
static const gchar *template_start = "";
static const gchar *template_key   = "%s\n";
static const gchar *template_end   = "";
static guint        n_printed = 0;
static gboolean	    count_keys = FALSE;


/* --- scanner configuration --- */
static GScannerConfig scanner_config =
{
  (
   " \t\r\n"
   )			/* cset_skip_characters */,
  (
   G_CSET_a_2_z
   "_"
   G_CSET_A_2_Z
   )			/* cset_identifier_first */,
  (
   G_CSET_a_2_z
   "_"
   G_CSET_A_2_Z
   G_CSET_DIGITS
   )			/* cset_identifier_nth */,
  ( 0 /*"#\n"*/ )	/* cpair_comment_single */,

  FALSE			/* case_sensitive */,

  TRUE			/* skip_comment_multi */,
  TRUE			/* skip_comment_single */,
  TRUE			/* scan_comment_multi */,
  TRUE			/* scan_identifier */,
  TRUE			/* scan_identifier_1char */,
  FALSE			/* scan_identifier_NULL */,
  FALSE			/* scan_symbols */,
  FALSE			/* scan_binary */,
  FALSE			/* scan_octal */,
  FALSE			/* scan_float */,
  TRUE			/* scan_hex */,
  FALSE			/* scan_hex_dollar */,
  FALSE			/* scan_string_sq */,
  TRUE			/* scan_string_dq */,
  FALSE			/* numbers_2_int */,
  FALSE			/* int_2_float */,
  FALSE			/* identifier_2_string */,
  TRUE			/* char_2_token */,
  FALSE			/* symbol_2_token */,
  FALSE			/* scope_0_fallback */,
  FALSE			/* store_int64 */,
};


/* --- functions --- */
static void
scan_file (const gchar *file)
{
  gint fd = strcmp (file, "-") ? open (file, O_RDONLY, 0) : 0;
  if (fd >= 0)
    {
      GTokenType token;
      g_scanner_input_file (scanner, fd);
      scanner->input_name = file;
      token = g_scanner_get_next_token (scanner);
      while (token != G_TOKEN_EOF) // !g_scanner_eof (scanner))
	{
#if 0	// shut up
	  if (token == G_TOKEN_ERROR)
	    g_scanner_unexp_token (scanner, G_TOKEN_IDENTIFIER, "identifier", "symbol", NULL, NULL, 0);
#endif
	  if (token == G_TOKEN_IDENTIFIER &&
	      g_scanner_peek_next_token (scanner) == '(' &&
	      g_hash_table_lookup (match_key_ht, scanner->value.v_identifier) &&
	      g_scanner_get_next_token (scanner) == '(' &&
	      g_scanner_get_next_token (scanner) == G_TOKEN_STRING &&
	      (g_scanner_peek_next_token (scanner) == ')' ||
	       scanner->next_token == ','))
	    {
	      gchar *key = g_strdup (scanner->value.v_string);
	      switch (job)
		{
		case JOB_SKIP_KEYS:
		  g_hash_table_replace (skip_key_ht, key, key);
		  break;
		case JOB_SCAN_KEYS:
		  g_hash_table_replace (need_key_ht, key, key);
		  break;
		}
	    }
	  token = g_scanner_get_next_token (scanner);
	}
    }
  if (strcmp (file, "-"))
    close (fd);
}

static void
print_keys (gpointer  key,
	    gpointer  value,
	    gpointer  user_data)
{
  gchar *word;
  guint i;
  if (g_hash_table_lookup (skip_key_ht, key) != NULL)
    return;
  if (n_printed == 0)
    g_print ("%s", template_start);
  word = g_strescape (value, NULL);
  for (i = 0; template_key[i]; i++)
    if (template_key[i] == '%')
      switch (template_key[++i])
	{
	case 's':
	  g_print ("\"%s\"", word);
	  break;
	case 'w':
	  g_print ("%s", (const gchar*) value);
	  break;
	default:
	  g_print ("%c", template_key[i]);
	  break;
	}
    else
      g_print ("%c", template_key[i]);
  g_free (word);
  n_printed++;
}

int
main (int   argc,
      char *argv[])
{
  guint i;

  scanner = g_scanner_new (&scanner_config);
  match_key_ht = g_hash_table_new (g_str_hash, g_str_equal);
  skip_key_ht = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, g_free);
  need_key_ht = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, g_free);
  for (i = 1; i < argc; i++)
    {
      if (strcmp ("--skipkeys", argv[i]) == 0)
	job = JOB_SKIP_KEYS;
      else if (strcmp ("--needkeys", argv[i]) == 0)
	job = JOB_SCAN_KEYS;
      else if (strcmp ("--count", argv[i]) == 0)
	count_keys = TRUE;
      else if (strcmp ("--key", argv[i]) == 0 && i + 1 < argc)
	{
	  gchar *key = argv[++i];
	  g_hash_table_insert (match_key_ht, key, key);
	}
      else if (strcmp ("--tmpl", argv[i]) == 0 && i + 1 < argc)
	template_key = g_strcompress (argv[++i]);
      else if (strcmp ("--tmplkey", argv[i]) == 0 && i + 1 < argc)
	template_key = g_strcompress (argv[++i]);
      else if (strcmp ("--tmplstart", argv[i]) == 0 && i + 1 < argc)
	template_start = g_strcompress (argv[++i]);
      else if (strcmp ("--tmplend", argv[i]) == 0 && i + 1 < argc)
	template_end = g_strcompress (argv[++i]);
      else
	scan_file (argv[i]);
    }

  g_hash_table_foreach (need_key_ht, print_keys, NULL);
  if (n_printed)
    g_print ("%s", template_end);

  return count_keys ? n_printed != 0 : 0;
}
