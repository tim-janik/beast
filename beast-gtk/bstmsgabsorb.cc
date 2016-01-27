// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstmsgabsorb.hh"
#include <string.h>

enum {
  MCOL_CHECK,
  MCOL_BLURB,
  MCOL_VERSION,
  N_MCOLS
};

/* --- variables --- */
static Bst::MsgAbsorbStringSeq *global_msg_absorb_config = NULL;
static GParamSpec              *pspec_msg_absorb_config = NULL;
static gboolean                 msg_absorb_config_loaded = FALSE;

/* --- functions --- */
void
_bst_msg_absorb_config_init (void)
{
  assert_return (global_msg_absorb_config == NULL);

  /* global config record description */
  Bst::MsgAbsorbStringSeq mconfig;
  GParamSpec *field_pspec = Bse::sfi_pspec_seq_field_from_visitable (mconfig);
  pspec_msg_absorb_config = sfi_pspec_seq ("beast-msg-absorb-config-v1", NULL, NULL,
                                           sfi_pspec_rec ("mstring", NULL, NULL,
                                                          SFI_PSPEC_REC (field_pspec)->fields,
                                                          SFI_PARAM_STANDARD),
                                           SFI_PARAM_STANDARD);
  g_param_spec_ref (pspec_msg_absorb_config);
  g_param_spec_sink (pspec_msg_absorb_config);
  /* create empty config record */
  SfiSeq *seq = sfi_seq_new ();
  GValue *value = sfi_value_seq (seq);
  /* fill out missing values with defaults */
  g_param_value_validate (pspec_msg_absorb_config, value);
  sfi_value_free (value);
  /* install global config */
  Bse::sfi_seq_to_visitable (seq, mconfig);
  sfi_seq_unref (seq);
  global_msg_absorb_config = new Bst::MsgAbsorbStringSeq (mconfig);
}

GParamSpec*
bst_msg_absorb_config_pspec (void)
{
  return pspec_msg_absorb_config;
}

Bst::MsgAbsorbStringSeq*
bst_msg_absorb_config_get_global (void)
{
  if (!msg_absorb_config_loaded)
    bst_msg_absorb_config_load();
  return global_msg_absorb_config;
}

static void
set_msg_absorb_config (const Bst::MsgAbsorbStringSeq &msg_absorb_config)
{
  Bst::MsgAbsorbStringSeq *oldconfig = global_msg_absorb_config;
  global_msg_absorb_config = new Bst::MsgAbsorbStringSeq (msg_absorb_config);
  delete oldconfig;
}

void
bst_msg_absorb_config_apply (SfiSeq *src_seq)
{
  assert_return (src_seq != NULL);

  SfiSeq *seq = sfi_seq_copy_deep (src_seq);
  sfi_seq_validate (seq, bst_msg_absorb_config_pspec());
  Bst::MsgAbsorbStringSeq mconfig;
  Bse::sfi_seq_to_visitable (seq, mconfig);
  sfi_seq_unref (seq);
  set_msg_absorb_config (mconfig);
}

gboolean
bst_msg_absorb_config_match (const gchar *config_blurb)
{
  const Bst::MsgAbsorbStringSeq &mstrings = *bst_msg_absorb_config_get_global();
  for (size_t i = 0; i < mstrings.size(); i++)
    if (mstrings[i].cstring == config_blurb)
      return !mstrings[i].enabled;
  return FALSE;
}

void
bst_msg_absorb_config_update (const gchar *config_blurb)
{
  Bst::MsgAbsorbStringSeq &mstrings = *bst_msg_absorb_config_get_global();
  bool changed = false;
  for (size_t i = 0; i < mstrings.size(); i++)
    if (mstrings[i].cstring == config_blurb)
      {
        Bst::MsgAbsorbString &mas = mstrings[i];
        if (mas.version != Bse::version())
          {
            mas.version = Bse::version();
            changed = TRUE;
          }
        break;
      }
  if (changed)
    bst_msg_absorb_config_save();
}

