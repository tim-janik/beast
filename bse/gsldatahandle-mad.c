/* GSL - Generic Sound Layer
 * Copyright (C) 2001-2003 Tim Janik and Stefan Westerfeld
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
#include <bse/gsldatahandle-mad.h>

#include "gslfilehash.h"
#include <bse/gsldatautils.h>
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
static SFI_MSG_TYPE_DEFINE (debug_mad, "mad", SFI_MSG_DEBUG, NULL);
#define MAD_DEBUG(...)  sfi_debug (debug_mad, __VA_ARGS__)


/* --- defines --- */
#define	FILE_BUFFER_SIZE	(1024 * 44)	/* approximately 1 second at 320 kbit */
#define	SEEK_BY_READ_AHEAD(h)	(((h)->sample_rate / ((h)->frame_size * 2))) /* FIXME */
#define	MAX_CHANNELS		(5)


/* --- typedefs & structures --- */
typedef struct
{
  GslDataHandle dhandle;

  /* setup data */
  gfloat        osc_freq;
  guint		sample_rate;
  guint		frame_size;
  guint         stream_options;
  guint         accumulate_state_frames;
  guint         skip_seek_table : 1;

  /* file IO */
  guint         eof : 1;
  GslHFile     *hfile;
  guint		file_pos;
  BseErrorType  error;

  /* seek table */
  GTime		seek_mtime;
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
static GslLong	dh_mad_coarse_seek	(GslDataHandle *dhandle,
					 GslLong        voffset);


/* --- functions --- */
static BseErrorType
error_from_mad_stream (struct mad_stream *mstream,
                       BseErrorType       fallback)
{
  switch (mstream->error)
    {
    case MAD_ERROR_NONE:                return BSE_ERROR_NONE;
    case MAD_ERROR_BUFLEN:              return BSE_ERROR_FILE_EOF;
    case MAD_ERROR_BUFPTR:              return fallback;
    case MAD_ERROR_NOMEM:               return BSE_ERROR_NO_MEMORY;
    case MAD_ERROR_LOSTSYNC:            return BSE_ERROR_DATA_CORRUPT;
    case MAD_ERROR_BADLAYER:            return BSE_ERROR_FORMAT_UNKNOWN;
    case MAD_ERROR_BADBITRATE:          return BSE_ERROR_FORMAT_INVALID;
    case MAD_ERROR_BADSAMPLERATE:       return BSE_ERROR_FORMAT_INVALID;
    case MAD_ERROR_BADEMPHASIS:         return BSE_ERROR_FORMAT_INVALID;
    case MAD_ERROR_BADCRC:              return BSE_ERROR_DATA_CORRUPT;
    case MAD_ERROR_BADBITALLOC:         return BSE_ERROR_FORMAT_INVALID;
    case MAD_ERROR_BADSCALEFACTOR:      return BSE_ERROR_FORMAT_INVALID;
    case MAD_ERROR_BADMODE:             return BSE_ERROR_FORMAT_INVALID;
    case MAD_ERROR_BADFRAMELEN:         return BSE_ERROR_FORMAT_INVALID;
    case MAD_ERROR_BADBIGVALUES:        return BSE_ERROR_FORMAT_INVALID;
    case MAD_ERROR_BADBLOCKTYPE:        return BSE_ERROR_FORMAT_UNKNOWN;
    case MAD_ERROR_BADSCFSI:            return BSE_ERROR_FORMAT_INVALID;
    case MAD_ERROR_BADDATAPTR:          return BSE_ERROR_FORMAT_INVALID;
    case MAD_ERROR_BADPART3LEN:         return BSE_ERROR_FORMAT_INVALID;
    case MAD_ERROR_BADHUFFTABLE:        return BSE_ERROR_FORMAT_INVALID;
    case MAD_ERROR_BADHUFFDATA:         return BSE_ERROR_DATA_CORRUPT;
    case MAD_ERROR_BADSTEREO:           return BSE_ERROR_FORMAT_INVALID;
    default:                            return fallback;
    }
}

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
  l = gsl_hfile_pread (handle->hfile, handle->file_pos, FILE_BUFFER_SIZE - handle->bfill, handle->buffer + handle->bfill);
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

  if (handle->frame_size && handle->dhandle.setup.n_channels)
    {
#if 0
      if (frame_size != handle->frame_size)
	reason = "frame with non-standard size";
#endif
      if (MAD_NCHANNELS (header) != handle->dhandle.setup.n_channels)
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
	      handle->error = handle->eof ? 0 : gsl_error_from_errno (errno, BSE_ERROR_FILE_EOF);
	      return FALSE;
	    }
	  return read_next_frame_header (handle);	/* retry */
	}

      if (!check_frame_validity (handle, &handle->frame.header))
	return read_next_frame_header (handle);		/* retry */

      succeeded = FALSE;
    }

  handle->error = handle->stream.error ? error_from_mad_stream (&handle->stream, BSE_ERROR_FILE_SEEK_FAILED) : 0;

  return succeeded;
}

