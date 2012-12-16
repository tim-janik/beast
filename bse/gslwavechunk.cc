/* GSL - Generic Sound Layer
 * Copyright (C) 2001-2002 Tim Janik
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
#include "gslwavechunk.h"
#include "bsemain.h"
#include "gslcommon.h"
#include "gsldatahandle.h"
#include "bsemathsignal.h"
#include <string.h>


/* --- macros --- */
#define	PRINT_DEBUG_INFO		(0)
#define	STATIC_ZERO_SIZE		(4096)
#define	PBLOCK_SIZE(pad, n_channels)	(MAX (2 * (pad), (n_channels) * BSE_CONFIG (wave_chunk_big_pad)))

#define	PHASE_NORM(wchunk)		((GslWaveChunkMem*) (0))
#define	PHASE_NORM_BACKWARD(wchunk)	((GslWaveChunkMem*) (+1))
#define	PHASE_UNDEF(wchunk)		((GslWaveChunkMem*) (+2))
#define	PHASE_HEAD(wchunk)		(&(wchunk)->head)
#define	PHASE_ENTER(wchunk)		(&(wchunk)->enter)
#define	PHASE_WRAP(wchunk)		(&(wchunk)->wrap)
#define	PHASE_PPWRAP(wchunk)		(&(wchunk)->ppwrap)
#define	PHASE_LEAVE(wchunk)		(&(wchunk)->leave)
#define	PHASE_TAIL(wchunk)		(&(wchunk)->tail)


/* --- typedefs & structures --- */
typedef struct {
  GslLong pos;			/* input */
  GslLong rel_pos;
  GslLong lbound, ubound;	/* PHASE_NORM/_BACKWARD */
} Iter;
typedef struct {
  GslLong one;
  GslLong dir;
  GslLong pos;
  GslLong loop_count;
} WPos;


/* --- variables --- */
static gfloat static_zero_block[STATIC_ZERO_SIZE] = { 0, };	// FIXME


/* --- functions --- */
static inline void
wpos_step (GslWaveChunk *wchunk,
	   WPos         *wpos)
{
  wpos->pos += wpos->dir;
  if (wpos->loop_count)
    {
      if (wchunk->loop_type == GSL_WAVE_LOOP_PINGPONG)
	{
	  if (wpos->dir < 0 &&
	      wpos->pos == wchunk->loop_first + wpos->dir)
	    {
	      wpos->loop_count--;
	      wpos->dir = -wpos->dir;
	      wpos->pos = wchunk->loop_first + wpos->dir;
	    }
	  else if (wpos->pos == wchunk->loop_last + wpos->dir)
	    {
	      wpos->loop_count--;
	      wpos->dir = -wpos->dir;
	      wpos->pos = wchunk->loop_last + wpos->dir;
	    }
	}
      else
	{
	  if (wpos->pos == wchunk->loop_last + wpos->one && wpos->loop_count)
	    {
	      wpos->loop_count--;
	      wpos->pos = wchunk->loop_first;
	    }
	}
    }
}

static void
fill_block (GslWaveChunk *wchunk,
	    gfloat	 *block,
	    GslLong	  offset,
	    guint	  length,
	    gboolean	  backward,
	    guint	  loop_count)
{
  GslLong dcache_length = gsl_data_handle_length (wchunk->dcache->dhandle);
  guint i, dnode_length = wchunk->dcache->node_size;
  GslDataCacheNode *dnode;
  WPos wpos;
  
  wpos.one = wchunk->n_channels;
  wpos.dir = 1; // wchunk->n_channels;
  if (backward)
    wpos.dir = -wpos.dir;
  wpos.pos = offset;
  wpos.loop_count = loop_count;
  dnode = gsl_data_cache_ref_node (wchunk->dcache, 0, GSL_DATA_CACHE_DEMAND_LOAD);
  for (i = 0; i < length; i++)
    {
      GslLong offset = wpos.pos;
      
      if (offset < 0 || offset >= dcache_length)
	block[i] = 0;
      else
	{
	  if (offset < dnode->offset || offset >= dnode->offset + dnode_length)
	    {
	      gsl_data_cache_unref_node (wchunk->dcache, dnode);
	      dnode = gsl_data_cache_ref_node (wchunk->dcache, offset, GSL_DATA_CACHE_DEMAND_LOAD);
	    }
	  block[i] = dnode->data[offset - dnode->offset];
	}
      wpos_step (wchunk, &wpos);
    }
  gsl_data_cache_unref_node (wchunk->dcache, dnode);
}

