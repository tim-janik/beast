// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include <bse/gslcommon.hh>
#include <bse/bsemath.hh>
#include <bse/bsemain.hh>
#include <bse/gslfft.hh>
#include <rapicorn-test.hh>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>

#define	MAX_FFT_SIZE	(65536 * 2) //  * 8 * 8
#define	MAX_DFT_SIZE	(1024 * 2) //  * 8 * 8
#define	EPSILON		(4.8e-6)
#define REF_ANALYSIS   (-1)
#define REF_SYNTHESIS  (1)
/* --- prototypes --- */
static void	reference_power2_fftc	(unsigned int       n_values,
					 const double      *rivalues_in,
					 double            *rivalues_out,
					 int                esign);
static void	reference_dftc	        (unsigned int       n_values,
					 const double      *rivalues_in,
					 double            *rivalues_out);
static void	fill_rand		(guint		    n,
					 double		   *a);
static void	scale_block    		(guint		    n,
					 double		   *a,
                                         double             factor);
static double	diff			(guint   	    m,
					 guint   	    p,
					 double 	   *a1,
					 double 	   *a2,
					 const gchar  	   *str);
static void     make_real               (guint              n,
                                         double            *a);
static void     extract_real            (guint              n,
                                         const double      *a,
                                         double            *b);



