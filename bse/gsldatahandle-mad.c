/* GSL - Generic Sound Layer
 * Copyright (C) 2001-2002 Tim Janik and Stefan Westerfeld
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <gsl/gsldatahandle-mad.h>

#include <gsl/gsldatautils.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#if	GSL_HAVE_LIBMAD
#include <mad.h>


/* --- debugging and errors --- */
#define	MAD_DEBUG		GSL_DEBUG_FUNCTION (GSL_MSG_DATA_HANDLE, "MAD")
#define	MAD_MSG			GSL_MESSAGE_FUNCTION (GSL_MSG_DATA_HANDLE, "MAD")


/* --- defines --- */
#define	FILE_BUFFER_SIZE	(1024 * 44)	/* approximately 1 second at 320 kbit */
#define	SEEK_BY_READ_AHEAD(h)	(((h)->sample_rate / ((h)->frame_size * 2))) /* FIXME */
#define	MAX_CHANNELS		(5)


/* --- typedefs & structures --- */
typedef struct
{
  GslDataHandle dhandle;

  /* setup data */
  guint		sample_rate;
  guint		n_channels;
  guint		frame_size;
  guint         stream_options;
  guint         accumulate_state_frames;
  guint         initial_setup : 1;
  guint         test_setup : 1;

  /* file IO */
  guint         eof : 1;
  gint          fd;
  guint		file_pos;
  const gchar  *error;

  /* seek table */
  guint		n_seeks;
  guint	       *seeks;

  /* file read buffer */
  guint		bfill;
  guint8        buffer[FILE_BUFFER_SIZE + MAD_BUFFER_GUARD];

  /* pcm housekeeping */
  GslLong	pcm_pos, pcm_length, next_pcm_pos;

  /* libmad structures */
  struct mad_stream   stream;
  struct mad_frame    frame;
  struct mad_synth    synth;
} MadHandle;


/* --- prototypes --- */
static GslLong	dh_mad_coarse_seek	(GslDataHandle *data_handle,
					 GslLong        voffset);


/* --- functions --- */
static gboolean		/* FALSE: handle->eof || errno != 0 */
stream_read (MadHandle *handle)
{
  struct mad_stream *stream = &handle->stream;
  guint l;

  /* no further data to read (flag must be reset upon seeks) */
  if (handle->eof)
    return FALSE;

  /* keep remaining data in buffer */
  if (stream->next_frame && handle->bfill)
    {
      handle->bfill = handle->buffer + handle->bfill - stream->next_frame;
      memmove (handle->buffer, stream->next_frame, handle->bfill);
    }

  /* fill buffer */
  do
    l = read (handle->fd, handle->buffer + handle->bfill, FILE_BUFFER_SIZE - handle->bfill);
  while (l < 0 && errno == EINTR);
  if (l > 0)
    {
      handle->bfill += l;
      handle->file_pos += l;
    }
  else if (l == 0)
    {
      handle->eof = TRUE;
      memset (handle->buffer + handle->bfill, 0, MAD_BUFFER_GUARD);
      handle->bfill += MAD_BUFFER_GUARD;
      handle->file_pos += MAD_BUFFER_GUARD;	/* bogus, but doesn't matter at eof */
    }

  mad_stream_buffer (stream, handle->buffer, handle->bfill);

  return l < 0 ? FALSE : TRUE;
}

static gboolean
check_frame_validity (MadHandle         *handle,
		      struct mad_header *header)
{
  guint frame_size = MAD_NSBSAMPLES (header) * 32;
  gchar *reason = NULL;

  if (frame_size <= 0)
    reason = "frame_size < 1";

  if (!handle->initial_setup)
    {
      if (frame_size != handle->frame_size)
	reason = "frame with non-standard size";
      if (MAD_NCHANNELS (header) != handle->n_channels)
	reason = "frame with non-standard channel count";
    }

  if (reason)
    {
      MAD_DEBUG ("skipping frame: %s", reason);
      return FALSE;
    }
  else
    return TRUE;
}

