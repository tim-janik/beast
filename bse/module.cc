// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "module.hh"
#include "internal.hh"

namespace Bse {

ModuleImpl::ModuleImpl (const String &module_type) :
  module_type_ (module_type)
{}

ModuleImpl::~ModuleImpl ()
{}

ModuleTypeInfo
ModuleImpl::module_type_info (const String &module_id)
{
  ModuleTypeInfo info;
  info.id = module_id;
  return info;
}

StringSeq
ModuleImpl::list_module_types ()
{
  StringSeq seq;
  seq.push_back ("Dumb");
  return seq;
}

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