static gfloat*
create_block_for_offset (GslWaveChunk *wchunk,
			 GslLong       offset,
			 guint         length)
{
  GslLong padding = wchunk->n_pad_values;
  GslLong one = wchunk->n_channels;
  GslLong wave_last = wchunk->length - one;
  GslLong loop_width = wchunk->loop_last - wchunk->loop_first;
  gfloat *mem;
  GslLong l, j, k;
  
  if (wchunk->loop_type != GSL_WAVE_LOOP_PINGPONG)
    loop_width += one;
  
  l = length + 2 * padding;
  mem = sfi_new_struct (gfloat, l);
  offset -= padding;
  j = ((wchunk->wave_length - one - offset) -
       (wchunk->pploop_ends_backwards ? wchunk->loop_first : wave_last - wchunk->loop_last));
  if (j >= 0)
    {
      k = j / loop_width;
      /* g_print ("endoffset-setup: j=%ld %%=%ld, k=%ld, k&1=%ld\n", j, j % loop_width, k, k & 1); */
      j %= loop_width;
      if (wchunk->loop_type == GSL_WAVE_LOOP_PINGPONG)
	{
	  if (wchunk->pploop_ends_backwards && (k & 1))
	    fill_block (wchunk, mem, wchunk->loop_last - j, l, FALSE, k);
	  else if (wchunk->pploop_ends_backwards)
	    fill_block (wchunk, mem, wchunk->loop_first + j, l, TRUE, k);
	  else if (k & 1)
	    fill_block (wchunk, mem, wchunk->loop_first + j, l, TRUE, k);
	  else
	    fill_block (wchunk, mem, wchunk->loop_last - j, l, FALSE, k);
	}
      else
	fill_block (wchunk, mem, wchunk->loop_last - j, l, FALSE, k);
    }
  else if (wchunk->pploop_ends_backwards)
    fill_block (wchunk, mem, wchunk->loop_first + j, l, TRUE, 0);
  else
    fill_block (wchunk, mem, wchunk->loop_last - j, l, FALSE, 0);
  return mem + padding;
}

