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
#include "gslengine.h"

#include "gslcommon.h"
#include "gslopnode.h"
#include "gslopmaster.h"


/* --- prototypes --- */
static void wakeup_master (void);


/* --- UserThread --- */
GslModule*
gsl_module_new (const GslClass *klass,
		gpointer        user_data)
{
  EngineNode *node;
  guint i;

  g_return_val_if_fail (klass != NULL, NULL);
  g_return_val_if_fail (klass->process != NULL || klass->process_defer != NULL, NULL);
  if (klass->process_defer)
    {
      g_warning ("%s: Delay cycle processing not yet implemented", G_STRLOC);
      return NULL;
    }
  
  node = gsl_new_struct0 (EngineNode, 1);
  
  /* setup GslModule */
  node->module.klass = klass;
  node->module.user_data = user_data;
  node->module.istreams = klass->n_istreams ? gsl_new_struct0 (GslIStream, ENGINE_NODE_N_ISTREAMS (node)) : NULL;
  node->module.jstreams = klass->n_jstreams ? gsl_new_struct0 (GslJStream, ENGINE_NODE_N_JSTREAMS (node)) : NULL;
  node->module.ostreams = _engine_alloc_ostreams (ENGINE_NODE_N_OSTREAMS (node));
  
  /* setup EngineNode */
  node->inputs = ENGINE_NODE_N_ISTREAMS (node) ? gsl_new_struct0 (EngineInput, ENGINE_NODE_N_ISTREAMS (node)) : NULL;
  node->jinputs = ENGINE_NODE_N_JSTREAMS (node) ? gsl_new_struct0 (EngineJInput*, ENGINE_NODE_N_JSTREAMS (node)) : NULL;
  node->outputs = ENGINE_NODE_N_OSTREAMS (node) ? gsl_new_struct0 (EngineOutput, ENGINE_NODE_N_OSTREAMS (node)) : NULL;
  node->output_nodes = NULL;
  node->integrated = FALSE;
  gsl_rec_mutex_init (&node->rec_mutex);
  for (i = 0; i < ENGINE_NODE_N_OSTREAMS (node); i++)
    {
      node->outputs[i].buffer = node->module.ostreams[i].values;
      node->module.ostreams[i].sub_sample_pattern = gsl_engine_sub_sample_test (node->module.ostreams[i].values);
    }
  node->flow_jobs = NULL;
  node->fjob_first = NULL;
  node->fjob_last = NULL;
  
  return &node->module;
}

/**
 * gsl_module_tick_stamp
 * @module:  a GSL engine module
 * @RETURNS: the module's tick stamp, indicating its process status
 *
 * Any thread may call this function on a valid engine module.
 * The module specific tick stamp is updated to gsl_tick_stamp() +
 * @n_values every time its GslProcessFunc() function was
 * called. See also gsl_tick_stamp().
 */
guint64
gsl_module_tick_stamp (GslModule *module)
{
  g_return_val_if_fail (module != NULL, 0);
  
  return ENGINE_NODE (module)->counter;
}

/**
 * gsl_job_integrate
 * @module: The module to integrate
 * @Returns: New job suitable for gsl_trans_add()
 *
 * Create a new transaction job to integrate @module into the engine.
 */
GslJob*
gsl_job_integrate (GslModule *module)
{
  GslJob *job;
  
  g_return_val_if_fail (module != NULL, NULL);
  
  job = gsl_new_struct0 (GslJob, 1);
  job->job_id = ENGINE_JOB_INTEGRATE;
  job->data.node = ENGINE_NODE (module);
  
  return job;
}

/**
 * gsl_job_discard
 * @module: The module to discard
 * @Returns: New job suitable for gsl_trans_add()
 *
 * Create a new transaction job which removes @module from the
 * engine and destroys it.
 */
GslJob*
gsl_job_discard (GslModule *module)
{
  GslJob *job;
  
  g_return_val_if_fail (module != NULL, NULL);
  
  job = gsl_new_struct0 (GslJob, 1);
  job->job_id = ENGINE_JOB_DISCARD;
  job->data.node = ENGINE_NODE (module);
  
  return job;
}

