// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "device.hh"
#include "internal.hh"

namespace Bse {

DeviceImpl::DeviceImpl (const String &uuiduri) :
  device_uri_ (uuiduri)
{}

DeviceImpl::~DeviceImpl ()
{}

void
DeviceImpl::xml_serialize (SerializationNode &xs)
{
  if (xs.in_save())
    {
      String dtype = device_uri_;
      xs["type"] & dtype; // used by TrackImpl.create_device()
    }

  ObjectImpl::xml_serialize (xs); // always chain to parent's method

  for (ModuleImplP module : modules_)           // in_save
    xs.save_under ("Module", *module);
  for (auto &xc : xs.children ("Module"))       // in_load
    xc.load (*dynamic_cast<ModuleImpl*> (create_module (xc.get ("type")).get()));
}

void
DeviceImpl::xml_reflink (SerializationNode &xs)
{
  ObjectImpl::xml_reflink (xs); // always chain to parent's method
}

DeviceInfo
DeviceImpl::device_info (const String &uuiduri)
{
  DeviceInfo info;
  info.uri = uuiduri; // TODO: fill other fields
  return info;
}

StringSeq
DeviceImpl::list_device_types ()
{
  StringSeq seq;
  seq.push_back ("Dummy"); // TODO: find other types
  return seq;
}

ModuleSeq
DeviceImpl::list_modules ()
{
  ModuleSeq modules;
  for (auto &m : modules_)
    modules.push_back (m);
  return modules;
}

ModuleIfaceP
DeviceImpl::create_module (const String &module_id)
{
  ModuleImplP modulep = FriendAllocator<ModuleImpl>::make_shared (module_id);
  assert_return (modulep, nullptr);
  modules_.push_back (modulep);
  notify ("modules");
  return modules_.back();
}

} // Bse
