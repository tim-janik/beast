/* GslWaveOsc - GSL Wave Oscillator
 * Copyright (C) 2001-2002 Tim Janik and Stefan Westerfeld
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
#ifndef __GSL_WAVE_OSC_H__
#define __GSL_WAVE_OSC_H__

#include <gsl/gsldefs.h>
#include <gsl/gslwavechunk.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GSL_WAVE_OSC_FILTER_ORDER	(8)	/* <= GslConfig.wave_chunk_padding ! */

typedef struct
{
  GslLong	  start_offset;
  gint            play_dir;

  gpointer	  wchunk_data;
  GslWaveChunk* (*wchunk_from_freq) (gpointer	wchunk_data,
				     gfloat     freq);

  gfloat	  fm_strength;	/* linear: 0..1, exponential: n_octaves */
  guint		  exponential_fm : 1;
  gfloat	  cfreq;
} GslWaveOscConfig;

typedef struct
{
  GslWaveOscConfig  config;
  guint		    last_mode;
  gfloat	    last_sync_level, last_freq_level, last_mod_level;
  GslWaveChunkBlock block;
  gfloat           *x;                  /* pointer into block */
  guint             cur_pos, istep;	// FIX
  gdouble           a[GSL_WAVE_OSC_FILTER_ORDER + 1];       /* order */
  gdouble           b[GSL_WAVE_OSC_FILTER_ORDER + 1];       /* reversed order */
  gdouble           y[GSL_WAVE_OSC_FILTER_ORDER + 1];
  guint             j;                  /* y[] index */
  GslWaveChunk     *wchunk;
  gfloat	    mix_freq;		/* gsl_engine_sample_freq() */
  gfloat	    step_factor;
  gboolean	    done;		/* FIX. caution, this is TRUE only if
					 * (play_dir < 0 && cur_pos < 0) ||
					 * (play_dir > 0 && cur_pos > wchunk.length)
					 */
} GslWaveOscData;


void		gsl_wave_osc_config	(GslWaveOscData	*wosc,
					 GslWaveOscConfig *config);
gboolean	gsl_wave_osc_process	(GslWaveOscData	*wosc,
					 guint		 n_values,
					 const gfloat	*freq,
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

/* setup:
 * wosc = g_new0 (GslWaveOscData, 1);
 * wosc->mix_freq = gsl_engine_sample_freq ();
 */



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_WAVE_OSC_H__ */
