// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "../../bse/bse.hh"
#include <node.h>
#include <uv.h>
#include <v8pp/class.hpp>
#include <v8pp/convert.hpp>

// == RemoteHandle Wrapping ==
/* NOTE: A RemoteHandle is a smart pointer to a complex C++ object (possibly behind a thread boundary),
 * so multiple RemoteHandle objects can point to the same C++ object. There are a couple ways that a
 * RemoteHandle can be mapped onto Javascript:
 * 1) A v8::Object contains a RemoteHandle, i.e. every function that returns a new RemoteHandle also
 *    returns a new Object in JS, even if they all point to the same C++ impl. In this case it is
 *    desirable that the JS Object cannot gain new properties (i.e. isSealed() === true).
 *    A major downside here is that two JS Objects that point to the same C++ impl are not === equal.
 * 2) All RemoteHandles that point to the same C++ impl are mapped onto a single JS Object of the
 *    appropriate down-cast type. This correctly provides === equality and new properties added to
 *    a JS Object are preseved whenever another RemoteHandles is mapped into a JS Object that points
 *    to the same C++ impl.
 *    Here, an extra map must be maintained to achieve the (n RemoteHandle) => (1 JS Object) mapping
 *    by storing and looking up the orbid_ that defines the object identity each RemoteHandle points to.
 *    The downside here is resource lockup. Once created, a JS Object must keep its RemoteHandle around
 *    which forces the C++ impl to stay alive. And the v8::Persistent holding the JS Object map entry
 *    must not be weak to prevent GC cycles from "forgetting" the properties stored on the JS Object.
 * 3) A viable middle ground might be using the map from (2) so a JS Object provides correct equality
 *    via ===, but the JS Objects are sealed as in (1) so we can use SetWeak persistents for the map
 *    to avoid resource leaks.
 */

/// Function to wrap a RemoteHandle derived type via v8pp::class_<>::import_external().
typedef v8::Local<v8::Object> (*AidaRemoteHandleWrapper) (v8::Isolate *const, Aida::RemoteHandle);

/// Default implementation to wrap a RemoteHandle derived type via v8pp::class_<>::import_external().
template<class Native> static v8::Local<v8::Object>
aida_remote_handle_wrapper_impl (v8::Isolate *const isolate, Aida::RemoteHandle rhandle)
{
  Native target = Native::down_cast (rhandle);
  if (target != NULL)
    return v8pp::class_<Native>::import_external (isolate, new Native (target));
  return v8::Local<v8::Object>();
}

/// Map Aida type ids to the corresponding AidaRemoteHandleWrapper.
static AidaRemoteHandleWrapper
aida_remote_handle_wrapper_map (const Aida::TypeHash &thash, AidaRemoteHandleWrapper newfunc)
{
  static std::map<Aida::TypeHash, AidaRemoteHandleWrapper> wmap;
  if (!newfunc)
    return wmap[thash];
  wmap[thash] = newfunc;
  return NULL;
}

/// Retrieve the native RemoteHandle from a JS Object.
template<class NativeClass> static NativeClass*
aida_remote_handle_unwrap_native (v8::Isolate *const isolate, v8::Local<v8::Value> value)
{
  v8::HandleScope scope (isolate);
  NativeClass *nobject = NULL;
  if (!value.IsEmpty() && value->IsObject())
    nobject = v8pp::class_<NativeClass>::unwrap_object (isolate, value);
  if (!nobject)
    throw std::runtime_error ("failed to unwrap C++ Aida::RemoteHandle");
  return nobject;
}

static v8::Isolate                                               *aida_remote_handle_idmap_isolate = NULL;
static std::unordered_map<uint64_t, v8pp::persistent<v8::Object>> aida_remote_handle_idmap;

static void
aida_remote_handle_cache_add (v8::Isolate *const isolate, const Aida::RemoteHandle &rhandle, const v8::Local<v8::Object> &wrapobj)
{
  assert_return (isolate == aida_remote_handle_idmap_isolate);
  // check handle consistency
  Aida::RemoteHandle *whandle = aida_remote_handle_unwrap_native<Aida::RemoteHandle> (isolate, wrapobj);
  assert_return (whandle && whandle->__aida_orbid__() == rhandle.__aida_orbid__());
  // seal object, since property extensions could not survive GC
  wrapobj->SetIntegrityLevel (isolate->GetCurrentContext(), v8::IntegrityLevel::kSealed);
  // use v8::UniquePersistent to keep a unique v8::Object per OrbObject around
  v8pp::persistent<v8::Object> po (isolate, wrapobj);
  // get rid of the unique v8::Object once all JS code forgot about its identity
  auto weak_callback = [] (const v8::WeakCallbackInfo<Aida::RemoteHandle> &data) {
    // v8::Isolate *const isolate = data.GetIsolate();
    Aida::RemoteHandle *whandle = data.GetParameter();
    const uint64_t orbid = whandle->__aida_orbid__();
    auto it = aida_remote_handle_idmap.find (orbid);
    if (it != aida_remote_handle_idmap.end())
      {
        it->second.Reset();
        aida_remote_handle_idmap.erase (it);
      }
  };
  po.SetWeak (whandle, weak_callback, v8::WeakCallbackType::kParameter);
  // enter per-isolate cache
  aida_remote_handle_idmap.emplace (rhandle.__aida_orbid__(), std::move (po));
}

