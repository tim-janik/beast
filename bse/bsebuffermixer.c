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

#include        "bseenums.h"



/* n valid bits for coefficient table (curve segment precision) */
#define	CUBIC_IPOL_CBITS	(9)	/* 2^9=512 segment values */
/* n fraction bits for integer multiplication */
#define	CUBIC_IPOL_SBITS	(15)
#define	CUBIC_IPOL_MIN_PADDING	(2)


/* --- typedefs --- */
typedef void	(*BufferMixerFunc)      (BseMixBuffer *mix_buffer,
					 BseMixSource *src_buffer,
					 BseMixVolume *volume,
					 BseMixRate   *rate);


/* --- mixing facilities --- */
#define MFLAG_CONST_RATE          (1 << 0)
#define MFLAG_DELTA_RATE          (1 << 1)
#define MFLAG_IPOL_LINEAR         (1 << 2)
#define MFLAG_IPOL_CUBIC          (1 << 3)
#define MFLAG_STEREO              (1 << 4)
#define MFLAG_VOLUME1_C           (1 << 5)
#define MFLAG_VOLUME1_D           (1 << 6)
#define MFLAG_VOLUME2_C           (1 << 7)
#define MFLAG_VOLUME2_D           (1 << 8)


/* --- prototypes --- */
static BufferMixerFunc fetch_buffer_mixer_function (guint flags);
static gint32	      *bse_cubic_ipol_W0 = NULL;
static gint32	      *bse_cubic_ipol_W1 = NULL;
static gint32	      *bse_cubic_ipol_W2 = NULL;
static gint32	      *bse_cubic_ipol_W3 = NULL;


/* --- functions --- */
static void
init_cubic_interpolation_coeff_table (void)
{
  static gint32 float_mem[4 * (1 << CUBIC_IPOL_CBITS)];
  static const int imul = 1 << CUBIC_IPOL_SBITS;
  guint i;
  
  bse_cubic_ipol_W0 = float_mem;
  bse_cubic_ipol_W1 = bse_cubic_ipol_W0 + (1 << CUBIC_IPOL_CBITS);
  bse_cubic_ipol_W2 = bse_cubic_ipol_W1 + (1 << CUBIC_IPOL_CBITS);
  bse_cubic_ipol_W3 = bse_cubic_ipol_W2 + (1 << CUBIC_IPOL_CBITS);
  
  for (i = 0; i < 1 << CUBIC_IPOL_CBITS; i++)
    {
      gfloat mmm, mm, m = i;
      
      m /= 1 << CUBIC_IPOL_CBITS;
      mm = m * m;
      mmm = m * m * m;
      bse_cubic_ipol_W3[i] = 0.5 + imul * (0.5 * mmm - 0.5 * mm);
      bse_cubic_ipol_W2[i] = 0.5 + imul * (-1.5 * mmm + 2   * mm + 0.5 * m);
      bse_cubic_ipol_W1[i] = 0.5 + imul * ( 1.5 * mmm - 2.5 * mm + 1);
      bse_cubic_ipol_W0[i] = 0.5 + imul * (-0.5 * mmm +       mm - 0.5 * m);
    }
}

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
      flags |= MFLAG_CONST_RATE;
      if (rate->delta)
	flags |= MFLAG_DELTA_RATE;
      switch (rate->interpolation)
	{
	case BSE_INTERPOL_LINEAR:
	  flags |= MFLAG_IPOL_LINEAR;
	  break;
	case BSE_INTERPOL_CUBIC:
	  if (source->block_padding < CUBIC_IPOL_MIN_PADDING)
	    {
	      static gboolean cubic_warn = FALSE;

	      if (!cubic_warn)
		{
		  cubic_warn = TRUE;
		  g_warning ("Cubic Interpolation for mix source without pad values, please report to beast@beast.gtk.org");
		}
	      flags |= MFLAG_IPOL_LINEAR;
	    }
	  else
	    flags |= MFLAG_IPOL_CUBIC;
	  break;
	case BSE_INTERPOL_NONE:
	default:
	  break;
	}
    }

  if (source->n_tracks == 2)
    {
      flags |= MFLAG_STEREO;
      if (volume)
	{
	  if (BSE_EPSILON_CMP (volume->left_delta, 0) || BSE_EPSILON_CMP (volume->right_delta, 0))
	    flags |= MFLAG_VOLUME2_D;
	  else if (BSE_EPSILON_CMP (volume->left, 1.0) || BSE_EPSILON_CMP (volume->right, 1.0))
	    flags |= MFLAG_VOLUME2_C;
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
	    flags |= MFLAG_VOLUME2_D;
	  else
	    flags |= MFLAG_VOLUME1_D;
	}
      else if (BSE_EPSILON_CMP (volume->left, 1.0) || BSE_EPSILON_CMP (volume->right, 1.0))
	flags |= BSE_EPSILON_CMP (volume->left, volume->right) ? MFLAG_VOLUME2_C : MFLAG_VOLUME1_C;
      else
	volume = NULL;
    }
  
  bmixer_func = fetch_buffer_mixer_function (flags);
  
  g_return_if_fail (bmixer_func != NULL);

  if (!bse_cubic_ipol_W0)
    init_cubic_interpolation_coeff_table ();
  
  /* mix buffer */
  bmixer_func (mix_buffer, source, volume, rate);
  
  /* paranoia checks */
  if (source->max_run_values < 0)
    g_warning (G_STRLOC ": run-length overflow by %d", - source->max_run_values);
}


