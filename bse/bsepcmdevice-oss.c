/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1996-1999, 2000-2002 Tim Janik
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

#include	"topconfig.h"

#include	"gsldatautils.h"

#ifndef	BSE_PCM_DEVICE_CONF_OSS
BSE_DUMMY_TYPE (BsePcmDeviceOSS);
#else   /* BSE_PCM_DEVICE_CONF_OSS */

#if HAVE_SYS_SOUNDCARD_H
#include	<sys/soundcard.h>
#elif HAVE_SOUNDCARD_H
#include	<soundcard.h>
#endif
#include	<sys/ioctl.h>
#include	<sys/types.h>
#include	<sys/time.h>
#include	<unistd.h>
#include	<string.h>
#include	<errno.h>
#include	<fcntl.h>

#if	G_BYTE_ORDER == G_LITTLE_ENDIAN
#define	AFMT_S16_HE	AFMT_S16_LE
#elif	G_BYTE_ORDER == G_BIG_ENDIAN
#define	AFMT_S16_HE	AFMT_S16_BE
#else
#error	unsupported byte order in G_BYTE_ORDER
#endif

#define OSS_DEBUG(...)          sfi_debug ("oss", __VA_ARGS__)
#define LATENCY_DEBUG(...)      sfi_debug ("latency", __VA_ARGS__)


/* --- OSS PCM handle --- */
typedef struct
{
  BsePcmHandle	handle;
  gint		fd;
  guint		n_frags;
  guint		frag_size;
  guint		bytes_per_value;
  gint16       *frag_buf;
  gboolean      needs_trigger;
} OSSHandle;
#define	FRAG_BUF_SIZE(oss)	((oss)->frag_size * 4)


/* --- prototypes --- */
static BseErrorType oss_device_setup			(OSSHandle		*oss);
static void	    oss_device_retrigger		(OSSHandle		*oss);
static gsize	    oss_device_read			(BsePcmHandle		*handle,
							 gsize			 n_values,
							 gfloat			*values);
static void	    oss_device_write			(BsePcmHandle		*handle,
							 gsize			 n_values,
							 const gfloat		*values);
static void	    oss_device_status			(BsePcmHandle		*handle,
							 BsePcmStatus		*status);


/* --- variables --- */
static gpointer parent_class = NULL;


/* --- functions --- */
static void
bse_pcm_device_oss_init (BsePcmDeviceOSS *oss)
{
  oss->device_name = g_strdup (BSE_PCM_DEVICE_CONF_OSS);
}

static BseErrorType
check_device_usage (const gchar *name,
                    const gchar *check_mode)
{
  BseErrorType error = gsl_check_file (name, check_mode);
  if (!error && strchr (check_mode, 'w'))
    {
      errno = 0;
      /* beware, some drivers panic on O_RDONLY */
      gint fd = open (name, O_WRONLY | O_NONBLOCK, 0);  /* open non blocking to avoid waiting for other clients */
      /* we only check for ENODEV here, since the mode
       * might be wrong and the device may be busy.
       */
      if (errno == ENODEV)
        error = BSE_ERROR_DEVICE_NOT_AVAILABLE;
      if (fd >= 0)
        close (fd);
    }
  return error;
}

static SfiRing*
bse_pcm_device_oss_list_devices (BseDevice    *device)
{
  const gchar *postfixes[] = { "", "0", "1", "2", "3" };
  SfiRing *ring = NULL;
  guint i;
  gchar *last = NULL;
  for (i = 0; i < G_N_ELEMENTS (postfixes); i++)
    {
      gchar *dname = g_strconcat (BSE_PCM_DEVICE_OSS (device)->device_name, postfixes[i], NULL);
      if (!gsl_check_file_equals (last, dname))
        {
          if (check_device_usage (dname, "crw") == GSL_ERROR_NONE)
            ring = sfi_ring_append (ring,
                                    bse_device_entry_new (device,
                                                          g_strdup_printf ("%s,rw", dname),
                                                          g_strdup_printf ("%s (read-write)", dname)));
          else if (check_device_usage (dname, "cw") == GSL_ERROR_NONE)
            ring = sfi_ring_append (ring,
                                    bse_device_entry_new (device,
                                                          g_strdup_printf ("%s,wo", dname),
                                                          g_strdup_printf ("%s (write only)", dname)));
        }
      g_free (last);
      last = dname;
    }
  g_free (last);
  if (!ring)
    ring = sfi_ring_append (ring, bse_device_error_new (device, g_strdup_printf ("No devices found")));
  return ring;
}

