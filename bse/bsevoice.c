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
#include	"bsevoice.h"



/* --- functions --- */
BseVoiceAllocator*
bse_voice_allocator_new (guint n_fixed_voices)
{
  BseVoiceAllocator *allocator;
  BseVoice *block;
  guint i;

  g_return_val_if_fail (n_fixed_voices > 0, NULL);

  allocator = g_new (BseVoiceAllocator, 1);
  allocator->n_total_voices = n_fixed_voices + n_fixed_voices * BSE_VOICES_POLY_OVER_FIXED;
  allocator->n_voices = n_fixed_voices;
  allocator->voices = g_new (BseVoice*, allocator->n_total_voices);
  allocator->next_voice = 0;
  allocator->n_fixed_voices = n_fixed_voices;

  block = g_new0 (BseVoice, allocator->n_total_voices);
  allocator->voice_blocks = g_slist_prepend (NULL, block);

  for (i = 0; i < allocator->n_total_voices; i++)
    {
      BseVoice *voice = block++;

      bse_voice_reset (voice);
      voice->allocator = allocator;
      voice->index = i;
      allocator->voices[i] = voice;
    }

  return allocator;
}

void
bse_voice_allocator_destroy (BseVoiceAllocator *allocator)
{
  guint i;
  GSList *slist;

  g_return_if_fail (allocator != NULL);

  while (allocator->n_voices > allocator->n_fixed_voices)
    bse_voice_reset (allocator->voices[allocator->n_voices - 1]);
  for (i = 0; i < allocator->n_fixed_voices; i++)
    bse_voice_reset (allocator->voices[i]);

  g_free (allocator->voices);

  for (slist = allocator->voice_blocks; slist; slist = slist->next)
    g_free (slist->data);

  g_slist_free (allocator->voice_blocks);

  g_free (allocator);
}

static inline void
bse_voice_allocator_check_free_voice (BseVoice *voice)
{
  BseVoiceAllocator *allocator = voice->allocator;
  
  g_return_if_fail (voice->index < allocator->n_voices);
  g_return_if_fail (voice->active == FALSE);
  
  if (voice->index >= allocator->n_fixed_voices)
    {
      guint i = voice->index;

      allocator->n_voices--;
      if (i < allocator->n_voices)
	{
	  allocator->voices[i] = allocator->voices[allocator->n_voices];
	  allocator->voices[i]->index = i;
	  allocator->voices[allocator->n_voices] = voice;
	  voice->index = allocator->n_voices;
	  if (i + 1 == allocator->next_voice)
	    allocator->next_voice--;
	}
    }
}

static inline BseVoice*
bse_voice_allocator_renew_voice (BseVoiceAllocator *allocator,
				 guint              index)
{
  BseVoice *voice;

  if (allocator->n_voices >= allocator->n_total_voices)
    {
      guint i, ototal = allocator->n_total_voices;
      BseVoice *block = g_new0 (BseVoice, allocator->n_fixed_voices);

      allocator->n_total_voices += allocator->n_fixed_voices;
      allocator->voice_blocks = g_slist_prepend (allocator->voice_blocks, block);
      allocator->voices = g_renew (BseVoice*,
				   allocator->voices,
				   allocator->n_total_voices);
      for (i = ototal; i < allocator->n_total_voices; i++)
	{
	  voice = block++;
	  bse_voice_reset (voice);
	  voice->allocator = allocator;
	  voice->index = i;
	  allocator->voices[i] = voice;
	}
    }

  voice = allocator->voices[allocator->n_voices];
  allocator->voices[index]->index = allocator->n_voices;
  allocator->voices[allocator->n_voices] = allocator->voices[index];
  allocator->voices[index] = voice;
  voice->index = index;

  allocator->n_voices++;

  return voice;
}

BseVoice*
bse_voice_make_poly_and_renew (BseVoice *voice)
{
  g_return_val_if_fail (voice != NULL, NULL);
  g_return_val_if_fail (voice->index < voice->allocator->n_fixed_voices, NULL);

  if (!voice->active)
    return voice;

  return bse_voice_allocator_renew_voice (voice->allocator, voice->index);
}

