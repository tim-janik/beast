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
 */
#include	"bsesongsequencer.h"
#include	"bsesong.h"
#include	"bsepattern.h"
#include	"bsevoice.h"
#include	"bsehunkmixer.h"
#include	<math.h>



/* --- mixing functions --- */
#include	"bsesongmixer.c"


/* --- functions --- */
void
bse_song_sequencer_setup (BseSong *song,
			  guint	   n_tracks)
{
  BseSongSequencer *sequencer;
  
  g_return_if_fail (BSE_IS_SONG (song));
  g_return_if_fail (song->sequencer == NULL);
  g_return_if_fail (n_tracks == 2);
  
  sequencer = g_new0 (BseSongSequencer, 1);
  sequencer->loop_type = BSE_LOOP_NONE;
  sequencer->next_pattern_row = 0;
  sequencer->first_pattern_row = 0;
  sequencer->last_pattern_row = 0;
  
  sequencer->next_pattern = 0;
  sequencer->first_pattern = 0;
  sequencer->last_pattern = 0;
  
  sequencer->step_counter = 0;
  sequencer->step_threshold = 0;

  sequencer->va = bse_voice_allocator_new (song->n_channels);
  sequencer->n_tracks = n_tracks;
  sequencer->mix_buffer_size = sequencer->n_tracks * BSE_TRACK_LENGTH;
  sequencer->mix_buffer = g_new0 (BseMixValue, sequencer->mix_buffer_size);
  
  song->sequencer = sequencer;
  
  bse_song_sequencer_recalc (song);
  sequencer->step_counter = sequencer->step_threshold;
}

void
bse_song_sequencer_recalc (BseSong *song)
{
  BseSongSequencer *sequencer;
  const gint notes_per_tact = 4;
  gdouble d;
  
  g_return_if_fail (BSE_IS_SONG (song));
  g_return_if_fail (song->sequencer != NULL);
  
  sequencer = song->sequencer;
  d = BSE_MIX_FREQ * 60;
  d /= song->bpm * notes_per_tact * BSE_TRACK_LENGTH;
  sequencer->step_threshold = d + 0.5;
  sequencer->step_counter %= sequencer->step_threshold;
}

void
bse_song_sequencer_set_loop (BseSong *song,
			     guint    start_pattern_index,
			     guint    end_pattern_index)
{
}

void
bse_song_sequencer_set_pattern_loop (BseSong *song,
				     guint    start_pattern_row,
				     guint    end_pattern_row)
{
}

void
bse_song_sequencer_step (BseSong *song)
{
  BseSongSequencer *sequencer;
  
  g_return_if_fail (BSE_IS_SONG (song));
  g_return_if_fail (song->sequencer != NULL);

  sequencer = song->sequencer;

  sequencer->step_counter++;
  if (sequencer->step_counter >= sequencer->step_threshold)
    {
      BsePattern *pattern;
      guint channel;
      
      sequencer->step_counter = 0;
      
      pattern = bse_song_get_pattern_from_list (song, sequencer->next_pattern);
      
      if (!pattern)
	{
	  // bse_song_sequencer_stop (song); /* FIXME */
	  // return;
	  sequencer->next_pattern = 0;
	  pattern = bse_song_get_pattern_from_list (song, sequencer->next_pattern);
	}
      
      for (channel = 0; channel < pattern->n_channels; channel++)
	{
	  BsePatternNote *note;
	  
	  note = bse_pattern_peek_note (pattern,
					channel,
					sequencer->next_pattern_row);
	  
	  bse_song_mixer_activate_voice (sequencer->va->voices[channel], note);
	}
      
      sequencer->next_pattern_row++;
      if (sequencer->next_pattern_row >= song->pattern_length)
	{
	  sequencer->next_pattern_row = 0;
	  sequencer->next_pattern++;
	}
    }
}

void
bse_song_sequencer_fill_hunk (BseSong	     *song,
			      BseSampleValue *hunk)
{
  BseSongSequencer *sequencer;
  
  g_return_if_fail (BSE_IS_SONG (song));
  g_return_if_fail (song->sequencer != NULL);
  
  sequencer = song->sequencer;
  
  bse_song_mixer_fill_buffer (sequencer, BSE_SOURCE (song)->index);
  
  /* fill the hunk and clip the values
   */
  bse_hunk_clip_mix_buffer (sequencer->n_tracks, hunk, song->volume_factor, sequencer->mix_buffer);
}

void
bse_song_sequencer_destroy (BseSong *song)
{
  BseSongSequencer *sequencer;
  
  g_return_if_fail (BSE_IS_SONG (song));
  g_return_if_fail (song->sequencer != NULL);

  sequencer = song->sequencer;
  song->sequencer = NULL;

  bse_voice_allocator_destroy (sequencer->va);
  
  g_free (sequencer->mix_buffer);
  
  g_free (sequencer);
}
