/* TOYPROF - Poor man's profiling toy
 * Copyright (C) 2000-2002 Tim Janik
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef	__TOYPROF_MEM_H__
#define __TOYPROF_MEM_H__

#include <glib.h>


G_BEGIN_DECLS


/* number of buckets to trace */
#define TOYPROF_MEM_TABLE_SIZE 4096

/* report memory statistics */
void               toyprof_report_mem   (void);

/* for g_mem_set_vtable (toyprof_mem_table); */
extern GMemVTable *toyprof_mem_table;

/* Gdb hooks */
extern volatile gulong toyprof_trap_free_size;
extern volatile gulong toyprof_trap_realloc_size;
extern volatile gulong toyprof_trap_malloc_size;


G_END_DECLS

#endif	/* __TOYPROF_MEM_H__ */
