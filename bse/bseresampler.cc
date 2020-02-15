// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseresampler.hh"
#include "bseblockutils.hh"
#ifdef __SSE__
#include <xmmintrin.h>
#endif

using namespace Bse;

/* see: http://ds9a.nl/gcc-simd/ */
union F4Vector
{
  float f[4];
#ifdef __SSE__
  __m128 v;   // vector of four single floats
#endif
};

using std::min;
using std::max;
using std::copy;

/* --- Resampler2 methods --- */
Resampler2::Resampler2 (Mode      mode,
                        Precision precision,
                        bool      use_sse_if_available)
{
  if (sse_available() && use_sse_if_available)
    {
      impl.reset (create_impl<true> (mode, precision));
    }
  else
    {
      impl.reset (create_impl<false> (mode, precision));
    }
}

bool
Resampler2::sse_available()
{
#ifdef __SSE__
  return true;
#else
  return false;
#endif
}

Resampler2::Precision
Resampler2::find_precision_for_bits (uint bits)
{
  if (bits <= 1)
    return PREC_LINEAR;
  if (bits <= 8)
    return PREC_48DB;
  if (bits <= 12)
    return PREC_72DB;
  if (bits <= 16)
    return PREC_96DB;
  if (bits <= 20)
    return PREC_120DB;

  /* thats the best precision we can deliver (and by the way also close to
   * the best precision possible with floats anyway)
   */
  return PREC_144DB;
}

const char *
Resampler2::precision_name (Precision precision)
{
  switch (precision)
  {
  case PREC_LINEAR:  return "linear interpolation";
  case PREC_48DB:    return "8 bit (48dB)";
  case PREC_72DB:    return "12 bit (72dB)";
  case PREC_96DB:    return "16 bit (96dB)";
  case PREC_120DB:   return "20 bit (120dB)";
  case PREC_144DB:   return "24 bit (144dB)";
  default:			    return "unknown precision enum value";
  }
}

