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
#include	"bsevoice.h"
#include	"bsechunk.h"


/* --- prototypes --- */
static void		bse_voice_reset			(BseVoice	*voice);


/* --- functions --- */
static inline void
bse_voice_meminit (BseVoice          *voice,
		   BseVoiceAllocator *allocator,
		   guint              index)
{
  voice->allocator = allocator;
  voice->index = index;
  voice->next = NULL;

  voice->input_type = BSE_VOICE_INPUT_NONE;
  voice->make_poly = FALSE;

  voice->fader_pending = FALSE;
  voice->is_fading = FALSE;
  
  voice->volume_factor = 1;
  voice->balance = 0;
  voice->transpose = 0;
  voice->fine_tune = 0;
  memset (&voice->env, 0, sizeof (voice->env));
  
  voice->note = BSE_KAMMER_NOTE;
  
  voice->env_part = BSE_ENVELOPE_PART_DONE;
  voice->env_steps_to_go = 0;
  voice->env_vol_delta = 0;
  voice->env_volume_factor = 1.0;
  
  memset (&voice->volume, 0, sizeof (voice->volume));
  memset (&voice->source, 0, sizeof (voice->source));

  memset (&voice->input, 0, sizeof (voice->input));
}

static void
bse_voice_reset (BseVoice *voice)
{
  BseVoiceAllocator *allocator;
  guint index;

  g_return_if_fail (voice != NULL);

  allocator = voice->allocator;
  index = voice->index;
  
  switch (voice->input_type)
    {
    case BSE_VOICE_INPUT_NONE:
      break;
    case BSE_VOICE_INPUT_SAMPLE:
      /* free object links */
      if (voice->input.sample.sample)
	bse_object_unlock (BSE_OBJECT (voice->input.sample.sample));
      if (voice->input.sample.bin_data)
	bse_object_unlock (BSE_OBJECT (voice->input.sample.bin_data));
      break;
    case BSE_VOICE_INPUT_SYNTH:
      /* free object links */
      if (voice->input.synth.sinstrument)
	{
	  bse_sinstrument_poke_foreigns (voice->input.synth.sinstrument,
					 voice->input.synth.sinstrument->instrument,
					 NULL);
	  bse_object_unlock (BSE_OBJECT (voice->input.synth.sinstrument));
	}
      break;
    default:
      g_assert_not_reached ();
      break;
    }
  voice->input_type = BSE_VOICE_INPUT_NONE;
      
  /* simply reset memory if this is a fixed voice (but save poly) */
  if (allocator->voices[index] == voice)
    {
      BseVoice *next = voice->next;

      bse_voice_meminit (voice, voice->allocator, voice->index);
      voice->next = next;
    }
  else /* ok this is a poly voice, need to unlink and free it */
    {
      BseVoice *cur, *last = allocator->voices[index];

      do
	{
	  cur = last->next;
	  if (cur == voice)
	    {
	      last->next = cur->next;
	      break;
	    }
	  last = cur;
	}
      while (cur);
      if (!cur)
	g_error (G_STRLOC ": unable to find and free poly voice (index=%u)", index);

      g_trash_stack_push (&allocator->free_voices, voice);
    }
}

BseVoiceAllocator*
bse_voice_allocator_new (guint n_voices)
{
  BseVoiceAllocator *allocator;
  BseVoice *block;
  guint i, n_prealloc = n_voices * 1; // FIXME: s/1/2/ debug code

  g_return_val_if_fail (n_voices > 0, NULL);

  allocator = g_malloc (sizeof (BseVoiceAllocator) + (n_voices - 1) * sizeof (BseVoice*));
  allocator->free_voices = NULL;
  allocator->voice_blocks = NULL;
  allocator->n_voices = n_voices;

  block = g_new (BseVoice, n_prealloc);
  allocator->voice_blocks = g_slist_prepend (allocator->voice_blocks, block);
  for (i = 0; i < n_voices; i++)
    {
      bse_voice_meminit (block, allocator, i);
      allocator->voices[i] = block++;
    }
  for (; i < n_prealloc; i++)
    {
      block->input_type = BSE_VOICE_INPUT_NONE;
      g_trash_stack_push (&allocator->free_voices, block++);
    }

  return allocator;
}

