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


/* --- prototypes --- */
static void	 bse_gen_osc_init		(BseGenOsc	*gen_osc);
static void	 bse_gen_osc_class_init		(BseGenOscClass	*class);
static void	 bse_gen_osc_class_destroy	(BseGenOscClass	*class);
static void	 bse_gen_osc_do_shutdown	(BseObject     	*object);
static void      bse_gen_osc_prepare            (BseSource      *source,
						 BseIndex        index);
static BseChunk* bse_gen_osc_calc_chunk         (BseSource      *source,
						 guint           ochannel_id);
static void      bse_gen_osc_reset              (BseSource      *source);


/* --- variables --- */
static BseType           type_id_gen_osc = 0;
static gpointer          parent_class = NULL;
static const BseTypeInfo type_info_gen_osc = {
  sizeof (BseGenOscClass),
  
  (BseBaseInitFunc) NULL,
  (BseBaseDestroyFunc) NULL,
  (BseClassInitFunc) bse_gen_osc_class_init,
  (BseClassDestroyFunc) bse_gen_osc_class_destroy,
  NULL /* class_data */,
  
  sizeof (BseGenOsc),
  0 /* n_preallocs */,
  (BseObjectInitFunc) bse_gen_osc_init,
};


/* --- functions --- */
static void
bse_gen_osc_class_init (BseGenOscClass *class)
{
  BseObjectClass *object_class;
  BseSourceClass *source_class;
  guint ochannel_id, ichannel_id;

  parent_class = bse_type_class_peek (BSE_TYPE_SOURCE);
  object_class = BSE_OBJECT_CLASS (class);
  source_class = BSE_SOURCE_CLASS (class);

  object_class->shutdown = bse_gen_osc_do_shutdown;

  source_class->prepare = bse_gen_osc_prepare;
  source_class->calc_chunk = bse_gen_osc_calc_chunk;
  source_class->reset = bse_gen_osc_reset;

  class->ref_count = 0;
  class->sine_table = NULL;
  
  ochannel_id = bse_source_class_add_ochannel (source_class, "OscOut", "Mono Oscillated Output", 1);
  g_assert (ochannel_id == BSE_GEN_OSC_OCHANNEL_MONO);
  ichannel_id = bse_source_class_add_ichannel (source_class, "FreqMod", "Mono Frequency Modulation Input", 1, 1);
  g_assert (ichannel_id == BSE_GEN_OSC_ICHANNEL_FREQ_MOD);
  ichannel_id = bse_source_class_add_ichannel (source_class, "AmpMod", "Mono Amplitude Modulation Input", 1, 1);
  g_assert (ichannel_id == BSE_GEN_OSC_ICHANNEL_AMP_MOD);
}

static void
bse_gen_osc_class_destroy (BseGenOscClass *class)
{
}
#include <fcntl.h>
static void
bse_gen_osc_class_ref_tables (BseGenOscClass *class)
{
  gdouble mix_freq = BSE_MIX_FREQ;
  BseSampleValue *table;
  guint i;
  
  class->ref_count += 1;
  if (class->ref_count > 1)
    return;

  table = g_new (BseSampleValue, mix_freq);

  for (i = 0; i < mix_freq; i += 1)
    {
      gdouble d = i;
      
      table[i] = 0.5 + sin (d * (360.0 / mix_freq) / 180.0 * PI) * BSE_MAX_SAMPLE_VALUE;
#if 0
      if (!(i%(guint)(mix_freq/100)))
	g_print ("sine_table[%06u]=%d\n", i, table[i]);
#endif
    }
  class->sine_table = table;
}

static void
bse_gen_osc_class_unref_tables (BseGenOscClass *class)
{
  class->ref_count -= 1;

  if (!class->ref_count)
    {
      g_free (class->sine_table);
      class->sine_table = NULL;
    }
}

static void
bse_gen_osc_init (BseGenOsc *gosc)
{
  gosc->table = NULL;
}

void
bse_gen_osc_sync (BseGenOsc *gosc)
{
  g_return_if_fail (BSE_IS_GEN_OSC (gosc));
}

static void
bse_gen_osc_do_shutdown (BseObject *object)
{
  BseGenOsc *gen_osc;

  gen_osc = BSE_GEN_OSC (object);

  /* chain parent class' shutdown handler */
  BSE_OBJECT_CLASS (parent_class)->shutdown (object);
}

static void
bse_gen_osc_prepare (BseSource *source,
		     BseIndex   index)
{
  BseGenOsc *gosc = BSE_GEN_OSC (source);
  BseGenOscClass *class = BSE_GEN_OSC_GET_CLASS (gosc);

  bse_gen_osc_class_ref_tables (BSE_GEN_OSC_GET_CLASS (gosc));
  gosc->table = class->sine_table;

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
  guint mix_freq = BSE_MIX_FREQ;
  BseIndex index = source->index - source->start - 1;
  BseSampleValue *hunk;
  guint i;
  guint phase_offset = 0;
  gdouble base_freq = BSE_KAMMER_FREQ_d;
  
  g_return_val_if_fail (ochannel_id == BSE_GEN_OSC_OCHANNEL_MONO, NULL);

  hunk = bse_hunk_alloc (1);
  index = index * BSE_TRACK_LENGTH * base_freq + 0.5;
  index += phase_offset;
  index %= mix_freq;
  for (i = 0; i < BSE_TRACK_LENGTH; i++)
    {
      BseIndex offset = index + base_freq * i + 0.5;

      hunk[i] = table[offset % mix_freq];
    }

  return bse_chunk_new_orphan (1, hunk);
}

static void
bse_gen_osc_reset (BseSource *source)
{
  BseGenOsc *gosc = BSE_GEN_OSC (source);

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
BSE_EXPORTS_END;
