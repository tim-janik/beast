/* BSE Engine - Flow module operation engine
 * Copyright (C) 2001, 2002, 2003, 2004 Tim Janik
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
#include "bseengine.h"
#include "gslcommon.h"
#include "bseengineutils.h"
#include "bseenginemaster.h"
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>


#define DEBUG(...)      sfi_debug ("engine", __VA_ARGS__)

/* some systems don't have ERESTART (which is what linux returns for system
 * calls on pipes which are being interrupted). most probably just use EINTR,
 * and maybe some can return both. so we check for both in the below code,
 * and alias ERESTART to EINTR if it's not present.
 */
#ifndef ERESTART
#define ERESTART        EINTR
#endif


/* --- prototypes --- */
static void wakeup_master (void);


/* --- UserThread --- */
/**
 * bse_module_new
 * @klass:     the BseModuleClass which determines the module's behaviour
 * @user_data: user data pointer
 * @RETURNS:   a newly created module
 *
 * Create a new module with methods specified in @klass and
 * a user_data field set to @user_data. The returned module
 * can then be integrated into the engine with bse_job_integrate().
 * This function is MT-safe and may be called from any thread.
 */
BseModule*
bse_module_new (const BseModuleClass *klass,
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
  
  node = sfi_new_struct0 (EngineNode, 1);
  
  /* setup BseModule */
  node->module.klass = klass;
  node->module.user_data = user_data;
  node->module.istreams = klass->n_istreams ? sfi_new_struct0 (BseIStream, ENGINE_NODE_N_ISTREAMS (node)) : NULL;
  node->module.jstreams = klass->n_jstreams ? sfi_new_struct0 (BseJStream, ENGINE_NODE_N_JSTREAMS (node)) : NULL;
  node->module.ostreams = _engine_alloc_ostreams (ENGINE_NODE_N_OSTREAMS (node));
  
  /* setup EngineNode */
  node->inputs = ENGINE_NODE_N_ISTREAMS (node) ? sfi_new_struct0 (EngineInput, ENGINE_NODE_N_ISTREAMS (node)) : NULL;
  node->jinputs = ENGINE_NODE_N_JSTREAMS (node) ? sfi_new_struct0 (EngineJInput*, ENGINE_NODE_N_JSTREAMS (node)) : NULL;
  node->outputs = ENGINE_NODE_N_OSTREAMS (node) ? sfi_new_struct0 (EngineOutput, ENGINE_NODE_N_OSTREAMS (node)) : NULL;
  node->output_nodes = NULL;
  node->integrated = FALSE;
  sfi_rec_mutex_init (&node->rec_mutex);
  for (i = 0; i < ENGINE_NODE_N_OSTREAMS (node); i++)
    node->outputs[i].buffer = node->module.ostreams[i].values;
  node->flow_jobs = NULL;
  node->boundary_jobs = NULL;
  node->probe_jobs = NULL;
  node->ujob_first = NULL;
  node->ujob_last = NULL;
  
  return &node->module;
}

/**
 * bse_module_tick_stamp
 * @module:  a BSE Engine Module
 * @RETURNS: the module's tick stamp, indicating its process status
 *
 * Any thread may call this function on a valid engine module.
 * The module specific tick stamp is updated to gsl_tick_stamp() +
 * @n_values every time its BseProcessFunc() function was
 * called. See also gsl_tick_stamp().
 * This function is MT-safe and may be called from any thread.
 */
guint64
bse_module_tick_stamp (BseModule *module)
{
  g_return_val_if_fail (module != NULL, 0);
  
  return ENGINE_NODE (module)->counter;
}

/**
 * bse_module_has_source
 * @module:  a BSE Engine Module
 * @istream: Index of input stream
 * @RETURNS: whether the module has a possible input
 *
 * Check whether @istream may be disconnected via
 * bse_job_disconnect(). This is not an indication for whether
 * BSE_MODULE_ISTREAM (@module, @istream).connected will be %TRUE
 * during process(), as the source may be a dangling virtual module,
 * resulting in BSE_MODULE_ISTREAM (@module, @istream).connected
 * being %FALSE.
 * See also bse_module_new_virtual().
 * This function is MT-safe and may be called from any thread.
 */
gboolean
bse_module_has_source (BseModule *module,
                       guint      istream)
{
  g_return_val_if_fail (module != NULL, FALSE);
  g_return_val_if_fail (istream < module->klass->n_istreams, FALSE);
  
  return ENGINE_NODE (module)->inputs[istream].src_node != NULL;
}

