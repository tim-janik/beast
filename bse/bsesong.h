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


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


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
  
  guint             bpm;
  gfloat            volume_factor;      /* 1-based factor */
  
  GList            *parts;              /* of type BsePart* */
  GList            *tracks;             /* of type BseTrack* */

  BseSource	   *context_merger;
  BseSource	   *output;

  /*< private >*/
  BseSongSequencer *sequencer;
};
struct _BseSongClass
{
  BseSNetClass parent_class;
};


/* --- prototypes --- */
void             bse_song_set_bpm                    (BseSong         *song,
						      guint            bpm);
BseSong*         bse_song_lookup                     (BseProject      *project,
						      const gchar     *name);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_SONG_H__ */
