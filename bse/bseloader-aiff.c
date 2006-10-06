/* GSL - Generic Sound Layer
 * Copyright (C) 1998-2002 Tim Janik
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
#include "bsemain.h"
#include "gsldatahandle.h"
#include "bsemath.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>


/* audio file loader for the FORM/AIFF sample format, according to:
 * "Audio Interchange File Format AIFF, A Standard for Sampled Sound Files, Version 1.3"
 */

/* --- macros --- */
static SFI_MSG_TYPE_DEFINE (debug_aiff, "aiff", SFI_MSG_DEBUG, NULL);
#define AIFF_DEBUG(...)         sfi_debug (debug_aiff, __VA_ARGS__)
#define AIFF_ULONG(a,b,c,d)     (((a) << 24) | ((b) << 16) | ((c) <<  8) | (d))
#define AIFF_ID(str4)           AIFF_ULONG (str4[0], str4[1], str4[2], str4[3])


/* --- structures & typedefs --- */
typedef struct
{
  guint32 form_type;    /* AIFF_ID("AIFF") */
  guint   n_channels;
  guint   bit_depth;
  guint   n_values;     /* n-channels * n-values-per-channel */
  gdouble mix_freq;
  gchar  *name, *author, *copyright, *annotation;
  guint   n_markers;
  struct {
    guint16 id;
    guint   pos;
    gchar  *name;
  }      *markers;
  struct {
    guint8  base_note;
    gint8   detune;
    guint8  low_note, high_note;
    guint8  low_velocity, high_velocity;
    gint16  gain_dB;
    guint16 sustain_loop_mode;        /* 0=none, 1=forward, 2=ping-pong */
    guint16 sustain_begin_id;         /* marker id */
    guint16 sustain_end_id;           /* marker id */
    guint16 release_loop_mode;        /* 0=none, 1=forward, 2=ping-pong */
    guint16 release_begin_id;         /* marker id */
    guint16 release_end_id;           /* marker id */
  }       instrument;
  guint   data_start;   /* file position */
  guint   data_size;    /* in bytes */
} AiffFile;


/* --- functions --- */
static inline int
aiff_read_u32 (int      fd,
               guint32 *data)
{
  gint r;
  do
    r = read (fd, data, 4);
  while (r < 0 && errno == EINTR);
  *data = GUINT32_FROM_BE (*data);
  return r;
}

static inline int
aiff_read_s16 (int     fd,
               gint16 *data)
{
  gint r;
  do
    r = read (fd, data, 2);
  while (r < 0 && errno == EINTR);
  *data = GINT16_FROM_BE (*data);
  return r;
}

static inline int
aiff_read_u16 (int      fd,
               guint16 *data)
{
  gint r;
  do
    r = read (fd, data, 2);
  while (r < 0 && errno == EINTR);
  *data = GUINT16_FROM_BE (*data);
  return r;
}

static inline int
aiff_read_f80 (int      fd,
               gdouble *data)
{
  guint8 bytes[10];
  guint32 mantissa_low, mantissa_high;
  gint32 biased_exponent, sign;
  gdouble d;
  gint r;
  do
    r = read (fd, bytes, 10);
  while (r < 0 && errno == EINTR);
  sign = bytes[0] >> 7;
  biased_exponent = ((bytes[0] & 0x7f) << 8) | bytes[1];
  mantissa_high = AIFF_ULONG (bytes[2], bytes[3], bytes[4], bytes[5]);
  mantissa_low = AIFF_ULONG (bytes[6], bytes[7], bytes[8], bytes[9]);
  if (biased_exponent == 0x7fff)        /* IS_NANINF() */
    d = mantissa_high | mantissa_low ? BSE_DOUBLE_NAN : BSE_DOUBLE_INF;
  else
    {
      biased_exponent -= 0x3fff;
      d = ldexp (mantissa_high, biased_exponent - 31);
      d += ldexp (mantissa_low, biased_exponent - 63);
    }
  d = sign ? -d : d;
  *data = d;
  return r;
}

static inline int
aiff_read_pstring (int     fd,
                   gchar **pstring)
{
  guint8 length;
  gchar *string;
  gint r;
  do
    r = read (fd, &length, 1);
  while (r < 0 && errno == EINTR);
  if (r < 0)
    return r;
  string = g_new (gchar, length + 1);
  do
    r = read (fd, string, length | 1);    /* force even string length */
  while (r < 0 && errno == EINTR);
  if (r < 0)
    {
      g_free (string);
      return r;
    }
  string[length] = 0;
  *pstring = string;
  return r;
}