static gboolean
read_next_frame_header (MadHandle *handle)
{
  gboolean succeeded = TRUE;

  /* fetch next frame header */
  if (mad_header_decode (&handle->frame.header, &handle->stream) < 0)
    {
      if (!MAD_RECOVERABLE (handle->stream.error) ||
	  handle->stream.error == MAD_ERROR_LOSTSYNC)
	{
	  /* read on */
	  if (!stream_read (handle))
	    {
	      handle->error = handle->eof ? NULL : g_strerror (errno);
	      return FALSE;
	    }
	  return read_next_frame_header (handle);	/* retry */
	}

      if (!check_frame_validity (handle, &handle->frame.header))
	return read_next_frame_header (handle);		/* retry */

      succeeded = FALSE;
    }

  handle->error = handle->stream.error ? mad_stream_errorstr (&handle->stream) : NULL;

  return succeeded;
}

static gboolean		/* FALSE: handle->eof || handle->error != NULL */
pcm_frame_read (MadHandle *handle,
		gboolean   synth)
{
  gboolean succeeded = TRUE;

  if (mad_frame_decode (&handle->frame, &handle->stream) < 0)
    {
      if (!MAD_RECOVERABLE (handle->stream.error) ||
	  handle->stream.error == MAD_ERROR_LOSTSYNC)
	{
	  /* MAD_RECOVERABLE()==TRUE:  frame was read, decoding failed (about to skip frame)
	   * MAD_RECOVERABLE()==FALSE: frame was not read, need data
	   * note: MAD_RECOVERABLE (MAD_ERROR_LOSTSYNC) == TRUE
	   */

	  /* read on */
	  if (!stream_read (handle))
	    {
	      handle->error = handle->eof ? NULL : g_strerror (errno);
	      return FALSE;
	    }
	  return pcm_frame_read (handle, synth);	/* retry */
	}

      succeeded = FALSE;
      if (synth)
	mad_frame_mute (&handle->frame);
    }

  handle->pcm_pos = handle->next_pcm_pos;
  handle->pcm_length = handle->frame_size;
  handle->next_pcm_pos += handle->pcm_length;

  if (synth)
    mad_synth_frame (&handle->synth, &handle->frame);

  handle->error = handle->stream.error && !succeeded ? mad_stream_errorstr (&handle->stream) : NULL;

  return succeeded;
}

static guint*
create_seek_table (MadHandle *handle,
		   guint     *n_seeks_p)
{
  guint *seeks = NULL;
  guint offs, n_seeks = 0;

  *n_seeks_p = 0;
  mad_synth_finish (&handle->synth);
  mad_frame_finish (&handle->frame);
  mad_stream_finish (&handle->stream);
  mad_stream_init (&handle->stream);
  mad_frame_init (&handle->frame);
  mad_synth_init (&handle->synth);
  mad_stream_options (&handle->stream, handle->stream_options);

  offs = 0;
  if (lseek (handle->fd, offs, SEEK_SET) != offs &&
      lseek (handle->fd, offs, SEEK_SET) != offs) /* retry */
    return NULL;	/* FIXME: errno */
  handle->eof = FALSE;
  handle->bfill = 0;
  handle->file_pos = 0;

  do
    {
      while (read_next_frame_header (handle))
	{
	  guint this_pos = handle->file_pos - handle->bfill + handle->stream.this_frame - handle->buffer;
	  guint i = n_seeks++;
	  
	  if (n_seeks > 256 * 1024)	/* FIXME: max_frames */
	    {
	      g_free (seeks);
	      return NULL;		/* FIXME: ETOOBIG */
	    }
	  
	  if (gsl_alloc_upper_power2 (n_seeks) > gsl_alloc_upper_power2 (i))
	    seeks = g_renew (guint, seeks, gsl_alloc_upper_power2 (n_seeks));
	  seeks[i] = this_pos;
	  
	  if (0)
	    {
	      if (mad_frame_decode (&handle->frame, &handle->stream) < 0)
		MAD_DEBUG ("seektable frame read failed: %s", mad_stream_errorstr (&handle->stream));
	      mad_synth_frame (&handle->synth, &handle->frame);
	      MAD_DEBUG ("frame(%u) PCM:%u => FILE:%u FDIFF:%d (%x %x %x) br:%lu time:%ld/%lu mode:%u ext:%u flags:0x%x phase:%u",
			 i, i * handle->frame_size, this_pos, this_pos - seeks[MAX (i, 1) - 1],
			 handle->stream.this_frame[0], handle->stream.this_frame[1],
			 (handle->stream.this_frame[1] >> 1) & 3,
			 handle->frame.header.bitrate,
			 handle->frame.header.duration.seconds,
			 handle->frame.header.duration.fraction,
			 handle->frame.header.mode,
			 handle->frame.header.mode_extension,
			 handle->frame.header.flags,
			 handle->synth.phase);
	    }
	}
      
      if (!handle->eof)
	{
	  MAD_DEBUG ("reading seektable frame failed: %s", handle->error ? handle->error : "Unknown");
	  
	  /* frame read failed for a reason other than eof */
	  g_free (seeks);
	  return NULL;		/* FIXME: EIO/errno */
	}
    }
  while (!handle->eof);
      
  /* reset file offset */
  offs = 0;
  if (lseek (handle->fd, offs, SEEK_SET) != offs)
    {
      g_free (seeks);
      return NULL;
    }
  handle->eof = FALSE;
  handle->file_pos = 0;
  handle->bfill = 0;
  
  /* shrink table */
  seeks = g_renew (guint, seeks, n_seeks);
  *n_seeks_p = n_seeks;

  return seeks;
}

