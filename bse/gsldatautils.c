/* GSL - Generic Sound Layer
 * Copyright (C) 2001 Stefan Westerfeld and Tim Janik
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
#include <errno.h>
#include <unistd.h>


#define	BSIZE		GSL_DATA_HANDLE_PEEK_BUFFER	/* FIXME: global buffer size setting */


/* --- functions --- */
gfloat
gsl_data_peek_value_f (GslDataHandle     *dhandle,
		       GslLong            pos,
		       GslDataPeekBuffer *peekbuf)
{
  if (pos < peekbuf->start || pos >= peekbuf->end)
    {
      GslLong inc, k, bsize = MIN (GSL_DATA_HANDLE_PEEK_BUFFER, dhandle->n_values);

      g_return_val_if_fail (pos >= 0 && pos < dhandle->n_values, 0);

      peekbuf->start = peekbuf->dir > 0 ? pos : peekbuf->dir < 0 ? pos - bsize + 1: pos - bsize / 2;
      peekbuf->end = MIN (peekbuf->start + bsize, dhandle->n_values);
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
	      gsl_message_send (G_STRLOC, GSL_ERROR_READ_FAILED, "unable to read from data handle (%p)", dhandle);
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
  g_return_val_if_fail (format >= GSL_WAVE_FORMAT_UNSIGNED_8 && format <= GSL_WAVE_FORMAT_FLOAT, EINVAL);
  g_return_val_if_fail (byte_order == G_LITTLE_ENDIAN || byte_order == G_BIG_ENDIAN, EINVAL);

  l = dhandle->n_values;
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
write_bytes (gint  fd,
	     guint n_bytes,
	     void *bytes)
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
gsl_data_handle_dump_wav (GslDataHandle *dhandle,
			  gint           fd,
			  guint          n_bits,
			  guint          n_channels,
			  guint          sample_freq)
{
  guint data_length, file_length, byte_per_sample, byte_per_second;

  g_return_val_if_fail (dhandle != NULL, EINVAL);
  g_return_val_if_fail (GSL_DATA_HANDLE_OPENED (dhandle), EINVAL);
  g_return_val_if_fail (fd >= 0, EINVAL);
  g_return_val_if_fail (n_bits == 16 || n_bits == 8, EINVAL);
  g_return_val_if_fail (n_channels >= 1, EINVAL);

  data_length = dhandle->n_values * (n_bits == 16 ? 2 : 1);
  file_length = data_length;
  file_length += 4 + 4;				/* 'RIFF' header */
  file_length += 4 + 4 + 2 + 2 + 4 + 4 + 2 + 2;	/* 'fmt ' header */
  file_length += 4 + 4;				/* 'data' header */
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
  write_uint32_le (fd, data_length);

  if (errno)
    return errno;

  return gsl_data_handle_dump (dhandle, fd,
			       n_bits == 16 ? GSL_WAVE_FORMAT_SIGNED_16 : GSL_WAVE_FORMAT_UNSIGNED_8,
			       G_LITTLE_ENDIAN);
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
  for (; k < handle->n_values; k++)
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

  if (start_offset >= dhandle->n_values || gsl_data_handle_open (dhandle) != 0)
    return -1;

  if (start_offset < 0)
    start_offset = dhandle->n_values - 1;

  peekbuf.dir = direction;
  if (min_value <= max_value)
    for (i = start_offset; i < dhandle->n_values && i >= 0; i += direction)
      {
	gfloat val = gsl_data_handle_peek_value (dhandle, i, &peekbuf);

	/* g_print ("(%lu): %f <= %f <= %f\n", i, min_value, val, max_value); */
	if (val >= min_value && val <= max_value)
	  break;
      }
  else
    for (i = start_offset; i < dhandle->n_values && i >= 0; i += direction)
      {
	gfloat val = gsl_data_handle_peek_value (dhandle, i, &peekbuf);

	/* g_print ("(%lu): %f > %f || %f < %f\n", i, val, max_value, val, min_value); */
	if (val > min_value || val < max_value)
	  break;
      }

  gsl_data_handle_close (dhandle);

  return i >= dhandle->n_values ? -1: i;
}

static inline gdouble
tailmatch_score_loop (GslDataHandle *shandle,
		      GslDataHandle *dhandle,
		      GslLong	   start,
		      gdouble	   worst_score)
{
  GslLong l, length = MIN (shandle->n_values, dhandle->n_values);
  gfloat v1[GSL_DATA_HANDLE_PEEK_BUFFER], v2[GSL_DATA_HANDLE_PEEK_BUFFER];
  gdouble score = 0;

  g_assert (start < length);

  for (l = start; l < length; )
    {
      GslLong b = MIN (GSL_DATA_HANDLE_PEEK_BUFFER, length - l);

      b = gsl_data_handle_read (shandle, l, b, v1);
      b = gsl_data_handle_read (dhandle, l, b, v2);
      g_assert (b >= 1);
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
  length = dhandle->n_values;
  g_return_val_if_fail (lspec != NULL, FALSE);
  g_return_val_if_fail (loop_start_p != NULL, FALSE);
  g_return_val_if_fail (loop_end_p != NULL, FALSE);
  g_return_val_if_fail (lspec->head_skip >= 0, FALSE);
  g_return_val_if_fail (lspec->tail_cut >= 0, FALSE);
  g_return_val_if_fail (lspec->min_loop >= 1, FALSE);
  g_return_val_if_fail (lspec->max_loop >= lspec->min_loop, FALSE);
  g_return_val_if_fail (lspec->head_skip < length, FALSE);
  offset = lspec->head_skip;
  length -= offset;
  g_return_val_if_fail (lspec->tail_cut >= lspec->max_loop, FALSE);
  g_return_val_if_fail (lspec->tail_cut < length, FALSE);
  length -= lspec->tail_cut;
  g_return_val_if_fail (lspec->max_loop <= length, FALSE);
  
  dcache = gsl_data_cache_new (dhandle, 1);
  shandle = gsl_data_handle_new_dcached (dcache);
  gsl_data_cache_unref (dcache);
  gsl_data_handle_open (shandle);
  
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
  gsl_data_handle_unref (shandle);

  g_print ("\nhalted: %f: [0x%lx..0x%lx] (%lu)\n", best_score, start, end, end - start + 1);
  
  *loop_start_p = start;
  *loop_end_p = end;

  return TRUE;
}
