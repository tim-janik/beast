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
 */

/* this file gets included from bsebuffermixer_gen.c which is generated from
 * mkcalls.pl acording to, and included from bsebuffermixer.c.
 */

#ifndef	__SRCBUFFER_SIZE_MAX_ITERATIONS
#define	__SRCBUFFER_SIZE_MAX_ITERATIONS
/* this is essentially bse_poly2_droots() except that we take 2*a and 2*c as arguments */
static inline gboolean
poly2_droots_2a2c (gdouble roots[2],
                   gdouble a2,
                   gdouble b,
                   gdouble c2)
{
  gdouble square = b * b - a2 * c2;
  gdouble tmp;

  if (square < 0)
    return FALSE;

  if (b > 0)
    tmp = -b - sqrt (square);
  else
    tmp = -b + sqrt (square);

  roots[0] = tmp / a2;
  roots[1] = c2 / tmp;

  return TRUE;
}
static inline gint
srcbuffer_size_max_iterations (gint rate_pos,   /* << 16 */
                               gint rate_step,  /* << 16 */
                               gint rate_delta, /* << 16 */
                               gint last_pos_unshifted)
{
  gint64 last_pos = last_pos_unshifted;

  /* to get the maximum number of iterations to reach last_pos, get the smallest
   * number of iterations to reach last_pos+1 and subtract one
   */
  last_pos += 1;

  /* last_pos <<= 16; // done later */
  if (rate_delta)
    {
      /* for j=[1:...] last_pos is:
       * j=1: 0
       * j>1: rate_pos + (j-2) * rate_step + rate_delta * E[1<=i<=j-3] (i)
       * with E[1<=i<=n] (i) = (n^2 + n) / 2 = n / 2 * (n + 1) we get:
       * last_pos = rate_pos + (j-2) * rate_step + rate_delta * (j-2) * (j-3) / 2
       * substituting z=j-2, we get:
       * j>1: z=j-2, last_pos = rate_pos + z*rate_step + rate_delta*z*(z-1)/2
       * solve through: a*x^2 + b*x + c = 0, with
       * a = 2.0*rate_delta, b = rate_step-(rate_delta/2.0), c = 2.0*(r-last_pos)
       * that gets us two zero positions; for rate_delta > 0 use the bigger,
       * for rate_delta < 0 use the smaller (constrain the apex ranges through the
       * first derivation to figure why).
       */
      gdouble r = rate_pos << 1, s = rate_step, d = rate_delta, roots[2], z;
      gint n_iter;

      /* no solution may be found for rate_delta < 0 because of last_pos += 1,
       * then we simply return the apex.
       */
      last_pos <<= 16 + 1;
      if (!poly2_droots_2a2c (roots, rate_delta, rate_step - rate_delta / 2.0, r - last_pos))
        return 2.5 - s / d;

      z = rate_delta > 0 ? MAX (roots[0], roots[1]) : MIN (roots[0], roots[1]);

      /* subtract one; in practice, floor(z) often does that already, except
       * when z is very close to floor(z), so we subtract an extra epsilon
       */
      n_iter = floor (z - 1e-10);

      /* j = z + 2 */
      n_iter += 2;

      return n_iter;
    }
  else
    {
      gint n_iter;

      /* we can majorly simplify the above for rate_delta=0:
       * j>1: z=j-2, last_pos = rate_pos + z*rate_step
       * thus, j = (last_pos - rate_pos) / rate_step + 2
       */
      last_pos <<= 16;
      n_iter = (last_pos - rate_pos) / rate_step;

      /* j = z + 2, subtract one */
      n_iter += 2 - 1;

      return n_iter;
    }
}
#endif	/* __SRCBUFFER_SIZE_MAX_ITERATIONS */

/* given conditions:
 * MIX_WITH_RATE, MIX_RATE_CONST, MIX_RATE_DELTA,
 * MIX_WITH_IPOL, MIX_IPOL_LINEAR, MIX_IPOL_CUBIC,
 * MIX_VOLUME1, MIX_VOLUME2,
 * MIX_VOLUME1_CONST, MIX_VOLUME1_DELTA,
 * MIX_VOLUME2_CONST, MIX_VOLUME2_DELTA,
 * MIX_STEREO, MIX_MONO, MIX_REVERSE,
 * MIXER_FLAGS, MIXER_FUNCTION_ID
 */

