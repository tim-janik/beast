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
#include	"bseglobals.h"

#include	"bseconfig.h"
#include	<math.h>
#include	"bsechunk.h"	/* for bse_chunks_nuke() */


/* --- defines --- */
/* factorization constants: 2^(1/12), ln(2^(1/12)) and 2^(1/(12*6))
 * retrived with:
 #include <stl.h>
 #include <complex.h>
 typedef long double ld;
 
 int main (void)
 {
 ld r, l;
 
 cout.precision(256);
 
 r = pow ((ld) 2, (ld) 1 / (ld) 12);
 cout << "2^(1/12) =\n";
 cout << "2^" << (ld) 1 / (ld) 12 << " =\n";
 cout << r << "\n";
 
 l = log (r);
 cout << "ln(2^(1/12)) =\n";
 cout << "ln(" << r << ") =\n";
 cout << l << "\n";
 
 r = pow ((ld) 2, (ld) 1 / (ld) 72);
 cout << "2^(1/72) =\n";
 cout << "2^" << (ld) 1 / (ld) 72 << " =\n";
 cout << r << "\n";
 
 return 0;
 }
*/
/* keep these defines in sync with bseutils.c */
#define	BSE_2_RAISED_TO_1_OVER_12_d	( /* 2^(1/12) */ \
              1.0594630943592953098431053149397484958171844482421875)
#define	BSE_LN_OF_2_RAISED_TO_1_OVER_12_d	( /* ln(2^(1/12)) */ \
              0.05776226504666215344485635796445421874523162841796875)
#define	BSE_2_RAISED_TO_1_OVER_72_d	( /* 2^(1/72) */ \
              1.009673533228510944326217213529162108898162841796875)


/* --- prototypes --- */
static void bse_gconfig_notify_lock_changed (void);


/* --- extern variables --- */
const guint	     bse_major_version = BSE_MAJOR_VERSION;
const guint	     bse_minor_version = BSE_MINOR_VERSION;
const guint	     bse_micro_version = BSE_MICRO_VERSION;
const guint	     bse_interface_age = BSE_INTERFACE_AGE;
const guint	     bse_binary_age = BSE_BINARY_AGE;
const gchar         *bse_version = BSE_VERSION;
const gchar         *bse_log_domain_bse = "BSE";
const gdouble*	_bse_halftone_factor_table = NULL;
const guint*	_bse_halftone_factor_table_fixed = NULL;
const gdouble*	_bse_fine_tune_factor_table = NULL;


/* --- variables --- */
static guint		 bse_globals_lock_count = 0;
static BseGlobals	 bse_globals_current = { 0, };
const BseGlobals * const bse_globals = &bse_globals_current;
static const BseGlobals	 bse_globals_defaults =
{
  0.1		/* step_volume_dB */,
  10		/* step_bpm */,
  4		/* step_n_channels */,
  4		/* step_pattern_length */,
  8		/* step_balance */,
  4		/* step_transpose */,
  4		/* step_fine_tune */,
  1		/* step_env_time */,
  
  256		/* track_length (hunk_size) */,
  44100		/* mixing_frequency */,
  G_PRIORITY_HIGH_IDLE + 20	/* heart_priority */,
};


/* --- functions --- */
gchar*
bse_check_version (guint required_major,
		   guint required_minor,
		   guint required_micro)
{
  if (required_major > BSE_MAJOR_VERSION)
    return "BSE version too old (major mismatch)";
  if (required_major < BSE_MAJOR_VERSION)
    return "BSE version too new (major mismatch)";
  if (required_minor > BSE_MINOR_VERSION)
    return "BSE version too old (minor mismatch)";
  if (required_minor < BSE_MINOR_VERSION)
    return "BSE version too new (minor mismatch)";
  if (required_micro < BSE_MICRO_VERSION - BSE_BINARY_AGE)
    return "BSE version too new (micro mismatch)";
  if (required_micro > BSE_MICRO_VERSION)
    return "BSE version too old (micro mismatch)";
  return NULL;
}

