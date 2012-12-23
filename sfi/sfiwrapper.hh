/* SfiWrapper - Birnet C wrapper
 * Copyright (C) 2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#ifndef __SFI_WRAPPER_H__
#define __SFI_WRAPPER_H__

#include <stdbool.h>
#include <sfi/glib-extra.hh>
#include <birnet/birnetcdefs.h> /* include glib before birnet for G_LOG_DOMAIN */

/* sfiwrapper.h is a thin C language wrapper around C++ features
 * provided by libbirnet.
 */

/* --- short integer types --- */
#ifdef __cplusplus
#include <birnet/birnetutils.hh>
using Birnet::uint8;
using Birnet::uint16;
using Birnet::uint32;
using Birnet::uint64;
using Birnet::int8;
using Birnet::int16;
using Birnet::int32;
using Birnet::int64;
using Birnet::unichar;
#else
typedef BirnetUInt8   uint8;
typedef BirnetUInt16  uint16;
typedef BirnetUInt32  uint32;
typedef BirnetUInt64  uint64;
typedef BirnetInt8    int8;
typedef BirnetInt16   int16;
typedef BirnetInt32   int32;
typedef BirnetInt64   int64;
typedef BirnetUnichar unichar;
#endif

BIRNET_EXTERN_C_BEGIN();

/* --- initialization --- */
typedef struct
{
  const char *value_name;       /* value list ends with value_name == NULL */
  const char *value_string;
  long double value_num;        /* valid if value_string == NULL */
} SfiInitValue;
void    sfi_init        (int            *argcp,
			 char         ***argvp,
			 const char     *app_name,
			 SfiInitValue    sivalues[]);
bool    sfi_init_value_bool   (SfiInitValue *value);
double  sfi_init_value_double (SfiInitValue *value);
gint64  sfi_init_value_int    (SfiInitValue *value);

typedef BirnetInitSettings SfiInitSettings;
SfiInitSettings sfi_init_settings (void);

/* --- CPU Info --- */
typedef BirnetCPUInfo SfiCPUInfo;

SfiCPUInfo sfi_cpu_info	   (void);
gchar*     sfi_cpu_info_string (const SfiCPUInfo *cpu_info);

/* --- file tests --- */
bool	birnet_file_check (const char *file,
			   const char *mode);
bool	birnet_file_equals (const char *file1,
			    const char *file2);

/* --- messaging --- */
typedef enum {
  SFI_MSG_NONE   = 0,   /* always off */
  SFI_MSG_ALWAYS = 1,   /* always on */
  SFI_MSG_ERROR,
  SFI_MSG_WARNING,
  SFI_MSG_SCRIPT,
  SFI_MSG_INFO,
  SFI_MSG_DIAG,
  SFI_MSG_DEBUG
} SfiMsgType;
typedef enum {
  SFI_MSG_TO_STDERR     = 1,
  SFI_MSG_TO_STDLOG     = 2,
  SFI_MSG_TO_HANDLER    = 4,
} SfiMsgLogFlags;
#define         sfi_error(...)                   sfi_msg_printf (SFI_MSG_ERROR, __VA_ARGS__)
#define         sfi_warning(...)                 sfi_msg_printf (SFI_MSG_WARNING, __VA_ARGS__)
#define         sfi_info(...)                    sfi_msg_printf (SFI_MSG_INFO, __VA_ARGS__)
#define         sfi_diag(...)                    sfi_msg_printf (SFI_MSG_DIAG, __VA_ARGS__)
#define         sfi_debug(lvl, ...)              sfi_msg_printf (lvl, __VA_ARGS__)
#define         sfi_nodebug(lvl, ...)            do { /* nothing */ } while (0)
bool            sfi_msg_check                   (SfiMsgType      mtype);
void            sfi_msg_enable                  (SfiMsgType      mtype);
void            sfi_msg_disable                 (SfiMsgType      mtype);
void            sfi_msg_allow                   (const char     *ident_list);
void            sfi_msg_deny                    (const char     *ident_list);
const char*     sfi_msg_type_ident              (SfiMsgType      mtype);
const char*     sfi_msg_type_label              (SfiMsgType      mtype);
SfiMsgType      sfi_msg_lookup_type             (const char     *ident);
SfiMsgType      sfi_msg_type_register           (const char     *ident,
                                                 SfiMsgType      default_ouput,
                                                 const char     *label);
