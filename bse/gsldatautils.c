/* GSL - Generic Sound Layer
 * Copyright (C) 2001-2004 Stefan Westerfeld and Tim Janik
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
#include "gsldatautils.h"
#include "gsldatacache.h"
#include <string.h>
#include <unistd.h>
#include <errno.h>


#define	BSIZE		GSL_DATA_HANDLE_PEEK_BUFFER	/* FIXME: global buffer size setting */


/* --- functions --- */
gfloat
gsl_data_peek_value_f (GslDataHandle     *dhandle,
		       GslLong            pos,
		       GslDataPeekBuffer *peekbuf)
{
  if (pos < peekbuf->start || pos >= peekbuf->end)
    {
      GslLong dhandle_length = dhandle->setup.n_values;
      GslLong inc, k, bsize = MIN (GSL_DATA_HANDLE_PEEK_BUFFER, dhandle_length);

      g_return_val_if_fail (pos >= 0 && pos < dhandle_length, 0);

      peekbuf->start = peekbuf->dir > 0 ? pos : peekbuf->dir < 0 ? pos - bsize + 1: pos - bsize / 2;
      peekbuf->end = MIN (peekbuf->start + bsize, dhandle_length);
      peekbuf->start = MAX (peekbuf->start, 0);
      for (k = peekbuf->start; k < peekbuf->end; k += inc)
	{
	  guint n_retries = 5;  /* FIXME: need global retry strategy */

	  do
	    inc = gsl_data_handle_read (dhandle, k, peekbuf->end - k, peekbuf->data + k - peekbuf->start);
	  while (inc < 1 && n_retries-- && GSL_DATA_HANDLE_OPENED (dhandle));
	  if (inc < 1)
	    {   /* pathologic */
	      peekbuf->data[k - peekbuf->start] = 0;
	      inc = 1;
	      sfi_diag ("%s: failed to read from data handle (%p)", G_STRLOC, dhandle);
	    }
	}
    }
  return peekbuf->data[pos - peekbuf->start];
}

gint /* errno */
gsl_data_handle_dump (GslDataHandle    *dhandle,
		      gint	        fd,
		      GslWaveFormatType format,
		      guint             byte_order)
{
  GslLong l, offs = 0;

  g_return_val_if_fail (dhandle != NULL, EINVAL);
  g_return_val_if_fail (GSL_DATA_HANDLE_OPENED (dhandle), EINVAL);
  g_return_val_if_fail (fd >= 0, EINVAL);
  g_return_val_if_fail (format > GSL_WAVE_FORMAT_NONE && format < GSL_WAVE_FORMAT_LAST, EINVAL);
  g_return_val_if_fail (!GSL_WAVE_FORMAT_IS_LAW (format), EINVAL);
  g_return_val_if_fail (byte_order == G_LITTLE_ENDIAN || byte_order == G_BIG_ENDIAN, EINVAL);

  l = dhandle->setup.n_values;
  while (l)
    {
      GslLong retry, j, n = MIN (l, GSL_DATA_HANDLE_PEEK_BUFFER);
      gfloat src[GSL_DATA_HANDLE_PEEK_BUFFER];

      retry = GSL_N_IO_RETRIES;
      do
	n = gsl_data_handle_read (dhandle, offs, n, src);
      while (n < 1 && retry--);
      if (retry < 0)
	return EIO;

      l -= n;
      offs += n;

      n = gsl_conv_from_float_clip (format, byte_order, src, src, n);

      do
	j = write (fd, src, n);
      while (j < 0 && errno == EINTR);
      if (j < 0)
	return errno ? errno : EIO;
    }
  return 0;
}

static void
write_bytes (gint        fd,
	     guint       n_bytes,
	     const void *bytes)
{
  gint errold = errno;
  guint j;

  do
    j = write (fd, bytes, n_bytes);
  while (j < 0 && errno == EINTR);

  if (!errno)
    errno = errold;
}

