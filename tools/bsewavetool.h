/* BseWaveTool - BSE Wave creation tool
 * Copyright (C) 2001-2004 Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include <bse/gsldatahandle.h>
#include <bse/gslwavechunk.h>

#include "bseloopfuncs.h"


/* --- structures --- */
typedef struct
{
  gfloat          osc_freq;
  guint		  midi_note;
  gfloat          mix_freq;
  GslDataHandle  *dhandle;
  
  GslWaveLoopType loop_type;
  GslLong	  loop_start;
  GslLong	  loop_end;
  
  /*< private >*/
  gchar		 *dump_name;
  gchar		 *dump_index;
  gchar          *oggname;
} BseWtChunk;

typedef struct
{
  gchar      *wave_name;
  guint       n_channels;
  guint       n_chunks;
  BseWtChunk *chunks;
} BseWtWave;

typedef struct
{
  gfloat threshold;	/* 0..+1 */
  guint  head_detect;
  guint  tail_detect;
  guint  head_fade;
  guint  tail_pad;
  guint  min_tail;
} GslLevelClip;

typedef enum
{
  GSL_LEVEL_UNCLIPPED,	/* no silence */
  GSL_LEVEL_CLIP_IO_ERROR,
  GSL_LEVEL_CLIP_FAILED_HEAD_DETECT,
  GSL_LEVEL_CLIP_FAILED_TAIL_DETECT,
  GSL_LEVEL_CLIP_ALL,	/* all silence */
  GSL_LEVEL_CLIPPED_HEAD,
  GSL_LEVEL_CLIPPED_TAIL,
  GSL_LEVEL_CLIPPED_HEAD_TAIL
} GslLevelClipStatus;


/* --- functions --- */
BseWtWave*	bse_wt_new_wave			(const gchar	*wave_name,
						 guint           n_channels);
void		bse_wt_add_chunk		(BseWtWave	*wave,
						 gfloat		 mix_freq,
						 gfloat		 osc_freq,
						 GslDataHandle	*dhandle);
void		bse_wt_add_chunk_midi		(BseWtWave	*wave,
						 gfloat		 mix_freq,
						 guint		 midi_note,
						 GslDataHandle	*dhandle);
void		bse_wt_remove_chunk		(BseWtWave	*wave,
						 guint		 nth_chunk);
void		bse_wt_free_wave		(BseWtWave	*wave);
GslErrorType	bse_wt_chunks_dump_wav		(BseWtWave	*wave,
						 const gchar	*base_dir,
						 const gchar	*prefix);
GslErrorType	bse_wt_dump_bsewave		(BseWtWave	*wave,
						 const gchar	*file_name);
GslErrorType	bse_wt_dump_bsewave_header	(BseWtWave	*wave,
						 const gchar	*file_name);

GslErrorType	bse_wt_dump_bsewave_wav		(BseWtWave	*wave,
						 const gchar	*base_dir,
						 const gchar	*bsewave, /* relative */
						 const gchar	*chunk_prefix); /* relative */

GslDataHandle*	gsl_data_level_clip_sample	(GslDataHandle	    *dhandle,
						 GslLevelClip	    *conf,
						 GslLevelClipStatus *status);
gfloat*		gsl_data_make_fade_ramp		(GslDataHandle	    *handle,
						 GslLong	     min_pos, /* *= 0.0 + delta */
						 GslLong	     max_pos, /* *= 1.0 - delta */
						 GslLong	    *length_p);
