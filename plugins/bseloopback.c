/* BseLoopback - BSE Recording output source
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
#include "bseloopback.h"

#include <bse/bsechunk.h>
#include <stdlib.h>
#include <time.h>


/* --- prototypes --- */
static void	 bse_loopback_init		(BseLoopback		*loopback);
static void	 bse_loopback_class_init	(BseLoopbackClass	*class);
static void	 bse_loopback_class_destroy	(BseLoopbackClass	*class);
static void	 bse_loopback_do_shutdown	(BseObject	     	*object);
static void      bse_loopback_prepare           (BseSource	        *source,
						 BseIndex 	         index);
static BseChunk* bse_loopback_calc_chunk        (BseSource	        *source,
						 guint    	         ochannel_id);
static void      bse_loopback_reset             (BseSource      	*source);


/* --- variables --- */
static BseType           type_id_loopback = 0;
static gpointer          parent_class = NULL;
static const BseTypeInfo type_info_loopback = {
  sizeof (BseLoopbackClass),
  
  (BseBaseInitFunc) NULL,
  (BseBaseDestroyFunc) NULL,
  (BseClassInitFunc) bse_loopback_class_init,
  (BseClassDestroyFunc) bse_loopback_class_destroy,
  NULL /* class_data */,
  
  sizeof (BseLoopback),
  0 /* n_preallocs */,
  (BseObjectInitFunc) bse_loopback_init,
};


/* --- functions --- */
static void
bse_loopback_class_init (BseLoopbackClass *class)
{
  BseObjectClass *object_class;
  BseSourceClass *source_class;
  guint ichannel_id, ochannel_id;

  parent_class = bse_type_class_peek (BSE_TYPE_SOURCE);
  object_class = BSE_OBJECT_CLASS (class);
  source_class = BSE_SOURCE_CLASS (class);

  object_class->shutdown = bse_loopback_do_shutdown;

  source_class->prepare = bse_loopback_prepare;
  source_class->calc_chunk = bse_loopback_calc_chunk;
  source_class->reset = bse_loopback_reset;
  
  ichannel_id = bse_source_class_add_ichannel (source_class, "mono_in", "Mono In", 1, 1);
  g_assert (ichannel_id == BSE_LOOPBACK_ICHANNEL_MONO);
  ichannel_id = bse_source_class_add_ichannel (source_class, "stereo_in", "Stereo In", 2, 2);
  g_assert (ichannel_id == BSE_LOOPBACK_ICHANNEL_STEREO);
  ochannel_id = bse_source_class_add_ochannel (source_class, "mono_out", "Mono Loopback", 1);
  g_assert (ochannel_id == BSE_LOOPBACK_OCHANNEL_MONO);
  ochannel_id = bse_source_class_add_ochannel (source_class, "stereo_out", "Stereo Loopback", 1);
  g_assert (ochannel_id == BSE_LOOPBACK_OCHANNEL_STEREO);
}

static void
bse_loopback_class_destroy (BseLoopbackClass *class)
{
}

static void
bse_loopback_init (BseLoopback *loopback)
{
  loopback->mchunk = NULL;
  loopback->schunk = NULL;
}

static void
bse_loopback_do_shutdown (BseObject *object)
{
  BseLoopback *loopback;

  loopback = BSE_LOOPBACK (object);

  /* chain parent class' shutdown handler */
  BSE_OBJECT_CLASS (parent_class)->shutdown (object);
}

#define N_STATIC_BLOCKS (17) /* FIXME: need n_blocks_per_second() */

static void
bse_loopback_prepare (BseSource *source,
		     BseIndex   index)
{
  BseLoopback *loopback = BSE_LOOPBACK (source);

  loopback->mchunk = bse_chunk_new (1);
  memset (loopback->mchunk->hunk, BSE_MAX_SAMPLE_VALUE, sizeof (BseSampleValue) * BSE_TRACK_LENGTH);
  loopback->mchunk->hunk_filled = TRUE;

  loopback->schunk = bse_chunk_new (2);
  memset (loopback->schunk->hunk, BSE_MAX_SAMPLE_VALUE, sizeof (BseSampleValue) * BSE_TRACK_LENGTH * 2);
  loopback->schunk->hunk_filled = TRUE;

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source, index);
}

static BseChunk*
bse_loopback_calc_chunk (BseSource *source,
			 guint      ochannel_id)
{
  BseLoopback *loopback = BSE_LOOPBACK (source);
  BseSourceInput *input;
  BseChunk *chunk = NULL;

  /* FIXME: drop delay */

  if (ochannel_id == BSE_LOOPBACK_OCHANNEL_MONO)
    {
      chunk = loopback->mchunk;
      input = bse_source_get_input (source, BSE_LOOPBACK_ICHANNEL_MONO);
      if (input)
	loopback->mchunk = bse_source_ref_chunk (input->osource, input->ochannel_id, source->index);
      else
	loopback->mchunk = bse_chunk_new_static_zero (1);
    }
  else if (ochannel_id == BSE_LOOPBACK_OCHANNEL_STEREO)
    {
      chunk = loopback->schunk;
      input = bse_source_get_input (source, BSE_LOOPBACK_ICHANNEL_STEREO);
      if (input)
	loopback->schunk = bse_source_ref_chunk (input->osource, input->ochannel_id, source->index);
      else
	loopback->schunk = bse_chunk_new_static_zero (2);
    }
  else
    g_assert_not_reached ();

  return chunk;
}

static void
bse_loopback_reset (BseSource *source)
{
  BseLoopback *loopback = BSE_LOOPBACK (source);

  bse_chunk_unref (loopback->mchunk);
  loopback->mchunk = NULL;
  bse_chunk_unref (loopback->schunk);
  loopback->schunk = NULL;

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}


/* --- Export to BSE --- */
#include "./icons/pipe.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_loopback, "BseLoopback", "BseSource",
    "BseLoopback simply puts out it's input",
    &type_info_loopback,
    "/Source/Loopback",
    { PIPE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      PIPE_WIDTH, PIPE_HEIGHT,
      PIPE_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;
