/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002 Tim Janik and Stefan Westerfeld
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
#ifndef __SFI_THREADS_H__
#define __SFI_THREADS_H__

#include <sfi/sfitypes.h>

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _SfiThreadTable		 SfiThreadTable;
typedef struct _SfiThread		 SfiThread;
typedef union  _SfiMutex		 SfiMutex;
typedef union  _SfiCond			 SfiCond;
typedef struct _SfiRecMutex		 SfiRecMutex;

/* --- SfiThread --- */
typedef void (*SfiThreadFunc)		(gpointer	 user_data);
typedef void (*SfiThreadWakeup)		(gpointer	 wakeup_data);
SfiThread*    sfi_thread_run		(const gchar    *name,
					 SfiThreadFunc	 func,
					 gpointer	 user_data);
SfiThread*    sfi_thread_self		(void);
gint          sfi_thread_self_pid	(void);
gint	      sfi_thread_get_pid	(SfiThread      *thread);
const gchar*  sfi_thread_get_name	(SfiThread      *thread);
void	      sfi_thread_set_name	(const gchar    *name);
gboolean      sfi_thread_sleep		(glong		 max_useconds);
gboolean      sfi_thread_aborted	(void);
void	      sfi_thread_queue_abort	(SfiThread	*thread);
void	      sfi_thread_abort		(SfiThread	*thread);
void	      sfi_thread_wakeup		(SfiThread	*thread);
void	      sfi_thread_awake_after	(guint64	 stamp);
void	      sfi_thread_emit_wakeups	(guint64	 stamp);
void	      sfi_thread_set_wakeup	(SfiThreadWakeup wakeup_func,
					 gpointer	 wakeup_data,
					 GDestroyNotify	 destroy);
gpointer      sfi_thread_get_qdata	(GQuark		 quark);
void	      sfi_thread_set_qdata_full	(GQuark		 quark,
					 gpointer	 data,
					 GDestroyNotify	 destroy);
gpointer      sfi_thread_steal_qdata	(GQuark		 quark);
#define	      sfi_thread_set_qdata(      quark, data)	sfi_thread_set_qdata_full ((quark), (data), NULL)
#define	      sfi_thread_get_data(       name)		sfi_thread_get_qdata (g_quark_try_string (name))
#define	      sfi_thread_set_data(       name, data)	sfi_thread_set_qdata_full (g_quark_from_string (name), (data), NULL)
#define	      sfi_thread_set_data_full(  name, data, x)	sfi_thread_set_qdata_full (g_quark_from_string (name), (data), (x))
#define	      sfi_thread_steal_data(     name)		sfi_thread_steal_qdata (g_quark_try_string (name))

/* --- thread info --- */
typedef enum /*< skip >*/
{
  SFI_THREAD_UNKNOWN    = '?',
  SFI_THREAD_RUNNING    = 'R',
  SFI_THREAD_SLEEPING   = 'S',
  SFI_THREAD_DISKWAIT   = 'D',
  SFI_THREAD_TRACED     = 'T',
  SFI_THREAD_PAGING     = 'W',
  SFI_THREAD_ZOMBIE     = 'Z',
  SFI_THREAD_DEAD       = 'X',
} SfiThreadState;
typedef struct {
  gint           thread_id;
  gchar         *name;
  guint          aborted : 1;
  SfiThreadState state;
  gint           priority;      /* nice value */
  gint           processor;     /* running processor # */
  SfiTime        utime;         /* user time */
  SfiTime        stime;         /* system time */
  SfiTime        cutime;        /* user time of dead children */
  SfiTime        cstime;        /* system time of dead children */
} SfiThreadInfo;
SfiThreadInfo*  sfi_thread_info_collect (SfiThread      *thread);
void            sfi_thread_info_free    (SfiThreadInfo  *info);

/* --- hazard pointers / thread guards --- */
typedef struct  SfiGuard                 SfiGuard;
SfiGuard*       sfi_guard_register      (void);
void            sfi_guard_deregister    (SfiGuard       *guard);
static inline
void            sfi_guard_store         (SfiGuard       *guard,
                                         gpointer        value);
guint           sfi_guard_get_n_values  (void);
gboolean        sfi_guard_collect       (guint          *n_values,
                                         gpointer       *values);
gboolean        sfi_guard_is_protected  (gpointer        value);

