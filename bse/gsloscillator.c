/* GSL - Generic Sound Layer
 * Copyright (C) 1999, 2000-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "gsloscillator.h"

#include "gslsignal.h"

#define	SIGNAL_LEVEL_INVAL	(-2.0)	/* trigger level-changed checks */


/* --- functions --- */
static inline void
osc_update_pwm_offset (GslOscData *osc,
		       gfloat	   pulse_mod) /* -1..+1 */
{
  guint32 maxp_offs, minp_offs, mpos, tpos;
  gfloat min, max, foffset;

  /* figure actual pulse width (0..1) */
  foffset = osc->config.pulse_width;	/* 0..1 */
  foffset += pulse_mod * osc->config.pulse_mod_strength;
  foffset = CLAMP (foffset, 0.0, 1.0);

  /* calculate pulse scaling range for this offset */
  osc->pwm_offset = foffset * osc->wave.n_values;
  osc->pwm_offset <<= osc->wave.n_frac_bits;

  maxp_offs = (osc->wave.min_pos + osc->wave.n_values + osc->wave.max_pos) << (osc->wave.n_frac_bits - 1);
  minp_offs = (osc->wave.max_pos + osc->wave.min_pos) << (osc->wave.n_frac_bits - 1);
  
  mpos = maxp_offs + (osc->pwm_offset >> 1);
  tpos = mpos >> osc->wave.n_frac_bits;
  max = osc->wave.values[tpos];
  mpos -= osc->pwm_offset;
  tpos = mpos >> osc->wave.n_frac_bits;
  max -= osc->wave.values[tpos];
  mpos = minp_offs + (osc->pwm_offset >> 1);
  tpos = mpos >> osc->wave.n_frac_bits;
  min = osc->wave.values[tpos];
  mpos -= osc->pwm_offset;
  tpos = mpos >> osc->wave.n_frac_bits;
  min -= osc->wave.values[tpos];
  osc->pwm_center = (min + max) / -2.0;
  min = fabs (min + osc->pwm_center);
  max = fabs (max + osc->pwm_center);
  max = MAX (max, min);
  if_reject (max < GSL_FLOAT_MIN_NORMAL)
    {
      osc->pwm_center = foffset < 0.5 ? -1.0 : +1.0;
      osc->pwm_max = 1.0;
    }
  else
    osc->pwm_max = 1.0 / max;
}


/* --- generate processing function variants --- */
#define OSC_FLAG_INVAL		(0xffffffff)
#define OSC_FLAG_ISYNC		(1)
#define OSC_FLAG_OSYNC		(2)
#define OSC_FLAG_FREQ		(4)
#define OSC_FLAG_SELF_MOD	(8)
#define	OSC_FLAG_LINEAR_MOD	(16)
#define	OSC_FLAG_EXP_MOD	(32)
#define	OSC_FLAG_PWM_MOD	(64)
#define	OSC_FLAG_PULSE_OSC	(128)

/* normal oscillator variants */
#define	OSC_INCLUDER_FLAGS	0
#define	GSL_INCLUDER_FIRST_CASE	0
#define	GSL_INCLUDER_LAST_CASE	63
#define	GSL_INCLUDER_NAME	oscillator_process_normal
#define	GSL_INCLUDER_TABLE	static void (*osc_process_table[]) (GslOscData*,guint, \
                                const gfloat*,const gfloat*,const gfloat*, \
		                const gfloat*,gfloat*,gfloat*)
#define	GSL_INCLUDER_FILE	"gsloscillator-aux.c"
#include "gslincluder.c"
#undef	OSC_INCLUDER_FLAGS

/* pulse width modulation oscillator variants */
#define	OSC_INCLUDER_FLAGS	OSC_FLAG_PULSE_OSC
#define	GSL_INCLUDER_FIRST_CASE	0
#define	GSL_INCLUDER_LAST_CASE	127
#define	GSL_INCLUDER_NAME	oscillator_process_pulse
#define	GSL_INCLUDER_TABLE	static void (*osc_process_pulse_table[]) (GslOscData*,guint, \
                                const gfloat*,const gfloat*,const gfloat*, \
		                const gfloat*,gfloat*,gfloat*)
#define	GSL_INCLUDER_FILE	"gsloscillator-aux.c"
#include "gslincluder.c"
#undef	OSC_INCLUDER_FLAGS


