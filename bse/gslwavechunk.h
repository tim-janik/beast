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
#ifndef __GSL_WAVE_CHUNK_H__
#define __GSL_WAVE_CHUNK_H__

#include <gsl/gsldefs.h>
#include <gsl/gsldatacache.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- typedefs & structures --- */
typedef enum /*< skip >*/
{
  GSL_WAVE_LOOP_NONE,
  GSL_WAVE_LOOP_JUMP,
  GSL_WAVE_LOOP_PINGPONG
} GslWaveLoopType;
typedef struct
{
  GslLong start, end, length;
  gfloat *mem;
} GslWaveChunkMem;
struct _GslWaveChunk
{
  /* wave chunk data residency */
  GslDataCache   *dcache;
  gpointer	  owner_data;
  GslLong	  offset;
  GslLong	  length;

  /* chunk specific parameters */
  gint		  n_channels;
  gfloat	  mix_freq;	/* recorded with mix_freq */
  gfloat	  osc_freq;	/* while oscillating at osc_freq */
  GslLong	  n_pad_values;	/* guaranteed pad values around blocks */
  GslLong	  wave_length;	/* start + loop duration + end */

  /* loop spec */
  GslWaveLoopType loop_type : 16;
  guint		  pploop_ends_backwards : 1;
  guint		  mini_loop : 1;
  GslLong	  loop_start;
  GslLong	  loop_end;
  guint		  loop_count;

  /* preformatted blocks */
  GslWaveChunkMem head;
  GslWaveChunkMem enter;
  GslWaveChunkMem wrap;
  GslWaveChunkMem ppwrap;
  GslWaveChunkMem leave;
  GslWaveChunkMem tail;
  GslLong	  leave_end_norm;
  GslLong	  tail_start_norm;
};
struct _GslWaveChunkBlock
{
  /* requisition (in) */
  gint		play_dir;	/* usually +1 */
  GslLong	offset;		/* requested offset into wave */
  /* result (out) */
  GslLong	length;		/* resulting signed? length of block in # values */
  gboolean	is_silent;	/* sample end reached, values are 0 */
  gint		dirstride;	/* >0 => increment, <0 => decrement */
  gfloat       *start;		/* first data value location */
  gfloat       *end;		/* last data value location +1 */
  GslLong	next_offset;	/* offset of next adjunct block */
  /*< private >*/
  gpointer	node;
};


/* --- prototypes --- */
void		gsl_wave_chunk_use_block	(GslWaveChunk		*wave_chunk,
						 GslWaveChunkBlock	*block);
void		gsl_wave_chunk_unuse_block	(GslWaveChunk		*wave_chunk,
						 GslWaveChunkBlock	*block);
GslWaveChunk*	_gsl_wave_chunk_create		(GslDataCache		*dcache,
						 GslLong		 offset,
						 GslLong		 n_values,
						 guint			 n_channels,
						 gfloat			 osc_freq,
						 gfloat			 mix_freq,
						 GslWaveLoopType	 loop_type,
						 GslLong		 loop_start,
						 GslLong		 loop_end,
						 guint			 loop_count);
void		_gsl_wave_chunk_destroy		(GslWaveChunk		*wchunk);
void		gsl_wave_chunk_debug_block	(GslWaveChunk		*wchunk,
						 GslLong		 offset,
						 GslLong		 length,
						 gfloat			*block);
GslWaveChunk*	gsl_wave_chunk_copy		(GslWaveChunk		*wchunk);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_WAVE_CHUNK_H__ */