/* --- functions --- */
int
main (int   argc,
      char *argv[])
{
  struct timeval tv;
  guint i;
  /* initialize */
  bse_init_test (&argc, argv);
  /* initialize random numbers */
  gettimeofday (&tv, NULL);
  srand (tv.tv_sec ^ tv.tv_usec);
  static double ref_fft_in[MAX_FFT_SIZE] = { 0, };
  static double ref_fft_aout[MAX_FFT_SIZE] = { 0, };
  static double ref_fft_sout[MAX_FFT_SIZE] = { 0, };
  static double ref_fft_back[MAX_FFT_SIZE] = { 0, };
  static double work_fft_in[MAX_FFT_SIZE] = { 0, };
  static double work_fft_aout[MAX_FFT_SIZE] = { 0, };
  static double work_fft_sout[MAX_FFT_SIZE] = { 0, };
  static double work_fft_back[MAX_FFT_SIZE] = { 0, };
  static double scaled_fft_back[MAX_FFT_SIZE] = { 0, };

  /* run tests */
  for (i = 8; i <= MAX_FFT_SIZE >> 1; i <<= 1)
    {
      double d;

      TSTART ("Testing fft code for size %u", i);

      /* setup reference and work fft records */
      fill_rand (i << 1, ref_fft_in);
      // memset (ref_fft_aout, 0, MAX_FFT_SIZE * sizeof (ref_fft_aout[0]));
      // memset (ref_fft_sout, 0, MAX_FFT_SIZE * sizeof (ref_fft_sout[0]));
      // memset (ref_fft_back, 0, MAX_FFT_SIZE * sizeof (ref_fft_sout[0]));
      memcpy (work_fft_in, ref_fft_in, MAX_FFT_SIZE * sizeof (work_fft_in[0]));
      // memset (work_fft_aout, 0, MAX_FFT_SIZE * sizeof (work_fft_aout[0]));
      // memset (work_fft_sout, 0, MAX_FFT_SIZE * sizeof (work_fft_sout[0]));
      // memset (work_fft_back, 0, MAX_FFT_SIZE * sizeof (work_fft_sout[0]));
      reference_power2_fftc (i, ref_fft_in, ref_fft_aout, REF_ANALYSIS);
      reference_power2_fftc (i, ref_fft_in, ref_fft_sout, REF_SYNTHESIS);
      reference_power2_fftc (i, ref_fft_aout, ref_fft_back, REF_SYNTHESIS);
      scale_block (i << 1, ref_fft_back, 1.0 / i);

      /* perform fft test */
      gsl_power2_fftac (i, work_fft_in, work_fft_aout);
      gsl_power2_fftsc (i, work_fft_in, work_fft_sout);
      gsl_power2_fftsc (i, work_fft_aout, work_fft_back);
      scale_block (i << 1, work_fft_back, 1.0 / i);
      gsl_power2_fftsc_scale (i, work_fft_aout, scaled_fft_back);

      /* check differences */
      d = diff (i << 1, 0, ref_fft_in, work_fft_in, "Checking input record");
      if (d)
	fatal ("Reference record was modified");
      else
        TOK();
      d = diff (i << 1, 0, ref_fft_aout, work_fft_aout, "Reference analysis against GSL analysis");
      if (fabs (d) > EPSILON)
	fatal ("Error sum in analysis FFT exceeds epsilon: %g > %g", d, EPSILON);
      else
        TOK();
      d = diff (i << 1, 0, ref_fft_sout, work_fft_sout, "Reference synthesis against GSL synthesis");
      if (fabs (d) > EPSILON)
	fatal ("Error sum in analysis FFT exceeds epsilon: %g > %g", d, EPSILON);
      else
        TOK();
      d = diff (i << 1, 0, ref_fft_in, ref_fft_back, "Reference analysis and re-synthesis");
      if (fabs (d) > EPSILON)
	fatal ("Error sum in analysis FFT exceeds epsilon: %g > %g", d, EPSILON);
      else
        TOK();
      d = diff (i << 1, 0, work_fft_in, work_fft_back, "GSL analysis and re-synthesis");
      if (fabs (d) > EPSILON)
	fatal ("Error sum in analysis FFT exceeds epsilon: %g > %g", d, EPSILON);
      else
        TOK();
      d = diff (i << 1, 0, work_fft_in, scaled_fft_back, "GSL analysis and scaled re-synthesis");
      if (fabs (d) > EPSILON)
	fatal ("Error sum in analysis FFT exceeds epsilon: %g > %g", d, EPSILON);
      else
        TOK();
      d = diff (i << 1, 0, ref_fft_back, work_fft_back, "Reference re-synthesis vs. GSL");
      if (fabs (d) > EPSILON)
	fatal ("Error sum in analysis FFT exceeds epsilon: %g > %g", d, EPSILON);
      else
        TOK();
      d = diff (i << 1, 0, ref_fft_back, scaled_fft_back, "Reference re-synthesis vs. scaled GSL");
      if (fabs (d) > EPSILON)
	fatal ("Error sum in analysis FFT exceeds epsilon: %g > %g", d, EPSILON);
      else
        TOK();
      /* test with real data */
      make_real (i << 1, ref_fft_in);
      extract_real (i << 1, ref_fft_in, work_fft_in);
      reference_power2_fftc (i, ref_fft_in, ref_fft_aout, REF_ANALYSIS);
      ref_fft_aout[1] = ref_fft_aout[i]; /* special packing for purely real FFTs */
      /* perform real fft test */
      gsl_power2_fftar (i, work_fft_in, work_fft_aout);
      gsl_power2_fftsr (i, work_fft_aout, work_fft_back);
      scale_block (i, work_fft_back, 1.0 / i);
      gsl_power2_fftsr_scale (i, work_fft_aout, scaled_fft_back);
      d = diff (i, 0, ref_fft_aout, work_fft_aout, "Reference real analysis vs. real GSL");
      if (fabs (d) > EPSILON)
	fatal ("Error sum in analysis FFT exceeds epsilon: %g > %g", d, EPSILON);
      else
        TOK();
      d = diff (i, 0, work_fft_in, scaled_fft_back, "Real input vs. scaled real GSL resynthesis");
      if (fabs (d) > EPSILON)
	fatal ("Error sum in analysis FFT exceeds epsilon: %g > %g", d, EPSILON);
      else
        TOK();
      d = diff (i, 0, work_fft_in, work_fft_back, "Real input vs. real GSL resynthesis");
      if (fabs (d) > EPSILON)
	fatal ("Error sum in analysis FFT exceeds epsilon: %g > %g", d, EPSILON);
      else
        TOK();
      TDONE();
    }
  static double dft_in[MAX_DFT_SIZE] = { 0, };
  static double dft_aout[MAX_DFT_SIZE] = { 0, };
  /* test reference fft against reference dft */
  for (i = 2; i <= MAX_DFT_SIZE >> 1; i <<= 1)
    {
      double d;
      TSTART ("Checking reference fft for size %u", i);
      /* setup reference and work fft records */
      fill_rand (i << 1, ref_fft_in);
      memcpy (dft_in, ref_fft_in, MAX_DFT_SIZE * sizeof (dft_in[0]));
      reference_power2_fftc (i, ref_fft_in, ref_fft_aout, REF_ANALYSIS);
      reference_dftc (i, dft_in, dft_aout);
      /* check differences */
      d = diff (i << 1, 0, ref_fft_in, dft_in, "Checking input record");
      if (d)
	fatal ("Input record was modified");
      else
        TOK();
      d = diff (i << 1, 0, ref_fft_aout, dft_aout, "Reference FFT analysis against reference DFT analysis");
      if (fabs (d) > EPSILON)
        fatal ("Error sum in analysis FFT exceeds epsilon: %g > %g", d, EPSILON);
      else
        TOK();
      TDONE();
    }
  return 0;
}
static void
fill_rand (guint   n,
	   double *a)
{
  while (n--)
    a[n] = -1. + 2. * rand() / (RAND_MAX + 1.0);
}