static gboolean		/* FALSE: handle->eof || handle->error != 0 */
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
	      handle->error = handle->eof ? 0 : gsl_error_from_errno (errno, BSE_ERROR_FILE_READ_FAILED);
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

  if (!succeeded && handle->stream.error)
    handle->error = error_from_mad_stream (&handle->stream, BSE_ERROR_FILE_READ_FAILED);
  else
    handle->error = 0;
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
  /* lseek (handle->hfile, offs, SEEK_SET) */
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
	  
	  if (sfi_alloc_upper_power2 (n_seeks) > sfi_alloc_upper_power2 (i))
	    seeks = g_renew (guint, seeks, sfi_alloc_upper_power2 (n_seeks));
	  seeks[i] = this_pos;
	  
	  if (0)
	    {
	      if (mad_frame_decode (&handle->frame, &handle->stream) < 0)
		MAD_DEBUG ("failed to read seektable frame: %s", mad_stream_errorstr (&handle->stream));
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
	  g_free (seeks);
	  /* frame read failed for a reason other than eof */
          if (!handle->error)
            handle->error = BSE_ERROR_IO;
	  MAD_DEBUG ("failed to read seektable frame: %s", handle->stream.error ? mad_stream_errorstr (&handle->stream) : bse_error_blurb (handle->error));
	  return NULL;
	}
    }
  while (!handle->eof);
      
  /* reset file offset */
  offs = 0;
  /* lseek (handle->hfile, offs, SEEK_SET) */
  handle->eof = FALSE;
  handle->file_pos = 0;
  handle->bfill = 0;
  
  /* shrink table */
  seeks = g_renew (guint, seeks, n_seeks);
  *n_seeks_p = n_seeks;

  return seeks;
}

