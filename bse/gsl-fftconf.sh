#!/bin/sh
# GSL-GENFFT - Power2 FFT C Code Generator
# Copyright (C) 2001 Tim Janik
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.

# include files
echo "#include $2"
echo "#include <math.h>"

MKFFT="$1"
IEEE_TYPE="double"
OPTIONS="--double"

# provide macros and inline stubs
$MKFFT $OPTIONS 0

#
# generate small fft sizes, seperating stage 2
#
for dir in --analysis --synthesis ; do
  DOPT="$OPTIONS --skip-macros $dir"
  echo "Generating FFT functions: $dir" >&2
  $MKFFT $DOPT	   2 F				# standalone fft2
  $MKFFT $DOPT	   4 S F			# reusable fft4
  $MKFFT $DOPT	   4 F X			# standalone fft4
  $MKFFT $DOPT	   8 s F F			# reusable fft8 w/o input fft2
  $MKFFT $DOPT	   8 F s X			# standalone fft8
  $MKFFT $DOPT	  16 s F F F			# reusable fft16
  $MKFFT $DOPT	  16 F s s X			# standalone fft16
  $MKFFT $DOPT	  32 s F F F F
  $MKFFT $DOPT	  32 F s s s X
  $MKFFT $DOPT	  64 s R R R R F		# reusable fft64
  $MKFFT $DOPT	  64 F s s s s X		# standalone fft64
  $MKFFT $DOPT	 128 s R R R R R F		# reusable fft128
  $MKFFT $DOPT	 128 L s s s s s X		#
  $MKFFT $DOPT	 256 s s s s s s X T		# reuse fft128
  $MKFFT $DOPT	 256 L s s s s s s X		#
  $MKFFT $DOPT	 512 s s s s s s X T T		# reuse fft128
  $MKFFT $DOPT	 512 L s s s s s s s X		# fft512
  $MKFFT $DOPT	1024 s s s s s s s s X L	#
  $MKFFT $DOPT	1024 L s s s s s s s s X	#
  $MKFFT $DOPT	2048 s s s s s s s s X L L	# reusable fft2048
  $MKFFT $DOPT	2048 L s s s s s s s s s X	#
  $MKFFT $DOPT	4096 s s s s s s s s s s X L	# reuses fft2048
  $MKFFT $DOPT	4096 L s s s s s s s s s s X	# fft4096
  $MKFFT $DOPT	8192 s s s s s s s s s s s X L	# reusable impl. for 8192, reuses 2048
  $MKFFT $DOPT	8192 L s s s s s s s s s s s X	# real impl. for 8192
done

