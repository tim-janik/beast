/* BseWaveTool - BSE Wave creation tool
 * Copyright (C) 2001-2004 Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bsewavetool.h"
#include "sfiutils.h"
#include <bse/gslvorbis-enc.h>
#include <bse/gslvorbis-cutter.h>
#include <bse/gslloader.h>
#include <bse/gsldatautils.h>
#include <bse/gsldatahandle-vorbis.h>

#include <bse/bsemain.h>	/* for bse_init_intern() */
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>

#define PRG     "bsewavetool"

#define	IROUND(dbl)	((int) (floor (dbl + 0.5)))


/* --- functions --- */
BseWtWave*
bse_wt_new_wave (const gchar *wave_name,
		 guint        n_channels)
{
  BseWtWave *wave;
  
  g_return_val_if_fail (wave_name != NULL, NULL);
  
  wave = g_new (BseWtWave, 1);
  wave->wave_name = g_strdup (wave_name);
  wave->n_channels = n_channels;
  wave->n_chunks = 0;
  wave->chunks = NULL;
  
  return wave;
}

void
bse_wt_add_chunk (BseWtWave     *wave,
		  gfloat         mix_freq,
		  gfloat         osc_freq,
		  GslDataHandle *dhandle)
{
  guint i;
  
  g_return_if_fail (wave != NULL);
  g_return_if_fail (dhandle != NULL);
  
  i = wave->n_chunks++;
  wave->chunks = g_renew (BseWtChunk, wave->chunks, wave->n_chunks);
  memset (wave->chunks + i, 0, sizeof (wave->chunks[i]));
  wave->chunks[i].osc_freq = osc_freq;
  wave->chunks[i].mix_freq = mix_freq;
  wave->chunks[i].dhandle = gsl_data_handle_ref (dhandle);
}

void
bse_wt_add_chunk_midi (BseWtWave     *wave,
		       gfloat         mix_freq,
		       guint          midi_note,
		       GslDataHandle *dhandle)
{
  gfloat osc_freq;
  guint i;
  
  g_return_if_fail (wave != NULL);
  
  i = wave->n_chunks;
  osc_freq = gsl_temp_freq (gsl_get_config ()->kammer_freq, midi_note - gsl_get_config ()->midi_kammer_note);
  bse_wt_add_chunk (wave, mix_freq, osc_freq, dhandle);
  if (i < wave->n_chunks)
    wave->chunks[i].midi_note = midi_note;
}

void
bse_wt_remove_chunk (BseWtWave *wave,
		     guint      nth_chunk)
{
  g_return_if_fail (wave != NULL);
  g_return_if_fail (nth_chunk < wave->n_chunks);
  
  wave->n_chunks--;
  gsl_data_handle_unref (wave->chunks[nth_chunk].dhandle);
  g_free (wave->chunks[nth_chunk].dump_name);
  g_free (wave->chunks[nth_chunk].dump_index);
  if (nth_chunk < wave->n_chunks)
    wave->chunks[nth_chunk] = wave->chunks[wave->n_chunks];
}

void
bse_wt_free_wave (BseWtWave *wave)
{
  g_return_if_fail (wave != NULL);
  
  while (wave->n_chunks)
    bse_wt_remove_chunk (wave, wave->n_chunks - 1);
  g_free (wave->chunks);
  g_free (wave->wave_name);
  g_free (wave);
}

#define	debug_print	g_printerr

gfloat*
gsl_data_make_fade_ramp (GslDataHandle *handle,
			 GslLong        min_pos, /* *= 0.0 + delta */
			 GslLong        max_pos, /* *= 1.0 - delta */
			 GslLong       *length_p)
{
  GslDataPeekBuffer peekbuf = { +1, 0, };
  gfloat ramp, rdelta, *values;
  GslLong l, i;
  
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (GSL_DATA_HANDLE_OPENED (handle), NULL);
  g_return_val_if_fail (min_pos >= 0 && max_pos >= 0, NULL);
  g_return_val_if_fail (min_pos < gsl_data_handle_n_values (handle), NULL);
  g_return_val_if_fail (max_pos < gsl_data_handle_n_values (handle), NULL);
  
  if (min_pos > max_pos)
    {
      l = min_pos;
      min_pos = max_pos;
      max_pos = l;
      l = max_pos - min_pos;
      rdelta = -1. / (gfloat) (l + 2);
      ramp = 1.0 + rdelta;
    }
  else
    {
      l = max_pos - min_pos;
      rdelta = +1. / (gfloat) (l + 2);
      ramp = rdelta;
    }
  
  l += 1;
  values = g_new (gfloat, l);
  for (i = 0; i < l; i++)
    {
      values[i] = gsl_data_handle_peek_value (handle, min_pos + i, &peekbuf) * ramp;
      ramp += rdelta;
    }
  
  if (length_p)
    *length_p = l;
  
  return values;
}