namespace { // Anon

/* --- coefficient sets for Resampler2 --- */
/* halfband FIR filter for factor 2 resampling, created with octave
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

static const double halfband_fir_96db_coeffs[32] =
{
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

/*   coefficients = 16
 *             x0 = 1.013
 *          alpha = 0.2
 */
static const double halfband_fir_48db_coeffs[16] =
{
  -0.00270578824181636,
  0.00566964586625895,
  -0.0106460585587187,
  0.0185209590435965,
  -0.0310433957594089,
  0.0525722488176905,
  -0.0991138314110143,
  0.315921760444802,
  /* here, a 0.5 coefficient will be used */
  0.315921760444802,
  -0.0991138314110145,
  0.0525722488176907,
  -0.031043395759409,
  0.0185209590435966,
  -0.0106460585587187,
  0.00566964586625899,
  -0.00270578824181638
};

/*   coefficients = 24
 *             x0 = 1.0105
 *          alpha = 0.93
 */
static const double halfband_fir_72db_coeffs[24] =
{
  -0.0002622341634289771,
  0.0007380549701258316,
  -0.001634275943268986,
  0.00315564206632209,
  -0.005564668530702518,
  0.009207977968023688,
  -0.0145854155294611,
  0.02253220964143239,
  -0.03474055058489597,
  0.05556350980411048,
  -0.1010616834297558,
  0.316597934725021,
  /* here, a 0.5 coefficient will be used */
  0.3165979347250216,
  -0.1010616834297563,
  0.0555635098041109,
  -0.03474055058489638,
  0.02253220964143274,
  -0.01458541552946141,
  0.00920797796802395,
  -0.005564668530702722,
  0.003155642066322248,
  -0.001634275943269096,
  0.000738054970125897,
  -0.0002622341634290046,
};

/*   coefficients = 42
 *             x0 = 1.0106
 *          alpha = 0.8
 */
static const double halfband_fir_120db_coeffs[42] = {
  2.359361930421347e-06,
  -9.506281154947505e-06,
  2.748456705299089e-05,
  -6.620621425709478e-05,
  0.0001411845354098405,
  -0.0002752082937581387,
  0.0005000548069542907,
  -0.0008581650926168509,
  0.001404290771748464,
  -0.002207303823772437,
  0.003352696749689989,
  -0.004946913550236211,
  0.007125821223639453,
  -0.01007206140806936,
  0.01405163477932994,
  -0.01949467352546547,
  0.02718899890919871,
  -0.038810852733035,
  0.05873397010869939,
  -0.1030762204838426,
  0.317288892550808,
  /* here, a 0.5 coefficient will be used */
  0.3172888925508079,
  -0.1030762204838425,
  0.0587339701086993,
  -0.03881085273303492,
  0.02718899890919862,
  -0.01949467352546535,
  0.01405163477932982,
  -0.01007206140806923,
  0.007125821223639309,
  -0.004946913550236062,
  0.003352696749689839,
  -0.00220730382377229,
  0.001404290771748321,
  -0.0008581650926167192,
  0.0005000548069541726,
  -0.0002752082937580344,
  0.0001411845354097548,
  -6.620621425702783e-05,
  2.748456705294319e-05,
  -9.506281154917077e-06,
  2.359361930409472e-06
};

/*   coefficients = 52
 *             x0 = 1.0104
 *          alpha = 0.8
 */
static const double halfband_fir_144db_coeffs[52] = {
  -1.841826652087099e-07,
  8.762360674826639e-07,
  -2.867933918842901e-06,
  7.670965310712155e-06,
  -1.795091436711159e-05,
  3.808294405088742e-05,
  -7.483688716947913e-05,
  0.0001381756990743866,
  -0.0002421379200249195,
  0.0004057667984715052,
  -0.0006540521320531017,
  0.001018873594538604,
  -0.001539987101083099,
  0.002266194978575507,
  -0.003257014968854008,
  0.004585469100383752,
  -0.006343174213238195,
  0.008650017657145861,
  -0.01167305853124126,
  0.01566484143899151,
  -0.02104586507283325,
  0.02859957136356252,
  -0.04000402932277326,
  0.05964131775019404,
  -0.1036437507243546,
  0.3174820359034792,
  /* here, a 0.5 coefficient will be used */
  0.3174820359034791,
  -0.1036437507243545,
  0.05964131775019401,
  -0.04000402932277325,
  0.0285995713635625,
  -0.02104586507283322,
  0.01566484143899148,
  -0.01167305853124122,
  0.008650017657145822,
  -0.006343174213238157,
  0.004585469100383712,
  -0.003257014968853964,
  0.002266194978575464,
  -0.00153998710108306,
  0.001018873594538566,
  -0.0006540521320530672,
  0.0004057667984714751,
  -0.0002421379200248905,
  0.0001381756990743623,
  -7.483688716946011e-05,
  3.808294405087123e-05,
  -1.795091436709889e-05,
  7.670965310702215e-06,
  -2.867933918835638e-06,
  8.762360674786308e-07,
  -1.841826652067372e-07,
};

/* linear interpolation coefficients; barely useful for actual audio use,
 * but useful for testing
 */
static const double halfband_fir_linear_coeffs[2] = {
  0.25,
  /* here, a 0.5 coefficient will be used */
  0.25,
};

/*
 * FIR filter routine
 *
 * A FIR filter has the characteristic that it has a finite impulse response,
 * and can be computed by convolution of the input signal with that finite
 * impulse response.
 *
 * Thus, we use this for computing the output of the FIR filter
 *
 * output = input[0] * taps[0] + input[1] * taps[1] + ... + input[N-1] * taps[N-1]
 *
 * where input is the input signal, taps are the filter coefficients, in
 * other texts sometimes called h[0]..h[N-1] (impulse response) or a[0]..a[N-1]
 * (non recursive part of a digital filter), and N is the filter order.
 */
template<class Accumulator> static inline Accumulator
fir_process_one_sample (const float *input,
                        const float *taps, /* [0..order-1] */
			const uint   order)
{
  Accumulator out = 0;
  for (uint i = 0; i < order; i++)
    out += input[i] * taps[i];
  return out;
}

/*
 * FIR filter routine for 4 samples simultaneously
 *
 * This routine produces (approximately) the same result as fir_process_one_sample
 * but computes four consecutive output values at once using vectorized SSE
 * instructions. Note that input and sse_taps need to be 16-byte aligned here.
 *
 * Also note that sse_taps is not a plain impulse response here, but a special
 * version that needs to be computed with fir_compute_sse_taps.
 */
static inline void
fir_process_4samples_sse (const float *input,
                          const float *sse_taps,
			  const uint   order,
			  float       *out0,
			  float       *out1,
			  float       *out2,
			  float       *out3)
{
#ifdef __SSE__
  /* input and taps must be 16-byte aligned */
  const F4Vector *input_v = reinterpret_cast<const F4Vector *> (input);
  const F4Vector *sse_taps_v = reinterpret_cast<const F4Vector *> (sse_taps);
  F4Vector out0_v, out1_v, out2_v, out3_v;

  out0_v.v = _mm_mul_ps (input_v[0].v, sse_taps_v[0].v);
  out1_v.v = _mm_mul_ps (input_v[0].v, sse_taps_v[1].v);
  out2_v.v = _mm_mul_ps (input_v[0].v, sse_taps_v[2].v);
  out3_v.v = _mm_mul_ps (input_v[0].v, sse_taps_v[3].v);

  for (uint i = 1; i < (order + 6) / 4; i++)
    {
      out0_v.v = _mm_add_ps (out0_v.v, _mm_mul_ps (input_v[i].v, sse_taps_v[i * 4 + 0].v));
      out1_v.v = _mm_add_ps (out1_v.v, _mm_mul_ps (input_v[i].v, sse_taps_v[i * 4 + 1].v));
      out2_v.v = _mm_add_ps (out2_v.v, _mm_mul_ps (input_v[i].v, sse_taps_v[i * 4 + 2].v));
      out3_v.v = _mm_add_ps (out3_v.v, _mm_mul_ps (input_v[i].v, sse_taps_v[i * 4 + 3].v));
    }

  *out0 = out0_v.f[0] + out0_v.f[1] + out0_v.f[2] + out0_v.f[3];
  *out1 = out1_v.f[0] + out1_v.f[1] + out1_v.f[2] + out1_v.f[3];
  *out2 = out2_v.f[0] + out2_v.f[1] + out2_v.f[2] + out2_v.f[3];
  *out3 = out3_v.f[0] + out3_v.f[1] + out3_v.f[2] + out3_v.f[3];
#else
  BSE_ASSERT_RETURN_UNREACHED();
#endif
}


/*
 * fir_compute_sse_taps takes a normal vector of FIR taps as argument and
 * computes a specially scrambled version of these taps, ready to be used
 * for SSE operations (by fir_process_4samples_sse).
 *
 * we require a special ordering of the FIR taps, to get maximum benefit of the SSE operations
 *
 * example: suppose the FIR taps are [ x1 x2 x3 x4 x5 x6 x7 x8 x9 ], then the SSE taps become
 *
 * [ x1 x2 x3 x4   0 x1 x2 x3   0  0 x1 x2   0  0  0 x1      <- for input[0]
 *   x5 x6 x7 x8  x4 x5 x6 x7  x3 x4 x5 x6  x2 x3 x4 x5      <- for input[1]
 *   x9  0  0  0  x8 x9  0  0  x7 x8 x9  0  x6 x7 x8 x9 ]    <- for input[2]
 * \------------/\-----------/\-----------/\-----------/
 *    for out0     for out1      for out2     for out3
 *
 * so that we can compute out0, out1, out2 and out3 simultaneously
 * from input[0]..input[2]
 */
static inline vector<float>
fir_compute_sse_taps (const vector<float>& taps)
{
  const int order = taps.size();
  vector<float> sse_taps ((order + 6) / 4 * 16);

  for (int j = 0; j < 4; j++)
    for (int i = 0; i < order; i++)
      {
	int k = i + j;
	sse_taps[(k / 4) * 16 + (k % 4) + j * 4] = taps[i];
      }

  return sse_taps;
}

/*
 * This function tests the SSEified FIR filter code (that is, the reordering
 * done by fir_compute_sse_taps and the actual computation implemented in
 * fir_process_4samples_sse).
 *
 * It prints diagnostic information, and returns true if the filter
 * implementation works correctly, and false otherwise. The maximum filter
 * order to be tested can be optionally specified as argument.
 */
static inline bool
fir_test_filter_sse (bool       verbose,
                     const uint max_order = 64)
{
  int errors = 0;
  if (verbose)
    printf ("testing SSE filter implementation:\n\n");

  for (uint order = 0; order < max_order; order++)
    {
      vector<float> taps (order);
      for (uint i = 0; i < order; i++)
	taps[i] = i + 1;

      FastMemArray<float> sse_taps (fir_compute_sse_taps (taps));
      if (verbose)
	{
	  for (uint i = 0; i < sse_taps.size(); i++)
	    {
	      printf ("%3d", (int) (sse_taps[i] + 0.5));
	      if (i % 4 == 3)
		printf ("  |");
	      if (i % 16 == 15)
		printf ("   ||| upper bound = %d\n", (order + 6) / 4);
	    }
	  printf ("\n\n");
	}

      FastMemArray<float> random_mem (order + 6);
      for (uint i = 0; i < order + 6; i++)
	random_mem[i] = 1.0 - rand() / (0.5 * RAND_MAX);

      /* FIXME: the problem with this test is that we explicitely test SSE code
       * here, but the test case is not compiled with -msse within the BEAST tree
       */
      float out[4];
      fir_process_4samples_sse (&random_mem[0], &sse_taps[0], order,
	                        &out[0], &out[1], &out[2], &out[3]);

      double avg_diff = 0.0;
      for (int i = 0; i < 4; i++)
	{
	  double diff = fir_process_one_sample<double> (&random_mem[i], &taps[0], order) - out[i];
	  avg_diff += fabs (diff);
	}
      avg_diff /= (order + 1);
      bool is_error = (avg_diff > 0.00001);
      if (is_error || verbose)
	printf ("*** order = %d, avg_diff = %g\n", order, avg_diff);
      if (is_error)
	errors++;
    }
  if (errors)
    printf ("*** %d errors detected\n", errors);

  return (errors == 0);
}

} // Anon

