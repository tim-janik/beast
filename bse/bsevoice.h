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
 *
 */
#ifndef __BSE_VOICE_H__
#define __BSE_VOICE_H__

#include        <bse/bseinstrument.h>
#include        <bse/bsesample.h>
#include        <bse/bsebindata.h>
#include        <bse/bsepattern.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- internal enums --- */
typedef enum			/* <skip> */
{
  BSE_ENVELOPE_PART_DELAY,
  BSE_ENVELOPE_PART_ATTACK,
  BSE_ENVELOPE_PART_DECAY,
  BSE_ENVELOPE_PART_SUSTAIN,
  BSE_ENVELOPE_PART_RELEASE,
  BSE_ENVELOPE_PART_END
} BseEnvelopePartType;


/* --- Voice allocator --- */
struct _BseVoiceAllocator
{
  guint		  n_voices;		/* fixed + poly */
  BseVoice	**voices;		/* relocatable, mutatable */
  guint		  next_voice;		/* loop compensator */

  /* private */
  guint		  n_fixed_voices;	/* const */
  guint		  n_total_voices;
  GSList	 *voice_blocks;
};


/* --- BseVoice --- */
struct _BseVoice
{
  /* allocation maintainance fields */
  BseVoiceAllocator	*allocator;	/* const */
  guint			 index : 24;	/* const */

  /* flags */
  guint		  active : 1;
  guint		  fading : 1;
  guint		  polyphony : 1;

  /* from BseInstrument
   */
  gfloat	  volume_factor;
  gint		  balance;
  gint		  transpose;
  gint		  fine_tune;
  BseEnvelope	  env;

  /* from BseNote
   */
  guint		  note;

  /* from BseSample and its munk
   * freq_factor	- sample->rec_freq / BSE_MIX_FREQ
   */
  guint		  n_tracks;
  gfloat	  freq_factor;

  /* from BseMunk and its bin_data
   */
  guint		  rec_note;
  BseSampleValue *sample_pos;
  BseSampleValue *sample_end_pos;
  guint		  sample_pos_frac;

  /* private sample mixer fields
   */
  guint32	  sample_base_rate;	/* sample rate for indexing */
  guint32	  sample_rate;		/* sample rate with fine tune */

  /* Envelope Generator
   *
   * env_part		- current part of envelope
   * env_steps_to_go	- number of buffers that this part lasts
   * env_vol_delta	- volume change per buffer step
   * env_volume_factor	- resulting env volume, adjusted by env_vol_delta
   */
  BseEnvelopePartType env_part;
  guint		      env_steps_to_go;
  gfloat	      env_vol_delta;
  gfloat	      env_volume_factor;

  /* preprocessed values, used by the buffer mixing routines
   */
  gfloat	      left_volume, left_volume_delta;
  gfloat	      right_volume, right_volume_delta;
  
  /* object references for locking
   */
  gpointer	 sample;
  gpointer	 bin_data;
};


/* --- prototypes --- */
void		bse_voice_activate		(BseVoice	*voice,
						 BseInstrument	*instrument);
void		bse_voice_reset			(BseVoice	*voice);
BseVoice*	bse_voice_make_poly_and_renew	(BseVoice	*voice);
void		bse_voice_fade_out		(BseVoice	*voice);
void		bse_voice_new_note		(BseVoice	*voice,
						 BseNote	*note);
void		bse_voice_set_note		(BseVoice	*voice,
						 guint		 note,
						 gint            fine_tune);
void		bse_voice_set_fine_tune		(BseVoice	*voice,
						 gint     	 fine_tune);
void		bse_voice_set_envelope_part	(BseVoice       *voice,
						 BseEnvelopePartType env_part);
void		bse_voice_preprocess		(BseVoice	*voice);
void		bse_voice_postprocess		(BseVoice	*voice);
     

/* --- allocation --- */
BseVoiceAllocator* bse_voice_allocator_new	(guint		    n_fixed_voices);
void		   bse_voice_allocator_destroy	(BseVoiceAllocator *allocator);





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_VOICE_H__ */
