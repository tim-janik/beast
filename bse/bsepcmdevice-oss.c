/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1996-1999, 2000-2001 Tim Janik
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
#include	"bsepcmdevice-oss.h"

#include	"../PKG_config.h"

#ifndef	BSE_PCM_DEVICE_CONF_OSS
BSE_DUMMY_TYPE (BsePcmDeviceOSS);
#else   /* BSE_PCM_DEVICE_CONF_OSS */

#include	<sys/soundcard.h>
#include	<sys/ioctl.h>
#include	<sys/types.h>
#include	<sys/time.h>
#include	<unistd.h>
#include	<errno.h>
#include	<fcntl.h>

#if	G_BYTE_ORDER == G_LITTLE_ENDIAN
#define	AFMT_S16_HE	AFMT_S16_LE
#elif	G_BYTE_ORDER == G_BIG_ENDIAN
#define	AFMT_S16_HE	AFMT_S16_BE
#else
#error	unsupported byte order in G_BYTE_ORDER
#endif


/* --- OSS PCM handle --- */
typedef struct
{
  BsePcmHandle	handle;
  gint		fd;
  guint		n_frags;
  guint		frag_size;
  guint		bytes_per_value;
  gint16       *frag_buf;
} OSSHandle;
#define	FRAG_BUF_SIZE(oss)	((oss)->frag_size * 4)


/* --- prototypes --- */
static void	    bse_pcm_device_oss_class_init	(BsePcmDeviceOSSClass	*class);
static void	    bse_pcm_device_oss_init		(BsePcmDeviceOSS	*pcm_device_oss);
static void	    bse_pcm_device_oss_destroy		(BseObject		*object);
static BseErrorType bse_pcm_device_oss_open		(BsePcmDevice		*pdev);
static BseErrorType oss_device_setup			(OSSHandle		*oss);
static void	    oss_device_retrigger		(OSSHandle		*oss);
static void	    oss_device_read			(BsePcmHandle		*handle,
							 gsize			 n_values,
							 BseSampleValue		*values);
static void	    oss_device_write			(BsePcmHandle		*handle,
							 gsize			 n_values,
							 const BseSampleValue	*values);
static void	    oss_device_status			(BsePcmHandle		*handle,
							 BsePcmStatus		*status);
static void	    bse_pcm_device_oss_close		(BsePcmDevice		*pdev);


/* --- variables --- */
static gpointer parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BsePcmDeviceOSS)
{
  GType pcm_device_oss_type;
  
  static const GTypeInfo pcm_device_oss_info = {
    sizeof (BsePcmDeviceOSSClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_pcm_device_oss_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BsePcmDeviceOSS),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_pcm_device_oss_init,
  };
  
  pcm_device_oss_type = bse_type_register_static (BSE_TYPE_PCM_DEVICE,
						  "BsePcmDeviceOSS",
						  "PCM device implementation for OSS Lite /dev/dsp",
						  &pcm_device_oss_info);
  return pcm_device_oss_type;
}

static void
bse_pcm_device_oss_class_init (BsePcmDeviceOSSClass *class)
{
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BsePcmDeviceClass *pcm_device_class = BSE_PCM_DEVICE_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  object_class->destroy = bse_pcm_device_oss_destroy;

  pcm_device_class->open = bse_pcm_device_oss_open;
  pcm_device_class->suspend = bse_pcm_device_oss_close;
  pcm_device_class->driver_rating = 1;
}

static void
bse_pcm_device_oss_init (BsePcmDeviceOSS *oss)
{
  oss->device_name = g_strdup (BSE_PCM_DEVICE_CONF_OSS);
}

