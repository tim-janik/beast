// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __GSL_COMMON_H__
#define __GSL_COMMON_H__
#include <bse/gsldefs.hh>
#include <bse/bseenums.hh>
G_BEGIN_DECLS
/* --- initialization --- */
void			gsl_init	(void);
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
#define	GSL_SPIN_LOCK	sfi_mutex_lock
#define	GSL_SPIN_UNLOCK	sfi_mutex_unlock
#define	GSL_SYNC_LOCK	sfi_mutex_lock
#define	GSL_SYNC_UNLOCK	sfi_mutex_unlock
/* --- misc --- */
const gchar* gsl_byte_order_to_string   (guint           byte_order);
guint        gsl_byte_order_from_string (const gchar    *string);
BseErrorType gsl_error_from_errno	(gint		 sys_errno,
					 BseErrorType	 fallback);
BseErrorType gsl_error_select           (guint           n_errors,
                                         BseErrorType    first_error,
                                         ...);
BseErrorType gsl_file_check		(const gchar	*file_name,
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
void	_gsl_init_fd_pool		(void);
void	_gsl_init_data_caches		(void);
void	_gsl_init_loader_gslwave	(void);
void	_gsl_init_loader_aiff		(void);
void	_gsl_init_loader_wav		(void);
void	_gsl_init_loader_oggvorbis	(void);
void	_gsl_init_loader_mad		(void);
void	bse_init_loader_gus_patch	(void);
#define		GSL_N_IO_RETRIES	(5)
#define		_GSL_TICK_STAMP_VAL()	(bse_engine_exvar_tick_stamp + 0)
extern volatile guint64	bse_engine_exvar_tick_stamp;
G_END_DECLS
#endif /* __GSL_COMMON_H__ */
