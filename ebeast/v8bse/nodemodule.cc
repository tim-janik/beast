// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "../bse.hh"
#include "v8bse.cc"
#include <node.h>
#include <uv.h>

// v8pp binding for Bse
static V8stub *bse_v8stub = NULL;

// event loop integration
static uv_poll_t                         bse_uv_watcher;
static Rapicorn::Aida::ClientConnectionP bse_client_connection;
static Bse::ServerH                      bse_server;

// register bindings and start Bse
static void
v8bse_register_module (v8::Local<v8::Object> exports)
{
  assert (bse_v8stub == NULL);
  v8::Isolate *const isolate = v8::Isolate::GetCurrent();
  v8::HandleScope scope (isolate);

  // start Bse
  Bse::String bseoptions = Bse::string_format ("debug-extensions=%d", 0);
  Bse::init_async (NULL, NULL, "BEAST", Bse::string_split (bseoptions, ":"));

  // fetch server handle
  assert (bse_server == NULL);
  assert (bse_client_connection == NULL);
  bse_client_connection = Bse::init_server_connection();
  assert (bse_client_connection != NULL);
  bse_server = Bse::init_server_instance();
  assert (bse_server != NULL);

  // hook BSE connection into libuv event loop
  uv_loop_t *loop = uv_default_loop();
  uv_poll_init (loop, &bse_uv_watcher, bse_client_connection->notify_fd());
  auto bse_uv_callback = [] (uv_poll_t *watcher, int status, int revents) {
    if (bse_client_connection && bse_client_connection->pending())
      bse_client_connection->dispatch();
  };
  uv_poll_start (&bse_uv_watcher, UV_READABLE, bse_uv_callback);

  // hook BSE connection into GLib event loop
  Bse::AidaGlibSource *source = Bse::AidaGlibSource::create (bse_client_connection.get());
  g_source_set_priority (source, G_PRIORITY_DEFAULT);
  g_source_attach (source, g_main_context_default());

  // register v8stub
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  bse_v8stub = new V8stub (isolate);
  v8::Local<v8::Object> module_instance = bse_v8stub->module_.new_instance();
  v8::Maybe<bool> ok = exports->SetPrototype (context, module_instance);
  assert (ok.FromJust() == true);
}

// node.js registration
NODE_MODULE (v8bse, v8bse_register_module);
