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
#include "gslloader.h"

#include "gslcommon.h"
#include "gsldatahandle.h"
#include "gslmagic.h"
#include "gslfilehash.h"


/* --- variables --- */
static GslLoader *gsl_loader_list = NULL;
static GslRing   *gsl_magic_list1 = NULL;
static GslRing   *gsl_magic_list2 = NULL;


/* --- functions --- */
static GslLoader*
loader_find_by_name (const gchar *name)
{
  GslLoader *loader;

  for (loader = gsl_loader_list; loader != NULL; loader = loader->next)
    if (strcmp (name, loader->name) == 0)
      return loader;
  return NULL;
}

void
gsl_loader_register (GslLoader *loader)
{
  g_return_if_fail (loader != NULL);
  g_return_if_fail (loader->name != NULL);
  g_return_if_fail (loader->extensions || loader->mime_types || loader->magic_specs);
  g_return_if_fail (loader_find_by_name (loader->name) == NULL);
  g_return_if_fail (loader->next == NULL);
  g_return_if_fail (loader->load_file_info != NULL);
  g_return_if_fail (loader->free_file_info != NULL);
  g_return_if_fail (loader->load_wave_dsc != NULL);
  g_return_if_fail (loader->free_wave_dsc != NULL);
  g_return_if_fail (loader->create_chunk_handle != NULL);
  
  loader->next = gsl_loader_list;
  gsl_loader_list = loader;

  if (loader->magic_specs)
    {
      GslMagic *magic;
      guint i, j;

      for (i = 0; loader->magic_specs[i]; i++)
	{
	  if (loader->extensions)
	    for (j = 0; loader->extensions[j]; j++)
	      {
		magic = gsl_magic_create (loader, loader->priority,
					  loader->extensions[j], loader->magic_specs[i]);
		gsl_magic_list1 = gsl_ring_append (gsl_magic_list1, magic);
		if (loader->flags & GSL_LOADER_SKIP_PRECEEDING_NULLS)
		  gsl_magic_list2 = gsl_ring_append (gsl_magic_list2, magic);
	      }
	  else
	    {
	      magic = gsl_magic_create (loader, loader->priority,
					NULL, loader->magic_specs[i]);
	      gsl_magic_list1 = gsl_ring_append (gsl_magic_list1, magic);
	      if (loader->flags & GSL_LOADER_SKIP_PRECEEDING_NULLS)
		gsl_magic_list2 = gsl_ring_append (gsl_magic_list2, magic);
	    }
	}
    }
}

static guint8*
skipchr (const guint8 *mem,
	 gchar         byte,
	 guint         maxlen)
{
  const guint8 *p = mem, *bound = p + maxlen;

  while (p < bound)
    if_reject (*p++ != byte)
      return (guint8*) p - 1;
  return NULL;
}

GslLoader*
gsl_loader_match (const gchar *file_name)
{
  GslMagic *magic = NULL;

  g_return_val_if_fail (file_name != NULL, NULL);

  /* normal magic check */
  magic = gsl_magic_list_match_file (gsl_magic_list1, file_name);

  /* in a sort-of fallback attempt,
   * work around files that have preceeding nulls
   */
  if (!magic && gsl_magic_list2)
    {
      guint8 buffer[1024], *p = NULL;
      GslLong n, pos = 0;
      GslHFile *hfile = gsl_hfile_open (file_name);
      if (!hfile)
	return NULL;
      while (!p)
	{
	  n = gsl_hfile_pread (hfile, pos, sizeof (buffer), buffer);
	  if (n < 1)
	    {
	      pos = 0;
	      break;
	    }
	  p = skipchr (buffer, 0, n);
	  if (p)
	    {
	      pos += p - buffer;
	      break;
	    }
	  else
	    pos += n;
	}
      gsl_hfile_close (hfile);
      if (pos > 0)
	magic = gsl_magic_list_match_file_skip (gsl_magic_list2, file_name, pos);
    }

  return magic ? magic->data : NULL;
}

