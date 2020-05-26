// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_ENGINE_H__
#define __BSE_ENGINE_H__

#include <bse/bseenginenode.hh>

/* --- constants --- */
#define BSE_ENGINE_MAX_BLOCK_SIZE               (128)
#define BSE_MODULE_N_OSTREAMS(module)           ((module)->n_ostreams)
#define BSE_MODULE_N_ISTREAMS(module)           ((module)->n_istreams)
#define BSE_MODULE_N_JSTREAMS(module)           ((module)->n_jstreams)
#define BSE_MODULE_ISTREAM(module, stream)      ((module)->istreams[(stream)])
#define BSE_MODULE_JSTREAM(module, stream)      ((module)->jstreams[(stream)])
#define BSE_MODULE_OSTREAM(module, stream)      ((module)->ostreams[(stream)])
#define BSE_MODULE_IBUFFER(module, stream)      (BSE_MODULE_ISTREAM ((module), (stream)).values)
#define BSE_MODULE_JBUFFER(module, stream, con) (BSE_MODULE_JSTREAM ((module), (stream)).values[con])
#define BSE_MODULE_OBUFFER(module, stream)      (BSE_MODULE_OSTREAM ((module), (stream)).values)
#define	BSE_MODULE_IS_EXPENSIVE(module)	        (0 != (size_t ((module)->klass.mflags) & size_t (Bse::ModuleFlag::EXPENSIVE)))
#define BSE_ENGINE_MAX_POLLFDS                  (128)


/* --- typedefs --- */
/* bsedefs.hh:
 * typedef void (*BseEngineAccessFunc)  (BseModule      *module,
 *                                       gpointer        data);
 * typedef void (*BseFreeFunc)          (gpointer        data);
 */
typedef gboolean (*BseEnginePollFunc)   (gpointer       data,
                                         guint          n_values,
                                         glong         *timeout_p,
                                         guint          n_fds,
                                         const GPollFD *fds,
                                         gboolean       revents_filled);
typedef gboolean (*BseEngineTimerFunc)  (gpointer       data,
                                         guint64        tick_stamp);
typedef void     (*BseEngineProbeFunc)  (gpointer       data,
					 guint          n_values,	/* bse_engine_block_size() */
					 guint64        tick_stamp,
					 guint          n_ostreams,	/* BSE_MODULE_N_OSTREAMS() */
					 BseOStream   **ostreams_p);
typedef void     (*BseProcessFunc)      (BseModule     *module,
                                         guint          n_values);
typedef guint    (*BseProcessDeferFunc) (BseModule     *module,
                                         guint          n_ivalues,
                                         guint          n_ovalues);
typedef void     (*BseModuleResetFunc)  (BseModule     *module);
typedef void     (*BseModuleFreeFunc)   (gpointer        data,
                                         const BseModuleClass *klass);

namespace Bse {

enum class ModuleFlag {
  NORMAL        = 0,      ///< Nutral flag
  CHEAP         = 1 << 0, ///< Very short or NOP as process() function
  EXPENSIVE     = 1 << 1, ///< Indicate lengthy process() functio
  VIRTUAL_      = 1 << 7, ///< Flag used internally
};

// streams, constructed by engine
struct JStream {
  const float **values;
  uint          n_connections;  // scheduler update
  /*< private >*/
  uint          jcount;         // internal field
};
struct IStream {
  const float *values;
  bool         connected;       // scheduler update
};
struct OStream {
  float *values;
  bool   connected;
};

} // Bse

// class, filled out by user
struct BseModuleClass {
  uint                n_istreams;
  uint                n_jstreams;
  uint                n_ostreams;
  BseProcessFunc      process;          // EngineThread
  BseProcessDeferFunc process_defer;    // EngineThread
  BseModuleResetFunc  reset;            // EngineThread
  BseModuleFreeFunc   free;             // UserThread
  Bse::ModuleFlag     mflags;
};

/* --- interface (UserThread functions) --- */
BseModule* bse_module_new               (const BseModuleClass *klass,
                                         gpointer              user_data);
BseModule* bse_module_new_virtual       (guint                 n_iostreams,
                                         gpointer              user_data,
                                         BseFreeFunc           free_data);
guint64    bse_module_tick_stamp        (BseModule            *module);
gboolean   bse_module_has_source        (BseModule            *module,
                                         guint                 istream);
gboolean   bse_module_is_scheduled      (BseModule            *module);
BseJob*    bse_job_connect              (BseModule            *src_module,
                                         guint                 src_ostream,
                                         BseModule            *dest_module,
                                         guint                 dest_istream);
BseJob*    bse_job_jconnect             (BseModule            *src_module,
                                         guint                 src_ostream,
                                         BseModule            *dest_module,
                                         guint                 dest_jstream);
BseJob*    bse_job_disconnect           (BseModule            *dest_module,
                                         guint                 dest_istream);
