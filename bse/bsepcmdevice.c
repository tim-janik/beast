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

#include <unistd.h>
#include <errno.h>


/* --- prototypes --- */
static void	   bse_pcm_device_init		(BsePcmDevice      *pdev);
static void	   bse_pcm_device_class_init	(BsePcmDeviceClass *class);
static guint       bse_pcm_device_default_read  (BsePcmDevice      *pdev,
						 guint              n_bytes,
						 guint8            *bytes);
static guint       bse_pcm_device_default_write (BsePcmDevice      *pdev,
						 guint              n_bytes,
						 guint8            *bytes);
static inline void bse_pcm_device_reset_state   (BsePcmDevice      *pdev);
static void	   bse_pcm_device_shutdown	(BseObject         *object);


/* --- variables --- */
static gpointer parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BsePcmDevice)
{
  static const BseTypeInfo pcm_device_info = {
    sizeof (BsePcmDeviceClass),
    
    (BseBaseInitFunc) NULL,
    (BseBaseDestroyFunc) NULL,
    (BseClassInitFunc) bse_pcm_device_class_init,
    (BseClassDestroyFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BsePcmDevice),
    0 /* n_preallocs */,
    (BseObjectInitFunc) bse_pcm_device_init,
  };
  
  g_assert (BSE_PCM_FLAGS_USHIFT < BSE_OBJECT_FLAGS_MAX_SHIFT);
  
  return bse_type_register_static (BSE_TYPE_OBJECT,
				   "BsePcmDevice",
				   "Base type for PCM devices",
				   &pcm_device_info);
}

static void
bse_pcm_device_class_init (BsePcmDeviceClass *class)
{
  BseObjectClass *object_class;
  
  parent_class = bse_type_class_peek (BSE_TYPE_OBJECT);
  object_class = BSE_OBJECT_CLASS (class);
  
  object_class->shutdown = bse_pcm_device_shutdown;
  
  class->update_caps = NULL;
  class->open = NULL;
  class->update_state = NULL;
  class->read = bse_pcm_device_default_read;
  class->write = bse_pcm_device_default_write;
  class->in_playback = NULL;
  class->close = NULL;
  class->device_name = NULL;
}

static inline void
bse_pcm_device_reset_state (BsePcmDevice *pdev)
{
  pdev->pfd.fd = -1;
  pdev->pfd.events = 0;
  pdev->pfd.revents = 0;
  pdev->n_channels = 0;
  pdev->sample_freq = 0;
  pdev->n_fragments = 0;
  pdev->fragment_size = 0;
  pdev->block_size = 0;
  pdev->n_playback_bytes = 0;
  pdev->n_capture_bytes = 0;
}

static void
bse_pcm_device_init (BsePcmDevice *pdev)
{
  pdev->last_error = BSE_ERROR_NONE;
  memset (&pdev->caps, 0, sizeof (pdev->caps));
  pdev->capture_cache = NULL;
  pdev->capture_cache_destroy = NULL;
  bse_pcm_device_reset_state (pdev);
}

static void
bse_pcm_device_shutdown (BseObject *object)
{
  BsePcmDevice *pdev = BSE_PCM_DEVICE (object);
  
  if (BSE_PCM_DEVICE_OPEN (pdev))
    bse_pcm_device_close (pdev);
  bse_pcm_device_set_capture_cache (pdev, NULL, NULL);
  
  /* chain parent class' shutdown handler */
  BSE_OBJECT_CLASS (parent_class)->shutdown (object);
}

gchar*
bse_pcm_device_get_device_name (BsePcmDevice *pdev)
{
  gchar *name;

  g_return_val_if_fail (BSE_IS_PCM_DEVICE (pdev), NULL);

  if (BSE_PCM_DEVICE_GET_CLASS (pdev)->device_name)
    name = BSE_PCM_DEVICE_GET_CLASS (pdev)->device_name (pdev, FALSE);
  else
    name = BSE_OBJECT_TYPE_NAME (pdev);

  return name ? name : "";
}

gchar*
bse_pcm_device_get_device_blurb (BsePcmDevice *pdev)
{
  gchar *name;

  g_return_val_if_fail (BSE_IS_PCM_DEVICE (pdev), NULL);

  if (BSE_PCM_DEVICE_GET_CLASS (pdev)->device_name)
    name = BSE_PCM_DEVICE_GET_CLASS (pdev)->device_name (pdev, TRUE);
  else
    name = BSE_OBJECT_TYPE_NAME (pdev);

  return name ? name : "";
}