GslWaveFileInfo*
gsl_wave_file_info_load (const gchar  *file_name,
			 GslErrorType *error_p)
{
  GslWaveFileInfo *finfo = NULL;
  GslErrorType error = GSL_ERROR_NONE;
  GslLoader *loader;
  
  if (error_p)
    *error_p = GSL_ERROR_INTERNAL;
  g_return_val_if_fail (file_name != NULL, NULL);

  loader = gsl_loader_match (file_name);
  if (loader)
    {
      finfo = loader->load_file_info (loader->data, file_name, &error);
      if (error && finfo)
	{
	  /* loaders shouldn't do this */
	  loader->free_file_info (loader->data, finfo);
	  finfo = NULL;
	}
      if (!finfo && !error)
	error = GSL_ERROR_FILE_EMPTY;	/* FIXME: try next loader */
      if (finfo)
	{
	  if (finfo->n_waves > 0)
	    {
	      guint i;

	      g_return_val_if_fail (finfo->loader == NULL, NULL);
	      g_return_val_if_fail (finfo->file_name == NULL, NULL);
	      
	      for (i = 0; i < finfo->n_waves; i++)
		g_return_val_if_fail (finfo->waves[i].name != NULL, NULL);
	      
	      finfo->file_name = g_strdup (file_name);
	      finfo->loader = loader;
	      finfo->ref_count = 1;
	    }
	  else
	    {
	      loader->free_file_info (loader->data, finfo);
	      finfo = NULL;
	      error = GSL_ERROR_FILE_EMPTY;   /* FIXME: try next loader */
	    }
	}
    }
  else /* no loader match */
    {
      /* try to provide apropriate error code */
      error = gsl_check_file (file_name, "rf");
      if (!error)
	error = GSL_ERROR_FORMAT_UNKNOWN;
    }

  if (error_p)
    *error_p = error;

  return finfo;
}

void
gsl_wave_file_info_unref (GslWaveFileInfo *wave_file_info)
{
  g_return_if_fail (wave_file_info != NULL);
  g_return_if_fail (wave_file_info->ref_count > 0);

  wave_file_info->ref_count--;
  if (!wave_file_info->ref_count)
    {
      GslLoader *loader = wave_file_info->loader;

      g_free (wave_file_info->file_name);
      wave_file_info->file_name = NULL;
      wave_file_info->loader = NULL;

      loader->free_file_info (loader->data, wave_file_info);
    }
}

GslWaveFileInfo*
gsl_wave_file_info_ref (GslWaveFileInfo *wave_file_info)
{
  g_return_val_if_fail (wave_file_info != NULL, NULL);
  g_return_val_if_fail (wave_file_info->ref_count > 0, NULL);

  wave_file_info->ref_count++;

  return wave_file_info;
}

GslWaveDsc*
gsl_wave_dsc_load (GslWaveFileInfo *wave_file_info,
		   guint            nth_wave,
		   GslErrorType    *error_p)
{
  GslErrorType error = GSL_ERROR_NONE;
  GslWaveDsc *wdsc;
  GslLoader *loader;

  if (error_p)
    *error_p = GSL_ERROR_INTERNAL;
  g_return_val_if_fail (wave_file_info != NULL, NULL);
  g_return_val_if_fail (wave_file_info->loader != NULL, NULL);
  g_return_val_if_fail (nth_wave < wave_file_info->n_waves, NULL);

  loader = wave_file_info->loader;
  wdsc = loader->load_wave_dsc (loader->data, wave_file_info, nth_wave,&error);

  if (error && wdsc)
    {
      /* loaders shouldn't do this */
      loader->free_wave_dsc (loader->data, wdsc);
      wdsc = NULL;
    }
  if (!wdsc && !error)
    error = GSL_ERROR_FILE_EMPTY;
  if (wdsc)
    {
      if (wdsc->n_chunks > 0)
	{
	  g_return_val_if_fail (wdsc->file_info == NULL, NULL);
	  g_return_val_if_fail (wdsc->name && strcmp (wdsc->name, wave_file_info->waves[nth_wave].name) == 0, NULL);
	  
	  wdsc->file_info = wave_file_info;
	  gsl_wave_file_info_ref (wave_file_info);
	}
      else
	{
	  loader->free_wave_dsc (loader->data, wdsc);
	  wdsc = NULL;
	  error = GSL_ERROR_FILE_EMPTY;
	}
    }

  if (error_p)
    *error_p = error;
  
  return wdsc;
}

