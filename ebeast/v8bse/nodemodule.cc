// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bse/bse.hh"
#include "bse/internal.hh"
#include <math.h>
#include <node.h>
#include <uv.h>
#include <v8pp/class.hpp>
#include <v8pp/convert.hpp>

// == RemoteHandle Wrapping ==
/* NOTE: A RemoteHandle is a smart pointer to a complex C++ object (possibly in another thread),
 * so multiple RemoteHandle objects can point to the same C++ object. There are a couple ways that a
 * RemoteHandle can be mapped onto Javascript:
 * 1) A v8::Object contains a RemoteHandle, i.e. every function that returns a new RemoteHandle also
 *    returns a new Object in JS, even if they all point to the same C++ impl. In this case it is
 *    desirable that the JS Object must not accept new properties (i.e. isSealed() === true).
 *    A major downside here is that two JS Objects that point to the same C++ impl are not === equal.
 * 2) All RemoteHandles that point to the same C++ impl are mapped onto a single JS Object of the
 *    appropriate down-cast type. This correctly provides === equality and new properties added to
 *    a JS Object are preseved whenever another RemoteHandles is mapped into a JS Object that points
 *    to the same C++ impl.
 *    Here, an extra map must be maintained to achieve the (n RemoteHandle) => (1 JS Object) mapping
 *    by storing and looking up a unique id that defines the object identity each RemoteHandle points to.
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
  Native target = Native::__cast__ (rhandle);
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
static std::unordered_map<ptrdiff_t, v8pp::persistent<v8::Object>> aida_remote_handle_idmap;

static void
aida_remote_handle_cache_add (v8::Isolate *const isolate, const Aida::RemoteHandle &rhandle, const v8::Local<v8::Object> &wrapobj)
{
  assert_return (isolate == aida_remote_handle_idmap_isolate);
  // check handle consistency
  Aida::RemoteHandle *whandle = aida_remote_handle_unwrap_native<Aida::RemoteHandle> (isolate, wrapobj);
  const ptrdiff_t rhandle_ptrid = ptrdiff_t (const_cast<Aida::RemoteHandle&> (rhandle).__iface_ptr__().get());
  assert_return (whandle && ptrdiff_t (whandle->__iface_ptr__().get()) == rhandle_ptrid);
  // seal object, since property extensions could not survive GC
  wrapobj->SetIntegrityLevel (isolate->GetCurrentContext(), v8::IntegrityLevel::kSealed);
  // use v8::UniquePersistent to keep a unique v8::Object per OrbObject around
  v8pp::persistent<v8::Object> po (isolate, wrapobj);
  // get rid of the unique v8::Object once all JS code forgot about its identity
  auto weak_callback = [] (const v8::WeakCallbackInfo<Aida::RemoteHandle> &data) {
    // v8::Isolate *const isolate = data.GetIsolate();
    Aida::RemoteHandle *whandle = data.GetParameter();
    const ptrdiff_t ptrid = ptrdiff_t (whandle->__iface_ptr__().get());
    auto it = aida_remote_handle_idmap.find (ptrid);
    if (it != aida_remote_handle_idmap.end())
      {
        it->second.Reset();
        aida_remote_handle_idmap.erase (it);
        // Bse::printerr ("NOTE:%s: clearing handle=%p (tid=%d)\n", __func__, (void*) ptrid, Bse::this_thread_gettid());
      }
  };
  po.SetWeak (whandle, weak_callback, v8::WeakCallbackType::kParameter);
  // enter per-isolate cache
  aida_remote_handle_idmap.emplace (rhandle_ptrid, std::move (po));
}

static v8::Local<v8::Object>
aida_remote_handle_cache_find (v8::Isolate *const isolate, const Aida::RemoteHandle &rhandle)
{
  v8::Local<v8::Object> result;
  assert_return (isolate == aida_remote_handle_idmap_isolate, result);
  const ptrdiff_t rhandle_ptrid = ptrdiff_t (const_cast<Aida::RemoteHandle&> (rhandle).__iface_ptr__().get());
  auto it = aida_remote_handle_idmap.find (rhandle_ptrid);
  if (it != aida_remote_handle_idmap.end())
    result = v8pp::to_local (isolate, it->second);
  return result;
}

/// Create (or find) the corresponding __cast__() JS Object for a RemoteHandle.
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
  Aida::TypeHashList thl = rhandle.__typelist__();
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

static v8::Local<v8::Value>     any_to_v8   (v8::Isolate*, const Aida::Any&);
static Aida::Any                any_from_v8 (v8::Isolate*, v8::Local<v8::Value>&);

struct convert_AidaAny {
  using to_type = v8::Local<v8::Value>; // Javascript type
  using from_type = Aida::Any;          // native C++ type
  static bool
  is_valid (v8::Isolate *const iso, v8::Local<v8::Value> v)
  {
    if (v.IsEmpty())
      return false;
    /* IsUndefined IsNull IsNullOrUndefined IsTrue IsFalse IsName IsSymbol IsFunction IsArray
     * IsExternal IsDate IsArgumentsObject IseanObject IsNumberObject IsStringObject IsSymbolObject
     * IsNativeError IsRegExp IsAsyncFunction IsGeneratorFunction IsGeneratorObject IsPromise IsMap
     * IsSet IsMapIterator IsSetIterator IsWeakMap IsWeakSet IsArrayBuffer IsArrayBufferView
     * IsTypedArray IsUint8Array IsUint8ClampedArray IsInt8Array IsUint16Array IsInt16Array
     * IsUint32Array IsInt32Array IsFloat32Array IsFloat64Array IsDataView IsSharedArrayBuffer
     * IsProxy IsWebAssemblyCompiledModule
     */
    if (v->IsBoolean())         return true; // Aida::BOOL
    if (v->IsInt32())           return true; // Aida::INT32
    if (v->IsUint32())          return true; // Aida::INT64
    if (v->IsNumber())          return true; // Aida::FLOAT64
    if (v->IsString())          return true; // Aida::STRING
    if (v->IsObject())          return true; // Aida::RECORD
    return false;
  }
  static from_type
  from_v8 (v8::Isolate *const iso, v8::Local<v8::Value> v)
  {
    Aida::Any a;
    if      (v->IsBoolean())    a.set<bool> (v->BooleanValue());
    else if (v->IsInt32())      a.set<int32_t> (v->Int32Value());
    else if (v->IsUint32())     a.set<int64_t> (v->Uint32Value());
    else if (v->IsNumber())     a.set<double> (v->NumberValue());
    else if (v->IsString())     a.set<std::string> (v8pp::from_v8<std::string> (iso, v));
    else if (v->IsObject())     any_from_v8object (iso, a, v.As<v8::Object>());
    return a;
  }
  static to_type
  to_v8 (v8::Isolate *const iso, const from_type &a)
  {
    switch (int (a.kind()))
      {
        int64_t big;
      case Aida::BOOL:          return v8::Boolean::New (iso, a.get<bool>());
      case Aida::INT32:         return v8::Integer::New (iso, a.get<int32_t>());
      case Aida::FLOAT64:       return v8::Number::New (iso, a.get<double>());
      case Aida::STRING:        return v8pp::to_v8 (iso, a.get<std::string>());
      case Aida::RECORD:        return any_to_v8object (iso, a);
      case Aida::INT64:
        big = a.get<int64_t>();
        if (big >= -2147483648 && big <= +2147483647)
          return v8::Integer::New (iso, big);
        if (big >= 0 && big <= +4294967295)
          return v8::Integer::NewFromUnsigned (iso, big);
        if (big >= -9007199254740992 && big <= +9007199254740992)
          return v8::Number::New (iso, big);
        Bse::warning ("converting Aida::Any exceeds v8::Number precision: %d", big);
        return v8::Number::New (iso, big);
      }
    return v8::Undefined (iso);
  }
  static void
  any_from_v8object (v8::Isolate *const iso, Aida::Any &a, v8::Local<v8::Object> object)
  {
    v8::Local<v8::Array> prop_names = object->GetPropertyNames();
    Aida::AnyRec r;
    const size_t l = prop_names->Length();
    for (size_t i = 0; i < l; i++)
      {
        v8::Local<v8::Value> v8key = prop_names->Get (i);
        v8::Local<v8::Value> v8val = object->Get (v8key);
        const std::string key = v8pp::from_v8<std::string> (iso, v8key);
        if (key.empty())
          continue;
        r[key] = any_from_v8 (iso, v8val);
      }
    a.set (r);
  }
  static to_type
  any_to_v8object (v8::Isolate *const iso, const from_type &a)
  {
    v8::EscapableHandleScope v8scope (iso);
    v8::Local<v8::Object> o = v8::Object::New (iso);
    const Aida::AnyRec *r = a.get<const Aida::AnyRec*>();
    if (r)
      for (auto const &field : *r)
        {
          v8::Local<v8::Value> v = any_to_v8 (iso, field);
          o->Set (v8pp::to_v8 (iso, field.name), v);
        }
    return v8scope.Escape (o);
  }
};

