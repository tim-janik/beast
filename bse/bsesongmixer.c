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
#include	"bsevoice.h"
#include	"bsepattern.h"
#include	<string.h>


/* --- prototypes --- */
static void	bse_song_mixer_activate_voice	(BseVoice		*voice,
						 BseNote		*note);
static void	bse_song_mixer_fill_buffer	(BseSongSequencer	*sequencer);


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
bse_song_mixer_activate_voice (BseVoice *voice,
			       BseNote  *note)
{
  g_return_if_fail (voice != NULL);
  g_return_if_fail (note != NULL);
  
  if (note->instrument)
    {
      BseInstrument *instrument = note->instrument;
      BseSample *sample = instrument->type == BSE_INSTRUMENT_SAMPLE ? instrument->sample : NULL;

      /* we got a new instrument, so we need to strike a new note
       */

      if (voice->active)
	{
	  if (!voice->polyphony)
	    bse_voice_fade_out (voice);
	  voice = bse_voice_make_poly_and_renew (voice);
	}

      if (note->note == BSE_NOTE_VOID || !sample)
	{
	  /* this is actually a pretty senseless note
	   * FIXME: olaf?
	   */
	  return;
	}

      /* ok, setup the voice
       */
      bse_voice_activate (voice, instrument);
      bse_voice_set_note (voice, note->note, voice->fine_tune);
      bse_voice_set_envelope_part (voice, BSE_ENVELOPE_PART_DELAY);  
    }
  else if (note->note != BSE_NOTE_VOID &&
	   voice->active &&
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
bse_song_mixer_add_voice_to_buffer (BseSongSequencer *sequencer,
				    BseVoice	     *voice,
				    BseMixValue	     *buffer)
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
  
  g_return_if_fail (sequencer != NULL);
  g_return_if_fail (voice != NULL);
  
  buffer_size = sequencer->mix_buffer_size;
  n_tracks = sequencer->n_tracks;
  
  l_buffer = buffer;
  r_buffer = buffer + 1;
  rate = voice->sample_rate;
  rate_pos = voice->sample_pos_frac;
  l_sample = voice->sample_pos;
  r_sample = voice->sample_pos + 1;
  
  /* check for rate < 0 */
  next_smp_word = MAX (rate >> 16, 1);
  
  /* check bounds and adjust buffer size if neccessary */
  n_sample_values = voice->sample_pos_frac + rate * (BSE_TRACK_LENGTH - 1);
  n_sample_values >>= 16;
  if (voice->sample_end_pos <= voice->sample_pos + n_sample_values)
    {
      n_sample_values = voice->sample_end_pos - voice->sample_pos;
      buffer_size = ((n_sample_values << 16) - voice->sample_pos_frac) / rate + 1;
      buffer_size *= n_tracks;
      jump_sample_pos = TRUE; /* need this flag to get around rounding errors */
    }
  
  l_volume_delta = voice->fading ? voice->left_volume_delta : 0;
  r_volume_delta = voice->fading ? voice->right_volume_delta : 0;
  l_volume = voice->left_volume;
  r_volume = voice->right_volume;
  
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
      voice->sample_pos += rate_pos >> 16;
      voice->sample_pos_frac = rate_pos & 0xffff;
    }
  else /* stereo sample */
    {
      guint i;

      next_smp_word *= 2;
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
	    ((BseMixValue) l_sample[ pos_high ]) * (0x00010000 - pos_low) +
	    ((BseMixValue) l_sample[ pos_high + next_smp_word ]) * pos_low;
	  
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
      voice->sample_pos += (rate_pos >> 16) << 1;
      voice->sample_pos_frac = rate_pos & 0xffff;
    }
  voice->left_volume = l_volume;
  voice->right_volume = r_volume;
  if (jump_sample_pos)
    {
      // printf ("last volumes (%3u): %9f %9f\n", voice->sample_end_pos - voice->sample_pos, l_volume, r_volume);
      voice->sample_pos = voice->sample_end_pos;
    }
}

/* fuellt den angegebenen buffer mit den aktiven voices
 */
static void
bse_song_mixer_fill_buffer (BseSongSequencer *sequencer)
{
  BseMixValue *buffer = sequencer->mix_buffer;
  BseVoiceAllocator *va = sequencer->va;
  
  /* clear buffer, we use memset() because this function is
   * most likely hand-optimized for the target machine
   */
  memset (buffer, 0, sizeof (BseMixValue) * sequencer->mix_buffer_size);

  /* iterate over the poly voices, we need an extra loop construction
   * for this because poly voices can get rearranged for efficiency
   * reasons. we compensate such rearangements with the ->next_voice
   * counter in the rearranging code.
   */
  va->next_voice = 0;
  while (va->next_voice < va->n_voices)
    {
      BseVoice *voice = va->voices[va->next_voice++];

      /* the fixed voices may be inactive */
      if (!voice->active)
	continue;

      /* update values for buffer mix
       */
      bse_voice_preprocess (voice);
      
      /* add voice to the buffer
       */
      bse_song_mixer_add_voice_to_buffer (sequencer, voice, buffer);

      /* handle effects
       */
      bse_voice_postprocess (voice);
    }
}
