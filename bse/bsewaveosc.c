/* BSE - Better Sound Engine
 * Copyright (C) 2001-2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "bsewaveosc.h"
#include <bse/bsemain.h>
#include <bse/bsecategories.h>
#include <bse/bseeditablesample.h>
#include <bse/bsewaverepo.h>
#include <bse/bseproject.h>
#include <bse/bseengine.h>
#include <bse/gslwavechunk.h>
#include <bse/gslfilter.h>



/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_WAVE,
  PARAM_CHANNEL,
  PARAM_FM_PERC,
  PARAM_FM_EXP,
  PARAM_FM_OCTAVES
};


/* --- prototypes --- */
static void     bse_wave_osc_init               (BseWaveOsc             *self);
static void     bse_wave_osc_class_init         (BseWaveOscClass        *class);
static void     bse_wave_osc_dispose            (GObject                *object);
static void     bse_wave_osc_finalize           (GObject                *object);
static void     bse_wave_osc_set_property       (GObject                *object,
                                                 guint                   param_id,
                                                 const GValue           *value,
                                                 GParamSpec             *pspec);
static void     bse_wave_osc_get_property       (GObject                *object,
                                                 guint                   param_id,
                                                 GValue                 *value,
                                                 GParamSpec             *pspec);
static void     bse_wave_osc_context_create     (BseSource              *source,
                                                 guint                   context_handle,
                                                 BseTrans               *trans);
static void   bse_wave_osc_update_config_wchunk (BseWaveOsc             *self);
static void     bse_wave_osc_update_modules     (BseWaveOsc             *self);


/* --- variables --- */
static gpointer parent_class = NULL;
static guint    signal_notify_pcm_position = 0;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseWaveOsc)
{
  static const GTypeInfo type_info = {
    sizeof (BseWaveOscClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_wave_osc_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseWaveOsc),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_wave_osc_init,
  };
#include "./icons/waveosc.c"
  GType type;
  
  type = bse_type_register_static (BSE_TYPE_SOURCE,
                                   "BseWaveOsc",
                                   "BseWaveOsc is a wave based oscillator module. "
                                   "It plays waves at arbitrary frequency with little to no "
                                   "aliasing artefacts by using a tschbyscheff type II resampling filter. "
                                   "The plaback frequency can be specified through a frequency input, and be "
                                   "modulated by another control signal with linear or exponential frequency "
                                   "response.",
                                   __FILE__, __LINE__,
                                   &type_info);
  bse_categories_register_stock_module(N_("/Audio Sources/Wave Oscillator"), type, waveosc_pixstream);
  return type;
}

static void
bse_wave_osc_init (BseWaveOsc *self)
{
  self->wave = NULL;
  self->esample_wchunk = NULL;
  self->fm_strength = 10.0;
  self->n_octaves = 1;
  self->config.start_offset = 0;
  self->config.play_dir = +1;
  self->config.channel = 0;
  self->config.wchunk_data = NULL;
  self->config.lookup_wchunk = NULL;
  self->config.fm_strength = self->fm_strength / 100.0;
  self->config.exponential_fm = FALSE;
  self->config.cfreq = 440.;
}

