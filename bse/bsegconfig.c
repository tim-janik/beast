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
#include	"bsegconfig.h"

#include	"bsepcmdevice.h"	/* for frequency alignment */


/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_STEP_VOLUME_dB,
  PARAM_STEP_BPM,
  PARAM_STEP_N_CHANNELS,
  PARAM_STEP_PATTERN_LENGTH,
  PARAM_STEP_BALANCE,
  PARAM_STEP_TRANSPOSE,
  PARAM_STEP_FINE_TUNE,
  PARAM_STEP_ENV_TIME,
  PARAM_TRACK_LENGTH,
  PARAM_MIXING_FREQUENCY,
  PARAM_HEART_PRIORITY
};


/* --- prototypes --- */
static void	   bse_gconfig_init		   (BseGConfig	     *gconf);
static void	   bse_gconfig_class_init	   (BseGConfigClass  *class);
static void	   bse_gconfig_do_finalize	   (GObject          *gobject);
static void        bse_gconfig_set_param           (BseGConfig	     *gconf,
						    guint             param_id,
						    GValue           *value,
						    GParamSpec       *pspec,
						    const gchar      *trailer);
static void        bse_gconfig_get_param           (BseGConfig	     *gconf,
						    guint             param_id,
						    GValue           *value,
						    GParamSpec       *pspec,
						    const gchar      *trailer);
static void        bse_gconfig_do_apply            (BseGConfig       *gconf);
static gboolean    bse_gconfig_do_can_apply        (BseGConfig       *gconf);
static void        bse_gconfig_do_revert           (BseGConfig       *gconf);
static void        bse_gconfig_do_default_revert   (BseGConfig       *gconf);
extern void        bse_gconfig_notify_lock_changed (void);                      /* for bseglobals.c */
extern void        bse_globals_copy                (const BseGlobals *globals_src,
						    BseGlobals       *globals); /* from bseglobals.c */
extern void        bse_globals_unset               (BseGlobals       *globals); /* from bseglobals.c */


/* --- variables --- */
static GTypeClass     *parent_class = NULL;
static GSList         *bse_gconfig_list = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseGConfig)
{
  static const GTypeInfo gconfig_info = {
    sizeof (BseGConfigClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_gconfig_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseGConfig),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_gconfig_init,
  };
  
  return bse_type_register_static (BSE_TYPE_OBJECT,
				   "BseGConfig",
				   "BSE Global Configuration interface",
				   &gconfig_info);
}

static void
bse_gconfig_init (BseGConfig *gconf)
{
  bse_globals_copy (NULL, &gconf->globals);
  
  bse_gconfig_list = g_slist_prepend (bse_gconfig_list, gconf);
}