static void
setup_pblocks (GslWaveChunk *wchunk)
{
  GslLong padding = wchunk->n_pad_values;
  GslLong big_pad = PBLOCK_SIZE (wchunk->n_pad_values, wchunk->n_channels);
  GslLong loop_width = wchunk->loop_last - wchunk->loop_first;
  GslLong one = wchunk->n_channels;
  GslLong loop_duration, wave_last = wchunk->length - one;
  gfloat *mem;
  guint l;
  
  if (wchunk->loop_type != GSL_WAVE_LOOP_PINGPONG)
    loop_width += one;
  loop_duration = loop_width * wchunk->loop_count;
  
  wchunk->head.first = -padding;
  wchunk->head.last = big_pad;
  wchunk->head.length = wchunk->head.last - wchunk->head.first + one;
  wchunk->tail_start_norm = wave_last - big_pad;
  wchunk->tail.first = wchunk->tail_start_norm + loop_duration;
  wchunk->tail.last = wchunk->tail.first + big_pad + padding;
  wchunk->tail.length = wchunk->tail.last - wchunk->tail.first + one;
  if (wchunk->loop_type)
    {
      wchunk->enter.first = wchunk->loop_last - padding;
      wchunk->enter.last = wchunk->loop_last + one + big_pad;
      wchunk->wrap.first = loop_width - padding;
      wchunk->wrap.last = big_pad;
      if (wchunk->loop_type == GSL_WAVE_LOOP_PINGPONG)
	{
	  wchunk->enter.last -= one;
	  wchunk->wrap.last -= one;
	  wchunk->ppwrap.first = wchunk->wrap.first;
	  wchunk->ppwrap.last = wchunk->wrap.last + loop_width;
	  wchunk->ppwrap.length = wchunk->ppwrap.last - wchunk->ppwrap.first + one;
	  wchunk->wrap.length = loop_width - wchunk->wrap.first + wchunk->wrap.last + one;
	  wchunk->wrap.first += loop_width;
	}
      else
	wchunk->wrap.length = loop_width - wchunk->wrap.first + wchunk->wrap.last + one;
      wchunk->leave_end_norm = wchunk->loop_last + big_pad;
      wchunk->leave.first = wchunk->loop_last + loop_duration - padding;
      wchunk->leave.last = wchunk->leave_end_norm + loop_duration;
      if (wchunk->mini_loop)
	{
	  wchunk->leave.first -= wchunk->wrap.length + padding;
	  wchunk->enter.last += wchunk->wrap.length + padding;
	}
      wchunk->leave.length = wchunk->leave.last - wchunk->leave.first + one;
      wchunk->enter.length = wchunk->enter.last - wchunk->enter.first + one;
      if (wchunk->pploop_ends_backwards)
	{
	  wchunk->tail.first += wchunk->loop_last - wave_last + wchunk->loop_first;
	  wchunk->tail.last += wchunk->loop_last - wave_last + wchunk->loop_first;
	  wchunk->tail_start_norm = 0 + big_pad;
	  wchunk->leave_end_norm = wchunk->loop_first - big_pad;
	}
    }
  else
    {
      wchunk->enter.first = wchunk->tail.first;
      wchunk->enter.last = wchunk->head.last;
      wchunk->enter.length = 0;
      wchunk->wrap.first = wchunk->tail.last + 1;
      wchunk->wrap.last = wchunk->head.first - 1;
      wchunk->wrap.length = 0;
      wchunk->ppwrap.first = wchunk->tail.last + 1;
      wchunk->ppwrap.last = wchunk->head.first - 1;
      wchunk->ppwrap.length = 0;
      wchunk->leave.first = wchunk->tail.first;
      wchunk->leave.last = wchunk->tail.last;
      wchunk->leave_end_norm = 0;
      wchunk->leave.length = 0;
    }
  
  l = wchunk->head.length + 2 * padding;
  mem = sfi_new_struct (gfloat, l);
  fill_block (wchunk, mem, wchunk->head.first - padding, l, FALSE, wchunk->loop_count);
  wchunk->head.mem = mem + padding;
  if (wchunk->loop_type)
    {
      l = wchunk->enter.length + 2 * padding;
      mem = sfi_new_struct (gfloat, l);
      fill_block (wchunk, mem, wchunk->enter.first - padding, l, FALSE, wchunk->loop_count);
      wchunk->enter.mem = mem + padding;
      if (wchunk->loop_type == GSL_WAVE_LOOP_PINGPONG)
	{
	  wchunk->wrap.mem = create_block_for_offset (wchunk, wchunk->loop_last + one + wchunk->wrap.first, wchunk->wrap.length);
	  wchunk->ppwrap.mem = create_block_for_offset (wchunk, wchunk->loop_last + one + wchunk->ppwrap.first, wchunk->ppwrap.length);
	}
      else
	{
	  l = wchunk->wrap.length + 2 * padding;
	  mem = sfi_new_struct (gfloat, l);
	  fill_block (wchunk, mem, wchunk->loop_first + wchunk->wrap.first - padding, l, FALSE, wchunk->loop_count - 1);
	  wchunk->wrap.mem = mem + padding;
	}
      wchunk->leave.mem = create_block_for_offset (wchunk, wchunk->leave.first, wchunk->leave.length);
    }
  wchunk->tail.mem = create_block_for_offset (wchunk, wchunk->tail.first, wchunk->tail.length);
}