/**
 * bse_job_integrate
 * @module: The module to integrate
 * @Returns: New job suitable for bse_trans_add()
 *
 * Create a new transaction job to integrate @module into the engine.
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_integrate (BseModule *module)
{
  BseJob *job;
  
  g_return_val_if_fail (module != NULL, NULL);
  
  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_INTEGRATE;
  job->data.node = ENGINE_NODE (module);
  
  return job;
}

/**
 * bse_job_discard
 * @module: The module to discard
 * @Returns: New job suitable for bse_trans_add()
 *
 * Create a new transaction job which removes @module from the
 * engine and destroys it.
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_discard (BseModule *module)
{
  BseJob *job;
  
  g_return_val_if_fail (module != NULL, NULL);
  
  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_DISCARD;
  job->data.node = ENGINE_NODE (module);
  
  return job;
}

/**
 * bse_job_kill_inputs
 * @module: Module with input streams
 * @Returns: New job suitable for bse_trans_add()
 *
 * Create a new transaction job which causes all connected input streams
 * of @module to be disconnected, like it's done upon discarding the module.
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_kill_inputs (BseModule *module)
{
  BseJob *job;
  
  g_return_val_if_fail (module != NULL, NULL);
  
  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_KILL_INPUTS;
  job->data.node = ENGINE_NODE (module);
  
  return job;
}

/**
 * bse_job_kill_outputs
 * @module: Module with output streams
 * @Returns: New job suitable for bse_trans_add()
 *
 * Create a new transaction job which causes all connected output streams
 * of @module to be disconnected, like it's done upon discarding the module.
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_kill_outputs (BseModule *module)
{
  BseJob *job;
  
  g_return_val_if_fail (module != NULL, NULL);
  
  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_KILL_OUTPUTS;
  job->data.node = ENGINE_NODE (module);
  
  return job;
}

/**
 * bse_job_connect
 * @src_module: Module with output stream
 * @src_ostream: Index of output stream of @src_module
 * @dest_module: Module with unconnected input stream
 * @dest_istream: Index of input stream of @dest_module
 * @Returns: New job suitable for bse_trans_add()
 *
 * Create a new transaction job which connects the ouput stream @src_ostream
 * of module @src_module to the input stream @dest_istream of module @dest_module
 * (it is an error if the input stream is already connected by the time the job
 * is executed).
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_connect (BseModule *src_module,
		 guint      src_ostream,
		 BseModule *dest_module,
		 guint      dest_istream)
{
  BseJob *job;
  
  g_return_val_if_fail (src_module != NULL, NULL);
  g_return_val_if_fail (src_ostream < src_module->klass->n_ostreams, NULL);
  g_return_val_if_fail (dest_module != NULL, NULL);
  g_return_val_if_fail (dest_istream < dest_module->klass->n_istreams, NULL);
  
  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_ICONNECT;
  job->data.connection.dest_node = ENGINE_NODE (dest_module);
  job->data.connection.dest_ijstream = dest_istream;
  job->data.connection.src_node = ENGINE_NODE (src_module);
  job->data.connection.src_ostream = src_ostream;
  
  return job;
}

/**
 * bse_job_jconnect
 * @src_module: Module with output stream
 * @src_ostream: Index of output stream of @src_module
 * @dest_module: Module with unconnected joint input stream
 * @dest_jstream: Index of joint input stream of @dest_module
 * @Returns: New job suitable for bse_trans_add()
 *
 * Create a new transaction job which connects the ouput stream @src_ostream
 * of module @src_module to the joint input stream @dest_istream of module
 * @dest_module.
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_jconnect (BseModule *src_module,
		  guint      src_ostream,
		  BseModule *dest_module,
		  guint      dest_jstream)
{
  BseJob *job;
  
  g_return_val_if_fail (src_module != NULL, NULL);
  g_return_val_if_fail (src_ostream < src_module->klass->n_ostreams, NULL);
  g_return_val_if_fail (dest_module != NULL, NULL);
  g_return_val_if_fail (dest_jstream < dest_module->klass->n_jstreams, NULL);
  
  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_JCONNECT;
  job->data.connection.dest_node = ENGINE_NODE (dest_module);
  job->data.connection.dest_ijstream = dest_jstream;
  job->data.connection.src_node = ENGINE_NODE (src_module);
  job->data.connection.src_ostream = src_ostream;
  
  return job;
}

/**
 * bse_job_disconnect
 * @dest_module: Module with connected input stream
 * @dest_istream: Index of input stream of @dest_module
 * @Returns: New job suitable for bse_trans_add()
 *
 * Create a new transaction job which causes the input stream @dest_istream
 * of @dest_module to be disconnected (it is an error if the input stream isn't
 * connected by the time the job is executed).
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_disconnect (BseModule *dest_module,
		    guint      dest_istream)
{
  BseJob *job;
  
  g_return_val_if_fail (dest_module != NULL, NULL);
  g_return_val_if_fail (dest_istream < dest_module->klass->n_istreams, NULL);
  
  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_IDISCONNECT;
  job->data.connection.dest_node = ENGINE_NODE (dest_module);
  job->data.connection.dest_ijstream = dest_istream;
  job->data.connection.src_node = NULL;
  job->data.connection.src_ostream = ~0;
  
  return job;
}

/**
 * bse_job_jdisconnect
 * @dest_module: Module with connected input stream
 * @dest_jstream: Index of input stream of @dest_module
 * @src_module: Module with output stream
 * @src_ostream: Index of output stream of @src_module
 * @Returns: New job suitable for bse_trans_add()
 *
 * Create a new transaction job which causes the joint input
 * stream @dest_jstream of @dest_module to be disconnected from
 * the output stream @src_ostream of @src_module (it is an
 * error if this connection isn't established by the time the
 * job is executed). Beware, the order of @dest_module and
 * @src_module is different from bse_job_jconnect().
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_jdisconnect (BseModule *dest_module,
		     guint      dest_jstream,
		     BseModule *src_module,
		     guint	src_ostream)
{
  BseJob *job;
  
  g_return_val_if_fail (dest_module != NULL, NULL);
  g_return_val_if_fail (dest_jstream < dest_module->klass->n_jstreams, NULL);
  g_return_val_if_fail (src_module != NULL, NULL);
  g_return_val_if_fail (src_ostream < src_module->klass->n_ostreams, NULL);
  
  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_JDISCONNECT;
  job->data.connection.dest_node = ENGINE_NODE (dest_module);
  job->data.connection.dest_ijstream = dest_jstream;
  job->data.connection.src_node = ENGINE_NODE (src_module);
  job->data.connection.src_ostream = src_ostream;
  
  return job;
}

BseJob*
bse_job_set_consumer (BseModule *module,
		      gboolean   is_toplevel_consumer)
{
  BseJob *job;
  
  g_return_val_if_fail (module != NULL, NULL);
  g_return_val_if_fail (ENGINE_MODULE_IS_VIRTUAL (module) == FALSE, NULL);
  
  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = is_toplevel_consumer ? ENGINE_JOB_SET_CONSUMER : ENGINE_JOB_UNSET_CONSUMER;
  job->data.node = ENGINE_NODE (module);
  
  return job;
}

/**
 * bse_job_force_reset
 * @module: The module to be reset
 * @Returns: New job suitable for bse_trans_add()
 *
 * Forces a reset of @module before its next call to
 * process(), if its class provides a reset()
 * implementation. This is usually
 * not a good idea, as forcing an immediate reset can
 * lead to multiple unnecessary reset() invocations.
 * The logic used to invoke reset() automatically is
 * usually good enough to cover all required cases.
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_force_reset (BseModule *module)
{
  BseJob *job;
  
  g_return_val_if_fail (module != NULL, NULL);
  
  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_FORCE_RESET;
  job->data.node = ENGINE_NODE (module);
  
  return job;
}

/**
 * BseEngineAccessFunc
 * @module:	Module to operate on
 * @data:	Accessor data
 *
 * The BseEngineAccessFunc is a user supplied callback function which can access
 * a module in times it is not processing. Accessors are usually used to
 * either read out a module's current state, or to modify its state. An
 * accessor may only operate on the @data and the @module passed
 * in to it.
 */
