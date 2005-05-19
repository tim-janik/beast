/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bsemidifile.h"
#include "bsemididecoder.h"
#include "bseitem.h"
#include "gslcommon.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

static SFI_MSG_TYPE_DEFINE (debug_midi_file, "midi-file", SFI_MSG_NONE, NULL);
#define DEBUG(...)      sfi_debug (debug_midi_file, __VA_ARGS__)

typedef struct {
  guint32       type;   /* four letter chunk identifier */
  guint32       length; /* length of data to follow, big-endian */
} ChunkHeader;

typedef struct {
  ChunkHeader   chunk;          /* 'MThd' */
  /* data section */
  guint16       format;         /* 0=single-track, 1=synchronous-tracks, 2=asynchronous-tracks */
  guint16       n_tracks;       /* always 1 for single-track */
  guint16       division;       /* if 0x8000 is set => SMPTE, ticks-per-quarter-note otherwise */
} SMFHeader;


/* --- functions --- */
static guint
dummy_read (gint  fd,
            guint n_bytes)
{
  guint8 space[1024];
  guint total = 0;
  while (total < n_bytes)
    {
      gint l = read (fd, space, MIN (n_bytes - total, 1024));
      if (l < 0)
        return total;
      total += l;
    }
  return total;
}

static BseErrorType
smf_read_header (gint       fd,
                 SMFHeader *header)
{
  guint n_bytes;
  /* read standard header */
  n_bytes = 4 + 4 + 2 + 2 + 2;
  if (read (fd, header, n_bytes) != n_bytes)
    {
      DEBUG ("failed to read midi file header: %s", g_strerror (errno));
      return gsl_error_from_errno (errno, BSE_ERROR_IO);
    }
  /* endianess corrections */
  header->chunk.type = GUINT32_FROM_BE (header->chunk.type);
  header->chunk.length = GUINT32_FROM_BE (header->chunk.length);
  header->format = GUINT16_FROM_BE (header->format);
  header->n_tracks = GUINT16_FROM_BE (header->n_tracks);
  header->division = GUINT16_FROM_BE (header->division);
  /* validation */
  if (header->chunk.type != ('M' << 24 | 'T' << 16 | 'h' << 8 | 'd'))
    {
      DEBUG ("unmatched token 'MThd'");
      return BSE_ERROR_FORMAT_INVALID;
    }
  if (header->chunk.length < 6)
    {
      DEBUG ("truncated midi file header");
      return BSE_ERROR_FORMAT_INVALID;
    }
  if (header->format > 2)
    {
      DEBUG ("unknown midi file format");
      return BSE_ERROR_FORMAT_UNKNOWN;
    }
  if (header->format == 0 && header->n_tracks != 1)
    {
      DEBUG ("invalid number of tracks: %d", header->n_tracks);
      return BSE_ERROR_FORMAT_INVALID;
    }
  if (header->n_tracks < 1)
    {
      DEBUG ("midi file without tracks");
      return BSE_ERROR_NO_DATA;
    }
  if (header->division & 0x8000)        // FIXME: this allowes only tpqn
    {
      DEBUG ("SMPTE time encoding not supported");
      return BSE_ERROR_FORMAT_UNKNOWN;
    }
  /* read up remaining unused header bytes */
  if (dummy_read (fd, header->chunk.length - 6) != header->chunk.length - 6)
    {
      DEBUG ("failed to read midi file header: %s", g_strerror (errno));
      return gsl_error_from_errno (errno, BSE_ERROR_IO);
    }
  return BSE_ERROR_NONE;
}