static inline GslWaveChunkMem*
wave_identify_offset (GslWaveChunk *wchunk,
		      Iter         *iter)
{
  GslLong pos = iter->pos;
  GslLong one = wchunk->n_channels;
  
  if (UNLIKELY (pos < wchunk->head.first))					/* outside wave boundaries */
    {
      iter->lbound = 0;
      iter->rel_pos = wchunk->n_pad_values;
      iter->ubound = iter->rel_pos + MIN (STATIC_ZERO_SIZE - 2 * wchunk->n_pad_values, wchunk->head.first - pos);
      if (PRINT_DEBUG_INFO)
	g_print ("PHASE_UNDEF, pre-head %lld %lld %lld\n", iter->lbound, iter->rel_pos, iter->ubound);
      return PHASE_UNDEF (wchunk);
    }
  if (UNLIKELY (pos > wchunk->tail.last))					/* outside wave boundaries */
    {
      iter->lbound = 0;
      iter->rel_pos = wchunk->n_pad_values;
      iter->ubound = iter->rel_pos + MIN (STATIC_ZERO_SIZE - 2 * wchunk->n_pad_values, pos - wchunk->tail.last);
      if (PRINT_DEBUG_INFO)
	g_print ("PHASE_UNDEF, post-tail %lld %lld %lld\n", iter->lbound, iter->rel_pos, iter->ubound);
      return PHASE_UNDEF (wchunk);
    }
  if (pos <= wchunk->head.last)
    {
      iter->rel_pos = pos - wchunk->head.first;
      if (PRINT_DEBUG_INFO)
	g_print ("PHASE_HEAD %lld %lld %lld\n", wchunk->head.first, iter->rel_pos, wchunk->head.last);
      return PHASE_HEAD (wchunk);
    }
  else if (pos <= wchunk->enter.last)					/* before loop */
    {
      if (pos >= wchunk->enter.first)
	{
	  iter->rel_pos = pos - wchunk->enter.first;
	  if (PRINT_DEBUG_INFO)
	    g_print ("PHASE_ENTER %lld %lld %lld\n", wchunk->enter.first, iter->rel_pos, wchunk->enter.last);
	  return PHASE_ENTER (wchunk);
	}
      iter->rel_pos = pos - wchunk->head.last;
      iter->lbound = wchunk->head.last;
      iter->ubound = wchunk->enter.first;
      if (PRINT_DEBUG_INFO)
	g_print ("PHASE_NORM, pre-enter %lld %lld %lld\n", iter->lbound, iter->rel_pos, iter->ubound);
      return PHASE_NORM (wchunk);
    }
  else if (pos >= wchunk->tail.first)
    {
      iter->rel_pos = pos - wchunk->tail.first;
      if (PRINT_DEBUG_INFO)
	g_print ("PHASE_TAIL %lld %lld %lld\n", wchunk->tail.first, iter->rel_pos, wchunk->tail.last);
      return PHASE_TAIL (wchunk);
    }
  else if (pos >= wchunk->leave.first)				/* after loop */
    {
      if (pos <= wchunk->leave.last)
	{
	  iter->rel_pos = pos - wchunk->leave.first;
	  if (PRINT_DEBUG_INFO)
	    g_print ("PHASE_LEAVE %lld %lld %lld\n", wchunk->leave.first, iter->rel_pos, wchunk->leave.last);
	  return PHASE_LEAVE (wchunk);
	}
      iter->rel_pos = pos - wchunk->leave.last;
      if (wchunk->pploop_ends_backwards)
	{
	  iter->lbound = wchunk->tail_start_norm;
	  iter->ubound = wchunk->leave_end_norm;
	  if (PRINT_DEBUG_INFO)
	    g_print ("PHASE_NORM_BACKWARD, post-leave %lld %lld %lld\n", iter->lbound, iter->rel_pos, iter->ubound);
	  return PHASE_NORM_BACKWARD (wchunk);
	}
      else
	{
	  iter->lbound = wchunk->leave_end_norm;
	  iter->ubound = wchunk->tail_start_norm;
	  if (PRINT_DEBUG_INFO)
	    g_print ("PHASE_NORM, post-leave %lld %lld %lld\n", iter->lbound, iter->rel_pos, iter->ubound);
	  return PHASE_NORM (wchunk);
	}
    }
  else if (wchunk->loop_type == GSL_WAVE_LOOP_PINGPONG)		/* in ping-pong loop */
    {
      guint loop_width = wchunk->loop_last - wchunk->loop_first;
      
      pos -= wchunk->loop_last + one;
      pos %= 2 * loop_width;
      if (pos <= wchunk->ppwrap.last)
	{
	  if (pos <= wchunk->wrap.last)
	    {
	      iter->rel_pos = wchunk->wrap.length - one - wchunk->wrap.last + pos;
	      if (PRINT_DEBUG_INFO)
		g_print ("PHASE_WRAP %lld %lld %lld\n", wchunk->wrap.first, iter->rel_pos, wchunk->wrap.last);
	      return PHASE_WRAP (wchunk);
	    }
	  if (pos >= wchunk->ppwrap.first)
	    {
	      iter->rel_pos = pos - wchunk->ppwrap.first;
	      if (PRINT_DEBUG_INFO)
		g_print ("PHASE_PPWRAP %lld %lld %lld\n", wchunk->ppwrap.first, iter->rel_pos, wchunk->ppwrap.last);
	      return PHASE_PPWRAP (wchunk);
	    }
	  iter->ubound = wchunk->loop_last - one - wchunk->wrap.last;
	  iter->lbound = wchunk->loop_last - one - wchunk->ppwrap.first;
	  iter->rel_pos = pos - wchunk->wrap.last;
	  if (PRINT_DEBUG_INFO)
	    g_print ("PHASE_NORM_BACKWARD, pploop %lld %lld %lld\n", iter->lbound, iter->rel_pos, iter->ubound);
	  return PHASE_NORM_BACKWARD (wchunk);
	}
      if (pos >= wchunk->wrap.first)
	{
	  iter->rel_pos = pos - wchunk->wrap.first;
	  if (PRINT_DEBUG_INFO)
	    g_print ("PHASE_WRAP %lld %lld %lld\n", wchunk->wrap.first, iter->rel_pos, wchunk->wrap.last);
	  return PHASE_WRAP (wchunk);
	}
      iter->rel_pos = pos - wchunk->ppwrap.last;
      iter->ubound = wchunk->loop_first + one + wchunk->wrap.first - loop_width;
      iter->lbound = wchunk->loop_first + one + wchunk->ppwrap.last - loop_width;
      if (PRINT_DEBUG_INFO)
	g_print ("PHASE_NORM, pploop %lld %lld %lld\n", iter->lbound, iter->rel_pos, iter->ubound);
      return PHASE_NORM (wchunk);
    }
  else if (wchunk->loop_type == GSL_WAVE_LOOP_JUMP)		/* in jump loop */
    {
      guint loop_width = wchunk->loop_last - wchunk->loop_first + one;
      
      pos -= wchunk->loop_last + one;
      pos %= loop_width;
      if (pos >= wchunk->wrap.first)
	{
	  iter->rel_pos = pos - wchunk->wrap.first;
	  if (PRINT_DEBUG_INFO)
	    g_print ("PHASE_WRAP %lld %lld %lld\n", wchunk->wrap.first, iter->rel_pos, wchunk->wrap.last);
	  return PHASE_WRAP (wchunk);
	}
      if (pos <= wchunk->wrap.last)
	{
	  iter->rel_pos = wchunk->wrap.length - one - wchunk->wrap.last + pos;
	  if (PRINT_DEBUG_INFO)
	    g_print ("PHASE_WRAP %lld %lld %lld\n", wchunk->wrap.first, iter->rel_pos, wchunk->wrap.last);
	  return PHASE_WRAP (wchunk);
	}
      iter->rel_pos = pos - wchunk->wrap.last;
      iter->lbound = wchunk->loop_first + wchunk->wrap.last;
      iter->ubound = wchunk->loop_first + wchunk->wrap.first;
      if (PRINT_DEBUG_INFO)
	g_print ("PHASE_NORM, jloop %lld %lld %lld\n", iter->lbound, iter->rel_pos, iter->ubound);
      return PHASE_NORM (wchunk);
    }
  iter->rel_pos = pos - wchunk->head.last;
  iter->lbound = wchunk->head.last;
  iter->ubound = wchunk->enter.first;
  if (PRINT_DEBUG_INFO)
    g_print ("PHASE_NORM, noloop %lld %lld %lld\n", iter->lbound, iter->rel_pos, iter->ubound);
  return PHASE_NORM (wchunk);
}

