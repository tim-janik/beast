// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "../bse.hh"
#include "v8bse.cc"
#include <node.h>

// v8pp binding for Bse
static V8stub *bse_v8stub = NULL;

// register bindings and start Bse
static void
v8bse_register_module (v8::Local<v8::Object> exports)
{
  assert (bse_v8stub == NULL);
  // start Bse
  Bse::String bseoptions = Bse::string_format ("debug-extensions=%d", 0);
  Bse::init_async (NULL, NULL, "BEAST", Bse::string_split (bseoptions, ":"));
  // register v8stub
  v8::Isolate *const isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  bse_v8stub = new V8stub (isolate);
  v8::Maybe<bool> ok = exports->SetPrototype (context, bse_v8stub->module_.new_instance());
  assert (ok.FromJust() == true);
}

// node.js registration
NODE_MODULE (v8bse, v8bse_register_module);
