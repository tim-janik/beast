/* GSL - Generic Sound Layer
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
#ifndef __GSL_LOADER_H__
#define __GSL_LOADER_H__

#include <gsl/gsldefs.h>
#include <gsl/gslcommon.h>
#include <gsl/gslwavechunk.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



typedef struct _GslWaveFileInfo GslWaveFileInfo;
typedef struct _GslWaveDsc      GslWaveDsc;
typedef struct _GslWaveChunkDsc GslWaveChunkDsc;

/* --- structures --- */
struct _GslWaveFileInfo
{
  guint	   n_waves;
  struct {
    gchar *name;
  }       *waves;

  /*< private >*/
  gchar     *file_name;
  GslLoader *loader;
  guint      ref_count;
};
struct _GslWaveDsc
{
  gchar		  *name;
  guint	           n_chunks;
  GslWaveChunkDsc *chunks;
  guint            n_channels;
  /*< private >*/
  GslWaveFileInfo *file_info;
};
struct _GslWaveChunkDsc
{
  gfloat	  osc_freq;
  gfloat	  mix_freq;
  GslWaveLoopType loop_type;
  GslLong	  loop_start;	/* sample offset */
  GslLong	  loop_end;	/* sample offset */
  guint		  loop_count;
  /* loader-specific */
  GslLong         loader_offset;
  GslLong         loader_length;
  gpointer	  loader_data1; /* generic pointers for more data */
  gpointer	  loader_data2;
};


/* --- functions --- */
GslWaveFileInfo*       gsl_wave_file_info_load	(const gchar	 *file_name,
						 GslErrorType	 *error);
GslWaveFileInfo*       gsl_wave_file_info_ref   (GslWaveFileInfo *wave_file_info);
void                   gsl_wave_file_info_unref (GslWaveFileInfo *wave_file_info);
GslWaveDsc*	       gsl_wave_dsc_load	(GslWaveFileInfo *wave_file_info,
						 guint		  nth_wave,
						 GslErrorType	 *error);
void		       gsl_wave_dsc_free	(GslWaveDsc	 *wave_dsc);
GslDataHandle*	       gsl_wave_handle_create	(GslWaveDsc	 *wave_dsc,
						 guint		  nth_chunk,
						 GslErrorType	 *error);
GslWaveChunk*	       gsl_wave_chunk_create	(GslWaveDsc	 *wave_dsc,
						 guint		  nth_chunk,
						 GslErrorType	 *error);


/* --- loader impl --- */
struct _GslLoader
{
  const gchar *name;		/* format/loader name, e.g. "GslWave" or "WAVE audio, RIFF (little-endian)" */

  /* at least one of the
   * following three must
   * be non-NULL
   */
  const gchar **extensions;	/* e.g.: "mp3", "ogg" or "gslwave" */
  const gchar **mime_types;	/* e.g.: "audio/x-mpg3" or "audio/x-wav" */
  const gchar **magic_specs;	/* e.g.: "0 string RIFF\n8 string WAVE\n" or "0 string #GslWave\n" */

  gint   priority;   /* -100=high, +100=low, 0=default */

  /*< private >*/
  gpointer		  data;
  GslWaveFileInfo*	(*load_file_info)	(gpointer	   data,
						 const gchar	  *file_name,
						 GslErrorType	  *error);
  void			(*free_file_info)	(gpointer	   data,
						 GslWaveFileInfo  *file_info);
  GslWaveDsc*		(*load_wave_dsc)	(gpointer	   data,
						 GslWaveFileInfo  *file_info,
						 guint		   nth_wave,
						 GslErrorType	  *error);
  void			(*free_wave_dsc)	(gpointer	   data,
						 GslWaveDsc	  *wave_dsc);
  GslDataHandle*	(*create_chunk_handle)	(gpointer	   data,
						 GslWaveDsc	  *wave_dsc,
						 guint		   nth_chunk,
						 GslErrorType	  *error);
  GslLoader   *next;	/* must be NULL */
};

void		gsl_loader_register	(GslLoader	*loader);
GslLoader*	gsl_loader_match	(const gchar	*file_name);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_LOADER_H__ */