void
gsl_wave_dsc_free (GslWaveDsc *wave_dsc)
{
  GslWaveFileInfo *file_info;

  g_return_if_fail (wave_dsc != NULL);
  g_return_if_fail (wave_dsc->file_info != NULL);

  file_info = wave_dsc->file_info;
  wave_dsc->file_info = NULL;
  
  file_info->loader->free_wave_dsc (file_info->loader->data, wave_dsc);

  gsl_wave_file_info_unref (file_info);
}

GslDataHandle*
gsl_wave_handle_create (GslWaveDsc   *wave_dsc,
			guint	      nth_chunk,
			GslErrorType *error_p)
{
  GslErrorType error = GSL_ERROR_NONE;
  GslDataHandle *dhandle;
  GslLoader *loader;

  if (error_p)
    *error_p = GSL_ERROR_INTERNAL;
  g_return_val_if_fail (wave_dsc != NULL, NULL);
  g_return_val_if_fail (wave_dsc->file_info != NULL, NULL);
  g_return_val_if_fail (nth_chunk < wave_dsc->n_chunks, NULL);

  loader = wave_dsc->file_info->loader;

  dhandle = loader->create_chunk_handle (loader->data,
					 wave_dsc,
					 nth_chunk,
					 &error);
  if (error && dhandle)
    {
      /* loaders shouldn't do this */
      gsl_data_handle_unref (dhandle);
      dhandle = NULL;
    }
  if (!dhandle && !error)
    error = GSL_ERROR_FORMAT_INVALID;

  if (error_p)
    *error_p = error;

  return dhandle;
}

GslWaveChunk*
gsl_wave_chunk_create (GslWaveDsc   *wave_dsc,
		       guint         nth_chunk,
		       GslErrorType *error_p)
{
  GslDataHandle *dhandle;
  GslDataCache *dcache;
  GslWaveChunk *wchunk;

  if (error_p)
    *error_p = GSL_ERROR_INTERNAL;
  g_return_val_if_fail (wave_dsc != NULL, NULL);
  g_return_val_if_fail (nth_chunk < wave_dsc->n_chunks, NULL);

  dhandle = gsl_wave_handle_create (wave_dsc, nth_chunk, error_p);
  if (!dhandle)
    return NULL;

  if (error_p)
    *error_p = GSL_ERROR_IO;

  /* FIXME: we essentially create a dcache for each wchunk here ;( */

  dcache = gsl_data_cache_from_dhandle (dhandle, gsl_get_config ()->wave_chunk_padding * wave_dsc->n_channels);
  gsl_data_handle_unref (dhandle);
  if (!dcache)
    return NULL;
  /* dcache keeps dhandle alive */

  wchunk = gsl_wave_chunk_new (dcache,
			       wave_dsc->chunks[nth_chunk].osc_freq,
			       wave_dsc->chunks[nth_chunk].mix_freq,
			       wave_dsc->chunks[nth_chunk].loop_type,
			       wave_dsc->chunks[nth_chunk].loop_start,
			       wave_dsc->chunks[nth_chunk].loop_end,
			       wave_dsc->chunks[nth_chunk].loop_count);
  gsl_data_cache_unref (dcache);

  if (error_p && wchunk)
    *error_p = GSL_ERROR_NONE;

  return wchunk;
}
