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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "bseloopback.h"

#include <stdlib.h>
#include <time.h>


/* --- prototypes --- */
static void	 bse_loopback_init		(BseLoopback		*loopback);
static void	 bse_loopback_class_init	(BseLoopbackClass	*class);
static void	 bse_loopback_class_finalize	(BseLoopbackClass	*class);
static void	 bse_loopback_do_destroy	(BseObject		*object);
static void	 bse_loopback_prepare		(BseSource		*source,
						 BseIndex		 index);
static BseChunk* bse_loopback_calc_chunk	(BseSource		*source,
						 guint			 ochannel_id);
static void	 bse_loopback_reset		(BseSource		*source);


/* --- variables --- */
static GType		 type_id_loopback = 0;
static gpointer		 parent_class = NULL;
static const GTypeInfo type_info_loopback = {
  sizeof (BseLoopbackClass),
  
  (GBaseInitFunc) NULL,
  (GBaseFinalizeFunc) NULL,
  (GClassInitFunc) bse_loopback_class_init,
  (GClassFinalizeFunc) bse_loopback_class_finalize,
  NULL /* class_data */,
  
  sizeof (BseLoopback),
  0 /* n_preallocs */,
  (GInstanceInitFunc) bse_loopback_init,
};


/* --- functions --- */
static void
bse_loopback_class_init (BseLoopbackClass *class)
{
  BseObjectClass *object_class;
  BseSourceClass *source_class;
  BseSourceIChannelDef *ic_def;
  guint ichannel_id, ochannel_id;

  parent_class = g_type_class_peek (BSE_TYPE_SOURCE);
  object_class = BSE_OBJECT_CLASS (class);
  source_class = BSE_SOURCE_CLASS (class);

  object_class->destroy = bse_loopback_do_destroy;

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

  /* we need history buffers for the input channels */
  ic_def = BSE_SOURCE_CLASS_ICHANNEL_DEF (class, BSE_LOOPBACK_ICHANNEL_MONO);
  ic_def->history = 1;
  ic_def = BSE_SOURCE_CLASS_ICHANNEL_DEF (class, BSE_LOOPBACK_ICHANNEL_STEREO);
  ic_def->history = 1;
}

static void
bse_loopback_class_finalize (BseLoopbackClass *class)
{
}

static void
bse_loopback_init (BseLoopback *loopback)
{
}

static void
bse_loopback_do_destroy (BseObject *object)
{
  BseLoopback *loopback;

  loopback = BSE_LOOPBACK (object);

  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

#define N_STATIC_BLOCKS (17) /* FIXME: need n_blocks_per_second() */
BseSampleValue *debug = NULL;
static void
bse_loopback_prepare (BseSource *source,
		     BseIndex	index)
{
  // BseLoopback *loopback = BSE_LOOPBACK (source);
  BseChunk *dummy = bse_chunk_new_static_zero (1);
  debug = dummy->hunk;
  bse_chunk_unref (dummy);

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source, index);
}

static BseChunk*
bse_loopback_calc_chunk (BseSource *source,
			 guint	    ochannel_id)
{
  // BseLoopback *loopback = BSE_LOOPBACK (source);
  BseSourceInput *input;
  guint ichannel_id = 0;

  /* FIXME: drop delay */

  if (ochannel_id == BSE_LOOPBACK_OCHANNEL_MONO)
    ichannel_id = BSE_LOOPBACK_ICHANNEL_MONO;
  else if (ochannel_id == BSE_LOOPBACK_OCHANNEL_STEREO)
    ichannel_id = BSE_LOOPBACK_ICHANNEL_STEREO;
  else
    g_assert_not_reached ();

  input = bse_source_get_input (source, ichannel_id);
  if (input)
    {
      BseChunk *chunk;

      /* make sure there is a chunk calculated, so we can fetch it through
       * the history on our next roundtrip
       */
      chunk = bse_source_ref_chunk (input->osource, input->ochannel_id, source->index);
      bse_chunk_unref (chunk);

      /* fetch history chunk from last roundtrip
       */
      chunk = bse_source_ref_chunk (input->osource, input->ochannel_id, source->index - 1);

      if (chunk->hunk == debug)
	{
	  g_print ("loopback to static zero\n");
	}

      return chunk;
    }
  else
    return bse_chunk_new_static_zero (ochannel_id == BSE_LOOPBACK_ICHANNEL_STEREO ? 2 : 1);
}

static void
bse_loopback_reset (BseSource *source)
{
  // BseLoopback *loopback = BSE_LOOPBACK (source);

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}


/* --- Export to BSE --- */
#include "./icons/pipe.c"
BSE_EXPORTS_BEGIN ();
BSE_EXPORT_OBJECTS = {
  { &type_id_loopback, "BseLoopback", "BseSource",
    "BseLoopback simply puts out it's input",
    &type_info_loopback,
    "/Modules/Loopback",
    { PIPE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      PIPE_WIDTH, PIPE_HEIGHT,
      PIPE_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;
