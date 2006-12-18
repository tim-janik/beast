/* DavOrgan - DAV Additive Organ Synthesizer
 * Copyright (c) 1999, 2000, 2002 David A. Bartold and Tim Janik
 * Copyright (c) 2006 Stefan Westerfeld
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
#include "davorgan.h"

#include <bse/bseengine.h>
#include <bse/bsemathsignal.h>
#include <bse/bsemain.h>

#include <stdio.h>
#include <stdlib.h>

/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_BASE_FREQ,
  PARAM_BASE_NOTE,
  PARAM_TRANSPOSE,
  PARAM_FINE_TUNE,
  PARAM_HARM0,
  PARAM_HARM1,
  PARAM_HARM2,
  PARAM_HARM3,
  PARAM_HARM4,
  PARAM_HARM5,
  PARAM_BRASS,
  PARAM_FLUTE,
  PARAM_REED
};


/* --- prototypes --- */
static void        dav_organ_init             (DavOrgan       *self);
static void        dav_organ_class_init       (DavOrganClass  *class);
static void        dav_organ_set_property     (GObject	      *object,
					       guint           param_id,
					       const GValue   *value,
					       GParamSpec     *pspec);
static void        dav_organ_get_property     (GObject	      *object,
					       guint           param_id,
					       GValue         *value,
					       GParamSpec     *pspec);
static void        dav_organ_prepare          (BseSource      *source);
static void	   dav_organ_context_create   (BseSource      *source,
					       guint	       context_handle,
					       BseTrans	      *trans);
static void        dav_organ_reset            (BseSource      *source);
static void	   dav_organ_update_modules   (DavOrgan       *self);


/* --- Export to DAV --- */
#include "./icons/organ.c"
BSE_REGISTER_OBJECT (DavOrgan, BseSource, "/Modules/Audio Sources/Organ", "",
                     "DavOrgan is a modifiable additive organ synthesizer",
                     organ_icon,
                     dav_organ_class_init, NULL, dav_organ_init);
BSE_DEFINE_EXPORTS ();


/* --- variables --- */
static gpointer          parent_class = NULL;


/* --- functions --- */
static inline GParamSpec*
harm_param (gchar *name,
	    gchar *nick,
	    gchar *blurb,
	    gint   dft)
{
  return sfi_pspec_real (name, nick, blurb, dft * 100., 0, 100.0, 1.0, SFI_PARAM_STANDARD ":scale");
}

