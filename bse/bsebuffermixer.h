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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * bsebuffermixer.h: BseMixValue buffer mixing functions
 */
#ifndef __BSE_BUFFER_MIXER_H__
#define __BSE_BUFFER_MIXER_H__

#include        <bse/bseglobals.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- structures --- */
#define BSE_MIX_SOURCE_ACTIVE(src) (((src)->cur_pos < (src)->bound || (src)->loop_count) && \
				     (!(src)->run_limit || (src)->max_run_values > 0))
struct _BseMixBuffer
{
  guint           n_tracks;	/* 2, stereo only currently */
  BseMixValue    *buffer;
  BseMixValue    *bound;
};
struct _BseMixSource
{
  guint           n_tracks;	/* 1 or 2 */
  BseSampleValue *cur_pos;	/* post-adjusted */
  BseSampleValue *bound;
  guint           loop_count;	/* post-adjusted */
  BseSampleValue *loop_start;
  BseSampleValue *loop_bound;
  /* run expiration */
  gint            max_run_values; /* post-adjusted */
  guint           run_limit : 1;
};
struct _BseMixVolume
{
  gfloat          left;		/* post-adjusted */
  gfloat          right;	/* post-adjusted */
  gfloat          left_delta;
  gfloat          right_delta;
};
struct _BseMixRate
{
  guint           interpolation : 1;
  gint            frac;		/* post-adjusted */
  gint            step;		/* post-adjusted */
  gint            delta;	/* stepping delta */
};


/* --- prototypes --- */
void	bse_mix_buffer_fill	(guint		 n_tracks,
				 BseMixValue	*mix_buffer,
				 BseSampleValue	 value);
void	bse_mix_buffer_add	(BseMixBuffer	*mix_buffer,
				 BseMixSource   *src_buffer,
				 BseMixVolume	*volume,
				 BseMixRate	*rate);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_BUFFER_MIXER_H__ */
