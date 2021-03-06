// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_MIDI_FILE_H__
#define __BSE_MIDI_FILE_H__

#include <bse/bsemidievent.hh>

typedef struct {
  guint          n_events;
  BseMidiEvent **events;
} BseMidiFileTrack;
typedef struct {
  Bse::MusicalTuning musical_tuning;
  guint  tpqn;          /* ticks-per-quarter-note */
  gfloat tpqn_rate;
  /* signature */
  gfloat bpm;
  guint  numerator, denominator;
  /* tracks */
  guint            n_tracks;
  BseMidiFileTrack tracks[1]; /* flexible array */
} BseMidiFile;

BseMidiFile* bse_midi_file_load            (const gchar  *file_name,
                                            Bse::Error *error_p);
void         bse_midi_file_free            (BseMidiFile  *smf);
void         bse_midi_file_add_part_events (BseMidiFile  *smf,
                                            guint         nth_track,
                                            BsePart      *part,
                                            BseTrack     *ptrack);
void         bse_midi_file_setup_song      (BseMidiFile  *smf,
                                            BseSong      *song);

#endif /* __BSE_MIDI_FILE_H__ */
