/* GSL - Generic Sound Layer
 * Copyright (C) 2001-2002 Tim Janik
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
#include "gsldatahandle-vorbis.h"

#include "gslfilehash.h"
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
#include <errno.h>


/* --- defines --- */
#define	MAX_CHANNELS			(16)		/* hard limit, eases our life somewhat */
/* number of values to decode and throw away instead of seeking. since
 * seeking can be quite time consuming, this should cover a good range
 * of seek-ahead space
 */
#define	SEEK_BY_READ_AHEAD(vhandle)	(vhandle->max_block_size * 8)


/* --- structure --- */
typedef struct {
  GslDataHandle dhandle;

  guint    stream;
  guint    n_streams;

  /* live data */
  gint64  soffset;	/* our PCM start offset */
  guint   max_block_size;

  /* pcm read out cache */
  GslLong pcm_pos, pcm_length;
  gfloat *pcm[MAX_CHANNELS];

  OggVorbis_File ofile;
} VorbisHandle;


/* --- functions --- */
static GslErrorType
ov_errno_to_error (gint         ov_errno,
		   GslErrorType fallback)
{
  switch (ov_errno)
    {
    case OV_EOF:	return GSL_ERROR_EOF;
    case OV_EBADLINK:
    case OV_EBADPACKET:
    case OV_HOLE:	return GSL_ERROR_DATA_CORRUPT;
    case OV_EREAD:	return GSL_ERROR_READ_FAILED;
    case OV_ENOSEEK:	return GSL_ERROR_SEEK_FAILED;
    case OV_EFAULT:
    case OV_EIMPL:	return GSL_ERROR_CODEC_FAILURE;
    case OV_EINVAL:	return GSL_ERROR_INTERNAL;
    case OV_ENOTAUDIO:
    case OV_EVERSION:
    case OV_EBADHEADER:
    case OV_ENOTVORBIS:	return GSL_ERROR_FORMAT_INVALID;
    case OV_FALSE:
    default:		return fallback;
    }
}

static size_t
rfile_read (void  *ptr,
	    size_t size,
	    size_t nmemb,
	    void  *datasource)
{
  GslRFile *rfile = datasource;
  return gsl_rfile_read (rfile, size * nmemb, ptr);
}

static int
rfile_seek (void       *datasource,
	    ogg_int64_t offset,
	    int         whence)
{
  GslRFile *rfile = datasource;
  GslLong l;
  switch (whence)
    {
    default:
    case SEEK_SET:
      l = gsl_rfile_seek_set (rfile, offset);
      break;
    case SEEK_CUR:
      l = gsl_rfile_position (rfile);
      l = gsl_rfile_seek_set (rfile, l + offset);
      break;
    case SEEK_END:
      l = gsl_rfile_length (rfile);
      l = gsl_rfile_seek_set (rfile, l + offset);
      break;
    }
  return l;
}

static int
rfile_close (void *datasource)
{
  GslRFile *rfile = datasource;
  gsl_rfile_close (rfile);
  return 0;
}

static long
rfile_tell (void *datasource)
{
  GslRFile *rfile = datasource;
  return gsl_rfile_position (rfile);
}

static ov_callbacks rfile_ov_callbacks = {
  rfile_read,
  rfile_seek,
  rfile_close,
  rfile_tell,
};

