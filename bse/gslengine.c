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


/* --- UserThread --- */
GslModule*
gsl_module_new (const GslClass *klass,
		gpointer        user_data)
{
  OpNode *node;
  guint i;

  g_return_val_if_fail (klass != NULL, NULL);
  g_return_val_if_fail (klass->process != NULL, NULL);
  
  node = gsl_new_struct0 (OpNode, 1);
  
  /* setup GslModule */
  node->module.klass = klass;
  node->module.user_data = user_data;
  node->module.istreams = klass->n_istreams ? gsl_new_struct0 (GslIStream, klass->n_istreams) : NULL;
  node->module.ostreams = _op_alloc_ostreams (klass->n_ostreams);
  
  /* setup OpNode */
  node->inputs = OP_NODE_N_ISTREAMS (node) ? gsl_new_struct0 (OpInput, OP_NODE_N_ISTREAMS (node)) : NULL;
  node->outputs = OP_NODE_N_OSTREAMS (node) ? gsl_new_struct0 (OpOutput, OP_NODE_N_OSTREAMS (node)) : NULL;
  node->onodes = NULL;
  node->mnl_contained = FALSE;
  gsl_rec_mutex_init (&node->rec_mutex);
  for (i = 0; i < OP_NODE_N_OSTREAMS (node); i++)
    node->outputs[i].buffer = node->module.ostreams[i].values;
  
  return &node->module;
}

guint64
gsl_module_counter (GslModule *module)	/* called by any thread */
{
  g_return_val_if_fail (module != NULL, 0);
  
  return OP_NODE (module)->counter;
}

/**
 * gsl_job_integrate
 * @module: The module to integrate
 * @Returns: New job suitable for gsl_trans_add()
 *
 * Create a new transaction job to integrate @module into the engine.
 **/
GslJob*
gsl_job_integrate (GslModule *module)
{
  GslJob *job;
  
  g_return_val_if_fail (module != NULL, NULL);
  
  job = gsl_new_struct0 (GslJob, 1);
  job->job_id = OP_JOB_INTEGRATE;
  job->data.node = OP_NODE (module);
  
  return job;
}

/**
 * gsl_job_discard
 * @module: The module to discard
 * @Returns: New job suitable for gsl_trans_add()
 *
 * Create a new transaction job which remove @module from the
 * engine and destroys it.
 **/
