/* TOYPROF - Poor man's profiling toy
 * Copyright (C) 2000-2003 Tim Janik
 *
 * This software is provided "as is"; redistribution and modification
 * is permitted, provided that the following disclaimer is retained.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */
#define	 _GNU_SOURCE	/* enable various glibc features we depend on */
#include "toyprof-mem.h"

/* --- configuration --- */
#define	STACK_TRACE_DEPTH	12	/* number of callers to log */

/* Basic usage:
 * 1) call toyprof_init_glib_memtable ("/tmp/mylogfile", SIGUSR1); in main()
 * 2) start the program
 * 3) kill -SIGUSR1 <program-pid> ; this starts memory leak logging
 * 4) kill -SIGUSR1 <program-pid> ; this dumps leaks since (3) to "/tmp/mylogfile"
 * 5) cat "/tmp/mylogfile" | toyprof.pl | less -S
 */

/* --- extern variables --- */
volatile gulong		 toyprof_memtotal = 0;


#if	 defined __GLIBC__ && __GLIBC__ >= 2
#include <pthread.h>
#include <stdio.h>
#include <execinfo.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>
#include <link.h>	/* _r_debug */

#define standard_malloc		malloc
#define standard_realloc	realloc
#define standard_free		free
#define standard_calloc		calloc

typedef struct _MemHeader MemHeader;
struct _MemHeader
{
  MemHeader *prev;
  guint      n_bytes;
  guint      stamp;
  void      *traces[STACK_TRACE_DEPTH];
  MemHeader *next;
};
#define	HEADER_SIZE	(sizeof (MemHeader))


/* --- variables --- */
extern GMemVTable	*toyprof_mem_table;
static gulong		 toyprof_n_traces;
static guint		 toyprof_stamp = 0;
static MemHeader	*memlist = NULL;
static pthread_mutex_t	 profile_mutex = PTHREAD_MUTEX_INITIALIZER;
static const gchar	*logfile_name = NULL;
static gint		 toyprof_logger_signal = 0;
static gint		 toyprof_need_dump = 0;
typedef struct {
  glong        sbase;
  const gchar *sname;
} SymEntry;
static SymEntry *toyprof_symtab = NULL;
static guint toyprof_symtab_length = 0;


/* --- functions --- */
static int
symentry_compare (const void *d1,
		  const void *d2)
{
  const SymEntry *e1 = d1;
  const SymEntry *e2 = d2;
  return e1->sbase < e2->sbase ? -1 : e1->sbase > e2->sbase;
}

static void
symtab_init (void)
{
  struct link_map *link;
  const gchar *main_sname = NULL;
  glong main_sbase = 0;
  for (link = _r_debug.r_map; link; link = link->l_next)
    if (!link->l_addr)
      {
	main_sname = link->l_name;
	main_sbase = link->l_addr;
      }
    else
      {
	guint n = toyprof_symtab_length++;
	toyprof_symtab = realloc (toyprof_symtab, sizeof (toyprof_symtab[0]) * toyprof_symtab_length);
	assert (toyprof_symtab != NULL);
	toyprof_symtab[n].sbase = link->l_addr;
	toyprof_symtab[n].sname = link->l_name;
      }
  qsort (toyprof_symtab, toyprof_symtab_length, sizeof (toyprof_symtab[0]), symentry_compare);
  toyprof_symtab = realloc (toyprof_symtab, sizeof (toyprof_symtab[0]) * (toyprof_symtab_length + 1));
  toyprof_symtab[toyprof_symtab_length].sbase = main_sbase;
  toyprof_symtab[toyprof_symtab_length].sname = main_sname;
  assert (main_sname != NULL);
}

static SymEntry*
symtab_lookup (void *symaddr)
{
  glong addr = (glong) symaddr;
  gulong offs = 0, n = toyprof_symtab_length;
  SymEntry *last = NULL;
  while (offs < n)
    {
      guint i = (offs + n) >> 1;
      if (addr < toyprof_symtab[i].sbase)
	n = i;
      else
	{
	  last = toyprof_symtab + i;
	  offs = i + 1;
	}
    }
  return last ? last : toyprof_symtab + toyprof_symtab_length; /* fallback to main */
}

static void
toyprof_dump_leaks_U (guint leak_logger_stamp,
		      gint  fd);
static void
memleak_handle_jobs (void)
{
  pthread_mutex_lock (&profile_mutex);
  if (toyprof_need_dump && !toyprof_symtab)
    {
      dprintf (2, "TOYPROF: start logging\n");
      symtab_init ();
      toyprof_stamp++;
      toyprof_need_dump = 0;
    }
  else if (toyprof_need_dump)
    {
      gint fd = open (logfile_name, O_CREAT | O_TRUNC | O_WRONLY, 0600);
      dprintf (2, "TOYPROF: dumping leak list to %s\n", logfile_name);
      if (fd >= 0)
	{
	  toyprof_dump_leaks_U (toyprof_stamp, fd);
	  close (fd);
	  dprintf (2, "TOYPROF: leak dump done\n");
	}
      else
	dprintf (2, "TOYPROF: failed to open file: %s\n", strerror (errno));
      toyprof_stamp++;
      toyprof_need_dump = 0;
    }
  pthread_mutex_unlock (&profile_mutex);
}

