// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsegconfig.hh"
#include "bseserver.hh"
#include "bsepcmdevice.hh"	/* for frequency alignment */
#include "data/config-paths.h"
#include <sys/types.h>
#include <regex.h>

using namespace Birnet;

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
  g_return_if_fail (bse_global_config == NULL);
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

static const char*
intern_path_user_data ()
{
  return g_intern_strconcat (BSE_PATH_USER_DATA ("/"), "", NULL);
}

static const char*
intern_default_author ()
{
  const char *user = g_get_user_name();
  const char *name = g_get_real_name();
  if (name && user && name[0] && strcmp (user, name) != 0)
    return g_intern_string (name);
  return g_intern_static_string ("");
}

static const char*
intern_default_license ()
{
  return g_intern_static_string ("Creative Commons Attribution 2.5 (http://creativecommons.org/licenses/by/2.5/)");
}

struct Substitutions {
  typedef const char* (*SubstituteFunc) ();
  const char *key;
  SubstituteFunc sfunc;
};

static const Substitutions subs[] = {
  { "bse.idl/default-author",   intern_default_author },
  { "bse.idl/default-license",  intern_default_license },
  { "bse.idl/user-data-path",   intern_path_user_data },
  { NULL, NULL, },
};

static char*
expand_sub14 (char *gstr)
{
  g_return_val_if_fail (gstr, gstr);
  static regex_t preg = { 0, };
  int rc;
  if (UNLIKELY (!preg.re_nsub))
    {
      // MATCH: "\uFFF9\u001A\uFFFA{{...}}\uFFFB" - annotated SUB with contents: {{...}}
      // UTF-8: "\xef\xbf\xb9\x1a\xef\xbf\xba{{...}}\xef\xbf\xbb";
      const char *pattern = "\357\277\271\357\277\272\\{\\{([^{}]*)\\}\\}\357\277\273";
      rc = regcomp (&preg, pattern, REG_EXTENDED); // FIXME: should be atomic
      g_assert (rc == 0 && preg.re_nsub);
      // if non-static: regfree (&preg);
    }
  regmatch_t pm[2] = { { 0, }, };
  int so = 0;
  rc = regexec (&preg, gstr + so, 2, pm, 0);
  if (rc != 0)
    return gstr; // no match
  String result;
  for ( ; rc == 0; so = pm[0].rm_eo, rc = regexec (&preg, gstr + so, 2, pm, REG_NOTBOL))
    {
      if (pm[0].rm_so > so)
        result += String (gstr + so, pm[0].rm_so - so);
      int i, l = pm[1].rm_eo - pm[1].rm_so;
      for (i = 0; subs[i].key; i++)
        if (strncmp (subs[i].key, gstr + pm[1].rm_so, l) == 0 && 0 == subs[i].key[l])
          {
            result += subs[i].sfunc();
            i = -1;
            break;
          }
      if (i != -1)
        result += "{{" + String (gstr + pm[1].rm_so, l) + "}}";
    }
  if (gstr[so] != 0)
    result += gstr + so;
  g_free (gstr);
  return g_strdup (result.c_str());
}

static BseGConfig*
gconfig_from_rec (SfiRec *rec)
{
  BseGConfig *cnf = bse_gconfig_from_rec (rec);
  cnf->author_default  = expand_sub14 (cnf->author_default);
  cnf->license_default = expand_sub14 (cnf->license_default);
  cnf->sample_path     = expand_sub14 (cnf->sample_path);
  cnf->effect_path     = expand_sub14 (cnf->effect_path);
  cnf->instrument_path = expand_sub14 (cnf->instrument_path);
  cnf->script_path     = expand_sub14 (cnf->script_path);
  cnf->plugin_path     = expand_sub14 (cnf->plugin_path);
  cnf->ladspa_path     = expand_sub14 (cnf->ladspa_path);
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
      g_print ("CONFIG:\n%s\n", gstring->str);
      g_string_free (gstring, TRUE);
      sfi_value_free (v);
      sfi_rec_unref (prec);
    }
}
void
bse_gconfig_apply (SfiRec *rec)
{
  g_return_if_fail (rec != NULL);
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
  g_return_if_fail (gconfig_lock_count > 0);
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
