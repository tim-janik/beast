// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseengine.hh"
#include "bsemain.hh"
#include "gslcommon.hh"
#include "bseengineutils.hh"
#include "bseenginemaster.hh"
#include "bsestartup.hh"        // for TaskRegistry
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#define EDEBUG(...)     Bse::debug ("engine", __VA_ARGS__)

/* some systems don't have ERESTART (which is what linux returns for system
 * calls on pipes which are being interrupted). most probably just use EINTR,
 * and maybe some can return both. so we check for both in the below code,
 * and alias ERESTART to EINTR if it's not present.
 */
#ifndef ERESTART
#define ERESTART        EINTR
#endif

/* --- UserThread --- */
/**
 * @param klass	the BseModuleClass which determines the module's behaviour
 * @param user_data	user data pointer
 * @return		a newly created module
 *
 * Create a new module with methods specified in @a klass and
 * a user_data field set to @a user_data. The returned module
 * can then be integrated into the engine with bse_job_integrate().
 * This function is MT-safe and may be called from any thread.
 */
BseModule*
bse_module_new (const BseModuleClass *klass,
		gpointer              user_data)
{
  EngineNode *node;
  guint i;

  assert_return (klass != NULL, NULL);
  assert_return (klass->process != NULL || klass->process_defer != NULL, NULL);
  if (klass->process_defer)
    {
      Bse::warning ("%s: Delay cycle processing not yet implemented", __func__);
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
  new (&node->rec_mutex) std::recursive_mutex();
  for (i = 0; i < ENGINE_NODE_N_OSTREAMS (node); i++)
    node->outputs[i].buffer = node->module.ostreams[i].values;
  node->flow_jobs = NULL;
  node->boundary_jobs = NULL;
  node->probe_jobs = NULL;
  node->tjob_head = node->tjob_tail = NULL;

  return &node->module;
}

/**
 * @param module	a BSE Engine Module
 * @return		the module's tick stamp, indicating its process status
 *
 * Any thread may call this function on a valid engine module.
 * The module specific tick stamp is updated to Bse::TickStamp::current() +
 * @a n_values every time its BseProcessFunc() function was
 * called. See also Bse::TickStamp::current().
 * This function is MT-safe and may be called from any thread.
 */
guint64
bse_module_tick_stamp (BseModule *module)
{
  assert_return (module != NULL, 0);

  return ENGINE_NODE (module)->counter;
}

/**
 * @param module	a BSE Engine Module
 * @param istream	Index of input stream
 * @return		whether the module has a possible input
 *
 * Check whether @a istream may be disconnected via
 * bse_job_disconnect(). This is not an indication for whether
 * BSE_MODULE_ISTREAM (@a module, @a istream).connected will be TRUE
 * during process(), as the source may be a dangling virtual module,
 * resulting in BSE_MODULE_ISTREAM (@a module, @a istream).connected
 * being FALSE.
 * See also bse_module_new_virtual().
 * This function is MT-safe and may be called from any thread.
 */
gboolean
bse_module_has_source (BseModule *module,
                       guint      istream)
{
  assert_return (module != NULL, FALSE);
  assert_return (istream < module->klass->n_istreams, FALSE);

  return ENGINE_NODE (module)->inputs[istream].src_node != NULL;
}

/**
 * @param module	a BSE Engine Module
 * @return		whether the module is scheduled
 *
 * Check whether @a module is part of the schedule required to
 * calculate the signal flow up to the consumer modules.
 * This state may frequently change with for instance connection
 * changes of other modules.
 * This function is MT-safe and may be called from any thread.
 */
gboolean
bse_module_is_scheduled (BseModule *module)
{
  assert_return (module != NULL, FALSE);
  EngineNode *node = ENGINE_NODE (module);
  return ENGINE_NODE_IS_INTEGRATED (node) && ENGINE_NODE_IS_SCHEDULED (node);
}

/**
 * @param module	The module to integrate
 * @return       	New job suitable for bse_trans_add()
 *
 * Create a new transaction job to integrate @a module into the engine.
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_integrate (BseModule *module)
{
  BseJob *job;

  assert_return (module != NULL, NULL);

  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_INTEGRATE;
  job->data.node = ENGINE_NODE (module);
  job->data.free_with_job = TRUE;

  return job;
}

/**
 * @param module	The module to discard
 * @return       	New job suitable for bse_trans_add()
 *
 * Create a new transaction job which removes @a module from the
 * engine and destroys it.
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_discard (BseModule *module)
{
  BseJob *job;

  assert_return (module != NULL, NULL);

  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_DISCARD;
  job->data.node = ENGINE_NODE (module);

  return job;
}

/**
 * @param module	Module with input streams
 * @return       	New job suitable for bse_trans_add()
 *
 * Create a new transaction job which causes all connected input streams
 * of @a module to be disconnected, like it's done upon discarding the module.
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_kill_inputs (BseModule *module)
{
  BseJob *job;

  assert_return (module != NULL, NULL);

  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_KILL_INPUTS;
  job->data.node = ENGINE_NODE (module);

  return job;
}

/**
 * @param module	Module with output streams
 * @return       	New job suitable for bse_trans_add()
 *
 * Create a new transaction job which causes all connected output streams
 * of @a module to be disconnected, like it's done upon discarding the module.
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_kill_outputs (BseModule *module)
{
  BseJob *job;

  assert_return (module != NULL, NULL);

  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_KILL_OUTPUTS;
  job->data.node = ENGINE_NODE (module);

  return job;
}

/**
 * @param src_module	Module with output stream
 * @param src_ostream	Index of output stream of @a src_module
 * @param dest_module	Module with unconnected input stream
 * @param dest_istream	Index of input stream of @a dest_module
 * @return       	New job suitable for bse_trans_add()
 *
 * Create a new transaction job which connects the ouput stream @a src_ostream
 * of module @a src_module to the input stream @a dest_istream of module @a dest_module
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

  assert_return (src_module != NULL, NULL);
  assert_return (src_ostream < src_module->klass->n_ostreams, NULL);
  assert_return (dest_module != NULL, NULL);
  assert_return (dest_istream < dest_module->klass->n_istreams, NULL);

  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_ICONNECT;
  job->connection.dest_node = ENGINE_NODE (dest_module);
  job->connection.dest_ijstream = dest_istream;
  job->connection.src_node = ENGINE_NODE (src_module);
  job->connection.src_ostream = src_ostream;

  return job;
}

/**
 * @param src_module	Module with output stream
 * @param src_ostream	Index of output stream of @a src_module
 * @param dest_module	Module with unconnected joint input stream
 * @param dest_jstream	Index of joint input stream of @a dest_module
 * @return       	New job suitable for bse_trans_add()
 *
 * Create a new transaction job which connects the ouput stream @a src_ostream
 * of module @a src_module to the joint input stream @a dest_istream of module
 * @a dest_module.
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_jconnect (BseModule *src_module,
		  guint      src_ostream,
		  BseModule *dest_module,
		  guint      dest_jstream)
{
  BseJob *job;

  assert_return (src_module != NULL, NULL);
  assert_return (src_ostream < src_module->klass->n_ostreams, NULL);
  assert_return (dest_module != NULL, NULL);
  assert_return (dest_jstream < dest_module->klass->n_jstreams, NULL);

  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_JCONNECT;
  job->connection.dest_node = ENGINE_NODE (dest_module);
  job->connection.dest_ijstream = dest_jstream;
  job->connection.src_node = ENGINE_NODE (src_module);
  job->connection.src_ostream = src_ostream;

  return job;
}

/**
 * @param dest_module	Module with connected input stream
 * @param dest_istream	Index of input stream of @a dest_module
 * @return       	New job suitable for bse_trans_add()
 *
 * Create a new transaction job which causes the input stream @a dest_istream
 * of @a dest_module to be disconnected (it is an error if the input stream isn't
 * connected by the time the job is executed).
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_disconnect (BseModule *dest_module,
		    guint      dest_istream)
{
  BseJob *job;

  assert_return (dest_module != NULL, NULL);
  assert_return (dest_istream < dest_module->klass->n_istreams, NULL);

  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_IDISCONNECT;
  job->connection.dest_node = ENGINE_NODE (dest_module);
  job->connection.dest_ijstream = dest_istream;
  job->connection.src_node = NULL;
  job->connection.src_ostream = ~0;

  return job;
}

/**
 * @param dest_module	Module with connected input stream
 * @param dest_jstream	Index of input stream of @a dest_module
 * @param src_module	Module with output stream
 * @param src_ostream	Index of output stream of @a src_module
 * @return       	New job suitable for bse_trans_add()
 *
 * Create a new transaction job which causes the joint input
 * stream @a dest_jstream of @a dest_module to be disconnected from
 * the output stream @a src_ostream of @a src_module (it is an
 * error if this connection isn't established by the time the
 * job is executed). Beware, the order of @a dest_module and
 * @a src_module is different from bse_job_jconnect().
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_jdisconnect (BseModule *dest_module,
		     guint      dest_jstream,
		     BseModule *src_module,
		     guint	src_ostream)
{
  BseJob *job;

  assert_return (dest_module != NULL, NULL);
  assert_return (dest_jstream < dest_module->klass->n_jstreams, NULL);
  assert_return (src_module != NULL, NULL);
  assert_return (src_ostream < src_module->klass->n_ostreams, NULL);

  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_JDISCONNECT;
  job->connection.dest_node = ENGINE_NODE (dest_module);
  job->connection.dest_ijstream = dest_jstream;
  job->connection.src_node = ENGINE_NODE (src_module);
  job->connection.src_ostream = src_ostream;

  return job;
}

BseJob*
bse_job_set_consumer (BseModule *module,
		      gboolean   is_toplevel_consumer)
{
  BseJob *job;

  assert_return (module != NULL, NULL);
  assert_return (ENGINE_MODULE_IS_VIRTUAL (module) == FALSE, NULL);

  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = is_toplevel_consumer ? ENGINE_JOB_SET_CONSUMER : ENGINE_JOB_UNSET_CONSUMER;
  job->data.node = ENGINE_NODE (module);

  return job;
}

/**
 * @param module	The module to be reset
 * @return       	New job suitable for bse_trans_add()
 *
 * Forces a reset of @a module before its next call to
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

  assert_return (module != NULL, NULL);

  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_FORCE_RESET;
  job->data.node = ENGINE_NODE (module);

  return job;
}

/**
 * \fn BseEngineAccessFunc
 * @param module	Module to operate on
 * @param data	Accessor data
 *
 * The BseEngineAccessFunc is a user supplied callback function which can access
 * a module in times it is not processing. Accessors are usually used to
 * either read out a module's current state, or to modify its state. An
 * accessor may only operate on the @a data and the @a module passed
 * in to it.
 */
/**
 * @param module	The module to access
 * @param access_func	The accessor function (executed in master thread)
 * @param data	Data passed in to the accessor
 * @param free_func	Function to free @a data (executed in user thread)
 * @return       	New job suitable for bse_trans_add()
 *
 * Create a new transaction job which will invoke @a access_func 
 * on @a module with @a data when the transaction queue is processed
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

  assert_return (module != NULL, NULL);
  assert_return (access_func != NULL, NULL);

  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_ACCESS;
  job->access.node = ENGINE_NODE (module);
  job->access.access_func = access_func;
  job->access.data = data;
  job->access.free_func = free_func;

  return job;
}

/**
 * @param data	Data passed in to the free_func
 * @param free_func	Function to free @a data (executed in user thread)
 *
 * Queues data to be collected by bse_engine_user_thread_collect(),
 * so @a free_func() will be called with @a data as argument
 * during the next garbage collection cycle in the user thread.
 * This function is MT-safe and may be called from any thread.
 */
void
bse_engine_add_user_callback (gpointer      data,
                              BseFreeFunc   free_func)
{
  assert_return (free_func != NULL);

  BseJob *job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_ACCESS;
  job->access.node = NULL;
  job->access.access_func = NULL;
  job->access.data = data;
  job->access.free_func = free_func;

  BseTrans *trans = bse_trans_open();
  bse_trans_add (trans, job);
  bse_trans_dismiss (trans);
}

/**
 * \fn BseEngineProbeFunc
 * @param data  	user data passed in to bse_job_probe_request()
 * @param n_values	number of values probed
 * @param tick_stamp	engine time in microseconds of the probe
 * @param n_ostreams	number of ostreams of the module
 * @param ostreams_p	location of a pointer to the probed ostream array
 *
 * A BseEngineProbeFunc() is provided by users as a means to be notified about
 * a completed probe. This function is executed in the user thread.
 * The complete set of output streams and associated output values is provided
 * by @a n_ostreams and @a ostreams_p.
 * For intermediate user thread processing, the set can be "stolen" in the
 * probe callback by assigning NULL to *ostreams_p. In this case, the set has
 * to later be freed with bse_engine_free_ostreams().
 * Note that output streams with FALSE connected flags will not contain valid
 * data in their value blocks.
 */

/**
 * @param module	The module to access
 * @param probe_func	Function invoked with @a data in the user thread
 * @param data	        Data passed in to the accessor
 * @return       	New job suitable for bse_trans_add()
 *
 * Create a new transaction job which inserts @a probe_func with @a data
 * into the job queue of @a module.
 * Probe jobs are jobs which collect data from the output
 * channels of a module as probe data. The job then returns to the
 * user thread before the next block boundary, and @a probe_func()
 * will be invoked as early as possible.
 * There's no free_func() supplied to delete @a data, because such a
 * function would always be called immediately after @a probe_func().
 * So instead, any @a data specific release handling should be integrated
 * into @a probe_func().
 * For multiple probe jobs enqueued on a module simultaneously, no
 * ordering is preserved.
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_probe_request (BseModule         *module,
                       BseEngineProbeFunc probe_func,
                       gpointer           data)
{
  assert_return (module != NULL, NULL);
  EngineNode *node = ENGINE_NODE (module);
  assert_return (probe_func != NULL, NULL);

  EngineTimedJob *tjob = (EngineTimedJob*) g_malloc0 (sizeof (tjob->probe));
  tjob->type = ENGINE_JOB_PROBE_JOB;
  tjob->tick_stamp = 0;
  tjob->probe.data = data;
  tjob->probe.probe_func = probe_func;
  tjob->probe.n_ostreams = ENGINE_NODE_N_OSTREAMS (node);
  tjob->probe.ostreams = _engine_alloc_ostreams (ENGINE_NODE_N_OSTREAMS (node));

  BseJob *job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_PROBE_JOB;
  job->timed_job.node = ENGINE_NODE (module);
  job->timed_job.tjob = tjob;

  return job;
}

/**
 * @param module	The module to access
 * @param tick_stamp	Engine time stamp
 * @param access_func	The accessor function
 * @param data	Data passed in to the accessor
 * @param free_func	Function to free @a data
 * @return       	New job suitable for bse_trans_add()
 *
 * Create a new transaction job which inserts @a access_func 
 * with @a data into the flow job queue of @a module.
 * Flow jobs are jobs with limited impact on modules, which
 * are executed during flow system progress at specific times.
 * Once the time stamp counter of @a module passed @a tick_stamp,
 * @a access_func is called to modify the module's state.
 * Flow jobs queued for executaion after a node's destruction
 * will not be executed but destroyed together with the node.
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
  assert_return (module != NULL, NULL);
  assert_return (ENGINE_MODULE_IS_VIRTUAL (module) == FALSE, NULL);
  assert_return (tick_stamp < Bse::TickStamp::max_stamp(), NULL);
  assert_return (access_func != NULL, NULL);
  EngineTimedJob *tjob = (EngineTimedJob*) g_malloc0 (sizeof (tjob->access));
  tjob->type = ENGINE_JOB_FLOW_JOB;
  tjob->tick_stamp = tick_stamp;
  tjob->access.free_func = free_func;
  tjob->access.data = data;
  tjob->access.access_func = access_func;
  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_FLOW_JOB;
  job->timed_job.node = ENGINE_NODE (module);
  job->timed_job.tjob = tjob;
  return job;
}
/**
 * @param module	The module to access
 * @param tick_stamp	Engine time stamp
 * @param access_func	The accessor function
 * @param data	Data passed in to the accessor
 * @param free_func	Function to free @a data
 * @return       	New job suitable for bse_trans_add()
 *
 * Create a new transaction job which inserts @a access_func 
 * with @a data into the boundary job queue of @a module.
 * Boundary jobs are executed at block boundaries, after all
 * ordinary jobs have been processed and before global time
 * stamp counter passed @a tick_stamp.
 * Boundary jobs queued for executaion after a node's destruction
 * will not be executed but destroyed together with the node.
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
  assert_return (module != NULL, NULL);
  assert_return (ENGINE_MODULE_IS_VIRTUAL (module) == FALSE, NULL);
  assert_return (tick_stamp < Bse::TickStamp::max_stamp(), NULL);
  assert_return (access_func != NULL, NULL);
  EngineTimedJob *tjob = (EngineTimedJob*) g_malloc0 (sizeof (tjob->access));
  tjob->type = ENGINE_JOB_BOUNDARY_JOB;
  tjob->tick_stamp = tick_stamp;
  tjob->access.free_func = free_func;
  tjob->access.data = data;
  tjob->access.access_func = access_func;
  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_BOUNDARY_JOB;
  job->timed_job.node = ENGINE_NODE (module);
  job->timed_job.tjob = tjob;
  return job;
}
static void
bse_engine_boundary_discard (BseModule      *module,
                             gpointer        data)
{
  BseTrans *trans = bse_trans_open();
  bse_trans_add (trans, bse_job_discard (module));
  bse_trans_commit (trans);
}

/**
 * @param module	The module to access
 * @return       	New job suitable for bse_trans_add()
 *
 * Discard @a module at block boundaries, after all ordinary jobs
 * have been processed. This job type should be used instead of
 * jobs from bse_job_discard() in situations where queueing of
 * past-discard jobs before the next block boundary is hard to
 * avoid (such as queing disconnection/suspend jobs from within
 * process()).
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_boundary_discard (BseModule *module)
{
  assert_return (module != NULL, NULL);

  EngineTimedJob *tjob = (EngineTimedJob*) g_malloc0 (sizeof (tjob->access));
  tjob->type = ENGINE_JOB_BOUNDARY_JOB;
  tjob->tick_stamp = 0;
  tjob->access.free_func = NULL;
  tjob->access.data = NULL;
  tjob->access.access_func = bse_engine_boundary_discard;

  BseJob *job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_BOUNDARY_JOB;
  job->timed_job.node = ENGINE_NODE (module);
  job->timed_job.tjob = tjob;

  return job;
}

/**
 * @param module	Module not currently suspended
 * @return       	New job suitable for bse_trans_add()
 *
 * Create a new transaction job which suspends the @a module
 * and all it's input modules which don't have other non-suspended
 * output connections.
 * Suspension of a module prevents it's process() method from being
 * called, it's outputs are simply filled with zero's instead.
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_suspend_now (BseModule *module)
{
  assert_return (module != NULL, NULL);
  assert_return (ENGINE_MODULE_IS_VIRTUAL (module) == FALSE, NULL);
  BseJob *job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_SUSPEND;
  job->tick.node = ENGINE_NODE (module);
  job->tick.stamp = Bse::TickStamp::max_stamp();
  return job;
}
/**
 * @param module	Module to resume
 * @param tick_stamp	Sample tick at which to resume @a module
 * @return       	New job suitable for bse_trans_add()
 *
 * Create a new transaction job which inserts a resumption
 * event into the job queue of @a module.
 * Once the time stamp counter of @a module passed @a tick_stamp,
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
  assert_return (module != NULL, NULL);
  assert_return (ENGINE_MODULE_IS_VIRTUAL (module) == FALSE, NULL);
  assert_return (tick_stamp < Bse::TickStamp::max_stamp(), NULL);
  BseJob *job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_RESUME;
  job->tick.node = ENGINE_NODE (module);
  job->tick.stamp = tick_stamp;
  return job;
}
/**
 * \fn BseEnginePollFunc
 * @param data	Data of poll function
 * @param n_values	Minimum number of values the engine wants to process
 * @param timeout_p	Location of timeout value
 * @param n_fds	Number of file descriptors used for polling
 * @param fds	File descriptors to be used for polling
 * @param revents_filled	Indicates whether @a fds actually have their ->revents field filled with valid data.
 * @return       	A boolean value indicating whether the engine should process data right now
 *
 * The BseEnginePollFunc is a user supplied callback function which can be hooked into the
 * BSE Engine. The engine uses the poll functions to determine whether processing of
 * @a n_values in its module network is necessary.
 * In order for the poll functions to react to extern events, such as device driver
 * status changes, the engine will poll(2) the @a fds of the poll function and invoke
 * the callback with @a revents_filled == TRUE if any of its @a fds changed state.
 * The callback may also be invoked at other random times with @a revents_filled = FALSE.
 * It is supposed to return TRUE if network processing is currently necessary, and
 * FALSE if not.
 * If FALSE is returned, @a timeout_p may be filled with the number of milliseconds
 * the engine should use for polling at maximum.
 */
/**
 * @param poll_func	Poll function to add
 * @param data	Data of poll function
 * @param free_func	Function to free @a data
 * @param n_fds	Number of poll file descriptors
 * @param fds	File descriptors to select(2) or poll(2) on
 * @return       	New job suitable for bse_trans_add()
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

  assert_return (poll_func != NULL, NULL);
  if (n_fds)
    assert_return (fds != NULL, NULL);

  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_ADD_POLL;
  job->poll.poll_func = poll_func;
  job->poll.data = data;
  job->poll.free_func = free_func;
  job->poll.n_fds = n_fds;
  job->poll.fds = (GPollFD*) g_memdup (fds, sizeof (fds[0]) * n_fds);

  return job;
}

/**
 * @param poll_func	Poll function to remove
 * @param data	Data of poll function
 * @return       	New job suitable for bse_trans_add()
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

  assert_return (poll_func != NULL, NULL);

  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_REMOVE_POLL;
  job->poll.poll_func = poll_func;
  job->poll.data = data;
  job->poll.free_func = NULL;
  job->poll.n_fds = 0;
  job->poll.fds = NULL;

  return job;
}

/**
 * @param timer_func	Timer function to add
 * @param data	Data of timer function
 * @param free_func	Function to free @a data
 * @return       	New job suitable for bse_trans_add()
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

  assert_return (timer_func != NULL, NULL);

  job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_ADD_TIMER;
  job->timer.timer_func = timer_func;
  job->timer.data = data;
  job->timer.free_func = free_func;

  return job;
}

/**
 * @param debug	Debug message
 * @return       	New job suitable for bse_trans_add()
 *
 * Create a new transaction job which issues @a debug message when
 * the job is executed. This function is meant for debugging purposes
 * during development phase only and shouldn't be used in production code.
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_debug (const gchar *debug)
{
  assert_return (debug != NULL, NULL);

  BseJob *job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_MESSAGE;
  job->data.message = g_strdup (debug);
  return job;
}

/**
 * @return       	New job suitable for bse_trans_add()
 *
 * Create a new transaction job which does nothing.
 * The job enforces a roundtrip to the engine's master
 * thread however, which may be relevant when comitting
 * otherwise empty transactions and calling
 * bse_engine_wait_on_trans().
 * This function is MT-safe and may be called from any thread.
 */
BseJob*
bse_job_nop (void)
{
  BseJob *job = sfi_new_struct0 (BseJob, 1);
  job->job_id = ENGINE_JOB_MESSAGE;
  job->data.message = NULL;
  return job;
}

/**
 * @return       	Newly opened empty transaction
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
 * @param trans	Opened transaction
 * @param job	Job to add
 *
 * Append a job to an opened transaction.
 * This function is MT-safe and may be called from any thread.
 */
void
bse_trans_add (BseTrans *trans,
	       BseJob   *job)
{
  assert_return (trans != NULL);
  assert_return (trans->comitted == FALSE);
  assert_return (job != NULL);
  assert_return (job->next == NULL);

  if (trans->jobs_tail)
    trans->jobs_tail->next = job;
  else
    trans->jobs_head = job;
  trans->jobs_tail = job;
}

/**
 * @param trans1	open transaction
 * @param trans2	open transaction
 * @return       	open transaction @a trans1
 *
 * Merge two open transactions by appending the jobs of @a trans2
 * to the jobs of @a trans1, returning the resulting transaction.
 * The empty transaction @a trans2 will be dismissed after the merge.
 * This function is MT-safe and may be called from any thread.
 */
BseTrans*
bse_trans_merge (BseTrans *trans1,
		 BseTrans *trans2)
{
  assert_return (trans1 != NULL, trans2);
  assert_return (trans1->comitted == FALSE, trans2);
  assert_return (trans2 != NULL, trans1);
  assert_return (trans2->comitted == FALSE, trans1);

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
 * @param trans	open transaction
 * @return		tick stamp of job execution
 *
 * Close the transaction and commit it to the engine. The engine
 * will execute the jobs contained in this transaction as soon as
 * it has completed its current processing cycle, at which point
 * Bse::TickStamp::current() matches the returned tick stamp.
 * The jobs will be executed in the exact order they were added
 * to the transaction.
 * This function is MT-safe and may be called from any thread.
 */
guint64
bse_trans_commit (BseTrans *trans)
{
  assert_return (trans != NULL, 0);
  assert_return (trans->comitted == FALSE, 0);

  guint64 exec_tick_stamp = 0;
  if (trans->jobs_head)
    {
      trans->comitted = TRUE;
      exec_tick_stamp = _engine_enqueue_trans (trans);
      Bse::MasterThread::wakeup();
    }
  else
    bse_trans_dismiss (trans);
  return exec_tick_stamp;
}
typedef struct {
  BseTrans               *trans;
  guint64                 tick_stamp;
  std::condition_variable cond;
  std::mutex              mutex;
} DTrans;
static gboolean
dtrans_timer (gpointer timer_data,
	      guint64  stamp)
{
  DTrans *data = (DTrans*) timer_data;
  if (data->tick_stamp <= stamp)
    {
      if (!data->trans->jobs_head)
	{
	  /* this is sick, is this some perverted way of
	   * trying to wait until tick_stamp passed by?
	   */
	  bse_trans_dismiss (data->trans);
	}
      else
	bse_trans_commit (data->trans);
      data->mutex.lock();
      data->trans = NULL;
      data->mutex.unlock();
      data->cond.notify_one();
      return FALSE;
    }
  return TRUE;
}

/**
 * @param trans	open transaction
 * @param tick_stamp	earliest stamp
 *
 * Commit the transaction like bse_trans_commit(), but make sure
 * that the commit happens no earlier than @a tick_stamp. This
 * function will block until the commit occoured, so it will not
 * return any earlier than @a tick_stamp.
 * This function is MT-safe and may be called from any thread.
 */ /* bullshit, this function can't be called from the master thread ;) */
void
bse_trans_commit_delayed (BseTrans *trans,
			  guint64   tick_stamp)
{
  assert_return (trans != NULL);
  assert_return (trans->comitted == FALSE);
  if (tick_stamp <= Bse::TickStamp::current())
    bse_trans_commit (trans);
  else
    {
      BseTrans *wtrans = bse_trans_open ();
      DTrans data = { 0, };
      data.trans = trans;
      data.tick_stamp = tick_stamp;
      bse_trans_add (wtrans, bse_job_add_timer (dtrans_timer, &data, NULL));
      std::unique_lock<std::mutex> data_lock (data.mutex);
      bse_trans_commit (wtrans);
      while (data.trans)
	data.cond.wait (data_lock);
    }
}

/**
 * @param trans	Opened transaction
 *
 * Close and discard the transaction, causes destruction of
 * all jobs currently contained in it and prevents their execution.
 * This function is MT-safe and may be called from any thread.
 */
void
bse_trans_dismiss (BseTrans *trans)
{
  assert_return (trans != NULL);
  assert_return (trans->comitted == FALSE);

  _engine_free_trans (trans);
}

/**
 * @param job  First job
 * @param ...  NULL terminated job list
 *
 * Convenience function which openes up a new transaction,
 * collects the NULL terminated job list passed to the function,
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
 * @param n_iostreams	number of input and output streams
 * @param user_data	user data, stored in module->user_data
 * @param free_data	function to free user_data when the module is discarded
 * @return		a newly created module
 *
 * Create a new virtual module which has @a n_iostreams input
 * streams and @a n_iostreams output streams. Simply put,
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
  assert_return (n_iostreams > 0, NULL);
  vclass = (VirtualModuleClass*) g_memdup (&virtual_module_class, sizeof (virtual_module_class));
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
  Bse::TaskRegistry::add ("DSP Slave", Bse::ThisThread::process_pid(), Bse::ThisThread::thread_pid());
  gboolean run = TRUE;
  while (run)
    {
      BseTrans *trans = bse_trans_open ();
      gchar *str = g_strdup_format ("SLAVE(%p): idle", g_thread_self ());
      bse_trans_add (trans, bse_job_debug (str));
      g_free (str);
      bse_trans_add (trans, bse_job_debug ("string2"));
      bse_trans_commit (trans);
      trans = bse_trans_open ();
      bse_trans_add (trans, bse_job_debug ("trans2"));
      bse_trans_commit (trans);
      g_usleep (1000*500);
    }
  Bse::TaskRegistry::remove (Bse::ThisThread::thread_pid());
}
/* --- setup & trigger --- */
static bool     bse_engine_initialized = false;
uint		bse_engine_exvar_sample_freq = 8000;
uint		bse_engine_exvar_block_size = 4096;
uint		bse_engine_exvar_control_mask = 4096 - 1;

/**
 * @param latency_ms	calculation latency in milli seconds
 * @param sample_freq	mixing frequency
 * @param control_freq	frequency at which to check control values or 0
 * @param block_size_p	location of number of values to process block wise
 * @param control_raster_p	location of number of values to skip between control values
 *
 * Calculate a suitable block size and control raster for a
 * @a sample_freq at a specific @a latency_ms (the latency should be > 0).
 * The @a control_freq if specified should me much smaller than the
 * @a sample_freq. It determines how often control values are to be
 * checked when calculating blocks of sample values.
 * The block size determines the amount by which the global tick
 * stamp (see Bse::TickStamp::current()) is updated everytime the whole
 * module network completed processing block size values.
 * This function is MT-safe and may be called prior to engine initialization.
 */
void
bse_engine_constrain (guint            latency_ms,
                      guint            sample_freq,
                      guint            control_freq,
                      guint           *block_size_p,
                      guint           *control_raster_p)
{
  assert_return (sample_freq >= 100);

  /* depending on how stable the overall system (cpu, kernel scheduler, etc.)
   * behaves, calculating a single block may take longer than expected,
   * block_jitter is meant to compensate for that. for an expected worst case
   * block calculation scenario, lasting 1.5 * block-playback-time, we choose
   * a suitable upper bound of 2 as ratio for block-calculation-time per
   * block-playback-time. if heavier jitter is to be expected, this value
   * should be increased (short of increasing overall latency, that is).
   */
  const guint block_jitter = 2; 
  /* constrain latency to avoid overflow */
  latency_ms = CLAMP (latency_ms, 1, 10000);
  /* derive block size from latency and sample frequency. for a perfect
   * capture->calc->playback setup, the playback time of a single block may
   * at most last latency/2 time. in practice, we need extra padding blocks,
   * which are accounted for by block_jitter.
   */
  guint block_size = latency_ms * sample_freq / 1000 / (1 + block_jitter);
  /* constrain block size */
  block_size = CLAMP (block_size, 8, MIN (BSE_STREAM_MAX_VALUES / 2, sample_freq / (2 * 3)));
  /* adjust block_size */
  if (0) /* shrink block size to a 2^n boundary */
    {
      guint tmp = sfi_alloc_upper_power2 (block_size);
      block_size = block_size < tmp ? tmp >> 1 : tmp;
    }
  else /* align block_size to 4 */
    block_size &= ~3;
  /* constrain control_freq */
  control_freq = MIN (control_freq, sample_freq);
  if (!control_freq)
    control_freq = (sample_freq + block_size - 1) / block_size;
  /* calc control stepping */
  guint control_raster = (sample_freq + control_freq - 1) / control_freq;
  /* control_raster > block_size doesn't make much sense */
  control_raster = CLAMP (control_raster, 1, block_size);
  /* shrink control_raster to a 2^n boundary */
  guint tmp = sfi_alloc_upper_power2 (control_raster);
  control_raster = control_raster < tmp ? tmp >> 1 : tmp;
  /* return values */
  if (block_size_p)
    *block_size_p = block_size;
  if (control_raster_p)
    *control_raster_p = control_raster;
}

/**
 * @param latency_ms	calculation latency in milli seconds
 * @param sample_freq	mixing frequency
 * @param control_freq	frequency at which to check control values or 0
 * @return      	whether reconfiguration was successful
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
  static std::condition_variable sync_cond;
  static std::mutex sync_mutex;
  static gboolean sync_lock = FALSE;
  guint block_size, control_raster, success = FALSE;
  BseTrans *trans;
  BseJob *job;
  assert_return (bse_engine_initialized == TRUE, FALSE);

  bse_engine_constrain (latency_ms, sample_freq, control_freq, &block_size, &control_raster);
  /* optimize */
  if (0 && block_size == bse_engine_block_size() && control_raster == bse_engine_control_raster())
    return TRUE;

  /* pseudo-sync first */
  bse_engine_wait_on_trans();
  /* paranoia checks */
  if (_engine_mnl_head() || sync_lock)
    return FALSE;

  /* block master */
  {
    std::unique_lock<std::mutex> sync_guard (sync_mutex);
    job = sfi_new_struct0 (BseJob, 1);
    job->job_id = ENGINE_JOB_SYNC;
    job->sync.lock_mutex = &sync_mutex;
    job->sync.lock_cond = &sync_cond;
    job->sync.lock_p = &sync_lock;
    sync_lock = FALSE;
    trans = bse_trans_open();
    bse_trans_add (trans, job);
    bse_trans_commit (trans);
    while (!sync_lock)
      sync_cond.wait (sync_guard);
  }
  if (!_engine_mnl_head())
    {
      /* cleanup */
      bse_engine_user_thread_collect();
      _engine_recycle_const_values (TRUE);
      /* adjust parameters */
      bse_engine_exvar_block_size = block_size;
      bse_engine_exvar_sample_freq = sample_freq;
      bse_engine_exvar_control_mask = control_raster - 1;
      /* fixup timer */
      Bse::TickStamp::_set_leap (bse_engine_block_size());
      Bse::TickStamp::_increment(); // ensure stamp validity (>0 and systime mark)
      success = TRUE;
    }

  /* unblock master */
  sync_mutex.lock();
  sync_lock = FALSE;
  sync_cond.notify_one();
  sync_mutex.unlock();
  /* ensure SYNC job got collected */
  bse_engine_wait_on_trans();
  bse_engine_user_thread_collect();
  if (success)
    EDEBUG ("configured%s: mixfreq=%uHz bsize=%uvals craster=%u (cfreq=%f)",
            "(threaded)",
            bse_engine_sample_freq(), bse_engine_block_size(), bse_engine_control_raster(),
            bse_engine_sample_freq() / (float) bse_engine_control_raster());
  return success;
}

/** Initialize the BSE audio processing engine.
 * This function must be called prior to any other engine related function and can only be invoked once.
 */
void
bse_engine_init()
{
  assert_return (bse_engine_initialized == FALSE);
  bse_engine_initialized = TRUE;
  /* assert correct implmentation of accessor macros defined in bsedefs.hh */
  assert_return (&BSE_MODULE_GET_USER_DATA ((BseModule*) 42) == &((BseModule*) 42)->user_data);
  assert_return (&BSE_MODULE_GET_ISTREAMSP ((BseModule*) 42) == (void*) &((BseModule*) 42)->istreams);
  assert_return (&BSE_MODULE_GET_JSTREAMSP ((BseModule*) 42) == (void*) &((BseModule*) 42)->jstreams);
  assert_return (&BSE_MODULE_GET_OSTREAMSP ((BseModule*) 42) == (void*) &((BseModule*) 42)->ostreams);
  /* setup threading */
  Bse::MasterThread::start (bse_main_wakeup);
  /* first configure */
  bse_engine_configure (50, 44100, 50);
  (void) slave; // FIXME: start slave ("DSP #2")
}

gboolean
bse_engine_prepare (BseEngineLoop *loop)
{
  assert_return (loop != NULL, FALSE);
  assert_return (bse_engine_initialized == TRUE, FALSE);
  loop->timeout = -1;
  loop->fds_changed = FALSE;
  loop->n_fds = 0;
  loop->revents_filled = FALSE;
  return bse_engine_has_garbage ();
}
gboolean
bse_engine_check (const BseEngineLoop *loop)
{
  assert_return (loop != NULL, FALSE);
  if (loop->n_fds)
    assert_return (loop->revents_filled == TRUE, FALSE);
  return bse_engine_has_garbage ();
}

/**
 *
 * Perform necessary work the engine has to handle
 * in the user thread.
 * This function may only be called from the user thread,
 * since it will invoke BseFreeFunc() functions (see
 * bse_engine_user_thread_collect()) and do
 * other things which are guranteed to be executed
 * in the user thread.
 */
void
bse_engine_dispatch (void)
{
  assert_return (bse_engine_initialized == TRUE);
  if (bse_engine_has_garbage ())	/* prevent extra mutex locking */
    bse_engine_user_thread_collect ();
}

/**
 * @param systime	System time in micro seconds.
 * @return		Engine tick stamp value
 *
 * Depending on the engine's sample frequency and the time
 * of the last global tick stamp update, calculate the
 * corresponding engine tick stamp from a given system time.
 * This function is MT-safe and may be called from any thread.
 */
guint64
bse_engine_tick_stamp_from_systime (guint64 systime)
{
  Bse::TickStamp::Update ustamp = Bse::TickStamp::get_last ();
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
    Bse::warning ("tick_stamp conversion problem:\n"
                  "  tick_stamp            = %llu\n"
                  "  usec_systime          = %llu\n"
                  "  current-systime       = %llu\n"
                  "  current-tickstamp     = %llu\n"
                  "  last-update-systime   = %llu\n"
                  "  last-update-tickstamp = %llu\n"
                  "  sample-freq           = %u\n",
                  tick_stamp, systime, sfi_time_system (), Bse::TickStamp::get_current (),
                  ustamp.system_time, ustamp.tick_stamp,
                  bse_engine_sample_freq ());
#endif
  return tick_stamp;
}

/**
 *
 * Wait until all pending transactions have been processed
 * by the BSE Engine. This function, when done waiting, will
 * run a garbage collection cycle before returning.
 * See bse_engine_user_thread_collect(), the same restrictions
 * apply to invokations of this function.
 */
void
bse_engine_wait_on_trans (void)
{
  assert_return (bse_engine_initialized == TRUE);

  /* threaded */
  _engine_wait_on_trans ();

  /* call all free() functions */
  bse_engine_user_thread_collect ();
}

/* vim:set ts=8 sts=2 sw=2: */