static void
make_real (guint              n,
           double            *a)
{
  guint x;
  for (x = 1; x < n; x += 2)
    a[x] = 0; /* eliminate complex part */
}

static void
extract_real (guint              n,
              const double      *a,
              double            *b)
{
  guint x;
  for (x = 0; x < n; x += 2)
    *b++ = a[x]; /* extract real part */
}


static void
scale_block (guint    n,
	     double  *a,
             double   factor)
{
  while (n--)
    a[n] *= factor;
}

static double
diff (guint         m,
      guint         p,
      double       *a1,
      double       *a2,
      const gchar  *str)
{
  double d = 0, max = 0, min = 1e+32;
  guint n;
  TMSG ("%s\n", str);
  for (n = 0; n < m; n++)
    {
      double a =  ABS (a1[n] - a2[n]);
      if (n < p)
	TMSG ("%3u:%.3f) % 19.9f - % 19.9f = % 19.9f (% 19.9f)\n",
                n, ((float) n) / (float) m,
                a1[n], a2[n],
                a1[n] - a2[n],
                a1[n] / a2[n]);
      d += a;
      max = MAX (max, a);
      min = MIN (min, a);
    }
  TMSG ("Diff sum: %.9f, ", d);
  TMSG ("min/av/max: %.9f %.9f %.9f, ", min, d / (double) m, max);
  TMSG ("noise: %u %u %u\n",
        g_bit_storage (1. / min),
        g_bit_storage (m / d),
        g_bit_storage (1. / max));
  return d;
}


/* --- fft implementation --- */
#define BUTTERFLY_XY(X1re,X1im,X2re,X2im,Y1re,Y1im,Y2re,Y2im,Wre,Wim) { \
  register double T1re, T1im, T2re, T2im; \
  T1re = X2re * Wre;  \
  T1im = X2im * Wre;  \
  T2re = X2im * Wim;  \
  T2im = X2re * Wim;  \
  T1re -= T2re;       \
  T1im += T2im;       \
  T2re = X1re - T1re; \
  T2im = X1im - T1im; \
  Y1re = X1re + T1re; \
  Y1im = X1im + T1im; \
  Y2re = T2re;        \
  Y2im = T2im;        \
}
#define BUTTERFLY_10(X1re,X1im,X2re,X2im,Y1re,Y1im,Y2re,Y2im,_1,_2) { \
  register double T2re, T2im; \
  T2re = X1re - X2re; \
  T2im = X1im - X2im; \
  Y1re = X1re + X2re; \
  Y1im = X1im + X2im; \
  Y2re = T2re;        \
  Y2im = T2im;        \
}
#define BUTTERFLY_01(X1re,X1im,X2re,X2im,Y1re,Y1im,Y2re,Y2im,_1,_2) { \
  register double T2re, T2im; \
  T2re = X1re + X2im; \
  T2im = X1im - X2re; \
  Y1re = X1re - X2im; \
  Y1im = X1im + X2re; \
  Y2re = T2re;        \
  Y2im = T2im;        \
}
#define BUTTERFLY_0m(X1re,X1im,X2re,X2im,Y1re,Y1im,Y2re,Y2im,_1,_2) { \
  register double T2re, T2im; \
  T2re = X1re - X2im; \
  T2im = X1im + X2re; \
  Y1re = X1re + X2im; \
  Y1im = X1im - X2re; \
  Y2re = T2re;        \
  Y2im = T2im;        \
}
#define BUTTERFLY_10scale(X1re,X1im,X2re,X2im,Y1re,Y1im,Y2re,Y2im,S) { \
  register double T2re, T2im; \
  T2re = X1re - X2re; \
  T2im = X1im - X2im; \
  Y1re = X1re + X2re; \
  Y1im = X1im + X2im; \
  Y2re = T2re * S;    \
  Y2im = T2im * S;    \
  Y1re *= S;          \
  Y1im *= S;          \
}
#define WMULTIPLY(Wre,Wim,Dre,Dim) { \
  register double T1re, T1im, T2re, T2im; \
  T1re = Wre * Dre;  \
  T1im = Wim * Dre;  \
  T2re = Wim * Dim;  \
  T2im = Wre * Dim;  \
  T1re -= T2re;      \
  T1im += T2im;      \
  Wre += T1re;       \
  Wim += T1im;       \
}

