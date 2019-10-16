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

DeviceTypeInfo
DeviceImpl::device_type_info (const String &device_id)
{
  DeviceTypeInfo info;
  info.id = device_id;
  return info;
}

StringSeq
DeviceImpl::list_device_types ()
{
  StringSeq seq;
  seq.push_back ("Dummy");
  return seq;
}

} // Bse
