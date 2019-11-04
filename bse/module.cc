// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "module.hh"
#include "internal.hh"

namespace Bse {

ModuleImpl::ModuleImpl (const String &module_type) :
  module_type_ (module_type)
{}

ModuleImpl::~ModuleImpl ()
{}

void
ModuleImpl::xml_serialize (SerializationNode &xs)
{
  if (xs.in_save())
    {
      String mtype = module_type_;
      xs["type"] & mtype; // used by DeviceImpl.create_module()
    }

  ObjectImpl::xml_serialize (xs); // always chain to parent's method
}

void
ModuleImpl::xml_reflink (SerializationNode &xs)
{
  ObjectImpl::xml_reflink (xs); // always chain to parent's method
}

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
