/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997, 1998, 1999 Olaf Hoehmann and Tim Janik
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


/* --- BsePcmDeviceOSS structs --- */
struct _BsePcmDeviceOSS
{
  BsePcmDevice parent_object;

  gchar	      *device_name;
};
struct _BsePcmDeviceOSSClass
{
  BsePcmDeviceClass parent_class;
};


/* --- prototypes --- */
static void	    bse_pcm_device_oss_class_init	(BsePcmDeviceOSSClass	*class);
static void	    bse_pcm_device_oss_init		(BsePcmDeviceOSS	*pcm_device_oss);
static void	    bse_pcm_device_oss_destroy		(BseObject		*object);
static gchar*	    bse_pcm_device_oss_device_name	(BseDevice              *dev,
							 gboolean                descriptive);
static BseErrorType bse_pcm_device_oss_update_caps	(BsePcmDevice		*pdev);
static BseErrorType bse_pcm_device_oss_open		(BsePcmDevice		*pdev,
							 gboolean       	 readable,
							 gboolean       	 writable,
							 guint                   n_channels,
							 BsePcmFreqMask 	 rate,
							 guint          	 fragment_size);
static BseErrorType bse_pcm_device_oss_setup		(BsePcmDevice		*pdev,
							 gboolean		 writable,
							 gboolean		 readable,
							 guint                   n_channels,
							 BsePcmFreqMask 	 rate,
							 guint                   n_fragments,
							 guint          	 fragment_size);
static void	    bse_pcm_device_oss_retrigger	(BsePcmDevice		*pdev);
static void	    bse_pcm_device_oss_update_state	(BsePcmDevice   	*pdev);
static void	    bse_pcm_device_oss_close		(BseDevice		*pdev);


/* --- variables --- */
static gpointer parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BsePcmDeviceOSS)
{
  GType   pcm_device_oss_type;
  
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
  // bse_categories_register_icon ("/Source/Projects/SNet", snet_type, &snet_pixdata);
  
  return pcm_device_oss_type;
}

static void
bse_pcm_device_oss_class_init (BsePcmDeviceOSSClass *class)
{
  BseObjectClass *object_class;
  BseDeviceClass *device_class;
  BsePcmDeviceClass *pcm_device_class;
  
  parent_class = g_type_class_peek (BSE_TYPE_PCM_DEVICE);
  object_class = BSE_OBJECT_CLASS (class);
  device_class = BSE_DEVICE_CLASS (class);
  pcm_device_class = BSE_PCM_DEVICE_CLASS (class);
  
  object_class->destroy = bse_pcm_device_oss_destroy;
  
  device_class->close = bse_pcm_device_oss_close;
  device_class->device_name = bse_pcm_device_oss_device_name;

  pcm_device_class->update_caps = bse_pcm_device_oss_update_caps;
  pcm_device_class->open = bse_pcm_device_oss_open;
  pcm_device_class->retrigger = bse_pcm_device_oss_retrigger;
  pcm_device_class->update_state = bse_pcm_device_oss_update_state;
}

static void
bse_pcm_device_oss_init (BsePcmDeviceOSS *oss)
{
  oss->device_name = g_strdup (BSE_PCM_DEVICE_CONF_OSS);
}

