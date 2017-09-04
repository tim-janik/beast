// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstskinconfig.hh"
#include <string.h>


/* --- variables --- */
static Bst::SkinConfig *global_skin_config = NULL;
static GParamSpec      *pspec_skin_config = NULL;


/* --- functions --- */
void
_bst_skin_config_init (void)
{
  GValue *value;
  SfiRec *rec;

  assert_return (global_skin_config == NULL);

  /* global config record description */
  Bst::SkinConfig skin_config;
  pspec_skin_config = sfi_pspec_rec ("beast-skin-config-v1", NULL, NULL,
                                     Bse::sfi_pspecs_rec_fields_from_visitable (skin_config),
                                     SFI_PARAM_STANDARD);
  g_param_spec_ref (pspec_skin_config);
  g_param_spec_sink (pspec_skin_config);
  /* create empty config record */
  rec = sfi_rec_new ();
  value = sfi_value_rec (rec);
  /* fill out missing values with defaults */
  g_param_value_validate (pspec_skin_config, value);
  /* install global config */
  Bse::sfi_rec_to_visitable (rec, skin_config);
  global_skin_config = new Bst::SkinConfig (skin_config);
  /* cleanup */
  sfi_value_free (value);
  sfi_rec_unref (rec);
}

GParamSpec*
bst_skin_config_pspec (void)
{
  return pspec_skin_config;
}

Bst::SkinConfig*
bst_skin_config_get_global (void)
{
  return global_skin_config;
}

static void
set_skin_config (const Bst::SkinConfig &skin_config)
{
  Bst::SkinConfig *oldconfig = global_skin_config;
  global_skin_config = new Bst::SkinConfig (skin_config);
  delete oldconfig;
}

static void
rec_make_absolute_pathnames (SfiRec       *rec,
                             const gchar  *path_prefix,
                             SfiRecFields  fields)
{
  guint i;
  for (i = 0; i < fields.n_fields; i++)
    {
      GParamSpec *pspec = fields.fields[i];
      if (G_IS_PARAM_SPEC_STRING (pspec) &&
          g_param_spec_check_option (pspec, "filename"))
        {
          GValue *value = sfi_rec_get (rec, pspec->name);
          if (value && G_VALUE_HOLDS_STRING (value))
            {
              const gchar *str = (const gchar*) g_value_get_string (value);
              if (str && strlen (str))
                sfi_value_take_string (value, sfi_path_get_filename (str, path_prefix));
            }
        }
      else if (SFI_IS_PSPEC_REC (pspec))
        {
          GValue *value = sfi_rec_get (rec, pspec->name);
          SfiRec *crec = value ? sfi_value_get_rec (value) : NULL;
          if (crec)
            rec_make_absolute_pathnames (crec, path_prefix, sfi_pspec_get_rec_fields (pspec));
        }
    }
}

static void
rec_make_relative_pathnames (SfiRec       *rec,
                             const gchar  *path_prefix,
                             SfiRecFields  fields)
{
  guint i, l = strlen (path_prefix);
  for (i = 0; i < fields.n_fields; i++)
    {
      GParamSpec *pspec = fields.fields[i];
      if (G_IS_PARAM_SPEC_STRING (pspec) &&
          g_param_spec_check_option (pspec, "filename"))
        {
          GValue *value = sfi_rec_get (rec, pspec->name);
          const gchar *str = value && G_VALUE_HOLDS_STRING (value) ? g_value_get_string (value) : NULL;
          if (str && strncmp (str, path_prefix, l) == 0)
            {
              const gchar *s = str + l;
              while (s[0] == G_DIR_SEPARATOR)
                s++;
              sfi_value_take_string (value, g_strdup (s));
            }
        }
      else if (SFI_IS_PSPEC_REC (pspec))
        {
          GValue *value = sfi_rec_get (rec, pspec->name);
          SfiRec *crec = value ? sfi_value_get_rec (value) : NULL;
          if (crec)
            rec_make_relative_pathnames (crec, path_prefix, sfi_pspec_get_rec_fields (pspec));
        }
    }
}

static gchar *skin_path_prefix = NULL;

void
bst_skin_config_apply (SfiRec *rec, const gchar *skin_file)
{
  SfiRec *vrec;
  Bst::SkinConfig skin_config;

  assert_return (rec != NULL);

  vrec = sfi_rec_copy_deep (rec);
  sfi_rec_validate (vrec, sfi_pspec_get_rec_fields (pspec_skin_config));
  if (skin_file)
    {
      gchar *skindir = g_path_get_dirname (skin_file);
      rec_make_absolute_pathnames (vrec, skindir, sfi_pspec_get_rec_fields (pspec_skin_config));
      g_free (skindir);
    }
  Bse::sfi_rec_to_visitable (vrec, skin_config);
  sfi_rec_unref (vrec);
  set_skin_config (skin_config);
  bst_skin_config_notify ();
  if (skin_file)
    {
      g_free (skin_path_prefix);
      skin_path_prefix = g_path_get_dirname (skin_file);
    }
}

