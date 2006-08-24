/* Birnet
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
#ifndef __BIRNET_CORE_H__
#define __BIRNET_CORE_H__

#include <stdbool.h>
#include <stddef.h>	/* NULL */
#include <sys/types.h>	/* uint, ssize */
#include <glib.h>
#include <birnet/birnetconfig.h>

BIRNET_EXTERN_C_BEGIN();

/* --- standard macros --- */
#define BIRNET_ABS(a)                       	((a) > -(a) ? (a) : -(a))
#define BIRNET_MIN(a,b)                         ((a) <= (b) ? (a) : (b))
#define BIRNET_MAX(a,b)                         ((a) >= (b) ? (a) : (b))
#define BIRNET_CLAMP(v,mi,ma)                   ((v) < (mi) ? (mi) : ((v) > (ma) ? (ma) : (v)))
#ifdef  _BIRNET_SOURCE_EXTENSIONS
#undef ABS
#define ABS                                     BIRNET_ABS
#undef MIN
#define MIN                                     BIRNET_MIN
#undef MAX
#define MAX                                     BIRNET_MAX
#undef CLAMP
#define CLAMP                                   BIRNET_CLAMP
#undef STRFUNC
#define	STRFUNC					G_STRFUNC
#endif  /* _BIRNET_SOURCE_EXTENSIONS */

/* --- reliable assert --- */
#define	BIRNET_LIKELY			G_LIKELY
#define	BIRNET_ISLIKELY			G_LIKELY
#define	BIRNET_UNLIKELY			G_UNLIKELY
#define BIRNET_RETURN_IF_FAIL(e)	do { if G_LIKELY (e) {} else { g_return_if_fail_warning (G_LOG_DOMAIN, __PRETTY_FUNCTION__, #e); return; } } while (0)
#define BIRNET_RETURN_VAL_IF_FAIL(e,v)	do { if G_LIKELY (e) {} else { g_return_if_fail_warning (G_LOG_DOMAIN, __PRETTY_FUNCTION__, #e); return v; } } while (0)
#define BIRNET_ASSERT_NOT_REACHED()	do { g_assert_warning (G_LOG_DOMAIN, __FILE__, __LINE__, __PRETTY_FUNCTION__, NULL); } while (0)
#define BIRNET_ASSERT(expr)   do { /* never disabled */ \
  if G_LIKELY (expr) {} else                            \
    g_assert_warning (G_LOG_DOMAIN, __FILE__, __LINE__, \
                      __PRETTY_FUNCTION__, #expr);      \
} while (0)
/* convenient aliases */
#ifdef  _BIRNET_SOURCE_EXTENSIONS
#define	RETURN_IF_FAIL		BIRNET_RETURN_IF_FAIL
#define	RETURN_VAL_IF_FAIL	BIRNET_RETURN_VAL_IF_FAIL
#define	ASSERT_NOT_REACHED	BIRNET_ASSERT_NOT_REACHED
#define	ASSERT			BIRNET_ASSERT
#define	LIKELY			BIRNET_LIKELY
#define	ISLIKELY		BIRNET_ISLIKELY
#define	UNLIKELY		BIRNET_UNLIKELY
#endif /* _BIRNET_SOURCE_EXTENSIONS */

/* --- compile time assertions --- */
#define BIRNET_CPP_PASTE4i(a,b,c,d)             a ## b ## c ## d  /* twofold indirection is required to expand macros like __LINE__ */
#define BIRNET_CPP_PASTE4(a,b,c,d)              BIRNET_CPP_PASTE4i (a,b,c,d)
#define BIRNET_CPP_PASTE3i(a,b,c)               a ## b ## c	  /* twofold indirection is required to expand macros like __LINE__ */
#define BIRNET_CPP_PASTE3(a,b,c)                BIRNET_CPP_PASTE3i (a,b,c)
#define BIRNET_CPP_PASTE2i(a,b)                 a ## b      	  /* twofold indirection is required to expand macros like __LINE__ */
#define BIRNET_CPP_PASTE2(a,b)                  BIRNET_CPP_PASTE2i (a,b)
#define BIRNET_STATIC_ASSERT_NAMED(expr,asname) typedef struct { char asname[(expr) ? 1 : -1]; } BIRNET_CPP_PASTE2 (Birnet_StaticAssertion_LINE, __LINE__)
#define BIRNET_STATIC_ASSERT(expr)              BIRNET_STATIC_ASSERT_NAMED (expr, compile_time_assertion_failed)

