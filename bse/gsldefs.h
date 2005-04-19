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
typedef struct _GslDataCache		GslDataCache;
typedef struct _GslDataHandle		GslDataHandle;
typedef struct _GslDataHandleFuncs	GslDataHandleFuncs;
typedef struct _GslWaveChunk		GslWaveChunk;
typedef struct _GslWaveChunkBlock	GslWaveChunkBlock;
/* ssize_t/off_t type used within Gsl */
typedef glong			  GslLong;
#define	GSL_MAXLONG		  G_MAXLONG
#define	GSL_MINLONG		  G_MINLONG


/* --- functions --- */
typedef void     (*GslFreeFunc)         (gpointer        data);


#if defined (BSE_COMPILATION) || defined (BSE_PLUGIN_FALLBACK) \
    || defined (GSL_WANT_GLIB_WRAPPER) || defined (GSL_EXTENSIONS)
#  define LIKELY(cond)		G_LIKELY (cond)
#  define ISLIKELY(cond)	G_LIKELY (cond)
#  define UNLIKELY(cond)        G_UNLIKELY (cond)
#endif


/* --- implementation details --- */
#if __GNUC__ >= 3 && defined __OPTIMIZE__
#  define GSL_GCC_PREFETCH(addr)  (__builtin_prefetch (addr, 0))
#  define GSL_GCC_RPREFETCH(addr) (__builtin_prefetch (addr, 0))
#  define GSL_GCC_WPREFETCH(addr) (__builtin_prefetch (addr, 1))
#else
#  define GSL_GCC_PREFETCH(addr)  /* addr */
#  define GSL_GCC_RPREFETCH(addr) /* addr */
#  define GSL_GCC_WPREFETCH(addr) /* addr */
#endif

G_END_DECLS

#endif /* __GSL_DEFS_H__ */

/* vim:set ts=8 sw=2 sts=2: */