const gchar*
bst_skin_config_dirname (void)
{
  return skin_path_prefix;
}

struct SkinNotify {
  BstSkinConfigNotify func;
  gpointer            data;
};

static SkinNotify *notifies = NULL;
static uint        n_notifies = 0;

void
bst_skin_config_add_notify (BstSkinConfigNotify func,
                            gpointer            data)
{
  uint i = n_notifies++;
  notifies = (SkinNotify*) g_realloc (notifies, sizeof (notifies[0]) * n_notifies);
  notifies[i].func = func;
  notifies[i].data = data;
}

void
bst_skin_config_notify (void)
{
  guint i;
  for (i = 0; i < n_notifies; i++)
    notifies[i].func (notifies[i].data);
}


/* --- skin file --- */
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sfi/sfistore.hh>       /* we rely on internal API here */
Bse::Error
bst_skin_dump (const gchar *file_name)
{
  SfiWStore *wstore;
  GValue *value;
  SfiRec *rec;
  gint fd;

  assert_return (file_name != NULL, Bse::Error::INTERNAL);

  sfi_make_dirname_path (file_name);
  fd = open (file_name,
             O_WRONLY | O_CREAT | O_TRUNC, /* O_EXCL, */
             0666);

  if (fd < 0)
    return errno == EEXIST ? Bse::Error::FILE_EXISTS : Bse::Error::IO;

  wstore = sfi_wstore_new ();

  sfi_wstore_printf (wstore, "; skin-file for BEAST v%s\n", Bse::version().c_str());

  /* store BstSkinConfig */
  sfi_wstore_puts (wstore, "\n");
  rec = Bse::sfi_rec_new_from_visitable (*bst_skin_config_get_global ());
  if (1)
    {
      /* make path names relative */
      gchar *dirname = g_path_get_dirname (file_name);
      rec_make_relative_pathnames (rec, dirname, sfi_pspec_get_rec_fields (bst_skin_config_pspec()));
      g_free (dirname);
    }
  value = sfi_value_rec (rec);
  sfi_wstore_put_param (wstore, value, bst_skin_config_pspec());
  sfi_value_free (value);
  sfi_rec_unref (rec);
  sfi_wstore_puts (wstore, "\n");

  /* flush buffers to file */
  sfi_wstore_flush_fd (wstore, fd);
  sfi_wstore_destroy (wstore);

  return close (fd) < 0 ? Bse::Error::IO : Bse::Error::NONE;
}

static GTokenType
skin_file_try_statement (gpointer   context_data,
                         SfiRStore *rstore,
                         GScanner  *scanner,
                         gpointer   user_data)
{
  const char *absname = (const char*) user_data;
  assert_return (scanner->next_token == G_TOKEN_IDENTIFIER, G_TOKEN_ERROR);
  if (strcmp (bst_skin_config_pspec()->name, scanner->next_value.v_identifier) == 0)
    {
      GValue *value = sfi_value_rec (NULL);
      GTokenType token;
      SfiRec *rec;
      g_scanner_get_next_token (rstore->scanner);
      token = sfi_rstore_parse_param (rstore, value, bst_skin_config_pspec());
      rec = sfi_value_get_rec (value);
      if (token == G_TOKEN_NONE && rec)
        bst_skin_config_apply (rec, absname);
      sfi_value_free (value);
      return token;
    }
  else
    return SFI_TOKEN_UNMATCHED;
}

Bse::Error
bst_skin_parse (const gchar *file_name)
{
  SfiRStore *rstore;
  Bse::Error error = Bse::Error::NONE;
  gchar *absname;
  gint fd;

  assert_return (file_name != NULL, Bse::Error::INTERNAL);

  absname = sfi_path_get_filename (file_name, NULL);
  fd = open (absname, O_RDONLY, 0);
  if (fd < 0)
    {
      g_free (absname);
      return (errno == ENOENT || errno == ENOTDIR || errno == ELOOP ?
              Bse::Error::FILE_NOT_FOUND : Bse::Error::IO);
    }

  rstore = sfi_rstore_new ();
  sfi_rstore_input_fd (rstore, fd, absname);
  if (sfi_rstore_parse_all (rstore, NULL, skin_file_try_statement, absname) > 0)
    error = Bse::Error::PARSE_ERROR;
  sfi_rstore_destroy (rstore);
  close (fd);
  g_free (absname);
  return error;
}

static gchar  *skinrc_name = NULL;

void
bst_skin_config_set_rcfile (const gchar *file_name)
{
  /* maybe used before _bst_skin_config_init() */
  if (file_name && strlen (file_name))
    {
      g_free (skinrc_name);
      skinrc_name = sfi_path_get_filename (file_name, NULL);
    }
}

const gchar*
bst_skin_config_rcfile (void)
{
  if (!skinrc_name)
    skinrc_name = sfi_path_get_filename (".beast/skinrc", "~");
  return skinrc_name;
}
