/* CSL Support for BSE
 * Copyright (C) 2003 Stefan Westerfeld
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
 *
 * bsepcmdevice-csl.c: pcm device support using the Common Sound Layer
 */
#include	"bsepcmdevice-csl.h"

#include	"../PKG_config.h"

#include	"gsldatautils.h"

#if !BSE_PCM_DEVICE_CSL_SUPPORT
BSE_DUMMY_TYPE (BsePcmDeviceCSL);
#else

#include        <csl/csl.h>
#include	<string.h>
#include	<unistd.h>
#include	<errno.h>
#include	<fcntl.h>

/* --- CSL PCM handle --- */
typedef struct
{
  BsePcmHandle	    handle;
  SfiMutex          sfi_mutex;
  CslMutex          csl_mutex;
  CslDriver        *driver;
  CslPcmStream     *input_stream;
  CslPcmStream     *output_stream;
  CslPcmFormatType  format;
  gint		    fd;
  guint		    n_frags;
  guint		    frag_size;
  guint		    bytes_per_value;
  gint16           *frag_buf;
} CSLHandle;
#define	FRAG_BUF_SIZE(csl)	((csl)->frag_size * sizeof(gfloat) * 2)


/* --- prototypes --- */
static void	    bse_pcm_device_csl_class_init	(BsePcmDeviceCSLClass	*class);
static void	    bse_pcm_device_csl_init		(BsePcmDeviceCSL	*pcm_device_csl);
static void	    bse_pcm_device_csl_finalize		(GObject		*object);
static BseErrorType bse_pcm_device_csl_open		(BsePcmDevice		*pdev);
static BseErrorType csl_device_setup			(CSLHandle		*csl);
static void	    csl_device_retrigger		(CSLHandle		*csl);
static gsize	    csl_device_read			(BsePcmHandle		*handle,
							 gsize			 n_values,
							 gfloat			*values);
static void	    csl_device_write			(BsePcmHandle		*handle,
							 gsize			 n_values,
							 const gfloat		*values);
static void	    csl_device_status			(BsePcmHandle		*handle,
							 BsePcmStatus		*status);
static void	    bse_pcm_device_csl_close		(BsePcmDevice		*pdev);


/* --- variables --- */
static gpointer parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BsePcmDeviceCSL)
{
  GType pcm_device_csl_type;
  
  static const GTypeInfo pcm_device_csl_info = {
    sizeof (BsePcmDeviceCSLClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_pcm_device_csl_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BsePcmDeviceCSL),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_pcm_device_csl_init,
  };
  
  pcm_device_csl_type = bse_type_register_static (BSE_TYPE_PCM_DEVICE,
						  "BsePcmDeviceCSL",
						  "PCM device implementation for Common Sound Layer",
						  &pcm_device_csl_info);
  return pcm_device_csl_type;
}

static void
bse_pcm_device_csl_class_init (BsePcmDeviceCSLClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BsePcmDeviceClass *pcm_device_class = BSE_PCM_DEVICE_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->finalize = bse_pcm_device_csl_finalize;
  
  pcm_device_class->open = bse_pcm_device_csl_open;
  pcm_device_class->suspend = bse_pcm_device_csl_close;
  pcm_device_class->driver_rating = BSE_RATING_DEFAULT + 1;
}

static void
bse_pcm_device_csl_init (BsePcmDeviceCSL *csl)
{
  csl->device_name = g_strdup ("csl"); /* BSE_PCM_DEVICE_CONF_CSL); */
}