static BseErrorType
bse_pcm_device_oss_open (BseDevice     *device,
                         gboolean       require_readable,
                         gboolean       require_writable,
                         guint          n_args,
                         const gchar  **args)
{
  const gchar *dname;
  if (n_args >= 1)      /* DEVICE */
    dname = args[0];
  else
    dname = BSE_PCM_DEVICE_OSS (device)->device_name;
  gint omode, retry_mode = 0;
  if (n_args >= 2)      /* MODE */
    omode = strcmp (args[1], "rw") == 0 ? O_RDWR : strcmp (args[1], "ro") == 0 ? O_RDONLY : O_WRONLY;   /* parse: ro rw wo */
  else
    {
      omode = O_RDWR;
      retry_mode = O_WRONLY;
    }
  OSSHandle *oss = g_new0 (OSSHandle, 1);
  BsePcmHandle *handle = &oss->handle;
  
  /* setup request */
  handle->n_channels = 2;
  handle->mix_freq = bse_pcm_freq_from_freq_mode (BSE_PCM_DEVICE (device)->req_freq_mode);
  handle->read = NULL;
  handle->write = NULL;
  handle->status = NULL;
  oss->n_frags = 1024;
  oss->frag_size = 128;
  oss->bytes_per_value = 2;
  oss->frag_buf = NULL;
  oss->fd = -1;
  oss->needs_trigger = TRUE;

  /* try open */
  BseErrorType error;
  gint fd = -1;
  handle->readable = (omode & O_RDWR) == O_RDWR || (omode & O_RDONLY) == O_RDONLY;
  handle->writable = (omode & O_RDWR) == O_RDWR || (omode & O_WRONLY) == O_WRONLY;
  if ((handle->readable || !require_readable) && (handle->writable || !require_writable))
    fd = open (dname, omode | O_NONBLOCK, 0);           /* open non blocking to avoid waiting for other clients */
  if (fd < 0 && retry_mode)
    {
      omode = retry_mode;
      handle->writable = (omode & O_RDWR) == O_RDWR || (omode & O_WRONLY) == O_WRONLY;
      handle->readable = (omode & O_RDWR) == O_RDWR || (omode & O_RDONLY) == O_RDONLY;
      if ((handle->readable || !require_readable) && (handle->writable || !require_writable))
        fd = open (dname, omode | O_NONBLOCK, 0);       /* open non blocking to avoid waiting for other clients */
    }
  if (fd >= 0)
    {
      oss->fd = fd;
      /* try setup */
      error = oss_device_setup (oss);
    }
  else
    error = bse_error_from_errno (errno, BSE_ERROR_FILE_OPEN_FAILED);
  
  /* setup pdev or shutdown */
  if (!error)
    {
      oss->frag_buf = g_malloc (FRAG_BUF_SIZE (oss));
      handle->minimum_watermark = oss->frag_size / oss->bytes_per_value;
      handle->playback_watermark = MIN (oss->n_frags, 5) * oss->frag_size / oss->bytes_per_value;
      BSE_OBJECT_SET_FLAGS (device, BSE_DEVICE_FLAG_OPEN);
      if (handle->readable)
	{
	  BSE_OBJECT_SET_FLAGS (device, BSE_DEVICE_FLAG_READABLE);
	  handle->read = oss_device_read;
	}
      if (handle->writable)
	{
	  BSE_OBJECT_SET_FLAGS (device, BSE_DEVICE_FLAG_WRITABLE);
	  handle->write = oss_device_write;
	}
      handle->status = oss_device_status;
      BSE_PCM_DEVICE (device)->handle = handle;
    }
  else
    {
      if (oss->fd >= 0)
	close (oss->fd);
      g_free (oss->frag_buf);
      g_free (oss);
    }
  
  return error;
}

