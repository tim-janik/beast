/* GSL - Generic Sound Layer
 * Copyright (C) 2001-2005 Tim Janik
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

  guint    bitstream;
  guint    bitstream_serialno; /* set after open() */
  gfloat   osc_freq;
  guint    n_bitstreams;
  guint    rfile_byte_offset : 31;
  guint    rfile_add_zoffset : 1;
  guint    rfile_byte_length;

  /* live data */
  gint64  soffset;	/* our PCM start offset */
  guint   max_block_size;

  /* pcm read out cache */
  GslLong pcm_pos, pcm_length;
  gfloat *pcm[MAX_CHANNELS];

  OggVorbis_File ofile;
} VorbisHandle;


/* --- functions --- */
static BseErrorType
ov_errno_to_error (gint         ov_errno,
		   BseErrorType fallback)
{
  switch (ov_errno)
    {
    case OV_EOF:	return BSE_ERROR_FILE_EOF;
    case OV_EBADLINK:
    case OV_EBADPACKET:
    case OV_HOLE:	return BSE_ERROR_DATA_CORRUPT;
    case OV_EREAD:	return BSE_ERROR_FILE_READ_FAILED;
    case OV_ENOSEEK:	return BSE_ERROR_FILE_SEEK_FAILED;
    case OV_EFAULT:
    case OV_EIMPL:	return BSE_ERROR_CODEC_FAILURE;
    case OV_EINVAL:	return BSE_ERROR_INTERNAL;
    case OV_ENOTAUDIO:
    case OV_EVERSION:
    case OV_EBADHEADER:
    case OV_ENOTVORBIS:	return BSE_ERROR_FORMAT_INVALID;
    case OV_FALSE:
    default:		return fallback;
    }
}

typedef struct {
  GslRFile *rfile;
  GslLong   byte_offset;
  GslLong   byte_length;
} VFile;

static size_t
vfile_read (void  *ptr,
	    size_t size,
	    size_t nmemb,
	    void  *datasource)
{
  VFile *vfile = datasource;
  return gsl_rfile_read (vfile->rfile, size * nmemb, ptr);
}

static int
vfile_seek (void       *datasource,
	    ogg_int64_t offset,
	    int         whence)
{
  VFile *vfile = datasource;
  ogg_int64_t l;
  switch (whence)
    {
    default:
    case SEEK_SET:
      l = vfile->byte_offset + offset;
      l = CLAMP (l, vfile->byte_offset, vfile->byte_offset + vfile->byte_length);
      l = gsl_rfile_seek_set (vfile->rfile, l);
      break;
    case SEEK_CUR:
      l = gsl_rfile_position (vfile->rfile) + offset;
      l = CLAMP (l, vfile->byte_offset, vfile->byte_offset + vfile->byte_length);
      l = gsl_rfile_seek_set (vfile->rfile, l);
      break;
    case SEEK_END:
      l = vfile->byte_offset + vfile->byte_length + offset;
      l = CLAMP (l, vfile->byte_offset, vfile->byte_offset + vfile->byte_length);
      l = gsl_rfile_seek_set (vfile->rfile, l);
      break;
    }
  return l < 0 ? -1 : l - vfile->byte_offset;
}

static int
vfile_close (void *datasource)
{
  VFile *vfile = datasource;
  gsl_rfile_close (vfile->rfile);
  g_free (vfile);
  return 0;
}

static long
vfile_tell (void *datasource)
{
  VFile *vfile = datasource;
  return gsl_rfile_position (vfile->rfile) - vfile->byte_offset;
}

static ov_callbacks vfile_ov_callbacks = {
  vfile_read,
  vfile_seek,
  vfile_close,
  vfile_tell,
};