static void
write_uint32_le (gint    fd,
		 guint32 val)
{
  val = GUINT32_TO_LE (val);
  write_bytes (fd, 4, &val);
}

static void
write_uint16_le (gint    fd,
		 guint16 val)
{
  val = GUINT16_TO_LE (val);
  write_bytes (fd, 2, &val);
}

gint /* errno */
gsl_wave_file_dump_header (gint           fd,
			   guint	  n_data_bytes,
			   guint          n_bits,
			   guint          n_channels,
			   guint          sample_freq)
{
  guint byte_per_sample, byte_per_second, file_length;

  g_return_val_if_fail (fd >= 0, EINVAL);
  g_return_val_if_fail (n_data_bytes < 4294967296LLU - 44, EINVAL);
  g_return_val_if_fail (n_bits == 16 || n_bits == 8, EINVAL);
  g_return_val_if_fail (n_channels >= 1, EINVAL);

  file_length = 0; /* 4 + 4; */				/* 'RIFF' header is left out*/
  file_length += 4 + 4 + 4 + 2 + 2 + 4 + 4 + 2 + 2;	/* 'fmt ' header */
  file_length += 4 + 4;					/* 'data' header */
  file_length += n_data_bytes;
  byte_per_sample = (n_bits == 16 ? 2 : 1) * n_channels;
  byte_per_second = byte_per_sample * sample_freq;

  errno = 0;
  write_bytes (fd, 4, "RIFF");		/* main_chunk */
  write_uint32_le (fd, file_length);
  write_bytes (fd, 4, "WAVE");		/* chunk_type */
  write_bytes (fd, 4, "fmt ");		/* sub_chunk */
  write_uint32_le (fd, 16);		/* sub chunk length */
  write_uint16_le (fd, 1);		/* format (1=PCM) */
  write_uint16_le (fd, n_channels);
  write_uint32_le (fd, sample_freq);
  write_uint32_le (fd, byte_per_second);
  write_uint16_le (fd, byte_per_sample);
  write_uint16_le (fd, n_bits);
  write_bytes (fd, 4, "data");		/* data chunk */
  write_uint32_le (fd, n_data_bytes);

  return errno;
}

gint /* errno */
gsl_wave_file_patch_length (gint           fd,
			    guint	   n_data_bytes)
{
  guint file_length;
  glong l;

  g_return_val_if_fail (fd >= 0, EINVAL);
  g_return_val_if_fail (n_data_bytes < 4294967296LLU - 44, EINVAL);

  file_length = 0; /* 4 + 4; */				/* 'RIFF' header is left out*/
  file_length += 4 + 4 + 4 + 2 + 2 + 4 + 4 + 2 + 2;	/* 'fmt ' header */
  file_length += 4 + 4;					/* 'data' header */
  file_length += n_data_bytes;

  errno = 0;

  do
    l = lseek (fd, 4, SEEK_SET);
  while (l < 0 && errno == EINTR);
  if (l != 4 || errno)
    return errno ? errno : EIO;
  write_uint32_le (fd, file_length);
  if (errno)
    return errno;

  do
    l = lseek (fd, 40, SEEK_SET);
  while (l < 0 && errno == EINTR);
  if (l != 40 || errno)
    return errno ? errno : EIO;
  write_uint32_le (fd, n_data_bytes);
  if (errno)
    return errno;

  return errno;
}

gint /* errno */
gsl_wave_file_dump_data (gint           fd,
			 guint		n_bits,
			 guint	        n_values,
			 const gfloat  *values)
{
  guint j;
  guint8 *dest = g_new (guint8, n_values * 2); /* enough for 16bit */
  guint n_bytes = gsl_conv_from_float_clip (n_bits > 8 ? GSL_WAVE_FORMAT_SIGNED_16 : GSL_WAVE_FORMAT_SIGNED_8,
					    G_BYTE_ORDER,
					    values,
					    dest,
					    n_values);
  do
    j = write (fd, dest, n_bytes);
  while (j < 0 && errno == EINTR);
  g_free (dest);
  if (j != n_bytes)
    return errno ? errno : EIO;
  else
    return 0;
}

