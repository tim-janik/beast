/* GSL - Generic Sound Layer
 * Copyright (C) 2001 Tim Janik
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
#ifndef __GSL_MULTI_WAVE_H__
#define __GSL_MULTI_WAVE_H__

#include <gsl/gslcommon.h>
#include <gsl/gslwavechunk.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- typedefs & structures --- */
typedef struct _GslMultiWave GslMultiWave;
struct _GslMultiWave
{
  gchar		 *name;
  GslDataLocator *multi_wave_location;
  guint		  ref_count;
  guint		  frozen;
  GslRing	 *wave_chunks;
  GslRing	 *dloc_cache;
};


/* --- prototypes --- */
GslMultiWave*	gsl_multi_wave_new		(const gchar		*name,
						 const GslDataLocator	*sloc);
GslWaveChunk*	gsl_multi_wave_add_chunk	(GslMultiWave		*mwave,
						 const GslDataLocator   *wcdata_loc,
						 GslDataReader          *reader,
						 gsize                   offset,
						 gsize                   length,
						 guint		         n_channels,
						 gfloat                  mix_freq,
						 gfloat                  osc_freq);
void		gsl_multi_wave_set_chunk_loop	(GslMultiWave		*mwave,
						 GslWaveChunk		*wchunk,
						 GslWaveLoopType	 loop_type,
						 GslLong		 loop_start,
						 GslLong		 loop_end,
						 guint			 loop_count);
GslWaveChunk*	gsl_multi_wave_match_chunk	(GslMultiWave		*mwave,
						 guint			 n_channels,
						 gfloat			 mix_freq,
						 gfloat			 mix_freq_epsilon,
						 gfloat			 osc_freq,
						 gfloat			 osc_freq_epsilon);
GslMultiWave*	gsl_multi_wave_ref		(GslMultiWave		*mwave);
void		gsl_multi_wave_unref		(GslMultiWave		*mwave);
void		gsl_multi_wave_freeze		(GslMultiWave		*mwave);
void		gsl_multi_wave_thaw		(GslMultiWave		*mwave);
void		gsl_multi_wave_open_chunk	(GslMultiWave		*mwave,
						 GslWaveChunk		*wchunk);
void		gsl_multi_wave_close_chunk	(GslMultiWave		*mwave,
						 GslWaveChunk		*wchunk);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_MULTI_WAVE_H__ */
