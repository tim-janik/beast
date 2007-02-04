/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002 Tim Janik and Stefan Westerfeld
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
#include "sfimemory.h"
#include <string.h>


#define PREALLOC                (8)
#define SIMPLE_CACHE_SIZE       (64)
#define TS8_SIZE                (MAX (sizeof (GTrashStack), 8))
#define DBG8_SIZE               (MAX (sizeof (gsize), 8))


/* --- variables --- */
static BirnetMutex     global_memory_mutex = { 0, };
static GTrashStack *simple_cache[SIMPLE_CACHE_SIZE] = { 0, 0, 0, /* ... */ };
static gulong       memory_allocated = 0;


/* --- functions --- */
gulong
sfi_alloc_upper_power2 (const gulong number)
{
  return number ? 1 << g_bit_storage (number - 1) : 0;
}

#if 0
static inline gpointer
low_alloc (gsize mem_size)
{
  gpointer mem;
  
  if (mem_size >= TS8_SIZE && mem_size / 8 < SIMPLE_CACHE_SIZE)
    {
      guint cell;
      
      mem_size = (mem_size + 7) & ~0x7;
      cell = (mem_size >> 3) - 1;
      sfi_mutex_lock (&global_memory_mutex);
      mem = g_trash_stack_pop (simple_cache + cell);
      sfi_mutex_unlock (&global_memory_mutex);
      if (!mem)
	{
	  guint8 *cache_mem = g_malloc (mem_size * PREALLOC);
	  guint i;
	  
	  sfi_mutex_lock (&global_memory_mutex);
	  memory_allocated += mem_size * PREALLOC;
	  for (i = 0; i < PREALLOC - 1; i++)
	    {
	      g_trash_stack_push (simple_cache + cell, cache_mem);
	      cache_mem += mem_size;
	    }
	  sfi_mutex_unlock (&global_memory_mutex);
	  mem = cache_mem;
	}
    }
  else
    {
      mem = g_malloc (mem_size);
      sfi_mutex_lock (&global_memory_mutex);
      memory_allocated += mem_size;
      sfi_mutex_unlock (&global_memory_mutex);
    }
  return mem;
}

static inline void
low_free (gsize    mem_size,
	  gpointer mem)
{
  if (mem_size >= TS8_SIZE && mem_size / 8 < SIMPLE_CACHE_SIZE)
    {
      guint cell;
      
      mem_size = (mem_size + 7) & ~0x7;
      cell = (mem_size >> 3) - 1;
      sfi_mutex_lock (&global_memory_mutex);
      g_trash_stack_push (simple_cache + cell, mem);
      sfi_mutex_unlock (&global_memory_mutex);
    }
  else
    {
      g_free (mem);
      sfi_mutex_lock (&global_memory_mutex);
      memory_allocated -= mem_size;
      sfi_mutex_unlock (&global_memory_mutex);
    }
}
#else
static inline gpointer
low_alloc (gsize mem_size)
{
  return g_malloc (mem_size);
}
static inline void
low_free (gsize    mem_size,
          gpointer mem)
{
  g_free (mem);
}
#endif

gpointer
sfi_alloc_memblock (gsize block_size)
{
  guint8 *cmem;
  gsize *debug_size;
  
  g_return_val_if_fail (block_size >= sizeof (gpointer), NULL);	/* cache-link size */
  
  cmem = low_alloc (block_size + DBG8_SIZE);
  debug_size = (gsize*) cmem;
  *debug_size = block_size;
  cmem += DBG8_SIZE;
  
  return cmem;
}

void
sfi_free_memblock (gsize    block_size,
		   gpointer mem)
{
  gsize *debug_size;
  guint8 *cmem;
  
  g_return_if_fail (mem != NULL);
  
  cmem = mem;
  cmem -= DBG8_SIZE;
  debug_size = (gsize*) cmem;
  if (block_size != *debug_size)
    g_printerr ("%s: in memory block at (%p): block_size=%zd != *debug_size=%zd\n", G_STRLOC, mem, block_size, *debug_size);
  
  low_free (block_size + DBG8_SIZE, cmem);
}

void
sfi_alloc_report (void)
{
  guint cell, cached = 0;
  
  sfi_mutex_lock (&global_memory_mutex);
  for (cell = 0; cell < SIMPLE_CACHE_SIZE; cell++)
    {
      GTrashStack *trash = simple_cache[cell];
      guint memsize, n = 0;
      
      while (trash)
	{
	  n++;
	  trash = trash->next;
	}
      
      if (n)
	{
	  memsize = (cell + 1) << 3;
	  g_message ("cell %4u): %u bytes in %u nodes", memsize, memsize * n, n);
	  cached += memsize * n;
	}
    }
  g_message ("%lu bytes allocated from system, %u bytes unused in cache", memory_allocated, cached);
  sfi_mutex_unlock (&global_memory_mutex);
}

gpointer
sfi_alloc_memblock0 (gsize block_size)
{
  gpointer mem = sfi_alloc_memblock (block_size);
  
  memset (mem, 0, block_size);
  
  return mem;
}

void
_sfi_free_node_list (gpointer mem,
		     gsize    node_size)
{
  struct { gpointer data, next; } *tmp, *node = mem;
  
  g_return_if_fail (node != NULL);
  g_return_if_fail (node_size >= 2 * sizeof (gpointer));
  
  /* FIXME: this can be optimized to an O(1) operation with T-style links in mem-caches */
  do
    {
      tmp = node->next;
      sfi_free_memblock (node_size, node);
      node = tmp;
    }
  while (node);
}

void
_sfi_init_memory (void)
{
  gboolean initialized = FALSE;
  g_assert (initialized == FALSE);
  initialized = TRUE;
  sfi_mutex_init (&global_memory_mutex);
}