gint /* errno */
gsl_data_handle_dump_wav (GslDataHandle *dhandle,
			  gint           fd,
			  guint          n_bits,
			  guint          n_channels,
			  guint          sample_freq)
{
  guint data_length;

  g_return_val_if_fail (dhandle != NULL, EINVAL);
  g_return_val_if_fail (GSL_DATA_HANDLE_OPENED (dhandle), EINVAL);
  g_return_val_if_fail (fd >= 0, EINVAL);
  g_return_val_if_fail (n_bits == 16 || n_bits == 8, EINVAL);
  g_return_val_if_fail (n_channels >= 1, EINVAL);

  data_length = dhandle->setup.n_values * (n_bits == 16 ? 2 : 1);

  errno = 0;
  errno = gsl_wave_file_dump_header (fd, data_length, n_bits, n_channels, sample_freq);
  if (errno)
    return errno;

  return gsl_data_handle_dump (dhandle, fd,
			       n_bits > 8 ? GSL_WAVE_FORMAT_SIGNED_16 : GSL_WAVE_FORMAT_UNSIGNED_8,
			       G_LITTLE_ENDIAN);
}

typedef struct {
  GslDataHandle    *dhandle;
  gboolean          opened;
  GslWaveFormatType format;
  guint             byte_order;
  guint             length;
} WStoreContext;

static void
wstore_context_destroy (gpointer data)
{
  WStoreContext *wc = data;
  if (wc->opened)
    gsl_data_handle_close (wc->dhandle);
  gsl_data_handle_unref (wc->dhandle);
  g_free (wc);
}

static gint /* -errno || length */
wstore_context_reader (gpointer data,
		       void    *buffer,
		       guint    blength)
{
  WStoreContext *wc = data;
  GslLong l;

  if (!wc->opened)
    {
      BseErrorType error = gsl_data_handle_open (wc->dhandle);
      if (error)
	return -ENOENT; /* approximation of OPEN_FAILED */
      wc->opened = TRUE;
    }

  blength /= 4;	/* we use buffer for floats */
  if (wc->length >= gsl_data_handle_length (wc->dhandle))
    return 0;	/* done */

  l = gsl_data_handle_read (wc->dhandle, wc->length, blength, buffer);
  if (l < 1)
    {
      /* single retry */
      l = gsl_data_handle_read (wc->dhandle, wc->length, blength, buffer);
      if (l < 1)
	return -EIO;	/* bail out */
    }
  wc->length += l;

  return gsl_conv_from_float_clip (wc->format, wc->byte_order, buffer, buffer, l);
}

void
gsl_data_handle_dump_wstore (GslDataHandle    *dhandle,
			     SfiWStore        *wstore,
			     GslWaveFormatType format,
			     guint             byte_order)
{
  WStoreContext *wc;

  g_return_if_fail (dhandle != NULL);
  g_return_if_fail (wstore);

  wc = g_new0 (WStoreContext, 1);
  wc->dhandle = gsl_data_handle_ref (dhandle);
  wc->opened = FALSE;
  wc->format = format;
  wc->byte_order = byte_order;
  sfi_wstore_put_binary (wstore, wstore_context_reader, wc, wstore_context_destroy);
}

