/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2001 Olaf Hoehmann and Tim Janik
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

#include        <bse/bsebuffermixer.h>
#include        <bse/bseinstrument.h>
#include        <bse/bsesample.h>
#include        <bse/bsebindata.h>
#include        <bse/bsepattern.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- Voice allocator --- */
struct _BseVoiceAllocator
{
  /*< private >*/
  GTrashStack	 *free_voices;
  GSList	 *voice_blocks;

  /*< public >*/
  guint		  n_voices;		/* from BseSong.n_channels */
  BseVoice	 *voices[1];		/* flexible array of size n_channels */
};


/* --- enums --- */
typedef enum			/*< skip >*/
{
  BSE_VOICE_INPUT_NONE,
  BSE_VOICE_INPUT_SAMPLE,
  BSE_VOICE_INPUT_SYNTH
} BseVoiceType;
typedef enum			/*< skip >*/
{
  BSE_ENVELOPE_PART_DELAY,
  BSE_ENVELOPE_PART_ATTACK,
  BSE_ENVELOPE_PART_DECAY,
  BSE_ENVELOPE_PART_SUSTAIN,
  BSE_ENVELOPE_PART_RELEASE,
  BSE_ENVELOPE_PART_DONE
} BseEnvelopePartType;

/* --- BseVoice --- */
struct _BseVoice
{
  /* allocation maintainance fields */
  BseVoiceAllocator	*allocator;	/* const */
  guint			 index;		/* const */
  BseVoice		*next;		/* pointer to next poly */

  /* flags */
  BseVoiceType	  input_type : 8;
  guint		  make_poly : 1;
  guint		  started : 1;

  /* fader setup */
  guint		  fader_pending : 1;
  guint		  is_fading : 1;

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

  /* Envelope Generator */
  BseEnvelopePartType env_part;		 /* current part of envelope */
  guint		      env_steps_to_go;   /* # chunks this part lasts */
  gfloat	      env_vol_delta;	 /* volume delta per sequencer step */
  gfloat	      env_volume_factor; /* resulting envelope volume */

  /* mixer source
   */
  BseMixSource source;

  /* mixer volume fields, volume factors for playback
   * and fader deltas for sample fading
   */
  BseMixVolume volume;

  union {
    struct {
      /* object reference for locking */
      BseSample*      sample;

      gfloat	      freq_factor;	/* sample->rec_freq / BSE_MIX_FREQ */
      gint	      base_rate;	/* base readout rate from note (<<16) */

      BseMixRate      rate;
    } sample;
    struct {
      BseSInstrument* sinstrument;

      gfloat	      base_freq;	/* base playback freq from note */
      gfloat	      freq;		/* fine tune adjusted base freq */
    } synth;
  } input;
};


/* --- prototypes (safely to be used from effects) --- */
void		_bse_voice_fade_out		(BseVoice	*voice);
void		_bse_voice_set_fine_tune	(BseVoice	*voice,
						 gint     	 fine_tune);
void		_bse_voice_set_envelope_part	(BseVoice       *voice,
						 BseEnvelopePartType env_part);
void		_bse_voice_set_volume		(BseVoice	*voice,
						 gfloat          volume_factor);
void		_bse_voice_set_balance		(BseVoice	*voice,
						 gint		 balance);


/* --- private --- */
BseVoice*	_bse_voice_make_poly_and_renew	(BseVoice	*voice);
void		_bse_voice_activate		(BseVoice	*voice,
						 BseInstrument	*instrument,
						 gint		 note);
void		_bse_voice_set_note		(BseVoice	*voice,
						 gint		 note);
void		_bse_voice_fade_out_until	(BseVoice	*voice,
						 guint           n_values);
gboolean	_bse_voice_need_after_fade	(BseVoice	*voice);
gboolean	_bse_voice_preprocess		(BseVoice	*voice);
gboolean	_bse_voice_postprocess		(BseVoice	*voice);


/* --- allocation --- */
BseVoiceAllocator* _bse_voice_allocator_new	(guint		    n_voices);
void		   _bse_voice_allocator_destroy	(BseVoiceAllocator *allocator);





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_VOICE_H__ */
