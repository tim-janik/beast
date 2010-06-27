// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __GSL_COMMON_H__
#define __GSL_COMMON_H__
#include <bse/gsldefs.hh>
#include <bse/bseenums.hh>
G_BEGIN_DECLS
/* --- initialization --- */
void			gsl_init	(void);

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
  uint            wipe_length;
  int             precision;
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
void	_gsl_init_fd_pool		(void);
void	_gsl_init_data_caches		(void);
void	_gsl_init_loader_gslwave	(void);
void	_gsl_init_loader_aiff		(void);
void	_gsl_init_loader_wav		(void);
void	_gsl_init_loader_oggvorbis	(void);
void	_gsl_init_loader_mad		(void);
void	bse_init_loader_gus_patch	(void);
void	bse_init_loader_flac      	(void);
#define		GSL_N_IO_RETRIES	(5)
G_END_DECLS


namespace Bse {

// == TickStamp ==
class TickStamp {
  static Rapicorn::Atomic<uint64> global_tick_stamp;
protected:
  static void           _init_forgsl  ();
public:
  class Wakeup : public std::enable_shared_from_this<Wakeup> {
    std::function<void()> wakeup_;
    uint64                awake_stamp_;
  protected:
    explicit            Wakeup (const std::function<void()> &wakeup);
    static void         _emit_wakeups   (uint64 wakeup_stamp);
  public:
    void                awake_after     (uint64 stamp);
    void                awake_before    (uint64 stamp);
  };
  typedef std::shared_ptr<Wakeup> WakeupP;
  struct Update {
    uint64 tick_stamp;
    uint64 system_time;
  };
  static Update         get_last        ();
  static WakeupP        create_wakeup   (const std::function<void()> &wakeup);
  static inline uint64  current         ()      { return global_tick_stamp; }
  static inline uint64  max_stamp       ()      { return 18446744073709551615LLU; } ///< Maximum stamp value, 2^64-1.
  static void	        _increment      ();
  static void	        _set_leap       (uint64 ticks);
};
typedef TickStamp::WakeupP TickStampWakeupP;

} // Bse


#endif /* __GSL_COMMON_H__ */
