/* BEAST - Bedevilled Audio System
 * Copyright (C) 2004 Tim Janik
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
#include "bstmsgabsorb.h"
#include <string.h>

enum {
  MCOL_CHECK,
  MCOL_BLURB,
  MCOL_VERSION,
  N_MCOLS
};

/* --- variables --- */
static BstMsgAbsorbStringSeq *global_msg_absorb_config = NULL;
static GParamSpec             *pspec_msg_absorb_config = NULL;
static gboolean                msg_absorb_config_loaded = FALSE;

/* --- functions --- */
void
_bst_msg_absorb_config_init (void)
{
  g_return_if_fail (global_msg_absorb_config == NULL);

  /* global config record description */
  pspec_msg_absorb_config = sfi_pspec_seq ("beast-msg-absorb-config-v1", NULL, NULL,
                                           sfi_pspec_rec ("mstring", NULL, NULL, bst_msg_absorb_string_fields, SFI_PARAM_STANDARD),
                                           SFI_PARAM_STANDARD);
  g_param_spec_ref (pspec_msg_absorb_config);
  g_param_spec_sink (pspec_msg_absorb_config);
  /* create empty config record */
  SfiSeq *seq = sfi_seq_new ();
  GValue *value = sfi_value_seq (seq);
  /* fill out missing values with defaults */
  g_param_value_validate (pspec_msg_absorb_config, value);
  /* install global config */
  BstMsgAbsorbStringSeq *mconfig = bst_msg_absorb_string_seq_from_seq (seq);
  global_msg_absorb_config = mconfig;
  /* cleanup */
  sfi_value_free (value);
  sfi_seq_unref (seq);
}

GParamSpec*
bst_msg_absorb_config_pspec (void)
{
  return pspec_msg_absorb_config;
}

BstMsgAbsorbStringSeq*
bst_msg_absorb_config_get_global (void)
{
  if (!msg_absorb_config_loaded)
    bst_msg_absorb_config_load();
  return global_msg_absorb_config;
}

static void
set_msg_absorb_config (BstMsgAbsorbStringSeq *msg_absorb_config)
{
  BstMsgAbsorbStringSeq *oldconfig = global_msg_absorb_config;
  global_msg_absorb_config = msg_absorb_config;
  bst_msg_absorb_string_seq_free (oldconfig);
}

void
bst_msg_absorb_config_apply (SfiSeq *src_seq)
{
  g_return_if_fail (src_seq != NULL);

  SfiSeq *seq = sfi_seq_copy_deep (src_seq);
  sfi_seq_validate (seq, bst_msg_absorb_config_pspec());
  BstMsgAbsorbStringSeq *mconfig = bst_msg_absorb_string_seq_from_seq (seq);
  sfi_seq_unref (seq);
  set_msg_absorb_config (mconfig);
}

# include "topconfig.h" /* BST_VERSION */

gboolean
bst_msg_absorb_config_match (const gchar *config_blurb)
{
  BstMsgAbsorbStringSeq *mstrings = bst_msg_absorb_config_get_global();
  guint i;
  for (i = 0; i < mstrings->n_strings; i++)
    if (strcmp (config_blurb, mstrings->strings[i]->cstring) == 0)
      return !mstrings->strings[i]->enabled;
  return FALSE;
}

void
bst_msg_absorb_config_update (const gchar *config_blurb)
{
  BstMsgAbsorbStringSeq *mstrings = bst_msg_absorb_config_get_global();
  gboolean changed = FALSE;
  guint i;
  for (i = 0; i < mstrings->n_strings; i++)
    if (strcmp (config_blurb, mstrings->strings[i]->cstring) == 0)
      {
        BstMsgAbsorbString *mas = mstrings->strings[i];
        if (strcmp (mas->version, BST_VERSION) != 0)
          {
            g_free (mas->version);
            mas->version = g_strdup (BST_VERSION);
            changed = TRUE;
          }
        break;
      }
  if (changed)
    bst_msg_absorb_config_save();
}