#
# generic variable length implementation
#
echo "Generating generic FFT function for sizes >8192" >&2
cat <<__EOF
static void
gsl_power2_fftc_big (const unsigned int n_values,
		     const $IEEE_TYPE  *rivalues_in,
		     $IEEE_TYPE        *rivalues,
                     const int          esign)
{
  const unsigned int n_values2 = n_values << 1;
  double theta = esign < 0 ? -3.1415926535897932384626433832795029 : 3.1415926535897932384626433832795029;
  unsigned int i, block_size = 8192 << 1;
  double last_sin;

  if (esign > 0)
    {
      bitreverse_fft2analysis (n_values, rivalues_in, rivalues);
      for (i = 0; i < n_values; i += 8192)
        gsl_power2_fft8192analysis_skip2 (rivalues + (i << 1), rivalues + (i << 1));
    }
  else
    {
      bitreverse_fft2synthesis (n_values, rivalues_in, rivalues);
      for (i = 0; i < n_values; i += 8192)
        gsl_power2_fft8192synthesis_skip2 (rivalues + (i << 1), rivalues + (i << 1));
    }
  theta *= (double) 1.0 / 8192.;
  last_sin = sin (theta);
  
  /* we're not combining the first and second halves coefficients
   * in the below loop, since for fft sizes beyond 8192, it'd actually
   * harm performance due to paging
   */
  do
    {
      double Dre, Dim, Wre, Wim;
      unsigned int k, i, half_block = block_size >> 1;
      unsigned int block_size2 = block_size << 1;

      theta *= 0.5;
      Dim = last_sin;
      last_sin = sin (theta);
      Dre = last_sin * last_sin * -2.;
      
      /* loop over first coefficient in each block ==> w == {1,0} */
      for (i = 0; i < n_values2; i += block_size2)
	{
	  unsigned int v1 = i, v2 = i + block_size;

          BUTTERFLY_10 (rivalues[v1], rivalues[v1 + 1],
                        rivalues[v2], rivalues[v2 + 1],
                        rivalues[v1], rivalues[v1 + 1],
                        rivalues[v2], rivalues[v2 + 1],
                        __1, __0);
	}
      Wre = Dre + 1.0;	/* update Wk */
      Wim = Dim;	/* update Wk */
      /* loop for every Wk in the first half of each subblock */
      for (k = 2; k < half_block; k += 2)
	{
	  /* loop over kth coefficient in each block */
	  for (i = k; i < n_values2; i += block_size2)
	    {
	      unsigned int v1 = i, v2 = i + block_size;
	      
              BUTTERFLY_XY (rivalues[v1], rivalues[v1 + 1],
                            rivalues[v2], rivalues[v2 + 1],
                            rivalues[v1], rivalues[v1 + 1],
                            rivalues[v2], rivalues[v2 + 1],
                            Wre, Wim);
	    }
	  WMULTIPLY (Wre, Wim, Dre, Dim);	/* update Wk */
	}
      /* handle middle coefficient ==> w == {0,+-1} */
      if (k < block_size)
	{
	  /* loop over kth coefficient in each block */
	  if (esign > 0)
	    for (i = k; i < n_values2; i += block_size2)
	      {
	        unsigned int v1 = i, v2 = i + block_size;
	      
                BUTTERFLY_01 (rivalues[v1], rivalues[v1 + 1],
                              rivalues[v2], rivalues[v2 + 1],
                              rivalues[v1], rivalues[v1 + 1],
                              rivalues[v2], rivalues[v2 + 1],
                              __0, __1);
	      }
	  else
	    for (i = k; i < n_values2; i += block_size2)
	      {
	        unsigned int v1 = i, v2 = i + block_size;
	      
                BUTTERFLY_0m (rivalues[v1], rivalues[v1 + 1],
                              rivalues[v2], rivalues[v2 + 1],
                              rivalues[v1], rivalues[v1 + 1],
                              rivalues[v2], rivalues[v2 + 1],
                              __0, __1);
	      }
	  /* update Wk */
	  if (esign > 0)
	    {
	      Wre = -Dim;
	      Wim = Dre + 1.0;
	    }
	  else
	    {
	      Wre = Dim;
	      Wim = -Dre - 1.0;
	    }
	  k += 2;
	}
      /* loop for every Wk in the second half of each subblock */
      for (; k < block_size; k += 2)
	{
	  /* loop over kth coefficient in each block */
	  for (i = k; i < n_values2; i += block_size2)
	    {
	      unsigned int v1 = i, v2 = i + block_size;

              BUTTERFLY_XY (rivalues[v1], rivalues[v1 + 1],
                            rivalues[v2], rivalues[v2 + 1],
                            rivalues[v1], rivalues[v1 + 1],
                            rivalues[v2], rivalues[v2 + 1],
                            Wre, Wim);
	    }
	  WMULTIPLY (Wre, Wim, Dre, Dim);	/* update Wk */
	}
      block_size = block_size2;
    }
  while (block_size <= n_values);
}
__EOF