/* --- SfiMutex & SfiCond --- */
#define sfi_mutex_init(mutex)		(sfi_thread_table.mutex_init (mutex))
#define SFI_SPIN_LOCK(mutex)		(sfi_thread_table.mutex_lock (mutex))
#define SFI_SPIN_UNLOCK(mutex)		(sfi_thread_table.mutex_unlock (mutex))
#define SFI_SYNC_LOCK(mutex)		(sfi_thread_table.mutex_lock (mutex))
#define SFI_SYNC_UNLOCK(mutex)		(sfi_thread_table.mutex_unlock (mutex))
#define sfi_mutex_trylock(mutex)	(sfi_thread_table.mutex_trylock (mutex) == 0) /* TRUE indicates success */
#define sfi_mutex_destroy(mutex)	(sfi_thread_table.mutex_destroy (mutex))
#define sfi_rec_mutex_init(rmutex)	(sfi_thread_table.rec_mutex_init (rmutex))
#define sfi_rec_mutex_lock(rmutex)	(sfi_thread_table.rec_mutex_lock (rmutex))
#define sfi_rec_mutex_unlock(rmutex)	(sfi_thread_table.rec_mutex_unlock (rmutex))
#define sfi_rec_mutex_trylock(rmutex)	(sfi_thread_table.rec_mutex_trylock (rmutex) == 0) /* TRUE indicates success */
#define sfi_rec_mutex_destroy(rmutex)	(sfi_thread_table.rec_mutex_destroy (rmutex))
#define sfi_cond_init(cond)		(sfi_thread_table.cond_init (cond))
#define sfi_cond_signal(cond)		(sfi_thread_table.cond_signal (cond))
#define sfi_cond_broadcast(cond)	(sfi_thread_table.cond_broadcast (cond))
#define sfi_cond_wait(cond, mutex)      (sfi_thread_table.cond_wait ((cond), (mutex)))
#define sfi_cond_destroy(cond)		(sfi_thread_table.cond_destroy (cond))
void    sfi_cond_wait_timed		(SfiCond  *cond,
					 SfiMutex *mutex,
					 glong	   max_useconds);

/* --- implementation --- */
#include <sfi/sficonfig.h>
union _SfiCond
{
  gpointer cond_pointer;
  guint8   cond_dummy[MAX (8, SFI_SIZEOF_PTH_COND_T)];
};
union _SfiMutex
{
  gpointer mutex_pointer;
  guint8   mutex_dummy[MAX (8, SFI_SIZEOF_PTH_MUTEX_T)];
};
struct _SfiRecMutex
{
  SfiThread *owner;
  SfiMutex   mutex;
  guint      depth;
};
struct _SfiThreadTable
{
  void		(*thread_set_handle)	(SfiThread	*handle);
  SfiThread*	(*thread_get_handle)	(void);
  void		(*mutex_init)		(SfiMutex	*mutex);
  void		(*mutex_lock)		(SfiMutex	*mutex);
  int		(*mutex_trylock)	(SfiMutex	*mutex); /* 0==has_lock */
  void		(*mutex_unlock)		(SfiMutex	*mutex);
  void		(*mutex_destroy)	(SfiMutex	*mutex);
  void		(*rec_mutex_init)	(SfiRecMutex	*mutex);
  void		(*rec_mutex_lock)	(SfiRecMutex	*mutex);
  int		(*rec_mutex_trylock)	(SfiRecMutex	*mutex); /* 0==has_lock */
  void		(*rec_mutex_unlock)	(SfiRecMutex	*mutex);
  void		(*rec_mutex_destroy)	(SfiRecMutex	*mutex);
  void		(*cond_init)		(SfiCond	*cond);
  void		(*cond_signal)		(SfiCond	*cond);
  void		(*cond_broadcast)	(SfiCond	*cond);
  void  	(*cond_wait)    	(SfiCond	*cond,
					 SfiMutex	*mutex);
  void  	(*cond_wait_timed)	(SfiCond	*cond,
					 SfiMutex	*mutex,
					 gulong		 abs_secs,
					 gulong		 abs_usecs);
  void		(*cond_destroy)		(SfiCond	*cond);
};
extern SfiThreadTable sfi_thread_table;
static inline void /* inlined for speed */
sfi_guard_store (SfiGuard      *guard,
                 gpointer       value)
{
  gpointer *hploc = (gpointer*) guard;
  /* simply writing the pointer value would omit memory barriers necessary on
   * some systems, so we use g_atomic_pointer_compare_and_exchange().
   */
  if (*hploc != value)
    g_atomic_pointer_compare_and_exchange (hploc, *hploc, value);
}
void	_sfi_init_threads (void);

G_END_DECLS

#endif /* __SFI_THREADS_H__ */

/* vim:set ts=8 sts=2 sw=2: */
