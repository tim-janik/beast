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
 *
 * bsesong.h: bse song object
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
  BseSource *ofreq;
  BseSource *synth;
} BseSongVoice;
typedef struct {
  BseSource    *lmixer;
  BseSource    *rmixer;
  BseSource    *output;
  BseSongVoice *voices;	/* [n_channels] */
} BseSongNet;
struct _BseSong
{
  BseSNet parent_object;
  
  guint             bpm;
  gfloat            volume_factor;      /* 1-based factor */
  
  guint             pattern_length;     /* >= 4 by convention */
  guint             n_channels;         /* >= 2 by convention */
  
  GList            *instruments;        /* of type BseInstrument* */
  GList            *patterns;           /* of type BsePattern* */
  GList            *pattern_groups;     /* of type BsePatternGroup* */

  guint             n_pgroups;
  BsePatternGroup **pgroups;		/* play list */

  BseSongNet	    net;

  /*< private >*/
  BseSongSequencer *sequencer;
  BseIndex          sequencer_index;
};
struct _BseSongClass
{
  BseSNetClass parent_class;
};


/* --- prototypes --- */
void             bse_song_set_pattern_length         (BseSong         *song,
						      guint            pattern_length);
void             bse_song_set_bpm                    (BseSong         *song,
						      guint            bpm);
BseSong*         bse_song_lookup                     (BseProject      *project,
						      const gchar     *name);
BsePattern*      bse_song_get_pattern                (BseSong         *song,
						      guint            seqid);
BseInstrument*   bse_song_get_instrument             (BseSong         *song,
						      guint            seqid);
BsePatternGroup* bse_song_get_default_pattern_group  (BseSong         *song);
void             bse_song_insert_pattern_group_link  (BseSong         *song,
						      BsePatternGroup *pgroup,
						      gint             position);
void             bse_song_insert_pattern_group_copy  (BseSong         *song,
						      BsePatternGroup *pgroup,
						      gint             position);
void             bse_song_remove_pattern_group_entry (BseSong         *song,
                                                      gint             position);
BsePattern*      bse_song_get_pattern_from_list      (BseSong         *song,
						      guint            pattern_index);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_SONG_H__ */