/* conditionalized expressions */
#undef DO
#undef NOP
#undef IF_MIX_MONO
#undef IF_MIX_STEREO
#undef IF_MIX_RATE
#undef IF_MIX_RATE_DELTA
#undef IF_MIX_VOLUME1
#undef IF_MIX_VOLUME2
#undef IF_MIX_VOLUME1_DELTA
#undef IF_MIX_VOLUME2_DELTA
#define	DO(expr)			expr
#define	NOP()
#if	MIX_MONO
#  define IF_MIX_MONO(expr)		expr
#  define IF_MIX_STEREO(expr)		/* expr */
#else	/* !MIX_MONO */
#  define IF_MIX_MONO(expr)		/* expr */
#  define IF_MIX_STEREO(expr)		expr
#endif
#if	MIX_WITH_RATE
#  define IF_MIX_RATE(expr)		expr
#else	/* !MIX_WITH_RATE */
#  define IF_MIX_RATE(expr)		/* expr */
#endif
#if	MIX_RATE_DELTA
#  define IF_MIX_RATE_DELTA(expr)	expr
#else	/* !MIX_RATE_DELTA */
#  define IF_MIX_RATE_DELTA(expr)	/* expr */
#endif
#if	MIX_VOLUME1
#  define IF_MIX_VOLUME1(expr)		expr
#  define IF_MIX_VOLUME2(expr)		/* expr */
#elif	MIX_VOLUME2
#  define IF_MIX_VOLUME1(expr)		/* expr */
#  define IF_MIX_VOLUME2(expr)		expr
#else	/* !MIX_VOLUME1 && !MIX_VOLUME2 */
#  define IF_MIX_VOLUME1(expr)		/* expr */
#  define IF_MIX_VOLUME2(expr)		/* expr */
#endif
#if	MIX_VOLUME1_DELTA
#  define IF_MIX_VOLUME1_DELTA(expr)	expr
#else	/* !MIX_VOLUME1_DELTA */
#  define IF_MIX_VOLUME1_DELTA(expr)	/* expr */
#endif
#if	MIX_VOLUME2_DELTA
#  define IF_MIX_VOLUME2_DELTA(expr)	expr
#else	/* !MIX_VOLUME2_DELTA */
#  define IF_MIX_VOLUME2_DELTA(expr)	/* expr */
#endif

/* increment/decrement and bound checking */
#undef	BOUND_UNREACHED
#undef	INC
#if	MIX_REVERSE
#  define BOUND_UNREACHED(val, bound)	((val) > (bound))
#  define INC(lval, n)			(lval -= (n))
#else	/* !MIX_REVERSE */
#  define BOUND_UNREACHED(val, bound)	((val) < (bound))
#  define INC(lval, n)			(lval += (n))
#endif

/* interpolation value correction */
#undef	POST_IPOL_SHIFT_BITS
#if	MIX_IPOL_LINEAR
# define POST_IPOL_SHIFT_BITS	BSE_SAMPLE_SHIFT
#elif	MIX_IPOL_CUBIC
# define POST_IPOL_SHIFT_BITS	CUBIC_IPOL_SAMPLE_SHIFT
#endif

#ifdef	INT_SAMPLES
#define	LEFT_SHIFT(value, bits)	 ((value) << (bits))
#define	RIGHT_SHIFT(value, bits) ((value) >> (bits))
#else
#define	LEFT_SHIFT(value, bits)	 ((value) * ((gfloat) (((guint32) 1) << (bits))))
#define	RIGHT_SHIFT(value, bits) ((value) / ((gfloat) (((guint32) 1) << (bits))))
#endif


/* prerequisites:
 * - if !rate, buffer->bound - buffer->start == source->bound - source->start
 */
