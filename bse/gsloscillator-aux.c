/* GSL - Generic Sound Layer
 * Copyright (C) 1999, 2000-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */


#define	OSC_FLAGS		(GSL_INCLUDER_CASE | OSC_INCLUDER_FLAGS)
#define ISYNC1_OSYNC0		((OSC_FLAGS & OSC_FLAG_ISYNC) && !(OSC_FLAGS & OSC_FLAG_OSYNC))
#define ISYNC1_OSYNC1		((OSC_FLAGS & OSC_FLAG_ISYNC) && (OSC_FLAGS & OSC_FLAG_OSYNC))
#define ISYNC0_OSYNC1		((OSC_FLAGS & OSC_FLAG_OSYNC) && !(OSC_FLAGS & OSC_FLAG_ISYNC))
#define WITH_OSYNC		(OSC_FLAGS & OSC_FLAG_OSYNC)
#define WITH_FREQ		(OSC_FLAGS & OSC_FLAG_FREQ)
#define WITH_SMOD		(OSC_FLAGS & OSC_FLAG_SELF_MOD)
#define WITH_LMOD		(OSC_FLAGS & OSC_FLAG_LINEAR_MOD)
#define WITH_EMOD		(OSC_FLAGS & OSC_FLAG_EXP_MOD)
#define WITH_PWM_MOD		(OSC_FLAGS & OSC_FLAG_PWM_MOD)
#define PULSE_OSC		(OSC_FLAGS & OSC_FLAG_PULSE_OSC)


