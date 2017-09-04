// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "../config/config.h"
#include "bse/bseloader.hh"
#include "gsldatahandle-mad.hh"

#include <unistd.h>
#include <string.h>


/* --- structures --- */
typedef struct
{
  BseWaveFileInfo wfi;
  uint            n_channels;
  float	          mix_freq;
  float	          osc_freq;
} FileInfo;


/* --- functions --- */
static BseWaveFileInfo*
mad_load_file_info (void         *data,
		    const char   *file_name,
		    Bse::Error *error_p)
{
  FileInfo *fi;
  uint n_channels;
  float mix_freq;
  Bse::Error error;

  error = gsl_data_handle_mad_testopen (file_name, &n_channels, &mix_freq);
  if (error != 0)
    {
      *error_p = error;
      return NULL;
    }

  fi = sfi_new_struct0 (FileInfo, 1);
  fi->wfi.n_waves = 1;	/* we support only a single MPEG stream */
  fi->wfi.waves = (BseWaveFileInfo::Wave*) g_malloc0 (sizeof (fi->wfi.waves[0]) * fi->wfi.n_waves);
  const char *dsep = strrchr (file_name, G_DIR_SEPARATOR);
  fi->wfi.waves[0].name = g_strdup (dsep ? dsep + 1 : file_name);
  fi->n_channels = n_channels;
  fi->mix_freq = mix_freq;
  fi->osc_freq = 440.0;	/* FIXME */

  return &fi->wfi;
}

static void
mad_free_file_info (void            *data,
		    BseWaveFileInfo *file_info)
{
  FileInfo *fi = (FileInfo*) file_info;
  uint i;

  for (i = 0; i < fi->wfi.n_waves; i++)
    g_free (fi->wfi.waves[i].name);
  g_free (fi->wfi.waves);
  sfi_delete_struct (FileInfo, fi);
}

static BseWaveDsc*
mad_load_wave_dsc (void            *data,
		   BseWaveFileInfo *file_info,
		   uint             nth_wave,
		   Bse::Error    *error_p)
{
  FileInfo *fi = (FileInfo*) file_info;
  BseWaveDsc *wdsc = sfi_new_struct0 (BseWaveDsc, 1);

  wdsc->name = g_strdup (fi->wfi.waves[0].name);
  wdsc->n_channels = fi->n_channels;
  wdsc->n_chunks = 1;
  wdsc->chunks = g_new0 (BseWaveChunkDsc, 1);
  wdsc->chunks[0].osc_freq = fi->osc_freq;
  wdsc->chunks[0].mix_freq = fi->mix_freq;

  return wdsc;
}

static void
mad_free_wave_dsc (void       *data,
		   BseWaveDsc *wdsc)
{
  uint i;
  for (i = 0; i < wdsc->n_chunks; i++)
    g_strfreev (wdsc->chunks[i].xinfos);
  g_free (wdsc->chunks);
  g_free (wdsc->name);
  sfi_delete_struct (BseWaveDsc, wdsc);
}

static GslDataHandle*
mad_create_chunk_handle (void         *data,
			 BseWaveDsc   *wdsc,
			 uint          nth_chunk,
			 Bse::Error *error_p)
{
  FileInfo *fi = (FileInfo*) wdsc->file_info;
  GslDataHandle *dhandle;

  assert_return (nth_chunk == 0, NULL);

  dhandle = gsl_data_handle_new_mad_err (fi->wfi.file_name, wdsc->chunks[0].osc_freq, error_p);
  if (dhandle && wdsc->chunks[0].xinfos)
    {
      GslDataHandle *tmp_handle = dhandle;
      dhandle = gsl_data_handle_new_add_xinfos (dhandle, wdsc->chunks[0].xinfos);
      gsl_data_handle_unref (tmp_handle);
    }
  if (!dhandle && 0 == *error_p)
    *error_p = Bse::Error::FILE_OPEN_FAILED;
  return dhandle;
}


#define	MAGIC_MPEG_HEADER	 "0 beshort   &0xffe0\n" /* MPEG */		\
                                 "2 ubyte&0x0c <0x0c\n"	 /* valid samplefreq */	\
                                 "2 ubyte&0xf0 <0xf0\n"	 /* valid bitrate */
#define	MAGIC_MPEG10_I		(MAGIC_MPEG_HEADER				\
                                 "1 byte&0x18 =0x18\n"   /* 1.0 */		\
                                 "1 byte&0x06 =0x06\n"   /* I */)
#define	MAGIC_MPEG10_II		(MAGIC_MPEG_HEADER				\
                                 "1 byte&0x18 =0x18\n"   /* 1.0 */		\
                                 "1 byte&0x06 =0x04\n"   /* II */)