static void
bse_wave_osc_get_candidates (BseItem               *item,
                             guint                  param_id,
                             BsePropertyCandidates *pc,
                             GParamSpec            *pspec)
{
  BseWaveOsc *self = BSE_WAVE_OSC (item);
  switch (param_id)
    {
      BseProject *project;
    case PARAM_WAVE:
      bse_property_candidate_relabel (pc, _("Available Waves"), _("List of available waves to choose as oscillator source"));
      project = bse_item_get_project (item);
      if (project)
        {
          BseWaveRepo *wrepo = bse_project_get_wave_repo (project);
          bse_item_gather_items_typed (BSE_ITEM (wrepo), pc->items, BSE_TYPE_WAVE, BSE_TYPE_WAVE_REPO, FALSE);
        }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
wave_osc_uncross_wave (BseItem *owner,
                       BseItem *ref_item)
{
  BseWaveOsc *self = BSE_WAVE_OSC (owner);
  bse_item_set (self, "wave", NULL, NULL);
}

static void
clear_wave_and_esample (BseWaveOsc *self)
{
  if (self->wave)
    {
      BseWave *wave = self->wave;
      bse_item_cross_unlink (BSE_ITEM (self), BSE_ITEM (self->wave), wave_osc_uncross_wave);
      bse_object_unproxy_notifies (self->wave, self, "notify::wave");
      self->wave = NULL;
      bse_wave_osc_update_config_wchunk (self);
      bse_wave_osc_update_modules (self);
      if (BSE_SOURCE_PREPARED (self))
        {
          /* need to make sure our modules know about BseWave vanishing
           * before we continue (since bse_wave_drop_index() will destroy
           * the wchunk index)
           */
          bse_engine_wait_on_trans ();
        }
      bse_wave_drop_index (wave);
      g_object_notify (self, "wave");
    }

  if (self->esample_wchunk)
    {
      GslWaveChunk *esample_wchunk = self->esample_wchunk;
      self->esample_wchunk = NULL;
      bse_wave_osc_update_config_wchunk (self);
      bse_wave_osc_update_modules (self);
      if (BSE_SOURCE_PREPARED (self))
        {
          /* need to make sure our modules know about the wave chunk vanishing
           * before we return (so the wchunk update propagates)
           */
          bse_engine_wait_on_trans ();
        }
      gsl_wave_chunk_close (esample_wchunk);
    }
}

void
bse_wave_osc_set_from_esample (BseWaveOsc        *self,
                               BseEditableSample *esample)
{
  g_return_if_fail (BSE_WAVE_OSC (self));

  clear_wave_and_esample (self);
  if (esample)
    {
      g_return_if_fail (BSE_EDITABLE_SAMPLE (esample));

      if (esample->wchunk && gsl_wave_chunk_open (esample->wchunk) == BSE_ERROR_NONE)
        {
          self->esample_wchunk = esample->wchunk;
          bse_wave_osc_update_config_wchunk (self);
          bse_wave_osc_update_modules (self);
          if (BSE_SOURCE_PREPARED (self))
            bse_engine_wait_on_trans ();
        }
    }
}

static void
bse_wave_osc_set_property (GObject      *object,
                           guint         param_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  BseWaveOsc *self = BSE_WAVE_OSC (object);
  
  switch (param_id)
    {
      BseWave *wave;
    case PARAM_WAVE:
      wave = bse_value_get_object (value);
      if (wave != self->wave)
        {
          clear_wave_and_esample (self);
          self->wave = wave;
          if (self->wave)
            {
              bse_item_cross_link (BSE_ITEM (self), BSE_ITEM (self->wave), wave_osc_uncross_wave);
              bse_object_proxy_notifies (self->wave, self, "notify::wave");
              bse_wave_request_index (self->wave);
              bse_wave_osc_update_config_wchunk (self);
              bse_wave_osc_update_modules (self);
              if (BSE_SOURCE_PREPARED (self))
                {
                  /* need to make sure our modules know about BseWave vanishing
                   * before we return (so the wchunk update propagates)
                   */
                  bse_engine_wait_on_trans ();
                }
            }
        }
      break;
    case PARAM_CHANNEL:
      self->config.channel = g_value_get_int (value) - 1;
      bse_wave_osc_update_modules (self);
      break;
    case PARAM_FM_PERC:
      self->fm_strength = sfi_value_get_real (value);
      if (!self->config.exponential_fm)
        {
          self->config.fm_strength = self->fm_strength / 100.0;
          bse_wave_osc_update_modules (self);
        }
      break;
    case PARAM_FM_EXP:
      self->config.exponential_fm = sfi_value_get_bool (value);
      if (self->config.exponential_fm)
        self->config.fm_strength = self->n_octaves;
      else
        self->config.fm_strength = self->fm_strength / 100.0;
      bse_wave_osc_update_modules (self);
      break;
    case PARAM_FM_OCTAVES:
      self->n_octaves = sfi_value_get_real (value);
      if (self->config.exponential_fm)
        {
          self->config.fm_strength = self->n_octaves;
          bse_wave_osc_update_modules (self);
        }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_wave_osc_get_property (GObject    *object,
                           guint       param_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  BseWaveOsc *self = BSE_WAVE_OSC (object);
  
  switch (param_id)
    {
    case PARAM_WAVE:
      bse_value_set_object (value, self->wave);
      break;
    case PARAM_CHANNEL:
      g_value_set_int (value, self->config.channel + 1);
      break;
    case PARAM_FM_PERC:
      sfi_value_set_real (value, self->fm_strength);
      break;
    case PARAM_FM_EXP:
      sfi_value_set_bool (value, self->config.exponential_fm);
      break;
    case PARAM_FM_OCTAVES:
      sfi_value_set_real (value, self->n_octaves);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_wave_osc_dispose (GObject *object)
{
  BseWaveOsc *self = BSE_WAVE_OSC (object);

  clear_wave_and_esample (self);
  
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
bse_wave_osc_finalize (GObject *object)
{
  BseWaveOsc *self = BSE_WAVE_OSC (object);
  
  if (self->esample_wchunk)
    gsl_wave_chunk_close (self->esample_wchunk);
  self->esample_wchunk = NULL;
  
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
wosc_access (BseModule *module,
             gpointer   data)
{
  GslWaveOscData *wosc = module->user_data;
  GslWaveOscConfig *config = data;
  
  /* this runs in the Gsl Engine threads */
  
  gsl_wave_osc_config (wosc, config);
}

static GslWaveChunk*
wchunk_from_data (gpointer wchunk_data,
                  gfloat   freq,
                  gfloat   velocity)
{
  return wchunk_data;
}

static void
bse_wave_osc_update_config_wchunk (BseWaveOsc *self)
{
  self->config.wchunk_data = NULL;
  self->config.lookup_wchunk = NULL;
  if (self->wave)
    {
      BseWaveIndex *index = bse_wave_get_index_for_modules (self->wave);
      self->config.wchunk_data = index && index->n_entries ? index : NULL;
      if (self->config.wchunk_data)
        self->config.lookup_wchunk = (gpointer) bse_wave_index_lookup_best;
    }
  else if (self->esample_wchunk)
    {
      self->config.wchunk_data = self->esample_wchunk;
      self->config.lookup_wchunk = wchunk_from_data;
    }
}

static void
bse_wave_osc_update_modules (BseWaveOsc *self)
{
  if (BSE_SOURCE_PREPARED (self))
    bse_source_access_modules (BSE_SOURCE (self),
                               wosc_access,
                               g_memdup (&self->config, sizeof (self->config)),
                               g_free,
                               NULL);
}

static void
wosc_free (gpointer        data,
           const BseModuleClass *klass)
{
  GslWaveOscData *wosc = data;
  
  gsl_wave_osc_shutdown (wosc);
  g_free (wosc);
}

static void
wosc_process (BseModule *module,
              guint      n_values)
{
  GslWaveOscData *wosc = module->user_data;
  gfloat gate, done;
  
  gsl_wave_osc_process (wosc,
                        n_values,
                        (BSE_MODULE_ISTREAM (module, BSE_WAVE_OSC_ICHANNEL_FREQ).connected ?
                         BSE_MODULE_IBUFFER (module, BSE_WAVE_OSC_ICHANNEL_FREQ) : NULL),
                        (BSE_MODULE_ISTREAM (module, BSE_WAVE_OSC_ICHANNEL_MOD).connected ?
                         BSE_MODULE_IBUFFER (module, BSE_WAVE_OSC_ICHANNEL_MOD) : NULL),
                        (BSE_MODULE_ISTREAM (module, BSE_WAVE_OSC_ICHANNEL_SYNC).connected ?
                         BSE_MODULE_IBUFFER (module, BSE_WAVE_OSC_ICHANNEL_SYNC) : NULL),
                        BSE_MODULE_OBUFFER (module, BSE_WAVE_OSC_OCHANNEL_WAVE));
  
  gate = wosc->done ? 0.0 : 1.0;
  done = wosc->done ? 1.0 : 0.0;
  module->ostreams[BSE_WAVE_OSC_OCHANNEL_GATE].values = bse_engine_const_values (gate);
  module->ostreams[BSE_WAVE_OSC_OCHANNEL_DONE].values = bse_engine_const_values (done);
}

static void
wosc_reset (BseModule *module)
{
  GslWaveOscData *wosc = module->user_data;
  gsl_wave_osc_reset (wosc);
}

static void
bse_wave_osc_context_create (BseSource *source,
                             guint      context_handle,
                             BseTrans  *trans)
{
  static const BseModuleClass wosc_class = {
    BSE_WAVE_OSC_N_ICHANNELS,   /* n_istreams */
    0,                          /* n_jstreams */
    BSE_WAVE_OSC_N_OCHANNELS,   /* n_ostreams */
    wosc_process,               /* process */
    NULL,                       /* process_defer */
    wosc_reset,                 /* reset */
    wosc_free,                  /* free */
    BSE_COST_NORMAL,            /* cost */
  };
  BseWaveOsc *self = BSE_WAVE_OSC (source);
  GslWaveOscData *wosc = g_new0 (GslWaveOscData, 1);
  BseModule *module;
  
  gsl_wave_osc_init (wosc);
  gsl_wave_osc_config (wosc, &self->config);
  
  module = bse_module_new (&wosc_class, wosc);
  
  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);
  
  /* commit module to engine */
  bse_trans_add (trans, bse_job_integrate (module));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}


typedef struct {
  BseWaveOsc *wosc;
  gfloat      perc;
  guint64     stamp;
  guint       module_pcm_position;
} PcmPos;

static void
pcm_pos_access (BseModule *module,      /* EngineThread */
                gpointer   data)
{
  GslWaveOscData *wosc = module->user_data;
  PcmPos *pos = data;

  pos->stamp = GSL_TICK_STAMP;
  pos->module_pcm_position = gsl_wave_osc_cur_pos (wosc);
  if (pos->perc >= 0 && wosc->wchunk)
    {
      GslWaveOscConfig config = wosc->config;
      config.start_offset = CLAMP (pos->perc, 0, 100) / 100.0 * wosc->wchunk->length;
      gsl_wave_osc_config (wosc, &config);
    }
}

static void
pcm_pos_access_free (gpointer data)     /* UserThread */
{
  PcmPos *pos = data;
  BseWaveOsc *self = pos->wosc;
  
  if (pos->perc < 0)
    g_signal_emit (self, signal_notify_pcm_position, 0, pos->stamp, pos->module_pcm_position);
  
  g_object_unref (self);
  g_free (pos);
}

void
bse_wave_osc_mass_seek (guint              n_woscs,
                        BseWaveOsc       **woscs,
                        gfloat             perc)
{
  guint i;
  g_return_if_fail (perc >= 0 && perc <= 100);
  BseTrans *trans = bse_trans_open();
  for (i = 0; i < n_woscs; i++)
    {
      BseWaveOsc *wosc = woscs[i];
      g_return_if_fail (BSE_IS_WAVE_OSC (wosc));
      if (BSE_SOURCE_PREPARED (wosc))
        {
          PcmPos *pos = g_new (PcmPos, 1);
          pos->perc = perc;
          pos->wosc = g_object_ref (wosc);
          bse_source_access_modules (BSE_SOURCE (pos->wosc),
                                     pcm_pos_access,
                                     pos,
                                     pcm_pos_access_free,
                                     NULL);
        }
    }
  bse_trans_commit (trans);
}

void
bse_wave_osc_request_pcm_position (BseWaveOsc *self)
{
  g_return_if_fail (BSE_IS_WAVE_OSC (self));
  
  if (BSE_SOURCE_PREPARED (self))
    {
      PcmPos *pos = g_new (PcmPos, 1);
      pos->perc = -1;
      pos->wosc = g_object_ref (self);
      bse_source_access_modules (BSE_SOURCE (self),
                                 pcm_pos_access,
                                 pos,
                                 pcm_pos_access_free,
                                 NULL);
    }
}

static void
bse_wave_osc_class_init (BseWaveOscClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseItemClass *item_class = BSE_ITEM_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ochannel, ichannel;
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = bse_wave_osc_set_property;
  gobject_class->get_property = bse_wave_osc_get_property;
  gobject_class->finalize = bse_wave_osc_finalize;
  gobject_class->dispose = bse_wave_osc_dispose;
  
  item_class->get_candidates = bse_wave_osc_get_candidates;
  
  source_class->context_create = bse_wave_osc_context_create;
  
  bse_object_class_add_param (object_class, _("Wave"),
                              PARAM_WAVE,
                              bse_param_spec_object ("wave", _("Wave"), _("Wave used as oscillator source"),
                                                     BSE_TYPE_WAVE, SFI_PARAM_STANDARD));
  bse_object_class_add_param (object_class, _("Wave"),
                              PARAM_CHANNEL,
                              sfi_pspec_int ("channel", _("Channel"), _("The audio channel to play, usually 1 is left, 2 is right"),
                                             1, 1, 256, 2, SFI_PARAM_STANDARD));
  bse_object_class_add_param (object_class, _("Modulation"),
                              PARAM_FM_PERC,
                              sfi_pspec_real ("fm_perc", _("Input Modulation [%]"),
                                              _("Modulation Strength for linear frequency modulation"),
                                              10.0, 0, 100.0,5.0,
                                              SFI_PARAM_STANDARD ":scale"));
  bse_object_class_add_param (object_class, _("Modulation"),
                              PARAM_FM_EXP,
                              sfi_pspec_bool ("exponential_fm", _("Exponential FM"),
                                              _("Perform exponential frequency modulation "
                                                "instead of linear"),
                                              FALSE, SFI_PARAM_STANDARD));
  bse_object_class_add_param (object_class, _("Modulation"),
                              PARAM_FM_OCTAVES,
                              sfi_pspec_real ("fm_n_octaves", _("Octaves"),
                                              _("Number of octaves to be affected by exponential frequency modulation"),
                                              1.0, 0, 3.0, 0.01,
                                              SFI_PARAM_STANDARD ":scale"));
  
  signal_notify_pcm_position = bse_object_class_add_signal (object_class, "notify_pcm_position",
                                                            G_TYPE_NONE, 2,
                                                            SFI_TYPE_NUM,
                                                            G_TYPE_INT);
  
  ichannel = bse_source_class_add_ichannel (source_class, "freq-in", _("Freq In"), _("Frequency Input"));
  g_assert (ichannel == BSE_WAVE_OSC_ICHANNEL_FREQ);
  ichannel = bse_source_class_add_ichannel (source_class, "sync-in", _("Sync In"), _("Syncronization Input"));
  g_assert (ichannel == BSE_WAVE_OSC_ICHANNEL_SYNC);
  ichannel = bse_source_class_add_ichannel (source_class, "mod-in", _("Mod In"), _("Modulation Input"));
  g_assert (ichannel == BSE_WAVE_OSC_ICHANNEL_MOD);
  ochannel = bse_source_class_add_ochannel (source_class, "audio-out", _("Audio Out"), _("Wave Output"));
  g_assert (ochannel == BSE_WAVE_OSC_OCHANNEL_WAVE);
  ochannel = bse_source_class_add_ochannel (source_class, "gate-out", _("Gate Out"), _("Gate Output"));
  g_assert (ochannel == BSE_WAVE_OSC_OCHANNEL_GATE);
  ochannel = bse_source_class_add_ochannel (source_class, "done-out", _("Done Out"), _("Done Output"));
  g_assert (ochannel == BSE_WAVE_OSC_OCHANNEL_DONE);
}
