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
