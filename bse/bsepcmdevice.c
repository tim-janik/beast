/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998-1999, 2000-2003 Tim Janik
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

#include "gslcommon.h"
#include <errno.h>


/* --- variables --- */
static gpointer parent_class = NULL;


/* --- functions --- */
static void
bse_pcm_device_init (BsePcmDevice *pdev)
{
  pdev->req_freq_mode = BSE_PCM_FREQ_44100;
  pdev->req_n_channels = 2;
  pdev->handle = NULL;
}

void
bse_pcm_device_request (BsePcmDevice  *self,
			guint          n_channels,
			BsePcmFreqMode freq_mode)
{
  g_return_if_fail (BSE_IS_PCM_DEVICE (self));
  g_return_if_fail (!BSE_DEVICE_OPEN (self));
  g_return_if_fail (n_channels >= 1 && n_channels <= 128);
  g_return_if_fail (freq_mode >= BSE_PCM_FREQ_8000 && freq_mode <= BSE_PCM_FREQ_192000);

  self->req_n_channels = n_channels;
  self->req_freq_mode = freq_mode;
}

static void
bse_pcm_device_dispose (GObject *object)
{
  BsePcmDevice *pdev = BSE_PCM_DEVICE (object);
  
  if (BSE_DEVICE_OPEN (pdev))
    {
      g_warning (G_STRLOC ": pcm device still opened");
      bse_device_close (BSE_DEVICE (pdev));
    }
  if (pdev->handle)
    g_warning (G_STRLOC ": pcm device with stale pcm handle");
  
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
pcm_device_post_open (BseDevice *device)
{
  BsePcmDevice *self = BSE_PCM_DEVICE (device);
  g_return_if_fail (BSE_DEVICE_OPEN (self) && self->handle);
  sfi_mutex_init (&self->handle->mutex);
}

static void
pcm_device_pre_close (BseDevice *device)
{
  BsePcmDevice *self = BSE_PCM_DEVICE (device);
  sfi_mutex_destroy (&self->handle->mutex);
}

BsePcmHandle*
bse_pcm_device_get_handle (BsePcmDevice *pdev)
{
  g_return_val_if_fail (BSE_IS_PCM_DEVICE (pdev), NULL);
  g_return_val_if_fail (BSE_DEVICE_OPEN (pdev), NULL);
  
  return pdev->handle;
}

gsize
bse_pcm_handle_read (BsePcmHandle *handle,
		     gsize         n_values,
		     gfloat       *values)
{
  gsize n;
  
  g_return_val_if_fail (handle != NULL, 0);
  g_return_val_if_fail (handle->readable, 0);
  if (n_values)
    g_return_val_if_fail (values != NULL, 0);
  else
    return 0;
  
  GSL_SPIN_LOCK (&handle->mutex);
  n = handle->read (handle, n_values, values);
  GSL_SPIN_UNLOCK (&handle->mutex);
  
  return n;
}

void
bse_pcm_handle_write (BsePcmHandle *handle,
		      gsize         n_values,
		      const gfloat *values)
{
  g_return_if_fail (handle != NULL);
  g_return_if_fail (handle->writable);
  if (n_values)
    g_return_if_fail (values != NULL);
  else
    return;
  
  GSL_SPIN_LOCK (&handle->mutex);
  handle->write (handle, n_values, values);
  GSL_SPIN_UNLOCK (&handle->mutex);
}

void
bse_pcm_handle_status (BsePcmHandle *handle,
		       BsePcmStatus *status)
{
  g_return_if_fail (handle != NULL);
  g_return_if_fail (status != NULL);
  
  GSL_SPIN_LOCK (&handle->mutex);
  handle->status (handle, status);
  GSL_SPIN_UNLOCK (&handle->mutex);
}

void
bse_pcm_handle_set_watermark (BsePcmHandle *handle,
			      guint         watermark)
{
  g_return_if_fail (handle != NULL);
  
  watermark = handle->mix_freq / 1000. * watermark * handle->n_channels;
  GSL_SPIN_LOCK (&handle->mutex);
  handle->playback_watermark = MAX (watermark, handle->minimum_watermark);
  GSL_SPIN_UNLOCK (&handle->mutex);
}


/* --- frequency utilities --- */
gfloat
bse_pcm_freq_from_freq_mode (BsePcmFreqMode freq_mode)
{
  switch (freq_mode)
    {
    case BSE_PCM_FREQ_8000:	return   8000;
    case BSE_PCM_FREQ_11025:	return  11025;
    case BSE_PCM_FREQ_16000:	return  16000;
    case BSE_PCM_FREQ_22050:	return  22050;
    case BSE_PCM_FREQ_32000:	return  32000;
    case BSE_PCM_FREQ_44100:	return  44100;
    case BSE_PCM_FREQ_48000:	return  48000;
    case BSE_PCM_FREQ_88200:	return  88200;
    case BSE_PCM_FREQ_96000:	return  96000;
    case BSE_PCM_FREQ_176400:	return 176400;
    case BSE_PCM_FREQ_192000:	return 192000;
    default:			return      0;
    }
}

BsePcmFreqMode
bse_pcm_freq_mode_from_freq (gfloat freq)
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

static void
bse_pcm_device_class_init (BsePcmDeviceClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseDeviceClass *device_class = BSE_DEVICE_CLASS (class);

  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->dispose = bse_pcm_device_dispose;

  device_class->post_open = pcm_device_post_open;
  device_class->pre_close = pcm_device_pre_close;
}

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
  
  return bse_type_register_abstract (BSE_TYPE_DEVICE,
                                     "BsePcmDevice",
                                     "PCM device base type",
                                     &pcm_device_info);
}