namespace v8pp {
template<> struct convert<Aida::Any> : convert_AidaAny {};
} // v8pp

static v8::Local<v8::Value>
any_to_v8 (v8::Isolate *iso, const Aida::Any &a)
{
  return v8pp::to_v8<Aida::Any> (iso, a);
}

static Aida::Any
any_from_v8 (v8::Isolate *iso, v8::Local<v8::Value> &v)
{
  return v8pp::from_v8<Aida::Any> (iso, v);
}

typedef v8pp::class_<Aida::Event>                V8ppType_AidaEvent;
typedef v8pp::class_<Aida::RemoteHandle>         V8ppType_AidaRemoteHandle;

static void
aida_event_generic_getter (v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value>& info)
{
  v8::Isolate *const __v8isolate = info.GetIsolate();
  const std::string __pname = v8pp::from_v8<std::string> (__v8isolate, property);
  const Aida::Event *const __event = V8ppType_AidaEvent::unwrap_object (__v8isolate, info.This());
  return_unless (__event != NULL);
  v8::ReturnValue<v8::Value> __v8ret = info.GetReturnValue();
  const Aida::Any &__any = (*__event)[__pname];
  switch (__any.kind())
    {
    case Aida::BOOL:		__v8ret.Set (__any.get<bool>()); break;
    case Aida::FLOAT64:         __v8ret.Set (__any.get<double>()); break;
    case Aida::INT32: 		__v8ret.Set (__any.get<int32>()); break;
    case Aida::INT64:		__v8ret.Set (v8pp::to_v8 (__v8isolate, __any.get<int64>())); break;
    case Aida::STRING:		__v8ret.Set (v8pp::to_v8 (__v8isolate, __any.get<std::string>())); break;
    case Aida::ENUM: {
      const std::string __str = Aida::enum_value_to_string (__any.get_enum_typename(), __any.as_int64(), "+");
      __v8ret.Set (v8pp::to_v8 (__v8isolate, __str));
      break; }
    case Aida::REMOTE: {
      const Aida::RemoteHandle __rhandle = __any.get_untyped_remote_handle();
      __v8ret.Set (v8pp::to_v8 (__v8isolate, aida_remote_handle_wrap_native (__v8isolate, __rhandle)));
      break; }
    case Aida::SEQUENCE:
    case Aida::RECORD:
    case Aida::INSTANCE:
    case Aida::ANY:
    case Aida::UNTYPED:
    case Aida::TRANSITION:
    default:        	; // undefined
    }
}