/* buffer_mixer_function__XXX() */
static void
MIXER_FUNCTION_ID (BseMixBuffer *buffer,
		   BseMixSource *source,
		   BseMixVolume *volume,
		   BseMixRate   *rate)
{
  BseMixValue *mix_value = buffer->start, *mix_bound = buffer->bound, *run_limit_start = mix_value;
  NOP ()
    IF_MIX_VOLUME1	 (  gfloat mix_volume = volume->left; 		     )
    IF_MIX_VOLUME1_DELTA (  gfloat mix_volume_delta = volume->left_delta;    )
    IF_MIX_VOLUME2	 (  gfloat left_volume = volume->left;
			    gfloat right_volume = volume->right;	     )
    IF_MIX_VOLUME2_DELTA (  gfloat left_volume_delta = volume->left_delta;
			    gfloat right_volume_delta = volume->right_delta; )
    IF_MIX_RATE		 (  gint32 rate_frac = rate->frac;
			    gint32 rate_step = rate->step;		     )
    IF_MIX_RATE /*FIXME:_DELTA*/	 (  gint32 rate_delta = rate->delta;		     )
    ;
  
  g_assert (mix_value < mix_bound); // paranoid
  do	/* while mix_value < mix_bound */
    {
      BseSampleValue *src, *src_bound;
      BseMixValue *loop_bound;
#if MIX_WITH_RATE
      /* calculate the number of values to be read from src buffer */
      guint32 x = RIGHT_SHIFT (mix_bound - mix_value, BSE_STEREO_SHIFT);
      guint n_values;
      
      if (mix_value + 1 >= mix_bound)	// FIXME: paranoid
	{
	  g_print ("failed: \"mix_value + 1 < mix_bound\": mix_value=%p mix_bound=%p diff=%u\n",
		   mix_value, mix_bound, mix_bound - mix_value);
	  break;
	  g_assert (mix_value + 1 < mix_bound);
	}
      if (x > 1)
	{
	  /* calculate the position of the last sample value to be read out: */
	  gint64 z = x - 2;
# if MIX_RATE_DELTA
	  /* z = n_iter - 2, last_pos = RIGHT_SHIFT (rate_pos + z * rate_step + RIGHT_SHIFT (rate_delta * z * (z - 1), 1), 16); */
	  n_values = RIGHT_SHIFT (rate_frac +
				  z * rate_step +
				  rate_delta * (RIGHT_SHIFT (z * (z - 1), 1)),
				  16);
# else /* RATE_CONST */
	  n_values = RIGHT_SHIFT (rate_frac + z * rate_step, 16);
# endif
	}
      else
	n_values = 0;
      source->n_values = n_values + 1; /* read out starts at 0 (or, get us that last value) */

      source->refill (source);
      if (!source->n_values)	/* sample end reached */
	break;
      g_assert (source->n_values <= n_values + 1);	// paranoid
      if (source->n_values < n_values + 1)
	{
	  guint32 j = srcbuffer_size_max_iterations (rate_frac,
						     rate_step,
						     rate_delta,
						     source->n_values - 1);
	  loop_bound = mix_value + (j << BSE_STEREO_SHIFT);
	  g_print ("ADJUST TRIGGERED: %u < %u new: %u (params: r=%u s=%u d=%u lp=%u)\n",
		   source->n_values, n_values + 1, j,
		   rate_frac, rate_step, rate_delta, source->n_values);
	  g_assert (source->bound >= source->start + source->n_tracks*source->n_values);
	  g_assert (loop_bound <= mix_bound);
	}
      else
	{
	  if (0)
	    g_print ("no-adjust: %u < %u (params: r=%u s=%u d=%u lp=%u)\n",
		     source->n_values, n_values + 1,
		     rate_frac, rate_step, rate_delta, source->n_values);
	  loop_bound = mix_bound;
	}
#else	/* !MIX_WITH_RATE */
      loop_bound = mix_bound;
#endif
      src = source->start;
      src_bound = source->bound;	// FIXME: debugging only

      do	/* while mix_value < loop_bound */
	{
	  register BseMixValue left_value, right_value;
#if MIX_WITH_RATE
	  register guint pos_high = rate_frac >> 16;
	  register guint pos_low = rate_frac & 0xffff;

	  rate_frac = pos_low + rate_step;
	  IF_MIX_RATE_DELTA (rate_step += rate_delta);
# if MIX_IPOL_CUBIC /* && WITH_RATE */
	  {
	    register guint cindex = CUBIC_IPOL_RASTER16 (pos_low);
	    register gint32 W3 = bse_cubic_ipol_W3[cindex], W2 = bse_cubic_ipol_W2[cindex];
	    register gint32 W1 = bse_cubic_ipol_W1[cindex], W0 = bse_cubic_ipol_W0[cindex];
#  if MIX_STEREO /* && IPOL_CUBIC && WITH_RATE */
	    register BseMixValue l_V0 = src[-2], l_V1 = src[0], l_V2 = src[2], l_V3 = src[4];
	    register BseMixValue r_V0 = src[-1], r_V1 = src[1], r_V2 = src[3], r_V3 = src[5];

	    right_value = W3 * r_V3 + W2 * r_V2 + W1 * r_V1 + W0 * r_V0;
#  else	 /* MONO && IPOL_CUBIC && WITH_RATE */
	    register BseMixValue l_V0 = src[-1], l_V1 = src[0], l_V2 = src[1], l_V3 = src[2];
#  endif /* remain: IPOL_CUBIC && WITH_RATE */
            left_value = W3 * l_V3 + W2 * l_V2 + W1 * l_V1 + W0 * l_V0;
	  }
# elif MIX_IPOL_LINEAR /* && WITH_RATE */
	  {
#  if MIX_STEREO /* && IPOL_LINEAR && WITH_RATE */
	    register BseMixValue l_V1 = src[0], r_V1 = src[1], l_V2 = src[2], r_V2 = src[3];

	    right_value = (LEFT_SHIFT (r_V1, BSE_SAMPLE_SHIFT)) + (r_V2 - r_V1) * pos_low;
#  else  /* MONO && IPOL_LINEAR && WITH_RATE */
	    register BseMixValue l_V1 = src[0], l_V2 = src[1];
#  endif /* remain: IPOL_LINEAR && WITH_RATE */
	    left_value = (LEFT_SHIFT (l_V1, BSE_SAMPLE_SHIFT)) + (l_V2 - l_V1) * pos_low;
	  }
# else  /* !WITH_IPOL && WITH_RATE */
	  left_value = src[0];
	  IF_MIX_STEREO (right_value = src[1]);
# endif /* remain: WITH_RATE */
	  g_assert (src < src_bound); IF_MIX_STEREO (g_assert (src + 1 < src_bound); ); // paranoid
	  INC (src, pos_high);	IF_MIX_STEREO (  INC (src, pos_high);  );
#else  /* !WITH_RATE */
	  left_value = *src;	INC (src, 1);
	  IF_MIX_STEREO (right_value = *src; INC (src, 1); );
#endif /* remain: none */
	  IF_MIX_MONO (right_value = left_value);

#if	MIX_VOLUME1
	  DO                   (  left_value *= mix_volume;        );
	  IF_MIX_STEREO        (  right_value *= mix_volume;       );
	  IF_MIX_MONO          (  right_value = left_value;        );
	  IF_MIX_VOLUME1_DELTA (  mix_volume += mix_volume_delta;  );
#elif	MIX_VOLUME2
	  DO                   (  left_value *= left_volume;           );
	  DO                   (  right_value *= right_volume;         );
	  IF_MIX_VOLUME2_DELTA (  left_volume += left_volume_delta;
				  right_volume += right_volume_delta;  );
#endif

	  /* post interpolation correction (shift *after* the volume multiplication) */
#if	MIX_WITH_IPOL
	  left_value = RIGHT_SHIFT (left_value, POST_IPOL_SHIFT_BITS);
# if	MIX_STEREO || MIX_VOLUME2
	  right_value = RIGHT_SHIFT (right_value, POST_IPOL_SHIFT_BITS);
# else	/* WITH_IPOL && !MIX_STEREO && !MIX_VOLUME2)  [not the same as MIX_MONO] */
	  right_value = left_value;
# endif
#endif
	  g_assert (mix_value < mix_bound); // paranoid
	  *mix_value++ += left_value;
	  g_assert (mix_value < mix_bound); // paranoid
	  *mix_value++ += right_value;
	}
      while (mix_value < loop_bound);

      /* post adjustments */
      source->start = src;
      if (source->run_limit)
	{
	  source->max_run_values -= RIGHT_SHIFT (mix_value - run_limit_start, BSE_STEREO_SHIFT);
	  g_assert (source->max_run_values >= 0); // paranoid
	  run_limit_start = mix_value;
	}
    }
  while (mix_value < mix_bound);

  /* post adjustments */
  IF_MIX_VOLUME1_DELTA	(  volume->left = mix_volume;     );
  IF_MIX_VOLUME2_DELTA	(  volume->left = left_volume;
			   volume->right = right_volume;  );
  IF_MIX_RATE		(  rate->frac = rate_frac;	  );
  IF_MIX_RATE_DELTA	(  rate->step = rate_step;	  );
  buffer->start = mix_value;
}
