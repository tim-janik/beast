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
static void	   bse_gconfig_class_destroy	   (BseGConfigClass  *class);
static void        bse_gconfig_set_param           (BseGConfig	     *gconf,
						    BseParam         *param);
static void        bse_gconfig_get_param           (BseGConfig	     *gconf,
						    BseParam         *param);
static void	   bse_gconfig_do_shutdown	   (BseObject        *object);
static void	   bse_gconfig_do_destroy	   (BseObject        *object);
static void        bse_gconfig_do_apply            (BseGConfig       *gconf);
static gboolean    bse_gconfig_do_can_apply        (BseGConfig       *gconf);
static void        bse_gconfig_do_revert           (BseGConfig       *gconf);
static void        bse_gconfig_do_default_revert   (BseGConfig       *gconf);
extern void        bse_gconfig_notify_lock_changed (void);                      /* for bseglobals.c */
extern void        bse_globals_copy                (const BseGlobals *globals_src,
						    BseGlobals       *globals); /* from bseglobals.c */
extern void        bse_globals_reset               (BseGlobals       *globals); /* from bseglobals.c */


/* --- variables --- */
static BseTypeClass     *parent_class = NULL;
static GSList           *bse_gconfig_list = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseGConfig)
{
  static const BseTypeInfo gconfig_info = {
    sizeof (BseGConfigClass),
    
    (BseBaseInitFunc) NULL,
    (BseBaseDestroyFunc) NULL,
    (BseClassInitFunc) bse_gconfig_class_init,
    (BseClassDestroyFunc) bse_gconfig_class_destroy,
    NULL /* class_data */,

    sizeof (BseGConfig),
    0 /* n_preallocs */,
    (BseObjectInitFunc) bse_gconfig_init,
  };

  return bse_type_register_static (BSE_TYPE_OBJECT,
				   "BseGConfig",
				   "Global configuration object",
				   &gconfig_info);
}

static void
bse_gconfig_class_destroy (BseGConfigClass *class)
{
}

static void
bse_gconfig_init (BseGConfig *gconf)
{
  bse_globals_copy (NULL, &gconf->globals);

  bse_gconfig_list = g_slist_prepend (bse_gconfig_list, gconf);
}

static void
bse_gconfig_do_shutdown (BseObject *object)
{
  BseGConfig *gconf;
  
  gconf = BSE_GCONFIG (object);
  
  bse_gconfig_list = g_slist_remove (bse_gconfig_list, gconf);

  /* chain parent class' shutdown handler */
  BSE_OBJECT_CLASS (parent_class)->shutdown (object);
}