/**
 * bse_job_access
 * @module: The module to access
 * @access_func: The accessor function (executed in master thread)
 * @data: Data passed in to the accessor
 * @free_func: Function to free @data (executed in user thread)
 * @Returns: New job suitable for bse_trans_add()
 *
 * Create a new transaction job which will invoke @access_func 
 * on @module with @data when the transaction queue is processed
 * to modify the module's state.
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_access (BseModule    *module,
		BseEngineAccessFunc access_func,
		gpointer      data,
		BseFreeFunc   free_func)
{
  BseJob *job;
  
  g_return_val_if_fail (module != NULL, NULL);
  g_return_val_if_fail (access_func != NULL, NULL);
  
  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_ACCESS;
  job->data.access.node = ENGINE_NODE (module);
  job->data.access.access_func = access_func;
  job->data.access.data = data;
  job->data.access.free_func = free_func;
  
  return job;
}

/**
 * bse_engine_add_garbage
 * @data:      Data passed in to the free_func
 * @free_func: Function to free @data (executed in user thread)
 *
 * Queues data to be collected by bse_engine_garbage_collect(),
 * so @free_func() will be called with @data as argument
 * during the next garbage collection cycle in the user thread.
 * This function is MT-safe and may be called from any thread.
 */
void
bse_engine_add_garbage (gpointer      data,
                        BseFreeFunc   free_func)
{
  g_return_if_fail (free_func != NULL);

  BseJob *job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_ACCESS;
  job->data.access.node = NULL;
  job->data.access.access_func = NULL;
  job->data.access.data = data;
  job->data.access.free_func = free_func;

  BseTrans *trans = bse_trans_open();
  bse_trans_add (trans, job);
  bse_trans_dismiss (trans);
}

/**
 * BseEngineProbeFunc
 * @data:       user data passed in to bse_job_probe_request()
 * @tick_stamp: engine time in microseconds of the probe
 * @n_values:   number of values probed
 * @oblocks:    array of probe value block per output channel
 *
 * A BseEngineProbeFunc() is provided by users as a means to be notified about
 * a completed probe. This function is executed in the user thread.
 * Per each output channel that a probe has been requested through
 * bse_job_request_probe(), a block of probe values is supplied as
 * @oblocks[channel-index]. These blocks are allocate via g_new() and
 * may be "stolen" by assigning NULL to the respective pointer (the
 * caller then is responsible to g_free() the block).
 * Note that n_values may be 0 in case the module to be probed was
 * inactive. The blocks still contain bse_engine_block_size() values
 * regardless.
 */