/* --- mix function generator --- */

/* mix function version list for mkcall.pl */
#ifdef  MAKE_MIXER_CALLS
MAKE_MIXER_CALLS_FOR = {
  /*  1 */      0,
  /*  2 */      MFLAG_VOLUME1_C,
  /*  3 */      MFLAG_VOLUME1_D,
  /*  4 */      MFLAG_VOLUME2_C,
  /*  5 */      MFLAG_VOLUME2_D,
  /*  6 */      MFLAG_CONST_RATE,
  /*  7 */      MFLAG_CONST_RATE | MFLAG_VOLUME1_C,
  /*  8 */      MFLAG_CONST_RATE | MFLAG_VOLUME1_D,
  /*  9 */      MFLAG_CONST_RATE | MFLAG_VOLUME2_C,
  /* 10 */      MFLAG_CONST_RATE | MFLAG_VOLUME2_D,
  /* 11 */      MFLAG_DELTA_RATE,
  /* 12 */      MFLAG_DELTA_RATE | MFLAG_VOLUME1_C,
  /* 13 */      MFLAG_DELTA_RATE | MFLAG_VOLUME1_D,
  /* 14 */      MFLAG_DELTA_RATE | MFLAG_VOLUME2_C,
  /* 15 */      MFLAG_DELTA_RATE | MFLAG_VOLUME2_D,
  /* 16 */      MFLAG_STEREO,
  /* 17 */      MFLAG_STEREO | MFLAG_VOLUME1_C,
  /* 18 */      MFLAG_STEREO | MFLAG_VOLUME1_D,
  /* 19 */      MFLAG_STEREO | MFLAG_VOLUME2_C,
  /* 20 */      MFLAG_STEREO | MFLAG_VOLUME2_D,
  /* 21 */      MFLAG_CONST_RATE | MFLAG_STEREO,
  /* 22 */      MFLAG_CONST_RATE | MFLAG_STEREO | MFLAG_VOLUME1_C,
  /* 23 */      MFLAG_CONST_RATE | MFLAG_STEREO | MFLAG_VOLUME1_D,
  /* 24 */      MFLAG_CONST_RATE | MFLAG_STEREO | MFLAG_VOLUME2_C,
  /* 25 */      MFLAG_CONST_RATE | MFLAG_STEREO | MFLAG_VOLUME2_D,
  /* 26 */      MFLAG_DELTA_RATE | MFLAG_STEREO,
  /* 27 */      MFLAG_DELTA_RATE | MFLAG_STEREO | MFLAG_VOLUME1_C,
  /* 28 */      MFLAG_DELTA_RATE | MFLAG_STEREO | MFLAG_VOLUME1_D,
  /* 29 */      MFLAG_DELTA_RATE | MFLAG_STEREO | MFLAG_VOLUME2_C,
  /* 30 */      MFLAG_DELTA_RATE | MFLAG_STEREO | MFLAG_VOLUME2_D,
  /* 31 */      MFLAG_CONST_RATE | MFLAG_IPOL_LINEAR,
  /* 32 */      MFLAG_CONST_RATE | MFLAG_IPOL_LINEAR | MFLAG_VOLUME1_C,
  /* 33 */      MFLAG_CONST_RATE | MFLAG_IPOL_LINEAR | MFLAG_VOLUME1_D,
  /* 34 */      MFLAG_CONST_RATE | MFLAG_IPOL_LINEAR | MFLAG_VOLUME2_C,
  /* 35 */      MFLAG_CONST_RATE | MFLAG_IPOL_LINEAR | MFLAG_VOLUME2_D,
  /* 36 */      MFLAG_DELTA_RATE | MFLAG_IPOL_LINEAR,
  /* 37 */      MFLAG_DELTA_RATE | MFLAG_IPOL_LINEAR | MFLAG_VOLUME1_C,
  /* 38 */      MFLAG_DELTA_RATE | MFLAG_IPOL_LINEAR | MFLAG_VOLUME1_D,
  /* 39 */      MFLAG_DELTA_RATE | MFLAG_IPOL_LINEAR | MFLAG_VOLUME2_C,
  /* 40 */      MFLAG_DELTA_RATE | MFLAG_IPOL_LINEAR | MFLAG_VOLUME2_D,
  /* 41 */      MFLAG_CONST_RATE | MFLAG_IPOL_LINEAR | MFLAG_STEREO,
  /* 42 */      MFLAG_CONST_RATE | MFLAG_IPOL_LINEAR | MFLAG_STEREO | MFLAG_VOLUME1_C,
  /* 43 */      MFLAG_CONST_RATE | MFLAG_IPOL_LINEAR | MFLAG_STEREO | MFLAG_VOLUME1_D,
  /* 44 */      MFLAG_CONST_RATE | MFLAG_IPOL_LINEAR | MFLAG_STEREO | MFLAG_VOLUME2_C,
  /* 45 */      MFLAG_CONST_RATE | MFLAG_IPOL_LINEAR | MFLAG_STEREO | MFLAG_VOLUME2_D,
  /* 46 */      MFLAG_DELTA_RATE | MFLAG_IPOL_LINEAR | MFLAG_STEREO,
  /* 47 */      MFLAG_DELTA_RATE | MFLAG_IPOL_LINEAR | MFLAG_STEREO | MFLAG_VOLUME1_C,
  /* 48 */      MFLAG_DELTA_RATE | MFLAG_IPOL_LINEAR | MFLAG_STEREO | MFLAG_VOLUME1_D,
  /* 49 */      MFLAG_DELTA_RATE | MFLAG_IPOL_LINEAR | MFLAG_STEREO | MFLAG_VOLUME2_C,
  /* 50 */      MFLAG_DELTA_RATE | MFLAG_IPOL_LINEAR | MFLAG_STEREO | MFLAG_VOLUME2_D,
  /* 51 */      MFLAG_CONST_RATE | MFLAG_IPOL_CUBIC,
  /* 52 */      MFLAG_CONST_RATE | MFLAG_IPOL_CUBIC | MFLAG_VOLUME1_C,
  /* 53 */      MFLAG_CONST_RATE | MFLAG_IPOL_CUBIC | MFLAG_VOLUME1_D,
  /* 54 */      MFLAG_CONST_RATE | MFLAG_IPOL_CUBIC | MFLAG_VOLUME2_C,
  /* 55 */      MFLAG_CONST_RATE | MFLAG_IPOL_CUBIC | MFLAG_VOLUME2_D,
  /* 56 */      MFLAG_DELTA_RATE | MFLAG_IPOL_CUBIC,
  /* 57 */      MFLAG_DELTA_RATE | MFLAG_IPOL_CUBIC | MFLAG_VOLUME1_C,
  /* 58 */      MFLAG_DELTA_RATE | MFLAG_IPOL_CUBIC | MFLAG_VOLUME1_D,
  /* 59 */      MFLAG_DELTA_RATE | MFLAG_IPOL_CUBIC | MFLAG_VOLUME2_C,
  /* 60 */      MFLAG_DELTA_RATE | MFLAG_IPOL_CUBIC | MFLAG_VOLUME2_D,
  /* 61 */      MFLAG_CONST_RATE | MFLAG_IPOL_CUBIC | MFLAG_STEREO,
  /* 62 */      MFLAG_CONST_RATE | MFLAG_IPOL_CUBIC | MFLAG_STEREO | MFLAG_VOLUME1_C,
  /* 63 */      MFLAG_CONST_RATE | MFLAG_IPOL_CUBIC | MFLAG_STEREO | MFLAG_VOLUME1_D,
  /* 64 */      MFLAG_CONST_RATE | MFLAG_IPOL_CUBIC | MFLAG_STEREO | MFLAG_VOLUME2_C,
  /* 65 */      MFLAG_CONST_RATE | MFLAG_IPOL_CUBIC | MFLAG_STEREO | MFLAG_VOLUME2_D,
  /* 66 */      MFLAG_DELTA_RATE | MFLAG_IPOL_CUBIC | MFLAG_STEREO,
  /* 67 */      MFLAG_DELTA_RATE | MFLAG_IPOL_CUBIC | MFLAG_STEREO | MFLAG_VOLUME1_C,
  /* 68 */      MFLAG_DELTA_RATE | MFLAG_IPOL_CUBIC | MFLAG_STEREO | MFLAG_VOLUME1_D,
  /* 69 */      MFLAG_DELTA_RATE | MFLAG_IPOL_CUBIC | MFLAG_STEREO | MFLAG_VOLUME2_C,
  /* 70 */      MFLAG_DELTA_RATE | MFLAG_IPOL_CUBIC | MFLAG_STEREO | MFLAG_VOLUME2_D,
};
#endif  MAKE_MIXER_CALLS

