// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bse/bsecxxmodule.hh"
#include "testplugin.genidl.hh"
#include <stdexcept>
#include <math.h>
#include <string.h>

namespace Namespace {
using namespace std;
using namespace Bse;

class TestObject : public TestObjectBase
{
public:
  //BSE_EFFECT_INTEGRATE_MODULE (TestObject, Module, Properties);

#if 0
  /* FIXME */
  Bse::SynthesisModule* create_module(unsigned int, BseTrans*)
  {
    assert_unreached ();
    return 0;
  }

  /* FIXME */
  Bse::SynthesisModule::Accessor* module_configurator()
  {
    assert_unreached ();
    return 0;
  }
#else
  Bse::SynthesisModule* create_module(unsigned int, BseTrans*) { return 0; }
  Bse::SynthesisModule::Closure* make_module_config_closure() { return 0; }
  void (* get_module_auto_update())(BseModule*, void*) { return 0; }
#endif
};

BSE_CXX_DEFINE_EXPORTS();
BSE_CXX_REGISTER_ENUM (FunkynessType);
BSE_CXX_REGISTER_RECORD (TestRecord);
BSE_CXX_REGISTER_SEQUENCE (TestSequence);
BSE_CXX_REGISTER_EFFECT (TestObject);

} // Test

/* vim:set ts=8 sw=2 sts=2: */