/**
 * bse_job_request_probe
 * @module:            The module to access
 * @n_delay_samples:   Number of samples to wait before taking probes
 * @n_probe_values:    Number of probe values to take
 * @ochannel_bytemask: One byte per ochannel, bytes != 0 indicate a probe request
 * @probe_func:        Function invoked with @data in the user thread
 * @data:              Data passed in to the accessor
 * @Returns:           New job suitable for bse_trans_add()
 *
 * Create a new transaction job which inserts @probe_func with @data
 * into the job queue of @module.
 * Probe jobs are jobs which collect data from a given set of output
 * channels of a module as probe data. The job then returns to the
 * user thread before the next block boundary, and @probe_func()
 * will be invoked as early as possible.
 * There's no free_func() supplied to delete @data, because such a
 * function would always be called immediately after @probe_func().
 * So instead, any @data specific release handling should be integrated
 * into @probe_func().
 * The @ochannel_bytemask must point to an array of bytes with a size
 * equal to the number of output channels of @module.
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_probe_request (BseModule         *module,
                       guint              n_delay_samples,
                       guint              n_probe_values,
                       guint8            *ochannel_bytemask,
                       BseEngineProbeFunc probe_func,
                       gpointer           data)
{
  g_return_val_if_fail (module != NULL, NULL);
  g_return_val_if_fail (probe_func != NULL, NULL);
  g_return_val_if_fail (ochannel_bytemask != NULL, NULL);
  g_return_val_if_fail (n_probe_values > 0, NULL);
  
  guint i, n_oblocks = module->klass->n_ostreams;
  EngineProbeJob *pjob = g_malloc0 (sizeof (*pjob) +  sizeof (pjob->oblocks[0]) * n_oblocks);
  
  pjob->job_type = ENGINE_JOB_PROBE_JOB;
  pjob->probe_func = probe_func;
  pjob->data = data;
  pjob->tick_stamp = 0;
  pjob->delay_counter = n_delay_samples;
  pjob->value_counter = n_probe_values;
  pjob->n_values = 0;
  pjob->n_oblocks = n_oblocks;
  for (i = 0; i < n_oblocks; i++)
    if (ochannel_bytemask[i])
      pjob->oblocks[i] = g_new0 (gfloat, n_probe_values);
  
  BseJob *job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_PROBE_JOB;
  job->data.probe_job.node = ENGINE_NODE (module);
  job->data.probe_job.pjob = pjob;
  
  return job;
}

/**
 * bse_job_flow_access
 * @module:      The module to access
 * @tick_stamp:  Engine time stamp
 * @access_func: The accessor function
 * @data:        Data passed in to the accessor
 * @free_func:   Function to free @data
 * @Returns:     New job suitable for bse_trans_add()
 *
 * Create a new transaction job which inserts @access_func 
 * with @data into the flow job queue of @module.
 * Flow jobs are jobs with limited impact on modules, which
 * are executed during flow system progress at specific times.
 * Once the time stamp counter of @module passed @tick_stamp,
 * @access_func is called to modify the module's state.
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_flow_access (BseModule    *module,
		     guint64       tick_stamp,
		     BseEngineAccessFunc access_func,
		     gpointer      data,
		     BseFreeFunc   free_func)
{
  BseJob *job;
  EngineTimedJob *tjob;
  
  g_return_val_if_fail (module != NULL, NULL);
  g_return_val_if_fail (ENGINE_MODULE_IS_VIRTUAL (module) == FALSE, NULL);
  g_return_val_if_fail (tick_stamp < GSL_MAX_TICK_STAMP, NULL);
  g_return_val_if_fail (access_func != NULL, NULL);
  
  tjob = g_new0 (EngineTimedJob, 1);
  tjob->job_type = ENGINE_JOB_FLOW_JOB;
  tjob->free_func = free_func;
  tjob->data = data;
  tjob->tick_stamp = tick_stamp;
  tjob->access_func = access_func;
  
  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_FLOW_JOB;
  job->data.timed_job.node = ENGINE_NODE (module);
  job->data.timed_job.tjob = tjob;
  
  return job;
}

/**
 * bse_job_boundary_access
 * @module:      The module to access
 * @tick_stamp:  Engine time stamp
 * @access_func: The accessor function
 * @data:        Data passed in to the accessor
 * @free_func:   Function to free @data
 * @Returns:     New job suitable for bse_trans_add()
 *
 * Create a new transaction job which inserts @access_func 
 * with @data into the boundary job queue of @module.
 * Boundary jobs are executed at block boundaries, after all
 * ordinary jobs have been processed and before global time
 * stamp counter passed @tick_stamp.
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_boundary_access (BseModule    *module,
                         guint64       tick_stamp,
                         BseEngineAccessFunc access_func,
                         gpointer      data,
                         BseFreeFunc   free_func)
{
  BseJob *job;
  EngineTimedJob *tjob;
  
  g_return_val_if_fail (module != NULL, NULL);
  g_return_val_if_fail (ENGINE_MODULE_IS_VIRTUAL (module) == FALSE, NULL);
  g_return_val_if_fail (tick_stamp < GSL_MAX_TICK_STAMP, NULL);
  g_return_val_if_fail (access_func != NULL, NULL);
  
  tjob = g_new0 (EngineTimedJob, 1);
  tjob->job_type = ENGINE_JOB_BOUNDARY_JOB;
  tjob->free_func = free_func;
  tjob->data = data;
  tjob->tick_stamp = tick_stamp;
  tjob->access_func = access_func;
  
  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_BOUNDARY_JOB;
  job->data.timed_job.node = ENGINE_NODE (module);
  job->data.timed_job.tjob = tjob;
  
  return job;
}

/**
 * bse_job_suspend_now
 * @module: Module not currently suspended
 * @Returns: New job suitable for bse_trans_add()
 *
 * Create a new transaction job which suspends the @module
 * and all it's input modules which don't have other non-suspended
 * output connections.
 * Suspension of a module prevents it's process() method from being
 * called, it's outputs are simply filled with zero's instead.
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_suspend_now (BseModule *module)
{
  BseJob *job;
  
  g_return_val_if_fail (module != NULL, NULL);
  g_return_val_if_fail (ENGINE_MODULE_IS_VIRTUAL (module) == FALSE, NULL);
  
  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_SUSPEND;
  job->data.tick.node = ENGINE_NODE (module);
  job->data.tick.stamp = GSL_MAX_TICK_STAMP;
  
  return job;
}

/**
 * bse_job_resume_at
 * @module:     Module to resume
 * @tick_stamp: Sample tick at which to resume @module
 * @Returns:    New job suitable for bse_trans_add()
 *
 * Create a new transaction job which inserts a resumption
 * event into the job queue of @module.
 * Once the time stamp counter of @module passed @tick_stamp,
 * if it is supended, its reset() method is called and the
 * module is resumed, causing it's process() method to be
 * called again.
 * Resuming a module also resumes all input modules it has,
 * unless those were explicitely suspended via bse_job_suspend_now().
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_resume_at (BseModule *module,
                   guint64    tick_stamp)
{
  BseJob *job;
  
  g_return_val_if_fail (module != NULL, NULL);
  g_return_val_if_fail (ENGINE_MODULE_IS_VIRTUAL (module) == FALSE, NULL);
  g_return_val_if_fail (tick_stamp < GSL_MAX_TICK_STAMP, NULL);
  
  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_RESUME;
  job->data.tick.node = ENGINE_NODE (module);
  job->data.tick.stamp = tick_stamp;
  
  return job;
}

/**
 * BseEnginePollFunc
 * @data: Data of poll function
 * @n_values: Minimum number of values the engine wants to process
 * @timeout_p: Location of timeout value
 * @n_fds: Number of file descriptors used for polling
 * @fds: File descriptors to be used for polling
 * @revents_filled: Indicates whether @fds actually have their ->revents field filled with valid data.
 * @Returns: A boolean value indicating whether the engine should process data right now
 *
 * The BseEnginePollFunc is a user supplied callback function which can be hooked into the
 * BSE Engine. The engine uses the poll functions to determine whether processing of
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
 * bse_job_add_poll
 * @poll_func: Poll function to add
 * @data: Data of poll function
 * @free_func: Function to free @data
 * @n_fds: Number of poll file descriptors
 * @fds: File descriptors to select(2) or poll(2) on
 * @Returns: New job suitable for bse_trans_add()
 *
 * Create a new transaction job which adds a poll function
 * to the engine. The poll function is used by the engine to
 * determine whether processing is currently necessary.
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_add_poll (BseEnginePollFunc    poll_func,
		  gpointer       data,
		  BseFreeFunc    free_func,
		  guint          n_fds,
		  const GPollFD *fds)
{
  BseJob *job;
  
  g_return_val_if_fail (poll_func != NULL, NULL);
  if (n_fds)
    g_return_val_if_fail (fds != NULL, NULL);
  
  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_ADD_POLL;
  job->data.poll.poll_func = poll_func;
  job->data.poll.data = data;
  job->data.poll.free_func = free_func;
  job->data.poll.n_fds = n_fds;
  job->data.poll.fds = g_memdup (fds, sizeof (fds[0]) * n_fds);
  
  return job;
}

/**
 * bse_job_remove_poll
 * @poll_func: Poll function to remove
 * @data: Data of poll function
 * @Returns: New job suitable for bse_trans_add()
 *
 * Create a new transaction job which removes a previously inserted poll
 * function from the engine.
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_remove_poll (BseEnginePollFunc poll_func,
		     gpointer    data)
{
  BseJob *job;
  
  g_return_val_if_fail (poll_func != NULL, NULL);
  
  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_REMOVE_POLL;
  job->data.poll.poll_func = poll_func;
  job->data.poll.data = data;
  job->data.poll.free_func = NULL;
  job->data.poll.n_fds = 0;
  job->data.poll.fds = NULL;
  
  return job;
}

/**
 * bse_job_add_timer
 * @timer_func: Timer function to add
 * @data: Data of timer function
 * @free_func: Function to free @data
 * @Returns: New job suitable for bse_trans_add()
 *
 * Create a new transaction job which adds a timer function
 * to the engine. The timer function is called after the engine
 * caused new tick stamp updates.
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_add_timer (BseEngineTimerFunc timer_func,
		   gpointer           data,
		   BseFreeFunc        free_func)
{
  BseJob *job;
  
  g_return_val_if_fail (timer_func != NULL, NULL);
  
  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_ADD_TIMER;
  job->data.timer.timer_func = timer_func;
  job->data.timer.data = data;
  job->data.timer.free_func = free_func;
  
  return job;
}

/**
 * bse_job_debug
 * @debug: Debug message
 * @Returns: New job suitable for bse_trans_add()
 *
 * Create a new transaction job which issues @debug message when
 * the job is executed. This function is meant for debugging purposes
 * during development phase only and shouldn't be used in production code.
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_debug (const gchar *debug)
{
  BseJob *job;
  
  g_return_val_if_fail (debug != NULL, NULL);
  
  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_DEBUG;
  job->data.debug = g_strdup (debug);
  
  return job;
}

/**
 * bse_trans_open
 * @Returns: Newly opened empty transaction
 *
 * Open up a new transaction to commit jobs to the BSE Engine.
 * While the distinct functions to operate on a transaction are
 * MT-safe, the caller has to take measures himself, to assure
 * that only one function operates on the transaction at a time.
 * This function is MT-safe and may be called from any thread.
 */
