// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "configure.h"
#include "bsepcmdevice-portaudio.hh"
#include <portaudio.h>
#include <bse/gsldatautils.hh>
#include <string.h>
#include <errno.h>
#include <string>
#include <vector>
#include <algorithm>
using std::string;
using std::vector;
using std::max;

#define PDEBUG(...)     BSE_KEY_DEBUG ("pcm-portaudio", __VA_ARGS__)

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
static const char type_blurb[] = ("PCM driver implementation using PortAudio (http://www.portaudio.com)");
BSE_REGISTER_OBJECT (BsePcmDevicePortAudio, BsePcmDevice, NULL, "", type_blurb, NULL, bse_pcm_device_port_audio_class_init, NULL, bse_pcm_device_port_audio_init);
BSE_DEFINE_EXPORTS();

/* --- variables --- */
static gpointer parent_class = NULL;

/* --- functions --- */
static void
bse_pcm_device_port_audio_init (BsePcmDevicePortAudio *self)
{
  Pa_Initialize();
}

static BseErrorType
bse_error_from_pa_error (PaError      pa_error,
                         BseErrorType fallback)
{
  switch (pa_error)
    {
    case paNoError:                     		return BSE_ERROR_NONE;
    case paNotInitialized:              		return BSE_ERROR_INTERNAL;      /* wrong portaudio usage */
    case paUnanticipatedHostError:      		return fallback;
    case paInvalidChannelCount:         		return BSE_ERROR_DEVICE_CHANNELS;
    case paInvalidSampleRate:           		return BSE_ERROR_DEVICE_FREQUENCY;
    case paInvalidDevice:               		return BSE_ERROR_DEVICE_NOT_AVAILABLE;
    case paInvalidFlag:                 		return fallback;
    case paSampleFormatNotSupported:    		return BSE_ERROR_DEVICE_FORMAT;
    case paBadIODeviceCombination:      		return BSE_ERROR_DEVICES_MISMATCH;
    case paInsufficientMemory:          		return BSE_ERROR_NO_MEMORY;
    case paBufferTooBig:                		return BSE_ERROR_DEVICE_BUFFER;
    case paBufferTooSmall:                              return BSE_ERROR_DEVICE_BUFFER;
    case paNullCallback:                                return BSE_ERROR_INTERNAL;      /* wrong portaudio usage */
    case paBadStreamPtr:                                return BSE_ERROR_INTERNAL;      /* wrong portaudio usage */
    case paTimedOut:                                    return fallback;
    case paInternalError:                               return fallback;                /* puhh, portaudio internal... */
    case paDeviceUnavailable:                           return BSE_ERROR_DEVICE_NOT_AVAILABLE;
    case paIncompatibleHostApiSpecificStreamInfo:       return fallback;                /* portaudio has odd errors... */
    case paStreamIsStopped:                             return fallback;
    case paStreamIsNotStopped:                          return fallback;
    case paInputOverflowed:                             return BSE_ERROR_IO;            /* driver should recover */
    case paOutputUnderflowed:                           return BSE_ERROR_IO;            /* driver should recover */
    case paHostApiNotFound:                             return BSE_ERROR_DEVICE_NOT_AVAILABLE;
    case paInvalidHostApi:                              return BSE_ERROR_DEVICE_NOT_AVAILABLE;
    case paCanNotReadFromACallbackStream:               return BSE_ERROR_INTERNAL;      /* wrong portaudio usage */
    case paCanNotWriteToACallbackStream:                return BSE_ERROR_INTERNAL;      /* wrong portaudio usage */
    case paCanNotReadFromAnOutputOnlyStream:            return BSE_ERROR_INTERNAL;      /* wrong portaudio usage */
    case paCanNotWriteToAnInputOnlyStream:              return BSE_ERROR_INTERNAL;      /* wrong portaudio usage */
    case paIncompatibleStreamHostApi:                   return fallback;
    }
  return fallback;
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

  char *api_str = g_strdup_format ("pa%02d", host_api_info->type);
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
	      char *device_args = g_strdup_format ("%s:%d", host_api_name.c_str(), host_api_device_index++);
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
							  g_strdup_format ("%-10s%s%s", device_args, device_info->name,
                                                                           device_index == default_device_index ? " (default)": ""));
      ring = sfi_ring_append (ring, entry);
    }
  if (!ring)
    ring = sfi_ring_append (ring, bse_device_error_new (device, g_strdup_format ("No devices found")));
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

  BseErrorType error = BSE_ERROR_NONE;

  PaStreamParameters inputParameters = { 0, };
  inputParameters.device = Pa_GetDefaultInputDevice();
  PaStreamParameters outputParameters = { 0, };
  outputParameters.device = Pa_GetDefaultOutputDevice();
  string device_name = "default";
  if (!error && n_args >= 1 && strcmp (args[0], "default") != 0)
    {
      /* choose device from string ("alsa:1" means use the second device offered by the alsa host api) */
      vector<string> devs = port_audio_devices();
      vector<string>::iterator di = find (devs.begin(), devs.end(), args[0]);
      if (di != devs.end())
	{
	  outputParameters.device = di - devs.begin();
	  inputParameters.device = di - devs.begin();
          device_name = args[0];
	}
      else
	error = BSE_ERROR_DEVICE_NOT_AVAILABLE;
    }
  if (!error)
    {
      inputParameters.channelCount  = handle->n_channels;
      outputParameters.channelCount = handle->n_channels;
      inputParameters.sampleFormat  = paFloat32;
      outputParameters.sampleFormat = paFloat32;
      inputParameters.suggestedLatency  = BSE_PCM_DEVICE (device)->req_latency_ms * 0.001;
      outputParameters.suggestedLatency = BSE_PCM_DEVICE (device)->req_latency_ms * 0.001;
      inputParameters.hostApiSpecificStreamInfo  = NULL;
      outputParameters.hostApiSpecificStreamInfo = NULL;
      PaError pa_error = Pa_OpenStream (&portaudio->stream, 
                                        handle->readable ? &inputParameters : NULL,
                                        handle->writable ? &outputParameters : NULL,
                                        handle->mix_freq,
                                        pdev->req_block_length,
                                        paDitherOff,
                                        NULL,	/* no callback -> blocking api */
                                        NULL);
      if (pa_error != paNoError || !portaudio->stream)
        error = bse_error_from_pa_error (pa_error, BSE_ERROR_FILE_OPEN_FAILED);
    }
  if (!error)
    {
      PaError pa_error = Pa_StartStream (portaudio->stream);
      if (pa_error != paNoError)
        {
          error = bse_error_from_pa_error (pa_error, BSE_ERROR_FILE_OPEN_FAILED);
          Pa_CloseStream (portaudio->stream);
          portaudio->stream = NULL;
        }
    }

  if (!error)
    {
      bse_device_set_opened (device, device_name.c_str(), handle->readable, handle->writable);
      if (handle->readable)
        handle->read = port_audio_device_read;
      if (handle->writable)
        handle->write = port_audio_device_write;
      handle->check_io = port_audio_device_check_io;
      handle->latency = port_audio_device_latency;
      pdev->handle = handle;
    }
  else
    {
      if (portaudio->stream)
        Pa_CloseStream (portaudio->stream);
      g_free (portaudio);
    }
  PDEBUG ("PortAudio: opening PCM \"%s\" readable=%d writable=%d: %s", device_name.c_str(), require_readable, require_writable, bse_error_blurb (error));
  if (!error)
    PDEBUG ("PortAudio: input-latency=%fms output-latency=%fms", Pa_GetStreamInfo (portaudio->stream)->inputLatency, Pa_GetStreamInfo (portaudio->stream)->outputLatency);
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
  float *silence = (float*) g_malloc0 (portaudio->handle.block_length * sizeof (float) * portaudio->handle.n_channels);

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

  /* FIXME: should PortAudio clip? */
  float clipped_values[handle->block_length * handle->n_channels];
  for (guint i = 0; i < handle->block_length * handle->n_channels; i++)
    clipped_values[i] = CLAMP (values[i], -1.0, 1.0);

  Pa_WriteStream (portaudio->stream, clipped_values, handle->block_length);
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
  const gchar *info = g_intern_format (/* TRANSLATORS: keep this text to 70 chars in width */
                                       _("PortAudio PCM driver, using %s.\n"
                                         "  DEVICE - the PortAudio device to use, 'default' selects default device\n"
                                         "  MODE   - rw = read/write, ro = readonly, wo = writeonly\n"),
                                       Pa_GetVersionText());
  bse_device_class_setup (klass, BSE_RATING_FALLBACK, name, syntax, info);
  device_class->open = bse_pcm_device_port_audio_open;
  device_class->close = bse_pcm_device_port_audio_close;
}
