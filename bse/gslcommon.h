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
#ifndef __GSL_COMMON_H__
#define __GSL_COMMON_H__

#include <gsl/gsldefs.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- global initialization --- */
typedef struct
{
  const char *value_name;
  double      value;
} GslConfigValue;
typedef struct
{
  guint  n_processors;
  /* # values to pad around wave chunk blocks per channel */
  guint  wave_chunk_padding;
  guint  wave_chunk_big_pad;
  /* data (file) cache block size (aligned to power of 2) */
  guint	 dcache_block_size;
  /* amount of bytes to spare for memory cache */
  guint  dcache_cache_memory;
  guint  midi_kammer_note;
  /* kammer frequency, normally 440Hz, historically 435Hz */
  gfloat kammer_freq;
} GslConfig;
typedef struct _GslMutexTable GslMutexTable;
void			gsl_init	(const GslConfigValue	values[],
					 GslMutexTable	       *mtable);
const GslConfig*	gsl_get_config	(void) G_GNUC_CONST;
#define	GSL_CONFIG(value)	((gsl_get_config () [0]) . value)


/* --- memory allocation --- */
#define gsl_new_struct(type, n)		((type*) gsl_alloc_memblock (sizeof (type) * (n)))
#define gsl_new_struct0(type, n)	((type*) gsl_alloc_memblock0 (sizeof (type) * (n)))
#define gsl_delete_struct(type, mem)	(gsl_delete_structs (type, 1, (mem)))
#ifndef	__GNUC__
#  define gsl_delete_structs(type, n, mem)	(gsl_free_memblock (sizeof (type) * (n), (mem)))
#else					/* provide typesafety if possible */
#  define gsl_delete_structs(type, n, mem)	({ \
  type *__typed_pointer = (mem); \
  gsl_free_memblock (sizeof (type) * (n), __typed_pointer); \
})
#endif
#define	GSL_ALIGNED_SIZE(size,align)	((align) > 0 ? _GSL_INTERN_ALIGN (((gsize) (size)), ((gsize) (align))) : (gsize) (size))
#define	_GSL_INTERN_ALIGN(s, a)		(((s + (a - 1)) / a) * a)
#define	GSL_STD_ALIGN			(MAX (MAX (sizeof (float), sizeof (int)), sizeof (void*)))


/* --- ring (circular-list) --- */
struct _GslRing
{
  GslRing  *next;
  GslRing  *prev;
  gpointer  data;
};
GslRing*	gsl_ring_prepend	(GslRing	*head,
					 gpointer	 data);
GslRing*	gsl_ring_prepend_uniq	(GslRing	*head,
					 gpointer	 data);
GslRing*	gsl_ring_append		(GslRing	*head,
					 gpointer	 data);
GslRing*	gsl_ring_concat		(GslRing	*head1,
					 GslRing	*head2);
GslRing*	gsl_ring_remove_node	(GslRing	*head,
					 GslRing	*node);
GslRing*	gsl_ring_remove		(GslRing	*head,
					 gpointer	 data);
guint		gsl_ring_length		(GslRing	*head);
GslRing*	gsl_ring_find		(GslRing	*head,
					 gconstpointer	 data);
GslRing*	gsl_ring_nth		(GslRing	*head,
					 guint           n);
gpointer	gsl_ring_nth_data	(GslRing	*head,
					 guint           n);
gpointer	gsl_ring_pop_head	(GslRing       **head);
gpointer	gsl_ring_pop_tail	(GslRing       **head);
void		gsl_ring_free		(GslRing	*head);
#define gsl_ring_walk(head,node)	((node) != (head)->prev ? (node)->next : NULL)


/* --- GslMessage and debugging --- */
typedef enum /*< skip >*/
{
  GSL_MSG_NOTIFY	= 1 << 0,
  GSL_MSG_DATA_CACHE	= 1 << 1,
  GSL_MSG_DATA_HANDLE	= 1 << 2,
  GSL_MSG_LOADER	= 1 << 3,
  GSL_MSG_OSC		= 1 << 4,
  GSL_MSG_ENGINE	= 1 << 5,
  GSL_MSG_JOBS		= 1 << 6,
  GSL_MSG_SCHED		= 1 << 7,
  GSL_MSG_MASTER	= 1 << 8,
  GSL_MSG_SLAVE		= 1 << 9
} GslDebugFlags;
extern const GDebugKey *gsl_debug_keys;
extern const guint      gsl_n_debug_keys;
void		gsl_debug		(GslDebugFlags  reporter,
					 const gchar   *section,
					 const gchar   *format,
					 ...)	G_GNUC_PRINTF (3, 4);