BseTrans*
bse_trans_open (void)
{
  BseTrans *trans;
  
  trans = sfi_new_struct0 (BseTrans, 1);
  
  trans->jobs_head = NULL;
  trans->jobs_tail = NULL;
  trans->comitted = FALSE;
  trans->cqt_next = NULL;
  
  return trans;
}

/**
 * bse_trans_add
 * @trans: Opened transaction
 * @job: Job to add
 *
 * Append a job to an opened transaction.
 * This function is MT-safe and may be called from any thread.
 */
void
bse_trans_add (BseTrans *trans,
	       BseJob   *job)
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
 * bse_trans_merge
 * @trans1:  open transaction
 * @trans2:  open transaction
 * @Returns: open transaction
 *
 * Merge two open transactions by appending the jobs of @trans2
 * to the jobs of @trans1, returning the resulting transaction.
 * This function is MT-safe and may be called from any thread.
 */
BseTrans*
bse_trans_merge (BseTrans *trans1,
		 BseTrans *trans2)
{
  g_return_val_if_fail (trans1 != NULL, trans2);
  g_return_val_if_fail (trans1->comitted == FALSE, trans2);
  g_return_val_if_fail (trans2 != NULL, trans1);
  g_return_val_if_fail (trans2->comitted == FALSE, trans1);
  
  if (!trans1->jobs_head)
    {
      trans1->jobs_head = trans2->jobs_head;
      trans1->jobs_tail = trans2->jobs_tail;
      trans2->jobs_head = NULL;
      trans2->jobs_tail = NULL;
    }
  else if (trans2->jobs_head)
    {
      trans1->jobs_tail->next = trans2->jobs_head;
      trans1->jobs_tail = trans2->jobs_tail;
      trans2->jobs_head = NULL;
      trans2->jobs_tail = NULL;
    }
  bse_trans_dismiss (trans2);
  return trans1;
}

/**
 * bse_trans_commit
 * @trans: open transaction
 *
 * Close the transaction and commit it to the engine. The engine
 * will execute the jobs contained in this transaction as soon as
 * it has completed its current processing cycle. The jobs will be
 * executed in the exact order they were added to the transaction.
 * This function is MT-safe and may be called from any thread.
 */
void
bse_trans_commit (BseTrans *trans)
{
  g_return_if_fail (trans != NULL);
  g_return_if_fail (trans->comitted == FALSE);
  
  if (trans->jobs_head)
    {
      trans->comitted = TRUE;
      _engine_enqueue_trans (trans);
      wakeup_master ();
    }
  else
    bse_trans_dismiss (trans);
}

typedef struct {
  BseTrans *trans;
  guint64   tick_stamp;
  SfiCond   cond;
  SfiMutex  mutex;
} DTrans;

static gboolean
dtrans_timer (gpointer timer_data,
	      guint64  stamp)
{
  DTrans *data = timer_data;
  if (data->tick_stamp <= stamp)
    {
      if (!data->trans->jobs_head)
	{
	  /* this is sick, is this some perverted way of
	   * trying to wait until @tick_stamp passed by?
	   */
	  bse_trans_dismiss (data->trans);
	}
      else
	bse_trans_commit (data->trans);
      SFI_SPIN_LOCK (&data->mutex);
      data->trans = NULL;
      SFI_SPIN_UNLOCK (&data->mutex);
      sfi_cond_signal (&data->cond);
      return FALSE;
    }
  return TRUE;
}

/**
 * bse_trans_commit_delayed
 * @trans:      open transaction
 * @tick_stamp: earliest stamp
 *
 * Commit the transaction like bse_trans_commit(), but make sure
 * that the commit happens no earlier than @tick_stamp. This
 * function will block until the commit occoured, so it will not
 * return any earlier than @tick_stamp.
 * This function is MT-safe and may be called from any thread.
 */ /* bullshit, this function can't be called from the master thread ;) */