void
gsl_wave_chunk_use_block (GslWaveChunk      *wchunk,
			  GslWaveChunkBlock *block)
{
  GslWaveChunkMem *phase;
  GslLong one;
  Iter iter = { 0, };
  gboolean reverse;
  
  g_return_if_fail (wchunk != NULL);
  g_return_if_fail (wchunk->open_count > 0);
  g_return_if_fail (block != NULL);
  g_return_if_fail (wchunk->dcache != NULL);
  g_return_if_fail (block->node == NULL);
  g_return_if_fail (block->play_dir == -1 || block->play_dir == +1);
  
  block->offset /= wchunk->n_channels;
  block->offset *= wchunk->n_channels;
  
  one = wchunk->n_channels;
  reverse = block->play_dir < 0;
  iter.pos = block->offset;
  phase = wave_identify_offset (wchunk, &iter);
  
  block->is_silent = FALSE;
  if (phase <= PHASE_UNDEF (wchunk))
    {
      GslDataCacheNode *dnode;
      guint offset;
      
      if (phase == PHASE_UNDEF (wchunk))
	{
	  block->is_silent = TRUE;
	  reverse = FALSE;
	  block->length = (iter.ubound - iter.rel_pos) / wchunk->n_channels;
	  block->length *= wchunk->n_channels;
	  g_assert (block->length <= STATIC_ZERO_SIZE - 2 * wchunk->n_pad_values);
	  block->start = static_zero_block + iter.rel_pos;
	}
      else
	{
	  GslLong max_length;
	  
	  if (phase == PHASE_NORM_BACKWARD (wchunk))
	    {
	      offset = iter.ubound - iter.rel_pos;
	      reverse = !reverse;
	    }
	  else
	    offset = iter.lbound + iter.rel_pos;
	  max_length = reverse ? offset - iter.lbound : iter.ubound - offset;
	  dnode = gsl_data_cache_ref_node (wchunk->dcache, offset, GSL_DATA_CACHE_DEMAND_LOAD);
	  offset -= dnode->offset;
	  block->start = dnode->data + offset;
	  if (reverse)
	    {
	      block->length = 1 + offset / wchunk->n_channels;
	      block->length *= wchunk->n_channels;
	    }
	  else
	    {
	      block->length = (wchunk->dcache->node_size - offset) / wchunk->n_channels;
	      block->length *= wchunk->n_channels;
	    }
	  block->length = MIN (block->length, max_length);
	  block->node = dnode;
	}
    }
  else
    {
      block->start = phase->mem + iter.rel_pos;
      if (reverse)
	block->length = one + iter.rel_pos;
      else
	block->length = phase->length - iter.rel_pos;
    }
  if (reverse)
    {
      block->dirstride = -wchunk->n_channels;
      block->end = block->start - block->length;
    }
  else
    {
      block->dirstride = +wchunk->n_channels;
      block->end = block->start + block->length;
    }
  g_assert (block->length > 0);
  /* we might want to partly reset this at some point to implement
   * truely infinite loops
   */
  block->next_offset = block->offset + (block->play_dir > 0 ? block->length : -block->length);
}