void		gsl_debug_enable	(GslDebugFlags	dbg_flags);
void		gsl_debug_disable	(GslDebugFlags	dbg_flags);
gboolean	gsl_debug_check		(GslDebugFlags	dbg_flags);
void		gsl_message_send	(GslDebugFlags  reporter,
					 const gchar   *section,  /* maybe NULL */
					 GslErrorType	error,	  /* maybe 0 */
					 const gchar   *messagef,
					 ...)	G_GNUC_PRINTF (4, 5);
const gchar*	gsl_strerror		(GslErrorType	error);

/* provide message/debugging macro templates, so custom messages
 * are done as:
 * #define FOO_DEBUG	GSL_DEBUG_FUNCTION (GSL_MSG_LOADER, "FOO")
 * FOO_DEBUG ("some debug message and number: %d", 5);
 */
#define GSL_DEBUG_FUNCTION(reporter, section)	_GSL_DEBUG_MACRO_IMPL((reporter), (section))
#define GSL_MESSAGE_FUNCTION(reporter, section)	_GSL_MESSAGE_MACRO_IMPL((reporter), (section))


/* --- GslThread --- */
typedef void (*GslThreadFunc)		(gpointer	user_data);
GslThread*	gsl_thread_new		(GslThreadFunc	func,
					 gpointer	user_data);
guint		gsl_threads_get_count	(void);
GslThread*	gsl_thread_self		(void);
GslThread*	gsl_thread_main		(void);


/* --- thread syncronization --- */
gboolean	gsl_thread_sleep	(glong		 max_msec);
gboolean	gsl_thread_aborted	(void);
void		gsl_thread_queue_abort	(GslThread	*thread);
void		gsl_thread_abort	(GslThread	*thread);
void		gsl_thread_wakeup	(GslThread	*thread);
void		gsl_thread_awake_after	(guint64	 tick_stamp);
void		gsl_thread_awake_before	(guint64	 tick_stamp);
void		gsl_thread_get_pollfd	(GPollFD	*pfd);
guint64		gsl_tick_stamp		(void);
#define		GSL_TICK_STAMP		(_GSL_TICK_STAMP_VAL ())
#define		GSL_MAX_TICK_STAMP	(~((guint64) 0))


/* --- GslMutex --- */
#define	gsl_mutex_init(mutex)		(gsl_mutex_table.mutex_init (mutex))
#define GSL_SPIN_LOCK(mutex)		(gsl_mutex_table.mutex_lock (mutex))
#define GSL_SPIN_UNLOCK(mutex)		(gsl_mutex_table.mutex_unlock (mutex))
#define GSL_SYNC_LOCK(mutex)		(gsl_mutex_table.mutex_lock (mutex))
#define GSL_SYNC_UNLOCK(mutex)		(gsl_mutex_table.mutex_unlock (mutex))
#define	gsl_mutex_trylock(mutex)	(!gsl_mutex_table.mutex_trylock (mutex))
#define	gsl_mutex_destroy(mutex)	(gsl_mutex_table.mutex_destroy (mutex))
#define	gsl_rec_mutex_init(rmutex)	(gsl_mutex_table.rec_mutex_init (rmutex))
#define	gsl_rec_mutex_lock(rmutex)	(gsl_mutex_table.rec_mutex_lock (rmutex))
#define	gsl_rec_mutex_unlock(rmutex)	(gsl_mutex_table.rec_mutex_unlock (rmutex))
#define	gsl_rec_mutex_trylock(rmutex)	(!gsl_mutex_table.rec_mutex_trylock (rmutex))
#define	gsl_rec_mutex_destroy(rmutex)	(gsl_mutex_table.rec_mutex_destroy (rmutex))
#define	gsl_cond_init(cond)		(gsl_mutex_table.cond_init (cond))
#define	gsl_cond_signal(cond)		(gsl_mutex_table.cond_signal (cond))
#define	gsl_cond_broadcast(cond)	(gsl_mutex_table.cond_broadcast (cond))
#define	gsl_cond_wait(cond, mutex)	(gsl_mutex_table.cond_wait ((cond), (mutex)))
#define	gsl_cond_destroy(cond)		(gsl_mutex_table.cond_destroy (cond))
void	gsl_cond_wait_timed		(GslCond  *cond,
					 GslMutex *mutex,
					 glong     max_useconds);
