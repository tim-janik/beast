/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2002 Tim Janik
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
#include "bsebiquadfilter.h"

#include <bse/bsecategories.h>
#include <bse/bseengine.h>
#include <bse/bsemathsignal.h>

#define	DEBUG(...)      sfi_debug ("biquadfilter", __VA_ARGS__)
#define FREQ_DELTA      0.1

/* --- parameters --- */
enum
{
  PROP_0,
  PROP_FILTER_TYPE,
  PROP_FREQ,
  PROP_NOTE,
  PROP_GAIN,
  PROP_NORM_TYPE,
  PROP_FM_PERC,
  PROP_FM_EXP,
  PROP_FM_OCTAVES,
  PROP_GAIN_PERC
};


/* --- prototypes --- */
static void	   bse_biquad_filter_init		(BseBiquadFilter	*self);
static void	   bse_biquad_filter_class_init		(BseBiquadFilterClass	*class);
static void	   bse_biquad_filter_set_property	(GObject		*object,
							 guint			 param_id,
							 const GValue		*value,
							 GParamSpec		*pspec);
static void	   bse_biquad_filter_get_property	(GObject		*object,
							 guint			 param_id,
							 GValue			*value,
							 GParamSpec		*pspec);
static void	   bse_biquad_filter_context_create	(BseSource		*source,
							 guint			 context_handle,
							 BseTrans		*trans);
static void	   bse_biquad_filter_update_modules	(BseBiquadFilter	*self);


/* --- variables --- */
static gpointer	       parent_class = NULL;
static const GTypeInfo type_info_biquad_filter = {
  sizeof (BseBiquadFilterClass),
  
  (GBaseInitFunc) NULL,
  (GBaseFinalizeFunc) NULL,
  (GClassInitFunc) bse_biquad_filter_class_init,
  (GClassFinalizeFunc) NULL,
  NULL /* class_data */,
  
  sizeof (BseBiquadFilter),
  0 /* n_preallocs */,
  (GInstanceInitFunc) bse_biquad_filter_init,
};


/* --- functions --- */
BSE_BUILTIN_TYPE (BseBiquadFilter)
{
  static const GTypeInfo type_info = {
    sizeof (BseBiquadFilterClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_biquad_filter_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseBiquadFilter),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_biquad_filter_init,
  };
#include "./icons/biquad.c"
  GType type;
  
  type = bse_type_register_static (BSE_TYPE_SOURCE,
				   "BseBiquadFilter",
				   "BseBiquadFilter - an infinite impulse "
				   "response filter, consisting of 2-zeros/2-poles "
				   "segments (thus it's name: bi-quadratic). "
				   "Various types of filters are supported with "
				   "modulatable center (Cut-Off) frequency and "
				   "gain input signals.",
				   &type_info);
  bse_categories_register_stock_module (N_("/Filters/Biquad Types"), type, biquad_pixstream);
  return type;
}