/**
 * gsl_job_connect
 * @src_module: Module with output stream
 * @src_ostream: Index of output stream of @src_module
 * @dest_module: Module with unconnected input stream
 * @dest_istream: Index of input stream of @dest_module
 * @Returns: New job suitable for gsl_trans_add()
 *
 * Create a new transaction job which connects the ouput stream @src_ostream
 * of module @src_module to the input stream @dest_istream of module @dest_module
 * (it is an error if the input stream is already connected by the time the job
 * is executed).
 */
GslJob*
gsl_job_connect (GslModule *src_module,
		 guint      src_ostream,
		 GslModule *dest_module,
		 guint      dest_istream)
{
  GslJob *job;
  
  g_return_val_if_fail (src_module != NULL, NULL);
  g_return_val_if_fail (src_ostream < src_module->klass->n_ostreams, NULL);
  g_return_val_if_fail (dest_module != NULL, NULL);
  g_return_val_if_fail (dest_istream < dest_module->klass->n_istreams, NULL);
  
  job = gsl_new_struct0 (GslJob, 1);
  job->job_id = ENGINE_JOB_ICONNECT;
  job->data.connection.dest_node = ENGINE_NODE (dest_module);
  job->data.connection.dest_ijstream = dest_istream;
  job->data.connection.src_node = ENGINE_NODE (src_module);
  job->data.connection.src_ostream = src_ostream;
  
  return job;
}

GslJob*
gsl_job_jconnect (GslModule *src_module,
		  guint      src_ostream,
		  GslModule *dest_module,
		  guint      dest_jstream)
{
  GslJob *job;
  
  g_return_val_if_fail (src_module != NULL, NULL);
  g_return_val_if_fail (src_ostream < src_module->klass->n_ostreams, NULL);
  g_return_val_if_fail (dest_module != NULL, NULL);
  g_return_val_if_fail (dest_jstream < dest_module->klass->n_jstreams, NULL);
  
  job = gsl_new_struct0 (GslJob, 1);
  job->job_id = ENGINE_JOB_JCONNECT;
  job->data.connection.dest_node = ENGINE_NODE (dest_module);
  job->data.connection.dest_ijstream = dest_jstream;
  job->data.connection.src_node = ENGINE_NODE (src_module);
  job->data.connection.src_ostream = src_ostream;
  
  return job;
}

/**
 * gsl_job_disconnect
 * @dest_module: Module with connected input stream
 * @dest_istream: Index of input stream of @dest_module
 * @Returns: New job suitable for gsl_trans_add()
 *
 * Create a new transaction job which causes the input stream @dest_istream
 * of @dest_module to be disconnected (it is an error if the input stream isn't
 * connected by the time the job is executed).
 */
GslJob*
gsl_job_disconnect (GslModule *dest_module,
		    guint      dest_istream)
{
  GslJob *job;
  
  g_return_val_if_fail (dest_module != NULL, NULL);
  g_return_val_if_fail (dest_istream < dest_module->klass->n_istreams, NULL);
  
  job = gsl_new_struct0 (GslJob, 1);
  job->job_id = ENGINE_JOB_IDISCONNECT;
  job->data.connection.dest_node = ENGINE_NODE (dest_module);
  job->data.connection.dest_ijstream = dest_istream;
  job->data.connection.src_node = NULL;
  job->data.connection.src_ostream = ~0;
  
  return job;
}

GslJob*
gsl_job_jdisconnect (GslModule *dest_module,
		     guint      dest_jstream,
		     GslModule *src_module,
		     guint	src_ostream)
{
  GslJob *job;
  
  g_return_val_if_fail (dest_module != NULL, NULL);
  g_return_val_if_fail (dest_jstream < dest_module->klass->n_jstreams, NULL);
  g_return_val_if_fail (src_module != NULL, NULL);
  g_return_val_if_fail (src_ostream < src_module->klass->n_ostreams, NULL);
  
  job = gsl_new_struct0 (GslJob, 1);
  job->job_id = ENGINE_JOB_JDISCONNECT;
  job->data.connection.dest_node = ENGINE_NODE (dest_module);
  job->data.connection.dest_ijstream = dest_jstream;
  job->data.connection.src_node = ENGINE_NODE (src_module);
  job->data.connection.src_ostream = src_ostream;
  
  return job;
}

