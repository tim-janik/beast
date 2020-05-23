// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseloader.hh"
#include "bsemain.hh"
#include "gslcommon.hh"
#include "gsldatahandle.hh"
#include "magic.hh"
#include "gslfilehash.hh"
#include "bse/internal.hh"
#include <string.h>


/* --- variables --- */
static BseLoader                   *bse_loader_list = NULL;
static std::vector<Bse::FileMagic*> magic_list1;
static std::vector<Bse::FileMagic*> magic_list2;


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
  assert_return (loader != NULL);
  assert_return (loader->name != NULL);
  assert_return (loader->extensions || loader->mime_types || loader->magic_specs);
  assert_return (loader_find_by_name (loader->name) == NULL);
  assert_return (loader->next == NULL);
  assert_return (loader->load_file_info != NULL);
  assert_return (loader->free_file_info != NULL);
  assert_return (loader->load_wave_dsc != NULL);
  assert_return (loader->free_wave_dsc != NULL);
  assert_return (loader->create_chunk_handle != NULL);

  loader->next = bse_loader_list;
  bse_loader_list = loader;

  if (loader->magic_specs)
    {
      for (uint i = 0; loader->magic_specs[i]; i++)
	{
	  if (loader->extensions)
	    for (uint j = 0; loader->extensions[j]; j++)
	      {
                Bse::FileMagic *magic = Bse::FileMagic::register_magic (loader->extensions[j], loader->magic_specs[i],
                                                                        loader->name + String (" (") + loader->extensions[j] + ")", loader->priority);
                magic->sdata (std::shared_ptr<void> (loader, [] (BseLoader*) {}));
                magic_list1.push_back (magic);
		if (loader->flags & BSE_LOADER_SKIP_PRECEEDING_NULLS)
                  magic_list2.push_back (magic);
	      }
	  else
	    {
              Bse::FileMagic *magic = Bse::FileMagic::register_magic ("", loader->magic_specs[i], loader->name, loader->priority); // loader
              magic->sdata (std::shared_ptr<void> (loader, [] (BseLoader*) {}));
              magic_list1.push_back (magic);
              if (loader->flags & BSE_LOADER_SKIP_PRECEEDING_NULLS)
                magic_list2.push_back (magic);
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
  assert_return (file_name != NULL, NULL);

  /* normal magic check */
  Bse::FileMagic *magic = Bse::FileMagic::match_list (magic_list1, file_name);

  /* in a sort-of fallback attempt,
   * work around files that have preceeding nulls
   */
  if (!magic && magic_list2.size())
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
        magic = Bse::FileMagic::match_list (magic_list2, file_name, pos);
    }

  return magic ? (BseLoader*) magic->sdata().get() : nullptr;
}

BseWaveFileInfo*
bse_wave_file_info_load (const char   *file_name,
			 Bse::Error *error_p)
{
  BseWaveFileInfo *finfo = NULL;
  Bse::Error error = Bse::Error::NONE;
  BseLoader *loader;

  if (error_p)
    *error_p = Bse::Error::INTERNAL;
  assert_return (file_name != NULL, NULL);

  loader = bse_loader_match (file_name);
  if (loader)
    {
      finfo = loader->load_file_info (loader->data, file_name, &error);
      if (error != 0 && finfo)
	{
	  /* loaders shouldn't do this */
	  loader->free_file_info (loader->data, finfo);
	  finfo = NULL;
	}
      if (!finfo && error == 0)
	error = Bse::Error::FILE_EMPTY;	/* FIXME: try next loader */
      if (finfo)
	{
	  if (finfo->n_waves > 0)
	    {
	      uint i;

	      assert_return (finfo->loader == NULL, NULL);
	      assert_return (finfo->file_name == NULL, NULL);

	      for (i = 0; i < finfo->n_waves; i++)
		assert_return (finfo->waves[i].name != NULL, NULL);

	      finfo->file_name = g_strdup (file_name);
	      finfo->loader = loader;
	      finfo->ref_count = 1;
	    }
	  else
	    {
	      loader->free_file_info (loader->data, finfo);
	      finfo = NULL;
	      error = Bse::Error::FILE_EMPTY;   /* FIXME: try next loader */
	    }
	}
    }
  else /* no loader match */
    {
      /* try to provide apropriate error code */
      error = gsl_file_check (file_name, "rf");
      if (error == 0)
	error = Bse::Error::FORMAT_UNKNOWN;
    }

  if (error_p)
    *error_p = error;

  return finfo;
}

void
bse_wave_file_info_unref (BseWaveFileInfo *wave_file_info)
{
  assert_return (wave_file_info != NULL);
  assert_return (wave_file_info->ref_count > 0);

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
  assert_return (wave_file_info != NULL, NULL);
  assert_return (wave_file_info->ref_count > 0, NULL);

  wave_file_info->ref_count++;

  return wave_file_info;
}

const char*
bse_wave_file_info_loader (BseWaveFileInfo *fi)
{
  assert_return (fi != NULL, NULL);

  return fi->loader->name;
}

BseWaveDsc*
bse_wave_dsc_load (BseWaveFileInfo *wave_file_info,
		   uint             nth_wave,
                   gboolean         accept_empty,
                   Bse::Error    *error_p)
{
  Bse::Error error = Bse::Error::NONE;
  BseWaveDsc *wdsc;
  BseLoader *loader;

  if (error_p)
    *error_p = Bse::Error::INTERNAL;
  assert_return (wave_file_info != NULL, NULL);
  assert_return (wave_file_info->loader != NULL, NULL);
  assert_return (nth_wave < wave_file_info->n_waves, NULL);

  loader = wave_file_info->loader;
  wdsc = loader->load_wave_dsc (loader->data, wave_file_info, nth_wave,&error);

  if (error != 0 && wdsc)
    {
      /* loaders shouldn't do this */
      loader->free_wave_dsc (loader->data, wdsc);
      wdsc = NULL;
    }
  if (!wdsc && error == 0)
    error = Bse::Error::FILE_EMPTY;
  if (wdsc)
    {
      if (accept_empty || wdsc->n_chunks > 0)
	{
	  assert_return (wdsc->file_info == NULL, NULL);
	  assert_return (wdsc->name && strcmp (wdsc->name, wave_file_info->waves[nth_wave].name) == 0, NULL);

	  wdsc->file_info = wave_file_info;
	  bse_wave_file_info_ref (wave_file_info);
	}
      else
	{
	  loader->free_wave_dsc (loader->data, wdsc);
	  wdsc = NULL;
	  error = Bse::Error::FILE_EMPTY;
	}
    }

  if (error_p)
    *error_p = error;

  return wdsc;
}

void
bse_wave_dsc_free (BseWaveDsc *wave_dsc)
{
  assert_return (wave_dsc != NULL);
  assert_return (wave_dsc->file_info != NULL);

  BseWaveFileInfo *file_info = wave_dsc->file_info;

  file_info->loader->free_wave_dsc (file_info->loader->data, wave_dsc);

  bse_wave_file_info_unref (file_info);
}

GslDataHandle*
bse_wave_handle_create (BseWaveDsc   *wave_dsc,
			uint	      nth_chunk,
			Bse::Error *error_p)
{
  Bse::Error error = Bse::Error::NONE;
  GslDataHandle *dhandle;
  BseLoader *loader;

  if (error_p)
    *error_p = Bse::Error::INTERNAL;
  assert_return (wave_dsc != NULL, NULL);
  assert_return (wave_dsc->file_info != NULL, NULL);
  assert_return (nth_chunk < wave_dsc->n_chunks, NULL);

  loader = wave_dsc->file_info->loader;

  dhandle = loader->create_chunk_handle (loader->data,
					 wave_dsc,
					 nth_chunk,
					 &error);
  if (error != 0 && dhandle)
    {
      /* loaders shouldn't do this */
      gsl_data_handle_unref (dhandle);
      dhandle = NULL;
    }
  if (!dhandle && error == 0)
    error = Bse::Error::FORMAT_INVALID;

  if (error_p)
    *error_p = error;

  return dhandle;
}

GslWaveChunk*
bse_wave_chunk_create (BseWaveDsc   *wave_dsc,
		       uint          nth_chunk,
		       Bse::Error *error_p)
{
  GslDataHandle *dhandle;
  GslDataCache *dcache;
  GslWaveChunk *wchunk;

  if (error_p)
    *error_p = Bse::Error::INTERNAL;
  assert_return (wave_dsc != NULL, NULL);
  assert_return (nth_chunk < wave_dsc->n_chunks, NULL);

  dhandle = bse_wave_handle_create (wave_dsc, nth_chunk, error_p);
  if (!dhandle)
    return NULL;

  BseWaveChunkDsc *chunk = wave_dsc->chunks + nth_chunk;

  if (error_p)
    *error_p = Bse::Error::IO;

  /* FIXME: we essentially create a dcache for each wchunk here ;( */

  dcache = gsl_data_cache_from_dhandle (dhandle, BSE_WAVE_CHUNK_PADDING * wave_dsc->n_channels);
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
    *error_p = Bse::Error::NONE;

  return wchunk;
}
