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

  gboolean setup_validation;	/* for internal setup phase */
  guint    n_streams;
  guint    stream;
  guint    n_channels;

  /* live data */
  gint64  soffset;	/* our PCM start offset */
  guint   max_block_size;

  GslLong pcm_pos, pcm_length;
  gfloat *pcm[MAX_CHANNELS];

  OggVorbis_File ofile;
} VorbisHandle;


/* --- functions --- */
static gint
dh_vorbis_open (GslDataHandle *data_handle)
{
  VorbisHandle *vhandle = (VorbisHandle*) data_handle;
  FILE *file = fopen (vhandle->dhandle.name, "r");
  vorbis_info *vi;
  GslLong n, i;
  gint err;
  
  if (!file)
    return errno ? errno : EIO;

  err = ov_open (file, &vhandle->ofile, NULL, 0);
  if (err < 0)
    {
      fclose (file);
      return EIO;
    }

  n = ov_streams (&vhandle->ofile);
  if (vhandle->setup_validation && n > 0)
    {
      vhandle->n_streams = n;
      vhandle->stream = n - 1;
    }
  else if (n != vhandle->n_streams)
    {
      ov_clear (&vhandle->ofile); /* closes file */
      return EIO;
    }

  vhandle->soffset = 0;
  for (i = 0; i < vhandle->stream; i++)
    vhandle->soffset += ov_pcm_total (&vhandle->ofile, i);

  n = ov_pcm_total (&vhandle->ofile, vhandle->stream);
  vi = ov_info (&vhandle->ofile, vhandle->stream);
  if (vhandle->setup_validation && n > 0 && vi)
    {
      vhandle->n_channels = vi->channels;
      vhandle->dhandle.n_values = n * vhandle->n_channels;
    }

  if (!vi || vi->channels != vhandle->n_channels ||
      n * vi->channels != vhandle->dhandle.n_values ||
      ov_pcm_seek (&vhandle->ofile, vhandle->soffset) < 0)
    {
      ov_clear (&vhandle->ofile); /* closes file */
      return EIO;
    }

  vhandle->max_block_size = vorbis_info_blocksize (vi, 0);
  n = vorbis_info_blocksize (vi, 1);
  vhandle->max_block_size = MAX (vhandle->max_block_size, n);
  vhandle->pcm_pos = 0;
  vhandle->pcm_length = 0;
  
  return 0;
}

static GslLong
dh_vorbis_coarse_seek (GslDataHandle *data_handle,
		       GslLong        voffset)
{
  VorbisHandle *vhandle = (VorbisHandle*) data_handle;
  GslLong opos = vhandle->pcm_pos, pos = voffset / vhandle->n_channels;
  
  if (voffset < 0)
    return vhandle->pcm_pos * vhandle->n_channels;

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
  g_printerr ("OggS-SEEK: at %lu want %lu got %lu (diff-requested %ld)\n",
	      opos, pos, vhandle->pcm_pos, pos - opos);

  return vhandle->pcm_pos * vhandle->n_channels;
}

static void
read_packet (VorbisHandle *vhandle)
{
  gfloat **pcm = NULL;
  gint stream_id, i;
  
  vhandle->pcm_pos = ov_pcm_tell (&vhandle->ofile) - vhandle->soffset;
  vhandle->pcm_length = ov_read_float (&vhandle->ofile, &pcm, &stream_id);
  if (vhandle->pcm_pos < 0 || vhandle->pcm_length < 0 || stream_id != vhandle->stream)
    {
      /* urg, this is bad! */
      dh_vorbis_coarse_seek (&vhandle->dhandle, 0);
    }
  else
    for (i = 0; i < vhandle->n_channels; i++)
      vhandle->pcm[i] = pcm[i];
}