void
bse_globals_init (void)
{
  static gdouble ht_factor_table_d[BSE_MAX_NOTE + 1] = { 0.0, };
  static guint	 ht_factor_table_fixed_ui[BSE_MAX_NOTE + 1] = { 0, };
  static gdouble ft_factor_table_d[BSE_MAX_FINE_TUNE * 2 + 1] = { 0.0, };
  gint i;
  
  g_return_if_fail (_bse_halftone_factor_table == NULL);
  
  /* setup half tone factorization table
   */
  g_assert (BSE_MIN_NOTE == 0);
  for (i = 0; i <= BSE_MAX_NOTE; i++)
    {
      ht_factor_table_d[i] = pow (BSE_2_RAISED_TO_1_OVER_12_d,
				  ((gdouble) i) - BSE_KAMMER_NOTE);
      ht_factor_table_fixed_ui[i] = 0.5 + ht_factor_table_d[i] * 65536;
      BSE_IF_DEBUG (TABLES)
	{
	  if (i == BSE_MIN_NOTE || i == BSE_MAX_NOTE ||
	      (i >= BSE_KAMMER_NOTE - 6 && i <= BSE_KAMMER_NOTE + 12))
	    g_message ("ht-table: [%d] -> %.20f (%d)",
		       i, ht_factor_table_d[i], ht_factor_table_fixed_ui[i]);
	}
    }
  _bse_halftone_factor_table = ht_factor_table_d;
  _bse_halftone_factor_table_fixed = ht_factor_table_fixed_ui;
  
  /* fine tune assertments, so BSE_2_RAISED_TO_1_OVER_72_d is the right
   * constant (12 * 6 = 72)
   */
  g_assert (- BSE_MIN_FINE_TUNE == BSE_MAX_FINE_TUNE &&
	    BSE_MAX_FINE_TUNE == 6);
  
  /* setup fine tune factorization table, since fine tunes are in the
   * positive and in the negative range, we allow negative indexes here.
   */
  for (i = -BSE_MAX_FINE_TUNE; i <= BSE_MAX_FINE_TUNE; i++)
    {
      ft_factor_table_d[BSE_MAX_FINE_TUNE + i] = pow (BSE_2_RAISED_TO_1_OVER_72_d, i);
      BSE_IF_DEBUG (TABLES)
	g_message ("ft-table: [%d] -> %.20f",
		   i, ft_factor_table_d[BSE_MAX_FINE_TUNE + i]);
    }
  _bse_fine_tune_factor_table = ft_factor_table_d + BSE_MAX_FINE_TUNE;
  
  /* setup BseGlobals
   */
  bse_globals_current = bse_globals_defaults;
  
  bse_globals_lock_count = 0;
}

void
bse_globals_lock (void)
{
  bse_globals_lock_count++;
  if (bse_globals_lock_count == 1)
    bse_gconfig_notify_lock_changed ();
}

void
bse_globals_unlock (void)
{
  if (bse_globals_lock_count)
    {
      bse_globals_lock_count--;
      if (bse_globals_lock_count == 0)
	{
	  bse_chunks_nuke ();
	  bse_gconfig_notify_lock_changed ();
	}
    }
}

gboolean
bse_globals_locked (void)
{
  return bse_globals_lock_count != 0;
}

gdouble
bse_dB_to_factor (gfloat dB)
{
  gdouble factor;
  
  factor = dB / 10; /* Bell */
  factor = pow (10, factor);
  
  return factor;
}

gfloat
bse_dB_from_factor (gdouble factor,
		    gfloat  min_dB)
{
  if (factor > 0)
    {
      gfloat dB;
      
      dB = log10 (factor); /* Bell */
      dB *= 10;
      
      return dB;
    }
  else
    return min_dB;
}


/* --- BseGConfig ---
 * "object-ified" interface to the globals structure
 */
#include "bseobject.h"		/* for object inheritance */
#include "bsepcmdevice.h"	/* for frequency alignment */

/* --- structures --- */
struct _BseGConfig
{
  BseObject parent_object;

  BseGlobals globals;
};
struct _BseGConfigClass
{
  BseObjectClass parent_class;
};


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
static void	 bse_gconfig_init		(BseGConfig	 *gconf);
static void	 bse_gconfig_class_init		(BseGConfigClass *class);
static void	 bse_gconfig_class_destroy	(BseGConfigClass *class);
static void      bse_gconfig_set_param          (BseGConfig	 *gconf,
						 BseParam        *param);
static void      bse_gconfig_get_param          (BseGConfig	 *gconf,
						 BseParam        *param);
