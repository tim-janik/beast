// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsecore.hh"
#include "bsemain.hh"

namespace Bse {

// == BSE Initialization ==

/** Create SFI glue layer context.
 * Create and push an SFI glue layer context for the calling thread, to enable communications with the
 * main BSE thread library.
 */
SfiGlueContext*
init_glue_context (const gchar *client, const std::function<void()> &caller_wakeup)
{
  return _bse_glue_context_create (client, caller_wakeup);
}

/** Initialize and start BSE.
 * Initialize the BSE library and start the main BSE thread. Arguments specific to BSE are removed
 * from @a argc / @a argv.
 */
void
init_async (int *argc, char ***argv, const char *app_name, SfiInitValue values[])
{
  _bse_init_async (argc, argv, app_name, values);
}

// == TaskRegistry ==
static Rapicorn::Mutex    task_registry_mutex_;
static TaskRegistry::List task_registry_tasks_;

void
TaskRegistry::add (const std::string &name, int pid, int tid)
{
  Rapicorn::TaskStatus task (pid, tid);
  task.name = name;
  task.update();
  Bse::ScopedLock<Rapicorn::Mutex> locker (task_registry_mutex_);
  task_registry_tasks_.push_back (task);
}

bool
TaskRegistry::remove (int tid)
{
  Bse::ScopedLock<Rapicorn::Mutex> locker (task_registry_mutex_);
  for (auto it = task_registry_tasks_.begin(); it != task_registry_tasks_.end(); it++)
    if (it->task_id == tid)
      {
        task_registry_tasks_.erase (it);
        return true;
      }
  return false;
}

void
TaskRegistry::update ()
{
  Bse::ScopedLock<Rapicorn::Mutex> locker (task_registry_mutex_);
  for (auto &task : task_registry_tasks_)
    task.update();
}

TaskRegistry::List
TaskRegistry::list ()
{
  Bse::ScopedLock<Rapicorn::Mutex> locker (task_registry_mutex_);
  return task_registry_tasks_;
}

} // Bse
