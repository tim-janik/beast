/* GSL - Generic Sound Layer
 * Copyright (C) 2001, 2003 Tim Janik
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
#include "gslvorbis-enc.h"
#include <bse/bseieee754.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>
#include <string.h>
#include <errno.h>

static SFI_MSG_TYPE_DEFINE (debug_vorbisenc, "vorbisenc", SFI_MSG_NONE, NULL);
#define DEBUG(...)      sfi_debug (debug_vorbisenc, __VA_ARGS__)

/* --- structures --- */
typedef struct {
  guint length;
  guint8 data[1];       /* flexible arary */
} EDataBlock;
struct _GslVorbisEncoder
{
  /* stream config */
  gfloat		vbr_quality;
  gint   		vbr_nominal;
  guint			n_channels;
  guint			sample_freq;
  guint			serial;		/* current serial number */
  /* state flags */
  guint			stream_setup : 1;
  guint			have_vblock : 1;        /* filled vorbis block pending */
  guint			pcm_done : 1;
  guint			eos : 1;        	/* end of stream reached */
  /* packed data */
  guint			dblock_offset;          /* read offset into topmost data block */
  SfiRing              *dblocks;                /* data block queue */
  /* ogg/vorbis codec state */
  ogg_stream_state	ostream;
  vorbis_block		vblock;
  vorbis_dsp_state	vdsp;
  vorbis_info		vinfo;
  /* comment part of stream config */
  vorbis_comment	vcomment;
};


/* --- prototypes --- */
static void     gsl_vorbis_encoder_reset (GslVorbisEncoder *self);


/* --- miscellaneous --- */
static void
gsl_vorbis_encoder_enqueue_page (GslVorbisEncoder *self,
                                 ogg_page         *opage)
{
  EDataBlock *dblock = g_malloc (sizeof (EDataBlock) - sizeof (dblock->data[0]) + opage->header_len);
  dblock->length = opage->header_len;
  memcpy (dblock->data, opage->header, dblock->length);
  self->dblocks = sfi_ring_append (self->dblocks, dblock);
  dblock = g_malloc (sizeof (EDataBlock) - sizeof (dblock->data[0]) + opage->body_len);
  dblock->length = opage->body_len;
  memcpy (dblock->data, opage->body, dblock->length);
  self->dblocks = sfi_ring_append (self->dblocks, dblock);
}


/* --- encoder API --- */
GslVorbisEncoder*
gsl_vorbis_encoder_new (void)
{
  GslVorbisEncoder *self;
  
  self = g_new0 (GslVorbisEncoder, 1);
  self->stream_setup = FALSE;
  
  vorbis_comment_init (&self->vcomment);
  
  /* defaults */
  gsl_vorbis_encoder_set_quality (self, 3.0);
  gsl_vorbis_encoder_set_n_channels (self, 2);
  gsl_vorbis_encoder_set_sample_freq (self, 44100);
  
  /* init portions */
  gsl_vorbis_encoder_reset (self);
  
  return self;
}

void
gsl_vorbis_encoder_destroy (GslVorbisEncoder *self)
{
  g_return_if_fail (self != NULL);
  
  gsl_vorbis_encoder_reset (self);
  vorbis_comment_clear (&self->vcomment);
  g_free (self);
}

void
gsl_vorbis_encoder_add_comment (GslVorbisEncoder *self,
                                const gchar      *comment)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->stream_setup == FALSE);
  g_return_if_fail (comment != NULL);
  
  vorbis_comment_add (&self->vcomment, (gpointer) comment);
}

void
gsl_vorbis_encoder_add_named_comment (GslVorbisEncoder *self,
                                      const gchar      *tag_name,
                                      const gchar      *comment)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->stream_setup == FALSE);
  g_return_if_fail (tag_name != NULL);
  g_return_if_fail (comment != NULL);
  
  vorbis_comment_add_tag (&self->vcomment, (gpointer) tag_name, (gpointer) comment);
}

static gchar*
convert_latin1_to_utf8 (const gchar *string)
{
  if (string)
    {
      const guchar *s = string;
      guint l = strlen (s);
      guchar *dest = g_new (guchar, l * 2 + 1), *d = dest;
      while (*s)
        if (*s >= 0xC0)
          *d++ = 0xC3, *d++ = *s++ - 0x40;
        else if (*s >= 0x80)
          *d++ = 0xC2, *d++ = *s++;
        else
          *d++ = *s++;
      *d++ = 0;
      return dest;
    }
  return NULL;
}

void
gsl_vorbis_encoder_add_lcomment (GslVorbisEncoder *self,
                                 const gchar      *comment)
{
  gchar *utf8_comment;
  
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->stream_setup == FALSE);
  g_return_if_fail (comment != NULL);
  
  utf8_comment = convert_latin1_to_utf8 (comment);
  vorbis_comment_add (&self->vcomment, utf8_comment);
  g_free (utf8_comment);
}