static gint
dh_mad_open (GslDataHandle *data_handle)
{
  MadHandle *handle = (MadHandle*) data_handle;
  GslLong n;
  gint fd;

  if (!handle->initial_setup && !gsl_check_file_mtime (handle->dhandle.name, handle->dhandle.mtime))
    return EBADFD;

  fd = open (handle->dhandle.name, O_RDONLY);
  if (fd < 0)
    return errno ? errno : EIO;

  handle->fd = fd;
  handle->bfill = 0;
  handle->eof = FALSE;
  handle->pcm_pos = 0;
  handle->pcm_length = 0;
  handle->next_pcm_pos = 0;
  handle->file_pos = 0;
  mad_stream_init (&handle->stream);
  mad_frame_init (&handle->frame);
  mad_synth_init (&handle->synth);
  mad_stream_options (&handle->stream, handle->stream_options);

  /* fetch first frame */
  if (!read_next_frame_header (handle))
    goto OPEN_FAILED;

  /* get n_channels, and assert it to be constant throughout stream */
  n = MAD_NCHANNELS (&handle->frame.header);
  if (handle->initial_setup && n > 0)
    handle->n_channels = n;
  else if (n != handle->n_channels || !n)
    goto OPEN_FAILED;

  /* same with decoded frame size */
  n = MAD_NSBSAMPLES (&handle->frame.header) * 32;
  if (handle->initial_setup && n > 0)
    handle->frame_size = n;
  else if (n != handle->frame_size || !n)
    goto OPEN_FAILED;

  /* and sample rate */
  n = handle->frame.header.samplerate;
  if (handle->initial_setup && n > 0)
    handle->sample_rate = n;
  else if (n != handle->sample_rate || !n)
    goto OPEN_FAILED;

  /* seek through the stream to collect frame positions */
  if (handle->test_setup)
    {
      /* fake seek table */
      handle->eof = FALSE;
      handle->file_pos = 0;
      handle->bfill = 0;
      handle->n_seeks = 1;
      handle->seeks = g_new (guint, handle->n_seeks);
      handle->seeks[0] = 0;
    }
  else if (handle->initial_setup)
    {
      handle->seeks = create_seek_table (handle, &handle->n_seeks);
      if (!handle->seeks)
	goto OPEN_FAILED;
      MAD_DEBUG ("frames in seektable: %u", handle->n_seeks);
    }

  /* validate/setup handle length */
  n = handle->n_seeks * handle->frame_size * handle->n_channels;
  if (handle->initial_setup && n > 0)
    handle->dhandle.n_values = n;
  else if (n != handle->dhandle.n_values || !n)
    goto OPEN_FAILED;

  if (dh_mad_coarse_seek (&handle->dhandle, 0) != 0)
    goto OPEN_FAILED;

  return 0;

 OPEN_FAILED:
  if (handle->initial_setup)
    {
      g_free (handle->seeks);
      handle->seeks = NULL;
      handle->n_seeks = 0;
    }
  handle->bfill = 0;
  handle->eof = FALSE;
  handle->pcm_pos = 0;
  handle->pcm_length = 0;
  handle->next_pcm_pos = 0;
  handle->file_pos = 0;
  mad_synth_finish (&handle->synth);
  mad_frame_finish (&handle->frame);
  mad_stream_finish (&handle->stream);
  close (handle->fd);
  handle->fd = -1;

  return EIO;
}

