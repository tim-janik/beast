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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

/* this file gets included from bsebuffermixer_gen.c which is generated from
 * mkcalls.pl acording to, and included from bsebuffermixer.c
 */

#ifndef	__MIXER_LOOP_MAX_ITERATIONS__
#define	__MIXER_LOOP_MAX_ITERATIONS__
static inline gint
mixer_loop_max_iterations (gfloat rate_pos	/* << 16 */,
			   gfloat step_rate	/* << 16 */,
			   gfloat rate_delta	/* << 16 */,
			   gfloat b)
{
  const gfloat shift = (1.0 / 65536.0);
  gfloat d = rate_delta * shift;
  gfloat s = step_rate * shift, r = rate_pos * shift;

  if (BSE_EPSILON_CMP (rate_delta, 0))
    {
      /* ok, here we have:
       * s = step_rate >> 16, d = rate_delta >> 16, r = rate_pos >> 16,
       * b (maximum new rate_pos)
       * and need: x = # iterations
       *
       * in the loop we do: r += s; s += d; for x times and then get b in r
       * so after the loop, we have (b = new r):
       * r + x * s + d * E[1<=i<=x-1] (i) = b
       * with E[1<=i<=n] (i) = (n^2 + n) / 2 = n / 2 * (n + 1) we get:
       * r + x * s + d * (x - 1) / 2 * x = b
       * which is:
       * x^2 + (2 * s / d - 1) * x + 2 * (r - b) / d = 0
       * solve through: x^2 + p * x + q = 0
       *                p = (2 * s / d - 1)
       *                q = 2 * (r - b) / d
       *                x = - (p / 2) +- sqrt ((p / 2)^2 - q)
       * then return the x > 0
       */
      
      gfloat p = 2 * s / d - 1, q = 2 * (r - b) / d;
      gfloat p2 = p / 2, q2 = sqrt (p2 * p2 - q);
      
      return - p2 > q2 ? - p2 - q2 : q2 - p2;
    }
  else
    {
      /* we can majorly simplify the above for d = 0
       */
      return (b - r) / s;
    }
}
#endif	/* __MIXER_LOOP_MAX_ITERATIONS__ */


/* shortcut macro definitions
 */
#undef	IF_STEREO
#undef	IF_MONO
#undef	IF_DELTA_RATE
#undef	IF_IPOL_LINEAR
#undef	IF_IPOL_CUBIC
#undef	MIX_WITH_RATE
#undef	MIX_WITH_IPOL
#define	MIX_WITH_RATE	(MIX_CONST_RATE | MIX_DELTA_RATE)
#define	MIX_WITH_IPOL	(MIX_IPOL_LINEAR | MIX_IPOL_CUBIC)
#undef	IPOL_SHIFT_BITS
#if MIX_IPOL_LINEAR
#  define IPOL_SHIFT_BITS	16
#elif MIX_IPOL_CUBIC
#  define IPOL_SHIFT_BITS	CUBIC_IPOL_SBITS
#endif
#if MIX_STEREO
#  define IF_STEREO(expr)		expr
#  define IF_MONO(expr)			/* expr */
#else /* mono */
#  define IF_STEREO(expr)		/* expr */
#  define IF_MONO(expr)			expr
#endif
#if MIX_DELTA_RATE
#  define IF_DELTA_RATE(expr)		expr
#else
#  define IF_DELTA_RATE(expr)		/* expr */
#endif


