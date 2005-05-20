/* GSL - Generic Sound Layer
 * Copyright (C) 1998-2005 Tim Janik
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
#include "bseloader.h"

#include "gsldatahandle.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>


#define FORMAT_IS_ALAW(f)       ((f) == 0x0006  /* Microsoft ALAW */ || \
                                 (f) == 0x0102) /* IBM ALAW */
#define FORMAT_IS_ULAW(f)       ((f) == 0x0007  /* Microsoft MULAW */ || \
                                 (f) == 0x0101) /* IBM MULAW */


/* load routine for the RIFF/WAVE sample format
 * ref.: C't 01/1993 pp. 213
 */

typedef guint32 DWord;
typedef guint16 Word;
#define	DWORD_FROM_BE	GUINT32_FROM_BE
#define	DWORD_FROM_LE	GUINT32_FROM_LE
#define	WORD_FROM_LE	GUINT16_FROM_LE


/* --- debugging and errors --- */
static SFI_MSG_TYPE_DEFINE (debug_wav, "wav", SFI_MSG_DEBUG, NULL);
#define WAV_DEBUG(...)  sfi_debug (debug_wav, __VA_ARGS__)


/* --- functions --- */
typedef struct
{
  DWord main_chunk;     /* 'RIFF', big endian as int */
  DWord file_length;	/* file length */
  DWord chunk_type;     /* 'WAVE', big endian as int */
} WavHeader;
static BseErrorType
wav_read_header (gint       fd,
		 WavHeader *header)
{
  guint n_bytes;
  
  memset (header, 0, sizeof (*header));
  
  /* read header contents */
  n_bytes = 4 + 4 + 4;
  g_assert (n_bytes == sizeof (*header));
  if (read (fd, header, n_bytes) != n_bytes)
    {
      WAV_DEBUG ("failed to read WavHeader: %s", g_strerror (errno));
      return gsl_error_from_errno (errno, BSE_ERROR_IO);
    }
  
  /* endianess corrections */
  header->main_chunk = DWORD_FROM_BE (header->main_chunk);
  header->file_length = DWORD_FROM_LE (header->file_length);
  header->chunk_type = DWORD_FROM_BE (header->chunk_type);
  
  /* validation */
  if (header->main_chunk != ('R' << 24 | 'I' << 16 | 'F' << 8 | 'F'))
    {
      WAV_DEBUG ("unmatched token 'RIFF'");
      return BSE_ERROR_FORMAT_INVALID;
    }
  if (header->file_length < 36)
    {
      WAV_DEBUG ("file length (%u) too small", header->file_length);
      return BSE_ERROR_FORMAT_INVALID;
    }
  if (header->chunk_type != ('W' << 24 | 'A' << 16 | 'V' << 8 | 'E'))
    {
      WAV_DEBUG ("unmatched token 'WAVE'");
      return BSE_ERROR_FORMAT_INVALID;
    }
  
  return BSE_ERROR_NONE;
}

