/* GSL Engine - Flow module operation engine
 * Copyright (C) 2001, 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __GSL_ENGINE_H__
#define __GSL_ENGINE_H__

#include <bse/gsldefs.h>

G_BEGIN_DECLS

/* --- constants --- */
#define	GSL_STREAM_MAX_VALUES		        (1024 /* power of 2 and <= 16384 */)
#define	GSL_MODULE_N_OSTREAMS(module)	        ((module)->klass->n_ostreams)
#define	GSL_MODULE_N_ISTREAMS(module)	        ((module)->klass->n_istreams)
#define	GSL_MODULE_N_JSTREAMS(module)	        ((module)->klass->n_jstreams)
#define	GSL_MODULE_ISTREAM(module, stream)      ((module)->istreams[(stream)])
#define	GSL_MODULE_JSTREAM(module, stream)      ((module)->jstreams[(stream)])
#define	GSL_MODULE_OSTREAM(module, stream)      ((module)->ostreams[(stream)])
#define	GSL_MODULE_IBUFFER(module, stream)      (GSL_MODULE_ISTREAM ((module), (stream)).values)
#define	GSL_MODULE_JBUFFER(module, stream, con) (GSL_MODULE_JSTREAM ((module), (stream)).values[con])
#define	GSL_MODULE_OBUFFER(module, stream)      (GSL_MODULE_OSTREAM ((module), (stream)).values)


/* --- typedefs --- */
typedef struct _GslProbe GslProbe;
typedef gboolean (*GslPollFunc)		(gpointer	data,
					 guint		n_values,
					 glong	       *timeout_p,
					 guint          n_fds,
					 const GPollFD *fds,
					 gboolean	revents_filled);
typedef gboolean (*GslEngineTimerFunc)	(gpointer	data,
					 guint64	tick_stamp);
typedef void     (*GslProcessFunc)	(GslModule     *module,
					 guint		n_values);
typedef guint    (*GslProcessDeferFunc)	(GslModule     *module,
					 guint		n_ivalues,
					 guint		n_ovalues);
typedef void     (*GslResetFunc)	(GslModule     *module);
typedef void     (*GslProbeFunc)	(gpointer       data,
                                         guint64        tick_stamp,
                                         guint          n_values,
                                         gfloat       **oblocks); /* [ENGINE_NODE_N_OSTREAMS()] */

/* gsldefs.h:
 * typedef void  (*GslAccessFunc)	(GslModule	*module,
 *					 gpointer	 data);
 * typedef void  (*GslFreeFunc)		(gpointer	 data);
 * typedef void  (*GslModuleFreeFunc)	(gpointer	 data,
 *                                       const GslClass *klass);
 */
typedef enum	/*< skip >*/
{
  GSL_COST_NORMAL	= 0,
  GSL_COST_CHEAP	= 1 << 0,
  GSL_COST_EXPENSIVE	= 1 << 1
} GslModuleFlags;
/* class, filled out by user */
struct _GslClass
{
  guint		      n_istreams;
  guint		      n_jstreams;
  guint		      n_ostreams;
  GslProcessFunc      process;		/* EngineThread */
  GslProcessDeferFunc process_defer;	/* EngineThread */
  GslResetFunc	      reset;		/* EngineThread */
  GslModuleFreeFunc   free;		/* UserThread */
  GslModuleFlags      mflags;
};
/* module, constructed by engine */
struct _GslModule
{
  const GslClass *klass;
  gpointer	  user_data;
  GslIStream	 *istreams;	/* input streams */
  GslJStream	 *jstreams;	/* joint (multiconnect) input streams */
  GslOStream	 *ostreams;	/* output streams */
};
/* streams, constructed by engine */
struct _GslJStream
{
  const gfloat **values;
  guint          n_connections;	/* scheduler update */
  /*< private >*/
  guint		 jcount;	/* internal field */
};
struct _GslIStream
{
  const gfloat *values;
  gboolean	connected;	/* scheduler update */
};
struct _GslOStream
{
  gfloat     *values;
  gboolean    connected;
};


/* --- interface (UserThread functions) --- */
GslModule*	gsl_module_new		(const GslClass	 *klass,
					 gpointer	  user_data);
GslModule*	gsl_module_new_virtual	(guint		  n_iostreams,
					 gpointer	  user_data,
					 GslFreeFunc	  free_data);
guint64		gsl_module_tick_stamp	(GslModule	 *module);
gboolean        gsl_module_has_source   (GslModule       *module,
                                         guint            istream);
GslJob*		gsl_job_connect		(GslModule	 *src_module,
					 guint		  src_ostream,
					 GslModule	 *dest_module,
					 guint		  dest_istream);
GslJob*		gsl_job_jconnect	(GslModule	 *src_module,
					 guint		  src_ostream,
					 GslModule	 *dest_module,
					 guint		  dest_jstream);
GslJob*		gsl_job_disconnect	(GslModule	 *dest_module,
					 guint		  dest_istream);
