/* BseResampler - FPU and SSE optimized FIR Resampling code
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
#ifndef __BSE_RESAMPLER_TCC__
#define __BSE_RESAMPLER_TCC__

#include <vector>
#include <bse/bseresampler.hh>
#include <birnet/birnet.h>
#ifdef __SSE__
#include <xmmintrin.h>
#endif

namespace Bse {
namespace Resampler {
using std::vector;
using std::min;
using std::max;
using std::copy;

/* see: http://ds9a.nl/gcc-simd/ */
union F4Vector 
{
  float f[4];
#ifdef __SSE__
  __m128 v;   // vector of four single floats
#endif
};

/**
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
			const guint  order)
{
  Accumulator out = 0;
  for (guint i = 0; i < order; i++)
    out += input[i] * taps[i];
  return out;
}

/**
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
			  const guint  order,
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
  
  for (guint i = 1; i < (order + 6) / 4; i++)
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
  g_assert_not_reached();
#endif
}


/**
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

/* Helper class to allocate aligned memory */
template<class T, int ALIGN>
class AlignedArray {
  unsigned char *unaligned_mem;
  T *data;
  unsigned int n_elements;
  
  void
  allocate_aligned_data()
  {
    g_assert ((ALIGN % sizeof (T)) == 0);
    data = reinterpret_cast<T *> (birnet_malloc_aligned (n_elements * sizeof (T), ALIGN, &unaligned_mem));
  }
  /* no copy constructor and no assignment operator */
  BIRNET_PRIVATE_CLASS_COPY (AlignedArray);
public:
  AlignedArray (const vector<T>& elements) :
    n_elements (elements.size())
  {
    allocate_aligned_data();
    
    for (unsigned int i = 0; i < n_elements; i++)
      new (data + i) T(elements[i]);
  }
  AlignedArray (unsigned int n_elements) :
    n_elements (n_elements)
  {
    allocate_aligned_data();
    
    for (unsigned int i = 0; i < n_elements; i++)
      new (data + i) T();
  }
  ~AlignedArray()
  {
    /* C++ destruction order: last allocated element is deleted first */
    while (n_elements)
      data[--n_elements].~T();
    
    g_free (unaligned_mem);
  }
  T&
  operator[] (unsigned int pos)
  {
    return data[pos];
  }
  const T&
  operator[] (unsigned int pos) const
  {
    return data[pos];
  }
  unsigned int
  size()
  {
    return n_elements;
  }
};

/**
 * Factor 2 upsampling of a data stream
 *
 * Template arguments:
 *   ORDER     number of resampling filter coefficients
 *   USE_SSE   whether to use SSE (vectorized) instructions or not
 */
