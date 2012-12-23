// CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0/
// TOYPROF - Poor man's profiling toy
#ifndef	__TOYPROF_MEM_H__
#define __TOYPROF_MEM_H__

#include <glib.h>


G_BEGIN_DECLS

/* causes toyprof to setup hooks via g_mem_set_vtable() */
void	toyprof_init_glib_memtable	(const gchar	*file_name,
					 gint		 logger_signal);
/* start logging memory allocations */
guint	toyprof_start_leak_logger	(void);
/* report memory statistics */
void	toyprof_dump_leaks		(guint		leak_logger_stamp,
					 gint		fd);

/* total amount of memory currently allocated */
extern volatile gulong toyprof_memtotal;

G_END_DECLS

#endif	/* __TOYPROF_MEM_H__ */
