/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-2002 Tim Janik
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
#include	"bsegconfig.h"
#include	"bseserver.h"
#include	"bsepcmdevice.h"	/* for frequency alignment */



/* --- variables --- */
BseGConfig        *bse_global_config = NULL;
static GParamSpec *pspec_global_config = NULL;
static guint       gconfig_lock_count = 0;


/* --- functions --- */
void
_bse_gconfig_init (void)
{
  BseGConfig *gconfig;
  GValue *value;
  SfiRec *rec;

  g_return_if_fail (bse_global_config == NULL);

  /* global config record description */
  pspec_global_config = sfi_pspec_rec ("bse-preferences", NULL, NULL,
				       bse_gconfig_fields, SFI_PARAM_STANDARD);
  g_param_spec_ref (pspec_global_config);
  g_param_spec_sink (pspec_global_config);
  /* create empty config record */
  rec = sfi_rec_new ();
  value = sfi_value_rec (rec);
  /* fill out missing values with defaults */
  g_param_value_validate (pspec_global_config, value);
  /* install global config */
  gconfig = bse_gconfig_from_rec (rec);
  bse_global_config = gconfig;
  /* cleanup */
  sfi_value_free (value);
  sfi_rec_unref (rec);
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
      gconfig = bse_gconfig_from_rec (vrec);
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