/* buffer_mixer_function__XX() */
static void
MIXER_FUNCTION_ID (BseMixBuffer *mix_buffer,
		   BseMixSource *source,
		   BseMixVolume *volume,
		   BseMixRate   *rate)
{
  BseMixValue *buffer = mix_buffer->buffer, *buffer_bound = mix_buffer->bound;
#if MIX_VOLUME1_C	/* mono volume, constant */
  gfloat m_volume = volume->left;
#elif MIX_VOLUME1_D	/* mono volume, delta slide */
  gfloat m_volume = volume->left, m_volume_delta = volume->left_delta;
#elif MIX_VOLUME2_C	/* stereo volume, constant */
  gfloat l_volume = volume->left, r_volume = volume->right;
#elif MIX_VOLUME2_D	/* stereo volume, delta slide */
  gfloat l_volume = volume->left, r_volume = volume->right;
  gfloat l_volume_delta = volume->left_delta, r_volume_delta = volume->right_delta;
#endif
  
  do /* restart mixing cycles for src buffers with loops */
    {
      BseMixValue *run_buffer_start = buffer, *bound = buffer;
      gint32 rate_frac = rate->frac, rate_step = rate->step, rate_delta = rate->delta;
      BseSampleValue *l_sample = source->cur_pos;
      gint i = source->loop_count ? source->loop_bound - l_sample : source->bound - l_sample;
      
      IF_STEREO (i >>= 1);
      i = mixer_loop_max_iterations (rate_frac, rate_step, rate_delta, i);
      if (source->run_limit)
	bound += MIN (buffer_bound - buffer, MIN (i, source->max_run_values) << 1);
      else
	bound += MIN (buffer_bound - buffer, i << 1);
#if MIX_STEREO
      rate_frac <<= 1;
      rate_step <<= 1;
      rate_delta <<= 1;
#endif

      do	/* inner mixing loop */
	{
	  register BseMixValue l_value, r_value;

#if MIX_WITH_RATE
	  register guint pos_high = rate_frac >> 16;
	  register guint pos_low = rate_frac & 0xffff;

	  rate_frac = pos_low + rate_step;
	  IF_DELTA_RATE (rate_step += rate_delta);
#  if MIX_IPOL_CUBIC
	  {
	    register guint cindex = pos_low >> (16 - CUBIC_IPOL_CBITS);
	    register gint32 W3 = bse_cubic_ipol_W3[cindex], W2 = bse_cubic_ipol_W2[cindex];
	    register gint32 W1 = bse_cubic_ipol_W1[cindex], W0 = bse_cubic_ipol_W0[cindex];
#    if MIX_STEREO
	    register BseMixValue l_V0 = l_sample[-2], l_V1 = l_sample[0], l_V2 = l_sample[2], l_V3 = l_sample[4];
	    register BseMixValue r_V0 = l_sample[-1], r_V1 = l_sample[1], r_V2 = l_sample[3], r_V3 = l_sample[5];

	    r_value = W3 * r_V3 + W2 * r_V2 + W1 * r_V1 + W0 * r_V0;
#    else /* mono */
	    register BseMixValue l_V0 = l_sample[-1], l_V1 = l_sample[0], l_V2 = l_sample[1], l_V3 = l_sample[2];
#    endif /* mono */
	    l_value = W3 * l_V3 + W2 * l_V2 + W1 * l_V1 + W0 * l_V0;
	  }
#  elif MIX_IPOL_LINEAR
	  {
#    if MIX_STEREO
	    register BseMixValue l_V1 = l_sample[0], r_V1 = l_sample[1], l_V2 = l_sample[2], r_V2 = l_sample[3];

	    r_value = (r_V1 << 16) + (r_V2 - r_V1) * pos_low;
#    else /* mono */
	    register BseMixValue l_V1 = l_sample[0], l_V2 = l_sample[1];
#    endif /* mono */
	    l_value = (l_V1 << 16) + (l_V2 - l_V1) * pos_low;
	  }
#  else /* no interpolation */
	  l_value = l_sample[0];
	  IF_STEREO (r_value = l_sample[1]);
#  endif /* no interpolation */
	  IF_MONO (r_value = l_value);
	  l_sample += pos_high;
#else /* not MIX_WITH_RATE */
	  l_value = *(l_sample++);
	  IF_STEREO (r_value = *(l_sample++));
	  IF_MONO (r_value = l_value);
#endif /* not MIX_WITH_RATE */


#if MIX_VOLUME1_C	/* volume multiplications */
	  l_value *= m_volume; 
	  IF_STEREO (r_value *= m_volume);
	  IF_MONO (r_value = l_value);
#elif MIX_VOLUME1_D
	  l_value *= m_volume;
	  IF_STEREO (r_value *= m_volume);
	  IF_MONO (r_value = l_value);
	  m_volume += m_volume_delta;
#elif MIX_VOLUME2_C
	  l_value *= l_volume;
	  r_value *= r_volume;
#elif MIX_VOLUME2_D
	  l_value *= l_volume;
	  r_value *= r_volume;
	  l_volume += l_volume_delta;
	  r_volume += r_volume_delta;
#endif /* MIX_VOLUME2_D */
	  
#if MIX_WITH_IPOL	/* post interpolation correction / 65536 (shift *after* the volume multiplication) */
	  l_value >>= IPOL_SHIFT_BITS;
#  if MIX_STEREO || MIX_VOLUME2
	  r_value >>= IPOL_SHIFT_BITS;
#  else
	  r_value = l_value;
#  endif
#endif
	  
	  *(buffer++) += l_value;
	  *(buffer++) += r_value;
	}
      while (buffer < bound);

#if MIX_VOLUME1_D
      volume->left = m_volume;
      volume->right = m_volume;
#elif MIX_VOLUME2_D
      volume->left = l_volume;
      volume->right = r_volume;
#endif
      IF_STEREO (rate_frac >>= 1);
      rate->frac = rate_frac;
      IF_STEREO (rate_step >>= 1);
      rate->step = rate_step;
      if (source->run_limit)
	source->max_run_values -= (buffer - run_buffer_start) >> 1;
      if (source->loop_count)
	{
	  i = source->loop_bound - source->loop_start;
	  while (l_sample >= source->loop_bound)
	    l_sample -= i;
	  source->cur_pos = l_sample;
	  source->loop_count -= 1;
	  if (source->run_limit && source->max_run_values < 1)
	    break;
	}
      else
	{
          source->cur_pos = l_sample;
	  break;
	}
    }
  while (buffer < buffer_bound);
  mix_buffer->buffer = buffer;
}

#undef MIXER_FLAGS
#undef MIXER_FUNCTION_ID
