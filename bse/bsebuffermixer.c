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
 */
#include        "bsebuffermixer.h"



/* --- typedefs --- */
typedef void	(*BufferMixerFunc)      (BseMixBuffer *mix_buffer,
					 BseMixSource *src_buffer,
					 BseMixVolume *volume,
					 BseMixRate   *rate);


/* --- mixing facilities --- */
#define MIX_LINEAR              (1 << 0)
#define MIX_INTERPOLATION       (1 << 1)
#define MIX_STEREO              (1 << 2)
#define MIX_RATE_C              (1 << 3)
#define MIX_RATE_D              (1 << 4)
#define MIX_VOLUME1_C           (1 << 5)
#define MIX_VOLUME1_D           (1 << 6)
#define MIX_VOLUME2_C           (1 << 7)
#define MIX_VOLUME2_D           (1 << 8)


/* --- prototypes --- */
static BufferMixerFunc fetch_buffer_mixer_function (guint flags);


/* --- functions --- */
void
bse_mix_buffer_fill (guint          n_tracks,
		     BseMixValue   *mix_buffer,
		     BseSampleValue value)
{
  g_return_if_fail (n_tracks >= 1 && n_tracks <= BSE_MAX_N_TRACKS);
  g_return_if_fail (mix_buffer != NULL);

  if (value == 0)
    memset (mix_buffer, value, n_tracks * BSE_TRACK_LENGTH * sizeof (BseMixValue));
  else
    {
      BseMixValue *bound = mix_buffer + n_tracks * BSE_TRACK_LENGTH;

      do
	*(mix_buffer++) = value;
      while (mix_buffer < bound);
    }
}

void
bse_mix_buffer_add (BseMixBuffer *mix_buffer,
		    BseMixSource *source,
		    BseMixVolume *volume,
		    BseMixRate   *rate)
{
  BufferMixerFunc bmixer_func;
  guint flags = 0;
  
  g_return_if_fail (mix_buffer != NULL);
  g_return_if_fail (source != NULL);
  g_return_if_fail (mix_buffer->n_tracks == 2);
  g_return_if_fail (mix_buffer->buffer != NULL);
  g_return_if_fail (mix_buffer->bound >= mix_buffer->buffer + mix_buffer->n_tracks);
  g_return_if_fail (source->n_tracks >= 1 && source->n_tracks <= 2);
  g_return_if_fail (source->bound >= source->cur_pos + source->n_tracks);
  if (source->loop_count)
    {
      g_return_if_fail (source->loop_start != NULL);
      g_return_if_fail (source->loop_bound >= source->loop_start + source->n_tracks);
      g_return_if_fail (source->loop_bound <= source->bound);
      g_return_if_fail (source->cur_pos < source->loop_bound);
    }
  g_return_if_fail (BSE_MIX_SOURCE_ACTIVE (source));
  if (rate)
    g_return_if_fail (rate->step > 0);
  
  if (rate)
    {
      if (rate->interpolation)
	flags |= MIX_INTERPOLATION;
      flags |= rate->delta ? MIX_RATE_D : MIX_RATE_C;
    }
  else
    flags |= MIX_LINEAR;
  if (source->n_tracks == 2)
    {
      flags |= MIX_STEREO;
      if (volume)
	{
	  if (BSE_EPSILON_CMP (volume->left_delta, 0) || BSE_EPSILON_CMP (volume->right_delta, 0))
	    flags |= MIX_VOLUME2_D;
	  else if (BSE_EPSILON_CMP (volume->left, 1.0) || BSE_EPSILON_CMP (volume->right, 1.0))
	    flags |= MIX_VOLUME2_C;
	  else
	    volume = NULL;
	}
    }
  else if (volume)
    {
      if (BSE_EPSILON_CMP (volume->left_delta, 0) || BSE_EPSILON_CMP (volume->right_delta, 0))
	{
	  if (BSE_EPSILON_CMP (volume->left_delta, volume->right_delta) ||
	      BSE_EPSILON_CMP (volume->left, volume->right))
	    flags |= MIX_VOLUME2_D;
	  else
	    flags |= MIX_VOLUME1_D;
	}
      else if (BSE_EPSILON_CMP (volume->left, 1.0) || BSE_EPSILON_CMP (volume->right, 1.0))
	flags |= BSE_EPSILON_CMP (volume->left, volume->right) ? MIX_VOLUME2_C : MIX_VOLUME1_C;
      else
	volume = NULL;
    }
  
  bmixer_func = fetch_buffer_mixer_function (flags);
  
  g_return_if_fail (bmixer_func != NULL);

  /* mix buffer */
  bmixer_func (mix_buffer, source, volume, rate);

  /* paranoia checks */
  if (source->max_run_values < 0)
    g_warning (G_STRLOC ": run-length overflow by %d", - source->max_run_values);
}


/* --- mix function generator --- */

/* facility tests */
#define DO_LINEAR               ((MIXER_FLAGS) & MIX_LINEAR)
#define DO_INTERPOLATION        ((MIXER_FLAGS) & MIX_INTERPOLATION)
#define DO_STEREO               ((MIXER_FLAGS) & MIX_STEREO)
#define DO_RATE_C               ((MIXER_FLAGS) & MIX_RATE_C)
#define DO_RATE_D               ((MIXER_FLAGS) & MIX_RATE_D)
#define DO_VOLUME1              (((MIXER_FLAGS) & MIX_VOLUME1_C) || ((MIXER_FLAGS) & MIX_VOLUME1_D))
#define DO_VOLUME2              (((MIXER_FLAGS) & MIX_VOLUME2_C) || ((MIXER_FLAGS) & MIX_VOLUME2_D))
#define DO_VOLUME1_C            ((MIXER_FLAGS) & MIX_VOLUME1_C)
#define DO_VOLUME1_D            ((MIXER_FLAGS) & MIX_VOLUME1_D)
#define DO_VOLUME2_C            ((MIXER_FLAGS) & MIX_VOLUME2_C)
#define DO_VOLUME2_D            ((MIXER_FLAGS) & MIX_VOLUME2_D)