GslDataHandle*
gsl_data_level_clip_sample (GslDataHandle      *dhandle,
			    GslLevelClip       *conf,
			    GslLevelClipStatus *status_p)
{
  GslDataHandle *clip_handle, *fade_handle;
  GslLevelClipStatus dummy;
  GslLong head, tail, last_value;
  
  g_return_val_if_fail (dhandle != NULL, NULL);
  g_return_val_if_fail (conf != NULL, NULL);
  if (!status_p)
    status_p = &dummy;
  
  if (gsl_data_handle_open (dhandle) != 0)
    {
      *status_p = GSL_LEVEL_CLIP_IO_ERROR;
      return NULL;
    }
  last_value = gsl_data_handle_n_values (dhandle);
  if (last_value < 1)
    {
      *status_p = GSL_LEVEL_CLIP_IO_ERROR;
      gsl_data_handle_close (dhandle);
      return NULL;
    }
  last_value -= 1;
  
  /* signal range detection */
  head = gsl_data_find_sample (dhandle, +conf->threshold, -conf->threshold, 0, +1);
  if (head < 0)
    {
      gsl_data_handle_close (dhandle);
      *status_p = GSL_LEVEL_CLIP_ALL;
      return NULL;
    }
  tail = gsl_data_find_sample (dhandle, +conf->threshold, -conf->threshold,  -1, -1);
  g_assert (tail >= 0);
  
  /* verify silence detection */
  if (last_value - tail < conf->tail_detect)
    {
      debug_print ("  tail above threshold, # samples below: %lu", last_value - tail);
      gsl_data_handle_close (dhandle);
      *status_p = GSL_LEVEL_CLIP_FAILED_TAIL_DETECT;
      return NULL;
    }
  if (head < conf->head_detect)
    {
      debug_print ("  head above threshold, # samples below: %lu", head);
      gsl_data_handle_close (dhandle);
      *status_p = GSL_LEVEL_CLIP_FAILED_HEAD_DETECT;
      return NULL;
    }
  debug_print ("  Silence detected: HeadPos: %6ld TailPos: %6ld (%lu)\n", head, tail, last_value - tail);
  
  /* tail clipping precaution */
  if (last_value - tail < conf->min_tail)
    {
      debug_print ("  Readjusting Tail: HeadPos: %6ld TailPos: %6ld (%lu < %u)\n",
		   head, last_value,
		   last_value - tail, conf->min_tail);
      tail = last_value;
    }
  
  /* padding */
  if (conf->tail_pad)
    {
      tail += conf->tail_pad;
      tail = MIN (last_value, tail);
      debug_print ("  Padding Tail:     HeadPos: %6ld TailPos: %6ld (%lu)\n", head, tail, last_value - tail);
    }
  
  /* clipping */
  if (head == 0 && last_value == tail)
    {
      gsl_data_handle_close (dhandle);
      *status_p = GSL_LEVEL_UNCLIPPED;
      return NULL;
    }
  clip_handle = gsl_data_handle_new_crop (dhandle, head, last_value - tail);
  gsl_data_handle_open (clip_handle);
  gsl_data_handle_unref (clip_handle);
  sfi_debug (PRG, "cropping: %lu %lu (%lu -> %lu)\n", head, last_value - tail,
             gsl_data_handle_n_values (dhandle), gsl_data_handle_n_values (clip_handle));
  if (last_value != tail)
    *status_p = head ? GSL_LEVEL_CLIPPED_HEAD_TAIL : GSL_LEVEL_CLIPPED_TAIL;
  else
    *status_p = GSL_LEVEL_CLIPPED_HEAD;
  
  /* fading */
  if (conf->head_fade && head)
    {
      GslLong l;
      gfloat *hvalues = gsl_data_make_fade_ramp (dhandle, MAX (head - 1 - conf->head_fade, 0), head - 1, &l);
      guint j;
      
      /* strip initial 0s */
      for (j = 0; j < l; j++)
	if (fabs (hvalues[j]) >= gsl_data_handle_bit_depth (dhandle))
	  break;
      if (j > 0)
	{
	  l -= j;
	  g_memmove (hvalues, hvalues + j, l * sizeof (hvalues[0]));
	}
      
      fade_handle = gsl_data_handle_new_insert (clip_handle, gsl_data_handle_bit_depth (clip_handle), 0, l, hvalues, g_free);
      gsl_data_handle_open (fade_handle);
      gsl_data_handle_unref (fade_handle);
      debug_print ("  Fading Head:      Ramp:    %6ld Length:  %6ld\n", l, gsl_data_handle_n_values (fade_handle));
    }
  else
    {
      fade_handle = clip_handle;
      gsl_data_handle_open (fade_handle);
    }
  
  /* nuke handles */
  gsl_data_handle_ref (fade_handle);
  gsl_data_handle_close (fade_handle);
  gsl_data_handle_close (clip_handle);
  gsl_data_handle_close (dhandle);
  
  return fade_handle;
}

