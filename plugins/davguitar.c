/* DavGuitar - DAV Physical Modelling Acoustic Guitar Synthesizer
 * Copyright (c) 2000 David A. Bartold
 *
 * This software uses technology under patent US 4,649,783, which is
 * set to expire in May of 2004.  In the meantime, a non-patented
 * alternative to this module is in the works.
 *
 ***********************************************************************
 *
 * Any commercial use of this module requires a license from
 * Stanford University.
 *
 ***********************************************************************
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <bse/bsechunk.h>
#include <bse/bsehunkmixer.h>
#include "davguitar.h"

/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_FREQ_1,
  PARAM_NOTE_1,
  PARAM_TRIGGER_1,
  PARAM_FREQ_2,
  PARAM_NOTE_2,
  PARAM_TRIGGER_2,
  PARAM_FREQ_3,
  PARAM_NOTE_3,
  PARAM_TRIGGER_3,
  PARAM_FREQ_4,
  PARAM_NOTE_4,
  PARAM_TRIGGER_4,
  PARAM_FREQ_5,
  PARAM_NOTE_5,
  PARAM_TRIGGER_5,
  PARAM_FREQ_6,
  PARAM_NOTE_6,
  PARAM_TRIGGER_6,
  PARAM_TRIGGER_VEL,
  PARAM_TRIGGER_ALL,
  PARAM_STOP_ALL,
  PARAM_METALLIC_FACTOR,
  PARAM_SNAP_FACTOR
};


/* --- prototypes --- */
static void	   dav_guitar_init	       (DavGuitar	*guitar);
static void	   dav_guitar_class_init       (DavGuitarClass	*guitar);
static void	   dav_guitar_class_finalize   (DavGuitarClass	*guitar);
static void	   dav_guitar_do_destroy       (BseObject       *object);
static void	   dav_guitar_set_property     (DavGuitar	*guitar,
						guint            param_id,
						GValue          *value,
						GParamSpec      *pspec);
static void	   dav_guitar_get_property     (DavGuitar	*guitar,
						guint            param_id,
						GValue          *value,
						GParamSpec      *pspec);
static void	   dav_guitar_prepare	       (BseSource	*source,
						BseIndex	  index);
static BseChunk*   dav_guitar_calc_chunk       (BseSource	*source,
						guint		  ochannel_id);
static void	   dav_guitar_reset	       (BseSource	*source);
static inline void dav_guitar_update_locals    (DavGuitar       *guitar);


/* --- variables --- */
static GType		 type_id_guitar = 0;
static gpointer		 parent_class = NULL;
static const GTypeInfo	 type_info_guitar = {
  sizeof (DavGuitarClass),
  
  (GBaseInitFunc) NULL,
  (GBaseFinalizeFunc) NULL,
  (GClassInitFunc) dav_guitar_class_init,
  (GClassFinalizeFunc) dav_guitar_class_finalize,
  NULL /* class_data */,
  
  sizeof (DavGuitar),
  0 /* n_preallocs */,
  (GInstanceInitFunc) dav_guitar_init,
};


/* --- functions --- */
static void
wave_guide_unstop (WaveGuide *wave)
{
  wave->lowpass_coeff = pow (0.5, 1.0 / (wave->freq * 0.02));
}

static void
wave_guide_stop (WaveGuide *wave)
{
  wave->lowpass_coeff = pow (0.5, 1.0 / (wave->freq * 0.002));
}

static void
wave_guide_set_freq (WaveGuide *wave, float frq)
{
  guint i;
  
  wave->freq = frq;
  wave->pos = 0;
  wave->wavelen = (int) (BSE_MIX_FREQ / frq);
  wave->lowpass_data = 0.0;
  wave_guide_unstop (wave);
  
  for (i = 0; i < wave->wavelen; i++)
    wave->data[i] = 0.0;
}