/* mix function version list for mkcall.pl */
#ifdef  MAKE_MIXER_CALLS
MAKE_MIXER_CALLS_FOR = {
  /*  1 */      MIX_LINEAR,
  /*  2 */      MIX_LINEAR | MIX_VOLUME1_C,
  /*  3 */      MIX_LINEAR | MIX_VOLUME1_D,
  /*  4 */      MIX_LINEAR | MIX_VOLUME2_C,
  /*  5 */      MIX_LINEAR | MIX_VOLUME2_D,
  /*  6 */      MIX_RATE_C,
  /*  7 */      MIX_RATE_C | MIX_VOLUME1_C,
  /*  8 */      MIX_RATE_C | MIX_VOLUME1_D,
  /*  9 */      MIX_RATE_C | MIX_VOLUME2_C,
  /* 10 */      MIX_RATE_C | MIX_VOLUME2_D,
  /* 11 */      MIX_RATE_D,
  /* 12 */      MIX_RATE_D | MIX_VOLUME1_C,
  /* 13 */      MIX_RATE_D | MIX_VOLUME1_D,
  /* 14 */      MIX_RATE_D | MIX_VOLUME2_C,
  /* 15 */      MIX_RATE_D | MIX_VOLUME2_D,
  /* 16 */      MIX_INTERPOLATION | MIX_RATE_C,
  /* 17 */      MIX_INTERPOLATION | MIX_RATE_C | MIX_VOLUME1_C,
  /* 18 */      MIX_INTERPOLATION | MIX_RATE_C | MIX_VOLUME1_D,
  /* 19 */      MIX_INTERPOLATION | MIX_RATE_C | MIX_VOLUME2_C,
  /* 20 */      MIX_INTERPOLATION | MIX_RATE_C | MIX_VOLUME2_D,
  /* 21 */      MIX_INTERPOLATION | MIX_RATE_D,
  /* 22 */      MIX_INTERPOLATION | MIX_RATE_D | MIX_VOLUME1_C,
  /* 23 */      MIX_INTERPOLATION | MIX_RATE_D | MIX_VOLUME1_D,
  /* 24 */      MIX_INTERPOLATION | MIX_RATE_D | MIX_VOLUME2_C,
  /* 25 */      MIX_INTERPOLATION | MIX_RATE_D | MIX_VOLUME2_D,
  /* 26 */      MIX_STEREO | MIX_LINEAR,
  /* 27 */      MIX_STEREO | MIX_LINEAR | MIX_VOLUME1_C,
  /* 28 */      MIX_STEREO | MIX_LINEAR | MIX_VOLUME1_D,
  /* 29 */      MIX_STEREO | MIX_LINEAR | MIX_VOLUME2_C,
  /* 30 */      MIX_STEREO | MIX_LINEAR | MIX_VOLUME2_D,
  /* 31 */      MIX_STEREO | MIX_RATE_C,
  /* 32 */      MIX_STEREO | MIX_RATE_C | MIX_VOLUME1_C,
  /* 33 */      MIX_STEREO | MIX_RATE_C | MIX_VOLUME1_D,
  /* 34 */      MIX_STEREO | MIX_RATE_C | MIX_VOLUME2_C,
  /* 35 */      MIX_STEREO | MIX_RATE_C | MIX_VOLUME2_D,
  /* 36 */      MIX_STEREO | MIX_RATE_D,
  /* 37 */      MIX_STEREO | MIX_RATE_D | MIX_VOLUME1_C,
  /* 38 */      MIX_STEREO | MIX_RATE_D | MIX_VOLUME1_D,
  /* 39 */      MIX_STEREO | MIX_RATE_D | MIX_VOLUME2_C,
  /* 40 */      MIX_STEREO | MIX_RATE_D | MIX_VOLUME2_D,
  /* 41 */      MIX_STEREO | MIX_INTERPOLATION | MIX_RATE_C,
  /* 42 */      MIX_STEREO | MIX_INTERPOLATION | MIX_RATE_C | MIX_VOLUME1_C,
  /* 43 */      MIX_STEREO | MIX_INTERPOLATION | MIX_RATE_C | MIX_VOLUME1_D,
  /* 44 */      MIX_STEREO | MIX_INTERPOLATION | MIX_RATE_C | MIX_VOLUME2_C,
  /* 45 */      MIX_STEREO | MIX_INTERPOLATION | MIX_RATE_C | MIX_VOLUME2_D,
  /* 46 */      MIX_STEREO | MIX_INTERPOLATION | MIX_RATE_D,
  /* 47 */      MIX_STEREO | MIX_INTERPOLATION | MIX_RATE_D | MIX_VOLUME1_C,
  /* 48 */      MIX_STEREO | MIX_INTERPOLATION | MIX_RATE_D | MIX_VOLUME1_D,
  /* 49 */      MIX_STEREO | MIX_INTERPOLATION | MIX_RATE_D | MIX_VOLUME2_C,
  /* 50 */      MIX_STEREO | MIX_INTERPOLATION | MIX_RATE_D | MIX_VOLUME2_D,
};
#endif  MAKE_MIXER_CALLS

/* include file generated from mkcalls.pl, it includes
 * bsebuffermixercore.c, specialized for all the above
 * variants listed
 */
#include "bsebuffermixer_gen.c"