struct _GslMutexTable
{
  void	(*mutex_init)		(GslMutex	*mutex);
  void	(*mutex_lock)		(GslMutex	*mutex);
  int	(*mutex_trylock)	(GslMutex	*mutex); /* 0==has_lock */
  void	(*mutex_unlock)		(GslMutex	*mutex);
  void	(*mutex_destroy)	(GslMutex	*mutex);
  void	(*rec_mutex_init)	(GslRecMutex	*mutex);
  void	(*rec_mutex_lock)	(GslRecMutex	*mutex);
  int	(*rec_mutex_trylock)	(GslRecMutex	*mutex); /* 0==has_lock */
  void	(*rec_mutex_unlock)	(GslRecMutex	*mutex);
  void	(*rec_mutex_destroy)	(GslRecMutex	*mutex);
  void	(*cond_init)		(GslCond	*cond);
  void	(*cond_signal)		(GslCond	*cond);
  void	(*cond_broadcast)	(GslCond	*cond);
  void	(*cond_wait)		(GslCond	*cond,
				 GslMutex	*mutex);
  void	(*cond_wait_timed)	(GslCond	*cond,
				 GslMutex	*mutex,
				 gulong		 abs_secs,
				 gulong		 abs_usecs);
  void	(*cond_destroy)		(GslCond	*cond);
};


/* --- misc --- */
const gchar* gsl_byte_order_to_string   (guint           byte_order);
guint        gsl_byte_order_from_string (const gchar    *string);
GslErrorType gsl_check_file		(const gchar	*file_name,
					 const gchar	*mode);
gboolean     gsl_check_file_mtime	(const gchar	*file_name,
					 GTime		 mtime);


/* --- implementation details --- */
gpointer	gsl_alloc_memblock	(gsize		 size);
gpointer	gsl_alloc_memblock0	(gsize		 size);
void		gsl_free_memblock	(gsize		 size,
					 gpointer	 memblock);
void		gsl_alloc_report	(void);
const guint	gsl_alloc_upper_power2	(const gulong	 number);
void	       _gsl_tick_stamp_inc	(void);
void	       _gsl_tick_stamp_set_leap (guint		 ticks);
void	_gsl_init_signal		(void);
void	_gsl_init_data_handles		(void);
void	_gsl_init_data_caches		(void);
void	_gsl_init_engine_utils		(void);
void	_gsl_init_loader_gslwave	(void);
void	_gsl_init_loader_wav		(void);
void	_gsl_init_loader_oggvorbis	(void);
void	_gsl_init_loader_mad		(void);
#define		GSL_N_IO_RETRIES	(5)
#define		_GSL_TICK_STAMP_VAL()	(gsl_externvar_tick_stamp + 0)
extern volatile guint64	gsl_externvar_tick_stamp;
extern GslMutexTable gsl_mutex_table;

/* we need to provide a REPORTER and SECTION string for the debugging
 * and message generation functions. for GCC, we also want to make use
 * of printf style format checking with G_GNUC_PRINTF(). for the non GCC
 * case, we push REPORTER and SECTION as thread specific data before
 * invoking the debugging/message generation function. for the GCC case
 * we use GNUC varargs. (using ISO varargs wouldn't provide any benefit,
 * for one, ISO vararg support is broken with gcc-2.95 and ansi/c++, and
 * we only need the macro magic for GCC in the first place to make use
 * of G_GNUC_PRINTF()).
 */
#if     __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define _GSL_DEBUG_MACRO_IMPL(reporter, section)	gsl_debug ((reporter), (section), _GSL_DEBUG_GCCTAIL
#define _GSL_DEBUG_GCCTAIL(GCCARGS...)			GCCARGS )
#define _GSL_MESSAGE_MACRO_IMPL(reporter, section)	gsl_message_send ((reporter), (section), _GSL_MESSGAE_GCCTAIL
#define _GSL_MESSGAE_GCCTAIL(GCCARGS...)		GCCARGS )
#else   /* non GCC, push data and invoke function */
#define _GSL_DEBUG_MACRO_IMPL(reporter, section)	(gsl_auxlog_push ((reporter), (section)), gsl_auxlog_debug)
#define _GSL_MESSAGE_MACRO_IMPL(reporter, section)	(gsl_auxlog_push ((reporter), (section)), gsl_auxlog_message)
#endif
/* non-GCC message helpers */
void		gsl_auxlog_push		(GslDebugFlags   reporter,
					 const gchar	*section);
void		gsl_auxlog_debug	(const gchar	*format,
					 ...);
void		gsl_auxlog_message	(GslErrorType	 error,
					 const gchar	*format,
					 ...);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_COMMON_H__ */