#include "v8bse.cc"

// event loop integration
static uv_poll_t               bse_uv_watcher;
static Aida::ExecutionContext *ebeast_execution_context = NULL;
static Bse::ServerH            bse_server;
static uv_async_t              bse_uv_dispatcher;
static uv_prepare_t            bse_uv_preparer;

/// Retrieve an ArrayBuffer from a BSE shared memory id.
static v8::Local<v8::Object>
bse_server_create_shared_memory_array_buffer (uint64 shm_id)
{
  v8::Isolate *const isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::ArrayBuffer> ab;
  Bse::SharedMemory sm = bse_server.get_shared_memory (shm_id);
  return_unless (uint64 (sm.shm_id) == shm_id, ab);
  return_unless (sm.shm_creator == Bse::this_thread_getpid(), ab);
  char *shm_start = (char*) sm.shm_start; // allowed if sm.shm_creator matches our pid
  return_unless (shm_start != NULL, ab);
  ab = v8::ArrayBuffer::New (isolate, shm_start, sm.shm_length);
  return ab;
}

static std::string
bse_server_gettext (const std::string &msg)
{
  return (Bse::_) (msg);
}

static std::string
bse_server_ngettext (const std::string &msg, const std::string &plural, double n)
{
  return (Bse::_) (msg, plural, int64_t (round (n)));
}

// v8pp binding for Bse
static V8stub *bse_v8stub = NULL;

