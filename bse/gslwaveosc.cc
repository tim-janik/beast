// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "gslwaveosc.hh"
#include "gslfilter.hh"
#include "bsemathsignal.hh"
#include "bseengine.hh"	/* for bse_engine_sample_freq() */
#include "bsemain.hh"
#include <string.h>

#define WDEBUG(...)     Bse::debug ("waveosc", __VA_ARGS__)

#define FRAC_SHIFT		(16)
#define FRAC_MASK		((1 << FRAC_SHIFT) - 1)
#define	SIGNAL_LEVEL_INVAL	(-2.0)	/* trigger level-changed checks */

/* --- prototype --- */
static void	wave_osc_transform_filter	(GslWaveOscData *wosc,
						 gfloat          play_freq);
/* --- generated function variants --- */
#define WOSC_MIX_VARIANT_INVAL  (0xffffffff)
#define WOSC_MIX_WITH_SYNC      (1)
#define WOSC_MIX_WITH_FREQ      (2)
#define WOSC_MIX_WITH_MOD       (4)
#define WOSC_MIX_WITH_EXP_FM    (8)
#define WOSC_MIX_VARIANT_NAME	wosc_process_sfme
#define WOSC_MIX_VARIANT	(WOSC_MIX_WITH_SYNC | WOSC_MIX_WITH_FREQ | WOSC_MIX_WITH_MOD | WOSC_MIX_WITH_EXP_FM)
#include "gslwaveosc-aux.cc"
#define WOSC_MIX_VARIANT_NAME	wosc_process_sfm_
#define WOSC_MIX_VARIANT	(WOSC_MIX_WITH_SYNC | WOSC_MIX_WITH_FREQ | WOSC_MIX_WITH_MOD | 0                   )
#include "gslwaveosc-aux.cc"
#if 0
#define WOSC_MIX_VARIANT_NAME	wosc_process_sf_e
#define WOSC_MIX_VARIANT	(WOSC_MIX_WITH_SYNC | WOSC_MIX_WITH_FREQ | 0                 | WOSC_MIX_WITH_EXP_FM)
#include "gslwaveosc-aux.cc"
#endif
#define WOSC_MIX_VARIANT_NAME	wosc_process_sf__
#define WOSC_MIX_VARIANT	(WOSC_MIX_WITH_SYNC | WOSC_MIX_WITH_FREQ | 0                 | 0                   )
#include "gslwaveosc-aux.cc"
#define WOSC_MIX_VARIANT_NAME	wosc_process_s_me
#define WOSC_MIX_VARIANT	(WOSC_MIX_WITH_SYNC | 0                  | WOSC_MIX_WITH_MOD | WOSC_MIX_WITH_EXP_FM)
#include "gslwaveosc-aux.cc"
#define WOSC_MIX_VARIANT_NAME	wosc_process_s_m_
#define WOSC_MIX_VARIANT	(WOSC_MIX_WITH_SYNC | 0                  | WOSC_MIX_WITH_MOD | 0                   )
#include "gslwaveosc-aux.cc"
#if 0
#define WOSC_MIX_VARIANT_NAME	wosc_process_s__e
#define WOSC_MIX_VARIANT	(WOSC_MIX_WITH_SYNC | 0                  | 0                 | WOSC_MIX_WITH_EXP_FM)
#include "gslwaveosc-aux.cc"
#endif
#define WOSC_MIX_VARIANT_NAME	wosc_process_s___
#define WOSC_MIX_VARIANT	(WOSC_MIX_WITH_SYNC | 0                  | 0                 | 0                   )
#include "gslwaveosc-aux.cc"
#define WOSC_MIX_VARIANT_NAME	wosc_process__fme
#define WOSC_MIX_VARIANT	(0                  | WOSC_MIX_WITH_FREQ | WOSC_MIX_WITH_MOD | WOSC_MIX_WITH_EXP_FM)
#include "gslwaveosc-aux.cc"
#define WOSC_MIX_VARIANT_NAME	wosc_process__fm_
#define WOSC_MIX_VARIANT	(0                  | WOSC_MIX_WITH_FREQ | WOSC_MIX_WITH_MOD | 0                   )
#include "gslwaveosc-aux.cc"
#if 0
#define WOSC_MIX_VARIANT_NAME	wosc_process__f_e
#define WOSC_MIX_VARIANT	(0                  | WOSC_MIX_WITH_FREQ | 0                 | WOSC_MIX_WITH_EXP_FM)
#include "gslwaveosc-aux.cc"
#endif
#define WOSC_MIX_VARIANT_NAME	wosc_process__f__
#define WOSC_MIX_VARIANT	(0                  | WOSC_MIX_WITH_FREQ | 0                 | 0                   )
#include "gslwaveosc-aux.cc"
#define WOSC_MIX_VARIANT_NAME	wosc_process___me
#define WOSC_MIX_VARIANT	(0                  | 0                  | WOSC_MIX_WITH_MOD | WOSC_MIX_WITH_EXP_FM)
#include "gslwaveosc-aux.cc"
#define WOSC_MIX_VARIANT_NAME	wosc_process___m_
#define WOSC_MIX_VARIANT	(0                  | 0                  | WOSC_MIX_WITH_MOD | 0                   )
#include "gslwaveosc-aux.cc"
#if 0
#define WOSC_MIX_VARIANT_NAME	wosc_process____e
#define WOSC_MIX_VARIANT	(0                  | 0                  | 0                 | WOSC_MIX_WITH_EXP_FM)
#include "gslwaveosc-aux.cc"
#endif
#define WOSC_MIX_VARIANT_NAME	wosc_process_____
#define WOSC_MIX_VARIANT	(0                  | 0                  | 0                 | 0                   )
#include "gslwaveosc-aux.cc"