static void
GSL_INCLUDER_FUNC (GslOscData   *osc,
		   guint         n_values,
		   const gfloat *ifreq,
		   const gfloat *mod_in,
		   const gfloat *sync_in,
		   const gfloat *pwm_in,
		   gfloat       *mono_out,
		   gfloat       *sync_out)
{
  gfloat last_sync_level = osc->last_sync_level;
  gfloat last_pwm_level = osc->last_pwm_level;
  gdouble last_freq_level = osc->last_freq_level;
  const gdouble transpose = osc->config.transpose_factor;
  const gdouble fine_tune = bse_cent_factor (CLAMP (osc->config.fine_tune, -100, +100));
  guint32 cur_pos = osc->cur_pos;
  guint32 last_pos = osc->last_pos;
  guint32 sync_pos, pos_inc;
  gfloat posm_strength, self_posm_strength;
  gfloat *boundary = mono_out + n_values;
  GslOscWave *wave = &osc->wave;
  
  pos_inc = bse_dtoi (osc->last_freq_level * transpose * fine_tune * wave->freq_to_step);
  sync_pos = osc->config.phase * wave->phase_to_pos;
  posm_strength = pos_inc * osc->config.fm_strength;
  self_posm_strength = pos_inc * osc->config.self_fm_strength;
  
  /* do the mixing */
  do
    {
      gfloat v;
      
      /* handle syncs
       */
#if (ISYNC1_OSYNC0)		/* input sync only */
      {
	gfloat sync_level = *sync_in++;
	if (UNLIKELY (BSE_SIGNAL_RAISING_EDGE (last_sync_level, sync_level)))
	  cur_pos = sync_pos;
	last_sync_level = sync_level;
      }
#elif (ISYNC1_OSYNC1)		/* input and output sync */
      {
	gfloat sync_level = *sync_in++;
	if (UNLIKELY (BSE_SIGNAL_RAISING_EDGE (last_sync_level, sync_level)))
	  {
	    cur_pos = sync_pos;
	    *sync_out++ = 1.0;
	  }
	else /* figure output sync position */
	  {
	    guint is_sync = (sync_pos <= cur_pos) + (last_pos < sync_pos) + (cur_pos < last_pos);
	    *sync_out++ = is_sync >= 2 ? 1.0 : 0.0;
	  }
	last_sync_level = sync_level;
      }
#elif (ISYNC0_OSYNC1)		/* output sync only */
      {
	/* figure output sync position */
	guint is_sync = (sync_pos <= cur_pos) + (last_pos < sync_pos) + (cur_pos < last_pos);
	*sync_out++ = is_sync >= 2 ? 1.0 : 0.0;
      }
#endif
      
      /* track frequency changes
       */
#if (WITH_FREQ)
      {
	gdouble freq_level = *ifreq++;
	freq_level = BSE_SIGNAL_TO_FREQ (freq_level);
	if (UNLIKELY (BSE_SIGNAL_FREQ_CHANGED (last_freq_level, freq_level)))
	  {
            gdouble transposed_freq = transpose * freq_level;
	    if (UNLIKELY (transposed_freq <= wave->min_freq || transposed_freq > wave->max_freq))
	      {
		gdouble fcpos, flpos;
		const gfloat *orig_values = wave->values;

		fcpos = cur_pos * wave->ifrac_to_float;
		flpos = last_pos * wave->ifrac_to_float;
		gsl_osc_table_lookup (osc->config.table, transposed_freq, wave);
		if (orig_values != wave->values)	/* catch non-changes */
		  {
		    last_pos = flpos / wave->ifrac_to_float;
		    cur_pos = fcpos / wave->ifrac_to_float;
		    sync_pos = osc->config.phase * wave->phase_to_pos;
		    pos_inc = bse_dtoi (transposed_freq * fine_tune * wave->freq_to_step);
#if (PULSE_OSC)
		    osc->last_pwm_level = 0;
		    osc_update_pwm_offset (osc, osc->last_pwm_level);
		    last_pwm_level = osc->last_pwm_level;
#endif
		  }
	      }
	    else
	      pos_inc = bse_dtoi (transposed_freq * fine_tune * wave->freq_to_step);
	    posm_strength = pos_inc * osc->config.fm_strength;
	    self_posm_strength = pos_inc * osc->config.self_fm_strength;
	    last_freq_level = freq_level;
	  }
      }
#endif

      /* track pulse witdh modulation
       */
#if (WITH_PWM_MOD)
      {
	gfloat pwm_level = *pwm_in++;
	if (fabs (last_pwm_level - pwm_level) > 1.0 / 65536.0)
	  {
	    last_pwm_level = pwm_level;
	    osc_update_pwm_offset (osc, pwm_level);
	  }
      }
#endif

      /* output signal calculation
       */
#if (PULSE_OSC)		/* pulse width modulation oscillator */
      {
	guint32 tpos, ipos;
	tpos = cur_pos >> wave->n_frac_bits;
	ipos = (cur_pos - osc->pwm_offset) >> wave->n_frac_bits;
	v = wave->values[tpos] - wave->values[ipos];
	v = (v + osc->pwm_center) * osc->pwm_max;
      }
#else			/* table read out and linear ipol */
      {
	guint32 tpos, ifrac;
	gfloat ffrac, w;
	tpos = cur_pos >> wave->n_frac_bits;
	ifrac = cur_pos & wave->frac_bitmask;
	ffrac = ifrac * wave->ifrac_to_float;
	v = wave->values[tpos];
	w = wave->values[tpos + 1];
	v *= 1.0 - ffrac;
	w *= ffrac;
	v += w;
      }
#endif	/* v = value_out done */
      *mono_out++ = v;
      
      /* position increment
       */
#if (WITH_OSYNC)
      last_pos = cur_pos;
#endif
#if (WITH_SMOD)			/* self modulation */
      cur_pos += self_posm_strength * v;
#endif
#if (WITH_LMOD)			/* linear fm */
      {
	gfloat mod_level = *mod_in++;
	cur_pos += pos_inc + posm_strength * mod_level;
      }
#elif (WITH_EMOD)		/* exponential fm */
      {
	gfloat mod_level = *mod_in++;
	cur_pos += pos_inc * bse_approx5_exp2 (osc->config.fm_strength * mod_level);
      }
#else				/* no modulation */
      cur_pos += pos_inc;
#endif
    }
  while (mono_out < boundary);
  
  osc->last_pos = WITH_OSYNC ? last_pos : cur_pos;
  osc->cur_pos = cur_pos;
  osc->last_sync_level = last_sync_level;
  osc->last_freq_level = last_freq_level;
  osc->last_pwm_level = last_pwm_level;
}

#undef ISYNC1_OSYNC0
#undef ISYNC1_OSYNC1
#undef ISYNC0_OSYNC1
#undef WITH_OSYNC
#undef WITH_FREQ
#undef WITH_SMOD
#undef WITH_LMOD
#undef WITH_EMOD
#undef OSC_FLAGS