static void
bse_gconfig_do_finalize (GObject *gobject)
{
  BseGConfig *gconf = BSE_GCONFIG (gobject);
  
  bse_gconfig_list = g_slist_remove (bse_gconfig_list, gconf);
  
  bse_globals_unset (&gconf->globals);
  
  /* chain parent class' finalize handler */
  G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

void
bse_gconfig_notify_lock_changed (void)
{
  GSList *slist;
  
  for (slist = bse_gconfig_list; slist; slist = slist->next)
    BSE_NOTIFY (slist->data, lock_changed, NOTIFY (OBJECT, DATA));
}

static void
bse_gconfig_class_init (BseGConfigClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseGlobals globals_defaults = { 0, };
  
  parent_class = g_type_class_peek (BSE_TYPE_OBJECT);
  
  gobject_class->set_param = (GObjectSetParamFunc) bse_gconfig_set_param;
  gobject_class->get_param = (GObjectGetParamFunc) bse_gconfig_get_param;
  gobject_class->finalize = bse_gconfig_do_finalize;
  
  class->apply = bse_gconfig_do_apply;
  class->can_apply = bse_gconfig_do_can_apply;
  class->revert = bse_gconfig_do_revert;
  class->default_revert = bse_gconfig_do_default_revert;
  
  bse_globals_copy (NULL, &globals_defaults);
  bse_object_class_add_param (object_class, "Mixing Heart",
			      PARAM_TRACK_LENGTH,
			      b_param_spec_uint ("track_length", "Track Length", "Internal BSE buffer length (hunk size)",
						 4, 4096,
						 globals_defaults.track_length, 4,
						 B_PARAM_DEFAULT | B_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Mixing Heart",
			      PARAM_MIXING_FREQUENCY,
			      b_param_spec_uint ("mixing_frequency", "Mixing Frequency [Hz]",
						 "Frequency for BSE internal buffer mixing, common "
						 "values are: 16000, 22050, 44100, 48000",
						 bse_pcm_freq_to_freq (BSE_PCM_FREQ_MIN),
						 bse_pcm_freq_to_freq (BSE_PCM_FREQ_MAX),
						 globals_defaults.mixing_frequency, 0,
						 B_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Mixing Heart",
			      PARAM_HEART_PRIORITY,
			      b_param_spec_int ("heart_priority", "BseHeart Priority", "GLib Main Loop priority for BseHeart",
						G_PRIORITY_HIGH - 100, G_PRIORITY_LOW + 100,
						globals_defaults.heart_priority, 10,
						B_PARAM_DEFAULT | B_PARAM_HINT_RDONLY));
  bse_object_class_add_param (object_class, "Step Widths",
			      PARAM_STEP_VOLUME_dB,
			      b_param_spec_float ("step_volume_dB", "Volume [dB] Steps", "Step width for volume in decibell",
						  0.001, 5,
						  globals_defaults.step_volume_dB, 0.01,
						  B_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Step Widths",
			      PARAM_STEP_BPM,
			      b_param_spec_uint ("step_bpm", "BPM Steps", "Step width for beats per minute",
						 1, 50,
						 globals_defaults.step_bpm, 1,
						 B_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Step Widths",
			      PARAM_STEP_N_CHANNELS,
			      b_param_spec_uint ("step_n_channels", "Channel Count Steps", "Step width for number of channels",
						 1, 16,
						 globals_defaults.step_n_channels, 1,
						 B_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Step Widths",
			      PARAM_STEP_PATTERN_LENGTH,
			      b_param_spec_uint ("step_pattern_length", "Pattern Length Steps", "Step width for pattern length",
						 1, 16,
						 globals_defaults.step_pattern_length, 1,
						 B_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Step Widths",
			      PARAM_STEP_BALANCE,
			      b_param_spec_uint ("step_balance", "Balance Steps", "Step width for balance",
						 1, 24,
						 globals_defaults.step_balance, 1,
						 B_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Step Widths",
			      PARAM_STEP_TRANSPOSE,
			      b_param_spec_uint ("step_transpose", "Transpose Steps", "Step width for transpositions",
						 1, 12,
						 globals_defaults.step_transpose, 1,
						 B_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Step Widths",
			      PARAM_STEP_FINE_TUNE,
			      b_param_spec_uint ("step_fine_tune", "Fine Tune Steps", "Step width for fine tunes",
						 1, 6,
						 globals_defaults.step_fine_tune, 1,
						 B_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Step Widths",
			      PARAM_STEP_ENV_TIME,
			      b_param_spec_uint ("step_env_time", "Envelope Time Steps", "Step width for envelope times",
						 1, 128,
						 globals_defaults.step_env_time, 1,
						 B_PARAM_DEFAULT));
  bse_globals_unset (&globals_defaults);
}

static void
bse_gconfig_set_param (BseGConfig  *gconf,
		       guint        param_id,
		       GValue      *value,
		       GParamSpec  *pspec,
		       const gchar *trailer)
{
  switch (param_id)
    {
    case PARAM_STEP_VOLUME_dB:
      gconf->globals.step_volume_dB = b_value_get_float (value);
      break;
    case PARAM_STEP_BPM:
      gconf->globals.step_bpm = b_value_get_uint (value);
      break;
    case PARAM_STEP_N_CHANNELS:
      gconf->globals.step_n_channels = b_value_get_uint (value);
      break;
    case PARAM_STEP_PATTERN_LENGTH:
      gconf->globals.step_pattern_length = b_value_get_uint (value);
      break;
    case PARAM_STEP_BALANCE:
      gconf->globals.step_balance = b_value_get_uint (value);
      break;
    case PARAM_STEP_TRANSPOSE:
      gconf->globals.step_transpose = b_value_get_uint (value);
      break;
    case PARAM_STEP_FINE_TUNE:
      gconf->globals.step_fine_tune = b_value_get_uint (value);
      break;
    case PARAM_STEP_ENV_TIME:
      gconf->globals.step_env_time = b_value_get_uint (value);
      break;
    case PARAM_TRACK_LENGTH:
      gconf->globals.track_length = b_value_get_uint (value);
      break;
    case PARAM_MIXING_FREQUENCY:
      gconf->globals.mixing_frequency = bse_pcm_freq_to_freq (bse_pcm_freq_from_freq (b_value_get_uint (value)));
      break;
    case PARAM_HEART_PRIORITY:
      gconf->globals.heart_priority = b_value_get_int (value);
      break;
    default:
      G_WARN_INVALID_PARAM_ID (gconf, param_id, pspec);
      break;
    }
}

static void
bse_gconfig_get_param (BseGConfig  *gconf,
		       guint        param_id,
		       GValue      *value,
		       GParamSpec  *pspec,
		       const gchar *trailer)
{
  switch (param_id)
    {
    case PARAM_STEP_VOLUME_dB:
      b_value_set_float (value, gconf->globals.step_volume_dB);
      break;
    case PARAM_STEP_BPM:
      b_value_set_uint (value, gconf->globals.step_bpm);
      break;
    case PARAM_STEP_N_CHANNELS:
      b_value_set_uint (value, gconf->globals.step_n_channels);
      break;
    case PARAM_STEP_PATTERN_LENGTH:
      b_value_set_uint (value, gconf->globals.step_pattern_length);
      break;
    case PARAM_STEP_BALANCE:
      b_value_set_uint (value, gconf->globals.step_balance);
      break;
    case PARAM_STEP_TRANSPOSE:
      b_value_set_uint (value, gconf->globals.step_transpose);
      break;
    case PARAM_STEP_FINE_TUNE:
      b_value_set_uint (value, gconf->globals.step_fine_tune);
      break;
    case PARAM_STEP_ENV_TIME:
      b_value_set_uint (value, gconf->globals.step_env_time);
      break;
    case PARAM_TRACK_LENGTH:
      b_value_set_uint (value, gconf->globals.track_length);
      break;
    case PARAM_MIXING_FREQUENCY:
      b_value_set_uint (value, gconf->globals.mixing_frequency);
      break;
    case PARAM_HEART_PRIORITY:
      b_value_set_int (value, gconf->globals.heart_priority);
      break;
    default:
      G_WARN_INVALID_PARAM_ID (gconf, param_id, pspec);
      break;
    }
}

void
bse_gconfig_apply (BseGConfig *gconf)
{
  g_return_if_fail (BSE_IS_GCONFIG (gconf));
  
  if (!bse_globals_locked ())
    BSE_GCONFIG_GET_CLASS (gconf)->apply (gconf);
}

static void
bse_gconfig_do_apply (BseGConfig *gconf)
{
  bse_globals_copy (&gconf->globals, NULL);
}

gboolean
bse_gconfig_can_apply (BseGConfig *gconf)
{
  g_return_val_if_fail (BSE_IS_GCONFIG (gconf), FALSE);
  
  return BSE_GCONFIG_GET_CLASS (gconf)->can_apply (gconf);
}

static gboolean
bse_gconfig_do_can_apply (BseGConfig *gconf)
{
  return !bse_globals_locked ();
}

void
bse_gconfig_revert (BseGConfig *gconf)
{
  GObjectClass *class;
  guint i;
  
  g_return_if_fail (BSE_IS_GCONFIG (gconf));
  
  bse_object_ref (BSE_OBJECT (gconf));
  
  BSE_GCONFIG_GET_CLASS (gconf)->revert (gconf);
  
  class = G_OBJECT_GET_CLASS (gconf);
  do
    {
      for (i = 0; i < class->n_param_specs; i++)
	{
	  GParamSpec *pspec = class->param_specs[i];
	  
	  bse_object_param_changed (BSE_OBJECT (gconf), pspec->name);
	}
      class = g_type_class_peek_parent (class);
    }
  while (class);
  
  bse_object_unref (BSE_OBJECT (gconf));
}

static void
bse_gconfig_do_revert (BseGConfig *gconf)
{
  bse_globals_unset (&gconf->globals);
  bse_globals_copy (bse_globals, &gconf->globals);
}

void
bse_gconfig_default_revert (BseGConfig *gconf)
{
  g_return_if_fail (BSE_IS_GCONFIG (gconf));
  
  BSE_GCONFIG_GET_CLASS (gconf)->default_revert (gconf);
}

static void
bse_gconfig_do_default_revert (BseGConfig *gconf)
{
  BseObject *object;
  GObjectClass *class;
  guint i;
  
  object = BSE_OBJECT (gconf);
  
  class = G_OBJECT_GET_CLASS (gconf);
  do
    {
      for (i = 0; i < class->n_param_specs; i++)
	{
	  GValue value = { 0, };
	  GParamSpec *pspec = class->param_specs[i];
	  
	  g_value_init_default (&value, pspec);
	  g_object_set_param (G_OBJECT (object), pspec->name, &value);
	  g_value_unset (&value);
	}
      class = g_type_class_peek_parent (class);
    }
  while (class);
}