/*
 * Factor 2 upsampling of a data stream
 *
 * Template arguments:
 *   ORDER     number of resampling filter coefficients
 *   USE_SSE   whether to use SSE (vectorized) instructions or not
 */
template<uint ORDER, bool USE_SSE>
class Resampler2::Upsampler2 final : public Resampler2::Impl {
  vector<float>          taps;
  FastMemArray<float> history;
  FastMemArray<float> sse_taps;
protected:
  /* fast SSE optimized convolution */
  void
  process_4samples_aligned (const float *input /* aligned */,
                            float       *output)
  {
    const uint H = (ORDER / 2); /* half the filter length */

    output[1] = input[H];
    output[3] = input[H + 1];
    output[5] = input[H + 2];
    output[7] = input[H + 3];

    fir_process_4samples_sse (input, &sse_taps[0], ORDER, &output[0], &output[2], &output[4], &output[6]);
  }
  /* slow convolution */
  void
  process_sample_unaligned (const float *input,
                            float       *output)
  {
    const uint H = (ORDER / 2); /* half the filter length */
    output[0] = fir_process_one_sample<float> (&input[0], &taps[0], ORDER);
    output[1] = input[H];
  }
  void
  process_block_aligned (const float *input,
                         uint         n_input_samples,
			 float       *output)
  {
    uint i = 0;
    if (USE_SSE)
      {
	while (i + 3 < n_input_samples)
	  {
	    process_4samples_aligned (&input[i], &output[i*2]);
	    i += 4;
	  }
      }
    while (i < n_input_samples)
      {
	process_sample_unaligned (&input[i], &output[2*i]);
	i++;
      }
  }
  void
  process_block_unaligned (const float *input,
                           uint         n_input_samples,
			   float       *output)
  {
    uint i = 0;
    if (USE_SSE)
      {
	while ((reinterpret_cast<ptrdiff_t> (&input[i]) & 15) && i < n_input_samples)
	  {
	    process_sample_unaligned (&input[i], &output[2 * i]);
	    i++;
	  }
      }
    process_block_aligned (&input[i], n_input_samples - i, &output[2 * i]);
  }
public:
  /*
   * Constructs an Upsampler2 object with a given set of filter coefficients.
   *
   * init_taps: coefficients for the upsampling FIR halfband filter
   */
  Upsampler2 (float *init_taps) :
    taps (init_taps, init_taps + ORDER),
    history (2 * ORDER),
    sse_taps (fir_compute_sse_taps (taps))
  {
    BSE_ASSERT_RETURN ((ORDER & 1) == 0);    /* even order filter */
  }
  /*
   * The function process_block() takes a block of input samples and produces a
   * block with twice the length, containing interpolated output samples.
   */
  void
  process_block (const float *input,
                 uint         n_input_samples,
		 float       *output) override
  {
    const uint history_todo = min (n_input_samples, ORDER - 1);

    copy (input, input + history_todo, &history[ORDER - 1]);
    process_block_aligned (&history[0], history_todo, output);
    if (n_input_samples > history_todo)
      {
	process_block_unaligned (input, n_input_samples - history_todo, &output [2 * history_todo]);

	// build new history from new input
	copy (input + n_input_samples - history_todo, input + n_input_samples, &history[0]);
      }
    else
      {
	// build new history from end of old history
	// (very expensive if n_input_samples tends to be a lot smaller than ORDER often)
	memmove (&history[0], &history[n_input_samples], sizeof (history[0]) * (ORDER - 1));
      }
  }
  /*
   * Returns the FIR filter order.
   */
  uint
  order() const override
  {
    return ORDER;
  }
  double
  delay() const override
  {
    return order() - 1;
  }
  void
  reset() override
  {
    Bse::Block::fill (history.size(), &history[0], 0.0);
  }
  bool
  sse_enabled() const override
  {
    return USE_SSE;
  }
};

