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
#include	"bsepcmstream-oss.h"

#include	"bseglobals.h"
#include	"bseconfig.h"
#include	<sys/soundcard.h>
#include	<sys/ioctl.h>
#include	<unistd.h>
#include	<errno.h>
#include	<fcntl.h>
#include	<string.h>



/* --- system specific class --- */
struct _BsePcmStreamOSS
{
  BsePcmStream	bse_pcm_stream;
  
  gint	fd;
  gint	block_size;
};
struct _BsePcmStreamOSSClass
{
  BsePcmStreamClass  bse_pcm_stream_class;
};


/* --- stream prototypes --- */
static void	bse_pcm_stream_oss_class_init	(BsePcmStreamOSSClass	*class);
static void	bse_pcm_stream_oss_init		(BsePcmStreamOSS	*pcm_stream_oss);
static void	bse_pcm_stream_oss_destroy	(BseObject		*object);
static void	pcm_stream_oss_open		(BseStream		*stream,
						 gboolean		 read_access,
						 gboolean		 write_access);
static void	pcm_stream_oss_close		(BseStream		*stream);
static void	pcm_stream_oss_start		(BseStream		*stream);
static void	pcm_stream_oss_stop		(BseStream		*stream);
static gboolean pcm_stream_oss_would_block	(BseStream		*stream,
						 guint			 n_values);
static guint	pcm_stream_oss_read_sv		(BseStream		*stream,
						 guint			 n_values,
						 BseSampleValue		*values);
static guint	pcm_stream_oss_write_sv		(BseStream		*stream,
						 guint			 n_values,
						 const BseSampleValue	*values);
static void	pcm_stream_oss_set_attribs	(BsePcmStream		*pcm_stream,
						 BsePcmStreamAttribMask	 mask,
						 BsePcmStreamAttribs	*attribs);
static guint	pcm_stream_oss_open_dsp		(BsePcmStreamOSS	*pcm_stream_oss);
static guint	pcm_stream_oss_set_dsp		(BsePcmStreamOSS	*pcm_stream_oss,
						 BsePcmStreamAttribMask	 attrib_mask,
						 BsePcmStreamAttribs	*attribs);


/* --- variables --- */
static BsePcmStreamClass *parent_class = NULL;
static const BsePcmStreamAttribMask OSS_PCMSA_MASK = (BSE_PCMSA_N_CHANNELS |
						      BSE_PCMSA_PLAY_FREQUENCY |
						      BSE_PCMSA_REC_FREQUENCY |
						      BSE_PCMSA_FRAGMENT_SIZE);


/* --- functions --- */
BSE_BUILTIN_TYPE (BsePcmStreamOSS)
{
  static const BseTypeInfo pcm_stream_oss_info = {
    sizeof (BsePcmStreamOSSClass),
    
    (BseClassInitBaseFunc) NULL,
    (BseClassDestroyBaseFunc) NULL,
    (BseClassInitFunc) bse_pcm_stream_oss_class_init,
    (BseClassDestroyFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BsePcmStreamOSS),
    0 /* n_preallocs */,
    (BseObjectInitFunc) bse_pcm_stream_oss_init,
  };
  
  return bse_type_register_static (BSE_TYPE_PCM_STREAM,
				   "BsePcmStreamOSS",
				   "Streamed pcm data handling for Open Sound System",
				   &pcm_stream_oss_info);
}

