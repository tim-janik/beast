/* BirnetThread
 * Copyright (C) 2002-2006 Tim Janik
 * Copyright (C) 2002 Stefan Westerfeld
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __BIRNET_THREADS_H__
#define __BIRNET_THREADS_H__

#include <stdbool.h>
#include <glib.h>
#include <birnet/birnetconfig.h>

G_BEGIN_DECLS

/* --- global initializer --- */
void	birnet_init (const gchar	*prg_name);

/* --- typedefs --- */
typedef struct _BirnetThreadTable	 BirnetThreadTable;
typedef struct _BirnetThread		 BirnetThread;
typedef union  _BirnetMutex		 BirnetMutex;
typedef union  _BirnetCond		 BirnetCond;
typedef struct _BirnetRecMutex		 BirnetRecMutex;

/* --- BirnetThread --- */
typedef void (*BirnetThreadFunc)		(gpointer	 user_data);
typedef void (*BirnetThreadWakeup)		(gpointer	 wakeup_data);
BirnetThread*	birnet_thread_run		(const gchar    *name,
						 BirnetThreadFunc	 func,
						 gpointer	 user_data);
BirnetThread*   birnet_thread_self		(void);
gint          	birnet_thread_self_pid		(void);
gint	      	birnet_thread_get_pid		(BirnetThread      *thread);
const gchar*  	birnet_thread_get_name		(BirnetThread      *thread);
void	      	birnet_thread_set_name		(const gchar    *name);
bool          	birnet_thread_sleep		(glong		 max_useconds);
bool          	birnet_thread_aborted		(void);
void	      	birnet_thread_queue_abort	(BirnetThread	*thread);
void	      	birnet_thread_abort		(BirnetThread	*thread);
void	      	birnet_thread_wakeup		(BirnetThread	*thread);
void	      	birnet_thread_awake_after	(guint64	 stamp);
void	      	birnet_thread_emit_wakeups	(guint64	 stamp);
void	      	birnet_thread_set_wakeup	(BirnetThreadWakeup wakeup_func,
						 gpointer	 wakeup_data,
						 GDestroyNotify	 destroy);
gpointer      	birnet_thread_get_qdata		(GQuark		 quark);
void	      	birnet_thread_set_qdata_full	(GQuark		 quark,
						 gpointer	 data,
						 GDestroyNotify	 destroy);
gpointer      	birnet_thread_steal_qdata	(GQuark		 quark);
#define	      	birnet_thread_set_qdata(      quark, data)	birnet_thread_set_qdata_full ((quark), (data), NULL)
#define	      	birnet_thread_get_data(       name)		birnet_thread_get_qdata (g_quark_try_string (name))
#define	      	birnet_thread_set_data(       name, data)	birnet_thread_set_qdata_full (g_quark_from_string (name), (data), NULL)
#define	      	birnet_thread_set_data_full(  name, data, x)	birnet_thread_set_qdata_full (g_quark_from_string (name), (data), (x))
#define	      	birnet_thread_steal_data(     name)		birnet_thread_steal_qdata (g_quark_try_string (name))

/* --- thread info --- */
typedef enum /*< skip >*/
  {
    BIRNET_THREAD_UNKNOWN    = '?',
    BIRNET_THREAD_RUNNING    = 'R',
    BIRNET_THREAD_SLEEPING   = 'S',
    BIRNET_THREAD_DISKWAIT   = 'D',
    BIRNET_THREAD_TRACED     = 'T',
    BIRNET_THREAD_PAGING     = 'W',
    BIRNET_THREAD_ZOMBIE     = 'Z',
    BIRNET_THREAD_DEAD       = 'X',
  } BirnetThreadState;
typedef struct {
  gint           	thread_id;
  gchar                *name;
  guint          	aborted : 1;
  BirnetThreadState 	state;
  gint           	priority;      /* nice value */
  gint           	processor;     /* running processor # */
  guint64        	utime;         /* user time */
  guint64        	stime;         /* system time */
  guint64        	cutime;        /* user time of dead children */
  guint64        	cstime;        /* system time of dead children */
} BirnetThreadInfo;
BirnetThreadInfo*	birnet_thread_info_collect (BirnetThread      *thread);
void            	birnet_thread_info_free    (BirnetThreadInfo  *info);

