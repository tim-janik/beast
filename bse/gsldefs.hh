// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __GSL_DEFS_H__
#define __GSL_DEFS_H__

#include <sfi/sfi.hh>
#include <sfi/sfistore.hh>


G_BEGIN_DECLS

/* --- forward decls --- */
typedef struct _GslMagic		GslMagic;
typedef struct _GslDataCache		GslDataCache;
typedef struct _GslDataHandle		GslDataHandle;
typedef struct _GslDataHandleFuncs	GslDataHandleFuncs;
typedef struct _GslWaveChunk		GslWaveChunk;
typedef struct _GslWaveChunkBlock	GslWaveChunkBlock;
/* ssize_t/off_t type used within Gsl */

/*
 * FIXME: GslLong is a temporary typedef - it should go away after all
 * code that uses GslLong has been ported to use int64 (as defined in
 * sfitypes.h).
 */
typedef int64			  GslLong;
#define	GSL_MAXLONG		  G_MAXINT64;
#define	GSL_MINLONG		  G_MININT64;


/* --- functions --- */
typedef void     (*GslFreeFunc)         (gpointer        data);


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
