/* BseCapture - BSE Recording output source
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
#include "bsecapture.h"

#include <bse/bsechunk.h>
#include <bse/bsepcmstream.h>
#include <stdlib.h>
#include <time.h>


/* --- prototypes --- */
static void	 bse_capture_init			(BseCapture	*capture);
static void	 bse_capture_class_init		(BseCaptureClass	*class);
static void	 bse_capture_class_destroy	(BseCaptureClass	*class);
static void	 bse_capture_do_shutdown		(BseObject     	*object);
static void      bse_capture_prepare              (BseSource      *source,
						 BseIndex        index);
static BseChunk* bse_capture_calc_chunk           (BseSource      *source,
						 guint           ochannel_id);
static void      bse_capture_reset                (BseSource      *source);


/* --- variables --- */
static BseType           type_id_capture = 0;
static gpointer          parent_class = NULL;
static const BseTypeInfo type_info_capture = {
  sizeof (BseCaptureClass),
  
  (BseBaseInitFunc) NULL,
  (BseBaseDestroyFunc) NULL,
  (BseClassInitFunc) bse_capture_class_init,
  (BseClassDestroyFunc) bse_capture_class_destroy,
  NULL /* class_data */,
  
  sizeof (BseCapture),
  0 /* n_preallocs */,
  (BseObjectInitFunc) bse_capture_init,
};


/* --- functions --- */
static void
bse_capture_class_init (BseCaptureClass *class)
{
  BseObjectClass *object_class;
  BseSourceClass *source_class;
  guint ochannel_id;

  parent_class = bse_type_class_peek (BSE_TYPE_SOURCE);
  object_class = BSE_OBJECT_CLASS (class);
  source_class = BSE_SOURCE_CLASS (class);

  object_class->shutdown = bse_capture_do_shutdown;

  source_class->prepare = bse_capture_prepare;
  source_class->calc_chunk = bse_capture_calc_chunk;
  source_class->reset = bse_capture_reset;
  
  ochannel_id = bse_source_class_add_ochannel (source_class, "Capture", "Mono Capture Output", 1);
  g_assert (ochannel_id == BSE_CAPTURE_OCHANNEL_MONO);
}

static void
bse_capture_class_destroy (BseCaptureClass *class)
{
}

static void
bse_capture_init (BseCapture *capture)
{
}

static void
bse_capture_do_shutdown (BseObject *object)
{
  BseCapture *capture;

  capture = BSE_CAPTURE (object);

  /* chain parent class' shutdown handler */
  BSE_OBJECT_CLASS (parent_class)->shutdown (object);
}

#define N_STATIC_BLOCKS (17) /* FIXME: need n_blocks_per_second() */

static void
bse_capture_prepare (BseSource *source,
		   BseIndex   index)
{
  BseCapture *capture = BSE_CAPTURE (source);
  BseCaptureClass *class = BSE_CAPTURE_GET_CLASS (capture);

  if (!class->ref_count)
    {
      class->buffer = g_new0 (BseSampleValue, 2 * BSE_TRACK_LENGTH);
      class->n_buffers = bse_pcm_stream_extern_mic ? bse_pcm_stream_extern_mic->n_blocks : 0;
      bse_stream_read_sv (BSE_STREAM (bse_pcm_stream_extern_mic), BSE_TRACK_LENGTH * 2, class->buffer);
      bse_stream_read_sv (BSE_STREAM (bse_pcm_stream_extern_mic), BSE_TRACK_LENGTH * 2, class->buffer);
      bse_stream_read_sv (BSE_STREAM (bse_pcm_stream_extern_mic), BSE_TRACK_LENGTH * 2, class->buffer);
      bse_stream_read_sv (BSE_STREAM (bse_pcm_stream_extern_mic), BSE_TRACK_LENGTH * 2, class->buffer);
      bse_stream_read_sv (BSE_STREAM (bse_pcm_stream_extern_mic), BSE_TRACK_LENGTH * 2, class->buffer);
      bse_stream_read_sv (BSE_STREAM (bse_pcm_stream_extern_mic), BSE_TRACK_LENGTH * 2, class->buffer);
      bse_stream_read_sv (BSE_STREAM (bse_pcm_stream_extern_mic), BSE_TRACK_LENGTH * 2, class->buffer);
      bse_stream_read_sv (BSE_STREAM (bse_pcm_stream_extern_mic), BSE_TRACK_LENGTH * 2, class->buffer);
      bse_stream_read_sv (BSE_STREAM (bse_pcm_stream_extern_mic), BSE_TRACK_LENGTH * 2, class->buffer);
    }
  class->ref_count++;

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source, index);
}

static BseChunk*
bse_capture_calc_chunk (BseSource *source,
		      guint      ochannel_id)
{
  BseCapture *capture = BSE_CAPTURE (source);
  BseCaptureClass *class = BSE_CAPTURE_GET_CLASS (capture);
  BseSampleValue *hunk;
  guint i;
  
  g_return_val_if_fail (ochannel_id == BSE_CAPTURE_OCHANNEL_MONO, NULL);

  if (bse_pcm_stream_extern_mic &&
      BSE_STREAM_READY (bse_pcm_stream_extern_mic) &&
      BSE_STREAM_READABLE (bse_pcm_stream_extern_mic) &&
      bse_pcm_stream_extern_mic->attribs.n_channels == 2 &&
      bse_pcm_stream_extern_mic->attribs.play_frequency ==
      bse_pcm_stream_extern_mic->attribs.record_frequency &&
      bse_pcm_stream_extern_mic->n_blocks > class->n_buffers)
    {
      //      class->n_buffers = bse_pcm_stream_extern_mic->n_blocks;
      bse_stream_read_sv (BSE_STREAM (bse_pcm_stream_extern_mic), BSE_TRACK_LENGTH * 2, class->buffer);
    }

  hunk = bse_hunk_alloc (1);
  for (i = 0; i < BSE_TRACK_LENGTH; i++)
    {
      BseMixValue v;

      v = class->buffer[i * 2];
      v += class->buffer[i * 2 + 1];
      hunk[i] = v >> 2;
    }

  return bse_chunk_new_orphan (1, hunk);
}

static void
bse_capture_reset (BseSource *source)
{
  BseCapture *capture = BSE_CAPTURE (source);
  BseCaptureClass *class = BSE_CAPTURE_GET_CLASS (capture);
  
  class->ref_count--;
  if (!class->ref_count)
    {
      g_free (class->buffer);
      class->buffer = NULL;
      class->n_buffers = 0;
    }

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}


/* --- Export to BSE --- */
#include "./icons/noicon.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_capture, "BseCapture", "BseSource",
    "BseCapture puts out sound recorded from the soundcard",
    &type_info_capture,
    "/Source/Capture",
    { NOICON_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      NOICON_WIDTH, NOICON_HEIGHT,
      NOICON_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;
