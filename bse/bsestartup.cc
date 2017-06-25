// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsestartup.hh"
#include "bsemain.hh"
#include <bse/bseclientapi.hh>
#include <bse/bse.hh>           // init_server_connection
#include "../config/config.h"   // BST_VERSION

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
    assert_return (connection != NULL, NULL);
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

static Rapicorn::Aida::ClientConnectionP *client_connection = NULL;

/// Retrieve a handle for the Bse::Server instance managing the Bse thread.
ServerHandle
init_server_instance () // bse.hh
{
  ServerH server;
  Rapicorn::Aida::ClientConnectionP connection = init_server_connection();
  if (connection)
    server = connection->remote_origin<ServerH>();
  return server;
}

/// Retrieve the ClientConnection used for RPC communication with the Bse thread.
Rapicorn::Aida::ClientConnectionP
init_server_connection () // bse.hh
{
  if (!client_connection)
    {
      using namespace Rapicorn::Aida;
      ClientConnectionP connection = ClientConnection::connect ("inproc://BSE-" BST_VERSION);
      ServerH server;
      if (connection)
        server = connection->remote_origin<ServerH>();
      if (!server) // shouldn't happen
        sfi_error ("%s: failed to establish BSE connection: %s", __func__, g_strerror (errno));
      constexpr SfiProxy BSE_SERVER = 1;
      assert_return (server.proxy_id() == BSE_SERVER, NULL);
      assert_return (server.from_proxy (BSE_SERVER) == server, NULL);
      assert_return (client_connection == NULL, NULL);
      client_connection = new Rapicorn::Aida::ClientConnectionP (connection);
    }
  return *client_connection;
}

} // Bse

#include "bseclientapi.cc"      // build IDL client interface