static BseErrorType
bse_pcm_device_oss_open (BsePcmDevice *pdev)
{
  OSSHandle *oss = g_new0 (OSSHandle, 1);
  BsePcmHandle *handle = &oss->handle;
  BseErrorType error = BSE_ERROR_NONE;

  /* setup request */
  handle->writable = TRUE;
  handle->readable = TRUE;
  handle->n_channels = 2;
  handle->mix_freq = bse_pcm_freq_from_freq_mode (pdev->req_freq_mode);
  handle->read = NULL;
  handle->write = NULL;
  handle->status = NULL;
  oss->fd = -1;
  oss->n_frags = 256;
  oss->frag_size = 512;
  oss->bytes_per_value = 2;
  oss->frag_buf = NULL;

  /* try open */
  if (!error)
    {
      gint omode = 0;
      gint fd;
      
      omode = (handle->readable && handle->writable ? O_RDWR
	       : handle->readable ? O_RDONLY
	       : handle->writable ? O_WRONLY : 0);
      
      /* need to open explicitely non-blocking or we'll have to wait untill someone else closes the device */
      fd = open (BSE_PCM_DEVICE_OSS (pdev)->device_name, omode | O_NONBLOCK, 0);
      if (fd >= 0)
	oss->fd = fd;
      else
	{
	  if (errno == EBUSY)
	    error = BSE_ERROR_DEVICE_BUSY;
	  else if (errno == EISDIR || errno == EACCES || errno == EROFS)
	    error = BSE_ERROR_DEVICE_PERMS;
	  else
	    error = BSE_ERROR_DEVICE_IO;
	}
    }

  /* try setup */
  if (!error)
    error = oss_device_setup (oss);

  /* setup pdev or shutdown */
  if (!error)
    {
      oss->frag_buf = g_malloc (FRAG_BUF_SIZE (oss));
      handle->playback_watermark = MIN (oss->n_frags, 5) * oss->frag_size;
      BSE_OBJECT_SET_FLAGS (pdev, BSE_PCM_FLAG_OPEN);
      if (handle->readable)
	{
	  BSE_OBJECT_SET_FLAGS (pdev, BSE_PCM_FLAG_READABLE);
	  handle->read = oss_device_read;
	}
      if (handle->writable)
	{
	  BSE_OBJECT_SET_FLAGS (pdev, BSE_PCM_FLAG_WRITABLE);
	  handle->write = oss_device_write;
	}
      handle->status = oss_device_status;
      pdev->handle = handle;
    }
  else
    {
      if (oss->fd < 0)
	close (oss->fd);
      g_free (oss->frag_buf);
      g_free (oss);
    }

  return error;
}

