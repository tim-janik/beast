/* TOYPROF - Poor man's profiling toy
 * Copyright (C) 2000-2002 Tim Janik
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include <stdlib.h>
#include "toyprof-mem.h"

/* Gdb hooks */
volatile gulong toyprof_trap_free_size = 0;
volatile gulong toyprof_trap_realloc_size = 0;
volatile gulong toyprof_trap_malloc_size = 0;

#if	 defined __GLIBC__ && __GLIBC__ >= 2
#include <pthread.h>
#include <stdio.h>
#include <execinfo.h>

/* FILE* for reports*/
#define RFL stderr

#define standard_malloc		malloc
#define standard_realloc	realloc
#define standard_free		free
#define standard_calloc		calloc
#define standard_try_malloc	malloc
#define standard_try_realloc	realloc

typedef enum {
  PROFILER_FREE		= 0,
  PROFILER_ALLOC	= 1,
  PROFILER_RELOC	= 2,
  PROFILER_ZINIT	= 4
} ProfilerJob;

#define	TOYPROF_TABLE_ENTRY(f1,f2,f3)   ( ( ((f3) << 2) | ((f2) << 1) | (f1) ) * (TOYPROF_MEM_TABLE_SIZE + 1))

/* statistic variables */
static guint          *profile_data = NULL;
static gulong          profile_allocs = 0;
static gulong          profile_zinit = 0;
static gulong          profile_frees = 0;
static pthread_mutex_t profile_mutex = PTHREAD_MUTEX_INITIALIZER;


/* --- functions --- */
static void
toyprof_log (ProfilerJob job,
	     gulong      n_bytes,
	     gboolean    success)
{
  pthread_mutex_lock (&profile_mutex);
  if (!profile_data)
    {
      profile_data = standard_malloc ((TOYPROF_MEM_TABLE_SIZE + 1) * 8 * sizeof (profile_data[0]));
      if (!profile_data)	/* memory system kiddin' me, eh? */
	{
	  pthread_mutex_unlock (&profile_mutex);
	  return;
	}
    }
  
  if (n_bytes < TOYPROF_MEM_TABLE_SIZE)
    profile_data[n_bytes + TOYPROF_TABLE_ENTRY ((job & PROFILER_ALLOC) != 0,
						(job & PROFILER_RELOC) != 0,
						success != 0)] += 1;
  else
    profile_data[TOYPROF_MEM_TABLE_SIZE + TOYPROF_TABLE_ENTRY ((job & PROFILER_ALLOC) != 0,
							   (job & PROFILER_RELOC) != 0,
							   success != 0)] += 1;
  if (success)
    {
      if (job & PROFILER_ALLOC)
	{
	  profile_allocs += n_bytes;
	  if (job & PROFILER_ZINIT)
	    profile_zinit += n_bytes;
	}
      else
	profile_frees += n_bytes;
    }
  pthread_mutex_unlock (&profile_mutex);
}

static void
toyprof_report_mem_table (guint   *local_data,
			  gboolean success)
{
  gboolean need_header = TRUE;
  guint i;
  
  for (i = 0; i <= TOYPROF_MEM_TABLE_SIZE; i++)
    {
      glong t_malloc = local_data[i + TOYPROF_TABLE_ENTRY (1, 0, success)];
      glong t_realloc = local_data[i + TOYPROF_TABLE_ENTRY (1, 1, success)];
      glong t_free = local_data[i + TOYPROF_TABLE_ENTRY (0, 0, success)];
      glong t_refree = local_data[i + TOYPROF_TABLE_ENTRY (0, 1, success)];
      
      if (!t_malloc && !t_realloc && !t_free && !t_refree)
	continue;
      else if (need_header)
	{
	  need_header = FALSE;
	  fprintf (RFL, " blocks of | allocated  | freed      | allocated  | freed      | n_bytes   \n");
	  fprintf (RFL, "  n_bytes  | n_times by | n_times by | n_times by | n_times by | remaining \n");
	  fprintf (RFL, "           | malloc()   | free()     | realloc()  | realloc()  |           \n");
	  fprintf (RFL, "===========|============|============|============|============|===========\n");
	}
      if (i < TOYPROF_MEM_TABLE_SIZE)
	fprintf (RFL, "%10u | %10ld | %10ld | %10ld | %10ld |%+11ld\n",
		 i, t_malloc, t_free, t_realloc, t_refree,
		 (t_malloc - t_free + t_realloc - t_refree) * i);
      else if (i >= TOYPROF_MEM_TABLE_SIZE)
	fprintf (RFL, "   >%6u | %10ld | %10ld | %10ld | %10ld |        ***\n",
		 i, t_malloc, t_free, t_realloc, t_refree);
    }
  if (need_header)
    fprintf (RFL, " --- none ---\n");
}

void
toyprof_report_mem (void)
{
  guint local_data[(TOYPROF_MEM_TABLE_SIZE + 1) * 8 * sizeof (profile_data[0])];
  gulong local_allocs;
  gulong local_zinit;
  gulong local_frees;
  
  pthread_mutex_lock (&profile_mutex);
  local_allocs = profile_allocs;
  local_zinit = profile_zinit;
  local_frees = profile_frees;
  if (!profile_data)
    {
      pthread_mutex_unlock (&profile_mutex);
      return;
    }
  memcpy (local_data, profile_data, 
	  (TOYPROF_MEM_TABLE_SIZE + 1) * 8 * sizeof (profile_data[0]));
  pthread_mutex_unlock (&profile_mutex);
  
  fprintf (RFL, "Memory statistics (successful operations):\n");
  toyprof_report_mem_table (local_data, TRUE);
  fprintf (RFL, "Memory statistics (failing operations):\n");
  toyprof_report_mem_table (local_data, FALSE);
  fprintf (RFL, "Total bytes: allocated=%lu, zero-initialized=%lu (%.2f%%), freed=%lu (%.2f%%), remaining=%lu\n",
	   local_allocs,
	   local_zinit,
	   ((gdouble) local_zinit) / local_allocs * 100.0,
	   local_frees,
	   ((gdouble) local_frees) / local_allocs * 100.0,
	   local_allocs - local_frees);
}

