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
 * bsesong.h: bse song object
 */
#ifndef __BSE_SONG_H__
#define __BSE_SONG_H__

#include	<bse/bsesuper.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- BSE type macros --- */
#define BSE_TYPE_SONG		   (BSE_TYPE_ID (BseSong))
#define BSE_SONG(object)	   (BSE_CHECK_STRUCT_CAST ((object), BSE_TYPE_SONG, BseSong))
#define BSE_SONG_CLASS(class)	   (BSE_CHECK_CLASS_CAST ((class), BSE_TYPE_SONG, BseSongClass))
#define BSE_IS_SONG(object)	   (BSE_CHECK_STRUCT_TYPE ((object), BSE_TYPE_SONG))
#define BSE_IS_SONG_CLASS(class)   (BSE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SONG))
#define BSE_SONG_GET_CLASS(object) ((BseSongClass*) (((BseObject*) (object))->bse_struct.bse_class))


/* --- BseSong object --- */
struct _BseSong
{
  BseSuper parent_object;
  
  guint	bpm;
  gfloat volume_factor;		/* 1-based factor */
  
  guint	pattern_length		/* >= 4 by convention */;
  guint	n_channels		/* >= 2 by convention */;
  
  GList	*instruments		/* of type BseInstrument* */;
  GList *patterns		/* of type BsePattern* */;
  GList	*effect_processors	/* of type BseEffectProcessor* */;
  GList	*lfos			/* of type BseLfo* */;
  
  BseSongSequencer *sequencer;
  
  guint pattern_list_length;
  BsePattern **pattern_list;
};
struct _BseSongClass
{
  BseSuperClass parent_class;
};


/* --- ochannels --- */
enum {
  BSE_SONG_OCHANNEL_NONE,
  BSE_SONG_OCHANNEL_STEREO
};


/* --- prototypes --- */
BseSong*	bse_song_new			(BseProject	*project,
						 guint		 n_channels);
BseSong*	bse_song_lookup			(BseProject	*project,
						 const gchar	*name);
BsePattern*	bse_song_get_pattern		(BseSong	*song,
						 guint		 seqid);
void		bse_song_delete_pattern		(BseSong	*song,
						 BsePattern	*pattern);
BsePattern*	bse_song_get_pattern_from_list	(BseSong	*song,
						 guint		 pattern_index);
BseInstrument*	bse_song_add_instrument		(BseSong	*song);
BseInstrument*	bse_song_get_instrument		(BseSong	*song,
						 guint		 seqid);
void		bse_song_set_pattern_length	(BseSong	*song,
						 guint		 pattern_length);
void		bse_song_set_bpm		(BseSong	*song,
						 guint		 bpm);
#if 0
void	bse_song_reload_instrument_samples	(BseSong		*song,
						 BseSampleLookupCB	cb_func,
						 gpointer		cb_data);
#endif




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_SONG_H__ */
