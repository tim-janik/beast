/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2004 Tim Janik
 * Copyright (C) 2004 Stefan Westerfeld
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "configure.h"
#include "bsepcmdevice-portaudio.h"
#include <portaudio.h>
#include <bse/gsldatautils.h>
#include <string.h>
#include <errno.h>
#include <string>
#include <vector>
#include <algorithm>

using std::string;
using std::vector;
using std::max;

#define PCM_DEBUG(...)          sfi_debug ("pcm", __VA_ARGS__)


/* --- PortAudio PCM handle --- */
typedef struct
{
  BsePcmHandle  handle;
  PaStream     *stream;
} PortAudioPcmHandle;

/* --- prototypes --- */
static void             bse_pcm_device_port_audio_class_init  (BsePcmDevicePortAudioClass  *klass);
static void             bse_pcm_device_port_audio_init        (BsePcmDevicePortAudio       *self);
static gsize            port_audio_device_read                (BsePcmHandle           *handle,
							       gfloat                 *values);
static void             port_audio_device_write               (BsePcmHandle           *handle,
							       const gfloat           *values);
static gboolean         port_audio_device_check_io            (BsePcmHandle           *handle,
							       glong                  *timeoutp);
static guint            port_audio_device_latency             (BsePcmHandle           *handle);

/* --- define object type and export to BSE --- */
BSE_REGISTER_OBJECT (BsePcmDevicePortAudio, BsePcmDevice, NULL, NULL, NULL, bse_pcm_device_port_audio_class_init, NULL, bse_pcm_device_port_audio_init);
BSE_DEFINE_EXPORTS (__FILE__);

/* --- variables --- */
static gpointer parent_class = NULL;

/* --- functions --- */
static void
bse_pcm_device_port_audio_init (BsePcmDevicePortAudio *self)
{
  Pa_Initialize();
}

static std::string
port_audio_host_api_name (PaHostApiIndex host_api_index)
{
  const PaHostApiInfo *host_api_info = Pa_GetHostApiInfo (host_api_index);

  switch (host_api_info->type)
    {
    case paOSS:   return "oss";
    case paJACK:  return "jack";
    case paALSA:  return "alsa";
    default:      ;
    }

  char *api_str = g_strdup_printf ("pa%02d", host_api_info->type);
  string name = api_str;
  g_free (api_str);
  return name;
}

/* we build a per-host-api device index, since we hope that this
 * will be a bit more reliable than the global index, so that
 * specifiying a device like alsa:0 results in (more or less)
 * the same PortAudio device being used, reliably
 */
static vector<string>
port_audio_devices()
{
  vector<string> devices (Pa_GetDeviceCount());

  for (PaHostApiIndex host_api_index = 0; host_api_index < Pa_GetHostApiCount(); host_api_index++)
    {
      int host_api_device_index = 0; /* host api specific index */
      string host_api_name = port_audio_host_api_name (host_api_index);

      for (PaDeviceIndex device_index = 0; device_index < Pa_GetDeviceCount(); device_index++)
	{
	  const PaDeviceInfo *device_info = Pa_GetDeviceInfo (device_index);
	  if (device_info->hostApi == host_api_index)
	    {
	      char *device_args = g_strdup_printf ("%s:%d", host_api_name.c_str(), host_api_device_index++);
	      devices[device_index] = device_args;
	      g_free (device_args);
	    }
	}
    }

  return devices;
}

static SfiRing*
bse_pcm_device_port_audio_list_devices (BseDevice *device)
{
  vector<string> devices = port_audio_devices();
  SfiRing *ring = NULL;

  int default_device_index = Pa_GetDefaultOutputDevice();

  for (PaDeviceIndex device_index = 0; device_index < Pa_GetDeviceCount(); device_index++)
    {
      const PaDeviceInfo *device_info = Pa_GetDeviceInfo (device_index);
      const PaHostApiInfo *host_api_info = Pa_GetHostApiInfo (device_info->hostApi);
      char *device_args = g_strdup (devices[device_index].c_str());

      BseDeviceEntry *entry = bse_device_group_entry_new (device, device_args,
							  g_strdup (host_api_info->name),
							  g_strdup_printf ("%-10s%s%s", device_args, device_info->name,
							                                device_index == default_device_index ? " (default)": ""));
      ring = sfi_ring_append (ring, entry);
    }
  if (!ring)
    ring = sfi_ring_append (ring, bse_device_error_new (device, g_strdup_printf ("No devices found")));
  return ring;
}