void
bse_trans_commit_delayed (BseTrans *trans,
			  guint64   tick_stamp)
{
  g_return_if_fail (trans != NULL);
  g_return_if_fail (trans->comitted == FALSE);
  
  if (tick_stamp <= gsl_tick_stamp ())
    bse_trans_commit (trans);
  else
    {
      BseTrans *wtrans = bse_trans_open ();
      DTrans data = { 0, };
      data.trans = trans;
      data.tick_stamp = tick_stamp;
      sfi_cond_init (&data.cond);
      sfi_mutex_init (&data.mutex);
      bse_trans_add (wtrans, bse_job_add_timer (dtrans_timer, &data, NULL));
      SFI_SPIN_LOCK (&data.mutex);
      bse_trans_commit (wtrans);
      while (data.trans)
	sfi_cond_wait (&data.cond, &data.mutex);
      SFI_SPIN_UNLOCK (&data.mutex);
      sfi_cond_destroy (&data.cond);
      sfi_mutex_destroy (&data.mutex);
    }
}

/**
 * bse_trans_dismiss
 * @trans: Opened transaction
 *
 * Close and discard the transaction, causes destruction of
 * all jobs currently contained in it and prevents their execution.
 * This function is MT-safe and may be called from any thread.
 */
void
bse_trans_dismiss (BseTrans *trans)
{
  g_return_if_fail (trans != NULL);
  g_return_if_fail (trans->comitted == FALSE);
  
  _engine_free_trans (trans);
}

/**
 * bse_transact
 * @job: First job
 * @...: %NULL terminated job list
 *
 * Convenience function which openes up a new transaction,
 * collects the %NULL terminated job list passed to the function,
 * and commits the transaction.
 * This function is MT-safe and may be called from any thread.
 */
void
bse_transact (BseJob *job,
	      ...)
{
  BseTrans *trans = bse_trans_open ();
  va_list var_args;
  
  va_start (var_args, job);
  while (job)
    {
      bse_trans_add (trans, job);
      job = va_arg (var_args, BseJob*);
    }
  va_end (var_args);
  bse_trans_commit (trans);
}


/* --- Virtual Modules --- */
static void
virtual_module_process (BseModule *module,
			guint      n_values)
{
  guint i;
  
  /* dumb pass-through task (FIXME: virtualization works without _process()) */
  for (i = 0; i < BSE_MODULE_N_OSTREAMS (module); i++)
    if (module->ostreams[i].connected)
      module->ostreams[i].values = (gfloat*) module->istreams[i].values;
}

typedef struct {
  BseModuleClass    klass;
  BseFreeFunc free_data;
} VirtualModuleClass;

static void
virtual_module_free (gpointer        data,
		     const BseModuleClass *klass)
{
  VirtualModuleClass *vclass = (VirtualModuleClass*) klass;
  
  if (vclass->free_data)
    vclass->free_data (data);
  g_free (vclass);
}

/**
 * bse_module_new_virtual
 * @n_iostreams: number of input and output streams
 * @user_data:   user data, stored in module->user_data
 * @free_data:   function to free user_data when the module is discarded
 * @RETURNS:     a newly created module
 *
 * Create a new virtual module which has @n_iostreams input
 * streams and @n_iostreams output streams. Simply put,
 * virtual modules just pass all input stream signals through
 * to the corresponsding output stream.
 * However, they are cheaper to compute than a literal module
 * implementation that just passes through all data in its
 * progress() method, because the connections can be virtualized
 * in a connection optimization stage during scheduling, so that
 * they don't end up in the list of modules which need to be
 * processed during calculation phase.
 * Beware though, flow jobs may not be scheduled on virtual
 * modules (thusly, suspend jobs cannot be queued on them
 * either), as virtual modules are ignored during calculation
 * phase.
 * They do, however, work just like ordinary modules with regards
 * to suspension propagation, so the suspension state from
 * output modules does only propagate across the virtual
 * module to its input modules, if all its outputs are suspended.
 * Instead of a single virtual module with multiple input/output
 * streams, multiple virtual modules can be used if suspension
 * is desired to propagate per stream.
 * This function is MT-safe and may be called from any thread.
 */
BseModule*
bse_module_new_virtual (guint       n_iostreams,
			gpointer    user_data,
			BseFreeFunc free_data)
{
  VirtualModuleClass virtual_module_class = {
    {
      0,			/* n_istreams */
      0,			/* n_jstreams */
      0,			/* n_ostreams */
      virtual_module_process,	/* process */
      NULL,			/* process_defer */
      NULL,			/* reset */
      virtual_module_free,	/* free */
      BSE_COST_CHEAP
    },
    NULL,			/* free_data */
  };
  VirtualModuleClass *vclass;
  BseModule *module;
  
  g_return_val_if_fail (n_iostreams > 0, NULL);
  
  vclass = g_memdup (&virtual_module_class, sizeof (virtual_module_class));
  vclass->klass.n_istreams = n_iostreams;
  vclass->klass.n_ostreams = n_iostreams;
  vclass->free_data = free_data;
  module = bse_module_new (&vclass->klass, user_data);
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
      BseTrans *trans = bse_trans_open ();
      gchar *str = g_strdup_printf ("SLAVE(%p): idle", g_thread_self ());
      
      bse_trans_add (trans, bse_job_debug (str));
      g_free (str);
      bse_trans_add (trans, bse_job_debug ("string2"));
      bse_trans_commit (trans);
      
      trans = bse_trans_open ();
      bse_trans_add (trans, bse_job_debug ("trans2"));
      bse_trans_commit (trans);
      
      g_usleep (1000*500);
    }
}

/* --- setup & trigger --- */
static gboolean		bse_engine_initialized = FALSE;
static gboolean		bse_engine_threaded = FALSE;
static SfiThread       *master_thread = NULL;
static EngineMasterData master_data;
guint			bse_engine_exvar_block_size = 0;
guint			bse_engine_exvar_sample_freq = 0;
guint			bse_engine_exvar_control_mask = 0;

