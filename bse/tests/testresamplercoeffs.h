/* SSE optimized FIR Resampling code
 * Copyright (C) 2006 Stefan Westerfeld
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

/* 
 * halfband FIR filter for factor 2 resampling, created with octave
 *
 * design method: windowed sinc,  using ultraspherical window
 *
 *   coefficients = 32
 *             x0 = 1.01065
 *          alpha = 0.75
 *
 * design criteria (44100 Hz => 88200 Hz):
 * 
 *       passband = [     0, 18000 ]  1 - 2^-16 <= H(z) <= 1+2^-16
 *     transition = [ 18000, 26100 ]
 *       stopband = [ 26100, 44100 ]  | H(z) | <= -96 dB
 *
 * and for 48 kHz => 96 kHz:
 *
 *       passband = [     0, 19589 ]  1 - 2^-16 <= H(z) <= 1+2^-16
 *     transition = [ 19588, 29386 ]
 *       stopband = [ 29386, 48000 ]  | H(z) | <= -96 dB
 *
 * in order to keep the coefficient number down to 32, the filter
 * does only "almost" fulfill the spec, but its really really close
 * (no stopband ripple > -95 dB)
 */

const double halfband_fir_upsample2_96db_coeffs[32] = {
    -3.48616530828033e-05,
     0.000112877490936198,
    -0.000278961878372482,
     0.000590495306376081,
    -0.00112566995029848,
     0.00198635062559427,
    -0.00330178798332932,
     0.00523534239035401,
    -0.00799905465189065,
     0.0118867161189188,
    -0.0173508611368417,
     0.0251928452706978,
    -0.0370909694665106,
     0.057408291607388,
    -0.102239638342325,
     0.317002929635456,
     /* here, a 0.5 coefficient will be used */
     0.317002929635456,
    -0.102239638342325,
     0.0574082916073878,
    -0.0370909694665105,
     0.0251928452706976,
    -0.0173508611368415,
     0.0118867161189186,
    -0.00799905465189052,
     0.0052353423903539,
    -0.00330178798332923,
     0.00198635062559419,
    -0.00112566995029842,
     0.000590495306376034,
    -0.00027896187837245,
     0.000112877490936177,
    -3.48616530827983e-05
};