BseJob*    bse_job_jdisconnect          (BseModule            *dest_module,
                                         guint                 dest_jstream,
                                         BseModule            *src_module,
                                         guint                 src_ostream);
BseJob*    bse_job_kill_inputs          (BseModule            *module);
BseJob*    bse_job_kill_outputs         (BseModule            *module);
BseJob*    bse_job_integrate            (BseModule            *module);
BseJob*    bse_job_discard              (BseModule            *module);
BseJob*    bse_job_force_reset          (BseModule            *module);
BseJob*    bse_job_set_consumer         (BseModule            *module,
                                         gboolean              is_toplevel_consumer);
BseJob*    bse_job_suspend_now          (BseModule            *module);
BseJob*    bse_job_resume_at            (BseModule            *module,
                                         guint64               tick_stamp);
BseJob*    bse_job_debug                (const gchar          *debug);
BseJob*    bse_job_nop                  (void);
BseJob*    bse_job_add_poll             (BseEnginePollFunc     poll_func,
                                         gpointer              data,
                                         BseFreeFunc           free_func,
                                         guint                 n_fds,
                                         const GPollFD        *fds);
BseJob*    bse_job_remove_poll          (BseEnginePollFunc     poll_func,
                                         gpointer              data);
BseJob*    bse_job_add_timer            (BseEngineTimerFunc    timer_func,
                                         gpointer              data,
                                         BseFreeFunc           free_func);
BseJob*    bse_job_access               (BseModule            *module,
                                         const std::function<void()> &lambda); /* EngineThread */
BseJob*    bse_job_access               (BseModule            *module,
                                         BseEngineAccessFunc   access_func,     /* EngineThread */
                                         gpointer              data,
                                         BseFreeFunc           free_func);      /* UserThread */
BseJob*    bse_job_probe_request        (BseModule            *module,
                                         BseEngineProbeFunc    probe,           /* UserThread */
                                         gpointer              data);
BseJob*    bse_job_flow_access          (BseModule            *module,
                                         guint64               tick_stamp,
                                         BseEngineAccessFunc   access_func,     /* EngineThread */
                                         gpointer              data,
                                         BseFreeFunc           free_func);      /* UserThread */
BseJob*    bse_job_boundary_access      (BseModule            *module,
                                         guint64               tick_stamp,
                                         BseEngineAccessFunc   access_func,     /* EngineThread */
                                         gpointer              data,
                                         BseFreeFunc           free_func);      /* UserThread */
BseJob*    bse_job_boundary_discard     (BseModule            *module);
BseTrans*  bse_trans_open               (void);
void       bse_trans_add                (BseTrans             *trans,
                                         BseJob               *job);
BseTrans*  bse_trans_merge              (BseTrans             *trans1,
                                         BseTrans             *trans2);
guint64    bse_trans_commit             (BseTrans             *trans);
void       bse_trans_commit_delayed     (BseTrans             *trans,
                                         guint64               tick_stamp);
void       bse_trans_dismiss            (BseTrans             *trans);
void       bse_transact                 (BseJob               *job,
                                         ...) G_GNUC_NULL_TERMINATED;

/* --- module utilities (EngineThread functions) --- */
float*     bse_engine_const_values      (float value);

/* --- initialization & main loop --- */
void       bse_engine_init              ();
void       bse_engine_shutdown          ();
bool       bse_engine_configure         ();

/* --- miscellaneous --- */
float*     bse_engine_const_zeros	      (uint smaller_than_MAX_BLOCK_SIZE);
gboolean   bse_engine_has_garbage             (void);
void       bse_engine_user_thread_collect     (void);
void       bse_engine_free_ostreams	      (guint         n_ostreams,
					       BseOStream   *ostreams);
void       bse_engine_add_user_callback       (gpointer      data,
                                               BseFreeFunc   free_func);        /* UserThread */
void       bse_engine_wait_on_trans           (void);
guint64    bse_engine_tick_stamp_from_systime (guint64       systime);
void       bse_engine_update_block_size       (uint new_block_size);
#define    bse_engine_block_size()            (0 + bse_engine_exvar_block_size)
#define    bse_engine_sample_freq()           (0 + bse_engine_exvar_sample_freq)
#define    bse_engine_control_raster()        (32) // legacy value
#define    BSE_CONTROL_CHECK(index)           ((bse_engine_control_mask() & (index)) == 0)

/* --- thread handling --- */
struct BseEngineLoop {
  glong         timeout;
  gboolean      fds_changed;
  guint         n_fds;
  GPollFD      *fds;
  gboolean      revents_filled;
};

gboolean    bse_engine_prepare                (BseEngineLoop       *loop);
gboolean    bse_engine_check                  (const BseEngineLoop *loop);
void        bse_engine_dispatch               (void);

/*< private >*/
extern const uint bse_engine_exvar_sample_freq;
extern uint       bse_engine_exvar_block_size;

#endif /* __BSE_ENGINE_H__ */
