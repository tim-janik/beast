/* GSL - Generic Sound Layer
 * Copyright (C) 2001-2003 Stefan Westerfeld and Tim Janik
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
#ifndef __GSL_DEFS_H__
#define __GSL_DEFS_H__

#include <sfi/sfi.h>
#include <sfi/sfistore.h>


/* configure checks */
#include <bse/gslconfig.h>

G_BEGIN_DECLS

/* --- forward decls --- */
typedef struct _GslMagic		GslMagic;
typedef struct _GslComplex		GslComplex;
typedef struct _GslDataCache		GslDataCache;
typedef struct _GslDataHandle		GslDataHandle;
typedef struct _GslDataHandleFuncs	GslDataHandleFuncs;
typedef struct _GslLoader		GslLoader;
typedef struct _GslWaveChunk		GslWaveChunk;
typedef struct _GslWaveChunkBlock	GslWaveChunkBlock;
typedef struct _GslWaveFileInfo         GslWaveFileInfo;
typedef struct _GslWaveDsc              GslWaveDsc;
typedef struct _GslWaveChunkDsc         GslWaveChunkDsc;
/* ssize_t/off_t type used within Gsl */
typedef glong			  GslLong;
#define	GSL_MAXLONG		  G_MAXLONG
#define	GSL_MINLONG		  G_MINLONG


/* --- GSL errors --- */
typedef enum    /*< skip >*/
{
  GSL_ERROR_NONE,
  GSL_ERROR_INTERNAL,
  GSL_ERROR_UNKNOWN,
  /* file/directory errors */
  GSL_ERROR_IO,
  GSL_ERROR_PERMS,
  GSL_ERROR_BUSY,
  GSL_ERROR_EXISTS,
  GSL_ERROR_EOF,
#define GSL_ERROR_FILE_EMPTY	GSL_ERROR_EOF
  GSL_ERROR_NOT_FOUND,
  GSL_ERROR_IS_DIR,
  GSL_ERROR_OPEN_FAILED,
  GSL_ERROR_SEEK_FAILED,
  GSL_ERROR_READ_FAILED,
  GSL_ERROR_WRITE_FAILED,
  /* out of resource conditions */
  GSL_ERROR_MANY_FILES,
  GSL_ERROR_NO_FILES,
  GSL_ERROR_NO_SPACE,
  GSL_ERROR_NO_MEMORY,
  /* content errors */
  GSL_ERROR_NO_HEADER,
  GSL_ERROR_NO_SEEK_INFO,
  GSL_ERROR_NO_DATA,
  GSL_ERROR_DATA_CORRUPT,
  GSL_ERROR_FORMAT_INVALID,
  GSL_ERROR_FORMAT_UNKNOWN,
  /* miscellaneous errors */
  GSL_ERROR_TEMP,
  GSL_ERROR_WAVE_NOT_FOUND,
  GSL_ERROR_CODEC_FAILURE,
  GSL_ERROR_LAST		/*< skip >*/
} GslErrorType;

/* --- functions --- */
typedef void     (*GslFreeFunc)         (gpointer        data);


#if defined (BSE_COMPILATION) || defined (BSE_PLUGIN_FALLBACK) \
    || defined (GSL_WANT_GLIB_WRAPPER) || defined (GSL_EXTENSIONS)
#  define if_expect(cond)		if (GSL_GCC_EXPECT1 (cond))
#  define if_reject(cond)		if (GSL_GCC_EXPECT0 (cond))
#endif


/* --- implementation details --- */
#if __GNUC__ >= 3 && defined __OPTIMIZE__
#  define GSL_GCC_EXPECT1(cond)	  (__builtin_expect (cond, 1))
#  define GSL_GCC_EXPECT0(cond)	  (__builtin_expect (cond, 0))
#  define GSL_GCC_PREFETCH(addr)  (__builtin_prefetch (addr, 0))
#  define GSL_GCC_RPREFETCH(addr) (__builtin_prefetch (addr, 0))
#  define GSL_GCC_WPREFETCH(addr) (__builtin_prefetch (addr, 1))
#else
#  define GSL_GCC_EXPECT1(cond)	cond
#  define GSL_GCC_EXPECT0(cond)	cond
#  define GSL_GCC_PREFETCH(addr)  /* addr */
#  define GSL_GCC_RPREFETCH(addr) /* addr */
#  define GSL_GCC_WPREFETCH(addr) /* addr */
#endif

G_END_DECLS

#endif /* __GSL_DEFS_H__ */

/* vim:set ts=8 sw=2 sts=2: */
