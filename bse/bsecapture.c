/* BseCapture - BSE Recording output source
 * Copyright (C) 1999 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include	"bsecapture.h"

#include	"bsechunk.h"
#include	"bsecategories.h"
#include	"bsehunkmixer.h"
#include	<stdlib.h>
#include	<time.h>

#include	"./icons/mic.c"


/* --- prototypes --- */
static void	 bse_capture_init		(BseCapture	 *capture);
static void	 bse_capture_class_init		(BseCaptureClass *class);
static void	 bse_capture_class_finalize	(BseCaptureClass *class);
static void	 bse_capture_do_destroy		(BseObject     	 *object);
static void      bse_capture_prepare            (BseSource       *source,
						 BseIndex         index);
static BseChunk* bse_capture_calc_chunk         (BseSource       *source,
						 guint            ochannel_id);
static void      bse_capture_reset              (BseSource       *source);


/* --- variables --- */
static gpointer  parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseCapture)
{
  GType   capture_type;
  
  static const GTypeInfo capture_info = {
    sizeof (BseCaptureClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_capture_class_init,
    (GClassFinalizeFunc) bse_capture_class_finalize,
    NULL /* class_data */,
    
    sizeof (BseCapture),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_capture_init,
  };
  static const BsePixdata mic_pixdata = {
    MIC_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
    MIC_WIDTH, MIC_HEIGHT,
    MIC_RLE_PIXEL_DATA,
  };
  
  capture_type = bse_type_register_static (BSE_TYPE_SOURCE,
					   "BseCapture",
					   "BSE Recording source",
					   &capture_info);
  bse_categories_register_icon ("/Source/Capture", capture_type, &mic_pixdata);

  return capture_type;
}

static void
bse_capture_class_init (BseCaptureClass *class)
{
  BseObjectClass *object_class;
  BseSourceClass *source_class;
  guint ochannel_id;

  parent_class = g_type_class_peek_parent (class);
  object_class = BSE_OBJECT_CLASS (class);
  source_class = BSE_SOURCE_CLASS (class);

  object_class->destroy = bse_capture_do_destroy;

#if 0
  source_class->prepare = bse_capture_prepare;
  source_class->calc_chunk = bse_capture_calc_chunk;
  source_class->reset = bse_capture_reset;
#endif
  
  ochannel_id = bse_source_class_add_ochannel (source_class, "Capture", "Mono Capture Output");
  g_assert (ochannel_id == BSE_CAPTURE_OCHANNEL_MONO);
}

static void
bse_capture_class_finalize (BseCaptureClass *class)
{
}

static void
bse_capture_init (BseCapture *capture)
{
  /* FIXME: idevice hack */
  // capture->idevice = g_strdup (bse_heart_default_idevice_name ());
  capture->pdev = NULL;
}

static void
bse_capture_do_destroy (BseObject *object)
{
  BseCapture *capture;

  capture = BSE_CAPTURE (object);

  g_free (capture->idevice);
  capture->idevice = NULL;

  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bse_capture_prepare (BseSource *source,
		     BseIndex   index)
{
  BseCapture *capture = BSE_CAPTURE (source);

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);

  // capture->pdev = bse_heart_get_device (capture->idevice);
  // if (capture->pdev)
  //   bse_heart_source_add_idevice (source, capture->pdev);
}

static BseChunk*
bse_capture_calc_chunk (BseSource *source,
			guint      ochannel_id)
{
  BseCapture *capture = BSE_CAPTURE (source);
  BseSampleValue *hunk;
  
  g_return_val_if_fail (ochannel_id == BSE_CAPTURE_OCHANNEL_MONO, NULL);

  hunk = bse_hunk_alloc (1);
#if 0
  if (capture->pdev && BSE_DEVICE_READABLE (capture->pdev))
    {
      BseChunk *chunk = bse_pcm_device_iqueue_peek (capture->pdev);

      bse_hunk_mix (1, hunk, NULL, chunk->n_tracks, chunk->hunk);
    }
  else
#endif
    memset (hunk, 0, BSE_TRACK_LENGTH * sizeof (BseSampleValue));

  return bse_chunk_new_orphan (1, hunk);
}

static void
bse_capture_reset (BseSource *source)
{
  BseCapture *capture = BSE_CAPTURE (source);

  // if (capture->pdev)
  //   bse_heart_source_remove_idevice (source, capture->pdev);
  capture->pdev = NULL;

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}
