/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include        "bsechunk.h"


#define	DFL_N_CHUNKS			32
#define	DFL_N_STATE_BLOCK_TRACKS	(BSE_MAX_N_TRACKS * 16)
#define	MIN_N_STATE_TRACKS		((sizeof (GTrashStack) + sizeof (BseSampleValue) - 1) / sizeof (BseSampleValue))
#define	HUNK_MAGIC			((BseSampleValue) 0xBeef)


/* --- variables --- */
static GTrashStack	*state_heap[BSE_MAX_N_TRACKS - MIN_N_STATE_TRACKS] = { NULL, };
static GSList		*state_blocks = NULL;
static guint		 n_state_block_tracks = 0;
static BseSampleValue	*state_block = NULL;
static GTrashStack	*hunk_heap[BSE_MAX_N_TRACKS] = { NULL, };
static guint       	 hunk_count = 0;
static BseSampleValue	*static_zero_hunk = NULL;
static GMemChunk	*chunks_mem_chunk = NULL;
static GTrashStack	*free_chunks = NULL;
static guint       	 chunk_count = 0;


/* --- functions --- */
static inline void
bse_nuke_state_allocs (void)
{
  guint i;
  GSList *slist;

  for (i = 0; i < BSE_MAX_N_TRACKS - MIN_N_STATE_TRACKS; i++)
    state_heap[i] = NULL;
  for (slist = state_blocks; slist; slist = slist->next)
    g_free (slist->data);
  g_slist_free (state_blocks);
  state_blocks = NULL;
  n_state_block_tracks = 0;
  state_block = NULL;
}

static inline BseSampleValue*
bse_state_alloc (guint n_tracks)
{
  BseSampleValue *state;

  n_tracks = MAX (MIN_N_STATE_TRACKS, n_tracks);

  state = g_trash_stack_pop (&state_heap[n_tracks - MIN_N_STATE_TRACKS]);
  if (!state)
    {
      if (n_state_block_tracks < n_tracks)
	{
	  if (n_state_block_tracks)
	    g_trash_stack_push (&state_heap[n_state_block_tracks - MIN_N_STATE_TRACKS], state_block);
	  n_state_block_tracks = DFL_N_STATE_BLOCK_TRACKS;
	  state_block = g_new (BseSampleValue, n_state_block_tracks);
	  state_blocks = g_slist_prepend (state_blocks, state_block);
	}

      state = state_block;
      n_state_block_tracks -= n_tracks;
      state_block += n_tracks;
    }

  return state;
}

static inline void
bse_state_free (BseSampleValue *state,
		guint           n_tracks)
{
  n_tracks = MAX (MIN_N_STATE_TRACKS, n_tracks);

  g_trash_stack_push (&state_heap[n_tracks - MIN_N_STATE_TRACKS], state);
}

static inline void
bse_nuke_hunk_allocs (void)
{
  guint i;

  for (i = 0; i < BSE_MAX_N_TRACKS - 1; i++)
    if (hunk_heap[i])
      {
	g_warning ("stale hunk fragments with %u tracks", i + 1);
	hunk_heap[i] = NULL;
      }
  i = BSE_MAX_N_TRACKS - 1;
  if (g_trash_stack_height (&hunk_heap[i]) != hunk_count)
    {
      g_warning ("hunk count discrepancy (%+d) - bleeding memory...", ((gint) i) - hunk_count);
      hunk_heap[i] = NULL;
    }
  while (hunk_heap[i])
    {
      BseSampleValue *mem;

      mem = g_trash_stack_pop (&hunk_heap[i]);
      mem--;
      if (*mem != HUNK_MAGIC)
	g_warning ("hunk with invalid magic id at %p", mem);
      else
	g_free (mem);
    }
  hunk_count = 0;
}

BseSampleValue*
bse_hunk_alloc (guint n_tracks)
{
  BseSampleValue *hunk;
  guint i;

  g_return_val_if_fail (n_tracks >= 1 && n_tracks <= BSE_MAX_N_TRACKS, NULL);

  n_tracks--;
  for (i = n_tracks; i < BSE_MAX_N_TRACKS; i++)
    if (hunk_heap[i])
      break;
  if (i >= BSE_MAX_N_TRACKS)
    {
      BseSampleValue *mem;

      i = BSE_MAX_N_TRACKS - 1;
      mem = g_new (BseSampleValue, BSE_MAX_N_TRACKS * BSE_TRACK_LENGTH + 1);
      *mem = HUNK_MAGIC;
      mem++;
      g_trash_stack_push (&hunk_heap[i], mem);
      hunk_count++;
    }

  hunk = g_trash_stack_pop (&hunk_heap[i]);
  i -= n_tracks;
  if (i)
    g_trash_stack_push (&hunk_heap[i - 1], hunk + BSE_TRACK_LENGTH * (n_tracks + 1));

  return hunk;
}