static void
bse_pcm_device_oss_destroy (BseObject *object)
{
  BsePcmDeviceOSS *oss = BSE_PCM_DEVICE_OSS (object);
  BsePcmDevice *pdev;
  
  pdev = BSE_PCM_DEVICE (oss);

  g_free (oss->device_name);
  oss->device_name = NULL;
  
  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static gchar*
bse_pcm_device_oss_device_name (BseDevice *dev,
				gboolean   descriptive)
{
  BsePcmDeviceOSS *oss = BSE_PCM_DEVICE_OSS (dev);

  return oss->device_name;
}

static BseErrorType
bse_pcm_device_oss_open (BsePcmDevice  *pdev,
			 gboolean       readable,
			 gboolean       writable,
			 guint          n_channels,
			 BsePcmFreqMask rate,
			 guint          fragment_size)
{
  BsePcmDeviceOSS *oss = BSE_PCM_DEVICE_OSS (pdev);
  BseDevice *dev = BSE_DEVICE (oss);
  BseErrorType error;
  gint omode = 0;
  gint fd;
  
  if (readable && writable)
    omode = O_RDWR;
  else if (readable)
    omode = O_RDONLY;
  else if (writable)
    omode = O_WRONLY;
  
  fd = open (oss->device_name, omode | O_NONBLOCK, 0);
  if (fd < 0)
    {
      if (errno == EBUSY)
	return BSE_ERROR_DEVICE_BUSY;
      else if (errno == EISDIR || errno == EACCES || errno == EROFS)
	return BSE_ERROR_DEVICE_PERMS;
      else
	return BSE_ERROR_DEVICE_IO;
    }
  dev->pfd.fd = fd;
  
  error = bse_pcm_device_oss_setup (pdev, writable, readable, n_channels, rate, 128, fragment_size);
  if (error)
    close (fd);
  else
    {
      if (readable)
	BSE_OBJECT_SET_FLAGS (oss, BSE_DEVICE_FLAG_READABLE);
      if (writable)
	BSE_OBJECT_SET_FLAGS (oss, BSE_DEVICE_FLAG_WRITABLE);
    }

  return error;
}

static void
bse_pcm_device_oss_close (BseDevice *dev)
{
  (void) ioctl (dev->pfd.fd, SNDCTL_DSP_RESET);
  
  (void) close (dev->pfd.fd);

  /* chain parent class' handler */
  BSE_DEVICE_CLASS (parent_class)->close (dev);
}

static void
bse_pcm_device_oss_retrigger (BsePcmDevice *pdev)
{
  BseDevice *dev = BSE_DEVICE (pdev);
  int d_int;

  /* it should be enough to select() on the fd to trigger
   * capture/playback, but with some new OSS drivers
   * (clones) this is not the case anymore, so we also
   * use the SNDCTL_DSP_SETTRIGGER ioctl to achive this.
   */
  
  d_int = 0;
  if (BSE_DEVICE_READABLE (pdev))
    d_int |= PCM_ENABLE_INPUT;
  if (BSE_DEVICE_WRITABLE (pdev))
    d_int |= PCM_ENABLE_OUTPUT;
  (void) ioctl (dev->pfd.fd, SNDCTL_DSP_SETTRIGGER, &d_int);
  
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
   *   we only need to asure that input is actually triggered, BseHeart
   *   takes care of the latency and at this point probably already has
   *   output buffers pending.
   */
  if (BSE_DEVICE_READABLE (pdev))
    {
      struct timeval tv = { 0, 0, };
      fd_set in_fds;
      fd_set out_fds;

      FD_ZERO (&in_fds);
      FD_ZERO (&out_fds);
      FD_SET (dev->pfd.fd, &in_fds);
      /* FD_SET (dev->pfd.fd, &out_fds); */
      select (dev->pfd.fd + 1, &in_fds, &out_fds, NULL, &tv);
    }
}

static void
bse_pcm_device_oss_update_state (BsePcmDevice *pdev)
{
  BsePcmDeviceOSS *oss = BSE_PCM_DEVICE_OSS (pdev);
  BseDevice *dev = BSE_DEVICE (oss);
  audio_buf_info info;
  
  memset (&info, 0, sizeof (info));
  (void) ioctl (dev->pfd.fd, SNDCTL_DSP_GETISPACE, &info);
  pdev->capture_buffer_size = info.fragstotal * info.fragsize;
  pdev->n_capture_bytes = info.fragments * info.fragsize;
  pdev->n_capture_bytes = info.bytes;

  if (1) /* OSS-bug fix, at least for es1371 in 2.3.34 */
    pdev->n_capture_bytes = MIN (pdev->n_capture_bytes, pdev->capture_buffer_size);

  BSE_IF_DEBUG (PCM)
    g_message ("ISPACE(%s): left=%5d/%d frags: total=%d size=%d count=%d bytes=%d\n",
	       oss->device_name,
	       pdev->n_capture_bytes,
	       pdev->capture_buffer_size,
	       info.fragstotal,
	       info.fragsize,
	       info.fragments,
	       info.bytes);
  
  memset (&info, 0, sizeof (info));
  (void) ioctl (dev->pfd.fd, SNDCTL_DSP_GETOSPACE, &info);
  pdev->playback_buffer_size = info.fragstotal * info.fragsize;
  pdev->n_playback_bytes = info.fragments * info.fragsize;
  pdev->n_playback_bytes = info.bytes;

  if (1) /* OSS-bug fix, at least for es1371 in 2.3.34 */
    pdev->n_playback_bytes = MIN (pdev->n_playback_bytes, pdev->playback_buffer_size);

  BSE_IF_DEBUG (PCM)
    g_message ("OSPACE(%s): left=%5d/%d frags: total=%d size=%d count=%d bytes=%d\n",
	       oss->device_name,
	       pdev->n_playback_bytes,
	       pdev->playback_buffer_size,
	       info.fragstotal,
	       info.fragsize,
	       info.fragments,
	       info.bytes);
}

static BseErrorType
bse_pcm_device_oss_setup (BsePcmDevice	*pdev,
			  gboolean       writable,
			  gboolean       readable,
			  guint          n_channels,
			  BsePcmFreqMask rate,
			  guint          n_fragments,
			  guint          fragment_size)
{
  BsePcmDeviceOSS *oss = BSE_PCM_DEVICE_OSS (pdev);
  BseDevice *dev = BSE_DEVICE (oss);
  gint fd = dev->pfd.fd;
  glong d_long;
  gint d_int;
  
  d_long = fcntl (fd, F_GETFL);
  d_long &= ~O_NONBLOCK;
  if (fcntl (fd, F_SETFL, d_long))
    return BSE_ERROR_DEVICE_ASYNC;
  
  d_int = 0;
  if (ioctl (fd, SNDCTL_DSP_GETFMTS, &d_int) < 0)
    return BSE_ERROR_DEVICE_GET_CAPS;
  if ((d_int & AFMT_S16_LE) != AFMT_S16_LE)
    return BSE_ERROR_DEVICE_CAPS_MISMATCH;
  d_int = AFMT_S16_LE;
  if (ioctl (fd, SNDCTL_DSP_SETFMT, &d_int) < 0 ||
      d_int != AFMT_S16_LE)
    return BSE_ERROR_DEVICE_SET_CAPS;
  
  d_int = n_channels - 1;
  if (ioctl (fd, SNDCTL_DSP_STEREO, &d_int) < 0)
    return BSE_ERROR_DEVICE_SET_CAPS;
  pdev->n_channels = d_int + 1;
  
  d_int = bse_pcm_freq_to_freq (rate);
  if (ioctl (fd, SNDCTL_DSP_SPEED, &d_int) < 0)
    return BSE_ERROR_DEVICE_SET_CAPS;
  pdev->sample_freq = d_int;
  
  /* Note: fragment = n_fragments << 16;
   *       fragment |= g_bit_storage (fragment_size - 1);
   */
  fragment_size = MAX (fragment_size, 128);
  n_fragments = MIN (n_fragments, 128);
  d_int = (n_fragments << 16) | g_bit_storage (fragment_size - 1);
  if (ioctl (fd, SNDCTL_DSP_SETFRAGMENT, &d_int) < 0)
    return BSE_ERROR_DEVICE_SET_CAPS;
  
  d_int = 0;
  if (ioctl (fd, SNDCTL_DSP_GETBLKSIZE, &d_int) < 0 ||
      d_int < 128 ||
      d_int > 131072 ||
      (d_int & 1))
    return BSE_ERROR_DEVICE_GET_CAPS;
  /* pdev->block_size = d_int; */

  if (writable)
    {
      audio_buf_info info = { 0, };

      if (ioctl (fd, SNDCTL_DSP_GETOSPACE, &info) < 0)
	return BSE_ERROR_DEVICE_GET_CAPS;
      pdev->playback_buffer_size = info.fragstotal * info.fragsize;
      pdev->n_playback_bytes = 0;
    }
  if (readable)
    {
      audio_buf_info info = { 0, };

      if (ioctl (fd, SNDCTL_DSP_GETISPACE, &info) < 0)
	return BSE_ERROR_DEVICE_GET_CAPS;
      pdev->capture_buffer_size = info.fragstotal * info.fragsize;
      pdev->n_capture_bytes = 0;
    }
  
  BSE_IF_DEBUG (PCM)
    g_message ("SETUP(%s): w=%d r=%d n_channels=%d sample_freq=%.0f playback_bufsz=%d capture_bufsz=%d\n",
               oss->device_name,
	       writable,
	       readable,
	       pdev->n_channels,
	       pdev->sample_freq,
	       pdev->playback_buffer_size,
	       pdev->capture_buffer_size);

  return BSE_ERROR_NONE;
}

static BseErrorType
bse_pcm_device_oss_update_caps (BsePcmDevice *pdev)
{
  BsePcmDeviceOSS *oss = BSE_PCM_DEVICE_OSS (pdev);
  gint fd, omode = -1;
  gint d_int;
  guint i;
  
  fd = open (oss->device_name, O_RDONLY | O_NONBLOCK, 0);
  if (fd >= 0)
    {
      omode = O_RDONLY;
      pdev->caps.readable = TRUE;
      close (fd);
    }
  else if (errno == EBUSY)
    return BSE_ERROR_DEVICE_BUSY;
  fd = open (oss->device_name, O_WRONLY | O_NONBLOCK, 0);
  if (fd >= 0)
    {
      omode = O_WRONLY;
      pdev->caps.writable = TRUE;
      close (fd);
    }
  fd = open (oss->device_name, O_RDWR | O_NONBLOCK, 0);
  if (fd >= 0)
    {
      omode = O_RDWR;
      close (fd);
    }
  
  fd = -1;
  if (omode > 0)
    fd = open (oss->device_name, omode | O_NONBLOCK, 0);
  if (fd < 0)
    {
      if (errno == EBUSY)
	return BSE_ERROR_DEVICE_BUSY;
      else if (errno == EISDIR || errno == EACCES || errno == EROFS)
	return BSE_ERROR_DEVICE_PERMS;
      else
	return BSE_ERROR_DEVICE_IO;
    }
  
  d_int = 0;
  if (ioctl (fd, SNDCTL_DSP_GETCAPS, &d_int) < 0)
    {
      close (fd);
      return BSE_ERROR_DEVICE_GET_CAPS;
    }
  if (omode == O_RDWR && (d_int & DSP_CAP_DUPLEX))
    pdev->caps.duplex = TRUE;
  
  pdev->caps.max_n_channels = 1;
  d_int = 1;
  if (ioctl (fd, SNDCTL_DSP_STEREO, &d_int) >= 0 && d_int == 1)
    pdev->caps.max_n_channels = 2;
  d_int = 2;
  if (ioctl (fd, SNDCTL_DSP_CHANNELS, &d_int) >= 0 && d_int > 0)
    pdev->caps.max_n_channels = d_int;
  
  d_int = 1;
  (void) ioctl (fd, SNDCTL_DSP_STEREO, &d_int);
  for (i = 0; i < BSE_PCM_FREQ_LAST_BIT; i++)
    {
      d_int = bse_pcm_freq_to_freq (1 << i);
      if (ioctl (fd, SNDCTL_DSP_SPEED, &d_int) >= 0)
	pdev->caps.playback_freq_mask |= bse_pcm_freq_from_freq (d_int);
    }
  pdev->caps.capture_freq_mask = pdev->caps.playback_freq_mask;
  
  for (i = 15; i > 0; i--)
    {
      d_int = (1 << 16) | i;
      if (ioctl (fd, SNDCTL_DSP_SETFRAGMENT, &d_int) >= 0)
	pdev->caps.max_fragment_size = MAX (pdev->caps.max_fragment_size, 1 << (d_int & 0xffff));
    }
  
  close (fd);
  
  BSE_IF_DEBUG (PCM)
    g_message ("CAPS(%s): w=%d r=%d d=%d max_ch=%d pfreq=0x%x cfreq=0x%x fragsize=%d\n",
	       oss->device_name,
	       pdev->caps.writable,
	       pdev->caps.readable,
	       pdev->caps.duplex,
	       pdev->caps.max_n_channels,
	       pdev->caps.playback_freq_mask,
	       pdev->caps.capture_freq_mask,
	       pdev->caps.max_fragment_size);
  
  return BSE_ERROR_NONE;
}

#endif	/* BSE_PCM_DEVICE_CONF_OSS */