typedef struct
{
  DWord sub_chunk;              /* 'fmt ', big endian as int */
  DWord length;                 /* sub chunk length, must be 16 */
  Word  format;                 /* 1 for PCM */
  Word  n_channels;             /* 1 = Mono, 2 = Stereo */
  DWord sample_freq;
  DWord byte_per_second;
  Word  byte_per_sample;        /* 1 = 8bit, 2 = 16bit */
  Word  bit_per_sample;         /* 8, 12 or 16 */
} FmtHeader;
static BseErrorType
wav_read_fmt_header (gint       fd,
		     FmtHeader *header)
{
  guint n_bytes;
  
  memset (header, 0, sizeof (*header));
  
  /* read header contents */
  n_bytes = 4 + 4 + 2 + 2 + 4 + 4 + 2 + 2;
  g_assert (n_bytes == sizeof (*header));
  if (read (fd, header, n_bytes) != n_bytes)
    {
      WAV_DEBUG ("failed to read FmtHeader");
      return gsl_error_from_errno (errno, BSE_ERROR_IO);
    }
  
  /* endianess corrections */
  header->sub_chunk = DWORD_FROM_BE (header->sub_chunk);
  header->length = DWORD_FROM_LE (header->length);
  header->format = WORD_FROM_LE (header->format);
  header->n_channels = WORD_FROM_LE (header->n_channels);
  header->sample_freq = DWORD_FROM_LE (header->sample_freq);
  header->byte_per_second = DWORD_FROM_LE (header->byte_per_second);
  header->byte_per_sample = WORD_FROM_LE (header->byte_per_sample);
  header->bit_per_sample = WORD_FROM_LE (header->bit_per_sample);
  
  /* validation */
  if (header->sub_chunk != ('f' << 24 | 'm' << 16 | 't' << 8 | ' '))
    {
      WAV_DEBUG ("unmatched token 'fmt '");
      return BSE_ERROR_FORMAT_UNKNOWN;
    }
  if (header->format != 1 /* PCM */ &&
      !FORMAT_IS_ALAW (header->format) &&
      !FORMAT_IS_ULAW (header->format))
    {
      WAV_DEBUG ("unknown format (%u)", header->format);
      return BSE_ERROR_FORMAT_UNKNOWN;
    }
  if (header->n_channels > 2 || header->n_channels < 1)
    {
      WAV_DEBUG ("invalid number of channels (%u)", header->n_channels);
      return BSE_ERROR_FORMAT_UNKNOWN;
    }
  if (header->length < 16)
    {
      WAV_DEBUG ("WAVE header too short (%u)", header->length);
      return BSE_ERROR_FORMAT_INVALID;
    }
  if (header->sample_freq < 1378 || header->sample_freq > 96000)
    {
      WAV_DEBUG ("invalid sample_freq (%u)", header->sample_freq);
      return BSE_ERROR_FORMAT_UNKNOWN;
    }
  if (header->byte_per_sample < 1 || header->byte_per_sample > 4 ||
      (header->bit_per_sample != 8 && header->bit_per_sample != 12 && header->bit_per_sample != 16))
    {
      WAV_DEBUG ("invalid byte_per_sample (%u) or bit_per_sample (%u)", header->byte_per_sample, header->bit_per_sample);
      return BSE_ERROR_FORMAT_UNKNOWN;
    }
  if (header->byte_per_second != header->sample_freq * header->byte_per_sample ||
      header->byte_per_sample != (header->bit_per_sample + 7) / 8 * header->n_channels)
    {
      WAV_DEBUG ("invalid byte_per_second (%u!=%u) or byte_per_sample (%u!=%u)",
		 header->byte_per_second, header->sample_freq * header->byte_per_sample,
		 header->byte_per_sample, (header->bit_per_sample + 7) / 8 * header->n_channels);
      return BSE_ERROR_FORMAT_INVALID;
    }
  if (header->length > 16)
    {
      guint n;
      
      WAV_DEBUG ("skipping %u bytes of overlong WAVE header", header->length - 16);
      
      n = header->length - 16;
      while (n)
	{
	  guint8 junk[64];
	  guint l = MIN (n, 64);
	  
	  l = read (fd, junk, l);
	  if (l < 1 || l > n)
	    {
	      sfi_diag ("failed to read FmtHeader from WAVE file");
	      return gsl_error_from_errno (errno, BSE_ERROR_IO);
	    }
	  n -= l;
	}
    }
  
  return BSE_ERROR_NONE;
}

typedef struct
{
  DWord data_chunk;             /* 'data', big endian as int */
  DWord data_length;
} DataHeader;
static BseErrorType
wav_read_data_header (gint        fd,
		      DataHeader *header,
		      guint       byte_alignment)
{
  guint n_bytes;
  
  memset (header, 0, sizeof (*header));
  
  /* read header contents */
  n_bytes = 4 + 4;
  g_assert (n_bytes == sizeof (*header));
  if (read (fd, header, n_bytes) != n_bytes)
    {
      WAV_DEBUG ("failed to read DataHeader");
      return gsl_error_from_errno (errno, BSE_ERROR_IO);
    }
  
  /* endianess corrections */
  header->data_chunk = DWORD_FROM_BE (header->data_chunk);
  header->data_length = DWORD_FROM_LE (header->data_length);
  
  /* validation */
  if (header->data_chunk != ('d' << 24 | 'a' << 16 | 't' << 8 | 'a'))
    {
      guchar chunk[5];
      gchar *esc;
      
      chunk[0] = header->data_chunk >> 24;
      chunk[1] = (header->data_chunk >> 16) & 0xff;
      chunk[2] = (header->data_chunk >> 8) & 0xff;
      chunk[3] = header->data_chunk & 0xff;
      chunk[4] = 0;
      esc = g_strescape (chunk, NULL);
      
      /* skip chunk and retry */
      WAV_DEBUG ("ignoring sub-chunk '%s'", esc);
      g_free (esc);
      if (lseek (fd, header->data_length, SEEK_CUR) < 0)
	{
	  WAV_DEBUG ("failed to seek while skipping sub-chunk");
	  return gsl_error_from_errno (errno, BSE_ERROR_IO);
	}
      return wav_read_data_header (fd, header, byte_alignment);
    }
  if (header->data_length < 1 || header->data_length % byte_alignment != 0)
    {
      WAV_DEBUG ("invalid data length (%u) or alignment (%u)",
		 header->data_length, header->data_length % byte_alignment);
      return BSE_ERROR_FORMAT_INVALID;
    }
  
  return BSE_ERROR_NONE;
}