static GslLong
dh_vorbis_read (GslDataHandle *data_handle,
		GslLong        voffset, /* in values */
		GslLong        n_values,
		gfloat        *values)
{
  VorbisHandle *vhandle = (VorbisHandle*) data_handle;
  GslLong pos = voffset / vhandle->n_channels;

  if (pos < vhandle->pcm_pos ||
      pos >= vhandle->pcm_pos + vhandle->pcm_length + SEEK_BY_READ_AHEAD (vhandle))
    {
      GslLong tmp;

      /* suckage, needs to seek in file, this takes ages */
      tmp = dh_vorbis_coarse_seek (data_handle, voffset);
      g_assert (tmp <= voffset);
    }

  while (pos >= vhandle->pcm_pos + vhandle->pcm_length)
    read_packet (vhandle);

  n_values = MIN (n_values, vhandle->pcm_length * vhandle->n_channels);

  /* interleave into output buffer */
  if (pos >= vhandle->pcm_pos && pos < vhandle->pcm_pos + vhandle->pcm_length)
    {
      guint offset = voffset - vhandle->pcm_pos * vhandle->n_channels;
      guint align = offset % vhandle->n_channels;
      guint n_samples = MIN (n_values, vhandle->pcm_length * vhandle->n_channels - offset);
      gfloat *pcm[MAX_CHANNELS], *bound = values + n_samples;
      guint i;
      
      offset /= vhandle->n_channels;
      for (i = 0; i < vhandle->n_channels; i++)
	pcm[i] = vhandle->pcm[i] + offset + (i < align);

      for (i = align; values < bound; values++)
	{
	  gfloat f = *(pcm[i]++);

	  f = CLAMP (f, -1.0, 1.0);
	  *values = f;
	  if (++i >= vhandle->n_channels)
	    i = 0;
	}
      return n_samples;
    }
  else /* something went wrong here, _badly_ */
    return 0;
}

static void
dh_vorbis_close (GslDataHandle *data_handle)
{
  VorbisHandle *vhandle = (VorbisHandle*) data_handle;

  ov_clear (&vhandle->ofile);
  vhandle->pcm_pos = 0;
  vhandle->pcm_length = 0;
}

static void
dh_vorbis_destroy (GslDataHandle *data_handle)
{
  VorbisHandle *vhandle = (VorbisHandle*) data_handle;

  gsl_data_handle_common_free (data_handle);
  gsl_delete_struct (VorbisHandle, vhandle);
}

static GslDataHandleFuncs dh_vorbis_vtable = {
  dh_vorbis_open,
  dh_vorbis_read,
  dh_vorbis_close,
  dh_vorbis_destroy,
  dh_vorbis_coarse_seek,
};

GslDataHandle*
gsl_data_handle_new_ogg_vorbis (const gchar *file_name,
				guint        lbitstream)
{
  VorbisHandle *vhandle;
  gboolean success;

  g_return_val_if_fail (file_name != NULL, NULL);

  vhandle = gsl_new_struct0 (VorbisHandle, 1);
  success = gsl_data_handle_common_init (&vhandle->dhandle, NULL, 16);
  if (success)
    {
      gint err;

      vhandle->dhandle.name = g_strdup (file_name);
      vhandle->dhandle.vtable = &dh_vorbis_vtable;
      vhandle->dhandle.n_values = 0;
      vhandle->n_streams = 0;
      vhandle->n_channels = 0;

      /* we can only do the remaining setup and check matters if we
       * actually open the handle
       */
      vhandle->setup_validation = TRUE;
      err = gsl_data_handle_open (&vhandle->dhandle);
      vhandle->setup_validation = FALSE;
      if (!err && vhandle->soffset + vhandle->dhandle.n_values &&
	  vhandle->n_streams && lbitstream < vhandle->n_streams)
	{
	  vorbis_info *vi = ov_info (&vhandle->ofile, lbitstream);

	  if (vi && vi->channels > 0 && vi->channels <= MAX_CHANNELS)
	    {
	      GslLong n = ov_pcm_total (&vhandle->ofile, lbitstream);
	      
	      if (n > 0)
		{
		  vhandle->stream = lbitstream;
		  vhandle->n_channels = vi->channels;
		  vhandle->dhandle.n_values = n * vhandle->n_channels;
		}
	      gsl_data_handle_close (&vhandle->dhandle);

	      return &vhandle->dhandle;
	    }
	}
      if (!err)
	gsl_data_handle_close (&vhandle->dhandle);
      gsl_data_handle_unref (&vhandle->dhandle);
      return NULL;
    }
  else
    {
      gsl_delete_struct (VorbisHandle, vhandle);
      return NULL;
    }
}