/**
 * bse_engine_constrain
 * @latency_ms:       calculation latency in milli seconds
 * @sample_freq:      mixing frequency
 * @control_freq:     frequency at which to check control values or 0
 * @block_size_p:     location of number of values to process block wise
 * @control_raster_p: location of number of values to skip between control values
 *
 * Calculate a suitable block size and control raster for a
 * @sample_freq at a specific @latency_ms (the latency shouldn't be 0).
 * The @control_freq if specified should me much smaller than the
 * @sample_freq. It determines how often control values are to be
 * checked when calculating blocks of sample values.
 * The block size determines the amount by which the global tick
 * stamp (see gsl_tick_stamp()) is updated everytime the whole
 * module network completed processing block size values.
 */
void
bse_engine_constrain (guint            latency_ms,
                      guint            sample_freq,
                      guint            control_freq,
                      guint           *block_size_p,
                      guint           *control_raster_p)
{
  guint tmp, block_size, control_raster;
  g_return_if_fail (sample_freq >= 100);
  
  /* constrain latency to avoid overflow */
  latency_ms = CLAMP (latency_ms, 1, 10000);
  /* derive block size from latency and sample frequency,
   * account for an effective latency split of 3 buffers
   */
  block_size = latency_ms * sample_freq / 1000 / 3;
  /* constrain block size */
  block_size = CLAMP (block_size, 8, MIN (BSE_STREAM_MAX_VALUES / 2, sample_freq / 3));
  /* shrink block size to a 2^n boundary */
  tmp = sfi_alloc_upper_power2 (block_size);
  block_size = block_size < tmp ? tmp >> 1 : tmp;
  /* constrain control_freq */
  control_freq = MIN (control_freq, sample_freq);
  if (!control_freq)
    control_freq = (sample_freq + block_size - 1) / block_size;
  /* calc control stepping */
  control_raster = (sample_freq + control_freq - 1) / control_freq;
  /* control_raster > block_size doesn't make much sense */
  control_raster = CLAMP (control_raster, 1, block_size);
  /* shrink control_raster to a 2^n boundary */
  tmp = sfi_alloc_upper_power2 (control_raster);
  control_raster = control_raster < tmp ? tmp >> 1 : tmp;
  /* return values */
  if (block_size_p)
    *block_size_p = block_size;
  if (control_raster_p)
    *control_raster_p = control_raster;
}

/**
 * bse_engine_configure
 * @latency_ms:       calculation latency in milli seconds
 * @sample_freq:      mixing frequency
 * @control_freq:     frequency at which to check control values or 0
 * @returns:          whether reconfiguration was successful
 *
 * Reconfigure engine parameters. This function may only be called
 * after engine initialization and can only succeed if no modules
 * are currently integrated.
 */
gboolean
bse_engine_configure (guint            latency_ms,
                      guint            sample_freq,
                      guint            control_freq)
{
  static SfiMutex sync_mutex = { 0, };
  static SfiCond  sync_cond = { 0, };
  static gboolean sync_lock = FALSE;
  guint block_size, control_raster, success = FALSE;
  BseTrans *trans;
  BseJob *job;
  g_return_val_if_fail (bse_engine_initialized == TRUE, FALSE);
  
  /* optimize */
  bse_engine_constrain (latency_ms, sample_freq, control_freq, &block_size, &control_raster);
  if (0 && block_size == bse_engine_block_size() && control_raster == bse_engine_control_raster())
    return TRUE;
  
  /* pseudo-sync first */
  bse_engine_wait_on_trans();
  /* paranoia checks */
  if (_engine_mnl_head() || sync_lock)
    return FALSE;
  
  /* block master */
  GSL_SPIN_LOCK (&sync_mutex);
  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_SYNC;
  job->data.sync.lock_mutex = &sync_mutex;
  job->data.sync.lock_cond = &sync_cond;
  job->data.sync.lock_p = &sync_lock;
  sync_lock = FALSE;
  trans = bse_trans_open();
  bse_trans_add (trans, job);
  if (bse_engine_threaded)
    bse_trans_commit (trans);
  else
    {
      bse_trans_dismiss (trans);
      /* simulate master */
      sync_lock = TRUE;
    }
  while (!sync_lock)
    sfi_cond_wait (&sync_cond, &sync_mutex);
  GSL_SPIN_UNLOCK (&sync_mutex);
  
  if (!_engine_mnl_head())
    {
      /* cleanup */
      bse_engine_garbage_collect();
      _engine_recycle_const_values (TRUE);
      /* adjust parameters */
      bse_engine_exvar_block_size = block_size;
      bse_engine_exvar_sample_freq = sample_freq;
      bse_engine_exvar_control_mask = control_raster - 1;
      /* fixup timer */
      _gsl_tick_stamp_set_leap (bse_engine_block_size());
      _gsl_tick_stamp_inc ();   /* ensure stamp validity (>0 and systime mark) */
      success = TRUE;
    }
  
  /* unblock master */
  GSL_SPIN_LOCK (&sync_mutex);
  sync_lock = FALSE;
  sfi_cond_signal (&sync_cond);
  GSL_SPIN_UNLOCK (&sync_mutex);
  
  if (success)
    DEBUG ("configured%s: mixfreq=%uHz bsize=%uvals craster=%u (cfreq=%f)",
           bse_engine_threaded ? "(threaded)" : "",
           bse_engine_sample_freq(), bse_engine_block_size(), bse_engine_control_raster(),
           bse_engine_sample_freq() / (float) bse_engine_control_raster());
  
  return success;
}

/**
 * bse_engine_init
 * @run_threaded:    whether the engine should be run threaded
 *
 * Initialize the BSE Engine, this function must be called prior to
 * any other engine related function and can only be invoked once.
 */
