/* BseWaveOsc - BSE Wave Oscillator
 * Copyright (C) 2001 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#define WMOD_MIX_WITH_SYNC		(1)
#define WMOD_MIX_WITH_FREQ		(2)
#define CHECK_SYNC			(WMOD_MIX_VARIANT & WMOD_MIX_WITH_SYNC)
#define CHECK_FREQ			(WMOD_MIX_VARIANT & WMOD_MIX_WITH_FREQ)
#define	DIRSTRIDE			(1)
#define RAISING_EDGE(v1,v2)		((v1) < (v2))
#define	FREQ_CHANGED(v1,v2)		(fabs (v1 - v2) > 1e-3)

static void
WMOD_MIX_VARIANT_NAME (GslModule *module,
		       guint      n_values)
{
  WaveOscModule *wmod = module->user_data;
  const gfloat *freq_in = GSL_MODULE_IBUFFER (module, BSE_WAVE_OSC_ICHANNEL_FREQ);
  const gfloat *sync_in = GSL_MODULE_IBUFFER (module, BSE_WAVE_OSC_ICHANNEL_SYNC);
  gfloat *wave_boundary, *wave_out = GSL_MODULE_OBUFFER (module, BSE_WAVE_OSC_OCHANNEL_WAVE);
  gfloat last_sync_level = wmod->last_sync_level;
  gfloat last_freq_level = wmod->last_freq_level;
  GslWaveChunkBlock *block = &wmod->block;
  gdouble *a = wmod->a, *b = wmod->b, *y = wmod->y;
  gfloat *boundary = block->end;
  guint wmod_j = wmod->j;
  
  if (!wmod->vars.index)
    {
      module->ostreams[0].values = gsl_engine_const_values (0);
      module->ostreams[1].values = gsl_engine_const_values (0);
      module->ostreams[2].values = gsl_engine_const_values (0);
      return;
    }
  /* sync */
  module->ostreams[1].values = gsl_engine_const_values (0.0); /* need output sync impl */
  /* gate */
  module->ostreams[2].values = gsl_engine_const_values (block->is_silent ? 0.0 : 1.0); /* want output gate? */
  
  /* do the mixing */
  wave_boundary = wave_out + n_values;
  do
    {
      gfloat ffrac;

      if (CHECK_FREQ)
	{
	  gfloat freq_level = *freq_in++;

	  freq_level *= BSE_MAX_FREQUENCY_d;
	  if_reject (FREQ_CHANGED (last_freq_level, freq_level))
	    wmod_set_freq (wmod, freq_level, FALSE);
	  last_freq_level = freq_level;
	}
      if (CHECK_SYNC)
	{
	  gfloat sync_level = *sync_in++;

	  if_reject (RAISING_EDGE (last_sync_level, sync_level))
	    {
	      block->next_offset = 0;
	      wmod_set_freq (wmod, wmod->play_freq, TRUE);
	      wmod->x = boundary;
	    }
#if 0
	  if ((last_sync_level < 0.5 && sync_level > 0.5) ||
	      (sync_level < 0.5 && last_sync_level > 0.5))
	    {
	      if (sync_level < 0.5)
		{
		  block->next_offset = -256;
		  block->play_dir = -1;
		}
	      else
		{
		  block->next_offset = 0;
		  block->play_dir = 1;
		}
	      wmod_set_freq (wmod, wmod->play_freq, TRUE);
	      wmod->x = boundary;
	    }
#endif
	  last_sync_level = sync_level;
	}

      /* process filter while necesary */
      while (wmod->cur_pos >= (FRAC_MASK + 1) << 1)
	{
	  gfloat c, c0, c1, c2, c3, c4, c5, c6, c7, c8;
	  gfloat d, d0, d1, d2, d3, d4, d5, d6, d7;
	  gfloat *x;
	  
	  if_reject (wmod->x == boundary)	/* wchunk block boundary */
	    {
	      GslLong next_offset = block->next_offset;
	      
	      gsl_wave_chunk_unuse_block (wmod->wchunk, block);
	      block->offset = next_offset;
	      gsl_wave_chunk_use_block (wmod->wchunk, block);
	      wmod->x = block->start;
	      boundary = block->end;
	      g_assert (ABS (block->dirstride) == 1);	/* paranoid */
	    }

	  if_expect (block->dirstride > 0)
	    {
	      x = wmod->x;
	      d0 = b[0] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      d1 = b[1] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      d2 = b[2] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      d3 = b[3] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      d4 = b[4] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      d5 = b[5] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      d6 = b[6] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      d7 = b[7] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      c8 = a[8] * x[-4 * DIRSTRIDE];
	      c6 = a[6] * x[-3 * DIRSTRIDE];
	      c4 = a[4] * x[-2 * DIRSTRIDE];
	      c2 = a[2] * x[-1 * DIRSTRIDE];
	      c0 = a[0] * x[0 * DIRSTRIDE];
	      d = d0 + d1 + d2 + d3 + d4 + d5 + d6 + d7;
	      c = c0 + c2 + c4 + c6 + c8;
	      y[wmod_j] = c - d; wmod_j++; wmod_j &= 0x7;
	      d0 = b[0] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      d1 = b[1] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      d2 = b[2] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      d3 = b[3] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      d4 = b[4] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      d5 = b[5] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      d6 = b[6] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      d7 = b[7] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      c7 = a[7] * x[-3 * DIRSTRIDE];
	      c5 = a[5] * x[-2 * DIRSTRIDE];
	      c3 = a[3] * x[-1 * DIRSTRIDE];
	      c1 = a[1] * x[0 * DIRSTRIDE];
	      d = d0 + d1 + d2 + d3 + d4 + d5 + d6 + d7;
	      c = c1 + c3 + c5 + c7;
	      y[wmod_j] = c - d; wmod_j++; wmod_j &= 0x7;
	      wmod->x += DIRSTRIDE;
	    }
	  else /* dirstride < 0 */
	    {
	      x = wmod->x;
	      d0 = b[0] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      d1 = b[1] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      d2 = b[2] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      d3 = b[3] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      d4 = b[4] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      d5 = b[5] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      d6 = b[6] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      d7 = b[7] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      c8 = a[8] * x[-4 * -DIRSTRIDE];
	      c6 = a[6] * x[-3 * -DIRSTRIDE];
	      c4 = a[4] * x[-2 * -DIRSTRIDE];
	      c2 = a[2] * x[-1 * -DIRSTRIDE];
	      c0 = a[0] * x[0 * -DIRSTRIDE];
	      d = d0 + d1 + d2 + d3 + d4 + d5 + d6 + d7;
	      c = c0 + c2 + c4 + c6 + c8;
	      y[wmod_j] = c - d; wmod_j++; wmod_j &= 0x7;
	      d0 = b[0] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      d1 = b[1] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      d2 = b[2] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      d3 = b[3] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      d4 = b[4] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      d5 = b[5] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      d6 = b[6] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      d7 = b[7] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	      c7 = a[7] * x[-3 * -DIRSTRIDE];
	      c5 = a[5] * x[-2 * -DIRSTRIDE];
	      c3 = a[3] * x[-1 * -DIRSTRIDE];
	      c1 = a[1] * x[0 * -DIRSTRIDE];
	      d = d0 + d1 + d2 + d3 + d4 + d5 + d6 + d7;
	      c = c1 + c3 + c5 + c7;
	      y[wmod_j] = c - d; wmod_j++; wmod_j &= 0x7;
	      wmod->x += -DIRSTRIDE;
	    }

	  wmod->cur_pos -= (FRAC_MASK + 1) << 1;
	}

      /* interpolate filter output from current pos
       * wmod->cur_pos >> FRAC_SHIFT is 1 or 0;
       */
      if (wmod->cur_pos >> FRAC_SHIFT)
	{
	  guint k = wmod_j - 2;

	  ffrac = wmod->cur_pos & FRAC_MASK;	/* int -> float */
	  ffrac *= 1. / (FRAC_MASK + 1.);
	  *wave_out++ = y[k & 0x7] * (1.0 - ffrac) + y[(k + 1) & 0x7] * ffrac;
	}
      else
	{
	  guint k = wmod_j - 3;
	  
	  ffrac = wmod->cur_pos;		/* int -> float */
	  ffrac *= 1. / (FRAC_MASK + 1.);
	  *wave_out++ = y[k & 0x7] * (1.0 - ffrac) + y[(k + 1) & 0x7] * ffrac;
	}
      
      /* increment */
      wmod->cur_pos += wmod->istep;
    }
  while (wave_out < wave_boundary);
  wmod->j = wmod_j;
  wmod->last_sync_level = last_sync_level;
  wmod->last_freq_level = last_freq_level;
  // g_print ("is_silent %d %lx\n", block->is_silent, block->offset);
}

#undef WMOD_MIX_WITH_SYNC
#undef WMOD_MIX_WITH_FREQ
#undef CHECK_SYNC
#undef CHECK_FREQ
#undef DIRSTRIDE
#undef RAISING_EDGE
#undef FREQ_CHANGED
