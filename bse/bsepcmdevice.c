/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
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
#include "bsepcmdevice.h"

#include "bsechunk.h"
#include <errno.h>


/* --- prototypes --- */
static void	   bse_pcm_device_init			(BsePcmDevice      *pdev);
static void	   bse_pcm_device_class_init		(BsePcmDeviceClass *class);
static inline void bse_pcm_device_reset_device  	(BsePcmDevice      *pdev);
static void	   bse_pcm_device_destroy		(BseObject         *object);
static void	   bse_pcm_device_close			(BseDevice	   *dev);


/* --- variables --- */
static gpointer parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BsePcmDevice)
{
  static const GTypeInfo pcm_device_info = {
    sizeof (BsePcmDeviceClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_pcm_device_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BsePcmDevice),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_pcm_device_init,
  };
  
  g_assert (BSE_PCM_FLAGS_USHIFT < BSE_OBJECT_FLAGS_MAX_SHIFT);
  
  return bse_type_register_static (BSE_TYPE_DEVICE,
				   "BsePcmDevice",
				   "PCM device base type",
				   &pcm_device_info);
}

static void
bse_pcm_device_class_init (BsePcmDeviceClass *class)
{
  BseObjectClass *object_class;
  BseDeviceClass *device_class;
  
  parent_class = g_type_class_peek (BSE_TYPE_DEVICE);
  object_class = BSE_OBJECT_CLASS (class);
  device_class = BSE_DEVICE_CLASS (class);
  
  object_class->destroy = bse_pcm_device_destroy;
  
  device_class->close = bse_pcm_device_close;

  class->update_caps = NULL;
  class->open = NULL;
  class->update_state = NULL;
  class->retrigger = NULL;
}

static inline void
bse_pcm_device_reset_device (BsePcmDevice *pdev)
{
  pdev->n_channels = 0;
  pdev->sample_freq = 0;
  bse_pcm_device_time_warp (pdev);
  while (pdev->iqueue)
    bse_pcm_device_iqueue_pop (pdev);
  while (pdev->oqueue)
    bse_pcm_device_oqueue_pop (pdev);
}

static void
bse_pcm_device_init (BsePcmDevice *pdev)
{
  memset (&pdev->caps, 0, sizeof (pdev->caps));
  pdev->iqueue = NULL;
  pdev->oqueue = NULL;
  bse_pcm_device_reset_device (pdev);
}

void
bse_pcm_device_time_warp (BsePcmDevice *pdev)
{
  g_return_if_fail (BSE_IS_PCM_DEVICE (pdev));

  BSE_OBJECT_UNSET_FLAGS (pdev, BSE_PCM_FLAG_STATE_SYNC);
  pdev->playback_buffer_size = 0;
  pdev->n_playback_bytes = 0;
  pdev->capture_buffer_size = 0;
  pdev->n_capture_bytes = 0;
}

