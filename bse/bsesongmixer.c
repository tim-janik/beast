/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997, 1998, 1999 Olaf Hoehmann and Tim Janik
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
/* we get included from bsesongsequencer.c */
#include	"bsesample.h"
#include	"bseinstrument.h"
#include	"bsesinstrument.h"
#include	"bsevoice.h"
#include	"bsepattern.h"
#include	<string.h>


/* --- prototypes --- */
static void	bse_song_mixer_activate_voice	(BseVoice		*voice,
						 BsePatternNote		*note);
static void	bse_song_mixer_fill_buffer	(BseSongSequencer	*sequencer,
						 BseIndex                index);


/* --- functions --- */
/* uebernimmt alle daten aus der instrumentstruktur
 * in die voicestruktur und aktiviert die voice
 * diese funktion kann unter anderem auch dazu verwendet
 * werden um eine note ohne sequenzer anzuschlagen
 * wird eine note ohne instrument angeschlagen d.h.
 * der pointer zum instrument ist gleich NULL
 * wird das instrument nicht neu angeschlagen
 */
static void
bse_song_mixer_activate_voice (BseVoice       *voice,
			       BsePatternNote *note)
{
  g_return_if_fail (voice != NULL);
  g_return_if_fail (note != NULL);
  
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

static void
bse_song_mixer_add_fade_ramp (BseSongSequencer *sequencer,
			      BseMixValue      *buffer,
			      BseVoice	       *voice)
{
  gfloat l_volume, l_volume_delta = 0.0;
  gfloat r_volume, r_volume_delta = 0.0;
  BseMixValue *bound;
  
  g_return_if_fail (voice->input_type == BSE_VOICE_INPUT_FADE_RAMP);
  g_return_if_fail (voice->input.fade_ramp.n_values_left > 0);
  g_return_if_fail (voice->fading == TRUE);

  /* check bounds and adjust buffer size if neccessary */
  if (voice->input.fade_ramp.n_values_left > BSE_TRACK_LENGTH)
    {
      bound = buffer + BSE_TRACK_LENGTH * sequencer->n_tracks;
      voice->input.fade_ramp.n_values_left -= BSE_TRACK_LENGTH;
    }
  else
    {
      bound = buffer + voice->input.fade_ramp.n_values_left * sequencer->n_tracks;
      voice->input.fade_ramp.n_values_left = 0;
    }
  l_volume_delta = voice->left_volume_delta;
  r_volume_delta = voice->right_volume_delta;

  l_volume = voice->left_volume;
  r_volume = voice->right_volume;

  /* mix voice to buffer
   */
  do
    {
      register BseMixValue l_value = l_volume;
      register BseMixValue r_value = r_volume;
      
      *(buffer++) += l_value;
      *(buffer++) += r_value;
      
      l_volume += l_volume_delta;
      r_volume += r_volume_delta;
    }
  while (buffer < bound);

  voice->left_volume = l_volume;
  voice->right_volume = r_volume;
}

static void
bse_song_mixer_add_synth_voice (BseSongSequencer *sequencer,
				BseMixValue	 *buffer,
				BseVoice	 *voice,
				BseChunk         *chunk)
{
  BseSampleValue *hunk = chunk->hunk;
  gfloat l_volume, r_volume;
  BseMixValue *bound;
  
  g_return_if_fail (voice->input_type == BSE_VOICE_INPUT_SYNTH);
  g_return_if_fail (voice->fading == FALSE);
  g_return_if_fail (sequencer->n_tracks == 2 && voice->n_tracks == 2 && chunk->n_tracks == 2);
  g_return_if_fail (chunk->hunk_filled == TRUE);
  
  /* adjust buffer size */
  bound = buffer + BSE_TRACK_LENGTH * sequencer->n_tracks;

  l_volume = voice->left_volume;
  r_volume = voice->right_volume;

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
	   hunk[-2], voice->left_volume, hunk[-1], voice->right_volume,
	   hunk[-2]*voice->left_volume, hunk[-1]*voice->right_volume);
}