gboolean
bst_msg_absorb_config_adjust (const gchar    *config_blurb,
                              gboolean        enabled,
                              gboolean        update_version)
{
  BstMsgAbsorbStringSeq *mstrings = bst_msg_absorb_config_get_global();
  guint i;
  for (i = 0; i < mstrings->n_strings; i++)
    if (strcmp (config_blurb, mstrings->strings[i]->cstring) == 0)
      break;
  if (i >= mstrings->n_strings)
    {
      BstMsgAbsorbString mas = { 0, };
      mas.version = g_strdup (BST_VERSION);
      mas.cstring = g_strdup (config_blurb);
      mas.enabled = !enabled; /* force change */
      i = mstrings->n_strings;
      bst_msg_absorb_string_seq_append (mstrings, &mas);
    }
  if (mstrings->strings[i]->enabled != enabled ||
      (update_version && strcmp (BST_VERSION, mstrings->strings[i]->version)))
    {
      BstMsgAbsorbString *mas = mstrings->strings[i];
      if (update_version)
        {
          g_free (mas->version);
          mas->version = g_strdup (BST_VERSION);
        }
      mas->enabled = enabled;
      return TRUE;
    }
  return FALSE;
}

static void
msg_absorb_string_seq_fill_value (GtkWidget      *self,
                                  guint           column,
                                  guint           row,
                                  GValue         *value,
                                  GxkListWrapper *lwrapper)
{
  BstMsgAbsorbStringSeq *mass = g_object_get_data (self, "BstMsgAbsorbStringSeq");
  gint i = row;
  if (i >= mass->n_strings || i < 0)
    {
      sfi_value_set_string (value, "<BUG: invalid row count>");
      return;
    }
  switch (column)
    {
    case MCOL_CHECK:
      sfi_value_set_bool (value, mass->strings[i]->enabled);
      break;
    case MCOL_BLURB:
      sfi_value_set_string (value, mass->strings[i]->cstring);
      break;
    case MCOL_VERSION:
      sfi_value_set_string (value, mass->strings[i]->version);
      break;
    }
}

static void
msg_absorb_string_toggled (GtkCellRendererToggle *cell,
                           const gchar           *strpath,
                           gpointer               data)
{
  GxkRadget *self = data;
  BstMsgAbsorbStringSeq *mass = g_object_get_data (self, "BstMsgAbsorbStringSeq");
  gint i = gxk_tree_spath_index0 (strpath);
  if (i >= 0 && i < mass->n_strings)
    {
      mass->strings[i]->enabled = !mass->strings[i]->enabled;
      gtk_cell_renderer_toggle_set_active (cell, mass->strings[i]->enabled);
    }
}

GtkWidget*
bst_msg_absorb_config_box (void)
{
  GxkRadget *self = gxk_radget_create ("beast", "message-absorb-config-box", NULL);
  GtkTreeView *tview = gxk_radget_find (self, "message-tree-view");
  /* setup model */
  GxkListWrapper *lwrapper = gxk_list_wrapper_new (N_MCOLS,
                                                   G_TYPE_BOOLEAN,      /* MCOL_CHECK */
                                                   G_TYPE_STRING,       /* MCOL_BLURB */
                                                   G_TYPE_STRING        /* MCOL_VERSION */
                                                   );
  g_signal_connect_object (lwrapper, "fill-value",
                           G_CALLBACK (msg_absorb_string_seq_fill_value),
                           self, G_CONNECT_SWAPPED);
  gtk_tree_view_set_model (tview, GTK_TREE_MODEL (lwrapper));
  /* setup selection mode */
  GtkTreeSelection *tsel = gtk_tree_view_get_selection (tview);
  gtk_tree_selection_set_mode (tsel, GTK_SELECTION_NONE);
  /* columns */
  GtkTreeViewColumn *col;
  col = gxk_tree_view_add_toggle_column (tview, MCOL_CHECK, "", 0.5, "X",
                                         _("Enable or disable message display of a specific message type"),
                                         msg_absorb_string_toggled, self, G_CONNECT_AFTER);
  col = gxk_tree_view_add_text_column (tview, MCOL_BLURB, "S", 0.0, _("Message Type"),
                                       NULL, // _("The message type selection phrase displayed in message dialogs"),
                                       NULL, NULL, 0);
  col = gxk_tree_view_add_text_column (tview, MCOL_VERSION, "S", 0.0, _("Version"),
                                       _("The last program version that displayed this message type"),
                                       NULL, NULL, 0);
  gtk_tree_view_set_expander_column (tview, col); /* where to put unused expander space */
  return self;
}