static void
dav_organ_class_init (DavOrganClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint channel_id;
  
  parent_class = g_type_class_peek (BSE_TYPE_SOURCE);
  
  gobject_class->set_property = dav_organ_set_property;
  gobject_class->get_property = dav_organ_get_property;
  
  source_class->prepare = dav_organ_prepare;
  source_class->context_create = dav_organ_context_create;
  source_class->reset = dav_organ_reset;
  
  bse_object_class_add_param (object_class, _("Base Frequency"), PARAM_BASE_FREQ,
			      bse_param_spec_freq ("base_freq", _("Frequency"),
                                                   _("Organ frequency in Hertz, i.e. the pitch of the base tone"),
						   BSE_KAMMER_FREQUENCY, BSE_MIN_OSC_FREQUENCY, BSE_MAX_OSC_FREQUENCY,
						   SFI_PARAM_STANDARD ":dial"));
  bse_object_class_add_param (object_class, _("Base Frequency"),
                              PARAM_BASE_NOTE,
                              bse_pspec_note_simple ("base_note", _("Note"),
                                                     _("Organ frequency as note, converted to Hertz according to the current musical tuning"),
                                                     SFI_PARAM_GUI));
  bse_object_class_add_param (object_class, _("Base Frequency"),
			      PARAM_TRANSPOSE,
			      sfi_pspec_int ("transpose", _("Transpose"), _("Transposition of the frequency in semitones"),
					     0, BSE_MIN_TRANSPOSE, BSE_MAX_TRANSPOSE, 12,
					     SFI_PARAM_STANDARD ":f:dial:skip-default"));
  bse_object_class_add_param (object_class, _("Base Frequency"),
			      PARAM_FINE_TUNE,
			      sfi_pspec_int ("fine_tune", _("Fine Tune"), _("Amount of detuning in cent (hundredth part of a semitone)"),
					     0, BSE_MIN_FINE_TUNE, BSE_MAX_FINE_TUNE, 10,
					     SFI_PARAM_STANDARD ":f:dial:skip-default"));
  bse_object_class_add_param (object_class, "Instrument flavour", PARAM_BRASS,
			      sfi_pspec_bool ("brass", "Brass Sounds", "Changes the organ to sound more brassy",
                                              FALSE, SFI_PARAM_STANDARD));
  bse_object_class_add_param (object_class, "Instrument flavour", PARAM_REED,
			      sfi_pspec_bool ("reed", "Reed Sounds", "Adds reeds sound",
                                              FALSE, SFI_PARAM_STANDARD));
  bse_object_class_add_param (object_class, "Instrument flavour", PARAM_FLUTE,
			      sfi_pspec_bool ("flute", "Flute Sounds", "Adds flute sounds",
                                              FALSE, SFI_PARAM_STANDARD));
  bse_object_class_add_param (object_class, "Harmonics", PARAM_HARM0,
			      harm_param ("harm0", "16th", "16th Harmonic", 1.0));
  bse_object_class_add_param (object_class, "Harmonics", PARAM_HARM1,
			      harm_param ("harm1", "8th", "8th Harmonic", 36. / 127.));
  bse_object_class_add_param (object_class, "Harmonics", PARAM_HARM2,
			      harm_param ("harm2", "5 1/3rd", "5 1/3rd Harmonic", 100. / 127.));
  bse_object_class_add_param (object_class, "Harmonics", PARAM_HARM3,
			      harm_param ("harm3", "4th", "4th Harmonic", 32. / 127.));
  bse_object_class_add_param (object_class, "Harmonics", PARAM_HARM4,
			      harm_param ("harm4", "2 2/3rd", "2 2/3rd Harmonic", 91. / 127.));
  bse_object_class_add_param (object_class, "Harmonics", PARAM_HARM5,
			      harm_param ("harm5", "2nd", "2nd Harmonic", 55. / 127.));
  
  channel_id = bse_source_class_add_ichannel (source_class, "freq-in", _("Freq In"), _("Frequency Input"));
  channel_id = bse_source_class_add_ochannel (source_class, "audio-out", _("Audio Out"), _("Organ Output"));
}

static void
dav_organ_init (DavOrgan *self)
{
  self->config.brass = FALSE;
  self->config.flute = FALSE;
  self->config.reed = FALSE;
  
  self->config.freq = BSE_KAMMER_FREQUENCY;
  self->config.transpose_factor = 0; // updated when prepared
  self->config.harm0 = 1.0;
  self->config.harm1 = 36. / 127.;
  self->config.harm2 = 100. / 127.;
  self->config.harm3 = 32. / 127.;
  self->config.harm4 = 91. / 127.;
  self->config.harm5 = 55. / 127.;
}

