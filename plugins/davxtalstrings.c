/* DavXtalStrings - DAV Physical Modelling String Synthesizer
 * Copyright (c) 2000 David A. Bartold
 *
 * Generate a string pluck sound using a modified Karplus-Strong algorithm
 * and then use Brensenham's algorithm to correct the frequency.
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

#include <stdlib.h>
#include <bse/bsechunk.h>
#include <bse/bsehunkmixer.h>
#include "davxtalstrings.h"

/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_BASE_FREQ,
  PARAM_BASE_NOTE,
  PARAM_TRIGGER_HIT,
  PARAM_TRIGGER_VEL,
  PARAM_NOTE_DECAY,
  PARAM_TENSION_DECAY,
  PARAM_METALLIC_FACTOR,
  PARAM_SNAP_FACTOR
};


/* --- prototypes --- */
static void        dav_xtal_strings_init             (DavXtalStrings       *strings);
static void        dav_xtal_strings_class_init       (DavXtalStringsClass  *class);
static void        dav_xtal_strings_class_destroy    (DavXtalStringsClass  *class);
static void        dav_xtal_strings_do_shutdown      (BseObject        *object);
static void        dav_xtal_strings_set_param        (DavXtalStrings       *strings,
						  BseParam         *param,
						  guint             param_id);
static void        dav_xtal_strings_get_param        (DavXtalStrings       *strings,
						  BseParam         *param,
						  guint             param_id);
static void        dav_xtal_strings_prepare          (BseSource        *source,
						  BseIndex          index);
static BseChunk*   dav_xtal_strings_calc_chunk       (BseSource        *source,
						  guint             ochannel_id);
static void        dav_xtal_strings_reset            (BseSource        *source);
static inline void dav_xtal_strings_update_locals    (DavXtalStrings   *strings);


/* --- variables --- */
static GType             type_id_xtal_strings = 0;
static gpointer          parent_class = NULL;
static const GTypeInfo type_info_xtal_strings = {
  sizeof (DavXtalStringsClass),
  
  (GBaseInitFunc) NULL,
  (GBaseDestroyFunc) NULL,
  (GClassInitFunc) dav_xtal_strings_class_init,
  (GClassDestroyFunc) dav_xtal_strings_class_destroy,
  NULL /* class_data */,
  
  sizeof (DavXtalStrings),
  0 /* n_preallocs */,
  (GInstanceInitFunc) dav_xtal_strings_init,
};


/* --- functions --- */

static inline void
dav_xtal_strings_update_locals (DavXtalStrings *strings)
{
}

static void
dav_xtal_strings_class_init (DavXtalStringsClass *class)
{
  BseObjectClass *object_class;
  BseSourceClass *source_class;
  guint ochannel_id;
  
  parent_class = g_type_class_peek (BSE_TYPE_SOURCE);
  object_class = BSE_OBJECT_CLASS (class);
  source_class = BSE_SOURCE_CLASS (class);
  
  object_class->set_param = (BseObjectSetParamFunc) dav_xtal_strings_set_param;
  object_class->get_param = (BseObjectGetParamFunc) dav_xtal_strings_get_param;
  object_class->shutdown = dav_xtal_strings_do_shutdown;
  
  source_class->prepare = dav_xtal_strings_prepare;
  source_class->calc_chunk = dav_xtal_strings_calc_chunk;
  source_class->reset = dav_xtal_strings_reset;
  
  bse_object_class_add_param (object_class, "Base Frequency", PARAM_BASE_FREQ,
			      bse_param_spec_float ("base_freq", "Frequency", NULL,
						    27.5, 2000.0,
						    0.1, 440.0,
						    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_DIAL));

  bse_object_class_add_param (object_class, "Base Frequency",
                              PARAM_BASE_NOTE,
                              bse_param_spec_note ("base_note", "Note", NULL,
                                                   BSE_NOTE_A(-3), BSE_NOTE_B (3),
                                                   1, BSE_NOTE_VOID, FALSE,
                                                   BSE_PARAM_GUI));
  
  bse_object_class_add_param (object_class, "Trigger", PARAM_TRIGGER_VEL,
			      bse_param_spec_float ("trigger_vel", "Trigger Velocity [%]",
						    "Set the velocity of the string pluck",
						    0.0, 100.0, 10.0, 100.0,
						    BSE_PARAM_GUI | BSE_PARAM_HINT_SCALE));

  bse_object_class_add_param (object_class, "Trigger", PARAM_TRIGGER_HIT,
			      bse_param_spec_bool ("trigger_pulse", "Trigger Hit", "Pluck the string",
						   FALSE, BSE_PARAM_GUI));

  bse_object_class_add_param (object_class, "Decay", PARAM_NOTE_DECAY,
			      bse_param_spec_float ("note_decay", "Note Decay",
						    "Set the 'half-life' of the note's decay in seconds",
						    0.001, 4.0, 0.0025, 0.4,
						    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_SCALE));

  bse_object_class_add_param (object_class, "Decay", PARAM_TENSION_DECAY,
			      bse_param_spec_float ("tension_decay", "Tension Decay",
						    "Set the tension of the string",
						    0.001, 1.0, 0.0025, 0.04,
						    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_SCALE));
  
  bse_object_class_add_param (object_class, "Flavour", PARAM_METALLIC_FACTOR,
			      bse_param_spec_float ("metallic_factor", "Metallic Factor [%]",
						    "Set the metallicness of the string",
						    0.0, 100.0, 0.25, 16.0,
						    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_SCALE));

  bse_object_class_add_param (object_class, "Flavour", PARAM_SNAP_FACTOR,
			      bse_param_spec_float ("snap_factor", "Snap Factor [%]",
						    "Set the snappiness of the string",
						    0.0, 100.0, 0.25, 34.0,
						    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_SCALE));

  ochannel_id = bse_source_class_add_ochannel (source_class, "mono_out", "XtalStringsOutput", 1);
  g_assert (ochannel_id == DAV_XTAL_STRINGS_OCHANNEL_MONO);
}

