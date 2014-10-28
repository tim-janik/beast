// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __GSL_WAVE_OSC_H__
#define __GSL_WAVE_OSC_H__

#include <bse/gsldefs.hh>
#include <bse/gslwavechunk.hh>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GSL_WAVE_OSC_FILTER_ORDER	(8)	/* <= GslConfig.wave_chunk_padding ! */

typedef struct
{
  GslLong	  start_offset;
  gint            play_dir, channel;

  gpointer	  wchunk_data;
  GslWaveChunk* (*lookup_wchunk) (gpointer	wchunk_data,
                                  gfloat        freq,
                                  gfloat        velocity);

  gfloat	  fm_strength;	/* linear: 0..1, exponential: n_octaves */
  guint		  exponential_fm : 1;
  gfloat	  cfreq;	/* for ifreq == NULL */
} GslWaveOscConfig;

typedef struct
{
  GslWaveOscConfig  config;
  guint		    last_mode;
  gfloat	    last_sync_level, last_freq_level, last_mod_level;
  GslWaveChunkBlock block;
  gfloat           *x;                  /* pointer into block */
  guint             cur_pos, istep;	/* FIXME */
  gdouble           a[GSL_WAVE_OSC_FILTER_ORDER + 1];       /* order */
  gdouble           b[GSL_WAVE_OSC_FILTER_ORDER + 1];       /* reversed order */
  gdouble           y[GSL_WAVE_OSC_FILTER_ORDER + 1];
  guint             j;                  /* y[] index */
  GslWaveChunk     *wchunk;
  gfloat	    mix_freq;		/* bse_engine_sample_freq() */
  gfloat	    step_factor;
  gboolean	    done;		/* FIXME. caution, this is TRUE only if
					 * (play_dir < 0 && cur_pos < 0) ||
					 * (play_dir > 0 && cur_pos > wchunk.length)
					 */
} GslWaveOscData;


void		gsl_wave_osc_config	(GslWaveOscData	*wosc,
					 GslWaveOscConfig *config);
void		gsl_wave_osc_reset	(GslWaveOscData *wosc);
gboolean	gsl_wave_osc_process	(GslWaveOscData	*wosc,
					 guint		 n_values,
					 const gfloat	*ifreq,
					 const gfloat	*mod,
					 const gfloat	*sync,
					 gfloat		*mono_out);
void		gsl_wave_osc_retrigger	(GslWaveOscData	*wosc,
					 gfloat		 freq);
void		gsl_wave_osc_set_filter	(GslWaveOscData	*wosc,
					 gfloat		 freq,
					 gboolean	 clear_state);

void		gsl_wave_osc_init	(GslWaveOscData	*wosc);
void		gsl_wave_osc_shutdown	(GslWaveOscData	*wosc);
GslLong		gsl_wave_osc_cur_pos	(GslWaveOscData	*wosc);

/* setup:
 * wosc = g_new0 (GslWaveOscData, 1);
 * wosc->mix_freq = bse_engine_sample_freq ();
 */



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_WAVE_OSC_H__ */
