/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2004 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bsepcmdevice-null.h"
#include "bsessequencer.h"
#include "bseengine.h"
#include <string.h>

typedef struct
{
  BsePcmHandle	handle;
  guint         buffer_size;
  guint         fill_level;
} NullHandle;

/* --- prototypes --- */
static gsize        null_device_read      (BsePcmHandle *handle,
                                           gsize         n_values,
                                           gfloat       *values);
static void         null_device_write     (BsePcmHandle *handle,
                                           gsize         n_values,
                                           const gfloat *values);
static void         null_device_status    (BsePcmHandle *handle,
                                           BsePcmStatus *status);


/* --- functions --- */
static void
bse_pcm_device_null_init (BsePcmDeviceNull *null)
{
}

static SfiRing*
bse_pcm_device_null_list_devices (BseDevice *device)
{
  SfiRing *ring = NULL;
  ring = sfi_ring_append (ring, bse_device_entry_new (device, g_strdup_printf ("default"), NULL));
  return ring;
}

static BseErrorType
bse_pcm_device_null_open (BseDevice     *device,
                          gboolean       require_readable,
                          gboolean       require_writable,
                          guint          n_args,
                          const gchar  **args)
{
  NullHandle *null = g_new0 (NullHandle, 1);
  BsePcmHandle *handle = &null->handle;
  
  /* setup request */
  handle->readable = require_readable;
  handle->writable = require_writable;
  handle->n_channels = 2;
  handle->mix_freq = bse_pcm_freq_from_freq_mode (BSE_PCM_DEVICE (device)->req_freq_mode);
  handle->read = NULL;
  handle->write = NULL;
  handle->status = NULL;
  null->buffer_size = 4096 * handle->n_channels;
  null->fill_level = 0;
  handle->minimum_watermark = null->buffer_size;
  handle->playback_watermark = null->buffer_size;
  BSE_OBJECT_SET_FLAGS (device, BSE_DEVICE_FLAG_OPEN);
  if (handle->readable)
    BSE_OBJECT_SET_FLAGS (device, BSE_DEVICE_FLAG_READABLE);
  handle->read = null_device_read;
  if (handle->writable)
    BSE_OBJECT_SET_FLAGS (device, BSE_DEVICE_FLAG_WRITABLE);
  handle->write = null_device_write;
  handle->status = null_device_status;
  BSE_PCM_DEVICE (device)->handle = handle;
  return BSE_ERROR_NONE;
}

static void
bse_pcm_device_null_close (BseDevice *device)
{
  NullHandle *null = (NullHandle*) BSE_PCM_DEVICE (device)->handle;
  BSE_PCM_DEVICE (device)->handle = NULL;
  g_free (null);
}

static void
null_device_status (BsePcmHandle *handle,
                    BsePcmStatus *status)
{
  NullHandle *null = (NullHandle*) handle;
  if (handle->readable)
    {
      status->total_capture_values = null->buffer_size;
      status->n_capture_values_available = null->buffer_size;
    }
  else
    {
      status->total_capture_values = 0;
      status->n_capture_values_available = 0;
    }
  if (handle->writable)
    {
      status->total_playback_values = null->buffer_size;
      null->fill_level = MIN (null->fill_level, null->buffer_size);
      /* ensure sequencer fairness */
      if (bse_sequencer_thread_lagging ())
        {       /* sequencer keep up */
          sfi_thread_wakeup (bse_ssequencer_thread);
        }
      else      /* drain buffer */
        null->fill_level -= MIN (null->fill_level, handle->n_channels * BSE_SSEQUENCER_PREPROCESS / 2);
      status->n_playback_values_available = null->buffer_size - null->fill_level;
    }
  else
    {
      status->total_playback_values = 0;
      status->n_playback_values_available = 0;
    }
}

static gsize
null_device_read (BsePcmHandle *handle,
		 gsize         n_values,
		 gfloat       *values)
{
  memset (values, 0, sizeof (values[0]) * n_values);
  return n_values;
}

static void
null_device_write (BsePcmHandle *handle,
		  gsize         n_values,
		  const gfloat *values)
{
  NullHandle *null = (NullHandle*) handle;
  null->fill_level += n_values;
}

static void
bse_pcm_device_null_class_init (BsePcmDeviceNullClass *class)
{
  BseDeviceClass *device_class = BSE_DEVICE_CLASS (class);
  
  device_class->list_devices = bse_pcm_device_null_list_devices;
  bse_device_class_setup (device_class,
                          -1,
                          "null",
                          NULL, /* syntax */
                          /* TRANSLATORS: keep this text to 70 chars in width */
                          _("Discard all PCM output and provide zero blocks as input. This\n"
                            "driver is not part of the automatic PCM device selection list."));
  device_class->open = bse_pcm_device_null_open;
  device_class->close = bse_pcm_device_null_close;
}

BSE_BUILTIN_TYPE (BsePcmDeviceNull)
{
  GType pcm_device_null_type;
  static const GTypeInfo pcm_device_null_info = {
    sizeof (BsePcmDeviceNullClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_pcm_device_null_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BsePcmDeviceNull),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_pcm_device_null_init,
  };
  pcm_device_null_type = bse_type_register_static (BSE_TYPE_PCM_DEVICE,
                                                   "BsePcmDeviceNull",
                                                   "Null PCM device implementation",
                                                   &pcm_device_null_info);
  return pcm_device_null_type;
}