void
bst_msg_absorb_config_box_set (GtkWidget             *self,
                               BstMsgAbsorbStringSeq *mass)
{
  GtkTreeView *tview = gxk_radget_find (self, "message-tree-view");
  GtkTreeModel *model = gtk_tree_view_get_model (tview);
  /* copy deep */
  SfiSeq *seq = bst_msg_absorb_string_seq_to_seq (mass);
  mass = bst_msg_absorb_string_seq_from_seq (seq);
  sfi_seq_unref (seq);
  g_object_set_data_full (self, "BstMsgAbsorbStringSeq", mass, bst_key_binding_item_seq_free);
  gxk_list_wrapper_notify_clear (GXK_LIST_WRAPPER (model));
  gxk_list_wrapper_notify_append (GXK_LIST_WRAPPER (model), mass->n_strings);
}

BstMsgAbsorbStringSeq*
bst_msg_absorb_config_box_get (GtkWidget      *self)
{
  BstMsgAbsorbStringSeq *mass = g_object_get_data (self, "BstMsgAbsorbStringSeq");
  return mass;
}

/* --- config file --- */
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "topconfig.h"          /* BST_VERSION */
#include <sfi/sfistore.h>       /* we rely on internal API here */

static BseErrorType
bst_msg_absorb_config_dump (const gchar *file_name)
{
  g_return_val_if_fail (file_name != NULL, BSE_ERROR_INTERNAL);

  sfi_make_dirname_path (file_name);
  gint fd = open (file_name,
                  O_WRONLY | O_CREAT | O_TRUNC, /* O_EXCL, */
                  0666);
  if (fd < 0)
    return errno == EEXIST ? BSE_ERROR_FILE_EXISTS : BSE_ERROR_IO;

  SfiWStore *wstore = sfi_wstore_new ();

  sfi_wstore_printf (wstore, "; message-absorb-config-file for BEAST v%s\n", BST_VERSION);

  /* store config */
  sfi_wstore_puts (wstore, "\n");
  SfiSeq *seq = bst_msg_absorb_string_seq_to_seq (bst_msg_absorb_config_get_global());
  GValue *value = sfi_value_seq (seq);
  sfi_wstore_put_param (wstore, value, bst_msg_absorb_config_pspec());
  sfi_value_free (value);
  sfi_seq_unref (seq);
  sfi_wstore_puts (wstore, "\n");

  /* flush buffers to file */
  sfi_wstore_flush_fd (wstore, fd);
  sfi_wstore_destroy (wstore);

  return close (fd) < 0 ? BSE_ERROR_IO : BSE_ERROR_NONE;
}

void
bst_msg_absorb_config_save (void)
{
  gchar *file_name = BST_STRDUP_ABSORBRC_FILE();
  BseErrorType error = bst_msg_absorb_config_dump (file_name);
  if (error)
    sfi_diag ("Failed to save config-file \"%s\": %s", file_name, bse_error_blurb (error));
  g_free (file_name);
}

static SfiTokenType
msg_absorb_config_try_statement (gpointer   context_data,
                                 SfiRStore *rstore,
                                 GScanner  *scanner,
                                 gpointer   user_data)
{
  g_assert (scanner->next_token == G_TOKEN_IDENTIFIER);
  if (strcmp (bst_msg_absorb_config_pspec()->name, scanner->next_value.v_identifier) == 0)
    {
      GValue *value = sfi_value_seq (NULL);
      g_scanner_get_next_token (rstore->scanner);
      GTokenType token = sfi_rstore_parse_param (rstore, value, bst_msg_absorb_config_pspec());
      SfiSeq *seq = sfi_value_get_seq (value);
      if (token == G_TOKEN_NONE && seq)
        bst_msg_absorb_config_apply (seq);
      sfi_value_free (value);
      return token;
    }
  else
    return SFI_TOKEN_UNMATCHED;
}

