/* BirnetCDefs - C compatible definitions
 * Copyright (C) 2006 Tim Janik
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
#ifndef __BIRNET_CDEFS_H__
#define __BIRNET_CDEFS_H__

#include <stdbool.h>
#include <stddef.h>			/* NULL */
#include <sys/types.h>			/* uint, ssize */
#include <birnet/birnetconfig.h>

BIRNET_EXTERN_C_BEGIN();

/* --- standard macros --- */
#ifndef FALSE
#  define FALSE					false
#endif
#ifndef TRUE
#  define TRUE					true
#endif
#define BIRNET_ABS(a)                       	((a) > -(a) ? (a) : -(a))
#define BIRNET_MIN(a,b)                         ((a) <= (b) ? (a) : (b))
#define BIRNET_MAX(a,b)                         ((a) >= (b) ? (a) : (b))
#define BIRNET_CLAMP(v,mi,ma)                   ((v) < (mi) ? (mi) : ((v) > (ma) ? (ma) : (v)))
#define BIRNET_ARRAY_SIZE(array)		(sizeof (array) / sizeof ((array)[0]))
#undef ABS
#define ABS                                     BIRNET_ABS
#undef MIN
#define MIN                                     BIRNET_MIN
#undef MAX
#define MAX                                     BIRNET_MAX
#undef CLAMP
#define CLAMP                                   BIRNET_CLAMP
#undef ARRAY_SIZE
#define ARRAY_SIZE				BIRNET_ARRAY_SIZE
#undef EXTERN_C
#ifdef	__cplusplus
#define EXTERN_C                                extern "C"
#else
#define EXTERN_C                                extern
#endif
#undef STRFUNC
#if defined (__GNUC__)
#  define STRFUNC				((const char*) (__PRETTY_FUNCTION__))
#elif defined (G_HAVE_ISO_VARARGS)
#  define STRFUNC				G_STRFUNC /* GLib present */
#else
#  define STRFUNC				("<unknown>()")
#endif

/* --- likelyness hinting --- */
#define	BIRNET__BOOL(expr)		__extension__ ({ bool _birnet__bool; if (expr) _birnet__bool = 1; else _birnet__bool = 0; _birnet__bool; })
#define	BIRNET_ISLIKELY(expr)		__builtin_expect (BIRNET__BOOL (expr), 1)
#define	BIRNET_UNLIKELY(expr)		__builtin_expect (BIRNET__BOOL (expr), 0)
#define	BIRNET_LIKELY			BIRNET_ISLIKELY

/* --- assertions and runtime errors --- */
#define BIRNET_RETURN_IF_FAIL(e)	do { if (BIRNET_ISLIKELY (e)) break; BIRNET__RUNTIME_PROBLEM ('R', BIRNET_LOG_DOMAIN, __FILE__, __LINE__, BIRNET_SIMPLE_FUNCTION, "%s", #e); return; } while (0)
#define BIRNET_RETURN_VAL_IF_FAIL(e,v)	do { if (BIRNET_ISLIKELY (e)) break; BIRNET__RUNTIME_PROBLEM ('R', BIRNET_LOG_DOMAIN, __FILE__, __LINE__, BIRNET_SIMPLE_FUNCTION, "%s", #e); return v; } while (0)
#define BIRNET_ASSERT(e)		do { if (BIRNET_ISLIKELY (e)) break; BIRNET__RUNTIME_PROBLEM ('A', BIRNET_LOG_DOMAIN, __FILE__, __LINE__, BIRNET_SIMPLE_FUNCTION, "%s", #e); while (1) *(void*volatile*)0; } while (0)
#define BIRNET_ASSERT_NOT_REACHED()	do { BIRNET__RUNTIME_PROBLEM ('N', BIRNET_LOG_DOMAIN, __FILE__, __LINE__, BIRNET_SIMPLE_FUNCTION, NULL); BIRNET_ABORT_NORETURN(); } while (0)
#define BIRNET_WARNING(...)		do { BIRNET__RUNTIME_PROBLEM ('W', BIRNET_LOG_DOMAIN, __FILE__, __LINE__, BIRNET_SIMPLE_FUNCTION, __VA_ARGS__); } while (0)
#define BIRNET_ERROR(...)		do { BIRNET__RUNTIME_PROBLEM ('E', BIRNET_LOG_DOMAIN, __FILE__, __LINE__, BIRNET_SIMPLE_FUNCTION, __VA_ARGS__); BIRNET_ABORT_NORETURN(); } while (0)
#define BIRNET_ABORT_NORETURN()		birnet_abort_noreturn()