static v8::Local<v8::Object>
aida_remote_handle_cache_find (v8::Isolate *const isolate, const Aida::RemoteHandle &rhandle)
{
  v8::Local<v8::Object> result;
  assert_return (isolate == aida_remote_handle_idmap_isolate, result);
  auto it = aida_remote_handle_idmap.find (rhandle.__aida_orbid__());
  if (it != aida_remote_handle_idmap.end())
    result = v8pp::to_local (isolate, it->second);
  return result;
}

/// Create (or find) the corresponding down_cast() JS Object for a RemoteHandle.
static v8::Local<v8::Object>
aida_remote_handle_wrap_native (v8::Isolate *const isolate, const Aida::RemoteHandle &rhandle)
{
  v8::EscapableHandleScope scope (isolate);
  v8::Local<v8::Object> wrapobj;
  if (AIDA_LIKELY (NULL != rhandle))
    {
      wrapobj = aida_remote_handle_cache_find (isolate, rhandle);
      if (AIDA_LIKELY (!wrapobj.IsEmpty()))
        return scope.Escape (wrapobj);
    }
  Aida::TypeHashList thl = rhandle.__aida_typelist__();
  for (const auto &th : thl)
    {
      AidaRemoteHandleWrapper wrapper = aida_remote_handle_wrapper_map (th, AidaRemoteHandleWrapper (NULL));
      if (wrapper)
        {
          wrapobj = wrapper (isolate, rhandle);
          aida_remote_handle_cache_add (isolate, rhandle, wrapobj);
          break;
        }
    }
  return scope.Escape (wrapobj);
}

/// Helper to specialize v8pp::convert<> for all RemoteHandle types.
template<class DerivedHandle>
struct convert_AidaRemoteHandle
{
  using N = DerivedHandle;              // native type, derived from Aida::RemoteHandle
  using J = v8::Local<v8::Object>;      // Javascript type
  static bool is_valid (v8::Isolate *const isolate, v8::Local<v8::Value> v) { return !v.IsEmpty() && v->IsObject(); }
  static N&   from_v8  (v8::Isolate *const isolate, v8::Local<v8::Value> v) { return *aida_remote_handle_unwrap_native<N> (isolate, v); }
  static J    to_v8    (v8::Isolate *const isolate, const N &rhandle)       { return aida_remote_handle_wrap_native (isolate, rhandle); }
};

/// Helper for convert_AidaRemoteHandle pointer types.
template<class DerivedHandle>
struct convert_AidaRemoteHandle<DerivedHandle*>
{
  using N = DerivedHandle;              // native type, derived from Aida::RemoteHandle
  using J = v8::Local<v8::Object>;      // Javascript type
  static bool is_valid (v8::Isolate *const isolate, v8::Local<v8::Value> v) { return v8pp::convert<N>::is_valid (isolate, v); }
  static N*   from_v8  (v8::Isolate *const isolate, v8::Local<v8::Value> v) { return &v8pp::convert<N>::from_v8 (isolate, v); }
  static J    to_v8    (v8::Isolate *const isolate, const N *n)             { return !n ? J() : v8pp::convert<N>::to_v8 (isolate, *n); }
};

/// Helper to specialize v8pp::convert<> for all Sequence types.
template<class Class>
struct convert_AidaSequence
{
  using from_type = Class;
  using value_type = typename Class::value_type;
  using to_type = v8::Handle<v8::Array>;
  static bool
  is_valid (v8::Isolate*, v8::Handle<v8::Value> value)
  {
    return !value.IsEmpty() && value->IsArray();
  }
  static from_type
  from_v8 (v8::Isolate *const isolate, v8::Handle<v8::Value> value)
  {
    v8::HandleScope scope (isolate);
    if (!is_valid (isolate, value))
      throw std::invalid_argument ("expected array object");
    v8::Local<v8::Array> arr = value.As<v8::Array>();
    const size_t arrlen = arr->Length();
    from_type result;
    result.reserve (arrlen);
    for (size_t i = 0; i < arrlen; i++)
      result.push_back (v8pp::from_v8<value_type> (isolate, arr->Get (i)));
    return result;
  }
  static to_type
  to_v8 (v8::Isolate *const isolate, from_type const &value)
  {
    v8::EscapableHandleScope scope (isolate);
    v8::Local<v8::Array> arr = v8::Array::New (isolate, value.size());
    for (size_t i = 0; i < value.size(); i++)
      arr->Set (i, v8pp::to_v8 (isolate, value[i]));
    return scope.Escape (arr);
  }
};

