// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseloader.hh"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <vector>
#include <string>

#undef  WITH_GUSPATCH_XINFOS

#define LDEBUG(...)     BSE_KEY_DEBUG ("gus-loader", __VA_ARGS__)

using std::vector;
using std::string;
/*
 * generic patch loading code from aRts
 */
namespace
{
typedef unsigned char byte;
typedef unsigned short int word;
typedef uint dword;
typedef char sbyte;
typedef short int sword;
typedef int sdword;
/*
 * executes read_me (which should be a function call to read something from the file),
 * and returns from the calling function if that fails
 */
#define read_or_return_error(read_me) G_STMT_START{ BseErrorType _error = read_me; if (_error) return _error; }G_STMT_END
static inline BseErrorType
fread_block (FILE *file,
             int   len,
             void *data)
{
  if (fread (data, len, 1, file) != 1)
    {
      if (feof (file))
        return BSE_ERROR_FILE_EOF;
      else
        return gsl_error_from_errno (errno, BSE_ERROR_FILE_READ_FAILED);
    }
  return BSE_ERROR_NONE;
}
static inline BseErrorType
skip (FILE *file,
      int   len)
{
  while (len > 0)
    {
      char junk;
      read_or_return_error (fread_block (file, 1, &junk));
      len--;
    }
  return BSE_ERROR_NONE;
}
static inline BseErrorType
fread_bytes (FILE          *file,
             unsigned char *bytes,
             int            len)
{
  return fread_block (file, len, bytes);
}
static inline BseErrorType
fread_string (FILE *file,
              char *str,
              int   len)
{
  return fread_block (file, len, str);
}
/* readXXX with sizeof(xxx) == 1 */
static inline BseErrorType
fread_byte (FILE *file,
          byte &b)
{
  return fread_block (file, 1, &b);
}
/* readXXX with sizeof(xxx) == 2 */
static inline BseErrorType
fread_word (FILE *file,
          word &w)
{
  byte h, l;
  read_or_return_error (fread_block (file, 1, &l));
  read_or_return_error (fread_block (file, 1, &h));
  w = (h << 8) + l;
  return BSE_ERROR_NONE;
}
static inline BseErrorType
fread_short_word (FILE  *file,
                  sword &sw)
{
  word w;
  read_or_return_error (fread_word (file, w));
  sw = (sword) w;
  return BSE_ERROR_NONE;
}
/* readXXX with sizeof(xxx) == 4 */
static inline BseErrorType
fread_dword (FILE *file, dword& dw)
{
  byte h, l, hh, hl;
  read_or_return_error (fread_block (file, 1, &l));
  read_or_return_error (fread_block (file, 1, &h));
  read_or_return_error (fread_block (file, 1, &hl));
  read_or_return_error (fread_block (file, 1, &hh));
  dw = (hh << 24) + (hl << 16) + (h << 8) + l;
  return BSE_ERROR_NONE;
}
struct PatHeader
{
  char id[12];		        /* ID='GF1PATCH110' */
  char manufacturer_id[10];	/* Manufacturer ID */
  char description[60];	        /* Description of the contained Instruments
                                 * or copyright of manufacturer. */
  byte instruments;		/* Number of instruments in this patch */
  byte voices;		        /* Number of voices for sample */
  byte channels;		/* Number of output channels
                                 * (1=mono,2=stereo) */
  word waveforms;		/* Number of waveforms */
  word mastervolume;		/* Master volume for all samples */
  dword size;			/* Size of the following data */
  char reserved[36];		/* reserved */
  PatHeader()
  {
  }
  BseErrorType
  load (FILE *file)
  {
    read_or_return_error (fread_string (file, id, 12));
    read_or_return_error (fread_string (file, manufacturer_id, 10));
    read_or_return_error (fread_string (file, description, 60));
    /*		skip(file, 2);*/
    read_or_return_error (fread_byte (file, instruments));
    read_or_return_error (fread_byte (file, voices));
    read_or_return_error (fread_byte (file, channels));
    read_or_return_error (fread_word (file, waveforms));
    read_or_return_error (fread_word (file, mastervolume));
    read_or_return_error (fread_dword (file, size));
    read_or_return_error (fread_string (file, reserved, 36));
    return BSE_ERROR_NONE;
  }
};
struct PatInstrument
{
  word	number;
  char	name[16];
  dword 	size;		  /* Size of the whole instrument in bytes. */
  byte	layers;
  char	reserved[40];
  /* layer? */
  word	layerUnknown;
  dword	layerSize;
  byte	sampleCount;	  /* number of samples in this layer (?) */
  char	layerReserved[40];
  PatInstrument()
  {
  }
  BseErrorType
  load (FILE *file)
  {
    read_or_return_error (fread_word (file, number));
    read_or_return_error (fread_string (file, name, 16));
    read_or_return_error (fread_dword (file, size));
    read_or_return_error (fread_byte (file, layers));
    read_or_return_error (fread_string (file, reserved, 40));
    /* layer: (?) */
    read_or_return_error (fread_word (file, layerUnknown));
    read_or_return_error (fread_dword (file, layerSize));
    read_or_return_error (fread_byte (file, sampleCount));
    read_or_return_error (fread_string (file, reserved, 40));
    return BSE_ERROR_NONE;
  }
};
enum
{
  PAT_FORMAT_16BIT = (1 << 0),
  PAT_FORMAT_UNSIGNED = (1 << 1),
  PAT_FORMAT_LOOPED = (1 << 2),
  PAT_FORMAT_LOOP_BIDI = (1 << 3),
  PAT_FORMAT_LOOP_BACKWARDS = (1 << 4),
  PAT_FORMAT_SUSTAIN = (1 << 5),
  PAT_FORMAT_ENVELOPE = (1 << 6),
  PAT_FORMAT_CLAMPED = (1 << 7) // timidity source says: ? (for last envelope??)
};
struct PatPatch
{
  char	filename[7];	  /* Wave file name */
  byte	fractions;	  /* Fractions */
  dword	wavesize;	  /* Wave size: Size of the wave digital data */
  dword	loopStart;
  dword	loopEnd;
  word	sampleRate;
  dword	minFreq;
  dword	maxFreq;
  dword	origFreq;
  sword	fineTune;
  byte	balance;
  byte	filterRate[6];
  byte	filterOffset[6];
  byte	tremoloSweep;
  byte	tremoloRate;
  byte	tremoloDepth;
  byte	vibratoSweep;
  byte	vibratoRate;
  byte	vibratoDepth;
  byte	waveFormat;
  sword	freqScale;
  word	freqScaleFactor;
  char	reserved[36];
  PatPatch()
  {
  }
  BseErrorType
  load (FILE *file)
  {
    read_or_return_error (fread_string (file, filename, 7));
    read_or_return_error (fread_byte (file, fractions));
    read_or_return_error (fread_dword (file, wavesize));
    read_or_return_error (fread_dword (file, loopStart));
    read_or_return_error (fread_dword (file, loopEnd));
    read_or_return_error (fread_word (file, sampleRate));
    read_or_return_error (fread_dword (file, minFreq));
    read_or_return_error (fread_dword (file, maxFreq));
    read_or_return_error (fread_dword (file, origFreq));
    read_or_return_error (fread_short_word (file, fineTune));
    read_or_return_error (fread_byte (file, balance));
    read_or_return_error (fread_bytes (file, filterRate, 6));
    read_or_return_error (fread_bytes (file, filterOffset, 6));
    read_or_return_error (fread_byte (file, tremoloSweep));
    read_or_return_error (fread_byte (file, tremoloRate));
    read_or_return_error (fread_byte (file, tremoloDepth));
    read_or_return_error (fread_byte (file, vibratoSweep));
    read_or_return_error (fread_byte (file, vibratoRate));
    read_or_return_error (fread_byte (file, vibratoDepth));
    read_or_return_error (fread_byte (file, waveFormat));
    read_or_return_error (fread_short_word (file, freqScale));
    read_or_return_error (fread_word (file, freqScaleFactor));
    read_or_return_error (fread_string (file, reserved, 36));
    return BSE_ERROR_NONE;
  }
};
#undef read_or_return_error
};
namespace {
/*
 * adaptation to GSL API
 */
struct FileInfo
{
  BseWaveFileInfo wfi;
  BseWaveDsc      wdsc;
  PatHeader          *header;
  PatInstrument      *instrument;
  vector<PatPatch *>  patches;
  GslWaveLoopType
  loop_type (int wave_format)
  {
    /* FIXME: is backwards for the loop or for the wave? */
    if (wave_format & PAT_FORMAT_LOOPED)
      {
	if (wave_format & PAT_FORMAT_LOOP_BIDI)
	  {
	    if (wave_format & PAT_FORMAT_LOOP_BACKWARDS)
	      {
		LDEBUG ("unsupported loop type (backwards-pingpong)");
		return GSL_WAVE_LOOP_PINGPONG;
	      }
	    else
	      {
		return GSL_WAVE_LOOP_PINGPONG;
	      }
	  }
	else
	  {
	    if (wave_format & PAT_FORMAT_LOOP_BACKWARDS)
	      {
		LDEBUG ("unsupported loop type (backwards-jump)");
		return GSL_WAVE_LOOP_JUMP;
	      }
	    else
	      {
		return GSL_WAVE_LOOP_JUMP;
	      }
	  }
      }
    else
      {
	return GSL_WAVE_LOOP_NONE;
      }
  }
  guint&
  data_offset (int chunk_number)
  {
    return wdsc.chunks[chunk_number].loader_data[0].uint;
  }
  GslWaveFormatType
  wave_format (int wave_format)
  {
    switch (wave_format & (PAT_FORMAT_UNSIGNED | PAT_FORMAT_16BIT))
      {
      case 0:					    return GSL_WAVE_FORMAT_SIGNED_8;
      case PAT_FORMAT_UNSIGNED:			    return GSL_WAVE_FORMAT_UNSIGNED_8;
      case PAT_FORMAT_16BIT:			    return GSL_WAVE_FORMAT_SIGNED_16;
      case PAT_FORMAT_UNSIGNED | PAT_FORMAT_16BIT:  return GSL_WAVE_FORMAT_UNSIGNED_16;
      }
    BIRNET_ASSERT_NOT_REACHED();
  }
  int
  bytes_per_frame (int wave_format)
  {
    return ((wave_format & PAT_FORMAT_16BIT) ? 2 : 1);
  }
  string
  envelope_point_to_string (guint value)
  {
    gchar *tmp_str = g_strdup_printf ("%u", value);
    string str = tmp_str;
    g_free (tmp_str);
    return str;
  }
  string
  envelope_array_to_string (byte *envelope_array)
  {
    string envelope_str;
    for (int i = 0; i < 6; i++)
      {
	if (i)
	  envelope_str += ",";
	envelope_str += envelope_point_to_string (envelope_array[i]);
      }
    return envelope_str;
  }
  FileInfo (const gchar  *file_name,
            BseErrorType *error_p)
  {
    /* initialize C structures with zeros */
    memset (&wfi, 0, sizeof (wfi));
    memset (&wdsc, 0, sizeof (wdsc));
    /* open patch file */
    FILE *patfile = fopen (file_name, "r");
    if (!patfile)
      {
	*error_p = gsl_error_from_errno (errno, BSE_ERROR_FILE_OPEN_FAILED);
	return;
      }
    /* parse contents of patfile into Pat* data structurs */
    header = new PatHeader();
    *error_p = header->load (patfile);
    if (*error_p)
      {
	fclose (patfile);
	return;
      }
    if (header->channels == 0) /* fixup channels setting */
      header->channels = 1;
    instrument = new PatInstrument();
    *error_p = instrument->load (patfile);
    if (*error_p)
      {
	fclose (patfile);
        return;
      }
    /* allocate BseWaveDsc */
    wdsc.n_chunks = instrument->sampleCount;
    wdsc.chunks = (typeof (wdsc.chunks)) g_malloc0 (sizeof (wdsc.chunks[0]) * wdsc.n_chunks);
    for (int i = 0; i<instrument->sampleCount; i++)
      {
	PatPatch *patch = new PatPatch();
	patches.push_back (patch);
        *error_p = patch->load (patfile);
        if (*error_p)
          return;
	data_offset (i) = (guint) ftell (patfile);
	*error_p = skip (patfile, patch->wavesize);
        if (*error_p)
	  {
	    fclose (patfile);
	    return;
	  }
	LDEBUG (" - read patch, srate = %d (%d bytes)", patch->sampleRate, patch->wavesize);
      }
    fclose (patfile);
    /* allocate and fill BseWaveFileInfo */
    wfi.n_waves = 1;
    wfi.waves = (typeof (wfi.waves)) g_malloc0 (sizeof (wfi.waves[0]) * wfi.n_waves);
    wfi.waves[0].name = g_strdup (file_name);
    /* fill BseWaveDsc */
    wdsc.name = g_strdup (file_name);
    /* header->channels means output channels, GUS Patches are mono only */
    wdsc.n_channels = 1;
#ifdef WITH_GUSPATCH_XINFOS
    wdsc.xinfos = bse_xinfos_add_value (wdsc.xinfos, "play-type", "gus-patch");
#endif
    for (guint i = 0; i < wdsc.n_chunks; i++)
      {
	/* fill GslWaveChunk */
	wdsc.chunks[i].mix_freq = patches[i]->sampleRate;
	wdsc.chunks[i].osc_freq = patches[i]->origFreq / 1000.0;
	LDEBUG ("orig_freq = %f (%d)", patches[i]->origFreq / 1000.0, patches[i]->origFreq);
	LDEBUG ("min_freq = %f", patches[i]->minFreq / 1000.0);
	LDEBUG ("max_freq = %f", patches[i]->maxFreq / 1000.0);
	LDEBUG ("fine_tune = %d", patches[i]->fineTune);
	LDEBUG ("scale_freq = %d", patches[i]->freqScale);
	LDEBUG ("scale_factor = %d", patches[i]->freqScaleFactor);
	/* fill xinfos */
	char**& xinfos = wdsc.chunks[i].xinfos;
	int frame_size = bytes_per_frame (patches[i]->waveFormat);
        if (loop_type (patches[i]->waveFormat))
          {
            xinfos = bse_xinfos_add_value (xinfos, "loop-type", gsl_wave_loop_type_to_string (loop_type (patches[i]->waveFormat)));
            xinfos = bse_xinfos_add_num (xinfos, "loop-count", 1000000);
            xinfos = bse_xinfos_add_num (xinfos, "loop-start", patches[i]->loopStart / frame_size);
            xinfos = bse_xinfos_add_num (xinfos, "loop-end", patches[i]->loopEnd / frame_size);
#ifdef WITH_GUSPATCH_XINFOS
            xinfos = bse_xinfos_add_value (xinfos, "gus-patch-envelope-rates",
                                           envelope_array_to_string (patches[i]->filterRate).c_str());
            xinfos = bse_xinfos_add_value (xinfos, "gus-patch-envelope-offsets",
                                           envelope_array_to_string (patches[i]->filterOffset).c_str());
#endif
          }
#ifdef WITH_GUSPATCH_XINFOS
	xinfos = bse_xinfos_add_num (xinfos, "gus-patch-loop-fractions", patches[i]->fractions);
	xinfos = bse_xinfos_add_float (xinfos, "gus-patch-min-freq", patches[i]->minFreq / 1000.0);
	xinfos = bse_xinfos_add_float (xinfos, "gus-patch-max-freq", patches[i]->maxFreq / 1000.0);
	xinfos = bse_xinfos_add_float (xinfos, "gus-patch-orig-freq", patches[i]->origFreq / 1000.0);
	xinfos = bse_xinfos_add_num (xinfos, "gus-patch-fine-tune", patches[i]->fineTune);
	xinfos = bse_xinfos_add_num (xinfos, "gus-patch-balance", patches[i]->balance);
	xinfos = bse_xinfos_add_num (xinfos, "gus-patch-tremolo-sweep", patches[i]->tremoloSweep);
	xinfos = bse_xinfos_add_num (xinfos, "gus-patch-tremolo-rate", patches[i]->tremoloRate);
	xinfos = bse_xinfos_add_num (xinfos, "gus-patch-tremolo-depth", patches[i]->tremoloDepth);
	xinfos = bse_xinfos_add_num (xinfos, "gus-patch-vibrato-sweep", patches[i]->vibratoSweep);
	xinfos = bse_xinfos_add_num (xinfos, "gus-patch-vibrato-rate", patches[i]->vibratoRate);
	xinfos = bse_xinfos_add_num (xinfos, "gus-patch-vibrato-depth", patches[i]->vibratoDepth);
	xinfos = bse_xinfos_add_num (xinfos, "gus-patch-wave-format", patches[i]->waveFormat);
	xinfos = bse_xinfos_add_num (xinfos, "gus-patch-freq-scale", patches[i]->freqScale);
	xinfos = bse_xinfos_add_num (xinfos, "gus-patch-freq-scale-factor", patches[i]->freqScaleFactor);
#endif
      }
  }
  ~FileInfo()
  {
    /* free patch data loaded from file */
    vector<PatPatch *>::iterator pi;
    for (pi = patches.begin(); pi != patches.end(); pi++)
      delete *pi;
    delete instrument;
    delete header;
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
static BseWaveFileInfo*
pat_load_file_info (gpointer      data,
		    const gchar  *file_name,
		    BseErrorType *error_p)
{
  FileInfo *file_info = new FileInfo (file_name, error_p);
  if (*error_p)
    {
      delete file_info;
      return NULL;
    }
  return &file_info->wfi;
}
static void
pat_free_file_info (gpointer         data,
		    BseWaveFileInfo *wave_file_info)
{
  FileInfo *file_info = reinterpret_cast<FileInfo*> (wave_file_info);
  delete file_info;
}
static BseWaveDsc*
pat_load_wave_dsc (gpointer         data,
		   BseWaveFileInfo *wave_file_info,
		   guint            nth_wave,
		   BseErrorType    *error_p)
{
  FileInfo *file_info = reinterpret_cast<FileInfo*> (wave_file_info);
  return &file_info->wdsc;
}
static void
pat_free_wave_dsc (gpointer    data,
		   BseWaveDsc *wave_dsc)
{
}
static GslDataHandle*
pat_create_chunk_handle (gpointer      data,
			 BseWaveDsc   *wave_dsc,
			 guint         nth_chunk,
			 BseErrorType *error_p)
{
  g_return_val_if_fail (nth_chunk < wave_dsc->n_chunks, NULL);
  FileInfo *file_info = reinterpret_cast<FileInfo*> (wave_dsc->file_info);
  const PatPatch *patch = file_info->patches[nth_chunk];
  const BseWaveChunkDsc *chunk = &wave_dsc->chunks[nth_chunk];
  LDEBUG ("pat loader chunk %d: gsl_wave_handle_new %s %d %d %d %f %f %u %d",
          nth_chunk,
          file_info->wfi.file_name,
          wave_dsc->n_channels,
          file_info->wave_format (patch->waveFormat),
          G_LITTLE_ENDIAN,
          chunk->mix_freq,
          chunk->osc_freq,
          file_info->data_offset (nth_chunk),
          patch->wavesize / file_info->bytes_per_frame (patch->waveFormat));
  GslDataHandle *dhandle;
  dhandle = gsl_wave_handle_new (file_info->wfi.file_name,
	                         wave_dsc->n_channels,
				 file_info->wave_format (patch->waveFormat),
				 G_LITTLE_ENDIAN,
				 chunk->mix_freq,
				 chunk->osc_freq,
				 file_info->data_offset (nth_chunk),
				 patch->wavesize / file_info->bytes_per_frame (patch->waveFormat),
                                 chunk->xinfos);
  return dhandle;
}
} // namespace
extern "C" void
bse_init_loader_gus_patch (void)
{
  static const gchar *file_exts[] = { "pat", NULL, };
  static const gchar *mime_types[] = { "audio/x-pat", NULL, }; // FIXME: correct?
  static const gchar *magics[] = {
    "0  string  GF1PATCH110\0ID#000002\0",      // GUS patch V1.1
    "0  string  GF1PATCH100\0ID#000002\0",      // GUS patch V1.0
    NULL,
  };
  static BseLoader loader = {
    "GUS Patch",
    file_exts,
    mime_types,
    BSE_LOADER_NO_FLAGS,
    magics,
    0,  /* priority */
    NULL,
    pat_load_file_info,
    pat_free_file_info,
    pat_load_wave_dsc,
    pat_free_wave_dsc,
    pat_create_chunk_handle,
  };
  static gboolean initialized = FALSE;
  g_assert (initialized == FALSE);
  initialized = TRUE;
  bse_loader_register (&loader);
}