GslJob*
gsl_job_set_consumer (GslModule *module,
		      gboolean   is_toplevel_consumer)
{
  GslJob *job;
  
  g_return_val_if_fail (module != NULL, NULL);
  
  job = gsl_new_struct0 (GslJob, 1);
  job->job_id = is_toplevel_consumer ? ENGINE_JOB_SET_CONSUMER : ENGINE_JOB_UNSET_CONSUMER;
  job->data.node = ENGINE_NODE (module);
  
  return job;
}

/**
 * GslAccessFunc
 * @module:	Module to operate on
 * @data:	Accessor data
 *
 * The GslAccessFunc is a user supplied callback function which can access
 * a module in times it is not processing. Accessors are usually used to
 * either read out a module's current state, or to modify its state. An
 * accessor may only operate on the @data and the @module passed
 * in to it.
 */
/**
 * gsl_job_access
 * @module: The module to access
 * @access_func: The accessor function
 * @data: Data passed in to the accessor
 * @free_func: Function to free @data
 * @Returns: New job suitable for gsl_trans_add()
 *
 * Create a new transaction job which will invoke @access_func 
 * on @module with @data when the transaction queue is processed
 * to modify the module's state.
 */
GslJob*
gsl_job_access (GslModule    *module,
		GslAccessFunc access_func,
		gpointer      data,
		GslFreeFunc   free_func)
{
  GslJob *job;
  
  g_return_val_if_fail (module != NULL, NULL);
  g_return_val_if_fail (access_func != NULL, NULL);
  
  job = gsl_new_struct0 (GslJob, 1);
  job->job_id = ENGINE_JOB_ACCESS;
  job->data.access.node = ENGINE_NODE (module);
  job->data.access.access_func = access_func;
  job->data.access.data = data;
  job->data.access.free_func = free_func;
  
  return job;
}

/**
 * gsl_flow_job_access
 */
GslJob*
gsl_flow_job_access (GslModule    *module,
		     guint64       tick_stamp,
		     GslAccessFunc access_func,
		     gpointer      data,
		     GslFreeFunc   free_func)
{
  GslJob *job;
  EngineFlowJob *fjob;

  g_return_val_if_fail (module != NULL, NULL);
  g_return_val_if_fail (access_func != NULL, NULL);
  
  fjob = (EngineFlowJob*) gsl_new_struct0 (EngineFlowJobAccess, 1);
  fjob->fjob_id = ENGINE_FLOW_JOB_ACCESS;
  fjob->any.tick_stamp = tick_stamp;
  fjob->access.access_func = access_func;
  fjob->access.data = data;
  fjob->access.free_func = free_func;

  job = gsl_new_struct0 (GslJob, 1);
  job->job_id = ENGINE_JOB_FLOW_JOB;
  job->data.flow_job.node = ENGINE_NODE (module);
  job->data.flow_job.fjob = fjob;
  
  return job;
}

GslJob*
gsl_job_suspend (GslModule *module)
{
  GslJob *job;

  g_return_val_if_fail (module != NULL, NULL);
  g_return_val_if_fail (ENGINE_MODULE_IS_VIRTUAL (module) == FALSE, NULL);
  
  job = gsl_new_struct0 (GslJob, 1);
  job->job_id = ENGINE_JOB_SUSPEND;
  job->data.node = ENGINE_NODE (module);

  return job;
}

GslJob*
gsl_flow_job_resume (GslModule *module,
		     guint64    tick_stamp)
{
  GslJob *job;
  EngineFlowJob *fjob;
  
  g_return_val_if_fail (module != NULL, NULL);
  g_return_val_if_fail (ENGINE_MODULE_IS_VIRTUAL (module) == FALSE, NULL);
  
  fjob = (EngineFlowJob*) gsl_new_struct0 (EngineFlowJobAny, 1);
  fjob->fjob_id = ENGINE_FLOW_JOB_RESUME;
  fjob->any.tick_stamp = tick_stamp;
  
  job = gsl_new_struct0 (GslJob, 1);
  job->job_id = ENGINE_JOB_FLOW_JOB;
  job->data.flow_job.node = ENGINE_NODE (module);
  job->data.flow_job.fjob = fjob;
  
  return job;
}