/* --- common type definitions --- */
typedef unsigned int	      BirnetUInt8	__attribute__ ((__mode__ (__QI__)));
typedef unsigned int	      BirnetUInt16	__attribute__ ((__mode__ (__HI__)));
typedef unsigned int	      BirnetUInt32	__attribute__ ((__mode__ (__SI__)));
typedef unsigned int	      BirnetUInt64	__attribute__ ((__mode__ (__DI__)));
typedef BirnetUInt32          BirnetUInt;
BIRNET_STATIC_ASSERT (sizeof (BirnetUInt8) == 1);
BIRNET_STATIC_ASSERT (sizeof (BirnetUInt16) == 2);
BIRNET_STATIC_ASSERT (sizeof (BirnetUInt32) == 4);
BIRNET_STATIC_ASSERT (sizeof (BirnetUInt64) == 8);
typedef signed int  	      BirnetInt8	__attribute__ ((__mode__ (__QI__)));
typedef signed int  	      BirnetInt16	__attribute__ ((__mode__ (__HI__)));
typedef signed int  	      BirnetInt32	__attribute__ ((__mode__ (__SI__)));
typedef signed int  	      BirnetInt64	__attribute__ ((__mode__ (__DI__)));
typedef BirnetInt32           BirnetInt;
BIRNET_STATIC_ASSERT (sizeof (BirnetInt8) == 1);
BIRNET_STATIC_ASSERT (sizeof (BirnetInt16) == 2);
BIRNET_STATIC_ASSERT (sizeof (BirnetInt32) == 4);
BIRNET_STATIC_ASSERT (sizeof (BirnetInt64) == 8);
typedef BirnetUInt32          BirnetUniChar;

/* --- attributes --- */
#if     __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
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
#else   /* !__GNUC__ */
#define BIRNET_PRETTY_FUNCTION                  (__func__)
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
#error  Failed to detect a recent GCC version (>= 3.1)
#endif  /* !__GNUC__ */

/* --- convenient type shorthands --- */
#ifdef  _BIRNET_SOURCE_EXTENSIONS
#if 	BIRNET_SIZEOF_SYS_TYPESH_UINT == 0
typedef unsigned int uint;	/* for systems that don't define uint in types.h */
#else
BIRNET_STATIC_ASSERT (BIRNET_SIZEOF_SYS_TYPESH_UINT == 4);
#endif
BIRNET_STATIC_ASSERT (sizeof (uint) == 4);
typedef BirnetUInt8		uint8;
typedef BirnetUInt16		uint16;
typedef BirnetUInt32		uint32;
typedef BirnetUInt64		uint64;
typedef BirnetInt8		int8;
typedef BirnetInt16		int16;
typedef BirnetInt32		int32;
typedef BirnetInt64		int64;
typedef BirnetUniChar		unichar;
#endif /* _BIRNET_SOURCE_EXTENSIONS */

/* --- path handling --- */
#define BIRNET_DIR_SEPARATOR		  G_DIR_SEPARATOR		/* '/' */
#define BIRNET_DIR_SEPARATOR_S		  G_DIR_SEPARATOR_S		/* "/" */
#define	BIRNET_IS_DIR_SEPARATOR(c)    	  G_IS_DIR_SEPARATOR(c)		/* (c) == G_DIR_SEPARATOR */
#define BIRNET_SEARCHPATH_SEPARATOR	  G_SEARCHPATH_SEPARATOR	/* ':' */
#define BIRNET_SEARCHPATH_SEPARATOR_S	  G_SEARCHPATH_SEPARATOR_S	/* ":" */
#ifdef G_OS_WIN32
#define BIRNET_IS_SEARCHPATH_SEPARATOR(c) ((c) == G_SEARCHPATH_SEPARATOR /* ';' */)
#else /* !G_OS_WIN32 */
#define BIRNET_IS_SEARCHPATH_SEPARATOR(c) ((c) == G_SEARCHPATH_SEPARATOR || (c) == ';')
#endif

/* --- birnet initialization --- */
typedef struct
{
  const char *value_name;	/* value list ends with value_name == NULL */
  const char *value_string;
  long double value_num;	/* valid if value_string == NULL */
} BirnetInitValue;
void	birnet_init_extended     (int             *argcp,
				  char          ***argvp,
				  const char      *app_name,
				  BirnetInitValue  bivalues[]); /* in birnetutilsxx.cc */
void	birnet_init 	         (int             *argcp,
				  char          ***argvp,
				  const char      *app_name);
bool	birnet_init_value_bool	 (BirnetInitValue *value);
double	birnet_init_value_double (BirnetInitValue *value);
gint64	birnet_init_value_int    (BirnetInitValue *value);
typedef struct {
  bool	stand_alone;	/* "stand-alone": no rcfiles, boot scripts, etc. */
} BirnetInitSettings;
extern BirnetInitSettings *birnet_init_settings;

BIRNET_EXTERN_C_END();

#endif /* __BIRNET_CORE_H__ */

/* vim:set ts=8 sts=2 sw=2: */