/* --- functions --- */
gboolean
gsl_wave_osc_process (GslWaveOscData *wosc,
		      guint           n_values,
		      const gfloat   *freq_in,
		      const gfloat   *mod_in,
		      const gfloat   *sync_in,
		      gfloat         *mono_out)
{
  guint mode = 0;

  assert_return (wosc != NULL, FALSE);
  assert_return (n_values > 0, FALSE);
  assert_return (mono_out != NULL, FALSE);

  if (UNLIKELY (!wosc->config.lookup_wchunk))
    return FALSE;

  /* mode changes:
   * freq_in:	if (freq_in) last_freq=inval else set_filter()
   * sync_in:	last_sync=0
   * mod_in:	if (mod_in) last_mod=0 else if (freq_in) last_freq=inval else transform_filter()
   * exp_mod:	n/a
   */

  if (sync_in)
    mode |= WOSC_MIX_WITH_SYNC;
  if (freq_in)
    mode |= WOSC_MIX_WITH_FREQ;
  if (mod_in)
    mode |= WOSC_MIX_WITH_MOD;
  if (wosc->config.exponential_fm)
    mode |= WOSC_MIX_WITH_EXP_FM;

  /* adapt to mode changes */
  if (UNLIKELY (mode != wosc->last_mode))
    {
      guint mask = wosc->last_mode ^ mode;

      if (mask & WOSC_MIX_WITH_SYNC)
	wosc->last_sync_level = 0;
      if (mask & WOSC_MIX_WITH_FREQ)
	{
	  if (freq_in)
	    wosc->last_freq_level = SIGNAL_LEVEL_INVAL;
	  else
	    gsl_wave_osc_set_filter (wosc, wosc->config.cfreq, FALSE);
	}
      if (mask & WOSC_MIX_WITH_MOD)
	{
	  if (mod_in)
	    wosc->last_mod_level = 0;
	  else if (freq_in)
	    wosc->last_freq_level = SIGNAL_LEVEL_INVAL;
	  else /* !mod_in && !freq_in */
	    wave_osc_transform_filter (wosc, wosc->config.cfreq);
	}
      wosc->last_mode = mode;
    }

  /* auto-trigger after reset */
  if (!sync_in && wosc->last_sync_level < 0.5)
    {
      gsl_wave_osc_retrigger (wosc, freq_in ? BSE_SIGNAL_TO_FREQ (*freq_in) : wosc->config.cfreq);
      wosc->last_sync_level = 1.0;
    }

  switch (mode)
    {
    case 0                  | 0                  | 0                 | 0:
    case 0                  | 0                  | 0                 | WOSC_MIX_WITH_EXP_FM:
      wosc_process_____ (wosc, n_values, freq_in, mod_in, sync_in, mono_out);
      break;
    case 0                  | 0                  | WOSC_MIX_WITH_MOD | 0:
      wosc_process___m_ (wosc, n_values, freq_in, mod_in, sync_in, mono_out);
      break;
    case 0                  | 0                  | WOSC_MIX_WITH_MOD | WOSC_MIX_WITH_EXP_FM:
      wosc_process___me (wosc, n_values, freq_in, mod_in, sync_in, mono_out);
      break;
    case 0                  | WOSC_MIX_WITH_FREQ | 0                 | 0:
    case 0                  | WOSC_MIX_WITH_FREQ | 0                 | WOSC_MIX_WITH_EXP_FM:
      wosc_process__f__ (wosc, n_values, freq_in, mod_in, sync_in, mono_out);
      break;
    case 0                  | WOSC_MIX_WITH_FREQ | WOSC_MIX_WITH_MOD | 0:
      wosc_process__fm_ (wosc, n_values, freq_in, mod_in, sync_in, mono_out);
      break;
    case 0                  | WOSC_MIX_WITH_FREQ | WOSC_MIX_WITH_MOD | WOSC_MIX_WITH_EXP_FM:
      wosc_process__fme (wosc, n_values, freq_in, mod_in, sync_in, mono_out);
      break;
    case WOSC_MIX_WITH_SYNC | 0                  | 0                 | 0:
    case WOSC_MIX_WITH_SYNC | 0                  | 0                 | WOSC_MIX_WITH_EXP_FM:
      wosc_process_s___ (wosc, n_values, freq_in, mod_in, sync_in, mono_out);
      break;
    case WOSC_MIX_WITH_SYNC | 0                  | WOSC_MIX_WITH_MOD | 0:
      wosc_process_s_m_ (wosc, n_values, freq_in, mod_in, sync_in, mono_out);
      break;
    case WOSC_MIX_WITH_SYNC | 0                  | WOSC_MIX_WITH_MOD | WOSC_MIX_WITH_EXP_FM:
      wosc_process_s_me (wosc, n_values, freq_in, mod_in, sync_in, mono_out);
      break;
    case WOSC_MIX_WITH_SYNC | WOSC_MIX_WITH_FREQ | 0                 | 0:
    case WOSC_MIX_WITH_SYNC | WOSC_MIX_WITH_FREQ | 0                 | WOSC_MIX_WITH_EXP_FM:
      wosc_process_sf__ (wosc, n_values, freq_in, mod_in, sync_in, mono_out);
      break;
    case WOSC_MIX_WITH_SYNC | WOSC_MIX_WITH_FREQ | WOSC_MIX_WITH_MOD | 0:
      wosc_process_sfm_ (wosc, n_values, freq_in, mod_in, sync_in, mono_out);
      break;
    case WOSC_MIX_WITH_SYNC | WOSC_MIX_WITH_FREQ | WOSC_MIX_WITH_MOD | WOSC_MIX_WITH_EXP_FM:
      wosc_process_sfme (wosc, n_values, freq_in, mod_in, sync_in, mono_out);
      break;
    default:
      assert_return_unreached (FALSE);
    }
  if (wosc->y[0] != 0.0 &&
      !(fabs (wosc->y[0]) > BSE_SIGNAL_EPSILON && fabs (wosc->y[0]) < BSE_SIGNAL_KAPPA))
    {
      guint i;
      WDEBUG ("clearing filter state at:\n");
      for (i = 0; i < GSL_WAVE_OSC_FILTER_ORDER; i++)
	{
	  WDEBUG ("%u) %+.38f\n", i, wosc->y[i]);
	  if (BSE_DOUBLE_IS_INF (wosc->y[0]) || fabs (wosc->y[0]) > BSE_SIGNAL_KAPPA)
	    wosc->y[i] = BSE_DOUBLE_SIGN (wosc->y[0]) ? -1.0 : 1.0;
	  else
	    wosc->y[i] = 0.0;
	}
    }
  assert_return (!BSE_DOUBLE_IS_NANINF (wosc->y[0]), FALSE);
  assert_return (!BSE_DOUBLE_IS_SUBNORMAL (wosc->y[0]), FALSE);

  wosc->done = (wosc->block.is_silent &&   /* FIXME, let filter state run out? */
		((wosc->block.play_dir < 0 && wosc->block.offset < 0) ||
		 (wosc->block.play_dir > 0 && wosc->block.offset > wosc->wchunk->wave_length)));

  return TRUE;
}

