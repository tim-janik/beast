/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2006 Tim Janik
 * Copyright (C) 2006 Stefan Westerfeld
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <bse/bsecxxplugin.hh>
#include <bse/bseblockutils.hh>
#ifndef __SSE__
#error  SSE support is required for this plugin.
#endif
#include <xmmintrin.h>

#define ALIGNMENT16(pointer) (0xf & (ptrdiff_t) (pointer))
#define ALIGNED16(pointer)   (!ALIGNMENT16 (pointer))
#ifndef _mm_extract_ss
#define _mm_extract_ss(_M128)   ({ float result; _mm_store_ss (&result, _M128); result; })
#endif

namespace {

using std::max;
using std::min;

class BlockImpl : virtual public Bse::Block::Impl {
  union F4Vector
  {
    __m128 m;
    float  f[4];
  };

  virtual void
  add (guint        n_values,
       float       *ovalues,
       const float *ivalues)
  {
    guint upos = 0, n_vectors = 0;
    if (ALIGNMENT16 (ovalues) == ALIGNMENT16 (ivalues) && n_values > 8)
      {
        /* loop until aligned */
        for (upos = 0;
             upos < n_values && (!ALIGNED16 (&ovalues[upos]) ||
                                 !ALIGNED16 (&ivalues[upos]));
             upos++)
          ovalues[upos] += ivalues[upos];
        /* loop while aligned */
        const __m128 *ivalues_m = (const __m128*) &ivalues[upos];
        __m128 *ovalues_m = (__m128*) (&ovalues[upos]);
        n_vectors = (n_values - upos) / 4;
        for (guint spos = 0; spos < n_vectors; spos++)
          ovalues_m[spos] = _mm_add_ps (ovalues_m[spos], ivalues_m[spos]);
      }
    /* loop while unaligned */
    for (upos += n_vectors * 4; upos < n_values; upos++)
      ovalues[upos] += ivalues[upos];
  }
  virtual void
  sub (guint        n_values,
       float       *ovalues,
       const float *ivalues)
  {
    guint upos = 0, n_vectors = 0;
    if (ALIGNMENT16 (ovalues) == ALIGNMENT16 (ivalues) && n_values > 8)
      {
        /* loop until aligned */
        for (upos = 0;
             upos < n_values && (!ALIGNED16 (&ovalues[upos]) ||
                                 !ALIGNED16 (&ivalues[upos]));
             upos++)
          ovalues[upos] -= ivalues[upos];
        /* loop while aligned */
        const __m128 *ivalues_m = (const __m128*) &ivalues[upos];
        __m128 *ovalues_m = (__m128*) (&ovalues[upos]);
        n_vectors = (n_values - upos) / 4;
        for (guint spos = 0; spos < n_vectors; spos++)
          ovalues_m[spos] = _mm_sub_ps (ovalues_m[spos], ivalues_m[spos]);
      }
    /* loop while unaligned */
    for (upos += n_vectors * 4; upos < n_values; upos++)
      ovalues[upos] -= ivalues[upos];
  }
  virtual void
  mul (guint        n_values,
       float       *ovalues,
       const float *ivalues)
  {
    guint upos = 0, n_vectors = 0;
    if (ALIGNMENT16 (ovalues) == ALIGNMENT16 (ivalues) && n_values > 8)
      {
        /* loop until aligned */
        for (upos = 0;
             upos < n_values && (!ALIGNED16 (&ovalues[upos]) ||
                                 !ALIGNED16 (&ivalues[upos]));
             upos++)
          ovalues[upos] *= ivalues[upos];
        /* loop while aligned */
        const __m128 *ivalues_m = (const __m128*) &ivalues[upos];
        __m128 *ovalues_m = (__m128*) (&ovalues[upos]);
        n_vectors = (n_values - upos) / 4;
        for (guint spos = 0; spos < n_vectors; spos++)
          ovalues_m[spos] = _mm_mul_ps (ovalues_m[spos], ivalues_m[spos]);
      }
    /* loop while unaligned */
    for (upos += n_vectors * 4; upos < n_values; upos++)
      ovalues[upos] *= ivalues[upos];
  }
  virtual void
  scale (guint        n_values,
         float       *ovalues,
         const float *ivalues,
         const float  level)
  {
    guint upos = 0, n_vectors = 0;
    if (ALIGNMENT16 (ovalues) == ALIGNMENT16 (ivalues) && n_values > 8)
      {
        /* loop until aligned */
        for (upos = 0;
             upos < n_values && (!ALIGNED16 (&ovalues[upos]) ||
                                 !ALIGNED16 (&ivalues[upos]));
             upos++)
          ovalues[upos] = ivalues[upos] * level;
        /* loop while aligned */
        const __m128 level_m = _mm_set1_ps (level);
        const __m128 *ivalues_m = (const __m128*) &ivalues[upos];
        __m128 *ovalues_m = (__m128 *) &ovalues[upos];
        n_vectors = (n_values - upos) / 4;
        for (guint spos = 0; spos < n_vectors; spos++)
          ovalues_m[spos] = _mm_mul_ps (ivalues_m[spos], level_m);
      }
    /* loop while unaligned */
    for (upos += n_vectors * 4; upos < n_values; upos++)
      ovalues[upos] = ivalues[upos] * level;
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
    if (LIKELY (n_values))
      {
	minv = maxv = ivalues[0];

	guint upos = 0, n_vectors = 0;
	if (n_values > 8)
	  {
	    /* loop until aligned */
	    for (upos = 0; upos < n_values && !ALIGNED16 (&ivalues[upos]); upos++)
	      {
		if (UNLIKELY (ivalues[upos] < minv))
		  minv = ivalues[upos];
		if (UNLIKELY (ivalues[upos] > maxv))
		  maxv = ivalues[upos];
	      }
	    /* n_vectors must be >= 1 if n_values was > 8 */
	    n_vectors = (n_values - upos) / 4;
	    g_assert (n_vectors > 0);
	    /* loop while aligned */
	    const __m128 *ivalues_m = (const __m128*) &ivalues[upos];
	    __m128 min_m = ivalues_m[0];
	    __m128 max_m = ivalues_m[0];
	    for (guint spos = 1; spos < n_vectors; spos++)
	      {
		min_m = _mm_min_ps (min_m, ivalues_m[spos]);
		max_m = _mm_max_ps (max_m, ivalues_m[spos]);
	      }
	    F4Vector f4v;
	    f4v.m = min_m;
	    minv = min (minv, min (min (f4v.f[0], f4v.f[1]), min (f4v.f[2], f4v.f[3])));
	    f4v.m = max_m;
	    maxv = max (maxv, max (max (f4v.f[0], f4v.f[1]), max (f4v.f[2], f4v.f[3])));
	  }
	/* loop while unaligned */
	for (upos += n_vectors * 4; upos < n_values; upos++)
	  {
	    if (UNLIKELY (ivalues[upos] < minv))
	      minv = ivalues[upos];
	    if (UNLIKELY (ivalues[upos] > maxv))
	      maxv = ivalues[upos];
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
    guint upos = 0, n_vectors = 0;
    if (n_values > 8)
      {
        /* loop until aligned */
        for (upos = 0; upos < n_values && !ALIGNED16 (&ivalues[upos]); upos++)
          square_sum += ivalues[upos] * ivalues[upos];
	/* n_vectors must be >= 1 if n_values was > 8 */
        n_vectors = (n_values - upos) / 4;
	g_assert (n_vectors > 0);
        /* loop while aligned */
        const __m128 *ivalues_m = (const __m128*) &ivalues[upos];
	__m128 square_sum_m = _mm_mul_ps (ivalues_m[0], ivalues_m[0]);
	for (guint spos = 1; spos < n_vectors; spos++)
	  square_sum_m = _mm_add_ps (square_sum_m, _mm_mul_ps (ivalues_m[spos], ivalues_m[spos]));
        /* sum up all 4 vector fields */
        __m128 vsum = _mm_shuffle_ps (square_sum_m, square_sum_m, _MM_SHUFFLE (0, 1, 2, 3));
        vsum = _mm_add_ps (vsum, square_sum_m); /* { 0+3, 1+2, 2+1, 3+0, } */
        square_sum_m = _mm_movehl_ps (square_sum_m, vsum);
        vsum = _mm_add_ps (vsum, square_sum_m); /* { 0+3+2+1, 1+2+3+0, 2+1+2+1. 3+0+3+0 } */
        square_sum += _mm_extract_ss (vsum);
      }
    /* loop while unaligned */
    for (upos += n_vectors * 4; upos < n_values; upos++)
      square_sum += ivalues[upos] * ivalues[upos];
    return square_sum;
  }
  virtual float
  range_and_square_sum (guint        n_values,
                        const float *ivalues,
	                float&       min_value,
	                float&       max_value)
  {
    float minv, maxv, square_sum = 0;
    if (LIKELY (n_values))
      {
	minv = maxv = ivalues[0];

	guint upos = 0, n_vectors = 0;
	if (n_values > 8)
	  {
	    /* loop until aligned */
	    for (upos = 0; upos < n_values && !ALIGNED16 (&ivalues[upos]); upos++)
	      {
		square_sum += ivalues[upos] * ivalues[upos];
		if (UNLIKELY (ivalues[upos] < minv))
		  minv = ivalues[upos];
		if (UNLIKELY (ivalues[upos] > maxv))
		  maxv = ivalues[upos];
	      }
	    /* n_vectors must be >= 1 if n_values was > 8 */
	    n_vectors = (n_values - upos) / 4;
	    g_assert (n_vectors > 0);
	    /* loop while aligned */
	    const __m128 *ivalues_m = (const __m128*) &ivalues[upos];
	    n_vectors = (n_values - upos) / 4;
	    __m128 square_sum_m = _mm_mul_ps (ivalues_m[0], ivalues_m[0]);
	    __m128 min_m = ivalues_m[0];
	    __m128 max_m = ivalues_m[0];
	    for (guint spos = 1; spos < n_vectors; spos++)
	      {
		square_sum_m = _mm_add_ps (square_sum_m, _mm_mul_ps (ivalues_m[spos], ivalues_m[spos]));
		min_m = _mm_min_ps (min_m, ivalues_m[spos]);
		max_m = _mm_max_ps (max_m, ivalues_m[spos]);
	      }
	    F4Vector f4v;
	    f4v.m = square_sum_m;
	    square_sum += f4v.f[0] + f4v.f[1] + f4v.f[2] + f4v.f[3];
	    f4v.m = min_m;
	    minv = min (minv, min (min (f4v.f[0], f4v.f[1]), min (f4v.f[2], f4v.f[3])));
	    f4v.m = max_m;
	    maxv = max (maxv, max (max (f4v.f[0], f4v.f[1]), max (f4v.f[2], f4v.f[3])));
	  }
	/* loop while unaligned */
	for (upos += n_vectors * 4; upos < n_values; upos++)
	  {
	    square_sum += ivalues[upos] * ivalues[upos];
	    if (UNLIKELY (ivalues[upos] < minv))
	      minv = ivalues[upos];
	    if (UNLIKELY (ivalues[upos] > maxv))
	      maxv = ivalues[upos];
	  }
      }
    else
      {
	minv = maxv = 0;
      }
    min_value = minv;
    max_value = maxv;
    return square_sum;
  }
public:
  void
  hookup ()
  {
    Bse::Block::Impl::substitute (this);
  }
};
static BlockImpl sse_block_impl;
} // Anon

namespace Bse {

class CoreHook {
public:
  void
  run ()
  {
    sse_block_impl.hookup();
  }
};

BSE_CXX_DEFINE_EXPORTS();
BSE_CXX_REGISTER_STATIC_HOOK (CoreHook);

} // Bse
