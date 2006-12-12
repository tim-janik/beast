/* DavSynDrum - DAV Drum Synthesizer
 * Copyright (c) 1999, 2000 David A. Bartold, 2003 Tim Janik
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
#include "davsyndrum.h"
#include <bse/bseengine.h>
#include <bse/bsemathsignal.h>

/* --- parameters --- */
enum
{
  PROP_0,
  PROP_BASE_FREQ,
  PROP_BASE_NOTE,
  PROP_TRIGGER_VEL,
  PROP_TRIGGER_HIT,
  PROP_RES,
  PROP_RATIO
};


/* --- prototypes --- */
static void dav_syn_drum_init           (DavSynDrum      *drum);
static void dav_syn_drum_class_init     (DavSynDrumClass *class);
static void dav_syn_drum_set_property   (GObject         *object,
                                         guint            param_id,
                                         const GValue    *value,
                                         GParamSpec      *pspec);
static void dav_syn_drum_get_property   (GObject         *object,
                                         guint            param_id,
                                         GValue          *value,
                                         GParamSpec      *pspec);
static void dav_syn_drum_prepare        (BseSource       *source);
static void dav_syn_drum_context_create (BseSource       *source,
                                         guint            context_handle,
                                         BseTrans        *trans);
static void dav_syn_drum_update_modules (DavSynDrum      *self,
                                         gboolean         force_trigger);


/* --- Export to DAV --- */
#include "./icons/drum.c"
BSE_REGISTER_OBJECT (DavSynDrum, BseSource, "/Modules/Audio Sources/Synthetic Drum", "",
                     "DavSynDrum produces synthesized drums. It accepts the drum frequency as "
                     "input channel or parameter setting. Drums are triggered through a trigger "
                     "parameter or via a trigger input channel which detects raising edges. "
                     "The initial frequency shift is controllable through the "
                     "\"Ratio In\" input channel, and adjustable through a parameter.",
                     drum_icon,
                     dav_syn_drum_class_init, NULL, dav_syn_drum_init);
BSE_DEFINE_EXPORTS ();


/* --- variables --- */
static gpointer          parent_class = NULL;


/* --- functions --- */
static void
dav_syn_drum_class_init (DavSynDrumClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ochannel_id, ichannel_id;
  
  parent_class = g_type_class_peek_parent (class);

  gobject_class->set_property = dav_syn_drum_set_property;
  gobject_class->get_property = dav_syn_drum_get_property;
  
  source_class->prepare = dav_syn_drum_prepare;
  source_class->context_create = dav_syn_drum_context_create;

  bse_object_class_add_param (object_class, _("Frequency"), PROP_BASE_FREQ,
                              bse_param_spec_freq ("base_freq", _("Frequency"),
                                                   _("Drum frequency in Herz"),
                                                   bse_note_to_freq (BSE_MUSICAL_TUNING_12_TET, SFI_NOTE_Gis (-1)),
                                                   BSE_MIN_OSC_FREQUENCY, BSE_MAX_OSC_FREQUENCY,
                                                   SFI_PARAM_STANDARD ":dial"));
  bse_object_class_add_param (object_class, _("Frequency"), PROP_BASE_NOTE,
                              bse_pspec_note_simple ("base_note", _("Note"),
                                                     _("Drum frequency as note, converted to Herz according to the current musical tuning"),
                                                     SFI_PARAM_GUI));

  bse_object_class_add_param (object_class, "Trigger", PROP_TRIGGER_VEL,
			      sfi_pspec_real ("trigger_vel", _("Trigger Velocity [%]"),
                                              _("The velocity of the drum hit"),
                                              100.0, 0.0, 1000.0, 10.0,
                                              SFI_PARAM_STANDARD ":scale"));
  bse_object_class_add_param (object_class, _("Trigger"), PROP_TRIGGER_HIT,
			      sfi_pspec_bool ("force_trigger", _("Trigger Hit"), _("Manual trigger for the drum"),
                                              FALSE, SFI_PARAM_GUI ":trigger:skip-undo"));
  bse_object_class_add_param (object_class, _("Parameters"), PROP_RES,
			      sfi_pspec_real ("res", _("Resonance"),
                                              _("The resonance half life in number of milli seconds"),
                                              50, 1, 1000.0, 2.5,
                                              SFI_PARAM_STANDARD ":scale"));
  bse_object_class_add_param (object_class, _("Parameters"), PROP_RATIO,
			      sfi_pspec_real ("ratio", _("Frequency Ratio"),
                                              _("The ratio of frequency shift. (i.e. 1.0 means shift equal to the drum's base frequency)"),
                                              1.0, 0.0, 10.0, 0.1,
                                              SFI_PARAM_STANDARD ":scale"));

  ichannel_id = bse_source_class_add_ichannel (source_class, "freq-in", _("Freq In"), _("Drum Frequency Input"));
  g_assert (ichannel_id == DAV_SYN_DRUM_ICHANNEL_FREQ);
  ichannel_id = bse_source_class_add_ichannel (source_class, "ratio-in", _("Ratio In"), _("Frequency shift ratio (assumed 1.0 if not connected)"));
  g_assert (ichannel_id == DAV_SYN_DRUM_ICHANNEL_RATIO);
  ichannel_id = bse_source_class_add_ichannel (source_class, "trigger-in", _("Trigger In"), _("Hit the drum on raising edges"));
  g_assert (ichannel_id == DAV_SYN_DRUM_ICHANNEL_TRIGGER);
  ochannel_id = bse_source_class_add_ochannel (source_class, "audio-out", _("Audio Out"), _("SynDrum Output"));
  g_assert (ochannel_id == DAV_SYN_DRUM_OCHANNEL_MONO);
}