static BseErrorType
dh_mad_open (GslDataHandle      *dhandle,
	     GslDataHandleSetup *setup)
{
  MadHandle *handle = (MadHandle*) dhandle;
  GslHFile *hfile;
  GslLong n;
  gboolean seek_invalidated = FALSE;
  BseErrorType error;

  hfile = gsl_hfile_open (handle->dhandle.name);
  if (!hfile)
    return gsl_error_from_errno (errno, BSE_ERROR_FILE_OPEN_FAILED);
  handle->hfile = hfile;

  seek_invalidated |= handle->seek_mtime != hfile->mtime;
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
    {
      error = BSE_ERROR_NO_HEADER;
      goto OPEN_FAILED;
    }

  /* get n_channels, frame size and sample rate */
  setup->n_channels = MAD_NCHANNELS (&handle->frame.header);
  n = MAD_NSBSAMPLES (&handle->frame.header) * 32;
  seek_invalidated |= n != handle->frame_size;
  handle->frame_size = n;
  handle->sample_rate = handle->frame.header.samplerate;
  if (setup->n_channels < 1 ||
      setup->n_channels > MAX_CHANNELS ||
      handle->sample_rate < 3999 ||
      handle->frame_size < 1 ||
      handle->sample_rate < 1)
    {
      error = BSE_ERROR_FORMAT_INVALID;
      goto OPEN_FAILED;
    }

  /* our code already uses dhandle->setup */
  dhandle->setup = *setup;

  /* seek through the stream to collect frame positions */
  if (seek_invalidated || !handle->n_seeks)
    {
      handle->seek_mtime = hfile->mtime;
      handle->n_seeks = 0;
      g_free (handle->seeks);
      handle->seeks = NULL;
      if (handle->skip_seek_table)
	{
	  /* fake seek table */
	  handle->n_seeks = 1;
	  handle->seeks = g_new (guint, handle->n_seeks);
	  handle->seeks[0] = 0;
	}
      else
	{
	  handle->seeks = create_seek_table (handle, &handle->n_seeks);
	  if (!handle->seeks)
	    {
	      error = BSE_ERROR_NO_SEEK_INFO;
	      goto OPEN_FAILED;
	    }
	  MAD_DEBUG ("frames in seektable: %u", handle->n_seeks);
	}
    }
  
  /* validate/setup handle length */
  n = handle->n_seeks * handle->frame_size * setup->n_channels;
  if (n > 0)
    setup->n_values = n;
  else
    {
      error = BSE_ERROR_NO_DATA;
      goto OPEN_FAILED;
    }

  /* our code already uses dhandle->setup */
  dhandle->setup = *setup;
  
  if (dh_mad_coarse_seek (&handle->dhandle, 0) != 0)
    {
      error = BSE_ERROR_FILE_SEEK_FAILED;
      goto OPEN_FAILED;
    }
  
  setup->bit_depth = 24;
  setup->mix_freq = handle->sample_rate;
  setup->needs_cache = TRUE;
  setup->xinfos = bse_xinfos_add_float (setup->xinfos, "osc-freq", handle->osc_freq);
  return BSE_ERROR_NONE;

 OPEN_FAILED:
  if (handle->error)
    error = handle->error;
  g_free (handle->seeks);
  handle->seeks = NULL;
  handle->n_seeks = 0;
  handle->seek_mtime = -1;
  handle->bfill = 0;
  handle->eof = FALSE;
  handle->pcm_pos = 0;
  handle->pcm_length = 0;
  handle->next_pcm_pos = 0;
  handle->file_pos = 0;
  mad_synth_finish (&handle->synth);
  mad_frame_finish (&handle->frame);
  mad_stream_finish (&handle->stream);
  gsl_hfile_close (handle->hfile);
  handle->hfile = NULL;

  return error;
}

static GslLong
dh_mad_read (GslDataHandle *dhandle,
	     GslLong        voffset, /* in values */
	     GslLong        n_values,
	     gfloat        *values)
{
  MadHandle *handle = (MadHandle*) dhandle;
  GslLong pos = voffset / dhandle->setup.n_channels;
  gboolean frame_read_ok = TRUE;

  if (pos < handle->pcm_pos ||
      pos >= handle->pcm_pos + handle->pcm_length + SEEK_BY_READ_AHEAD (handle) * handle->frame_size)
    {
      GslLong tmp;

      /* suckage, need to do lengthy seek in file */
      tmp = dh_mad_coarse_seek (dhandle, voffset);
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
	      dh_mad_coarse_seek (dhandle, 0);
	      return dh_mad_read (dhandle, voffset, n_values, values);
	    }
	  else
	    {
	      MAD_DEBUG ("synthesizing frame failed, accumulate_state_frames is already %u: %s",
			 handle->accumulate_state_frames, handle->stream.error ? mad_stream_errorstr (&handle->stream) : bse_error_blurb (handle->error));
	      return -1;
	    }
	}
      else
	{
	  MAD_DEBUG ("failed to synthesize frame: %s", handle->stream.error ? mad_stream_errorstr (&handle->stream) : bse_error_blurb (handle->error));
	  return -1;
	}
    }

  n_values = MIN (n_values, handle->pcm_length * dhandle->setup.n_channels);

  /* interleave into output buffer */
  if (pos >= handle->pcm_pos && pos < handle->pcm_pos + handle->pcm_length)
    {
      guint offset = voffset - handle->pcm_pos * dhandle->setup.n_channels;
      guint align = offset % dhandle->setup.n_channels;
      guint n_samples = MIN (n_values, handle->pcm_length * dhandle->setup.n_channels - offset);
      mad_fixed_t *pcm[MAX_CHANNELS];
      gfloat *bound = values + n_samples;
      guint i;

      offset /= dhandle->setup.n_channels;
      for (i = 0; i < dhandle->setup.n_channels; i++)
	pcm[i] = handle->synth.pcm.samples[i] + offset + (i < align);
      
      for (i = align; values < bound; values++)
	{
	  mad_fixed_t mf = *(pcm[i]++);

	  *values = CLAMP (mf, -MAD_F_ONE, MAD_F_ONE) * (1. / (double) MAD_F_ONE);
	  if (++i >= dhandle->setup.n_channels)
	    i = 0;
	}
      return n_samples;
    }
  else /* something went wrong here, _badly_ */
    {
      sfi_diag ("MAD-init: pcm position screwed (pos: %lu, handle-pos: %lu), aborting read",
                pos, handle->pcm_pos);	
      return -1;
    }
}

