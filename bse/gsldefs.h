/* GSL - Generic Sound Layer
 * Copyright (C) 2001 Stefan Westerfeld and Tim Janik
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

#ifdef	GSL_WANT_GLIB_WRAPPER
#include <gsl/gslglib.h>	/* GSL just uses a certain subset of GLib */
#else
#include <glib.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- forward decls --- */
typedef struct _GslClass		GslClass;
typedef struct _GslComplex		GslComplex;
typedef struct _GslCond			GslCond;
typedef struct _GslDataCache		GslDataCache;
typedef struct _GslDataHandle		GslDataHandle;
typedef struct _GslDataHandleFuncs	GslDataHandleFuncs;
typedef struct _GslJob			GslJob;
typedef struct _GslModule		GslModule;
typedef struct _GslIStream		GslIStream;
typedef struct _GslJStream		GslJStream;
typedef struct _GslOStream		GslOStream;
typedef struct _GslTrans		GslTrans;
typedef struct _GslWaveDsc		GslWaveDsc;
typedef struct _GslWaveChunk		GslWaveChunk;
typedef struct _GslWaveChunkBlock	GslWaveChunkBlock;
typedef struct _GslWaveChunkDsc		GslWaveChunkDsc;
typedef struct _GslRecMutex		GslRecMutex;
typedef struct _GslRing			GslRing;
typedef union _GslMutex			GslMutex;
/* ssize_t/off_t type used within Gsl */
typedef glong			  GslLong;
#define	GSL_MAXLONG		  G_MAXLONG


/* --- functions --- */
typedef void     (*GslAccessFunc)       (GslModule      *module,
					 gpointer        data);
typedef void     (*GslFreeFunc)         (gpointer        data);
typedef void     (*GslModuleFreeFunc)   (gpointer        data,
					 const GslClass	*klass);


/* --- GslPollFD (for poll(2)) --- */
#define GSL_POLLIN      (0x0001 /* There is data to read */)
#define GSL_POLLPRI     (0x0002 /* There is urgent data to read */)
#define GSL_POLLOUT     (0x0004 /* Writing now will not block */)
#define GSL_POLLERR     (0x0008 /* Error condition */)
#define GSL_POLLHUP     (0x0010 /* Hung up */)
#define GSL_POLLNVAL    (0x0020 /* Invalid request: fd not open */)
typedef struct
{
  gint    fd;
  gushort events;
  gushort revents;
} GslPollFD;


/* --- implementation specific --- */
/*< private >*/
/* FIXME: gslconfig.h stuff */
#define	GSL_ENGINE_MAX_POLLFDS	(128)
#define GSL_SIZEOF_MUTEX        (8)
#define GSL_SIZEOF_GTIME        (4)
#define GSL_SIZEOF_GUINT        (4)
#define GSL_SIZEOF_GSIZE        (4)
#define GSL_SIZEOF_INTMAX	(8)
union _GslMutex
{
  gpointer mutex_pointer;
  guint8   mutex_dummy[GSL_SIZEOF_MUTEX];
};
struct _GslRecMutex
{
  guint    depth;
  gpointer owner;
  GslMutex sync_mutex;
};


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_DEFS_H__ */
/* vim:set ts=8 sw=2 sts=2: */