static void
toyprof_memsignal (int sigid)
{
  toyprof_need_dump = 1;
  signal (toyprof_logger_signal, toyprof_memsignal);
}

void
toyprof_init_glib_memtable (const gchar *file_name,
			    gint         logger_signal)
{
  static int toyprof_memtable_initialized = 0;
  assert (file_name != NULL);
  assert (++toyprof_memtable_initialized == 1);
  toyprof_n_traces = 0;
  logfile_name = strdup (file_name);
  g_mem_set_vtable (toyprof_mem_table);
  if (logger_signal > 0)
    {
      toyprof_logger_signal = logger_signal;
      signal (toyprof_logger_signal, toyprof_memsignal);
    }
}

guint
toyprof_start_leak_logger (void)
{
  pthread_mutex_lock (&profile_mutex);
  toyprof_stamp++;
  pthread_mutex_unlock (&profile_mutex);
  return toyprof_stamp;
}

static void
toyprof_dump_leaks_U (guint leak_logger_stamp,
		      gint  fd)
{
  if (fd >= 0)
    {
      MemHeader *h;
      for (h = memlist; h; h = h->next)
	if (h->stamp == leak_logger_stamp)
	  {
	    guint i;
	    dprintf (fd, "TOYPROFDATA: %u ", h->n_bytes);
	    for (i = 0; i < STACK_TRACE_DEPTH; i++)
	      {
		SymEntry *e = symtab_lookup (h->traces[i]);
		dprintf (fd, "%s:%u ", e->sname, ((char*) h->traces[i]) - ((char*) e->sbase));
	      }
	    dprintf (fd, "\n");
	  }
    }
}

void
toyprof_dump_leaks (guint leak_logger_stamp,
		    gint  fd)
{
  pthread_mutex_lock (&profile_mutex);
  toyprof_dump_leaks_U (leak_logger_stamp, fd);
  pthread_mutex_unlock (&profile_mutex);
}

static void
memlist_add (MemHeader *h,
	     size_t     n_bytes)
{
  h->n_bytes = n_bytes;
  h->stamp = toyprof_stamp;
  h->prev = NULL;
  memset (h->traces, 0, sizeof (h->traces[0]) * STACK_TRACE_DEPTH);
  pthread_mutex_lock (&profile_mutex);
  if (memlist)
    memlist->prev = h;
  h->next = memlist;
  memlist = h;
  toyprof_memtotal += h->n_bytes;
  pthread_mutex_unlock (&profile_mutex);
}

static void
memlist_remove (MemHeader *h)
{
  pthread_mutex_lock (&profile_mutex);
  if (h->next)
    h->next->prev = h->prev;
  if (h->prev)
    h->prev->next = h->next;
  else
    memlist = h->next;
  toyprof_memtotal -= h->n_bytes;
  pthread_mutex_unlock (&profile_mutex);
  h->next = NULL;
  h->prev = NULL;
}

static gpointer
toyprof_malloc (gsize n_bytes)
{
  MemHeader *h = standard_malloc (HEADER_SIZE + n_bytes);
  if (!h)
    return NULL;
  memleak_handle_jobs ();
  memlist_add (h, n_bytes);
  backtrace (h->traces, STACK_TRACE_DEPTH);
  return h + 1;
}

static void
toyprof_free (gpointer mem)
{
  MemHeader *h = mem;

  h -= 1;
  memlist_remove (h);
  memset (h, 0xaa, HEADER_SIZE + h->n_bytes);
  standard_free (h);
  memleak_handle_jobs ();
}

static gpointer
toyprof_realloc (gpointer mem,
		 gsize    n_bytes)
{
  MemHeader *tmp, *h = mem;
  if (!mem)
    return toyprof_malloc (n_bytes);
  else if (!n_bytes)
    {
      toyprof_free (mem);
      return NULL;
    }

  h -= 1;
  memlist_remove (h);
  tmp = standard_realloc (h, HEADER_SIZE + n_bytes);
  memleak_handle_jobs ();
  if (!tmp)
    {
      memlist_add (h, h->n_bytes);
      backtrace (h->traces, STACK_TRACE_DEPTH);
      return NULL;
    }
  h = tmp;
  memlist_add (h, n_bytes);
  backtrace (h->traces, STACK_TRACE_DEPTH);
  return h + 1;
}

static GMemVTable mprof_table = {
  toyprof_malloc,
  toyprof_realloc,
  toyprof_free,
  NULL, /* calloc */
  NULL, /* try_malloc */
  NULL, /* try_realloc */
};
GMemVTable *toyprof_mem_table = &mprof_table;


#else	/* !__GLIBC__ || __GLIBC__ < 2 */
void
toyprof_init_glib_memtable (const gchar *file_name,
			    gint         logger_signal)
{
}
guint
toyprof_start_leak_logger (void)
{
  return 0;
}
void
toyprof_dump_leaks (guint          leak_logger_stamp,
		    gint           fd)
{
}
#endif	/* !__GLIBC__ || __GLIBC__ < 2 */
