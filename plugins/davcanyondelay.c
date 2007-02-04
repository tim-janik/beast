/* DavCanyonDelay - DAV Canyon Delay
 * Copyright (c) 1999, 2000 David A. Bartold
 * Copyright (c) 2003, 2006 Tim Janik
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
#include "davcanyondelay.h"

#include <bse/bseengine.h>
#include <bse/bsemathsignal.h>
#include <string.h>


/* --- parameters --- */
enum
{
  PROP_0,
  PROP_LEFT_TO_RIGHT_TIME,
  PROP_LEFT_TO_RIGHT_FEEDBACK,
  PROP_RIGHT_TO_LEFT_TIME,
  PROP_RIGHT_TO_LEFT_FEEDBACK,
  PROP_FILTER_FREQ,
  PROP_FILTER_NOTE
};


/* --- prototypes --- */
static void dav_canyon_delay_init           (DavCanyonDelay      *self);
static void dav_canyon_delay_class_init     (DavCanyonDelayClass *class);
static void dav_canyon_delay_set_property   (GObject             *object,
                                             guint                param_id,
                                             const GValue        *value,
                                             GParamSpec          *pspec);
static void dav_canyon_delay_get_property   (GObject             *object,
                                             guint                param_id,
                                             GValue              *value,
                                             GParamSpec          *pspec);
static void dav_canyon_delay_prepare        (BseSource           *source);
static void dav_canyon_delay_context_create (BseSource           *source,
                                             guint                context_handle,
                                             BseTrans            *trans);
static void dav_canyon_delay_update_modules (DavCanyonDelay      *self);


/* --- Export to DAV --- */
#include "./icons/canyon.c"
BSE_REGISTER_OBJECT (DavCanyonDelay, BseSource, "/Modules/Enhance/CanyonDelay", "",
                     "DavCanyonDelay adds deep and long canyon-alike echos to stereo signals.",
                     canyon_icon,
                     dav_canyon_delay_class_init, NULL, dav_canyon_delay_init);
BSE_DEFINE_EXPORTS ();


/* --- variables --- */
static gpointer          parent_class = NULL;


/* --- functions --- */
static void
dav_canyon_delay_class_init (DavCanyonDelayClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint channel;

  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = dav_canyon_delay_set_property;
  gobject_class->get_property = dav_canyon_delay_get_property;
  
  source_class->prepare = dav_canyon_delay_prepare;
  source_class->context_create = dav_canyon_delay_context_create;
  
  bse_object_class_add_param (object_class, _("Left to Right"), PROP_LEFT_TO_RIGHT_TIME,
                              sfi_pspec_real ("left_to_right_time", _("Delay (seconds)"),
                                              _("The time for the left to right delay"),
                                              0.09, 0.01, 0.99, 0.01,
                                              SFI_PARAM_STANDARD ":scale"));
  bse_object_class_add_param (object_class, _("Left to Right"), PROP_LEFT_TO_RIGHT_FEEDBACK,
                              sfi_pspec_real ("left_to_right_feedback", _("Feedback [%]"),
                                              _("The feedback amount; a negative feedback inverts the signal"),
                                              60.0, -100.0, 100.0, 0.01,
                                              SFI_PARAM_STANDARD ":scale"));
  bse_object_class_add_param (object_class, _("Right to Left"), PROP_RIGHT_TO_LEFT_TIME,
                              sfi_pspec_real ("right_to_left_time", _("Delay (seconds)"),
                                              _("The time for the right to left delay"),
                                              0.26, 0.01, 0.99, 0.01,
                                              SFI_PARAM_STANDARD ":scale"));
  bse_object_class_add_param (object_class, _("Right to Left"), PROP_RIGHT_TO_LEFT_FEEDBACK,
                              sfi_pspec_real ("right_to_left_feedback", _("Feedback [%]"),
                                              _("Set the feedback amount; a negative feedback inverts the signal"),
                                              -70.0, -100.0, 100.0, 0.01,
                                              SFI_PARAM_STANDARD ":scale"));

  bse_object_class_add_param (object_class, _("IIR Low-Pass Filter"), PROP_FILTER_FREQ,
                              bse_param_spec_freq ("filter_freq", _("Frequency"),
                                                   _("Reflection cutoff frequency"),
                                                   bse_note_to_freq (BSE_MUSICAL_TUNING_12_TET, SFI_NOTE_C (+3)),
                                                   BSE_MIN_OSC_FREQUENCY, BSE_MAX_OSC_FREQUENCY,
                                                   SFI_PARAM_STANDARD ":dial"));
  bse_object_class_add_param (object_class, _("IIR Low-Pass Filter"), PROP_FILTER_NOTE,
                              bse_pspec_note_simple ("filter_note", _("Note"),
                                                     _("Filter cutoff frequency as note, converted to Hertz according to the current musical tuning"),
                                                     SFI_PARAM_GUI));

  channel = bse_source_class_add_ichannel (source_class, "left-in", _("Left In"), _("Left Audio Input"));
  g_assert (channel == DAV_CANYON_DELAY_ICHANNEL_LEFT);
  channel = bse_source_class_add_ichannel (source_class, "right-in", _("Right In"), _("Right Audio Input"));
  g_assert (channel == DAV_CANYON_DELAY_ICHANNEL_RIGHT);
  channel = bse_source_class_add_ochannel (source_class, "left-out", _("Left Out"), _("Left Audio Output"));
  g_assert (channel == DAV_CANYON_DELAY_OCHANNEL_LEFT);
  channel = bse_source_class_add_ochannel (source_class, "right-out", _("Right Out"), _("Right Audio Output"));
  g_assert (channel == DAV_CANYON_DELAY_OCHANNEL_RIGHT);
}

