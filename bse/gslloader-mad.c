/* GSL - Generic Sound Layer
 * Copyright (C) 2002 Tim Janik
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
#include "bse/gslloader.h"

#include <bse/gsldatahandle.h>
#include "gsldatahandle-mad.h"

#include <unistd.h>


/* --- structures --- */
typedef struct
{
  GslWaveFileInfo wfi;
  guint           n_channels;
  gfloat	  mix_freq;
  gfloat	  osc_freq;
} FileInfo;


/* --- functions --- */
static GslWaveFileInfo*
mad_load_file_info (gpointer      data,
		    const gchar  *file_name,
		    BseErrorType *error_p)
{
  FileInfo *fi;
  guint n_channels;
  gfloat mix_freq;
  BseErrorType error;

  error = gsl_data_handle_mad_testopen (file_name, &n_channels, &mix_freq);
  if (error)
    {
      *error_p = error;
      return NULL;
    }

  fi = sfi_new_struct0 (FileInfo, 1);
  fi->wfi.n_waves = 1;	/* we support only a single MPEG stream */
  fi->wfi.waves = g_malloc0 (sizeof (fi->wfi.waves[0]) * fi->wfi.n_waves);
  fi->wfi.waves[0].name = g_strdup (file_name);
  fi->n_channels = n_channels;
  fi->mix_freq = mix_freq;
  fi->osc_freq = 440.0;	/* FIXME */

  return &fi->wfi;
}

static void
mad_free_file_info (gpointer         data,
		    GslWaveFileInfo *file_info)
{
  FileInfo *fi = (FileInfo*) file_info;
  guint i;

  for (i = 0; i < fi->wfi.n_waves; i++)
    g_free (fi->wfi.waves[i].name);
  g_free (fi->wfi.waves);
  sfi_delete_struct (FileInfo, fi);
}

static GslWaveDsc*
mad_load_wave_dsc (gpointer         data,
		   GslWaveFileInfo *file_info,
		   guint            nth_wave,
		   BseErrorType    *error_p)
{
  FileInfo *fi = (FileInfo*) file_info;
  GslWaveDsc *wdsc = sfi_new_struct0 (GslWaveDsc, 1);

  wdsc->name = g_strdup (fi->wfi.waves[0].name);
  wdsc->n_channels = fi->n_channels;
  wdsc->n_chunks = 1;
  wdsc->chunks = g_new0 (GslWaveChunkDsc, 1);
  wdsc->chunks[0].osc_freq = fi->osc_freq;
  wdsc->chunks[0].mix_freq = fi->mix_freq;

  return wdsc;
}

static void
mad_free_wave_dsc (gpointer    data,
		   GslWaveDsc *wdsc)
{
  guint i;
  for (i = 0; i < wdsc->n_chunks; i++)
    g_strfreev (wdsc->chunks[i].xinfos);
  g_free (wdsc->chunks);
  g_free (wdsc->name);
  sfi_delete_struct (GslWaveDsc, wdsc);
}

static GslDataHandle*
mad_create_chunk_handle (gpointer      data,
			 GslWaveDsc   *wdsc,
			 guint         nth_chunk,
			 BseErrorType *error_p)
{
  FileInfo *fi = (FileInfo*) wdsc->file_info;
  GslDataHandle *dhandle;

  g_return_val_if_fail (nth_chunk == 0, NULL);

  dhandle = gsl_data_handle_new_mad_err (fi->wfi.file_name, wdsc->chunks[0].osc_freq, error_p);
  if (dhandle && wdsc->chunks[0].xinfos)
    {
      GslDataHandle *tmp_handle = dhandle;
      dhandle = gsl_data_handle_new_add_xinfos (dhandle, wdsc->chunks[0].xinfos);
      gsl_data_handle_unref (tmp_handle);
    }
  if (!dhandle && !*error_p)
    *error_p = BSE_ERROR_FILE_OPEN_FAILED;
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
  static const gchar *file_exts[] = {
    "mp1", "mp2", "mp3",
    NULL,
  };
  static const gchar *mime_types[] = {
    "audio/mp3", "audio/x-mp3", "audio/mpg3", "audio/x-mpg3", "audio/mpeg3", "audio/x-mpeg3",
    "audio/mp2", "audio/x-mp2", "audio/mpg2", "audio/x-mpg2", "audio/mpeg2", "audio/x-mpeg2",
    "audio/mp1", "audio/x-mp1", "audio/mpg1", "audio/x-mpg1", "audio/mpeg1", "audio/x-mpeg1",
    "audio/mpeg", "audio/x-mpeg",
    NULL,
  };
  static const gchar *magics[] = {
    MAGIC_MPEG10_I, MAGIC_MPEG10_II, MAGIC_MPEG10_III,
    MAGIC_MPEG20_I, MAGIC_MPEG20_II, MAGIC_MPEG20_III,
    MAGIC_MPEG25_I, MAGIC_MPEG25_II, MAGIC_MPEG25_III,
    MAGIC_RIFF_MPEG, MAGIC_RIFF_MPEG_III,
    MAGIC_MPEG_ID3,
    NULL,
  };
  static GslLoader loader = {
    "MPEG Audio (MAD: MPEG 1.0/2.0/2.5 Layer III/II/I Decoder)",
    file_exts,
    mime_types,
    GSL_LOADER_SKIP_PRECEEDING_NULLS,	/* some mp3's have preceeding 0s (partial silence frames?) */
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

  g_assert (initialized == FALSE);
  initialized = TRUE;

  if (GSL_HAVE_LIBMAD)
    gsl_loader_register (&loader);
}