static GslErrorType
dh_vorbis_open (GslDataHandle      *data_handle,
		GslDataHandleSetup *setup)
{
  VorbisHandle *vhandle = (VorbisHandle*) data_handle;
  GslRFile *rfile;
  vorbis_info *vi;
  GslLong n, i;
  gint err;
  
  rfile = gsl_rfile_open (vhandle->dhandle.name);
  if (!rfile)
    return gsl_error_from_errno (errno, GSL_ERROR_OPEN_FAILED);
  err = ov_open_callbacks (rfile, &vhandle->ofile, NULL, 0, rfile_ov_callbacks);
  if (err < 0)
    {
      gsl_rfile_close (rfile);
      return ov_errno_to_error (err, GSL_ERROR_OPEN_FAILED);
    }

  n = ov_streams (&vhandle->ofile);
  if (n > vhandle->stream)
    vhandle->n_streams = n;
  else
    {
      ov_clear (&vhandle->ofile); /* closes file */
      return GSL_ERROR_NO_DATA;	/* requested stream not available */
    }

  vhandle->soffset = 0;
  for (i = 0; i < vhandle->stream; i++)
    vhandle->soffset += ov_pcm_total (&vhandle->ofile, i);

  n = ov_pcm_total (&vhandle->ofile, vhandle->stream);
  vi = ov_info (&vhandle->ofile, vhandle->stream);
  if (n > 0 && vi && vi->channels && ov_pcm_seek (&vhandle->ofile, vhandle->soffset) >= 0)
    {
      setup->n_channels = vi->channels;
      setup->n_values = n * setup->n_channels;
      setup->bit_depth = 24;
    }
  else
    {
      ov_clear (&vhandle->ofile); /* closes file */
      return GSL_ERROR_NO_DATA;
    }

  vhandle->max_block_size = vorbis_info_blocksize (vi, 0);
  n = vorbis_info_blocksize (vi, 1);
  vhandle->max_block_size = MAX (vhandle->max_block_size, n);
  vhandle->pcm_pos = 0;
  vhandle->pcm_length = 0;
  
  return GSL_ERROR_NONE;
}

static GslLong
dh_vorbis_coarse_seek (GslDataHandle *dhandle,
		       GslLong        voffset)
{
  VorbisHandle *vhandle = (VorbisHandle*) dhandle;
  GslLong opos = vhandle->pcm_pos, pos = voffset / dhandle->setup.n_channels;
  
  if (voffset < 0)
    return vhandle->pcm_pos * dhandle->setup.n_channels;

  if (pos < vhandle->pcm_pos ||
      pos >= vhandle->pcm_pos + vhandle->pcm_length + SEEK_BY_READ_AHEAD (vhandle))
    {
      gint err = ov_pcm_seek_page (&vhandle->ofile, vhandle->soffset + pos);

      if (err)	/* eek */
	err = ov_pcm_seek_page (&vhandle->ofile, vhandle->soffset);
      else
	vhandle->pcm_pos = ov_pcm_tell (&vhandle->ofile) - vhandle->soffset;
      if (err || vhandle->pcm_pos < 0)  /* urg, we're completely screwed */
	vhandle->pcm_pos = 0;
      vhandle->pcm_length = 0;
    }
  if (0)
    g_printerr ("OggS-SEEK: at %lu want %lu got %lu (diff-requested %ld)\n",
		opos, pos, vhandle->pcm_pos, pos - opos);

  return vhandle->pcm_pos * dhandle->setup.n_channels;
}

static void
read_packet (VorbisHandle *vhandle)
{
  gfloat **pcm = NULL;
  gint stream_id, i;
  
  vhandle->pcm_pos = ov_pcm_tell (&vhandle->ofile) - vhandle->soffset;
  vhandle->pcm_length = ov_read_float (&vhandle->ofile, &pcm, G_MAXINT, &stream_id);
  if (vhandle->pcm_pos < 0 || vhandle->pcm_length < 0 || stream_id != vhandle->stream)
    {
      /* urg, this is bad! */
      dh_vorbis_coarse_seek (&vhandle->dhandle, 0);
    }
  else
    for (i = 0; i < vhandle->dhandle.setup.n_channels; i++)
      vhandle->pcm[i] = pcm[i];
}