GslJob*		gsl_job_jdisconnect	(GslModule	 *dest_module,
					 guint		  dest_jstream,
					 GslModule	 *src_module,
					 guint		  src_ostream);
GslJob*		gsl_job_kill_inputs	(GslModule	 *module);
GslJob*		gsl_job_kill_outputs	(GslModule	 *module);
GslJob*		gsl_job_integrate	(GslModule	 *module);
GslJob*		gsl_job_discard		(GslModule	 *module);
GslJob*		gsl_job_force_reset	(GslModule	 *module);
GslJob*		gsl_job_access		(GslModule	 *module,
					 GslAccessFunc	  access_func,	/* EngineThread */
					 gpointer	  data,
					 GslFreeFunc	  free_func);	/* UserThread */
GslJob*		gsl_job_set_consumer	(GslModule	 *module,
					 gboolean	  is_toplevel_consumer);
GslJob*		gsl_job_suspend_now	(GslModule	 *module);
GslJob*		gsl_job_resume_at	(GslModule	 *module,
					 guint64	  tick_stamp);
GslJob*		gsl_job_debug		(const gchar	 *debug);
GslJob*		gsl_job_add_poll	(GslPollFunc	  poll_func,
					 gpointer	  data,
					 GslFreeFunc	  free_func,
					 guint		  n_fds,
					 const GPollFD   *fds);
GslJob*		gsl_job_remove_poll	(GslPollFunc	  poll_func,
					 gpointer	  data);
GslJob*		gsl_job_add_timer	(GslEngineTimerFunc timer_func,
					 gpointer	  data,
					 GslFreeFunc	  free_func);
GslTrans*	gsl_trans_open		(void);
void		gsl_trans_add		(GslTrans	 *trans,
					 GslJob		 *job);
GslTrans*	gsl_trans_merge		(GslTrans	 *trans1,
					 GslTrans	 *trans2);
void		gsl_trans_commit	(GslTrans	 *trans);
void		gsl_trans_commit_delayed(GslTrans	 *trans,
					 guint64	  tick_stamp);
void		gsl_trans_dismiss	(GslTrans	 *trans);
void		gsl_transact		(GslJob		 *job,
					 ...);
GslJob*         gsl_job_probe_request   (GslModule       *module,
                                         guint8          *ochannel_bytemask,
                                         GslProbeFunc     probe,        /* UserThread */
                                         gpointer         data);
GslJob*		gsl_job_flow_access	(GslModule	 *module,
					 guint64	  tick_stamp,
					 GslAccessFunc	  access_func,	/* EngineThread */
					 gpointer	  data,
					 GslFreeFunc	  free_func);	/* UserThread */
GslJob*		gsl_job_boundary_access	(GslModule	 *module,
					 guint64	  tick_stamp,
					 GslAccessFunc	  access_func,	/* EngineThread */
					 gpointer	  data,
					 GslFreeFunc      free_func);	/* UserThread */


/* --- module utilities (EngineThread functions) --- */
gfloat*	      gsl_engine_const_values   (gfloat		  value);


/* --- initialization & main loop --- */
void            gsl_engine_constrain    (guint            latency_ms,
                                         guint            sample_freq,
                                         guint            control_freq,
                                         guint           *block_size_p,
                                         guint           *control_raster_p);
void	        gsl_engine_init		(gboolean	  threaded);
gboolean        gsl_engine_configure    (guint		  latency_ms,
					 guint		  sample_freq,
					 guint		  control_freq);
typedef struct
{
  glong		timeout;
  gboolean	fds_changed;
  guint		n_fds;
  GPollFD      *fds;
  gboolean	revents_filled;
} GslEngineLoop;
gboolean        gsl_engine_prepare	(GslEngineLoop		*loop);
gboolean        gsl_engine_check	(const GslEngineLoop	*loop);
void	        gsl_engine_dispatch	(void);
SfiThread**     gsl_engine_get_threads	(guint                  *n_threads);


/* --- miscellaneous --- */
gboolean      gsl_engine_has_garbage		 (void);
void	      gsl_engine_garbage_collect	 (void);
void	      gsl_engine_wait_on_trans		 (void);
guint64	      gsl_engine_tick_stamp_from_systime (guint64	systime);
#define	      gsl_engine_block_size()		 (0 + (const guint) gsl_externvar_block_size)
#define	      gsl_engine_sample_freq()		 (0 + (const guint) gsl_externvar_sample_freq)
#define	      gsl_engine_control_raster()	 (1 + (const guint) gsl_externvar_control_mask)
#define	      gsl_engine_control_mask()	         (0 + (const guint) gsl_externvar_control_mask)
#define	      GSL_CONTROL_CHECK(index)	         ((gsl_engine_control_mask() & (index)) == 0)


/*< private >*/
extern guint	gsl_externvar_block_size;
extern guint	gsl_externvar_sample_freq;
extern guint	gsl_externvar_control_mask;

G_END_DECLS

#endif /* __GSL_ENGINE_H__ */
