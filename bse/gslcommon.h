/* GSL - Generic Sound Layer
 * Copyright (C) 2001-2003 Tim Janik and Stefan Westerfeld
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

#include <bse/gsldefs.h>

G_BEGIN_DECLS


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


/* --- tick stamps --- */
typedef struct {
  guint64 tick_stamp;
  guint64 system_time;
} GslTickStampUpdate;
guint64		   gsl_tick_stamp	(void);
guint64		   gsl_time_system	(void);
GslTickStampUpdate gsl_tick_stamp_last	(void);
#define		   GSL_TICK_STAMP	(_GSL_TICK_STAMP_VAL ())
#define		   GSL_MAX_TICK_STAMP	(18446744073709551615LLU /* 2^64-1*/)
void		gsl_thread_awake_before	(guint64	 tick_stamp);
#define	GSL_SPIN_LOCK	SFI_SPIN_LOCK
#define	GSL_SPIN_UNLOCK	SFI_SPIN_UNLOCK
#define	GSL_SYNC_LOCK	SFI_SYNC_LOCK
#define	GSL_SYNC_UNLOCK	SFI_SYNC_UNLOCK


/* --- misc --- */
const gchar* gsl_strerror		(GslErrorType	error);
const gchar* gsl_byte_order_to_string   (guint           byte_order);
guint        gsl_byte_order_from_string (const gchar    *string);
GslErrorType gsl_error_from_errno	(gint		 sys_errno,
					 GslErrorType	 fallback);
GslErrorType gsl_error_select           (guint           n_errors,
                                         GslErrorType    first_error,
                                         ...);
GslErrorType gsl_file_check		(const gchar	*file_name,
					 const gchar	*mode);


/* --- progress notification --- */
typedef struct _GslProgressState GslProgressState;
typedef guint (*GslProgressFunc)        (gpointer          data,
                                         gfloat            pval, /* -1, 0..100 */
                                         const gchar      *detail,
                                         GslProgressState *pstate);
struct _GslProgressState
{
  guint           wipe_length, precision;
  gfloat          pval, epsilon;
  gpointer        pdata;
  GslProgressFunc pfunc;
};
GslProgressState gsl_progress_state     (gpointer          data,
                                         GslProgressFunc   pfunc,
                                         guint             precision);
void             gsl_progress_notify    (GslProgressState *pstate,
                                         gfloat            pval,
                                         const gchar      *detail_format,
                                         ...);
void             gsl_progress_wipe      (GslProgressState *pstate);
guint            gsl_progress_printerr  (gpointer          message,
                                         gfloat            pval,
                                         const gchar      *detail,
                                         GslProgressState *pstate);


/* --- implementation details --- */
void	       _gsl_tick_stamp_inc	(void);
void	       _gsl_tick_stamp_set_leap (guint		 ticks);
void	_gsl_init_signal		(void);
void	_gsl_init_fd_pool		(void);
void	_gsl_init_data_caches		(void);
void	_gsl_init_loader_gslwave	(void);
void	_gsl_init_loader_aiff		(void);
void	_gsl_init_loader_wav		(void);
void	_gsl_init_loader_oggvorbis	(void);
void	_gsl_init_loader_mad		(void);
#define		GSL_N_IO_RETRIES	(5)
#define		_GSL_TICK_STAMP_VAL()	(bse_engine_exvar_tick_stamp + 0)
extern volatile guint64	bse_engine_exvar_tick_stamp;


G_END_DECLS

#endif /* __GSL_COMMON_H__ */
