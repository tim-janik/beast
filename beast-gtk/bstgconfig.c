/* BEAST - Bedevilled Audio System
 * Copyright (C) 1999-2002 Tim Janik and Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include	"bstgconfig.h"


/* --- variables --- */
BstGConfig        *bst_global_config = NULL;
static GParamSpec *pspec_global_config = NULL;


/* --- functions --- */
void
_bst_gconfig_init (void)
{
  BstGConfig *gconfig;
  GValue *value;
  SfiRec *rec;

  g_return_if_fail (bst_global_config == NULL);

  /* global config record description */
  pspec_global_config = sfi_pspec_rec ("beast-preferences", NULL, NULL,
				       bst_gconfig_fields, SFI_PARAM_DEFAULT);
  g_param_spec_ref (pspec_global_config);
  g_param_spec_sink (pspec_global_config);
  /* create empty config record */
  rec = sfi_rec_new ();
  value = sfi_value_rec (rec);
  /* fill out missing values with defaults */
  g_param_value_validate (pspec_global_config, value);
  /* install global config */
  gconfig = bst_gconfig_from_rec (rec);
  bst_global_config = gconfig;
  /* cleanup */
  sfi_value_free (value);
  sfi_rec_unref (rec);
}

GParamSpec*
bst_gconfig_pspec (void)
{
  return pspec_global_config;
}

static BstGConfig*
copy_gconfig (BstGConfig *src_config)
{
  SfiRec *rec = bst_gconfig_to_rec (src_config);
  BstGConfig *gconfig = bst_gconfig_from_rec (rec);
  sfi_rec_unref (rec);
  return gconfig;
}

static void
set_gconfig (BstGConfig *gconfig)
{
  BstGConfig *oldconfig = bst_global_config;
  bst_global_config = gconfig;
  bst_gconfig_free (oldconfig);
  if (0)
    {
      SfiRec *prec = bst_gconfig_to_rec (bst_global_config);
      GValue *v = sfi_value_rec (prec);
      GString *gstring = g_string_new (NULL);
      sfi_value_store_param (v, gstring, pspec_global_config, 2);
      g_print ("CONFIG:\n%s\n", gstring->str);
      g_string_free (gstring, TRUE);
      sfi_value_free (v);
      sfi_rec_unref (prec);
    }
}

void
bst_gconfig_apply (SfiRec *rec)
{
  SfiRec *vrec;
  BstGConfig *gconfig;

  g_return_if_fail (rec != NULL);

  vrec = sfi_rec_copy_deep (rec);
  sfi_rec_validate (vrec, sfi_pspec_get_rec_fields (pspec_global_config));
  gconfig = bst_gconfig_from_rec (vrec);
  sfi_rec_unref (vrec);
  set_gconfig (gconfig);
}

void
bst_gconfig_set_rc_version (const gchar *rc_version)
{
  BstGConfig *gconfig;

  gconfig = copy_gconfig (bst_global_config);
  g_free (gconfig->rc_version);
  gconfig->rc_version = g_strdup (rc_version);
  set_gconfig (gconfig);
}


/* --- loading and saving rc file --- */
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "../PKG_config.h"	/* BST_VERSION */
#include <sfi/sfistore.h>	/* we rely on internal API here */
static void
accel_map_print (gpointer        data,
		 const gchar    *accel_path,
		 guint           accel_key,
		 guint           accel_mods,
		 gboolean        changed)
{
  GString *gstring = g_string_new (changed ? NULL : "; ");
  SfiWStore *wstore = data;
  gchar *tmp, *name;

  g_string_append (gstring, "(gtk_accel_path \"");

  tmp = g_strescape (accel_path, NULL);
  g_string_append (gstring, tmp);
  g_free (tmp);

  g_string_append (gstring, "\" \"");

  name = gtk_accelerator_name (accel_key, accel_mods);
  tmp = g_strescape (name, NULL);
  g_free (name);
  g_string_append (gstring, tmp);
  g_free (tmp);

  g_string_append (gstring, "\")");

  sfi_wstore_break (wstore);
  sfi_wstore_puts (wstore, gstring->str);

  g_string_free (gstring, TRUE);
}

