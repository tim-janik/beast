/* GSL - Generic Sound Layer
 * Copyright (C) 2001 Tim Janik
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
#include "gslwavechunk.h"

#include "gslcommon.h"
#include "gsldatahandle.h"


/* --- macros --- */
#define	PRINT_DEBUG_INFO		(0)
#define	STATIC_ZERO_SIZE		(4096)
#define	PBLOCK_SIZE(pad, n_channels)	(MAX (2 * (pad), (n_channels) * gsl_get_config ()->wave_chunk_big_pad))

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
  GslLong dir;
  GslLong pos;
  GslLong loop_count;
} WPos;


/* --- prototypes --- */
static void	wave_chunk_set_loop	(GslWaveChunk   *wchunk,
					 GslWaveLoopType loop_type,
					 GslLong         loop_start,
					 GslLong         loop_end,
					 guint           loop_count);
static void	wave_chunk_open		(GslWaveChunk	*wchunk);
static void	wave_chunk_close	(GslWaveChunk	*wchunk);


/* --- variables --- */
static gfloat static_zero_block[STATIC_ZERO_SIZE] = { 0, };


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
	      wpos->pos == wchunk->loop_start + wpos->dir)
	    {
	      wpos->loop_count--;
	      wpos->dir = -wpos->dir;
	      wpos->pos = wchunk->loop_start + wpos->dir;
	    }
	  else if (wpos->pos == wchunk->loop_end + wpos->dir)
	    {
	      wpos->loop_count--;
	      wpos->dir = -wpos->dir;
	      wpos->pos = wchunk->loop_end + wpos->dir;
	    }
	}
      else
	{
	  if (wpos->pos == wchunk->loop_end + wpos->dir && wpos->loop_count)
	    {
	      wpos->loop_count--;
	      wpos->pos = wchunk->loop_start;
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
  GslLong dcache_length = wchunk->dcache->handle->n_values;
  guint i, dnode_length = wchunk->dcache->node_size;
  GslDataCacheNode *dnode;
  WPos wpos;

  wpos.dir = wchunk->n_channels;
  if (backward)
    wpos.dir = -wpos.dir;
  wpos.pos = offset;
  wpos.loop_count = loop_count;
  dnode = gsl_data_cache_ref_node (wchunk->dcache, 0, TRUE);
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
	      dnode = gsl_data_cache_ref_node (wchunk->dcache, offset, TRUE);
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
  GslLong loop_width = wchunk->loop_end - wchunk->loop_start;
  gfloat *mem;
  GslLong l, j, k;

  if (wchunk->loop_type != GSL_WAVE_LOOP_PINGPONG)
    loop_width += one;
  
  l = length + 2 * padding;
  mem = gsl_new_struct (gfloat, l);
  offset -= padding;
  j = ((wchunk->wave_length - one - offset) -
       (wchunk->pploop_ends_backwards ? wchunk->loop_start : wave_last - wchunk->loop_end));
  if (j >= 0)
    {
      k = j / loop_width;
      /* g_print ("endoffset-setup: j=%ld %%=%ld, k=%ld, k&1=%ld\n", j, j % loop_width, k, k & 1); */
      j %= loop_width;
      if (wchunk->loop_type == GSL_WAVE_LOOP_PINGPONG)
	{
	  if (wchunk->pploop_ends_backwards && (k & 1))
	    fill_block (wchunk, mem, wchunk->loop_end - j, l, FALSE, k);
	  else if (wchunk->pploop_ends_backwards)
	    fill_block (wchunk, mem, wchunk->loop_start + j, l, TRUE, k);
	  else if (k & 1)
	    fill_block (wchunk, mem, wchunk->loop_start + j, l, TRUE, k);
	  else
	    fill_block (wchunk, mem, wchunk->loop_end - j, l, FALSE, k);
	}
      else
	fill_block (wchunk, mem, wchunk->loop_end - j, l, FALSE, k);
    }
  else if (wchunk->pploop_ends_backwards)
    fill_block (wchunk, mem, wchunk->loop_start + j, l, TRUE, 0);
  else
    fill_block (wchunk, mem, wchunk->loop_end - j, l, FALSE, 0);
  return mem + padding;
}

static void
setup_pblocks (GslWaveChunk *wchunk)
{
  GslLong padding = wchunk->n_pad_values;
  GslLong big_pad = PBLOCK_SIZE (wchunk->n_pad_values, wchunk->n_channels);
  GslLong loop_width = wchunk->loop_end - wchunk->loop_start;
  GslLong one = wchunk->n_channels;
  GslLong loop_duration, wave_last = wchunk->length - one;
  gfloat *mem;
  guint l;

  if (wchunk->loop_type != GSL_WAVE_LOOP_PINGPONG)
    loop_width += one;
  loop_duration = loop_width * wchunk->loop_count;

  wchunk->head.start = -padding;
  wchunk->head.end = big_pad;
  wchunk->head.length = wchunk->head.end - wchunk->head.start + one;
  wchunk->tail_start_norm = wave_last - big_pad;
  wchunk->tail.start = wchunk->tail_start_norm + loop_duration;
  wchunk->tail.end = wchunk->tail.start + big_pad + padding;
  wchunk->tail.length = wchunk->tail.end - wchunk->tail.start + one;
  if (wchunk->loop_type)
    {
      wchunk->enter.start = wchunk->loop_end - padding;
      wchunk->enter.end = wchunk->loop_end + one + big_pad;
      wchunk->wrap.start = loop_width - padding;
      wchunk->wrap.end = big_pad;
      if (wchunk->loop_type == GSL_WAVE_LOOP_PINGPONG)
	{
	  wchunk->enter.end -= one;
	  wchunk->wrap.end -= one;
	  wchunk->ppwrap.start = wchunk->wrap.start;
	  wchunk->ppwrap.end = wchunk->wrap.end + loop_width;
	  wchunk->ppwrap.length = wchunk->ppwrap.end - wchunk->ppwrap.start + one;
	  wchunk->wrap.length = loop_width - wchunk->wrap.start + wchunk->wrap.end + one;
	  wchunk->wrap.start += loop_width;
	}
      else
	wchunk->wrap.length = loop_width - wchunk->wrap.start + wchunk->wrap.end + one;
      wchunk->leave_end_norm = wchunk->loop_end + big_pad;
      wchunk->leave.start = wchunk->loop_end + loop_duration - padding;
      wchunk->leave.end = wchunk->leave_end_norm + loop_duration;
      if (wchunk->mini_loop)
	{
	  wchunk->leave.start -= wchunk->wrap.length + padding;
	  wchunk->enter.end += wchunk->wrap.length + padding;
	}
      wchunk->leave.length = wchunk->leave.end - wchunk->leave.start + one;
      wchunk->enter.length = wchunk->enter.end - wchunk->enter.start + one;
      if (wchunk->pploop_ends_backwards)
	{
	  wchunk->tail.start += wchunk->loop_end - wave_last + wchunk->loop_start;
	  wchunk->tail.end += wchunk->loop_end - wave_last + wchunk->loop_start;
	  wchunk->tail_start_norm = 0 + big_pad;
	  wchunk->leave_end_norm = wchunk->loop_start - big_pad;
	}
    }
  else
    {
      wchunk->enter.start = wchunk->head.end;
      wchunk->enter.end = wchunk->head.end;
      wchunk->enter.length = 0;
      wchunk->wrap.start = wchunk->tail.end + 1;
      wchunk->wrap.end = wchunk->head.start - 1;
      wchunk->wrap.length = 0;
      wchunk->ppwrap.start = wchunk->tail.end + 1;
      wchunk->ppwrap.end = wchunk->head.start - 1;
      wchunk->ppwrap.length = 0;
      wchunk->leave.start = wchunk->tail.start;
      wchunk->leave.end = wchunk->tail.end;
      wchunk->leave_end_norm = 0;
      wchunk->leave.length = 0;
    }

  l = wchunk->head.length + 2 * padding;
  mem = gsl_new_struct (gfloat, l);
  fill_block (wchunk, mem, wchunk->head.start - padding, l, FALSE, wchunk->loop_count);
  wchunk->head.mem = mem + padding;
  if (wchunk->loop_type)
    {
      l = wchunk->enter.length + 2 * padding;
      mem = gsl_new_struct (gfloat, l);
      fill_block (wchunk, mem, wchunk->enter.start - padding, l, FALSE, wchunk->loop_count);
      wchunk->enter.mem = mem + padding;
      if (wchunk->loop_type == GSL_WAVE_LOOP_PINGPONG)
	{
	  wchunk->wrap.mem = create_block_for_offset (wchunk, wchunk->loop_end + one + wchunk->wrap.start, wchunk->wrap.length);
	  wchunk->ppwrap.mem = create_block_for_offset (wchunk, wchunk->loop_end + one + wchunk->ppwrap.start, wchunk->ppwrap.length);
	}
      else
	{
	  l = wchunk->wrap.length + 2 * padding;
	  mem = gsl_new_struct (gfloat, l);
	  fill_block (wchunk, mem, wchunk->loop_start + wchunk->wrap.start - padding, l, FALSE, wchunk->loop_count - 1);
	  wchunk->wrap.mem = mem + padding;
	}
      wchunk->leave.mem = create_block_for_offset (wchunk, wchunk->leave.start, wchunk->leave.length);
    }
  wchunk->tail.mem = create_block_for_offset (wchunk, wchunk->tail.start, wchunk->tail.length);
}

static inline GslWaveChunkMem*
wave_identify_offset (GslWaveChunk *wchunk,
		      Iter         *iter)
{
  GslLong pos = iter->pos;
  GslLong one = wchunk->n_channels;

  if (pos < wchunk->head.start)					/* outside wave boundaries */
    {
      iter->lbound = 0;
      iter->rel_pos = wchunk->n_pad_values;
      iter->ubound = iter->rel_pos + MIN (STATIC_ZERO_SIZE - 2 * wchunk->n_pad_values, wchunk->head.start - pos);
      if (PRINT_DEBUG_INFO)
	g_print ("PHASE_UNDEF, pre-head %ld %ld %ld\n", iter->lbound, iter->rel_pos, iter->ubound);
      return PHASE_UNDEF (wchunk);
    }
  if (pos > wchunk->tail.end)					/* outside wave boundaries */
    {
      iter->lbound = 0;
      iter->rel_pos = wchunk->n_pad_values;
      iter->ubound = iter->rel_pos + MIN (STATIC_ZERO_SIZE - 2 * wchunk->n_pad_values, pos - wchunk->tail.end);
      if (PRINT_DEBUG_INFO)
	g_print ("PHASE_UNDEF, post-tail %ld %ld %ld\n", iter->lbound, iter->rel_pos, iter->ubound);
      return PHASE_UNDEF (wchunk);
    }
  if (pos <= wchunk->enter.end)					/* before loop */
    {
      if (pos <= wchunk->head.end)
	{
	  iter->rel_pos = pos - wchunk->head.start;
	  if (PRINT_DEBUG_INFO)
	    g_print ("PHASE_HEAD %ld %ld %ld\n", wchunk->head.start, iter->rel_pos, wchunk->head.end);
	  return PHASE_HEAD (wchunk);
	}
      if (pos >= wchunk->enter.start)
	{
	  iter->rel_pos = pos - wchunk->enter.start;
	  if (PRINT_DEBUG_INFO)
	    g_print ("PHASE_ENTER %ld %ld %ld\n", wchunk->enter.start, iter->rel_pos, wchunk->enter.end);
	  return PHASE_ENTER (wchunk);
	}
      iter->rel_pos = pos - wchunk->head.end;
      iter->lbound = wchunk->head.end;
      iter->ubound = wchunk->enter.start;
      if (PRINT_DEBUG_INFO)
	g_print ("PHASE_NORM, pre-enter %ld %ld %ld\n", iter->lbound, iter->rel_pos, iter->ubound);
      return PHASE_NORM (wchunk);
    }
  else if (pos >= wchunk->leave.start)				/* after loop */
    {
      if (pos >= wchunk->tail.start)
	{
	  iter->rel_pos = pos - wchunk->tail.start;
	  if (PRINT_DEBUG_INFO)
	    g_print ("PHASE_TAIL %ld %ld %ld\n", wchunk->tail.start, iter->rel_pos, wchunk->tail.end);
	  return PHASE_TAIL (wchunk);
	}
      if (pos <= wchunk->leave.end)
	{
	  iter->rel_pos = pos - wchunk->leave.start;
	  if (PRINT_DEBUG_INFO)
	    g_print ("PHASE_LEAVE %ld %ld %ld\n", wchunk->leave.start, iter->rel_pos, wchunk->leave.end);
	  return PHASE_LEAVE (wchunk);
	}
      iter->rel_pos = pos - wchunk->leave.end;
      if (wchunk->pploop_ends_backwards)
	{
	  iter->lbound = wchunk->tail_start_norm;
	  iter->ubound = wchunk->leave_end_norm;
	  if (PRINT_DEBUG_INFO)
	    g_print ("PHASE_NORM_BACKWARD, post-leave %ld %ld %ld\n", iter->lbound, iter->rel_pos, iter->ubound);
	  return PHASE_NORM_BACKWARD (wchunk);
	}
      else
	{
	  iter->lbound = wchunk->leave_end_norm;
	  iter->ubound = wchunk->tail_start_norm;
	  if (PRINT_DEBUG_INFO)
	    g_print ("PHASE_NORM, post-leave %ld %ld %ld\n", iter->lbound, iter->rel_pos, iter->ubound);
	  return PHASE_NORM (wchunk);
	}
    }
  else if (wchunk->loop_type == GSL_WAVE_LOOP_PINGPONG)		/* in ping-pong loop */
    {
      guint loop_width = wchunk->loop_end - wchunk->loop_start;

      pos -= wchunk->loop_end + one;
      pos %= 2 * loop_width;
      if (pos <= wchunk->ppwrap.end)
	{
	  if (pos <= wchunk->wrap.end)
	    {
	      iter->rel_pos = wchunk->wrap.length - one - wchunk->wrap.end + pos;
	      if (PRINT_DEBUG_INFO)
		g_print ("PHASE_WRAP %ld %ld %ld\n", wchunk->wrap.start, iter->rel_pos, wchunk->wrap.end);
	      return PHASE_WRAP (wchunk);
	    }
	  if (pos >= wchunk->ppwrap.start)
	    {
	      iter->rel_pos = pos - wchunk->ppwrap.start;
	      if (PRINT_DEBUG_INFO)
		g_print ("PHASE_PPWRAP %ld %ld %ld\n", wchunk->ppwrap.start, iter->rel_pos, wchunk->ppwrap.end);
	      return PHASE_PPWRAP (wchunk);
	    }
	  iter->ubound = wchunk->loop_end - one - wchunk->wrap.end;
	  iter->lbound = wchunk->loop_end - one - wchunk->ppwrap.start;
	  iter->rel_pos = pos - wchunk->wrap.end;
	  if (PRINT_DEBUG_INFO)
	    g_print ("PHASE_NORM_BACKWARD, pploop %ld %ld %ld\n", iter->lbound, iter->rel_pos, iter->ubound);
	  return PHASE_NORM_BACKWARD (wchunk);
	}
      if (pos >= wchunk->wrap.start)
	{
	  iter->rel_pos = pos - wchunk->wrap.start;
	  if (PRINT_DEBUG_INFO)
	    g_print ("PHASE_WRAP %ld %ld %ld\n", wchunk->wrap.start, iter->rel_pos, wchunk->wrap.end);
	  return PHASE_WRAP (wchunk);
	}
      iter->rel_pos = pos - wchunk->ppwrap.end;
      iter->ubound = wchunk->loop_start + one + wchunk->wrap.start - loop_width;
      iter->lbound = wchunk->loop_start + one + wchunk->ppwrap.end - loop_width;
      if (PRINT_DEBUG_INFO)
	g_print ("PHASE_NORM, pploop %ld %ld %ld\n", iter->lbound, iter->rel_pos, iter->ubound);
      return PHASE_NORM (wchunk);
    }
  else if (wchunk->loop_type == GSL_WAVE_LOOP_JUMP)		/* in jump loop */
    {
      guint loop_width = wchunk->loop_end - wchunk->loop_start + one;

      pos -= wchunk->loop_end + one;
      pos %= loop_width;
      if (pos >= wchunk->wrap.start)
	{
	  iter->rel_pos = pos - wchunk->wrap.start;
	  if (PRINT_DEBUG_INFO)
	    g_print ("PHASE_WRAP %ld %ld %ld\n", wchunk->wrap.start, iter->rel_pos, wchunk->wrap.end);
	  return PHASE_WRAP (wchunk);
	}
      if (pos <= wchunk->wrap.end)
	{
	  iter->rel_pos = wchunk->wrap.length - one - wchunk->wrap.end + pos;
	  if (PRINT_DEBUG_INFO)
	    g_print ("PHASE_WRAP %ld %ld %ld\n", wchunk->wrap.start, iter->rel_pos, wchunk->wrap.end);
	  return PHASE_WRAP (wchunk);
	}
      iter->rel_pos = pos - wchunk->wrap.end;
      iter->lbound = wchunk->loop_start + wchunk->wrap.end;
      iter->ubound = wchunk->loop_start + wchunk->wrap.start;
      if (PRINT_DEBUG_INFO)
	g_print ("PHASE_NORM, jloop %ld %ld %ld\n", iter->lbound, iter->rel_pos, iter->ubound);
      return PHASE_NORM (wchunk);
    }
  iter->rel_pos = pos - wchunk->head.end;
  iter->lbound = wchunk->head.end;
  iter->ubound = wchunk->enter.start;
  if (PRINT_DEBUG_INFO)
    g_print ("PHASE_NORM, noloop %ld %ld %ld\n", iter->lbound, iter->rel_pos, iter->ubound);
  return PHASE_NORM (wchunk);
}

void
gsl_wave_chunk_use_block (GslWaveChunk      *wchunk,
			  GslWaveChunkBlock *block)
{
  GslWaveChunkMem *phase;
  GslLong one;
  Iter iter;
  gboolean reverse;

  g_return_if_fail (wchunk != NULL);
  g_return_if_fail (block != NULL);
  g_return_if_fail (wchunk->dcache != NULL);
  g_return_if_fail (block->node == NULL);
  g_return_if_fail (block->play_dir == -1 || block->play_dir == +1);

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
	  guint max_length;

	  if (phase == PHASE_NORM_BACKWARD (wchunk))
	    {
	      offset = iter.ubound - iter.rel_pos;
	      reverse = !reverse;
	    }
	  else
	    offset = iter.lbound + iter.rel_pos;
	  max_length = reverse ? offset - iter.lbound : iter.ubound - offset;
	  offset += wchunk->offset;
	  dnode = gsl_data_cache_ref_node (wchunk->dcache, offset, TRUE); /* FIXME: demand_load */
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
      if (0)
	g_print ("phase: head=%u enter=%u wrap=%u leave=%u tail=%u\n",
		 phase == &wchunk->head, phase == &wchunk->enter,
		 phase == &wchunk->wrap, phase == &wchunk->leave,
		 phase == &wchunk->tail);
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
      gsl_data_cache_unref_node (wchunk->dcache, block->node);
      block->node = NULL;
    }
}

