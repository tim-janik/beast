/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2002 Tim Janik
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
#ifndef __BSE_SONG_SEQUENCER_H__
#define __BSE_SONG_SEQUENCER_H__

#include <bse/bsesong.h>
#include <bse/bsepart.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef struct {
  BsePart         *part;
  BseMidiReceiver *midi_receiver;
  guint	         tick;	/* next unhandled part tick */
} BseSongSequencerTrack;

#define	bse_song_sequencer_recalc(song)		({})
struct _BseSongSequencer
{
  BseSong *song;

  guint64  start_stamp;
  guint64  cur_stamp;
  guint64  next_stamp;
  gdouble  beats_per_second;
  guint    beat_tick;

  guint	   n_tracks;
  BseSongSequencerTrack *tracks;
};


BseSongSequencer*	bse_song_sequencer_setup	(BseSong		*song);
void			bse_song_sequencer_destroy	(BseSongSequencer	*sequencer);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_SONG_SEQUENCER_H__ */
