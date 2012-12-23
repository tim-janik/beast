/* GSL - Generic Sound Layer
 * Copyright (C) 2001, 2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "gslvorbis-cutter.hh"
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <string.h>
#include <errno.h>

static SFI_MSG_TYPE_DEFINE (debug_vorbis, "vorbis", SFI_MSG_DEBUG, NULL);
#define DEBUG(...)      sfi_debug (debug_vorbis, __VA_ARGS__)
#define DIAG(...)       sfi_diag (__VA_ARGS__)


/* --- structures --- */
typedef struct {
  guint length;
  guint8 data[1];       /* flexible arary */
} CDataBlock;
struct _GslVorbisCutter
{
  /* user config */
  SfiNum                cutpoint;
  GslVorbisCutterMode   cutmode;
  guint                 filtered_serialno;
  guint                 forced_serialno;
  guint                 filter_serialno : 1;
  guint                 force_serialno : 1;
  /* state flags */
  guint                 vorbis_initialized : 1;
  guint                 eos : 1;
  /* packed data */
  guint                 dblock_offset;          /* read offset into topmost data block */
  SfiRing              *dblocks;                /* data block queue (partial output pages) */
  /* ogg stream states */
  ogg_int64_t           initial_granule;        /* granulepos of first incoming packet */
  guint                 n_packets;              /* amount of incoming/outgoing packets */
  gint                  last_window;            /* window (block) size of last packet */
  ogg_int64_t           tracking_granule;       /* granulepos of last audio packet */
  ogg_sync_state        isync;                  /* syncing input bitstream, bits => page */
  ogg_stream_state      istream;                /* input stream, page => packet */
  ogg_stream_state      ostream;                /* output stream, packet => page */
  vorbis_info           vinfo;
  vorbis_comment        vcomment;
  vorbis_dsp_state      vdsp;
};


/* --- miscellaneous --- */
static const gchar*
ov_error_blurb (gint ov_error)
{
  switch (ov_error)
    {
    case OV_EOF:        return "Premature end of file";
    case OV_EBADLINK:   return "Failed to relocate stream pointer";
    case OV_EBADPACKET: return "Malformed packet";
    case OV_HOLE:       return "Discontinuous data stream";
    case OV_EREAD:      return "Read failed";
    case OV_ENOSEEK:    return "Unseekable stream";
    case OV_EFAULT:     return "CODEC failure";
    case OV_EIMPL:      return "Unimplemented feature";
    case OV_EINVAL:     return "Invalid value";
    case OV_ENOTAUDIO:  return "Not AUDIO";
    case OV_EVERSION:   return "Version mismatch";
    case OV_EBADHEADER: return "Malformed header";
    case OV_ENOTVORBIS: return "Not Vorbis";
    case OV_FALSE:
    default:            return "Unknown failure";
    }
}

static void
enqueue_page (SfiRing **dblocks,
              ogg_page *opage)
{
  CDataBlock *dblock = (CDataBlock*) g_malloc (sizeof (CDataBlock) - sizeof (dblock->data[0]) + opage->header_len);
  dblock->length = opage->header_len;
  memcpy (dblock->data, opage->header, dblock->length);
  *dblocks = sfi_ring_append (*dblocks, dblock);
  dblock = (CDataBlock*) g_malloc (sizeof (CDataBlock) - sizeof (dblock->data[0]) + opage->body_len);
  dblock->length = opage->body_len;
  memcpy (dblock->data, opage->body, dblock->length);
  *dblocks = sfi_ring_append (*dblocks, dblock);
}


/* --- cutter API --- */
GslVorbisCutter*
gsl_vorbis_cutter_new (void)
{
  GslVorbisCutter *self = g_new0 (GslVorbisCutter, 1);
  self->cutpoint = 0;
  self->cutmode = GSL_VORBIS_CUTTER_NONE;
  self->eos = FALSE;
  self->dblock_offset = 0;
  self->dblocks = NULL;
  self->n_packets = 0;
  self->last_window = 0;
  self->initial_granule = 0;
  self->tracking_granule = 0;
  ogg_sync_init (&self->isync);
  ogg_stream_init (&self->istream, 0);
  ogg_stream_init (&self->ostream, 0);
  vorbis_info_init (&self->vinfo);
  vorbis_comment_init (&self->vcomment);
  return self;
}