static BseErrorType
bse_pcm_device_port_audio_open (BseDevice     *device,
			        gboolean       require_readable,
			        gboolean       require_writable,
			        guint          n_args,
			        const gchar  **args)
{
  PortAudioPcmHandle *portaudio = g_new0 (PortAudioPcmHandle, 1);
  BsePcmHandle *handle = &portaudio->handle;
  BsePcmDevice *pdev = BSE_PCM_DEVICE (device);

  handle->readable = require_readable;
  handle->writable = require_writable;
  handle->n_channels = 2;   /* TODO: mono */
  handle->mix_freq = pdev->req_mix_freq;

  PaStreamParameters inputParameters;
  PaStreamParameters outputParameters;

  inputParameters.device = Pa_GetDefaultInputDevice();
  outputParameters.device = Pa_GetDefaultOutputDevice();

  /* choose device from string ("alsa:1" means use the second device offered by the alsa host api) */
  if (n_args >= 1)
    {
      vector<string> devs = port_audio_devices();
      vector<string>::iterator di = find (devs.begin(), devs.end(), args[0]);
      if (di != devs.end())
	{
	  outputParameters.device = di - devs.begin();
	  inputParameters.device = di - devs.begin();
	}
      else
	return BSE_ERROR_DEVICE_NOT_AVAILABLE;
    }
  if (n_args >= 2)
    {
      if (strcmp (args[1], "rw") == 0)
	{
	  handle->readable = TRUE;
	  handle->writable = TRUE;
	}
      else if (strcmp (args[1], "ro") == 0)
	{
	  handle->readable = TRUE;
	  handle->writable = FALSE;
	}
      else if (strcmp (args[1], "wo") == 0)
	{
	  handle->readable = FALSE;
	  handle->writable = TRUE;
	}
    }

  inputParameters.channelCount  = handle->n_channels;
  outputParameters.channelCount = handle->n_channels;

  inputParameters.sampleFormat  = paFloat32;
  outputParameters.sampleFormat = paFloat32;

  inputParameters.suggestedLatency  = BSE_PCM_DEVICE (device)->req_latency_ms * 0.001;
  outputParameters.suggestedLatency = BSE_PCM_DEVICE (device)->req_latency_ms * 0.001;

  inputParameters.hostApiSpecificStreamInfo  = NULL;
  outputParameters.hostApiSpecificStreamInfo = NULL;

  BseErrorType error = BSE_ERROR_NONE;
  PaError pa_error;
  pa_error = Pa_OpenStream (&portaudio->stream, 
      handle->readable ? &inputParameters : NULL,
      handle->writable ? &outputParameters : NULL,
      handle->mix_freq,
      pdev->req_block_length,
      paDitherOff,
      NULL,	/* no callback -> blocking api */
      NULL);
  Pa_StartStream (portaudio->stream);
  /*
  printf ("latency input: %f\n", Pa_GetStreamInfo (portaudio->stream)->inputLatency);
  printf ("latency output: %f\n", Pa_GetStreamInfo (portaudio->stream)->outputLatency);
  */

  if (pa_error != paNoError)
    error = BSE_ERROR_DEVICE_NOT_AVAILABLE; /* _DEVICE_BUSY? */

  if (!error)
    {
      BSE_OBJECT_SET_FLAGS (pdev, BSE_DEVICE_FLAG_OPEN);

      if (handle->readable)
	{
	  BSE_OBJECT_SET_FLAGS (pdev, BSE_DEVICE_FLAG_READABLE);
	  handle->read = port_audio_device_read;
	}
      if (handle->writable)
	{
	  BSE_OBJECT_SET_FLAGS (pdev, BSE_DEVICE_FLAG_WRITABLE);
	  handle->write = port_audio_device_write;
	}
      handle->check_io = port_audio_device_check_io;
      handle->latency = port_audio_device_latency;
      pdev->handle = handle;
    }

  return error;
}