GslWaveChunk*
_gsl_wave_chunk_create (GslDataCache   *dcache,
			GslLong         offset,
			GslLong         n_values, /* per channel */
			guint           n_channels,
			gfloat          osc_freq,
			gfloat          mix_freq,
			GslWaveLoopType loop_type,
			GslLong         loop_start,
			GslLong         loop_end,
			guint           loop_count)
{
  GslWaveChunk *wchunk;

  g_return_val_if_fail (dcache != NULL, NULL);
  g_return_val_if_fail (offset >= 0, NULL);
  g_return_val_if_fail (n_values > 0, NULL);
  g_return_val_if_fail (n_channels > 0, NULL);
  g_return_val_if_fail (offset + n_values * n_channels <= dcache->handle->n_values, NULL);
  g_return_val_if_fail (osc_freq < mix_freq / 2, NULL);
  g_return_val_if_fail (dcache->padding >= gsl_get_config ()->wave_chunk_padding * n_channels, NULL);

  wchunk = gsl_new_struct0 (GslWaveChunk, 1);
  wchunk->dcache = gsl_data_cache_ref (dcache);
  wchunk->owner_data = NULL;
  wchunk->offset = offset;
  wchunk->n_channels = n_channels;
  wchunk->length = n_values * wchunk->n_channels;
  wchunk->mix_freq = mix_freq;
  wchunk->osc_freq = osc_freq;
  wchunk->n_pad_values = gsl_get_config ()->wave_chunk_padding * n_channels;
  wchunk->wave_length = wchunk->length;
  wchunk->loop_type = 0;
  wchunk->loop_start = wchunk->length + 1;
  wchunk->loop_end = -1;
  wchunk->loop_count = 0;
  wchunk->leave_end_norm = 0;
  wchunk->tail_start_norm = 0;
  wave_chunk_set_loop (wchunk, loop_type, loop_start, loop_end, loop_count);
  wave_chunk_open (wchunk);

  return wchunk;
}

