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
#ifndef __GSL_WAVE_DSC_H__
#define __GSL_WAVE_DSC_H__

#include <gsl/gsldefs.h>
#include <gsl/gsldatahandle.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- structures --- */
struct _GslWaveChunkDsc
{
  gchar         *file_name;
  guint          byte_order;
  guint          format : 16;
  guint		 n_channels : 16;

  gfloat         mix_freq;	/* recorded with mix_freq */
  gfloat         osc_freq;	/* while oscillating at osc_freq */
  GslLong        boffset;	/* offset in bytes into file */
  GslLong        n_values;	/* # of format values per channel (0==eof) */
  guint          loop_start;    /* sample offset */
  guint          loop_end;      /* sample offset */
  guint          loop_count;
};
struct _GslWaveDsc
{
  gchar		  *name;

  guint		   n_chunks;
  GslWaveChunkDsc *chunks;

  guint		   max_n_channels;

  /* chunk defaults */
  guint		   dfl_byte_order;
  guint		   dfl_format : 16;
  guint		   dfl_n_channels : 16;
  gfloat	   dfl_mix_freq;
};


/* --- prototypes --- */
GslRing*	gsl_wave_file_scan		(const gchar	  *file_name);
void		gsl_wave_file_scan_free		(GslRing	  *ring);
GslWaveDsc*	gsl_wave_dsc_read		(const gchar	  *file_name);
void		gsl_wave_dsc_free		(GslWaveDsc	  *wave);


/* --- utils --- */
GslDataHandle*	gsl_data_handle_from_wave_file	(const gchar	  *file_name,
						 GslWaveFormatType format,
						 guint		   byte_order,
						 guint		   seek_boffset);
GslWaveChunk*	gsl_wave_chunk_from_dsc		(GslWaveDsc	  *wave_dsc,
						 guint		   nth_chunk);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_WAVE_DSC_H__ */
