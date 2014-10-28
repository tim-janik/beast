// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __GSL_FFT_H__
#define __GSL_FFT_H__

#include <bse/gsldefs.hh>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/**
 * @param n_values      Number of complex values
 * @param ri_values_in  Complex sample values [0..n_values*2-1]
 * @param ri_values_out Complex frequency values [0..n_values*2-1]
 *
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
 *
 * In general for the gsl_power2_fft*() family of functions, normalization is
 * only performed during backward transform if the gsl_power2_fftsc_scale()
 * is used. No normalization is performed if gsl_power2_fftsc() is used.
 *
 * However, a popular mathematical strategy of defining the FFT and IFFT in a
 * way that the formulas are symmetric is normalizing both, the forward and
 * backward transform with 1/sqrt(N) - where N is the number of complex values
 * (n_values).
 *
 * Compared to the above definition, in this implementation, the analyzed
 * values produced by gsl_power2_fftac()/gsl_power2_fftar() will be too large
 * by a factor of sqrt(N), which however are cancelled out on the backward
 * transform (for _scale variants).
 *
 * Note that the transformation is performed out of place, the input
 * array is not modified, and may not overlap with the output array.
 */
void	gsl_power2_fftac (const uint         n_values,
			  const double      *ri_values_in,
			  double            *ri_values_out);

/**
 * @param n_values      Number of complex values
 * @param ri_values_in  Complex frequency values [0..n_values*2-1]
 * @param ri_values_out Complex sample values [0..n_values*2-1]
 *
 * This function performs a decimation in time fourier transformation
 * in backwards direction with normalization. As such, this function
 * represents the counterpart to gsl_power2_fftac(), that is, a value
 * array which is transformed into the frequency domain with
 * gsl_power2_fftac() can be reconstructed by issuing gsl_power2_fftsc()
 * on the transform. This function does not perform scaling, so calling
 * gsl_power2_fftac() and gsl_power2_fftsc() will scale the data with a factor
 * of n_values.  See also gsl_power2_fftsc_scale().
 *
 * More details on normalization can be found in the documentation of
 * gsl_power2_fftac().
 *
 * Note that the transformation is performed out of place, the input
 * array is not modified, and may not overlap with the output array.
 */
void	gsl_power2_fftsc (const uint         n_values,
			  const double      *ri_values_in,
			  double            *ri_values_out);

/**
 * @param n_values      Number of complex values
 * @param ri_values_in  Complex frequency values [0..n_values*2-1]
 * @param ri_values_out Complex sample values [0..n_values*2-1]
 * This function performs a decimation in time fourier transformation
 * in backwards direction with normalization. As such, this function
 * represents the counterpart to gsl_power2_fftac(), that is, a value
 * array which is transformed into the frequency domain with
 * gsl_power2_fftac() can be reconstructed by issuing gsl_power2_fftsc()
 * on the transform.
 *
 * This function also scales the time domain coefficients by a
 * factor of 1.0/n_values which is required for perfect reconstruction
 * of time domain data formerly transformed via gsl_power2_fftac().
 * More details on normalization can be found in the documentation of
 * gsl_power2_fftac().
 *
 * Note that the transformation is performed out of place, the input
 * array is not modified, and may not overlap with the output array.
 */
void    gsl_power2_fftsc_scale (const unsigned int n_values,
			        const double      *ri_values_in,
			        double            *ri_values_out);
/**
 * @param n_values      Number of real sample values
 * @param r_values_in   Real sample values [0..n_values-1]
 * @param ri_values_out Complex frequency values [0..n_values-1]
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
 *
 * The normalization of the results of the analysis is explained in
 * gsl_power2_fftac(). Note that in the real valued case, the number of
 * complex values N for normalization is n_values/2.
 *
 * Note that the transformation is performed out of place, the input
 * array is not modified, and may not overlap with the output array.
 */
void	gsl_power2_fftar (const uint         n_values,
			  const double      *r_values_in,
			  double            *ri_values_out);

/**
 * @param n_values     Number of real sample values
 * @param ri_values_in Complex frequency values [0..n_values-1]
 * @param r_values_out Real sample values [0..n_values-1]
 *
 * Real valued variant of gsl_power2_fftsc(), counterpart to
 * gsl_power2_fftar(), using the same frequency storage format.
 * A real valued data set transformed into the frequency domain
 * with gsl_power2_fftar() can be reconstructed using this function.
 *
 * This function does not perform normalization, so data that is transformed
 * back from gsl_power2_fftar() will be scaled by a factor of n_values. See
 * also gsl_power2_fftsr_scale().
 *
 * More details on normalization can be found in the documentation of
 * gsl_power2_fftac().
 *
 * Note that the transformation is performed out of place, the input
 * array is not modified, and may not overlap with the output array.
 */
void	gsl_power2_fftsr (const unsigned int n_values,
			  const double      *ri_values_in,
			  double            *r_values_out);

/**
 * @param n_values     Number of real sample values
 * @param ri_values_in Complex frequency values [0..n_values-1]
 * @param r_values_out Real sample values [0..n_values-1]
 * Real valued variant of gsl_power2_fftsc(), counterpart to
 * gsl_power2_fftar(), using the same frequency storage format.
 * A real valued data set transformed into the frequency domain
 * with gsl_power2_fftar() can be reconstructed using this function.
 *
 * This function also scales the time domain coefficients by a
 * factor of 1.0/(n_values/2) which is required for perfect
 * reconstruction of time domain data formerly transformed via
 * gsl_power2_fftar().
 * More details on normalization can be found in the documentation of
 * gsl_power2_fftac().
 *
 * Note that the transformation is performed out of place, the input
 * array is not modified, and may not overlap with the output array.
 */
void	gsl_power2_fftsr_scale (const unsigned int n_values,
			        const double      *ri_values_in,
			        double            *r_values_out);


/* --- convenience wrappers --- */
void	gsl_power2_fftar_simple	(const uint         n_values,
				 const float       *real_values,
				 float		   *complex_values);
void	gsl_power2_fftsr_simple	(const uint         n_values,
				 const float	   *complex_values,
				 float             *real_values);
void	gsl_power2_fftsr_scale_simple (const unsigned int n_values,
				       const float	   *complex_values,
				       float             *real_values);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_FFT_H__ */   /* vim:set ts=8 sw=2 sts=2: */
