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
 */
#ifndef __BSE_VOICE_H__
#define __BSE_VOICE_H__

#include        <bse/bseinstrument.h>
#include        <bse/bsesinstrument.h>
#include        <bse/bsesample.h>
#include        <bse/bsebindata.h>
#include        <bse/bsepattern.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- internal enums --- */
typedef enum			/*< skip >*/
{
  BSE_ENVELOPE_PART_DELAY,
  BSE_ENVELOPE_PART_ATTACK,
  BSE_ENVELOPE_PART_DECAY,
  BSE_ENVELOPE_PART_SUSTAIN,
  BSE_ENVELOPE_PART_RELEASE,
  BSE_ENVELOPE_PART_DONE
} BseEnvelopePartType;


/* --- Voice allocator --- */
struct _BseSongChannel
{
  guint		  n_poly_voices;
  BseVoice	**voices;		/* n_voices here is 1 + n_poly_voices */
};
struct _BseVoiceAllocator
{
  /*< private >*/
  GTrashStack	 *free_voices;
  GSList	 *voice_blocks;

  /*< public >*/
  guint		  n_voices;		/* from BseSong.n_channels */
  BseVoice	 *voices[1];		/* flexible array of size n_channels */
};


typedef enum
{
  BSE_VOICE_INPUT_NONE,
  BSE_VOICE_INPUT_SAMPLE,
  BSE_VOICE_INPUT_SYNTH,
  BSE_VOICE_INPUT_FADE_RAMP
} BseVoiceType;

/* --- BseVoice --- */
struct _BseVoice
{
  /* allocation maintainance fields */
  BseVoiceAllocator	*allocator;	/* const */
  guint			 index;		/* const */
  BseVoice		*next;		/* pointer to next poly */

  /* flags */
  BseVoiceType	  input_type : 8;
  guint		  fading : 1;
  guint		  make_poly : 1;

  /* from BseInstrument
   */
  gfloat	  volume_factor;
  gint		  balance;
  gint		  transpose;
  gint		  fine_tune;
  BseEnvelope	  env;

  /* from BsePatternNote
   */
  gint		  note;

  guint		  n_tracks;

  /* Envelope Generator */
  BseEnvelopePartType env_part;		 /* current part of envelope */
  guint		      env_steps_to_go;   /* # chunks this part lasts */
  gfloat	      env_vol_delta;	 /* volume delta per sequencer step */
  gfloat	      env_volume_factor; /* resulting envelope volume */

  /* mixer fields, volume factors while playback and sample fading
   * concrete sample values for fade ramps
   */
  gfloat	      left_volume;
  gfloat	      right_volume;

  /* fader deltas */
  gfloat	      left_volume_delta;
  gfloat	      right_volume_delta;

  union {
    struct {
      /* from BseSample, freq_factor = sample->rec_freq / BSE_MIX_FREQ */
      gfloat	      freq_factor;
      BseSampleValue *bound;

      guint32	      base_rate;	/* base readout rate from note (<<16) */
      guint32	      rate;		/* fine tune adjusted base rate */
      
      /* sample mixer fields, runtime-adjusted */
      BseSampleValue *cur_pos;
      guint	      pos_frac;

      /* object references for locking */
      BseSample*      sample;
      BseBinData*     bin_data;
    } sample;
    struct {
      gfloat	      base_freq;	/* base playback freq from note */
      gfloat	      freq;		/* fine tune adjusted base freq */

      BseIndex	      last_index;
      BseSInstrument* sinstrument;
    } synth;
    struct {
      gint            n_values_left;	/* # values left while fading */
    } fade_ramp;
  } input;
};


/* --- prototypes (safely to be used from effects) --- */
void		bse_voice_fade_out		(BseVoice	*voice);
void		bse_voice_set_fine_tune		(BseVoice	*voice,
						 gint     	 fine_tune);
void		bse_voice_set_envelope_part	(BseVoice       *voice,
						 BseEnvelopePartType env_part);
void		bse_voice_set_note		(BseVoice	*voice,
						 gint		 note,
						 gint            fine_tune);


/* --- private --- */
BseVoice*	bse_voice_make_poly_and_renew	(BseVoice	*voice);
void		bse_voice_activate		(BseVoice	*voice,
						 BseInstrument	*instrument,
						 gint		 note,
						 gint            fine_tune);
gboolean	bse_voice_preprocess		(BseVoice	*voice);
gboolean	bse_voice_postprocess		(BseVoice	*voice);


/* --- allocation --- */
BseVoiceAllocator* bse_voice_allocator_new	(guint		    n_voices);
void		   bse_voice_allocator_destroy	(BseVoiceAllocator *allocator);





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_VOICE_H__ */
