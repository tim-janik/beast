/* BseMult - BSE Multiplier
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
#include "bsemult.h"

#include <bse/bsechunk.h>
#include <bse/bsehunkmixer.h>


/* --- prototypes --- */
static void	 bse_mult_init			(BseMult	*mult);
static void	 bse_mult_class_init		(BseMultClass	*class);
static void	 bse_mult_class_destroy		(BseMultClass	*class);
static void	 bse_mult_do_shutdown		(BseObject     	*object);
static void      bse_mult_prepare               (BseSource      *source,
						 BseIndex        index);
static BseChunk* bse_mult_calc_chunk            (BseSource      *source,
						 guint           ochannel_id);
static void      bse_mult_reset                 (BseSource      *source);


/* --- variables --- */
static GType             type_id_mult = 0;
static gpointer          parent_class = NULL;
static const GTypeInfo type_info_mult = {
  sizeof (BseMultClass),
  
  (GBaseInitFunc) NULL,
  (GBaseDestroyFunc) NULL,
  (GClassInitFunc) bse_mult_class_init,
  (GClassDestroyFunc) bse_mult_class_destroy,
  NULL /* class_data */,
  
  sizeof (BseMult),
  0 /* n_preallocs */,
  (GInstanceInitFunc) bse_mult_init,
};


/* --- functions --- */
static void
bse_mult_class_init (BseMultClass *class)
{
  BseObjectClass *object_class;
  BseSourceClass *source_class;
  guint ichannel_id, ochannel_id;
  
  parent_class = g_type_class_peek (BSE_TYPE_SOURCE);
  object_class = BSE_OBJECT_CLASS (class);
  source_class = BSE_SOURCE_CLASS (class);
  
  object_class->shutdown = bse_mult_do_shutdown;
  
  source_class->prepare = bse_mult_prepare;
  source_class->calc_chunk = bse_mult_calc_chunk;
  source_class->reset = bse_mult_reset;
  
  ichannel_id = bse_source_class_add_ichannel (source_class, "mono_in1", "Mono Input 1", 1, 1);
  g_assert (ichannel_id == BSE_MULT_ICHANNEL_MONO1);
  ichannel_id = bse_source_class_add_ichannel (source_class, "mono_in2", "Mono Input 2", 1, 1);
  g_assert (ichannel_id == BSE_MULT_ICHANNEL_MONO2);
  ichannel_id = bse_source_class_add_ichannel (source_class, "mono_in3", "Mono Input 3", 1, 1);
  g_assert (ichannel_id == BSE_MULT_ICHANNEL_MONO3);
  ichannel_id = bse_source_class_add_ichannel (source_class, "mono_in4", "Mono Input 4", 1, 1);
  g_assert (ichannel_id == BSE_MULT_ICHANNEL_MONO4);
  ochannel_id = bse_source_class_add_ochannel (source_class, "mono_out", "Mono Output", 1);
  g_assert (ochannel_id == BSE_MULT_OCHANNEL_MONO);
}

static void
bse_mult_class_destroy (BseMultClass *class)
{
}

static void
bse_mult_init (BseMult *mult)
{
}

static void
bse_mult_do_shutdown (BseObject *object)
{
  BseMult *mult;
  
  mult = BSE_MULT (object);
  
  /* chain parent class' shutdown handler */
  BSE_OBJECT_CLASS (parent_class)->shutdown (object);
}

static void
bse_mult_prepare (BseSource *source,
		  BseIndex   index)
{
  BseMult *mult;

  mult = BSE_MULT (source);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source, index);
}

static BseChunk*
bse_mult_calc_chunk (BseSource *source,
		     guint      ochannel_id)
{
  BseSampleValue *hunk, *bound;
  guint c;
  
  
  g_return_val_if_fail (ochannel_id == BSE_MULT_OCHANNEL_MONO, NULL);

  if (source->n_inputs == 0)
    return bse_chunk_new_static_zero (1);
  else if (source->n_inputs == 1)
    return bse_source_ref_chunk (source->inputs[0].osource, source->inputs[0].ochannel_id, source->index);

  hunk = bse_hunk_alloc (1);
  bound = hunk + BSE_TRACK_LENGTH;
  for (c = BSE_MULT_ICHANNEL_MONO1; c <= BSE_MULT_ICHANNEL_MONO4; c++)
    {
      BseSourceInput *input = bse_source_get_input (source, c);

      if (input)
	{
	  BseChunk *chunk = bse_source_ref_chunk (input->osource, input->ochannel_id, source->index);

	  bse_hunk_mix (1, hunk, NULL, chunk->n_tracks, bse_chunk_complete_hunk (chunk));

	  bse_chunk_unref (chunk);
	  break;
	}
    }
  for (c = c + 1; c <= BSE_MULT_ICHANNEL_MONO4; c++)
    {
      BseSourceInput *input = bse_source_get_input (source, c);

      if (input)
	{
	  BseChunk *chunk = bse_source_ref_chunk (input->osource, input->ochannel_id, source->index);
	  BseSampleValue *d, *s = bse_chunk_complete_hunk (chunk);

	  d = hunk;
	  do
	    {
	      BseMixValue mv = *(s++);
	      
	      mv *= *d;
	      mv >>= 15;
	      if (mv >= -BSE_MAX_SAMPLE_VALUE)
		*(d++) = mv;
	      else
		*(d++) = BSE_MAX_SAMPLE_VALUE;
	    }
	  while (d < bound);

	  bse_chunk_unref (chunk);
	}
    }

  return bse_chunk_new_orphan (1, hunk);
}

static void
bse_mult_reset (BseSource *source)
{
  BseMult *mult;

  mult = BSE_MULT (source);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}


/* --- Export to BSE --- */
#include "./icons/prod.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_mult, "BseMult", "BseSource",
    "BseMult is a channel multiplier to fold incoming signals",
    &type_info_mult,
    "/Source/Mult",
    { PROD_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      PROD_IMAGE_WIDTH, PROD_IMAGE_HEIGHT,
      PROD_IMAGE_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;