BseErrorType
bse_pcm_device_update_caps (BsePcmDevice *pdev)
{
  g_return_val_if_fail (BSE_IS_PCM_DEVICE (pdev), BSE_ERROR_INTERNAL);
  
  if (!BSE_PCM_DEVICE_OPEN (pdev))
    {
      memset (&pdev->caps, 0, sizeof (pdev->caps));
      pdev->last_error = BSE_PCM_DEVICE_GET_CLASS (pdev)->update_caps (pdev);
      if (pdev->last_error)
	{
	  BSE_OBJECT_UNSET_FLAGS (pdev, BSE_PCM_FLAG_HAS_CAPS);
	  memset (&pdev->caps, 0, sizeof (pdev->caps));
	}
      else
	BSE_OBJECT_SET_FLAGS (pdev, BSE_PCM_FLAG_HAS_CAPS);
    }
  else
    pdev->last_error = BSE_ERROR_NONE;
  
  errno = 0;
  
  return pdev->last_error;
}

void
bse_pcm_device_invalidate_caps (BsePcmDevice *pdev)
{
  g_return_if_fail (BSE_IS_PCM_DEVICE (pdev));
  g_return_if_fail (!BSE_PCM_DEVICE_OPEN (pdev));

  pdev->last_error = BSE_ERROR_NONE;

  BSE_OBJECT_UNSET_FLAGS (pdev, BSE_PCM_FLAG_HAS_CAPS);
  memset (&pdev->caps, 0, sizeof (pdev->caps));

  errno = 0;
}