static void
wave_guide_pluck (WaveGuide *wave,
		  gfloat     metallic_factor,
		  gfloat     snap_factor,
		  gfloat     trigger_vel)
{
  guint i;
  guint pivot;
  
  wave->lowpass_data = 0.0;
  wave_guide_unstop (wave);
  
  /* Initialize wave guide (i.e. string) by setting it to random data. */
  
  /* Create envelope. */
  pivot = wave->wavelen / 5;
  
  for (i = 0; i <= pivot; i++)
    wave->data[i] = ((float) i) / pivot;
  
  for (; i < wave->wavelen; i++)
    wave->data[i] = ((float) (wave->wavelen - i - 1)) / (wave->wavelen - pivot - 1);
  
  /* Add some snap. */
  for (i = 0; i < wave->wavelen; i++)
    wave->data[i] = pow (wave->data[i], snap_factor * 10.0F + 1.0F);
  
  /* Add static to displacements. */
  for (i = 0; i < wave->wavelen; i++)
    wave->data[i] = wave->data[i] * (1.0F - metallic_factor) +
      ((rand() & 1) ? -1.0F : 1.0F) * metallic_factor;
  
  /* Set velocity. */
  for (i = 0; i < wave->wavelen; i++)
    wave->data[i] *= trigger_vel;
}

static void
wave_guide_init (WaveGuide *wave, float frq)
{
  wave->data = g_new0 (gfloat, (BSE_MIX_FREQ + 49) / 50);
  wave_guide_set_freq (wave, frq);
}

static void
wave_guide_free (WaveGuide *wave)
{
  g_free (wave->data);
  wave->data = NULL;
}

static void
wave_guide_prepare (WaveGuide *wave)
{
  wave_guide_free (wave);
  wave_guide_init (wave, wave->freq);
}

static inline gint
fast_mod (gint x, gint y)
{
  return (x >= y) ? x - y : x;
}

static inline void
wave_guide_advance (WaveGuide *wave)
{
  wave->lowpass_data = wave->lowpass_data * (1.0F - wave->lowpass_coeff) + wave->data[wave->pos] * wave->lowpass_coeff;
  wave->data[wave->pos] = wave->lowpass_data;
  
  wave->pos = ((wave->pos + 1) == wave->wavelen) ? 0 : wave->pos + 1;
}

static inline int
wave_guide_get_pos (WaveGuide *wave, int x)
{
  return fast_mod (wave->pos + x, wave->wavelen);
}

static void
add_string_param (BseObjectClass *object_class,
		  gchar		 *name,
		  gchar		 *freq_param,
		  gint		  freq_enum,
		  gfloat	  freq_value,
		  gchar		 *note_param,
		  gint		  note_enum,
		  gint		  note_value,
		  gchar		 *trigger_param,
		  gint		  trigger_enum)
{
  bse_object_class_add_param (object_class, name, freq_enum,
			      bse_param_spec_float (freq_param, "Frequency", NULL,
						  50.0, 2000.0,
						  freq_value, 0.1,
						  BSE_PARAM_DEFAULT | BSE_PARAM_HINT_DIAL));
  
  bse_object_class_add_param (object_class, name, note_enum,
			      bse_param_spec_note (note_param, "Note", NULL,
						 BSE_NOTE_G (-2), BSE_NOTE_B (3),
						 note_value, 1, FALSE,
						 BSE_PARAM_GUI));
  
  bse_object_class_add_param (object_class, name, trigger_enum,
			      bse_param_spec_bool (trigger_param, "Trigger Hit", "Pluck the string",
						 FALSE, BSE_PARAM_GUI));
}

static inline void
dav_guitar_update_locals (DavGuitar *guitar)
{
}