static GslLong
dh_mad_read (GslDataHandle *data_handle,
	     GslLong        voffset, /* in values */
	     GslLong        n_values,
	     gfloat        *values)
{
  MadHandle *handle = (MadHandle*) data_handle;
  GslLong pos = voffset / handle->n_channels;
  gboolean frame_read_ok = TRUE;

  if (pos < handle->pcm_pos ||
      pos >= handle->pcm_pos + handle->pcm_length + SEEK_BY_READ_AHEAD (handle) * handle->frame_size)
    {
      GslLong tmp;

      /* suckage, need to do lengthy seek in file */
      tmp = dh_mad_coarse_seek (data_handle, voffset);
      g_assert (tmp <= voffset);
    }
 
  while (pos >= handle->pcm_pos + handle->pcm_length)
    frame_read_ok = pcm_frame_read (handle, TRUE);

  /* check if the last call to pcm_frame_read() failed */
  if (!frame_read_ok)
    {
      if (handle->stream.error == MAD_ERROR_BADDATAPTR)
	{
	  /* if we encounter that the inter-frame accumulated layer-III state
	   * is not complete now, we'll try to increase the amount of frames
	   * we accumulate
	   */
	  if (handle->accumulate_state_frames < 10)
	    {
	      handle->accumulate_state_frames++;
	      MAD_DEBUG ("retrying seek with accumulate_state_frames=%d",
			 handle->accumulate_state_frames);

	      /* force dh_mad_read to retry the seek */
	      dh_mad_coarse_seek (data_handle, 0);
	      return dh_mad_read (data_handle, voffset, n_values, values);
	    }
	  else
	    {
	      MAD_DEBUG ("synthesizing frame failed, accumulate_state_frames is already %u: %s",
			 handle->accumulate_state_frames, handle->error);
	      return -1;
	    }
	}
      else
	{
	  MAD_DEBUG ("failed to synthesize frame: %s", handle->error);
	  return -1;
	}
    }

  n_values = MIN (n_values, handle->pcm_length * handle->n_channels);

  /* interleave into output buffer */
  if (pos >= handle->pcm_pos && pos < handle->pcm_pos + handle->pcm_length)
    {
      guint offset = voffset - handle->pcm_pos * handle->n_channels;
      guint align = offset % handle->n_channels;
      guint n_samples = MIN (n_values, handle->pcm_length * handle->n_channels - offset);
      mad_fixed_t *pcm[MAX_CHANNELS];
      gfloat *bound = values + n_samples;
      guint i;

      offset /= handle->n_channels;
      for (i = 0; i < handle->n_channels; i++)
	pcm[i] = handle->synth.pcm.samples[i] + offset + (i < align);
      
      for (i = align; values < bound; values++)
	{
	  mad_fixed_t mf = *(pcm[i]++);

	  *values = CLAMP (mf, -MAD_F_ONE, MAD_F_ONE) * (1. / (double) MAD_F_ONE);
	  if (++i >= handle->n_channels)
	    i = 0;
	}
      return n_samples;
    }
  else /* something went wrong here, _badly_ */
    {
      MAD_MSG (GSL_ERROR_READ_FAILED,
	       "pcm position screwed (pos: %lu, handle-pos: %lu), aborting read",
	       pos, handle->pcm_pos);	
      return -1;
    }
}