static void
bse_pcm_stream_oss_class_init (BsePcmStreamOSSClass *class)
{
  BseObjectClass *object_class;
  BseStreamClass *stream_class;
  BsePcmStreamClass *pcm_stream_class;
  
  parent_class = bse_type_class_peek (BSE_TYPE_PCM_STREAM);
  object_class = BSE_OBJECT_CLASS (class);
  stream_class = BSE_STREAM_CLASS (class);
  pcm_stream_class = BSE_PCM_STREAM_CLASS (class);
  
  object_class->destroy = bse_pcm_stream_oss_destroy;
  
  stream_class->open = pcm_stream_oss_open;
  stream_class->close = pcm_stream_oss_close;
  stream_class->start = pcm_stream_oss_start;
  stream_class->stop = pcm_stream_oss_stop;
  stream_class->would_block = pcm_stream_oss_would_block;
  stream_class->read = NULL;
  stream_class->write = NULL;
  stream_class->read_sv = pcm_stream_oss_read_sv;
  stream_class->write_sv = pcm_stream_oss_write_sv;
  
  pcm_stream_class->set_attribs = pcm_stream_oss_set_attribs;
}

static void
bse_pcm_stream_oss_init (BsePcmStreamOSS *pcm_stream_oss)
{
  BseStream *stream;
  BsePcmStream *pcm_stream;
  
  stream = BSE_STREAM (pcm_stream_oss);
  pcm_stream = BSE_PCM_STREAM (pcm_stream_oss);
  
  BSE_STREAM_UNSET_FLAG (pcm_stream_oss, READABLE);
  BSE_STREAM_SET_FLAG (pcm_stream_oss, WRITABLE);
  
  /* default device */
  stream->file_name = g_strdup (BSE_PATH_DEVICE_OSS);
  
  pcm_stream->max_channels = 2;
  pcm_stream->min_play_frequency = 8000;
  pcm_stream->max_play_frequency = 48000;
  pcm_stream->min_record_frequency = 8000;
  pcm_stream->max_record_frequency = 48000;
  pcm_stream->min_fragment_size = 128;
  pcm_stream->max_fragment_size = 8192;
  
  pcm_stream->attribs.n_channels = pcm_stream->max_channels;
  pcm_stream->attribs.play_frequency = pcm_stream->max_play_frequency;
  pcm_stream->attribs.record_frequency = pcm_stream->max_record_frequency;
  pcm_stream->attribs.fragment_size = 1 << g_bit_storage (MAX (bse_globals->pcm_buffer_size,
							       256) - 1);
  
  pcm_stream_oss->fd = -1;
  pcm_stream_oss->block_size = 0;
}