static inline BseVoice*
bse_voice_new (BseVoiceAllocator *allocator,
	       guint              index)
{
  BseVoice *voice = g_trash_stack_pop (&allocator->free_voices);

  if (!voice)
    {
      BseVoice *block;
      guint i;

      block = g_new (BseVoice, allocator->n_voices);
      allocator->voice_blocks = g_slist_prepend (allocator->voice_blocks, block);
      voice = block++;
      for (i = 1; i < allocator->n_voices; i++)
	{
	  block->input_type = BSE_VOICE_INPUT_NONE;
	  g_trash_stack_push (&allocator->free_voices, block++);
	}
    }

  bse_voice_meminit (voice, allocator, index);

  return voice;
}

void
bse_voice_allocator_destroy (BseVoiceAllocator *allocator)
{
  GSList *slist;
  guint i;

  g_return_if_fail (allocator != NULL);

  for (i = 0; i < allocator->n_voices; i++)
    {
      BseVoice *voice = allocator->voices[i];

      if (voice->input_type != BSE_VOICE_INPUT_NONE)
	bse_voice_reset (voice);
      while (voice->next)
	bse_voice_reset (voice->next);
    }

  for (slist = allocator->voice_blocks; slist; slist = slist->next)
    g_free (slist->data);
  g_slist_free (allocator->voice_blocks);

  g_free (allocator);
}

/* never call this from effects */
BseVoice*
bse_voice_make_poly_and_renew (BseVoice *voice)
{
  BseVoiceAllocator *allocator;
  guint index;

  g_return_val_if_fail (voice != NULL, NULL);
  g_return_val_if_fail (voice->index < voice->allocator->n_voices, NULL);

  allocator = voice->allocator;
  index = voice->index;

  /* only need to handle fixed voices */
  g_return_val_if_fail (voice == allocator->voices[index], NULL);

  if (voice->input_type != BSE_VOICE_INPUT_NONE)
    {
      allocator->voices[index] = bse_voice_new (allocator, index);
      allocator->voices[index]->next = voice;
      voice = allocator->voices[index];
    }

  return voice;
}