static void
bse_pcm_device_oss_close (BseDevice *device)
{
  OSSHandle *oss = (OSSHandle*) BSE_PCM_DEVICE (device)->handle;
  BSE_PCM_DEVICE (device)->handle = NULL;

  (void) ioctl (oss->fd, SNDCTL_DSP_RESET, NULL);
  (void) close (oss->fd);
  g_free (oss->frag_buf);
  g_free (oss);
}

static void
bse_pcm_device_oss_finalize (GObject *object)
{
  BsePcmDeviceOSS *pdev_oss = BSE_PCM_DEVICE_OSS (object);
  
  g_free (pdev_oss->device_name);
  pdev_oss->device_name = NULL;
  
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
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
    return BSE_ERROR_DEVICE_FORMAT;
  if ((d_int & AFMT_S16_HE) != AFMT_S16_HE)
    return BSE_ERROR_DEVICE_FORMAT;
  d_int = AFMT_S16_HE;
  if (ioctl (fd, SNDCTL_DSP_SETFMT, &d_int) < 0 ||
      d_int != AFMT_S16_HE)
    return BSE_ERROR_DEVICE_FORMAT;
  oss->bytes_per_value = 2;
  
  d_int = handle->n_channels - 1;
  if (ioctl (fd, SNDCTL_DSP_STEREO, &d_int) < 0)
    return BSE_ERROR_DEVICE_CHANNELS;
  handle->n_channels = d_int + 1;
  
  d_int = handle->mix_freq;
  if (ioctl (fd, SNDCTL_DSP_SPEED, &d_int) < 0)
    return BSE_ERROR_DEVICE_FREQUENCY;
  handle->mix_freq = d_int;
  
  /* Note: fragment = n_fragments << 16;
   *       fragment |= g_bit_storage (fragment_size - 1);
   */
  oss->frag_size = CLAMP (oss->frag_size, 128, 65536);
  oss->n_frags = CLAMP (oss->n_frags, 128, 65536);
  d_int = (oss->n_frags << 16) | g_bit_storage (oss->frag_size - 1);
  if (ioctl (fd, SNDCTL_DSP_SETFRAGMENT, &d_int) < 0)
    return BSE_ERROR_DEVICE_LATENCY;
  
  d_int = 0;
  if (ioctl (fd, SNDCTL_DSP_GETBLKSIZE, &d_int) < 0 ||
      d_int < 128 ||
      d_int > 131072 ||
      (d_int & 1))
    return BSE_ERROR_DEVICE_BUFFER;
  /* handle->block_size = d_int; */
  
  if (handle->writable)
    {
      audio_buf_info info = { 0, };
      
      if (ioctl (fd, SNDCTL_DSP_GETOSPACE, &info) < 0)
	return BSE_ERROR_DEVICE_BUFFER;
      oss->frag_size = info.fragsize;
      oss->n_frags = info.fragstotal;
    }
  else if (handle->readable)
    {
      audio_buf_info info = { 0, };
      
      if (ioctl (fd, SNDCTL_DSP_GETISPACE, &info) < 0)
	return BSE_ERROR_DEVICE_BUFFER;
      oss->frag_size = info.fragsize;
      oss->n_frags = info.fragstotal;
    }
  
  OSS_DEBUG ("OSS-SETUP: w=%d r=%d n_channels=%d sample_freq=%.0f fsize=%u nfrags=%u bufsz=%u\n",
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

  oss->needs_trigger = FALSE;
}

static void
oss_device_status (BsePcmHandle *handle,
		   BsePcmStatus *status)
{
  OSSHandle *oss = (OSSHandle*) handle;
  gint fd = oss->fd;
  audio_buf_info info;
  
  if (handle->writable && oss->needs_trigger)
    oss_device_retrigger (oss);

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
      LATENCY_DEBUG ("OSS-ISPACE: left=%5d/%d frags: total=%d size=%d count=%d bytes=%d\n",
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
      guint value_count, frag_fill;
      memset (&info, 0, sizeof (info));
      (void) ioctl (fd, SNDCTL_DSP_GETOSPACE, &info);
      status->total_playback_values = info.fragstotal * info.fragsize / oss->bytes_per_value;
      frag_fill = info.fragments * info.fragsize / oss->bytes_per_value;
      value_count = info.bytes / oss->bytes_per_value;  /* probably more accurate */
      status->n_playback_values_available = MAX (frag_fill, value_count);
      /* OSS-bug fix, at least for es1371 in 2.3.34 */
      status->n_playback_values_available = MIN (status->total_playback_values, status->n_playback_values_available);
      LATENCY_DEBUG ("OSS-OSPACE: left=%5d/%d frags: total=%d size=%d count=%d bytes=%d\n",
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

static gsize
oss_device_read (BsePcmHandle *handle,
		 gsize         n_values,
		 gfloat       *values)
{
  OSSHandle *oss = (OSSHandle*) handle;
  gint fd = oss->fd;
  gsize buf_size = FRAG_BUF_SIZE (oss);
  gpointer buf = oss->frag_buf;
  gfloat *d = values;
  gsize n_left = n_values;
  
  g_return_val_if_fail (oss->bytes_per_value == 2, 0);
  
  do
    {
      gsize n = MIN (buf_size, n_left << 1);
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
      n_left -= l;
    }
  while (n_left);
  
  return n_values;
}

static void
oss_device_write (BsePcmHandle *handle,
		  gsize         n_values,
		  const gfloat *values)
{
  OSSHandle *oss = (OSSHandle*) handle;
  gint fd = oss->fd;
  gsize buf_size = FRAG_BUF_SIZE (oss);
  gpointer buf = oss->frag_buf;
  const gfloat *s = values;
  
  g_return_if_fail (oss->bytes_per_value == 2);
  
  do
    {
      gsize n = MIN (buf_size, n_values << 1);
      gssize l;
      
      gsl_conv_from_float_clip (GSL_WAVE_FORMAT_SIGNED_16,
				G_BYTE_ORDER,
				s,
				buf,
				n >> 1);
      s += n >> 1;
      do
	l = write (fd, buf, n);
      while (l < 0 && errno == EINTR); /* don't mind signals */
      if (l < 0) /* sigh, errors during write? */
	l = n;
      n_values -= l >> 1;
    }
  while (n_values);
}

static void
bse_pcm_device_oss_class_init (BsePcmDeviceOSSClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseDeviceClass *device_class = BSE_DEVICE_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->finalize = bse_pcm_device_oss_finalize;
  
  device_class->list_devices = bse_pcm_device_oss_list_devices;
  bse_device_class_setup (class,
                          BSE_RATING_DEFAULT,
                          "oss",
                          _("DEVICE,MODE"),
                          /* TRANSLATORS: keep this text to 70 chars in width */
                          _("Open Sound System PCM driver:\n"
                            "  DEVICE - PCM device file name\n"
                            "  MODE   - one of \"ro\", \"rw\" or \"wo\" for\n"
                            "           read-only, read-write or write-only access."));
  device_class->open = bse_pcm_device_oss_open;
  device_class->close = bse_pcm_device_oss_close;
}

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

#endif	/* BSE_PCM_DEVICE_CONF_OSS */