#
# public complex fft frontends
#
echo "Generating public complex FFT frontends" >&2
cat <<__EOF
void
gsl_power2_fftac (const unsigned int n_values,
                  const $IEEE_TYPE  *rivalues_in,
                  $IEEE_TYPE        *rivalues_out)
{
  g_return_if_fail ((n_values & (n_values - 1)) == 0 && n_values >= 1);
  
  switch (n_values)
    {
      case    1: rivalues_out[0] = rivalues_in[0], rivalues_out[1] = rivalues_in[1]; break;
      case    2: gsl_power2_fft2analysis (rivalues_in, rivalues_out);		break;
      case    4: gsl_power2_fft4analysis (rivalues_in, rivalues_out);		break;
      case    8: gsl_power2_fft8analysis (rivalues_in, rivalues_out);		break;
      case   16: gsl_power2_fft16analysis (rivalues_in, rivalues_out);		break;
      case   32: gsl_power2_fft32analysis (rivalues_in, rivalues_out);		break;
      case   64: gsl_power2_fft64analysis (rivalues_in, rivalues_out);		break;
      case  128: gsl_power2_fft128analysis (rivalues_in, rivalues_out);		break;
      case  256: gsl_power2_fft256analysis (rivalues_in, rivalues_out);		break;
      case  512: gsl_power2_fft512analysis (rivalues_in, rivalues_out);		break;
      case 1024: gsl_power2_fft1024analysis (rivalues_in, rivalues_out);	break;
      case 2048: gsl_power2_fft2048analysis (rivalues_in, rivalues_out);	break;
      case 4096: gsl_power2_fft4096analysis (rivalues_in, rivalues_out);	break;
      case 8192: gsl_power2_fft8192analysis (rivalues_in, rivalues_out);	break;
      default:	 gsl_power2_fftc_big (n_values, rivalues_in, rivalues_out, +1);
    }
}
void
gsl_power2_fftsc (const unsigned int n_values,
                  const $IEEE_TYPE  *rivalues_in,
                  $IEEE_TYPE        *rivalues_out)
{
  g_return_if_fail ((n_values & (n_values - 1)) == 0 && n_values >= 1);
  
  switch (n_values)
    {
      case    1: rivalues_out[0] = rivalues_in[0], rivalues_out[1] = rivalues_in[1]; break;
      case    2: gsl_power2_fft2synthesis (rivalues_in, rivalues_out);		break;
      case    4: gsl_power2_fft4synthesis (rivalues_in, rivalues_out);		break;
      case    8: gsl_power2_fft8synthesis (rivalues_in, rivalues_out);		break;
      case   16: gsl_power2_fft16synthesis (rivalues_in, rivalues_out);		break;
      case   32: gsl_power2_fft32synthesis (rivalues_in, rivalues_out);		break;
      case   64: gsl_power2_fft64synthesis (rivalues_in, rivalues_out);		break;
      case  128: gsl_power2_fft128synthesis (rivalues_in, rivalues_out);	break;
      case  256: gsl_power2_fft256synthesis (rivalues_in, rivalues_out);	break;
      case  512: gsl_power2_fft512synthesis (rivalues_in, rivalues_out);	break;
      case 1024: gsl_power2_fft1024synthesis (rivalues_in, rivalues_out);	break;
      case 2048: gsl_power2_fft2048synthesis (rivalues_in, rivalues_out);	break;
      case 4096: gsl_power2_fft4096synthesis (rivalues_in, rivalues_out);	break;
      case 8192: gsl_power2_fft8192synthesis (rivalues_in, rivalues_out);	break;
      default:	 gsl_power2_fftc_big (n_values, rivalues_in, rivalues_out, -1);
    }
  /* { unsigned int i; for (i = 0; i < n_values << 1; i++) rivalues_out[i] *= (double) n_values; } */
}
__EOF


