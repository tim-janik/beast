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
#include "testplugin.gen-idl.h"

#include <math.h>
#include <string.h>

namespace Test {
using namespace std;
using namespace Bse;

class Plugin : public PluginBase
{
public:
  //BSE_EFFECT_INTEGRATE_MODULE (Plugin, Module, Properties);

  /* FIXME */
  Bse::SynthesisModule* create_module(unsigned int, GslTrans*)
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
};

BSE_CXX_DEFINE_EXPORTS();
BSE_CXX_REGISTER_EFFECT (Plugin);

} // Test

/* vim:set ts=8 sw=2 sts=2: */