static void
dav_guitar_class_init (DavGuitarClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ochannel_id;
  
  parent_class = g_type_class_peek (BSE_TYPE_SOURCE);
  
  gobject_class->set_property = (GObjectSetPropertyFunc) dav_guitar_set_property;
  gobject_class->get_property = (GObjectGetPropertyFunc) dav_guitar_get_property;

  object_class->destroy = dav_guitar_do_destroy;
  
  source_class->prepare = dav_guitar_prepare;
  source_class->calc_chunk = dav_guitar_calc_chunk;
  source_class->reset = dav_guitar_reset;
  
  add_string_param (object_class, "String 1",
		    "freq_1", PARAM_FREQ_1, 97.998859,	"note_1", PARAM_NOTE_1, BSE_NOTE_G (-1), "trigger_1", PARAM_TRIGGER_1);
  add_string_param (object_class, "String 2",
		    "freq_2", PARAM_FREQ_2, 123.470825, "note_2", PARAM_NOTE_2, BSE_NOTE_B (-1), "trigger_2", PARAM_TRIGGER_2);
  add_string_param (object_class, "String 3",
		    "freq_3", PARAM_FREQ_3, 146.832384, "note_3", PARAM_NOTE_3, BSE_NOTE_D (0),	 "trigger_3", PARAM_TRIGGER_3);
  add_string_param (object_class, "String 4",
		    "freq_4", PARAM_FREQ_4, 195.997718, "note_4", PARAM_NOTE_4, BSE_NOTE_G (0),	 "trigger_4", PARAM_TRIGGER_4);
  add_string_param (object_class, "String 5",
		    "freq_5", PARAM_FREQ_5, 246.941651, "note_5", PARAM_NOTE_5, BSE_NOTE_B (0),	 "trigger_5", PARAM_TRIGGER_5);
  add_string_param (object_class, "String 6",
		    "freq_6", PARAM_FREQ_6, 391.995436, "note_6", PARAM_NOTE_6, BSE_NOTE_G (1),	 "trigger_6", PARAM_TRIGGER_6);
  
  bse_object_class_add_param (object_class, "Global Control", PARAM_TRIGGER_VEL,
			      bse_param_spec_float ("trigger_vel", "Trigger Velocity [%]",
						  "Set the velocity of the string pluck",
						  0.0, 100.0, 100.0, 10.0,
						  BSE_PARAM_GUI | BSE_PARAM_HINT_SCALE));
  
  bse_object_class_add_param (object_class, "Global Control", PARAM_TRIGGER_ALL,
			      bse_param_spec_bool ("trigger_all", "Trigger Hit All", "Strum guitar",
						 FALSE, BSE_PARAM_GUI));
  
  bse_object_class_add_param (object_class, "Global Control", PARAM_STOP_ALL,
			      bse_param_spec_bool ("stop_all", "Stop All", "Stop all the strings from vibrating",
						 FALSE, BSE_PARAM_GUI));
  
  bse_object_class_add_param (object_class, "Flavour", PARAM_METALLIC_FACTOR,
			      bse_param_spec_float ("metallic_factor", "Metallic Factor [%]",
						  "Set the metallicness of the string",
						  0.0, 100.0, 16.0, 0.25,
						  BSE_PARAM_DEFAULT | BSE_PARAM_HINT_SCALE));
  
  bse_object_class_add_param (object_class, "Flavour", PARAM_SNAP_FACTOR,
			      bse_param_spec_float ("snap_factor", "Snap Factor [%]",
						  "Set the snappiness of the string",
						  0.0, 100.0, 34.0, 0.25,
						  BSE_PARAM_DEFAULT | BSE_PARAM_HINT_SCALE));
  
  ochannel_id = bse_source_class_add_ochannel (source_class, "mono_out", "GuitarOutput", 1);
  g_assert (ochannel_id == DAV_GUITAR_OCHANNEL_MONO);
}

static void
dav_guitar_class_finalize (DavGuitarClass *class)
{
}