/*
 * Factor 2 downsampling of a data stream
 *
 * Template arguments:
 *   ORDER    number of resampling filter coefficients
 *   USE_SSE  whether to use SSE (vectorized) instructions or not
 */
template<uint ORDER, bool USE_SSE>
class Resampler2::Downsampler2 final : public Resampler2::Impl {
  vector<float>        taps;
  FastMemArray<float> history_even;
  FastMemArray<float> history_odd;
  FastMemArray<float> sse_taps;
  /* fast SSE optimized convolution */
  template<int ODD_STEPPING> void
  process_4samples_aligned (const float *input_even /* aligned */,
                            const float *input_odd,
			    float       *output)
  {
    const uint H = (ORDER / 2) - 1; /* half the filter length */

    fir_process_4samples_sse (input_even, &sse_taps[0], ORDER, &output[0], &output[1], &output[2], &output[3]);

    output[0] += 0.5f * input_odd[H * ODD_STEPPING];
    output[1] += 0.5f * input_odd[(H + 1) * ODD_STEPPING];
    output[2] += 0.5f * input_odd[(H + 2) * ODD_STEPPING];
    output[3] += 0.5f * input_odd[(H + 3) * ODD_STEPPING];
  }
  /* slow convolution */
  template<int ODD_STEPPING> float
  process_sample_unaligned (const float *input_even,
                            const float *input_odd)
  {
    const uint H = (ORDER / 2) - 1; /* half the filter length */

    return fir_process_one_sample<float> (&input_even[0], &taps[0], ORDER) + 0.5f * input_odd[H * ODD_STEPPING];
  }
  template<int ODD_STEPPING> void
  process_block_aligned (const float *input_even,
                         const float *input_odd,
			 float       *output,
			 uint         n_output_samples)
  {
    uint i = 0;
    if (USE_SSE)
      {
	while (i + 3 < n_output_samples)
	  {
	    process_4samples_aligned<ODD_STEPPING> (&input_even[i], &input_odd[i * ODD_STEPPING], &output[i]);
	    i += 4;
	  }
      }
    while (i < n_output_samples)
      {
	output[i] = process_sample_unaligned<ODD_STEPPING> (&input_even[i], &input_odd[i * ODD_STEPPING]);
	i++;
      }
  }
  template<int ODD_STEPPING> void
  process_block_unaligned (const float *input_even,
                           const float *input_odd,
			   float       *output,
			   uint         n_output_samples)
  {
    uint i = 0;
    if (USE_SSE)
      {
	while ((reinterpret_cast<ptrdiff_t> (&input_even[i]) & 15) && i < n_output_samples)
	  {
	    output[i] = process_sample_unaligned<ODD_STEPPING> (&input_even[i], &input_odd[i * ODD_STEPPING]);
	    i++;
	  }
      }
    process_block_aligned<ODD_STEPPING> (&input_even[i], &input_odd[i * ODD_STEPPING], &output[i], n_output_samples);
  }
  void
  deinterleave2 (const float *data,
                 uint         n_data_values,
		 float       *output)
  {
    for (uint i = 0; i < n_data_values; i += 2)
      output[i / 2] = data[i];
  }
public:
  /*
   * Constructs a Downsampler2 class using a given set of filter coefficients.
   *
   * init_taps: coefficients for the downsampling FIR halfband filter
   */
  Downsampler2 (float *init_taps) :
    taps (init_taps, init_taps + ORDER),
    history_even (2 * ORDER),
    history_odd (2 * ORDER),
    sse_taps (fir_compute_sse_taps (taps))
  {
    BSE_ASSERT_RETURN ((ORDER & 1) == 0);    /* even order filter */
  }
  /*
   * The function process_block() takes a block of input samples and produces
   * a block with half the length, containing downsampled output samples.
   */
  void
  process_block (const float *input,
                 uint         n_input_samples,
		 float       *output) override
  {
    BSE_ASSERT_RETURN ((n_input_samples & 1) == 0);

    const uint BLOCKSIZE = 1024;

    F4Vector  block[BLOCKSIZE / 4]; /* using F4Vector ensures 16-byte alignment */
    float    *input_even = &block[0].f[0];

    while (n_input_samples)
      {
	uint n_input_todo = min (n_input_samples, BLOCKSIZE * 2);

        /* since the halfband filter contains zeros every other sample
	 * and since we're using SSE instructions, which expect the
	 * data to be consecutively represented in memory, we prepare
	 * a block of samples containing only even-indexed samples
	 *
	 * we keep the deinterleaved data on the stack (instead of per-class
	 * allocated memory), to ensure that even running a lot of these
	 * downsampler streams will not result in cache trashing
	 *
         * FIXME: this implementation is suboptimal for non-SSE, because it
	 * performs an extra deinterleaving step in any case, but deinterleaving
	 * is only required for SSE instructions
	 */
	deinterleave2 (input, n_input_todo, input_even);

	const float       *input_odd = input + 1; /* we process this one with a stepping of 2 */

	const uint n_output_todo = n_input_todo / 2;
	const uint history_todo = min (n_output_todo, ORDER - 1);

	copy (input_even, input_even + history_todo, &history_even[ORDER - 1]);
	deinterleave2 (input_odd, history_todo * 2, &history_odd[ORDER - 1]);

	process_block_aligned <1> (&history_even[0], &history_odd[0], output, history_todo);
	if (n_output_todo > history_todo)
	  {
	    process_block_unaligned<2> (input_even, input_odd, &output[history_todo], n_output_todo - history_todo);

	    // build new history from new input (here: history_todo == ORDER - 1)
	    copy (input_even + n_output_todo - history_todo, input_even + n_output_todo, &history_even[0]);
	    deinterleave2 (input_odd + n_input_todo - history_todo * 2, history_todo * 2, &history_odd[0]); /* FIXME: can be optimized */
	  }
	else
	  {
	    // build new history from end of old history
	    // (very expensive if n_output_todo tends to be a lot smaller than ORDER often)
	    memmove (&history_even[0], &history_even[n_output_todo], sizeof (history_even[0]) * (ORDER - 1));
	    memmove (&history_odd[0], &history_odd[n_output_todo], sizeof (history_odd[0]) * (ORDER - 1));
	  }

	n_input_samples -= n_input_todo;
	input += n_input_todo;
	output += n_output_todo;
      }
  }
  /*
   * Returns the filter order.
   */
  uint
  order() const override
  {
    return ORDER;
  }
  double
  delay() const override
  {
    return order() / 2 - 0.5;
  }
  void
  reset() override
  {
    Bse::Block::fill (history_even.size(), &history_even[0], 0.0);
    Bse::Block::fill (history_odd.size(), &history_odd[0], 0.0);
  }
  bool
  sse_enabled() const override
  {
    return USE_SSE;
  }
};