void
bse_engine_init (gboolean run_threaded)
{
  g_return_if_fail (bse_engine_initialized == FALSE);
  
  bse_engine_initialized = TRUE;
  bse_engine_reinit_utils();
  /* first configure */
  bse_engine_configure (50, 44100, 50);
  
  /* then setup threading */
  bse_engine_threaded = run_threaded;
  
  if (bse_engine_threaded)
    {
      gint err = pipe (master_data.wakeup_pipe);
      master_data.user_thread = sfi_thread_self ();
      if (!err)
	{
	  glong d_long = fcntl (master_data.wakeup_pipe[0], F_GETFL, 0);
	  /* DEBUG ("master_wpipe-readfd, blocking=%ld", d_long & O_NONBLOCK); */
	  d_long |= O_NONBLOCK;
	  err = fcntl (master_data.wakeup_pipe[0], F_SETFL, d_long);
	}
      if (!err)
	{
	  glong d_long = fcntl (master_data.wakeup_pipe[1], F_GETFL, 0);
	  /* DEBUG ("master_wpipe-writefd, blocking=%ld", d_long & O_NONBLOCK); */
	  d_long |= O_NONBLOCK;
	  err = fcntl (master_data.wakeup_pipe[1], F_SETFL, d_long);
	}
      if (err)
	g_error ("failed to create wakeup pipe: %s", g_strerror (errno));
      master_thread = sfi_thread_run ("DSP #1", (SfiThreadFunc) _engine_master_thread, &master_data);
      if (!master_thread)
	g_error ("failed to create master thread");
      if (0)
	sfi_thread_run ("DSP #2", slave, NULL);
    }
}

static void
wakeup_master (void)
{
  if (master_thread)
    {
      guint8 data = 'W';
      gint l;
      do
	l = write (master_data.wakeup_pipe[1], &data, 1);
      while (l < 0 && (errno == EINTR || errno == ERESTART));
    }
}

gboolean
bse_engine_prepare (BseEngineLoop *loop)
{
  g_return_val_if_fail (loop != NULL, FALSE);
  g_return_val_if_fail (bse_engine_initialized == TRUE, FALSE);
  
  if (!bse_engine_threaded)
    return _engine_master_prepare (loop) || bse_engine_has_garbage ();
  else
    {
      loop->timeout = -1;
      loop->fds_changed = FALSE;
      loop->n_fds = 0;
      loop->revents_filled = FALSE;
      return bse_engine_has_garbage ();
    }
}

gboolean
bse_engine_check (const BseEngineLoop *loop)
{
  g_return_val_if_fail (loop != NULL, FALSE);
  if (loop->n_fds)
    g_return_val_if_fail (loop->revents_filled == TRUE, FALSE);
  
  if (!bse_engine_threaded)
    return _engine_master_check (loop) || bse_engine_has_garbage ();
  else
    return bse_engine_has_garbage ();
}

/**
 * bse_engine_dispatch
 *
 * Perform necessary work the engine has to handle
 * in the user thread.
 * This function may only be called from the user thread,
 * since it will invoke BseFreeFunc() functions (see
 * bse_engine_garbage_collect()) and do
 * other things which are guranteed to be executed
 * in the user thread.
 */
void
bse_engine_dispatch (void)
{
  g_return_if_fail (bse_engine_initialized == TRUE);
  
  if (!bse_engine_threaded)
    _engine_master_dispatch ();
  
  if (bse_engine_has_garbage ())	/* prevent extra mutex locking */
    bse_engine_garbage_collect ();
}

SfiThread**
bse_engine_get_threads (guint *n_threads)
{
  SfiThread **t;
  if (!master_thread)
    {
      *n_threads = 0;
      return NULL;
    }
  *n_threads = 1;
  t = g_new0 (SfiThread*, 2);
  t[0] = master_thread;
  return t;
}

/**
 * bse_engine_tick_stamp_from_systime
 * @systime: System time in micro seconds.
 * @RETURNS: Engine tick stamp value
 *
 * Depending on the engine's sample frequency and the time
 * of the last global tick stamp update, calculate the
 * corresponding engine tick stamp from a given system time.
 * This function is MT-safe and may be called from any thread.
 */
guint64
bse_engine_tick_stamp_from_systime (guint64 systime)
{
  GslTickStampUpdate ustamp = gsl_tick_stamp_last ();
  guint64 tick_stamp;
  
  /* FIXME: we should add special guards here
   * for sfi_time_system() - ustamp.system_time ~> (44100 / bse_engine_block_size ())
   */
  
  if (systime > ustamp.system_time)
    {
      tick_stamp = systime - ustamp.system_time;
      tick_stamp = tick_stamp * bse_engine_sample_freq () / 1000000;
      tick_stamp += ustamp.tick_stamp;
    }
  else
    {
      tick_stamp = ustamp.system_time - systime;
      tick_stamp = tick_stamp * bse_engine_sample_freq () / 1000000;
      tick_stamp = ustamp.tick_stamp - MIN (tick_stamp, ustamp.tick_stamp);
    }
  
#if 0
  if (tick_stamp > 158760000)
    g_error ("tick_stamp conversion problem:\n"
	     "  tick_stamp            = %llu\n"
	     "  usec_systime          = %llu\n"
	     "  current-systime       = %llu\n"
	     "  current-tickstamp     = %llu\n"
	     "  last-update-systime   = %llu\n"
	     "  last-update-tickstamp = %llu\n"
	     "  sample-freq           = %u\n",
	     tick_stamp, systime, sfi_time_system (), gsl_tick_stamp (),
	     ustamp.system_time, ustamp.tick_stamp,
	     bse_engine_sample_freq ());
#endif
  
  return tick_stamp;
}

/**
 * bse_engine_wait_on_trans
 *
 * Wait until all pending transactions have been processed
 * by the BSE Engine.
 * This function, when done waiting, will run a garbage
 * collection cycle before returning (see
 * bse_engine_garbage_collect()).
 */
void
bse_engine_wait_on_trans (void)
{
  g_return_if_fail (bse_engine_initialized == TRUE);
  
  /* non-threaded */
  if (!bse_engine_threaded)
    _engine_master_dispatch_jobs ();
  
  /* threaded */
  _engine_wait_on_trans ();
  
  /* call all free() functions */
  bse_engine_garbage_collect ();
}

/* vim:set ts=8 sts=2 sw=2: */