static BseErrorType
bse_pcm_device_csl_open (BsePcmDevice *pdev)
{
  CSLHandle *csl = g_new0 (CSLHandle, 1);
  BsePcmHandle *handle = &csl->handle;
  BseErrorType error = BSE_ERROR_NONE;
  CslErrorType csl_error;
  
  /* setup request */
  handle->writable = TRUE;
  handle->readable = TRUE;
  handle->n_channels = 2;
  handle->mix_freq = bse_pcm_freq_from_freq_mode (pdev->req_freq_mode);
  handle->read = NULL;
  handle->write = NULL;
  handle->status = NULL;
  csl->driver = NULL;
  csl->input_stream = NULL;
  csl->output_stream = NULL;
  csl->n_frags = 64;
  csl->frag_size = 1024;
  csl->bytes_per_value = 4;
  csl->frag_buf = NULL;

  sfi_mutex_init (&csl->sfi_mutex);
  csl->csl_mutex.user_data = &csl->sfi_mutex;
  csl->csl_mutex.lock = (CslMutexLock) sfi_thread_table.mutex_lock;
  csl->csl_mutex.unlock = (CslMutexUnlock) sfi_thread_table.mutex_unlock;
  csl->csl_mutex.destroy = NULL;

  csl_error = csl_driver_init_mutex (NULL, CSL_DRIVER_CAP_PCM, &csl->csl_mutex, &csl->driver);
  if (csl_error)
    error = BSE_ERROR_FILE_OPEN_FAILED;

  /* try open */
  if (!error)
    {
      if (!error && handle->readable)
	{
	  /* open PCM input stream */
	  if (!csl->format)
	    {
	      csl->format = CSL_PCM_FORMAT_FLOAT_HE;
	      csl_error = csl_pcm_open_input (csl->driver,
		  "beast input",
		  handle->mix_freq,
		  handle->n_channels,
		  CSL_PCM_FORMAT_FLOAT_HE, &csl->input_stream);

	      if (csl_error)
		csl->format = CSL_PCM_FORMAT_S16_HE;
	    }
	  if (!csl->input_stream)
	    {
	      csl_error = csl_pcm_open_input (csl->driver,
		  "beast input",
		  handle->mix_freq,
		  handle->n_channels,
		  csl->format, &csl->input_stream);

	      if (csl_error)
		{
		  /*
		     errorMsg = "failed to open CSL input stream: ";
		     errorMsg += csl_strerror(error);
		   */
		  error = bse_error_from_errno (errno, BSE_ERROR_FILE_OPEN_FAILED);
		}
	      /* csl_pcm_set_callback(inputStream, handleRead, 0, 0); FIXME */
	    }
	}
      if (!error && handle->writable)
	{
	  /* open PCM output stream */
	  if (!csl->format)
	    {
	      csl->format = CSL_PCM_FORMAT_FLOAT_HE;
	      csl_error = csl_pcm_open_output (csl->driver,
		  "beast output",
		  handle->mix_freq,
		  handle->n_channels,
		  csl->format, &csl->output_stream);

	      if (csl_error)
		csl->format = CSL_PCM_FORMAT_S16_HE;
	    }
	  if (!csl->output_stream)
	    {
	      csl_error = csl_pcm_open_output (csl->driver,
		  "beast output",
		  handle->mix_freq,
		  handle->n_channels,
		  csl->format, &csl->output_stream);


	      if (csl_error)
		{
		  /*
		    errorMsg = "failed to open CSL output stream: ";
		    errorMsg += csl_strerror(error);
		  */
		  error = bse_error_from_errno (errno, BSE_ERROR_FILE_OPEN_FAILED);
		}
	      /* csl_pcm_set_callback(outputStream, handleWrite, 0, 0); FIXME */
	    }
	}
    }
  
  /* try setup */
  if (!error)
    error = csl_device_setup (csl);
  
  /* setup pdev or shutdown */
  if (!error)
    {
      csl->frag_buf = g_malloc (FRAG_BUF_SIZE (csl));
      handle->playback_watermark = MIN (csl->n_frags, 20) * csl->frag_size;
      BSE_OBJECT_SET_FLAGS (pdev, BSE_PCM_FLAG_OPEN);
      if (handle->readable)
	{
	  BSE_OBJECT_SET_FLAGS (pdev, BSE_PCM_FLAG_READABLE);
	  handle->read = csl_device_read;
	}
      if (handle->writable)
	{
	  BSE_OBJECT_SET_FLAGS (pdev, BSE_PCM_FLAG_WRITABLE);
	  handle->write = csl_device_write;
	}
      handle->status = csl_device_status;
      pdev->handle = handle;
    }
  else
    {
      pdev->handle = handle;
      bse_pcm_device_csl_close (pdev); /* is this okay? */
    }
  
  return error;
}

