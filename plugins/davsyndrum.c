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
static void        dav_syn_drum_class_finalize   (DavSynDrumClass  *class);
static void        dav_syn_drum_set_param        (DavSynDrum       *drum,
						  guint             param_id,
						  GValue           *value,
						  GParamSpec       *pspec,
						  const gchar      *trailer);
static void        dav_syn_drum_get_param        (DavSynDrum       *drum,
						  guint             param_id,
						  GValue           *value,
						  GParamSpec       *pspec,
						  const gchar      *trailer);
static void        dav_syn_drum_prepare          (BseSource        *source,
						  BseIndex          index);
static BseChunk*   dav_syn_drum_calc_chunk       (BseSource        *source,
						  guint             ochannel_id);
static void        dav_syn_drum_reset            (BseSource        *source);
static inline void dav_syn_drum_update_locals    (DavSynDrum       *drum);


/* --- variables --- */
static GType             type_id_syn_drum = 0;
static gpointer          parent_class = NULL;
static const GTypeInfo type_info_syn_drum = {
  sizeof (DavSynDrumClass),
  
  (GBaseInitFunc) NULL,
  (GBaseFinalizeFunc) NULL,
  (GClassInitFunc) dav_syn_drum_class_init,
  (GClassFinalizeFunc) dav_syn_drum_class_finalize,
  NULL /* class_data */,
  
  sizeof (DavSynDrum),
  0 /* n_preallocs */,
  (GInstanceInitFunc) dav_syn_drum_init,
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
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ochannel_id, ichannel_id;
  
  parent_class = g_type_class_peek (BSE_TYPE_SOURCE);
  
  gobject_class->set_param = (GObjectSetParamFunc) dav_syn_drum_set_param;
  gobject_class->get_param = (GObjectGetParamFunc) dav_syn_drum_get_param;
  
  source_class->prepare = dav_syn_drum_prepare;
  source_class->calc_chunk = dav_syn_drum_calc_chunk;
  source_class->reset = dav_syn_drum_reset;
  
  bse_object_class_add_param (object_class, "Base Frequency", PARAM_BASE_FREQ,
			      b_param_spec_float ("base_freq", "Frequency", NULL,
						  1.0, bse_note_to_freq (BSE_NOTE_Gis (-1)),
						  30.0, 10.0,
						  B_PARAM_DEFAULT | B_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Base Frequency",
                              PARAM_BASE_NOTE,
                              b_param_spec_note ("base_note", "Note", NULL,
						 BSE_MIN_NOTE, BSE_NOTE_Gis (-1),
						 BSE_NOTE_VOID, 1, TRUE,
						 B_PARAM_GUI));
  
  bse_object_class_add_param (object_class, "Trigger", PARAM_TRIGGER_VEL,
			      b_param_spec_float ("trigger_vel", "Trigger Velocity [%]",
						  "Set the velocity of the drum hit",
						  0.0, 1000.0, 75.0, 10.0,
						  B_PARAM_DEFAULT | B_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Trigger", PARAM_TRIGGER_HIT,
			      b_param_spec_bool ("trigger_pulse", "Trigger Hit", "Hit the drum",
						 FALSE, B_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Parameters", PARAM_RES,
			      b_param_spec_float ("res", "Resonance",
						  "Set resonance half life in number of seconds",
						  0.001, 1.0, 0.07, 0.0025,
						  B_PARAM_DEFAULT | B_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Parameters", PARAM_RATIO,
			      b_param_spec_float ("ratio", "Frequency Ratio",
						  "Set ratio of frequency shift. (i.e. 1.0 means shift equal to the drum's base frequency)",
						  0.0, 10.0, 2.0, 0.1,
						  B_PARAM_DEFAULT | B_PARAM_HINT_SCALE));
  
  ochannel_id = bse_source_class_add_ochannel (source_class, "mono_out", "SynDrum Output", 1);
  g_assert (ochannel_id == DAV_SYN_DRUM_OCHANNEL_MONO);
  ichannel_id = bse_source_class_add_ichannel (source_class, "trigger_in", "Trigger Input", 1, 1);
  g_assert (ichannel_id == BSE_SYN_DRUM_ICHANNEL_TRIGGER);
}

static void
dav_syn_drum_class_finalize (DavSynDrumClass *class)
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
  drum->input_trigger_state = FALSE;
  dav_syn_drum_update_locals (drum);
}

void
dav_syn_drum_trigger (DavSynDrum *drum)
{
  g_return_if_fail (DAV_IS_SYN_DRUM (drum));

  drum->spring_vel = drum->trigger_vel;
  drum->env = drum->trigger_vel;
}

static void
dav_syn_drum_set_param (DavSynDrum  *drum,
			guint        param_id,
			GValue      *value,
			GParamSpec  *pspec,
			const gchar *trailer)
{
  switch (param_id)
    {
    case PARAM_BASE_FREQ:
      drum->freq = b_value_get_float (value);
      bse_object_param_changed (BSE_OBJECT (drum), "base_note");
      break;
    case PARAM_BASE_NOTE:
      drum->freq = bse_note_to_freq (b_value_get_note (value));
      bse_object_param_changed (BSE_OBJECT (drum), "base_freq");
      break;
    case PARAM_TRIGGER_VEL:
      drum->trigger_vel = b_value_get_float (value) / 100.0;
      break;
    case PARAM_TRIGGER_HIT:
      dav_syn_drum_trigger (drum);
      break;
    case PARAM_RATIO:
      drum->ratio = b_value_get_float (value);
      break;
    case PARAM_RES:
      drum->half = b_value_get_float (value);
      dav_syn_drum_update_locals (drum);
      break;
    default:
      G_WARN_INVALID_PARAM_ID (drum, param_id, pspec);
      break;
    }
}

static void
dav_syn_drum_get_param (DavSynDrum *drum,
			guint        param_id,
			GValue      *value,
			GParamSpec  *pspec,
			const gchar *trailer)
{
  switch (param_id)
    {
    case PARAM_BASE_FREQ:
      b_value_set_float (value, drum->freq);
      break;
    case PARAM_BASE_NOTE:
      b_value_set_note (value, bse_note_from_freq (drum->freq));
      break;
    case PARAM_TRIGGER_VEL:
      b_value_set_float (value, drum->trigger_vel * 100.0);
      break;
    case PARAM_TRIGGER_HIT:
      b_value_set_bool (value, FALSE);
      break;
    case PARAM_RATIO:
      b_value_set_float (value, drum->ratio);
      break;
    case PARAM_RES:
      b_value_set_float (value, drum->half);
      break;
      
    default:
      G_WARN_INVALID_PARAM_ID (drum, param_id, pspec);
      break;
    }
}

static void
dav_syn_drum_prepare (BseSource *source,
		      BseIndex index)
{
  DavSynDrum *drum = DAV_SYN_DRUM (source);
  
  drum->input_trigger_state = FALSE;
  dav_syn_drum_update_locals (drum);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source, index);
}

static BseChunk*
dav_syn_drum_calc_chunk (BseSource *source,
			 guint ochannel_id)
{
  DavSynDrum *drum = DAV_SYN_DRUM (source);
  BseSourceInput *trigger_input;
  BseSampleValue *hunk;
  gfloat sample;
  gfloat freq_shift;
  gfloat factor;
  guint i;
  
  g_return_val_if_fail (ochannel_id == DAV_SYN_DRUM_OCHANNEL_MONO, NULL);
  
  trigger_input = bse_source_get_input (source, BSE_SYN_DRUM_ICHANNEL_TRIGGER);
  if (trigger_input)
    {
      BseChunk *chunk = bse_source_ref_chunk (trigger_input->osource, trigger_input->ochannel_id, source->index);
      gboolean old_state = drum->input_trigger_state;

      drum->input_trigger_state = bse_chunk_get_trigger_state (chunk, 0);
      bse_chunk_unref (chunk);

      if (!old_state && drum->input_trigger_state)
	dav_syn_drum_trigger (drum);
    }

  factor = 2.0 * PI / BSE_MIX_FREQ_f;
  hunk = bse_hunk_alloc (1);
  
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
  DavSynDrum *drum = DAV_SYN_DRUM (source);

  drum->input_trigger_state = FALSE;

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}

/* --- Export to DAV --- */
#include "./icons/drum.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_syn_drum, "DavSynDrum", "BseSource",
    "DavSynDrum is a synthesized drum",
    &type_info_syn_drum,
    "/Source/SynDrum",
    { DRUM_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      DRUM_IMAGE_WIDTH, DRUM_IMAGE_HEIGHT,
      DRUM_IMAGE_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;
