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
#include <bse/bseheart.h>
#include <bse/bsemixer.h>
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
  /* FIXME: idevice hack */
  capture->idevice = g_strdup (bse_heart_get_default_idevice ());
  capture->pdev = NULL;
}

static void
bse_capture_do_shutdown (BseObject *object)
{
  BseCapture *capture;

  capture = BSE_CAPTURE (object);

  g_free (capture->idevice);
  capture->idevice = NULL;

  /* chain parent class' shutdown handler */
  BSE_OBJECT_CLASS (parent_class)->shutdown (object);
}

#define N_STATIC_BLOCKS (17) /* FIXME: need n_blocks_per_second() */

static void
bse_capture_prepare (BseSource *source,
		     BseIndex   index)
{
  BseCapture *capture = BSE_CAPTURE (source);

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source, index);

  capture->pdev = bse_heart_get_device (capture->idevice);
  if (capture->pdev)
    bse_heart_source_add_idevice (source, capture->pdev);
}

static BseChunk*
bse_capture_calc_chunk (BseSource *source,
		      guint      ochannel_id)
{
  BseCapture *capture = BSE_CAPTURE (source);
  BseSampleValue *hunk;
  
  g_return_val_if_fail (ochannel_id == BSE_CAPTURE_OCHANNEL_MONO, NULL);

  hunk = bse_hunk_alloc (1);
  if (capture->pdev && BSE_PCM_DEVICE_READABLE (capture->pdev))
    {
      BseChunk *chunk = bse_pcm_device_iqueue_peek (capture->pdev);

      bse_hunk_mix (1, hunk, NULL, chunk->n_tracks, chunk->hunk);
    }
  else
    memset (hunk, 0, BSE_TRACK_LENGTH * sizeof (BseSampleValue));

  return bse_chunk_new_orphan (1, hunk);
}

static void
bse_capture_reset (BseSource *source)
{
  BseCapture *capture = BSE_CAPTURE (source);

  if (capture->pdev)
    bse_heart_source_remove_idevice (source, capture->pdev);
  capture->pdev = NULL;

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}


/* --- Export to BSE --- */
#include "./icons/mic.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_capture, "BseCapture", "BseSource",
    "BseCapture puts out sound recorded from the soundcard",
    &type_info_capture,
    "/Source/Capture",
    { MIC_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      MIC_WIDTH, MIC_HEIGHT,
      MIC_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;