void
gsl_vorbis_encoder_add_named_lcomment (GslVorbisEncoder *self,
				       const gchar      *tag_name,
				       const gchar      *comment)
{
  gchar *utf8_comment;
  
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->stream_setup == FALSE);
  g_return_if_fail (tag_name != NULL);
  g_return_if_fail (comment != NULL);
  
  utf8_comment = convert_latin1_to_utf8 (comment);
  vorbis_comment_add_tag (&self->vcomment, (gpointer) tag_name, utf8_comment);
  g_free (utf8_comment);
}

void
gsl_vorbis_encoder_set_quality (GslVorbisEncoder *self,
				gfloat            quality)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->stream_setup == FALSE);

  self->vbr_quality = CLAMP (quality, -1.0, 10.0) * 0.1;
  self->vbr_nominal = -1;
}

void
gsl_vorbis_encoder_set_bitrate (GslVorbisEncoder *self,
				guint             nominal)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->stream_setup == FALSE);
  g_return_if_fail (nominal >= 32 && nominal <= 1048576);

  self->vbr_quality = -1;
  self->vbr_nominal = nominal;
}

void
gsl_vorbis_encoder_set_n_channels (GslVorbisEncoder *self,
				   guint             n_channels)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->stream_setup == FALSE);
  g_return_if_fail (n_channels >= 1 && n_channels <= 2);
  
  self->n_channels = n_channels;
}

void
gsl_vorbis_encoder_set_sample_freq (GslVorbisEncoder *self,
				    guint             sample_freq)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->stream_setup == FALSE);
  g_return_if_fail (sample_freq >= 8000 && sample_freq <= 96000);
  
  self->sample_freq = sample_freq;
}

static void
gsl_vorbis_encoder_reset (GslVorbisEncoder *self)
{
  g_return_if_fail (self != NULL);

  /* cleanup codec state */
  if (self->stream_setup)
    {
      ogg_stream_clear (&self->ostream);
      vorbis_block_clear (&self->vblock);
      vorbis_dsp_clear (&self->vdsp);
      vorbis_info_clear (&self->vinfo);
      self->stream_setup = FALSE;
    }
  /* cleanup encoded data blocks */
  while (self->dblocks)
    g_free (sfi_ring_pop_head (&self->dblocks));
  self->dblock_offset = 0;
  /* reset comments */
  vorbis_comment_clear (&self->vcomment);
  vorbis_comment_init (&self->vcomment);
  /* cleanup state flags */
  self->pcm_done = FALSE;
  self->eos = FALSE;
  self->have_vblock = FALSE;
}

BseErrorType
gsl_vorbis_encoder_setup_stream (GslVorbisEncoder *self,
				 guint		   serial)
{
  ogg_packet opacket1, opacket2, opacket3;
  ogg_page opage;
  gint result;

  g_return_val_if_fail (self != NULL, BSE_ERROR_INTERNAL);
  g_return_val_if_fail (self->stream_setup == FALSE, BSE_ERROR_INTERNAL);
  
  self->serial = serial;
  vorbis_info_init (&self->vinfo);
  DEBUG ("init: channels=%u mixfreq=%u quality=%f bitrate=%d\n",
         self->n_channels, self->sample_freq, self->vbr_quality, self->vbr_nominal);
  if (self->vbr_nominal > 0)    /* VBR setup by nominal bitrate */
    result = vorbis_encode_setup_managed (&self->vinfo,
                                          self->n_channels,
                                          self->sample_freq,
                                          -1,
                                          self->vbr_nominal,
                                          -1) ||
             vorbis_encode_ctl (&self->vinfo, OV_ECTL_RATEMANAGE_AVG, NULL) ||
             vorbis_encode_setup_init (&self->vinfo);
  else                          /* VBR setup by quality */
    result = vorbis_encode_init_vbr (&self->vinfo,
                                     self->n_channels,
                                     self->sample_freq,
                                     self->vbr_quality);
  if (result != 0)
    {
      vorbis_info_clear (&self->vinfo);
      return BSE_ERROR_CODEC_FAILURE;
    }
  
  self->stream_setup = TRUE;
  vorbis_analysis_init (&self->vdsp, &self->vinfo);
  vorbis_block_init (&self->vdsp, &self->vblock);
  ogg_stream_init (&self->ostream, self->serial);

  /* flush pages with header packets (initial, comments, codebooks) */
  vorbis_analysis_headerout (&self->vdsp, &self->vcomment, &opacket1, &opacket2, &opacket3);
  ogg_stream_packetin (&self->ostream, &opacket1);
  ogg_stream_packetin (&self->ostream, &opacket2);
  ogg_stream_packetin (&self->ostream, &opacket3);
  while (ogg_stream_flush (&self->ostream, &opage))
    gsl_vorbis_encoder_enqueue_page (self, &opage);

  return BSE_ERROR_NONE;
}

