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
#define BSE_MIX_SOURCE_EMPTY(src)   ((src)->values_left > 0)
#define BSE_MIX_SOURCE_RUNNING(src) ((src)->values_left && (!(src)->run_limit || (src)->max_run_values > 0))
struct _BseMixBuffer
{
  guint           n_tracks;	/* supported: 2 */
  BseMixValue    *start;	/* post-adjusted */
  BseMixValue    *bound;
};
struct _BseMixSource
{
  /* constant fields */
  guint                 n_tracks;		/* supported: 1, 2 */
  gboolean              direction;		/* != 0: backwards (bound < start) */
  guint                 offset;			/* 0 */
  guint                 padding;		/* n_values around start and bound */

  /* runtime fields */
  guint                  n_values;		/* request/result */
  guint			 values_left;		/* result, just 0 or >0 matters */
  BseSampleValue        *start;			/* result */
  BseSampleValue        *bound;			/* result */

  /* run expiration */
  guint			 run_limit : 1;
  gint			 max_run_values;	/* post-adjusted */

  /* have to call refill() with n_values=0 for release */
  void                 (*refill) (BseMixSource *source);

  /* fields private to source owner */
  guint			 total_offset;
  guint                  loop_count;
  gpointer               data;
  gpointer		 block;
  BseSampleValue	*block_start;
  BseSampleValue	*block_bound;
};
struct _BseMixVolume
{
  gfloat          left;         /* post-adjusted */
  gfloat          right;        /* post-adjusted */
  gfloat          left_delta;
  gfloat          right_delta;
};
struct _BseMixRate
{
  gint            frac;         /* post-adjusted */
  gint            step;         /* post-adjusted */
  gint            delta;        /* stepping delta */
  guint           interpolation : 8;
};


/* --- prototypes --- */
void	bse_mix_buffer_add	(BseMixBuffer	*mix_buffer,
				 BseMixSource   *src_buffer,
				 BseMixVolume	*volume,
				 BseMixRate	*rate);
void	bse_mix_buffer_fill	(guint		 n_tracks,
				 BseMixValue	*mix_buffer,
				 BseSampleValue	 value);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_BUFFER_MIXER_H__ */