// register bindings and start Bse
static void
v8bse_register_module (v8::Local<v8::Object> exports, v8::Local<v8::Object> module)
{
  assert (bse_v8stub == NULL);
  v8::Isolate *const isolate = v8::Isolate::GetCurrent();
  assert (aida_remote_handle_idmap_isolate == NULL);
  aida_remote_handle_idmap_isolate = isolate;

  v8::HandleScope scope (isolate);

  Bse::this_thread_set_name ("EBeast-module");

  // workaround electron appending argv[1:] to argv[0]
  if (Bse::program_alias().find ("electron ") != std::string::npos)
    Bse::program_alias_init (Bse::Path::cwd()); // a guess at the actual electron application

  // prepare Bse environment
  v8::Local<v8::String> v8_modulefile = module->Get (v8pp::to_v8 (isolate, "filename")).As<v8::String>();
  // get from $beastroot/ebeast-bundle/app/assets/v8bse.node -> $beastroot/Demo/...
  const char *canary = "partymonster.bse";
  if (!Bse::Path::check (Bse::Path::join (Bse::runpath (Bse::RPath::DEMODIR), canary), "r"))
    Bse::fatal_error ("failed to locate BSE library containing '%s'", canary);

  // start Bse
  Bse::String bseoptions = Bse::string_format ("debug-extensions=%d", 0);
  Bse::init_async (NULL, NULL, "BEAST", Bse::string_split (bseoptions, ":"));

  // fetch server handle for remote calls
  assert (bse_server == NULL);
  bse_server = Bse::init_server_instance();
  assert (bse_server != NULL);

  // enable handling of local callbacks from remote notifications
  assert_return (ebeast_execution_context == NULL);
  ebeast_execution_context = Aida::ExecutionContext::new_context();
  ebeast_execution_context->push_thread_current();

  // libuv event loop integration

  /* Electron drives the uvloop via UV_RUN_NOWAIT and outsources fd polling into
   * a dedicated worker thread (however this does not affect the uv_poll_start
   * callback thread). That means if the ExecutionContext is fetching remote
   * events and is re-queueing those internally outside of uvdispatchcb (e.g.
   * due to a Bse call from a JS timeout), notify_fd will not be getting a
   * chance to wakeup poll(2) to trigger uvdispatchcb eventhough work is pending.
   * Avoiding this depends on Aida::remote_callr() *not* fetching and internally
   * re-queueing events, something the old Aida::ClientConnection used to do in
   * ProtoScope::invoke(), which required pending() checks after each invoke().
   * That's also why we always process the *entire* work queue in uvdispatchcb().
   */
  uv_loop_t *const uvloop = uv_default_loop();
  // Dispatch all pending events from uvlooop
  auto uvdispatchcb = [] (uv_async_t*) {
    if (ebeast_execution_context)
      while (ebeast_execution_context->pending())
        ebeast_execution_context->dispatch();
  };
  uv_async_init (uvloop, &bse_uv_dispatcher, uvdispatchcb);
  // Poll notify_fd, clear fd and queue dispatcher events
  auto uvpollcb = [] (uv_poll_t*, int, int) {
    if (ebeast_execution_context && ebeast_execution_context->pending())
      uv_async_send (&bse_uv_dispatcher);
  };
  uv_poll_init (uvloop, &bse_uv_watcher, ebeast_execution_context->notify_fd());
  uv_poll_start (&bse_uv_watcher, UV_READABLE, uvpollcb);
  // Prevent libuv from waiting in poll if events are pending
  auto uvpreparecb = [] (uv_prepare_t*) {
    if (ebeast_execution_context && ebeast_execution_context->pending())
      uv_async_send (&bse_uv_dispatcher);
  };
  uv_prepare_init (uvloop, &bse_uv_preparer);
  uv_prepare_start (&bse_uv_preparer, uvpreparecb);

  // register v8stub C++ bindings
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  bse_v8stub = new V8stub (isolate);
  v8::Local<v8::Object> module_instance = bse_v8stub->module_.new_instance();
  v8::Maybe<bool> ok = exports->SetPrototype (context, module_instance);
  assert (ok.FromJust() == true);

  // manual binding extensions
  V8ppType_BseServer &server_class = bse_v8stub->BseServer_class_;
  server_class.set ("create_shared_memory_array_buffer", bse_server_create_shared_memory_array_buffer);
  server_class.set ("gettext", bse_server_gettext);
  server_class.set ("ngettext", bse_server_ngettext);

  // export server handle
  v8::Local<v8::Object> v8_server = server_class.import_external (isolate, new Bse::ServerH (bse_server));
  module_instance->DefineOwnProperty (context, v8pp::to_v8 (isolate, "server"),
                                      v8_server, v8::PropertyAttribute (v8::ReadOnly | v8::DontDelete));

  // execute v8stub javascript initialization
  bse_v8stub->jsinit (context, exports);

  // debugging aids:
  if (0)
    {
      Bse::printerr ("gdb %s %u -ex 'catch catch' -ex 'catch throw'\n", Bse::string_split (program_invocation_name, " ", 1)[0], Bse::this_thread_getpid());
      g_usleep (3 * 1000000);
    }

  // Ensure Bse has everything properly loaded
  bse_server.load_assets();
}

// node.js registration
NODE_MODULE (v8bse, v8bse_register_module);