static void
bse_pcm_device_oss_destroy (BseObject *object)
{
  BsePcmDeviceOSS *pdev_oss = BSE_PCM_DEVICE_OSS (object);

  g_free (pdev_oss->device_name);
  pdev_oss->device_name = NULL;
  
  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bse_pcm_device_oss_close (BsePcmDevice *pdev)
{
  OSSHandle *oss = (OSSHandle*) pdev->handle;

  pdev->handle = NULL;
  (void) ioctl (oss->fd, SNDCTL_DSP_RESET);
  (void) close (oss->fd);
  g_free (oss->frag_buf);
  g_free (oss);
}

static BseErrorType
oss_device_setup (OSSHandle *oss)
{
  BsePcmHandle *handle = &oss->handle;
  gint fd = oss->fd;
  glong d_long;
  gint d_int;

  d_long = fcntl (fd, F_GETFL);
  d_long &= ~O_NONBLOCK;
  if (fcntl (fd, F_SETFL, d_long))
    return BSE_ERROR_DEVICE_ASYNC;

  d_int = 0;
  if (ioctl (fd, SNDCTL_DSP_GETFMTS, &d_int) < 0)
    return BSE_ERROR_DEVICE_GET_CAPS;
  if ((d_int & AFMT_S16_HE) != AFMT_S16_HE)
    return BSE_ERROR_DEVICE_CAPS_MISMATCH;
  d_int = AFMT_S16_HE;
  if (ioctl (fd, SNDCTL_DSP_SETFMT, &d_int) < 0 ||
      d_int != AFMT_S16_HE)
    return BSE_ERROR_DEVICE_SET_CAPS;
  oss->bytes_per_value = 2;

  d_int = handle->n_channels - 1;
  if (ioctl (fd, SNDCTL_DSP_STEREO, &d_int) < 0)
    return BSE_ERROR_DEVICE_SET_CAPS;
  handle->n_channels = d_int + 1;
  
  d_int = handle->mix_freq;
  if (ioctl (fd, SNDCTL_DSP_SPEED, &d_int) < 0)
    return BSE_ERROR_DEVICE_SET_CAPS;
  handle->mix_freq = d_int;

  /* Note: fragment = n_fragments << 16;
   *       fragment |= g_bit_storage (fragment_size - 1);
   */
  oss->frag_size = CLAMP (oss->frag_size, 128, 65536);
  oss->n_frags = CLAMP (oss->n_frags, 128, 65536);
  d_int = (oss->n_frags << 16) | g_bit_storage (oss->frag_size - 1);
  if (ioctl (fd, SNDCTL_DSP_SETFRAGMENT, &d_int) < 0)
    return BSE_ERROR_DEVICE_SET_CAPS;
  
  d_int = 0;
  if (ioctl (fd, SNDCTL_DSP_GETBLKSIZE, &d_int) < 0 ||
      d_int < 128 ||
      d_int > 131072 ||
      (d_int & 1))
    return BSE_ERROR_DEVICE_GET_CAPS;
  /* handle->block_size = d_int; */

  if (handle->writable)
    {
      audio_buf_info info = { 0, };

      if (ioctl (fd, SNDCTL_DSP_GETOSPACE, &info) < 0)
	return BSE_ERROR_DEVICE_GET_CAPS;
      oss->frag_size = info.fragsize;
      oss->n_frags = info.fragstotal;
    }
  else if (handle->readable)
    {
      audio_buf_info info = { 0, };

      if (ioctl (fd, SNDCTL_DSP_GETISPACE, &info) < 0)
	return BSE_ERROR_DEVICE_GET_CAPS;
      oss->frag_size = info.fragsize;
      oss->n_frags = info.fragstotal;
    }
  
  BSE_IF_DEBUG (PCM)
    g_message ("OSS-SETUP: w=%d r=%d n_channels=%d sample_freq=%.0f fsize=%u nfrags=%u bufsz=%u\n",
	       handle->writable,
	       handle->readable,
	       handle->n_channels,
	       handle->mix_freq,
	       oss->frag_size,
	       oss->n_frags,
	       oss->n_frags * oss->frag_size);

  return BSE_ERROR_NONE;
}

static void
oss_device_retrigger (OSSHandle *oss)
{
  gint d_int;

  /* it should be enough to select() on the fd to trigger
   * capture/playback, but with some new OSS drivers
   * (clones) this is not the case anymore, so we also
   * use the SNDCTL_DSP_SETTRIGGER ioctl to achive this.
   */
  
  d_int = 0;
  if (oss->handle.readable)
    d_int |= PCM_ENABLE_INPUT;
  if (oss->handle.writable)
    d_int |= PCM_ENABLE_OUTPUT;
  (void) ioctl (oss->fd, SNDCTL_DSP_SETTRIGGER, &d_int);

  /* Jaroslav Kysela <perex@jcu.cz>:
   *  Important sequence:
   *     1) turn on capture
   *     2) write at least one fragment to create appropriate delay between
   *        capture and playback; we need some time for process wakeup and
   *        data copy in the user space
   * e.g.:
   * we write two fragments to playback
   * software latency is:
   *   2 * frag_size + delay between capture start & playback start
   *
   * TIMJ:
   *   we only need to ensure that input is actually triggered, Bse's engine
   *   takes care of the latency and at this point probably already has
   *   output buffers pending.
   */
  if (oss->handle.readable)
    {
      struct timeval tv = { 0, 0, };
      fd_set in_fds;
      fd_set out_fds;

      FD_ZERO (&in_fds);
      FD_ZERO (&out_fds);
      FD_SET (oss->fd, &in_fds);
      /* FD_SET (dev->pfd.fd, &out_fds); */
      select (oss->fd + 1, &in_fds, &out_fds, NULL, &tv);
    }
}

static void
oss_device_status (BsePcmHandle *handle,
		   BsePcmStatus *status)
{
  OSSHandle *oss = (OSSHandle*) handle;
  gint fd = oss->fd;
  audio_buf_info info;

  if (handle->readable)
    {
      memset (&info, 0, sizeof (info));
      (void) ioctl (fd, SNDCTL_DSP_GETISPACE, &info);
      status->total_capture_values = info.fragstotal * info.fragsize / oss->bytes_per_value;
      status->n_capture_values_available = info.fragments * info.fragsize / oss->bytes_per_value;
      /* probably more accurate: */
      status->n_capture_values_available = info.bytes / oss->bytes_per_value;
      /* OSS-bug fix, at least for es1371 in 2.3.34 */
      status->n_capture_values_available = MIN (status->total_capture_values, status->n_capture_values_available);
      BSE_IF_DEBUG (PCM)
	g_message ("OSS-ISPACE: left=%5d/%d frags: total=%d size=%d count=%d bytes=%d\n",
		   status->n_capture_values_available,
		   status->total_capture_values,
		   info.fragstotal,
		   info.fragsize,
		   info.fragments,
		   info.bytes);
    }
  else
    {
      status->total_capture_values = 0;
      status->n_capture_values_available = 0;
    }
  if (handle->writable)
    {
      memset (&info, 0, sizeof (info));
      (void) ioctl (fd, SNDCTL_DSP_GETOSPACE, &info);
      status->total_playback_values = info.fragstotal * info.fragsize / oss->bytes_per_value;
      status->n_playback_values_available = info.fragments * info.fragsize / oss->bytes_per_value;
      /* probably more accurate: */
      status->n_playback_values_available = info.bytes / oss->bytes_per_value;
      /* OSS-bug fix, at least for es1371 in 2.3.34 */
      status->n_playback_values_available = MIN (status->total_playback_values, status->n_playback_values_available);
      BSE_IF_DEBUG (PCM)
	g_message ("OSS-OSPACE: left=%5d/%d frags: total=%d size=%d count=%d bytes=%d\n",
		   status->n_playback_values_available,
		   status->total_playback_values,
		   info.fragstotal,
		   info.fragsize,
		   info.fragments,
		   info.bytes);
    }
  else
    {
      status->total_playback_values = 0;
      status->n_playback_values_available = 0;
    }
}

static void
oss_device_read (BsePcmHandle   *handle,
		 gsize           n_values,
		 BseSampleValue *values)
{
  OSSHandle *oss = (OSSHandle*) handle;
  gint fd = oss->fd;
  gsize buf_size = FRAG_BUF_SIZE (oss);
  gpointer buf = oss->frag_buf;
  BseSampleValue *d = values;

  g_return_if_fail (oss->bytes_per_value == 2);

  do
    {
      gsize n = MIN (buf_size, n_values << 1);
      gint16 *b, *s = buf;
      gssize l;

      do
	l = read (fd, buf, n);
      while (l < 0 && errno == EINTR); /* don't mind signals */
      if (l < 0) /* sigh, errors during read? */
	{
	  memset (buf, 0, n);
	  l = n;
	}
      l >>= 1;
      for (b = s + l; s < b; s++)
	*d++ = *s * (1.0 / 32768.0);
      n_values -= l;
    }
  while (n_values);
}

static void
oss_device_write (BsePcmHandle         *handle,
		  gsize                 n_values,
		  const BseSampleValue *values)
{
  OSSHandle *oss = (OSSHandle*) handle;
  gint fd = oss->fd;
  gsize buf_size = FRAG_BUF_SIZE (oss);
  gpointer buf = oss->frag_buf;
  const BseSampleValue *s = values;

  g_return_if_fail (oss->bytes_per_value == 2);

  do
    {
      gsize n = MIN (buf_size, n_values << 1);
      gint16 *b, *d = buf;
      gssize l;

      for (b = d + (n >> 1); d < b; d++)
	{
	  BseSampleValue v = *s++;

	  *d = v > 1.0 ? 32767 : v < -1.0 ? -32767 : (gint16) (v * 32767.0);
	}
      do
	l = write (fd, buf, n);
      while (l < 0 && errno == EINTR); /* don't mind signals */
      if (l < 0) /* sigh, errors during write? */
	l = n;
      n_values -= l >> 1;
    }
  while (n_values);
}

#endif	/* BSE_PCM_DEVICE_CONF_OSS */