static GslLong
dh_mad_coarse_seek (GslDataHandle *dhandle,
		    GslLong        voffset)
{
  MadHandle *handle = (MadHandle*) dhandle;
  GslLong opos = handle->pcm_pos, pos = voffset / dhandle->setup.n_channels;

  if (voffset < 0)	/* pcm_tell() */
    return handle->pcm_pos * dhandle->setup.n_channels;

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
      /* lseek (handle->hfile, file_pos, SEEK_SET) */
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
		MAD_DEBUG ("failed to read frame ahead (%u) in coarse-seek: failed: %s", i,
                           handle->stream.error ? mad_stream_errorstr (&handle->stream) : bse_error_blurb (handle->error));
	    }
	}

      MAD_DEBUG ("seek-done: at %lu (f:%lu) want %lu (f:%lu) got %lu (f:%lu) diff %ld (diff-requested %ld)",
		 opos, opos / handle->frame_size,
		 pos, pos / handle->frame_size,
		 handle->pcm_pos, handle->pcm_pos / handle->frame_size,
		 handle->pcm_pos - opos, pos - opos);
    }

  return handle->pcm_pos * dhandle->setup.n_channels;
}

static void
dh_mad_close (GslDataHandle *dhandle)
{
  MadHandle *handle = (MadHandle*) dhandle;

  g_strfreev (dhandle->setup.xinfos);
  dhandle->setup.xinfos = NULL;
  handle->bfill = 0;
  handle->eof = FALSE;
  handle->pcm_pos = 0;
  handle->pcm_length = 0;
  handle->next_pcm_pos = 0;
  handle->file_pos = 0;
  mad_synth_finish (&handle->synth);
  mad_frame_finish (&handle->frame);
  mad_stream_finish (&handle->stream);
  gsl_hfile_close (handle->hfile);
  handle->hfile = NULL;
}

static void
dh_mad_destroy (GslDataHandle *dhandle)
{
  MadHandle *handle = (MadHandle*) dhandle;

  g_free (handle->seeks);
  handle->seeks = NULL;
  handle->n_seeks = 0;
  gsl_data_handle_common_free (dhandle);
  sfi_delete_struct (MadHandle, handle);
}

static GslDataHandleFuncs dh_mad_vtable = {
  dh_mad_open,
  dh_mad_read,
  dh_mad_close,
  NULL,
  dh_mad_destroy,
};