static void
bse_pcm_device_csl_finalize (GObject *object)
{
  BsePcmDeviceCSL *pdev_csl = BSE_PCM_DEVICE_CSL (object);
  
  g_free (pdev_csl->device_name);
  pdev_csl->device_name = NULL;
  
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bse_pcm_device_csl_close (BsePcmDevice *pdev)
{
  CSLHandle *csl = (CSLHandle*) pdev->handle;
  
  pdev->handle = NULL;

  if (csl->input_stream)
    csl_pcm_close (csl->input_stream);
  if (csl->output_stream)
    csl_pcm_close (csl->output_stream);
  if (csl->driver)
    csl_driver_shutdown (csl->driver);
  if (csl->csl_mutex.user_data)
    sfi_mutex_destroy (csl->csl_mutex.user_data);

  g_free (csl->frag_buf);
  g_free (csl);
}

static BseErrorType
csl_device_setup (CSLHandle *csl)
{
  BsePcmHandle *handle = &csl->handle;
  CslPcmStatus csl_status;
  CslErrorType csl_error;
  CslPcmFormatType input_format = 0, output_format = 0;

  /* check which formats input_stream and output_stream are using */
  if (csl->input_stream)
    csl->format = input_format = csl_pcm_get_format (csl->input_stream);
  if (csl->output_stream)
    csl->format = output_format = csl_pcm_get_format (csl->output_stream);
  if (csl->input_stream && csl->output_stream && input_format != output_format)
    return BSE_ERROR_DEVICE_CAPS_MISMATCH;

  /* we currently support float and linear signed 16bit samples */
  if (csl->format == CSL_PCM_FORMAT_FLOAT_HE)
    csl->bytes_per_value = 4;
  else if (csl->format == CSL_PCM_FORMAT_S16_HE)
    csl->bytes_per_value = 2;
  else
    return BSE_ERROR_DEVICE_CAPS_MISMATCH;

  if (csl->input_stream)
    {
      csl_error = csl_pcm_get_status (csl->input_stream, &csl_status);
      if (csl_error)
	return BSE_ERROR_DEVICE_GET_CAPS;
    }
  else if (csl->output_stream)
    {
      csl_error = csl_pcm_get_status (csl->output_stream, &csl_status);
      if (csl_error)
	return BSE_ERROR_DEVICE_GET_CAPS;
    }
  else
    {
      return BSE_ERROR_INTERNAL;
    }

  handle->n_channels = csl_status.n_channels;
  handle->mix_freq = csl_status.rate;

  /* Note: fragment = n_fragments << 16;
   *       fragment |= g_bit_storage (fragment_size - 1);
   */
  csl->frag_size = CLAMP (csl->frag_size, 128, 65536);
  csl->n_frags = CLAMP (csl->n_frags, 128, 65536);

  if (csl->input_stream)
    {
      csl_error = csl_pcm_set_packet_mode (csl->input_stream,
	  csl->n_frags, csl->frag_size, csl->frag_size);

      if (csl_error)
	return BSE_ERROR_DEVICE_SET_CAPS;
    }
  if (csl->output_stream)
    {
      csl_error = csl_pcm_set_packet_mode (csl->output_stream,
	  csl->n_frags, csl->frag_size, csl->frag_size);

      if (csl_error)
	return BSE_ERROR_DEVICE_SET_CAPS;
    }
  {
    guint n_packets, packet_size, watermark;

    if (csl->input_stream)
      {
	csl_pcm_get_packet_settings (csl->input_stream, &n_packets, &packet_size, &watermark);
      }
    else if (csl->output_stream)
      {
	csl_pcm_get_packet_settings (csl->output_stream, &n_packets, &packet_size, &watermark);
      }
      csl->frag_size = packet_size;
      csl->n_frags = n_packets;
    }
  
  BSE_IF_DEBUG (PCM)
    g_message ("CSL-SETUP: w=%d r=%d n_channels=%d sample_freq=%.0f fsize=%u nfrags=%u bufsz=%u\n",
	       handle->writable,
	       handle->readable,
	       handle->n_channels,
	       handle->mix_freq,
	       csl->frag_size,
	       csl->n_frags,
	       csl->n_frags * csl->frag_size);
  
  return BSE_ERROR_NONE;
}

static void
csl_device_retrigger (CSLHandle *csl)
{
  /* FIXME */
}

static void
csl_device_status (BsePcmHandle *handle,
		   BsePcmStatus *status)
{
  CSLHandle *csl = (CSLHandle*) handle;
  CslPcmStatus csl_status;
  CslErrorType csl_error;
  
  if (handle->readable)
    {
      memset (&csl_status, 0, sizeof (csl_status));
      csl_error = csl_pcm_get_status (csl->input_stream, &csl_status);
      if (!csl_error)
	{
	  status->total_capture_values = csl_status.buffer_size / csl->bytes_per_value;
	  status->n_capture_values_available = csl_status.n_bytes_available / csl->bytes_per_value;
	}

      /* OSS-bug fix, at least for es1371 in 2.3.34 */
      status->n_capture_values_available =
	MIN (status->total_capture_values, status->n_capture_values_available);

      BSE_IF_DEBUG (PCM)
	g_message ("CSL-ISPACE: total=%d size=%d\n",
		   status->n_capture_values_available,
		   status->total_capture_values);
    }
  else
    {
      status->total_capture_values = 0;
      status->n_capture_values_available = 0;
    }
  if (handle->writable)
    {
      memset (&csl_status, 0, sizeof (csl_status));
      csl_error = csl_pcm_get_status (csl->output_stream, &csl_status);
      if (!csl_error)
	{
	  status->total_playback_values = csl_status.buffer_size / csl->bytes_per_value;
	  status->n_playback_values_available = csl_status.n_bytes_available / csl->bytes_per_value;
	}

      /* OSS-bug fix, at least for es1371 in 2.3.34 */
      status->n_playback_values_available =
	MIN (status->total_playback_values, status->n_playback_values_available);

      BSE_IF_DEBUG (PCM)
	g_message ("CSL-OSPACE: total=%d size=%d\n",
		   status->n_playback_values_available,
		   status->total_playback_values);
    }
  else
    {
      status->total_playback_values = 0;
      status->n_playback_values_available = 0;
    }
}

static gsize
csl_device_read_s16 (BsePcmHandle *handle,
		     gsize         n_values,
		     gfloat       *values)
{
  CSLHandle *csl = (CSLHandle*) handle;
  gsize buf_size = FRAG_BUF_SIZE (csl);
  gpointer buf = csl->frag_buf;
  gfloat *d = values;
  gsize n_left = n_values;
  
  g_return_val_if_fail (csl->bytes_per_value == 2, 0);
  
  do
    {
      gsize n = MIN (buf_size, n_left << 1);
      gint16 *b, *s = buf;
      gssize l;
      
      do
	l = csl_pcm_read (csl->input_stream, n, buf);
      while (l < 0 && errno == EINTR); /* don't mind signals */
      if (l < 0) /* sigh, errors during read? */
	{
	  memset (buf, 0, n);
	  l = n;
	}
      l >>= 1;
      for (b = s + l; s < b; s++)
	*d++ = *s * (1.0 / 32768.0);
      n_left -= l;
    }
  while (n_left);
  
  return n_values;
}

static gsize
csl_device_read (BsePcmHandle *handle,
		 gsize         n_values,
		 gfloat       *values)
{
  CSLHandle *csl = (CSLHandle*) handle;
  guint8 *buf = (guint8 *)values;

  if (csl->format == CSL_PCM_FORMAT_S16_HE)
    return csl_device_read_s16 (handle, n_values, values);

  g_return_val_if_fail (csl->bytes_per_value == 4, 0);
  
  do
    {
      gssize l;
      gsize n = n_values * csl->bytes_per_value;
     
      do
	l = csl_pcm_read (csl->input_stream, n, buf);
      while (l < 0 && errno == EINTR); /* don't mind signals */
      if (l < 0) /* sigh, errors during write? */
	l = n;
      n_values -= l >> 2;
      buf += l;
    }
  while (n_values);
  
  return (buf - (guint8*)values);
}

static void
csl_device_write (BsePcmHandle *handle,
		  gsize         n_values,
		  const gfloat *values)
{
  CSLHandle *csl = (CSLHandle*) handle;
  gsize buf_size = FRAG_BUF_SIZE (csl);
  gpointer buf = csl->frag_buf;
  const gfloat *s = values;

  g_return_if_fail (csl->format == CSL_PCM_FORMAT_FLOAT_HE
                 || csl->format == CSL_PCM_FORMAT_S16_HE);

  do
    {
      gsize n = MIN (buf_size, n_values * csl->bytes_per_value);
      gssize l;

      if (csl->format == CSL_PCM_FORMAT_S16_HE)
	{
	  gsl_conv_from_float_clip (GSL_WAVE_FORMAT_SIGNED_16,
				    G_BYTE_ORDER,
				    s,
				    buf,
				    n / csl->bytes_per_value);
	}
      else /* CSL_PCM_FORMAT_FLOAT_HE */
	{
	  buf = (const guint8*) s;
	}

      s += n / csl->bytes_per_value;
      do
	l = csl_pcm_write (csl->output_stream, n, buf);
      while (l < 0 && errno == EINTR); /* don't mind signals */
      if (l < 0) /* sigh, errors during write? */
	l = n;
      n_values -= l / csl->bytes_per_value;
    }
  while (n_values);
}

#endif	/* HAVE_LIBCSL */
/* vim:set ts=8 sw=2 sts=2: */
