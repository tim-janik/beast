// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsecore.hh"

namespace Bse {

// == TaskRegistry ==
static Rapicorn::Mutex    task_registry_mutex_;
static TaskRegistry::List task_registry_tasks_;

void
TaskRegistry::add (const std::string &name, int pid, int tid)
{
  Rapicorn::TaskStatus task (pid, tid);
  task.name = name;
  task.update();
  Rapicorn::ScopedLock<Rapicorn::Mutex> locker (task_registry_mutex_);
  task_registry_tasks_.push_back (task);
}

bool
TaskRegistry::remove (int tid)
{
  Rapicorn::ScopedLock<Rapicorn::Mutex> locker (task_registry_mutex_);
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
  Rapicorn::ScopedLock<Rapicorn::Mutex> locker (task_registry_mutex_);
  for (auto &task : task_registry_tasks_)
    task.update();
}

TaskRegistry::List
TaskRegistry::list ()
{
  Rapicorn::ScopedLock<Rapicorn::Mutex> locker (task_registry_mutex_);
  return task_registry_tasks_;
}

} // Bse