static GslLong
dh_vorbis_read (GslDataHandle *dhandle,
		GslLong        voffset, /* in values */
		GslLong        n_values,
		gfloat        *values)
{
  VorbisHandle *vhandle = (VorbisHandle*) dhandle;
  GslLong pos = voffset / dhandle->setup.n_channels;

  if (pos < vhandle->pcm_pos ||
      pos >= vhandle->pcm_pos + vhandle->pcm_length + SEEK_BY_READ_AHEAD (vhandle))
    {
      GslLong tmp;

      /* suckage, needs to seek in file, this takes ages */
      tmp = dh_vorbis_coarse_seek (dhandle, voffset);
      g_assert (tmp <= voffset);
    }

  while (pos >= vhandle->pcm_pos + vhandle->pcm_length)
    read_packet (vhandle);

  n_values = MIN (n_values, vhandle->pcm_length * dhandle->setup.n_channels);

  /* interleave into output buffer */
  if (pos >= vhandle->pcm_pos && pos < vhandle->pcm_pos + vhandle->pcm_length)
    {
      guint offset = voffset - vhandle->pcm_pos * dhandle->setup.n_channels;
      guint align = offset % dhandle->setup.n_channels;
      guint n_samples = MIN (n_values, vhandle->pcm_length * dhandle->setup.n_channels - offset);
      gfloat *pcm[MAX_CHANNELS], *bound = values + n_samples;
      guint i;
      
      offset /= dhandle->setup.n_channels;
      for (i = 0; i < dhandle->setup.n_channels; i++)
	pcm[i] = vhandle->pcm[i] + offset + (i < align);

      for (i = align; values < bound; values++)
	{
	  gfloat f = *(pcm[i]++);

	  f = CLAMP (f, -1.0, 1.0);
	  *values = f;
	  if (++i >= dhandle->setup.n_channels)
	    i = 0;
	}
      return n_samples;
    }
  else /* something went wrong here, _badly_ */
    return 0;
}

static void
dh_vorbis_close (GslDataHandle *dhandle)
{
  VorbisHandle *vhandle = (VorbisHandle*) dhandle;

  ov_clear (&vhandle->ofile);
  vhandle->pcm_pos = 0;
  vhandle->pcm_length = 0;
}

static void
dh_vorbis_destroy (GslDataHandle *data_handle)
{
  VorbisHandle *vhandle = (VorbisHandle*) data_handle;

  gsl_data_handle_common_free (data_handle);
  sfi_delete_struct (VorbisHandle, vhandle);
}

static gboolean
dh_vorbis_ojob (GslDataHandle    *dhandle,
		GslDataHandleOJob ojob,
		gpointer          data)
{
  switch (ojob)
    {
      gboolean *needs_cache;
    case GSL_DATA_HANDLE_NEEDS_CACHE:
      needs_cache = data;
      *needs_cache = TRUE;
      return TRUE;      /* case implemented */
    default:
      return FALSE;     /* unimplemented cases */
    }
}

static GslDataHandleFuncs dh_vorbis_vtable = {
  dh_vorbis_open,
  dh_vorbis_read,
  dh_vorbis_close,
  NULL,
  dh_vorbis_destroy,
  dh_vorbis_ojob,
};

GslDataHandle*
gsl_data_handle_new_ogg_vorbis (const gchar *file_name,
				guint        lbitstream)
{
  VorbisHandle *vhandle;
  gboolean success;

  g_return_val_if_fail (file_name != NULL, NULL);

  vhandle = sfi_new_struct0 (VorbisHandle, 1);
  success = gsl_data_handle_common_init (&vhandle->dhandle, file_name);
  if (success)
    {
      GslErrorType error;

      vhandle->dhandle.vtable = &dh_vorbis_vtable;
      vhandle->n_streams = 0;
      vhandle->stream = lbitstream;

      /* we can only check matters upon opening
       */
      error = gsl_data_handle_open (&vhandle->dhandle);
      if (!error)
	{
	  gsl_data_handle_close (&vhandle->dhandle);
	  return &vhandle->dhandle;
	}
      else
	{
	  gsl_data_handle_unref (&vhandle->dhandle);
	  return NULL;
	}
    }
  else
    {
      sfi_delete_struct (VorbisHandle, vhandle);
      return NULL;
    }
}