static inline void
reference_bitreverse_fft2analysis (const unsigned int n,
				   const double      *X,
				   double            *Y)
{
  const unsigned int n2 = n >> 1, n1 = n + n2, max = n >> 2;
  unsigned int i, r;

  BUTTERFLY_10 (X[0], X[1],
		X[n], X[n + 1],
		Y[0], Y[1],
		Y[2], Y[3],
		__1, __0);
  if (n < 4)
    return;
  BUTTERFLY_10 (X[n2], X[n2 + 1],
		X[n1], X[n1 + 1],
		Y[4], Y[5],
		Y[6], Y[7],
		__1, __0);
  if (n < 8)
    return;
  for (i = 1, r = 0; i < max; i++)
    {
      unsigned int k, j = n >> 1;

      while (r >= j)
	{
	  r -= j;
	  j >>= 1;
	}
      r |= j;

      k = r >> 1;
      j = i << 3;
      BUTTERFLY_10 (X[k], X[k + 1],
		    X[k + n], X[k + n + 1],
		    Y[j], Y[j + 1],
		    Y[j + 2], Y[j + 3],
		    __1, __0);
      k += n2;
      j += 4;
      BUTTERFLY_10 (X[k], X[k + 1],
		    X[k + n], X[k + n + 1],
		    Y[j], Y[j + 1],
		    Y[j + 2], Y[j + 3],
		    __1, __0);
    }
}

static inline void
reference_bitreverse_fft2synthesis (const unsigned int n,
				    const double      *X,
				    double            *Y)
{
  const unsigned int n2 = n >> 1, n1 = n + n2, max = n >> 2;
  unsigned int i, r;
  double scale = n;

  scale = 1; /* set to 1.0 / scale to get scaled synthesis */
  BUTTERFLY_10scale (X[0], X[1],
		     X[n], X[n + 1],
		     Y[0], Y[1],
		     Y[2], Y[3],
		     scale);
  if (n < 4)
    return;
  BUTTERFLY_10scale (X[n2], X[n2 + 1],
		     X[n1], X[n1 + 1],
		     Y[4], Y[5],
		     Y[6], Y[7],
		     scale);
  if (n < 8)
    return;
  for (i = 1, r = 0; i < max; i++)
    {
      unsigned int k, j = n >> 1;

      while (r >= j)
	{
	  r -= j;
	  j >>= 1;
	}
      r |= j;

      k = r >> 1;
      j = i << 3;
      BUTTERFLY_10scale (X[k], X[k + 1],
			 X[k + n], X[k + n + 1],
			 Y[j], Y[j + 1],
			 Y[j + 2], Y[j + 3],
			 scale);
      k += n2;
      j += 4;
      BUTTERFLY_10scale (X[k], X[k + 1],
			 X[k + n], X[k + n + 1],
			 Y[j], Y[j + 1],
			 Y[j + 2], Y[j + 3],
			 scale);
    }
}

static void
reference_power2_fftc (unsigned int  n_values,
		       const double *rivalues_in,
		       double       *rivalues,
		       int           esign)
{
  const unsigned int n_values2 = n_values << 1;
  double theta = esign < 0 ? -3.1415926535897932384626433832795029 : 3.1415926535897932384626433832795029;
  unsigned int block_size = 2 << 1;
  double last_sin;

  if (esign > 0)
    reference_bitreverse_fft2analysis (n_values, rivalues_in, rivalues);
  else
    reference_bitreverse_fft2synthesis (n_values, rivalues_in, rivalues);
  theta *= (double) 1.0 / 2.;
  last_sin = sin (theta);

  if (n_values < 4)
    return;

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

/*--------------- reference DFT -----------------*/

static BseComplex
complex_exp (BseComplex z)
{
  /* also found in g++-4.2 C++ complex numbers */
  return bse_complex_polar (exp(z.re), z.im);
}

void
reference_dftc (unsigned int       n_values,
		const double      *rivalues_in,
		double            *rivalues_out)
{
  /* http://en.wikipedia.org/wiki/Discrete_Fourier_transform says:
   *
   * out[k] = SUM{n=0..N-1} (in[n] * exp (-2 * pi * j / N * k * n))
   */
  guint k, n;
  for (k = 0; k < n_values; k++)
    {
      BseComplex result = { 0, 0 };

      for (n = 0; n < n_values; n++)
        result = bse_complex_add (result,
                                  bse_complex_mul (bse_complex (rivalues_in[n * 2], rivalues_in[n * 2 + 1]),
                                                   complex_exp (bse_complex (0, -2 * PI / n_values * ((k * n) % n_values)))));

      rivalues_out[k * 2]     = result.re;
      rivalues_out[k * 2 + 1] = result.im;
    }
}