void
gsl_vorbis_cutter_set_cutpoint (GslVorbisCutter    *self,
                                GslVorbisCutterMode cutmode,
                                SfiNum              cutpoint)
{
  g_return_if_fail (self != NULL);

  /* cutpoint is interpreted as last_sample + 1,
   * i.e. sample[cutpoint] is removed for SAMPLE_BOUNDARY
   */

  switch (cutpoint > 0 ? cutmode : 0)
    {
    case GSL_VORBIS_CUTTER_SAMPLE_BOUNDARY:
    case GSL_VORBIS_CUTTER_PACKET_BOUNDARY:
    case GSL_VORBIS_CUTTER_PAGE_BOUNDARY:
      self->cutmode = cutmode;
      self->cutpoint = cutpoint;
      break;
    default:
      self->cutmode = GSL_VORBIS_CUTTER_NONE;
      self->cutpoint = 0;
      break;
    }
}

void
gsl_vorbis_cutter_filter_serialno (GslVorbisCutter        *self,
                                   guint                   serialno)
{
  g_return_if_fail (self != NULL);

  /* only read an input Ogg/Vorbis stream with serial number "serialno" */

  self->filtered_serialno = serialno;
  self->filter_serialno = TRUE;
}

void
gsl_vorbis_cutter_force_serialno (GslVorbisCutter        *self,
                                  guint                   serialno)
{
  g_return_if_fail (self != NULL);

  /* change the Ogg/Vorbis stream serial number on output to "serialno" */

  self->forced_serialno = serialno;
  self->force_serialno = TRUE;
}

void
gsl_vorbis_cutter_destroy (GslVorbisCutter *self)
{
  g_return_if_fail (self != NULL);
  
  /* cleanup codec state */
  if (self->vorbis_initialized)
    vorbis_dsp_clear (&self->vdsp);
  /* cleanup stream state */
  vorbis_comment_clear (&self->vcomment);
  vorbis_info_clear (&self->vinfo);
  ogg_stream_clear (&self->ostream);
  ogg_stream_clear (&self->istream);
  ogg_sync_clear (&self->isync);
  /* cleanup encoded data blocks */
  while (self->dblocks)
    g_free (sfi_ring_pop_head (&self->dblocks));
  /* cleanup self */
  g_free (self);
}

static void
vorbis_cutter_abort (GslVorbisCutter *self)
{
  /* cleanup encoded data blocks */
  while (self->dblocks)
    g_free (sfi_ring_pop_head (&self->dblocks));
  self->eos = TRUE;
}

gboolean
gsl_vorbis_cutter_ogg_eos (GslVorbisCutter *self)
{
  g_return_val_if_fail (self != NULL, FALSE);

  return self->eos && !self->dblocks;
}

