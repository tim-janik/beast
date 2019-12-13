// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsestartup.hh"
#include "bsemain.hh"
#include "bse/internal.hh"
#include "bse/bseserver.hh"
#include <bse/bse.hh>

namespace Bse {

// == BSE Initialization ==

/** Initialize and start BSE.
 * Initialize the BSE library and start the main BSE thread. Arguments specific to BSE are removed
 * from @a argc / @a argv.
 */
void
init_async (int *argc, char **argv, const char *app_name, const StringVector &args)
{
  _bse_init_async (argc, argv, app_name, args);
}

/// Check wether init_async() still needs to be called.
bool
init_needed ()
{
  return _bse_initialized() == false;
}

// == TaskRegistry ==
static std::mutex         task_registry_mutex_;
static TaskRegistry::List task_registry_tasks_;

void
TaskRegistry::add (const std::string &name, int pid, int tid)
{
  Bse::TaskStatus task (pid, tid);
  task.name = name;
  task.update();
  std::lock_guard<std::mutex> locker (task_registry_mutex_);
  task_registry_tasks_.push_back (task);
}

bool
TaskRegistry::remove (int tid)
{
  std::lock_guard<std::mutex> locker (task_registry_mutex_);
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
  std::lock_guard<std::mutex> locker (task_registry_mutex_);
  for (auto &task : task_registry_tasks_)
    task.update();
}

TaskRegistry::List
TaskRegistry::list ()
{
  std::lock_guard<std::mutex> locker (task_registry_mutex_);
  return task_registry_tasks_;
}

/// Retrieve a handle for the Bse::Server instance managing the Bse thread.
#if 0 // FIXME
ServerHandle
init_server_instance () // bse.hh
{
  ServerH server;
  server = BSE_SERVER.__handle__();
  return server;
}
#endif

} // Bse

// #include "bse/bseapi_handles.cc"        // build IDL client interface
