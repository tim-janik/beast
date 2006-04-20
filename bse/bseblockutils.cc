/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2006 Tim Janik
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
#include "bseblockutils.hh"

namespace {
class BlockImpl : virtual public Bse::Block::Impl {
  virtual void
  add (guint        n_values,
       float       *ovalues,
       const float *ivalues)
  {
    guint upos;
    for (upos = 0; upos < n_values; upos++)
      ovalues[upos] += ivalues[upos];
  }
  virtual void
  scale (guint        n_values,
         float       *ovalues,
         const float *ivalues,
         const float  level)
  {
    guint upos;
    for (upos = 0; upos < n_values; upos++)
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

extern "C" void
bse_block_add_floats (guint          n_values,
                      float         *ovalues,
                      const float   *ivalues)
{
  Bse::Block::add (n_values, ovalues, ivalues);
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
