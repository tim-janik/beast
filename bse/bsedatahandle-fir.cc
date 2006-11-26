/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2001, 2003 Tim Janik and Stefan Westerfeld
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
#include "gsldatahandle.h"
#include "gsldatautils.h"
#include "gslfilter.h"
#include <complex>
#include <string.h>
#include <stdio.h>
#include <math.h>

/*
 * 0db ----------           __________
 *                         /
 *                        /
 *                       /
 * f1_level (dB) - _____/
 *
 *                      |   |
 *                      f1 f2
 *
 * @f1: high pass start frequency [0:PI] (SR = 2*PI)
 * @f2: high pass end frequency [0:PI] (SR = 2*PI)
 */

static void
fir_hp (const gfloat *src,
	const guint   n_samples,
	gfloat       *dest,
	gdouble       cutoff_freq,
	guint         iorder)
{
  double a[iorder + 1];

  //const guint transfer_func_length = 64;
  const guint transfer_func_length = 4;
  double transfer_func_freqs[transfer_func_length];
  double transfer_func_values[transfer_func_length];

  transfer_func_freqs[0]  = 0;
  transfer_func_values[0] = 0;

  transfer_func_freqs[1]  = cutoff_freq;
  transfer_func_values[1] = 0;

  transfer_func_freqs[2]  = cutoff_freq;
  transfer_func_values[2] = 1.0; // 0 dB

  transfer_func_freqs[3]  = PI;
  transfer_func_values[3] = 1.0; // 0 dB

  gsl_filter_fir_approx (iorder, a, transfer_func_length, transfer_func_freqs, transfer_func_values, false);

#if 0 // debugging
  gfloat freq;
  for (freq = 0; freq < PI; freq += 0.01)
    {
      complex z = cexp (I * freq);
      complex r = 0;

      guint i;
      for (i = 0; i <= iorder; i++)
	{
	  r /= z;
	  r += a[i];
	}
      printf ("%f %f\n", freq, cabs (r));
    }

  /* normalize */
  gdouble norm = 0;
  guint k;
  for (k = 0; k <= iorder; k++)
    norm += fabs (a[k]);

  printf ("# norm = %f\n", norm);
  for (k = 0; k <= iorder; k++)
    a[k] /= norm;
#endif

  /* tiny FIR evaluation: not optimized for speed */
  guint i, j;
  for (i = 0; i < n_samples; i++)
    {
      gdouble accu = 0;
      for (j = 0; j <= iorder; j++)
	{
	  GslLong p = i + j;
	  p -= iorder / 2;

	  if (p >= 0 && p < n_samples)
	    accu += a[j] * src[p];
	}
      dest[i] = accu;
    }
}

/*
 *           __________
 *          /
 *         /
 *        /
 *  _____/
 *         |
 *    cutoff_freq
 *
 * @cutoff_freq: cutoff frequency in Hz in intervall [0..SR/2]
 * @order:       number of filter coefficients
 */
extern "C" GslDataHandle*
gsl_data_handle_new_fir_highpass (GslDataHandle *src_handle,
				  gdouble        cutoff_freq,
				  guint          order)
{
  g_return_val_if_fail (src_handle != NULL, NULL);
  
  /* check out data handle */
  if (gsl_data_handle_open (src_handle) != BSE_ERROR_NONE)
    return NULL;
  
  GslLong src_handle_n_values   = gsl_data_handle_n_values (src_handle);
  GslLong src_handle_n_channels = gsl_data_handle_n_channels (src_handle);
  GslLong src_handle_mix_freq   = gsl_data_handle_mix_freq (src_handle);
  GslLong src_handle_osc_freq   = gsl_data_handle_osc_freq (src_handle);
  
  /* read input */
  gfloat *src_values = g_new (gfloat, src_handle_n_values);
  GslLong i;
  GslDataPeekBuffer pbuf = { +1, };
  for (i = 0; i < src_handle_n_values; i++) /* FIXME: use gsl_data_handle_read() */
    src_values[i] = gsl_data_handle_peek_value (src_handle, i, &pbuf);
  
  /* apply fir filter to new memory buffer */
  gfloat *dest_values = g_new (gfloat, src_handle_n_values);
  fir_hp (src_values, src_handle_n_values, dest_values, cutoff_freq * 2 * M_PI / src_handle_mix_freq, order);
  g_free (src_values);
  
  /* create a mem handle with filtered data */
  GslDataHandle *mhandle = gsl_data_handle_new_mem (src_handle_n_channels,
                                                    32, // bit_depth: possibly increased by filtering
                                                    src_handle_mix_freq,
                                                    src_handle_osc_freq,
                                                    src_handle_n_values,
                                                    dest_values,
                                                    g_free);
  GslDataHandle *dhandle = gsl_data_handle_new_add_xinfos (mhandle, src_handle->setup.xinfos);
  gsl_data_handle_close (src_handle);
  gsl_data_handle_unref (mhandle);
  return dhandle;
}
