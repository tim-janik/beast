/* BSE - Better Sound Engine
 * Copyright (C) 2001-2005 Tim Janik
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
#include "bseloader.h"
#include "bsemain.h"
#include "gslcommon.h"
#include "gsldatahandle.h"
#include "gslmagic.h"
#include "gslfilehash.h"
#include <string.h>


/* --- variables --- */
static BseLoader *bse_loader_list = NULL;
static SfiRing   *gsl_magic_list1 = NULL;
static SfiRing   *gsl_magic_list2 = NULL;


/* --- functions --- */
static BseLoader*
loader_find_by_name (const char *name)
{
  BseLoader *loader;

  for (loader = bse_loader_list; loader != NULL; loader = loader->next)
    if (strcmp (name, loader->name) == 0)
      return loader;
  return NULL;
}

void
bse_loader_register (BseLoader *loader)
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
  
  loader->next = bse_loader_list;
  bse_loader_list = loader;

  if (loader->magic_specs)
    {
      GslMagic *magic;
      uint i, j;

      for (i = 0; loader->magic_specs[i]; i++)
	{
	  if (loader->extensions)
	    for (j = 0; loader->extensions[j]; j++)
	      {
		magic = gsl_magic_create (loader, loader->priority,
					  loader->extensions[j], loader->magic_specs[i]);
		gsl_magic_list1 = sfi_ring_append (gsl_magic_list1, magic);
		if (loader->flags & BSE_LOADER_SKIP_PRECEEDING_NULLS)
		  gsl_magic_list2 = sfi_ring_append (gsl_magic_list2, magic);
	      }
	  else
	    {
	      magic = gsl_magic_create (loader, loader->priority,
					NULL, loader->magic_specs[i]);
	      gsl_magic_list1 = sfi_ring_append (gsl_magic_list1, magic);
	      if (loader->flags & BSE_LOADER_SKIP_PRECEEDING_NULLS)
		gsl_magic_list2 = sfi_ring_append (gsl_magic_list2, magic);
	    }
	}
    }
}

static uint8*
skipchr (const uint8 *mem,
	 char         byte,
	 uint         maxlen)
{
  const uint8 *p = mem, *bound = p + maxlen;

  while (p < bound)
    if (UNLIKELY (*p++ != byte))
      return (uint8*) p - 1;
  return NULL;
}

BseLoader*
bse_loader_match (const char *file_name)
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
      uint8 buffer[1024], *p = NULL;
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

  return magic ? (BseLoader*) magic->data : NULL;
}

BseWaveFileInfo*
bse_wave_file_info_load (const char   *file_name,
			 BseErrorType *error_p)
{
  BseWaveFileInfo *finfo = NULL;
  BseErrorType error = BSE_ERROR_NONE;
  BseLoader *loader;
  
  if (error_p)
    *error_p = BSE_ERROR_INTERNAL;
  g_return_val_if_fail (file_name != NULL, NULL);

  loader = bse_loader_match (file_name);
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
	error = BSE_ERROR_FILE_EMPTY;	/* FIXME: try next loader */
      if (finfo)
	{
	  if (finfo->n_waves > 0)
	    {
	      uint i;

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
	      error = BSE_ERROR_FILE_EMPTY;   /* FIXME: try next loader */
	    }
	}
    }
  else /* no loader match */
    {
      /* try to provide apropriate error code */
      error = gsl_file_check (file_name, "rf");
      if (!error)
	error = BSE_ERROR_FORMAT_UNKNOWN;
    }

  if (error_p)
    *error_p = error;

  return finfo;
}

void
bse_wave_file_info_unref (BseWaveFileInfo *wave_file_info)
{
  g_return_if_fail (wave_file_info != NULL);
  g_return_if_fail (wave_file_info->ref_count > 0);

  wave_file_info->ref_count--;
  if (!wave_file_info->ref_count)
    {
      BseLoader *loader = wave_file_info->loader;

      g_free (wave_file_info->file_name);
      wave_file_info->file_name = NULL;
      wave_file_info->loader = NULL;
      g_strfreev (wave_file_info->comments);
      wave_file_info->comments = NULL;

      loader->free_file_info (loader->data, wave_file_info);
    }
}

BseWaveFileInfo*
bse_wave_file_info_ref (BseWaveFileInfo *wave_file_info)
{
  g_return_val_if_fail (wave_file_info != NULL, NULL);
  g_return_val_if_fail (wave_file_info->ref_count > 0, NULL);

  wave_file_info->ref_count++;

  return wave_file_info;
}