gboolean
bst_msg_absorb_config_adjust (const gchar *config_blurb, bool enabled, bool update_version)
{
  Bst::MsgAbsorbStringSeq &mstrings = *bst_msg_absorb_config_get_global();
  size_t i;
  for (i = 0; i < mstrings.size(); i++)
    if (mstrings[i].cstring == config_blurb)
      break;
  if (i >= mstrings.size())
    {
      Bst::MsgAbsorbString mas;
      mas.version = Bse::version();
      mas.cstring = config_blurb;
      mas.enabled = !enabled; // forces change
      i = mstrings.size();
      mstrings.push_back (mas);
    }
  if (mstrings[i].enabled != enabled || (update_version && Bse::version() != mstrings[i].version))
    {
      Bst::MsgAbsorbString &mas = mstrings[i];
      if (update_version)
        mas.version = Bse::version();
      mas.enabled = enabled;
      return true;
    }
  return false;
}

static void
msg_absorb_string_seq_fill_value (GtkWidget *self, guint column, guint row, GValue *value, GxkListWrapper *lwrapper)
{
  Bst::MsgAbsorbStringSeq *mass = (Bst::MsgAbsorbStringSeq*) g_object_get_data ((GObject*) self, "BstMsgAbsorbStringSeq");
  size_t i = row;
  if (i >= mass->size())
    {
      sfi_value_set_string (value, "<BUG: invalid row count>");
      return;
    }
  switch (column)
    {
    case MCOL_CHECK:
      sfi_value_set_bool (value, (*mass)[i].enabled);
      break;
    case MCOL_BLURB:
      sfi_value_set_string (value, (*mass)[i].cstring.c_str());
      break;
    case MCOL_VERSION:
      sfi_value_set_string (value, (*mass)[i].version.c_str());
      break;
    }
}

static void
msg_absorb_string_toggled (GtkCellRendererToggle *cell, const gchar *strpath, gpointer data)
{
  GxkRadget *self = data;
  Bst::MsgAbsorbStringSeq *mass = (Bst::MsgAbsorbStringSeq*) g_object_get_data ((GObject*) self, "BstMsgAbsorbStringSeq");
  ssize_t i = gxk_tree_spath_index0 (strpath);
  if (i >= 0 && size_t (i) < mass->size())
    {
      (*mass)[i].enabled = !(*mass)[i].enabled;
      gtk_cell_renderer_toggle_set_active (cell, (*mass)[i].enabled);
    }
}

GtkWidget*
bst_msg_absorb_config_box (void)
{
  GxkRadget *self = gxk_radget_create ("beast", "message-absorb-config-box", NULL);
  GtkTreeView *tview = (GtkTreeView*) gxk_radget_find (self, "message-tree-view");
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
                                         (void*) msg_absorb_string_toggled, self, G_CONNECT_AFTER);
  col = gxk_tree_view_add_text_column (tview, MCOL_BLURB, "S", 0.0, _("Message Type"),
                                       NULL, // _("The message type selection phrase displayed in message dialogs"),
                                       NULL, NULL, GConnectFlags (0));
  col = gxk_tree_view_add_text_column (tview, MCOL_VERSION, "S", 0.0, _("Version"),
                                       _("The last program version that displayed this message type"),
                                       NULL, NULL, GConnectFlags (0));
  gtk_tree_view_set_expander_column (tview, col); /* where to put unused expander space */
  return (GtkWidget*) self;
}

void
bst_msg_absorb_config_box_set (GtkWidget *self, Bst::MsgAbsorbStringSeq *mconfig_seq)
{
  GtkTreeView *tview = (GtkTreeView*) gxk_radget_find (self, "message-tree-view");
  GtkTreeModel *model = gtk_tree_view_get_model (tview);
  /* copy deep */
  Bst::MsgAbsorbStringSeq *mass = new Bst::MsgAbsorbStringSeq (*mconfig_seq);
  g_object_set_data_full ((GObject*) self, "BstMsgAbsorbStringSeq", mass,
                          [] (void *p) { delete (Bst::MsgAbsorbStringSeq*) p; });
  gxk_list_wrapper_notify_clear (GXK_LIST_WRAPPER (model));
  gxk_list_wrapper_notify_append (GXK_LIST_WRAPPER (model), mass->size());
}

