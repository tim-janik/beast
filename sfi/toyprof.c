/* TOYPROF - Poor man's profiling toy
 * Copyright (C) 2001-2002 Tim Janik
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "toyprof.h"

#ifdef	TOYPROF_GNUC_NO_INSTRUMENT	/* only have this with GNU GCC */

/* --- configuration --- */
/* TOYPROF_PENTIUM	- define this if you're going to run the compiled program
 *			  on an Intel Pentium(R) uni-processor machine. it will
 *			  use the pentium's RDTSC intruction to read out the
 *			  processor's clock cycle counter which usually produces
 *			  more accurate results and runs faster than using
 *			  gettimeofday(2) to obtain timing results.
 * TOYPROF_AUTOSTART	- collect function timing statistics by default (without
 *			  prior invocation of toyprof_set_profiling())
 * TOYPROF_DISABLE	- defining this will prevent Toyprof from suppling
 *			  the profiling symbols __cyg_profile_func_enter and
 *			  __cyg_profile_func_exit which are necessary to profile
 *			  code compiled with gcc's -finstrument-functions option.
 *			  note that code compiled with -finstrument-functions which
 *			  lacks __cyg_profile_func_enter and __cyg_profile_func_exit
 *			  symbol will still continue to work and not fail due to an
 *			  undefined symbol error.
 * TOYPROF_EXIT		- exit code to return upon profiler aborts
 */
#define TOYPROF_PENTIUM		0	/* not every gcc user has a pentium */
#define TOYPROF_AUTOSTART	1
#define TOYPROF_DISABLE		1
#define TOYPROF_EXIT		-1


/* --- implementation --- */
#define	_GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <execinfo.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <dlfcn.h>
#include <stddef.h>
#include <fcntl.h>
#include <link.h>
#include <elf.h>


/* --- common/usefull macros --- */
#ifndef	MAX
#define MAX(a,b)	((a) > (b) ? (a) : (b))
#endif
#ifndef	MIN
#define MIN(a,b)	((a) < (b) ? (a) : (b))
#endif
#ifndef	CLAMP
#define CLAMP(v,l,h)	((v) < (l) ? (l) : (v) > (h) ? (h) : (v))
#endif
#ifndef	FALSE
#define FALSE		0
#endif
#ifndef	TRUE
#define TRUE		(!FALSE)
#endif
#ifndef	NULL
#define NULL		((void*) 0)
#endif
#define	TOYPROF_ABORT(msg)		({ dprintf (2, "FATAL(%s:%u): error in profiler, aborting: %s\n", __FILE__, __LINE__, (msg)); _exit (TOYPROF_EXIT); })
#define	TOYPROF_ASSERT(cond)		({ if (!(cond)) TOYPROF_ABORT (#cond); })
#define	TOYPROF_CHECK_ALLOC(mem)	({ if (!mem) TOYPROF_ABORT ("{m|re}alloc() failed"); })
#define TOYPROF_SYMBOL_ALIAS(alias_name, symbol)	\
    extern __typeof (symbol) alias_name __attribute__ ((alias (#symbol)))


/* --- time stamping --- */
#ifdef	TOYPROF_PENTIUM

#define ToyprofStamp 		unsigned long long int

#define	toyprof_clock_name()	("Pentium(R) RDTSC - CPU clock cycle counter")

/* capturing time stamps via rdtsc can produce inaccurate results due
 * to parallel instruction execution. so we issue cpuid as serializaion
 * barrier first.
 */
#define toyprof_stamp(stamp)	({ unsigned int low, high;				\
                                   __asm__ __volatile__ ("pushl %%ebx\n"		\
							 "cpuid\n" /* serialization */	\
							 "rdtsc\n"			\
							 "popl %%ebx\n"			\
							 : "=a" (low), "=d" (high)	\
							 : "a" (0)			\
							 : "cx", "cc");			\
                                   (stamp) = high;					\
                                   (stamp) <<= 32;					\
                                   (stamp) += low;					\
})

#define toyprof_stamp_ticks()			(toyprof_stampfreq)

/* special case (fstamp) > (lstamp), this should never happen
 * because we always stamp fstamp first, and invoke rdtsc after
 * a serialization barrier (cpuid). in case this still happens
 * that's probably due to running on an SMP system.
 */
#define toyprof_elapsed(fstamp, lstamp)        ({ \
  ToyprofStamp diff;								\
  if ((fstamp) > (lstamp))							\
  dprintf(2, "%llu > %llu\n",fstamp,lstamp); \
  if ((fstamp) > (lstamp))							\
    TOYPROF_ABORT ("pentium CPU clock warped backwards, running on SMP?");	\
  diff = (lstamp) - (fstamp); diff;						\
})