static void
bse_pcm_stream_oss_destroy (BseObject *object)
{
  BsePcmStreamOSS *pcm_stream_oss;
  BsePcmStream *pcm_stream;
  BseStream *stream;
  
  pcm_stream_oss = BSE_PCM_STREAM_OSS (object);
  pcm_stream = BSE_PCM_STREAM (object);
  stream = BSE_STREAM (object);
  
  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
pcm_stream_oss_open (BseStream	    *stream,
		     gboolean	     read_access,
		     gboolean	     write_access)
{
  BsePcmStream *pcm_stream;
  BsePcmStreamOSS *pcm_stream_oss;
  
  pcm_stream = BSE_PCM_STREAM (stream);
  pcm_stream_oss = BSE_PCM_STREAM_OSS (stream);
  
  pcm_stream->attribs.n_channels = CLAMP (pcm_stream->attribs.n_channels,
					  1,
					  pcm_stream->max_channels);
  pcm_stream->attribs.play_frequency = CLAMP (pcm_stream->attribs.play_frequency,
					      pcm_stream->min_play_frequency,
					      pcm_stream->max_play_frequency);
  pcm_stream->attribs.record_frequency = CLAMP (pcm_stream->attribs.record_frequency,
						pcm_stream->min_record_frequency,
						pcm_stream->max_record_frequency);
  pcm_stream->attribs.fragment_size = 0xfff0 & CLAMP (pcm_stream->attribs.fragment_size,
						      pcm_stream->min_fragment_size,
						      pcm_stream->max_fragment_size);
  
  BSE_STREAM_SET_ERROR (pcm_stream_oss,
			pcm_stream_oss_open_dsp (pcm_stream_oss));
  
  if (!BSE_STREAM_ERROR (pcm_stream_oss))
    {
      pcm_stream_oss_set_attribs (pcm_stream,
				  OSS_PCMSA_MASK,
				  &pcm_stream->attribs);
      
      if (BSE_STREAM_ERROR (pcm_stream_oss))
	{
	  close (pcm_stream_oss->fd);
	  pcm_stream_oss->fd = -1;
	  pcm_stream_oss->block_size = 0;
	}
      else
	BSE_STREAM_SET_FLAG (stream, OPENED);
    }
  
  errno = 0;
}

static void
pcm_stream_oss_close (BseStream	*stream)
{
  BsePcmStream *pcm_stream;
  BsePcmStreamOSS *pcm_stream_oss;
  
  pcm_stream = BSE_PCM_STREAM (stream);
  pcm_stream_oss = BSE_PCM_STREAM_OSS (stream);
  
  (void) ioctl (pcm_stream_oss->fd, SNDCTL_DSP_RESET);
  
  (void) close (pcm_stream_oss->fd);
  
  pcm_stream_oss->fd = -1;
  pcm_stream_oss->block_size = 0;
  
  BSE_STREAM_UNSET_FLAG (stream, OPENED);
  
  errno = 0;
}

static void
pcm_stream_oss_start (BseStream	*stream)
{
  static BseSampleValue *zeros = NULL;
  static guint z_size = 0;
  BsePcmStream *pcm_stream;
  BsePcmStreamOSS *pcm_stream_oss;
  
  pcm_stream = BSE_PCM_STREAM (stream);
  pcm_stream_oss = BSE_PCM_STREAM_OSS (stream);
  
  /* this is sick!
   * we need to put an amount of zeros into the soundcard prior to atual
   * playing, to avoid an extra click-noise.
   * this is with GUS-MAX and the ultra-driver from jaroslav.
   */
  
  if (!zeros)
    {
      z_size = bse_globals->pcm_buffer_size;
      zeros = g_new0 (BseSampleValue, z_size);
    }
  
  (void) ioctl (pcm_stream_oss->fd, SNDCTL_DSP_RESET);
  
  while (!pcm_stream_oss_would_block (stream, z_size))
    write (pcm_stream_oss->fd, zeros, z_size);
  
  BSE_STREAM_SET_FLAG (stream, READY);
  
  errno = 0;
}

static void
pcm_stream_oss_stop (BseStream *stream)
{
  BsePcmStream *pcm_stream;
  BsePcmStreamOSS *pcm_stream_oss;
  
  pcm_stream = BSE_PCM_STREAM (stream);
  pcm_stream_oss = BSE_PCM_STREAM_OSS (stream);
  
  /* (void) ioctl (pcm_stream_oss->fd, SNDCTL_DSP_SYNC); */
  
  (void) ioctl (pcm_stream_oss->fd, SNDCTL_DSP_RESET);
  
  BSE_STREAM_UNSET_FLAG (stream, READY);
  
  errno = 0;
}

static guint
pcm_stream_oss_read_sv (BseStream      *stream,
			guint		n_values,
			BseSampleValue *values)
{
  BsePcmStream *pcm_stream;
  BsePcmStreamOSS *pcm_stream_oss;
  
  pcm_stream = BSE_PCM_STREAM (stream);
  pcm_stream_oss = BSE_PCM_STREAM_OSS (stream);
  
  memset (values, 0, sizeof (BseSampleValue) * n_values);
  
  errno = 0;
  
  return sizeof (BseSampleValue) * n_values;
}

static gboolean
pcm_stream_oss_would_block (BseStream *stream,
			    guint      n_values)
{
  static guint debug = 0;
  BsePcmStreamOSS *pcm_stream_oss;
  audio_buf_info info;
  gboolean would_block;
  
  pcm_stream_oss = BSE_PCM_STREAM_OSS (stream);
  
  (void) ioctl (pcm_stream_oss->fd, SNDCTL_DSP_GETOSPACE, &info);
  
  if (debug)
    {
      g_print ("pcm_stream_oss_would_block: fragstotal=%d, fragsize=%d, fragments=%d, would_block=%d\n",
	       info.fragstotal,
	       info.fragsize,
	       info.fragments,
	       info.fragsize * info.fragments < n_values);
      debug = 1;
    }
  
  errno = 0;
  would_block = info.fragsize * info.fragments < n_values * sizeof (BseSampleValue);
  
  // printf("%c", would_block ? 'y' : 'n');
  // fflush(stdout);
  
  return would_block;
}

static guint
pcm_stream_oss_write_sv (BseStream	      *stream,
			 guint		       n_values,
			 const BseSampleValue *values)
{
  BsePcmStreamOSS *pcm_stream_oss;
  guint n_bytes;
  guint8 *buffer = (guint8*) values;
  guint bsize = n_values * sizeof (BseSampleValue);
  
  pcm_stream_oss = BSE_PCM_STREAM_OSS (stream);
  
  /* the fd is set to blocking behaviour by default */
  do
    n_bytes = write (pcm_stream_oss->fd, buffer, bsize);
  while (n_bytes < 0 && errno == EINTR); /* don't mind signals */
  
  errno = 0;
  
  return n_values * sizeof (BseSampleValue);
}

static void
pcm_stream_oss_set_attribs (BsePcmStream	  *pcm_stream,
			    BsePcmStreamAttribMask mask,
			    BsePcmStreamAttribs	  *attribs)
{
  BsePcmStreamOSS *pcm_stream_oss;
  
  pcm_stream_oss = BSE_PCM_STREAM_OSS (pcm_stream);
  
  mask &= OSS_PCMSA_MASK;
  
  BSE_STREAM_SET_ERROR (pcm_stream_oss,
			pcm_stream_oss_set_dsp (pcm_stream_oss, mask, attribs));
  
  if (attribs != &pcm_stream->attribs)
    g_memmove (attribs, &pcm_stream->attribs, sizeof (*attribs));
  
  errno = 0;
}

gint
bse_pcm_stream_oss_get_unix_fd (BsePcmStreamOSS *pcm_stream_oss)
{
  g_return_val_if_fail (BSE_IS_PCM_STREAM_OSS (pcm_stream_oss), -1);
  
  return pcm_stream_oss->fd;
}

static guint
pcm_stream_oss_open_dsp (BsePcmStreamOSS *pcm_stream_oss)
{
  BseStream *stream;
  BsePcmStream *pcm_stream;
  gint fd;
  guint block_size;
  gint d_int;
  
  stream = BSE_STREAM (pcm_stream_oss);
  pcm_stream = BSE_PCM_STREAM (pcm_stream_oss);
  
  /* currently, we feature only BSE_STREAMF_WRITABLE */
  
  fd = open (stream->file_name, O_WRONLY);
  if (fd < 0)
    {
      if (errno == EBUSY)
	return BSE_ERROR_STREAM_DEVICE_BUSY;
      else
	return BSE_ERROR_STREAM_IO;
    }
  
  block_size = pcm_stream_oss->block_size;
  d_int = block_size;
  if (ioctl (fd, SNDCTL_DSP_GETBLKSIZE, &d_int) < 0 ||
      d_int < 1024 ||
      d_int > 131072 ||
      d_int != (d_int & 0xffffffe))
    {
      close (fd);
      return BSE_ERROR_STREAM_GET_ATTRIB;
    }
  block_size = d_int;
  
  d_int = AFMT_S16_LE;
  if (ioctl (fd, SNDCTL_DSP_GETFMTS, &d_int) < 0 ||
      (d_int & AFMT_S16_LE) != AFMT_S16_LE)
    {
      /* audio format not supported
       */
      close (fd);
      return BSE_ERROR_STREAM_GET_ATTRIB;
    }
  
  d_int = AFMT_S16_LE;
  if (ioctl (fd, SNDCTL_DSP_SETFMT, &d_int) < 0 ||
      d_int != AFMT_S16_LE)
    {
      /* failed to set audio format
       */
      close (fd);
      return BSE_ERROR_STREAM_SET_ATTRIB;
    }
  
  pcm_stream_oss->fd = fd;
  pcm_stream_oss->block_size = block_size;
  
  return BSE_ERROR_NONE;
}

static guint
pcm_stream_oss_set_dsp (BsePcmStreamOSS	      *pcm_stream_oss,
			BsePcmStreamAttribMask attrib_mask,
			BsePcmStreamAttribs   *attribs)
{
  BsePcmStream *pcm_stream;
  gint d_int;
  glong d_long;
  
  pcm_stream = BSE_PCM_STREAM (pcm_stream_oss);
  
  if (attrib_mask & BSE_PCMSA_N_CHANNELS)
    {
      attribs->n_channels = CLAMP (attribs->n_channels, 1, pcm_stream->max_channels);
      d_int = attribs->n_channels - 1;
      if (ioctl (pcm_stream_oss->fd, SNDCTL_DSP_STEREO, &d_int) < 0)
	{
	  /* failed to set audio format
	   */
	  return BSE_ERROR_STREAM_SET_ATTRIB;
	}
      d_int++;
      pcm_stream->attribs.n_channels = d_int;
      if (attribs->n_channels > d_int)
	pcm_stream->max_channels = d_int;
    }
  
  if (attrib_mask & BSE_PCMSA_PLAY_FREQUENCY ||
      attrib_mask & BSE_PCMSA_REC_FREQUENCY)
    {
      guint freq;
      
      freq = 0;
      if (attrib_mask & BSE_PCMSA_PLAY_FREQUENCY)
	{
	  attribs->play_frequency = CLAMP (attribs->play_frequency,
					   pcm_stream->min_play_frequency,
					   pcm_stream->max_play_frequency);
	  freq = attribs->play_frequency;
	}
      if (attrib_mask & BSE_PCMSA_REC_FREQUENCY)
	{
	  attribs->record_frequency = CLAMP (attribs->record_frequency,
					     pcm_stream->min_record_frequency,
					     pcm_stream->max_record_frequency);
	  freq = MAX (freq, attribs->record_frequency);
	}
      d_int = freq;
      if (ioctl (pcm_stream_oss->fd, SNDCTL_DSP_SPEED, &d_int) < 0)
	{
	  /* failed to set audio format
	   */
	  return BSE_ERROR_STREAM_SET_ATTRIB;
	}
      pcm_stream->attribs.play_frequency = d_int;
      pcm_stream->attribs.record_frequency = d_int;
      
      g_message ("OSS_DEBUG: play_freq: %d\n", d_int);
    }
  
  if (attrib_mask & BSE_PCMSA_FRAGMENT_SIZE)
    {
      /* Note: fragment = block_count << 16;
       *       fragment |= g_bit_storage (size - 1);
       */
      attribs->fragment_size = CLAMP (attribs->fragment_size,
				      pcm_stream->min_fragment_size,
				      pcm_stream->max_fragment_size);
      attribs->fragment_size &= 0xfff0;
      
      d_int = (1024 << 16) | g_bit_storage (attribs->fragment_size - 1);
      if (ioctl (pcm_stream_oss->fd, SNDCTL_DSP_SETFRAGMENT, &d_int) < 0)
	{
	  /* failed to set audio format
	   */
	  return BSE_ERROR_STREAM_SET_ATTRIB;
	}
      pcm_stream->attribs.fragment_size = 1 << (d_int & 0xffff);
    }
  
  d_long = fcntl (pcm_stream_oss->fd, F_GETFL);
  d_long &= ~O_NONBLOCK;
  if (fcntl (pcm_stream_oss->fd, F_SETFL, d_long))
    return BSE_ERROR_STREAM_SET_ATTRIB;
  
  return BSE_ERROR_NONE;
}
