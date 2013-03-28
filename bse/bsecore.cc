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
static Bse::Mutex         task_registry_mutex_;
static TaskRegistry::List task_registry_tasks_;

void
TaskRegistry::add (const std::string &name, int pid, int tid)
{
  Rapicorn::TaskStatus task (pid, tid);
  task.name = name;
  task.update();
  Bse::ScopedLock<Bse::Mutex> locker (task_registry_mutex_);
  task_registry_tasks_.push_back (task);
}

bool
TaskRegistry::remove (int tid)
{
  Bse::ScopedLock<Bse::Mutex> locker (task_registry_mutex_);
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
  Bse::ScopedLock<Bse::Mutex> locker (task_registry_mutex_);
  for (auto &task : task_registry_tasks_)
    task.update();
}

TaskRegistry::List
TaskRegistry::list ()
{
  Bse::ScopedLock<Bse::Mutex> locker (task_registry_mutex_);
  return task_registry_tasks_;
}

class AidaGlibSourceImpl : public AidaGlibSource {
  static AidaGlibSourceImpl* self_         (GSource *src)                     { return (AidaGlibSourceImpl*) src; }
  static int                 glib_prepare  (GSource *src, int *timeoutp)      { return self_ (src)->prepare (timeoutp); }
  static int                 glib_check    (GSource *src)                     { return self_ (src)->check(); }
  static int                 glib_dispatch (GSource *src, GSourceFunc, void*) { return self_ (src)->dispatch(); }
  static void                glib_finalize (GSource *src)                     { self_ (src)->~AidaGlibSourceImpl(); }
  Rapicorn::Aida::BaseConnection *connection_;
  GPollFD                         pfd_;
  AidaGlibSourceImpl (Rapicorn::Aida::BaseConnection *connection) :
    connection_ (connection), pfd_ { -1, 0, 0 }
  {
    pfd_.fd = connection_->notify_fd();
    pfd_.events = G_IO_IN;
    g_source_add_poll (this, &pfd_);
  }
  ~AidaGlibSourceImpl ()
  {
    g_source_remove_poll (this, &pfd_);
  }
  bool
  prepare (int *timeoutp)
  {
    return pfd_.revents || connection_->pending();
  }
  bool
  check ()
  {
    return pfd_.revents || connection_->pending();
  }
  bool
  dispatch ()
  {
    pfd_.revents = 0;
    connection_->dispatch();
    return true;
  }
public:
  static AidaGlibSourceImpl*
  create (Rapicorn::Aida::BaseConnection *connection)
  {
    AIDA_ASSERT (connection != NULL);
    static GSourceFuncs glib_source_funcs = { glib_prepare, glib_check, glib_dispatch, glib_finalize, NULL, NULL };
    GSource *src = g_source_new (&glib_source_funcs, sizeof (AidaGlibSourceImpl));
    return new (src) AidaGlibSourceImpl (connection);
  }
};

AidaGlibSource*
AidaGlibSource::create (Rapicorn::Aida::BaseConnection *connection)
{
  return AidaGlibSourceImpl::create (connection);
}

} // Bse