static BseErrorType
aiff_read_comm (gint      fd,
                AiffFile *afile,
                guint32   chunk_size)
{
  gint16 num_channels, sample_size;
  guint32 num_sample_frames;
  gdouble sample_rate;
  if (chunk_size < 18)
    return BSE_ERROR_FORMAT_INVALID;
  if (aiff_read_s16 (fd, &num_channels) < 0 ||
      aiff_read_u32 (fd, &num_sample_frames) < 0 ||
      aiff_read_s16 (fd, &sample_size) < 0 ||
      aiff_read_f80 (fd, &sample_rate) < 0)
    return gsl_error_from_errno (errno, BSE_ERROR_FILE_READ_FAILED);
  AIFF_DEBUG ("COMM: num_channels=%d num_sample_frames=%u sample_size=%d sample_rate=%f",
              num_channels, num_sample_frames, sample_size, sample_rate);
  if (num_channels <= 0 || sample_size <= 0 || sample_rate <= 0)
    return BSE_ERROR_DATA_CORRUPT;
  afile->n_channels = num_channels;
  afile->bit_depth = sample_size;
  afile->n_values = num_sample_frames * num_channels;
  afile->mix_freq = sample_rate;
  return BSE_ERROR_NONE;
}

static BseErrorType
aiff_read_mark (gint      fd,
                AiffFile *afile,
                guint32   chunk_size)
{
  guint16 num_markers;
  guint i;
  if (chunk_size < 2)
    return BSE_ERROR_FORMAT_INVALID;
  if (aiff_read_u16 (fd, &num_markers) < 0)
    return gsl_error_from_errno (errno, BSE_ERROR_FILE_READ_FAILED);
  for (i = 0; i < num_markers; i++)
    {
      guint16 marker_id;
      guint32 position, j;
      gchar *marker_name;
      if (aiff_read_u16 (fd, &marker_id) < 0 ||
          aiff_read_u32 (fd, &position) < 0 ||
          aiff_read_pstring (fd, &marker_name) < 0)
        return gsl_error_from_errno (errno, BSE_ERROR_FILE_READ_FAILED);
      j = afile->n_markers++;
      afile->markers = g_realloc (afile->markers, sizeof (afile->markers[0]) * afile->n_markers);
      afile->markers[j].id = marker_id;
      afile->markers[j].pos = position;
      afile->markers[j].name = marker_name;
      AIFF_DEBUG ("MARK: %u) >%u< \"%s\"", marker_id, position, marker_name);
    }
  return BSE_ERROR_NONE;
}

static BseErrorType
aiff_read_inst (gint      fd,
                AiffFile *afile,
                guint32   chunk_size)
{
  gint r;
  g_static_assert (sizeof (afile->instrument) == 20);
  if (chunk_size < 20)
    return BSE_ERROR_FORMAT_INVALID;
  do
    r = read (fd, &afile->instrument, 20);
  while (r < 0 && errno == EINTR);
  if (r < 0)
    return r;
  afile->instrument.gain_dB = GINT16_FROM_BE (afile->instrument.gain_dB);
  afile->instrument.sustain_loop_mode = GUINT16_FROM_BE (afile->instrument.sustain_loop_mode);
  afile->instrument.sustain_begin_id = GUINT16_FROM_BE (afile->instrument.sustain_begin_id);
  afile->instrument.sustain_end_id = GUINT16_FROM_BE (afile->instrument.sustain_end_id);
  afile->instrument.release_loop_mode = GUINT16_FROM_BE (afile->instrument.release_loop_mode);
  afile->instrument.release_begin_id = GUINT16_FROM_BE (afile->instrument.release_begin_id);
  afile->instrument.release_end_id = GUINT16_FROM_BE (afile->instrument.release_end_id);
  AIFF_DEBUG ("INST: N:%u<=%u%+d<=%u V:%u..%u G:%+ddB S:{%u:%u..%u} R:{%u:%u..%u}",
              afile->instrument.low_note, afile->instrument.base_note, afile->instrument.detune, afile->instrument.high_note,
              afile->instrument.low_velocity, afile->instrument.high_velocity, afile->instrument.gain_dB,
              afile->instrument.sustain_loop_mode, afile->instrument.sustain_begin_id, afile->instrument.sustain_end_id,
              afile->instrument.release_loop_mode, afile->instrument.release_begin_id, afile->instrument.release_end_id);
  return BSE_ERROR_NONE;
}