const char*
bse_wave_file_info_loader (BseWaveFileInfo *fi)
{
  g_return_val_if_fail (fi != NULL, NULL);

  return fi->loader->name;
}

BseWaveDsc*
bse_wave_dsc_load (BseWaveFileInfo *wave_file_info,
		   uint             nth_wave,
                   gboolean         accept_empty,
                   BseErrorType    *error_p)
{
  BseErrorType error = BSE_ERROR_NONE;
  BseWaveDsc *wdsc;
  BseLoader *loader;

  if (error_p)
    *error_p = BSE_ERROR_INTERNAL;
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
    error = BSE_ERROR_FILE_EMPTY;
  if (wdsc)
    {
      if (accept_empty || wdsc->n_chunks > 0)
	{
	  g_return_val_if_fail (wdsc->file_info == NULL, NULL);
	  g_return_val_if_fail (wdsc->name && strcmp (wdsc->name, wave_file_info->waves[nth_wave].name) == 0, NULL);
	  
	  wdsc->file_info = wave_file_info;
	  bse_wave_file_info_ref (wave_file_info);
	}
      else
	{
	  loader->free_wave_dsc (loader->data, wdsc);
	  wdsc = NULL;
	  error = BSE_ERROR_FILE_EMPTY;
	}
    }

  if (error_p)
    *error_p = error;
  
  return wdsc;
}

void
bse_wave_dsc_free (BseWaveDsc *wave_dsc)
{
  g_return_if_fail (wave_dsc != NULL);
  g_return_if_fail (wave_dsc->file_info != NULL);

  BseWaveFileInfo *file_info = wave_dsc->file_info;

  file_info->loader->free_wave_dsc (file_info->loader->data, wave_dsc);

  bse_wave_file_info_unref (file_info);
}

GslDataHandle*
bse_wave_handle_create (BseWaveDsc   *wave_dsc,
			uint	      nth_chunk,
			BseErrorType *error_p)
{
  BseErrorType error = BSE_ERROR_NONE;
  GslDataHandle *dhandle;
  BseLoader *loader;

  if (error_p)
    *error_p = BSE_ERROR_INTERNAL;
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
    error = BSE_ERROR_FORMAT_INVALID;

  if (error_p)
    *error_p = error;

  return dhandle;
}

GslWaveChunk*
bse_wave_chunk_create (BseWaveDsc   *wave_dsc,
		       uint          nth_chunk,
		       BseErrorType *error_p)
{
  GslDataHandle *dhandle;
  GslDataCache *dcache;
  GslWaveChunk *wchunk;

  if (error_p)
    *error_p = BSE_ERROR_INTERNAL;
  g_return_val_if_fail (wave_dsc != NULL, NULL);
  g_return_val_if_fail (nth_chunk < wave_dsc->n_chunks, NULL);

  dhandle = bse_wave_handle_create (wave_dsc, nth_chunk, error_p);
  if (!dhandle)
    return NULL;

  BseWaveChunkDsc *chunk = wave_dsc->chunks + nth_chunk;

  if (error_p)
    *error_p = BSE_ERROR_IO;

  /* FIXME: we essentially create a dcache for each wchunk here ;( */

  dcache = gsl_data_cache_from_dhandle (dhandle, BSE_CONFIG (wave_chunk_padding) * wave_dsc->n_channels);
  gsl_data_handle_unref (dhandle);
  if (!dcache)
    return NULL;
  /* dcache keeps dhandle alive */

  const char *ltype = bse_xinfos_get_value (chunk->xinfos, "loop-type");
  GslWaveLoopType loop_type = ltype ? gsl_wave_loop_type_from_string (ltype) : GSL_WAVE_LOOP_NONE;
  SfiNum loop_start = bse_xinfos_get_num (chunk->xinfos, "loop-start");
  SfiNum loop_end = bse_xinfos_get_num (chunk->xinfos, "loop-end");
  SfiNum loop_count = bse_xinfos_get_num (chunk->xinfos, "loop-count");
  if (loop_type && loop_count == 0)     /* loop_count defaults to maximum */
    loop_count = 1000000;
  if (loop_end <= loop_start)
    {
      loop_start = loop_end = 0;
      loop_type = GSL_WAVE_LOOP_NONE;
      loop_count = 0;
    }

  wchunk = gsl_wave_chunk_new (dcache, chunk->mix_freq, chunk->osc_freq,
                               loop_type, loop_start, loop_end, loop_count);
  gsl_data_cache_unref (dcache);

  if (error_p && wchunk)
    *error_p = BSE_ERROR_NONE;

  return wchunk;
}
