// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "device.hh"
#include "internal.hh"

namespace Bse {

DeviceImpl::DeviceImpl (const String &device_type) :
  device_type_ (device_type)
{}

DeviceImpl::~DeviceImpl ()
{}

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

ModuleSeq
DeviceImpl::list_modules ()
{
  ModuleSeq modules;
  for (auto &m : modules_)
    modules.push_back (m->__handle__());
  return modules;
}

ModuleIfaceP
DeviceImpl::create_module (const String &module_id)
{
  ModuleImplP modulep = FriendAllocator<ModuleImpl>::make_shared (module_id);
  assert_return (modulep, nullptr);
  modules_.push_back (modulep);
  notify ("modules");
  return nullptr;
}

} // Bse