guint
gsl_vorbis_cutter_read_ogg (GslVorbisCutter *self,
                            guint            n_bytes,
                            guint8          *bytes)
{
  guint8 *ubytes = bytes;

  g_return_val_if_fail (self != NULL, 0);

  while (n_bytes && self->dblocks)
    {
      CDataBlock *dblock = (CDataBlock*) self->dblocks->data;
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

static void
vorbis_cutter_process_paket (GslVorbisCutter *self,
                             ogg_packet      *opacket)
{
  guint processed_packets = self->n_packets;
  switch (self->n_packets)
    {
      gint error, window;
    case 0:     /* initial vorbis setup packet */
      error = vorbis_synthesis_headerin (&self->vinfo, &self->vcomment, opacket);
      if (error < 0)
        {
          DIAG ("ignoring packet preceeding Vorbis stream: %s", ov_error_blurb (error));
        }
      else /* valid vorbis stream start */
        {
          self->n_packets++;
          self->initial_granule = opacket->granulepos;
          self->tracking_granule = self->initial_granule;
        }
      break;
    case 1:     /* vorbis comments */
      error = vorbis_synthesis_headerin (&self->vinfo, &self->vcomment, opacket);
      if (error < 0)
        {
          DIAG ("invalid Vorbis (comment) header packet: %s", ov_error_blurb (error));
          vorbis_cutter_abort (self);
        }
      else
        self->n_packets++;
      break;
    case 2:     /* vorbis codebooks */
      error = vorbis_synthesis_headerin (&self->vinfo, &self->vcomment, opacket);
      if (error < 0)
        {
          DIAG ("invalid Vorbis (codebook) header packet: %s", ov_error_blurb (error));
          vorbis_cutter_abort (self);
        }
      else
        {
          self->n_packets++;
          /* vorbis successfully setup from the three header packets */
          vorbis_synthesis_init (&self->vdsp, &self->vinfo);
          self->vorbis_initialized = TRUE;
        }
      break;
    default:    /* audio packets */
      window = vorbis_packet_blocksize (&self->vinfo, opacket);
      if (window < 0)
        DIAG ("skipping package: %s", ov_error_blurb (window));
      else
        {
          self->n_packets++;
          /* track granule */
          if (self->last_window)
            self->tracking_granule += (self->last_window + window) >> 2;
          self->last_window = window;
        }
      break;
    }
  if (processed_packets < self->n_packets)    /* successfully processed a packet */
    {
      /* granule updates and clipping */
      if (self->n_packets > 3)  /* audio packet */
        {
          gboolean last_on_page = FALSE;
          DEBUG ("packet[%d]: b_o_s=%ld e_o_s=%ld packetno=%ld pgran=%ld granule=%ld", self->n_packets - 1,
                 opacket->b_o_s, opacket->e_o_s,
                 opacket->packetno, opacket->granulepos,
                 self->tracking_granule);
          /* update packet granulepos */
          if (opacket->granulepos < 0)
            opacket->granulepos = self->tracking_granule;
          else /* update granule (from stream if possible) */
            {
              if (!opacket->e_o_s &&    /* catch granule mismatches (before end) */
                  self->tracking_granule != opacket->granulepos)
                DIAG ("failed to track position of input ogg stream, output possibly corrupted");
              self->tracking_granule = opacket->granulepos;
              last_on_page = TRUE;      /* only the last packet of a page has a granule */
            }
          /* clip output stream */
          if (self->cutmode &&
              opacket->granulepos >= MAX (self->cutpoint, self->initial_granule + 1))
            switch (self->cutmode)
              {
              case GSL_VORBIS_CUTTER_SAMPLE_BOUNDARY:
                opacket->granulepos = MAX (self->cutpoint, self->initial_granule + 1);
                opacket->e_o_s = 1;
                break;
              case GSL_VORBIS_CUTTER_PACKET_BOUNDARY:
                opacket->e_o_s = 1;
                break;
              case GSL_VORBIS_CUTTER_PAGE_BOUNDARY:
                if (last_on_page)
                  opacket->e_o_s = 1;
                break;
              default: ;
              }
        }
      else
        DEBUG ("packet[%d]: b_o_s=%ld e_o_s=%ld packetno=%ld pgran=%ld", self->n_packets - 1,
               opacket->b_o_s, opacket->e_o_s,
               opacket->packetno, opacket->granulepos);
      /* copy packet to output stream */
      ogg_stream_packetin (&self->ostream, opacket);
      /* write output stream (vorbis needs certain packets to be page-flushed) */
      switch (self->n_packets - 1)
        {
          ogg_page opage;
        case 0:         /* initial header packet must be flushed onto it's own page */
        case 2:         /* third header package must be flushed */
          while (ogg_stream_flush (&self->ostream, &opage))
            enqueue_page (&self->dblocks, &opage);
          break;
        case 1:         /* second header packet, queued, flushed with third packet */
          break;
        case 3:         /* first audio packet */
        default:        /* remaining audio packets */
          while (ogg_stream_pageout (&self->ostream, &opage))
            enqueue_page (&self->dblocks, &opage);
          break;
        case 4:         /* second audio packet */
          while (self->initial_granule ? /* flush page on granule adjustments */
                 ogg_stream_flush (&self->ostream, &opage) :
                 ogg_stream_pageout (&self->ostream, &opage))
            enqueue_page (&self->dblocks, &opage);
          break;
        }
      /* keep track of stream end */
      self->eos = opacket->e_o_s > 0;
    }
}

void
gsl_vorbis_cutter_write_ogg (GslVorbisCutter *self,
                             guint            n_bytes,
                             guint8          *bytes)
{
  g_return_if_fail (self != NULL);
  if (n_bytes)
    g_return_if_fail (bytes != NULL);
  else
    return;

  if (!self->eos)
    {
      ogg_page opage;
      guint8 *buffer = (guint8*) ogg_sync_buffer (&self->isync, n_bytes);
      memcpy (buffer, bytes, n_bytes);
      ogg_sync_wrote (&self->isync, n_bytes);
      /* process incoming data page wise */
      while (!self->eos && ogg_sync_pageout (&self->isync, &opage) > 0)
        {
          ogg_packet opacket;
          /* resync serialno while we have no packets */
          if (self->n_packets == 0)
            {
              int serialno;
              if (self->filter_serialno)
                serialno = self->filtered_serialno;
              else
                serialno = ogg_page_serialno (&opage);
              ogg_stream_reset_serialno (&self->istream, serialno);
              if (self->force_serialno)
                ogg_stream_reset_serialno (&self->ostream, self->forced_serialno);
              else
                ogg_stream_reset_serialno (&self->ostream, serialno);
            }
          /* unpack page */
          ogg_stream_pagein (&self->istream, &opage);
          /* for each incoming packet... */
          while (!self->eos && ogg_stream_packetout (&self->istream, &opacket) > 0)
            vorbis_cutter_process_paket (self, &opacket);
        }
    }
}