static void
dav_syn_drum_init (DavSynDrum *self)
{
  self->params.freq = bse_note_to_freq (bse_item_current_musical_tuning (BSE_ITEM (self)), SFI_NOTE_Gis (-1));
  self->params.trigger_vel = 100.0 * 0.01;
  self->params.ratio = 1.0;
  self->params.res = 0;
  self->half = 50 * 0.001;
}

static void
dav_syn_drum_set_property (GObject         *object,
                           guint            param_id,
                           const GValue    *value,
                           GParamSpec      *pspec)
{
  DavSynDrum *self = DAV_SYN_DRUM (object);
  gboolean force_trigger = FALSE;
  switch (param_id)
    {
    case PROP_BASE_FREQ:
      self->params.freq = sfi_value_get_real (value);
      g_object_notify (self, "base-note");
      break;
    case PROP_BASE_NOTE:
      self->params.freq = bse_note_to_freq (bse_item_current_musical_tuning (BSE_ITEM (self)), sfi_value_get_note (value));
      g_object_notify (self, "base-freq");
      break;
    case PROP_RATIO:
      self->params.ratio = sfi_value_get_real (value);
      break;
    case PROP_RES:
      self->half = sfi_value_get_real (value) * 0.001;
      break;
    case PROP_TRIGGER_VEL:
      self->params.trigger_vel = sfi_value_get_real (value) * 0.01;
      break;
    case PROP_TRIGGER_HIT:
      force_trigger = TRUE;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
  dav_syn_drum_update_modules (self, force_trigger);
}

static void
dav_syn_drum_get_property (GObject         *object,
                           guint            param_id,
                           GValue          *value,
                           GParamSpec      *pspec)
{
  DavSynDrum *self = DAV_SYN_DRUM (object);
  switch (param_id)
    {
    case PROP_BASE_FREQ:
      sfi_value_set_real (value, self->params.freq);
      break;
    case PROP_BASE_NOTE:
      sfi_value_set_note (value, bse_note_from_freq (bse_item_current_musical_tuning (BSE_ITEM (self)), self->params.freq));
      break;
    case PROP_TRIGGER_VEL:
      sfi_value_set_real (value, self->params.trigger_vel * 100.0);
      break;
    case PROP_TRIGGER_HIT:
      sfi_value_set_bool (value, FALSE);
      break;
    case PROP_RATIO:
      sfi_value_set_real (value, self->params.ratio);
      break;
    case PROP_RES:
      sfi_value_set_real (value, self->half * 1000);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
dav_syn_drum_prepare (BseSource *source)
{
  DavSynDrum *self = DAV_SYN_DRUM (source);

  /* initialize calculated params (mix-freq dependant) */
  dav_syn_drum_update_modules (self, FALSE);

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

static inline void
dmod_trigger (DavSynDrumModule *dmod,
              gfloat            freq,
              gfloat            ratio)
{
  dmod->spring_vel = dmod->params.trigger_vel;
  dmod->env = dmod->params.trigger_vel;
  dmod->freq_rad = freq * 2.0 * PI / bse_engine_sample_freq();
  dmod->freq_shift = dmod->freq_rad * dmod->params.ratio * CLAMP (ratio, 0, 1.0);
}

static void
dmod_process (BseModule *module,
              guint      n_values)
{
  DavSynDrumModule *dmod = module->user_data;
  const gfloat *freq_in = BSE_MODULE_IBUFFER (module, DAV_SYN_DRUM_ICHANNEL_FREQ);
  const gfloat *ratio_in = BSE_MODULE_IBUFFER (module, DAV_SYN_DRUM_ICHANNEL_RATIO);
  const gfloat *trigger_in = BSE_MODULE_IBUFFER (module, DAV_SYN_DRUM_ICHANNEL_TRIGGER);
  gfloat *wave_out = BSE_MODULE_OBUFFER (module, DAV_SYN_DRUM_OCHANNEL_MONO);
  const gfloat res = dmod->params.res;
  gfloat freq_rad = dmod->freq_rad;
  gfloat freq_shift = dmod->freq_shift;
  gfloat last_trigger_level = dmod->last_trigger_level;
  gfloat spring_vel = dmod->spring_vel;
  gfloat spring_pos = dmod->spring_pos;
  gfloat env = dmod->env;
  guint i;

  if (!BSE_MODULE_ISTREAM (module, DAV_SYN_DRUM_ICHANNEL_FREQ).connected)
    freq_in = NULL;
  if (!BSE_MODULE_ISTREAM (module, DAV_SYN_DRUM_ICHANNEL_RATIO).connected)
    ratio_in = NULL;

  for (i = 0; i < n_values; i++)
    {
      gfloat cur_freq;

      /* check input triggers */
      if (G_UNLIKELY (BSE_SIGNAL_RAISING_EDGE (last_trigger_level, trigger_in[i])))
        {
          /* trigger drum */
          dmod_trigger (dmod,
                        freq_in ? BSE_FREQ_FROM_VALUE (freq_in[i]) : dmod->params.freq,
                        ratio_in ? ratio_in[i] : 1.0);
          spring_vel = dmod->spring_vel;
          env = dmod->env;
          freq_rad = dmod->freq_rad;
          freq_shift = dmod->freq_shift;
        }
      last_trigger_level = trigger_in[i];

      cur_freq = freq_rad + (env * freq_shift);
      spring_vel -= spring_pos * cur_freq;
      spring_pos += spring_vel * cur_freq;
      spring_vel *= res;
      env *= res;

      wave_out[i] = spring_pos;
    }

  dmod->env = env;
  dmod->spring_pos = spring_pos;
  dmod->spring_vel = spring_vel;
  dmod->last_trigger_level = last_trigger_level;
}

static void
dmod_reset (BseModule *module)
{
  DavSynDrumModule *dmod = module->user_data;
  /* this function is called whenever we need to start from scratch */
  dmod->last_trigger_level = 0;
  dmod->spring_vel = 0.0;
  dmod->spring_pos = 0.0;
  dmod->env = 0.0;
  dmod->freq_rad = 0;
  dmod->freq_shift = 0;
}

static void
dav_syn_drum_context_create (BseSource *source,
                             guint      context_handle,
                             BseTrans  *trans)
{
  static const BseModuleClass dmod_class = {
    DAV_SYN_DRUM_N_ICHANNELS,           /* n_istreams */
    0,                                  /* n_jstreams */
    DAV_SYN_DRUM_N_OCHANNELS,           /* n_ostreams */
    dmod_process,                       /* process */
    NULL,                               /* process_defer */
    dmod_reset,                         /* reset */
    (BseModuleFreeFunc) g_free,         /* free */
    BSE_COST_NORMAL,                    /* cost */
  };
  DavSynDrum *self = DAV_SYN_DRUM (source);
  DavSynDrumModule *dmod = g_new0 (DavSynDrumModule, 1);
  BseModule *module;

  dmod->params = self->params;
  module = bse_module_new (&dmod_class, dmod);
  dmod_reset (module);

  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);

  /* commit module to engine */
  bse_trans_add (trans, bse_job_integrate (module));

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}

/* update module configuration from new parameter set */
static void
dmod_access (BseModule *module,
             gpointer   data)
{
  DavSynDrumModule *dmod = module->user_data;
  DavSynDrumParams *params = data;

  dmod->params = *params;
}

/* update module configuration from new parameter set and force trigger */
static void
dmod_access_trigger (BseModule *module,
                     gpointer   data)
{
  DavSynDrumModule *dmod = module->user_data;
  DavSynDrumParams *params = data;

  dmod->params = *params;
  dmod_trigger (dmod, dmod->params.freq, 1.0);
}

static void
dav_syn_drum_update_modules (DavSynDrum *self,
                             gboolean    force_trigger)
{
  if (BSE_SOURCE_PREPARED (self))
    {
      /* Calculate the half life rate given:
       *  half - the length of the half life
       *  rate - time divisor (usually the # calcs per second)
       *
       * Basically, find r given 1/2 = e^(-r*(half/rate))
       *
       * ln(1/2) = -ln(2) = -BSE_LN2 = -0.693147...
       */
      self->params.res = exp (-BSE_LN2 / (self->half * bse_engine_sample_freq()));

      /* update all DavSynDrumModules. take a look at davxtalstrings.c
       * if you don't understand what this code does.
       */
      bse_source_access_modules (BSE_SOURCE (self),
                                 force_trigger ? dmod_access_trigger : dmod_access,
                                 g_memdup (&self->params, sizeof (self->params)),
                                 g_free,
                                 NULL);
    }
}