static void
bse_pcm_device_destroy (BseObject *object)
{
  BsePcmDevice *pdev;

  pdev = BSE_PCM_DEVICE (object);
  
  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

BseErrorType
bse_pcm_device_update_caps (BsePcmDevice *pdev)
{
  BseDevice *dev;

  g_return_val_if_fail (BSE_IS_PCM_DEVICE (pdev), BSE_ERROR_INTERNAL);
  
  dev = BSE_DEVICE (pdev);

  if (!BSE_DEVICE_OPEN (pdev))
    {
      memset (&pdev->caps, 0, sizeof (pdev->caps));
      dev->last_error = BSE_PCM_DEVICE_GET_CLASS (pdev)->update_caps (pdev);
      if (dev->last_error)
	{
	  BSE_OBJECT_UNSET_FLAGS (pdev, BSE_PCM_FLAG_HAS_CAPS);
	  memset (&pdev->caps, 0, sizeof (pdev->caps));
	}
      else
	BSE_OBJECT_SET_FLAGS (pdev, BSE_PCM_FLAG_HAS_CAPS);
    }
  else
    dev->last_error = BSE_ERROR_NONE;
  
  errno = 0;
  
  return dev->last_error;
}

void
bse_pcm_device_invalidate_caps (BsePcmDevice *pdev)
{
  BseDevice *dev;

  g_return_if_fail (BSE_IS_PCM_DEVICE (pdev));
  g_return_if_fail (!BSE_DEVICE_OPEN (pdev));

  dev = BSE_DEVICE (pdev);

  dev->last_error = BSE_ERROR_NONE;

  BSE_OBJECT_UNSET_FLAGS (pdev, BSE_PCM_FLAG_HAS_CAPS);
  memset (&pdev->caps, 0, sizeof (pdev->caps));

  errno = 0;
}

BseErrorType
bse_pcm_device_open (BsePcmDevice *pdev,
		     gboolean      readable,
		     gboolean      writable,
		     guint         n_channels,
		     gdouble       sample_freq)
{
  BseDevice *dev;
  BsePcmFreqMask rate;
  guint fragment_size;
  
  g_return_val_if_fail (BSE_IS_PCM_DEVICE (pdev), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (!BSE_DEVICE_OPEN (pdev), BSE_ERROR_INTERNAL);
  
  dev = BSE_DEVICE (pdev);

  if (!BSE_PCM_DEVICE_HAS_CAPS (pdev))
    {
      BseErrorType error;
      
      error = bse_pcm_device_update_caps (pdev);
      if (error)
	return error;
    }
  
  /* normalize arguments */
  readable = readable != FALSE;
  writable = writable != FALSE;
  rate = bse_pcm_freq_from_freq (sample_freq);
  
  /* check arguments */
  dev->last_error = BSE_ERROR_INTERNAL;
  if (readable)
    {
      g_return_val_if_fail (pdev->caps.readable == readable, BSE_ERROR_INTERNAL);
      g_return_val_if_fail (pdev->caps.capture_freq_mask & rate, BSE_ERROR_INTERNAL);
      if (writable)
	g_return_val_if_fail (pdev->caps.duplex == (readable && writable), BSE_ERROR_INTERNAL);
    }
  else
    g_return_val_if_fail (readable || writable, BSE_ERROR_INTERNAL);
  if (writable)
    {
      g_return_val_if_fail (pdev->caps.writable == writable, BSE_ERROR_INTERNAL);
      g_return_val_if_fail (pdev->caps.playback_freq_mask & rate, BSE_ERROR_INTERNAL);
    }
  g_return_val_if_fail (n_channels >= 1 && n_channels <= pdev->caps.max_n_channels, BSE_ERROR_INTERNAL);

  bse_globals_lock ();
  
  /* attempt opening */
  fragment_size = BSE_TRACK_LENGTH * n_channels * sizeof (BseSampleValue);
  fragment_size = CLAMP (fragment_size, 32, pdev->caps.max_fragment_size);
  dev->last_error = BSE_PCM_DEVICE_GET_CLASS (pdev)->open (pdev,
							    readable,
							    writable,
							    n_channels,
							    rate,
							    fragment_size);
  if (dev->last_error)
    bse_pcm_device_reset_device (pdev);
  else
    {
      bse_object_lock (BSE_OBJECT (pdev));
      BSE_OBJECT_SET_FLAGS (pdev, BSE_DEVICE_FLAG_OPEN);
      g_main_add_poll (&dev->pfd, BSE_HEART_PRIORITY);
    }
  
  bse_globals_unlock ();

  errno = 0;
  
  return dev->last_error;
}

static void
bse_pcm_device_close (BseDevice *dev)
{
  BsePcmDevice *pdev = BSE_PCM_DEVICE (dev);

  g_main_remove_poll (&dev->pfd);
  bse_pcm_device_reset_device (pdev);
  bse_object_unlock (BSE_OBJECT (pdev));
}

void
bse_pcm_device_retrigger (BsePcmDevice *pdev)
{
  BseDevice *dev;

  g_return_if_fail (BSE_IS_PCM_DEVICE (pdev));
  g_return_if_fail (BSE_DEVICE_OPEN (pdev));
  g_return_if_fail (BSE_PCM_DEVICE_GET_CLASS (pdev)->retrigger != NULL);

  dev = BSE_DEVICE (pdev);

  bse_pcm_device_time_warp (pdev);

  dev->last_error = BSE_ERROR_NONE;

  BSE_PCM_DEVICE_GET_CLASS (pdev)->retrigger (pdev);
  
  errno = 0;
}

void
bse_pcm_device_read (BsePcmDevice   *pdev,
		     guint           n_values,
		     BseSampleValue *values)
{
  BseDevice *dev;

  g_return_if_fail (BSE_IS_PCM_DEVICE (pdev));
  g_return_if_fail (BSE_DEVICE_READABLE (pdev));

  dev = BSE_DEVICE (pdev);
  dev->last_error = BSE_ERROR_NONE;
  
  if (n_values)
    {
      guint n, n_bytes = n_values * sizeof (BseSampleValue);
      
      g_return_if_fail (values != NULL);
      
      n = BSE_DEVICE_GET_CLASS (dev)->read (dev, n_bytes, (guint8*) values);
      if (n < n_bytes)
	{
	  bse_pcm_device_time_warp (pdev);
	  g_warning ("%s: failed to read %u bytes of %u (%s)",
		     BSE_OBJECT_TYPE_NAME (pdev),
		     n_bytes - n,
		     n_bytes,
		     g_strerror (errno));
	}
      else if (BSE_PCM_DEVICE_STATE_SYNC (pdev) && n < pdev->n_capture_bytes)
	pdev->n_capture_bytes -= n;
      else
	bse_pcm_device_time_warp (pdev);
    }
  
  errno = 0;
}

void
bse_pcm_device_write (BsePcmDevice   *pdev,
		      guint           n_values,
		      BseSampleValue *values)
{
  BseDevice *dev;

  g_return_if_fail (BSE_IS_PCM_DEVICE (pdev));
  g_return_if_fail (BSE_DEVICE_WRITABLE (pdev));
  
  dev = BSE_DEVICE (pdev);
  dev->last_error = BSE_ERROR_NONE;
  
  if (n_values)
    {
      guint n, n_bytes = n_values * sizeof (BseSampleValue);
      
      g_return_if_fail (values != NULL);
      
      n = BSE_DEVICE_GET_CLASS (pdev)->write (dev, n_bytes, (guint8*) values);
      if (n < n_bytes)
	{
	  bse_pcm_device_time_warp (pdev);
	  g_warning ("%s: failed to write %u bytes of %u (%s)",
		     BSE_OBJECT_TYPE_NAME (pdev),
		     n_bytes - n,
		     n_bytes,
		     g_strerror (errno));
	}
      else if (BSE_PCM_DEVICE_STATE_SYNC (pdev) && n < pdev->n_playback_bytes)
	pdev->n_playback_bytes -= n;
      else
	bse_pcm_device_time_warp (pdev);
    }
  
  errno = 0;
}

void
bse_pcm_device_update_state (BsePcmDevice *pdev)
{
  BseDevice *dev;

  g_return_if_fail (BSE_IS_PCM_DEVICE (pdev));

  dev = BSE_DEVICE (pdev);

  dev->last_error = BSE_ERROR_NONE;

  if (BSE_DEVICE_OPEN (pdev) && !BSE_PCM_DEVICE_STATE_SYNC (pdev))
    {
      BSE_PCM_DEVICE_GET_CLASS (pdev)->update_state (pdev);
      BSE_OBJECT_SET_FLAGS (pdev, BSE_PCM_FLAG_STATE_SYNC);
    }

  errno = 0;
}

void
bse_pcm_device_iqueue_push (BsePcmDevice *pdev,
			    BseChunk     *chunk)
{
  g_return_if_fail (BSE_IS_PCM_DEVICE (pdev));
  g_return_if_fail (BSE_DEVICE_READABLE (pdev));
  g_return_if_fail (chunk != NULL);
  g_return_if_fail (chunk->n_tracks == pdev->n_channels);

  bse_chunk_ref (chunk);
  pdev->iqueue = g_slist_append (pdev->iqueue, chunk);
}

BseChunk*
bse_pcm_device_iqueue_peek (BsePcmDevice *pdev)
{
  g_return_val_if_fail (BSE_IS_PCM_DEVICE (pdev), NULL);

  return pdev->iqueue ? pdev->iqueue->data : NULL;
}

void
bse_pcm_device_iqueue_pop (BsePcmDevice *pdev)
{
  GSList *slist;
  BseChunk *chunk;
  guint n_chunk_values, n_ivalues;
  
  g_return_if_fail (BSE_IS_PCM_DEVICE (pdev));
  g_return_if_fail (BSE_DEVICE_READABLE (pdev));
  g_return_if_fail (pdev->iqueue != NULL);

  slist = pdev->iqueue;
  pdev->iqueue = slist->next;
  chunk = slist->data;
  g_slist_free_1 (slist);
  bse_chunk_unref (chunk);

  n_chunk_values = BSE_TRACK_LENGTH * pdev->n_channels;
  bse_pcm_device_update_state (pdev);
  n_ivalues = pdev->n_capture_bytes / sizeof (BseSampleValue);
  if (n_ivalues)
    while (n_chunk_values <= n_ivalues)
      {
	chunk = bse_chunk_new (pdev->n_channels);
	
	bse_pcm_device_read (pdev, chunk->n_tracks * BSE_TRACK_LENGTH, chunk->hunk);
	chunk->hunk_filled = TRUE;
	bse_pcm_device_iqueue_push (pdev, chunk);
	bse_chunk_unref (chunk);
	n_ivalues = pdev->n_capture_bytes / sizeof (BseSampleValue);
      }
}

void
bse_pcm_device_oqueue_push (BsePcmDevice *pdev,
			    BseChunk     *chunk)
{
  guint n_chunk_values, n_ovalues;

  g_return_if_fail (BSE_IS_PCM_DEVICE (pdev));
  g_return_if_fail (BSE_DEVICE_WRITABLE (pdev));
  g_return_if_fail (chunk != NULL);
  g_return_if_fail (chunk->n_tracks == pdev->n_channels);

  bse_chunk_ref (chunk);
  pdev->oqueue = g_slist_append (pdev->oqueue, chunk);

  n_chunk_values = BSE_TRACK_LENGTH * pdev->n_channels;
  bse_pcm_device_update_state (pdev);
  n_ovalues = pdev->n_playback_bytes / sizeof (BseSampleValue);
  while (n_chunk_values <= n_ovalues && pdev->oqueue)
    {
      BseChunk *chunk = pdev->oqueue->data;
      
      bse_pcm_device_write (pdev, chunk->n_tracks * BSE_TRACK_LENGTH, bse_chunk_complete_hunk (chunk));
      bse_pcm_device_oqueue_pop (pdev);
      n_ovalues = pdev->n_playback_bytes / sizeof (BseSampleValue);
    }
}

BseChunk*
bse_pcm_device_oqueue_peek (BsePcmDevice *pdev)
{
  g_return_val_if_fail (BSE_IS_PCM_DEVICE (pdev), NULL);

  return pdev->oqueue ? pdev->oqueue->data : NULL;
}

void
bse_pcm_device_oqueue_pop (BsePcmDevice *pdev)
{
  GSList *slist;
  BseChunk *chunk;

  g_return_if_fail (BSE_IS_PCM_DEVICE (pdev));
  g_return_if_fail (BSE_DEVICE_WRITABLE (pdev));
  g_return_if_fail (pdev->oqueue != NULL);

  slist = pdev->oqueue;
  pdev->oqueue = slist->next;
  chunk = slist->data;
  g_slist_free_1 (slist);
  bse_chunk_unref (chunk);
}

gulong
bse_pcm_device_n_values_to_msecs (BsePcmDevice *pdev,
				  gulong      	n_values)
{
  gulong msecs = 0;

  g_return_val_if_fail (BSE_IS_PCM_DEVICE (pdev), ~msecs);
  g_return_val_if_fail (BSE_DEVICE_OPEN (pdev), ~msecs);

  if (n_values)
    {
      gdouble d = n_values;

      d *= 1000;
      d /= pdev->n_channels * pdev->sample_freq;
      msecs = d + 0.5;
    }

  return msecs;
}

gulong
bse_pcm_device_msecs_to_n_values (BsePcmDevice *pdev,
				  gulong        msecs)
{
  gulong n_values = 0;

  g_return_val_if_fail (BSE_IS_PCM_DEVICE (pdev), ~n_values);
  g_return_val_if_fail (BSE_DEVICE_OPEN (pdev), ~n_values);

  if (msecs)
    {
      gdouble d = msecs;

      d /= 1000;
      d *= pdev->n_channels * pdev->sample_freq;
      n_values = d + 0.5;
    }

  return n_values;
}

gulong
bse_pcm_device_need_processing (BsePcmDevice *pdev,
				gulong        latency)
{
  guint n_chunk_values, n_values_left = ~0;

  g_return_val_if_fail (BSE_IS_PCM_DEVICE (pdev), ~0);
  g_return_val_if_fail (BSE_DEVICE_OPEN (pdev), ~0);

  /* check whether we need to process incoming or outgoing data
   * return 0 timeout here.
   * if we don't currently need to process data, we return an
   * estimated timout value (milli seconds) on when the need is
   * likely to occour.
   * refer to docs/bse-heart.txt for further details
   */

  n_chunk_values = BSE_TRACK_LENGTH * pdev->n_channels;
  bse_pcm_device_update_state (pdev);
  if (BSE_DEVICE_READABLE (pdev))
    {
      guint n_ivalues = pdev->n_capture_bytes / sizeof (BseSampleValue);

      if (n_chunk_values <= n_ivalues)
	return 0;
      n_values_left = MIN (n_values_left, n_chunk_values - n_ivalues);
    }
  if (BSE_DEVICE_WRITABLE (pdev))
    {
      guint n_ovalues = pdev->n_playback_bytes / sizeof (BseSampleValue);
      guint n_required, n_buffered, n_queued;

      if (pdev->oqueue)
	{
	  if (n_chunk_values <= n_ovalues)
	    return 0;
	  n_values_left = MIN (n_values_left, n_chunk_values - n_ovalues);
	}

      n_required = bse_pcm_device_msecs_to_n_values (pdev, latency);
      /* normalize latency queue to chunk bounds */
      n_required /= BSE_TRACK_LENGTH * pdev->n_channels;
      n_required = MAX (n_required, 1);
      n_required *= BSE_TRACK_LENGTH * pdev->n_channels;

      n_buffered = (pdev->playback_buffer_size - pdev->n_playback_bytes) / sizeof (BseSampleValue);
      n_queued = g_slist_length (pdev->oqueue) * BSE_TRACK_LENGTH * pdev->n_channels;
      if (n_buffered + n_queued <= n_required)
	return 0;
      n_values_left = MIN (n_values_left, n_buffered + n_queued - n_required);
    }

  return bse_pcm_device_n_values_to_msecs (pdev, n_values_left);
}

gboolean
bse_pcm_device_process (BsePcmDevice *pdev,
			gulong        latency)
{
  guint n_chunk_values;
  gboolean need_oqueue_fill = FALSE;

  g_return_val_if_fail (BSE_IS_PCM_DEVICE (pdev), FALSE);
  g_return_val_if_fail (BSE_DEVICE_OPEN (pdev), FALSE);

  /* process outgoing and incoming data, report whether the
   * output queue needs to be filled.
   * refer to docs/bse-heart.txt for further details
   */

  n_chunk_values = BSE_TRACK_LENGTH * pdev->n_channels;
  bse_pcm_device_update_state (pdev);
  if (BSE_DEVICE_WRITABLE (pdev))
    {
      guint n_ovalues = pdev->n_playback_bytes / sizeof (BseSampleValue);

      while (n_chunk_values <= n_ovalues && pdev->oqueue)
	{
	  BseChunk *chunk = pdev->oqueue->data;

	  bse_pcm_device_write (pdev, chunk->n_tracks * BSE_TRACK_LENGTH, bse_chunk_complete_hunk (chunk));
	  bse_pcm_device_oqueue_pop (pdev);
	  n_ovalues = pdev->n_playback_bytes / sizeof (BseSampleValue);
	}
    }
  if (BSE_DEVICE_READABLE (pdev))
    {
      guint n_ivalues = pdev->n_capture_bytes / sizeof (BseSampleValue);

      while (n_chunk_values <= n_ivalues)
	{
	  BseChunk *chunk = bse_chunk_new (pdev->n_channels);

	  bse_pcm_device_read (pdev, chunk->n_tracks * BSE_TRACK_LENGTH, chunk->hunk);
	  chunk->hunk_filled = TRUE;
	  bse_pcm_device_iqueue_push (pdev, chunk);
	  bse_chunk_unref (chunk);
	  n_ivalues = pdev->n_capture_bytes / sizeof (BseSampleValue);
	}
    }
  if (BSE_DEVICE_WRITABLE (pdev))
    {
      guint n_required = bse_pcm_device_msecs_to_n_values (pdev, latency);
      guint n_buffered = (pdev->playback_buffer_size - pdev->n_playback_bytes) / sizeof (BseSampleValue);
      guint n_queued = g_slist_length (pdev->oqueue) * BSE_TRACK_LENGTH * pdev->n_channels;

      /* normalize latency queue to chunk bounds */
      n_required /= BSE_TRACK_LENGTH * pdev->n_channels;
      n_required = MAX (n_required, 1);
      n_required *= BSE_TRACK_LENGTH * pdev->n_channels;

      need_oqueue_fill = n_buffered + n_queued <= n_required;
    }

  return need_oqueue_fill;
}


/* --- frequency utilities --- */
gdouble
bse_pcm_freq_to_freq (BsePcmFreqMask pcm_freq)
{
  switch (pcm_freq)
    {
    case BSE_PCM_FREQ_8000:   return 8000;
    case BSE_PCM_FREQ_11025:  return 11025;
    case BSE_PCM_FREQ_16000:  return 16000;
    case BSE_PCM_FREQ_22050:  return 22050;
    case BSE_PCM_FREQ_32000:  return 32000;
    case BSE_PCM_FREQ_44100:  return 44100;
    case BSE_PCM_FREQ_48000:  return 48000;
    case BSE_PCM_FREQ_88200:  return 88200;
    case BSE_PCM_FREQ_96000:  return 96000;
    case BSE_PCM_FREQ_176400: return 176400;
    case BSE_PCM_FREQ_192000: return 192000;
    default:		      return 0;
    }
}

BsePcmFreqMask
bse_pcm_freq_from_freq (gdouble freq)
{
  if (freq < (0      +   8000) / 2) return 0;
  if (freq < (8000   +  11025) / 2) return BSE_PCM_FREQ_8000;
  if (freq < (11025  +  16000) / 2) return BSE_PCM_FREQ_11025;
  if (freq < (16000  +  22050) / 2) return BSE_PCM_FREQ_16000;
  if (freq < (22050  +  32000) / 2) return BSE_PCM_FREQ_22050;
  if (freq < (32000  +  44100) / 2) return BSE_PCM_FREQ_32000;
  if (freq < (44100  +  48000) / 2) return BSE_PCM_FREQ_44100;
  if (freq < (48000  +  88200) / 2) return BSE_PCM_FREQ_48000;
  if (freq < (88200  +  96000) / 2) return BSE_PCM_FREQ_88200;
  if (freq < (96000  + 176400) / 2) return BSE_PCM_FREQ_96000;
  if (freq < (176400 + 192000) / 2) return BSE_PCM_FREQ_176400;
  if (freq < (192000 + 200000) / 2) return BSE_PCM_FREQ_192000;
  return 0;
}