void
bse_hunk_free (BseSampleValue *hunk,
	       guint           n_tracks)
{
  g_return_if_fail (hunk != NULL);
  g_return_if_fail (n_tracks >= 1 && n_tracks <= BSE_MAX_N_TRACKS);

 recurse:

  if (n_tracks < BSE_MAX_N_TRACKS)
    {
      BseSampleValue *rest;
      guint i;

      rest = hunk + BSE_TRACK_LENGTH * n_tracks;
      for (i = 0; i < BSE_MAX_N_TRACKS; i++)
	{
	  GTrashStack *walk, *last;
	  guint rest2_add;

	  rest2_add = BSE_TRACK_LENGTH * (i + 1);
	  for (last = NULL, walk = hunk_heap[i]; walk; last = walk, walk = last->next)
	    if (walk == (GTrashStack*) rest)
	      {
		if (last)
		  last->next = walk->next;
		else
		  hunk_heap[i] = walk->next;
		n_tracks += i + 1;
		
		goto recurse;
	      }
	    else
	      {
		BseSampleValue *rest2;

		rest2 = (BseSampleValue*) walk;
		rest2 += rest2_add;
		if (rest2 == hunk)
		  {
		    if (last)
		      last->next = walk->next;
		    else
		      hunk_heap[i] = walk->next;
		    n_tracks += i + 1;
		    hunk = (BseSampleValue*) walk;

		    goto recurse;
		  }
	      }
	}
    }

  g_trash_stack_push (&hunk_heap[n_tracks - 1], hunk);
}

BseSampleValue*
bse_hunk_alloc0 (guint n_tracks)
{
  BseSampleValue *hunk;
  
  g_return_val_if_fail (n_tracks >= 1 && n_tracks <= BSE_MAX_N_TRACKS, NULL);

  hunk = bse_hunk_alloc (n_tracks);

  memset (hunk, 0, sizeof (BseSampleValue) * BSE_TRACK_LENGTH * n_tracks);

  return hunk;
}

static inline void
bse_nuke_chunk_allocs (void)
{
  if (chunk_count)
    g_warning ("Eeek, freeing chunks while %u are still alive", chunk_count);
  
  if (chunks_mem_chunk)
    g_mem_chunk_destroy (chunks_mem_chunk);
  if (static_zero_hunk)
    g_free (static_zero_hunk);
  chunks_mem_chunk = NULL;
  static_zero_hunk = NULL;
  free_chunks = NULL;
  chunk_count = 0;
}

static inline BseChunk*
bse_chunk_alloc (guint n_tracks)
{
  BseChunk *chunk;

  chunk = g_trash_stack_pop (&free_chunks);
  if (!chunk)
    {
      if (!chunks_mem_chunk)
	{
	  chunks_mem_chunk = g_mem_chunk_create (BseChunk, DFL_N_CHUNKS, G_ALLOC_ONLY);
	  static_zero_hunk = g_new0 (BseSampleValue, BSE_MAX_N_TRACKS * BSE_TRACK_LENGTH);
	}

      chunk = g_chunk_new (BseChunk, chunks_mem_chunk);
    }

  chunk->n_tracks = n_tracks;
  chunk->state = bse_state_alloc (n_tracks);
  chunk->hunk = NULL;
  chunk->state_filled = FALSE;
  chunk->hunk_filled = FALSE;
  chunk->free_state = TRUE;
  chunk->foreign_hunk = FALSE;
  chunk->g_free_hunk = FALSE;
  chunk->ref_count = 1;

  chunk_count++;

  return chunk;
}

void
bse_chunk_unref (BseChunk *chunk)
{
  g_return_if_fail (chunk != NULL);
  g_return_if_fail (chunk->ref_count > 0);

  chunk->ref_count--;
  if (!chunk->ref_count)
    {
      if (chunk->hunk)
	{
	  if (!chunk->foreign_hunk)
	    bse_hunk_free (chunk->hunk, chunk->n_tracks);
	  else if (chunk->g_free_hunk)
	    g_free (chunk->hunk);
	}

      if (chunk->free_state)
	bse_state_free (chunk->state, chunk->n_tracks);

      chunk->state_filled = FALSE;
      chunk->hunk_filled = FALSE;

      g_trash_stack_push (&free_chunks, chunk);

      chunk_count--;
    }

}

void
bse_chunk_ref (BseChunk *chunk)
{
  g_return_if_fail (chunk != NULL);
  g_return_if_fail (chunk->ref_count > 0);

  chunk->ref_count += 1;
}

BseChunk*
bse_chunk_new_from_state (guint           n_tracks,
			  BseSampleValue *state)
{
  BseChunk *chunk;

  g_return_val_if_fail (n_tracks >= 1 && n_tracks <= BSE_MAX_N_TRACKS, NULL);
  g_return_val_if_fail (state != NULL, NULL);

  chunk = bse_chunk_alloc (n_tracks);
  memcpy (chunk->state, state, sizeof (BseSampleValue) * n_tracks);
  chunk->state_filled = TRUE;

  return chunk;
}

BseChunk*
bse_chunk_new (guint n_tracks)
{
  BseChunk *chunk;

  g_return_val_if_fail (n_tracks >= 1 && n_tracks <= BSE_MAX_N_TRACKS, NULL);

  chunk = bse_chunk_alloc (n_tracks);

  chunk->hunk = bse_hunk_alloc (chunk->n_tracks);
  chunk->foreign_hunk = FALSE;
  
  return chunk;
}

