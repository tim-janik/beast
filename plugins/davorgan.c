/* DavOrgan - DAV Additive Organ Synthesizer
 * Copyright (c) 1999, 2000, 2002 David A. Bartold and Tim Janik
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

#include <gsl/gslengine.h>

#include <stdio.h>
#include <stdlib.h>

/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_BASE_FREQ,
  PARAM_BASE_NOTE,
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
static void        dav_organ_class_finalize   (DavOrganClass  *class);
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
					       GslTrans	      *trans);
static void        dav_organ_reset            (BseSource      *source);
static void	   dav_organ_update_modules   (DavOrgan       *self);


/* --- variables --- */
static GType             type_id_organ = 0;
static gpointer          parent_class = NULL;
static const GTypeInfo type_info_organ = {
  sizeof (DavOrganClass),
  
  (GBaseInitFunc) NULL,
  (GBaseFinalizeFunc) NULL,
  (GClassInitFunc) dav_organ_class_init,
  (GClassFinalizeFunc) dav_organ_class_finalize,
  NULL /* class_data */,
  
  sizeof (DavOrgan),
  0 /* n_preallocs */,
  (GInstanceInitFunc) dav_organ_init,
};


/* --- functions --- */
static inline GParamSpec*
harm_param (gchar *name,
	    gchar *nick,
	    gchar *blurb,
	    gint   dft)
{
  return bse_param_spec_float (name, nick, blurb, 0, 100.0, dft * 100., 1.0, BSE_PARAM_DEFAULT | BSE_PARAM_HINT_SCALE);
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
  
  bse_object_class_add_param (object_class, "Base Frequency", PARAM_BASE_FREQ,
			      bse_param_spec_float ("base_freq", "Frequency", NULL,
						    BSE_MIN_OSC_FREQ_d,
						    BSE_MAX_OSC_FREQ_d,
						    BSE_KAMMER_FREQ,
						    10.0,
						    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_DIAL));
  bse_object_class_set_param_log_scale (object_class, "base_freq", 880.0, 2, 4);
  bse_object_class_add_param (object_class, "Base Frequency",
                              PARAM_BASE_NOTE,
                              bse_param_spec_note ("base_note", "Note", NULL,
						   BSE_MIN_NOTE, BSE_MAX_NOTE,
						   BSE_KAMMER_NOTE, 1, TRUE,
						   BSE_PARAM_GUI));
  bse_object_class_add_param (object_class, "Instrument flavour", PARAM_BRASS,
			      bse_param_spec_bool ("brass", "Brass Sounds", "Changes the organ to sound more brassy",
						   FALSE, BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Instrument flavour", PARAM_REED,
			      bse_param_spec_bool ("reed", "Reed Sounds", "Adds reeds sound",
						   FALSE, BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Instrument flavour", PARAM_FLUTE,
			      bse_param_spec_bool ("flute", "Flute Sounds", "Adds flute sounds",
						   FALSE, BSE_PARAM_DEFAULT));
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
  
  channel_id = bse_source_class_add_ichannel (source_class, "freq_in", "Frequency Input");
  channel_id = bse_source_class_add_ochannel (source_class, "organ_out", "Organ Output");
}

static void
dav_organ_class_finalize (DavOrganClass *class)
{
}

static void
dav_organ_init (DavOrgan *self)
{
  self->config.brass = FALSE;
  self->config.flute = FALSE;
  self->config.reed = FALSE;
  
  self->config.freq = BSE_KAMMER_FREQ;
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
      self->config.freq = g_value_get_float (value);
      g_object_notify (G_OBJECT (self), "base_note");
      dav_organ_update_modules (self);
      break;
    case PARAM_BASE_NOTE:
      self->config.freq = bse_note_to_freq (bse_value_get_note (value));
      g_object_notify (G_OBJECT (self), "base_freq");
      dav_organ_update_modules (self);
      break;
    case PARAM_BRASS:
      self->config.brass = g_value_get_boolean (value);
      dav_organ_update_modules (self);
      break;
    case PARAM_FLUTE:
      self->config.flute = g_value_get_boolean (value);
      dav_organ_update_modules (self);
      break;
    case PARAM_REED:
      self->config.reed = g_value_get_boolean (value);
      dav_organ_update_modules (self);
      break;
    case PARAM_HARM0:
      self->config.harm0 = g_value_get_float (value) / 100.0;
      dav_organ_update_modules (self);
      break;
    case PARAM_HARM1:
      self->config.harm1 = g_value_get_float (value) / 100.0;
      dav_organ_update_modules (self);
      break;
    case PARAM_HARM2:
      self->config.harm2 = g_value_get_float (value) / 100.0;
      dav_organ_update_modules (self);
      break;
    case PARAM_HARM3:
      self->config.harm3 = g_value_get_float (value) / 100.0;
      dav_organ_update_modules (self);
      break;
    case PARAM_HARM4:
      self->config.harm4 = g_value_get_float (value) / 100.0;
      dav_organ_update_modules (self);
      break;
    case PARAM_HARM5:
      self->config.harm5 = g_value_get_float (value) / 100.0;
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
      g_value_set_float (value, self->config.freq);
      break;
    case PARAM_BASE_NOTE:
      bse_value_set_note (value, bse_note_from_freq (self->config.freq));
      break;
    case PARAM_BRASS:
      g_value_set_boolean (value, self->config.brass);
      break;
    case PARAM_FLUTE:
      g_value_set_boolean (value, self->config.flute);
      break;
    case PARAM_REED:
      g_value_set_boolean (value, self->config.reed);
      break;
    case PARAM_HARM0:
      g_value_set_float (value, self->config.harm0 * 100.0);
      break;
    case PARAM_HARM1:
      g_value_set_float (value, self->config.harm1 * 100.0);
      break;
    case PARAM_HARM2:
      g_value_set_float (value, self->config.harm2 * 100.0);
      break;
    case PARAM_HARM3:
      g_value_set_float (value, self->config.harm3 * 100.0);
      break;
    case PARAM_HARM4:
      g_value_set_float (value, self->config.harm4 * 100.0);
      break;
    case PARAM_HARM5:
      g_value_set_float (value, self->config.harm5 * 100.0);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
dav_organ_class_ref_tables (DavOrganClass *class)
{
  gfloat rate = BSE_MIX_FREQ;
  gfloat half = rate / 2;
  gfloat slope = rate / 10;
  gint i;
  
  class->ref_count++;
  if (class->ref_count > 1)
    return;
  
  /* Initialize sine table. */
  class->sine_table = g_new (BseSampleValue, rate);
  for (i = 0; i < rate; i++)
    class->sine_table[i] = sin ((i / rate) * 2.0 * PI) / 6.0;
  
  /* Initialize triangle table. */
  class->triangle_table = g_new (BseSampleValue, rate);
  for (i = 0; i < rate / 2; i++)
    class->triangle_table[i] = (4 / rate * i - 1.0) / 6.0;
  for (; i < rate; i++)
    class->triangle_table[i] = (4 / rate * (rate - i) - 1.0) / 6.0;
  
  /* Initialize pulse table. */
  class->pulse_table = g_new (BseSampleValue, rate);
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
  DavOrganClass *class = DAV_ORGAN_GET_CLASS (source);
  
  dav_organ_class_ref_tables (class);
  
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
    bse_source_update_modules (BSE_SOURCE (self),
			       G_STRUCT_OFFSET (Organ, config),
			       &self->config,
			       sizeof (self->config),
			       NULL);
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
dav_organ_process (GslModule *module,
		   guint      n_values)
{
  Organ *organ = module->user_data;
  DavOrganClass *class = organ->class;
  const gfloat *sine_table = class->sine_table;
  const gfloat *flute_table = organ->config.flute ? class->triangle_table : sine_table;
  const gfloat *reed_table = organ->config.reed ? class->pulse_table : sine_table;
  const gfloat *ifreq = GSL_MODULE_IBUFFER (module, DAV_ORGAN_ICHANNEL_FREQ);
  gfloat *ovalues = GSL_MODULE_OBUFFER (module, DAV_ORGAN_OCHANNEL_MONO);
  guint freq_256, mix_freq_256;
  guint freq_256_harm0, freq_256_harm1;
  guint i;

  if (GSL_MODULE_ISTREAM (module, DAV_ORGAN_ICHANNEL_FREQ).connected)
    freq_256 = BSE_FREQ_FROM_VALUE (ifreq[0]) * 256 + 0.5;
  else
    freq_256 = organ->config.freq * 256 + 0.5;
  mix_freq_256 = BSE_MIX_FREQ * 256;
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
      
      for (i = 0; i < BSE_TRACK_LENGTH; i++)
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
			  GslTrans  *trans)
{
  static const GslClass organ_class = {
    DAV_ORGAN_N_ICHANNELS,	/* n_istreams */
    0,				/* n_jstreams */
    DAV_ORGAN_N_OCHANNELS,	/* n_ostreams */
    dav_organ_process,		/* process */
    NULL,                       /* process_defer */
    NULL,                       /* reconnect */
    (GslModuleFreeFunc) g_free,	/* free */
    GSL_COST_NORMAL,		/* cost */
  };
  DavOrgan *self = DAV_ORGAN (source);
  Organ *organ = g_new0 (Organ, 1);
  GslModule *module;
  
  /* initialize organ data */
  organ->class = DAV_ORGAN_GET_CLASS (self);
  organ->config = self->config;
  module = gsl_module_new (&organ_class, organ);
  
  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);
  
  /* commit module to engine */
  gsl_trans_add (trans, gsl_job_integrate (module));
  
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

/* --- Export to DAV --- */
#include "./icons/organ.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_organ, "DavOrgan", "BseSource",
    "DavOrgan is a modifiable additive organ synthesizer",
    &type_info_organ,
    "/Modules/Audio Sources/Organ",
    { ORGAN_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      ORGAN_IMAGE_WIDTH, ORGAN_IMAGE_HEIGHT,
      ORGAN_IMAGE_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;