#define         sfi_msg_printf(level, ...)       SFI_MSG_PRINTF (level, __VA_ARGS__)    /* level, printf_format, ... */
#define         sfi_msg_display(level, ...)      SFI_MSG_DISPLAY (level, __VA_ARGS__)   /* level, part, part, ... */
#define         SFI_MSG_TEXT0(...)               sfi_msg_part_printf ('0', __VA_ARGS__) /* message title */
#define         SFI_MSG_TEXT1(...)               sfi_msg_part_printf ('1', __VA_ARGS__) /* primary message */
#define         SFI_MSG_TEXT2(...)               sfi_msg_part_printf ('2', __VA_ARGS__) /* secondary message */
#define         SFI_MSG_TEXT3(...)               sfi_msg_part_printf ('3', __VA_ARGS__) /* message details */
#define         SFI_MSG_CHECK(...)               sfi_msg_part_printf ('c', __VA_ARGS__) /* user switch */
#define         SFI_MSG_TITLE                    SFI_MSG_TEXT0 /* alias */
#define         SFI_MSG_PRIMARY                  SFI_MSG_TEXT1 /* alias */
#define         SFI_MSG_SECONDARY                SFI_MSG_TEXT2 /* alias */
#define         SFI_MSG_DETAIL                   SFI_MSG_TEXT3 /* alias */
#define         SFI_MSG_TYPE_DEFINE(variable, ident, default_ouput, label) SFI_MSG__TYPEDEF (variable, ident, default_ouput, label)

/* --- messaging implementation --- */
typedef struct SfiMsgPart SfiMsgPart;
SfiMsgPart*     sfi_msg_part_printf     (uint8          msg_part_id,
                                         const char    *format,
                                         ...) G_GNUC_PRINTF (2, 3);
void            sfi_msg_display_parts   (const char     *log_domain,
                                         SfiMsgType      mtype,
                                         guint           n_mparts,
                                         SfiMsgPart    **mparts);
void            sfi_msg_display_printf  (const char    *log_domain,
                                         SfiMsgType     mtype,
                                         const char    *format,
                                         ...) G_GNUC_PRINTF (3, 4);
#define SFI_MSG_PRINTF(lvl, ...)        do { SfiMsgType __mt = lvl; if (sfi_msg_check (__mt)) \
                                             sfi_msg_display_printf (BIRNET_LOG_DOMAIN, __mt, __VA_ARGS__); } while (0)
#define SFI_MSG_DISPLAY(lvl, ...)       do { SfiMsgType __mt = lvl; if (sfi_msg_check (__mt)) { \
                                               SfiMsgPart *__pa[] = { __VA_ARGS__ };            \
                                               sfi_msg_display_parts (BIRNET_LOG_DOMAIN, __mt,  \
                                                 BIRNET_ARRAY_SIZE (__pa), __pa); } } while (0)
#define SFI_MSG__TYPEDEF(variable, identifier, default_ouput, label) \
  SfiMsgType variable = (SfiMsgType) 0; \
  static void BIRNET_CONSTRUCTOR \
  BIRNET_CPP_PASTE4 (__sfi_msg_type__init, __LINE__, __, variable) (void) \
  { variable = sfi_msg_type_register (identifier, default_ouput, label); }

/* --- debug channels --- */
typedef struct SfiDebugChannel SfiDebugChannel;
SfiDebugChannel* sfi_debug_channel_from_file_async (const char      *file_name);
void             sfi_debug_channel_printf          (SfiDebugChannel *debug_channel,
                                                    const char      *dummy,
                                                    const char      *format,
                                                    ...) G_GNUC_PRINTF (3, 4);
void             sfi_debug_channel_destroy         (SfiDebugChannel *debug_channel);

/* --- url handling --- */
void sfi_url_show                   	(const char           *url);
void sfi_url_show_with_cookie       	(const char           *url,
					 const char           *url_title,
					 const char           *cookie);
bool sfi_url_test_show              	(const char           *url);
bool sfi_url_test_show_with_cookie	(const char           *url,
					 const char           *url_title,
					 const char           *cookie);

/* --- cleanup handlers --- */
void birnet_cleanup_force_handlers     (void); // FIXME: remove

/* --- threading API --- */
typedef BirnetThread   		SfiThread;
typedef void         	      (*SfiThreadFunc)   (void *user_data);
typedef void         	      (*SfiThreadWakeup) (void *wakeup_data);
typedef BirnetCond     		SfiCond;
typedef BirnetMutex    		SfiMutex;
typedef BirnetRecMutex 		SfiRecMutex;
typedef BirnetThreadState       SfiThreadState;
typedef BirnetThreadInfo        SfiThreadInfo;
#define SFI_THREAD_UNKNOWN      BIRNET_THREAD_UNKNOWN
#define SFI_THREAD_RUNNING      BIRNET_THREAD_RUNNING
#define SFI_THREAD_SLEEPING     BIRNET_THREAD_SLEEPING
#define SFI_THREAD_DISKWAIT     BIRNET_THREAD_DISKWAIT
#define SFI_THREAD_TRACED       BIRNET_THREAD_TRACED
#define SFI_THREAD_PAGING       BIRNET_THREAD_PAGING
#define SFI_THREAD_ZOMBIE       BIRNET_THREAD_ZOMBIE
#define SFI_THREAD_DEAD         BIRNET_THREAD_DEAD
SfiThread* sfi_thread_run			(const char   *name, /* new + start */
						 SfiThreadFunc func,
						 gpointer      user_data);