static BseErrorType
dh_vorbis_open (GslDataHandle      *dhandle,
		GslDataHandleSetup *setup)
{
  VorbisHandle *vhandle = (VorbisHandle*) dhandle;
  VFile *vfile;
  vorbis_info *vi;
  GslLong n, i;
  gint err;

  vfile = g_new0 (VFile, 1);
  vfile->rfile = gsl_rfile_open (vhandle->dhandle.name);
  if (!vfile->rfile)
    {
      g_free (vfile);
      return gsl_error_from_errno (errno, BSE_ERROR_FILE_OPEN_FAILED);
    }
  vfile->byte_length = gsl_rfile_length (vfile->rfile);
  if (vhandle->rfile_add_zoffset)
    {
      vfile->byte_offset = gsl_hfile_zoffset (vfile->rfile->hfile) + 1;
      vfile->byte_offset += vhandle->rfile_byte_offset;
      vfile->byte_offset = MIN (vfile->byte_offset, vfile->byte_length);
      vfile->byte_length -= vfile->byte_offset;
    }
  else
    {
      vfile->byte_offset = MIN (vhandle->rfile_byte_offset, vfile->byte_length);
      vfile->byte_length -= vfile->byte_offset;
    }
  if (vhandle->rfile_byte_length > 0)
    vfile->byte_length = MIN (vfile->byte_length, vhandle->rfile_byte_length);
  err = ov_open_callbacks (vfile, &vhandle->ofile, NULL, 0, vfile_ov_callbacks);
  if (err < 0)
    {
      if (0)
        g_printerr ("failed to open ogg at offset %d (real offset=%ld) (add-zoffset=%d): %s\n",
                    vhandle->rfile_byte_offset,
                    vfile->byte_offset,
                    vhandle->rfile_add_zoffset,
                    bse_error_blurb (ov_errno_to_error (err, BSE_ERROR_FILE_OPEN_FAILED)));
      vfile_close (vfile);
      return ov_errno_to_error (err, BSE_ERROR_FILE_OPEN_FAILED);
    }

  n = ov_streams (&vhandle->ofile);
  if (n > vhandle->bitstream)
    {
      vhandle->bitstream_serialno = ov_serialnumber (&vhandle->ofile, vhandle->bitstream);
      vhandle->n_bitstreams = n;
      if (0)
        g_printerr ("Ogg/Vorbis: opening ogg: bitstream=%d/%d serialno=%d\n", vhandle->bitstream, vhandle->n_bitstreams, vhandle->bitstream_serialno);
    }
  else
    {
      ov_clear (&vhandle->ofile); /* closes file */
      return BSE_ERROR_NO_DATA;	/* requested bitstream not available */
    }

  vhandle->soffset = 0;
  for (i = 0; i < vhandle->bitstream; i++)
    vhandle->soffset += ov_pcm_total (&vhandle->ofile, i);

  n = ov_pcm_total (&vhandle->ofile, vhandle->bitstream);
  vi = ov_info (&vhandle->ofile, vhandle->bitstream);
  if (n > 0 && vi && vi->channels && ov_pcm_seek (&vhandle->ofile, vhandle->soffset) >= 0)
    {
      setup->n_channels = vi->channels;
      setup->n_values = n * setup->n_channels;
    }
  else
    {
      ov_clear (&vhandle->ofile); /* closes file */
      return BSE_ERROR_NO_DATA;
    }

  vhandle->max_block_size = vorbis_info_blocksize (vi, 0);
  n = vorbis_info_blocksize (vi, 1);
  vhandle->max_block_size = MAX (vhandle->max_block_size, n);
  vhandle->pcm_pos = 0;
  vhandle->pcm_length = 0;
  
  setup->bit_depth = 24;
  setup->mix_freq = vi->rate;
  setup->needs_cache = TRUE;
  setup->xinfos = bse_xinfos_add_float (setup->xinfos, "osc-freq", vhandle->osc_freq);
  return BSE_ERROR_NONE;
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
  if (vhandle->pcm_pos < 0 || vhandle->pcm_length < 0 || stream_id != vhandle->bitstream)
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

  g_strfreev (dhandle->setup.xinfos);
  dhandle->setup.xinfos = NULL;
  ov_clear (&vhandle->ofile);
  vhandle->pcm_pos = 0;
  vhandle->pcm_length = 0;
}