void
gsl_wave_chunk_unuse_block (GslWaveChunk      *wchunk,
			    GslWaveChunkBlock *block)
{
  g_return_if_fail (wchunk != NULL);
  g_return_if_fail (block != NULL);
  g_return_if_fail (wchunk->dcache != NULL);
  
  if (block->node)
    {
      gsl_data_cache_unref_node (wchunk->dcache, (GslDataCacheNode*) block->node);
      block->node = NULL;
    }
}

static void
wave_chunk_setup_loop (GslWaveChunk *wchunk)
{
  GslWaveLoopType loop_type = wchunk->requested_loop_type;
  GslLong loop_first = wchunk->requested_loop_first;
  GslLong loop_last = wchunk->requested_loop_last;
  guint loop_count = wchunk->requested_loop_count;
  GslLong one, padding, big_pad;
  
  g_return_if_fail (wchunk->open_count > 0);
  
  one = wchunk->n_channels;
  padding = wchunk->n_pad_values;
  big_pad = PBLOCK_SIZE (wchunk->n_pad_values, wchunk->n_channels);
  
  /* check validity */
  if (loop_count < 1 || loop_first < 0 || loop_last < 0 || wchunk->length < 1)
    loop_type = GSL_WAVE_LOOP_NONE;
  
  /* setup loop types */
  switch (loop_type)
    {
    case GSL_WAVE_LOOP_JUMP:
      loop_first /= wchunk->n_channels;
      loop_last /= wchunk->n_channels;
      if (loop_last >= wchunk->length ||
	  loop_first >= loop_last)
	goto CASE_DONT_LOOP;
      wchunk->loop_type = loop_type;
      wchunk->loop_first = loop_first * wchunk->n_channels;
      wchunk->loop_last = loop_last * wchunk->n_channels;
      wchunk->loop_count = (G_MAXINT - wchunk->length) / (wchunk->loop_last - wchunk->loop_first + one);
      wchunk->loop_count = MIN (wchunk->loop_count, loop_count);
      wchunk->wave_length = wchunk->length + (wchunk->loop_last - wchunk->loop_first + one) * wchunk->loop_count;
      break;
    case GSL_WAVE_LOOP_PINGPONG:
      loop_first /= wchunk->n_channels;
      loop_last /= wchunk->n_channels;
      if (loop_last >= wchunk->length ||
	  loop_first >= loop_last)
	goto CASE_DONT_LOOP;
      wchunk->loop_type = loop_type;
      wchunk->loop_first = loop_first * wchunk->n_channels;
      wchunk->loop_last = loop_last * wchunk->n_channels;
      wchunk->loop_count = (G_MAXINT - wchunk->loop_last - one) / (wchunk->loop_last - wchunk->loop_first);
      wchunk->loop_count = MIN (wchunk->loop_count, loop_count);
      wchunk->wave_length = wchunk->loop_last + one + (wchunk->loop_last - wchunk->loop_first) * wchunk->loop_count;
      if (wchunk->loop_count & 1)	/* FIXME */
	wchunk->wave_length += wchunk->loop_first;
      else
	wchunk->wave_length += wchunk->length - one - wchunk->loop_last;
      break;
    CASE_DONT_LOOP:
      loop_type = GSL_WAVE_LOOP_NONE;
    case GSL_WAVE_LOOP_NONE:
      wchunk->loop_type = loop_type;
      wchunk->loop_first = wchunk->length + 1;
      wchunk->loop_last = -1;
      wchunk->loop_count = 0;
      wchunk->wave_length = wchunk->length;
      break;
    }
  wchunk->pploop_ends_backwards = wchunk->loop_type == GSL_WAVE_LOOP_PINGPONG && (wchunk->loop_count & 1);
  wchunk->mini_loop = wchunk->loop_type && wchunk->loop_last - wchunk->loop_first < 2 * big_pad + padding;
}