static unsigned long long int toyprof_stampfreq = 0;

static void TOYPROF_GNUC_NO_INSTRUMENT
toyprof_stampinit (void)
{	/* grep "cpu MHz         : 551.256" from /proc/cpuinfo */
  int fd = open ("/proc/cpuinfo", O_RDONLY);
  if (fd >= 0) {
    char *val, buf[8192]; unsigned int l;
    l = read (fd, buf, sizeof (buf));
    buf[CLAMP (l, 0, sizeof (buf) - 1)] = 0;
    close (fd);
    val = l > 10 ? strstr (buf, "cpu MHz") : NULL;
    val = val ? strpbrk (val, "0123456789\n") : NULL;
    if (val && *val >= '0' && *val <= '9') {
      int frac = 6;
      while (*val >= '0' && *val <= '9')
	toyprof_stampfreq = toyprof_stampfreq * 10 + *val++ - '0';
      if (*val++ == '.')
	while (*val >= '0' && *val <= '9' && frac-- > 0)
	  toyprof_stampfreq = toyprof_stampfreq * 10 + *val++ - '0';
      while (frac-- > 0)
	toyprof_stampfreq *= 10;
    }
  }
  TOYPROF_ASSERT (toyprof_stampfreq > 0);
}

#else /* !TOYPROF_PENTIUM */

#define	ToyprofStamp		struct timeval

#define	toyprof_clock_name()	("Glibc gettimeofday(2)")

#define toyprof_stampinit()	/* nothing */

#define	toyprof_stamp(st)	gettimeofday (&(st), 0)

#define	toyprof_stamp_ticks()	(1000000)

#define	toyprof_elapsed(fstamp, lstamp)	({							\
  unsigned long long int first = (fstamp).tv_sec * toyprof_stamp_ticks () + (fstamp).tv_usec;	\
  unsigned long long int last  = (lstamp).tv_sec * toyprof_stamp_ticks () + (lstamp).tv_usec;	\
  last -= first;										\
  last;												\
})

#endif  /* !TOYPROF_PENTIUM */


/* --- profiling structures --- */
typedef struct {
  void                  *cur_func, *call_site, *caller;
  const char		*tmp_name;
  long long unsigned int n_calls;
  long long unsigned int uticks;
  ToyprofStamp           stamp;
} ToyprofEntry;
typedef struct {
  unsigned int   n_nodes;
  ToyprofEntry *nodes;
  unsigned int   stack_length;
  void         **stack;
} ToyprofRoot;


/* --- prototypes --- */
static int	toyprof_dladdr	(const void	*address,
				 Dl_info	*info) TOYPROF_GNUC_NO_INSTRUMENT;
static char*	toyprof_dlname	(void		*addr,
				 int		*resolved_p) TOYPROF_GNUC_NO_INSTRUMENT;


/* --- variables --- */
static ToyprofRoot	toyprof_root = { 0, NULL, 0, NULL, };
#ifdef	TOYPROF_AUTOSTART
static volatile ToyprofBehaviour toyprof_behaviour = TOYPROF_PROFILE_TIMING;
#else
static volatile ToyprofBehaviour toyprof_behaviour = TOYPROF_OFF;
#endif