static void
bse_biquad_filter_class_init (BseBiquadFilterClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint channel_id;
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = bse_biquad_filter_set_property;
  gobject_class->get_property = bse_biquad_filter_get_property;
  
  source_class->context_create = bse_biquad_filter_context_create;
  
  bse_object_class_add_param (object_class, _("Filter"),
			      PROP_FILTER_TYPE,
			      bse_param_spec_genum ("filter_type", _("Filter Type"), _("The filter design type"),
						    BSE_TYPE_BIQUAD_FILTER_TYPE,
						    BSE_BIQUAD_FILTER_RESONANT_LOWPASS,
						    SFI_PARAM_STANDARD));
  bse_object_class_add_param (object_class, _("Center Frequency"),
			      PROP_FREQ,
			      sfi_pspec_log_scale ("freq", _("Cutoff [Hz]"), NULL,
						   BSE_KAMMER_FREQUENCY_f * 2,
						   BSE_MIN_OSC_FREQUENCY_f, BSE_MAX_OSC_FREQUENCY_f - FREQ_DELTA,
						   5.0,
						   BSE_KAMMER_FREQUENCY_f * 2, 2, 4,
						   SFI_PARAM_STANDARD ":dial"));
  bse_object_class_add_param (object_class, _("Center Frequency"),
			      PROP_NOTE,
			      sfi_pspec_note ("note", _("Note"), NULL,
					      bse_note_from_freq (BSE_KAMMER_FREQUENCY_f * 2),
					      BSE_MIN_NOTE, BSE_MAX_NOTE,
					      FALSE,
					      SFI_PARAM_GUI));
  bse_object_class_add_param (object_class, _("Emphasis"),
			      PROP_GAIN,
			      sfi_pspec_real ("gain", _("Gain [dB]"), NULL,
					      3, -48., +48., 3,
					      SFI_PARAM_STANDARD ":dial"));
  bse_object_class_add_param (object_class, _("Emphasis"),
			      PROP_NORM_TYPE,
			      bse_param_spec_genum ("norm_type", _("Norm Type"),
						    _("The filter gain normalization type (supported only "
						      "by highpass and lowpass)"),
						    BSE_TYPE_BIQUAD_FILTER_NORM,
						    BSE_BIQUAD_FILTER_NORM_PASSBAND,
						    SFI_PARAM_STANDARD));
  bse_object_class_add_param (object_class, _("Modulation"),
			      PROP_FM_PERC,
			      sfi_pspec_real ("fm_perc", "Input Modulation [%]",
					      _("Strength of linear frequency modulation"),
					      0, 0.0, 100.0, 5.0,
					      SFI_PARAM_STANDARD ":scale"));
  bse_object_class_add_param (object_class, _("Modulation"),
			      PROP_FM_EXP,
			      sfi_pspec_bool ("exponential_fm", "Exponential FM",
					      _("Perform exponential frequency modulation "
						"instead of linear"),
					      FALSE, SFI_PARAM_STANDARD));
  bse_object_class_add_param (object_class, _("Modulation"),
			      PROP_FM_OCTAVES,
			      sfi_pspec_real ("fm_n_octaves", "Octaves",
					      _("Number of octaves to be affected by exponential frequency modulation"),
					      1.0, 0, 5.0, 0.01,
					      SFI_PARAM_STANDARD ":scale"));
  bse_object_class_add_param (object_class, _("Modulation"),
			      PROP_GAIN_PERC,
			      /* xgettext:no-c-format */
			      sfi_pspec_real ("gain_perc", _("Gain Modulation [%]"),
					      _("Strength of gain modulation"),
					      0.0, 0.0, 100.0, 5.0,
					      SFI_PARAM_STANDARD ":scale"));
  
  channel_id = bse_source_class_add_ichannel (source_class, "audio-in", _("Audio In"), _("Unfiltered Audio Signal"));
  g_assert (channel_id == BSE_BIQUAD_FILTER_ICHANNEL_AUDIO);
  channel_id = bse_source_class_add_ichannel (source_class, "freq-in", _("Freq In"), _("Center Frequency Input"));
  g_assert (channel_id == BSE_BIQUAD_FILTER_ICHANNEL_FREQ);
  channel_id = bse_source_class_add_ichannel (source_class, "freq-mod-in", _("Freq Mod In"), _("Frequency Modulation Input"));
  g_assert (channel_id == BSE_BIQUAD_FILTER_ICHANNEL_FREQ_MOD);
  channel_id = bse_source_class_add_ichannel (source_class, "gain-mod-in", _("Gain Mod In"), _("Gain Modulation Input"));
  g_assert (channel_id == BSE_BIQUAD_FILTER_ICHANNEL_GAIN_MOD);
  channel_id = bse_source_class_add_ochannel (source_class, "audio-out", _("Audio Out"), _("Filtered Audio Signal"));
  g_assert (channel_id == BSE_BIQUAD_FILTER_OCHANNEL_AUDIO);
}