GslWaveChunk*
gsl_wave_chunk_new (GslDataCache   *dcache,
                    gfloat          mix_freq,
                    gfloat          osc_freq,
                    GslWaveLoopType loop_type,
		    GslLong         loop_first,
		    GslLong         loop_last,
		    guint           loop_count)
{
  GslWaveChunk *wchunk;
  
  g_return_val_if_fail (dcache != NULL, NULL);
  g_return_val_if_fail (osc_freq < mix_freq / 2, NULL);
  g_return_val_if_fail (loop_type >= GSL_WAVE_LOOP_NONE && loop_type <= GSL_WAVE_LOOP_PINGPONG, NULL);
  
  wchunk = sfi_new_struct0 (GslWaveChunk, 1);
  wchunk->dcache = gsl_data_cache_ref (dcache);
  wchunk->length = 0;
  wchunk->n_channels = 0;
  wchunk->n_pad_values = 0;
  wchunk->wave_length = 0;
  wchunk->loop_type = GSL_WAVE_LOOP_NONE;
  wchunk->leave_end_norm = 0;
  wchunk->tail_start_norm = 0;
  wchunk->ref_count = 1;
  wchunk->open_count = 0;
  wchunk->mix_freq = mix_freq;
  wchunk->osc_freq = osc_freq;
  wchunk->volume_adjust = 0.0;	    /* will be set in gsl_wave_chunk_open */
  wchunk->fine_tune_factor = 0.0;   /* will be set in gsl_wave_chunk_open */
  wchunk->requested_loop_type = loop_type;
  wchunk->requested_loop_first = loop_first;
  wchunk->requested_loop_last = loop_last;
  wchunk->requested_loop_count = loop_count;
  
  return wchunk;
}

GslWaveChunk*
gsl_wave_chunk_ref (GslWaveChunk *wchunk)
{
  g_return_val_if_fail (wchunk != NULL, NULL);
  g_return_val_if_fail (wchunk->ref_count > 0, NULL);
  
  wchunk->ref_count++;
  return wchunk;
}

void
gsl_wave_chunk_unref (GslWaveChunk *wchunk)
{
  g_return_if_fail (wchunk != NULL);
  g_return_if_fail (wchunk->ref_count > 0);
  
  wchunk->ref_count--;
  if (wchunk->ref_count == 0)
    {
      g_return_if_fail (wchunk->open_count == 0);
      gsl_data_cache_unref (wchunk->dcache);
      sfi_delete_struct (GslWaveChunk, wchunk);
    }
}

BseErrorType
gsl_wave_chunk_open (GslWaveChunk *wchunk)
{
  g_return_val_if_fail (wchunk != NULL, BSE_ERROR_INTERNAL);
  g_return_val_if_fail (wchunk->ref_count > 0, BSE_ERROR_INTERNAL);
  
  if (wchunk->open_count == 0)
    {
      BseErrorType error;
      
      error = gsl_data_handle_open (wchunk->dcache->dhandle);
      if (error != BSE_ERROR_NONE)
	return error;
      if (gsl_data_handle_n_values (wchunk->dcache->dhandle) < gsl_data_handle_n_channels (wchunk->dcache->dhandle))
	{
	  gsl_data_handle_close (wchunk->dcache->dhandle);
	  return BSE_ERROR_FILE_EMPTY;
	}
      wchunk->mix_freq = gsl_data_handle_mix_freq (wchunk->dcache->dhandle);
      wchunk->osc_freq = gsl_data_handle_osc_freq (wchunk->dcache->dhandle);
      wchunk->n_channels = gsl_data_handle_n_channels (wchunk->dcache->dhandle);
      wchunk->length = gsl_data_handle_n_values (wchunk->dcache->dhandle) / wchunk->n_channels;
      wchunk->length *= wchunk->n_channels;
      wchunk->n_pad_values = BSE_CONFIG (wave_chunk_padding) * wchunk->n_channels;
      wchunk->volume_adjust = gsl_data_handle_volume (wchunk->dcache->dhandle);
      wchunk->fine_tune_factor = bse_cent_tune (gsl_data_handle_fine_tune (wchunk->dcache->dhandle));
      gsl_data_cache_open (wchunk->dcache);
      gsl_data_handle_close (wchunk->dcache->dhandle);
      g_return_val_if_fail (wchunk->dcache->padding >= wchunk->n_pad_values, BSE_ERROR_INTERNAL);
      wchunk->open_count++;
      wchunk->ref_count++;
      wave_chunk_setup_loop (wchunk);
      setup_pblocks (wchunk);
    }
  else
    wchunk->open_count++;
  return BSE_ERROR_NONE;
}