/* --- functions --- */
static inline unsigned long TOYPROF_GNUC_NO_INSTRUMENT
toyprof_upper_power2 (unsigned long number)
{
  unsigned long n_bits = 0;
  
  /* this implements: number ? 1 << g_bit_storage (number - 1) : 0 */
  if (!number--)
    return 0;
  do
    {
      n_bits++;
      number >>= 1;
    }
  while (number);
  return 1 << n_bits;
}

static inline void TOYPROF_GNUC_NO_INSTRUMENT
toyprof_push (ToyprofRoot *proot,
	      void        *data)
{
  unsigned int i, new_size;
  
  i = proot->stack_length++;
  new_size = toyprof_upper_power2 (proot->stack_length);
  if (new_size != toyprof_upper_power2 (i))
    {
      proot->stack = realloc (proot->stack, new_size * sizeof (proot->stack[0]));
      TOYPROF_CHECK_ALLOC (proot->stack);
    }
  proot->stack[i] = data;
}
#define	toyprof_pop(proot)	({ (proot)->stack[--(proot)->stack_length]; })
#define	toyprof_peek(proot)	((proot)->stack[(proot)->stack_length - 1])
#define	toyprof_peekpeek(proot)	((proot)->stack[(proot)->stack_length - 2])

static inline ToyprofEntry* TOYPROF_GNUC_NO_INSTRUMENT
toyprof_insert (ToyprofRoot *proot,
		unsigned int pos)
{
  unsigned int index, new_size, old_size = proot->n_nodes;
  ToyprofEntry *node;
  
  index = proot->n_nodes++;
  new_size = toyprof_upper_power2 (proot->n_nodes);
  if (new_size != toyprof_upper_power2 (old_size))
    {
      proot->nodes = realloc (proot->nodes, new_size * sizeof (node[0]));
      TOYPROF_CHECK_ALLOC (proot->nodes);
    }
  node = proot->nodes + pos;
  memmove (node + 1, node, (index - pos) * sizeof (node[0]));
  
  return node;
}

static inline ToyprofEntry* TOYPROF_GNUC_NO_INSTRUMENT
toyprof_lookup (ToyprofRoot *proot,
		void        *cur_func,
		void	    *caller,
		int          exact_match)
{
  if (proot->n_nodes > 0)
    {
      ToyprofEntry *check, *nodes = proot->nodes;
      unsigned int n_nodes = proot->n_nodes;
      
      nodes -= 1;
      do
	{
	  register unsigned int i;
	  register int cmp;
	  
	  i = (n_nodes + 1) >> 1;
	  check = nodes + i;
	  cmp = (cur_func < check->cur_func ? -1 :
		 cur_func > check->cur_func ? 1 :
		 caller < check->caller ? -1 :
		 caller > check->caller);
	  if (cmp == 0)
	    return check;	/* exact match */
	  else if (cmp > 0)
	    {
	      n_nodes -= i;
	      nodes = check;
	    }
	  else /* if (cmp < 0) */
	    n_nodes = i - 1;
	}
      while (n_nodes);
      if (!exact_match)
	return check;	/* return nextmost */
    }
  return NULL;
}

