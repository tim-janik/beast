// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "device.hh"
#include "processor.hh"
#include "bseserver.hh"
#include "internal.hh"

namespace Bse {

// == AspDeviceImpl ==
class AspDeviceImpl : public DeviceImpl {
  AudioSignal::ProcessorP procp_;
public:
  AudioSignal::ProcessorP processor () override { return procp_; }
  explicit AspDeviceImpl (const String &uri, AudioSignal::ProcessorP procp) :
    DeviceImpl (uri), procp_ (procp)
  {
    assert_return (procp != nullptr);
  }
};

// == AspDeviceContainerImpl ==
class AspDeviceContainerImpl : public DeviceContainerImpl {
  AudioSignal::ChainP chainp_;
protected:
  void
  added_device (size_t idx) override
  {
    auto devs = device_vec();
    ProcessorP procp = devs[idx]->processor();
    AudioSignal::ChainP chainp = chainp_;
    commit_job ([chainp, procp, idx] () {
      chainp->insert (procp, idx);
    });
  }
  void
  removing_device (size_t idx) override
  {
    auto devs = device_vec();
    ProcessorP procp = devs[idx]->processor();
    assert_return (procp);
    AudioSignal::ChainP chainp = chainp_;
    commit_job ([chainp, procp] () {
      const bool proc_found_and_removed = chainp->remove (*procp);
      assert_return (proc_found_and_removed);
    });
  }
public:
  AudioSignal::ProcessorP processor () override { return chainp_; }
  explicit AspDeviceContainerImpl (const String &uri, AudioSignal::ChainP chainp) :
    DeviceContainerImpl (uri), chainp_ (chainp)
  {
    assert_return (chainp != nullptr);
  }
};

// == DeviceImpl ==
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

DeviceImplP
DeviceImpl::create_single_device (const String &uri)
{
  DeviceImplP devicep;
  AudioSignal::ProcessorP procp = AudioSignal::Processor::registry_create (uri);
  AudioSignal::ChainP chainp = std::dynamic_pointer_cast<AudioSignal::Chain> (procp);
  if (chainp) // FIXME: need Processor container interface
    devicep = FriendAllocator<AspDeviceContainerImpl>::make_shared (uri, chainp);
  else if (procp)
    devicep = FriendAllocator<AspDeviceImpl>::make_shared (uri, procp);
  return devicep;
}

// == DeviceContainerImpl ==
DeviceInfoSeq
DeviceContainerImpl::list_device_types ()
{
  using namespace AudioSignal;
  DeviceInfoSeq iseq;
  const auto rlist = Processor::registry_list();
  iseq.reserve (rlist.size());
  for (const auto &entry : rlist)
    {
      DeviceInfo info;
      info.uri          = entry.uri;
      info.name         = entry.label;
      info.category     = entry.category;
      info.description  = entry.description;
      info.website_url  = entry.website_url;
      info.creator_name = entry.creator_name;
      info.creator_url  = entry.creator_url;
      iseq.push_back (info);
    }
  return iseq;
}

DeviceIfaceP
DeviceContainerImpl::create_device (const String &uuiduri)
{
  DeviceImplP devicep = DeviceImpl::create_single_device (uuiduri);
  if (devicep)
    {
      devices_.push_back (devicep);
      added_device (devices_.size() - 1);
      notify ("devices");
      return devices_.back();
    }
  return nullptr;
}

DeviceIfaceP
DeviceContainerImpl::create_device_before (const String &uuiduri, DeviceIface &sibling)
{
  DeviceImplP devicep = DeviceImpl::create_single_device (uuiduri);
  if (devicep)
    {
      size_t i;
      for (i = 0; i < devices_.size(); i++)
        if (devices_[i].get() == &sibling)
          break;
      devices_.insert (devices_.begin() + i, devicep);
      added_device (i);
      notify ("devices");
      return devices_[i];
    }
  return nullptr;
}

bool
DeviceContainerImpl::remove_device (DeviceIface &containee)
{
  size_t i;
  for (i = 0; i < devices_.size(); i++)
    if (devices_[i].get() == &containee)
      {
        removing_device (i);
        assert_return (i < devices_.size() && devices_[i].get() == &containee, false);
        devices_.erase (devices_.begin() + i);
        notify ("devices");
        return true;
      }
  return false;
}

DeviceSeq
DeviceContainerImpl::list_devices ()
{
  DeviceSeq devices;
  for (auto &d : devices_)
    devices.push_back (d);
  return devices;
}

DeviceContainerImpl::DeviceContainerImpl (const String &uuiduri) :
  DeviceImpl (uuiduri)
{}

DeviceContainerImpl::~DeviceContainerImpl ()
{}

const DeviceContainerImpl::DeviceVec&
DeviceContainerImpl::device_vec () const
{
  return devices_;
}

void
DeviceContainerImpl::commit_job (const std::function<void()> &lambda)
{
  BSE_SERVER.commit_job (lambda);
}

void
DeviceContainerImpl::xml_serialize (SerializationNode &xs)
{
  if (xs.in_save())
    {
      String dtype = device_uri();
      xs["type"] & dtype; // used by create_device()
    }

  ObjectImpl::xml_serialize (xs); // always chain to parent's method

  for (DeviceImplP device : devices_)           // in_save
    xs.save_under ("Device", *device);
  for (auto &xc : xs.children ("Device"))       // in_load
    xc.load (*dynamic_cast<ModuleImpl*> (create_device (xc.get ("type")).get()));
}

void
DeviceContainerImpl::xml_reflink (SerializationNode &xs)
{
  ObjectImpl::xml_reflink (xs); // always chain to parent's method
}

} // Bse