#define	SFI_MUTEX_DECLARE_INITIALIZED(name)	SFI_MUTEX__DECLARE_INITIALIZED (name)
#define sfi_mutex_init(mtx)        		(sfi_thread_table->mutex_init (mtx))
#define sfi_mutex_lock(mtx)        		(sfi_thread_table->mutex_lock (mtx))
#define sfi_mutex_trylock(mtx)     		(0 == sfi_thread_table->mutex_trylock (mtx))
#define sfi_mutex_unlock(mtx)      		(sfi_thread_table->mutex_unlock (mtx))
#define sfi_mutex_destroy(mtx)     		(sfi_thread_table->mutex_destroy (mtx))
#define	SFI_REC_MUTEX_DECLARE_INITIALIZED(name)	SFI_REC_MUTEX__DECLARE_INITIALIZED (name)
#define sfi_rec_mutex_init(mtx)    		(sfi_thread_table->rec_mutex_init (mtx))
#define sfi_rec_mutex_lock(mtx)    		(sfi_thread_table->rec_mutex_lock (mtx))
#define sfi_rec_mutex_trylock(mtx) 		(0 == sfi_thread_table->rec_mutex_trylock (mtx))
#define sfi_rec_mutex_unlock(mtx)  		(sfi_thread_table->rec_mutex_unlock (mtx))
#define sfi_rec_mutex_destroy(mtx) 		(sfi_thread_table->rec_mutex_destroy (mtx))
#define	SFI_COND_DECLARE_INITIALIZED(name)	SFI_COND__DECLARE_INITIALIZED (name)
#define sfi_cond_init(cond)  			(sfi_thread_table->cond_init (cond))
#define sfi_cond_signal(cond)			(sfi_thread_table->cond_signal (cond))
#define sfi_cond_broadcast(cond)		(sfi_thread_table->cond_broadcast (cond))
#define sfi_cond_wait(cond,mtx)			(sfi_thread_table->cond_wait (cond, mtx))
#define sfi_cond_wait_timed(cond,mtx,usecs)	(sfi_thread_table->cond_wait_timed (cond, mtx, usecs))
#define sfi_cond_destroy(cond)			(sfi_thread_table->cond_destroy (cond))
#define sfi_thread_new(name)            	(sfi_thread_table->thread_new (name))
#define sfi_thread_ref(thrd)			(sfi_thread_table->thread_ref (thrd))
#define sfi_thread_ref_sink(thrd)		(sfi_thread_table->thread_ref_sink (thrd))
#define sfi_thread_unref(thrd)			(sfi_thread_table->thread_unref (thrd))
#define sfi_thread_start(thrd,func,udata)	(sfi_thread_table->thread_start (thrd, func, udata))
#define sfi_thread_self()			(sfi_thread_table->thread_self ())
#define sfi_thread_self_pid()			(sfi_thread_table->thread_pid (sfi_thread_self()))
#define sfi_thread_get_pid(thrd)		(sfi_thread_table->thread_pid (thrd))
#define sfi_thread_get_name(thrd)		(sfi_thread_table->thread_name (thrd))
#define sfi_thread_set_name(name)		(sfi_thread_table->thread_set_name (name))
#define sfi_thread_sleep(usecs)			(sfi_thread_table->thread_sleep (usecs))
#define sfi_thread_wakeup(thrd)			(sfi_thread_table->thread_wakeup (thrd))
#define sfi_thread_awake_after(stamp)		(sfi_thread_table->thread_awake_after (stamp))
#define sfi_thread_emit_wakeups(stamp)		(sfi_thread_table->thread_emit_wakeups (stamp))
#define sfi_thread_set_wakeup(func,udata,dstry)	(sfi_thread_table->thread_set_wakeup (func, udata, dstry))
#define sfi_thread_abort(thrd)			(sfi_thread_table->thread_abort (thrd))
#define sfi_thread_queue_abort(thrd)		(sfi_thread_table->thread_queue_abort (thrd))
#define sfi_thread_aborted()			(sfi_thread_table->thread_aborted ())
#define sfi_thread_get_aborted(thrd)		(sfi_thread_table->thread_get_aborted (thrd))
#define sfi_thread_get_running(thrd)		(sfi_thread_table->thread_get_running (thrd))
#define sfi_thread_wait_for_exit(thrd)		(sfi_thread_table->thread_wait_for_exit (thrd))
#define sfi_thread_yield()			(sfi_thread_table->thread_yield ())
#define sfi_thread_info_collect(thrd)		(sfi_thread_table->info_collect (thrd))
#define sfi_thread_info_free(tinfo)		(sfi_thread_table->info_free (tinfo))
#define	sfi_thread_get_qdata(quark)		(sfi_thread_table->qdata_get (quark))
#define	sfi_thread_set_qdata_full(q,udat,dstry)	(sfi_thread_table->qdata_set (q, udat, dstry))
#define	sfi_thread_set_qdata(quark,udata)	(sfi_thread_table->qdata_set (quark, udata, NULL))
#define	sfi_thread_steal_qdata(quark)		(sfi_thread_table->qdata_steal (quark))
#define sfi_atomic_pointer_set(atmc,val)	(sfi_thread_table->atomic_pointer_set (atmc, val))
#define sfi_atomic_pointer_get(atmc)		(sfi_thread_table->atomic_pointer_get (atmc))
#define sfi_atomic_pointer_cas(atmc,oval,nval)	(sfi_thread_table->atomic_pointer_cas (atmc, oval, nval))
#define sfi_atomic_int_set(atmc,val)		(sfi_thread_table->atomic_int_set (atmc, val))
#define sfi_atomic_int_get(atmc)		(sfi_thread_table->atomic_int_get (atmc))
#define sfi_atomic_int_cas(atmc,oval,nval)	(sfi_thread_table->atomic_int_cas (atmc, oval, nval))
#define sfi_atomic_int_add(atmc,diff)		(sfi_thread_table->atomic_int_add (atmc, diff))
#define sfi_atomic_int_swap_add(atmc,diff)	(sfi_thread_table->atomic_int_swap_add (atmc, diff))
#define sfi_atomic_uint_set(atmc,val)		(sfi_thread_table->atomic_uint_set (atmc, val))
#define sfi_atomic_uint_get(atmc)		(sfi_thread_table->atomic_uint_get (atmc))
#define sfi_atomic_uint_cas(atmc,oval,nval)	(sfi_thread_table->atomic_uint_cas (atmc, oval, nval))
#define sfi_atomic_uint_add(atmc,diff)		(sfi_thread_table->atomic_uint_add (atmc, diff))
#define sfi_atomic_uint_swap_add(atmc,diff)	(sfi_thread_table->atomic_uint_swap_add (atmc, diff))

