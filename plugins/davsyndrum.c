/* DavSynDrum - DAV Drum Synthesizer
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
#include <bse/bsehunkmixer.h>
#include "davsyndrum.h"

/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_BASE_FREQ,
  PARAM_BASE_NOTE,
  PARAM_TRIGGER_VEL,
  PARAM_TRIGGER_HIT,
  PARAM_RES,
  PARAM_RATIO
};


/* --- prototypes --- */
static void        dav_syn_drum_init             (DavSynDrum       *drum);
static void        dav_syn_drum_class_init       (DavSynDrumClass  *class);
static void        dav_syn_drum_class_destroy    (DavSynDrumClass  *class);
static void        dav_syn_drum_do_shutdown      (BseObject        *object);
static void        dav_syn_drum_set_param        (DavSynDrum       *drum,
						  BseParam         *param);
static void        dav_syn_drum_get_param        (DavSynDrum       *drum,
						  BseParam         *param);
static void        dav_syn_drum_prepare          (BseSource        *source,
						  BseIndex          index);
static BseChunk*   dav_syn_drum_calc_chunk       (BseSource        *source,
						  guint             ochannel_id);
static void        dav_syn_drum_reset            (BseSource        *source);
static inline void dav_syn_drum_update_locals    (DavSynDrum       *drum);


/* --- variables --- */
static BseType           type_id_syn_drum = 0;
static gpointer          parent_class = NULL;
static const BseTypeInfo type_info_syn_drum = {
  sizeof (DavSynDrumClass),
  
  (BseBaseInitFunc) NULL,
  (BseBaseDestroyFunc) NULL,
  (BseClassInitFunc) dav_syn_drum_class_init,
  (BseClassDestroyFunc) dav_syn_drum_class_destroy,
  NULL /* class_data */,
  
  sizeof (DavSynDrum),
  0 /* n_preallocs */,
  (BseObjectInitFunc) dav_syn_drum_init,
};


/* --- functions --- */

/* Calculate the half life rate given:
 *  half - the length of the half life
 *  rate - time divisor (usually the # calcs per second)
 *
 * Basically, find r given 1/2 = e^(-r*(half/rate))
 */
static gfloat
calc_exponent (gfloat half, gfloat rate)
{
  /* ln (1 / 2) = ~-0.69314718056 */
  return exp (-0.69314718056 / (half * rate));
}

static inline void
dav_syn_drum_update_locals (DavSynDrum *drum)
{
  drum->res = calc_exponent (drum->half, BSE_MIX_FREQ);
}

