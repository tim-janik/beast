/* GSL Engine - Flow module operation engine
 * Copyright (C) 2001 Tim Janik
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
#ifndef __GSL_ENGINE_H__
#define __GSL_ENGINE_H__

#include	<gsl/gsldefs.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- constants --- */
#define	GSL_STREAM_MAX_VALUES		   (16384)	/* FIXME */
#define	GSL_MODULE_N_OSTREAMS(module)	   ((module)->klass->n_ostreams)
#define	GSL_MODULE_N_ISTREAMS(module)	   ((module)->klass->n_istreams)
#define	GSL_MODULE_OBUFFER(module, stream) ((module)->ostreams[(stream)].values)
#define	GSL_MODULE_IBUFFER(module, stream) ((module)->istreams[(stream)].values)


/* --- typedefs --- */
typedef gboolean (*GslPollFunc)		(gpointer	  data,
					 guint		  n_values,
					 glong		 *timeout_p,
					 guint            n_fds,
					 const GslPollFD *fds,
					 gboolean	  revents_filled);
typedef void     (*GslProcessFunc)	(GslModule	 *module,
					 guint		  n_values);
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
  GSL_COST_EXPENSIVE	= 1 << 1,
  GSL_ALWAYS_PROCESS	= 1 << 2
} GslModuleFlags;
/* class, filled out by user */
struct _GslClass
{
  guint		    n_istreams;
  guint		    n_jstreams;
  guint		    n_ostreams;
  GslProcessFunc    process;	/* EngineThread */
  GslModuleFreeFunc free;	/* UserThread */
  GslModuleFlags    mflags;
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
  guint          n_connections;
  const gfloat **values;
  guint		 user_flags : 16;
};
struct _GslIStream
{
  const gfloat *values;
  guint		user_flags : 16;
  const guint	connected : 1;
};
struct _GslOStream
{
  gfloat     *values;
  guint	      user_flags : 16;
  const guint connected : 1;
  guint	      zero_initialize : 1;
};


/* --- interface (UserThread functions) --- */
GslModule*	gsl_module_new		(const GslClass	 *klass,
					 gpointer	  user_data);
guint64		gsl_module_counter	(GslModule	 *module);
GslJob*		gsl_job_connect		(GslModule	 *src_module,
					 guint		  src_ostream,
					 GslModule	 *dest_module,
					 guint		  dest_istream);
GslJob*		gsl_job_disconnect	(GslModule	 *dest_module,
					 guint		  dest_istream);
GslJob*		gsl_job_integrate	(GslModule	 *module);
GslJob*		gsl_job_discard		(GslModule	 *module);
GslJob*		gsl_job_access		(GslModule	 *module,
					 GslAccessFunc	  access_func,	/* EngineThread */
					 gpointer	  data,
					 GslFreeFunc	  free_func);	/* UserThread */
GslJob*		gsl_job_debug		(const gchar	 *debug);
GslJob*		gsl_job_add_poll	(GslPollFunc	  poll_func,
					 gpointer	  data,
					 GslFreeFunc	  free_func,
					 guint		  n_fds,
					 const GslPollFD *fds);
GslJob*		gsl_job_remove_poll	(GslPollFunc	  poll_func,
					 gpointer	  data);
GslTrans*	gsl_trans_open		(void);
void		gsl_trans_add		(GslTrans	 *trans,
					 GslJob		 *job);
void		gsl_trans_commit	(GslTrans	 *trans);
void		gsl_trans_dismiss	(GslTrans	 *trans);
void		gsl_transact		(GslJob		 *job,
					 ...);


/* --- module utilities --- */
gfloat*		gsl_engine_const_values	(gfloat		 value);


/* --- initialization & main loop --- */
void	      gsl_engine_init		(gboolean	 threaded,
					 guint		 block_size,
					 guint		 sample_freq);
typedef struct
{
  glong		timeout;
  gboolean	fds_changed;
  guint		n_fds;
  GslPollFD    *fds;
  gboolean	revents_filled;
} GslEngineLoop;
gboolean      gsl_engine_prepare	(GslEngineLoop		*loop);
gboolean      gsl_engine_check		(const GslEngineLoop	*loop);
void	      gsl_engine_dispatch	(void);


/* --- miscellaneous --- */
void	      gsl_engine_wait_on_trans	(void);
#define	      gsl_engine_block_size()	(/* guint */	gsl_externvar_bsize + 0)
#define	      gsl_engine_last_counter()	(/* guint64 */	gsl_externvar_lcounter + 0)
#define	      gsl_engine_sample_freq()	(/* guint */	gsl_externvar_sample_freq + 0)


/* --- debugging --- */
typedef enum
{
  GSL_ENGINE_DEBUG_NONE         = 0,
  GSL_ENGINE_DEBUG_ENGINE       = (1 << 0),
  GSL_ENGINE_DEBUG_JOBS         = (1 << 1),
  GSL_ENGINE_DEBUG_SCHED        = (1 << 2),
  GSL_ENGINE_DEBUG_MASTER       = (1 << 3),
  GSL_ENGINE_DEBUG_SLAVE        = (1 << 4)
} GslEngineDebugLevel;
void gsl_engine_debug_enable  (GslEngineDebugLevel level);
void gsl_engine_debug_disable (GslEngineDebugLevel level);


/*< private >*/
extern guint	gsl_externvar_bsize;
extern guint	gsl_externvar_sample_freq;
extern guint64	gsl_externvar_lcounter;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_ENGINE_H__ */
