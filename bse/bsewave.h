/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2001 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BSE_WAVE_H__
#define __BSE_WAVE_H__

#include	<bse/bsesource.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- BSE type macros --- */
#define BSE_TYPE_WAVE		   (BSE_TYPE_ID (BseWave))
#define BSE_WAVE(object)	   (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_WAVE, BseWave))
#define BSE_WAVE_CLASS(class)	   (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_WAVE, BseWaveClass))
#define BSE_IS_WAVE(object)	   (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_WAVE))
#define BSE_IS_WAVE_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_WAVE))
#define BSE_WAVE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_WAVE, BseWaveClass))


/* --- BseWave --- */
typedef struct _BseSourceClass    BseWaveClass;
typedef struct
{
  guint		 n_wchunks;
  GslWaveChunk **wchunks;
} BseWaveIndex;
struct _BseWave
{
  BseSource	     parent_object;

  guint		     n_channels;

  /* locator */
  guint		     locator_set : 1;
  gchar		    *file_name;
  gchar		    *wave_name;
  GSList	    *wave_chunk_urls;

  /* wave chunks */
  guint		     n_wchunks;
  GSList	    *wave_chunks;

  /* prepared-state keep-alive-cache */
  GSList	    *abandoned_chunks;

  /* prepared-state GslModule index */
  GSList	    *index_list;
};


/* --- prototypes -- */
void		bse_wave_add_chunk		(BseWave	*wave,
						 GslWaveChunk	*wchunk);
void		bse_wave_add_chunk_with_locator	(BseWave	*wave,
						 GslWaveChunk	*wchunk,
						 const gchar	*file_name,
						 const gchar	*wave_name);
GslWaveChunk*   bse_wave_lookup_chunk           (BseWave        *wave,
						 gfloat		 osc_freq,
						 gfloat		 mix_freq);
void            bse_wave_remove_chunk           (BseWave        *wave,
						 GslWaveChunk   *wchunk);
BseErrorType	bse_wave_load_wave		(BseWave	*wave,
						 const gchar	*file_name,
						 const gchar	*wave_name,
						 GDArray	*list_array,
						 GDArray	*skip_array);
void		bse_wave_set_locator		(BseWave	*wave,
						 const gchar	*file_name,
						 const gchar	*wave_name);
BseWaveIndex*	bse_wave_get_index_for_modules	(BseWave	*wave);
/* BseWaveIndex is safe to use from GslModules */
GslWaveChunk*	bse_wave_index_lookup_best	(BseWaveIndex	*windex,
						 gfloat		 osc_freq);
						 


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_WAVE_H__ */