void TOYPROF_GNUC_NO_INSTRUMENT
toyprof_dump_stats (int fd)
{
  double total_ticks = 0;
  double n_ticks;
  unsigned int i;
  ToyprofBehaviour saved_behaviour = toyprof_behaviour;

  toyprof_behaviour = 0;	/* stop profiling */
  
  n_ticks = toyprof_stamp_ticks ();
  dprintf (fd, "\n");
  dprintf (fd, "TOYPROFMETA: device = %s\n", toyprof_clock_name ());
  dprintf (fd, "TOYPROFMETA: columns/;/ =Percentage;Total;Average;# Calls;Function;Caller\n");
  dprintf (fd, "TOYPROFMETA: ticks_per_second = %f\n", n_ticks);
  for (i = 0; i < toyprof_root.n_nodes; i++)
    total_ticks += toyprof_root.nodes[i].uticks;
  total_ticks = MAX (total_ticks, 0.5);	/* prevent broken timers */
  dprintf (fd, "TOYPROFMETA: total_ticks = %f\n", total_ticks);
  dprintf (fd, "TOYPROFMETA: total_time = %f\n", total_ticks / n_ticks);
  total_ticks /= 100;
  for (i = 0; i < toyprof_root.n_nodes; i++)
    {
      ToyprofEntry *e = toyprof_root.nodes + i;
      char *name, *caller, *string;
      int nresolved, cresolved;
      
      name = toyprof_dlname (e->cur_func, &nresolved);
      caller = toyprof_dlname (e->call_site, &cresolved);
      asprintf (&string, " %.16f  %.16f  %.16f  %llu",
		e->uticks / total_ticks,
		e->uticks / n_ticks,
		(e->uticks / (double) e->n_calls) / n_ticks,
		e->n_calls);
      dprintf (fd, "TOYPROFDATA:  %s  %s  %s \n",
	       string,
	       name, caller);
      free (string);
      free (name);
      free (caller);
    }

  toyprof_behaviour = saved_behaviour;
}

static void TOYPROF_GNUC_NO_INSTRUMENT
toyprof_atexit (void)
{
  toyprof_dump_stats (2);
}

static void TOYPROF_GNUC_NO_INSTRUMENT TOYPROF_GNUC_UNUSED
toyprof_func_enter (void *cur_func,
		    void *call_site)
{
  ToyprofStamp stamp;
  void *caller;
  ToyprofEntry *e, *c;
  
  toyprof_stamp (stamp);	/* stop timing */
  if (!toyprof_behaviour)
    return;
  
  if (!toyprof_root.n_nodes)	/* initialization */
    {
      toyprof_stampinit ();
      toyprof_push (&toyprof_root, NULL); /* make peek for caller work */
      toyprof_push (&toyprof_root, NULL); /* make peekpeek for caller's parent work */
      atexit (toyprof_atexit);
    }

  /* last function entry we recognized */
  caller = toyprof_peek (&toyprof_root);

  /* get profiling entry for this (function,caller) combination */
  e = toyprof_lookup (&toyprof_root, cur_func, caller, FALSE);
  if (!e || e->cur_func != cur_func || e->caller != caller)	/* none or inexact match */
    {
      unsigned int insertion_pos;

      /* no profiling entry found, need to insert a new one */
      if (e)
	{
	  insertion_pos = e - toyprof_root.nodes;		/* insert before neighbour */
	  if (cur_func > e->cur_func ||
	      (cur_func == e->cur_func && caller > e->caller))
	    insertion_pos += 1;				/* insert after neighbour */
	}
      else
	insertion_pos = 0;
      e = toyprof_insert (&toyprof_root, insertion_pos);
      e->cur_func = cur_func;
      e->caller = caller;
      e->call_site = call_site;
      e->uticks = 0;
      e->n_calls = 0;
      if ((toyprof_behaviour & TOYPROF_TRACE_FUNCTIONS) == TOYPROF_TRACE_FUNCTIONS)
	e->tmp_name = toyprof_dlname (e->cur_func, NULL);
    }

  /* update profiling stats for this function */
  e->n_calls += 1;

  /* update profiling stats for our caller (don't account child function time) */
  c = toyprof_lookup (&toyprof_root, caller, toyprof_peekpeek (&toyprof_root), TRUE);
  if (c)	/* might not have a caller, e.g. in main() */
    c->uticks += toyprof_elapsed (c->stamp, stamp);

  if ((toyprof_behaviour & TOYPROF_TRACE_FUNCTIONS) == TOYPROF_TRACE_FUNCTIONS)
    dprintf (2, "ENTER: %s\n", e->tmp_name);

  toyprof_push (&toyprof_root, cur_func);	/* preserve caller for next entry */

  toyprof_stamp (e->stamp);	/* start timing */
}