/* --- convenient aliases --- */
#ifdef  _BIRNET_SOURCE_EXTENSIONS
#define	ISLIKELY		BIRNET_ISLIKELY
#define	UNLIKELY		BIRNET_UNLIKELY
#define	LIKELY			BIRNET_LIKELY
#define	RETURN_IF_FAIL		BIRNET_RETURN_IF_FAIL
#define	RETURN_VAL_IF_FAIL	BIRNET_RETURN_VAL_IF_FAIL
#define	ASSERT_NOT_REACHED	BIRNET_ASSERT_NOT_REACHED
#define	ASSERT			BIRNET_ASSERT
#define	WARNING			BIRNET_WARNING
#define	ERROR			BIRNET_ERROR
#endif /* _BIRNET_SOURCE_EXTENSIONS */

/* --- preprocessor pasting --- */
#define BIRNET_CPP_PASTE4i(a,b,c,d)             a ## b ## c ## d  /* twofold indirection is required to expand macros like __LINE__ */
#define BIRNET_CPP_PASTE4(a,b,c,d)              BIRNET_CPP_PASTE4i (a,b,c,d)
#define BIRNET_CPP_PASTE3i(a,b,c)               a ## b ## c	  /* twofold indirection is required to expand macros like __LINE__ */
#define BIRNET_CPP_PASTE3(a,b,c)                BIRNET_CPP_PASTE3i (a,b,c)
#define BIRNET_CPP_PASTE2i(a,b)                 a ## b      	  /* twofold indirection is required to expand macros like __LINE__ */
#define BIRNET_CPP_PASTE2(a,b)                  BIRNET_CPP_PASTE2i (a,b)
#define BIRNET_STATIC_ASSERT_NAMED(expr,asname) typedef struct { char asname[(expr) ? 1 : -1]; } BIRNET_CPP_PASTE2 (Birnet_StaticAssertion_LINE, __LINE__)
#define BIRNET_STATIC_ASSERT(expr)              BIRNET_STATIC_ASSERT_NAMED (expr, compile_time_assertion_failed)

/* --- attributes --- */
#if     __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)
#define BIRNET_PRETTY_FUNCTION                  (__PRETTY_FUNCTION__)
#define BIRNET_PURE                             __attribute__ ((__pure__))
#define BIRNET_MALLOC                           __attribute__ ((__malloc__))
#define BIRNET_PRINTF(format_idx, arg_idx)      __attribute__ ((__format__ (__printf__, format_idx, arg_idx)))
#define BIRNET_SCANF(format_idx, arg_idx)       __attribute__ ((__format__ (__scanf__, format_idx, arg_idx)))
#define BIRNET_FORMAT(arg_idx)                  __attribute__ ((__format_arg__ (arg_idx)))
#define BIRNET_NORETURN                         __attribute__ ((__noreturn__))
#define BIRNET_CONST                            __attribute__ ((__const__))
#define BIRNET_UNUSED                           __attribute__ ((__unused__))
#define BIRNET_NO_INSTRUMENT                    __attribute__ ((__no_instrument_function__))
#define BIRNET_DEPRECATED                       __attribute__ ((__deprecated__))
#define BIRNET_ALWAYS_INLINE			__attribute__ ((always_inline))
#define BIRNET_NEVER_INLINE			__attribute__ ((noinline))
#define BIRNET_CONSTRUCTOR			__attribute__ ((constructor,used)) /* gcc-3.3 also needs "used" to emit code */
#define BIRNET_MAY_ALIAS                        __attribute__ ((may_alias))
#else   /* !__GNUC__ */
#define BIRNET_PRETTY_FUNCTION                  ("<unknown>")
#define BIRNET_PURE
#define BIRNET_MALLOC
#define BIRNET_PRINTF(format_idx, arg_idx)
#define BIRNET_SCANF(format_idx, arg_idx)
#define BIRNET_FORMAT(arg_idx)
#define BIRNET_NORETURN
#define BIRNET_CONST
#define BIRNET_UNUSED
#define BIRNET_NO_INSTRUMENT
#define BIRNET_DEPRECATED
#define BIRNET_ALWAYS_INLINE
#define BIRNET_NEVER_INLINE
#define BIRNET_CONSTRUCTOR
#define BIRNET_MAY_ALIAS
#error  Failed to detect a recent GCC version (>= 3.3)
#endif  /* !__GNUC__ */
#ifdef	__cplusplus
#define	BIRNET_SIMPLE_FUNCTION			(__func__)
#else
#define	BIRNET_SIMPLE_FUNCTION			BIRNET_PRETTY_FUNCTION
#endif