static GslLong
dh_mad_coarse_seek (GslDataHandle *data_handle,
		    GslLong        voffset)
{
  MadHandle *handle = (MadHandle*) data_handle;
  GslLong opos = handle->pcm_pos, pos = voffset / handle->n_channels;

  if (voffset < 0)	/* pcm_tell() */
    return handle->pcm_pos * handle->n_channels;

  if (pos < handle->pcm_pos ||
      pos >= handle->pcm_pos + handle->pcm_length + SEEK_BY_READ_AHEAD (handle))
    {
      GslLong offs = pos;
      guint i, file_pos;

      /* reset decoder state */
      mad_synth_finish (&handle->synth);
      mad_frame_finish (&handle->frame);
      mad_stream_finish (&handle->stream);
      mad_stream_init (&handle->stream);
      mad_frame_init (&handle->frame);
      mad_synth_init (&handle->synth);
      mad_stream_options (&handle->stream, handle->stream_options);

      /* seek to some frames read ahead to accumulate layer III IDCMT state */
      offs -= (gint) (handle->frame_size * handle->accumulate_state_frames);
      offs = CLAMP (offs, 0, (gint) (handle->n_seeks * handle->frame_size));

      /* get file position from seek table */
      i = offs / handle->frame_size;
      file_pos = handle->seeks[i];

      /* perform file seek and adjust positions */
      if (lseek (handle->fd, file_pos, SEEK_SET) != file_pos)
	{
	  /* retry */
	  if (lseek (handle->fd, file_pos, SEEK_SET) != file_pos)
	    {
	      /* this is baaaad */
	      MAD_MSG (GSL_ERROR_IO,
		       "file \"%s\" seek failed: %s",
		       handle->dhandle.name, g_strerror (errno));
	      lseek (handle->fd, 0, SEEK_SET);
	    }
	}
      handle->eof = FALSE;
      handle->bfill = 0;
      handle->file_pos = file_pos;
      handle->pcm_pos = i * handle->frame_size;
      handle->pcm_length = 0;
      handle->next_pcm_pos = handle->pcm_pos;

#if 0
      /* adapt synth phase */
      handle->synth.phase = ((i + 1) * (handle->frame_size / 32)) % 16;
#endif

      /* accumulate state */
      if (pos >= handle->accumulate_state_frames * handle->frame_size)
	{
	  guint i;
	  for (i = 0; i < handle->accumulate_state_frames; i++)
	    {
	      gboolean synth = i + 1 == handle->accumulate_state_frames;

	      if (!pcm_frame_read (handle, synth) && handle->stream.error != MAD_ERROR_BADDATAPTR)
		MAD_DEBUG ("COARSE-SEEK: frame read ahead (%u): failed: %s", i, handle->error);
	    }
	}

      MAD_DEBUG ("seek-done: at %lu (f:%lu) want %lu (f:%lu) got %lu (f:%lu) diff %ld (diff-requested %ld)",
		 opos, opos / handle->frame_size,
		 pos, pos / handle->frame_size,
		 handle->pcm_pos, handle->pcm_pos / handle->frame_size,
		 handle->pcm_pos - opos, pos - opos);
    }

  return handle->pcm_pos * handle->n_channels;
}

static void
dh_mad_close (GslDataHandle *data_handle)
{
  MadHandle *handle = (MadHandle*) data_handle;

  handle->bfill = 0;
  handle->eof = FALSE;
  handle->pcm_pos = 0;
  handle->pcm_length = 0;
  handle->next_pcm_pos = 0;
  handle->file_pos = 0;
  mad_synth_finish (&handle->synth);
  mad_frame_finish (&handle->frame);
  mad_stream_finish (&handle->stream);
  close (handle->fd);
  handle->fd = -1;
}