void
gsl_wave_osc_set_filter (GslWaveOscData *wosc,
			 gfloat          play_freq,
			 gboolean	 clear_state)
{
  gfloat zero_padding = 2;
  gfloat step;
  guint i, istep;

  assert_return (play_freq > 0);

  if (UNLIKELY (!wosc->config.lookup_wchunk))
    return;

  wosc->step_factor = zero_padding * wosc->wchunk->mix_freq * wosc->wchunk->fine_tune_factor;
  wosc->step_factor /= wosc->wchunk->osc_freq * wosc->mix_freq;
  step = wosc->step_factor * play_freq;
  istep = step * (FRAC_MASK + 1.) + 0.5;

  if (istep != wosc->istep)
    {
      gfloat nyquist_fact = 2.0 * PI / wosc->mix_freq, cutoff_freq = 18000, stop_freq = MAX (wosc->wchunk->mix_freq / 2, 24000);
      gfloat empiric_filter_stability_limit = 6.;
      gfloat filt_fact = CLAMP (1. / step,
				1. / (empiric_filter_stability_limit * zero_padding),
				1. / zero_padding /* spectrum half */);
      /* the following frequencies are 2pi relative, where 2*PI=mix-freq */
      gfloat freq_r = stop_freq * nyquist_fact * filt_fact;
      gfloat freq_c = cutoff_freq * nyquist_fact * filt_fact;

      /* FIXME: this should store filter roots and poles, so modulation does lp->lp transform */

      wosc->istep = istep;
      gsl_filter_tscheb2_lp (GSL_WAVE_OSC_FILTER_ORDER, freq_c, freq_r / freq_c, 0.18, wosc->a, wosc->b);

      /* Scale to compensate for zero-padding.
       * Adjust volume of the chunk according to its volume adjustment.
       */
      for (i = 0; i < GSL_WAVE_OSC_FILTER_ORDER + 1; i++)
	wosc->a[i] *= zero_padding * wosc->wchunk->volume_adjust;
      for (i = 0; i < (GSL_WAVE_OSC_FILTER_ORDER + 1) / 2; i++)     /* reverse bs */
	{
	  gfloat t = wosc->b[GSL_WAVE_OSC_FILTER_ORDER - i];
	  wosc->b[GSL_WAVE_OSC_FILTER_ORDER - i] = wosc->b[i];
	  wosc->b[i] = t;
	}
      WDEBUG ("filter: fc=%f fr=%f st=%f is=%u\n", freq_c/PI*2, freq_r/PI*2, step, wosc->istep);
    }
  if (clear_state)
    {
      /* clear filter state */
      memset (wosc->y, 0, sizeof (wosc->y));
      wosc->j = 0;
      wosc->cur_pos = 0;    /* might want to initialize with istep? */
    }
}

