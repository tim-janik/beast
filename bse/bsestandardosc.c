/* BseStandardOsc - BSE Standard Oscillator
 * Copyright (C) 1999, 2000-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bsestandardosc.h"

#include "bsecategories.h"
#include "gslengine.h"
#include "gslsignal.h"


#define	FRAC_N_BITS	(19)
#define	FRAC_BIT_MASK	((1 << FRAC_N_BITS) - 1)
#define	TABLE_SIZE	(1 << (32 - FRAC_N_BITS))


/* --- parameters --- */
enum
{
  PROP_0,
  PROP_WAVE_FORM,
  PROP_PHASE,
  PROP_BASE_FREQ,
  PROP_BASE_NOTE,
  PROP_FINE_TUNE,
  PROP_FM_PERC,
  PROP_FM_EXP,
  PROP_FM_OCTAVES,
  PROP_SELF_PERC,
  PROP_PULSE_WIDTH,
  PROP_PULSE_MOD_PERC
};


/* --- prototypes --- */
static void	bse_standard_osc_init		(BseStandardOsc		*standard_osc);
static void	bse_standard_osc_class_init	(BseStandardOscClass	*class);
static void	bse_standard_osc_class_finalize	(BseStandardOscClass	*class);
static void	bse_standard_osc_set_property	(GObject		*object,
						 guint			 param_id,
						 const GValue		*value,
						 GParamSpec		*pspec);
static void	bse_standard_osc_get_property	(GObject		*object,
						 guint			 param_id,
						 GValue			*value,
						 GParamSpec		*pspec);
static void	bse_standard_osc_prepare	(BseSource		*source);
static void	bse_standard_osc_context_create	(BseSource		*source,
						 guint			 context_handle,
						 GslTrans		*trans);
static void	bse_standard_osc_reset		(BseSource		*source);
static void	bse_standard_osc_update_modules	(BseStandardOsc		*standard_osc,
						 gboolean		 recreate_table,
						 GslTrans		*trans);


/* --- variables --- */
static gpointer	    parent_class = NULL;
static const gfloat osc_table_freqs[] = {
  BSE_KAMMER_FREQUENCY_f,
  BSE_KAMMER_FREQUENCY_f / 2.0,
  BSE_KAMMER_FREQUENCY_f / 4.0,
  BSE_KAMMER_FREQUENCY_f / 8.0,
  BSE_KAMMER_FREQUENCY_f / 16.0,
  BSE_KAMMER_FREQUENCY_f * 2.0,
  BSE_KAMMER_FREQUENCY_f * 4.0,
  BSE_KAMMER_FREQUENCY_f * 8.0,
  BSE_KAMMER_FREQUENCY_f * 16.0
};