static void
dh_mad_destroy (GslDataHandle *data_handle)
{
  MadHandle *handle = (MadHandle*) data_handle;

  g_free (handle->seeks);
  handle->seeks = NULL;
  handle->n_seeks = 0;
  gsl_data_handle_common_free (data_handle);
  gsl_delete_struct (MadHandle, handle);
}

static GslDataHandleFuncs dh_mad_vtable = {
  dh_mad_open,
  dh_mad_read,
  dh_mad_close,
  dh_mad_destroy,
  dh_mad_coarse_seek,
};

static GslDataHandle*
dh_mad_new (const gchar *file_name,
	    gboolean     need_seek_table)
{
  MadHandle *handle;
  gboolean success;

  handle = gsl_new_struct0 (MadHandle, 1);
  success = gsl_data_handle_common_init (&handle->dhandle, file_name, 24);
  if (success)
    {
      gint err;

      handle->dhandle.vtable = &dh_mad_vtable;
      handle->dhandle.n_values = 0;
      handle->sample_rate = 0;
      handle->n_channels = 0;
      handle->frame_size = 0;
      handle->stream_options = MAD_OPTION_IGNORECRC;
      handle->accumulate_state_frames = 0;
      handle->eof = FALSE;
      handle->fd = -1;
      handle->file_pos = 0;
      handle->error = NULL;
      handle->n_seeks = 0;
      handle->seeks = NULL;
      handle->bfill = 0;
      handle->pcm_pos = handle->pcm_length = handle->next_pcm_pos = 0;

      /* we can only do the remaining setup and check matters
       * after we actually opened the handle
       */
      handle->initial_setup = TRUE;
      handle->test_setup = !need_seek_table;
      err = gsl_data_handle_open (&handle->dhandle);
      handle->initial_setup = FALSE;
      handle->test_setup = FALSE;

      /* check setup
       */
      if (!err &&
	  handle->dhandle.n_values &&
	  handle->sample_rate > 0 &&
	  handle->n_channels > 0 &&
	  handle->n_channels <= MAX_CHANNELS)
	{
	  /* handle ok, creation succeeded */
	  gsl_data_handle_close (&handle->dhandle);

	  return &handle->dhandle;
	}

      /* failed to create handle properly, clean up */
      if (!err)
	gsl_data_handle_close (&handle->dhandle);
      gsl_data_handle_unref (&handle->dhandle);
      return NULL;
    }
  else
    {
      g_free (handle->seeks);
      gsl_delete_struct (MadHandle, handle);
      return NULL;
    }
}

GslDataHandle*
gsl_data_handle_new_mad (const gchar *file_name)
{
  g_return_val_if_fail (file_name != NULL, NULL);

  return dh_mad_new (file_name, TRUE);
}

GslErrorType
gsl_data_handle_mad_testopen (const gchar *file_name,
			      guint       *n_channels,
			      gfloat      *mix_freq)
{
  GslDataHandle *dhandle;
  MadHandle *handle;
  
  g_return_val_if_fail (file_name != NULL, GSL_ERROR_INTERNAL);

  dhandle = dh_mad_new (file_name, FALSE);
  if (!dhandle)
    return GSL_ERROR_OPEN_FAILED;

  handle = (MadHandle*) dhandle;
  if (n_channels)
    *n_channels = handle->n_channels;
  if (mix_freq)
    *mix_freq = handle->sample_rate;
  gsl_data_handle_unref (dhandle);

  return GSL_ERROR_NONE;
}

#else	/* !GSL_HAVE_LIBMAD */

GslDataHandle*
gsl_data_handle_new_mad (const gchar *file_name)
{
  return NULL;
}

GslErrorType
gsl_data_handle_mad_testopen (const gchar *file_name,
			      guint       *n_channels,
			      gfloat      *mix_freq)
{
  return GSL_ERROR_FORMAT_UNKNOWN;
}

#endif	/* !GSL_HAVE_LIBMAD */

/* vim:set ts=8 sts=2 sw=2: */