static BseErrorType
aiff_read_ssnd (gint      fd,
                AiffFile *afile,
                guint32   chunk_size)
{
  guint32 alignment_offset, alignment_block_size;
  off_t pos;
  if (chunk_size < 8)
    return BSE_ERROR_FORMAT_INVALID;
  if (aiff_read_u32 (fd, &alignment_offset) < 0 ||
      aiff_read_u32 (fd, &alignment_block_size) < 0)
    return gsl_error_from_errno (errno, BSE_ERROR_FILE_READ_FAILED);
  do
    pos = lseek (fd, 0, SEEK_CUR);
  while (pos < 0 && errno == EINTR);
  if (pos < 0)
    return gsl_error_from_errno (errno, BSE_ERROR_FILE_SEEK_FAILED);
  if (chunk_size < 8 + alignment_offset)
    return BSE_ERROR_FORMAT_INVALID;
  afile->data_start = pos + alignment_offset;
  afile->data_size = chunk_size - 8 - alignment_offset;
  AIFF_DEBUG ("SSND: pos:>%u< n_bytes:%u", afile->data_start, afile->data_size);
  return BSE_ERROR_NONE;
}

static BseErrorType
aiff_append_string (gint      fd,
                    AiffFile *afile,
                    guint32   chunk_id,
                    guint32   chunk_size,
                    gchar   **text)
{
  gchar *string, *old = *text;
  chunk_size = MIN (chunk_size, 0xfffe);
  string = g_new (gchar, chunk_size + 1);
  gint r;
  do
    r = read (fd, string, chunk_size);
  while (r < 0 && errno == EINTR);
  string[r] = 0;
  AIFF_DEBUG ("%c%c%c%c: %s", chunk_id >> 24, chunk_id >> 16 & 0xff, chunk_id >> 8 & 0xff, chunk_id & 0xff, string);
  *text = g_strconcat (old ? old : "", string, NULL);
  g_free (old);
  g_free (string);
  return BSE_ERROR_NONE;
}

static BseErrorType
aiff_file_load (gint      fd,
                AiffFile *afile)
{
  guint32 form_id, form_size, form_type, seek_pos;
  if (lseek (fd, 0, SEEK_SET) < 0)
    return gsl_error_from_errno (errno, BSE_ERROR_FILE_SEEK_FAILED);
  if (aiff_read_u32 (fd, &form_id) < 0 ||
      aiff_read_u32 (fd, &form_size) < 0 ||
      aiff_read_u32 (fd, &form_type) < 0)
    return gsl_error_from_errno (errno, BSE_ERROR_FILE_READ_FAILED);
  if (form_id != AIFF_ID ("FORM") || form_size < 4 || form_type != AIFF_ID ("AIFF"))
    return BSE_ERROR_FORMAT_UNKNOWN;

  afile->form_type = form_type;
  seek_pos = 12; /* we've read up 12 bytes so far */
  while (seek_pos < 8 + form_size)
    {
      guint32 chunk_id, chunk_size;
      BseErrorType error;
      if (aiff_read_u32 (fd, &chunk_id) < 0 ||
          aiff_read_u32 (fd, &chunk_size) < 0)
        return gsl_error_from_errno (errno, BSE_ERROR_FILE_EOF); /* premature eof? */
      seek_pos += 4 + 4;
      switch (chunk_id)
        {
        case AIFF_ULONG ('C','O','M','M'): error = aiff_read_comm (fd, afile, chunk_size); break;
        case AIFF_ULONG ('M','A','R','K'): error = aiff_read_mark (fd, afile, chunk_size); break;
        case AIFF_ULONG ('I','N','S','T'): error = aiff_read_inst (fd, afile, chunk_size); break;
        case AIFF_ULONG ('S','S','N','D'): error = aiff_read_ssnd (fd, afile, chunk_size); break;
        case AIFF_ULONG ('N','A','M','E'): error = aiff_append_string (fd, afile, chunk_id, chunk_size, &afile->name); break;
        case AIFF_ULONG ('A','U','T','H'): error = aiff_append_string (fd, afile, chunk_id, chunk_size, &afile->author); break;
        case AIFF_ULONG ('(','c',')',' '): error = aiff_append_string (fd, afile, chunk_id, chunk_size, &afile->copyright); break;
        case AIFF_ULONG ('A','N','N','O'): error = aiff_append_string (fd, afile, chunk_id, chunk_size, &afile->annotation); break;
        default:                           error = 0;      /* ignore unknown chunks */
          AIFF_DEBUG ("%c%c%c%c: ignored...",
                      chunk_id >> 24, chunk_id >> 16 & 0xff, chunk_id >> 8 & 0xff, chunk_id & 0xff);
        }
      if (error)
        return error;
      seek_pos += chunk_size;
      /* align to even seek sizes by skipping pad bytes */
      seek_pos = seek_pos & 1 ? seek_pos + 1 : seek_pos;
      if (lseek (fd, seek_pos, SEEK_SET) < 0)
        return gsl_error_from_errno (errno, BSE_ERROR_FILE_SEEK_FAILED);
    }
  return BSE_ERROR_NONE;
}