BseChunk*
bse_chunk_new0 (guint n_tracks)
{
  BseChunk *chunk;

  g_return_val_if_fail (n_tracks >= 1 && n_tracks <= BSE_MAX_N_TRACKS, NULL);

  chunk = bse_chunk_alloc (n_tracks);

  chunk->hunk = bse_hunk_alloc0 (chunk->n_tracks);
  chunk->foreign_hunk = FALSE;
  
  return chunk;
}

BseChunk*
bse_chunk_new_orphan (guint           n_tracks,
		      BseSampleValue *orphan_hunk)
{
  BseChunk *chunk;

  g_return_val_if_fail (n_tracks >= 1 && n_tracks <= BSE_MAX_N_TRACKS, NULL);
  g_return_val_if_fail (orphan_hunk != NULL, NULL);

  chunk = bse_chunk_alloc (n_tracks);

  chunk->hunk = orphan_hunk;
  chunk->foreign_hunk = FALSE;
  chunk->hunk_filled = TRUE;
  
  return chunk;
}

BseChunk*
bse_chunk_new_foreign (guint           n_tracks,
		       BseSampleValue *data,
		       gboolean        g_free_data)
{
  BseChunk *chunk;

  g_return_val_if_fail (n_tracks >= 1 && n_tracks <= BSE_MAX_N_TRACKS, NULL);
  g_return_val_if_fail (data != NULL, NULL);

  chunk = bse_chunk_alloc (n_tracks);

  chunk->hunk = data;
  chunk->hunk_filled = TRUE;
  chunk->foreign_hunk = TRUE;
  chunk->g_free_hunk = g_free_data != 0;

  return chunk;
}

BseChunk*
bse_chunk_new_static_zero (guint n_tracks)
{
  static BseSampleValue	static_zero_state[BSE_MAX_N_TRACKS] = { 0, };
  BseChunk *chunk;

  g_return_val_if_fail (n_tracks >= 1 && n_tracks <= BSE_MAX_N_TRACKS, NULL);

  chunk = bse_chunk_alloc (n_tracks);

  bse_state_free (chunk->state, chunk->n_tracks);
  chunk->state = static_zero_state;
  chunk->state_filled = TRUE;
  chunk->free_state = FALSE;

  chunk->hunk = static_zero_hunk;
  chunk->hunk_filled = TRUE;
  chunk->foreign_hunk = TRUE;
  chunk->g_free_hunk = FALSE;

  return chunk;
}

void
bse_chunk_complete_state (BseChunk *chunk)
{
  g_return_if_fail (chunk != NULL);
  g_return_if_fail (chunk->ref_count > 0);

  if (!chunk->state_filled)
    {
      BseMixValue values[BSE_MAX_N_TRACKS] = { 0, };
      guint i;

      g_return_if_fail (chunk->hunk_filled == TRUE);

      for (i = 0; i < BSE_TRACK_LENGTH * chunk->n_tracks; i++)
	values[i % chunk->n_tracks] += chunk->hunk[i];
      
      for (i = 0; i < chunk->n_tracks; i++)
	if (values[i] > 0)
	  chunk->state[i] = (values[i] + BSE_TRACK_LENGTH / 2) / BSE_TRACK_LENGTH;
	else
	  chunk->state[i] = (values[i] - BSE_TRACK_LENGTH / 2) / BSE_TRACK_LENGTH;

      chunk->state_filled = TRUE;
    }
}

void
bse_chunk_complete_hunk (BseChunk *chunk)
{
  g_return_if_fail (chunk != NULL);
  g_return_if_fail (chunk->ref_count > 0);

  if (!chunk->hunk_filled)
    {
      guint i;

      g_return_if_fail (chunk->state_filled == TRUE);

      if (!chunk->hunk)
	{
	  chunk->hunk = bse_hunk_alloc (chunk->n_tracks);
	  chunk->foreign_hunk = FALSE;
	}

      for (i = 0; i < BSE_TRACK_LENGTH * chunk->n_tracks; i++)
	chunk->hunk[i] = chunk->state[i % chunk->n_tracks];

      chunk->hunk_filled = TRUE;
    }
}

void
bse_chunk_debug (void)
{
  extern BseIndex bse_index; /* FIXME */
  
  g_message ("BseIndex: %lld", bse_index);
  g_message ("n_state_blocks: %d", g_slist_length (state_blocks));
  g_message ("hunk_count: %d", hunk_count);
  g_message ("chunk_count: %d", chunk_count);
  g_message ("n_free_chunks: %d", g_trash_stack_height (&free_chunks));
  if (chunks_mem_chunk)
    g_mem_chunk_print (chunks_mem_chunk);
  else
    g_message ("BseChunk mem_chunk is NULL");
}

void
bse_chunks_nuke (void)
{
  extern BseIndex bse_index;

  bse_nuke_state_allocs ();
  bse_nuke_chunk_allocs ();
  bse_nuke_hunk_allocs ();

  bse_index = -1;
}
