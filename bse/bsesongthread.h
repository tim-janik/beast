/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2002 Tim Janik
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
#ifndef __BSE_SONG_SEQUENCER_H__
#define __BSE_SONG_SEQUENCER_H__

#include <bse/bsesong.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define	bse_song_sequencer_recalc(song)		({})
struct _BseSongSequencer
{
  BseSong *song;
  guint64  next_tick;
  guint    row;
};


BseSongSequencer*	bse_song_sequencer_setup	(BseSong		*song);
void			bse_song_sequencer_destroy	(BseSongSequencer	*sequencer);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_SONG_SEQUENCER_H__ */
