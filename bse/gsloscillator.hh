// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __GSL_OSCILLATOR_H__
#define __GSL_OSCILLATOR_H__
#include <bse/gsldefs.hh>
#include <bse/gslosctable.hh>
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
  double	 transpose_factor;	/* -132..0..+132 */
  gint		 fine_tune;		/* -100..+100 */
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
void	gsl_osc_reset		(GslOscData	*osc);
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