#define	MAGIC_MPEG10_III	(MAGIC_MPEG_HEADER				\
                                 "1 byte&0x18 =0x18\n"   /* 1.0 */		\
                                 "1 byte&0x06 =0x02\n"   /* III */)
#define	MAGIC_MPEG20_I		(MAGIC_MPEG_HEADER				\
                                 "1 byte&0x18 =0x10\n"   /* 2.0 */		\
                                 "1 byte&0x06 =0x06\n"   /* I */)
#define	MAGIC_MPEG20_II		(MAGIC_MPEG_HEADER				\
                                 "1 byte&0x18 =0x10\n"   /* 2.0 */		\
                                 "1 byte&0x06 =0x04\n"   /* II */)
#define	MAGIC_MPEG20_III	(MAGIC_MPEG_HEADER				\
                                 "1 byte&0x18 =0x10\n"   /* 2.0 */		\
                                 "1 byte&0x06 =0x02\n"   /* III */)
#define	MAGIC_MPEG25_I		(MAGIC_MPEG_HEADER				\
                                 "1 byte&0x18 =0x00\n"   /* 2.5 */		\
                                 "1 byte&0x06 =0x06\n"   /* I */)
#define	MAGIC_MPEG25_II		(MAGIC_MPEG_HEADER				\
                                 "1 byte&0x18 =0x00\n"   /* 2.5 */		\
                                 "1 byte&0x06 =0x04\n"   /* II */)
#define	MAGIC_MPEG25_III	(MAGIC_MPEG_HEADER				\
                                 "1 byte&0x18 =0x00\n"   /* 2.5 */		\
                                 "1 byte&0x06 =0x02\n"   /* III */)
#define	MAGIC_RIFF_MPEG		("0  string  RIFF\n"				\
                                 "8  string  WAVE\n"				\
                                 "12 string  fmt\\s\n"	/* "fmt " */		\
                                 "20 leshort 80\n"	/* format: MPEG */)
#define	MAGIC_RIFF_MPEG_III	("0  string  RIFF\n"				\
                                 "8  string  WAVE\n"				\
                                 "12 string  fmt\\s\n"	/* "fmt " */		\
                                 "20 leshort 85\n"	/* format: MPEG III */)
#define	MAGIC_MPEG_ID3		("0  string  ID3\n"	/* ID3v2 tag for mp3 */	\
                                 "3  ubyte   <0xff\n"	/* major version */	\
                                 "4  ubyte   <0xff\n"	/* revision */)

void
_gsl_init_loader_mad (void)
{
  static const char *file_exts[] = {
    "mp1", "mp2", "mp3",
    NULL,
  };
  static const char *mime_types[] = {
    "audio/mp3", "audio/x-mp3", "audio/mpg3", "audio/x-mpg3", "audio/mpeg3", "audio/x-mpeg3",
    "audio/mp2", "audio/x-mp2", "audio/mpg2", "audio/x-mpg2", "audio/mpeg2", "audio/x-mpeg2",
    "audio/mp1", "audio/x-mp1", "audio/mpg1", "audio/x-mpg1", "audio/mpeg1", "audio/x-mpeg1",
    "audio/mpeg", "audio/x-mpeg",
    NULL,
  };
  static const char *magics[] = {
    MAGIC_MPEG10_I, MAGIC_MPEG10_II, MAGIC_MPEG10_III,
    MAGIC_MPEG20_I, MAGIC_MPEG20_II, MAGIC_MPEG20_III,
    MAGIC_MPEG25_I, MAGIC_MPEG25_II, MAGIC_MPEG25_III,
    MAGIC_RIFF_MPEG, MAGIC_RIFF_MPEG_III,
    MAGIC_MPEG_ID3,
    NULL,
  };
  static BseLoader loader = {
    "MPEG Audio (MAD: MPEG 1.0/2.0/2.5 Layer III/II/I Decoder)",
    file_exts,
    mime_types,
    BSE_LOADER_SKIP_PRECEEDING_NULLS,	/* some mp3's have preceeding 0s (partial silence frames?) */
    magics,
    0,	/* priority */
    NULL,
    mad_load_file_info,
    mad_free_file_info,
    mad_load_wave_dsc,
    mad_free_wave_dsc,
    mad_create_chunk_handle,
  };
  static gboolean initialized = FALSE;

  assert_return (initialized == FALSE);
  initialized = TRUE;

  if (BSE_HAVE_LIBMAD)
    bse_loader_register (&loader);
}