GslJob*
gsl_job_discard (GslModule *module)
{
  GslJob *job;
  
  g_return_val_if_fail (module != NULL, NULL);
  
  job = gsl_new_struct0 (GslJob, 1);
  job->job_id = OP_JOB_DISCARD;
  job->data.node = OP_NODE (module);
  
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
 **/
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
  job->job_id = OP_JOB_CONNECT;
  job->data.connection.dest_node = OP_NODE (dest_module);
  job->data.connection.dest_istream = dest_istream;
  job->data.connection.src_node = OP_NODE (src_module);
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
 **/
GslJob*
gsl_job_disconnect (GslModule *dest_module,
		    guint      dest_istream)
{
  GslJob *job;
  
  g_return_val_if_fail (dest_module != NULL, NULL);
  g_return_val_if_fail (dest_istream < dest_module->klass->n_istreams, NULL);
  
  job = gsl_new_struct0 (GslJob, 1);
  job->job_id = OP_JOB_DISCONNECT;
  job->data.connection.dest_node = OP_NODE (dest_module);
  job->data.connection.dest_istream = dest_istream;
  job->data.connection.src_node = NULL;
  job->data.connection.src_ostream = ~0;
  
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
 **/
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
 **/
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
  job->job_id = OP_JOB_ACCESS;
  job->data.access.node = OP_NODE (module);
  job->data.access.access_func = access_func;
  job->data.access.data = data;
  job->data.access.free_func = free_func;
  
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
 **/
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
 **/
GslJob*
gsl_job_add_poll (GslPollFunc      poll_func,
		  gpointer         data,
		  GslFreeFunc      free_func,
		  guint            n_fds,
		  const GslPollFD *fds)
{
  GslJob *job;
  
  g_return_val_if_fail (poll_func != NULL, NULL);
  if (n_fds)
    g_return_val_if_fail (fds != NULL, NULL);
  
  job = gsl_new_struct0 (GslJob, 1);
  job->job_id = OP_JOB_ADD_POLL;
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
 **/
GslJob*
gsl_job_remove_poll (GslPollFunc poll_func,
		     gpointer    data)
{
  GslJob *job;
  
  g_return_val_if_fail (poll_func != NULL, NULL);
  
  job = gsl_new_struct0 (GslJob, 1);
  job->job_id = OP_JOB_REMOVE_POLL;
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
 **/
GslJob*
gsl_job_debug (const gchar *debug)
{
  GslJob *job;
  
  g_return_val_if_fail (debug != NULL, NULL);
  
  job = gsl_new_struct0 (GslJob, 1);
  job->job_id = OP_JOB_DEBUG;
  job->data.debug = g_strdup (debug);
  
  return job;
}

/**
 * gsl_trans_open
 * @Returns: Newly opened empty transaction
 *
 * Open up a new transaction to commit jobs to the GSL engine.
 **/
GslTrans*
gsl_trans_open (void)
{
  GslTrans *trans = _op_alloc_trans ();
  
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
 **/
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
 **/
void
gsl_trans_commit (GslTrans *trans)
{
  g_return_if_fail (trans != NULL);
  g_return_if_fail (trans->comitted == FALSE);
  g_return_if_fail (trans->cqt_next == NULL);
  
  if (trans->jobs_head)
    {
      trans->comitted = TRUE;
      op_com_enqueue_trans (trans);
      _gsl_com_fire_master_wakeup ();
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
 **/
void
gsl_trans_dismiss (GslTrans *trans)
{
  g_return_if_fail (trans != NULL);
  g_return_if_fail (trans->comitted == FALSE);
  g_return_if_fail (trans->cqt_next == NULL);
  
  _op_free_trans (trans);
}

/**
 * gsl_transact
 * @job: First job
 * @...: %NULL terminated job list
 *
 * Convenience function which openes up a new transaction,
 * collects the %NULL terminated job list passed to the function,
 * and commits the transaction.
 **/
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


/* --- debugging --- */
static volatile GslEngineDebugLevel op_debug_levels = 0;

void
gsl_engine_debug_enable (GslEngineDebugLevel level)
{
  level |= op_debug_levels;
  op_debug_levels = level;
}

void
gsl_engine_debug_disable (GslEngineDebugLevel level)
{
  level = op_debug_levels & ~level;
  op_debug_levels = level;
}

void
_gsl_op_debug (GslEngineDebugLevel lvl,
	       const gchar        *format,
	       ...)
{
  if (lvl & op_debug_levels)
    {
      va_list var_args;
      gchar *s, *l;
      
      switch (lvl)
	{
	case GSL_ENGINE_DEBUG_ENGINE:	l = "ENGINE";	break;
	case GSL_ENGINE_DEBUG_JOBS:	l = "JOBS";	break;
	case GSL_ENGINE_DEBUG_SCHED:	l = "SCHED";	break;
	case GSL_ENGINE_DEBUG_MASTER:	l = "MASTER";	break;
	case GSL_ENGINE_DEBUG_SLAVE:	l = "SLAVE";	break;
	default:			l = "UNKNOWN";	break;
	}
      va_start (var_args, format);
      s = g_strdup_vprintf (format, var_args);
      va_end (var_args);
      g_printerr (G_LOG_DOMAIN ": DEBUG_%s(%p): %s\n", l, gsl_thread_self (), s);
      g_free (s);
    }
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
static gboolean gsl_engine_initialized = FALSE;
static gboolean gsl_engine_threaded = FALSE;
guint		gsl_externvar_bsize = 0;
guint		gsl_externvar_sample_freq = 0;
guint64		gsl_externvar_lcounter = 0;

/**
 * gsl_engine_init
 *
 * Initialize the GSL engine, this function must be called prior to
 * any other engine related function and can only be invoked once.
 **/
void
gsl_engine_init (gboolean run_threaded,
		 guint	  block_size,
		 guint	  sample_freq)
{
  g_return_if_fail (gsl_engine_initialized == FALSE);
  g_return_if_fail (block_size > 0 && block_size <= GSL_STREAM_MAX_VALUES);
  g_return_if_fail (sample_freq > 0);
  
  gsl_engine_initialized = TRUE;
  gsl_engine_threaded = run_threaded;
  gsl_externvar_bsize = block_size;
  gsl_externvar_lcounter = 0;
  gsl_externvar_sample_freq = sample_freq;
  
  OP_DEBUG (GSL_ENGINE_DEBUG_ENGINE, "initialization: threaded=%s", gsl_engine_threaded ? "TRUE" : "FALSE");
  
  if (gsl_engine_threaded)
    {
      GslTrans *trans = gsl_trans_open ();

      _gsl_com_add_master_wakeup (trans);
      gsl_trans_commit (trans);
      gsl_thread_new (_gsl_master_thread, NULL);
      if (0)
	gsl_thread_new (slave, NULL);
    }
}

void
_op_engine_inc_counter (guint64 delta)
{
  volatile guint64 newval;
  
  g_return_if_fail (delta > 0);
  
  newval = gsl_engine_last_counter ();
  newval += delta;
  gsl_externvar_lcounter = newval;
}

gboolean
gsl_engine_prepare (GslEngineLoop *loop)
{
  g_return_val_if_fail (loop != NULL, FALSE);
  g_return_val_if_fail (gsl_engine_initialized == TRUE, FALSE);

  if (!gsl_engine_threaded)
    return _gsl_master_prepare (loop);
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
    return _gsl_master_check (loop);
  else
    return FALSE;
}

void
gsl_engine_dispatch (void)
{
  g_return_if_fail (gsl_engine_initialized == TRUE);

  if (!gsl_engine_threaded)
    _gsl_master_dispatch ();
}

void
gsl_engine_wait_on_trans (void)
{
  g_return_if_fail (gsl_engine_initialized == TRUE);
  
  /* non-threaded */
  if (!gsl_engine_threaded)
    _gsl_master_dispatch_jobs ();

  /* threaded */
  op_com_wait_on_trans ();
  
  /* call all free() functions */
  _op_collect_trans ();
}

/* vim:set ts=8 sts=2 sw=2: */