template<bool USE_SSE> Resampler2::Impl*
Resampler2::create_impl (Mode      mode,
	                 Precision precision)
{
  if (mode == UP)
    {
      switch (precision)
	{
	case PREC_LINEAR: return create_impl_with_coeffs <Upsampler2<2, USE_SSE> > (halfband_fir_linear_coeffs, 2, 2.0);
	case PREC_48DB:   return create_impl_with_coeffs <Upsampler2<16, USE_SSE> > (halfband_fir_48db_coeffs, 16, 2.0);
	case PREC_72DB:   return create_impl_with_coeffs <Upsampler2<24, USE_SSE> > (halfband_fir_72db_coeffs, 24, 2.0);
	case PREC_96DB:   return create_impl_with_coeffs <Upsampler2<32, USE_SSE> > (halfband_fir_96db_coeffs, 32, 2.0);
	case PREC_120DB:  return create_impl_with_coeffs <Upsampler2<42, USE_SSE> > (halfband_fir_120db_coeffs, 42, 2.0);
	case PREC_144DB:  return create_impl_with_coeffs <Upsampler2<52, USE_SSE> > (halfband_fir_144db_coeffs, 52, 2.0);
	}
    }
  else if (mode == DOWN)
    {
      switch (precision)
	{
	case PREC_LINEAR: return create_impl_with_coeffs <Downsampler2<2, USE_SSE> > (halfband_fir_linear_coeffs, 2, 1.0);
	case PREC_48DB:   return create_impl_with_coeffs <Downsampler2<16, USE_SSE> > (halfband_fir_48db_coeffs, 16, 1.0);
	case PREC_72DB:   return create_impl_with_coeffs <Downsampler2<24, USE_SSE> > (halfband_fir_72db_coeffs, 24, 1.0);
	case PREC_96DB:   return create_impl_with_coeffs <Downsampler2<32, USE_SSE> > (halfband_fir_96db_coeffs, 32, 1.0);
	case PREC_120DB:  return create_impl_with_coeffs <Downsampler2<42, USE_SSE> > (halfband_fir_120db_coeffs, 42, 1.0);
	case PREC_144DB:  return create_impl_with_coeffs <Downsampler2<52, USE_SSE> > (halfband_fir_144db_coeffs, 52, 1.0);
	}
    }
  return 0;
}

bool
Resampler2::test_filter_impl (bool verbose)
{
  if (sse_available())
    {
      return fir_test_filter_sse (verbose);
    }
  else
    {
      if (verbose)
        Bse::printout ("SSE filter implementation not tested: no SSE support available\n");
      return true;
    }
}