void
_gsl_wave_chunk_destroy (GslWaveChunk *wchunk)
{
  g_return_if_fail (wchunk != NULL);

  wave_chunk_close (wchunk);
  gsl_data_cache_unref (wchunk->dcache);
  gsl_delete_struct (GslWaveChunk, 1, wchunk);
}

static void
wave_chunk_open (GslWaveChunk *wchunk)
{
  g_return_if_fail (wchunk != NULL);

  gsl_data_cache_open (wchunk->dcache);
  setup_pblocks (wchunk);
}

static void
wave_chunk_close (GslWaveChunk *wchunk)
{
  GslLong padding;

  g_return_if_fail (wchunk != NULL);

  padding = wchunk->n_pad_values;
  gsl_data_cache_close (wchunk->dcache);
  if (wchunk->head.mem)
    gsl_delete_struct (gfloat, wchunk->head.length + 2 * padding, wchunk->head.mem - padding);
  memset (&wchunk->head, 0, sizeof (GslWaveChunkMem));
  if (wchunk->enter.mem)
    gsl_delete_struct (gfloat, wchunk->enter.length + 2 * padding, wchunk->enter.mem - padding);
  memset (&wchunk->enter, 0, sizeof (GslWaveChunkMem));
  if (wchunk->wrap.mem)
    gsl_delete_struct (gfloat, wchunk->wrap.length + 2 * padding, wchunk->wrap.mem - padding);
  memset (&wchunk->wrap, 0, sizeof (GslWaveChunkMem));
  if (wchunk->ppwrap.mem)
    gsl_delete_struct (gfloat, wchunk->ppwrap.length + 2 * padding, wchunk->ppwrap.mem - padding);
  memset (&wchunk->ppwrap, 0, sizeof (GslWaveChunkMem));
  if (wchunk->leave.mem)
    gsl_delete_struct (gfloat, wchunk->leave.length + 2 * padding, wchunk->leave.mem - padding);
  memset (&wchunk->leave, 0, sizeof (GslWaveChunkMem));
  if (wchunk->tail.mem)
    gsl_delete_struct (gfloat, wchunk->tail.length + 2 * padding, wchunk->tail.mem - padding);
  memset (&wchunk->tail, 0, sizeof (GslWaveChunkMem));
  wchunk->leave_end_norm = 0;
  wchunk->tail_start_norm = 0;
}

