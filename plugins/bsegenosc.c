/* BseGenOsc - BSE Generic Oscillator
 * Copyright (C) 1999 Tim Janik
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
#include "bsegenosc.h"

#include <bse/bsechunk.h>

/* include generated enums
 */
#include "bsegenosc.enums"


/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_WAVE_FORM,
  PARAM_SINE,
  PARAM_GSAW,
  PARAM_SSAW,
  PARAM_PULSE,
  PARAM_TRIANGLE,
  PARAM_PHASE,
  PARAM_BASE_FREQ,
  PARAM_FM_PERC,
  PARAM_BASE_NOTE,
  PARAM_SELF_MODULATION,
  PARAM_SELF_PERC
};


/* --- prototypes --- */
static void	   bse_gen_osc_init	        (BseGenOsc	*gen_osc);
static void	   bse_gen_osc_class_init       (BseGenOscClass	*class);
static void	   bse_gen_osc_class_destroy    (BseGenOscClass	*class);
static void	   bse_gen_osc_do_shutdown      (BseObject     	*object);
static void        bse_gen_osc_set_param        (BseGenOsc	*gen_osc,
						 BseParam       *param,
						 guint           param_id);
static void        bse_gen_osc_get_param        (BseGenOsc	*gen_osc,
						 BseParam       *param,
						 guint           param_id);
static void        bse_gen_osc_prepare          (BseSource      *source,
						 BseIndex        index);
static BseChunk*   bse_gen_osc_calc_chunk       (BseSource      *source,
						 guint           ochannel_id);
static void        bse_gen_osc_reset            (BseSource      *source);
static inline void bse_gen_osc_update_locals	(BseGenOsc      *gosc);


/* --- variables --- */
static GType             type_id_gen_osc = 0;
static gpointer          parent_class = NULL;
static const GTypeInfo type_info_gen_osc = {
  sizeof (BseGenOscClass),
  
  (GBaseInitFunc) NULL,
  (GBaseDestroyFunc) NULL,
  (GClassInitFunc) bse_gen_osc_class_init,
  (GClassDestroyFunc) bse_gen_osc_class_destroy,
  NULL /* class_data */,
  
  sizeof (BseGenOsc),
  0 /* n_preallocs */,
  (GInstanceInitFunc) bse_gen_osc_init,
};


