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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "bsewaveosc.h"

#include <bse/gslengine.h>
#include <bse/gslwavechunk.h>
#include <bse/gslfilter.h>



/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_WAVE,
  PARAM_FM_PERC
};


/* --- prototypes --- */
static void	bse_wave_osc_init		(BseWaveOsc		*wosc);
static void	bse_wave_osc_class_init		(BseWaveOscClass	*class);
static void	bse_wave_osc_class_finalize	(BseWaveOscClass	*class);
static void	bse_wave_osc_destroy		(BseObject		*object);
static void	bse_wave_osc_set_property	(BseWaveOsc		*wosc,
						 guint			 param_id,
						 GValue			*value,
						 GParamSpec		*pspec);
static void	bse_wave_osc_get_property	(BseWaveOsc		*wosc,
						 guint			 param_id,
						 GValue			*value,
						 GParamSpec		*pspec);
static void     bse_wave_osc_prepare		(BseSource		*source);
static void	bse_wave_osc_context_create	(BseSource		*source,
						 guint			 context_handle,
						 GslTrans		*trans);
static void     bse_wave_osc_reset		(BseSource		*source);
static void	bse_wave_osc_update_modules	(BseWaveOsc		*wosc);


/* --- variables --- */
static GType		 type_id_wave_osc = 0;
static gpointer		 parent_class = NULL;
static const GTypeInfo type_info_wave_osc = {
  sizeof (BseWaveOscClass),
  
  (GBaseInitFunc) NULL,
  (GBaseFinalizeFunc) NULL,
  (GClassInitFunc) bse_wave_osc_class_init,
  (GClassFinalizeFunc) bse_wave_osc_class_finalize,
  NULL /* class_data */,
  
  sizeof (BseWaveOsc),
  0 /* n_preallocs */,
  (GInstanceInitFunc) bse_wave_osc_init,
};


/* --- functions --- */
static void
bse_wave_osc_class_init (BseWaveOscClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ochannel, ichannel;
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = (GObjectSetPropertyFunc) bse_wave_osc_set_property;
  gobject_class->get_property = (GObjectGetPropertyFunc) bse_wave_osc_get_property;

  object_class->destroy = bse_wave_osc_destroy;
  
  source_class->prepare = bse_wave_osc_prepare;
  source_class->context_create = bse_wave_osc_context_create;
  source_class->reset = bse_wave_osc_reset;
  
  bse_object_class_add_param (object_class, "Wave",
			      PARAM_WAVE,
			      g_param_spec_object ("wave", "Wave", "Wave to play",
						   BSE_TYPE_WAVE,
						   BSE_PARAM_DEFAULT));
  
  ichannel = bse_source_class_add_ichannel (source_class, "freq_in", "Frequency Input");
  g_assert (ichannel == BSE_WAVE_OSC_ICHANNEL_FREQ);
  ichannel = bse_source_class_add_ichannel (source_class, "sync_in", "Syncronization Input");
  g_assert (ichannel == BSE_WAVE_OSC_ICHANNEL_SYNC);
  ochannel = bse_source_class_add_ochannel (source_class, "wave_out", "Wave Output");
  g_assert (ochannel == BSE_WAVE_OSC_OCHANNEL_WAVE);
  ochannel = bse_source_class_add_ochannel (source_class, "sync_out", "Syncronization Output");
  g_assert (ochannel == BSE_WAVE_OSC_OCHANNEL_SYNC);
  ochannel = bse_source_class_add_ochannel (source_class, "gate_out", "Gate Output");
  g_assert (ochannel == BSE_WAVE_OSC_OCHANNEL_GATE);
}

static void
bse_wave_osc_class_finalize (BseWaveOscClass *class)
{
}

static void
bse_wave_osc_init (BseWaveOsc *wosc)
{
  wosc->wave = NULL;
  wosc->vars.start_offset = 0;
  wosc->vars.play_dir = 1;
  wosc->vars.index = NULL;
}

static void
bse_wave_osc_destroy (BseObject *object)
{
  BseWaveOsc *wosc = BSE_WAVE_OSC (object);

  if (wosc->wave)
    {
      g_object_unref (wosc->wave);
      wosc->wave = NULL;
    }

  /* chain parent class' handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
notify_wave_changed (BseWaveOsc *wosc)
{
  g_object_notify (G_OBJECT (wosc), "wave");
}

static void
wave_uncross (BseItem *owner,
	      BseItem *ref_item)
{
  BseWaveOsc *wosc = BSE_WAVE_OSC (owner);

  wosc->wave = NULL;
  wosc->vars.index = NULL;
  g_object_notify (G_OBJECT (wosc), "wave");

  bse_wave_osc_update_modules (wosc);
  if (BSE_SOURCE_PREPARED (wosc))
    {
      /* need to make sure our modules know about BseWave vanishing
       * before we return
       */
      gsl_engine_wait_on_trans ();
    }
}

