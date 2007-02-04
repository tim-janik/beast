/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998-1999, 2000-2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
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
  pdev->req_n_channels = 2;
  pdev->req_mix_freq = 44100;
  pdev->req_latency_ms = 150;
  pdev->req_block_length = 1024;
  pdev->handle = NULL;
}

void
bse_pcm_device_request (BsePcmDevice  *self,
                        guint	       n_channels,
                        guint          mix_freq,
                        guint          latency_ms,
                        guint          block_length) /* in frames */
{
  g_return_if_fail (BSE_IS_PCM_DEVICE (self));
  g_return_if_fail (!BSE_DEVICE_OPEN (self));
  g_return_if_fail (n_channels >= 1 && n_channels <= 128);
  g_return_if_fail (mix_freq >= 1000 && mix_freq <= 192000);

  self->req_n_channels = n_channels;
  self->req_mix_freq = mix_freq;
  self->req_block_length = MAX (block_length, 2);
  self->req_latency_ms = latency_ms;
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
  g_return_if_fail (BSE_DEVICE_OPEN (self) && self->handle->block_length == 0);
  sfi_mutex_init (&self->handle->mutex);
}

static void
pcm_device_pre_close (BseDevice *device)
{
  BsePcmDevice *self = BSE_PCM_DEVICE (device);
  sfi_mutex_destroy (&self->handle->mutex);
}

guint
bse_pcm_device_get_mix_freq (BsePcmDevice *pdev)
{
  g_return_val_if_fail (BSE_IS_PCM_DEVICE (pdev), 0);
  if (BSE_DEVICE_OPEN (pdev))
    return pdev->handle->mix_freq;
  else
    return 0;
}

BsePcmHandle*
bse_pcm_device_get_handle (BsePcmDevice *pdev,
                           guint         block_length)
{
  g_return_val_if_fail (BSE_IS_PCM_DEVICE (pdev), NULL);
  g_return_val_if_fail (BSE_DEVICE_OPEN (pdev), NULL);
  g_return_val_if_fail (block_length > 0, NULL);

  GSL_SPIN_LOCK (&pdev->handle->mutex);
  if (!pdev->handle->block_length)
    pdev->handle->block_length = block_length;
  GSL_SPIN_UNLOCK (&pdev->handle->mutex);

  if (pdev->handle->block_length == block_length)
    return pdev->handle;
  else
    return NULL;
}

gsize
bse_pcm_handle_read (BsePcmHandle *handle,
		     gsize         n_values,
		     gfloat       *values)
{
  gsize n;
  
  g_return_val_if_fail (handle != NULL, 0);
  g_return_val_if_fail (handle->readable, 0);
  g_return_val_if_fail (n_values == handle->block_length * handle->n_channels, 0);
  
  GSL_SPIN_LOCK (&handle->mutex);
  n = handle->read (handle, values);
  GSL_SPIN_UNLOCK (&handle->mutex);
  
  g_return_val_if_fail (n == handle->block_length * handle->n_channels, n);
  return n;
}

void
bse_pcm_handle_write (BsePcmHandle *handle,
		      gsize         n_values,
		      const gfloat *values)
{
  g_return_if_fail (handle != NULL);
  g_return_if_fail (handle->writable);
  g_return_if_fail (values != NULL);
  g_return_if_fail (n_values == handle->block_length * handle->n_channels);

  GSL_SPIN_LOCK (&handle->mutex);
  handle->write (handle, values);
  GSL_SPIN_UNLOCK (&handle->mutex);
}

gboolean
bse_pcm_handle_check_io (BsePcmHandle           *handle,
                         glong                  *timeoutp)
{
  g_return_val_if_fail (handle != NULL, 0);

  glong dummy;
  if (!timeoutp)
    timeoutp = &dummy;
  GSL_SPIN_LOCK (&handle->mutex);
  gboolean can_read_write = handle->check_io (handle, timeoutp);
  GSL_SPIN_UNLOCK (&handle->mutex);
  return can_read_write;
}

guint
bse_pcm_handle_latency (BsePcmHandle *handle)
{
  g_return_val_if_fail (handle != NULL, 0);
  GSL_SPIN_LOCK (&handle->mutex);
  guint n_frames = handle->latency (handle);
  GSL_SPIN_UNLOCK (&handle->mutex);
  return n_frames;
}


/* --- frequency utilities --- */
guint
bse_pcm_device_frequency_align (gint mix_freq)
{
  static const gint frequency_list[] = {
    5512, 8000, 11025, 16000, 22050, 32000,
    44100, 48000, 64000, 88200, 96000, 176400, 192000
  };
  guint i, best = frequency_list[0], diff = ABS (mix_freq - frequency_list[0]);
  for (i = 1; i < G_N_ELEMENTS (frequency_list); i++)
    {
      guint diff2 = ABS (mix_freq - frequency_list[i]);
      if (diff2 <= diff)
        {
          best = frequency_list[i];
          diff = diff2;
        }
    }
  return best;
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
                                     __FILE__, __LINE__,
                                     &pcm_device_info);
}