BseErrorType
bst_rc_dump (const gchar *file_name)
{
  SfiWStore *wstore;
  GValue *value;
  SfiRec *rec;
  gint fd;

  g_return_val_if_fail (file_name != NULL, BSE_ERROR_INTERNAL);

  fd = open (file_name,
	     O_WRONLY | O_CREAT | O_TRUNC, /* O_EXCL, */
	     0666);

  if (fd < 0)
    return errno == EEXIST ? BSE_ERROR_EXISTS : BSE_ERROR_IO;

  wstore = sfi_wstore_new ();

  sfi_wstore_printf (wstore, "; rc-file for BEAST v%s\n", BST_VERSION);

  /* store BstGConfig */
  sfi_wstore_puts (wstore, "\n; BstGConfig Dump\n");
  rec = bst_gconfig_to_rec (bst_global_config);
  value = sfi_value_rec (rec);
  sfi_wstore_put_param (wstore, value, bst_gconfig_pspec ());
  sfi_value_free (value);
  sfi_rec_unref (rec);
  sfi_wstore_puts (wstore, "\n");

  /* store accelerator paths */
  sfi_wstore_puts (wstore, "\n; Gtk+ Accel Map Path Dump\n");
  sfi_wstore_puts (wstore, "(menu-accelerators ");
  sfi_wstore_push_level (wstore);
  gtk_accel_map_foreach (wstore, accel_map_print);
  sfi_wstore_break (wstore);	/* make sure this is no comment line */
  sfi_wstore_puts (wstore, ")\n");
  sfi_wstore_pop_level (wstore);

  /* flush stuff to rc file */
  sfi_wstore_flush_fd (wstore, fd);
  sfi_wstore_destroy (wstore);

  return close (fd) < 0 ? BSE_ERROR_IO : BSE_ERROR_NONE;
}

static SfiTokenType
rc_file_try_statement (gpointer   context_data,
		       SfiRStore *rstore,
		       GScanner  *scanner,
		       gpointer   user_data)
{
  g_assert (scanner->next_token == G_TOKEN_IDENTIFIER);
  if (strcmp ("beast-preferences", scanner->next_value.v_identifier) == 0)
    {
      GValue *value = sfi_value_rec (NULL);
      GTokenType token;
      SfiRec *rec;
      g_scanner_get_next_token (rstore->scanner);
      token = sfi_rstore_parse_param (rstore, value, bst_gconfig_pspec ());
      rec = sfi_value_get_rec (value);
      if (token == G_TOKEN_NONE && rec)
	bst_gconfig_apply (rec);
      sfi_value_free (value);
      return token;
    }
  else if (strcmp ("menu-accelerators", scanner->next_value.v_identifier) == 0)
    {
      g_scanner_get_next_token (rstore->scanner); /* eat identifier */
      gtk_accel_map_load_scanner (scanner);
      if (g_scanner_get_next_token (scanner) != ')')
	return ')';
      else
	return G_TOKEN_NONE;
    }
  else
    return SFI_TOKEN_UNMATCHED;
}

BseErrorType
bst_rc_parse (const gchar *file_name)
{
  SfiRStore *rstore;
  BseErrorType error = BSE_ERROR_NONE;
  gint fd;

  g_return_val_if_fail (file_name != NULL, BSE_ERROR_INTERNAL);

  fd = open (file_name, O_RDONLY, 0);
  if (fd < 0)
    return (errno == ENOENT || errno == ENOTDIR || errno == ELOOP ?
	    BSE_ERROR_NOT_FOUND : BSE_ERROR_IO);

  rstore = sfi_rstore_new ();
  sfi_rstore_input_fd (rstore, fd, file_name);
  if (sfi_rstore_parse_all (rstore, NULL, rc_file_try_statement, NULL) > 0)
    error = BSE_ERROR_PARSE_ERROR;
  sfi_rstore_destroy (rstore);
  close (fd);
  return error;
}