#include "v8bse.cc"

// v8pp binding for Bse
static V8stub *bse_v8stub = NULL;

// event loop integration
static uv_poll_t               bse_uv_watcher;
static Aida::ClientConnectionP bse_client_connection;
static Bse::ServerH            bse_server;
static uv_async_t              bse_uv_dispatcher;
static uv_prepare_t            bse_uv_preparer;

// register bindings and start Bse
static void
v8bse_register_module (v8::Local<v8::Object> exports)
{
  assert (bse_v8stub == NULL);
  v8::Isolate *const isolate = v8::Isolate::GetCurrent();
  assert (aida_remote_handle_idmap_isolate == NULL);
  aida_remote_handle_idmap_isolate = isolate;

  v8::HandleScope scope (isolate);

  // workaround electron appending argv[1:] to argv[0]
  if (Bse::program_alias().find ("electron ") != std::string::npos)
    Bse::program_alias_init (Bse::Path::cwd()); // a guess at the actual electron application

  // prepare Bse environment
  const char *canary = "library/demo/partymonster.bse";
  const std::string installpath = Bse::Path::realpath (Bse::Path::abspath ("..")); // ebeast/..
  if (!Bse::Path::check (Bse::Path::join (installpath, canary), "r"))
    Bse::fatal_error ("%s: BSE: failed to locate library containing '%s'", installpath, canary);
  Bse::installpath_override (installpath);

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
  uv_loop_t *uvloop = uv_default_loop();
  // Dispatch any pending events from uvlooop
  auto uvdispatchcb = [] (uv_async_t*) {
    if (bse_client_connection)
      while (bse_client_connection->pending())
        bse_client_connection->dispatch();
  };
  uv_async_init (uvloop, &bse_uv_dispatcher, uvdispatchcb);
  // Poll notify_fd, clear fd and queue dispatcher events
  auto uvpollcb = [] (uv_poll_t*, int, int) {
    if (bse_client_connection && bse_client_connection->pending())
      uv_async_send (&bse_uv_dispatcher);
  };
  uv_poll_init (uvloop, &bse_uv_watcher, bse_client_connection->notify_fd());
  uv_poll_start (&bse_uv_watcher, UV_READABLE, uvpollcb);
  // Prevent libuv from waiting in poll if events are pending
  auto uvpreparecb = [] (uv_prepare_t*) {
    if (bse_client_connection && bse_client_connection->pending())
      uv_async_send (&bse_uv_dispatcher);
  };
  uv_prepare_init (uvloop, &bse_uv_preparer);
  uv_prepare_start (&bse_uv_preparer, uvpreparecb);
  /* Electron drives the uvloop via UV_RUN_NOWAIT and outsources fd polling into
   * a dedicated worker thread. And that means the bse_client_connection may be
   * dispatching calls *and* fetching remote events or return values without
   * notify_fd getting a chance to wakeup poll(2). So we use a notify_callback
   * to check for pending events after each remote call.
   */
  auto bsenotfycb = [] (Rapicorn::Aida::ClientConnection &con) {
    // bse_client_connection == &con
    if (bse_client_connection && bse_client_connection->pending())
      uv_async_send (&bse_uv_dispatcher);
  };
  bse_client_connection->notify_callback (bsenotfycb);

  // register v8stub C++ bindings
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  bse_v8stub = new V8stub (isolate);
  v8::Local<v8::Object> module_instance = bse_v8stub->module_.new_instance();
  v8::Maybe<bool> ok = exports->SetPrototype (context, module_instance);
  assert (ok.FromJust() == true);

  // export server handle
  V8ppType_BseServer &class_ = bse_v8stub->BseServer_class_;
  v8::Local<v8::Object> v8_server = class_.import_external (isolate, new Bse::ServerH (bse_server));
  module_instance->DefineOwnProperty (context, v8pp::to_v8 (isolate, "server"),
                                      v8_server, v8::PropertyAttribute (v8::ReadOnly | v8::DontDelete));

  // execute v8stub javascript initialization
  bse_v8stub->jsinit (context, exports);

  // debugging aids:
  if (0)
    Bse::printerr ("gdb %s %u -ex 'catch catch' -ex 'catch throw'\n", Bse::string_split (program_invocation_name, " ", 1)[0], Bse::this_thread_getpid());

  // Ensure Bse has everything properly loaded
  bse_server.load_assets();
}

// node.js registration
NODE_MODULE (v8bse, v8bse_register_module);