static void
bse_wave_osc_set_property (BseWaveOsc  *wosc,
			   guint        param_id,
			   GValue      *value,
			   GParamSpec  *pspec)
{
  switch (param_id)
    {
    case PARAM_WAVE:
      if (wosc->wave)
	{
	  g_object_disconnect (wosc->wave,
			       "any_signal", notify_wave_changed, wosc,
			       NULL);
	  bse_item_cross_unref (BSE_ITEM (wosc), BSE_ITEM (wosc->wave));
	  wosc->wave = NULL;
	  wosc->vars.index = NULL;
	}
      wosc->wave = g_value_get_object (value);
      if (wosc->wave)
	{
	  bse_item_cross_ref (BSE_ITEM (wosc), BSE_ITEM (wosc->wave), wave_uncross);
	  g_object_connect (wosc->wave,
			    "swapped_signal::notify::name", notify_wave_changed, wosc,
			    NULL);
	  if (BSE_SOURCE_PREPARED (wosc))
	    wosc->vars.index = bse_wave_get_index_for_modules (wosc->wave);
	}
      bse_wave_osc_update_modules (wosc);
      if (BSE_SOURCE_PREPARED (wosc))
	{
	  /* need to make sure our modules know about BseWave vanishing
	   * before we return
	   */
	  gsl_engine_wait_on_trans ();
	}
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (wosc, param_id, pspec);
      break;
    }
}

static void
bse_wave_osc_get_property (BseWaveOsc   *wosc,
			   guint        param_id,
			   GValue      *value,
			   GParamSpec  *pspec)
{
  switch (param_id)
    {
    case PARAM_WAVE:
      g_value_set_object (value, wosc->wave);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (wosc, param_id, pspec);
      break;
    }
}