static void	 bse_gconfig_do_shutdown	(BseObject     	 *object);


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
  gconf->globals = bse_globals_current;

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
    
  parent_class = bse_type_class_peek (BSE_TYPE_OBJECT);
  object_class = BSE_OBJECT_CLASS (class);
  
  object_class->set_param = (BseObjectSetParamFunc) bse_gconfig_set_param;
  object_class->get_param = (BseObjectGetParamFunc) bse_gconfig_get_param;
  object_class->shutdown = bse_gconfig_do_shutdown;
  
  bse_object_class_add_param (object_class, "Mixing Heart",
			      PARAM_TRACK_LENGTH,
			      bse_param_spec_uint ("track_length", "Track Length", "Internal BSE buffer length (hunk size)",
						   4, 4096, 4,
						   bse_globals_defaults.track_length,
						   BSE_PARAM_DEFAULT | BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Mixing Heart",
			      PARAM_MIXING_FREQUENCY,
			      bse_param_spec_uint ("mixing_frequency", "Mixing Frequency [Hz]",
						   "Frequency for BSE internal buffer mixing, common "
						   "values are: 16000, 22050, 44100, 48000",
						   bse_pcm_freq_to_freq (BSE_PCM_FREQ_MIN),
						   bse_pcm_freq_to_freq (BSE_PCM_FREQ_MAX),
						   0,
						   bse_globals_defaults.mixing_frequency,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Mixing Heart",
			      PARAM_HEART_PRIORITY,
			      bse_param_spec_int ("heart_priority", "BseHeart Priority", "GLib Main Loop priority for BseHeart",
						  G_PRIORITY_HIGH - 100, G_PRIORITY_LOW + 100, 10,
						  bse_globals_defaults.heart_priority,
						  BSE_PARAM_DEFAULT | BSE_PARAM_HINT_RDONLY));
  bse_object_class_add_param (object_class, "Step Widths",
			      PARAM_STEP_VOLUME_dB,
			      bse_param_spec_float ("step_volume_dB", "Volume [dB] Steps", "Step width for volume in decibell",
						    0.001, 5, 0.01,
						    bse_globals_defaults.step_volume_dB,
						    BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Step Widths",
			      PARAM_STEP_BPM,
			      bse_param_spec_uint ("step_bpm", "BPM Steps", "Step width for beats per minute",
						   1, 50, 1,
						   bse_globals_defaults.step_bpm,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Step Widths",
			      PARAM_STEP_N_CHANNELS,
			      bse_param_spec_uint ("step_n_channels", "Channel Count Steps", "Step width for number of channels",
						   1, 16, 1,
						   bse_globals_defaults.step_n_channels,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Step Widths",
			      PARAM_STEP_PATTERN_LENGTH,
			      bse_param_spec_uint ("step_pattern_length", "Pattern Length Steps", "Step width for pattern length",
						   1, 16, 1,
						   bse_globals_defaults.step_pattern_length,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Step Widths",
			      PARAM_STEP_BALANCE,
			      bse_param_spec_uint ("step_balance", "Balance Steps", "Step width for balance",
						   1, 24, 1,
						   bse_globals_defaults.step_balance,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Step Widths",
			      PARAM_STEP_TRANSPOSE,
			      bse_param_spec_uint ("step_transpose", "Transpose Steps", "Step width for transpositions",
						   1, 12, 1,
						   bse_globals_defaults.step_transpose,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Step Widths",
			      PARAM_STEP_FINE_TUNE,
			      bse_param_spec_uint ("step_fine_tune", "Fine Tune Steps", "Step width for fine tunes",
						   1, 6, 1,
						   bse_globals_defaults.step_fine_tune,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Step Widths",
			      PARAM_STEP_ENV_TIME,
			      bse_param_spec_uint ("step_env_time", "Envelope Time Steps", "Step width for envelope times",
						   1, 128, 1,
						   bse_globals_defaults.step_env_time,
						   BSE_PARAM_DEFAULT));
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
    bse_globals_current = gconf->globals;
}

gboolean
bse_gconfig_can_apply (BseGConfig *gconf)
{
  g_return_val_if_fail (BSE_IS_GCONFIG (gconf), FALSE);

  return !bse_globals_locked ();
}

void
bse_gconfig_revert (BseGConfig *gconf)
{
  BseObjectClass *class;
  guint i;

  g_return_if_fail (BSE_IS_GCONFIG (gconf));

  gconf->globals = bse_globals_current;

  class = BSE_OBJECT_GET_CLASS (gconf);
  for (i = 0; i < class->n_params; i++)
    {
      BseParamSpec *pspec = class->param_specs[i];

      bse_object_param_changed (BSE_OBJECT (gconf), pspec->any.name);
    }
}

void
bse_gconfig_default_revert (BseGConfig *gconf)
{
  BseObject *object;
  BseObjectClass *class;
  guint i;

  g_return_if_fail (BSE_IS_GCONFIG (gconf));

  object = BSE_OBJECT (gconf);

  gconf->globals = bse_globals_current;

  class = BSE_OBJECT_GET_CLASS (gconf);
  for (i = 0; i < class->n_params; i++)
    {
      BseParam param = { NULL };
      BseParamSpec *pspec = class->param_specs[i];

      bse_param_init_default (&param, pspec);

      bse_object_set_param (object, &param);
      bse_param_free_value (&param);
    }
}