/* --- provide canonical integer types --- */
#if 	BIRNET_SIZEOF_SYS_TYPESH_UINT == 0
typedef unsigned int		uint;	/* for systems that don't define uint in types.h */
#else
BIRNET_STATIC_ASSERT (BIRNET_SIZEOF_SYS_TYPESH_UINT == 4);
#endif
BIRNET_STATIC_ASSERT (sizeof (uint) == 4);
typedef unsigned int		BirnetUInt8  __attribute__ ((__mode__ (__QI__)));
typedef unsigned int		BirnetUInt16 __attribute__ ((__mode__ (__HI__)));
typedef unsigned int		BirnetUInt32 __attribute__ ((__mode__ (__SI__)));
// typedef unsigned int         BirnetUInt64 __attribute__ ((__mode__ (__DI__)));
typedef unsigned long long int  BirnetUInt64; // AMD64 needs this for %llu printf format strings
// provided by birnetcdefs.h: uint;
BIRNET_STATIC_ASSERT (sizeof (BirnetUInt8)  == 1);
BIRNET_STATIC_ASSERT (sizeof (BirnetUInt16) == 2);
BIRNET_STATIC_ASSERT (sizeof (BirnetUInt32) == 4);
BIRNET_STATIC_ASSERT (sizeof (BirnetUInt64) == 8);
typedef signed int		BirnetInt8  __attribute__ ((__mode__ (__QI__)));
typedef signed int		BirnetInt16 __attribute__ ((__mode__ (__HI__)));
typedef signed int		BirnetInt32 __attribute__ ((__mode__ (__SI__)));
// typedef signed long long int BirnetInt64 __attribute__ ((__mode__ (__DI__)));
typedef signed long long int	BirnetInt64;  // AMD64 needs this for %lld printf format strings
// provided by compiler       int;
BIRNET_STATIC_ASSERT (sizeof (BirnetInt8)  == 1);
BIRNET_STATIC_ASSERT (sizeof (BirnetInt16) == 2);
BIRNET_STATIC_ASSERT (sizeof (BirnetInt32) == 4);
BIRNET_STATIC_ASSERT (sizeof (BirnetInt64) == 8);
typedef BirnetUInt32		BirnetUnichar;
BIRNET_STATIC_ASSERT (sizeof (BirnetUnichar) == 4);


/* --- path handling --- */
#ifdef	BIRNET_OS_WIN32
#define BIRNET_DIR_SEPARATOR		  '\\'
#define BIRNET_DIR_SEPARATOR_S		  "\\"
#define BIRNET_SEARCHPATH_SEPARATOR	  ';'
#define BIRNET_SEARCHPATH_SEPARATOR_S	  ";"
#else	/* !BIRNET_OS_WIN32 */
#define BIRNET_DIR_SEPARATOR		  '/'
#define BIRNET_DIR_SEPARATOR_S		  "/"
#define BIRNET_SEARCHPATH_SEPARATOR	  ':'
#define BIRNET_SEARCHPATH_SEPARATOR_S	  ":"
#endif	/* !BIRNET_OS_WIN32 */
#define	BIRNET_IS_DIR_SEPARATOR(c)    	  ((c) == BIRNET_DIR_SEPARATOR)
#define BIRNET_IS_SEARCHPATH_SEPARATOR(c) ((c) == BIRNET_SEARCHPATH_SEPARATOR)