template<guint ORDER, bool USE_SSE>
class Upsampler2 : public Resampler2 {
  vector<float>          taps;
  AlignedArray<float,16> history;
  AlignedArray<float,16> sse_taps;
protected:
  /* fast SSE optimized convolution */
  void
  process_4samples_aligned (const float *input /* aligned */,
                            float       *output)
  {
    const guint H = (ORDER / 2) - 1; /* half the filter length */
    
    output[0] = input[H];
    output[2] = input[H + 1];
    output[4] = input[H + 2];
    output[6] = input[H + 3];
    
    fir_process_4samples_sse (input, &sse_taps[0], ORDER, &output[1], &output[3], &output[5], &output[7]);
  }
  /* slow convolution */
  void
  process_sample_unaligned (const float *input,
                            float       *output)
  {
    const guint H = (ORDER / 2) - 1; /* half the filter length */
    output[0] = input[H];
    output[1] = fir_process_one_sample<float> (&input[0], &taps[0], ORDER);
  }
  void
  process_block_aligned (const float *input,
                         guint        n_input_samples,
			 float       *output)
  {
    unsigned int i = 0;
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
                           guint        n_input_samples,
			   float       *output)
  {
    unsigned int i = 0;
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
  /**
   * Constructs an Upsampler2 object with a given set of filter coefficients.
   *
   * init_taps: coefficients for the upsampling FIR halfband filter
   */
  Upsampler2 (float *init_taps) :
    taps (init_taps, init_taps + ORDER),
    history (2 * ORDER),
    sse_taps (fir_compute_sse_taps (taps))
  {
    g_assert ((ORDER & 1) == 0);    /* even order filter */
  }
  /**
   * The function process_block() takes a block of input samples and produces a
   * block with twice the length, containing interpolated output samples.
   */
  void
  process_block (const float *input,
                 guint        n_input_samples,
		 float       *output)
  {
    unsigned int history_todo = min (n_input_samples, ORDER);
    
    copy (input, input + history_todo, &history[ORDER]);
    process_block_aligned (&history[0], history_todo, output);
    if (n_input_samples >= ORDER)
      {
	process_block_unaligned (input, n_input_samples - history_todo, &output [2 * history_todo]);
        
	// build new history from new input
	copy (input + n_input_samples - ORDER, input + n_input_samples, &history[0]);
      }
    else
      {
	// build new history from end of old history
	// (very expensive if n_input_samples tends to be a lot smaller than ORDER often)
	g_memmove (&history[0], &history[n_input_samples], sizeof (history[0]) * ORDER);
      }
  }
  /**
   * Returns the FIR filter order.
   */
  guint
  order() const
  {
    return ORDER;
  }
};

/**
 * Factor 2 downsampling of a data stream
 *
 * Template arguments:
 *   ORDER    number of resampling filter coefficients
 *   USE_SSE  whether to use SSE (vectorized) instructions or not
 */
template<guint ORDER, bool USE_SSE>
class Downsampler2 : public Resampler2 {
  vector<float>        taps;
  AlignedArray<float,16> history_even;
  AlignedArray<float,16> history_odd;
  AlignedArray<float,16> sse_taps;
  /* fast SSE optimized convolution */
  template<int ODD_STEPPING> void
  process_4samples_aligned (const float *input_even /* aligned */,
                            const float *input_odd,
			    float       *output)
  {
    const guint H = (ORDER / 2) - 1; /* half the filter length */
    
    fir_process_4samples_sse (input_even, &sse_taps[0], ORDER, &output[0], &output[1], &output[2], &output[3]);
    
    output[0] += 0.5 * input_odd[H * ODD_STEPPING];
    output[1] += 0.5 * input_odd[(H + 1) * ODD_STEPPING];
    output[2] += 0.5 * input_odd[(H + 2) * ODD_STEPPING];
    output[3] += 0.5 * input_odd[(H + 3) * ODD_STEPPING];
  }
  /* slow convolution */
  template<int ODD_STEPPING> float
  process_sample_unaligned (const float *input_even,
                            const float *input_odd)
  {
    const guint H = (ORDER / 2) - 1; /* half the filter length */
    
    return fir_process_one_sample<float> (&input_even[0], &taps[0], ORDER) + 0.5 * input_odd[H * ODD_STEPPING];
  }
  template<int ODD_STEPPING> void
  process_block_aligned (const float *input_even,
                         const float *input_odd,
			 float       *output,
			 guint        n_output_samples)
  {
    unsigned int i = 0;
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
			   guint        n_output_samples)
  {
    unsigned int i = 0;
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
                 guint        n_data_values,
		 float       *output)
  {
    for (unsigned int i = 0; i < n_data_values; i += 2)
      output[i / 2] = data[i];
  }
public:
  /**
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
    g_assert ((ORDER & 1) == 0);    /* even order filter */
  }
  /**
   * The function process_block() takes a block of input samples and produces
   * a block with half the length, containing downsampled output samples.
   */
  void
  process_block (const float *input,
                 guint        n_input_samples,
		 float       *output)
  {
    g_assert ((n_input_samples & 1) == 0);
    
    const unsigned int BLOCKSIZE = 1024;
    
    F4Vector  block[BLOCKSIZE / 4]; /* using F4Vector ensures 16-byte alignment */
    float    *input_even = &block[0].f[0];
    
    while (n_input_samples)
      {
	unsigned int n_input_todo = min (n_input_samples, BLOCKSIZE * 2);
        
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
        
	const unsigned int n_output_todo = n_input_todo / 2;
	const unsigned int history_todo = min (n_output_todo, ORDER);
        
	copy (input_even, input_even + history_todo, &history_even[ORDER]);
	deinterleave2 (input_odd, history_todo * 2, &history_odd[ORDER]);
        
	process_block_aligned <1> (&history_even[0], &history_odd[0], output, history_todo);
	if (n_output_todo >= ORDER)
	  {
	    process_block_unaligned<2> (input_even, input_odd, &output[history_todo], n_output_todo - history_todo);
            
	    // build new history from new input
	    copy (input_even + n_output_todo - ORDER, input_even + n_output_todo, &history_even[0]);
	    deinterleave2 (input_odd + n_input_todo - ORDER * 2, ORDER * 2, &history_odd[0]); /* FIXME: can be optimized */
	  }
	else
	  {
	    // build new history from end of old history
	    // (very expensive if n_output_todo tends to be a lot smaller than ORDER often)
	    g_memmove (&history_even[0], &history_even[n_output_todo], sizeof (history_even[0]) * ORDER);
	    g_memmove (&history_odd[0], &history_odd[n_output_todo], sizeof (history_odd[0]) * ORDER);
	  }
        
	n_input_samples -= n_input_todo;
	input += n_input_todo;
	output += n_output_todo;
      }
  }
  /**
   * Returns the filter order.
   */
  guint
  order() const
  {
    return ORDER;
  }
};

