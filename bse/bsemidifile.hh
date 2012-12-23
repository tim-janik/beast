/* BSE - Better Sound Engine
 * Copyright (C) 2003 Tim Janik
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
#ifndef __BSE_MIDI_FILE_H__
#define __BSE_MIDI_FILE_H__

#include <bse/bsemidievent.hh>

G_BEGIN_DECLS

typedef struct {
  guint          n_events;
  BseMidiEvent **events;
} BseMidiFileTrack;
typedef struct {
  BseMusicalTuningType musical_tuning;
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
                                            BseErrorType *error_p);
void         bse_midi_file_free            (BseMidiFile  *smf);
void         bse_midi_file_add_part_events (BseMidiFile  *smf,
                                            guint         nth_track,
                                            BsePart      *part,
                                            BseTrack     *ptrack);
void         bse_midi_file_setup_song      (BseMidiFile  *smf,
                                            BseSong      *song);

G_END_DECLS

#endif /* __BSE_MIDI_FILE_H__ */