static void
bse_wave_osc_prepare (BseSource *source)
{
#if 0 // shame, BseWave might not be prepared already, so defer to context_create
  BseWaveOsc *wosc = BSE_WAVE_OSC (source);

  wosc->vars.index = wosc->wave ? bse_wave_get_index_for_modules (wosc->wave) : NULL;
#endif

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

#define	ORDER	8	/* <= BSE_MAX_BLOCK_PADDING !! */

typedef struct
{
  BseWaveOscVars    vars;
  gfloat	    play_freq, last_sync_level, last_freq_level;
  GslWaveChunk     *wchunk;
  gfloat            zero_padding, part_step;
  GslWaveChunkBlock block;
  gfloat           *x;			/* pointer into block */
  guint             cur_pos, istep;
  gdouble           a[ORDER + 1];	/* order */
  gdouble           b[ORDER + 1];	/* reversed order */
  gdouble           y[ORDER + 1];
  guint             j;			/* y[] index */
} WaveOscModule;
#define FRAC_SHIFT	(16)
#define FRAC_MASK	((1 << FRAC_SHIFT) - 1)

static inline void
wmod_set_freq (WaveOscModule *wmod,
	       gfloat	      play_freq,
	       gboolean	      change_wave)
{
  gfloat step;
  guint i, istep;

  wmod->play_freq = play_freq;
  if (!wmod->vars.index)
    return;

  if (change_wave)
    {
      if (wmod->wchunk)
	gsl_wave_chunk_unuse_block (wmod->wchunk, &wmod->block);
      wmod->wchunk = bse_wave_index_lookup_best (wmod->vars.index, wmod->play_freq);
      wmod->zero_padding = 2;
      wmod->part_step = wmod->zero_padding * wmod->wchunk->mix_freq;
      wmod->part_step /= wmod->wchunk->osc_freq * BSE_MIX_FREQ_d;
      g_print ("wave lookup: want=%f got=%f length=%lu\n",
	       play_freq, wmod->wchunk->osc_freq, wmod->wchunk->wave_length);
      wmod->block.offset = wmod->vars.start_offset;
      gsl_wave_chunk_use_block (wmod->wchunk, &wmod->block);
      // wmod->last_sync_level = 0;
      // wmod->last_freq_level = 0;
      wmod->x = wmod->block.start;
    }
  step = wmod->part_step * wmod->play_freq;
  istep = step * (FRAC_MASK + 1.) + 0.5;

  if (istep != wmod->istep)
    {
      gfloat nyquist_fact = GSL_PI / 22050., cutoff_freq = 18000, stop_freq = 24000;
      gfloat filt_fact = CLAMP (1. / step, 1. / (6. * wmod->zero_padding), 1. / wmod->zero_padding);
      gfloat freq_c = cutoff_freq * nyquist_fact * filt_fact;
      gfloat freq_r = stop_freq * nyquist_fact * filt_fact;

      wmod->istep = istep;
      // FIXME: gsl_filter_tscheb2 (ORDER, freq_c, freq_r, gsl_trans_zepsilon2ss (0.18), wmod->a, wmod->b);
      gsl_filter_tscheb1_lp (ORDER, freq_c, gsl_trans_zepsilon2ss (0.18), wmod->a, wmod->b);
      for (i = 0; i < ORDER + 1; i++)
	wmod->a[i] *= wmod->zero_padding;	/* scale to compensate for zero-padding */
      for (i = 0; i < (ORDER + 1) / 2; i++)	/* reverse bs */
	{
	  gfloat t = wmod->b[ORDER - i];
	  
	  wmod->b[ORDER - i] = wmod->b[i];
	  wmod->b[i] = t;
	}
      g_print ("filter: fc=%f fr=%f st=%f is=%u\n", freq_c/GSL_PI*2, freq_r/GSL_PI*2, step, wmod->istep);
    }
}

static void
wmod_access (GslModule *module,
	     gpointer   data)
{
  WaveOscModule *wmod = module->user_data;
  BseWaveOscVars *vars = data;

  if (wmod->vars.index != vars->index)
    {
      if (wmod->wchunk)
	gsl_wave_chunk_unuse_block (wmod->wchunk, &wmod->block);
      wmod->wchunk = NULL;
    }
  wmod->vars = *vars;
  wmod_set_freq (wmod, wmod->play_freq, wmod->wchunk == NULL);
}

static void
wmod_free (gpointer        data,
	   const GslClass *klass)
{
  WaveOscModule *wmod = data;

  if (wmod->vars.index && wmod->wchunk)
    gsl_wave_chunk_unuse_block (wmod->wchunk, &wmod->block);
  g_free (wmod);
}

static void
bse_wave_osc_update_modules (BseWaveOsc *wosc)
{
  if (BSE_SOURCE_PREPARED (wosc))
    bse_source_access_omodules (BSE_SOURCE (wosc),
				BSE_WAVE_OSC_OCHANNEL_WAVE,
				wmod_access,
				g_memdup (&wosc->vars, sizeof (wosc->vars)),
				g_free,
				NULL);
}

#define WMOD_MIX_VARIANT_NAME	wmod_mix_nofreq_nosync
#define WMOD_MIX_VARIANT	(0)
#include "bsewaveosc-aux.c"
#undef  WMOD_MIX_VARIANT
#undef  WMOD_MIX_VARIANT_NAME

#define WMOD_MIX_VARIANT_NAME	wmod_mix_nofreq_sync
#define WMOD_MIX_VARIANT	(WMOD_MIX_WITH_SYNC)
#include "bsewaveosc-aux.c"
#undef  WMOD_MIX_VARIANT
#undef  WMOD_MIX_VARIANT_NAME

#define WMOD_MIX_VARIANT_NAME	wmod_mix_freq_nosync
#define WMOD_MIX_VARIANT	(WMOD_MIX_WITH_FREQ)
#include "bsewaveosc-aux.c"
#undef  WMOD_MIX_VARIANT
#undef  WMOD_MIX_VARIANT_NAME

#define WMOD_MIX_VARIANT_NAME	wmod_mix_freq_sync
#define WMOD_MIX_VARIANT	(WMOD_MIX_WITH_SYNC | WMOD_MIX_WITH_FREQ)
#include "bsewaveosc-aux.c"
#undef  WMOD_MIX_VARIANT
#undef  WMOD_MIX_VARIANT_NAME


static void
wmod_process (GslModule *module,
	      guint      n_values)
{
  if (module->istreams[BSE_WAVE_OSC_ICHANNEL_FREQ].connected)
    {
      if (module->istreams[BSE_WAVE_OSC_ICHANNEL_SYNC].connected)
	wmod_mix_freq_sync (module, n_values);
      else /* nosync */
	wmod_mix_freq_nosync (module, n_values);
    }
  else /* nofreq */
    {
      if (module->istreams[BSE_WAVE_OSC_ICHANNEL_SYNC].connected)
	wmod_mix_nofreq_sync (module, n_values);
      else /* nosync */
	wmod_mix_nofreq_nosync (module, n_values);
    }
  
#if 0
  WaveOscModule *wmod = module->user_data;
  // const BseSampleValue *freq_in = GSL_MODULE_IBUFFER (module, 0);
  // const BseSampleValue *mod_in = GSL_MODULE_IBUFFER (module, 1);
  // const BseSampleValue *sync_in = GSL_MODULE_IBUFFER (module, 2);
  gfloat *wave_boundary, *wave_out = GSL_MODULE_OBUFFER (module, 0);
  gboolean with_self_mod = wmod->vars.with_self_mod;
  gfloat fm_strength = wmod->vars.freq_mod;
  gfloat self_strength = wmod->vars.self_mod;
  GslWaveChunkBlock *block = &wmod->block;
  gdouble *a = wmod->a, *b = wmod->b, *y = wmod->y;
  guint istep = wmod->istep;
  gfloat ffrac, *boundary = block->end;
  gint dirstride = block->dirstride;
  guint wmod_j = wmod->j;
  
  if (!wmod->vars.index)
    {
      module->ostreams[0].values = gsl_engine_const_values (0);
      module->ostreams[1].values = gsl_engine_const_values (0);
      module->ostreams[2].values = gsl_engine_const_values (0);
      return;
    }
  /* sync */
  module->ostreams[1].values = gsl_engine_const_values (0.0);
  /* gate */
  module->ostreams[2].values = gsl_engine_const_values (block->is_silent ? 0.0 : 1.0);
  
  /* do the mixing */
  wave_boundary = wave_out + n_values;
  do
    {
      /* process filter while necesary */
      while (wmod->cur_pos >= (FRAC_MASK + 1) << 1)
	{
	  gfloat c, c0, c1, c2, c3, c4, c5, c6, c7, c8;
	  gfloat d, d0, d1, d2, d3, d4, d5, d6, d7;
	  gfloat *x;
	  
	  if (wmod->x == boundary)	/* wchunk block boundary */
	    {
	      GslLong next_offset = block->next_offset;
	      
	      gsl_wave_chunk_unuse_block (wmod->wchunk, block);
	      block->offset = next_offset;
	      gsl_wave_chunk_use_block (wmod->wchunk, block);
	      wmod->x = block->start;
	      boundary = block->end;
	      dirstride = block->dirstride;
	    }
	  
	  g_assert (dirstride == 1);
	  g_assert (ORDER == 8);
	  
	  x = wmod->x;
	  d0 = b[0] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	  d1 = b[1] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	  d2 = b[2] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	  d3 = b[3] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	  d4 = b[4] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	  d5 = b[5] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	  d6 = b[6] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	  d7 = b[7] * y[wmod_j]; wmod_j++; wmod_j &= 0x7;
	  c8 = a[8] * x[-4];
	  c6 = a[6] * x[-3];
	  c4 = a[4] * x[-2];
	  c2 = a[2] * x[-1];
	  c0 = a[0] * x[0];
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
	  c7 = a[7] * x[-3];
	  c5 = a[5] * x[-2];
	  c3 = a[3] * x[-1];
	  c1 = a[1] * x[0];
	  d = d0 + d1 + d2 + d3 + d4 + d5 + d6 + d7;
	  c = c1 + c3 + c5 + c7;
	  y[wmod_j] = c - d; wmod_j++; wmod_j &= 0x7;
	  wmod->x += 1;	/* dirstride == 1 !! */
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
      wmod->cur_pos += istep;
    }
  while (wave_out < wave_boundary);
  wmod->j = wmod_j;
#endif
}

static void
bse_wave_osc_context_create (BseSource *source,
			     guint      context_handle,
			     GslTrans  *trans)
{
  static const GslClass wmod_class = {
    3,				/* n_istreams */
    0,                          /* n_jstreams */
    3,				/* n_ostreams */
    wmod_process,		/* process */
    wmod_free,			/* free */
    GSL_COST_NORMAL,		/* cost */
  };
  BseWaveOsc *wosc = BSE_WAVE_OSC (source);
  WaveOscModule *wmod = g_new0 (WaveOscModule, 1);
  GslModule *module;

  if (!wosc->vars.index && wosc->wave)	/* BseWave is prepared now */
    wosc->vars.index = bse_wave_get_index_for_modules (wosc->wave);

  wmod->vars = wosc->vars;
  wmod->last_sync_level = 0;
  wmod->last_freq_level = 0;
  wmod->wchunk = NULL;
  wmod->play_freq = 0;
  wmod->block.play_dir = wmod->vars.play_dir;
  wmod->block.offset = wmod->vars.start_offset;
  wmod_set_freq (wmod, wmod->play_freq, TRUE);

  module = gsl_module_new (&wmod_class, wmod);

  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);

  /* commit module to engine */
  gsl_trans_add (trans, gsl_job_integrate (module));

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}

static void
bse_wave_osc_reset (BseSource *source)
{
  BseWaveOsc *wosc = BSE_WAVE_OSC (source);

  wosc->vars.index = NULL;

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}


/* --- Export to BSE --- */
#include "./icons/waveosc.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_wave_osc, "BseWaveOsc", "BseSource",
    "BseWaveOsc is a waveeric oscillator source",
    &type_info_wave_osc,
    "/Source/Oscillators/Wave Oscillator",
    { WAVE_OSC_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      WAVE_OSC_IMAGE_WIDTH, WAVE_OSC_IMAGE_HEIGHT,
      WAVE_OSC_IMAGE_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;