/**
 * GslPollFunc
 * @data: Data of poll function
 * @n_values: Minimum number of values the engine wants to process
 * @timeout_p: Location of timeout value
 * @n_fds: Number of file descriptors used for polling
 * @fds: File descriptors to be used for polling
 * @revents_filled: Indicates whether @fds actually have their ->revents field filled with valid data.
 * @Returns: A boolean value indicating whether the engine should process data right now
 *
 * The GslPollFunc is a user supplied callback function which can be hooked into the
 * GSL engine. The engine uses the poll functions to determine whether processing of
 * @n_values in its module network is necessary.
 * In order for the poll functions to react to extern events, such as device driver
 * status changes, the engine will poll(2) the @fds of the poll function and invoke
 * the callback with @revents_filled==%TRUE if any of its @fds changed state.
 * The callback may also be invoked at other random times with @revents_filled=%FALSE.
 * It is supposed to return %TRUE if network processing is currently necessary, and
 * %FALSE if not.
 * If %FALSE is returned, @timeout_p may be filled with the number of milliseconds
 * the engine should use for polling at maximum.
 */
/**
 * gsl_job_add_poll
 * @poll_func: Poll function to add
 * @data: Data of poll function
 * @free_func: Function to free @data
 * @n_fds: Number of poll file descriptors
 * @fds: File descriptors to select(2) or poll(2) on
 * @Returns: New job suitable for gsl_trans_add()
 *
 * Create a new transaction job which adds a poll function
 * to the engine. The poll function is used by the engine to
 * determine whether processing is currently necessary.
 */
GslJob*
gsl_job_add_poll (GslPollFunc    poll_func,
		  gpointer       data,
		  GslFreeFunc    free_func,
		  guint          n_fds,
		  const GPollFD *fds)
{
  GslJob *job;
  
  g_return_val_if_fail (poll_func != NULL, NULL);
  if (n_fds)
    g_return_val_if_fail (fds != NULL, NULL);
  
  job = gsl_new_struct0 (GslJob, 1);
  job->job_id = ENGINE_JOB_ADD_POLL;
  job->data.poll.poll_func = poll_func;
  job->data.poll.data = data;
  job->data.poll.n_fds = n_fds;
  job->data.poll.fds = g_memdup (fds, sizeof (fds[0]) * n_fds);
  
  return job;
}

/**
 * gsl_job_remove_poll
 * @poll_func: Poll function to remove
 * @data: Data of poll function
 * @Returns: New job suitable for gsl_trans_add()
 *
 * Create a new transaction job which removes a previously inserted poll
 * function from the engine.
 */
GslJob*
gsl_job_remove_poll (GslPollFunc poll_func,
		     gpointer    data)
{
  GslJob *job;
  
  g_return_val_if_fail (poll_func != NULL, NULL);
  
  job = gsl_new_struct0 (GslJob, 1);
  job->job_id = ENGINE_JOB_REMOVE_POLL;
  job->data.poll.poll_func = poll_func;
  job->data.poll.data = data;
  job->data.poll.free_func = NULL;
  job->data.poll.fds = NULL;
  
  return job;
}

/**
 * gsl_job_debug
 * @debug: Debug message
 * @Returns: New job suitable for gsl_trans_add()
 *
 * Create a new transaction job which issues @debug message when
 * the job is executed. This function is meant for debugging purposes
 * during development phase only and shouldn't be used in production code.
 */
GslJob*
gsl_job_debug (const gchar *debug)
{
  GslJob *job;
  
  g_return_val_if_fail (debug != NULL, NULL);
  
  job = gsl_new_struct0 (GslJob, 1);
  job->job_id = ENGINE_JOB_DEBUG;
  job->data.debug = g_strdup (debug);
  
  return job;
}

/**
 * gsl_trans_open
 * @Returns: Newly opened empty transaction
 *
 * Open up a new transaction to commit jobs to the GSL engine.
 * This function may cause garbage collection (see
 * gsl_engine_garbage_collect()).
 */
GslTrans*
gsl_trans_open (void)
{
  GslTrans *trans;

  gsl_engine_garbage_collect ();

  trans = gsl_new_struct0 (GslTrans, 1);
  
  trans->jobs_head = NULL;
  trans->jobs_tail = NULL;
  trans->comitted = FALSE;
  trans->cqt_next = NULL;
  
  return trans;
}