static void
dh_vorbis_destroy (GslDataHandle *dhandle)
{
  VorbisHandle *vhandle = (VorbisHandle*) dhandle;

  gsl_data_handle_common_free (dhandle);
  sfi_delete_struct (VorbisHandle, vhandle);
}

static GslDataHandleFuncs dh_vorbis_vtable = {
  dh_vorbis_open,
  dh_vorbis_read,
  dh_vorbis_close,
  NULL,
  dh_vorbis_destroy,
};

static GslDataHandle*
gsl_data_handle_new_ogg_vorbis_any (const gchar *file_name,
                                    guint        lbitstream,
                                    gfloat       osc_freq,
                                    gboolean     add_zoffset,
                                    guint        byte_offset,
                                    guint        byte_size,
                                    guint       *n_channelsp,
                                    gfloat      *mix_freqp)
{
  if (n_channelsp)
    *n_channelsp = 0;
  if (mix_freqp)
    *mix_freqp = 0;
  VorbisHandle *vhandle = sfi_new_struct0 (VorbisHandle, 1);
  gboolean success = gsl_data_handle_common_init (&vhandle->dhandle, file_name);
  if (success)
    {
      BseErrorType error;

      vhandle->dhandle.vtable = &dh_vorbis_vtable;
      vhandle->n_bitstreams = 0;
      vhandle->bitstream = lbitstream;
      vhandle->osc_freq = osc_freq;
      vhandle->rfile_byte_offset = byte_offset;
      vhandle->rfile_add_zoffset = add_zoffset != FALSE;
      vhandle->rfile_byte_length = byte_size;

      /* we can only check matters upon opening and need
       * to initialize things like the bitstream_serialno.
       */
      error = gsl_data_handle_open (&vhandle->dhandle);
      if (!error)
	{
          if (n_channelsp)
            *n_channelsp = vhandle->dhandle.setup.n_channels;
          if (mix_freqp)
            *mix_freqp = vhandle->dhandle.setup.mix_freq;
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

GslDataHandle*
gsl_data_handle_new_ogg_vorbis_muxed (const gchar *file_name,
                                      guint        lbitstream,
                                      gfloat       osc_freq)
{
  g_return_val_if_fail (file_name != NULL, NULL);

  return gsl_data_handle_new_ogg_vorbis_any (file_name, lbitstream, osc_freq, FALSE, 0, 0, NULL, NULL);
}

GslDataHandle*
gsl_data_handle_new_ogg_vorbis_zoffset (const gchar *file_name,
                                        gfloat       osc_freq,
                                        GslLong      byte_offset,
                                        GslLong      byte_size,
                                        guint       *n_channelsp,
                                        gfloat      *mix_freq_p)
{
  g_return_val_if_fail (file_name != NULL, NULL);
  g_return_val_if_fail (byte_offset >= 0, NULL);
  g_return_val_if_fail (byte_size > 0, NULL);

  return gsl_data_handle_new_ogg_vorbis_any (file_name, 0, osc_freq, TRUE, byte_offset, byte_size, n_channelsp, mix_freq_p);
}

/* --- writing vorbis files --- */
#include "gslvorbis-cutter.h"

struct GslVorbis1Handle
{
  GslDataHandle   *dhandle;
  guint            bitstream_serialno;
  guint            rfile_byte_offset : 31;
  guint            rfile_add_zoffset : 1;
  guint            rfile_byte_length;
  GslRFile        *rfile;
  guint            byte_offset;
  guint            byte_length;
  GslVorbisCutter *vcutter;
};

GslVorbis1Handle*
gsl_vorbis1_handle_new (GslDataHandle *ogg_vorbis_handle)
{
  GslVorbis1Handle *v1h = NULL;
  if (ogg_vorbis_handle->vtable == &dh_vorbis_vtable &&
      gsl_data_handle_open (ogg_vorbis_handle) == BSE_ERROR_NONE)
    {
      v1h = g_new0 (GslVorbis1Handle, 1);
      v1h->dhandle = ogg_vorbis_handle;
      VorbisHandle *vhandle = (VorbisHandle*) ogg_vorbis_handle;
      v1h->bitstream_serialno = vhandle->bitstream_serialno;
      v1h->rfile_byte_offset = vhandle->rfile_byte_offset;
      v1h->rfile_add_zoffset = vhandle->rfile_add_zoffset;
      v1h->rfile_byte_length = vhandle->rfile_byte_length;
    }
  return v1h;
}

gint
gsl_vorbis1_handle_read (GslVorbis1Handle *v1h, /* returns -errno || length */
                         guint             blength,
                         guint8           *buffer)
{
  if (!v1h->rfile)
    {
      v1h->rfile = gsl_rfile_open (v1h->dhandle->name);
      if (!v1h->rfile)
        return gsl_error_from_errno (errno, BSE_ERROR_FILE_OPEN_FAILED);
      v1h->byte_length = gsl_rfile_length (v1h->rfile);
      if (v1h->rfile_add_zoffset)
        {
          v1h->byte_offset = gsl_hfile_zoffset (v1h->rfile->hfile) + 1;
          v1h->byte_offset += v1h->rfile_byte_offset;
          v1h->byte_offset = MIN (v1h->byte_offset, v1h->byte_length);
          v1h->byte_length -= v1h->byte_offset;
        }
      else
        {
          v1h->byte_offset = MIN (v1h->rfile_byte_offset, v1h->byte_length);
          v1h->byte_length -= v1h->byte_offset;
        }
      if (v1h->rfile_byte_length > 0)
        v1h->byte_length = MIN (v1h->byte_length, v1h->rfile_byte_length);
      v1h->vcutter = gsl_vorbis_cutter_new();
      gsl_vorbis_cutter_filter_serialno (v1h->vcutter, v1h->bitstream_serialno);
    }

  while (1) /* repeats until data is available */
    {
      guint j, n = gsl_vorbis_cutter_read_ogg (v1h->vcutter, blength, buffer);
      if (n)
        return n;               /* got data */
      if (gsl_vorbis_cutter_ogg_eos (v1h->vcutter))
        return 0;               /* done at end-of-stream */
      do                        /* need to feed data */
        j = gsl_rfile_read (v1h->rfile, blength, buffer);
      while (j < 0 && errno == EINTR);
      if (j <= 0)               /* bail on errors */
        return errno ? -errno : -ENODATA;
      gsl_vorbis_cutter_write_ogg (v1h->vcutter, j, buffer);
    }
}

void
gsl_vorbis1_handle_destroy (GslVorbis1Handle *v1h)
{
  if (v1h->vcutter)
    gsl_vorbis_cutter_destroy (v1h->vcutter);
  if (v1h->rfile)
    gsl_rfile_close (v1h->rfile);
  gsl_data_handle_close (v1h->dhandle);
  v1h->dhandle = NULL;
  g_free (v1h);
}

static gint /* -errno || length */
vorbis1_handle_reader (gpointer data,
                       SfiNum   pos,
                       void    *buffer,
                       guint    blength)
{
  GslVorbis1Handle *vhandle = (GslVorbis1Handle*) data;
  return gsl_vorbis1_handle_read (vhandle, blength, (guint8*) buffer);
}

static void
vorbis1_handle_destroy (gpointer data)
{
  GslVorbis1Handle *vhandle = (GslVorbis1Handle*) data;
  gsl_vorbis1_handle_destroy (vhandle);
}

void
gsl_vorbis1_handle_put_wstore (GslVorbis1Handle *vorbis1,
                               SfiWStore        *wstore)
{
  sfi_wstore_put_binary (wstore, vorbis1_handle_reader, vorbis1, vorbis1_handle_destroy);
}