/* --- hazard pointers / thread guards --- */
typedef struct  BirnetGuard                 BirnetGuard;
volatile BirnetGuard* birnet_guard_register    	 (guint			n_hazards);
void                  birnet_guard_deregister  	 (volatile BirnetGuard *guard);
static inline void    birnet_guard_protect     	 (volatile BirnetGuard *guard,
						  guint           	nth_hazard,
						  gpointer        	value);
guint		      birnet_guard_n_snap_values (void);
bool                  birnet_guard_snap_values   (guint                *n_values,
						  gpointer             *values);
bool                  birnet_guard_is_protected  (gpointer             	value);

/* --- BirnetMutex & BirnetCond --- */
#define birnet_mutex_init(mutex)		(birnet_thread_table.mutex_init (mutex))
#define birnet_mutex_lock(mutex)		(birnet_thread_table.mutex_lock (mutex))
#define birnet_mutex_unlock(mutex)		(birnet_thread_table.mutex_unlock (mutex))
#define birnet_mutex_trylock(mutex)		(birnet_thread_table.mutex_trylock (mutex) == 0) /* TRUE indicates success */
#define birnet_mutex_destroy(mutex)		(birnet_thread_table.mutex_destroy (mutex))
#define birnet_rec_mutex_init(rmutex)		(birnet_thread_table.rec_mutex_init (rmutex))
#define birnet_rec_mutex_lock(rmutex)		(birnet_thread_table.rec_mutex_lock (rmutex))
#define birnet_rec_mutex_unlock(rmutex)		(birnet_thread_table.rec_mutex_unlock (rmutex))
#define birnet_rec_mutex_trylock(rmutex)	(birnet_thread_table.rec_mutex_trylock (rmutex) == 0) /* TRUE indicates success */
#define birnet_rec_mutex_destroy(rmutex)	(birnet_thread_table.rec_mutex_destroy (rmutex))
#define birnet_cond_init(cond)			(birnet_thread_table.cond_init (cond))
#define birnet_cond_signal(cond)		(birnet_thread_table.cond_signal (cond))
#define birnet_cond_broadcast(cond)		(birnet_thread_table.cond_broadcast (cond))
#define birnet_cond_wait(cond, mutex)      	(birnet_thread_table.cond_wait ((cond), (mutex)))
#define birnet_cond_destroy(cond)		(birnet_thread_table.cond_destroy (cond))
void    birnet_cond_wait_timed			(BirnetCond    *cond,
						 BirnetMutex   *mutex,
						 glong	   	max_useconds);

/* --- atomic operations --- */
#define birnet_atomic_set(PtrType, atomic_ptr_adr, new_ptr)        	  ((void(*)(PtrType*,PtrType)) (void*) birnet_atomic_set_impl) (atomic_ptr_adr, new_ptr)
#define birnet_atomic_get(PtrType, atomic_ptr_adr)                 	  ((PtrType(*)(PtrType*)) (void*) birnet_atomic_get_impl) (atomic_ptr_adr)
#define birnet_atomic_compare_and_swap(PtrType, apadr, optr, nptr) 	  ((bool(*)(PtrType*, PtrType, PtrType)) (void*) g_atomic_compare_and_exchange) (apadr, optr, nptr)
#define birnet_atomic_int_set(atomic_int_ptr, value)                      ((void(*)(volatile gint*,gint)) (void*) birnet_atomic_int_set_impl) (atomic_int_ptr, value)
#define birnet_atomic_int_get(atomic_int_ptr)                             ((gint(*)(volatile gint*))      (void*) birnet_atomic_int_get_impl) (atomic_int_ptr)
#define birnet_atomic_int_add(atomic_int_ptr, signed_delta)               ((void(*)(volatile gint*,gint)) (void*) g_atomic_int_add) (atomic_int_ptr, signed_delta)
#define birnet_atomic_int_swap_and_add(atomic_int_ptr, value)             ((gint(*)(volatile gint*,gint)) (void*) g_atomic_int_exchange_and_add) (atomic_int_ptr, value)
#define birnet_atomic_int_compare_and_swap(atomic_int_ptr, oldval, nwval) ((gboolean(*)(volatile gint*,gint,gint)) (void*) g_atomic_int_compare_and_exchange) (atomic_int_ptr, oldval, nwval)
static inline gint G_GNUC_UNUSED
birnet_atomic_int_get_impl (volatile gint *atomic)
{
  return g_atomic_int_get (atomic);
}
static inline void G_GNUC_UNUSED
birnet_atomic_int_set_impl (volatile gint *atomic, gint value)
{
  while (!g_atomic_int_compare_and_exchange ((gint*) atomic, *atomic, value));
}
static inline gpointer G_GNUC_UNUSED
birnet_atomic_get_impl (volatile gpointer *atomic)
{
  return g_atomic_pointer_get (atomic);
}
static inline void G_GNUC_UNUSED
birnet_atomic_set_impl (volatile gpointer *atomic, gpointer value)
{
  while (!g_atomic_pointer_compare_and_exchange ((void**) atomic, *atomic, value));
}

