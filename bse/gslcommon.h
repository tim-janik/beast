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
void			gsl_init	(const GslConfigValue	values[]);
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


/* --- GslMessage --- */
#define	GSL_MSG_NOTIFY		"Notify"
#define	GSL_MSG_DATA_CACHE	"DataCache"
#define	GSL_MSG_DATA_HANDLE	"DataHandle"
#define	GSL_MSG_LOADER		"GslLoader"
void		gsl_message_send	(const gchar   *reporter, /* GSL_MSG_* */
					 GslErrorType	error,	  /* maybe 0 */
					 const gchar   *messagef,
					 ...)	G_GNUC_PRINTF (3, 4);
const gchar*	gsl_strerror		(GslErrorType	error);


/* --- GslThread --- */
typedef void (*GslThreadFunc)		(gpointer	user_data);
GslThread*	gsl_thread_new		(GslThreadFunc	func,
					 gpointer	user_data);
guint		gsl_threads_get_count	(void);
GslThread*	gsl_thread_self		(void);


/* --- thread syncronization --- */
gboolean	gsl_thread_sleep	(glong		 max_msec);
gboolean	gsl_thread_aborted	(void);
void		gsl_thread_queue_abort	(GslThread	*thread);
void		gsl_thread_abort	(GslThread	*thread);
void		gsl_thread_wakeup	(GslThread	*thread);
void		gsl_thread_awake_after	(guint64	 tick_stamp);
void		gsl_thread_awake_before	(guint64	 tick_stamp);
void		gsl_thread_get_pollfd	(GslPollFD	*pfd);
guint64		gsl_tick_stamp		(void);
#define		GSL_TICK_STAMP		(_GSL_TICK_STAMP_VAL ())
#define		GSL_MAX_TICK_STAMP	(~((guint64) 0))


/* --- GslMutex --- */
#define GSL_SPIN_LOCK(mutex)	 gsl_mutex_spin_lock (mutex)
#define GSL_SPIN_UNLOCK(mutex)	 gsl_mutex_unlock (mutex)
#define GSL_SYNC_LOCK(mutex)	 gsl_mutex_sync_lock (mutex)
#define GSL_SYNC_UNLOCK(mutex)	 gsl_mutex_unlock (mutex)
void	gsl_mutex_init		(GslMutex	*mutex);
void	gsl_mutex_destroy	(GslMutex	*mutex);


/* --- GslRecMutex --- */
void	gsl_rec_mutex_init	(GslRecMutex	*rec_mutex);
void	gsl_rec_mutex_destroy	(GslRecMutex	*rec_mutex);
void	gsl_rec_mutex_lock	(GslRecMutex	*rec_mutex);
void	gsl_rec_mutex_unlock	(GslRecMutex	*rec_mutex);


/* --- GslCond --- */
GslCond* gsl_cond_new		(void);
#if 0
void	 gsl_cond_wait_timed	(GslCond	*cond,
				 GslMutex	*mutex,
				 glong		 max_useconds);
#endif
void	 gsl_cond_wait		(GslCond	*cond,
				 GslMutex	*mutex);
void	 gsl_cond_signal	(GslCond	*cond);
void	 gsl_cond_broadcast	(GslCond	*cond);
void	 gsl_cond_destroy	(GslCond	*cond);


/* --- misc --- */
const gchar* gsl_byte_order_to_string   (guint           byte_order);
guint        gsl_byte_order_from_string (const gchar    *string);


/* --- implementation details --- */
void		gsl_mutex_spin_lock	(GslMutex	*mutex);
void		gsl_mutex_sync_lock	(GslMutex	*mutex);
void		gsl_mutex_unlock	(GslMutex	*mutex);
gpointer	gsl_alloc_memblock	(gsize		 size);
gpointer	gsl_alloc_memblock0	(gsize		 size);
void		gsl_free_memblock	(gsize		 size,
					 gpointer	 memblock);
void		gsl_alloc_report	(void);
const guint	gsl_alloc_upper_power2	(const gulong	 number);
gboolean	gsl_rec_mutex_test_self	(GslRecMutex	*rec_mutex);
void	       _gsl_init_data_handles	(void);
void	       _gsl_init_data_caches	(void);
void	       _gsl_init_engine_utils	(void);
void	       _gsl_init_loader_gslwave (void);
void	       _gsl_init_loader_wav     (void);
void	       _gsl_tick_stamp_inc	(void);
void	       _gsl_tick_stamp_set_leap (guint		 ticks);
#define		GSL_N_IO_RETRIES	(5)
#define		_GSL_TICK_STAMP_VAL()	(gsl_externvar_tick_stamp + 0)
extern volatile guint64	gsl_externvar_tick_stamp;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_COMMON_H__ */