/* --- functions --- */
BSE_BUILTIN_TYPE (BseStandardOsc)
{
  static const GTypeInfo type_info = {
    sizeof (BseStandardOscClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_standard_osc_class_init,
    (GClassFinalizeFunc) bse_standard_osc_class_finalize,
    NULL /* class_data */,
    
    sizeof (BseStandardOsc),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_standard_osc_init,
  };
#include "./icons/osc.c"
  static const BsePixdata pixdata = {
    OSC_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
    OSC_IMAGE_WIDTH, OSC_IMAGE_HEIGHT,
    OSC_IMAGE_RLE_PIXEL_DATA,
  };
  GType type;
  
  type = bse_type_register_static (BSE_TYPE_SOURCE,
				   "BseStandardOsc",
				   "StandardOsc is a basis oscillator that supports frequency, "
				   "modulation and sync inputs",
				   &type_info);
  bse_categories_register_icon ("/Modules/Audio Sources/Standard Oscillator",
				type,
				&pixdata);
  return type;
}

static void
bse_standard_osc_class_init (BseStandardOscClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ochannel, ichannel;
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = bse_standard_osc_set_property;
  gobject_class->get_property = bse_standard_osc_get_property;
  
  source_class->prepare = bse_standard_osc_prepare;
  source_class->context_create = bse_standard_osc_context_create;
  source_class->reset = bse_standard_osc_reset;
  
  bse_object_class_add_param (object_class, "Wave Form",
			      PROP_WAVE_FORM,
			      bse_param_spec_genum ("wave_form", "Wave", "Oscillator wave form",
						    BSE_TYPE_STANDARD_OSC_WAVE_TYPE,
						    BSE_STANDARD_OSC_SAW_FALL,
						    SFI_PARAM_STANDARD));
  bse_object_class_add_param (object_class, "Wave Form",
			      PROP_PHASE,
			      sfi_pspec_real ("phase", "Phase", NULL,
					      0.0, -180.0, 180.0, 5.0,
					      SFI_PARAM_STANDARD ":f:dial:skip-default"));
  bse_object_class_add_param (object_class, "Base Frequency",
			      PROP_BASE_FREQ,
			      bse_param_spec_freq_simple ("base_freq", "Frequency", NULL,
							  SFI_PARAM_STANDARD ":f:dial"));
  bse_object_class_add_param (object_class, "Base Frequency",
			      PROP_BASE_NOTE,
			      bse_pspec_note_simple ("base_note", "Note", NULL, SFI_PARAM_GUI));
  bse_object_class_add_param (object_class, "Base Frequency",
			      PROP_FINE_TUNE,
			      sfi_pspec_int ("fine_tune", "Fine Tune", NULL,
					     0, BSE_MIN_FINE_TUNE, BSE_MAX_FINE_TUNE, 10,
					     SFI_PARAM_STANDARD ":f:dial:skip-default"));
  bse_object_class_add_param (object_class, "Modulation",
			      PROP_FM_PERC,
			      sfi_pspec_real ("fm_perc", "Input Modulation [%]",
					      "Strength of linear frequency modulation",
					      0.0, 0.0, 100.0, 5.0,
					      SFI_PARAM_STANDARD ":f:scale"));
  bse_object_class_add_param (object_class, "Modulation",
			      PROP_FM_EXP,
			      sfi_pspec_bool ("exponential_fm", "Exponential FM",
					      "Perform exponential frequency modulation "
					      "instead of linear",
					      FALSE, SFI_PARAM_STANDARD));
  bse_object_class_add_param (object_class, "Modulation",
			      PROP_FM_OCTAVES,
			      sfi_pspec_real ("fm_n_octaves", "Octaves",
					      "Number of octaves to be affected by exponential frequency modulation",
					      1.0, 0, 5.0, 0.01,
					      SFI_PARAM_STANDARD ":f:scale"));
  bse_object_class_add_param (object_class, "Modulation",
			      PROP_SELF_PERC,
			      sfi_pspec_real ("self_perc", "Self Modulation [%]",
					      "Strength of self modulation",
					      0.0, 0.0, 100.0, 5.0,
					      SFI_PARAM_STANDARD ":f:scale:skip-default"));
  bse_object_class_add_param (object_class, "Pulse Modulation",
			      PROP_PULSE_WIDTH,
			      sfi_pspec_real ("pulse_width", "Pulse Width",
					      "Proportion of the positive component duration of the pulse wave form "
					      "(Pulse has to be selected as wave form for this to take effect)",
					      50.0, 0.0, 100.0, 5.0,
					      SFI_PARAM_STANDARD ":f:dial"));
  bse_object_class_add_param (object_class, "Pulse Modulation",
			      PROP_PULSE_MOD_PERC,
			      sfi_pspec_real ("pulse_mod_perc", "Pulse Modulation [%]",
					      "Strength of pulse wisth modulation input "
					      "(Pulse has to be selected as wave form for this to take effect)",
					      0.0, 0.0, 100.0, 5.0,
					      SFI_PARAM_STANDARD ":f:dial"));
  
  ichannel = bse_source_class_add_ichannel (source_class, "Freq In", "Oscillating Frequency Input");
  g_assert (ichannel == BSE_STANDARD_OSC_ICHANNEL_FREQ);
  ichannel = bse_source_class_add_ichannel (source_class, "Freq Mod In", "Frequency Modulation Input");
  g_assert (ichannel == BSE_STANDARD_OSC_ICHANNEL_FREQ_MOD);
  ichannel = bse_source_class_add_ichannel (source_class, "PWM In", "Pulse Width Modulation Input");
  g_assert (ichannel == BSE_STANDARD_OSC_ICHANNEL_PWM);
  ichannel = bse_source_class_add_ichannel (source_class, "Sync In", "Syncronization Input");
  g_assert (ichannel == BSE_STANDARD_OSC_ICHANNEL_SYNC);
  ochannel = bse_source_class_add_ochannel (source_class, "Audio Out", "Oscillated Output");
  g_assert (ochannel == BSE_STANDARD_OSC_OCHANNEL_OSC);
  ochannel = bse_source_class_add_ochannel (source_class, "Sync Out", "Syncronization Output");
  g_assert (ochannel == BSE_STANDARD_OSC_OCHANNEL_SYNC);
}

static void
bse_standard_osc_class_finalize (BseStandardOscClass *class)
{
}

static void
bse_standard_osc_init (BseStandardOsc *self)
{
  self->wave = BSE_STANDARD_OSC_SAW_FALL;
  self->config.phase = 0.0;
  self->config.cfreq = BSE_KAMMER_FREQUENCY_f;
  self->config.exponential_fm = FALSE;
  self->config.self_fm_strength = 0;
  self->config.pulse_width = 0.5;
  self->config.pulse_mod_strength = 0;
  self->fm_strength = 0;
  self->n_octaves = 1;
}

static void
bse_standard_osc_set_property (GObject      *object,
			       guint         param_id,
			       const GValue *value,
			       GParamSpec   *pspec)
{
  BseStandardOsc *self = BSE_STANDARD_OSC (object);
  
  switch (param_id)
    {
    case PROP_WAVE_FORM:
      self->wave = g_value_get_enum (value);
      bse_standard_osc_update_modules (self, TRUE, NULL);
      break;
    case PROP_PHASE:
      self->config.phase = sfi_value_get_real (value) / 180.0;
      bse_standard_osc_update_modules (self, FALSE, NULL);
      break;
    case PROP_BASE_FREQ:
      self->config.cfreq = sfi_value_get_real (value);
      bse_standard_osc_update_modules (self, FALSE, NULL);
      g_object_notify (self, "base_note");
      break;
    case PROP_BASE_NOTE:
      self->config.cfreq = bse_note_to_freq (sfi_value_get_note (value));
      self->config.cfreq = MAX (self->config.cfreq, BSE_MIN_OSC_FREQUENCY_d);
      bse_standard_osc_update_modules (self, FALSE, NULL);
      g_object_notify (self, "base_freq");
      if (bse_note_from_freq (self->config.cfreq) != sfi_value_get_note (value))
	g_object_notify (self, "base_note");
      break;
    case PROP_FINE_TUNE:
      self->config.fine_tune = sfi_value_get_int (value);
      bse_standard_osc_update_modules (self, FALSE, NULL);
      break;
    case PROP_FM_PERC:
      self->fm_strength = sfi_value_get_real (value) / 100.0;
      bse_standard_osc_update_modules (self, FALSE, NULL);
      break;
    case PROP_FM_EXP:
      self->config.exponential_fm = sfi_value_get_bool (value);
      bse_standard_osc_update_modules (self, FALSE, NULL);
      break;
    case PROP_FM_OCTAVES:
      self->n_octaves = sfi_value_get_real (value);
      bse_standard_osc_update_modules (self, FALSE, NULL);
      break;
    case PROP_SELF_PERC:
      self->config.self_fm_strength = sfi_value_get_real (value) / 100.0;
      bse_standard_osc_update_modules (self, FALSE, NULL);
      break;
    case PROP_PULSE_WIDTH:
      self->config.pulse_width = sfi_value_get_real (value) / 100.0;
      bse_standard_osc_update_modules (self, FALSE, NULL);
      break;
    case PROP_PULSE_MOD_PERC:
      self->config.pulse_mod_strength = sfi_value_get_real (value) / 200.0;
      bse_standard_osc_update_modules (self, FALSE, NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_standard_osc_get_property (GObject    *object,
			       guint       param_id,
			       GValue     *value,
			       GParamSpec *pspec)
{
  BseStandardOsc *self = BSE_STANDARD_OSC (object);
  
  switch (param_id)
    {
    case PROP_WAVE_FORM:
      g_value_set_enum (value, self->wave);
      break;
    case PROP_PHASE:
      sfi_value_set_real (value, self->config.phase * 180.0);
      break;
    case PROP_BASE_FREQ:
      sfi_value_set_real (value, self->config.cfreq);
      break;
    case PROP_BASE_NOTE:
      sfi_value_set_note (value, bse_note_from_freq (self->config.cfreq));
      break;
    case PROP_FINE_TUNE:
      sfi_value_set_int (value, self->config.fine_tune);
      break;
    case PROP_FM_PERC:
      sfi_value_set_real (value, self->fm_strength * 100.0);
      break;
    case PROP_FM_EXP:
      sfi_value_set_bool (value, self->config.exponential_fm);
      break;
    case PROP_FM_OCTAVES:
      sfi_value_set_real (value, self->n_octaves);
      break;
    case PROP_SELF_PERC:
      sfi_value_set_real (value, self->config.self_fm_strength * 100.0);
      break;
    case PROP_PULSE_WIDTH:
      sfi_value_set_real (value, self->config.pulse_width * 100.0);
      break;
    case PROP_PULSE_MOD_PERC:
      sfi_value_set_real (value, self->config.pulse_mod_strength * 200.0);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_standard_osc_prepare (BseSource *source)
{
  BseStandardOsc *self = BSE_STANDARD_OSC (source);
  
  self->config.table = gsl_osc_table_create (gsl_engine_sample_freq (),
					     self->wave,
					     gsl_window_blackman,
					     G_N_ELEMENTS (osc_table_freqs),
					     osc_table_freqs);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

typedef struct
{
  GslOscConfig config;
  GslOscTable *old_osc_table;
} OscConfigData;

static void
standard_osc_access (GslModule *module,
		     gpointer   data)
{
  GslOscData *osc = module->user_data;
  OscConfigData *cdata = data;
  
  /* this runs in the Gsl Engine threads */
  
  gsl_osc_config (osc, &cdata->config);
}

static void
standard_osc_access_free (gpointer data)
{
  OscConfigData *cdata = data;
  
  if (cdata->old_osc_table)
    gsl_osc_table_free (cdata->old_osc_table);
  g_free (cdata);
}

static void
bse_standard_osc_update_modules (BseStandardOsc *self,
				 gboolean	 recreate_table,
				 GslTrans       *trans)
{
  self->config.fm_strength = self->config.exponential_fm ? self->n_octaves : self->fm_strength;
  
  /* update modules in all contexts with the new vars */
  if (BSE_SOURCE_PREPARED (self))
    {
      OscConfigData cdata;
      
      cdata.config = self->config;
      if (recreate_table)
	{
	  cdata.old_osc_table = self->config.table;
	  self->config.table = gsl_osc_table_create (gsl_engine_sample_freq (),
						     self->wave,
						     gsl_window_blackman,
						     G_N_ELEMENTS (osc_table_freqs),
						     osc_table_freqs);
	  cdata.config.table = self->config.table;
	}
      else
	cdata.old_osc_table = NULL;
      bse_source_access_modules (BSE_SOURCE (self),
				 standard_osc_access,
				 g_memdup (&cdata, sizeof (cdata)),
				 standard_osc_access_free,
				 trans);
    }
}

static void
standard_osc_reset (GslModule *module)
{
  GslOscData *osc = module->user_data;
  
  gsl_osc_reset (osc);
}

static void
standard_osc_process (GslModule *module,
		      guint      n_values)
{
  GslOscData *osc = module->user_data;
  const gfloat *freq_in = NULL;
  const gfloat *mod_in = NULL;
  const gfloat *sync_in = NULL;
  const gfloat *pwm_in = NULL;
  gfloat *osc_out = NULL;
  gfloat *sync_out = NULL;
  
  if (GSL_MODULE_OSTREAM (module, BSE_STANDARD_OSC_OCHANNEL_SYNC).connected)
    sync_out = GSL_MODULE_OBUFFER (module, BSE_STANDARD_OSC_OCHANNEL_SYNC);
  osc_out = GSL_MODULE_OBUFFER (module, BSE_STANDARD_OSC_OCHANNEL_OSC);
  if (!GSL_MODULE_OSTREAM (module, BSE_STANDARD_OSC_OCHANNEL_OSC).connected &&
      !sync_out)
    return;	/* nothing to process */
  
  if (GSL_MODULE_ISTREAM (module, BSE_STANDARD_OSC_ICHANNEL_FREQ).connected)
    freq_in = GSL_MODULE_IBUFFER (module, BSE_STANDARD_OSC_ICHANNEL_FREQ);
  if (GSL_MODULE_ISTREAM (module, BSE_STANDARD_OSC_ICHANNEL_FREQ_MOD).connected)
    mod_in = GSL_MODULE_IBUFFER (module, BSE_STANDARD_OSC_ICHANNEL_FREQ_MOD);
  if (GSL_MODULE_ISTREAM (module, BSE_STANDARD_OSC_ICHANNEL_SYNC).connected)
    sync_in = GSL_MODULE_IBUFFER (module, BSE_STANDARD_OSC_ICHANNEL_SYNC);
  if (GSL_MODULE_ISTREAM (module, BSE_STANDARD_OSC_ICHANNEL_PWM).connected)
    pwm_in = GSL_MODULE_IBUFFER (module, BSE_STANDARD_OSC_ICHANNEL_PWM);
  
  if (osc->config.table->wave_form == GSL_OSC_WAVE_PULSE_SAW)
    gsl_osc_process_pulse (osc, n_values, freq_in, mod_in, sync_in, pwm_in, osc_out, sync_out);
  else
    gsl_osc_process (osc, n_values, freq_in, mod_in, sync_in, osc_out, sync_out);
}

static void
bse_standard_osc_context_create (BseSource *source,
				 guint      context_handle,
				 GslTrans  *trans)
{
  static const GslClass sosc_class = {
    BSE_STANDARD_OSC_N_ICHANNELS, /* n_istreams */
    0,                            /* n_jstreams */
    BSE_STANDARD_OSC_N_OCHANNELS, /* n_ostreams */
    standard_osc_process,	  /* process */
    NULL,                         /* process_defer */
    standard_osc_reset,           /* reset */
    (GslModuleFreeFunc) g_free,	  /* free */
    GSL_COST_NORMAL,		  /* cost */
  };
  BseStandardOsc *self = BSE_STANDARD_OSC (source);
  GslOscData *osc = g_new0 (GslOscData, 1);
  GslModule *module;
  
  gsl_osc_reset (osc);
  gsl_osc_config (osc, &self->config);
  module = gsl_module_new (&sosc_class, osc);
  
  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);
  
  /* commit module to engine */
  gsl_trans_add (trans, gsl_job_integrate (module));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}

static void
bse_standard_osc_reset (BseSource *source)
{
  BseStandardOsc *self = BSE_STANDARD_OSC (source);
  
  gsl_osc_table_free (self->config.table);
  self->config.table = NULL;
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}