/* facility tests */
#define MIX_CONST_RATE           ((MIXER_FLAGS) & MFLAG_CONST_RATE)
#define MIX_DELTA_RATE           ((MIXER_FLAGS) & MFLAG_DELTA_RATE)
#define MIX_IPOL_LINEAR          ((MIXER_FLAGS) & MFLAG_IPOL_LINEAR)
#define MIX_IPOL_CUBIC           ((MIXER_FLAGS) & MFLAG_IPOL_CUBIC)
#define MIX_STEREO               ((MIXER_FLAGS) & MFLAG_STEREO)
#define MIX_VOLUME1              (((MIXER_FLAGS) & MFLAG_VOLUME1_C) || ((MIXER_FLAGS) & MFLAG_VOLUME1_D))
#define MIX_VOLUME2              (((MIXER_FLAGS) & MFLAG_VOLUME2_C) || ((MIXER_FLAGS) & MFLAG_VOLUME2_D))
#define MIX_VOLUME1_C            ((MIXER_FLAGS) & MFLAG_VOLUME1_C)
#define MIX_VOLUME1_D            ((MIXER_FLAGS) & MFLAG_VOLUME1_D)
#define MIX_VOLUME2_C            ((MIXER_FLAGS) & MFLAG_VOLUME2_C)
#define MIX_VOLUME2_D            ((MIXER_FLAGS) & MFLAG_VOLUME2_D)

/* include file generated from mkcalls.pl, it includes
 * bsebuffermixercore.c, specialized for all the above
 * variants listed
 */
#include "bsebuffermixer_gen.c"