BseErrorType
bse_pcm_device_open (BsePcmDevice *pdev,
		     gboolean      readable,
		     gboolean      writable,
		     guint         n_channels,
		     gdouble       sample_freq,
		     guint         fragment_size)
{
  BsePcmFreqMask rate;
  
  g_return_val_if_fail (BSE_IS_PCM_DEVICE (pdev), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (!BSE_PCM_DEVICE_OPEN (pdev), BSE_ERROR_INTERNAL);
  
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
  fragment_size = MAX (fragment_size, 32);
  
  /* check arguments */
  pdev->last_error = BSE_ERROR_INTERNAL;
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
  g_return_val_if_fail (fragment_size <= pdev->caps.max_fragment_size, BSE_ERROR_INTERNAL);

  bse_globals_lock ();
  
  /* attempt opening */
  pdev->last_error = BSE_PCM_DEVICE_GET_CLASS (pdev)->open (pdev,
							    readable,
							    writable,
							    n_channels,
							    rate,
							    fragment_size);
  if (pdev->last_error)
    bse_pcm_device_reset_state (pdev);
  else
    {
      bse_object_lock (BSE_OBJECT (pdev));
      BSE_OBJECT_SET_FLAGS (pdev, BSE_PCM_FLAG_OPEN);
      g_main_add_poll (&pdev->pfd, BSE_HEART_PRIORITY);
    }
  
  bse_globals_unlock ();

  errno = 0;
  
  return pdev->last_error;
}

void
bse_pcm_device_close (BsePcmDevice *pdev)
{
  g_return_if_fail (BSE_IS_PCM_DEVICE (pdev));
  g_return_if_fail (BSE_PCM_DEVICE_OPEN (pdev));
  
  pdev->last_error = BSE_ERROR_NONE;
  
  g_main_remove_poll (&pdev->pfd);
  BSE_PCM_DEVICE_GET_CLASS (pdev)->close (pdev);
  
  errno = 0;
  
  BSE_OBJECT_UNSET_FLAGS (pdev,
			  BSE_PCM_FLAG_OPEN |
			  BSE_PCM_FLAG_READABLE |
			  BSE_PCM_FLAG_WRITABLE);
  bse_pcm_device_reset_state (pdev);
  bse_object_unlock (BSE_OBJECT (pdev));
}

guint
bse_pcm_device_oready (BsePcmDevice *pdev,
		       guint         n_values)
{
  guint n_bytes;

  g_return_val_if_fail (BSE_IS_PCM_DEVICE (pdev), FALSE);
  
  pdev->last_error = BSE_ERROR_NONE;
  
  n_bytes = sizeof (BseSampleValue) * n_values;
  if (BSE_PCM_DEVICE_OPEN (pdev) &&
      BSE_PCM_DEVICE_WRITABLE (pdev) &&
      pdev->n_playback_bytes < n_bytes)
    {
      pdev->n_playback_bytes = 0;
      BSE_PCM_DEVICE_GET_CLASS (pdev)->update_state (pdev);
    }
  
  errno = 0;
  
  return pdev->n_playback_bytes / n_bytes;
}

guint
bse_pcm_device_iready (BsePcmDevice *pdev,
		       guint         n_values)
{
  guint n_bytes;

  g_return_val_if_fail (BSE_IS_PCM_DEVICE (pdev), FALSE);

  pdev->last_error = BSE_ERROR_NONE;

  n_bytes = sizeof (BseSampleValue) * n_values;
  if (BSE_PCM_DEVICE_OPEN (pdev) &&
      BSE_PCM_DEVICE_READABLE (pdev) &&
      pdev->n_capture_bytes < n_bytes)
    {
      pdev->n_capture_bytes = 0;
      BSE_PCM_DEVICE_GET_CLASS (pdev)->update_state (pdev);
    }
  
  errno = 0;
  
  return pdev->n_capture_bytes / n_bytes;
}

void
bse_pcm_device_set_capture_cache (BsePcmDevice   *pdev,
				  BseSampleValue *cache,
				  GDestroyNotify  destroy)
{
  g_return_if_fail (BSE_IS_PCM_DEVICE (pdev));
  
  if (pdev->capture_cache_destroy)
    pdev->capture_cache_destroy (pdev->capture_cache);
  pdev->capture_cache = cache;
  pdev->capture_cache_destroy = destroy;
}

void
bse_pcm_device_read (BsePcmDevice   *pdev,
		     guint           n_values,
		     BseSampleValue *values)
{
  g_return_if_fail (BSE_IS_PCM_DEVICE (pdev));
  g_return_if_fail (BSE_PCM_DEVICE_READABLE (pdev));
  
  pdev->last_error = BSE_ERROR_NONE;
  
  if (n_values)
    {
      guint n, n_bytes = n_values * sizeof (BseSampleValue);
      
      g_return_if_fail (values != NULL);
      
      n = BSE_PCM_DEVICE_GET_CLASS (pdev)->read (pdev, n_bytes, (guint8*) values);
      if (n < n_bytes)
	{
	  pdev->n_capture_bytes = 0;
	  g_warning ("%s: failed to read %u bytes of %u (%s)",
		     BSE_OBJECT_TYPE_NAME (pdev),
		     n_bytes - n,
		     n_bytes,
		     g_strerror (errno));
	}
      if (n < pdev->n_capture_bytes)
	pdev->n_capture_bytes -= n;
      else
	pdev->n_capture_bytes = 0;
    }
  
  errno = 0;
}

static guint
bse_pcm_device_default_read (BsePcmDevice *pdev,
			     guint         n_bytes,
			     guint8       *bytes)
{
  gint n;
  
  do
    n = read (pdev->pfd.fd, bytes, n_bytes);
  while (n < 0 && errno == EINTR); /* don't mind signals */
  
  return MAX (n, 0);
}

void
bse_pcm_device_write (BsePcmDevice   *pdev,
		      guint           n_values,
		      BseSampleValue *values)
{
  g_return_if_fail (BSE_IS_PCM_DEVICE (pdev));
  g_return_if_fail (BSE_PCM_DEVICE_WRITABLE (pdev));
  
  pdev->last_error = BSE_ERROR_NONE;
  
  if (n_values)
    {
      guint n, n_bytes = n_values * sizeof (BseSampleValue);
      
      g_return_if_fail (values != NULL);
      
      pdev->n_playback_bytes = 0;
      n = BSE_PCM_DEVICE_GET_CLASS (pdev)->write (pdev, n_bytes, (guint8*) values);
      if (n < n_bytes)
	{
	  pdev->n_playback_bytes = 0;
	  g_warning ("%s: failed to write %u bytes of %u (%s)",
		     BSE_OBJECT_TYPE_NAME (pdev),
		     n_bytes - n,
		     n_bytes,
		     g_strerror (errno));
	}
      if (n < pdev->n_playback_bytes)
	pdev->n_playback_bytes -= n;
      else
	pdev->n_playback_bytes = 0;
    }
  
  errno = 0;
}

static guint
bse_pcm_device_default_write (BsePcmDevice *pdev,
			      guint         n_bytes,
			      guint8       *bytes)
{
  gint n;
  
  do
    n = write (pdev->pfd.fd, bytes, n_bytes);
  while (n < 0 && errno == EINTR); /* don't mind signals */
  
  return MAX (n, 0);
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