void
bse_voice_reset (BseVoice *voice)
{
  g_return_if_fail (voice != NULL);

  if (voice->active)
    {
      voice->active = FALSE;
      voice->fading = FALSE;
      voice->polyphony = FALSE;
      
      voice->volume_factor = 1;
      voice->balance = 0;
      voice->transpose = 0;
      voice->fine_tune = 0;
      memset (&voice->env, 0, sizeof (voice->env));
      
      voice->note = BSE_KAMMER_NOTE;
      
      voice->n_tracks = 0;
      voice->freq_factor = 0;
      
      voice->rec_note = 0;
      voice->sample_pos = 0;
      voice->sample_end_pos = 0;
      voice->sample_pos_frac = 0;
      
      voice->sample_base_rate = 0;
      voice->sample_rate = 0;
      
      voice->env_part = BSE_ENVELOPE_PART_END;
      voice->env_steps_to_go = 0;
      voice->env_vol_delta = 0;
      voice->env_volume_factor = 1;

      voice->left_volume = 0;
      voice->left_volume_delta = 0;
      voice->right_volume = 0;
      voice->right_volume_delta = 0;
      
      if (voice->sample)
	bse_object_unlock (voice->sample);
      voice->sample = NULL;
      if (voice->bin_data)
	bse_object_unlock (voice->bin_data);
      voice->bin_data = NULL;

      bse_voice_allocator_check_free_voice (voice);
    }
}

void
bse_voice_activate (BseVoice      *voice,
		    BseInstrument *instrument)
{
  BseSample *sample;

  g_return_if_fail (voice != NULL);
  g_return_if_fail (voice->active == FALSE);
  g_return_if_fail (instrument != NULL);
  g_return_if_fail (instrument->sample != NULL);

  sample = instrument->sample;

  /* set and lock sample
   */
  voice->active = TRUE;
  voice->sample = sample;
  bse_object_lock (voice->sample);
  voice->n_tracks = sample->n_tracks;
  voice->freq_factor = ((gfloat) sample->rec_freq) / ((gfloat) BSE_MIX_FREQ);

  /* set instrument
   */
  voice->polyphony = instrument->polyphony;
  voice->volume_factor = instrument->volume_factor;
  voice->balance = instrument->balance;
  voice->transpose = instrument->transpose;
  voice->fine_tune = instrument->fine_tune;
  memcpy (&voice->env, &instrument->env, sizeof (voice->env));
}

/* adjust sample rate to a new fine_tune
 */
void
bse_voice_set_fine_tune (BseVoice *voice,
			 gint      fine_tune)
{
  g_return_if_fail (voice != NULL);

  if (!voice->active)
    return;

  fine_tune = CLAMP (fine_tune, BSE_MIN_FINE_TUNE, BSE_MAX_FINE_TUNE);

  voice->fine_tune = fine_tune;
  voice->sample_rate = 0.5 + BSE_FINE_TUNE_FACTOR (voice->fine_tune) * voice->sample_base_rate;
}

/* grab the appropriate sample munk and adjust sample rates
 */
void
bse_voice_set_note (BseVoice *voice,
		    guint     note,
		    gint      fine_tune)
{
  BseMunk *munk;
  BseSample *sample;
  gint note_rate;

  g_return_if_fail (voice != NULL);
  g_return_if_fail (note >= BSE_MIN_NOTE && note <= BSE_MAX_NOTE);
  g_return_if_fail (voice->active == TRUE);
  g_return_if_fail (voice->sample != NULL);

  /* first update the munk, and adjust bin_data reference
   */
  sample = voice->sample;
  munk = &sample->munks[note];
  if (voice->bin_data != munk->bin_data)
    {
      if (voice->bin_data)
	bse_object_unlock (voice->bin_data);
      voice->bin_data = munk->bin_data;
      bse_object_lock (voice->bin_data);

      voice->rec_note = munk->rec_note;
      voice->sample_pos = (BseSampleValue*) munk->bin_data->values;
      voice->sample_end_pos = voice->sample_pos + munk->bin_data->n_values;
      voice->sample_pos_frac = 0;
    }

  /* now calculate the appropriate sample rate for this note
   */
  voice->note = note;
  note_rate = note + voice->transpose;
  note_rate += BSE_KAMMER_NOTE - voice->rec_note;
  note_rate = BSE_HALFTONE_FACTOR_FIXED (note_rate);
  voice->sample_base_rate = voice->freq_factor * note_rate;

  bse_voice_set_fine_tune (voice, fine_tune);
}

/* setzt die position der envelope auf attack decay usw.
 * initialisiert alle noetigen variablen fuer die envelope und
 * errechnet die anzahl der steps etc
 * die time fuer den envelopepart wird unabhaenig von der buffer
 * groesse ermittelt
 */
