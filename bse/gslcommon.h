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
#ifndef	__GNUC__
#  define gsl_delete_struct(type, n, mem)	(gsl_free_memblock (sizeof (type) * (n), (mem)))
#else					/* provide typesafety if possible */
#  define gsl_delete_struct(type, n, mem)	({ \
  type *__typed_pointer = (mem); \
  gsl_free_memblock (sizeof (type) * (n), __typed_pointer); \
})
#endif


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
gpointer	gsl_ring_pop_head	(GslRing       **head);
gpointer	gsl_ring_pop_tail	(GslRing       **head);
void		gsl_ring_free		(GslRing	*head);
#define gsl_ring_walk(head,node)	((node) != (head)->prev ? (node)->next : NULL)


/* --- GslMessage --- */
typedef enum	/*< skip >*/
{
  GSL_MSG_NOTIFY,
  GSL_MSG_DATA_CACHE,
  GSL_MSG_WAVE_DATA_HANDLE,
  GSL_MSG_LAST
} GslMsgType;
typedef enum	/*< skip >*/
{
  GSL_ERROR_NONE,
  GSL_ERROR_INTERNAL,
  GSL_ERROR_UNKNOWN,
  GSL_ERROR_IO,
  GSL_ERROR_NOT_FOUND,
  GSL_ERROR_READ_FAILED,
  GSL_ERROR_SEEK_FAILED
} GslErrorType;
void		gsl_message_send	(GslMsgType	msgtype,
					 GslErrorType	error,
					 const gchar   *reasonf,
					 ...)	G_GNUC_PRINTF (3, 4);
const gchar*	gsl_strerror		(GslErrorType	error);


/* --- GslThread --- */
typedef void (*GslThreadFunc)		(gpointer	user_data);
gboolean	gsl_thread_new		(GslThreadFunc	func,
					 gpointer	user_data);
guint		gsl_threads_get_count	(void);
gpointer	gsl_thread_self		(void);


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
void		_gsl_init_data_handles	(void);
void		_gsl_init_data_caches	(void);
void		_gsl_init_wave_dsc	(void);
void		_gsl_init_engine_utils	(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_COMMON_H__ */
