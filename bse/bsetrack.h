/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2002 Tim Janik
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
#ifndef __BSE_TRACK_H__
#define __BSE_TRACK_H__

#include	<bse/bseitem.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- BSE type macros --- */
#define BSE_TYPE_TRACK		    (BSE_TYPE_ID (BseTrack))
#define BSE_TRACK(object)	    (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_TRACK, BseTrack))
#define BSE_TRACK_CLASS(class)	    (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_TRACK, BseTrackClass))
#define BSE_IS_TRACK(object)	    (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_TRACK))
#define BSE_IS_TRACK_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_TRACK))
#define BSE_TRACK_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_TRACK, BseTrackClass))


/* --- BseTrack --- */
struct _BseTrack
{
  BseItem	   parent_instance;

  BsePart	  *part_SL;
  BseSNet	  *snet;
  guint		   max_voices;

  BseMidiReceiver *midi_receiver_SL;

  BseSource       *voice_input;
  BseSource       *sub_synth;
  BseSource       *voice_switch;
  BseSource       *context_merger;
};
struct _BseTrackClass
{
  BseItemClass parent_class;
};


/* --- prototypes -- */
void	bse_track_add_modules		(BseTrack		*self,
					 BseContainer		*container,
					 BseSource		*merger);
void	bse_track_remove_modules	(BseTrack		*self,
					 BseContainer		*container);
void	bse_track_clone_voices		(BseTrack		*self,
					 BseSNet		*snet,
					 guint			 context,
					 GslTrans		*trans);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_TRACK_H__ */