/**
 * gsl_trans_add
 * @trans: Opened transaction
 * @job: Job to add
 *
 * Append a job to an opened transaction.
 */
void
gsl_trans_add (GslTrans *trans,
	       GslJob   *job)
{
  g_return_if_fail (trans != NULL);
  g_return_if_fail (trans->comitted == FALSE);
  g_return_if_fail (job != NULL);
  g_return_if_fail (job->next == NULL);
  
  if (trans->jobs_tail)
    trans->jobs_tail->next = job;
  else
    trans->jobs_head = job;
  trans->jobs_tail = job;
}

/**
 * gsl_trans_commit
 * @trans: Opened transaction
 *
 * Close the transaction and commit it to the engine. The engine
 * will execute the jobs contained in this transaction as soon as
 * it has completed its current processing cycle. The jobs will be
 * executed in the exact order they were added to the transaction.
 */
void
gsl_trans_commit (GslTrans *trans)
{
  g_return_if_fail (trans != NULL);
  g_return_if_fail (trans->comitted == FALSE);
  g_return_if_fail (trans->cqt_next == NULL);
  
  if (trans->jobs_head)
    {
      trans->comitted = TRUE;
      _engine_enqueue_trans (trans);
      wakeup_master ();
    }
  else
    gsl_trans_dismiss (trans);
}

/**
 * gsl_trans_dismiss
 * @trans: Opened transaction
 *
 * Close and discard the transaction, destroy all jobs currently
 * contained in it and do not execute them.
 * This function may cause garbage collection (see
 * gsl_engine_garbage_collect()).
 */
void
gsl_trans_dismiss (GslTrans *trans)
{
  g_return_if_fail (trans != NULL);
  g_return_if_fail (trans->comitted == FALSE);
  g_return_if_fail (trans->cqt_next == NULL);
  
  _engine_free_trans (trans);

  gsl_engine_garbage_collect ();
}

/**
 * gsl_transact
 * @job: First job
 * @...: %NULL terminated job list
 *
 * Convenience function which openes up a new transaction,
 * collects the %NULL terminated job list passed to the function,
 * and commits the transaction.
 */
void
gsl_transact (GslJob *job,
	      ...)
{
  GslTrans *trans = gsl_trans_open ();
  va_list var_args;
  
  va_start (var_args, job);
  while (job)
    {
      gsl_trans_add (trans, job);
      job = va_arg (var_args, GslJob*);
    }
  va_end (var_args);
  gsl_trans_commit (trans);
}


/* --- Virtual Modules --- */
static void
virtual_module_process (GslModule *module,
			guint      n_values)
{
  guint i;

  /* dumb pass-through task (FIXME: virtualization works without _process()) */
  for (i = 0; i < GSL_MODULE_N_OSTREAMS (module); i++)
    if (module->ostreams[i].connected)
      module->ostreams[i].values = (gfloat*) module->istreams[i].values;
}

static void
virtual_module_free (gpointer        data,
		     const GslClass *klass)
{
  g_free ((gpointer) klass);
}

GslModule*
gsl_module_new_virtual (guint n_iostreams)
{
  GslClass virtual_module_class = {
    0,				/* n_istreams */
    0,				/* n_jstreams */
    0,				/* n_ostreams */
    virtual_module_process,	/* process */
    NULL,			/* process_defer */
    NULL,			/* reset */
    virtual_module_free,	/* free */
    GSL_COST_CHEAP
  };
  GslClass *klass;
  GslModule *module;

  g_return_val_if_fail (n_iostreams > 0, NULL);

  klass = g_memdup (&virtual_module_class, sizeof (virtual_module_class));
  klass->n_istreams = n_iostreams;
  klass->n_ostreams = n_iostreams;
  module = gsl_module_new (klass, NULL);	/* FIXME: support virtual modules */
  ENGINE_NODE (module)->virtual_node = TRUE;

  return module;
}


/* --- initialization --- */
static void
slave (gpointer data)
{
  gboolean run = TRUE;
  
  while (run)
    {
      GslTrans *trans = gsl_trans_open ();
      gchar *str = g_strdup_printf ("SLAVE(%p): idle", g_thread_self ());
      
      gsl_trans_add (trans, gsl_job_debug (str));
      g_free (str);
      gsl_trans_add (trans, gsl_job_debug ("string2"));
      gsl_trans_commit (trans);
      
      trans = gsl_trans_open ();
      gsl_trans_add (trans, gsl_job_debug ("trans2"));
      gsl_trans_commit (trans);
      
      g_usleep (1000*500);
    }
}