static void
vorbis_encoder_write_pcm_1k (GslVorbisEncoder *self,
                             guint             n_values,
                             gfloat           *values)
{
  gfloat **dest;

  /* the vorbis encoding engine has a bug that produces junk at
   * certain block sizes beyond 1024
   */
  g_assert (n_values <= 1024);

  /* people passing in non-channel-aligned data get what they deserve */
  n_values /= self->n_channels;

  /* allocate required space */
  dest = vorbis_analysis_buffer (&self->vdsp, n_values);
  /* uninterleave incoming data */
  if (self->n_channels == 1)
    memcpy (dest[0], values, n_values * sizeof (values[0]));
  else /* self->n_channels == 2 */
    {
      gfloat *dest0 = dest[0], *dest1 = dest[1], *bound = dest0 + n_values;
      do
	{
	  *dest0++ = *values++;
	  *dest1++ = *values++;
	}
      while (dest0 < bound);
    }
  /* let the analysis engine know how much data arrived */
  vorbis_analysis_wrote (&self->vdsp, n_values);
}

void
gsl_vorbis_encoder_write_pcm (GslVorbisEncoder *self,
			      guint             n_values,
			      gfloat           *values)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->stream_setup == TRUE);
  g_return_if_fail (self->pcm_done == FALSE);
  g_return_if_fail (self->n_channels * (n_values / self->n_channels) == n_values); /* check alignment */
  if (n_values)
    g_return_if_fail (values != NULL);
  
  /* compress away remaining data so we only buffer encoded data */
  while (gsl_vorbis_encoder_needs_processing (self))
    gsl_vorbis_encoder_process (self);

  /* feed analysis engine with unencoded data */
  while (n_values)
    {
      guint l = MIN (n_values, 1024);
      vorbis_encoder_write_pcm_1k (self, l, values);
      values += l;
      n_values -= l;
    }
}

void
gsl_vorbis_encoder_pcm_done (GslVorbisEncoder *self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->stream_setup == TRUE);
  
  if (!self->pcm_done)
    {
      self->pcm_done = TRUE;
      vorbis_analysis_wrote (&self->vdsp, 0);	/* termination mark */
    }
}

static gboolean
gsl_vorbis_encoder_blockout (GslVorbisEncoder *self)
{
  if (!self->have_vblock)
    self->have_vblock = vorbis_analysis_blockout (&self->vdsp, &self->vblock) > 0;
  return self->have_vblock;
}

gboolean
gsl_vorbis_encoder_needs_processing (GslVorbisEncoder *self)
{
  g_return_val_if_fail (self != NULL, FALSE);

  return self->stream_setup && !self->eos && gsl_vorbis_encoder_blockout (self);
}

void
gsl_vorbis_encoder_process (GslVorbisEncoder *self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->stream_setup == TRUE);

  /* analyse data blockwise */
  if (gsl_vorbis_encoder_blockout (self))
    {
      ogg_packet opacket;
      /* perform main analysis */
      vorbis_analysis (&self->vblock, NULL);
      self->have_vblock = FALSE;
      /* confine to bitrate specs */
      vorbis_bitrate_addblock (&self->vblock);
      /* add packets to bitstream */
      while (vorbis_bitrate_flushpacket (&self->vdsp, &opacket))
        {
          ogg_page opage;
          ogg_stream_packetin (&self->ostream, &opacket);
          while (ogg_stream_pageout (&self->ostream, &opage))
            {
              /* queue bitstream pages as outgoing data */
              gsl_vorbis_encoder_enqueue_page (self, &opage);
              /* catch end of stream */
              if (ogg_page_eos (&opage))
                {
                  self->eos = TRUE;
                  return;       /* break all loops */
                }
            }
        }
    }
}

guint
gsl_vorbis_encoder_read_ogg (GslVorbisEncoder *self,
                             guint             n_bytes,
                             guint8           *bytes)
{
  guint8 *ubytes = bytes;

  g_return_val_if_fail (self != NULL, 0);
  g_return_val_if_fail (self->stream_setup == TRUE, 0);

  if (!self->dblocks)
    gsl_vorbis_encoder_process (self);
  while (n_bytes && self->dblocks)
    {
      EDataBlock *dblock = self->dblocks->data;
      guint l = MIN (n_bytes, dblock->length - self->dblock_offset);
      memcpy (bytes, dblock->data + self->dblock_offset, l);
      n_bytes -= l;
      bytes += l;
      self->dblock_offset += l;
      if (self->dblock_offset >= dblock->length)
        {
          g_free (sfi_ring_pop_head (&self->dblocks));
          self->dblock_offset = 0;
        }
    }
  return bytes - ubytes;
}

gboolean
gsl_vorbis_encoder_ogg_eos (GslVorbisEncoder *self)
{
  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (self->stream_setup == TRUE, FALSE);
  
  return self->eos && !self->dblocks;
}
