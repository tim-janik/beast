// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "beast-sound-engine.hh"

namespace {
/// Helper class for conscise Jsonipc::Class inheritance registration.
template<class C, class B>
struct JsonipcClass : ::Jsonipc::Class<C> {
  JsonipcClass()
  {
    this->template inherit<B>();
  }
};
} // Anon

void
bse_jsonipc_stub_rest()
{
  using namespace Bse;

  // Add Jsonipc stubs that cannot be generated from IDL files
}
