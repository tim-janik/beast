/* TestPlugin - used to test the C++ language binding
 * Copyright (C) 2003 Stefan Westerfeld <stefan@space.twc.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
#include "bse/bsecxxmodule.h"
#include "testplugin.gen-idl.h"
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
    g_assert_not_reached ();
    return 0;
  }

  /* FIXME */
  Bse::SynthesisModule::Accessor* module_configurator()
  {
    g_assert_not_reached ();
    return 0;
  }
#else
  Bse::SynthesisModule* create_module(unsigned int, BseTrans*) { return 0; }
  Bse::SynthesisModule::Closure* make_module_config_closure() { return 0; }
  void (* get_module_auto_update())(BseModule*, void*) { return 0; }
#endif
};

SfiInt
Procedure::test_exception::exec (SfiInt        i,
                                 TestObject*   o,
                                 SfiInt        bar,
                                 FunkynessType ft)
{
  g_print ("testplugin.cc: test_exception: i=%d obj=%p bar=%d ft=%d (MODERATELY_FUNKY=%d)\n",
           i, o, bar, ft, (int) MODERATELY_FUNKY);
  if (ft != MODERATELY_FUNKY)
    throw std::runtime_error ("need to be moderately funky");
  if (!o)
    throw std::runtime_error ("object pointer is NULL");
  return i + bar;
}

BSE_CXX_DEFINE_EXPORTS();
BSE_CXX_REGISTER_ENUM (FunkynessType);
BSE_CXX_REGISTER_RECORD (TestRecord);
BSE_CXX_REGISTER_SEQUENCE (TestSequence);
BSE_CXX_REGISTER_EFFECT (TestObject);
BSE_CXX_REGISTER_PROCEDURE (test_exception);

} // Test

/* vim:set ts=8 sw=2 sts=2: */