static void
aiff_file_free (AiffFile *afile)
{
  guint i;
  for (i = 0; i < afile->n_markers; i++)
    g_free (afile->markers[i].name);
  g_free (afile->markers);
  g_free (afile->name);
  g_free (afile->author);
  g_free (afile->copyright);
  g_free (afile->annotation);
  g_free (afile);
}

typedef struct
{
  BseWaveFileInfo wfi;
  AiffFile       *afile;
} FileInfo;

static BseWaveFileInfo*
aiff_load_file_info (gpointer      data,
                     const gchar  *file_name,
                     BseErrorType *error_p)
{
  AiffFile *afile;
  FileInfo *fi;
  gint fd = open (file_name, O_RDONLY);
  gchar *str;
  if (fd < 0)
    {
      *error_p = gsl_error_from_errno (errno, BSE_ERROR_FILE_OPEN_FAILED);
      return NULL;
    }
  afile = g_new0 (AiffFile, 1);
  /* defaults */
  afile->instrument.low_note = 0;
  afile->instrument.base_note = 60;
  afile->instrument.high_note = 127;
  afile->instrument.low_velocity = 1;
  afile->instrument.high_velocity = 127;
  *error_p = aiff_file_load (fd, afile);
  close (fd);
  if (*error_p)
    {
      aiff_file_free (afile);
      return NULL;
    }
  /* sanity check file contents */
  if (afile->n_channels < 1 || afile->n_channels > 2 ||
      afile->bit_depth > 16 || afile->mix_freq < 8000)
    {
      aiff_file_free (afile);
      *error_p = BSE_ERROR_FORMAT_UNKNOWN;
      return NULL;
    }
  if (afile->n_values < afile->n_channels ||
      afile->data_size < (afile->bit_depth + 7) / 8 * afile->n_values)
    {
      aiff_file_free (afile);
      *error_p = BSE_ERROR_NO_DATA;
      return NULL;
    }
  fi = sfi_new_struct0 (FileInfo, 1);
  fi->wfi.n_waves = 1;
  fi->wfi.waves = g_malloc0 (sizeof (fi->wfi.waves[0]) * fi->wfi.n_waves);
  str = g_path_get_basename (file_name);
  fi->wfi.waves[0].name = g_strdup (afile->name ? afile->name : str);
  g_free (str);
  fi->afile = afile;

  return &fi->wfi;
}

