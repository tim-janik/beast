// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "module.hh"

namespace Bse {

ModuleImpl::ModuleImpl (const String &module_type) :
  module_type_ (module_type)
{}

ModuleImpl::~ModuleImpl ()
{}

String
ModuleImpl::get_module_type ()
{
  return String();
}

ModuleTypeInfo
ModuleImpl::module_type_info ()
{
  return ModuleTypeInfo();
}

} // Bse