static BseErrorType
smf_read_track (BseMidiFile    *smf,
                gint            fd,
                BseMidiDecoder *md)
{
  ChunkHeader chunk; /* 'MTrk' */
  guint n_bytes;
  /* chunk header */
  n_bytes = 4 + 4;
  if (read (fd, &chunk, n_bytes) != n_bytes)
    {
      DEBUG ("failed to read midi track header: %s", g_strerror (errno));
      return gsl_error_from_errno (errno, BSE_ERROR_IO);
    }
  /* endianess corrections */
  chunk.type = GUINT32_FROM_BE (chunk.type);
  chunk.length = GUINT32_FROM_BE (chunk.length);
  /* validation */
  if (chunk.type != ('M' << 24 | 'T' << 16 | 'r' << 8 | 'k'))
    {
      DEBUG ("unmatched token 'MTrk'");
      return BSE_ERROR_FORMAT_INVALID;
    }
  /* read up and decode track data */
  n_bytes = chunk.length;
  while (n_bytes)
    {
      guint8 buffer[4096];
      gint l = MIN (n_bytes, sizeof (buffer));
      if (read (fd, buffer, l) < 0)
        {
          DEBUG ("failed to read (got %d bytes) midi track: %s", l, g_strerror (errno));
          return gsl_error_from_errno (errno, BSE_ERROR_IO);
        }
      bse_midi_decoder_push_smf_data (md, l, buffer);
      n_bytes -= l;
    }
  return BSE_ERROR_NONE;
}

BseMidiFile*
bse_midi_file_load (const gchar  *file_name,
                    BseErrorType *error_p)
{
  BseMidiFile *smf;
  SMFHeader header;
  BseErrorType dummy_error;
  gint i, fd = open (file_name, O_RDONLY);
  if (!error_p)
    error_p = &dummy_error;
  if (fd < 0)
    {
      *error_p = gsl_error_from_errno (errno, BSE_ERROR_FILE_OPEN_FAILED);
      return NULL;
    }

  *error_p = smf_read_header (fd, &header);
  if (*error_p)
    {
      close (fd);
      return NULL;
    }

  smf = g_malloc0 (sizeof (BseMidiFile) + header.n_tracks * sizeof (smf->tracks[0]));
#if 0
  smf->tpqn = header.division;
  smf->tpqn_rate = 1;
#else
  smf->tpqn = 384;
  smf->tpqn_rate = smf->tpqn / (double) header.division;
#endif
  smf->bpm = 120;
  smf->numerator = 4;
  smf->denominator = 4;
  smf->n_tracks = header.n_tracks;
  for (i = 0; i < header.n_tracks; i++)
    {
      BseMidiDecoder *md = bse_midi_decoder_new (FALSE, TRUE);
      SfiRing *events;
      *error_p = smf_read_track (smf, fd, md);
      events = bse_midi_decoder_pop_event_list (md);
      while (events)
        {
          guint n = smf->tracks[i].n_events++;
          smf->tracks[i].events = g_renew (BseMidiEvent*, smf->tracks[i].events, smf->tracks[i].n_events);
          smf->tracks[i].events[n] = sfi_ring_pop_head (&events);
        }
      g_printerr ("track%u: n_events=%u\n", i, smf->tracks[i].n_events);
      bse_midi_decoder_destroy (md);
      if (*error_p)
        {
          close (fd);
          bse_midi_file_free (smf);
          return NULL;
        }
    }
  /* extract time signature events from beginning of master track */
  for (i = 0; i < MIN (16, smf->tracks[0].n_events); i++)
    {
      BseMidiEvent *event = smf->tracks[0].events[i];
      if (event->status == BSE_MIDI_SET_TEMPO)
        smf->bpm = event->data.usecs_pqn ? 60000000.0 / event->data.usecs_pqn : 120;
      else if (event->status == BSE_MIDI_TIME_SIGNATURE)
        {
          smf->numerator = event->data.time_signature.numerator;
          smf->denominator = event->data.time_signature.denominator;
        }
    }
  *error_p = BSE_ERROR_NONE;
  return smf;
}

void
bse_midi_file_free (BseMidiFile *smf)
{
  guint i, j;
  for (i = 0; i < smf->n_tracks; i++)
    for (j = 0; j < smf->tracks[i].n_events; j++)
      bse_midi_free_event (smf->tracks[i].events[j]);
  for (i = 0; i < smf->n_tracks; i++)
    g_free (smf->tracks[i].events);
  g_free (smf);
}

