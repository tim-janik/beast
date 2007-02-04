/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2006 Tim Janik
 * Copyright (C) 2006 Stefan Westerfeld
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "bseblockutils.hh"
#include "bseresampler.hh"
#include "bseresamplerimpl.hh"

namespace {
class BlockImpl : virtual public Bse::Block::Impl {
  virtual const char*
  impl_name ()
  {
    return "FPU";
  }
  virtual void
  add (guint        n_values,
       float       *ovalues,
       const float *ivalues)
  {
    for (guint i = 0; i < n_values; i++)
      ovalues[i] += ivalues[i];
  }
  virtual void
  sub (guint        n_values,
       float       *ovalues,
       const float *ivalues)
  {
    for (guint i = 0; i < n_values; i++)
      ovalues[i] -= ivalues[i];
  }
  virtual void
  mul (guint        n_values,
       float       *ovalues,
       const float *ivalues)
  {
    for (guint i = 0; i < n_values; i++)
      ovalues[i] *= ivalues[i];
  }
  virtual void
  scale (guint        n_values,
         float       *ovalues,
         const float *ivalues,
         const float  level)
  {
    for (guint i = 0; i < n_values; i++)
      ovalues[i] = ivalues[i] * level;
  }
  virtual void
  interleave2 (guint	       n_ivalues,
               float          *ovalues,         /* length_ovalues = n_ivalues * 2 */
               const float    *ivalues,
               guint           offset)          /* 0=left, 1=right */
  {
    int n = n_ivalues;
    ovalues += offset;
    for (int pos = 0; pos < n; pos++)
      ovalues[pos * 2] = ivalues[pos];
  }
  virtual void
  interleave2_add (guint           n_ivalues,
                   float          *ovalues,	/* length_ovalues = n_ivalues * 2 */
                   const float    *ivalues,
                   guint           offset)      /* 0=left, 1=right */
  {
    int n = n_ivalues;
    ovalues += offset;
    for (int pos = 0; pos < n; pos++)
      ovalues[pos * 2] += ivalues[pos];
  }
  virtual void
  range (guint        n_values,
         const float *ivalues,
	 float&       min_value,
	 float&       max_value)
  {
    float minv, maxv;
    if (n_values)
      {
	minv = maxv = ivalues[0];

	for (guint i = 1; i < n_values; i++)
	  {
	    if (UNLIKELY (ivalues[i] < minv))
	      minv = ivalues[i];
	    if (UNLIKELY (ivalues[i] > maxv))
	      maxv = ivalues[i];
	  }
      }
    else
      {
	minv = maxv = 0;
      }
    min_value = minv;
    max_value = maxv;
  }
  virtual float
  square_sum (guint        n_values,
              const float *ivalues)
  {
    float square_sum = 0.0;

    for (guint i = 0; i < n_values; i++)
      square_sum += ivalues[i] * ivalues[i];

    return square_sum;
  }
  virtual float
  range_and_square_sum (guint        n_values,
                        const float *ivalues,
	                float&       min_value,
	                float&       max_value)
  {
    float square_sum;
    float minv, maxv;
    if (n_values)
      {
	minv = maxv = ivalues[0];
	square_sum = ivalues[0] * ivalues[0];

	for (guint i = 1; i < n_values; i++)
	  {
	    square_sum += ivalues[i] * ivalues[i];

	    if (UNLIKELY (ivalues[i] < minv))
	      minv = ivalues[i];
	    if (UNLIKELY (ivalues[i] > maxv))
	      maxv = ivalues[i];
	  }
      }
    else
      {
	minv = maxv = square_sum = 0.0;
      }
    min_value = minv;
    max_value = maxv;
    return square_sum;
  }
  virtual Bse::Resampler::Resampler2*
  create_resampler2 (BseResampler2Mode      mode,
                     BseResampler2Precision precision)
  {
    struct FPUResampler2 : public Bse::Resampler::Resampler2 {
      static inline Resampler2*
      create_resampler (BseResampler2Mode      mode,
                        BseResampler2Precision precision)
      {
        return create_impl<false> (mode, precision);
      }
    };
    return FPUResampler2::create_resampler (mode, precision);
  }
  virtual bool
  test_resampler2 (bool verbose)
  {
    /* there is currently only a test for the SSE filter code, so in the
     * non-SSE-case we simply always return true
     */
    g_print ("SSE filter implementation not tested: no SSE support available\n");
    return true;
  }
};
static BlockImpl default_block_impl;
} // Anon

namespace Bse {

Block::Impl*
Block::default_singleton ()
{
  return &default_block_impl;
}

Block::Impl *Block::singleton = &default_block_impl;

Block::Impl*
Block::current_singleton ()
{
  return Block::singleton;
}

Block::Impl::~Impl()
{}

void
Block::Impl::substitute (Impl *substitute_impl)
{
  if (!substitute_impl)
    substitute_impl = &default_block_impl;
  Block::singleton = substitute_impl;
}

} // Bse

extern "C" const char*
bse_block_impl_name (void)
{
  return Bse::Block::impl_name();
}

extern "C" void
bse_block_add_floats (guint          n_values,
                      float         *ovalues,
                      const float   *ivalues)
{
  Bse::Block::add (n_values, ovalues, ivalues);
}

extern "C" void
bse_block_sub_floats (guint          n_values,
                      float         *ovalues,
                      const float   *ivalues)
{
  Bse::Block::sub (n_values, ovalues, ivalues);
}

extern "C" void
bse_block_mul_floats (guint          n_values,
                      float         *ovalues,
                      const float   *ivalues)
{
  Bse::Block::mul (n_values, ovalues, ivalues);
}

void
bse_block_scale_floats (guint           n_values,
                        float         *ovalues,
                        const float   *ivalues,
                        const float    level)
{
  Bse::Block::scale (n_values, ovalues, ivalues, level);
}

extern "C" void
bse_block_interleave2_floats (guint	   n_ivalues,
			      float       *ovalues,	  /* length_ovalues = n_ivalues * 2 */
			      const float *ivalues,
			      guint        offset)	  /* 0=left, 1=right */
{
  Bse::Block::interleave2 (n_ivalues, ovalues, ivalues, offset);
}

extern "C" void
bse_block_interleave2_add_floats (guint	       n_ivalues,
				  float       *ovalues,	  /* length_ovalues = n_ivalues * 2 */
				  const float *ivalues,
				  guint        offset)	  /* 0=left, 1=right */
{
  Bse::Block::interleave2_add (n_ivalues, ovalues, ivalues, offset);
}

extern "C" void
bse_block_calc_float_range (guint          n_values,
                            const float   *ivalues,
			    float         *min_value,
			    float         *max_value)
{
  Bse::Block::range (n_values, ivalues, *min_value, *max_value);
}

extern "C" float
bse_block_calc_float_square_sum (guint          n_values,
                                 const float   *ivalues)
{
  return Bse::Block::square_sum (n_values, ivalues);
}

extern "C" float
bse_block_calc_float_range_and_square_sum (guint          n_values,
                                           const float   *ivalues,
					   float         *min_value,
					   float         *max_value)
{
  return Bse::Block::range_and_square_sum (n_values, ivalues, *min_value, *max_value);
}