template<bool USE_SSE> Resampler2*
Resampler2::create_impl (BseResampler2Mode      mode,
	                 BseResampler2Precision precision)
{
  if (mode == BSE_RESAMPLER2_MODE_UPSAMPLE)
    {
      switch (precision)
	{
	case BSE_RESAMPLER2_PREC_48DB:   return create_impl_with_coeffs <Upsampler2<16, USE_SSE> > (halfband_fir_48db_coeffs, 16, 2.0);
	case BSE_RESAMPLER2_PREC_72DB:   return create_impl_with_coeffs <Upsampler2<24, USE_SSE> > (halfband_fir_72db_coeffs, 24, 2.0);
	case BSE_RESAMPLER2_PREC_96DB:   return create_impl_with_coeffs <Upsampler2<32, USE_SSE> > (halfband_fir_96db_coeffs, 32, 2.0);
	case BSE_RESAMPLER2_PREC_120DB:  return create_impl_with_coeffs <Upsampler2<42, USE_SSE> > (halfband_fir_120db_coeffs, 42, 2.0);
	case BSE_RESAMPLER2_PREC_144DB:  return create_impl_with_coeffs <Upsampler2<52, USE_SSE> > (halfband_fir_144db_coeffs, 52, 2.0);
	}
    }
  else if (mode == BSE_RESAMPLER2_MODE_DOWNSAMPLE)
    {
      switch (precision)
	{
	case BSE_RESAMPLER2_PREC_48DB:   return create_impl_with_coeffs <Downsampler2<16, USE_SSE> > (halfband_fir_48db_coeffs, 16, 1.0);
	case BSE_RESAMPLER2_PREC_72DB:   return create_impl_with_coeffs <Downsampler2<24, USE_SSE> > (halfband_fir_72db_coeffs, 24, 1.0);
	case BSE_RESAMPLER2_PREC_96DB:   return create_impl_with_coeffs <Downsampler2<32, USE_SSE> > (halfband_fir_96db_coeffs, 32, 1.0);
	case BSE_RESAMPLER2_PREC_120DB:  return create_impl_with_coeffs <Downsampler2<42, USE_SSE> > (halfband_fir_120db_coeffs, 42, 1.0);
	case BSE_RESAMPLER2_PREC_144DB:  return create_impl_with_coeffs <Downsampler2<52, USE_SSE> > (halfband_fir_144db_coeffs, 52, 1.0);
	}
    }
  return 0;
}

} // Resampler
} // Bse

#endif /* __BSE_RESAMPLER_TCC__ */