static void
dav_guitar_init (DavGuitar *guitar)
{
  guitar->hipass_coeff = 0;
  guitar->hipass_data = 0.0;
  guitar->trigger_vel = 1.0;
  guitar->metallic_factor = 0.16;
  guitar->snap_factor = 0.34;
  
  wave_guide_init (&guitar->strings[0], 097.998859); /* G */
  wave_guide_init (&guitar->strings[1], 123.470825); /* B */
  wave_guide_init (&guitar->strings[2], 146.832384); /* D */
  wave_guide_init (&guitar->strings[3], 195.997718); /* G */
  wave_guide_init (&guitar->strings[4], 246.941651); /* B */
  wave_guide_init (&guitar->strings[5], 391.995436); /* G */
  wave_guide_init (&guitar->body, 50.0);
  
  dav_guitar_update_locals (guitar);
}

static void
dav_guitar_do_destroy (BseObject *object)
{
  DavGuitar *guitar;
  int i;
  
  guitar = DAV_GUITAR (object);
  
  for (i = 0; i < 6; i++)
    wave_guide_free (&guitar->strings[i]);
  
  wave_guide_free (&guitar->body);
  
  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
dav_guitar_trigger_string (DavGuitar *guitar, int str)
{
  g_return_if_fail (DAV_IS_GUITAR (guitar));
  
  wave_guide_pluck (&guitar->strings[str], guitar->metallic_factor, guitar->snap_factor, guitar->trigger_vel);
  wave_guide_unstop (&guitar->body);
}

static void
dav_guitar_trigger (DavGuitar *guitar)
{
  guint i;
  
  for (i = 0; i < 6; i++)
    wave_guide_pluck (&guitar->strings[i], guitar->metallic_factor, guitar->snap_factor, guitar->trigger_vel);
  
  wave_guide_unstop (&guitar->body);
}

static void
dav_guitar_stop (DavGuitar *guitar)
{
  guint i;
  
  for (i = 0; i < 6; i++)
    wave_guide_stop (&guitar->strings[i]);
  
  wave_guide_stop (&guitar->body);
}

static void
dav_guitar_set_property (DavGuitar   *guitar,
			 guint        param_id,
			 GValue      *value,
			 GParamSpec  *pspec)
{
  switch (param_id)
    {
    case PARAM_FREQ_1:
      wave_guide_set_freq (&guitar->strings[0], g_value_get_float (value));
      bse_object_param_changed (BSE_OBJECT (guitar), "note_1");
      break;
    case PARAM_NOTE_1:
      wave_guide_set_freq (&guitar->strings[0], bse_note_to_freq (bse_value_get_note (value)));
      bse_object_param_changed (BSE_OBJECT (guitar), "freq_1");
      break;
    case PARAM_TRIGGER_1:
      dav_guitar_trigger_string (guitar, 0);
      break;
    case PARAM_FREQ_2:
      wave_guide_set_freq (&guitar->strings[1], g_value_get_float (value));
      bse_object_param_changed (BSE_OBJECT (guitar), "note_2");
      break;
    case PARAM_NOTE_2:
      wave_guide_set_freq (&guitar->strings[1], bse_note_to_freq (bse_value_get_note (value)));
      bse_object_param_changed (BSE_OBJECT (guitar), "freq_2");
      break;
    case PARAM_TRIGGER_2:
      dav_guitar_trigger_string (guitar, 1);
      break;
    case PARAM_FREQ_3:
      wave_guide_set_freq (&guitar->strings[2], g_value_get_float (value));
      bse_object_param_changed (BSE_OBJECT (guitar), "note_3");
      break;
    case PARAM_NOTE_3:
      wave_guide_set_freq (&guitar->strings[2], bse_note_to_freq (bse_value_get_note (value)));
      bse_object_param_changed (BSE_OBJECT (guitar), "freq_3");
      break;
    case PARAM_TRIGGER_3:
      dav_guitar_trigger_string (guitar, 2);
      break;
    case PARAM_FREQ_4:
      wave_guide_set_freq (&guitar->strings[3], g_value_get_float (value));
      bse_object_param_changed (BSE_OBJECT (guitar), "note_4");
      break;
    case PARAM_NOTE_4:
      wave_guide_set_freq (&guitar->strings[3], bse_note_to_freq (bse_value_get_note (value)));
      bse_object_param_changed (BSE_OBJECT (guitar), "freq_4");
      break;
    case PARAM_TRIGGER_4:
      dav_guitar_trigger_string (guitar, 3);
      break;
    case PARAM_FREQ_5:
      wave_guide_set_freq (&guitar->strings[4], g_value_get_float (value));
      bse_object_param_changed (BSE_OBJECT (guitar), "note_5");
      break;
    case PARAM_NOTE_5:
      wave_guide_set_freq (&guitar->strings[4], bse_note_to_freq (bse_value_get_note (value)));
      bse_object_param_changed (BSE_OBJECT (guitar), "freq_5");
      break;
    case PARAM_TRIGGER_5:
      dav_guitar_trigger_string (guitar, 4);
      break;
    case PARAM_FREQ_6:
      wave_guide_set_freq (&guitar->strings[5], g_value_get_float (value));
      bse_object_param_changed (BSE_OBJECT (guitar), "note_6");
      break;
    case PARAM_NOTE_6:
      wave_guide_set_freq (&guitar->strings[5], bse_note_to_freq (bse_value_get_note (value)));
      bse_object_param_changed (BSE_OBJECT (guitar), "freq_6");
      break;
    case PARAM_TRIGGER_6:
      dav_guitar_trigger_string (guitar, 5);
      break;
    case PARAM_TRIGGER_VEL:
      guitar->trigger_vel = g_value_get_float (value) / 100.0;
      break;
    case PARAM_TRIGGER_ALL:
      dav_guitar_trigger (guitar);
      break;
    case PARAM_STOP_ALL:
      dav_guitar_stop (guitar);
      break;
    case PARAM_METALLIC_FACTOR:
      guitar->metallic_factor = g_value_get_float (value) / 100.0;
      break;
    case PARAM_SNAP_FACTOR:
      guitar->snap_factor = g_value_get_float (value) / 100.0;
      break;
      
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (guitar, param_id, pspec);
      break;
    }
}

static void
dav_guitar_get_property (DavGuitar   *guitar,
			 guint        param_id,
			 GValue      *value,
			 GParamSpec  *pspec)
{
  switch (param_id)
    {
    case PARAM_FREQ_1:
      g_value_set_float (value, guitar->strings[0].freq);
      break;
    case PARAM_NOTE_1:
      bse_value_set_note (value, bse_note_from_freq (guitar->strings[0].freq));
      break;
    case PARAM_TRIGGER_1:
      g_value_set_boolean (value, FALSE);
      break;
    case PARAM_FREQ_2:
      g_value_set_float (value, guitar->strings[1].freq);
      break;
    case PARAM_NOTE_2:
      bse_value_set_note (value, bse_note_from_freq (guitar->strings[1].freq));
      break;
    case PARAM_TRIGGER_2:
      g_value_set_boolean (value, FALSE);
      break;
    case PARAM_FREQ_3:
      g_value_set_float (value, guitar->strings[2].freq);
      break;
    case PARAM_NOTE_3:
      bse_value_set_note (value, bse_note_from_freq (guitar->strings[2].freq));
      break;
    case PARAM_TRIGGER_3:
      g_value_set_boolean (value, FALSE);
      break;
    case PARAM_FREQ_4:
      g_value_set_float (value, guitar->strings[3].freq);
      break;
    case PARAM_NOTE_4:
      bse_value_set_note (value, bse_note_from_freq (guitar->strings[3].freq));
      break;
    case PARAM_TRIGGER_4:
      g_value_set_boolean (value, FALSE);
      break;
    case PARAM_FREQ_5:
      g_value_set_float (value, guitar->strings[4].freq);
      break;
    case PARAM_NOTE_5:
      bse_value_set_note (value, bse_note_from_freq (guitar->strings[4].freq));
      break;
    case PARAM_TRIGGER_5:
      g_value_set_boolean (value, FALSE);
      break;
    case PARAM_FREQ_6:
      g_value_set_float (value, guitar->strings[5].freq);
      break;
    case PARAM_NOTE_6:
      bse_value_set_note (value, bse_note_from_freq (guitar->strings[5].freq));
      break;
    case PARAM_TRIGGER_6:
      g_value_set_boolean (value, FALSE);
      break;
    case PARAM_TRIGGER_VEL:
      g_value_set_float (value, guitar->trigger_vel * 100.0);
      break;
    case PARAM_TRIGGER_ALL:
      g_value_set_boolean (value, FALSE);
      break;
    case PARAM_STOP_ALL:
      g_value_set_boolean (value, FALSE);
      break;
    case PARAM_METALLIC_FACTOR:
      g_value_set_float (value, guitar->metallic_factor * 100.0);
      break;
    case PARAM_SNAP_FACTOR:
      g_value_set_float (value, guitar->snap_factor * 100.0);
      break;
      
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (guitar, param_id, pspec);
      break;
    }
}

static void
dav_guitar_prepare (BseSource *source,
		    BseIndex index)
{
  DavGuitar *guitar = DAV_GUITAR (source);
  int i;
  
  guitar->hipass_coeff = pow (0.5, 20.0 / BSE_MIX_FREQ_f);
  guitar->hipass_data = 0.0;
  
  wave_guide_prepare (&guitar->body);
  
  for (i = 0; i < 6; i++)
    {
      wave_guide_prepare (&guitar->strings[i]);
      guitar->body_taps[i] = ((i + 1) * (guitar->body.wavelen - 1)) / 7;
    }
  
  dav_guitar_update_locals (guitar);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source, index);
}