static void
dav_canyon_delay_init (DavCanyonDelay *self)
{
  self->l_to_r_seconds = 0.09;
  self->l_to_r_feedback = 60.0;
  self->r_to_l_seconds = 0.26;
  self->r_to_l_feedback = -70.0;
  self->filter_freq = bse_note_to_freq (bse_item_current_musical_tuning (BSE_ITEM (self)), SFI_NOTE_C (+3));
}

static void
dav_canyon_delay_set_property (GObject             *object,
                               guint                param_id,
                               const GValue        *value,
                               GParamSpec          *pspec)
{
  DavCanyonDelay *self = DAV_CANYON_DELAY (object);
  switch (param_id)
    {
    case PROP_LEFT_TO_RIGHT_TIME:
      self->l_to_r_seconds = sfi_value_get_real (value);
      break;
    case PROP_LEFT_TO_RIGHT_FEEDBACK:
      self->l_to_r_feedback = sfi_value_get_real (value);
      break;
    case PROP_RIGHT_TO_LEFT_TIME:
      self->r_to_l_seconds = sfi_value_get_real (value);
      break;
    case PROP_RIGHT_TO_LEFT_FEEDBACK:
      self->r_to_l_feedback = sfi_value_get_real (value);
      break;
    case PROP_FILTER_FREQ:
      self->filter_freq = sfi_value_get_real (value);
      g_object_notify (self, "filter-note");
      break;
    case PROP_FILTER_NOTE:
      self->filter_freq = bse_note_to_freq (bse_item_current_musical_tuning (BSE_ITEM (self)), sfi_value_get_note (value));
      g_object_notify (self, "filter-freq");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
  dav_canyon_delay_update_modules (self);
}

static void
dav_canyon_delay_get_property (GObject             *object,
                               guint                param_id,
                               GValue              *value,
                               GParamSpec          *pspec)
{
  DavCanyonDelay *self = DAV_CANYON_DELAY (object);
  switch (param_id)
    {
    case PROP_LEFT_TO_RIGHT_TIME:
      sfi_value_set_real (value, self->l_to_r_seconds);
      break;
    case PROP_LEFT_TO_RIGHT_FEEDBACK:
      sfi_value_set_real (value, self->l_to_r_feedback);
      break;
    case PROP_RIGHT_TO_LEFT_TIME:
      sfi_value_set_real (value, self->r_to_l_seconds);
      break;
    case PROP_RIGHT_TO_LEFT_FEEDBACK:
      sfi_value_set_real (value, self->r_to_l_feedback);
      break;
    case PROP_FILTER_FREQ:
      sfi_value_set_real (value, self->filter_freq);
      break;
    case PROP_FILTER_NOTE:
      sfi_value_set_note (value, bse_note_from_freq (bse_item_current_musical_tuning (BSE_ITEM (self)), self->filter_freq));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
dav_canyon_delay_prepare (BseSource *source)
{
  DavCanyonDelay *self = DAV_CANYON_DELAY (source);

  /* initialize calculated params (mix-freq dependant) */
  dav_canyon_delay_update_modules (self);

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

static void
canyon_delay_process (BseModule *module,
                      guint      n_values)
{
  DavCanyonDelayModule *cmod = module->user_data;
  const gfloat *left_in = BSE_MODULE_IBUFFER (module, DAV_CANYON_DELAY_ICHANNEL_LEFT);
  const gfloat *right_in = BSE_MODULE_IBUFFER (module, DAV_CANYON_DELAY_ICHANNEL_RIGHT);
  gfloat *left_out = BSE_MODULE_OBUFFER (module, DAV_CANYON_DELAY_OCHANNEL_LEFT);
  gfloat *right_out = BSE_MODULE_OBUFFER (module, DAV_CANYON_DELAY_OCHANNEL_RIGHT);
  guint i;

  for (i = 0; i < n_values; i++)
    {
      gint32 pos1, pos2;
      gdouble accum_l = left_in[i];
      gdouble accum_r = right_in[i];

      pos1 = cmod->pos - cmod->params.r_to_l_pos + cmod->datasize;
      while (pos1 >= cmod->datasize)
        pos1 -= cmod->datasize;

      pos2 = cmod->pos - cmod->params.l_to_r_pos + cmod->datasize;
      while (pos2 >= cmod->datasize)
        pos2 -= cmod->datasize;
      
      /* Mix channels with past samples. */
      accum_l = accum_l * cmod->params.r_to_l_invmag + cmod->data_r[pos1] * cmod->params.r_to_l_mag;
      accum_r = accum_r * cmod->params.l_to_r_invmag + cmod->data_l[pos2] * cmod->params.l_to_r_mag;

      /* Low-pass filter output. */
      accum_l = cmod->accum_l * cmod->params.filter_invmag + accum_l * cmod->params.filter_mag;
      accum_l = BSE_SIGNAL_CLIP (accum_l);
      accum_r = cmod->accum_r * cmod->params.filter_invmag + accum_r * cmod->params.filter_mag;
      accum_r = BSE_SIGNAL_CLIP (accum_r);
      
      /* Store IIR samples. */
      cmod->accum_l = accum_l;
      cmod->accum_r = accum_r;
      
      /* Store samples in arrays. */
      cmod->data_l[cmod->pos] = accum_l;
      cmod->data_r[cmod->pos] = accum_r;
      
      /* Write output. */
      left_out[i] = accum_l;
      right_out[i] = accum_r;

      cmod->pos++;
      if (cmod->pos >= cmod->datasize)
        cmod->pos -= cmod->datasize;
    }
}

static void
canyon_delay_reset (BseModule *module)
{
  DavCanyonDelayModule *cmod = module->user_data;
  cmod->pos = 0;
  cmod->accum_l = 0;
  cmod->accum_r = 0;
  memset (cmod->data_l, 0, sizeof (cmod->data_l[0]) * cmod->datasize);
  memset (cmod->data_r, 0, sizeof (cmod->data_r[0]) * cmod->datasize);
}

static void
canyon_delay_free (gpointer        data,
                   const BseModuleClass *klass)
{
  DavCanyonDelayModule *cmod = data;
  /* Free tables */
  g_free (cmod->data_l);
  g_free (cmod->data_r);
}

static void
dav_canyon_delay_context_create (BseSource *source,
                                 guint      context_handle,
                                 BseTrans  *trans)
{
  static const BseModuleClass cmod_class = {
    DAV_CANYON_DELAY_N_ICHANNELS,       /* n_istreams */
    0,                                  /* n_jstreams */
    DAV_CANYON_DELAY_N_OCHANNELS,       /* n_ostreams */
    canyon_delay_process,               /* process */
    NULL,                               /* process_defer */
    canyon_delay_reset,                 /* reset */
    canyon_delay_free,                  /* free */
    BSE_COST_NORMAL,                    /* cost */
  };
  DavCanyonDelay *self = DAV_CANYON_DELAY (source);
  DavCanyonDelayModule *cmod = g_new0 (DavCanyonDelayModule, 1);
  BseModule *module;

  module = bse_module_new (&cmod_class, cmod);
  cmod->datasize = bse_engine_sample_freq();
  cmod->data_l = g_new0 (gdouble, cmod->datasize);
  cmod->data_r = g_new0 (gdouble, cmod->datasize);
  cmod->params = self->params;
  canyon_delay_reset (module);

  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);

  /* commit module to engine */
  bse_trans_add (trans, bse_job_integrate (module));

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}

/* update module configuration from new parameter set */
static void
canyon_delay_access (BseModule *module,
                     gpointer   data)
{
  DavCanyonDelayModule *cmod = module->user_data;
  DavCanyonDelayParams *params = data;

  cmod->params = *params;
}

static void
dav_canyon_delay_update_modules (DavCanyonDelay *self)
{
  if (BSE_SOURCE_PREPARED (self))
    {
      self->params.l_to_r_mag = self->l_to_r_feedback / 100.0;
      self->params.l_to_r_invmag = 1.0 - ABS (self->params.l_to_r_mag);
      self->params.r_to_l_mag = self->r_to_l_feedback / 100.0;
      self->params.r_to_l_invmag = 1.0 - ABS (self->params.r_to_l_mag);
      self->params.l_to_r_pos = self->l_to_r_seconds * bse_engine_sample_freq();
      self->params.r_to_l_pos = self->r_to_l_seconds * bse_engine_sample_freq();
      
      /* The following stuff (except the multiplicative inverse)
       * is a guesstimate. The calculations seem to be right, tho.
       * Compare to the FIR filter for a reference.
       */
      gdouble half = 1.0 / (4.0 * PI * self->filter_freq);
      
      /* Calculate the half life rate given:
       *   half        - the length of the half life
       *   sample_freq - time divisor (usually the # calcs per second)
       * Basically, find r given 1/2 = e^(-r*(half/rate))
       * ln(1/2) = -ln(2) = -BSE_LN2 = -0.693147...
       */
      self->params.filter_invmag = exp (-BSE_LN2 / (half * bse_engine_sample_freq()));
      self->params.filter_mag = 1.0 - self->params.filter_invmag;
      
      /* update all DavCanyonDelayModules. take a look at davxtalstrings.c
       * to understand what this code does.
       */
      bse_source_access_modules (BSE_SOURCE (self),
                                 canyon_delay_access,
                                 g_memdup (&self->params, sizeof (self->params)),
                                 g_free,
                                 NULL);
    }
}
