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
#ifndef __BSE_MIDI_FILE_H__
#define __BSE_MIDI_FILE_H__

#include <bse/bsemidievent.h>

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