/* --- implementation bits --- */
extern const BirnetThreadTable *sfi_thread_table;
#define SFI_MUTEX__DECLARE_INITIALIZED(mutexname)                        	\
  SfiMutex mutexname = { 0 };                                                	\
  static void BIRNET_CONSTRUCTOR                                                \
  BIRNET_CPP_PASTE4 (__sfi_mutex__autoinit, __LINE__, __, mutexname) (void)  	\
  { sfi_thread_table->mutex_chain4init (&mutexname); }
#define SFI_REC_MUTEX__DECLARE_INITIALIZED(recmtx)                            	\
  SfiRecMutex recmtx = { { 0 } };                                            	\
  static void BIRNET_CONSTRUCTOR                                                \
  BIRNET_CPP_PASTE4 (__sfi_rec_mutex__autoinit, __LINE__, __, recmtx) (void) 	\
  { sfi_thread_table->rec_mutex_chain4init (&recmtx); }
#define SFI_COND__DECLARE_INITIALIZED(condname)                               	\
  SfiCond condname = { 0 };                                                  	\
  static void BIRNET_CONSTRUCTOR                                                \
  BIRNET_CPP_PASTE4 (__sfi_cond__autoinit, __LINE__, __, condname) (void)    	\
  { sfi_thread_table->cond_chain4init (&condname); }
#ifndef BIRNET__RUNTIME_PROBLEM
#define BIRNET__RUNTIME_PROBLEM(ErrorWarningReturnAssertNotreach,domain,file,line,funcname,...) \
        sfi_runtime_problem (ErrorWarningReturnAssertNotreach, domain, file, line, funcname, __VA_ARGS__)
#endif
void sfi_runtime_problem (char        ewran_tag,
			  const char *domain,
			  const char *file,
			  int         line,
			  const char *funcname,
			  const char *msgformat,
			  ...) BIRNET_PRINTF (6, 7);

BIRNET_EXTERN_C_END();

#endif /* __SFI_WRAPPER_H__ */

/* vim:set ts=8 sts=2 sw=2: */
