/* DavOrgan - DAV Additive Organ Synthesizer
 * Copyright (c) 1999, 2000 David A. Bartold
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

#include <bse/bsechunk.h>
#include <stdio.h>
#include <stdlib.h>
#include "davorgan.h"

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
static void        dav_organ_init             (DavOrgan      *organ);
static void        dav_organ_class_init       (DavOrganClass  *class);
static void        dav_organ_class_destroy    (DavOrganClass  *class);
static void        dav_organ_do_shutdown      (BseObject      *object);
static void        dav_organ_set_param        (DavOrgan       *organ,
					       BseParam       *param);
static void        dav_organ_get_param        (DavOrgan       *organ,
					       BseParam       *param);
static void        dav_organ_prepare          (BseSource      *source,
					       BseIndex        index);
static BseChunk*   dav_organ_calc_chunk       (BseSource      *source,
					       guint           ochannel_id);
static void        dav_organ_reset            (BseSource      *source);
static inline void dav_organ_update_locals    (DavOrgan       *organ);


/* --- variables --- */
static BseType           type_id_organ = 0;
static gpointer          parent_class = NULL;
static const BseTypeInfo type_info_organ = {
  sizeof (DavOrganClass),
  
  (BseBaseInitFunc) NULL,
  (BseBaseDestroyFunc) NULL,
  (BseClassInitFunc) dav_organ_class_init,
  (BseClassDestroyFunc) dav_organ_class_destroy,
  NULL /* class_data */,
  
  sizeof (DavOrgan),
  0 /* n_preallocs */,
  (BseObjectInitFunc) dav_organ_init,
};


/* --- functions --- */
static BseParamSpec*
harm_param (gchar *name,
	    gchar *nick,
	    gchar *blurb,
	    gint   dft)
{
  return bse_param_spec_int (name, nick, blurb, 0, 127, 1, dft, BSE_PARAM_DEFAULT | BSE_PARAM_HINT_SCALE);
}

