/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BSE_SONG_H__
#define __BSE_SONG_H__

#include        <bse/bsesnet.h>


G_BEGIN_DECLS


/* --- BSE type macros --- */
#define BSE_TYPE_SONG              (BSE_TYPE_ID (BseSong))
#define BSE_SONG(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SONG, BseSong))
#define BSE_SONG_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SONG, BseSongClass))
#define BSE_IS_SONG(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SONG))
#define BSE_IS_SONG_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SONG))
#define BSE_SONG_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SONG, BseSongClass))


/* --- BseSong object --- */
typedef struct {
  BseSource *constant;
  BseSource *sub_synth;
} BseSongVoice;
struct _BseSong
{
  BseSNet           parent_instance;
  
  guint		    tpqn;		/* ticks per querter note */
  guint		    numerator;
  guint		    denominator;
  gfloat            bpm;
  gfloat            volume_factor;      /* 1-based factor */
  
  GList            *parts;              /* of type BsePart* */

  BseSource	   *context_merger;
  BseSource	   *postprocess;
  BseSource	   *output;

  BseSNet         *pnet;

  /* song position pointer */
  SfiInt	    last_position;
  guint		    position_handler;

  BseMidiReceiver  *midi_receiver_SL;

  /* fields protected by sequencer mutex */
  gdouble	    tpsi_SL;		/* ticks per stamp increment (sample) */
  SfiRing	   *tracks_SL;		/* of type BseTrack* */
  /* sequencer stuff */
  SfiTime	    start_SL;		/* playback start */
  gdouble	    delta_stamp_SL;	/* start + delta_stamp => tick */
  guint		    tick_SL;		/* tick at stamp_SL */
  guint		    song_done_SL : 1;
  guint		    loop_enabled_SL : 1;
  SfiInt	    loop_left_SL;	/* left loop tick */
  SfiInt	    loop_right_SL;	/* left loop tick */
};
struct _BseSongClass
{
  BseSNetClass parent_class;
};


/* --- prototypes --- */
BseSong*	bse_song_lookup			(BseProject	*project,
						 const gchar	*name);
void		bse_song_stop_sequencing_SL	(BseSong	*self);
void		bse_song_get_timing		(BseSong	*self,
						 guint		 tick,
						 BseSongTiming	*timing);
void		bse_song_timing_get_default	(BseSongTiming	*timing);


G_END_DECLS

#endif /* __BSE_SONG_H__ */