static void TOYPROF_GNUC_NO_INSTRUMENT TOYPROF_GNUC_UNUSED
toyprof_func_exit (void *cur_func,
		   void *call_site)
{
  ToyprofStamp stamp;
  void *caller;
  ToyprofEntry *e, *c;
  
  toyprof_stamp (stamp);	/* stop timing */
  if (!toyprof_behaviour)
    return;

  toyprof_pop (&toyprof_root);	/* throw away preserved caller for entries */

  /* last function entry we recognized */
  caller = toyprof_peek (&toyprof_root);

  /* get profiling entry for this (function,caller) combination */
  e = toyprof_lookup (&toyprof_root, cur_func, caller, TRUE);
  /* if we got to this pair in _exit, we must have handled it in _entry */
  TOYPROF_ASSERT (e && e->cur_func == cur_func && e->caller == caller);

  /* update profiling stats for this function */
  e->uticks += toyprof_elapsed (e->stamp, stamp);
  
  if ((toyprof_behaviour & TOYPROF_TRACE_FUNCTIONS) == TOYPROF_TRACE_FUNCTIONS)
    dprintf (2, "EXIT: %s (n_calls:%lld)\n", e->tmp_name, e->n_calls);

  /* update profiling stats for our caller (child exited, restart accounting) */
  c = toyprof_lookup (&toyprof_root, caller, toyprof_peekpeek (&toyprof_root), TRUE);
  if (c)
    toyprof_stamp (c->stamp);	/* start timing */
}

void TOYPROF_GNUC_NO_INSTRUMENT
toyprof_set_profiling (ToyprofBehaviour behav)
{
  toyprof_behaviour = behav & (TOYPROF_PROFILE_TIMING |
			       TOYPROF_TRACE_FUNCTIONS);
}

static char* TOYPROF_GNUC_NO_INSTRUMENT
toyprof_dlname (void *addr,
		int  *resolved_p)
{
  Dl_info dlinfo;
  int result = toyprof_dladdr (addr, &dlinfo);
  char *name;
  int resolved = TRUE;
  
  if (result)
    {
      if (dlinfo.dli_saddr == addr && dlinfo.dli_sname)
	asprintf (&name, "%s", dlinfo.dli_sname);
      else if (FALSE /* DISABLE, addr2line does better */ && dlinfo.dli_saddr && dlinfo.dli_sname)
	asprintf (&name, "%s%c0x%x",
		  dlinfo.dli_sname,
		  addr > dlinfo.dli_saddr ? '+' : '-',
		  MAX (addr, dlinfo.dli_saddr) - MIN (addr, dlinfo.dli_saddr));
      else
	{
	  asprintf (&name, "%s:%p",	/* needs addr2line lookup */
		    dlinfo.dli_fname,
		    (void*) (addr - dlinfo.dli_fbase));
	  resolved = FALSE;
	}
    }
  else
    {
      asprintf (&name, "???:%p", addr);
      resolved = FALSE;
    }
  if (resolved_p)
    *resolved_p = resolved;
  
  return name;
}