static gpointer
toyprof_try_malloc (gsize n_bytes)
{
  gulong *p;
  
  if (toyprof_trap_malloc_size == n_bytes)
    G_BREAKPOINT ();
  
  p = standard_malloc (sizeof (gulong) * 2 + n_bytes);
  
  if (p)
    {
      p[0] = 0;		/* free count */
      p[1] = n_bytes;	/* length */
      toyprof_log (PROFILER_ALLOC, n_bytes, TRUE);
      p += 2;
    }
  else
    toyprof_log (PROFILER_ALLOC, n_bytes, FALSE);
  
  return p;
}

static gpointer
toyprof_malloc (gsize n_bytes)
{
  gpointer mem = toyprof_try_malloc (n_bytes);
  
  if (!mem)
    g_mem_profile ();
  
  return mem;
}

static gpointer
toyprof_calloc (gsize n_blocks,
		gsize n_block_bytes)
{
  gsize l = n_blocks * n_block_bytes;
  gulong *p;
  
  if (toyprof_trap_malloc_size == l)
    G_BREAKPOINT ();
  
  p = standard_calloc (1, sizeof (gulong) * 2 + l);
  
  if (p)
    {
      p[0] = 0;		/* free count */
      p[1] = l;		/* length */
      toyprof_log (PROFILER_ALLOC | PROFILER_ZINIT, l, TRUE);
      p += 2;
    }
  else
    {
      toyprof_log (PROFILER_ALLOC | PROFILER_ZINIT, l, FALSE);
      g_mem_profile ();
    }
  
  return p;
}

static void
toyprof_free (gpointer mem)
{
  gulong *p = mem;
  
  p -= 2;
  if (p[0])	/* free count */
    {
      g_warning ("free(%p): memory has been freed %lu times already", p + 2, p[0]);
      toyprof_log (PROFILER_FREE,
		   p[1],	/* length */
		   FALSE);
    }
  else
    {
      if (toyprof_trap_free_size == p[1])
	G_BREAKPOINT ();
      
      toyprof_log (PROFILER_FREE,
		   p[1],	/* length */
		   TRUE);
      memset (p + 2, 0xaa, p[1]);
      
      /* for all those that miss standard_free (p); in this place, yes,
       * we do leak all memory when profiling, and that is intentional
       * to catch double frees. patch submissions are futile.
       */
    }
  p[0] += 1;
}

static gpointer
toyprof_try_realloc (gpointer mem,
		     gsize    n_bytes)
{
  gulong *p = mem;
  
  p -= 2;
  
  if (toyprof_trap_realloc_size == n_bytes)
    G_BREAKPOINT ();
  
  if (mem && p[0])	/* free count */
    {
      g_warning ("realloc(%p, %lu): memory has been freed %lu times already", p + 2, (gulong)n_bytes, p[0]);
      toyprof_log (PROFILER_ALLOC | PROFILER_RELOC, n_bytes, FALSE);
      
      return NULL;
    }
  else
    {
      p = standard_realloc (mem ? p : NULL, sizeof (gulong) * 2 + n_bytes);
      
      if (p)
	{
	  if (mem)
	    toyprof_log (PROFILER_FREE | PROFILER_RELOC, p[1], TRUE);
	  p[0] = 0;
	  p[1] = n_bytes;
	  toyprof_log (PROFILER_ALLOC | PROFILER_RELOC, p[1], TRUE);
	  p += 2;
	}
      else
	toyprof_log (PROFILER_ALLOC | PROFILER_RELOC, n_bytes, FALSE);
      
      return p;
    }
}

static gpointer
toyprof_realloc (gpointer mem,
		 gsize    n_bytes)
{
  mem = toyprof_try_realloc (mem, n_bytes);
  
  if (!mem)
    g_mem_profile ();
  
  return mem;
}

static GMemVTable mprof_table = {
  toyprof_malloc,
  toyprof_realloc,
  toyprof_free,
  toyprof_calloc,
  toyprof_try_malloc,
  toyprof_try_realloc,
};
GMemVTable *toyprof_mem_table = &mprof_table;
#else	/* !__GLIBC__ || __GLIBC__ < 2 */
GMemVTable *toyprof_mem_table = NULL;
void
toyprof_report_mem (void)
{
}
#endif	/* !__GLIBC__ || __GLIBC__ < 2 */


/* --- stack tracing --- */
#if 0
{
  header->strace_size = backtrace (header->strace, STRACE_DEPTH);
}

static void
trace_headers (void)
{
  Header *h;
  for (h = header_list; h; h = h->next)
    {
      guint i;
      char **strings = backtrace_symbols (header->strace, header->strace_size);
      fprintf (RFL, "Header-trace: ");
      for (i = 0; i < header->strace_size; i++)
	fprintf (RFL, "%s ", strings[i]);
      fprintf (RFL, "\n");
      free (strings);
    }
}
#endif
