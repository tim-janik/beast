// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsegconfig.hh"
#include "bseserver.hh"
#include "bsepcmdevice.hh"	/* for frequency alignment */
#include "bse/internal.hh"
#include <sys/types.h>
#include <regex.h>

using namespace Bse;

// == Declarations ==
static BseGConfig*    gconfig_from_rec  (SfiRec *rec);
BseGConfig           *bse_global_config = NULL;
static GParamSpec    *pspec_global_config = NULL;
static uint           gconfig_lock_count = 0;

// == functions ==
void
_bse_gconfig_init (void)
{
  BseGConfig *gconfig;
  GValue *value;
  SfiRec *rec;

  assert_return (bse_global_config == NULL);

  /* global config record description */
  pspec_global_config = sfi_pspec_rec ("bse-preferences", NULL, NULL,
				       bse_gconfig_get_fields (), SFI_PARAM_STANDARD);
  g_param_spec_ref (pspec_global_config);
  g_param_spec_sink (pspec_global_config);
  /* create empty config record */
  rec = sfi_rec_new ();
  value = sfi_value_rec (rec);
  /* fill out missing values with defaults */
  g_param_value_validate (pspec_global_config, value);
  /* install global config */
  gconfig = gconfig_from_rec (rec);
  bse_global_config = gconfig;
  /* cleanup */
  sfi_value_free (value);
  sfi_rec_unref (rec);
}

static std::string
default_author ()
{
  const char *user = g_get_user_name();
  const char *name = g_get_real_name();
  if (name && user && name[0] && strcmp (user, name) != 0)
    return name;
  return "";
}

static std::string
default_license ()
{
  return "Creative Commons Attribution-ShareAlike 4.0 (https://creativecommons.org/licenses/by-sa/4.0/)";
}

static std::string
default_user_path()
{
  return Path::join (Path::user_home(), "Beast");
}

struct Substitution {
  typedef std::string (*SubstituteFunc) ();
  const char    *key = NULL;
  SubstituteFunc sfunc = NULL;
};

static const Substitution substitutions[] = {
  { "$(defaultauthor)",         default_author },
  { "$(defaultlicense)",        default_license },
  { "$(defaultsamplepath)",     [] () { return default_user_path() + "/Samples"; } },
  { "$(defaulteffectpath)",     [] () { return default_user_path() + "/Effects"; } },
  { "$(defaultinstrumentpath)", [] () { return default_user_path() + "/Instruments"; } },
  { "$(defaultpluginpath)",     [] () { return default_user_path() + "/Plugins"; } },
};

static gchar*
substitute_defaults (gchar *input)
{
  assert_return (input, input);
  std::vector<Substitution> subs;
  // simpistic token replacement
  for (uint i = 0; i < ARRAY_SIZE (substitutions); i++)
    if (strcmp (input, substitutions[i].key) == 0)
      {
        g_free (input);
        return g_strdup (substitutions[i].sfunc().c_str());
      }
  return input;
}

static BseGConfig*
gconfig_from_rec (SfiRec *rec)
{
  BseGConfig *cnf = bse_gconfig_from_rec (rec);
  cnf->author_default  = substitute_defaults (cnf->author_default);
  cnf->license_default = substitute_defaults (cnf->license_default);
  cnf->sample_path     = substitute_defaults (cnf->sample_path);
  cnf->effect_path     = substitute_defaults (cnf->effect_path);
  cnf->instrument_path = substitute_defaults (cnf->instrument_path);
  cnf->plugin_path     = substitute_defaults (cnf->plugin_path);
  return cnf;
}

static void
set_gconfig (BseGConfig *gconfig)
{
  BseGConfig *oldconfig = bse_global_config;
  bse_global_config = gconfig;
  bse_gconfig_free (oldconfig);
  if (0)
    {
      SfiRec *prec = bse_gconfig_to_rec (bse_global_config);
      GValue *v = sfi_value_rec (prec);
      GString *gstring = g_string_new (NULL);
      sfi_value_store_param (v, gstring, pspec_global_config, 2);
      printout ("CONFIG:\n%s\n", gstring->str);
      g_string_free (gstring, TRUE);
      sfi_value_free (v);
      sfi_rec_unref (prec);
    }
}

void
bse_gconfig_apply (SfiRec *rec)
{
  assert_return (rec != NULL);

  if (!bse_gconfig_locked ())
    {
      BseGConfig *gconfig;
      SfiRec *vrec = sfi_rec_copy_deep (rec);
      sfi_rec_validate (vrec, sfi_pspec_get_rec_fields (pspec_global_config));
      gconfig = gconfig_from_rec (vrec);
      sfi_rec_unref (vrec);
      set_gconfig (gconfig);
    }
}

GParamSpec*
bse_gconfig_pspec (void)
{
  return pspec_global_config;
}

void
bse_gconfig_lock (void)
{
  gconfig_lock_count++;
  if (gconfig_lock_count == 1)
    bse_server_notify_gconfig (bse_server_get ());
}

void
bse_gconfig_unlock (void)
{
  assert_return (gconfig_lock_count > 0);
  if (gconfig_lock_count)
    {
      gconfig_lock_count--;
      if (!gconfig_lock_count)
	bse_server_notify_gconfig (bse_server_get ());
    }
}

gboolean
bse_gconfig_locked (void)
{
  return gconfig_lock_count != 0;
}

void
bse_gconfig_merge_args (const BseMainArgs *margs)
{
  if (bse_gconfig_locked ())
    return;
  SfiRec *rec = bse_gconfig_to_rec (bse_global_config);
  if (margs->latency > 0)
    sfi_rec_set_int (rec, "synth_latency", margs->latency);
  if (margs->mixing_freq >= 1000)
    sfi_rec_set_int (rec, "synth_mixing_freq", margs->mixing_freq);
  if (margs->control_freq > 0)
    sfi_rec_set_int (rec, "synth_control_freq", margs->control_freq);
  bse_gconfig_apply (rec);
  sfi_rec_unref (rec);
}
