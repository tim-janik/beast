/* GSL - Generic Sound Layer
 * Copyright (C) 2001-2002 Tim Janik and Stefan Westerfeld
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


#define CHECK_SYNC		(WOSC_MIX_VARIANT & WOSC_MIX_WITH_SYNC)
#define CHECK_FREQ		(WOSC_MIX_VARIANT & WOSC_MIX_WITH_FREQ)
#define CHECK_MOD		(WOSC_MIX_VARIANT & WOSC_MIX_WITH_MOD)
#define EXPONENTIAL_FM		(WOSC_MIX_VARIANT & WOSC_MIX_WITH_EXP_FM)
#define DIRSTRIDE(b)		(b->dirstride)	 /* changes according to n_channels and directions */


static void
WOSC_MIX_VARIANT_NAME (GslWaveOscData *wosc,
		       guint           n_values,
		       const gfloat   *freq_in,
		       const gfloat   *mod_in,
		       const gfloat   *sync_in,
		       gfloat         *wave_out)
{
  gfloat *wave_boundary;
  gfloat last_sync_level = wosc->last_sync_level;
  gfloat last_freq_level = wosc->last_freq_level;
  gfloat last_mod_level = wosc->last_mod_level;
  GslWaveChunkBlock *block = &wosc->block;
  gdouble *a = wosc->a, *b = wosc->b, *y = wosc->y;
  gfloat *boundary = block->end;
  guint wosc_j = wosc->j;

  /* do the mixing */
  wave_boundary = wave_out + n_values;
  do
    {
      if (CHECK_SYNC)
	{
	  gfloat sync_level = *sync_in++;
	  if (UNLIKELY (BSE_SIGNAL_RAISING_EDGE (last_sync_level, sync_level)))
	    {
	      wosc->j = wosc_j;
	      gsl_wave_osc_retrigger (wosc, CHECK_FREQ ? BSE_SIGNAL_TO_FREQ (*freq_in) : wosc->config.cfreq);
	      /* retrigger alters last_freq and last_mod */
	      last_freq_level = wosc->last_freq_level;
	      last_mod_level = wosc->last_mod_level;
	      wosc_j = wosc->j;
	      boundary = block->end;
	      last_sync_level = sync_level;
	    }
	}
      if (CHECK_MOD && CHECK_FREQ)
	{
	  gfloat mod_level = *mod_in++, freq_level = *freq_in++;
	  if (UNLIKELY (BSE_SIGNAL_FREQ_CHANGED (last_freq_level, freq_level)))
	    {
	      last_freq_level = freq_level;
	      if (BSE_SIGNAL_MOD_CHANGED (last_mod_level, mod_level))
		last_mod_level = mod_level;
	      goto UPDATE_FREQ;
	    }
          else if (UNLIKELY (BSE_SIGNAL_MOD_CHANGED (last_mod_level, mod_level)))
	    {
	      gfloat new_freq;
	      last_mod_level = mod_level;
	    UPDATE_FREQ:
	      new_freq = BSE_SIGNAL_TO_FREQ (freq_level);
	      if (EXPONENTIAL_FM)
		new_freq *= bse_approx5_exp2 (wosc->config.fm_strength * mod_level);
	      else /* LINEAR_FM */
		new_freq *= 1.0 + wosc->config.fm_strength * mod_level;
	      wave_osc_transform_filter (wosc, new_freq);
	    }
	}
      else if (CHECK_MOD)
	{
	  gfloat mod_level = *mod_in++;
	  if (BSE_SIGNAL_MOD_CHANGED (last_mod_level, mod_level))
	    {
	      gfloat new_freq = wosc->config.cfreq;
	      if (EXPONENTIAL_FM)
		new_freq *= bse_approx5_exp2 (wosc->config.fm_strength * mod_level);
	      else /* LINEAR_FM */
		new_freq *= 1.0 + wosc->config.fm_strength * mod_level;
	      last_mod_level = mod_level;
	      wave_osc_transform_filter (wosc, new_freq);
	    }
	}
      else if (CHECK_FREQ)
	{
	  gfloat freq_level = *freq_in++;
	  if (BSE_SIGNAL_FREQ_CHANGED (last_freq_level, freq_level))
	    {
	      last_freq_level = freq_level;
	      wave_osc_transform_filter (wosc, BSE_SIGNAL_TO_FREQ (freq_level));
	    }
	}

      /* process filter while necesary */
      while (wosc->cur_pos >= (FRAC_MASK + 1) << 1)
	{
	  double c, c0, c1, c2, c3, c4, c5, c6, c7, c8;
	  double d, d0, d1, d2, d3, d4, d5, d6, d7;
	  gfloat *x;

	  if (UNLIKELY ((block->dirstride > 0 && wosc->x >= boundary) ||
		        (block->dirstride < 0 && wosc->x <= boundary)))       /* wchunk block boundary */
	    {
	      GslLong next_offset = block->next_offset;

	      gsl_wave_chunk_unuse_block (wosc->wchunk, block);
	      block->play_dir = wosc->config.play_dir;
	      block->offset = next_offset;
	      gsl_wave_chunk_use_block (wosc->wchunk, block);
	      wosc->x = block->start + CLAMP (wosc->config.channel, 0, wosc->wchunk->n_channels - 1);
	      boundary = block->end;
	    }
	  
	  x = wosc->x;
	  d0 = b[0] * y[wosc_j]; wosc_j++; wosc_j &= 0x7;
	  d1 = b[1] * y[wosc_j]; wosc_j++; wosc_j &= 0x7;
	  d2 = b[2] * y[wosc_j]; wosc_j++; wosc_j &= 0x7;
	  d3 = b[3] * y[wosc_j]; wosc_j++; wosc_j &= 0x7;
	  d4 = b[4] * y[wosc_j]; wosc_j++; wosc_j &= 0x7;
	  d5 = b[5] * y[wosc_j]; wosc_j++; wosc_j &= 0x7;
	  d6 = b[6] * y[wosc_j]; wosc_j++; wosc_j &= 0x7;
	  d7 = b[7] * y[wosc_j]; wosc_j++; wosc_j &= 0x7;
	  c8 = a[8] * x[-4 * DIRSTRIDE (block)];
	  c6 = a[6] * x[-3 * DIRSTRIDE (block)];
	  c4 = a[4] * x[-2 * DIRSTRIDE (block)];
	  c2 = a[2] * x[-1 * DIRSTRIDE (block)];
	  c0 = a[0] * x[0 * DIRSTRIDE (block)];
	  d = d0 + d1 + d2 + d3 + d4 + d5 + d6 + d7;
	  c = c0 + c2 + c4 + c6 + c8;
	  y[wosc_j] = c - d; wosc_j++; wosc_j &= 0x7;
	  d0 = b[0] * y[wosc_j]; wosc_j++; wosc_j &= 0x7;
	  d1 = b[1] * y[wosc_j]; wosc_j++; wosc_j &= 0x7;
	  d2 = b[2] * y[wosc_j]; wosc_j++; wosc_j &= 0x7;
	  d3 = b[3] * y[wosc_j]; wosc_j++; wosc_j &= 0x7;
	  d4 = b[4] * y[wosc_j]; wosc_j++; wosc_j &= 0x7;
	  d5 = b[5] * y[wosc_j]; wosc_j++; wosc_j &= 0x7;
	  d6 = b[6] * y[wosc_j]; wosc_j++; wosc_j &= 0x7;
	  d7 = b[7] * y[wosc_j]; wosc_j++; wosc_j &= 0x7;
	  c7 = a[7] * x[-3 * DIRSTRIDE (block)];
	  c5 = a[5] * x[-2 * DIRSTRIDE (block)];
	  c3 = a[3] * x[-1 * DIRSTRIDE (block)];
	  c1 = a[1] * x[0 * DIRSTRIDE (block)];
	  d = d0 + d1 + d2 + d3 + d4 + d5 + d6 + d7;
	  c = c1 + c3 + c5 + c7;
	  y[wosc_j] = c - d; wosc_j++; wosc_j &= 0x7;
	  wosc->x += DIRSTRIDE (block);
	  wosc->cur_pos -= (FRAC_MASK + 1) << 1;
	}

      /* interpolate filter output from current pos
       * wosc->cur_pos >> FRAC_SHIFT is 1 or 0;
       */
      if (wosc->cur_pos >> FRAC_SHIFT)
	{
	  guint k = wosc_j - 2;
	  double ffrac = wosc->cur_pos & FRAC_MASK;    /* int -> float */
	  ffrac *= 1. / (FRAC_MASK + 1.);
	  *wave_out++ = y[k & 0x7] * (1.0 - ffrac) + y[(k + 1) & 0x7] * ffrac;
	}
      else
	{
	  guint k = wosc_j - 3;
	  double ffrac = wosc->cur_pos;                /* int -> float */
	  ffrac *= 1. / (FRAC_MASK + 1.);
	  *wave_out++ = y[k & 0x7] * (1.0 - ffrac) + y[(k + 1) & 0x7] * ffrac;
	}

      /* increment */
      wosc->cur_pos += wosc->istep;
    }
  while (wave_out < wave_boundary);
  wosc->j = wosc_j;
  wosc->last_sync_level = last_sync_level;
  wosc->last_freq_level = last_freq_level;
  wosc->last_mod_level = last_mod_level;
}

#undef CHECK_SYNC
#undef CHECK_FREQ
#undef CHECK_MOD
#undef EXPONENTIAL_FM
#undef DIRSTRIDE

#undef WOSC_MIX_VARIANT
#undef WOSC_MIX_VARIANT_NAME