static void
bse_pcm_device_port_audio_close (BseDevice *device)
{
  PortAudioPcmHandle *portaudio = (PortAudioPcmHandle*) BSE_PCM_DEVICE (device)->handle;
  BSE_PCM_DEVICE (device)->handle = NULL;

  if (portaudio->stream)
    {
      Pa_StopStream (portaudio->stream);
      Pa_CloseStream (portaudio->stream);
    }
  g_free (portaudio);
}

static void
bse_pcm_device_port_audio_finalize (GObject *object)
{
  Pa_Terminate();
  /* BsePcmDevicePortAudio *self = BSE_PCM_DEVICE_PORT_AUDIO (object); */
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
port_audio_device_retrigger (PortAudioPcmHandle *portaudio)
{
  /* silence fill output, to resynchronize output and input streams to the desired latency */
  guint write_frames_avail = Pa_GetStreamWriteAvailable (portaudio->stream);
  float *silence = (float *)g_malloc0 (portaudio->handle.block_length * sizeof (float) * portaudio->handle.n_channels);

  while (write_frames_avail >= portaudio->handle.block_length)
    {
      Pa_WriteStream (portaudio->stream, silence, portaudio->handle.block_length);
      write_frames_avail = Pa_GetStreamWriteAvailable (portaudio->stream);
    }
  g_free (silence);
}

static gboolean
port_audio_device_check_io (BsePcmHandle *handle,
			    glong        *timeoutp)
{
  PortAudioPcmHandle *portaudio = (PortAudioPcmHandle*) handle;
  guint read_frames_avail = handle->readable ? Pa_GetStreamReadAvailable (portaudio->stream) : 0;
  guint write_frames_avail = handle->writable ? Pa_GetStreamWriteAvailable (portaudio->stream) : 0;
  guint n_frames_avail = handle->readable ? read_frames_avail : write_frames_avail;

  if (handle->readable && handle->writable && write_frames_avail >= (2 * handle->block_length) && read_frames_avail == 0)
    {
      /* underrun occured (or stream just initialized) */
      port_audio_device_retrigger (portaudio);
    }
  /* check whether data can be processed */
  if (n_frames_avail >= handle->block_length)
    return TRUE;        /* need processing */
  
  /* calculate timeout until processing is possible or needed */
  guint diff_frames = handle->block_length - n_frames_avail;
  *timeoutp = diff_frames * 1000 / handle->mix_freq;
  
  return FALSE;
}

static guint
port_audio_device_latency (BsePcmHandle *handle)
{
  PortAudioPcmHandle *portaudio = (PortAudioPcmHandle*) handle;

  /* return total latency in frames */
  /* -> this is probably an estimate, as I don't think the PortAudio API exports a precise value at any place */
  return guint (max (Pa_GetStreamInfo (portaudio->stream)->inputLatency, Pa_GetStreamInfo (portaudio->stream)->outputLatency) * handle->mix_freq);
}

static gsize
port_audio_device_read (BsePcmHandle *handle,
		        gfloat       *values)
{
  PortAudioPcmHandle *portaudio = (PortAudioPcmHandle*) handle;
  Pa_ReadStream (portaudio->stream, values, handle->block_length);
  return handle->block_length * handle->n_channels;
}

static void
port_audio_device_write (BsePcmHandle *handle,
			 const gfloat *values)
{
  PortAudioPcmHandle *portaudio = (PortAudioPcmHandle*) handle;
  Pa_WriteStream (portaudio->stream, values, handle->block_length);
}

static void
bse_pcm_device_port_audio_class_init (BsePcmDevicePortAudioClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseDeviceClass *device_class = BSE_DEVICE_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);
  
  gobject_class->finalize = bse_pcm_device_port_audio_finalize;
  
  device_class->list_devices = bse_pcm_device_port_audio_list_devices;
  const gchar *name = "portaudio";
  const gchar *syntax = _("DEVICE,MODE");
  const gchar *info = g_intern_printf (/* TRANSLATORS: keep this text to 70 chars in width */
                                       _("PortAudio PCM driver, using %s.\n"
                                         "  DEVICE - the PortAudio device to use, 'default' selects a default device\n"
                                         "  MODE   - rw = read/write, ro = readonly, wo = writeonly\n"),
                                       Pa_GetVersionText());
  bse_device_class_setup (klass, BSE_RATING_FALLBACK, name, syntax, info);
  device_class->open = bse_pcm_device_port_audio_open;
  device_class->close = bse_pcm_device_port_audio_close;
}