GslErrorType
bse_wt_chunks_dump_wav (BseWtWave   *wave,
			const gchar *base_dir,
			const gchar *prefix)
{
  guint serial = 0;
  guint i, l;
  gchar sep[2];
  
  g_return_val_if_fail (wave != NULL, GSL_ERROR_INTERNAL);
  g_return_val_if_fail (base_dir != NULL, GSL_ERROR_INTERNAL);
  g_return_val_if_fail (base_dir[0] != 0, GSL_ERROR_INTERNAL);
  g_return_val_if_fail (prefix != NULL, GSL_ERROR_INTERNAL);
  
  l = strlen (base_dir);
  sep[0] = base_dir[l - 1] == G_DIR_SEPARATOR ? 0 : G_DIR_SEPARATOR;
  sep[1] = 0;
  for (i = 0; i < wave->n_chunks; i++)
    {
      BseWtChunk *chunk = wave->chunks + i;
      gchar *name, *string;
      gint fd, err;
      
      if (chunk->midi_note)
	string = g_strdup_printf ("%s-s%03u-m%03u-chunk.wav", prefix, ++serial, chunk->midi_note);
      else
	string = g_strdup_printf ("%s-s%03u-f%.3f-chunk.wav", prefix, ++serial, chunk->osc_freq);
      name = g_strdup_printf ("%s%s%s", base_dir, sep, string);
      
      debug_print ("SAVING: %s\n", name);
      fd = open (name, O_CREAT | O_TRUNC | O_WRONLY, 0666);
      if (fd < 0)
	{
	  debug_print ("  open(\"%s\") failed: %s\n", name, g_strerror (errno));
	  goto abort;
	}
      err = gsl_data_handle_open (chunk->dhandle);
      if (err)
	{
	  debug_print ("  opening chunk %f failed: %s\n", chunk->osc_freq, g_strerror (err));
	  goto abort;
	}
      if (0)	// FIXME
	err = gsl_data_handle_dump (chunk->dhandle, fd, GSL_WAVE_FORMAT_SIGNED_16, G_LITTLE_ENDIAN);
      else
	err = gsl_data_handle_dump_wav (chunk->dhandle, fd,
					gsl_data_handle_bit_depth (chunk->dhandle) <= 8 ? 8 : 16,
					wave->n_channels, chunk->mix_freq);
      if (err)
	{
	  debug_print ("  write(\"%s\") failed: %s\n", name, g_strerror (err));
	  goto abort;
	}
      close (fd);
      gsl_data_handle_close (chunk->dhandle);
      g_free (chunk->dump_name);
      g_free (chunk->dump_index);
      chunk->dump_name = string;
      chunk->dump_index = NULL;
      g_free (name);
      continue;
      
    abort:
      g_free (string);
      g_free (name);
      return GSL_ERROR_OPEN_FAILED;
    }
  return GSL_ERROR_NONE;
}

GslErrorType
bse_wt_dump_bsewave_header (BseWtWave   *wave,
			    const gchar *file_name)
{
  SfiWStore *wstore;
  gint fd, i;
  gchar *str;
  
  g_return_val_if_fail (wave != NULL, GSL_ERROR_INTERNAL);
  g_return_val_if_fail (file_name != NULL, GSL_ERROR_INTERNAL);
  
  fd = open (file_name, O_CREAT | O_TRUNC | O_WRONLY, 0666);
  if (fd < 0)
    {
      debug_print ("  open(\"%s\") failed: %s\n", file_name, g_strerror (errno));
      return GSL_ERROR_OPEN_FAILED;
    }
  
  errno = 0;
  wstore = sfi_wstore_new ();
  sfi_wstore_puts (wstore, "#BseWave\n\n");
  sfi_wstore_puts (wstore, "wave {\n");
  str = g_strescape (wave->wave_name, NULL);
  sfi_wstore_printf (wstore, "  name = \"%s\"\n", str);
  sfi_wstore_printf (wstore, "  byte_order = %s\n", gsl_byte_order_to_string (BYTE_ORDER));
  g_free (str);
  for (i = 0; i < wave->n_chunks; i++)
    {
      BseWtChunk *chunk = wave->chunks + i;
      
      sfi_wstore_puts (wstore, "  chunk {\n");
      if (chunk->midi_note)
	sfi_wstore_printf (wstore, "    midi_note = %u\n", chunk->midi_note);
      else
	sfi_wstore_printf (wstore, "    osc_freq = %.3f\n", chunk->osc_freq);
      if (chunk->dump_name)
	{
	  str = g_strescape (chunk->dump_name, NULL);
	  sfi_wstore_printf (wstore, "    file = \"%s\"\n", str);
	  g_free (str);
	}
      if (chunk->dump_index)
	{
	  str = g_strescape (chunk->dump_index, NULL);
	  sfi_wstore_printf (wstore, "    index = \"%s\"\n", str);
	  g_free (str);
	}
      if (chunk->loop_type)
	{
	  sfi_wstore_printf (wstore, "    loop_type = %s\n", gsl_wave_loop_type_to_string (chunk->loop_type));
	  sfi_wstore_printf (wstore, "    loop_start = %ld\n", chunk->loop_start);
	  sfi_wstore_printf (wstore, "    loop_end = %ld\n", chunk->loop_end);
	}
      if (0)
	{
	  /* raw sample stuff... */
	  sfi_wstore_printf (wstore, "    mix_freq = %.3f\n", chunk->mix_freq);
	}
      sfi_wstore_puts (wstore, "  }\n");
    }
  sfi_wstore_puts (wstore, "}\n");
  
  sfi_wstore_flush_fd (wstore, fd);
  sfi_wstore_destroy (wstore);
  close (fd);
  return GSL_ERROR_NONE;
}