static void
wave_osc_transform_filter (GslWaveOscData *wosc,
			   gfloat          play_freq)
{
  gfloat step;
  guint istep;

  step = wosc->step_factor * play_freq;
  istep = step * (FRAC_MASK + 1.) + 0.5;
  if (istep != wosc->istep)
    {
      wosc->istep = istep;
      /* transform filter poles and roots, normalize filter, update a[] and b[] */
    }
}

GslLong
gsl_wave_osc_cur_pos (GslWaveOscData *wosc)
{
  assert_return (wosc != NULL, -1);

  if (wosc->wchunk)
    return wosc->block.offset;
  else
    return wosc->config.start_offset;
}

void
gsl_wave_osc_retrigger (GslWaveOscData *wosc,
			gfloat          base_freq)
{
  assert_return (wosc != NULL);

  if (UNLIKELY (!wosc->config.lookup_wchunk))
    return;

  if (wosc->wchunk)
    gsl_wave_chunk_unuse_block (wosc->wchunk, &wosc->block);
  wosc->wchunk = wosc->config.lookup_wchunk (wosc->config.wchunk_data, base_freq, 1); // FIXME: velocity=1 hardcoded
  wosc->block.play_dir = wosc->config.play_dir;
  wosc->block.offset = wosc->config.start_offset;
  gsl_wave_chunk_use_block (wosc->wchunk, &wosc->block);
  wosc->x = wosc->block.start + CLAMP (wosc->config.channel, 0, wosc->wchunk->n_channels - 1);
  WDEBUG ("wave lookup: want=%f got=%f length=%llu\n", base_freq, wosc->wchunk->osc_freq, wosc->wchunk->wave_length);
  wosc->last_freq_level = BSE_SIGNAL_FROM_FREQ (base_freq);
  wosc->last_mod_level = 0;
  gsl_wave_osc_set_filter (wosc, base_freq, TRUE);
}
void
gsl_wave_osc_config (GslWaveOscData   *wosc,
		     GslWaveOscConfig *config)
{
  assert_return (wosc != NULL);
  assert_return (config != NULL);
  if (wosc->config.wchunk_data != config->wchunk_data ||
      wosc->config.lookup_wchunk != config->lookup_wchunk ||
      wosc->config.channel != config->channel)
    {
      if (wosc->wchunk)
	gsl_wave_chunk_unuse_block (wosc->wchunk, &wosc->block);
      wosc->wchunk = NULL;
      wosc->config = *config;
      gsl_wave_osc_retrigger (wosc, wosc->config.cfreq);
      wosc->last_sync_level = MIN (wosc->last_sync_level, 0.0);
    }
  else
    {
      wosc->config.play_dir = config->play_dir;
      wosc->config.fm_strength = config->fm_strength;
      if (wosc->config.cfreq != config->cfreq ||
	  wosc->config.start_offset != config->start_offset)
	{
	  wosc->config.start_offset = config->start_offset;
	  wosc->config.cfreq = config->cfreq;
	  gsl_wave_osc_retrigger (wosc, wosc->config.cfreq);
	  wosc->last_sync_level = MIN (wosc->last_sync_level, 0.0);
	}
    }
}

void
gsl_wave_osc_reset (GslWaveOscData *wosc)
{
  assert_return (wosc != NULL);

  gsl_wave_osc_set_filter (wosc, wosc->config.cfreq, TRUE);
  wosc->last_mode = 0;
  wosc->last_sync_level = 0;
  wosc->last_freq_level = SIGNAL_LEVEL_INVAL;
  wosc->last_mod_level = 0;
  wosc->done = FALSE;
}

void
gsl_wave_osc_init (GslWaveOscData *wosc)
{
  assert_return (wosc != NULL);

  assert_return (GSL_WAVE_OSC_FILTER_ORDER <= BSE_CONFIG (wave_chunk_padding));

  memset (wosc, 0, sizeof (GslWaveOscData));
  wosc->mix_freq = bse_engine_sample_freq ();
}

void
gsl_wave_osc_shutdown (GslWaveOscData *wosc)
{
  assert_return (wosc != NULL);

  if (wosc->wchunk)
    gsl_wave_chunk_unuse_block (wosc->wchunk, &wosc->block);
  memset (wosc, 0xaa, sizeof (GslWaveOscData));
}