static void
dav_syn_drum_class_init (DavSynDrumClass *class)
{
  BseObjectClass *object_class;
  BseSourceClass *source_class;
  guint ochannel_id;
  
  parent_class = bse_type_class_peek (BSE_TYPE_SOURCE);
  object_class = BSE_OBJECT_CLASS (class);
  source_class = BSE_SOURCE_CLASS (class);
  
  object_class->set_param = (BseObjectSetParamFunc) dav_syn_drum_set_param;
  object_class->get_param = (BseObjectGetParamFunc) dav_syn_drum_get_param;
  object_class->shutdown = dav_syn_drum_do_shutdown;
  
  source_class->prepare = dav_syn_drum_prepare;
  source_class->calc_chunk = dav_syn_drum_calc_chunk;
  source_class->reset = dav_syn_drum_reset;
  
  bse_object_class_add_param (object_class, "Base Frequency", PARAM_BASE_FREQ,
			      bse_param_spec_float ("base_freq", "Frequency", NULL,
						    1.0, bse_note_to_freq (BSE_NOTE_Gis (-1)),
						    10.0, 30.0,
						    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Base Frequency",
                              PARAM_BASE_NOTE,
                              bse_param_spec_note ("base_note", "Note", NULL,
                                                   BSE_MIN_NOTE, BSE_NOTE_Gis (-1),
                                                   1, BSE_NOTE_VOID, TRUE,
                                                   BSE_PARAM_GUI));
  
  bse_object_class_add_param (object_class, "Trigger", PARAM_TRIGGER_VEL,
			      bse_param_spec_float ("trigger_vel", "Trigger Velocity [%]",
						    "Set the velocity of the drum hit",
						    0.0, 1000.0, 10.0, 75.0,
						    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Trigger", PARAM_TRIGGER_HIT,
			      bse_param_spec_bool ("trigger_pulse", "Trigger Hit", "Hit the drum",
						   FALSE, BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Parameters", PARAM_RES,
			      bse_param_spec_float ("res", "Resonance",
						    "Set resonance half life in number of seconds",
						    0.001, 1.0, 0.0025, 0.07,
						    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Parameters", PARAM_RATIO,
			      bse_param_spec_float ("ratio", "Frequency Ratio",
						    "Set ratio of frequency shift. (i.e. 1.0 means shift equal to the drum's base frequency)",
						    0.0, 10.0, 0.1, 2.0,
						    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_SCALE));
  
  ochannel_id = bse_source_class_add_ochannel (source_class,
					       "SynDrumOut", "SynDrum Output", 1);
}

static void
dav_syn_drum_class_destroy (DavSynDrumClass *class)
{
}

static void
dav_syn_drum_init (DavSynDrum *drum)
{
  drum->ratio = 2.0;
  drum->freq = 30.0;
  drum->trigger_vel = 0.75;
  drum->spring_pos = 0.0;
  drum->spring_vel = 0.0;
  drum->env = 0.0;
  drum->half = 0.07;
  drum->res = 0;
  dav_syn_drum_update_locals (drum);
}

static void
dav_syn_drum_do_shutdown (BseObject *object)
{
  DavSynDrum *syndrum;
  
  syndrum = DAV_SYN_DRUM (object);
  
  /* chain parent class' shutdown handler */
  BSE_OBJECT_CLASS (parent_class)->shutdown (object);
}

static void
dav_syn_drum_set_param (DavSynDrum *drum,
			BseParam *param)
{
  switch (param->pspec->any.param_id)
    {
    case PARAM_BASE_FREQ:
      drum->freq = param->value.v_float;
      bse_object_param_changed (BSE_OBJECT (drum), "base_note");
      break;
    case PARAM_BASE_NOTE:
      drum->freq = bse_note_to_freq (param->value.v_note);
      bse_object_param_changed (BSE_OBJECT (drum), "base_freq");
      break;
    case PARAM_TRIGGER_VEL:
      drum->trigger_vel = param->value.v_float / 100.0;
      break;
    case PARAM_TRIGGER_HIT:
      drum->spring_vel = drum->trigger_vel;
      drum->env = drum->trigger_vel;
      break;
    case PARAM_RATIO:
      drum->ratio = param->value.v_float;
      break;
    case PARAM_RES:
      drum->half = param->value.v_float;
      dav_syn_drum_update_locals (drum);
      break;
    default:
      g_warning ("%s(\"%s\"): invalid attempt to set parameter \"%s\" of type `%s'",
		 BSE_OBJECT_TYPE_NAME (drum),
		 BSE_OBJECT_NAME (drum),
		 param->pspec->any.name,
		 bse_type_name (param->pspec->type));
      break;
    }
}

static void
dav_syn_drum_get_param (DavSynDrum *drum,
			BseParam *param)
{
  switch (param->pspec->any.param_id)
    {
    case PARAM_BASE_FREQ:
      param->value.v_float = drum->freq;
      break;
    case PARAM_BASE_NOTE:
      param->value.v_note = bse_note_from_freq (drum->freq);
      break;
    case PARAM_TRIGGER_VEL:
      param->value.v_float = drum->trigger_vel * 100.0;
      break;
    case PARAM_TRIGGER_HIT:
      param->value.v_bool = FALSE;
      break;
    case PARAM_RATIO:
      param->value.v_float = drum->ratio;
      break;
    case PARAM_RES:
      param->value.v_float = drum->half;
      break;
      
    default:
      g_warning ("%s(\"%s\"): invalid attempt to get parameter \"%s\" of type `%s'",
                 BSE_OBJECT_TYPE_NAME (drum),
                 BSE_OBJECT_NAME (drum),
                 param->pspec->any.name,
                 bse_type_name (param->pspec->type));
      break;
    }
}

static void
dav_syn_drum_prepare (BseSource *source,
		      BseIndex index)
{
  DavSynDrum *drum = DAV_SYN_DRUM (source);
  
  dav_syn_drum_update_locals (drum);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source, index);
}

static BseChunk*
dav_syn_drum_calc_chunk (BseSource *source,
			 guint ochannel_id)
{
  DavSynDrum *drum = DAV_SYN_DRUM (source);
  BseSampleValue *hunk;
  gfloat sample;
  gfloat freq_shift;
  gfloat factor;
  guint i;
  
  g_return_val_if_fail (ochannel_id == DAV_SYN_DRUM_OCHANNEL_MONO, NULL);
  
  hunk = bse_hunk_alloc (1);
  factor = 2.0 * PI / BSE_MIX_FREQ_f;
  
  freq_shift = drum->freq * drum->ratio;
  
  for (i = 0; i < BSE_TRACK_LENGTH; i++)
    {
      gfloat cur_freq;
      
      cur_freq = drum->freq + (drum->env * freq_shift);
      cur_freq *= factor;
      drum->spring_vel -= drum->spring_pos * cur_freq;
      drum->spring_pos += drum->spring_vel * cur_freq;
      drum->spring_vel *= drum->res;
      drum->env *= drum->res;
      
      sample = drum->spring_pos * BSE_MAX_SAMPLE_VALUE_f;
      
      hunk[i] = BSE_CLIP_SAMPLE_VALUE (sample);
    }
  
  return bse_chunk_new_orphan (1, hunk);
}

static void
dav_syn_drum_reset (BseSource *source)
{
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}

/* --- Export to DAV --- */
#include "./icons/noicon.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_syn_drum, "DavSynDrum", "BseSource",
    "DavSynDrum is a synthesized drum",
    &type_info_syn_drum,
    "/Source/SynDrum",
    { NOICON_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      NOICON_WIDTH, NOICON_HEIGHT,
      NOICON_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;
