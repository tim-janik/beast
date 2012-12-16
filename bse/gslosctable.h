/* GSL - Generic Sound Layer
 * Copyright (C) 2001-2003 Tim Janik
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
#ifndef __GSL_OSC_TABLE_H__
#define __GSL_OSC_TABLE_H__

#include <bse/gsldefs.h>
#include <sfi/gbsearcharray.h>

G_BEGIN_DECLS


/* --- structures & enums --- */
typedef enum    /*< skip >*/
{
  GSL_OSC_WAVE_NONE,
  GSL_OSC_WAVE_SINE,
  GSL_OSC_WAVE_TRIANGLE,
  GSL_OSC_WAVE_SAW_RISE,
  GSL_OSC_WAVE_SAW_FALL,
  GSL_OSC_WAVE_PEAK_RISE,
  GSL_OSC_WAVE_PEAK_FALL,
  GSL_OSC_WAVE_MOOG_SAW,
  GSL_OSC_WAVE_SQUARE,
  GSL_OSC_WAVE_PULSE_SAW
} GslOscWaveForm;

typedef struct
{
  gfloat         mix_freq;
  GslOscWaveForm wave_form;
  GBSearchArray *entry_array;
} GslOscTable;

typedef struct
{
  gfloat        min_freq;
  gfloat        max_freq;
  guint         n_values;
  const gfloat *values;	/* contains n_values+1 values with values[0]==values[n_values] */
  /* integer stepping (block size dependant) */
  guint32	n_frac_bits;
  guint32	frac_bitmask;
  gfloat	freq_to_step;		/* freq -> int.frac */
  gfloat	phase_to_pos;		/* 0..1 -> int.frac */
  gfloat	ifrac_to_float;		/* frac -> 0..1 float */
  guint		min_pos, max_pos;	/* pulse extension */
} GslOscWave;


/* --- oscillator table --- */
GslOscTable*    gsl_osc_table_create            (gfloat                  mix_freq,
						 GslOscWaveForm          wave_form,
						 double                (*filter_func) (double),
						 guint                   n_freqs,
						 const gfloat           *freqs);
void            gsl_osc_table_lookup            (const GslOscTable      *table,
						 gfloat                  freq,
						 GslOscWave             *wave);
void            gsl_osc_table_free              (GslOscTable            *table);


/* --- oscillator wave utils --- */
void            gsl_osc_wave_fill_buffer        (GslOscWaveForm          type,
						 guint                   n_values,
						 gfloat                 *values);
void		gsl_osc_wave_extrema		(guint                   n_values,
						 const gfloat		*values,
						 gfloat			*min,
						 gfloat			*max);
void		gsl_osc_wave_normalize		(guint			 n_values,
						 gfloat			*values,
						 gfloat			 new_center,
						 gfloat			 new_max);
void		gsl_osc_wave_adjust_range	(guint			 n_values,
						 gfloat			*values,
						 gfloat			 min,
						 gfloat			 max,
						 gfloat			 new_center,
						 gfloat			 new_max);
const gchar*	gsl_osc_wave_form_name		(GslOscWaveForm		 wave_form);


G_END_DECLS

#endif /* __GSL_OSC_TABLE_H__ */