static void
bse_biquad_filter_init (BseBiquadFilter *self)
{
  self->filter_type = BSE_BIQUAD_FILTER_RESONANT_LOWPASS;
  self->type_change = TRUE;
  self->exponential_fm = FALSE;
  self->freq = BSE_KAMMER_FREQUENCY_f * 2;
  self->fm_strength = 0.25;
  self->fm_n_octaves = 1;
  self->norm_type = BSE_BIQUAD_FILTER_NORM_PASSBAND;
  self->gain = 3;
  self->gain_strength = 0.25;
}

static void
bse_biquad_filter_set_property (GObject	     *object,
				guint	      param_id,
				const GValue *value,
				GParamSpec   *pspec)
{
  BseBiquadFilter *self = BSE_BIQUAD_FILTER (object);
  
  switch (param_id)
    {
    case PROP_FILTER_TYPE:
      self->filter_type = g_value_get_enum (value);
      self->type_change = TRUE;
      bse_biquad_filter_update_modules (self);
      break;
    case PROP_FREQ:
      self->freq = sfi_value_get_real (value);
      bse_biquad_filter_update_modules (self);
      g_object_notify (self, "note");
      break;
    case PROP_NOTE:
      self->freq = bse_note_to_freq (sfi_value_get_note (value));
      bse_biquad_filter_update_modules (self);
      g_object_notify (self, "freq");
      break;
    case PROP_FM_PERC:
      self->fm_strength = sfi_value_get_real (value) / 100.0;
      bse_biquad_filter_update_modules (self);
      break;
    case PROP_FM_EXP:
      self->exponential_fm = sfi_value_get_bool (value);
      bse_biquad_filter_update_modules (self);
      break;
    case PROP_FM_OCTAVES:
      self->fm_n_octaves = sfi_value_get_real (value);
      bse_biquad_filter_update_modules (self);
      break;
    case PROP_NORM_TYPE:
      self->norm_type = g_value_get_enum (value);
      self->type_change = TRUE;
      bse_biquad_filter_update_modules (self);
      break;
    case PROP_GAIN:
      self->gain = sfi_value_get_real (value);
      bse_biquad_filter_update_modules (self);
      break;
    case PROP_GAIN_PERC:
      self->gain_strength = sfi_value_get_real (value) / 100.0;
      bse_biquad_filter_update_modules (self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_biquad_filter_get_property (GObject	   *object,
				guint	    param_id,
				GValue	   *value,
				GParamSpec *pspec)
{
  BseBiquadFilter *self = BSE_BIQUAD_FILTER (object);
  
  switch (param_id)
    {
    case PROP_FILTER_TYPE:
      g_value_set_enum (value, self->filter_type);
      break;
    case PROP_FREQ:
      sfi_value_set_real (value, self->freq);
      break;
    case PROP_NOTE:
      sfi_value_set_note (value, bse_note_from_freq (self->freq));
      break;
    case PROP_FM_PERC:
      sfi_value_set_real (value, self->fm_strength * 100.0);
      break;
    case PROP_FM_EXP:
      sfi_value_set_bool (value, self->exponential_fm);
      break;
    case PROP_FM_OCTAVES:
      sfi_value_set_real (value, self->fm_n_octaves);
      break;
    case PROP_NORM_TYPE:
      g_value_set_enum (value, self->norm_type);
      break;
    case PROP_GAIN:
      sfi_value_set_real (value, self->gain);
      break;
    case PROP_GAIN_PERC:
      sfi_value_set_real (value, self->gain_strength * 100.0);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

typedef struct {
  GslBiquadFilter       biquad;
  BseFrequencyModulator fm;
  GslBiquadConfig       config;
  gfloat		base_freq;
  gfloat		gain;
  gfloat		gain_strength;
  guint			clear_state : 1;
} FilterModule;

static void
biquad_filter_access (BseModule *module,
		      gpointer   data)
{
  FilterModule *fmod = module->user_data;
  const FilterModule *cfg = data;
  
  fmod->fm = cfg->fm;
  fmod->config = cfg->config;
  fmod->base_freq = cfg->base_freq;
  fmod->gain = cfg->gain;
  fmod->gain_strength = cfg->gain_strength;
  gsl_biquad_filter_config (&fmod->biquad, &fmod->config, cfg->clear_state);
}

static void
bse_biquad_filter_update_modules (BseBiquadFilter *self)
{
  if (BSE_SOURCE_PREPARED (self))
    {
      FilterModule *cfg = g_new0 (FilterModule, 1);
      gfloat nyquist_freq = 0.5 * bse_engine_sample_freq ();
      
      cfg->base_freq = MIN (self->freq, nyquist_freq);
      cfg->gain = self->gain;
      cfg->gain_strength = self->gain_strength;
      cfg->clear_state = self->type_change;
      self->type_change = FALSE;
      cfg->fm.fm_strength = self->exponential_fm ? self->fm_n_octaves : self->fm_strength;
      cfg->fm.exponential_fm = self->exponential_fm;
      cfg->fm.signal_freq = BSE_SIGNAL_FROM_FREQ (cfg->base_freq);
      cfg->fm.fine_tune = 0;
      gsl_biquad_config_init (&cfg->config, self->filter_type, self->norm_type);
      gsl_biquad_config_setup (&cfg->config, cfg->base_freq / nyquist_freq, cfg->gain, 0);
      bse_source_access_modules (BSE_SOURCE (self),
				 biquad_filter_access, cfg, g_free,
				 NULL);
      
      
      if (1)
	{
	  GslBiquadFilter biquad, approx;
	  GslBiquadConfig c;
	  
	  gsl_biquad_config_init (&c, self->filter_type, self->norm_type);
	  gsl_biquad_config_setup (&c, self->freq / nyquist_freq, self->gain, 0);
	  gsl_biquad_filter_config (&biquad, &c, TRUE);
	  DEBUG ("Bxx(z) = (%.14g + (%.14g + %.14g * z) * z) / (1 + (%.14g + %.14g * z) * z)\n",
                 biquad.xc0, biquad.xc1, biquad.xc2, biquad.yc1, biquad.yc2);
	  
	  gsl_biquad_config_approx_gain (&c, self->gain);
	  gsl_biquad_filter_config (&approx, &c, TRUE);
	  DEBUG ("Byy(z) = (%.14g + (%.14g + %.14g * z) * z) / (1 + (%.14g + %.14g * z) * z)\n",
                 approx.xc0, approx.xc1, approx.xc2, approx.yc1, approx.yc2);
	  DEBUG ("Bdd(z) = (%.14g + (%.14g + %.14g * z) * z) / (1 + (%.14g + %.14g * z) * z)\n",
                 biquad.xc0 - approx.xc0,
                 biquad.xc1 - approx.xc1,
                 biquad.xc2 - approx.xc2,
                 biquad.yc1 - approx.yc1,
                 biquad.yc2 - approx.yc2);
	}
    }
}

static void
biquad_filter_reset (BseModule *module)
{
  FilterModule *fmod = module->user_data;
  gfloat nyquist_freq = 0.5 * bse_engine_sample_freq ();
  
  gsl_biquad_config_setup (&fmod->config, fmod->base_freq / nyquist_freq, fmod->gain, 0);
  gsl_biquad_filter_config (&fmod->biquad, &fmod->config, TRUE);
}

static void
biquad_filter_process (BseModule *module,
		       guint      n_values)
{
  FilterModule *fmod = module->user_data;
  const gfloat *audio_in = BSE_MODULE_IBUFFER (module, BSE_BIQUAD_FILTER_ICHANNEL_AUDIO);
  gfloat *sig_out = BSE_MODULE_OBUFFER (module, BSE_BIQUAD_FILTER_OCHANNEL_AUDIO);
  gfloat *bound = sig_out + n_values;
  gboolean sig_out_as_freq = TRUE;
  
  if (BSE_MODULE_ISTREAM (module, BSE_BIQUAD_FILTER_ICHANNEL_FREQ).connected &&
      BSE_MODULE_ISTREAM (module, BSE_BIQUAD_FILTER_ICHANNEL_FREQ_MOD).connected)
    bse_frequency_modulator (&fmod->fm,
			     n_values,
			     BSE_MODULE_IBUFFER (module, BSE_BIQUAD_FILTER_ICHANNEL_FREQ),
			     BSE_MODULE_IBUFFER (module, BSE_BIQUAD_FILTER_ICHANNEL_FREQ_MOD),
			     sig_out);
  else if (BSE_MODULE_ISTREAM (module, BSE_BIQUAD_FILTER_ICHANNEL_FREQ).connected)
    bse_frequency_modulator (&fmod->fm,
			     n_values,
			     BSE_MODULE_IBUFFER (module, BSE_BIQUAD_FILTER_ICHANNEL_FREQ),
			     NULL,
			     sig_out);
  else if (BSE_MODULE_ISTREAM (module, BSE_BIQUAD_FILTER_ICHANNEL_FREQ_MOD).connected)
    bse_frequency_modulator (&fmod->fm,
			     n_values,
			     NULL,
			     BSE_MODULE_IBUFFER (module, BSE_BIQUAD_FILTER_ICHANNEL_FREQ_MOD),
			     sig_out);
  else
    sig_out_as_freq = FALSE;
  
  if (BSE_MODULE_ISTREAM (module, BSE_BIQUAD_FILTER_ICHANNEL_GAIN_MOD).connected && sig_out_as_freq)
    {
      const gfloat *gain_in = BSE_MODULE_IBUFFER (module, BSE_BIQUAD_FILTER_ICHANNEL_GAIN_MOD);
      gfloat freq, nyquist = 0.5 * bse_engine_sample_freq ();
      gfloat last_gain = fmod->config.gain / fmod->gain_strength;
      gfloat last_freq = BSE_SIGNAL_FROM_FREQ (fmod->config.f_fn * nyquist);
      nyquist = 1.0 / nyquist;
      do
	{
	  guint n = MIN (bound - sig_out, bse_engine_control_raster ());
	  
	  if (UNLIKELY (BSE_SIGNAL_FREQ_CHANGED (*sig_out, last_freq)))
	    {
	      last_freq = *sig_out;
	      freq = BSE_SIGNAL_TO_FREQ (last_freq) * nyquist;
	      gsl_biquad_config_approx_freq (&fmod->config, CLAMP (freq, 0.0, 1.0));
	      if (BSE_SIGNAL_GAIN_CHANGED (*gain_in, last_gain))
		{
		  last_gain = *gain_in;
		  gsl_biquad_config_approx_gain (&fmod->config, fmod->gain * (1.0 + last_gain * fmod->gain_strength));
		}
	      gsl_biquad_filter_config (&fmod->biquad, &fmod->config, FALSE);
	    }
	  else if (UNLIKELY (BSE_SIGNAL_GAIN_CHANGED (*gain_in, last_gain)))
	    {
	      last_gain = *gain_in;
	      gsl_biquad_config_approx_gain (&fmod->config, fmod->gain * (1.0 + last_gain * fmod->gain_strength));
	      gsl_biquad_filter_config (&fmod->biquad, &fmod->config, FALSE);
	    }
	  gain_in += n;
	  gsl_biquad_filter_eval (&fmod->biquad, n, audio_in, sig_out);
	  audio_in += n;
	  sig_out += n;
	}
      while (sig_out < bound);
    }
  else if (sig_out_as_freq)
    {
      gfloat freq, nyquist = 0.5 * bse_engine_sample_freq ();
      gfloat last_freq = BSE_SIGNAL_FROM_FREQ (fmod->config.f_fn * nyquist);
      nyquist = 1.0 / nyquist;
      do
	{
          guint n = MIN (bound - sig_out, bse_engine_control_raster ());
	  
          if (UNLIKELY (BSE_SIGNAL_FREQ_CHANGED (*sig_out, last_freq)))
	    {
	      last_freq = *sig_out;
	      freq = BSE_SIGNAL_TO_FREQ (last_freq) * nyquist;
	      gsl_biquad_config_approx_freq (&fmod->config, CLAMP (freq, 0.0, 1.0));
	      gsl_biquad_filter_config (&fmod->biquad, &fmod->config, FALSE);
	    }
	  gsl_biquad_filter_eval (&fmod->biquad, n, audio_in, sig_out);
	  audio_in += n;
	  sig_out += n;
	}
      while (sig_out < bound);
    }
  else if (BSE_MODULE_ISTREAM (module, BSE_BIQUAD_FILTER_ICHANNEL_GAIN_MOD).connected)
    {
      const gfloat *gain_in = BSE_MODULE_IBUFFER (module, BSE_BIQUAD_FILTER_ICHANNEL_GAIN_MOD);
      gfloat last_gain = fmod->config.gain / fmod->gain_strength;
      do
	{
          guint n = MIN (bound - sig_out, bse_engine_control_raster ());
	  
	  if (UNLIKELY (BSE_SIGNAL_GAIN_CHANGED (*gain_in, last_gain)))
	    {
	      last_gain = *gain_in;
              gsl_biquad_config_approx_gain (&fmod->config, fmod->gain * (1.0 + last_gain * fmod->gain_strength));
	      gsl_biquad_filter_config (&fmod->biquad, &fmod->config, FALSE);
	    }
	  gain_in += n;
	  gsl_biquad_filter_eval (&fmod->biquad, n, audio_in, sig_out);
	  audio_in += n;
	  sig_out += n;
	}
      while (sig_out < bound);
    }
  else
    gsl_biquad_filter_eval (&fmod->biquad, n_values, audio_in, sig_out);
}

static void
bse_biquad_filter_context_create (BseSource *source,
				  guint      context_handle,
				  BseTrans  *trans)
{
  static const BseModuleClass biquad_filter_class = {
    BSE_BIQUAD_FILTER_N_ICHANNELS,	/* n_istreams */
    0,					/* n_jstreams */
    BSE_BIQUAD_FILTER_N_OCHANNELS,	/* n_ostreams */
    biquad_filter_process,		/* process */
    NULL,                       	/* process_defer */
    biquad_filter_reset,		/* reset */
    (BseModuleFreeFunc) g_free,		/* free */
    BSE_COST_NORMAL,			/* flags */
  };
  BseBiquadFilter *self = BSE_BIQUAD_FILTER (source);
  FilterModule *fmod = g_new0 (FilterModule, 1);
  gfloat nyquist_freq = 0.5 * bse_engine_sample_freq ();
  BseModule *module;
  
  fmod->base_freq = MIN (self->freq, nyquist_freq);
  fmod->gain = self->gain;
  fmod->gain_strength = self->gain_strength;
  fmod->clear_state = TRUE;
  fmod->fm.fm_strength = self->exponential_fm ? self->fm_n_octaves : self->fm_strength;
  fmod->fm.exponential_fm = self->exponential_fm;
  fmod->fm.signal_freq = BSE_SIGNAL_FROM_FREQ (fmod->base_freq);
  fmod->fm.fine_tune = 0;
  gsl_biquad_config_init (&fmod->config, self->filter_type, self->norm_type);
  gsl_biquad_config_setup (&fmod->config, fmod->base_freq / nyquist_freq, fmod->gain, 0);
  
  module = bse_module_new (&biquad_filter_class, fmod);
  
  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);
  
  /* commit module to engine */
  bse_trans_add (trans, bse_job_integrate (module));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}