static inline void
resonate (DavGuitar *guitar, float factor, int n)
{
  int body_pos;
  float vel;
  WaveGuide *body, *string;
  
  body = &guitar->body;
  string = &guitar->strings[n];
  
  /* Find position on body. */
  body_pos = wave_guide_get_pos (body, guitar->body_taps[n]);
  
  /* Linearly approximate the force of an imaginary
     spring connected between those two points. */
  vel = (string->data[string->pos] - body->data[body_pos]) * factor;
  
  /* Modify positions. */
  string->data[string->pos] -= vel;
  body->data[body_pos] += vel;
}

static BseChunk*
dav_guitar_calc_chunk (BseSource *source,
		       guint ochannel_id)
{
  DavGuitar *guitar = DAV_GUITAR (source);
  BseSampleValue *hunk;
  gfloat sample;
  guint i, j;
  
  g_return_val_if_fail (ochannel_id == DAV_GUITAR_OCHANNEL_MONO, NULL);
  
  hunk = bse_hunk_alloc (1);
  
  for (i = 0; i < BSE_TRACK_LENGTH; i++)
    {
      /* Get sample from body and hipass it to remove DC offset. */
      sample = guitar->body.data[guitar->body.pos] * 25.0F;
      guitar->hipass_data = guitar->hipass_data * guitar->hipass_coeff + sample * (1.0F - guitar->hipass_coeff);
      
      /* Store sample. */
      hunk[i] = BSE_CLIP_SAMPLE_VALUE ((sample - guitar->hipass_data) * BSE_MAX_SAMPLE_VALUE_f);
      
      /* Resonate strings + body and advance waves one position. */
      for (j = 0; j < 6; j++)
	{
	  resonate (guitar, 0.01, j);
	  wave_guide_advance (&guitar->strings[j]);
	}
      
      wave_guide_advance (&guitar->body);
    }
  
  return bse_chunk_new_orphan (1, hunk);
}

static void
dav_guitar_reset (BseSource *source)
{
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}

/* --- Export to DAV --- */
#include "./icons/noicon.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_guitar, "DavGuitar", "BseSource",
    "DavGuitar is a physical modelling acoustic guitar synthesizer - "
    "Any commercial use of this module requires a license from Stanford University",
    &type_info_guitar,
    "/Modules/Guitar",
    { NOICON_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      NOICON_WIDTH, NOICON_HEIGHT,
      NOICON_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;
