/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2006 Tim Janik
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

namespace {
class BlockImpl : virtual public Bse::Block::Impl {
  virtual void
  add (guint        n_values,
       float       *ovalues,
       const float *ivalues)
  {
    guint upos = 0, n_vectors = 0;
    if (ALIGNMENT16 (ovalues) == ALIGNMENT16 (ivalues) && n_values > 8)
      {
        /* loop until unaligned */
        for (upos = 0;
             upos < n_values && (!ALIGNED16 (&ovalues[upos]) ||
                                 !ALIGNED16 (&ivalues[upos]));
             upos++)
          ovalues[upos] += ivalues[upos];
        /* loop while aligned */
        const __m128 *ivalues_m = (const __m128*) &ivalues[upos];
        __m128 *ovalues_m = (__m128*) (&ovalues[upos]);
        guint spos, n_vectors = (n_values - upos) / 4;
        for (spos = 0; spos < n_vectors; spos++)
          ovalues_m[spos] = _mm_add_ps (ovalues_m[spos], ivalues_m[spos]);
      }
    /* loop while unaligned */
    for (upos += n_vectors * 4; upos < n_values; upos++)
      ovalues[upos] += ivalues[upos];
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
        /* loop until unaligned */
        for (upos = 0;
             upos < n_values && (!ALIGNED16 (&ovalues[upos]) ||
                                 !ALIGNED16 (&ivalues[upos]));
             upos++)
          ovalues[upos] = ivalues[upos] * level;
        /* loop while aligned */
        const __m128 level_m = _mm_set1_ps (level);
        const __m128 *ivalues_m = (const __m128*) &ivalues[upos];
        __m128 *ovalues_m = (__m128 *) &ovalues[upos];
        guint spos, n_vectors = (n_values - upos) / 4;
        for (spos = 0; spos < n_vectors; spos++)
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
