/* GSL - Generic Sound Layer
 * Copyright (C) 2001-2002 Tim Janik and Stefan Westerfeld
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

#include <gsl/gsldefs.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



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


/* --- GslMessage and debugging --- */
typedef enum /*< skip >*/
{
  GSL_MSG_NOTIFY	= 1 << 0,
  GSL_MSG_DATA_CACHE	= 1 << 1,
  GSL_MSG_DATA_HANDLE	= 1 << 2,
  GSL_MSG_LOADER	= 1 << 3,
  GSL_MSG_OSC		= 1 << 4,
  GSL_MSG_ENGINE	= 1 << 5,
  GSL_MSG_JOBS		= 1 << 6,
  GSL_MSG_FJOBS		= 1 << 7,
  GSL_MSG_SCHED		= 1 << 8,
  GSL_MSG_MASTER	= 1 << 9,
  GSL_MSG_SLAVE		= 1 << 10
} GslDebugFlags;
extern const GDebugKey *gsl_debug_keys;
extern const guint      gsl_n_debug_keys;
void		gsl_debug		(GslDebugFlags  reporter,
					 const gchar   *section,
					 const gchar   *format,
					 ...)	G_GNUC_PRINTF (3, 4);
void		gsl_debug_enable	(GslDebugFlags	dbg_flags);
void		gsl_debug_disable	(GslDebugFlags	dbg_flags);
gboolean	gsl_debug_check		(GslDebugFlags	dbg_flags);
void		gsl_message_send	(GslDebugFlags  reporter,
					 const gchar   *section,  /* maybe NULL */
					 GslErrorType	error,	  /* maybe 0 */
					 const gchar   *messagef,
					 ...)	G_GNUC_PRINTF (4, 5);
const gchar*	gsl_strerror		(GslErrorType	error);

/* provide message/debugging macro templates, so custom messages
 * are done as:
 * #define FOO_DEBUG	GSL_DEBUG_FUNCTION (GSL_MSG_LOADER, "FOO")
 * FOO_DEBUG ("some debug message and number: %d", 5);
 */
#define GSL_DEBUG_FUNCTION(reporter, section)	_GSL_DEBUG_MACRO_IMPL((reporter), (section))
#define GSL_MESSAGE_FUNCTION(reporter, section)	_GSL_MESSAGE_MACRO_IMPL((reporter), (section))


/* --- tick stamps --- */
typedef struct {
  guint64 tick_stamp;
  guint64 system_time;
} GslTickStampUpdate;
guint64		   gsl_tick_stamp	(void);
guint64		   gsl_time_system	(void);
GslTickStampUpdate gsl_tick_stamp_last	(void);
#define		   GSL_TICK_STAMP	(_GSL_TICK_STAMP_VAL ())
#define		   GSL_MAX_TICK_STAMP	(G_MAXUINT64)
void		gsl_thread_awake_before	(guint64	 tick_stamp);
#define	GSL_SPIN_LOCK	SFI_SPIN_LOCK
#define	GSL_SPIN_UNLOCK	SFI_SPIN_UNLOCK
#define	GSL_SYNC_LOCK	SFI_SYNC_LOCK
#define	GSL_SYNC_UNLOCK	SFI_SYNC_UNLOCK


/* --- misc --- */
const gchar* gsl_byte_order_to_string   (guint           byte_order);
guint        gsl_byte_order_from_string (const gchar    *string);
GslErrorType gsl_error_from_errno	(gint		 sys_errno,
					 GslErrorType	 fallback);
GslErrorType gsl_check_file		(const gchar	*file_name,
					 const gchar	*mode);


/* --- implementation details --- */
void	       _gsl_tick_stamp_inc	(void);
void	       _gsl_tick_stamp_set_leap (guint		 ticks);
void	_gsl_init_signal		(void);
void	_gsl_init_fd_pool		(void);
void	_gsl_init_data_caches		(void);
void	_gsl_init_engine_utils		(void);
void	_gsl_init_loader_gslwave	(void);
void	_gsl_init_loader_wav		(void);
void	_gsl_init_loader_oggvorbis	(void);
void	_gsl_init_loader_mad		(void);
#define		GSL_N_IO_RETRIES	(5)
#define		_GSL_TICK_STAMP_VAL()	(gsl_externvar_tick_stamp + 0)
extern volatile guint64	gsl_externvar_tick_stamp;

/* we need to provide a REPORTER and SECTION string for the debugging
 * and message generation functions. for GCC, we also want to make use
 * of printf style format checking with G_GNUC_PRINTF(). for the non GCC
 * case, we push REPORTER and SECTION as thread specific data before
 * invoking the debugging/message generation function. for the GCC case
 * we use GNUC varargs. (using ISO varargs wouldn't provide any benefit,
 * for one, ISO vararg support is broken with gcc-2.95 and ansi/c++, and
 * we only need the macro magic for GCC in the first place to make use
 * of G_GNUC_PRINTF()).
 */
#if     __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define _GSL_DEBUG_MACRO_IMPL(reporter, section)	gsl_debug ((reporter), (section), _GSL_DEBUG_GCCTAIL
#define _GSL_DEBUG_GCCTAIL(GCCARGS...)			GCCARGS )
#define _GSL_MESSAGE_MACRO_IMPL(reporter, section)	gsl_message_send ((reporter), (section), _GSL_MESSGAE_GCCTAIL
#define _GSL_MESSGAE_GCCTAIL(GCCARGS...)		GCCARGS )
#else   /* non GCC, push data and invoke function */
#define _GSL_DEBUG_MACRO_IMPL(reporter, section)	(gsl_auxlog_push ((reporter), (section)), gsl_auxlog_debug)
#define _GSL_MESSAGE_MACRO_IMPL(reporter, section)	(gsl_auxlog_push ((reporter), (section)), gsl_auxlog_message)
#endif
/* non-GCC message helpers */
void		gsl_auxlog_push		(GslDebugFlags   reporter,
					 const gchar	*section);
void		gsl_auxlog_debug	(const gchar	*format,
					 ...);
void		gsl_auxlog_message	(GslErrorType	 error,
					 const gchar	*format,
					 ...);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_COMMON_H__ */