void
bse_voice_activate (BseVoice      *voice,
		    BseInstrument *instrument,
		    gint           note,
		    gint           fine_tune)
{
  g_return_if_fail (voice != NULL);
  g_return_if_fail (voice->input_type == BSE_VOICE_INPUT_NONE);
  g_return_if_fail (BSE_IS_INSTRUMENT (instrument));
  g_return_if_fail (instrument->type != BSE_INSTRUMENT_NONE);

  /* set instrument
   */
  voice->make_poly = FALSE;
  voice->fader_pending = FALSE;
  voice->is_fading = FALSE;
  voice->volume_factor = instrument->volume_factor;
  voice->balance = instrument->balance;
  voice->transpose = instrument->transpose;
  voice->fine_tune = instrument->fine_tune;
  memcpy (&voice->env, &instrument->env, sizeof (voice->env));
  
  switch (instrument->type)
    {
      BseSample *sample;
      BseSInstrument *sinstrument;

    case BSE_INSTRUMENT_SAMPLE:
      sample = BSE_SAMPLE (instrument->input);
      voice->input_type = BSE_VOICE_INPUT_SAMPLE;
      voice->make_poly = instrument->polyphony;
      /* set and lock sample */
      voice->input.sample.sample = sample;
      bse_object_lock (BSE_OBJECT (voice->input.sample.sample));
      voice->input.sample.freq_factor = ((gfloat) sample->rec_freq) / BSE_MIX_FREQ_f;
      voice->input.sample.rate.interpolation = instrument->interpolation;
      voice->source.n_tracks = sample->n_tracks;
      voice->source.loop_count = 0;
      voice->source.loop_start = NULL;
      voice->source.loop_bound = NULL;
      voice->source.run_limit = FALSE;
      voice->source.max_run_values = 0;
      break;
    case BSE_INSTRUMENT_SYNTH:
      sinstrument = BSE_SINSTRUMENT (instrument->input);
      g_return_if_fail (BSE_SOURCE_N_OCHANNELS (sinstrument) >= BSE_DFL_OCHANNEL_ID);
      g_assert (BSE_SOURCE_OCHANNEL_DEF (sinstrument, BSE_DFL_OCHANNEL_ID)->n_tracks <= 2);
      voice->input_type = BSE_VOICE_INPUT_SYNTH;
      voice->input.synth.sinstrument = sinstrument;
      bse_object_lock (BSE_OBJECT (voice->input.synth.sinstrument));
      bse_sinstrument_poke_foreigns (voice->input.synth.sinstrument,
				     voice->input.synth.sinstrument->instrument,
				     voice);
      voice->source.n_tracks = BSE_SOURCE_OCHANNEL_DEF (sinstrument, BSE_DFL_OCHANNEL_ID)->n_tracks;
      voice->source.loop_count = 0;
      voice->source.loop_start = NULL;
      voice->source.loop_bound = NULL;
      voice->source.run_limit = FALSE;
      voice->source.max_run_values = 0;
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  bse_voice_set_note (voice, note, fine_tune);
  bse_voice_set_envelope_part (voice, BSE_ENVELOPE_PART_DELAY);
}

void
bse_voice_set_note (BseVoice *voice,
		    gint      note,
		    gint      fine_tune)
{
  g_return_if_fail (voice != NULL);
  g_return_if_fail (note >= BSE_MIN_NOTE && note <= BSE_MAX_NOTE);

  /* don't screw the fading process */
  if (voice->is_fading)
    return;

  switch (voice->input_type)
    {
      BseSample *sample;
      BseMunk *munk;
      gint note_rate;

    case BSE_VOICE_INPUT_SAMPLE:	/* fetch corresponding sample munk, adjust sample rates */
      sample = voice->input.sample.sample;
      munk = &sample->munks[note];
      /* adjust object references */
      if (voice->input.sample.bin_data != munk->bin_data)
	{
	  if (voice->input.sample.bin_data)
	    bse_object_unlock (BSE_OBJECT (voice->input.sample.bin_data));
	  voice->input.sample.bin_data = munk->bin_data;
	  bse_object_lock (BSE_OBJECT (voice->input.sample.bin_data));

	  voice->source.cur_pos = (BseSampleValue*) munk->bin_data->values;
	  voice->source.bound = voice->source.cur_pos + munk->bin_data->n_values;
	  voice->input.sample.rate.frac = 0;
	  voice->input.sample.rate.step = 0;
	  voice->input.sample.rate.delta = 0;
	}
      /* calc new sample rate according to note */
      voice->note = note;
      note_rate = note + voice->transpose;
      note_rate += BSE_KAMMER_NOTE - munk->rec_note;
      note_rate = BSE_HALFTONE_FACTOR_FIXED (note_rate);
      voice->input.sample.base_rate = voice->input.sample.freq_factor * note_rate;
      break;
    case BSE_VOICE_INPUT_SYNTH:
      voice->input.synth.base_freq = bse_note_to_freq (note) + voice->transpose;
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  bse_voice_set_fine_tune (voice, fine_tune);
}

void
bse_voice_set_fine_tune (BseVoice *voice,
			 gint      fine_tune)
{
  g_return_if_fail (voice != NULL);

  /* don't screw the fading process */
  if (voice->is_fading)
    return;

  fine_tune = CLAMP (fine_tune, BSE_MIN_FINE_TUNE, BSE_MAX_FINE_TUNE);
  voice->fine_tune = fine_tune;

  switch (voice->input_type)
    {
    case BSE_VOICE_INPUT_SAMPLE:	/* adjust sample rate to a new fine_tune */
      voice->input.sample.rate.step = 0.5 + BSE_FINE_TUNE_FACTOR (voice->fine_tune) * voice->input.sample.base_rate;
      break;
    case BSE_VOICE_INPUT_SYNTH:		/* adjust playback freq to a new fine_tune */
      voice->input.synth.freq = BSE_FINE_TUNE_FACTOR (voice->fine_tune) * voice->input.synth.base_freq;
      break;
    default:
      break;
    }
}

void
bse_voice_set_envelope_part (BseVoice           *voice,
			     BseEnvelopePartType env_part)
{
  BseEnvelope *env;
  gfloat start_volume = 0;
  gfloat end_volume   = 0;
  gdouble time = 0;
  
  g_return_if_fail (voice != NULL);
  g_return_if_fail (voice->input_type != BSE_VOICE_INPUT_NONE);
  
  /* don't screw the fading process */
  if (voice->is_fading)
    return;

  /* set envelope position to attack, decay, etc...
   * initialise all envelope variables, calculate amount
   * of envelope steps
   * the time slices for the envelope part are figured
   * regardless of the buffer size
   */
  
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
      voice->env_part = BSE_ENVELOPE_PART_DONE;
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
bse_voice_fade_out_until (BseVoice *voice,
			  guint     n_values)
{
  guint n_fade_values;

  g_return_if_fail (voice != NULL);
  g_return_if_fail (voice->input_type != BSE_VOICE_INPUT_NONE);
  g_return_if_fail (n_values > 0);

  /* don't screw the fading process */
  if (voice->is_fading)
    return;
  g_return_if_fail (voice->source.run_limit == FALSE);

  n_fade_values = BSE_FADE_OUT_TIME_ms * BSE_MIX_FREQ_f / 1000.0;
  voice->fader_pending = TRUE;
  n_fade_values = MIN (n_values, n_fade_values);
  voice->source.run_limit = TRUE;
  voice->source.max_run_values = n_values - n_fade_values;
}

void
bse_voice_fade_out (BseVoice *voice)
{
  g_return_if_fail (voice != NULL);
  g_return_if_fail (voice->input_type != BSE_VOICE_INPUT_NONE);

  /* don't screw the fading process */
  if (voice->is_fading)
    return;

  voice->fader_pending = TRUE;
  voice->source.run_limit = TRUE;
  voice->source.max_run_values = 0;
  bse_voice_need_after_fade (voice);
}

gboolean /* return whether voice needs fading (always alive) */
bse_voice_need_after_fade (BseVoice *voice)
{
  g_return_val_if_fail (voice != NULL, FALSE);
  g_return_val_if_fail (voice->input_type != BSE_VOICE_INPUT_NONE, FALSE);

  if (voice->fader_pending && !voice->is_fading && !BSE_MIX_SOURCE_ACTIVE (&voice->source))
    {
      guint n_fade_values;

      voice->is_fading  = TRUE;
      n_fade_values = BSE_FADE_OUT_TIME_ms * BSE_MIX_FREQ_f / 1000.0;
      voice->source.run_limit = TRUE;
      voice->source.max_run_values = MIN (n_fade_values,
					  MAX (voice->volume.left, voice->volume.right) * n_fade_values);

      /* calculate volume deltas for fade ramp */
      if (voice->source.max_run_values)
	{
	  voice->volume.left_delta = - voice->volume.left / voice->source.max_run_values;
	  voice->volume.right_delta = - voice->volume.right / voice->source.max_run_values;

	  return BSE_MIX_SOURCE_ACTIVE (&voice->source);
	}
    }

  return FALSE;
}

gboolean /* return whether voice is still alive */
bse_voice_preprocess (BseVoice *voice)
{
  gfloat l_volume, r_volume;

  g_return_val_if_fail (voice != NULL, FALSE);
  g_return_val_if_fail (voice->input_type != BSE_VOICE_INPUT_NONE, FALSE);

  /* check whether we are done playing the voice or are currently fading */
  if (!BSE_MIX_SOURCE_ACTIVE (&voice->source))
    {
      bse_voice_reset (voice);
      return FALSE;
    }

  /* don't screw the fading process */
  if (voice->is_fading)
    return TRUE;

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

  voice->volume.left = l_volume;
  voice->volume.right = r_volume;

  return TRUE;
}

gboolean /* return whether voice is still alive */
bse_voice_postprocess (BseVoice *voice)
{
  g_return_val_if_fail (voice != NULL, FALSE);
  g_return_val_if_fail (voice->input_type != BSE_VOICE_INPUT_NONE, FALSE);

  /* check whether we are done playing the voice or are currently fading */
  if (!BSE_MIX_SOURCE_ACTIVE (&voice->source) &&
      !bse_voice_need_after_fade (voice))
    {
      bse_voice_reset (voice);
      return FALSE;
    }

  /* don't screw the fading process */
  if (voice->is_fading)
    return TRUE;

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
      if (voice->env_part == BSE_ENVELOPE_PART_DONE)
	{
	  bse_voice_reset (voice);
	  return FALSE;
	}
    }

  return TRUE;
}