gboolean
gsl_data_detect_signal (GslDataHandle *handle,
			GslLong       *sigstart_p,
			GslLong       *sigend_p)
{
  gfloat level_0, level_1, level_2, level_3, level_4;
  gfloat signal_threshold = 16. * 16. * 16.;	/* noise level threshold */
  GslLong k, xcheck = -1, minsamp = -1, maxsamp = -2;
  GslDataPeekBuffer peek_buffer = { +1 /* incremental direction */, 0, };
  
  g_return_val_if_fail (handle != NULL, FALSE);
  g_return_val_if_fail (GSL_DATA_HANDLE_OPENED (handle), FALSE);
  g_return_val_if_fail (sigstart_p || sigend_p, FALSE);

  /* keep open */
  gsl_data_handle_open (handle);

  /* find fadein/fadeout point */
  k = 0;
  level_4 = gsl_data_handle_peek_value (handle, k, &peek_buffer);
  level_4 *= 32768;
  level_0 = level_1 = level_2 = level_3 = level_4;
  for (; k < handle->setup.n_values; k++)
    {
      gfloat mean, needx, current;

      current = gsl_data_handle_peek_value (handle, k, &peek_buffer) * 32768.;
      if (xcheck < 0 && ABS (current) >= 16)
	xcheck = k;
      mean = (level_0 + level_1 + level_2 + level_3 + level_4) / 5;
      needx = (ABS (level_4 + current - (level_0 + level_1 + level_2 + level_3) / 2) *
	       ABS (level_4 - mean) * ABS (current - mean));
      /* shift */
      level_0 = level_1; level_1 = level_2; level_2 = level_3; level_3 = level_4; level_4 = current;
      /* aprox. noise compare */
      if (ABS (needx) > signal_threshold)
	{
	  if (minsamp < 0)
	    minsamp = k;
	  if (maxsamp < k)
	    maxsamp = k;
	}
      /* if (minsamp >= 0 && xcheck >= 0)
       *   break;
       */
    }
  if (xcheck - minsamp > 0)
    g_printerr("###################");
  g_printerr ("active area %ld .. %ld, signal>16 at: %ld\t diff: %ld\n",minsamp,maxsamp,xcheck, xcheck-minsamp);

  /* release open reference */
  gsl_data_handle_close (handle);

  if (sigstart_p)
    *sigstart_p = minsamp;
  if (sigend_p)
    *sigend_p = MAX (-1, maxsamp);

  return maxsamp >= minsamp;
}

GslLong
gsl_data_find_sample (GslDataHandle *dhandle,
		      gfloat         min_value,
		      gfloat         max_value,
		      GslLong        start_offset,
		      gint           direction)
{
  GslDataPeekBuffer peekbuf = { 0, 0, 0, };
  GslLong i;

  g_return_val_if_fail (dhandle != NULL, -1);
  g_return_val_if_fail (direction == -1 || direction == +1, -1);

  if (gsl_data_handle_open (dhandle) != BSE_ERROR_NONE ||
      start_offset >= dhandle->setup.n_values)
    return -1;

  if (start_offset < 0)
    start_offset = dhandle->setup.n_values - 1;

  peekbuf.dir = direction;
  if (min_value <= max_value)
    for (i = start_offset; i < dhandle->setup.n_values && i >= 0; i += direction)
      {
	gfloat val = gsl_data_handle_peek_value (dhandle, i, &peekbuf);

	/* g_print ("(%lu): %f <= %f <= %f\n", i, min_value, val, max_value); */
	if (val >= min_value && val <= max_value)
	  break;
      }
  else
    for (i = start_offset; i < dhandle->setup.n_values && i >= 0; i += direction)
      {
	gfloat val = gsl_data_handle_peek_value (dhandle, i, &peekbuf);

	/* g_print ("(%lu): %f > %f || %f < %f\n", i, val, max_value, val, min_value); */
	if (val > min_value || val < max_value)
	  break;
      }

  gsl_data_handle_close (dhandle);

  return i >= dhandle->setup.n_values ? -1: i;
}