Bst::MsgAbsorbStringSeq*
bst_msg_absorb_config_box_get (GtkWidget *self)
{
  return (Bst::MsgAbsorbStringSeq*) g_object_get_data ((GObject*) self, "BstMsgAbsorbStringSeq");
}

/* --- config file --- */
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sfi/sfistore.hh>       /* we rely on internal API here */

static Bse::ErrorType
bst_msg_absorb_config_dump (const gchar *file_name)
{
  assert_return (file_name != NULL, Bse::Error::INTERNAL);

  sfi_make_dirname_path (file_name);
  gint fd = open (file_name,
                  O_WRONLY | O_CREAT | O_TRUNC, /* O_EXCL, */
                  0666);
  if (fd < 0)
    return errno == EEXIST ? Bse::Error::FILE_EXISTS : Bse::Error::IO;

  SfiWStore *wstore = sfi_wstore_new ();

  sfi_wstore_printf (wstore, "; message-absorb-config-file for BEAST v%s\n", Bse::version().c_str());

  /* store config */
  sfi_wstore_puts (wstore, "\n");
  SfiSeq *seq = Bse::sfi_seq_new_from_visitable (*bst_msg_absorb_config_get_global());
  GValue *value = sfi_value_seq (seq);
  sfi_wstore_put_param (wstore, value, bst_msg_absorb_config_pspec());
  sfi_value_free (value);
  sfi_seq_unref (seq);
  sfi_wstore_puts (wstore, "\n");

  /* flush buffers to file */
  sfi_wstore_flush_fd (wstore, fd);
  sfi_wstore_destroy (wstore);

  return close (fd) < 0 ? Bse::Error::IO : Bse::Error::NONE;
}

void
bst_msg_absorb_config_save (void)
{
  gchar *file_name = BST_STRDUP_ABSORBRC_FILE();
  Bse::ErrorType error = bst_msg_absorb_config_dump (file_name);
  if (error)
    sfi_diag ("Failed to save config-file \"%s\": %s", file_name, Bse::error_blurb (error));
  g_free (file_name);
}

static GTokenType
msg_absorb_config_try_statement (gpointer   context_data,
                                 SfiRStore *rstore,
                                 GScanner  *scanner,
                                 gpointer   user_data)
{
  assert (scanner->next_token == G_TOKEN_IDENTIFIER);
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

static Bse::ErrorType
bst_msg_absorb_config_parse (const gchar *file_name)
{
  assert_return (file_name != NULL, Bse::Error::INTERNAL);

  gchar *absname = sfi_path_get_filename (file_name, NULL);
  gint fd = open (absname, O_RDONLY, 0);
  if (fd < 0)
    {
      g_free (absname);
      return (errno == ENOENT || errno == ENOTDIR || errno == ELOOP ?
              Bse::Error::FILE_NOT_FOUND : Bse::Error::IO);
    }

  SfiRStore *rstore = sfi_rstore_new ();
  sfi_rstore_input_fd (rstore, fd, absname);
  Bse::ErrorType error = Bse::Error::NONE;
  if (sfi_rstore_parse_all (rstore, NULL, msg_absorb_config_try_statement, absname) > 0)
    error = Bse::Error::PARSE_ERROR;
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
string_versions_compare (const gchar *version1, const gchar *version2)
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
  Bse::ErrorType error = bst_msg_absorb_config_parse (file_name);
  if (0 && error)
    sfi_diag ("Failed to load config-file \"%s\": %s", file_name, Bse::error_blurb (error));
  g_free (file_name);
  msg_absorb_config_loaded = TRUE;
  /* filter aged strings */
  const gchar *min_version = "0.6.3";
  Bst::MsgAbsorbStringSeq &mstrings = *bst_msg_absorb_config_get_global();
  size_t i = 0;
  while (i < mstrings.size())
    if (string_versions_compare (min_version, mstrings[i].version.c_str()) <= 0)
      i++;
    else
      mstrings.erase (mstrings.begin() + i);
}