static GslDataHandle*
dh_mad_new (const gchar  *file_name,
            gfloat        osc_freq,
	    gboolean      skip_seek_keep_open,
            BseErrorType *errorp)
{
  MadHandle *handle;
  gboolean success;

  handle = sfi_new_struct0 (MadHandle, 1);
  success = gsl_data_handle_common_init (&handle->dhandle, file_name);
  if (success)
    {
      handle->dhandle.vtable = &dh_mad_vtable;
      handle->osc_freq = osc_freq;
      handle->sample_rate = 0;
      handle->frame_size = 0;
      handle->stream_options = MAD_OPTION_IGNORECRC;
      handle->accumulate_state_frames = 0;
      handle->eof = FALSE;
      handle->hfile = NULL;
      handle->file_pos = 0;
      handle->error = 0;
      handle->n_seeks = 0;
      handle->seeks = NULL;
      handle->seek_mtime = -1;
      handle->bfill = 0;
      handle->pcm_pos = handle->pcm_length = handle->next_pcm_pos = 0;

      /* we can only check matters upon opening
       */
      handle->skip_seek_table = skip_seek_keep_open != FALSE;
      BseErrorType error = gsl_data_handle_open (&handle->dhandle);
      if (!error)
	{
	  if (!skip_seek_keep_open)
	    gsl_data_handle_close (&handle->dhandle);
          *errorp = 0;
	  return &handle->dhandle;
	}
      MAD_DEBUG ("failed to open \"%s\": %s", file_name, handle->stream.error ? mad_stream_errorstr (&handle->stream) : bse_error_blurb (error));
      gsl_data_handle_unref (&handle->dhandle);
      *errorp = error;
      return NULL;
    }
  else
    {
      g_free (handle->seeks);
      sfi_delete_struct (MadHandle, handle);
      *errorp = BSE_ERROR_INTERNAL;
      return NULL;
    }
}

GslDataHandle*
gsl_data_handle_new_mad_err (const gchar  *file_name,
                             gfloat        osc_freq,
                             BseErrorType *errorp)
{
  g_return_val_if_fail (file_name != NULL, NULL);
  g_return_val_if_fail (osc_freq > 0, NULL);

  BseErrorType error = 0;
  return dh_mad_new (file_name, osc_freq, FALSE, errorp ? errorp : &error);
}

BseErrorType
gsl_data_handle_mad_testopen (const gchar *file_name,
			      guint       *n_channels,
			      gfloat      *mix_freq)
{
  GslDataHandle *dhandle;
  MadHandle *handle;
  
  g_return_val_if_fail (file_name != NULL, BSE_ERROR_INTERNAL);

  BseErrorType error = 0;
  dhandle = dh_mad_new (file_name, 439, TRUE, &error);
  if (!dhandle)
    return error ? error : BSE_ERROR_FILE_OPEN_FAILED;

  handle = (MadHandle*) dhandle;
  if (n_channels)
    *n_channels = handle->dhandle.setup.n_channels;
  if (mix_freq)
    *mix_freq = handle->sample_rate;
  gsl_data_handle_close (dhandle);
  gsl_data_handle_unref (dhandle);

  return BSE_ERROR_NONE;
}

const gchar*
gsl_data_handle_mad_version (void)
{
  return MAD_VERSION;
}

#else	/* !GSL_HAVE_LIBMAD */

GslDataHandle*
gsl_data_handle_new_mad_err (const gchar  *file_name,
                             gfloat        osc_freq,
                             BseErrorType *errorp)
{
  if (errorp)
    *errorp = BSE_ERROR_FORMAT_UNKNOWN;
  return NULL;
}

BseErrorType
gsl_data_handle_mad_testopen (const gchar *file_name,
                              guint       *n_channels,
                              gfloat      *mix_freq)
{
  return BSE_ERROR_FORMAT_UNKNOWN;
}

const gchar*
gsl_data_handle_mad_version (void)
{
  return NULL;
}

#endif	/* !GSL_HAVE_LIBMAD */

GslDataHandle*
gsl_data_handle_new_mad (const gchar  *file_name,
                         gfloat        osc_freq)
{
  return gsl_data_handle_new_mad_err (file_name, osc_freq, NULL);
}

/* vim:set ts=8 sts=2 sw=2: */