static inline gdouble
tailmatch_score_loop (GslDataHandle *shandle,
		      GslDataHandle *dhandle,
		      GslLong	   start,
		      gdouble	   worst_score)
{
  GslLong l, length = MIN (shandle->setup.n_values, dhandle->setup.n_values);
  gfloat v1[GSL_DATA_HANDLE_PEEK_BUFFER], v2[GSL_DATA_HANDLE_PEEK_BUFFER];
  gdouble score = 0;

  g_assert (start < length);

  for (l = start; l < length; )
    {
      GslLong b = MIN (GSL_DATA_HANDLE_PEEK_BUFFER, length - l);

      b = gsl_data_handle_read (shandle, l, b, v1);
      b = gsl_data_handle_read (dhandle, l, b, v2);
      g_assert (b >= 1);        // FIXME
      l += b;

      while (b--)
	score += (v1[b] - v2[b]) * (v1[b] - v2[b]);

      /* for performance, prematurely abort */
      if (score > worst_score)
	break;
    }
  return score;
}

gboolean
gsl_data_find_tailmatch (GslDataHandle     *dhandle,
			 const GslLoopSpec *lspec,
			 GslLong           *loop_start_p,
			 GslLong           *loop_end_p)
{
  GslDataHandle *shandle;
  GslDataCache *dcache;
  GslLong length, offset, l, lsize, pcount, start = 0, end = 0;
  gdouble pbound, pval, best_score = GSL_MAXLONG;
  
  g_return_val_if_fail (dhandle != NULL, FALSE);
  g_return_val_if_fail (lspec != NULL, FALSE);
  g_return_val_if_fail (loop_start_p != NULL, FALSE);
  g_return_val_if_fail (loop_end_p != NULL, FALSE);
  g_return_val_if_fail (lspec->head_skip >= 0, FALSE);
  g_return_val_if_fail (lspec->tail_cut >= 0, FALSE);
  g_return_val_if_fail (lspec->min_loop >= 1, FALSE);
  g_return_val_if_fail (lspec->max_loop >= lspec->min_loop, FALSE);
  g_return_val_if_fail (lspec->tail_cut >= lspec->max_loop, FALSE);

  if (gsl_data_handle_open (dhandle) != BSE_ERROR_NONE)
    return FALSE;
  length = dhandle->setup.n_values;
  if (lspec->head_skip < length)
    {
      gsl_data_handle_close (dhandle);
      return FALSE;
    }
  offset = lspec->head_skip;
  length -= offset;
  if (lspec->tail_cut < length)
    {
      gsl_data_handle_close (dhandle);
      return FALSE;
    }
  length -= lspec->tail_cut;
  if (lspec->max_loop <= length)
    {
      gsl_data_handle_close (dhandle);
      return FALSE;
    }
  
  dcache = gsl_data_cache_new (dhandle, 1);
  shandle = gsl_data_handle_new_dcached (dcache);
  gsl_data_cache_unref (dcache);
  gsl_data_handle_open (shandle);
  gsl_data_handle_close (dhandle);
  gsl_data_handle_unref (shandle);
  /* at this point, we just hold one open() count on shandle */
  
  pbound = (lspec->max_loop - lspec->min_loop + 1.);
  pbound *= length / 100.;
  pval = 0;
  pcount = 100;

  for (lsize = lspec->min_loop; lsize <= lspec->max_loop; lsize++)
    {
      for (l = length - lsize; l >= 0; l--)
	{
	  GslDataHandle *lhandle = gsl_data_handle_new_looped (shandle, offset + l, offset + l + lsize - 1);
	  gdouble score;
	  
	  gsl_data_handle_open (lhandle);
	  score = tailmatch_score_loop (shandle, lhandle, offset + l, best_score);
	  gsl_data_handle_close (lhandle);
	  gsl_data_handle_unref (lhandle);
	  
	  if (score < best_score)
	    {
	      start = offset + l;
	      end = offset + l + lsize - 1;
	      g_print ("\nimproved: %f < %f: [0x%lx..0x%lx] (%lu)\n", score, best_score, start, end, lsize);
	      best_score = score;
	    }
	  else
	    break;
	}
      if (!pcount--)
	{
	  pcount = 100;
	  pval = lsize - lspec->min_loop;
	  pbound = (lspec->max_loop - lspec->min_loop + 1.);
	  g_print ("\rprocessed: %f%%                  \r", pval / pbound);
	}
    }
  gsl_data_handle_close (shandle);

  g_print ("\nhalted: %f: [0x%lx..0x%lx] (%lu)\n", best_score, start, end, end - start + 1);
  
  *loop_start_p = start;
  *loop_end_p = end;

  return TRUE;
}

