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
 * bsemixer.h: BSE chunk mixing functions
 */
#ifndef __BSE_HUNK_MIXER_H__
#define __BSE_HUNK_MIXER_H__

#include        <bse/bsechunk.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- prototypes --- */
void	bse_hunk_mix			(guint                 n_dest_tracks,
					 BseSampleValue       *dest_hunk,
					 const gfloat         *dest_volumes,
					 guint                 n_src_tracks,
					 const BseSampleValue *src_hunk);
void	bse_hunk_fill			(guint		       n_tracks,
					 BseSampleValue	      *hunk,
					 BseSampleValue	       value);
void	bse_hunk_clip_mix_buffer	(guint		       n_tracks,
					 BseSampleValue	      *dest_hunk,
					 gfloat                master_volume,
					 BseMixValue	      *src_mix_buffer);
void	bse_mix_buffer_fill		(guint		       n_tracks,
					 BseMixValue	      *mix_buffer,
					 BseSampleValue	       value);

       
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_HUNK_MIXER_H__ */
