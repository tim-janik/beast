/* BseNoise - BSE Noise generator
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
#include "bsenoise.h"

#include <bse/bsechunk.h>
#include <stdlib.h>
#include <time.h>


/* --- prototypes --- */
static void	 bse_noise_init			(BseNoise	*noise);
static void	 bse_noise_class_init		(BseNoiseClass	*class);
static void	 bse_noise_class_destroy	(BseNoiseClass	*class);
static void	 bse_noise_do_shutdown		(BseObject     	*object);
static void      bse_noise_prepare              (BseSource      *source,
						 BseIndex        index);
static BseChunk* bse_noise_calc_chunk           (BseSource      *source,
						 guint           ochannel_id);
static void      bse_noise_reset                (BseSource      *source);


/* --- variables --- */
static BseType           type_id_noise = 0;
static gpointer          parent_class = NULL;
static const BseTypeInfo type_info_noise = {
  sizeof (BseNoiseClass),
  
  (BseBaseInitFunc) NULL,
  (BseBaseDestroyFunc) NULL,
  (BseClassInitFunc) bse_noise_class_init,
  (BseClassDestroyFunc) bse_noise_class_destroy,
  NULL /* class_data */,
  
  sizeof (BseSource),
  0 /* n_preallocs */,
  (BseObjectInitFunc) bse_noise_init,
};


/* --- functions --- */
static void
bse_noise_class_init (BseNoiseClass *class)
{
  BseObjectClass *object_class;
  BseSourceClass *source_class;
  guint ochannel_id;

  parent_class = bse_type_class_peek (BSE_TYPE_SOURCE);
  object_class = BSE_OBJECT_CLASS (class);
  source_class = BSE_SOURCE_CLASS (class);

  object_class->shutdown = bse_noise_do_shutdown;

  source_class->prepare = bse_noise_prepare;
  source_class->calc_chunk = bse_noise_calc_chunk;
  source_class->reset = bse_noise_reset;
  
  ochannel_id = bse_source_class_add_ochannel (source_class, "Noise", "Mono Noise Output", 1);
  g_assert (ochannel_id == BSE_NOISE_OCHANNEL_MONO);
}

static void
bse_noise_class_destroy (BseNoiseClass *class)
{
}

static void
bse_noise_init (BseNoise *noise)
{
}

static void
bse_noise_do_shutdown (BseObject *object)
{
  BseNoise *noise;

  noise = BSE_NOISE (object);

  /* chain parent class' shutdown handler */
  BSE_OBJECT_CLASS (parent_class)->shutdown (object);
}

#define N_STATIC_BLOCKS (17)

static void
bse_noise_prepare (BseSource *source,
		   BseIndex   index)
{
  BseNoise *noise = BSE_NOISE (source);
  guint i, l;

  l = 1 * BSE_TRACK_LENGTH * (N_STATIC_BLOCKS + 1);
  noise->static_noise = g_new (BseSampleValue, l);

  srand (time (NULL));
  for (i = 0; i < l; i++)
    noise->static_noise[i] = rand ();

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source, index);
}

static BseChunk*
bse_noise_calc_chunk (BseSource *source,
		      guint      ochannel_id)
{
  BseNoise *noise = BSE_NOISE (source);
  BseSampleValue *hunk;
  
  g_return_val_if_fail (ochannel_id == BSE_NOISE_OCHANNEL_MONO, NULL);

  hunk = noise->static_noise + 1 * (rand () % (BSE_TRACK_LENGTH * N_STATIC_BLOCKS));

  return bse_chunk_new_foreign (1, hunk, FALSE);
}

static void
bse_noise_reset (BseSource *source)
{
  BseNoise *noise = BSE_NOISE (source);

  g_free (noise->static_noise);
  noise->static_noise = NULL;

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}


/* --- Export to BSE --- */
#include "./icons/noicon.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_noise, "BseNoise", "BseSource",
    "BseNoise is a noise generator source",
    &type_info_noise,
    "/Source/Noise",
    { NOICON_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      NOICON_WIDTH, NOICON_HEIGHT,
      NOICON_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;