static int TOYPROF_GNUC_NO_INSTRUMENT
toyprof_dladdr (const void *sym_address,
		Dl_info    *dlinfo)
{
  struct link_map *link, *best, *mlink = NULL, *lmain = NULL;
  ElfW(Addr) addr = (long) sym_address;
  ElfW(Dyn) *dyn;
  ElfW(Sym) *sym, *symtab, *msym = NULL;
  ElfW(Addr) strtabsize;
  const char *mstrtab = NULL, *strtab;
  
  /* let dladdr() fill in fbase and name */
  if (!dladdr (sym_address, dlinfo))
    return 0;
  dlinfo->dli_sname = NULL;
  dlinfo->dli_saddr = NULL;
  /* need to adjust main module handle */
  for (link = _r_debug.r_map; link; link = link->l_next)
    if (link->l_addr == (long) dlinfo->dli_fbase)
      break;
    else if (!link->l_addr)
      lmain = link;
  if (!link)	/* dladdr() returned main module */
    dlinfo->dli_fbase = (void*) lmain->l_addr;	/* always 0 */
  
  /* now lets do our own run to find the correct module */
  best = NULL;
  for (link = _r_debug.r_map; link; link = link->l_next)
    {
      if (addr > link->l_addr &&
	  (!best || link->l_addr > best->l_addr))
	best = link;
      if (!link->l_addr)
	lmain = link;
    }
  if (!best)
    best = lmain;
  
 next_module:
  link = best;
  
  strtabsize = 0;
  strtab = NULL;
  symtab = NULL;
  for (dyn = link->l_ld; dyn->d_tag != DT_NULL; dyn++)
    if (dyn->d_tag == DT_SYMTAB && dyn->d_un.d_ptr)
      symtab = (void*) dyn->d_un.d_ptr;
    else if (dyn->d_tag == DT_STRTAB && dyn->d_un.d_ptr)
      strtab = (void*) dyn->d_un.d_ptr;
    else if (dyn->d_tag == DT_STRSZ)
      strtabsize = dyn->d_un.d_val;
  
  if (!strtab || !symtab || !strtabsize || (void*) strtab < (void*) symtab)
    return 1;
  
  for (sym = symtab; (void*) sym < (void*) strtab; sym++)
    {
      if (0)
	dprintf (2, "SYM: %5x %8x %5x %3x %3x %5x \"%s\"\n",
		 sym->st_name, sym->st_value, sym->st_size, sym->st_info & 15,
		 sym->st_other, sym->st_shndx, strtab + sym->st_name);
      
      if (/* confine matches to symbol boundaries */
	  addr >= sym->st_value + link->l_addr &&
	  addr <= sym->st_value + link->l_addr + sym->st_size &&
	  /* sym can point to junk already since we don't know symtab size,
	   * so ensure we don't exceede strtab boundary
	   */
	  sym->st_name < strtabsize &&
	  /* match only STB_GLOBAL or STB_WEAK symbols */
	  ((sym->st_info & 15) == STB_GLOBAL || (sym->st_info & 15) == STB_WEAK) &&
	  (!msym || sym->st_value > msym->st_value))
	{
	  msym = sym;
	  mstrtab = strtab;
	  mlink = link;
	  
	  // dprintf (2, "CANDIDATE(%p) \"%s\"\n", msym, msym ? mstrtab + msym->st_name : NULL);
	}
    }
  // dprintf (2, "MATCH(%p) \"%s\"\n", msym, msym ? mstrtab + msym->st_name : NULL);
  
  if (best != lmain)
    {
      best = lmain;
      goto next_module;
    }
  
  if (msym)
    {
      if (dlinfo->dli_fbase != (void*) mlink->l_addr)
	{
	  dlinfo->dli_fname = mlink->l_name;
	  dlinfo->dli_fbase = (void*) mlink->l_addr;
	}
      dlinfo->dli_sname = mstrtab + msym->st_name;
      dlinfo->dli_saddr = dlinfo->dli_fbase + msym->st_value;
    }
  return 1;
}

#if	!defined (TOYPROF_DISABLE) || TOYPROF_DISABLE == 0
TOYPROF_SYMBOL_ALIAS (__cyg_profile_func_enter, toyprof_func_enter);
TOYPROF_SYMBOL_ALIAS (__cyg_profile_func_exit, toyprof_func_exit);
#endif

#endif	/* TOYPROF_GNUC_NO_INSTRUMENT, GNU GCC check */