GslErrorType
bse_wt_dump_bsewave_wav (BseWtWave   *wave,
			 const gchar *base_dir,
			 const gchar *bsewave,		/* relative */
			 const gchar *chunk_prefix)	/* relative */
{
  GslErrorType error;
  gchar *string, sep[2];
  guint l;
  
  g_return_val_if_fail (wave != NULL, GSL_ERROR_INTERNAL);
  g_return_val_if_fail (base_dir != NULL, GSL_ERROR_INTERNAL);
  g_return_val_if_fail (base_dir[0] != 0, GSL_ERROR_INTERNAL);
  g_return_val_if_fail (bsewave != NULL, GSL_ERROR_INTERNAL);
  g_return_val_if_fail (chunk_prefix != NULL, GSL_ERROR_INTERNAL);
  g_return_val_if_fail (strchr (bsewave, G_DIR_SEPARATOR) == NULL, GSL_ERROR_INTERNAL);
  /* g_return_val_if_fail (g_path_is_absolute (chunk_prefix) == FALSE, GSL_ERROR_INTERNAL); */
  
  /* first thing to do */
  error = bse_wt_chunks_dump_wav (wave, base_dir, chunk_prefix);
  
  l = strlen (base_dir);
  sep[0] = base_dir[l - 1] == G_DIR_SEPARATOR ? 0 : G_DIR_SEPARATOR;
  sep[1] = 0;
  string = g_strconcat (base_dir, sep, bsewave, NULL);
  
  /* next best thing to do */
  if (!error)
    error = bse_wt_dump_bsewave_header (wave, string);
  
  g_free (string);
  
  return error;
}

typedef struct {
  gchar   *tmpfile;
  gint     fd;
  SfiNum   pos;
} TmpStoreContext;

static void
tmp_store_context_destroy (gpointer data)
{
  TmpStoreContext *tc = data;
  if (tc->fd >= 0)
    close (tc->fd);
  g_free (tc->tmpfile);
  g_free (tc);
}

static gint /* -errno || length */
tmp_store_context_reader (gpointer data,
                          SfiNum   pos,
                          void    *buffer,
                          guint    blength)
{
  TmpStoreContext *tc = data;
  GslLong n;
  
  if (tc->fd < 0)
    {
      tc->fd = open (tc->tmpfile, O_RDONLY);
      if (tc->fd < 0)
        return -errno;
      tc->pos = 0;
    }
  
  /* shouldn't need to seek */
  g_return_val_if_fail (pos == tc->pos, -EIO);
  
  do
    n = read (tc->fd, buffer, blength);
  while (n < 0 && errno == EINTR);
  if (n < 0)            /* single retry */
    {
      do
        n = read (tc->fd, buffer, blength);
      while (n < 0 && errno == EINTR);
      if (n < 0)        /* bail out */
        return -errno;
    }
  tc->pos += n;
  return n;
}

typedef struct {
  gchar           *oggfile;
  gint             fd;
  SfiNum           pos;
  GslVorbisCutter *cutter;
} OggStoreContext;

static void
ogg_store_context_destroy (gpointer data)
{
  OggStoreContext *oc = data;
  if (oc->fd >= 0)
    close (oc->fd);
  gsl_vorbis_cutter_destroy (oc->cutter);
  g_free (oc->oggfile);
  g_free (oc);
}

static gint /* -errno || length */
ogg_store_context_reader (gpointer data,
                          SfiNum   pos,
                          void    *buffer,
                          guint    blength)
{
  OggStoreContext *oc = data;
  GslLong n = 0, j = 1;
  
  if (oc->fd < 0)
    {
      oc->fd = open (oc->oggfile, O_RDONLY);
      if (oc->fd < 0)
        return -errno;
      oc->pos = 0;
    }
  
  /* shouldn't need to seek */
  g_return_val_if_fail (pos == oc->pos, -EIO);
  
  while (n == 0 && !gsl_vorbis_cutter_ogg_eos (oc->cutter) && j > 0)
    {
      /* feed the cutter ;) */
      do
        j = read (oc->fd, buffer, blength);
      while (j < 0 && errno == EINTR);
      if (j < 0)        /* single retry */
        {
          do
            j = read (oc->fd, buffer, blength);
          while (j < 0 && errno == EINTR);
          if (j < 0)    /* bail out */
            return -errno;
        }
      gsl_vorbis_cutter_write_ogg (oc->cutter, j, buffer);
      n = gsl_vorbis_cutter_read_ogg (oc->cutter, blength, buffer);
    }
  oc->pos += n;
  return n;
}