static void
dav_xtal_strings_class_destroy (DavXtalStringsClass *class)
{
}

static void
dav_xtal_strings_init (DavXtalStrings *strings)
{
  strings->freq = 440.0;
  strings->trigger_vel = 1.0;
  strings->note_decay = 0.4;
  strings->tension_decay = 0.04;
  strings->metallic_factor = 0.16;
  strings->snap_factor = 0.34;

  strings->string = NULL;
  strings->size = 1;
  strings->pos = 0;
  strings->count = 0;

  strings->a = 0.0;
  strings->damping_factor = 0.0;

  dav_xtal_strings_update_locals (strings);
}

static void
dav_xtal_strings_do_shutdown (BseObject *object)
{
  DavXtalStrings *strings;
  
  strings = DAV_XTAL_STRINGS (object);

  g_free (strings->string);
  strings->string = NULL;

  /* chain parent class' shutdown handler */
  BSE_OBJECT_CLASS (parent_class)->shutdown (object);
}

static gfloat inline
calc_factor (gfloat freq, gfloat t)
{
  return pow (0.5, 1.0 / (freq * t));
}

void
dav_xtal_strings_trigger (DavXtalStrings *strings)
{
  guint i;
  guint pivot;

  g_return_if_fail (DAV_IS_XTAL_STRINGS (strings));

  if (strings->string == NULL)
    return;

  strings->pos = 0;
  strings->count = 0;
  strings->size = (int) ((BSE_MIX_FREQ + strings->freq - 1) / strings->freq);

  strings->a = calc_factor (strings->freq, strings->tension_decay);
  strings->damping_factor = calc_factor (strings->freq, strings->note_decay);

  /* Create envelope. */
  pivot = strings->size / 5;

  for (i = 0; i <= pivot; i++)
    strings->string[i] = ((float) i) / pivot;

  for (; i < strings->size; i++)
    strings->string[i] = ((float) (strings->size - i - 1)) / (strings->size - pivot - 1);

  /* Add some snap. */
  for (i = 0; i < strings->size; i++)
    strings->string[i] = pow (strings->string[i], strings->snap_factor * 10.0F + 1.0F);

  /* Add static to displacements. */
  for (i = 0; i < strings->size; i++)
    strings->string[i] = strings->string[i] * (1.0F - strings->metallic_factor) +
                         ((rand() & 1) ? -1.0F : 1.0F) * strings->metallic_factor;

  /* Set velocity. */
  for (i = 0; i < strings->size; i++)
    strings->string[i] *= strings->trigger_vel;
}

