/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997, 1998, 1999, 2000 Olaf Hoehmann and Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * bsesongsequencer.h: BSE song sequencer
 */
#ifndef __BSE_SONG_SEQUENCER_H__
#define __BSE_SONG_SEQUENCER_H__

#include	<bse/bseenums.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- structures --- */
struct _BseSongSequencer
{
  BseLoopType	loop_type;

  /* play order setup */
  guint first_pattern;
  guint last_pattern;
  guint first_pattern_row;
  guint last_pattern_row;

  /* row fraction */
  guint step_counter;
  guint step_threshold;
  
  /* current row */
  guint cur_pattern;
  guint cur_row;

  /* look ahead */
  guint next_pattern;
  guint next_row;
  
  /* mixer section
   */
  BseVoiceAllocator *va;
  guint	             n_tracks;
  guint	             mix_buffer_size;
  BseMixValue       *mix_buffer;
};


/* --- functions --- */
void	bse_song_sequencer_setup		(BseSong	*song,
						 guint		 n_tracks);
void	bse_song_sequencer_recalc		(BseSong	*song);
void	bse_song_sequencer_set_loop		(BseSong	*song,
						 guint		 start_pattern_index,
						 guint		 end_pattern_index);
void	bse_song_sequencer_set_pattern_loop	(BseSong	*song,
						 guint		 start_pattern_row,
						 guint		 end_pattern_row);
void	bse_song_sequencer_step			(BseSong	*song);
void	bse_song_sequencer_fill_hunk		(BseSong	*song,
						 BseSampleValue	*hunk);
void	bse_song_sequencer_destroy		(BseSong	*song);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_SONG_SEQUENCER_H__ */
