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

#include	"bseconfig.h"
#include	<sys/soundcard.h>
#include	<sys/ioctl.h>
#include	<unistd.h>
#include	<errno.h>
#include	<fcntl.h>

#define OSS_DEBUG

/* --- BsePcmDeviceOSS structs --- */
struct _BsePcmDeviceOSS
{
  BsePcmDevice parent_object;
};
struct _BsePcmDeviceOSSClass
{
  BsePcmDeviceClass parent_class;
};


/* --- prototypes --- */
static void	    bse_pcm_device_oss_class_init	(BsePcmDeviceOSSClass	*class);
static void	    bse_pcm_device_oss_init		(BsePcmDeviceOSS	*pcm_device_oss);
static void	    bse_pcm_device_oss_shutdown		(BseObject		*object);
static BseErrorType bse_pcm_device_oss_update_caps	(BsePcmDevice		*pdev);
static BseErrorType bse_pcm_device_oss_open		(BsePcmDevice		*pdev,
							 gboolean       	 readable,
							 gboolean       	 writable,
							 guint                   n_channels,
							 BsePcmFreqMask 	 rate,
							 guint          	 fragment_size);
static BseErrorType bse_pcm_device_oss_setup		(BsePcmDevice		*pdev,
							 gboolean		 writable,
							 guint                   n_channels,
							 BsePcmFreqMask 	 rate,
							 guint                   n_fragments,
							 guint          	 fragment_size);
static void	    bse_pcm_device_oss_update_state	(BsePcmDevice   	*pdev);
static gboolean	    bse_pcm_device_oss_in_playback	(BsePcmDevice		*pdev);
static void	    bse_pcm_device_oss_close		(BsePcmDevice		*pdev);