/* --- implementation --- */
union _BirnetCond
{
  gpointer cond_pointer;
  guint8   cond_dummy[MAX (8, BIRNET_SIZEOF_PTH_COND_T)];
};
union _BirnetMutex
{
  gpointer mutex_pointer;
  guint8   mutex_dummy[MAX (8, BIRNET_SIZEOF_PTH_MUTEX_T)];
};
struct _BirnetRecMutex
{
  BirnetThread *owner;
  BirnetMutex   mutex;
  guint      depth;
};
struct _BirnetThreadTable
{
  void		(*thread_set_handle)	(BirnetThread	*handle);
  BirnetThread*	(*thread_get_handle)	(void);
  void		(*mutex_init)		(BirnetMutex	*mutex);
  void		(*mutex_lock)		(BirnetMutex	*mutex);
  int		(*mutex_trylock)	(BirnetMutex	*mutex); /* 0==has_lock */
  void		(*mutex_unlock)		(BirnetMutex	*mutex);
  void		(*mutex_destroy)	(BirnetMutex	*mutex);
  void		(*rec_mutex_init)	(BirnetRecMutex	*mutex);
  void		(*rec_mutex_lock)	(BirnetRecMutex	*mutex);
  int		(*rec_mutex_trylock)	(BirnetRecMutex	*mutex); /* 0==has_lock */
  void		(*rec_mutex_unlock)	(BirnetRecMutex	*mutex);
  void		(*rec_mutex_destroy)	(BirnetRecMutex	*mutex);
  void		(*cond_init)		(BirnetCond	*cond);
  void		(*cond_signal)		(BirnetCond	*cond);
  void		(*cond_broadcast)	(BirnetCond	*cond);
  void  	(*cond_wait)    	(BirnetCond	*cond,
					 BirnetMutex	*mutex);
  void  	(*cond_wait_timed)	(BirnetCond	*cond,
					 BirnetMutex	*mutex,
					 gulong		 abs_secs,
					 gulong		 abs_usecs);
  void		(*cond_destroy)		(BirnetCond	*cond);
};
extern BirnetThreadTable birnet_thread_table;
static inline void /* inlined for speed */
birnet_guard_protect (volatile BirnetGuard *guard,
		      guint                 nth_hazard,
		      gpointer              value)
{
  volatile gpointer *hparray = (volatile gpointer*) guard;
  /* simply writing the pointer value would omit memory barriers necessary on
   * some systems, so we use g_atomic_pointer_compare_and_exchange().
   */
  if (hparray[nth_hazard] != value)
    birnet_atomic_set (volatile gpointer, &hparray[nth_hazard], value);
}
void	_birnet_init_threads (void);

G_END_DECLS

#endif /* __BIRNET_THREADS_H__ */

/* vim:set ts=8 sts=2 sw=2: */