void
bse_voice_set_envelope_part (BseVoice           *voice,
			     BseEnvelopePartType env_part)
{
  BseEnvelope *env;
  gfloat start_volume = 0;
  gfloat end_volume   = 0;
  gdouble time = 0;
  
  g_return_if_fail (voice != NULL);
  g_return_if_fail (voice->active == TRUE);

  env = &voice->env;

 recalc:

  voice->env_part = env_part;

  switch (voice->env_part)
    {
    case BSE_ENVELOPE_PART_DELAY:
      time = env->delay_time;
      start_volume = 0;
      end_volume = 0;
      break;
    case BSE_ENVELOPE_PART_ATTACK: 
      time = env->attack_time;
      start_volume = 0;
      end_volume = env->attack_level;
      break;
    case BSE_ENVELOPE_PART_DECAY: 
      time = env->decay_time;
      start_volume = env->attack_level;
      end_volume = env->sustain_level;
      break;
    case BSE_ENVELOPE_PART_SUSTAIN: 
      time = env->sustain_time;
      start_volume = env->sustain_level;
      end_volume = env->release_level;
      break;
    case BSE_ENVELOPE_PART_RELEASE: 
      time = env->release_time;
      start_volume = env->release_level;
      end_volume = 0;
      break;
    default:
      voice->env_part = BSE_ENVELOPE_PART_END;
      return;
    }
  
  // printf ("start %f end %f\n", start_volume, end_volume);

  /* calculate steps to go, this is the number of
   * buffers that need to be filled during time[ms]
   */
  voice->env_steps_to_go = 0.5 + time * BSE_MIX_FREQ / (BSE_TRACK_LENGTH * 1000.0);
  
  if (voice->env_steps_to_go == 0)
    {
      env_part++;
      goto recalc;
    }
  
  // printf ("steps %u \n", voice->env_steps_to_go);
  
  voice->env_vol_delta = (end_volume - start_volume) / (gdouble) voice->env_steps_to_go;
  voice->env_volume_factor = start_volume;
}

void
bse_voice_fade_out (BseVoice *voice)
{
  guint time;
  guint n_values;
  guint n_sample_values;

  g_return_if_fail (voice != NULL);
  g_return_if_fail (voice->active == TRUE);

  voice->fading = TRUE;

  /* adjust time to a nominative volume factor of 1.0 */
  time = MAX (voice->left_volume, voice->right_volume) * BSE_FADE_OUT_TIME_ms;

  /* number of buffer values we need to fade */
  n_values = BSE_MIX_FREQ * time / 1000.0;

  /* stop playing after fading is done */
  n_sample_values = voice->sample_pos_frac + voice->sample_rate * n_values;
  n_sample_values >>= 16;
  voice->sample_end_pos = MIN (voice->sample_end_pos,
			       voice->sample_pos + n_sample_values + voice->n_tracks);

  /* calculate volume adjustment */
  if (n_values)
    {
      voice->left_volume_delta = - voice->left_volume / n_values;
      voice->right_volume_delta = - voice->right_volume / n_values;
      // printf ("deltas: %9f/%d=%9f %9f/%d=%9f\n", voice->left_volume, n_values, voice->left_volume_delta, voice->right_volume, n_values, voice->right_volume_delta);
    }
  else
    {
      voice->left_volume_delta = 0;
      voice->right_volume_delta = 0;
    }
}

void
bse_voice_preprocess (BseVoice *voice)
{
  gfloat l_volume, r_volume;

  g_return_if_fail (voice != NULL);
  g_return_if_fail (voice->active == TRUE);

  if (voice->fading)
    {
      /* don't screw the fading process */
      return;
    }

  /* stereo position */
  if (voice->balance < 0)
    {
      l_volume = voice->volume_factor;
      r_volume = voice->volume_factor * (BSE_MAX_BALANCE + voice->balance) / BSE_MAX_BALANCE;
    }
  else
    {
      l_volume = voice->volume_factor * (BSE_MAX_BALANCE - voice->balance) / BSE_MAX_BALANCE;
      r_volume = voice->volume_factor;
    }

  /* adjust to envelope */
  l_volume *= voice->env_volume_factor;
  r_volume *= voice->env_volume_factor;

  voice->left_volume = l_volume;
  voice->right_volume = r_volume;
}

void
bse_voice_postprocess (BseVoice *voice)
{
  g_return_if_fail (voice != NULL);
  g_return_if_fail (voice->active == TRUE);

  /* check sample position
   */
  if (voice->sample_pos >= voice->sample_end_pos)
    {
      bse_voice_reset (voice);
      return;
    }

  if (voice->fading)
    {
      /* don't screw the fading process */
      return;
    }

  /* step envelope
   */
  if (voice->env_steps_to_go)
    {
      voice->env_steps_to_go--;
      voice->env_volume_factor += voice->env_vol_delta;
    }
  else
    {
      bse_voice_set_envelope_part (voice, voice->env_part + 1);
      if (voice->env_part == BSE_ENVELOPE_PART_END)
	{
	  bse_voice_reset (voice);
	  return;
	}
    }
}