typedef struct
{
  BseWaveFileInfo wfi;
  gint            fd;
} FileInfo;

static BseWaveFileInfo*
wav_load_file_info (gpointer      data,
		    const gchar  *file_name,
		    BseErrorType *error_p)
{
  WavHeader wav_header;
  FileInfo *fi;
  gint fd;
  
  fd = open (file_name, O_RDONLY);
  if (fd < 0)
    {
      *error_p = gsl_error_from_errno (errno, BSE_ERROR_FILE_OPEN_FAILED);
      return NULL;
    }
  
  *error_p = wav_read_header (fd, &wav_header);
  if (*error_p)
    {
      close (fd);
      return NULL;
    }
  
  fi = sfi_new_struct0 (FileInfo, 1);
  fi->wfi.n_waves = 1;
  fi->wfi.waves = g_malloc0 (sizeof (fi->wfi.waves[0]) * fi->wfi.n_waves);
  fi->wfi.waves[0].name = g_strdup (file_name);
  fi->fd = fd;
  
  return &fi->wfi;
}

static void
wav_free_file_info (gpointer         data,
		    BseWaveFileInfo *file_info)
{
  FileInfo *fi = (FileInfo*) file_info;
  
  g_free (fi->wfi.waves[0].name);
  g_free (fi->wfi.waves);
  close (fi->fd);
  sfi_delete_struct (FileInfo, fi);
}

typedef struct
{
  BseWaveDsc wdsc;
  GslLong    data_offset;
  GslLong    n_values;
  GslWaveFormatType format;
} WaveDsc;

static BseWaveDsc*
wav_load_wave_dsc (gpointer         data,
		   BseWaveFileInfo *file_info,
		   guint            nth_wave,
		   BseErrorType    *error_p)
{
  FileInfo *fi = (FileInfo*) file_info;
  DataHeader data_header;
  FmtHeader fmt_header;
  WaveDsc *dsc;
  GslWaveFormatType format;
  GslLong data_offset, data_width;
  
  g_return_val_if_fail (nth_wave == 0, NULL);
  
  if (lseek (fi->fd, sizeof (WavHeader), SEEK_SET) != sizeof (WavHeader))
    {
      WAV_DEBUG ("failed to seek to end of WavHeader");
      *error_p = gsl_error_from_errno (errno, BSE_ERROR_IO);
      return NULL;
    }
  
  *error_p = wav_read_fmt_header (fi->fd, &fmt_header);
  if (*error_p)
    return NULL;
  
  data_width = (fmt_header.bit_per_sample + 7) / 8;
  *error_p = wav_read_data_header (fi->fd, &data_header, data_width * fmt_header.n_channels);
  data_offset = lseek (fi->fd, 0, SEEK_CUR);
  if (data_offset < sizeof (WavHeader) && !*error_p)
    {
      WAV_DEBUG ("failed to seek to start of data");
      *error_p = gsl_error_from_errno (errno, BSE_ERROR_IO);
    }
  if (*error_p)
    return NULL;

  if (fmt_header.bit_per_sample == 8 && FORMAT_IS_ALAW (fmt_header.format))
    format = GSL_WAVE_FORMAT_ALAW;
  else if (fmt_header.bit_per_sample == 8 && FORMAT_IS_ULAW (fmt_header.format))
    format = GSL_WAVE_FORMAT_ULAW;
  else switch (fmt_header.bit_per_sample)
    {
    case 8:	format = GSL_WAVE_FORMAT_UNSIGNED_8;	break;
    case 12:	format = GSL_WAVE_FORMAT_SIGNED_12;	break;
    case 16:	format = GSL_WAVE_FORMAT_SIGNED_16;	break;
    default:
      WAV_DEBUG ("unrecognized sample width (%u)", fmt_header.bit_per_sample);
      *error_p = BSE_ERROR_FORMAT_UNKNOWN;
      return NULL;
    }
  if (0)
    WAV_DEBUG ("n_channels: %d sample_freq: %d bit_width: %u",
	       fmt_header.n_channels, fmt_header.sample_freq, fmt_header.bit_per_sample);
  
  dsc = sfi_new_struct0 (WaveDsc, 1);
  dsc->wdsc.name = g_strdup (fi->wfi.waves[0].name);
  dsc->wdsc.n_channels = fmt_header.n_channels;
  dsc->wdsc.n_chunks = 1;
  dsc->wdsc.chunks = g_malloc0 (sizeof (dsc->wdsc.chunks[0]) * dsc->wdsc.n_chunks);
  dsc->wdsc.chunks[0].mix_freq = fmt_header.sample_freq;
  dsc->wdsc.chunks[0].osc_freq = 440.0;	/* FIXME */
  dsc->data_offset = data_offset;
  dsc->n_values = data_header.data_length / data_width;
  dsc->format = format;
  
  return &dsc->wdsc;
}