static void
dav_organ_class_init (DavOrganClass *class)
{
  BseObjectClass *object_class;
  BseSourceClass *source_class;
  guint ochannel_id;
  
  parent_class = bse_type_class_peek (BSE_TYPE_SOURCE);
  object_class = BSE_OBJECT_CLASS (class);
  source_class = BSE_SOURCE_CLASS (class);
  
  object_class->set_param = (BseObjectSetParamFunc) dav_organ_set_param;
  object_class->get_param = (BseObjectGetParamFunc) dav_organ_get_param;
  object_class->shutdown = dav_organ_do_shutdown;
  
  source_class->prepare = dav_organ_prepare;
  source_class->calc_chunk = dav_organ_calc_chunk;
  source_class->reset = dav_organ_reset;
  
  bse_object_class_add_param (object_class, "Base Frequency", PARAM_BASE_FREQ,
			      bse_param_spec_float( "base_freq", "Frequency", NULL,
						    BSE_MIN_OSC_FREQ_d,
						    BSE_MAX_OSC_FREQ_d,
						    10.0,
						    BSE_KAMMER_FREQ,
						    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Base Frequency",
                              PARAM_BASE_NOTE,
                              bse_param_spec_note ("base_note", "Note", NULL,
                                                   BSE_MIN_NOTE, BSE_MAX_NOTE,
                                                   1, BSE_KAMMER_NOTE, TRUE,
                                                   BSE_PARAM_GUI));
  
  bse_object_class_add_param (object_class, "Instrument flavour", PARAM_BRASS,
			      bse_param_spec_bool ("brass", "Brass sounds", "Changes the organ to sound more brassy",
						   FALSE, BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Instrument flavour", PARAM_REED,
			      bse_param_spec_bool ("reed", "Reed sounds", "Adds reeds sound",
						   FALSE, BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Instrument flavour", PARAM_FLUTE,
			      bse_param_spec_bool ("flute", "Flute sounds", "Adds flute sounds",
						   FALSE, BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Harmonics", PARAM_HARM0,
			      harm_param ("harm0", "16th", "16th harmonic", 127));
  bse_object_class_add_param (object_class, "Harmonics", PARAM_HARM1,
			      harm_param ("harm1", "8th", "8th harmonic", 36));
  bse_object_class_add_param (object_class, "Harmonics", PARAM_HARM2,
			      harm_param ("harm2", "5 1/3rd", "5 1/3rd harmonic", 100));
  bse_object_class_add_param (object_class, "Harmonics", PARAM_HARM3,
			      harm_param ("harm3", "4th", "4th harmonic", 32));
  bse_object_class_add_param (object_class, "Harmonics", PARAM_HARM4,
			      harm_param ("harm4", "2 2/3rd", "2 2/3rd harmonic", 91));
  bse_object_class_add_param (object_class, "Harmonics", PARAM_HARM5,
			      harm_param ("harm5", "2nd", "2nd harmonic", 55));
  
  ochannel_id = bse_source_class_add_ochannel (source_class,
					       "OrganOut", "Organ Output", 1);
}

static void
dav_organ_class_destroy (DavOrganClass *class)
{
}

static void
dav_organ_init (DavOrgan *organ)
{
  organ->brass = FALSE;
  organ->flute = FALSE;
  organ->reed = FALSE;
  
  organ->freq = BSE_KAMMER_FREQ;
  organ->harm0 = 127;
  organ->harm1 = 36;
  organ->harm2 = 100;
  organ->harm3 = 32;
  organ->harm4 = 91;
  organ->harm5 = 55;
}

static void
dav_organ_do_shutdown (BseObject *object)
{
  DavOrgan *organ;
  
  organ = DAV_ORGAN (object);
  
  /* chain parent class' shutdown handler */
  BSE_OBJECT_CLASS (parent_class)->shutdown (object);
}

static void
dav_organ_set_param (DavOrgan *organ,
                     BseParam *param)
{
  switch (param->pspec->any.param_id)
    {
    case PARAM_BASE_FREQ:
      organ->freq = param->value.v_float;
      bse_object_param_changed (BSE_OBJECT (organ), "base_note");
      break;
    case PARAM_BASE_NOTE:
      organ->freq = bse_note_to_freq (param->value.v_note);
      bse_object_param_changed (BSE_OBJECT (organ), "base_freq");
      break;
    case PARAM_BRASS:
      organ->brass = param->value.v_bool;
      break;
    case PARAM_FLUTE:
      organ->flute = param->value.v_bool;
      break;
    case PARAM_REED:
      organ->reed = param->value.v_bool;
      break;
    case PARAM_HARM0:
      organ->harm0 = param->value.v_int;
      break;
    case PARAM_HARM1:
      organ->harm1 = param->value.v_int;
      break;
    case PARAM_HARM2:
      organ->harm2 = param->value.v_int;
      break;
    case PARAM_HARM3:
      organ->harm3 = param->value.v_int;
      break;
    case PARAM_HARM4:
      organ->harm4 = param->value.v_int;
      break;
    case PARAM_HARM5:
      organ->harm5 = param->value.v_int;
      break;
    default:
      g_warning ("%s(\"%s\"): invalid attempt to set parameter \"%s\" of type `%s'",
		 BSE_OBJECT_TYPE_NAME (organ),
		 BSE_OBJECT_NAME (organ),
		 param->pspec->any.name,
		 bse_type_name (param->pspec->type));
      break;
    }
}

static void
dav_organ_get_param (DavOrgan *organ,
                     BseParam *param)
{
  switch (param->pspec->any.param_id)
    {
    case PARAM_BASE_FREQ:
      param->value.v_float = organ->freq;
      break;
    case PARAM_BASE_NOTE:
      param->value.v_note = bse_note_from_freq (organ->freq);
      break;
    case PARAM_BRASS:
      param->value.v_bool = organ->brass;
      break;
    case PARAM_FLUTE:
      param->value.v_bool = organ->flute;
      break;
    case PARAM_REED:
      param->value.v_bool = organ->reed;
      break;
    case PARAM_HARM0:
      param->value.v_int = organ->harm0;
      break;
    case PARAM_HARM1:
      param->value.v_int = organ->harm1;
      break;
    case PARAM_HARM2:
      param->value.v_int = organ->harm2;
      break;
    case PARAM_HARM3:
      param->value.v_int = organ->harm3;
      break;
    case PARAM_HARM4:
      param->value.v_int = organ->harm4;
      break;
    case PARAM_HARM5:
      param->value.v_int = organ->harm5;
      break;
      
    default:
      g_warning ("%s(\"%s\"): invalid attempt to get parameter \"%s\" of type `%s'",
                 BSE_OBJECT_TYPE_NAME (organ),
                 BSE_OBJECT_NAME (organ),
                 param->pspec->any.name,
                 bse_type_name (param->pspec->type));
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
    class->sine_table[i] = sin ((i / rate) * 2.0 * PI) * BSE_MAX_SAMPLE_VALUE_f;
  
  /* Initialize triangle table. */
  class->triangle_table = g_new (BseSampleValue, rate);
  for (i = 0; i < rate / 2; i++)
    class->triangle_table[i] = 4 * BSE_MAX_SAMPLE_VALUE_f / rate * i - BSE_MAX_SAMPLE_VALUE_f;
  for (; i < rate; i++)
    class->triangle_table[i] = 4 * BSE_MAX_SAMPLE_VALUE_f / rate * (rate / 2 - i - 1) - BSE_MAX_SAMPLE_VALUE_f;
  
  /* Initialize pulse table. */
  class->pulse_table = g_new (BseSampleValue, rate);
  for (i = 0; i < slope; i++)
    class->pulse_table[i] = -i * BSE_MAX_SAMPLE_VALUE_f / slope;
  for (; i < half - slope; i++)
    class->pulse_table[i] = -BSE_MAX_SAMPLE_VALUE_f;
  for (; i < half + slope; i++)
    class->pulse_table[i] = (i - half) * BSE_MAX_SAMPLE_VALUE_f / slope;
  for (; i < rate - slope; i++)
    class->pulse_table[i] = BSE_MAX_SAMPLE_VALUE_f;
  for (; i < rate; i++)
    class->pulse_table[i] = (rate - i) * BSE_MAX_SAMPLE_VALUE_f / slope;
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

static inline BseSampleValue
table_pos (BseSampleValue *table,
	   guint           freq_2,
	   guint           mix_freq_2,
	   guint32        *accum)
{
  *accum += freq_2;
  while (*accum >= mix_freq_2)
    *accum -= mix_freq_2;
  
  return table[*accum >> 1];
}

static void
dav_organ_prepare (BseSource *source,
                   BseIndex index)
{
  DavOrganClass *class = DAV_ORGAN_GET_CLASS (source);

  dav_organ_class_ref_tables (class);

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source, index);
}

static BseChunk*
dav_organ_calc_chunk (BseSource *source,
                      guint ochannel_id)
{
  DavOrgan *organ = DAV_ORGAN (source);
  DavOrganClass *class = DAV_ORGAN_GET_CLASS (source);
  BseSampleValue *sine_table, *flute_table, *reed_table;
  BseSampleValue *hunk;
  guint freq_2, mix_freq_2;
  guint i;
  
  g_return_val_if_fail (ochannel_id == DAV_ORGAN_OCHANNEL_MONO, NULL);
  
  hunk = bse_hunk_alloc (1);

  sine_table = class->sine_table;
  reed_table = organ->reed ? class->pulse_table : sine_table;
  flute_table = organ->flute ? class->triangle_table : sine_table;
  freq_2 = organ->freq * 2;
  mix_freq_2 = BSE_MIX_FREQ * 2;
  
  if (organ->brass)
    for (i = 0; i < BSE_TRACK_LENGTH; i++)
      {
	BseMixValue accum;

	accum = table_pos (sine_table, freq_2, mix_freq_2, &organ->harm1_accum) * organ->harm1;
	accum += table_pos (sine_table, freq_2 / 2, mix_freq_2, &organ->harm0_accum) * organ->harm0;
	accum += table_pos (reed_table, freq_2 * 2, mix_freq_2, &organ->harm2_accum) * organ->harm2;
	accum += table_pos (sine_table, freq_2 * 4, mix_freq_2, &organ->harm3_accum) * organ->harm3;
	accum += table_pos (flute_table, freq_2 * 8, mix_freq_2, &organ->harm4_accum) * organ->harm4;
	accum += table_pos (flute_table, freq_2 * 16, mix_freq_2, &organ->harm5_accum) * organ->harm5;
	hunk[i] = accum / (6 * 128);
      }
  else
    for (i = 0; i < BSE_TRACK_LENGTH; i++)
      {
	BseMixValue accum;

	accum = table_pos (sine_table, freq_2, mix_freq_2, &organ->harm1_accum) * organ->harm1;
	accum += table_pos (sine_table, freq_2 / 2, mix_freq_2, &organ->harm0_accum) * organ->harm0;
	accum += table_pos (sine_table, freq_2 * 3 / 2, mix_freq_2, &organ->harm2_accum) * organ->harm2;
	accum += table_pos (reed_table, freq_2 * 2, mix_freq_2, &organ->harm3_accum) * organ->harm3;
	accum += table_pos (sine_table, freq_2 * 3, mix_freq_2, &organ->harm4_accum) * organ->harm4;
	accum += table_pos (flute_table, freq_2 * 4, mix_freq_2, &organ->harm5_accum) * organ->harm5;
	hunk[i] = accum / (6 * 128);
      }

  return bse_chunk_new_orphan (1, hunk);
}

static void
dav_organ_reset (BseSource *source)
{
  DavOrganClass *class = DAV_ORGAN_GET_CLASS (source);

  dav_organ_class_unref_tables (class);

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}

static inline void
dav_organ_update_locals (DavOrgan *organ)
{
}

/* --- Export to DAV --- */
#include "./icons/noicon.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_organ, "DavOrgan", "BseSource",
    "DavOrgan is a modifiable additive organ synthesizer",
    &type_info_organ,
    "/Source/Organ",
    { NOICON_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      NOICON_WIDTH, NOICON_HEIGHT,
      NOICON_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;
