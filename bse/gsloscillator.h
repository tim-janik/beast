/* GSL - Generic Sound Layer
 * Copyright (C) 1999, 2000-2002 Tim Janik
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
#ifndef __GSL_OSCILLATOR_H__
#define __GSL_OSCILLATOR_H__

#include <gsl/gsldefs.h>
#include <gsl/gslosctable.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- structures --- */
typedef struct
{
  GslOscTable	*table;
  guint		 exponential_fm : 1;
  gfloat	 fm_strength;		/* linear: 0..1, exponential: n_octaves */
  gfloat	 self_fm_strength;	/* 0..1 */
  gfloat	 phase;			/* -0.5..+0.5 */
  gfloat	 cfreq;			/* for ifreq == NULL */
  gfloat	 pulse_width;		/* 0..1 */
  gfloat	 pulse_mod_strength;	/* 0..0.5 */
} GslOscConfig;
typedef struct
{
  GslOscConfig	 config;
  guint		 last_mode;
  guint32	 cur_pos, last_pos;
  gfloat	 last_sync_level;
  gdouble	 last_freq_level;
  gfloat	 last_pwm_level;
  GslOscWave	 wave;
  /* pwm */
  guint32	 pwm_offset;
  gfloat	 pwm_max, pwm_center;
} GslOscData;


/* --- Oscillator --- */
void	gsl_osc_config		(GslOscData	*osc,
				 GslOscConfig	*config);
void	gsl_osc_process		(GslOscData	*osc,
				 guint		 n_values,
				 const gfloat	*ifreq,
				 const gfloat	*imod,
				 const gfloat	*isync,
				 gfloat		*mono_out,
				 gfloat		*sync_out);
void	gsl_osc_process_pulse	(GslOscData	*osc,
				 guint		 n_values,
				 const gfloat	*ifreq,
				 const gfloat	*imod,
				 const gfloat	*isync,
				 const gfloat	*ipwm,
				 gfloat		*mono_out,
				 gfloat		*sync_out);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_OSCILLATOR_H__ */
