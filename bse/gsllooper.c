/* GSL - Generic Sound Layer
 * Copyright (C) 2001 Tim Janik
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
#include "gsllooper.h"

#include "gsldatahandle.h"
#include "gsldatacache.h"

#define BSIZE		(8192)
#define	PERC_TICKS	(100)


/* --- functions --- */
static inline gdouble
score_loop (GslDataHandle *shandle,
	    GslDataHandle *dhandle,
	    GslLong	   start,
	    gdouble	   worst_score)
{
  GslLong l, length = MIN (shandle->n_values, dhandle->n_values);
  gdouble score = 0;

  g_assert (start < length);

  for (l = start; l < length; )
    {
      gfloat v1[BSIZE], v2[BSIZE];
      GslLong b = MIN (BSIZE, length - l);

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
  pcount = PERC_TICKS;

  for (lsize = lspec->min_loop; lsize <= lspec->max_loop; lsize++)
    {
      for (l = length - lsize; l >= 0; l--)
	{
	  GslDataHandle *lhandle = gsl_data_handle_new_looped (shandle, offset + l, offset + l + lsize - 1);
	  gdouble score;
	  
	  gsl_data_handle_open (lhandle);
	  score = score_loop (shandle, lhandle, offset + l, best_score);
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
	  pcount = PERC_TICKS;
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

gfloat
gsl_data_peek_value_f (GslDataHandle     *dhandle,
		       GslLong            pos,
		       GslDataPeekBuffer *peekbuf)
{
  if (pos < peekbuf->start || pos >= peekbuf->end)
    {
      GslLong inc, k, bsize = GSL_DATA_HANDLE_PEEK_BUFFER;

      g_return_val_if_fail (pos >= 0 && pos < dhandle->n_values, 0);

      peekbuf->start = peekbuf->dir > 0 ? pos : peekbuf->dir < 0 ? pos - bsize + 1: pos - bsize / 2;
      peekbuf->end = MIN (peekbuf->start + bsize, dhandle->n_values);
      peekbuf->start = MAX (peekbuf->start, 0);
      for (k = peekbuf->start; k < peekbuf->end; k += inc)
	{
	  guint n_retries = 5;	/* FIXME: need global retry strategy */
	  
	  do
	    inc = gsl_data_handle_read (dhandle, k, peekbuf->end - k, peekbuf->data + k - peekbuf->start);
	  while (inc < 1 && n_retries-- && GSL_DATA_HANDLE_OPENED (dhandle));
	  if (inc < 1)
	    {	/* pathologic */
	      peekbuf->data[k - peekbuf->start] = 0;
	      inc = 1;
	      gsl_message_send (G_STRLOC, GSL_ERROR_READ_FAILED, "unable to read from data handle (%p)", dhandle);
	    }
	}
    }
  return peekbuf->data[pos - peekbuf->start];
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
	gfloat val = GSL_DATA_PEEK_VALUE (dhandle, i, &peekbuf);
	
	// g_print ("(%lu): %f <= %f <= %f\n", i, min_value, val, max_value);
	if (val >= min_value && val <= max_value)
	  break;
      }
  else
    for (i = start_offset; i < dhandle->n_values && i >= 0; i += direction)
      {
	gfloat val = GSL_DATA_PEEK_VALUE (dhandle, i, &peekbuf);
	
	// g_print ("(%lu): %f > %f || %f < %f\n", i, val, max_value, val, min_value);
	if (val > min_value || val < max_value)
	  break;
      }

  gsl_data_handle_close (dhandle);

  return i >= dhandle->n_values ? -1: i;
}