static void
dav_xtal_strings_set_param (DavXtalStrings *strings,
			BseParam   *param,
			guint       param_id)
{
  switch (param_id)
    {
    case PARAM_BASE_FREQ:
      strings->freq = param->value.v_float;
      bse_object_param_changed (BSE_OBJECT (strings), "base_note");
      break;
    case PARAM_BASE_NOTE:
      strings->freq = bse_note_to_freq (param->value.v_note);
      bse_object_param_changed (BSE_OBJECT (strings), "base_freq");
      break;
    case PARAM_TRIGGER_HIT:
      dav_xtal_strings_trigger (strings);
      break;
    case PARAM_TRIGGER_VEL:
      strings->trigger_vel = param->value.v_float / 100.0;
      break;
    case PARAM_NOTE_DECAY:
      strings->note_decay = param->value.v_float;
      break;
    case PARAM_TENSION_DECAY:
      strings->tension_decay = param->value.v_float;
      break;
    case PARAM_METALLIC_FACTOR:
      strings->metallic_factor = param->value.v_float / 100.0;
      break;
    case PARAM_SNAP_FACTOR:
      strings->snap_factor = param->value.v_float / 100.0;
      break;

    default:
      BSE_UNHANDLED_PARAM_ID (strings, param, param_id);
      break;
    }
}

static void
dav_xtal_strings_get_param (DavXtalStrings *strings,
			BseParam   *param,
			guint       param_id)
{
  switch (param_id)
    {
    case PARAM_BASE_FREQ:
      param->value.v_float = strings->freq;
      break;
    case PARAM_BASE_NOTE:
      param->value.v_note = bse_note_from_freq (strings->freq);
      break;
    case PARAM_TRIGGER_HIT:
      param->value.v_bool = FALSE;
      break;
    case PARAM_TRIGGER_VEL:
      param->value.v_float = strings->trigger_vel * 100.0;
      break;
    case PARAM_NOTE_DECAY:
      param->value.v_float = strings->note_decay;
      break;
    case PARAM_TENSION_DECAY:
      param->value.v_float = strings->tension_decay;
      break;
    case PARAM_METALLIC_FACTOR:
      param->value.v_float = strings->metallic_factor * 100.0;
      break;
    case PARAM_SNAP_FACTOR:
      param->value.v_float = strings->snap_factor * 100.0;
      break;
      
    default:
      BSE_UNHANDLED_PARAM_ID (strings, param, param_id);
      break;
    }
}

static void
dav_xtal_strings_prepare (BseSource *source,
		      BseIndex index)
{
  DavXtalStrings *strings = DAV_XTAL_STRINGS (source);

  g_free (strings->string);
  strings->string = g_new (gfloat, (BSE_MIX_FREQ + 19) / 20);
  strings->size = 1;
  strings->string[0] = 0.0;

  dav_xtal_strings_update_locals (strings);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source, index);
}

static BseChunk*
dav_xtal_strings_calc_chunk (BseSource *source,
			 guint ochannel_id)
{
  DavXtalStrings *strings = DAV_XTAL_STRINGS (source);
  BseSampleValue *hunk;
  guint i;
  gint32 pos2;
  gfloat sample;
  guint real_freq_256, actual_freq_256;
  
  g_return_val_if_fail (ochannel_id == DAV_XTAL_STRINGS_OCHANNEL_MONO, NULL);

  hunk = bse_hunk_alloc (1);
  
  real_freq_256 = (int) (strings->freq * 256);
  actual_freq_256 = (int) (BSE_MIX_FREQ_f * 256 / strings->size);

  for (i = 0; i < BSE_TRACK_LENGTH; i++)
    {
      /* Get next position. */
      pos2 = strings->pos + 1;
      if (pos2 >= strings->size)
        pos2 = 0;

      /* Linearly interpolate sample. */
      sample = strings->string[strings->pos] * (1.0 - (((float) strings->count) / actual_freq_256));
      sample += strings->string[pos2] * (((float) strings->count) / actual_freq_256);

      /* Store sample. */
      hunk[i] = BSE_CLIP_SAMPLE_VALUE (sample * BSE_MAX_SAMPLE_VALUE_f);

      /* Use Bresenham's algorithm to advance to the next position. */
      strings->count += real_freq_256;
      while (strings->count >= actual_freq_256)
        {
          strings->d = ((strings->d * (1.0 - strings->a)) + (strings->string[strings->pos] * strings->a)) * strings->damping_factor;
          strings->string[strings->pos] = strings->d;

          strings->pos++;
          if (strings->pos >= strings->size)
            strings->pos = 0;

          strings->count -= actual_freq_256;
        }
    }

  return bse_chunk_new_orphan (1, hunk);
}

static void
dav_xtal_strings_reset (BseSource *source)
{
  /* DavXtalStrings *strings = DAV_XTAL_STRINGS (source); */

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}

/* --- Export to DAV --- */
#include "./icons/noicon.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_xtal_strings, "DavXtalStrings", "BseSource",
    "DavXtalStrings is a string synthesizer",
    &type_info_xtal_strings,
    "/Source/XtalStrings",
    { NOICON_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      NOICON_WIDTH, NOICON_HEIGHT,
      NOICON_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;