/**
 * gsl_data_find_block
 * @handle:   an open GslDataHandle
 * @n_values: amount of values to look for
 * @values:   values to find
 * @epsilon:  maximum difference upon comparisions
 * @RETURNS:  position of values in data handle or -1
 *
 * Find the position of a block of values within a
 * data handle, where all values compare to the reference
 * values with a delta smaller than epsilon.
 */
GslLong
gsl_data_find_block (GslDataHandle *handle,
		     guint          n_values,
		     const gfloat  *values,
		     gfloat         epsilon)
{
  GslDataPeekBuffer pbuf = { +1 /* random access: 0 */ };
  guint i;

  g_return_val_if_fail (handle != NULL, -1);
  g_return_val_if_fail (GSL_DATA_HANDLE_OPENED (handle), -1);

  if (n_values < 1)
    return -1;
  else
    g_return_val_if_fail (values != NULL, -1);

  for (i = 0; i < handle->setup.n_values; i++)
    {
      guint j;

      if (n_values > handle->setup.n_values - i)
	return -1;

      for (j = 0; j < n_values; j++)
	{
	  if (fabs (values[j] - gsl_data_handle_peek_value (handle, i + j, &pbuf)) >= epsilon)
	    break;
	}
      if (j >= n_values)
	return i;
    }
  return -1;
}

/**
 * gsl_data_make_fade_ramp
 * @dhandle:  valid and opened #GslDataHandle
 * @min_pos:  position within @dhandle
 * @max_pos:  position within @dhandle
 * @length_p: location to store the length of the fade ramp in
 * @RETURNS:  newly allocated float block with fade ramp
 * Create a float value block of abs (@max_pos - @min_pos) values,
 * which contain a fade ramp of values from @dhandle, with @min_pos
 * indicating the minimum of the fade ramp and @max_pos indicating
 * its maximum.
 */
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

/**
 * gsl_data_clip_sample
 * @dhandle:  valid and opened #GslDataHandle
 * @cconfig:  clip configuration
 * @result:   clip result
 * @RETURNS:  error code as stored in @result
 * Clip silence at head and/or tail of a data handle
 * according to a given threshold and optionally produce
 * a fade ramp.
 */