static void
aiff_free_file_info (gpointer         data,
                     BseWaveFileInfo *file_info)
{
  FileInfo *fi = (FileInfo*) file_info;
  
  aiff_file_free (fi->afile);
  g_free (fi->wfi.waves[0].name);
  g_free (fi->wfi.waves);
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
aiff_load_wave_dsc (gpointer         data,
                    BseWaveFileInfo *file_info,
                    guint            nth_wave,
                    BseErrorType    *error_p)
{
  FileInfo *fi = (FileInfo*) file_info;
  AiffFile *afile = fi->afile;
  WaveDsc *dsc;

  g_return_val_if_fail (nth_wave == 0, NULL);

  dsc = sfi_new_struct0 (WaveDsc, 1);
  dsc->wdsc.name = fi->wfi.waves[0].name;
  dsc->wdsc.n_channels = afile->n_channels;
  dsc->wdsc.xinfos = bse_xinfos_add_value (dsc->wdsc.xinfos, "authors", fi->afile->author);
  dsc->wdsc.xinfos = bse_xinfos_add_value (dsc->wdsc.xinfos, "license", fi->afile->copyright);
  dsc->wdsc.xinfos = bse_xinfos_add_value (dsc->wdsc.xinfos, "blurb", fi->afile->annotation);
  dsc->wdsc.n_chunks = 1;
  dsc->wdsc.chunks = g_malloc0 (sizeof (dsc->wdsc.chunks[0]) * dsc->wdsc.n_chunks);
  dsc->wdsc.chunks[0].mix_freq = afile->mix_freq;
  dsc->wdsc.chunks[0].osc_freq = bse_temp_freq (BSE_CONFIG (kammer_freq),
                                                afile->instrument.base_note - BSE_CONFIG (midi_kammer_note));
  if (afile->instrument.sustain_loop_mode > 0 && afile->instrument.sustain_loop_mode <= 2 &&
      afile->instrument.sustain_begin_id && afile->instrument.sustain_end_id)
    {
      guint16 bid = afile->instrument.sustain_begin_id, eid = afile->instrument.sustain_end_id;
      guint bpos = 0, epos = 0;
      guint i;
      for (i = 0; i < afile->n_markers && (bid || eid); i++)
        if (afile->markers[i].id == bid)
          {
            bid = 0;
            bpos = afile->markers[i].pos;
          }
        else if (afile->markers[i].id == eid)
          {
            eid = 0;
            epos = afile->markers[i].pos;
          }
      if (!bid && !eid && bpos < epos && epos <= afile->n_values / afile->n_channels)
        {
          dsc->wdsc.chunks[0].xinfos = bse_xinfos_add_value (dsc->wdsc.chunks[0].xinfos,
                                                             "loop-type",
                                                             gsl_wave_loop_type_to_string (afile->instrument.sustain_loop_mode == 1 ?
                                                                                           GSL_WAVE_LOOP_JUMP :
                                                                                           GSL_WAVE_LOOP_PINGPONG));
          dsc->wdsc.chunks[0].xinfos = bse_xinfos_add_num (dsc->wdsc.chunks[0].xinfos, "loop-start", bpos * afile->n_channels);
          dsc->wdsc.chunks[0].xinfos = bse_xinfos_add_num (dsc->wdsc.chunks[0].xinfos, "loop-end", epos * afile->n_channels);
          dsc->wdsc.chunks[0].xinfos = bse_xinfos_add_num (dsc->wdsc.chunks[0].xinfos, "loop-count", 1000000); // FIXME
        }
    }
  dsc->data_offset = afile->data_start;
  dsc->n_values = afile->n_values;
  /* in aiff, data is left shifted up to byte boundary */
  dsc->format = afile->bit_depth > 8 ? GSL_WAVE_FORMAT_SIGNED_16 : GSL_WAVE_FORMAT_SIGNED_8;
  
  return &dsc->wdsc;
}

static void
aiff_free_wave_dsc (gpointer    data,
                    BseWaveDsc *wave_dsc)
{
  WaveDsc *dsc = (WaveDsc*) wave_dsc;
  
  guint i;
  for (i = 0; i < dsc->wdsc.n_chunks; i++)
    g_strfreev (dsc->wdsc.chunks[i].xinfos);
  g_free (dsc->wdsc.chunks);
  sfi_delete_struct (WaveDsc, dsc);
}

static GslDataHandle*
aiff_create_chunk_handle (gpointer      data,
                          BseWaveDsc   *wave_dsc,
                          guint         nth_chunk,
                          BseErrorType *error_p)
{
  WaveDsc *dsc = (WaveDsc*) wave_dsc;
  FileInfo *fi = (FileInfo*) dsc->wdsc.file_info;
  GslDataHandle *dhandle;
  
  g_return_val_if_fail (nth_chunk == 0, NULL);
  
  dhandle = gsl_wave_handle_new (fi->wfi.file_name,
				 dsc->wdsc.n_channels,
				 dsc->format, G_BIG_ENDIAN,
                                 dsc->wdsc.chunks[nth_chunk].mix_freq,
                                 dsc->wdsc.chunks[nth_chunk].osc_freq,
				 dsc->data_offset, dsc->n_values,
                                 dsc->wdsc.chunks[nth_chunk].xinfos);
  return dhandle;
}

void
_gsl_init_loader_aiff (void)
{
  static const gchar *file_exts[] = { "aiff", "aif", NULL, };
  static const gchar *mime_types[] = { "audio/aiff", "audio/x-aiff", NULL, };
  static const gchar *magics[] = {
    (
     "0  string  FORM\n"
     "8  string  AIFF\n"
     ),
    NULL,
  };
  static BseLoader loader = {
    "Audio Interchange File Format",
    file_exts,
    mime_types,
    0,	/* flags */
    magics,
    0,  /* priority */
    NULL,
    aiff_load_file_info,
    aiff_free_file_info,
    aiff_load_wave_dsc,
    aiff_free_wave_dsc,
    aiff_create_chunk_handle,
  };
  static gboolean initialized = FALSE;
  
  g_assert (initialized == FALSE);
  initialized = TRUE;
  
  bse_loader_register (&loader);
}
