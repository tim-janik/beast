// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "device.hh"

namespace Bse {

DeviceImpl::DeviceImpl (const String &device_type) :
  device_type_ (device_type)
{}

DeviceImpl::~DeviceImpl ()
{}

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
