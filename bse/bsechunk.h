/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
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
 * bsechunk.h: BSE chunk handling
 */
#ifndef __BSE_CHUNK_H__
#define __BSE_CHUNK_H__

#include        <bse/bsesource.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- structures --- */
struct _BseChunk
{
  guint		  n_tracks;
  BseSampleValue *state;
  BseSampleValue *hunk;

  guint		  state_filled : 1;
  guint		  hunk_filled : 1;

  /* private fields */
  guint		  free_state : 1;
  guint		  foreign_hunk : 1;
  guint		  g_free_hunk : 1;
  guint		  ref_count;
};


/* --- prototypes --- */
BseChunk*	bse_chunk_new_from_state	(guint		 n_tracks,
						 BseSampleValue	*state);
BseChunk*	bse_chunk_new			(guint		 n_tracks);
BseChunk*	bse_chunk_new0			(guint		 n_tracks);
BseChunk*	bse_chunk_new_orphan		(guint		 n_tracks,
						 BseSampleValue *orphan_hunk);
BseChunk*	bse_chunk_new_foreign		(guint		 n_tracks,
						 BseSampleValue	*data,
						 gboolean	 g_free_data);
BseChunk*	bse_chunk_new_static_zero	(guint		 n_tracks);
void		bse_chunk_ref			(BseChunk	*chunk);
void		bse_chunk_unref			(BseChunk	*chunk);
void		bse_chunk_complete_state 	(BseChunk	*chunk);
BseSampleValue*	bse_chunk_complete_hunk 	(BseChunk	*chunk);

BseSampleValue*	bse_hunk_alloc			(guint		 n_tracks);
BseSampleValue*	bse_hunk_copy			(guint		 n_tracks,
						 BseSampleValue *src_hunk);
BseSampleValue*	bse_hunk_alloc0			(guint		 n_tracks);
void		bse_hunk_free			(guint		 n_tracks,
						 BseSampleValue	*hunk);

/* FIXME: we could need mix_buffer alloc routines similar to the hunk ones */

void		bse_chunk_debug			(void);
void		bse_chunks_nuke			(void);





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_CHUNK_H__ */