/* --- initialization --- */
typedef struct {
  bool stand_alone;		/* "stand-alone": no rcfiles, boot scripts, etc. */
  bool test_quick;		/* run quick tests */
  bool test_slow;		/* run slow tests */
  bool test_perf;		/* run benchmarks, test performance */
} BirnetInitSettings;

typedef struct {
  const char *value_name;     	/* value list ends with value_name == NULL */
  const char *value_string;
  long double value_num;     	/* valid if value_string == NULL */
} BirnetInitValue;

/* --- CPU info --- */
typedef struct {
  /* architecture name */
  const char *machine;
  /* CPU Vendor ID */
  const char *cpu_vendor;
  /* CPU features on X86 */
  uint x86_fpu : 1, x86_tsc    : 1, x86_htt   : 1;
  uint x86_mmx : 1, x86_mmxext : 1, x86_3dnow : 1, x86_3dnowext : 1;
  uint x86_sse : 1, x86_sse2   : 1, x86_sse3  : 1, x86_ssesys   : 1;
} BirnetCPUInfo;

/* --- Thread info --- */
typedef enum {
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
  int                	thread_id;
  char                 *name;
  uint                 	aborted : 1;
  BirnetThreadState     state;
  int                  	priority;      	/* nice value */
  int                  	processor;     	/* running processor # */
  BirnetUInt64         	utime;		/* user time */
  BirnetUInt64         	stime;         	/* system time */
  BirnetUInt64		cutime;        	/* user time of dead children */
  BirnetUInt64		cstime;		/* system time of dead children */
} BirnetThreadInfo;