/* --- variables --- */
static gpointer parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BsePcmDeviceOSS)
{
  BseType pcm_device_oss_type;

  static const BseTypeInfo pcm_device_oss_info = {
    sizeof (BsePcmDeviceOSSClass),
    
    (BseBaseInitFunc) NULL,
    (BseBaseDestroyFunc) NULL,
    (BseClassInitFunc) bse_pcm_device_oss_class_init,
    (BseClassDestroyFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BsePcmDeviceOSS),
    0 /* n_preallocs */,
    (BseObjectInitFunc) bse_pcm_device_oss_init,
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
  BsePcmDeviceClass *pcm_device_class;
  
  parent_class = bse_type_class_peek (BSE_TYPE_PCM_DEVICE);
  object_class = BSE_OBJECT_CLASS (class);
  pcm_device_class = BSE_PCM_DEVICE_CLASS (class);
  
  object_class->shutdown = bse_pcm_device_oss_shutdown;

  pcm_device_class->update_caps = bse_pcm_device_oss_update_caps;
  pcm_device_class->open = bse_pcm_device_oss_open;
  pcm_device_class->update_state = bse_pcm_device_oss_update_state;
  pcm_device_class->in_playback = bse_pcm_device_oss_in_playback;
  pcm_device_class->close = bse_pcm_device_oss_close;
}

static void
bse_pcm_device_oss_init (BsePcmDeviceOSS *oss)
{
  BsePcmDevice *pdev;

  pdev = BSE_PCM_DEVICE (oss);
}

static void
bse_pcm_device_oss_shutdown (BseObject *object)
{
  BsePcmDeviceOSS *oss;
  BsePcmDevice *pdev;
  
  oss = BSE_PCM_DEVICE_OSS (object);
  pdev = BSE_PCM_DEVICE (oss);

  /* chain parent class' shutdown handler */
  BSE_OBJECT_CLASS (parent_class)->shutdown (object);
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
  BseErrorType error;
  guint omode = 0;
  gint fd;

  if (readable && writable)
    omode = O_RDWR;
  else if (readable)
    omode = O_RDONLY;
  else if (writable)
    omode = O_WRONLY;

  fd = open (pdev->device_name, omode, 0);
  if (fd < 0)
    {
      if (errno == EBUSY)
	return BSE_ERROR_DEVICE_BUSY;
      else if (errno == EISDIR || errno == EACCES || errno == EROFS)
	return BSE_ERROR_DEVICE_PERMS;
      else
	return BSE_ERROR_DEVICE_IO;
    }
  pdev->pfd.fd = fd;

  error = bse_pcm_device_oss_setup (pdev, writable, n_channels, rate, 128, fragment_size);
  if (error)
    close (fd);
  else
    {
      if (readable)
	BSE_OBJECT_SET_FLAGS (oss, BSE_PCM_FLAG_READABLE);
      if (writable)
	BSE_OBJECT_SET_FLAGS (oss, BSE_PCM_FLAG_WRITABLE);
    }

  return error;
}

static void
bse_pcm_device_oss_close (BsePcmDevice *pdev)
{
  (void) ioctl (pdev->pfd.fd, SNDCTL_DSP_RESET);
  
  (void) close (pdev->pfd.fd);
}

static void
bse_pcm_device_oss_update_state	(BsePcmDevice *pdev)
{
  BsePcmDeviceOSS *oss;
  audio_buf_info info;
  
  oss = BSE_PCM_DEVICE_OSS (pdev);

  memset (&info, 0, sizeof (info));
  (void) ioctl (pdev->pfd.fd, SNDCTL_DSP_GETISPACE, &info);
  pdev->n_capture_bytes = info.fragments * info.fragsize;
  BSE_IF_DEBUG (PCM)
    g_message ("ISPACE(%s) frags: total=%d, size=%d, count=%d, bytes=%d, n=%u\n",
	       pdev->device_name,
	       info.fragstotal,
	       info.fragsize,
	       info.fragments,
	       info.bytes,
	       pdev->n_capture_bytes);
  
  memset (&info, 0, sizeof (info));
  (void) ioctl (pdev->pfd.fd, SNDCTL_DSP_GETOSPACE, &info);
  pdev->n_playback_bytes = info.fragments * info.fragsize;
  BSE_IF_DEBUG (PCM)
    g_message ("OSPACE(%s) frags: total=%d, size=%d, count=%d, bytes=%d, n=%u\n",
	       pdev->device_name,
	       info.fragstotal,
	       info.fragsize,
	       info.fragments,
	       info.bytes,
	       pdev->n_playback_bytes);
}

static gboolean
bse_pcm_device_oss_in_playback (BsePcmDevice *pdev)
{
  audio_buf_info info = { 0, };

  (void) ioctl (pdev->pfd.fd, SNDCTL_DSP_GETOSPACE, &info);

  return info.fragments < info.fragstotal;
}

static BseErrorType
bse_pcm_device_oss_setup (BsePcmDevice	*pdev,
			  gboolean       writable,
			  guint          n_channels,
			  BsePcmFreqMask rate,
			  guint          n_fragments,
			  guint          fragment_size)
{
  audio_buf_info info = { 0, };
  gint fd = pdev->pfd.fd;
  glong d_long;
  gint d_int;

  d_long = fcntl (fd, F_GETFL);
  d_long &= ~O_NONBLOCK;
  if (fcntl (fd, F_SETFL, d_long))
    return BSE_ERROR_DEVICE_ASYNC;

  d_int = 0;
  if (ioctl (fd, SNDCTL_DSP_GETFMTS, &d_int) < 0 || (d_int & AFMT_S16_LE) != AFMT_S16_LE)
    return BSE_ERROR_DEVICE_GET_CAPS;
  d_int = AFMT_S16_LE;
  if (ioctl (fd, SNDCTL_DSP_SETFMT, &d_int) < 0 || d_int != AFMT_S16_LE)
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
  pdev->block_size = d_int;

  if (ioctl (fd, writable ? SNDCTL_DSP_GETOSPACE : SNDCTL_DSP_GETISPACE, &info) < 0)
    return BSE_ERROR_DEVICE_GET_CAPS;
  pdev->n_fragments = info.fragstotal;
  pdev->fragment_size = info.fragsize;

  return BSE_ERROR_NONE;
}

static BseErrorType
bse_pcm_device_oss_update_caps (BsePcmDevice *pdev)
{
  gint fd, omode = -1;
  gint d_int;
  guint i;

  fd = open (pdev->device_name, O_RDONLY, 0);
  if (fd >= 0)
    {
      omode = O_RDONLY;
      pdev->caps.readable = TRUE;
      close (fd);
    }
  else if (errno == EBUSY)
    return BSE_ERROR_DEVICE_BUSY;
  fd = open (pdev->device_name, O_WRONLY, 0);
  if (fd >= 0)
    {
      omode = O_WRONLY;
      pdev->caps.writable = TRUE;
      close (fd);
    }
  fd = open (pdev->device_name, O_RDWR, 0);
  if (fd >= 0)
    {
      omode = O_RDWR;
      close (fd);
    }

  fd = -1;
  if (omode > 0)
    fd = open (pdev->device_name, omode, 0);
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
      if (ioctl (fd, SNDCTL_DSP_SETFRAGMENT, &d_int) < 0)
	pdev->caps.max_fragment_size = MAX (pdev->caps.max_fragment_size, 1 << (d_int & 0xffff));
    }

  close (fd);
  BSE_IF_DEBUG (PCM)
    g_message ("CAPS(%s): w=%d r=%d d=%d max_ch=%d pfreq=0x%x cfreq=0x%x fragsize=%d\n",
	       pdev->device_name,
	       pdev->caps.writable,
	       pdev->caps.readable,
	       pdev->caps.duplex,
	       pdev->caps.max_n_channels,
	       pdev->caps.playback_freq_mask,
	       pdev->caps.capture_freq_mask,
	       pdev->caps.max_fragment_size);
  
  return BSE_ERROR_NONE;
}