BseErrorType
gsl_data_clip_sample (GslDataHandle     *dhandle,
                      GslDataClipConfig *cconfig,
                      GslDataClipResult *result)
{
  g_return_val_if_fail (result != NULL, BSE_ERROR_INTERNAL);
  memset (result, 0, sizeof (*result));
  result->error = BSE_ERROR_INTERNAL;
  g_return_val_if_fail (dhandle, BSE_ERROR_INTERNAL);
  g_return_val_if_fail (GSL_DATA_HANDLE_OPENED (dhandle), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (cconfig != NULL, BSE_ERROR_INTERNAL);
  gboolean info = cconfig->produce_info != FALSE;

  SfiNum last_value = gsl_data_handle_n_values (dhandle);
  if (last_value < 1)
    {
      if (info)
        sfi_info ("Signal too short");
      result->error = BSE_ERROR_FILE_EMPTY;
      return result->error;
    }
  last_value -= 1;
  
  /* signal range detection */
  SfiNum head = gsl_data_find_sample (dhandle, +cconfig->threshold, -cconfig->threshold, 0, +1);
  if (head < 0)
    {
      if (info)
        sfi_info ("All of signal below threshold");
      result->clipped_to_0length = TRUE;
      result->error = BSE_ERROR_DATA_UNMATCHED;
      return result->error;
    }
  SfiNum tail = gsl_data_find_sample (dhandle, +cconfig->threshold, -cconfig->threshold,  -1, -1);
  g_assert (tail >= 0);
  
  /* verify silence detection */
  if (last_value - tail < cconfig->tail_samples)
    {
      if (info)
        sfi_info ("Signal tail above threshold, # samples below: %llu", last_value - tail);
      result->error = BSE_ERROR_DATA_UNMATCHED;
      return result->error;
    }
  result->tail_detected = TRUE;
  if (head < cconfig->head_samples)
    {
      if (info)
        sfi_info ("Signal head above threshold, # samples below: %llu", head);
      result->error = BSE_ERROR_DATA_UNMATCHED;
      return result->error;
    }
  result->head_detected = TRUE;
  if (info)
    sfi_info ("Silence detected: head_silence=%lld tail_silence=%llu", head, last_value - tail);
  
  /* tail clipping protection */
  if (last_value - tail < cconfig->tail_silence)
    {
      if (info)
        sfi_info ("Tail silence too short for clipping: silence_length=%lld minimum_length=%u", last_value - tail, cconfig->tail_silence);
      tail = last_value;
    }
  
  /* padding */
  if (cconfig->pad_samples)
    {
      SfiNum otail = tail;
      tail += cconfig->pad_samples;
      tail = MIN (last_value, tail);
      if (info && otail != tail)
        sfi_info ("Padding Tail: old_tail=%lld tail=%llu padding=%lld", otail, tail, tail - otail);
    }
  
  /* unclipped handles */
  if (head == 0 && last_value == tail)
    {
      result->dhandle = gsl_data_handle_ref (dhandle);
      result->error = BSE_ERROR_NONE;
      return result->error;
    }

  /* clipping */
  GslDataHandle *clip_handle = gsl_data_handle_new_crop (dhandle, head, last_value - tail);
  gsl_data_handle_open (clip_handle);
  gsl_data_handle_unref (clip_handle);
  if (info)
    sfi_info ("Clipping: start=%llu end=%llu length=%ld (delta=%ld)", head, tail, gsl_data_handle_n_values (clip_handle),
              gsl_data_handle_n_values (clip_handle) - gsl_data_handle_n_values (dhandle));
  result->clipped_head = head > 0;
  result->clipped_tail = last_value != tail;
  
  /* fading */
  GslDataHandle *fade_handle;
  if (cconfig->fade_samples && head)
    {
      GslLong l;
      gfloat *ramp = gsl_data_make_fade_ramp (dhandle, MAX (head - 1 - (gint) cconfig->fade_samples, 0), head - 1, &l);

      /* strip initial ramp silence */
      gint j, bdepth = gsl_data_handle_bit_depth (dhandle);
      gdouble threshold = 1.0 / (((SfiNum) 1) << (bdepth ? bdepth : 16));
      for (j = 0; j < l; j++)
        if (fabs (ramp[j]) >= threshold)
          break;
      if (j > 0) /* shorten ramp by j values which are below threshold */
        {
          l -= j;
          g_memmove (ramp, ramp + j, l * sizeof (ramp[0]));
        }
      
      fade_handle = gsl_data_handle_new_insert (clip_handle, gsl_data_handle_bit_depth (clip_handle), 0, l, ramp, g_free);
      gsl_data_handle_open (fade_handle);
      gsl_data_handle_unref (fade_handle);
      if (info)
        sfi_info ("Adding fade-in ramp: ramp_length=%ld length=%ld", l, gsl_data_handle_n_values (fade_handle));
    }
  else
    {
      fade_handle = clip_handle;
      gsl_data_handle_open (fade_handle);
    }
  
  /* prepare result and cleanup */
  result->dhandle = gsl_data_handle_ref (fade_handle);
  gsl_data_handle_close (fade_handle);
  gsl_data_handle_close (clip_handle);
  result->error = BSE_ERROR_NONE;
  return result->error;
}

/* vim:set ts=8 sts=2 sw=2: */