/* --- threading ABI --- */
typedef struct _BirnetThread BirnetThread;
typedef void (*BirnetThreadFunc)   (void *user_data);
typedef void (*BirnetThreadWakeup) (void *wakeup_data);
typedef union {
  void	     *cond_pointer;
  BirnetUInt8 cond_dummy[MAX (8, BIRNET_SIZEOF_PTH_COND_T)];
} BirnetCond;
typedef union {
  void	     *mutex_pointer;
  BirnetUInt8 mutex_dummy[MAX (8, BIRNET_SIZEOF_PTH_MUTEX_T)];
} BirnetMutex;
typedef struct {
  BirnetMutex   mutex;
  BirnetThread *owner;
  uint 		depth;
} BirnetRecMutex;
typedef struct {
  void              (*mutex_chain4init)     (BirnetMutex       *mutex);
  void              (*mutex_unchain)        (BirnetMutex       *mutex);
  void              (*rec_mutex_chain4init) (BirnetRecMutex    *mutex);
  void              (*rec_mutex_unchain)    (BirnetRecMutex    *mutex);
  void              (*cond_chain4init)      (BirnetCond        *cond);
  void              (*cond_unchain)         (BirnetCond        *cond);
  void		    (*atomic_pointer_set)   (volatile void     *atomic,
					     volatile void     *value);
  void*		    (*atomic_pointer_get)   (volatile void     *atomic);
  int/*bool*/	    (*atomic_pointer_cas)   (volatile void     *atomic,
					     volatile void     *oldval,
					     volatile void     *newval);
  void		    (*atomic_int_set)	    (volatile int      *atomic,
					     int                newval);
  int		    (*atomic_int_get)	    (volatile int      *atomic);
  int/*bool*/	    (*atomic_int_cas)	    (volatile int      *atomic,
					     int           	oldval,
					     int           	newval);
  void		    (*atomic_int_add)	    (volatile int      *atomic,
					     int           	diff);
  int		    (*atomic_int_swap_add)  (volatile int      *atomic,
					     int           	diff);
  void		    (*atomic_uint_set)	    (volatile uint     *atomic,
					     uint               newval);
  uint		    (*atomic_uint_get)	    (volatile uint     *atomic);
  int/*bool*/	    (*atomic_uint_cas)	    (volatile uint     *atomic,
					     uint           	oldval,
					     uint           	newval);
  void		    (*atomic_uint_add)	    (volatile uint     *atomic,
					     uint           	diff);
  uint		    (*atomic_uint_swap_add) (volatile uint     *atomic,
					     uint           	diff);
  BirnetThread*     (*thread_new)           (const char        *name);
  BirnetThread*     (*thread_ref)           (BirnetThread      *thread);
  BirnetThread*     (*thread_ref_sink)      (BirnetThread      *thread);
  void              (*thread_unref)         (BirnetThread      *thread);
  bool              (*thread_start)         (BirnetThread      *thread,
					     BirnetThreadFunc 	func,
					     void              *user_data);
  BirnetThread*     (*thread_self)          (void);
  void*             (*thread_selfxx)        (void);
  void*             (*thread_getxx)         (BirnetThread      *thread);
  bool              (*thread_setxx)         (BirnetThread      *thread,
					     void              *xxdata);
  int               (*thread_pid)           (BirnetThread      *thread);
  const char*       (*thread_name)          (BirnetThread      *thread);
  void              (*thread_set_name)      (const char        *newname);
  bool		    (*thread_sleep)	    (BirnetInt64        max_useconds);
  void		    (*thread_wakeup)	    (BirnetThread      *thread);
  void		    (*thread_awake_after)   (BirnetUInt64       stamp);
  void		    (*thread_emit_wakeups)  (BirnetUInt64       wakeup_stamp);
  void		    (*thread_set_wakeup)    (BirnetThreadWakeup wakeup_func,
					     void              *wakeup_data,
					     void             (*destroy_data) (void*));
  void              (*thread_abort) 	    (BirnetThread      *thread);
  void              (*thread_queue_abort)   (BirnetThread      *thread);
  bool              (*thread_aborted)	    (void);
  bool		    (*thread_get_aborted)   (BirnetThread      *thread);
  bool	            (*thread_get_running)   (BirnetThread      *thread);
  void		    (*thread_wait_for_exit) (BirnetThread      *thread);
  void              (*thread_yield)         (void);
  void              (*thread_exit)          (void              *retval) BIRNET_NORETURN;
  void              (*thread_set_handle)    (BirnetThread      *handle);
  BirnetThread*     (*thread_get_handle)    (void);
  BirnetThreadInfo* (*info_collect)         (BirnetThread      *thread);
  void              (*info_free)            (BirnetThreadInfo  *info);
  void*		    (*qdata_get)	    (uint               glib_quark);
  void		    (*qdata_set)	    (uint               glib_quark,
					     void              *data,
                                             void             (*destroy_data) (void*));
  void*		    (*qdata_steal)	    (uint		glib_quark);
  void              (*mutex_init)           (BirnetMutex       *mutex);
  void              (*mutex_lock)           (BirnetMutex       *mutex);
  int               (*mutex_trylock)        (BirnetMutex       *mutex); /* 0==has_lock */
  void              (*mutex_unlock)         (BirnetMutex       *mutex);
  void              (*mutex_destroy)        (BirnetMutex       *mutex);
  void              (*rec_mutex_init)       (BirnetRecMutex    *mutex);
  void              (*rec_mutex_lock)       (BirnetRecMutex    *mutex);
  int               (*rec_mutex_trylock)    (BirnetRecMutex    *mutex); /* 0==has_lock */
  void              (*rec_mutex_unlock)     (BirnetRecMutex    *mutex);
  void              (*rec_mutex_destroy)    (BirnetRecMutex    *mutex);
  void              (*cond_init)            (BirnetCond        *cond);
  void              (*cond_signal)          (BirnetCond        *cond);
  void              (*cond_broadcast)       (BirnetCond        *cond);
  void              (*cond_wait)            (BirnetCond        *cond,
					     BirnetMutex       *mutex);
  void		    (*cond_wait_timed)      (BirnetCond        *cond,
					     BirnetMutex       *mutex,
					     BirnetInt64 	max_useconds);
  void              (*cond_destroy)         (BirnetCond        *cond);
} BirnetThreadTable;

/* --- implementation bits --- */
/* the above macros rely on a problem handler macro: */
// BIRNET__RUNTIME_PROBLEM(ErrorWarningReturnAssertNotreach,domain,file,line,funcname,exprformat,...); // noreturn cases: 'E', 'A', 'N'
extern inline void birnet_abort_noreturn (void) BIRNET_NORETURN;
extern inline void birnet_abort_noreturn (void) { while (1) *(void*volatile*)0; }

BIRNET_EXTERN_C_END();

#endif /* __BIRNET_CDEFS_H__ */

/* vim:set ts=8 sts=2 sw=2: */