void
gsl_wave_chunk_close (GslWaveChunk *wchunk)
{
  GslLong padding;
  
  g_return_if_fail (wchunk != NULL);
  g_return_if_fail (wchunk->open_count > 0);
  g_return_if_fail (wchunk->ref_count > 0);
  
  wchunk->open_count--;
  if (wchunk->open_count)
    return;
  
  padding = wchunk->n_pad_values;
  gsl_data_cache_close (wchunk->dcache);
  if (wchunk->head.mem)
    sfi_delete_structs (gfloat, wchunk->head.length + 2 * padding, wchunk->head.mem - padding);
  memset (&wchunk->head, 0, sizeof (GslWaveChunkMem));
  if (wchunk->enter.mem)
    sfi_delete_structs (gfloat, wchunk->enter.length + 2 * padding, wchunk->enter.mem - padding);
  memset (&wchunk->enter, 0, sizeof (GslWaveChunkMem));
  if (wchunk->wrap.mem)
    sfi_delete_structs (gfloat, wchunk->wrap.length + 2 * padding, wchunk->wrap.mem - padding);
  memset (&wchunk->wrap, 0, sizeof (GslWaveChunkMem));
  if (wchunk->ppwrap.mem)
    sfi_delete_structs (gfloat, wchunk->ppwrap.length + 2 * padding, wchunk->ppwrap.mem - padding);
  memset (&wchunk->ppwrap, 0, sizeof (GslWaveChunkMem));
  if (wchunk->leave.mem)
    sfi_delete_structs (gfloat, wchunk->leave.length + 2 * padding, wchunk->leave.mem - padding);
  memset (&wchunk->leave, 0, sizeof (GslWaveChunkMem));
  if (wchunk->tail.mem)
    sfi_delete_structs (gfloat, wchunk->tail.length + 2 * padding, wchunk->tail.mem - padding);
  memset (&wchunk->tail, 0, sizeof (GslWaveChunkMem));
  wchunk->length = 0;
  wchunk->n_channels = 0;
  wchunk->n_pad_values = 0;
  wchunk->wave_length = 0;
  wchunk->loop_type = GSL_WAVE_LOOP_NONE;
  wchunk->leave_end_norm = 0;
  wchunk->tail_start_norm = 0;
  wchunk->volume_adjust = 0.0;
  wchunk->fine_tune_factor = 0.0;
  gsl_wave_chunk_unref (wchunk);
}

void
gsl_wave_chunk_debug_block (GslWaveChunk *wchunk,
			    GslLong       offset,
			    GslLong       length,
			    gfloat	 *block)
{
  g_return_if_fail (wchunk != NULL);
  
  fill_block (wchunk, block, offset, length, FALSE, wchunk->loop_count);
}

GslWaveChunk*
_gsl_wave_chunk_copy (GslWaveChunk *wchunk)
{
  g_return_val_if_fail (wchunk != NULL, NULL);
  g_return_val_if_fail (wchunk->ref_count > 0, NULL);
  
  return gsl_wave_chunk_new (wchunk->dcache,
                             wchunk->mix_freq,
                             wchunk->osc_freq,
                             wchunk->loop_type,
			     wchunk->loop_first,
			     wchunk->loop_last,
			     wchunk->loop_count);
}

const gchar*
gsl_wave_loop_type_to_string (GslWaveLoopType wave_loop)
{
  g_return_val_if_fail (wave_loop >= GSL_WAVE_LOOP_NONE && wave_loop <= GSL_WAVE_LOOP_PINGPONG, NULL);
  
  switch (wave_loop)
    {
    case GSL_WAVE_LOOP_NONE:		return "none";
    case GSL_WAVE_LOOP_JUMP:		return "jump";
    case GSL_WAVE_LOOP_PINGPONG:	return "pingpong";
    default:				return NULL;
    }
}

GslWaveLoopType
gsl_wave_loop_type_from_string (const gchar *string)
{
  g_return_val_if_fail (string != NULL, GSL_WAVE_LOOP_NONE);
  
  while (*string == ' ')
    string++;
  if (strncasecmp (string, "jump", 4) == 0)
    return GSL_WAVE_LOOP_JUMP;
  if (strncasecmp (string, "pingpong", 8) == 0)
    return GSL_WAVE_LOOP_PINGPONG;
  return GSL_WAVE_LOOP_NONE;
}