/* --- functions --- */
static inline void
osc_process (GslOscData   *osc,
	     guint         n_values,
	     guint	   mode,
	     const gfloat *ifreq,
	     const gfloat *imod,
	     const gfloat *isync,
	     const gfloat *ipwm,
	     gfloat       *mono_out,
	     gfloat       *sync_out)
{
  mode |= isync ? OSC_FLAG_ISYNC : 0;
  mode |= sync_out ? OSC_FLAG_OSYNC : 0;
  mode |= ifreq ? OSC_FLAG_FREQ : 0;
  if (osc->config.pulse_mod_strength > GSL_FLOAT_MIN_NORMAL)
    mode |= ipwm ? OSC_FLAG_PWM_MOD : 0;
  if (osc->config.self_fm_strength > GSL_FLOAT_MIN_NORMAL)
    mode |= OSC_FLAG_SELF_MOD;
  if (imod && osc->config.exponential_fm)
    mode |= OSC_FLAG_EXP_MOD;
  else if (imod)
    mode |= OSC_FLAG_LINEAR_MOD;
  
  if_reject (mode != osc->last_mode)
    {
      guint change_mask = osc->last_mode >= OSC_FLAG_INVAL ? OSC_FLAG_INVAL : osc->last_mode ^ mode;
      
      if (change_mask & OSC_FLAG_FREQ)
	{
	  gdouble fcpos, flpos;

	  fcpos = osc->cur_pos * osc->wave.ifrac_to_float;
	  flpos = osc->last_pos * osc->wave.ifrac_to_float;
	  osc->last_freq_level = osc->config.cfreq;
	  gsl_osc_table_lookup (osc->config.table, osc->last_freq_level, &osc->wave);
	  osc->last_pos = flpos / osc->wave.ifrac_to_float;
	  osc->cur_pos = fcpos / osc->wave.ifrac_to_float;
	}
      if (!(mode & OSC_FLAG_ISYNC))
	osc->last_sync_level = 0;
      if (mode & OSC_FLAG_PULSE_OSC)
	{
	  osc->last_pwm_level = 0;
	  osc_update_pwm_offset (osc, osc->last_pwm_level);
	}
      osc->last_mode = mode;
    }
  
  /* invoke generated function variant */
  if (mode & OSC_FLAG_PULSE_OSC)
    osc_process_pulse_table[mode & ~OSC_FLAG_PULSE_OSC] (osc, n_values,
							 ifreq, imod, isync, ipwm,
							 mono_out, sync_out);
  else
    osc_process_table[mode] (osc, n_values,
			     ifreq, imod, isync, NULL,
			     mono_out, sync_out);
}

void
gsl_osc_process (GslOscData   *osc,
		 guint         n_values,
		 const gfloat *ifreq,
		 const gfloat *imod,
		 const gfloat *isync,
		 gfloat       *mono_out,
		 gfloat       *sync_out)
{
  g_return_if_fail (osc != NULL);
  g_return_if_fail (n_values > 0);
  g_return_if_fail (mono_out != NULL);

  if (osc->last_mode & OSC_FLAG_PULSE_OSC)
    osc->last_mode = OSC_FLAG_INVAL;
  osc_process (osc, n_values, 0,
	       ifreq, imod, isync, NULL,
	       mono_out, sync_out);
}

void
gsl_osc_process_pulse (GslOscData   *osc,
		       guint         n_values,
		       const gfloat *ifreq,
		       const gfloat *imod,
		       const gfloat *isync,
		       const gfloat *ipwm,
		       gfloat       *mono_out,
		       gfloat       *sync_out)
{
  g_return_if_fail (osc != NULL);
  g_return_if_fail (n_values > 0);
  g_return_if_fail (mono_out != NULL);

  if (!(osc->last_mode & OSC_FLAG_PULSE_OSC))
    osc->last_mode = OSC_FLAG_INVAL;
  osc_process (osc, n_values, OSC_FLAG_PULSE_OSC,
	       ifreq, imod, isync, ipwm,
	       mono_out, sync_out);
}

void
gsl_osc_config (GslOscData   *osc,
		GslOscConfig *config)
{
  g_return_if_fail (osc != NULL);
  g_return_if_fail (config != NULL);
  g_return_if_fail (config->table != NULL);

  osc->config = *config;
  osc->last_mode = OSC_FLAG_INVAL;
}

void
gsl_osc_reset (GslOscData *osc)
{
  g_return_if_fail (osc != NULL);

  osc->cur_pos = 0;
  osc->last_pos = 0;
  osc->last_sync_level = 0;
  osc->last_freq_level = 0;
  osc->last_pwm_level = 0;
  osc->pwm_offset = 0;
  osc->pwm_max = 0;
  osc->pwm_center = 0;
  osc->last_mode = OSC_FLAG_INVAL;
}
