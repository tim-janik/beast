/* GSL - Generic Sound Layer
 * Copyright (C) 1998, 2000, 2001 Tim Janik
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
#include "gsl/gslloader.h"

#include <gsl/gsldatahandle.h>
#include "gsldatahandle-vorbis.h"

#include <vorbis/vorbisfile.h>
#include <unistd.h>


/* --- structures --- */
typedef struct
{
  GslWaveFileInfo wfi;
  OggVorbis_File  ofile;
} FileInfo;


/* --- functions --- */
static GslWaveFileInfo*
load_file_info (gpointer      data,
		const gchar  *file_name,
		GslErrorType *error_p)
{
  FileInfo *fi = gsl_new_struct0 (FileInfo, 1);
  FILE *file;
  gint err, i;
  
  file = fopen (file_name, "r");
  if (!file)
    {
      *error_p = GSL_ERROR_OPEN_FAILED;
      return NULL;
    }
  
  fi = gsl_new_struct0 (FileInfo, 1);
  err = ov_open (file, &fi->ofile, NULL, 0);
  if (err)
    {
      fclose (file);
      gsl_delete_struct (FileInfo, fi);
      *error_p = GSL_ERROR_CODEC_FAILURE;
      return NULL;
    }

  fi->wfi.n_waves = ov_streams (&fi->ofile);
  fi->wfi.waves = g_malloc0 (sizeof (fi->wfi.waves[0]) * fi->wfi.n_waves);
  for (i = 0; i < fi->wfi.n_waves; i++)
    {
      vorbis_comment *vc = ov_comment (&fi->ofile, i);
      guint n;

      for (n = 0; n < vc->comments; n++)
	if (strcmp (vc->user_comments[n], "title=") == 0)
	  break;
      if (n < vc->comments)
	fi->wfi.waves[i].name = g_strdup (vc->user_comments[n] + 6);
      else
	fi->wfi.waves[i].name = g_strdup_printf ("Unnamed-%u", i);
    }

  return &fi->wfi;
}

static void
free_file_info (gpointer         data,
		GslWaveFileInfo *file_info)
{
  FileInfo *fi = (FileInfo*) file_info;
  guint i;

  for (i = 0; i < fi->wfi.n_waves; i++)
    g_free (fi->wfi.waves[i].name);
  g_free (fi->wfi.waves);
  ov_clear (&fi->ofile);
  gsl_delete_struct (FileInfo, fi);
}

static GslWaveDsc*
load_wave_dsc (gpointer         data,
	       GslWaveFileInfo *file_info,
	       guint            nth_wave,
	       GslErrorType    *error_p)
{
  FileInfo *fi = (FileInfo*) file_info;
  GslWaveDsc *wdsc = gsl_new_struct0 (GslWaveDsc, 1);
  vorbis_info *vi = ov_info (&fi->ofile, nth_wave);

  wdsc->name = g_strdup (fi->wfi.waves[nth_wave].name);
  wdsc->n_channels = vi->channels;
  wdsc->n_chunks = 1;
  wdsc->chunks = g_new0 (GslWaveChunkDsc, 1);
  wdsc->chunks[0].osc_freq = 440.0; // FIXME
  wdsc->chunks[0].mix_freq = vi->rate;
  wdsc->chunks[0].loader_offset = nth_wave;	/* lbitstream */

  return wdsc;
}

static void
free_wave_dsc (gpointer    data,
	       GslWaveDsc *wdsc)
{
  g_free (wdsc->name);
  g_free (wdsc->chunks);
  gsl_delete_struct (GslWaveDsc, wdsc);
}

static GslDataHandle*
create_chunk_handle (gpointer      data,
		     GslWaveDsc   *wdsc,
		     guint         nth_chunk,
		     GslErrorType *error_p)
{
  FileInfo *fi = (FileInfo*) wdsc->file_info;
  GslDataHandle *dhandle;

  g_return_val_if_fail (nth_chunk == 0, NULL);

  dhandle = gsl_data_handle_new_ogg_vorbis (fi->wfi.file_name,
					    wdsc->chunks[0].loader_offset); /* lbitstream */
  if (!dhandle)
    *error_p = GSL_ERROR_OPEN_FAILED;
  return dhandle;
}

void
_gsl_init_loader_oggvorbis (void)
{
  static gboolean initialized = FALSE;
  static GslLoader ov_loader = {
    "Ogg/Vorbis",
    "ogg",
    "audio/x-ogg",
    "0 string OggS\n"
    "29 string vorbis",
    0,
    NULL,
    load_file_info,
    free_file_info,
    load_wave_dsc,
    free_wave_dsc,
    create_chunk_handle,
  };

  g_assert (initialized == FALSE);
  initialized = TRUE;

  gsl_loader_register (&ov_loader);
}