GslErrorType
bse_wt_dump_bsewave (BseWtWave   *wave,
		     const gchar *file_name)
{
  SfiWStore *wstore;
  gint fd, i;
  gchar *str;
  
  g_return_val_if_fail (wave != NULL, GSL_ERROR_INTERNAL);
  g_return_val_if_fail (file_name != NULL, GSL_ERROR_INTERNAL);
  
  fd = open (file_name, O_CREAT | O_TRUNC | O_WRONLY, 0666);
  if (fd < 0)
    {
      debug_print ("  open(\"%s\") failed: %s\n", file_name, g_strerror (errno));
      return GSL_ERROR_OPEN_FAILED;
    }
  
  errno = 0;
  wstore = sfi_wstore_new ();
  wstore->comment_start = '#';
  sfi_wstore_puts (wstore, "#BseWave\n\n");
  sfi_wstore_puts (wstore, "wave {\n");
  str = g_strescape (wave->wave_name, NULL);
  sfi_wstore_printf (wstore, "  name = \"%s\"\n", str);
  sfi_wstore_printf (wstore, "  byte_order = %s\n", gsl_byte_order_to_string (BYTE_ORDER));
  g_free (str);
  for (i = 0; i < wave->n_chunks; i++)
    {
      BseWtChunk *chunk = wave->chunks + i;
      
      sfi_wstore_puts (wstore, "  chunk {\n");
      if (chunk->midi_note)
	sfi_wstore_printf (wstore, "    midi_note = %u\n", chunk->midi_note);
      else
	sfi_wstore_printf (wstore, "    osc_freq = %.3f\n", chunk->osc_freq);
      
      if (chunk->oggname)
        {
          sfi_wstore_puts (wstore, "    ogglink = ");
          if (chunk->loop_type)
            {
              OggStoreContext *oc = g_new0 (OggStoreContext, 1);
              oc->fd = -1;
              oc->oggfile = g_strdup (chunk->oggname);
              oc->cutter = gsl_vorbis_cutter_new ();
              gsl_vorbis_cutter_set_cutpoint (oc->cutter, chunk->loop_end + 1, GSL_VORBIS_CUTTER_PACKET_BOUNDARY);
              sfi_wstore_put_binary (wstore, ogg_store_context_reader, oc, ogg_store_context_destroy);
            }
          else
            {
              TmpStoreContext *tc = g_new0 (TmpStoreContext, 1);
              tc->fd = -1;
              tc->tmpfile = g_strdup (chunk->oggname);
              sfi_wstore_put_binary (wstore, tmp_store_context_reader, tc, tmp_store_context_destroy);
            }
          sfi_wstore_puts (wstore, "\n");
        }
      else
        {
          sfi_wstore_puts (wstore, "    rawlink = ");
          gsl_data_handle_dump_wstore (chunk->dhandle, wstore, GSL_WAVE_FORMAT_SIGNED_16, G_LITTLE_ENDIAN);
          sfi_wstore_puts (wstore, "\n");
          sfi_wstore_printf (wstore, "    mix_freq = %.3f\n", chunk->mix_freq);
        }
      
      if (chunk->loop_type)
	{
	  sfi_wstore_printf (wstore, "    loop_type = %s\n", gsl_wave_loop_type_to_string (chunk->loop_type));
	  sfi_wstore_printf (wstore, "    loop_start = %ld\n", chunk->loop_start);
	  sfi_wstore_printf (wstore, "    loop_end = %ld\n", chunk->loop_end);
	}
      sfi_wstore_puts (wstore, "  }\n");
    }
  sfi_wstore_puts (wstore, "}\n");
  
  sfi_wstore_flush_fd (wstore, fd);
  sfi_wstore_destroy (wstore);
  close (fd);
  return GSL_ERROR_NONE;
}


/* --- main program --- */
#define die g_error
#define warn g_warning

static void
help (void)
{
  g_print ("Usage: bsewavetool [options] samples...\n");
  g_print ("  --name <str>   wave name\n");
  g_print ("  --clip <cfg>   clipping configuration, consisting of ':'-seperated values:\n");
  g_print ("                 minimum signal threshold (0..32767) [16]\n");
  g_print ("                 n_samples of silence to verify at head [0]\n");
  g_print ("                 n_samples of silence to verify at tail [0]\n");
  g_print ("                 n_samples of fade-in before signal starts [16]\n");
  g_print ("                 n_samples of padding after signal ends [16]\n");
  g_print ("                 n_samples of tail silence to detect at minimum to\n");
  g_print ("                           allow tail clipping [0]\n");
  g_print ("  --loop <cfg>   loop finder configuration, consisting of ':'-seperated values:\n");
  g_print ("                 n_samples indicating minimum loop size [4410]\n");
  g_print ("                 n_samples indicating maximum loop size [-1]\n");
  g_print ("                 n_samples compare area size (ahead of loop start) [8820]\n");
  g_print ("  --cnote <fmt>  format to extract chunk's midi note from its file name [b1]\n");
  g_print ("                 #<something> => input is assumed to be <something>\n");
  g_print ("                 n<nth>       => refers to <nth> number found in input\n");
  g_print ("                 b<nth>       => behaves like n<nth> on basename(input)\n");
  g_print ("                 c[*<num>]    => counter (with optional factor)\n");
  g_print ("  --cfreq <fmt>  format to extract chunk frequency from its file name\n");
  g_print ("                 only one of --cnote and --cfreq may be given\n");
  g_print ("  --odir <dir>   output directory [./]\n");
  /*       "********************************************************************************" */
}

