// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "device.hh"

namespace Bse {

DeviceImpl::DeviceImpl (const String &device_type) :
  device_type_ (device_type)
{}

DeviceImpl::~DeviceImpl ()
{}

StringSeq
DeviceImpl::list_module_types ()
{
  return StringSeq();
}

ModuleTypeInfo
DeviceImpl::module_type_info (const String &module_type)
{
  return ModuleTypeInfo();
}

ModuleIfaceP
DeviceImpl::create_module (const String &module_type)
{
  return nullptr;
}

String
DeviceImpl::get_device_type ()
{
  return String();
}

DeviceTypeInfo
DeviceImpl::device_type_info ()
{
  return DeviceTypeInfo();
}

} // Bse
