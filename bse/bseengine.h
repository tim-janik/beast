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

#include	<gsl/gsldefs.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- constants --- */
#define	GSL_STREAM_MAX_VALUES		        (8192 /* power of 2, <= 16384 */)	/* FIXME */
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
typedef gboolean (*GslPollFunc)		(gpointer	data,
					 guint		n_values,
					 glong	       *timeout_p,
					 guint          n_fds,
					 const GPollFD *fds,
					 gboolean	revents_filled);
typedef void     (*GslProcessFunc)	(GslModule	 *module,
					 guint		  n_values);
typedef guint    (*GslProcessDeferFunc)	(GslModule	 *module,
					 guint		  n_ivalues,
					 guint		  n_ovalues);
typedef void     (*GslResetFunc)	(GslModule	 *module);
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
  guint		 jcount;	/* internal field */
  guint		 reserved : 16;
};
struct _GslIStream
{
  const gfloat *values;
  guint		reserved : 16;
  guint		connected : 1;	/* scheduler update */
};
struct _GslOStream
{
  gfloat     *values;
  guint	      sub_sample_pattern : 16;
  guint	      connected : 1;
};


/* --- interface (UserThread functions) --- */
GslModule*	gsl_module_new		(const GslClass	 *klass,
					 gpointer	  user_data);
GslModule*	gsl_module_new_virtual	(guint		  n_iostreams,
					 gpointer	  user_data,
					 GslFreeFunc	  free_data);
guint64		gsl_module_tick_stamp	(GslModule	 *module);
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
GslJob*		gsl_job_access		(GslModule	 *module,
					 GslAccessFunc	  access_func,	/* EngineThread */
					 gpointer	  data,
					 GslFreeFunc	  free_func);	/* UserThread */
GslJob*		gsl_job_set_consumer	(GslModule	 *module,
					 gboolean	  is_toplevel_consumer);
GslJob*		gsl_job_suspend		(GslModule	 *module);
GslJob*		gsl_job_debug		(const gchar	 *debug);
GslJob*		gsl_job_add_poll	(GslPollFunc	  poll_func,
					 gpointer	  data,
					 GslFreeFunc	  free_func,
					 guint		  n_fds,
					 const GPollFD   *fds);
GslJob*		gsl_job_remove_poll	(GslPollFunc	  poll_func,
					 gpointer	  data);
GslTrans*	gsl_trans_open		(void);
void		gsl_trans_add		(GslTrans	 *trans,
					 GslJob		 *job);
void		gsl_trans_commit	(GslTrans	 *trans);
void		gsl_trans_dismiss	(GslTrans	 *trans);
void		gsl_transact		(GslJob		 *job,
					 ...);
GslJob*		gsl_flow_job_access	(GslModule	 *module,
					 guint64	  tick_stamp,
					 GslAccessFunc	  access_func,	/* EngineThread */
					 gpointer	  data,
					 GslFreeFunc	  free_func);	/* UserThread */
GslJob*		gsl_flow_job_resume	(GslModule	 *module,
					 guint64	  tick_stamp);


/* --- module utilities --- */
gfloat*		gsl_engine_const_values	(gfloat		 value);


/* --- initialization & main loop --- */
void	        gsl_engine_init		(gboolean	 threaded,
					 guint		 block_size,
					 guint		 sample_freq,
					 guint		 sub_sample_mask);
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


/* --- miscellaneous --- */
gboolean      gsl_engine_has_garbage		 (void);
void	      gsl_engine_garbage_collect	 (void);
void	      gsl_engine_wait_on_trans		 (void);
guint64	      gsl_engine_tick_stamp_from_systime (guint64	systime);
#define	      gsl_engine_block_size()		 ((const guint)	gsl_externvar_bsize + 0)
#define	      gsl_engine_sample_freq()		 ((const guint)	gsl_externvar_sample_freq + 0)
#define	      gsl_engine_sub_sample_mask()	 ((const guint)	gsl_externvar_sub_sample_mask + 0)
#define	      gsl_engine_sub_sample_steps()	 ((const guint)	gsl_externvar_sub_sample_steps + 0)
#define	      gsl_engine_sub_sample_test(ptr)	 (((guint) (ptr)) & gsl_engine_sub_sample_mask ())
#define	      GSL_SUB_SAMPLE_MATCH(ptr,sspatrn)	 (gsl_engine_sub_sample_test (ptr) == (sspatrn))


/*< private >*/
extern guint	gsl_externvar_bsize;
extern guint	gsl_externvar_sample_freq;
extern guint	gsl_externvar_sub_sample_mask;
extern guint	gsl_externvar_sub_sample_steps;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_ENGINE_H__ */