int
main (int   argc,
      char *argv[])
{
  const gchar *wave_name = "Test Sample";
  const gchar *raw_format = "signed-16", *raw_byte_order = "little_endian", *raw_mfreq = "44100";
  const gchar *wave_channels = "1", *raw_chunks = "0", *odir = "./";
  const gchar *clip_cfg = NULL, *loop_cfg = NULL, *cnote = NULL, *cfreq = NULL, *cnote_default = "b1";
  gdouble dthreshold = 16, dhsilence = 0, dtsilence = 0, dfade = 16, dtpad = 16, dtmin = 0;
  gdouble dminloop = 4410, dmaxloop = -1, dlooparea = 8820, counter = 36;
  SfiArgument options[] = {
    /* tweaking */
    {   0, "channels",		&wave_channels, TRUE },
    /* reading raw sample data */
    {   0, "raw",		&raw_chunks, FALSE },
    {   0, "format",		&raw_format, TRUE },
    {   0, "byte-order",	&raw_byte_order, TRUE },
    {   0, "mix-freq",		&raw_mfreq, TRUE },
    /* wave name */
    {   0, "name",              &wave_name, TRUE },
    /* clipping */
    {   0, "clip",		&clip_cfg, TRUE },
    /* loop finder */
    {   0, "loop",		&loop_cfg, TRUE },
    /* chunk osc freq */
    {   0, "cnote",		&cnote, TRUE },
    {   0, "cfreq",		&cfreq, TRUE },
    /* output */
    {   0, "odir",		&odir, TRUE },
  };
  GslLevelClip lc_conf;
  GslDataTailLoop tl_conf;
  BseWtWave *wave;
  SfiRing *cnames, *ring;
  gchar *string1, *string2;
  guint i, n_tail_clips = 0, want_ogg = 1;
  
  /* initialization
   */
  g_thread_init (NULL);
  bse_init_intern (&argc, &argv, NULL);
  
  /* parse and check options */
  cnames = sfi_arguments_parse_list (&argc, &argv, G_N_ELEMENTS (options), options);
  if (!cnames ||
      (cnote && cfreq))
    {
      help ();
      return 1;
    }
  if (!cnote && !cfreq)
    cnote = cnote_default;
  
  sfi_arguments_read_all_nums (clip_cfg, &dthreshold,
                               &dhsilence, &dtsilence,
                               &dfade, &dtpad, &dtmin, NULL);
  lc_conf.threshold = ABS (dthreshold);
  lc_conf.threshold = CLAMP (lc_conf.threshold, 0, 32767);
  lc_conf.threshold /= 32768.;
  lc_conf.head_detect = IROUND (dhsilence);
  lc_conf.tail_detect = IROUND (dtsilence);
  lc_conf.head_fade = IROUND (dfade);
  lc_conf.tail_pad = IROUND (dtpad);
  lc_conf.min_tail = IROUND (dtmin);
  g_print ("CLIPPING: threshold=%g hsilence=%u tsilence=%u fade=%u pad=%u min_tail=%u\n",
	   lc_conf.threshold, lc_conf.head_detect, lc_conf.tail_detect,
	   lc_conf.head_fade, lc_conf.tail_pad, lc_conf.min_tail);
  
  sfi_arguments_read_all_nums (loop_cfg, &dminloop, &dmaxloop, &dlooparea, NULL);
  tl_conf.min_loop = IROUND (dminloop);
  tl_conf.max_loop = IROUND (dmaxloop);
  tl_conf.pre_loop_compare = IROUND (dlooparea);
  tl_conf.cmp_strategy = GSL_DATA_TAIL_LOOP_CMP_LEAST_SQUARE;
  g_print ("LOOPING:  min_loop=%ld max_loop=%ld loop_area=%ld\n",
	   tl_conf.min_loop, tl_conf.max_loop, tl_conf.pre_loop_compare);
  
  /* create wave-tool wave and load in chunks
   */
  wave = bse_wt_new_wave (wave_name, atoi (wave_channels));
  for (ring = cnames; ring; ring = sfi_ring_walk (ring, cnames))
    {
      gchar *chunk_file = ring->data;
      guint osc_note = cnote ? sfi_arguments_extract_num (chunk_file, cnote, &counter, 0) : 0;
      gdouble osc_freq = cfreq ? sfi_arguments_extract_num (chunk_file, cfreq, &counter, 0) : 0;
      gdouble mix_freq = atof (raw_mfreq);
      GslWaveFileInfo *fi = NULL;
      GslDataHandle *dhandle = NULL;
      GslErrorType error;
      if (osc_note)
        osc_freq = gsl_temp_freq (gsl_get_config ()->kammer_freq, osc_note - gsl_get_config ()->midi_kammer_note);
      
      if (raw_chunks[0] == '0')
	fi = gsl_wave_file_info_load (chunk_file, &error);
      if (fi)
	{
	  GslWaveDsc *wdsc = gsl_wave_dsc_load (fi, 0, &error);
          
	  if (wdsc)
	    {
	      dhandle = gsl_wave_handle_create (wdsc, 0, &error);
	      if (!dhandle)
		warn ("failed to load wave chunk from file \"%s\": %s (loader: %s)",
		      chunk_file, gsl_strerror (error),
		      gsl_wave_file_info_loader (fi));
	      gsl_wave_dsc_free (wdsc);
	    }
	  else
	    die ("failed to load wave description from file \"%s\": %s (loader: %s)",
		 chunk_file, gsl_strerror (error),
		 gsl_wave_file_info_loader (fi));
	  gsl_wave_file_info_unref (fi);
	}
      if (!dhandle)
	{
	  if (raw_chunks[0] == '0')
	    die ("no loader found for file \"%s\": %s",
		 chunk_file, gsl_strerror (error));
	  dhandle = gsl_wave_handle_new (chunk_file, wave->n_channels,
					 gsl_wave_format_from_string (raw_format),
					 gsl_byte_order_from_string (raw_byte_order),
					 mix_freq, osc_freq, 0, -1);
	  error = GSL_ERROR_OPEN_FAILED;
	  if (!dhandle)
	    die ("failed to open file \"%s\" as wave: %s",
		 chunk_file, gsl_strerror (error));
	}
      g_assert (dhandle != NULL);
      
      if (osc_note)
	bse_wt_add_chunk_midi (wave, mix_freq, osc_note, dhandle);
      else
	bse_wt_add_chunk (wave, mix_freq, osc_freq, dhandle);
      gsl_data_handle_unref (dhandle);
    }
  
  /* level clipping */
  for (i = clip_cfg ? 0 : wave->n_chunks; i < wave->n_chunks; i++)
    {
      GslDataHandle *dhandle;
      GslLevelClipStatus lstatus;
      
    continue_after_deletion:
      g_printerr ("LEVELCLIP: chunk %f/%f\n", wave->chunks[i].osc_freq, wave->chunks[i].mix_freq);
      dhandle = gsl_data_level_clip_sample (wave->chunks[i].dhandle, &lc_conf, &lstatus);
      if (dhandle)
	{
	  gsl_data_handle_unref (wave->chunks[i].dhandle);
	  wave->chunks[i].dhandle = dhandle;
	}
      switch (lstatus)
	{
	case GSL_LEVEL_CLIP_FAILED_HEAD_DETECT:
	case GSL_LEVEL_CLIP_FAILED_TAIL_DETECT:
	case GSL_LEVEL_CLIP_IO_ERROR:
	  die ("failed to level clip chunk %f/%f: %s",
	       wave->chunks[i].osc_freq, wave->chunks[i].mix_freq,
	       lstatus == GSL_LEVEL_CLIP_IO_ERROR ? "IO Error" : "Head/tail detection failed");
	  break;
	case GSL_LEVEL_CLIP_ALL:
	  g_printerr ("deleting entirely silent chunk %f/%f",
		      wave->chunks[i].osc_freq, wave->chunks[i].mix_freq);
	  bse_wt_remove_chunk (wave, i);
	  goto continue_after_deletion;
	case GSL_LEVEL_CLIPPED_TAIL:
	case GSL_LEVEL_CLIPPED_HEAD_TAIL:
	  n_tail_clips++;
	  break;
	default:
	  break;
	}
    }
  
#define ENCODER_BUFFER  (16 * 1024)
  
  want_ogg = 1;
  
  /* ogg encoder */
  for (i = want_ogg ? 0 : G_MAXINT; i < wave->n_chunks; i++)
    {
      BseWtChunk *chunk = wave->chunks + i;
      GslVorbisEncoder *enc = gsl_vorbis_encoder_new ();
      GslDataHandle *dhandle = chunk->dhandle;
      GslErrorType error = gsl_data_handle_open (chunk->dhandle);
      GslLong r, l, j, n = 0;
      gint ofd;
      if (error)
        g_error ("chunk %f/%f: (x1) failed to open file: %s\n", chunk->osc_freq, chunk->mix_freq, gsl_strerror (error));
      gsl_vorbis_encoder_set_quality (enc, 1.5);
      gsl_vorbis_encoder_set_bitrate (enc, 64000);
      gsl_vorbis_encoder_set_n_channels (enc, 1);
      gsl_vorbis_encoder_set_sample_freq (enc, chunk->mix_freq);
      error = gsl_vorbis_encoder_setup_stream (enc, (rand () << 16) ^ rand ());
      if (error)
        g_error ("chunk %f/%f: failed to encode: %s", chunk->osc_freq, chunk->mix_freq, gsl_strerror (error));
      chunk->oggname = g_strdup_printf ("/tmp/bsewave-%d-%d.XXXXXX", i + 101, getpid ());
      ofd = mkstemp (chunk->oggname);
      if (ofd < 0)
        g_error ("chunk %f/%f: failed to open tmp file: %s", chunk->osc_freq, chunk->mix_freq, g_strerror (errno));
      g_printerr ("ENCODING: chunk %f/%f\n", chunk->osc_freq, chunk->mix_freq);
      l = gsl_data_handle_length (dhandle);
      while (n < l)
        {
          gfloat buffer[ENCODER_BUFFER];
          r = gsl_data_handle_read (dhandle, n, ENCODER_BUFFER, buffer);
          if (r > 0)
            {
              gpointer buf = buffer;
              n += r;
              gsl_vorbis_encoder_write_pcm (enc, r, buffer);
              r = gsl_vorbis_encoder_read_ogg (enc, ENCODER_BUFFER, buf);
              while (r > 0)
                {
                  do
                    j = write (ofd, buf, r);
                  while (j < 0 && errno == EINTR);
                  if (j < 0)
                    g_error ("chunk %f/%f: failed to write to tmp file: %s", chunk->osc_freq, chunk->mix_freq, g_strerror (errno));
                  r = gsl_vorbis_encoder_read_ogg (enc, ENCODER_BUFFER, buf);
                }
            }
          g_printerr ("processed %0.1f%%       \r", n * 100.0 / (gfloat) l);
        }
      gsl_vorbis_encoder_pcm_done (enc);
      while (!gsl_vorbis_encoder_ogg_eos (enc))
        {
          guint8 buf[ENCODER_BUFFER];
          r = gsl_vorbis_encoder_read_ogg (enc, ENCODER_BUFFER, buf);
          if (r > 0)
            {
              do
                j = write (ofd, buf, r);
              while (j < 0 && errno == EINTR);
              if (j < 0)
                g_error ("chunk %f/%f: failed to write to tmp file: %s", chunk->osc_freq, chunk->mix_freq, g_strerror (errno));
            }
        }
      if (close (ofd) < 0)
        g_error ("chunk %f/%f: failed to write to tmp file: %s", chunk->osc_freq, chunk->mix_freq, g_strerror (errno));
      g_printerr ("\n");
      gsl_vorbis_encoder_destroy (enc);
      gsl_data_handle_close (chunk->dhandle);
    }
  
  /* switch to ogg data handles */
  for (i = want_ogg ? 0 : G_MAXINT; i < wave->n_chunks; i++)
    {
      BseWtChunk *chunk = wave->chunks + i;
      GslDataHandle *dhandle;
      gsl_data_handle_unref (chunk->dhandle);
      dhandle = gsl_data_handle_new_ogg_vorbis_muxed (chunk->oggname, 0, chunk->osc_freq);
      if (!dhandle)
        g_error ("chunk %f/%f: failed to reopen tmp file (%s)", chunk->osc_freq, chunk->mix_freq, chunk->oggname);
      chunk->dhandle = dhandle;
    }
  
  /* tail loop finder */
  i = wave->n_chunks;
  if (loop_cfg)
    {
      if (n_tail_clips)
	g_printerr ("SKIPPING TAIL LOOP FINDER: due to %u chunks being tail clipped\n", n_tail_clips);
      else
	i = 0;
    }
  for (; i < wave->n_chunks; i++)
    {
      BseWtChunk *chunk = wave->chunks + i;
      GslLong lstart = 0, lend = 0;
      gdouble score;
      GslErrorType err = gsl_data_handle_open (chunk->dhandle);
      
      if (err)
	{
	  g_printerr ("SKIPPING TAIL LOOP: chunk %f/%f: failed to open file: %s\n", chunk->osc_freq, chunk->mix_freq,
		      gsl_strerror (err));
	  continue;
	}
      if ((tl_conf.max_loop < 0 && tl_conf.pre_loop_compare + tl_conf.min_loop + 10 >= gsl_data_handle_n_values (chunk->dhandle)) ||
	  (tl_conf.max_loop > 0 && tl_conf.max_loop + tl_conf.pre_loop_compare >= gsl_data_handle_n_values (chunk->dhandle)))
	{
	  g_printerr ("SKIPPING TAIL LOOP: chunk %f/%f: file too short\n", chunk->osc_freq, chunk->mix_freq);
	  gsl_data_handle_close (chunk->dhandle);
	  continue;
	}
      if (1)
        {
          g_printerr ("FIND TAIL LOOP: chunk %f/%f\n", chunk->osc_freq, chunk->mix_freq);
          score = gsl_data_find_loop0 (chunk->dhandle, &tl_conf, &lstart, &lend);
          chunk->loop_type = GSL_WAVE_LOOP_JUMP;
          chunk->loop_start = lstart;
          chunk->loop_end = lend;
          g_printerr ("boundaries %ld .. %ld (score_norm = %f, score = %f, n_values = %ld)\n",
                      lstart, lend,
                      score / (gdouble) tl_conf.pre_loop_compare,
                      score,
                      gsl_data_handle_n_values (chunk->dhandle));
        }
      else
        {
          GslDataLoopConfig cfg;
          g_printerr ("SEEKING LOOP: chunk %u (%f/%f)\n", chunk->midi_note, chunk->osc_freq, chunk->mix_freq);
          cfg.block_start = chunk->mix_freq;    /* skip first second */
          cfg.block_length = -1;                /* to end */
          cfg.analysis_points = 1;
          cfg.repetitions = 2;
          cfg.min_loop = MAX (chunk->mix_freq / 10, 8820);
          if (gsl_data_find_loop5 (chunk->dhandle, &cfg, NULL, gsl_progress_printerr))
            {
              chunk->loop_type = GSL_WAVE_LOOP_JUMP;
              chunk->loop_start = cfg.loop_start;
              chunk->loop_end = cfg.loop_start + cfg.loop_length;
              if(0)   g_printerr ("  LOOP: %6lu - %6lu [%6lu] (%f) (block: %6lu - %6lu [%6lu]) \n",
                                  chunk->loop_start, chunk->loop_end, chunk->loop_end - chunk->loop_start,
                                  cfg.score,
                                  cfg.block_start, cfg.block_start + cfg.block_length, cfg.block_length);
            }
          else
            g_printerr ("  NONE\n");
        }
      gsl_data_handle_close (chunk->dhandle);
    }
  
  
  /* debugging dump
   */
  /* dump contents */
  string1 = g_strcanon (g_strdup (wave_name),
			G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "-.@()",
			'_');
  string2 = g_strconcat (odir, G_DIR_SEPARATOR_S, string1, ".bsewave", NULL);
  // bse_wt_dump_bsewave_wav (wave, odir, string2, string1);
  bse_wt_dump_bsewave (wave, string2);
  g_free (string1);
  g_free (string2);
  
  
  /* cleanup */
  g_printerr ("Cleaning up temporary files...\n");
  for (i = 0; i < wave->n_chunks; i++)
    if (wave->chunks[i].oggname)
      unlink (wave->chunks[i].oggname);
  bse_wt_free_wave (wave);
  
  return 0;
}