static BseErrorType
bst_msg_absorb_config_parse (const gchar *file_name)
{
  g_return_val_if_fail (file_name != NULL, BSE_ERROR_INTERNAL);

  gchar *absname = sfi_path_get_filename (file_name, NULL);
  gint fd = open (absname, O_RDONLY, 0);
  if (fd < 0)
    {
      g_free (absname);
      return (errno == ENOENT || errno == ENOTDIR || errno == ELOOP ?
              BSE_ERROR_FILE_NOT_FOUND : BSE_ERROR_IO);
    }

  SfiRStore *rstore = sfi_rstore_new ();
  sfi_rstore_input_fd (rstore, fd, absname);
  BseErrorType error = BSE_ERROR_NONE;
  if (sfi_rstore_parse_all (rstore, NULL, msg_absorb_config_try_statement, absname) > 0)
    error = BSE_ERROR_PARSE_ERROR;
  sfi_rstore_destroy (rstore);
  close (fd);
  g_free (absname);
  return error;
}

static gboolean
parse_version (const gchar *version,
               glong       *vmajorp,
               glong       *vminorp,
               glong       *vmicrop)
{
  gchar *vstring = g_strdup (version);
  gchar *pminor = strchr (vstring, '.');
  gchar *pmicro = !pminor ? NULL : strchr (pminor + 1, '.');
  if (pmicro)
    {
      glong vmajor, vminor = -1, vmicro = -1;
      *pminor++ = 0;
      *pmicro++ = 0;
      gchar *ep = NULL;
      vmajor = strtol (vstring, &ep, 10);
      if (!ep || *ep == 0)
        vminor = strtol (pminor, &ep, 10);
      if (!ep || *ep == 0)
        vmicro = strtol (pmicro, &ep, 10);
      if ((!ep || *ep == 0 || ep > pmicro) && vmajor >= 0 && vminor >= 0 && vmicro >= 0)
        {
          *vmajorp = vmajor;
          *vminorp = vminor;
          *vmicrop = vmicro;
          g_free (vstring);
          return TRUE;
        }
    }
  g_free (vstring);
  return FALSE;
}

static gint
string_versions_compare (const gchar *version1,
                         const gchar *version2)
{
  glong vmajor1 = 0, vminor1 = 0, vmicro1 = 0, vmajor2 = 0, vminor2 = 0, vmicro2 = 0;
  parse_version (version1, &vmajor1, &vminor1, &vmicro1);
  parse_version (version2, &vmajor2, &vminor2, &vmicro2);
  return ((vmajor1 != vmajor2) ? (vmajor1 > vmajor2 ? +1 : -1) :
          (vminor1 != vminor2) ? (vminor1 > vminor2 ? +1 : -1) :
          (vmicro1 < vmicro2 ? -1 : vmicro1 > vmicro2));
}

void
bst_msg_absorb_config_load (void)
{
  gchar *file_name = BST_STRDUP_ABSORBRC_FILE();
  BseErrorType error = bst_msg_absorb_config_parse (file_name);
  if (0 && error)
    sfi_diag ("Failed to load config-file \"%s\": %s", file_name, bse_error_blurb (error));
  g_free (file_name);
  msg_absorb_config_loaded = TRUE;
  /* filter aged strings */
  const gchar *min_version = "0.6.3";
  BstMsgAbsorbStringSeq *mstrings = bst_msg_absorb_config_get_global();
  guint i = 0;
  while (i < mstrings->n_strings)
    if (string_versions_compare (min_version, mstrings->strings[i]->version) <= 0)
      i++;
    else
      {
        BstMsgAbsorbString *mas = mstrings->strings[i];
        mstrings->n_strings--;
        mstrings->strings[i] = mstrings->strings[mstrings->n_strings];
        bst_msg_absorb_string_free (mas);
      }
}