static void
bse_song_mixer_add_sample_voice (BseSongSequencer *sequencer,
				 BseMixValue	  *buffer,
				 BseVoice	  *voice)
{
  BseMixValue  *l_buffer;
  BseMixValue  *r_buffer;
  gint16 *l_sample;
  gint16 *r_sample;
  gfloat l_volume, l_volume_delta;
  gfloat r_volume, r_volume_delta;
  guint	buffer_size, n_tracks;
  gint rate;
  gint rate_pos;
  guint next_smp_word;
  guint n_sample_values;
  gboolean jump_sample_pos = FALSE;
  
  g_return_if_fail (voice->input_type == BSE_VOICE_INPUT_SAMPLE);
  g_return_if_fail (voice->input.sample.bound > voice->input.sample.cur_pos);
  
  buffer_size = sequencer->mix_buffer_size;
  n_tracks = sequencer->n_tracks;
  
  l_buffer = buffer;
  r_buffer = buffer + 1;
  rate = voice->input.sample.rate;
  rate_pos = voice->input.sample.pos_frac;
  l_sample = voice->input.sample.cur_pos;
  r_sample = voice->input.sample.cur_pos + 1;
  
  /* check for rate < 0 */
  next_smp_word = MAX (rate >> 16, 1);
  
  /* check bounds and adjust buffer size if neccessary */
  n_sample_values = voice->input.sample.pos_frac + rate * (BSE_TRACK_LENGTH - 1);
  n_sample_values >>= 16;
  if (voice->input.sample.bound <= voice->input.sample.cur_pos + n_sample_values)
    {
      n_sample_values = voice->input.sample.bound - voice->input.sample.cur_pos;
      buffer_size = ((n_sample_values << 16) - voice->input.sample.pos_frac) / rate + 1;
      buffer_size *= n_tracks;
      jump_sample_pos = TRUE; /* need this flag to get around rounding errors */
    }

  l_volume = voice->left_volume;
  r_volume = voice->right_volume;

  /* these are always 0, except when fading */
  l_volume_delta = voice->left_volume_delta;
  r_volume_delta = voice->right_volume_delta;
  
  /* hier kommen spaeter die aufrufe fuer die optimierten
   * routinen hin
   */
  
  /* mix voice to buffer
   */
  if (voice->n_tracks == 1) /* mono sample */
    { 
      guint i;
      
      for (i = 0; i < buffer_size; i += n_tracks)
	{
	  BseMixValue pos_low;
	  BseMixValue pos_high;
	  BseMixValue l_value;
	  BseMixValue r_value;
	  
	  pos_low  = rate_pos & 0xffff;
	  pos_high = rate_pos >> 16;
	  
	  l_value =
	    ((BseMixValue) l_sample[ pos_high ]) * (0x00010000 - pos_low) +
	    ((BseMixValue) l_sample[ pos_high + next_smp_word ]) * pos_low;
	  r_value = l_value;
	  
	  /* shift *atfer* the volume multiplication
	   */
	  l_value *= l_volume;
	  r_value *= r_volume;
	  l_value >>= 16;
	  r_value >>= 16;

	  l_buffer[i] += l_value;
	  r_buffer[i] += r_value;
	  
	  l_volume += l_volume_delta;
	  r_volume += r_volume_delta;
	  rate_pos += rate;
	}
      voice->input.sample.cur_pos += rate_pos >> 16;
      voice->input.sample.pos_frac = rate_pos & 0xffff;
    }
  else if (voice->n_tracks == 2) /* stereo sample */
    {
      guint i;

      next_smp_word *= n_tracks;
      for (i = 0; i < buffer_size; i += n_tracks)
	{
	  register BseMixValue pos_low;
	  register BseMixValue pos_high;
	  register BseMixValue l_value;
	  register BseMixValue r_value;
	  
	  pos_low  = rate_pos & 0xffff;
	  pos_high = rate_pos >> 16;
	  
	  l_value =
	    ((BseMixValue) l_sample[ pos_high ]) * (0x00010000 - pos_low) +
	    ((BseMixValue) l_sample[ pos_high + next_smp_word ]) * pos_low;
	  r_value =
	    ((BseMixValue) r_sample[ pos_high ]) * (0x00010000 - pos_low) +
	    ((BseMixValue) r_sample[ pos_high + next_smp_word ]) * pos_low;
	  
	  /* shift *after* the volume multiply
	   */
	  l_value *= l_volume;
	  r_value *= r_volume;
	  l_value >>= 16;
	  r_value >>= 16;
	  
	  l_buffer[i] += l_value;
	  r_buffer[i] += r_value;
	  
	  l_volume += l_volume_delta;
	  r_volume += r_volume_delta;
	  rate_pos += rate;
	}
      voice->input.sample.cur_pos += (rate_pos >> 16) * n_tracks;
      voice->input.sample.pos_frac = rate_pos & 0xffff;
    }
  else
    g_assert_not_reached ();

  if (voice->fading)
    {
      voice->left_volume = l_volume;
      voice->right_volume = r_volume;
    }
  if (jump_sample_pos)
    {
      // printf ("last volumes (%3u): %9f %9f\n", voice->input.sample.bound - voice->input.sample.cur_pos, l_volume, r_volume);
      voice->input.sample.cur_pos = voice->input.sample.bound;
    }
}

/* fuellt den angegebenen buffer mit den aktiven voices
 */
static void
bse_song_mixer_fill_buffer (BseSongSequencer *sequencer,
			    BseIndex          index)
{
  BseMixValue *buffer = sequencer->mix_buffer;
  BseVoiceAllocator *va = sequencer->va;
  guint i, n_fixed_voices = va->n_voices;
  
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
	    bse_song_mixer_add_sample_voice (sequencer, buffer, voice);
	  else if (voice->input_type == BSE_VOICE_INPUT_SYNTH)
	    {
	      BseSource *source = BSE_SOURCE (voice->input.synth.sinstrument);
	      BseChunk *chunk = bse_source_ref_chunk (source, BSE_DFL_OCHANNEL_ID, index);

	      voice->input.synth.last_index = index;

	      bse_chunk_complete_hunk (chunk);
	      bse_song_mixer_add_synth_voice (sequencer, buffer, voice, chunk);
	      bse_chunk_unref (chunk);
	    }
	  else if (voice->input_type == BSE_VOICE_INPUT_FADE_RAMP)
	    bse_song_mixer_add_fade_ramp (sequencer, buffer, voice);
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
