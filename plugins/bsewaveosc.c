/* BseWaveOsc - BSE Wave Oscillator
 * Copyright (C) 2001 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "bsewaveosc.h"

#include <bse/gslengine.h>
#include <bse/gslwavechunk.h>
#include <bse/gslfilter.h>



/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_WAVE,
  PARAM_FM_PERC,
  PARAM_FM_EXP,
  PARAM_FM_OCTAVES
};


/* --- prototypes --- */
static void	bse_wave_osc_init		(BseWaveOsc		*wosc);
static void	bse_wave_osc_class_init		(BseWaveOscClass	*class);
static void	bse_wave_osc_class_finalize	(BseWaveOscClass	*class);
static void	bse_wave_osc_destroy		(BseObject		*object);
static void	bse_wave_osc_set_property	(BseWaveOsc		*wosc,
						 guint			 param_id,
						 GValue			*value,
						 GParamSpec		*pspec);
static void	bse_wave_osc_get_property	(BseWaveOsc		*wosc,
						 guint			 param_id,
						 GValue			*value,
						 GParamSpec		*pspec);
static void     bse_wave_osc_prepare		(BseSource		*source);
static void	bse_wave_osc_context_create	(BseSource		*source,
						 guint			 context_handle,
						 GslTrans		*trans);
static void     bse_wave_osc_reset		(BseSource		*source);
static void	bse_wave_osc_update_modules	(BseWaveOsc		*wosc);


/* --- variables --- */
static GType		 type_id_wave_osc = 0;
static gpointer		 parent_class = NULL;
static const GTypeInfo type_info_wave_osc = {
  sizeof (BseWaveOscClass),
  
  (GBaseInitFunc) NULL,
  (GBaseFinalizeFunc) NULL,
  (GClassInitFunc) bse_wave_osc_class_init,
  (GClassFinalizeFunc) bse_wave_osc_class_finalize,
  NULL /* class_data */,
  
  sizeof (BseWaveOsc),
  0 /* n_preallocs */,
  (GInstanceInitFunc) bse_wave_osc_init,
};