void
bse_midi_file_add_part_events (BseMidiFile *smf,
                               guint        nth_track,
                               BsePart     *part,
                               BseTrack    *ptrack)
{
  BseMidiFileTrack *track = smf->tracks + nth_track;
  BseMidiSignalType msignal = 0;
  guint i, j, dur, start = 0;
  gfloat fvalue = 0;
  for (i = 0; i < track->n_events; i++)
    {
      BseMidiEvent *event = track->events[i];
      start += event->delta_time;
      switch (event->status)
        {
          gfloat frequency, velocity;
          gint note, fine_tune;
        case BSE_MIDI_NOTE_ON:
          frequency = event->data.note.frequency;
          velocity = event->data.note.velocity;
          for (dur = 0, j = i + 1; j < track->n_events; j++)
            {
              dur += track->events[j]->delta_time;
              if (track->events[j]->status == BSE_MIDI_NOTE_OFF &&
                  track->events[j]->data.note.frequency == frequency)
                break;
            }
          note = bse_note_from_freq (frequency);
          fine_tune = bse_note_fine_tune_from_note_freq (note, frequency);
          bse_item_exec_void (part, "insert-note-auto",
                              (guint) (start * smf->tpqn_rate),
                              (guint) (dur * smf->tpqn_rate),
                              note, fine_tune, velocity);
          break;
        case BSE_MIDI_CONTROL_CHANGE:
          if (!msignal)
            {
              msignal = BSE_MIDI_SIGNAL_CONTROL_0 + event->data.control.control;
              fvalue = event->data.control.value;
            }
        case BSE_MIDI_PROGRAM_CHANGE:
          if (!msignal)
            {
              msignal = BSE_MIDI_SIGNAL_PROGRAM;
              fvalue = event->data.program * (1.0 / (double) 0x7F);
            }
        case BSE_MIDI_CHANNEL_PRESSURE:
          if (!msignal)
            {
              msignal = BSE_MIDI_SIGNAL_PRESSURE;
              fvalue = event->data.intensity;
            }
        case BSE_MIDI_PITCH_BEND:
          if (!msignal)
            {
              msignal = BSE_MIDI_SIGNAL_PITCH_BEND;
              fvalue = event->data.pitch_bend;
            }
          bse_item_exec_void (part, "insert-control",
                              (guint) (start * smf->tpqn_rate), msignal, fvalue);
          break;
        case BSE_MIDI_TEXT_EVENT:
          if (track)
            {
              gchar *string;
              bse_item_get (ptrack, "blurb", &string, NULL);
              if (string && string[0])
                string = g_strconcat (string, " ", event->data.text, NULL);
              else
                string = g_strdup (event->data.text);
              bse_item_set_undoable (ptrack, "blurb", string, NULL);
              g_free (string);
            }
          break;
        case BSE_MIDI_TRACK_NAME:
          if (track)
            bse_item_set_undoable (ptrack, "uname", event->data.text, NULL);
          break;
        case BSE_MIDI_INSTRUMENT_NAME:
          bse_item_set_undoable (part, "uname", event->data.text, NULL);
          break;
        default: ;
        }
    }
}

void
bse_midi_file_setup_song (BseMidiFile    *smf,
                          BseSong        *song)
{
  guint i, j;
  bse_item_set_undoable (song,
                         "tpqn", smf->tpqn,
                         "numerator", smf->numerator,
                         "denominator", smf->denominator,
                         "bpm", smf->bpm,
                         NULL);
  for (i = 0; i < smf->n_tracks; i++)
    {
      BseMidiFileTrack *track = smf->tracks + i;
      gboolean uses_voice = FALSE;
      for (j = 0; j < track->n_events && !uses_voice; j++)
        uses_voice = BSE_MIDI_CHANNEL_VOICE_MESSAGE (track->events[j]->status);
      if (uses_voice)
        {
          BseTrack *track;
          BsePart *part;
          bse_item_exec (song, "create-track", &track);
          bse_item_set_undoable (track, "n-voices", 24, NULL);
          bse_item_exec (song, "create-part", &part);
          g_printerr ("part1: %p %s\n", part, G_OBJECT_TYPE_NAME (part));
          bse_item_exec_void (track, "insert-part", 0, part);
          g_printerr ("part2: %p %s\n", part, G_OBJECT_TYPE_NAME (part));
          bse_midi_file_add_part_events (smf, i, part, track);
        }
    }
}