static void
dav_organ_set_property (GObject      *object,
			guint         param_id,
			const GValue *value,
			GParamSpec   *pspec)
{
  DavOrgan *self = DAV_ORGAN (object);
  
  switch (param_id)
    {
    case PARAM_BASE_FREQ:
      self->config.freq = sfi_value_get_real (value);
      g_object_notify (G_OBJECT (self), "base_note");
      dav_organ_update_modules (self);
      break;
    case PARAM_BASE_NOTE:
      self->config.freq = bse_note_to_freq (bse_item_current_musical_tuning (BSE_ITEM (self)), sfi_value_get_note (value));
      g_object_notify (G_OBJECT (self), "base_freq");
      dav_organ_update_modules (self);
      break;
    case PARAM_TRANSPOSE:
      self->transpose = sfi_value_get_int (value);
      dav_organ_update_modules (self);
      break;
    case PARAM_FINE_TUNE:
      self->config.fine_tune = sfi_value_get_int (value);
      dav_organ_update_modules (self);
      break;
    case PARAM_BRASS:
      self->config.brass = sfi_value_get_bool (value);
      dav_organ_update_modules (self);
      break;
    case PARAM_FLUTE:
      self->config.flute = sfi_value_get_bool (value);
      dav_organ_update_modules (self);
      break;
    case PARAM_REED:
      self->config.reed = sfi_value_get_bool (value);
      dav_organ_update_modules (self);
      break;
    case PARAM_HARM0:
      self->config.harm0 = sfi_value_get_real (value) / 100.0;
      dav_organ_update_modules (self);
      break;
    case PARAM_HARM1:
      self->config.harm1 = sfi_value_get_real (value) / 100.0;
      dav_organ_update_modules (self);
      break;
    case PARAM_HARM2:
      self->config.harm2 = sfi_value_get_real (value) / 100.0;
      dav_organ_update_modules (self);
      break;
    case PARAM_HARM3:
      self->config.harm3 = sfi_value_get_real (value) / 100.0;
      dav_organ_update_modules (self);
      break;
    case PARAM_HARM4:
      self->config.harm4 = sfi_value_get_real (value) / 100.0;
      dav_organ_update_modules (self);
      break;
    case PARAM_HARM5:
      self->config.harm5 = sfi_value_get_real (value) / 100.0;
      dav_organ_update_modules (self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
dav_organ_get_property (GObject    *object,
			guint       param_id,
			GValue     *value,
			GParamSpec *pspec)
{
  DavOrgan *self = DAV_ORGAN (object);
  
  switch (param_id)
    {
    case PARAM_BASE_FREQ:
      sfi_value_set_real (value, self->config.freq);
      break;
    case PARAM_BASE_NOTE:
      sfi_value_set_note (value, bse_note_from_freq (bse_item_current_musical_tuning (BSE_ITEM (self)), self->config.freq));
      break;
    case PARAM_TRANSPOSE:
      sfi_value_set_int (value, self->transpose);
      break;
    case PARAM_FINE_TUNE:
      sfi_value_set_int (value, self->config.fine_tune);
      break;
    case PARAM_BRASS:
      sfi_value_set_bool (value, self->config.brass);
      break;
    case PARAM_FLUTE:
      sfi_value_set_bool (value, self->config.flute);
      break;
    case PARAM_REED:
      sfi_value_set_bool (value, self->config.reed);
      break;
    case PARAM_HARM0:
      sfi_value_set_real (value, self->config.harm0 * 100.0);
      break;
    case PARAM_HARM1:
      sfi_value_set_real (value, self->config.harm1 * 100.0);
      break;
    case PARAM_HARM2:
      sfi_value_set_real (value, self->config.harm2 * 100.0);
      break;
    case PARAM_HARM3:
      sfi_value_set_real (value, self->config.harm3 * 100.0);
      break;
    case PARAM_HARM4:
      sfi_value_set_real (value, self->config.harm4 * 100.0);
      break;
    case PARAM_HARM5:
      sfi_value_set_real (value, self->config.harm5 * 100.0);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
dav_organ_class_ref_tables (DavOrganClass *class)
{
  gfloat rate = bse_engine_sample_freq();
  gfloat half = rate / 2;
  gfloat slope = rate / 10;
  gint i;
  
  class->ref_count++;
  if (class->ref_count > 1)
    return;
  
  /* Initialize sine table. */
  class->sine_table = g_new (gfloat, rate);
  for (i = 0; i < rate; i++)
    class->sine_table[i] = sin ((i / rate) * 2.0 * PI) / 6.0;
  
  /* Initialize triangle table. */
  class->triangle_table = g_new (gfloat, rate);
  for (i = 0; i < rate / 2; i++)
    class->triangle_table[i] = (4 / rate * i - 1.0) / 6.0;
  for (; i < rate; i++)
    class->triangle_table[i] = (4 / rate * (rate - i) - 1.0) / 6.0;
  
  /* Initialize pulse table. */
  class->pulse_table = g_new (gfloat, rate);
  for (i = 0; i < slope; i++)
    class->pulse_table[i] = (-i / slope) / 6.0;
  for (; i < half - slope; i++)
    class->pulse_table[i] = -1.0 / 6.0;
  for (; i < half + slope; i++)
    class->pulse_table[i] = ((i - half) / slope) / 6.0;
  for (; i < rate - slope; i++)
    class->pulse_table[i] = 1.0 / 6.0;
  for (; i < rate; i++)
    class->pulse_table[i] = ((rate - i) * 1.0 / slope) / 6.0;
}

static void
dav_organ_class_unref_tables (DavOrganClass *class)
{
  g_return_if_fail (class->ref_count > 0);
  
  class->ref_count -= 1;
  
  if (!class->ref_count)
    {
      g_free (class->sine_table);
      class->sine_table = NULL;
      g_free (class->triangle_table);
      class->triangle_table = NULL;
      g_free (class->pulse_table);
      class->pulse_table = NULL;
    }
}

static void
dav_organ_prepare (BseSource *source)
{
  DavOrgan *self = DAV_ORGAN (source);
  DavOrganClass *class = DAV_ORGAN_GET_CLASS (self);
  
  dav_organ_class_ref_tables (class);
  self->config.transpose_factor = bse_transpose_factor (bse_source_prepared_musical_tuning (source), self->transpose);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

typedef struct {
  DavOrganClass *class;
  DavOrganConfig config;
  
  /* position accumulators */
  guint32     harm0_paccu;
  guint32     harm1_paccu;
  guint32     harm2_paccu;
  guint32     harm3_paccu;
  guint32     harm4_paccu;
  guint32     harm5_paccu;
} Organ;

static void
dav_organ_update_modules (DavOrgan *self)
{
  /* update organ data during runtime */
  if (BSE_SOURCE_PREPARED (self))
    {
      self->config.transpose_factor = bse_transpose_factor (bse_source_prepared_musical_tuning (BSE_SOURCE (self)), self->transpose);
      bse_source_update_modules (BSE_SOURCE (self),
                                 G_STRUCT_OFFSET (Organ, config),
                                 &self->config,
                                 sizeof (self->config),
                                 NULL);
    }
}

static inline gfloat
table_pos (const gfloat *table,
	   guint         freq_256,
	   guint         mix_freq_256,
	   guint32      *paccu)
{
  *paccu += freq_256;
  while (*paccu >= mix_freq_256)
    *paccu -= mix_freq_256;
  
  return table[*paccu >> 8];
}

static void
dav_organ_reset_module (BseModule *module)
{
  Organ *organ = module->user_data;

  guint32 rfactor = bse_main_args->allow_randomization ? 1 : 0;
  guint32 mix_freq_256 = bse_engine_sample_freq() * 256;

  /* to make all notes sound a bit different, randomize the initial phase of
   * each harmonic (except if the user requested deterministic behaviour)
   */
  organ->harm0_paccu = rfactor * g_random_int_range (0, mix_freq_256);
  organ->harm1_paccu = rfactor * g_random_int_range (0, mix_freq_256);
  organ->harm2_paccu = rfactor * g_random_int_range (0, mix_freq_256);
  organ->harm3_paccu = rfactor * g_random_int_range (0, mix_freq_256);
  organ->harm4_paccu = rfactor * g_random_int_range (0, mix_freq_256);
  organ->harm5_paccu = rfactor * g_random_int_range (0, mix_freq_256);
}

static void
dav_organ_process (BseModule *module,
		   guint      n_values)
{
  Organ *organ = module->user_data;
  DavOrganClass *class = organ->class;
  const gfloat *sine_table = class->sine_table;
  const gfloat *flute_table = organ->config.flute ? class->triangle_table : sine_table;
  const gfloat *reed_table = organ->config.reed ? class->pulse_table : sine_table;
  const gfloat *ifreq = BSE_MODULE_IBUFFER (module, DAV_ORGAN_ICHANNEL_FREQ);
  gfloat *ovalues = BSE_MODULE_OBUFFER (module, DAV_ORGAN_OCHANNEL_MONO);
  const gdouble transpose = organ->config.transpose_factor;
  const gdouble fine_tune = bse_cent_factor (organ->config.fine_tune);
  guint freq_256, mix_freq_256;
  guint freq_256_harm0, freq_256_harm1;
  guint i;
  
  if (BSE_MODULE_ISTREAM (module, DAV_ORGAN_ICHANNEL_FREQ).connected)
    freq_256 = BSE_FREQ_FROM_VALUE (ifreq[0]) * transpose * fine_tune * 256 + 0.5;
  else
    freq_256 = organ->config.freq * transpose * fine_tune * 256 + 0.5;
  mix_freq_256 = bse_engine_sample_freq() * 256;
  freq_256_harm0 = freq_256 / 2;
  freq_256_harm1 = freq_256;
  
  if (organ->config.brass)
    {
      guint freq_256_harm2 = freq_256 * 2;
      guint freq_256_harm3 = freq_256_harm2 * 2;
      guint freq_256_harm4 = freq_256_harm3 * 2;
      guint freq_256_harm5 = freq_256_harm4 * 2;
      
      for (i = 0; i < n_values; i++)
	{
	  gfloat vaccu;
	  
	  vaccu = table_pos (sine_table, freq_256_harm0, mix_freq_256, &organ->harm0_paccu) * organ->config.harm0;
	  vaccu += table_pos (sine_table, freq_256_harm1, mix_freq_256, &organ->harm1_paccu) * organ->config.harm1;
	  vaccu += table_pos (reed_table, freq_256_harm2, mix_freq_256, &organ->harm2_paccu) * organ->config.harm2;
	  vaccu += table_pos (sine_table, freq_256_harm3, mix_freq_256, &organ->harm3_paccu) * organ->config.harm3;
	  vaccu += table_pos (flute_table, freq_256_harm4, mix_freq_256, &organ->harm4_paccu) * organ->config.harm4;
	  vaccu += table_pos (flute_table, freq_256_harm5, mix_freq_256, &organ->harm5_paccu) * organ->config.harm5;
	  ovalues[i] = vaccu;
	}
    }
  else
    {
      guint freq_256_harm2 = freq_256 * 3 / 2;
      guint freq_256_harm3 = freq_256 * 2;
      guint freq_256_harm4 = freq_256 * 3;
      guint freq_256_harm5 = freq_256_harm3 * 2;
      
      for (i = 0; i < n_values; i++)
	{
	  gfloat vaccu;
	  
	  vaccu = table_pos (sine_table, freq_256_harm0, mix_freq_256, &organ->harm0_paccu) * organ->config.harm0;
	  vaccu += table_pos (sine_table, freq_256_harm1, mix_freq_256, &organ->harm1_paccu) * organ->config.harm1;
	  vaccu += table_pos (sine_table, freq_256_harm2, mix_freq_256, &organ->harm2_paccu) * organ->config.harm2;
	  vaccu += table_pos (reed_table, freq_256_harm3, mix_freq_256, &organ->harm3_paccu) * organ->config.harm3;
	  vaccu += table_pos (sine_table, freq_256_harm4, mix_freq_256, &organ->harm4_paccu) * organ->config.harm4;
	  vaccu += table_pos (flute_table, freq_256_harm5, mix_freq_256, &organ->harm5_paccu) * organ->config.harm5;
	  ovalues[i] = vaccu;
	}
    }
}

static void
dav_organ_context_create (BseSource *source,
			  guint      context_handle,
			  BseTrans  *trans)
{
  static const BseModuleClass organ_class = {
    DAV_ORGAN_N_ICHANNELS,	/* n_istreams */
    0,				/* n_jstreams */
    DAV_ORGAN_N_OCHANNELS,	/* n_ostreams */
    dav_organ_process,		/* process */
    NULL,                       /* process_defer */
    dav_organ_reset_module,     /* reset */
    (BseModuleFreeFunc) g_free,	/* free */
    BSE_COST_NORMAL,		/* cost */
  };
  DavOrgan *self = DAV_ORGAN (source);
  Organ *organ = g_new0 (Organ, 1);
  BseModule *module;
  
  /* initialize organ data */
  organ->class = DAV_ORGAN_GET_CLASS (self);
  organ->config = self->config;
  module = bse_module_new (&organ_class, organ);
  
  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);
  
  /* commit module to engine */
  bse_trans_add (trans, bse_job_integrate (module));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}

static void
dav_organ_reset (BseSource *source)
{
  DavOrganClass *class = DAV_ORGAN_GET_CLASS (source);
  
  dav_organ_class_unref_tables (class);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}