/* --- functions --- */
static void
bse_wave_osc_class_init (BseWaveOscClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ochannel, ichannel;
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = (GObjectSetPropertyFunc) bse_wave_osc_set_property;
  gobject_class->get_property = (GObjectGetPropertyFunc) bse_wave_osc_get_property;

  object_class->destroy = bse_wave_osc_destroy;
  
  source_class->prepare = bse_wave_osc_prepare;
  source_class->context_create = bse_wave_osc_context_create;
  source_class->reset = bse_wave_osc_reset;
  
  bse_object_class_add_param (object_class, "Wave",
			      PARAM_WAVE,
			      g_param_spec_object ("wave", "Wave", "Wave to play",
						   BSE_TYPE_WAVE,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Modulation",
			      PARAM_FM_PERC,
			      bse_param_spec_float ("fm_perc", "Input Modulation [%]",
						    "Modulation Strength for linear frequency modulation",
						    0, 100.0,
						    10.0, 5.0,
						    BSE_PARAM_DEFAULT |
						    BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Modulation",
			      PARAM_FM_EXP,
			      bse_param_spec_boolean ("exponential_fm", "Exponential FM",
						      "Perform exponential frequency modulation"
						      "instead of linear",
						      FALSE,
						      BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Modulation",
			      PARAM_FM_OCTAVES,
			      bse_param_spec_float ("fm_n_octaves", "Octaves",
						    "Number of octaves to be affected by exponential frequency modulation",
						    0, 3.0,
						    1.0, 0.5,
						    BSE_PARAM_DEFAULT |
						    BSE_PARAM_HINT_SCALE));

  ichannel = bse_source_class_add_ichannel (source_class, "freq_in", "Frequency Input");
  g_assert (ichannel == BSE_WAVE_OSC_ICHANNEL_FREQ);
  ichannel = bse_source_class_add_ichannel (source_class, "sync_in", "Syncronization Input");
  g_assert (ichannel == BSE_WAVE_OSC_ICHANNEL_SYNC);
  ichannel = bse_source_class_add_ichannel (source_class, "mod_in", "Modulation Input");
  g_assert (ichannel == BSE_WAVE_OSC_ICHANNEL_MOD);
  ochannel = bse_source_class_add_ochannel (source_class, "wave_out", "Wave Output");
  g_assert (ochannel == BSE_WAVE_OSC_OCHANNEL_WAVE);
  ochannel = bse_source_class_add_ochannel (source_class, "gate_out", "Gate Output");
  g_assert (ochannel == BSE_WAVE_OSC_OCHANNEL_GATE);
}

static void
bse_wave_osc_class_finalize (BseWaveOscClass *class)
{
}

static void
bse_wave_osc_init (BseWaveOsc *wosc)
{
  wosc->wave = NULL;
  wosc->fm_strength = 10.0;
  wosc->n_octaves = 1;
  wosc->config.start_offset = 0;
  wosc->config.play_dir = +1;
  wosc->config.wchunk_data = NULL;
  wosc->config.wchunk_from_freq = NULL;
  wosc->config.fm_strength = wosc->fm_strength / 100.0;
  wosc->config.exponential_fm = FALSE;
  wosc->config.cfreq = 440.;
}

static void
bse_wave_osc_destroy (BseObject *object)
{
  BseWaveOsc *wosc = BSE_WAVE_OSC (object);

  if (wosc->wave)
    {
      g_object_unref (wosc->wave);
      wosc->wave = NULL;
    }

  /* chain parent class' handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
notify_wave_changed (BseWaveOsc *wosc)
{
  g_object_notify (G_OBJECT (wosc), "wave");
}

static void
wave_uncross (BseItem *owner,
	      BseItem *ref_item)
{
  BseWaveOsc *wosc = BSE_WAVE_OSC (owner);

  wosc->wave = NULL;
  wosc->config.wchunk_data = NULL;
  wosc->config.wchunk_from_freq = NULL;
  bse_wave_osc_update_modules (wosc);

  if (BSE_SOURCE_PREPARED (wosc))
    {
      /* need to make sure our modules know about BseWave vanishing
       * before we return
       */
      gsl_engine_wait_on_trans ();
    }
  g_object_notify (G_OBJECT (wosc), "wave");
}

static void
bse_wave_osc_set_property (BseWaveOsc  *wosc,
			   guint        param_id,
			   GValue      *value,
			   GParamSpec  *pspec)
{
  switch (param_id)
    {
    case PARAM_WAVE:
      if (wosc->wave)
	{
	  g_object_disconnect (wosc->wave,
			       "any_signal", notify_wave_changed, wosc,
			       NULL);
	  bse_item_cross_unref (BSE_ITEM (wosc), BSE_ITEM (wosc->wave));
	  wosc->wave = NULL;
	  wosc->config.wchunk_data = NULL;
	  wosc->config.wchunk_from_freq = NULL;
	}
      wosc->wave = g_value_get_object (value);
      if (wosc->wave)
	{
	  bse_item_cross_ref (BSE_ITEM (wosc), BSE_ITEM (wosc->wave), wave_uncross);
	  g_object_connect (wosc->wave,
			    "swapped_signal::notify::name", notify_wave_changed, wosc,
			    NULL);
	  if (BSE_SOURCE_PREPARED (wosc))
	    {
	      wosc->config.wchunk_data = bse_wave_get_index_for_modules (wosc->wave);
	      if (wosc->config.wchunk_data)
		wosc->config.wchunk_from_freq = (gpointer) bse_wave_index_lookup_best;
	    }
	}
      bse_wave_osc_update_modules (wosc);
      if (BSE_SOURCE_PREPARED (wosc))
	{
	  /* need to make sure our modules know about BseWave vanishing
	   * before we return
	   */
	  gsl_engine_wait_on_trans ();
	}
      break;
    case PARAM_FM_PERC:
      wosc->fm_strength = g_value_get_float (value);
      if (!wosc->config.exponential_fm)
	{
	  wosc->config.fm_strength = wosc->fm_strength / 100.0;
	  bse_wave_osc_update_modules (wosc);
	}
      break;
    case PARAM_FM_EXP:
      wosc->config.exponential_fm = g_value_get_boolean (value);
      if (wosc->config.exponential_fm)
	wosc->config.fm_strength = wosc->n_octaves;
      else
	wosc->config.fm_strength = wosc->fm_strength / 100.0;
      bse_wave_osc_update_modules (wosc);
      break;
    case PARAM_FM_OCTAVES:
      wosc->n_octaves = g_value_get_float (value);
      if (wosc->config.exponential_fm)
	{
	  wosc->config.fm_strength = wosc->n_octaves;
	  bse_wave_osc_update_modules (wosc);
	}
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (wosc, param_id, pspec);
      break;
    }
}

static void
bse_wave_osc_get_property (BseWaveOsc   *wosc,
			   guint        param_id,
			   GValue      *value,
			   GParamSpec  *pspec)
{
  switch (param_id)
    {
    case PARAM_WAVE:
      g_value_set_object (value, wosc->wave);
      break;
    case PARAM_FM_PERC:
      g_value_set_float (value, wosc->fm_strength);
      break;
    case PARAM_FM_EXP:
      g_value_set_boolean (value, wosc->config.exponential_fm);
      break;
    case PARAM_FM_OCTAVES:
      g_value_set_float (value, wosc->n_octaves);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (wosc, param_id, pspec);
      break;
    }
}

static void
bse_wave_osc_prepare (BseSource *source)
{
  BseWaveOsc *wosc = BSE_WAVE_OSC (source);

  wosc->config.wchunk_data = bse_wave_get_index_for_modules (wosc->wave);
  if (wosc->config.wchunk_data)
    wosc->config.wchunk_from_freq = (gpointer) bse_wave_index_lookup_best;
  else
    wosc->config.wchunk_from_freq = NULL;

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

static void
wmod_access (GslModule *module,
	     gpointer   data)
{
  GslWaveOscData *wmod = module->user_data;
  GslWaveOscConfig *config = data;

  gsl_wave_osc_config (wmod, config);
}

static void
wmod_free (gpointer        data,
	   const GslClass *klass)
{
  GslWaveOscData *wmod = data;

  gsl_wave_osc_shutdown (wmod);
  g_free (wmod);
}

static void
bse_wave_osc_update_modules (BseWaveOsc *wosc)
{
  if (BSE_SOURCE_PREPARED (wosc))
    bse_source_access_omodules (BSE_SOURCE (wosc),
				BSE_WAVE_OSC_OCHANNEL_WAVE,
				wmod_access,
				g_memdup (&wosc->config, sizeof (wosc->config)),
				g_free,
				NULL);
}

static void
wmod_process (GslModule *module,
	      guint      n_values)
{
  GslWaveOscData *wmod = module->user_data;
  gfloat gate;

  gsl_wave_osc_process (wmod,
			n_values,
			GSL_MODULE_IBUFFER (module, BSE_WAVE_OSC_ICHANNEL_FREQ),
			GSL_MODULE_IBUFFER (module, BSE_WAVE_OSC_ICHANNEL_MOD),
			GSL_MODULE_IBUFFER (module, BSE_WAVE_OSC_ICHANNEL_SYNC),
			GSL_MODULE_OBUFFER (module, BSE_WAVE_OSC_OCHANNEL_WAVE));

  gate = wmod->done ? 0.0 : 1.0;
  module->ostreams[BSE_WAVE_OSC_OCHANNEL_GATE].values = gsl_engine_const_values (gate);
}

static void
bse_wave_osc_context_create (BseSource *source,
			     guint      context_handle,
			     GslTrans  *trans)
{
  static const GslClass wmod_class = {
    BSE_WAVE_OSC_N_ICHANNELS,	/* n_istreams */
    0,                          /* n_jstreams */
    BSE_WAVE_OSC_N_OCHANNELS,	/* n_ostreams */
    wmod_process,		/* process */
    wmod_free,			/* free */
    GSL_COST_NORMAL,		/* cost */
  };
  BseWaveOsc *wosc = BSE_WAVE_OSC (source);
  GslWaveOscData *wmod = g_new0 (GslWaveOscData, 1);
  GslModule *module;

  gsl_wave_osc_init (wmod);
  gsl_wave_osc_config (wmod, &wosc->config);

  module = gsl_module_new (&wmod_class, wmod);

  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);

  /* commit module to engine */
  gsl_trans_add (trans, gsl_job_integrate (module));

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}

static void
bse_wave_osc_reset (BseSource *source)
{
  BseWaveOsc *wosc = BSE_WAVE_OSC (source);

  wosc->config.wchunk_data = NULL;
  wosc->config.wchunk_from_freq = NULL;

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}


/* --- Export to BSE --- */
#include "./icons/waveosc.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_wave_osc, "BseWaveOsc", "BseSource",
    "BseWaveOsc is a waveeric oscillator source",
    &type_info_wave_osc,
    "/Source/Oscillators/Wave Oscillator",
    { WAVE_OSC_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      WAVE_OSC_IMAGE_WIDTH, WAVE_OSC_IMAGE_HEIGHT,
      WAVE_OSC_IMAGE_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;