/* --- functions --- */
static void
bse_gen_osc_class_init (BseGenOscClass *class)
{
  BseObjectClass *object_class;
  BseSourceClass *source_class;
  guint ochannel_id, ichannel_id;

  parent_class = g_type_class_peek (BSE_TYPE_SOURCE);
  object_class = BSE_OBJECT_CLASS (class);
  source_class = BSE_SOURCE_CLASS (class);

  object_class->set_param = (BseObjectSetParamFunc) bse_gen_osc_set_param;
  object_class->get_param = (BseObjectGetParamFunc) bse_gen_osc_get_param;
  object_class->shutdown = bse_gen_osc_do_shutdown;

  source_class->prepare = bse_gen_osc_prepare;
  source_class->calc_chunk = bse_gen_osc_calc_chunk;
  source_class->reset = bse_gen_osc_reset;

  class->ref_count = 0;
  class->sine_table_size = 0;
  class->sine_table = NULL;
  class->gsaw_table_size = 0;
  class->gsaw_table = NULL;
  class->ssaw_table_size = 0;
  class->ssaw_table = NULL;
  class->pulse_table_size = 0;
  class->pulse_table = NULL;
  class->triangle_table_size = 0;
  class->triangle_table = NULL;

  bse_object_class_add_param (object_class, "Wave Form",
			      PARAM_WAVE_FORM,
			      bse_param_spec_enum ("wave_form", "Wave", "Oscillator wave form",
						   BSE_TYPE_GEN_OSC_WAVE_TYPE,
						   BSE_GEN_OSC_SINE,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Wave Form",
			      PARAM_PHASE,
                              bse_param_spec_float ("phase", "Phase", NULL,
						    -180.0, 180.0,
						    5.0,
						    0.0,
						    BSE_PARAM_DEFAULT |
						    BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Wave Form",
			      PARAM_SINE,
			      bse_param_spec_bool ("sine_table", "Sine Wave", NULL,
						   TRUE,
						   BSE_PARAM_GUI | BSE_PARAM_HINT_RADIO));
  bse_object_class_add_param (object_class, "Wave Form",
			      PARAM_PULSE,
			      bse_param_spec_bool ("pulse_table", "Pulse", NULL,
						   FALSE,
						   BSE_PARAM_GUI | BSE_PARAM_HINT_RADIO));
  bse_object_class_add_param (object_class, "Wave Form",
			      PARAM_GSAW,
			      bse_param_spec_bool ("gsaw_table", "Growing Saw", NULL,
						   FALSE,
						   BSE_PARAM_GUI | BSE_PARAM_HINT_RADIO));
  bse_object_class_add_param (object_class, "Wave Form",
			      PARAM_SSAW,
			      bse_param_spec_bool ("ssaw_table", "Shrinking Saw", NULL,
						   FALSE,
						   BSE_PARAM_GUI | BSE_PARAM_HINT_RADIO));
  bse_object_class_add_param (object_class, "Wave Form",
			      PARAM_TRIANGLE,
			      bse_param_spec_bool ("triangle_table", "Triangle", NULL,
						   FALSE,
						   BSE_PARAM_GUI | BSE_PARAM_HINT_RADIO));
  bse_object_class_add_param (object_class, "Base Frequency",
			      PARAM_BASE_FREQ,
                              bse_param_spec_float ("base_freq", "Frequency", NULL,
						    BSE_MIN_OSC_FREQ_d, BSE_MAX_OSC_FREQ_d,
						    10.0,
						    BSE_KAMMER_FREQ,
						    BSE_PARAM_DEFAULT |
						    BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Base Frequency",
			      PARAM_BASE_NOTE,
                              bse_param_spec_note ("base_note", "Note", NULL,
						   BSE_MIN_NOTE, BSE_MAX_NOTE,
						   1, BSE_KAMMER_NOTE, TRUE,
						   BSE_PARAM_GUI));
  bse_object_class_add_param (object_class, "Modulation",
			      PARAM_FM_PERC,
                              bse_param_spec_float ("fm_perc", "Input Modulation [%]", "Modulation Strength for input channel",
						    0, 100.0,
						    5.0,
						    10.0,
						    BSE_PARAM_DEFAULT |
						    BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Modulation",
			      PARAM_SELF_MODULATION,
			      bse_param_spec_bool ("self_modulation", "Self Modulation", "Modulate oscillator with itself",
						   FALSE,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Modulation",
			      PARAM_SELF_PERC,
                              bse_param_spec_float ("self_perc", "Self Modulation [%]", "Modualtion Strength for self modulation",
						    0, 100.0,
						    5.0,
						    10.0,
						    BSE_PARAM_DEFAULT |
						    BSE_PARAM_HINT_SCALE));
  
  ochannel_id = bse_source_class_add_ochannel (source_class, "mono_out", "Mono Oscillated Output", 1);
  g_assert (ochannel_id == BSE_GEN_OSC_OCHANNEL_MONO);
  ichannel_id = bse_source_class_add_ichannel (source_class, "freq_mod_in", "Mono Frequency Modulation Input", 1, 1);
  g_assert (ichannel_id == BSE_GEN_OSC_ICHANNEL_FREQ_MOD);
}

static void
bse_gen_osc_class_destroy (BseGenOscClass *class)
{
}

static void
bse_gen_osc_class_ref_tables (BseGenOscClass *class)
{
  guint max_table_size;
  gdouble table_size;
  BseSampleValue *table;
  guint i;
  
  class->ref_count += 1;
  if (class->ref_count > 1)
    return;

  /* express assertments the code makes */
  max_table_size = BSE_MIX_FREQ / 2;
  g_assert (max_table_size + 2 * BSE_MAX_OSC_FREQ_d < 65535);

  /* sine */
  class->sine_table_size = max_table_size;
  table_size = class->sine_table_size;
  table = g_new (BseSampleValue, table_size);
  for (i = 0; i < table_size; i += 1)
    {
      gdouble d = i;
      
      table[i] = 0.5 + sin (d * (360.0 / table_size) / 180.0 * PI) * BSE_MAX_SAMPLE_VALUE;
#if 0
      if (!(i%(guint)(table_size/100)))
	g_print ("sine_table[%06u]=%d\n", i, table[i]);
#endif
    }
  class->sine_table = table;

  /* gsaw */
  class->gsaw_table_size = 512;
  table_size = class->gsaw_table_size;
  table = g_new (BseSampleValue, table_size);
  for (i = 0; i < table_size; i++)
    {
      table[i] = 0.5 + (1.0 * i) / table_size * BSE_MAX_SAMPLE_VALUE;
#if 0
      if (!(i%(guint)(table_size/100)))
	g_print ("gsaw_table[%06u]=%d\n", i, table[i]);
#endif
    }
  class->gsaw_table = table;

  /* ssaw */
  class->ssaw_table_size = max_table_size;
  table_size = class->ssaw_table_size;
  table = g_new (BseSampleValue, table_size);
  for (i = 0; i < table_size; i++)
    {
      table[i] = 0.5 + (table_size - i) / table_size * BSE_MAX_SAMPLE_VALUE;
#if 0
      if (!(i%(guint)(table_size/100)))
	g_print ("ssaw_table[%06u]=%d\n", i, table[i]);
#endif
    }
  class->ssaw_table = table;

  /* pulse */
  class->pulse_table_size = 512;
  table_size = class->pulse_table_size;
  table = g_new (BseSampleValue, table_size);
  for (i = 0; i < table_size; i++)
    {
      table[i] = i < table_size / 2 ? BSE_MAX_SAMPLE_VALUE : BSE_MIN_SAMPLE_VALUE;
    }
  class->pulse_table = table;

  /* triangle */
  class->triangle_table_size = 512;
  table_size = class->triangle_table_size;
  table = g_new (BseSampleValue, table_size);
  for (i = 0; i < table_size; i++)
    {
      gdouble d = i * 2;

      if (d < table_size)
	table[i] = 0.5 + d / table_size * BSE_MAX_SAMPLE_VALUE;
      else
	{
	  d -= table_size;
	  table[i] = 0.5 + (table_size - d) / table_size * BSE_MAX_SAMPLE_VALUE;
	}
#if 0
      g_print ("triangle_table[%06u]=%d\n", i, table[i]);
#endif
    }
  class->triangle_table = table;
}

static void
bse_gen_osc_class_unref_tables (BseGenOscClass *class)
{
  class->ref_count -= 1;

  if (!class->ref_count)
    {
      class->sine_table_size = 0;
      g_free (class->sine_table);
      class->sine_table = NULL;
      class->gsaw_table_size = 0;
      g_free (class->gsaw_table);
      class->gsaw_table = NULL;
      class->ssaw_table_size = 0;
      g_free (class->ssaw_table);
      class->ssaw_table = NULL;
      class->pulse_table_size = 0;
      g_free (class->pulse_table);
      class->pulse_table = NULL;
      class->triangle_table_size = 0;
      g_free (class->triangle_table);
      class->triangle_table = NULL;
    }
}

static void
bse_gen_osc_init (BseGenOsc *gosc)
{
  gosc->wave = BSE_GEN_OSC_SINE;
  gosc->phase = 0.0;
  gosc->base_freq = BSE_KAMMER_FREQ;
  gosc->fm_perc = 10;
  gosc->self_modulation = FALSE;
  gosc->self_perc = 10;
  gosc->rate_pos = 0;
  gosc->rate = 0;
  gosc->fm_strength = 0;
  gosc->self_strength = 0;
  gosc->table_size = 1;
  gosc->table = NULL;
}

static void
bse_gen_osc_do_shutdown (BseObject *object)
{
  BseGenOsc *gen_osc;

  gen_osc = BSE_GEN_OSC (object);

  /* chain parent class' shutdown handler */
  BSE_OBJECT_CLASS (parent_class)->shutdown (object);
}

static inline void
bse_gen_osc_update_locals (BseGenOsc *gosc)
{
  BseGenOscClass *class = BSE_GEN_OSC_GET_CLASS (gosc);
  gdouble d;
  guint32 r;

  switch (BSE_SOURCE_PREPARED (gosc) ? gosc->wave : BSE_GEN_OSC_NOWAVE)
    {
    case BSE_GEN_OSC_SINE:
      gosc->table_size = class->sine_table_size;
      gosc->table = class->sine_table;
      break;
    case BSE_GEN_OSC_GSAW:
      gosc->table_size = class->gsaw_table_size;
      gosc->table = class->gsaw_table;
      break;
    case BSE_GEN_OSC_SSAW:
      gosc->table_size = class->ssaw_table_size;
      gosc->table = class->ssaw_table;
      break;
    case BSE_GEN_OSC_PULSE:
      gosc->table_size = class->pulse_table_size;
      gosc->table = class->pulse_table;
      break;
    case BSE_GEN_OSC_TRIANGLE:
      gosc->table_size = class->triangle_table_size;
      gosc->table = class->triangle_table;
      break;
    default:
      gosc->table_size = 1;
      gosc->table = NULL;
      break;
    }

  r = gosc->rate_pos;
  gosc->rate_pos = (gosc->phase + 360.0) / 360.0 * gosc->table_size + (r >> 16);
  gosc->rate_pos = (gosc->rate_pos % gosc->table_size) << 16;
  gosc->rate_pos |= r & 0xffff;
  d = gosc->table_size;
  d /= BSE_MIX_FREQ;
  d *= gosc->base_freq;
  gosc->rate = d;
  gosc->rate = (gosc->rate << 16) + (d - gosc->rate) * 65536.0;
  gosc->fm_strength = gosc->rate * (gosc->fm_perc / 100.0) / (BSE_MAX_SAMPLE_VALUE + 1);
  gosc->self_strength = gosc->rate * (gosc->self_perc / 100.0) / (BSE_MAX_SAMPLE_VALUE + 1);
}

static void
bse_gen_osc_set_param (BseGenOsc *gosc,
		       BseParam  *param,
		       guint      param_id)
{
  guint wave = 0;

  switch (param_id)
    {
    case PARAM_WAVE_FORM:
      gosc->wave = param->value.v_enum;
      bse_gen_osc_update_locals (gosc);
      bse_object_param_changed (BSE_OBJECT (gosc), "sine_table");
      bse_object_param_changed (BSE_OBJECT (gosc), "gsaw_table");
      bse_object_param_changed (BSE_OBJECT (gosc), "ssaw_table");
      bse_object_param_changed (BSE_OBJECT (gosc), "pulse_table");
      bse_object_param_changed (BSE_OBJECT (gosc), "triangle_table");
      break;
    case PARAM_TRIANGLE:
      wave++;
      /* fall through */
    case PARAM_SSAW:
      wave++;
      /* fall through */
    case PARAM_GSAW:
      wave++;
      /* fall through */
    case PARAM_PULSE:
      wave++;
      /* fall through */
    case PARAM_SINE:
      wave++;
      if (param->value.v_bool)
	{
	  gosc->wave = wave;
	  bse_gen_osc_update_locals (gosc);
	  bse_object_param_changed (BSE_OBJECT (gosc), "wave_form");
	  bse_object_param_changed (BSE_OBJECT (gosc), "sine_table");
	  bse_object_param_changed (BSE_OBJECT (gosc), "gsaw_table");
	  bse_object_param_changed (BSE_OBJECT (gosc), "ssaw_table");
	  bse_object_param_changed (BSE_OBJECT (gosc), "pulse_table");
	  bse_object_param_changed (BSE_OBJECT (gosc), "triangle_table");
	}
      break;
    case PARAM_PHASE:
      gosc->phase = param->value.v_float;
      bse_gen_osc_update_locals (gosc);
      break;
    case PARAM_BASE_NOTE:
      gosc->base_freq = bse_note_to_freq (param->value.v_note);
      gosc->base_freq = MAX (gosc->base_freq, BSE_MIN_OSC_FREQ_d);
      bse_gen_osc_update_locals (gosc);
      bse_object_param_changed (BSE_OBJECT (gosc), "base_freq");
      if (bse_note_from_freq (gosc->base_freq) != param->value.v_note)
	bse_object_param_changed (BSE_OBJECT (gosc), "base_note");
      break;
    case PARAM_BASE_FREQ:
      gosc->base_freq = param->value.v_float;
      bse_gen_osc_update_locals (gosc);
      bse_object_param_changed (BSE_OBJECT (gosc), "base_note");
      break;
    case PARAM_FM_PERC:
      gosc->fm_perc = param->value.v_float;
      bse_gen_osc_update_locals (gosc);
      break;
    case PARAM_SELF_MODULATION:
      gosc->self_modulation = param->value.v_bool;
      break;
    case PARAM_SELF_PERC:
      gosc->self_perc = param->value.v_float;
      bse_gen_osc_update_locals (gosc);
      break;
    default:
      BSE_UNHANDLED_PARAM_ID (gosc, param, param_id);
      break;
    }
}

static void
bse_gen_osc_get_param (BseGenOsc *gosc,
                       BseParam  *param,
		       guint      param_id)
{
  switch (param_id)
    {
    case PARAM_WAVE_FORM:
      param->value.v_enum = gosc->wave;
      break;
    case PARAM_SINE:
      param->value.v_bool = gosc->wave == BSE_GEN_OSC_SINE;
      break;
    case PARAM_GSAW:
      param->value.v_bool = gosc->wave == BSE_GEN_OSC_GSAW;
      break;
    case PARAM_SSAW:
      param->value.v_bool = gosc->wave == BSE_GEN_OSC_SSAW;
      break;
    case PARAM_PULSE:
      param->value.v_bool = gosc->wave == BSE_GEN_OSC_PULSE;
      break;
    case PARAM_TRIANGLE:
      param->value.v_bool = gosc->wave == BSE_GEN_OSC_TRIANGLE;
      break;
    case PARAM_BASE_NOTE:
      param->value.v_note = bse_note_from_freq (gosc->base_freq);
      break;
    case PARAM_BASE_FREQ:
      param->value.v_float = gosc->base_freq;
      break;
    case PARAM_PHASE:
      param->value.v_float = gosc->phase;
      break;
    case PARAM_FM_PERC:
      param->value.v_float = gosc->fm_perc;
      break;
    case PARAM_SELF_MODULATION:
      param->value.v_bool = gosc->self_modulation;
      break;
    case PARAM_SELF_PERC:
      param->value.v_float = gosc->self_perc;
      break;
    default:
      BSE_UNHANDLED_PARAM_ID (gosc, param, param_id);
      break;
    }
}

void
bse_gen_osc_sync (BseGenOsc *gosc)
{
  g_return_if_fail (BSE_IS_GEN_OSC (gosc));

  gosc->rate_pos = 0x8000;
  bse_gen_osc_update_locals (gosc);
}

static void
bse_gen_osc_prepare (BseSource *source,
		     BseIndex   index)
{
  BseGenOsc *gosc = BSE_GEN_OSC (source);

  bse_gen_osc_class_ref_tables (BSE_GEN_OSC_GET_CLASS (gosc));

  bse_gen_osc_sync (gosc);

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source, index);
}

static BseChunk*
bse_gen_osc_calc_chunk (BseSource *source,
			guint      ochannel_id)
{
  BseGenOsc *gosc = BSE_GEN_OSC (source);
  BseSampleValue *table = gosc->table;
  BseChunk *fmchunk = NULL;
  BseSampleValue *hunk, *fmhunk = NULL;
  gfloat fm_strength = gosc->fm_strength;
  gfloat self_strength = gosc->self_strength;
  BseSourceInput *freq_mod;
  guint32 table_size, rate_pos, rate;
  guint i;
  
  g_return_val_if_fail (ochannel_id == BSE_GEN_OSC_OCHANNEL_MONO, NULL);

  freq_mod = bse_source_get_input (source, BSE_GEN_OSC_ICHANNEL_FREQ_MOD); /* mono */
  if (freq_mod)
    fmchunk = bse_source_ref_chunk (freq_mod->osource, freq_mod->ochannel_id, source->index);

  hunk = bse_hunk_alloc (1);

  table_size = gosc->table_size << 16;
  rate_pos = gosc->rate_pos % table_size;
  rate = gosc->rate % table_size;

  if (fmchunk && fmchunk->state_filled)
    rate += fm_strength * fmchunk->state[0];
  else if (fmchunk && fmchunk->hunk_filled)
    fmhunk = fmchunk->hunk;

  if (fmhunk && gosc->self_modulation)
    for (i = 0; i < BSE_TRACK_LENGTH; i++)
      {
	BseSampleValue v = table[rate_pos >> 16];
	
	hunk[i] = v;
	rate_pos += rate + fm_strength * fmhunk[i] + self_strength * v;
	while (rate_pos >= table_size)
	  rate_pos -= table_size;
      }
  else if (fmhunk)
    for (i = 0; i < BSE_TRACK_LENGTH; i++)
      {
	hunk[i] = table[rate_pos >> 16];
	rate_pos += rate + fm_strength * fmhunk[i];
	while (rate_pos >= table_size)
	  rate_pos -= table_size;
      }
  else if (gosc->self_modulation)
    for (i = 0; i < BSE_TRACK_LENGTH; i++)
      {
	BseSampleValue v = table[rate_pos >> 16];

	hunk[i] = v;
	rate_pos += rate + self_strength * v;
	while (rate_pos >= table_size)
	  rate_pos -= table_size;
      }
  else
    for (i = 0; i < BSE_TRACK_LENGTH; i++)
      {
	hunk[i] = table[rate_pos >> 16];
	rate_pos += rate;
	while (rate_pos >= table_size)
	  rate_pos -= table_size;
      }
  gosc->rate_pos = rate_pos;

  if (fmchunk)
    bse_chunk_unref (fmchunk);

  return bse_chunk_new_orphan (1, hunk);
}

static void
bse_gen_osc_reset (BseSource *source)
{
  BseGenOsc *gosc = BSE_GEN_OSC (source);

  gosc->rate_pos = 0;
  gosc->table_size = 1;
  gosc->table = NULL;
  bse_gen_osc_class_unref_tables (BSE_GEN_OSC_GET_CLASS (gosc));

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}


/* --- Export to BSE --- */
#include "./icons/sine.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_gen_osc, "BseGenOsc", "BseSource",
    "BseGenOsc is a generic oscillator source",
    &type_info_gen_osc,
    "/Source/GenOsc",
    { SINE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      SINE_WIDTH, SINE_HEIGHT,
      SINE_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORT_AND_GENERATE_ENUMS ();
BSE_EXPORTS_END;
