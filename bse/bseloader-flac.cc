// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseloader.hh"
#include "bsedatahandle-flac.hh"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <vector>
#include <string>
#include <FLAC/stream_decoder.h>

namespace {

struct FileInfo
{
  BseWaveFileInfo wfi;
  BseWaveDsc      wdsc;
  // pass error status from flac callback to caller
  bool                              error_occurred;
  FLAC__StreamDecoderErrorStatus    error_status;

  static void
  flac_error_callback (const FLAC__StreamDecoder     *decoder,
                       FLAC__StreamDecoderErrorStatus status,
                       void                          *client_data)
  {
    FileInfo *fi = static_cast<FileInfo *> (client_data);
    fi->error_occurred = true;
    fi->error_status = status;
  }
  static FLAC__StreamDecoderWriteStatus
  flac_write_callback (const FLAC__StreamDecoder  *decoder,
                       const FLAC__Frame          *frame,
                       const FLAC__int32          *const buffer[],
                       void                       *client_data)
  {
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
  }
  FileInfo (const gchar  *file_name,
            Bse::Error *error_p)
  {
    /* initialize C structures with zeros */
    memset (&wfi, 0, sizeof (wfi));
    memset (&wdsc, 0, sizeof (wdsc));

    /* setup decoder, decoding from file */
    FLAC__StreamDecoder* decoder = FLAC__stream_decoder_new();
    if (!decoder)
      {
        *error_p = Bse::Error::INTERNAL;  // should not happen
        return;
      }
    error_occurred = false;
    int r = FLAC__stream_decoder_init_file (decoder, file_name, flac_write_callback, NULL, flac_error_callback, this);
    if (r != 0)
      {
        *error_p = gsl_error_from_errno (errno, Bse::Error::FILE_OPEN_FAILED);
        return;
      }

    /* decode enough to figure out channel count */
    FLAC__bool mdok;
    do
      mdok = FLAC__stream_decoder_process_single (decoder);
    while (FLAC__stream_decoder_get_channels (decoder) == 0 && mdok);

    if (error_occurred || FLAC__stream_decoder_get_channels (decoder) == 0)
      {
        *error_p = Bse::Error::IO;
        return;
      }

    /* allocate and fill BseWaveFileInfo */
    wfi.n_waves = 1;
    wfi.waves = (typeof (wfi.waves)) g_malloc0 (sizeof (wfi.waves[0]) * wfi.n_waves);
    wfi.waves[0].name = g_strdup (file_name);

    /* allocate and fill BseWaveDsc */
    wdsc.n_chunks = 1;
    wdsc.chunks = (typeof (wdsc.chunks)) g_malloc0 (sizeof (wdsc.chunks[0]) * wdsc.n_chunks);
    wdsc.name = g_strdup (file_name);
    wdsc.n_channels = FLAC__stream_decoder_get_channels (decoder);

    /* fill GslWaveChunk */
    wdsc.chunks[0].mix_freq = FLAC__stream_decoder_get_sample_rate (decoder);
    wdsc.chunks[0].osc_freq = 440.0;
  }
  ~FileInfo()
  {
    /* free BseWaveDsc */
    for (guint i = 0; i < wdsc.n_chunks; i++)
      g_strfreev (wdsc.chunks[i].xinfos);

    g_strfreev (wdsc.xinfos);
    g_free (wdsc.name);
    g_free (wdsc.chunks);

    /* free BseWaveFileInfo */
    if (wfi.waves)
      {
	g_free (wfi.waves[0].name);
	g_free (wfi.waves);
      }
  }
};

} // Anon

static BseWaveFileInfo*
flac_load_file_info (gpointer      data,
		     const gchar  *file_name,
		     Bse::Error *error_p)
{
  FileInfo *file_info = new FileInfo (file_name, error_p);
  if (*error_p != 0)
    {
      delete file_info;
      return NULL;
    }

  return &file_info->wfi;
}

static void
flac_free_file_info (gpointer data, BseWaveFileInfo *wave_file_info)
{
  FileInfo *file_info = reinterpret_cast<FileInfo*> (wave_file_info);
  delete file_info;
}

static BseWaveDsc*
flac_load_wave_dsc (gpointer         data,
		    BseWaveFileInfo *wave_file_info,
		    guint            nth_wave,
		    Bse::Error    *error_p)
{
  FileInfo *file_info = reinterpret_cast<FileInfo*> (wave_file_info);
  return &file_info->wdsc;
}

static void
flac_free_wave_dsc (gpointer    data,
		    BseWaveDsc *wave_dsc)
{
}

static GslDataHandle*
flac_create_chunk_handle (gpointer      data,
			  BseWaveDsc   *wave_dsc,
			  guint         nth_chunk,
			  Bse::Error *error_p)
{
  assert_return (nth_chunk == 0, NULL);

  FileInfo *file_info = reinterpret_cast<FileInfo*> (wave_dsc->file_info);
  const BseWaveChunkDsc *chunk = &wave_dsc->chunks[nth_chunk];
  GslDataHandle *dhandle = bse_data_handle_new_flac (file_info->wfi.file_name, chunk->osc_freq);
  return dhandle;
}

void
bse_init_loader_flac (void)
{
  static const gchar *file_exts[] = { "flac", NULL, };
  static const gchar *mime_types[] = { "audio/x-flac", NULL, };
  static const gchar *magics[] = {
    "0  string  fLaC",              // free lossless audio codec magic
    NULL,
  };
  static BseLoader loader = {
    "FLAC",
    file_exts,
    mime_types,
    BSE_LOADER_NO_FLAGS,
    magics,
    0,  /* priority */
    NULL,
    flac_load_file_info,
    flac_free_file_info,
    flac_load_wave_dsc,
    flac_free_wave_dsc,
    flac_create_chunk_handle,
  };
  static gboolean initialized = FALSE;

  assert_return (initialized == FALSE);
  initialized = TRUE;

  bse_loader_register (&loader);
}