#
# public real fft frontends
#
echo "Generating public real FFT frontends" >&2
cat <<__EOF
void
gsl_power2_fftar (const unsigned int n_values,
                  const $IEEE_TYPE  *r_values_in,
                  $IEEE_TYPE        *rivalues_out)
{
  unsigned int n_cvalues = n_values >> 1;
  double Dre, Dim, Wre, Wim, theta;
  unsigned int i;

  g_return_if_fail ((n_values & (n_values - 1)) == 0 && n_values >= 2);

  gsl_power2_fftac (n_cvalues, r_values_in, rivalues_out);
  theta = 3.1415926535897932384626433832795029;
  theta /= (double) n_cvalues;

  Dre = sin (0.5 * theta);
  Dim = sin (theta);
  Dre = Dre * Dre;
  Wre = 0.5 - Dre;
  Dre *= -2.;
  Wim = Dim * 0.5;
  for (i = 2; i < n_values >> 1; i += 2)
    {
      double F1re, F1im, F2re, F2im, H1re, H1im, H2re, H2im;
      unsigned int r = n_values - i;
      double FEre = rivalues_out[i] + rivalues_out[r];
      double FEim = rivalues_out[i + 1] - rivalues_out[r + 1];
      double FOre = rivalues_out[r] - rivalues_out[i];
      double FOim = rivalues_out[r + 1] + rivalues_out[i + 1];

      FEre *= 0.5;
      FEim *= 0.5;
      F2re = FOre * Wim;
      F2im = FOim * Wim;
      F1re = FOre * Wre;
      F1im = FOim * Wre;

      H1im = F2im + F1re;
      H1re = F1im - F2re;
      H2re = F2re - F1im;
      H2im = H1im - FEim;
      H1re += FEre;
      H2re += FEre;
      H1im += FEim;
      rivalues_out[i] = H1re;
      rivalues_out[i + 1] = H1im;
      rivalues_out[r] = H2re;
      rivalues_out[r + 1] = H2im;
      WMULTIPLY (Wre, Wim, Dre, Dim);
    }
  Dre = rivalues_out[0];
  rivalues_out[0] = Dre + rivalues_out[1];
  rivalues_out[1] = Dre - rivalues_out[1];
}
void
gsl_power2_fftsr (const unsigned int n_values,
                  const double      *rivalues_in,
                  double            *r_values_out)
{
  unsigned int n_cvalues = n_values >> 1;
  double Dre, Dim, Wre, Wim, theta, scale;
  unsigned int i, ri;

  g_return_if_fail ((n_values & (n_values - 1)) == 0 && n_values >= 2);

  theta = -3.1415926535897932384626433832795029;
  theta /= (double) n_cvalues;

  Dre = sin (0.5 * theta);
  Dim = sin (theta);
  Dre = Dre * Dre;
  Wre = 0.5 - Dre;
  Dre *= -2.;
  Wim = Dim * 0.5;
  for (i = 2, ri = 0; i < n_values >> 1; i += 2)
    {
      double F1re, F1im, F2re, F2im, H1re, H1im, H2re, H2im;
      unsigned int g = n_values - i, j = n_values >> 2, rg = n_values - (ri << 1) - 2;
      double FEre = rivalues_in[i] + rivalues_in[g];
      double FEim = rivalues_in[i + 1] - rivalues_in[g + 1];
      double FOre = rivalues_in[g] - rivalues_in[i];
      double FOim = rivalues_in[g + 1] + rivalues_in[i + 1];

      while (ri >= j)
        {
          ri -= j;
          j >>= 1;
        }
      ri |= j;

      FOre = -FOre;
      FOim = -FOim;
      FEre *= 0.5;
      FEim *= 0.5;
      F2re = FOre * Wim;
      F2im = FOim * Wim;
      F1re = FOre * Wre;
      F1im = FOim * Wre;

      H1im = F2im + F1re;
      H1re = F1im - F2re;
      H2re = F2re - F1im;
      H2im = H1im - FEim;
      H1re += FEre;
      H2re += FEre;
      H1im += FEim;

      j = ri << 1;
      r_values_out[j] = H1re;
      r_values_out[j + 1] = H1im;
      r_values_out[rg] = H2re;
      r_values_out[rg + 1] = H2im;
      WMULTIPLY (Wre, Wim, Dre, Dim);
    }
  Dre = rivalues_in[0];
  r_values_out[0] = Dre + rivalues_in[1];
  r_values_out[1] = Dre - rivalues_in[1];
  r_values_out[0] *= 0.5;
  r_values_out[1] *= 0.5;
  if (n_values < 4)
    return;
  r_values_out[2] = rivalues_in[i];
  r_values_out[2 + 1] = rivalues_in[i + 1];
  scale = n_cvalues;
  scale = 1.0 / scale;
  for (i = 0; i < n_values; i += 4)
    BUTTERFLY_10scale (r_values_out[i], r_values_out[i + 1],
                       r_values_out[i + 2], r_values_out[i + 3],
                       r_values_out[i], r_values_out[i + 1],
                       r_values_out[i + 2], r_values_out[i + 3],
                       scale);
  switch (n_cvalues)
    {
      case    2: break;
      case    4: gsl_power2_fft4synthesis_skip2 (NULL, r_values_out);	 break;
      case    8: gsl_power2_fft8synthesis_skip2 (NULL, r_values_out);	 break;
      case   16: gsl_power2_fft16synthesis_skip2 (NULL, r_values_out);	 break;
      case   32: gsl_power2_fft32synthesis_skip2 (NULL, r_values_out);	 break;
      case   64: gsl_power2_fft64synthesis_skip2 (NULL, r_values_out);	 break;
      case  128: gsl_power2_fft128synthesis_skip2 (NULL, r_values_out);	 break;
      case  256: gsl_power2_fft256synthesis_skip2 (NULL, r_values_out);	 break;
      case  512: gsl_power2_fft512synthesis_skip2 (NULL, r_values_out);	 break;
      case 1024: gsl_power2_fft1024synthesis_skip2 (NULL, r_values_out); break;
      case 2048: gsl_power2_fft2048synthesis_skip2 (NULL, r_values_out); break;
      case 4096: gsl_power2_fft4096synthesis_skip2 (NULL, r_values_out); break;
      case 8192: gsl_power2_fft8192synthesis_skip2 (NULL, r_values_out); break;
      default:	 gsl_power2_fftc_big (n_values, NULL, r_values_out, -1);
    }
}
__EOF
