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
static void      bse_song_mixer_activate_voice  (BseVoice               *voice,
						 BsePatternNote         *note);
static void      bse_song_mixer_fill_buffer     (BseSongSequencer       *sequencer,
						 BseIndex                index);
static void	 bse_song_mixer_add_voice	(BseSongSequencer	*sequencer,
						 BseVoice		*voice);
     

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
  // FIXME
}

void
bse_song_sequencer_set_pattern_loop (BseSong *song,
				     guint    start_pattern_row,
				     guint    end_pattern_row)
{
  // FIXME
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
      
      for (channel = 0; channel < song->n_channels; channel++)
	{
	  static const BsePatternNote empty_note = { NULL, BSE_NOTE_VOID, 0, 0, NULL };
	  BsePatternNote *note;
	  
	  note = (!pattern ? &empty_note :
		  bse_pattern_peek_note (pattern,
					 channel,
					 sequencer->next_pattern_row));
	  
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
  bse_hunk_clip_from_mix_buffer (sequencer->n_tracks, hunk, song->volume_factor, sequencer->mix_buffer);
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

static void
bse_song_mixer_fill_buffer (BseSongSequencer *sequencer,
			    BseIndex          index)
{
  BseMixValue *buffer = sequencer->mix_buffer;
  BseVoiceAllocator *va = sequencer->va;
  guint i, n_fixed_voices = va->n_voices;

  /* fuellt den angegebenen buffer mit den aktiven voices
   */

  /* clear buffer, we use memset() because this function is
   * most likely hand-optimized for the target machine
   */
  memset (buffer, 0, sizeof (BseMixValue) * sequencer->mix_buffer_size);

  for (i = 0; i < n_fixed_voices; i++)
    {
      BseVoice *voice, *last = va->voices[i];

      /* handle the fixed voice first (which is always accessible), then
       * process the poly voices, we need an extra loop construction
       * for this because poly voices can get unlinked within this
       * loop for efficiency reasons. we compensate such rearangements
       * through the last voice pointer.
       */
      for (voice = last->input_type != BSE_VOICE_INPUT_NONE ? last : last->next; voice; voice = last->next)
	{
	  /* update values for buffer mix, handle some effects
	   */
	  if (!bse_voice_preprocess (voice))
	    continue;

	  /* add voice to mix buffer
	   */
	  if (voice->input_type == BSE_VOICE_INPUT_SAMPLE)
	    bse_song_mixer_add_voice (sequencer, voice);
	  else if (voice->input_type == BSE_VOICE_INPUT_SYNTH)
	    {
	      g_error ("foo");
	      /*
	      BseSource *source = BSE_SOURCE (voice->input.synth.sinstrument);
	      BseChunk *chunk = bse_source_ref_chunk (source, BSE_DFL_OCHANNEL_ID, index);

	      voice->input.synth.last_index = index;

	      bse_chunk_complete_hunk (chunk);
	      bse_song_mixer_add_synth_voice (sequencer, buffer, voice, chunk);
	      bse_chunk_unref (chunk);
	      */
	    }
	  else
	    g_assert_not_reached ();

	  /* handle effects
	   */
	  if (!bse_voice_postprocess (voice))
	    continue;
	  last = voice;
	}
    }
}

static void
bse_song_mixer_activate_voice (BseVoice       *voice,
			       BsePatternNote *note)
{
  g_return_if_fail (voice != NULL);
  g_return_if_fail (note != NULL);

  /* uebernimmt alle daten aus der instrumentstruktur
   * in die voicestruktur und aktiviert die voice
   * diese funktion kann unter anderem auch dazu verwendet
   * werden um eine note ohne sequenzer anzuschlagen
   * wird eine note ohne instrument angeschlagen d.h.
   * der pointer zum instrument ist gleich NULL
   * wird das instrument nicht neu angeschlagen
   */

  if (note->instrument)
    {
      BseInstrument *instrument = note->instrument;

      /* we got a new instrument, so we need to strike a new note
       */

      if (voice->input_type != BSE_VOICE_INPUT_NONE)
	{
	  if (voice->input_type != BSE_VOICE_INPUT_SAMPLE || !voice->make_poly)
	    bse_voice_fade_out (voice);
	  voice = bse_voice_make_poly_and_renew (voice);
	}

      if (note->note == BSE_NOTE_VOID || instrument->type == BSE_INSTRUMENT_NONE)
	{
	  /* this is actually a pretty senseless note
	   * FIXME: olaf?
	   */
	  return;
	}

      /* ok, setup the voice
       */
      bse_voice_activate (voice, instrument, note->note, voice->fine_tune);
      bse_voice_set_envelope_part (voice, BSE_ENVELOPE_PART_DELAY);
    }
  else if (note->note != BSE_NOTE_VOID &&
	   voice->input_type != BSE_VOICE_INPUT_NONE &&
	   note->note != voice->note)
    {
      /* only the note changed, so adjust the stepping rates
       */
      bse_voice_set_note (voice, note->note, voice->fine_tune);
    }

  /* FIXME: handle effects here
   */
}

#if 0
static void
bse_song_mixer_add_synth_voice (BseSongSequencer *sequencer,
				BseMixValue      *buffer,
				BseVoice         *voice,
				BseChunk         *chunk)
{
  BseSampleValue *hunk = chunk->hunk;
  gfloat l_volume, r_volume;
  BseMixValue *bound;

  g_return_if_fail (voice->input_type == BSE_VOICE_INPUT_SYNTH);
  g_return_if_fail (voice->fading == FALSE);
  g_return_if_fail (sequencer->n_tracks == 2 /* && voice->n_tracks == 2 */ && chunk->n_tracks == 2);
  g_return_if_fail (chunk->hunk_filled == TRUE);

  /* adjust buffer size */
  bound = buffer + BSE_TRACK_LENGTH * sequencer->n_tracks;

  l_volume = voice->volume.left;
  r_volume = voice->volume.right;

  /* mix voice to buffer
   */
  do
    {
      register BseMixValue l_value = *(hunk++);
      register BseMixValue r_value = *(hunk++);

      l_value *= l_volume;
      r_value *= r_volume;

      *(buffer++) += l_value;
      *(buffer++) += r_value;
    }
  while (buffer < bound);
  g_print ("last_vals: %p: %+6d*%f %+6d*%f %+6.0f %+6.0f\n", chunk->hunk,
	   hunk[-2], voice->volume.left, hunk[-1], voice->volume.right,
	   hunk[-2]*voice->volume.left, hunk[-1]*voice->volume.right);
}
#endif

static void
bse_song_mixer_add_voice (BseSongSequencer *sequencer,
			  BseVoice         *voice)
{
  BseMixBuffer mbuffer = { 0, };

  g_return_if_fail (voice->input_type == BSE_VOICE_INPUT_SAMPLE);
  g_return_if_fail (BSE_MIX_SOURCE_ACTIVE (&voice->source));

  mbuffer.n_tracks = sequencer->n_tracks;
  mbuffer.buffer = sequencer->mix_buffer;
  mbuffer.bound = mbuffer.buffer + sequencer->mix_buffer_size;

  bse_mix_buffer_add (&mbuffer, &voice->source, &voice->volume, &voice->input.sample.rate);
  if (mbuffer.buffer < mbuffer.bound &&
      bse_voice_need_after_fade (voice))
    bse_mix_buffer_add (&mbuffer, &voice->source, &voice->volume, &voice->input.sample.rate);
}