/* --- setup & trigger --- */
static gboolean   gsl_engine_initialized = FALSE;
static gboolean   gsl_engine_threaded = FALSE;
static GslThread *master_thread = NULL;
guint		gsl_externvar_bsize = 0;
guint		gsl_externvar_sample_freq = 0;
guint		gsl_externvar_sub_sample_mask = 0;
guint		gsl_externvar_sub_sample_steps = 0;

/**
 * gsl_engine_init
 * @block_size: number of values to process block wise
 *
 * Initialize the GSL engine, this function must be called prior to
 * any other engine related function and can only be invoked once.
 * The @block_size determines the amount by which the global tick
 * stamp (see gsl_tick_stamp()) is updated everytime the whole
 * module network completed processing @block_size values.
 */
void
gsl_engine_init (gboolean run_threaded,
		 guint	  block_size,
		 guint	  sample_freq,
		 guint    sub_sample_mask)
{
  g_return_if_fail (gsl_engine_initialized == FALSE);
  g_return_if_fail (block_size > 0 && block_size <= GSL_STREAM_MAX_VALUES);
  g_return_if_fail (sample_freq > 0);
  g_return_if_fail (sub_sample_mask < block_size);
  g_return_if_fail ((sub_sample_mask & (sub_sample_mask + 1)) == 0);	/* power of 2 */
  
  gsl_engine_initialized = TRUE;
  gsl_engine_threaded = run_threaded;
  gsl_externvar_bsize = block_size;
  gsl_externvar_sample_freq = sample_freq;
  gsl_externvar_sub_sample_mask = sub_sample_mask << 2;	/* shift out sizeof (float) alignment */
  gsl_externvar_sub_sample_steps = sub_sample_mask + 1;
  _gsl_tick_stamp_set_leap (block_size);
  
  ENG_DEBUG ("initialization: threaded=%s", gsl_engine_threaded ? "TRUE" : "FALSE");
  
  if (gsl_engine_threaded)
    {
      master_thread = gsl_thread_new (_engine_master_thread, NULL);
      if (0)
	gsl_thread_new (slave, NULL);
    }
}

static void
wakeup_master (void)
{
  if (master_thread)
    gsl_thread_wakeup (master_thread);
}

gboolean
gsl_engine_prepare (GslEngineLoop *loop)
{
  g_return_val_if_fail (loop != NULL, FALSE);
  g_return_val_if_fail (gsl_engine_initialized == TRUE, FALSE);

  if (!gsl_engine_threaded)
    return _engine_master_prepare (loop);
  else
    {
      loop->timeout = -1;
      loop->fds_changed = FALSE;
      loop->n_fds = 0;
      loop->revents_filled = FALSE;
      return FALSE;
    }
}

gboolean
gsl_engine_check (const GslEngineLoop *loop)
{
  g_return_val_if_fail (loop != NULL, FALSE);
  if (loop->n_fds)
    g_return_val_if_fail (loop->revents_filled == TRUE, FALSE);
  
  if (!gsl_engine_threaded)
    return _engine_master_check (loop);
  else
    return FALSE;
}

void
gsl_engine_dispatch (void)
{
  g_return_if_fail (gsl_engine_initialized == TRUE);

  if (!gsl_engine_threaded)
    _engine_master_dispatch ();
}

/**
 * gsl_engine_wait_on_trans
 *
 * Wait until all pending transactions have been processed
 * by the GSL Engine.
 * This function may cause garbage collection (see
 * gsl_engine_garbage_collect()).
 */
void
gsl_engine_wait_on_trans (void)
{
  g_return_if_fail (gsl_engine_initialized == TRUE);
  
  /* non-threaded */
  if (!gsl_engine_threaded)
    _engine_master_dispatch_jobs ();

  /* threaded */
  _engine_wait_on_trans ();
  
  /* call all free() functions */
  gsl_engine_garbage_collect ();
}

/* vim:set ts=8 sts=2 sw=2: */