static void
bse_gconfig_do_destroy (BseObject *object)
{
  BseGConfig *gconf = BSE_GCONFIG (object);

  bse_globals_reset (&gconf->globals);

  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
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
  BseObjectClass *object_class;
  BseGlobals globals_defaults = { 0, };
    
  parent_class = bse_type_class_peek (BSE_TYPE_OBJECT);
  object_class = BSE_OBJECT_CLASS (class);
  
  object_class->set_param = (BseObjectSetParamFunc) bse_gconfig_set_param;
  object_class->get_param = (BseObjectGetParamFunc) bse_gconfig_get_param;
  object_class->shutdown = bse_gconfig_do_shutdown;
  object_class->destroy = bse_gconfig_do_destroy;

  class->apply = bse_gconfig_do_apply;
  class->can_apply = bse_gconfig_do_can_apply;
  class->revert = bse_gconfig_do_revert;
  class->default_revert = bse_gconfig_do_default_revert;

  bse_globals_copy (NULL, &globals_defaults);
  bse_object_class_add_param (object_class, "Mixing Heart",
			      PARAM_TRACK_LENGTH,
			      bse_param_spec_uint ("track_length", "Track Length", "Internal BSE buffer length (hunk size)",
						   4, 4096, 4,
						   globals_defaults.track_length,
						   BSE_PARAM_DEFAULT | BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Mixing Heart",
			      PARAM_MIXING_FREQUENCY,
			      bse_param_spec_uint ("mixing_frequency", "Mixing Frequency [Hz]",
						   "Frequency for BSE internal buffer mixing, common "
						   "values are: 16000, 22050, 44100, 48000",
						   bse_pcm_freq_to_freq (BSE_PCM_FREQ_MIN),
						   bse_pcm_freq_to_freq (BSE_PCM_FREQ_MAX),
						   0,
						   globals_defaults.mixing_frequency,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Mixing Heart",
			      PARAM_HEART_PRIORITY,
			      bse_param_spec_int ("heart_priority", "BseHeart Priority", "GLib Main Loop priority for BseHeart",
						  G_PRIORITY_HIGH - 100, G_PRIORITY_LOW + 100, 10,
						  globals_defaults.heart_priority,
						  BSE_PARAM_DEFAULT | BSE_PARAM_HINT_RDONLY));
  bse_object_class_add_param (object_class, "Step Widths",
			      PARAM_STEP_VOLUME_dB,
			      bse_param_spec_float ("step_volume_dB", "Volume [dB] Steps", "Step width for volume in decibell",
						    0.001, 5, 0.01,
						    globals_defaults.step_volume_dB,
						    BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Step Widths",
			      PARAM_STEP_BPM,
			      bse_param_spec_uint ("step_bpm", "BPM Steps", "Step width for beats per minute",
						   1, 50, 1,
						   globals_defaults.step_bpm,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Step Widths",
			      PARAM_STEP_N_CHANNELS,
			      bse_param_spec_uint ("step_n_channels", "Channel Count Steps", "Step width for number of channels",
						   1, 16, 1,
						   globals_defaults.step_n_channels,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Step Widths",
			      PARAM_STEP_PATTERN_LENGTH,
			      bse_param_spec_uint ("step_pattern_length", "Pattern Length Steps", "Step width for pattern length",
						   1, 16, 1,
						   globals_defaults.step_pattern_length,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Step Widths",
			      PARAM_STEP_BALANCE,
			      bse_param_spec_uint ("step_balance", "Balance Steps", "Step width for balance",
						   1, 24, 1,
						   globals_defaults.step_balance,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Step Widths",
			      PARAM_STEP_TRANSPOSE,
			      bse_param_spec_uint ("step_transpose", "Transpose Steps", "Step width for transpositions",
						   1, 12, 1,
						   globals_defaults.step_transpose,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Step Widths",
			      PARAM_STEP_FINE_TUNE,
			      bse_param_spec_uint ("step_fine_tune", "Fine Tune Steps", "Step width for fine tunes",
						   1, 6, 1,
						   globals_defaults.step_fine_tune,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Step Widths",
			      PARAM_STEP_ENV_TIME,
			      bse_param_spec_uint ("step_env_time", "Envelope Time Steps", "Step width for envelope times",
						   1, 128, 1,
						   globals_defaults.step_env_time,
						   BSE_PARAM_DEFAULT));
  bse_globals_reset (&globals_defaults);
}

static void
bse_gconfig_set_param (BseGConfig *gconf,
		       BseParam   *param)
{
  switch (param->pspec->any.param_id)
    {
    case PARAM_STEP_VOLUME_dB:
      gconf->globals.step_volume_dB = param->value.v_float;
      break;
    case PARAM_STEP_BPM:
      gconf->globals.step_bpm = param->value.v_uint;
      break;
    case PARAM_STEP_N_CHANNELS:
      gconf->globals.step_n_channels = param->value.v_uint;
      break;
    case PARAM_STEP_PATTERN_LENGTH:
      gconf->globals.step_pattern_length = param->value.v_uint;
      break;
    case PARAM_STEP_BALANCE:
      gconf->globals.step_balance = param->value.v_uint;
      break;
    case PARAM_STEP_TRANSPOSE:
      gconf->globals.step_transpose = param->value.v_uint;
      break;
    case PARAM_STEP_FINE_TUNE:
      gconf->globals.step_fine_tune = param->value.v_uint;
      break;
    case PARAM_STEP_ENV_TIME:
      gconf->globals.step_env_time = param->value.v_uint;
      break;
    case PARAM_TRACK_LENGTH:
      gconf->globals.track_length = param->value.v_uint;
      break;
    case PARAM_MIXING_FREQUENCY:
      gconf->globals.mixing_frequency = bse_pcm_freq_to_freq (bse_pcm_freq_from_freq (param->value.v_uint));
      break;
    case PARAM_HEART_PRIORITY:
      gconf->globals.heart_priority = param->value.v_int;
      break;
    default:
      g_warning ("%s(\"%s\"): invalid attempt to set parameter \"%s\" of type `%s'",
		 BSE_OBJECT_TYPE_NAME (gconf),
		 BSE_OBJECT_NAME (gconf),
		 param->pspec->any.name,
		 bse_type_name (param->pspec->type));
      break;
    }
}

static void
bse_gconfig_get_param (BseGConfig *gconf,
		       BseParam   *param)
{
  switch (param->pspec->any.param_id)
    {
    case PARAM_STEP_VOLUME_dB:
      param->value.v_float = gconf->globals.step_volume_dB;
      break;
    case PARAM_STEP_BPM:
      param->value.v_uint = gconf->globals.step_bpm;
      break;
    case PARAM_STEP_N_CHANNELS:
      param->value.v_uint = gconf->globals.step_n_channels;
      break;
    case PARAM_STEP_PATTERN_LENGTH:
      param->value.v_uint = gconf->globals.step_pattern_length;
      break;
    case PARAM_STEP_BALANCE:
      param->value.v_uint = gconf->globals.step_balance;
      break;
    case PARAM_STEP_TRANSPOSE:
      param->value.v_uint = gconf->globals.step_transpose;
      break;
    case PARAM_STEP_FINE_TUNE:
      param->value.v_uint = gconf->globals.step_fine_tune;
      break;
    case PARAM_STEP_ENV_TIME:
      param->value.v_uint = gconf->globals.step_env_time;
      break;
    case PARAM_TRACK_LENGTH:
      param->value.v_uint = gconf->globals.track_length;
      break;
    case PARAM_MIXING_FREQUENCY:
      param->value.v_uint = gconf->globals.mixing_frequency;
      break;
    case PARAM_HEART_PRIORITY:
      param->value.v_int = gconf->globals.heart_priority;
      break;
    default:
      g_warning ("%s(\"%s\"): invalid attempt to get parameter \"%s\" of type `%s'",
		 BSE_OBJECT_TYPE_NAME (gconf),
		 BSE_OBJECT_NAME (gconf),
		 param->pspec->any.name,
		 bse_type_name (param->pspec->type));
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
  BseObjectClass *class;
  guint i;
  
  g_return_if_fail (BSE_IS_GCONFIG (gconf));
  
  bse_object_ref (BSE_OBJECT (gconf));
  
  BSE_GCONFIG_GET_CLASS (gconf)->revert (gconf);
  
  class = BSE_OBJECT_GET_CLASS (gconf);
  do
    {
      for (i = 0; i < class->n_params; i++)
	{
	  BseParamSpec *pspec = class->param_specs[i];
	  
	  bse_object_param_changed (BSE_OBJECT (gconf), pspec->any.name);
	}
      class = bse_type_class_peek_parent (class);
    }
  while (class);
  
  bse_object_unref (BSE_OBJECT (gconf));
}

static void
bse_gconfig_do_revert (BseGConfig *gconf)
{
  bse_globals_reset (&gconf->globals);
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
  BseObjectClass *class;
  guint i;

  object = BSE_OBJECT (gconf);

  class = BSE_OBJECT_GET_CLASS (gconf);
  do
    {
      for (i = 0; i < class->n_params; i++)
	{
	  BseParam param = { NULL };
	  BseParamSpec *pspec = class->param_specs[i];
	  
	  bse_param_init_default (&param, pspec);
	  
	  bse_object_set_param (object, &param);
	  bse_param_free_value (&param);
	}
      class = bse_type_class_peek_parent (class);
    }
  while (class);
}
