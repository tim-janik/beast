/* GSL - Generic Sound Layer
 * Copyright (C) 2001-2002 Tim Janik
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
#ifndef __GSL_FFT_H__
#define __GSL_FFT_H__

#include <bse/gsldefs.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/**
 * gsl_power2_fftac
 * @n_values:      Number of complex values
 * @ri_values_in:  Complex sample values [0..n_values*2-1]
 * @ri_values_out: Complex frequency values [0..n_values*2-1]
 * This function performs a decimation in time fourier transformation
 * in forward direction, where the input values are equidistant sampled
 * data, and the output values contain the frequency proportions of the
 * input.
 * The input and output arrays are complex values with real and imaginery
 * portions interleaved, adressable in the range [0..2*n_values-1], where
 * n_values must be a power of two.
 * Frequencies are stored in-order, the K-th output corresponds to the
 * frequency K/n_values. (If you want to interpret negative frequencies,
 * note that the frequencies -K/n_values and (n_values-K)/n_values are
 * equivalent).
 * Note that the transformation is performed out of place, the input
 * array is not modified, and may not overlap with the output array.
 */
void	gsl_power2_fftac (const unsigned int n_values,
			  const double      *ri_values_in,
			  double            *ri_values_out);

/**
 * gsl_power2_fftsc
 * @n_values:      Number of complex values
 * @ri_values_in:  Complex frequency values [0..n_values*2-1]
 * @ri_values_out: Complex sample values [0..n_values*2-1]
 * This function performs a decimation in time fourier transformation
 * in backwards direction with normalization. As such, this function
 * represents the counterpart to gsl_power2_fftac(), that is, a value
 * array which is transformed into the frequency domain with
 * gsl_power2_fftac() can be reconstructed by issuing gsl_power2_fftsc()
 * on the transform.
 * This function also scales the time domain coefficients by a
 * factor of 1.0/n_values which is required for perfect reconstruction
 * of time domain data formerly transoformed via gsl_power2_fftac().
 * Note that the transformation is performed out of place, the input
 * array is not modified, and may not overlap with the output array.
 */
void	gsl_power2_fftsc (const unsigned int n_values,
			  const double      *ri_values_in,
			  double            *ri_values_out);

/**
 * gsl_power2_fftar
 * @n_values:      Number of real sample values
 * @r_values_in:   Real sample values [0..n_values-1]
 * @ri_values_out: Complex frequency values [0..n_values-1]
 * Real valued variant of gsl_power2_fftac(), the input array contains
 * real valued equidistant sampled data [0..n_values-1], and the output
 * array contains the positive frequency half of the complex valued
 * fourier transform. Note, that the complex valued fourier transform H
 * of a purely real valued set of data, satisfies H(-f) = Conj(H(f)),
 * where Conj() denotes the complex conjugate, so that just the positive
 * frequency half suffices to describe the entire frequency spectrum.
 * However, the resulting n_values/2+1 complex frequencies are one value
 * off in storage size, but the resulting frequencies H(0) and
 * H(n_values/2) are both real valued, so the real portion of
 * H(n_values/2) is stored in ri_values_out[1] (the imaginery part of
 * H(0)), so that both arrays r_values_in and ri_values_out can be of
 * size n_values.
 * Note that the transformation is performed out of place, the input
 * array is not modified, and may not overlap with the output array.
 */
void	gsl_power2_fftar (const unsigned int n_values,
			  const double      *r_values_in,
			  double            *ri_values_out);

/**
 * gsl_power2_fftsr
 * @n_values:     Number of real sample values
 * @ri_values_in: Complex frequency values [0..n_values-1]
 * @r_values_out: Real sample values [0..n_values-1]
 * Real valued variant of gsl_power2_fftsc(), counterpart to
 * gsl_power2_fftar(), using the same frequency storage format.
 * A real valued data set transformed into the frequency domain
 * with gsl_power2_fftar() can be reconstructed using this function.
 * This function also scales the time domain coefficients by a
 * factor of 1.0/n_values which is required for perfect reconstruction
 * of time domain data formerly transformed via gsl_power2_fftar().
 * Note that the transformation is performed out of place, the input
 * array is not modified, and may not overlap with the output array.
 */
void	gsl_power2_fftsr (const unsigned int n_values,
			  const double      *ri_values_in,
			  double            *r_values_out);


/* --- convenience wrappers --- */
void	gsl_power2_fftar_simple	(const unsigned int n_values,
				 const float       *real_values,
				 float		   *complex_values);
void	gsl_power2_fftsr_simple	(const unsigned int n_values,
				 const float	   *complex_values,
				 float             *real_values);
     
     
     
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_FFT_H__ */   /* vim:set ts=8 sw=2 sts=2: */
