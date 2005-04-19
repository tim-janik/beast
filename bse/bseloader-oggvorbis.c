/* GSL - Generic Sound Layer
 * Copyright (C) 2000-2005 Tim Janik
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
#include "bse/bseloader.h"

#include <bse/gsldatahandle.h>
#include "gsldatahandle-vorbis.h"

#include <vorbis/vorbisfile.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>


/* --- structures --- */
typedef struct
{
  BseWaveFileInfo wfi;
  OggVorbis_File  ofile;
} FileInfo;
#define LOADER_LOGICAL_BIT_STREAM(chunk)    ((chunk).loader_data[0].uint)

/* --- functions --- */
static BseWaveFileInfo*
oggv_load_file_info (gpointer      data,
		     const gchar  *file_name,
		     BseErrorType *error_p)
{
  FileInfo *fi = sfi_new_struct0 (FileInfo, 1);
  FILE *file;
  gint err, i;
  
  file = fopen (file_name, "r");
  if (!file)
    {
      *error_p = gsl_error_from_errno (errno, BSE_ERROR_FILE_OPEN_FAILED);
      return NULL;
    }
  
  fi = sfi_new_struct0 (FileInfo, 1);
  err = ov_open (file, &fi->ofile, NULL, 0);
  if (err)
    {
      fclose (file);
      sfi_delete_struct (FileInfo, fi);
      *error_p = BSE_ERROR_CODEC_FAILURE;
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
oggv_free_file_info (gpointer         data,
		     BseWaveFileInfo *file_info)
{
  FileInfo *fi = (FileInfo*) file_info;
  guint i;

  for (i = 0; i < fi->wfi.n_waves; i++)
    g_free (fi->wfi.waves[i].name);
  g_free (fi->wfi.waves);
  ov_clear (&fi->ofile);
  sfi_delete_struct (FileInfo, fi);
}

static BseWaveDsc*
oggv_load_wave_dsc (gpointer         data,
		    BseWaveFileInfo *file_info,
		    guint            nth_wave,
		    BseErrorType    *error_p)
{
  FileInfo *fi = (FileInfo*) file_info;
  BseWaveDsc *wdsc = sfi_new_struct0 (BseWaveDsc, 1);
  vorbis_info *vi = ov_info (&fi->ofile, nth_wave);

  wdsc->name = g_strdup (fi->wfi.waves[nth_wave].name);
  wdsc->n_channels = vi->channels;
  wdsc->n_chunks = 1;
  wdsc->chunks = g_new0 (BseWaveChunkDsc, 1);
  wdsc->chunks[0].osc_freq = 440.0; /* FIXME */
  wdsc->chunks[0].mix_freq = vi->rate;
  LOADER_LOGICAL_BIT_STREAM (wdsc->chunks[0]) = nth_wave;

  return wdsc;
}

static void
oggv_free_wave_dsc (gpointer    data,
		    BseWaveDsc *wdsc)
{
  guint i;
  for (i = 0; i < wdsc->n_chunks; i++)
    g_strfreev (wdsc->chunks[i].xinfos);
  g_free (wdsc->chunks);
  g_free (wdsc->name);
  sfi_delete_struct (BseWaveDsc, wdsc);
}

static GslDataHandle*
oggv_create_chunk_handle (gpointer      data,
			  BseWaveDsc   *wdsc,
			  guint         nth_chunk,
			  BseErrorType *error_p)
{
  FileInfo *fi = (FileInfo*) wdsc->file_info;
  GslDataHandle *dhandle;

  g_return_val_if_fail (nth_chunk == 0, NULL);

  dhandle = gsl_data_handle_new_ogg_vorbis_muxed (fi->wfi.file_name,
					          LOADER_LOGICAL_BIT_STREAM (wdsc->chunks[0]),
					          wdsc->chunks[0].osc_freq);
  if (dhandle && wdsc->chunks[0].xinfos)
    {
      GslDataHandle *tmp_handle = dhandle;
      dhandle = gsl_data_handle_new_add_xinfos (dhandle, wdsc->chunks[0].xinfos);
      gsl_data_handle_unref (tmp_handle);
    }
  if (!dhandle)
    *error_p = BSE_ERROR_FILE_OPEN_FAILED;
  return dhandle;
}

void
_gsl_init_loader_oggvorbis (void)
{
  static const gchar *file_exts[] = { "ogg", NULL, };
  static const gchar *mime_types[] = { "application/ogg", "application/x-ogg", "audio/x-vorbis", "audio/x-ogg", NULL, };
  static const gchar *magics[] = { "0 string OggS\n" "29 string vorbis", NULL, };
  static BseLoader loader = {
    "Ogg/Vorbis",
    file_exts,
    mime_types,
    0, /* flags */
    magics,
    0,  /* priority */
    NULL,
    oggv_load_file_info,
    oggv_free_file_info,
    oggv_load_wave_dsc,
    oggv_free_wave_dsc,
    oggv_create_chunk_handle,
  };
  static gboolean initialized = FALSE;

  g_assert (initialized == FALSE);
  initialized = TRUE;

  bse_loader_register (&loader);
}