static void
wav_free_wave_dsc (gpointer    data,
		   BseWaveDsc *wave_dsc)
{
  WaveDsc *dsc = (WaveDsc*) wave_dsc;
  guint i;
  for (i = 0; i < dsc->wdsc.n_chunks; i++)
    g_strfreev (dsc->wdsc.chunks[i].xinfos);
  g_free (dsc->wdsc.chunks);
  g_free (dsc->wdsc.name);
  sfi_delete_struct (WaveDsc, dsc);
}

static GslDataHandle*
wav_create_chunk_handle (gpointer      data,
			 BseWaveDsc   *wave_dsc,
			 guint         nth_chunk,
			 BseErrorType *error_p)
{
  WaveDsc *dsc = (WaveDsc*) wave_dsc;
  GslDataHandle *dhandle;
  
  g_return_val_if_fail (nth_chunk == 0, NULL);
  
  dhandle = gsl_wave_handle_new (dsc->wdsc.file_info->file_name,
				 dsc->wdsc.n_channels,
				 dsc->format, G_LITTLE_ENDIAN,
                                 dsc->wdsc.chunks[nth_chunk].mix_freq,
                                 dsc->wdsc.chunks[nth_chunk].osc_freq,
				 dsc->data_offset, dsc->n_values,
                                 dsc->wdsc.chunks[nth_chunk].xinfos);
  return dhandle;
}

void
_gsl_init_loader_wav (void)
{
  static const gchar *file_exts[] = { "wav", NULL, };
  static const gchar *mime_types[] = { "audio/wav", "audio/x-wav", NULL, };
  static const gchar *magics[] = {
    (
     "0  string  RIFF\n"        /* RIFF little-endian data */
     "8  string  WAVE\n"        /* WAVE audio */
     "12 string  fmt\\s\n"	/* expect "fmt " */
     "16 lelong  >15\n"		/* expect valid sub chunk length */
     "20 leshort =1\n"		/* Microsoft PCM */
     ),
    (
     "0  string  RIFF\n" "8  string  WAVE\n" "12 string  fmt\\s\n" "16 lelong  >15\n"
     "20 leshort =0x0006\n"	/* Microsoft A-LAW */
     ),
    (
     "0  string  RIFF\n" "8  string  WAVE\n" "12 string  fmt\\s\n" "16 lelong  >15\n"
     "20 leshort =0x0102\n"	/* IBM A-LAW */
     ),
    (
     "0  string  RIFF\n" "8  string  WAVE\n" "12 string  fmt\\s\n" "16 lelong  >15\n"
     "20 leshort =0x0007\n"	/* Microsoft U-LAW */
     ),
    (
     "0  string  RIFF\n" "8  string  WAVE\n" "12 string  fmt\\s\n" "16 lelong  >15\n"
     "20 leshort =0x0101\n"	/* IBM U-LAW */
     ),
    NULL,
  };
  static BseLoader loader = {
    "RIFF, WAVE audio, PCM",
    file_exts,
    mime_types,
    0,	/* flags */
    magics,
    0,  /* priority */
    NULL,
    wav_load_file_info,
    wav_free_file_info,
    wav_load_wave_dsc,
    wav_free_wave_dsc,
    wav_create_chunk_handle,
  };
  static gboolean initialized = FALSE;
  
  g_assert (initialized == FALSE);
  initialized = TRUE;
  
  bse_loader_register (&loader);
}
