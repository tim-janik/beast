// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsemidifile.hh"
#include "bsemididecoder.hh"
#include "bseitem.hh"
#include "gslcommon.hh"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define MDEBUG(...)     BSE_KEY_DEBUG ("midi-file", __VA_ARGS__)

typedef struct {
  uint32       type;   /* four letter chunk identifier */
  uint32       length; /* length of data to follow, big-endian */
} ChunkHeader;
typedef struct {
  ChunkHeader   chunk;          /* 'MThd' */
  /* data section */
  uint16        format;         /* 0=single-track, 1=synchronous-tracks, 2=asynchronous-tracks */
  uint16        n_tracks;       /* always 1 for single-track */
  uint16        division;       /* if 0x8000 is set => SMPTE, ticks-per-quarter-note otherwise */
} SMFHeader;
/* --- functions --- */
static uint
dummy_read (int  fd,
            uint n_bytes)
{
  uint8 space[1024];
  uint total = 0;
  while (total < n_bytes)
    {
      int l = read (fd, space, MIN (n_bytes - total, 1024));
      if (l < 0)
        return total;
      total += l;
    }
  return total;
}
static BseErrorType
smf_read_header (int        fd,
                 SMFHeader *header)
{
  uint n_bytes;
  /* read standard header */
  n_bytes = 4 + 4 + 2 + 2 + 2;
  if (read (fd, header, n_bytes) != n_bytes)
    {
      MDEBUG ("failed to read midi file header: %s", g_strerror (errno));
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
      MDEBUG ("unmatched token 'MThd'");
      return BSE_ERROR_FORMAT_INVALID;
    }
  if (header->chunk.length < 6)
    {
      MDEBUG ("truncated midi file header");
      return BSE_ERROR_FORMAT_INVALID;
    }
  if (header->format > 2)
    {
      MDEBUG ("unknown midi file format");
      return BSE_ERROR_FORMAT_UNKNOWN;
    }
  if (header->format == 0 && header->n_tracks != 1)
    {
      MDEBUG ("invalid number of tracks: %d", header->n_tracks);
      return BSE_ERROR_FORMAT_INVALID;
    }
  if (header->n_tracks < 1)
    {
      MDEBUG ("midi file without tracks");
      return BSE_ERROR_NO_DATA;
    }
  if (header->division & 0x8000)        // FIXME: this allowes only tpqn
    {
      MDEBUG ("SMPTE time encoding not supported");
      return BSE_ERROR_FORMAT_UNKNOWN;
    }
  /* read up remaining unused header bytes */
  if (dummy_read (fd, header->chunk.length - 6) != header->chunk.length - 6)
    {
      MDEBUG ("failed to read midi file header: %s", g_strerror (errno));
      return gsl_error_from_errno (errno, BSE_ERROR_IO);
    }
  return BSE_ERROR_NONE;
}
static BseErrorType
smf_read_track (BseMidiFile    *smf,
                int             fd,
                BseMidiDecoder *md)
{
  ChunkHeader chunk; /* 'MTrk' */
  uint n_bytes;
  /* chunk header */
  n_bytes = 4 + 4;
  if (read (fd, &chunk, n_bytes) != n_bytes)
    {
      MDEBUG ("failed to read midi track header: %s", g_strerror (errno));
      return gsl_error_from_errno (errno, BSE_ERROR_IO);
    }
  /* endianess corrections */
  chunk.type = GUINT32_FROM_BE (chunk.type);
  chunk.length = GUINT32_FROM_BE (chunk.length);
  /* validation */
  if (chunk.type != ('M' << 24 | 'T' << 16 | 'r' << 8 | 'k'))
    {
      MDEBUG ("unmatched token 'MTrk'");
      return BSE_ERROR_FORMAT_INVALID;
    }
  /* read up and decode track data */
  n_bytes = chunk.length;
  while (n_bytes)
    {
      uint8 buffer[4096];
      int l = MIN (n_bytes, sizeof (buffer));
      if (read (fd, buffer, l) < 0)
        {
          MDEBUG ("failed to read (got %d bytes) midi track: %s", l, g_strerror (errno));
          return gsl_error_from_errno (errno, BSE_ERROR_IO);
        }
      bse_midi_decoder_push_smf_data (md, l, buffer);
      n_bytes -= l;
    }
  return BSE_ERROR_NONE;
}
BseMidiFile*
bse_midi_file_load (const char   *file_name,
                    BseErrorType *error_p)
{
  BseMidiFile *smf;
  SMFHeader header;
  BseErrorType dummy_error;
  int i, fd = open (file_name, O_RDONLY);
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
  smf = (BseMidiFile*) g_malloc0 (sizeof (BseMidiFile) + header.n_tracks * sizeof (smf->tracks[0]));
  smf->musical_tuning = BSE_MUSICAL_TUNING_12_TET;
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
      BseMidiDecoder *md = bse_midi_decoder_new (FALSE, TRUE, smf->musical_tuning);
      SfiRing *events;
      *error_p = smf_read_track (smf, fd, md);
      events = bse_midi_decoder_pop_event_list (md);
      while (events)
        {
          uint n = smf->tracks[i].n_events++;
          smf->tracks[i].events = g_renew (BseMidiEvent*, smf->tracks[i].events, smf->tracks[i].n_events);
          smf->tracks[i].events[n] = (BseMidiEvent*) sfi_ring_pop_head (&events);
        }
      // g_printerr ("track%u: n_events=%u\n", i, smf->tracks[i].n_events);
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
  uint i, j;
  for (i = 0; i < smf->n_tracks; i++)
    for (j = 0; j < smf->tracks[i].n_events; j++)
      bse_midi_free_event (smf->tracks[i].events[j]);
  for (i = 0; i < smf->n_tracks; i++)
    g_free (smf->tracks[i].events);
  g_free (smf);
}
void
bse_midi_file_add_part_events (BseMidiFile *smf,
                               uint         nth_track,
                               BsePart     *part,
                               BseTrack    *ptrack)
{
  BseMidiFileTrack *track = smf->tracks + nth_track;
  uint i, j, dur, start = 0;
  float fvalue = 0;
  for (i = 0; i < track->n_events; i++)
    {
      BseMidiEvent *event = track->events[i];
      BseMidiSignalType msignal = BseMidiSignalType (0);
      start += event->delta_time;
      switch (event->status)
        {
          float frequency, velocity;
          int note, fine_tune;
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
          note = bse_note_from_freq (smf->musical_tuning, frequency);
          fine_tune = bse_note_fine_tune_from_note_freq (smf->musical_tuning, note, frequency);
          bse_item_exec_void (part, "insert-note-auto",
                              (uint) (start * smf->tpqn_rate),
                              (uint) (dur * smf->tpqn_rate),
                              note, fine_tune, velocity);
          break;
        case BSE_MIDI_CONTROL_CHANGE:
          if (!msignal)
            {
              msignal = BseMidiSignalType (BSE_MIDI_SIGNAL_CONTROL_0 + event->data.control.control);
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
                              (uint) (start * smf->tpqn_rate), msignal, fvalue);
          break;
        case BSE_MIDI_TEXT_EVENT:
          if (track)
            {
              char *string;
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
  BseBus *master_bus;
  uint i, j;
  bse_item_set_undoable (song,
                         "tpqn", smf->tpqn,
                         "numerator", smf->numerator,
                         "denominator", smf->denominator,
                         "bpm", smf->bpm,
                         NULL);
  bse_item_exec (song, "ensure-master-bus", &master_bus);
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
          BseErrorType error;
          bse_item_exec (song, "create-track", &track);
          bse_item_exec (track, "ensure-output", &error);
          bse_assert_ok (error);
          bse_item_set_undoable (track, "n-voices", 24, NULL);
          bse_item_exec (song, "create-part", &part);
          // g_printerr ("part1: %p %s\n", part, G_OBJECT_TYPE_NAME (part));
          bse_item_exec_void (track, "insert-part", 0, part);
          // g_printerr ("part2: %p %s\n", part, G_OBJECT_TYPE_NAME (part));
          bse_midi_file_add_part_events (smf, i, part, track);
        }
    }
}