static void
wave_chunk_set_loop (GslWaveChunk   *wchunk,
		     GslWaveLoopType loop_type,
		     GslLong         loop_start,
		     GslLong         loop_end,
		     guint           loop_count)
{
  GslLong one, padding, big_pad;

  g_return_if_fail (wchunk != NULL);

  if (loop_count < 1 || loop_start < 0 || loop_end < loop_start)
    loop_type = GSL_WAVE_LOOP_NONE;
  one = wchunk->n_channels;
  padding = wchunk->n_pad_values;
  big_pad = PBLOCK_SIZE (wchunk->n_pad_values, wchunk->n_channels);
  switch (loop_type)
    {
    case GSL_WAVE_LOOP_NONE:
      wchunk->loop_type = loop_type;
      wchunk->loop_start = wchunk->length + 1;
      wchunk->loop_end = -1;
      wchunk->loop_count = 0;
      wchunk->wave_length = wchunk->length;
      break;
    case GSL_WAVE_LOOP_JUMP:
      g_return_if_fail (loop_start >= 0 && loop_start < wchunk->length);
      g_return_if_fail (loop_end < wchunk->length);
      loop_start /= wchunk->n_channels;
      loop_end /= wchunk->n_channels;
      g_return_if_fail (loop_start < loop_end);
      wchunk->loop_type = loop_type;
      wchunk->loop_start = loop_start * wchunk->n_channels;
      wchunk->loop_end = loop_end * wchunk->n_channels;
      wchunk->loop_count = (G_MAXINT - wchunk->length) / (wchunk->loop_end - wchunk->loop_start + one);
      wchunk->loop_count = MIN (wchunk->loop_count, loop_count);
      wchunk->wave_length = wchunk->length + (wchunk->loop_end - wchunk->loop_start + one) * wchunk->loop_count;
      break;
    case GSL_WAVE_LOOP_PINGPONG:
      g_return_if_fail (loop_start >= 0 && loop_start < wchunk->length);
      g_return_if_fail (loop_end <= wchunk->length);
      loop_start /= wchunk->n_channels;
      loop_end /= wchunk->n_channels;
      g_return_if_fail (loop_start + 1 < loop_end);
      wchunk->loop_type = loop_type;
      wchunk->loop_start = loop_start * wchunk->n_channels;
      wchunk->loop_end = loop_end * wchunk->n_channels;
      wchunk->loop_count = (G_MAXINT - wchunk->loop_end - one) / (wchunk->loop_end - wchunk->loop_start);
      wchunk->loop_count = MIN (wchunk->loop_count, loop_count);
      wchunk->wave_length = wchunk->loop_end + one + (wchunk->loop_end - wchunk->loop_start) * wchunk->loop_count;
      if (wchunk->loop_count & 1)	/* FIXME */
	wchunk->wave_length += wchunk->loop_start;
      else
	wchunk->wave_length += wchunk->length - one - wchunk->loop_end;
      break;
    default:
      g_return_if_fail (loop_type >= GSL_WAVE_LOOP_NONE && loop_type <= GSL_WAVE_LOOP_PINGPONG);
    }
  wchunk->pploop_ends_backwards = wchunk->loop_type == GSL_WAVE_LOOP_PINGPONG && (wchunk->loop_count & 1);
  wchunk->mini_loop = wchunk->loop_type && wchunk->loop_end - wchunk->loop_start < 2 * big_pad + padding;
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
gsl_wave_chunk_copy (GslWaveChunk *wchunk)
{
  g_return_val_if_fail (wchunk != NULL, NULL);

  return _gsl_wave_chunk_create (wchunk->dcache,
				 wchunk->offset,
				 wchunk->length / wchunk->n_channels,
				 wchunk->n_channels,
				 wchunk->osc_freq,
				 wchunk->mix_freq,
				 wchunk->loop_type,
				 wchunk->loop_start,
				 wchunk->loop_end,
				 wchunk->loop_count);
}
