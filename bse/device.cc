// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "device.hh"
#include "processor.hh"
#include "property.hh"
#include "bseserver.hh"
#include "internal.hh"

namespace Bse {

// == AspDeviceImpl ==
AspDeviceImpl::AspDeviceImpl (const String &uri, AudioSignal::ProcessorP procp) :
  DeviceImpl (uri), procp_ (procp), params_ (procp->list_params())
{
  assert_return (procp != nullptr);
}

// == AspDeviceContainerImpl ==
AspDeviceContainerImpl::AspDeviceContainerImpl (const String &uri, AudioSignal::ChainP chainp) :
  DeviceContainerImpl (uri), chainp_ (chainp), params_ (chainp->list_params())
{
  assert_return (chainp != nullptr);
}

void
AspDeviceContainerImpl::added_device (size_t idx)
{
  auto devs = device_vec();
  ProcessorP procp = devs[idx]->processor();
  AudioSignal::ChainP chainp = chainp_;
  commit_job ([chainp, procp, idx] () {
    chainp->insert (procp, idx);
  });
}

void
AspDeviceContainerImpl::removing_device (size_t idx)
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

// == DevicePropertyWrapper ==
class DevicePropertyWrapper : public PropertyWrapper {
  using ParamId = AudioSignal::ParamId;
  DeviceImplW             device_;
  AudioSignal::ParamInfoP info_;
public:
  friend class DeviceImpl;
  virtual std::string get_tag          (Tag) override;
  virtual bool        is_numeric       () override;
  virtual void        get_range        (double *min, double *max, double *step) override;
  virtual double      get_num          () override;
  virtual bool        set_num          (double v) override;
  virtual std::string get_string       () override;
  virtual bool        set_string       (const std::string &v) override;
  explicit DevicePropertyWrapper (DeviceImplP device, AudioSignal::ParamInfoP param_);
};

DevicePropertyWrapper::DevicePropertyWrapper (DeviceImplP device, AudioSignal::ParamInfoP param_) :
  device_ (device), info_ (param_)
{}

std::string
DevicePropertyWrapper::get_tag (Tag tag)
{
  auto &info = *info_;
  switch (tag)
    {
    case IDENTIFIER:    return info.ident;
    case LABEL:         return info.label;
    case NICK:          return info.nick;
    case UNIT:          return info.unit;
    case HINTS:         return info.hints;
    case GROUP:         return info.group;
    case BLURB:         return info.blurb;
    case DESCRIPTION:   return info.description;
    }
}

void
DevicePropertyWrapper::get_range (double *min, double *max, double *step)
{
  double a, b, c;
  info_->get_range (min ? *min : a, max ? *max : b, step ? *step : c);
}

double
DevicePropertyWrapper::get_num ()
{
  DeviceImplP device = device_.lock();
  return_unless (device, FP_NAN);
  AudioSignal::ProcessorP proc = device->processor();
  return_unless (proc, FP_NAN);
  return AudioSignal::Processor::peek_param_mt (proc, info_->id);
}

bool
DevicePropertyWrapper::set_num (double v)
{
  DeviceImplP device = device_.lock();
  return_unless (device, false);
  AudioSignal::ProcessorP proc = device->processor();
  return_unless (proc, false);
  const AudioSignal::ParamId pid = info_->id;
  auto lambda = [proc, pid, v] () {
    proc->set_param (pid, v);
  };
  BSE_SERVER.commit_job (lambda);
  return true;
}

bool
DevicePropertyWrapper::is_numeric ()
{
  // TODO: we have yet to implement non-numeric Processor parameters
  return true;
}

std::string
DevicePropertyWrapper::get_string ()
{
  // TODO: we have yet to implement non-numeric Processor parameters
  return {};
}

bool
DevicePropertyWrapper::set_string (const std::string &v)
{
  // TODO: we have yet to implement non-numeric Processor parameters
  return false;
}

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

StringSeq
DeviceImpl::list_properties ()
{
  using namespace AudioSignal;
  const auto &iv = list_params();
  StringSeq names;
  names.reserve (iv.size());
  for (const auto &param : iv)
    names.push_back (param->ident);
  return names;
}

PropertyIfaceP
DeviceImpl::access_property (const std::string &ident)
{
  using namespace AudioSignal;
  const ParamInfoPVec &piv = list_params();
  ParamInfoP infop;
  for (const ParamInfoP info : piv)
    if (info->ident == ident)
      {
        infop = info;
        break;
      }
  return_unless (infop, {});
  auto devicep = Bse::shared_ptr_cast<DeviceImpl> (this);
  auto pwrapper = std::make_unique<DevicePropertyWrapper> (devicep, infop);
  return PropertyImpl::create (std::move (pwrapper));
}

PropertySeq
DeviceImpl::access_properties (const std::string &hints)
{
  using namespace AudioSignal;
  auto devicep = Bse::shared_ptr_cast<DeviceImpl> (this);
  PropertySeq ps;
  for (const ParamInfoP info : list_params())
    {
      auto pwrapper = std::make_unique<DevicePropertyWrapper> (devicep, info);
      PropertyIfaceP prop = PropertyImpl::create (std::move (pwrapper));
      ps.push_back (prop);
    }
  return ps;
}

AudioSignal::ParamInfoP
DeviceImpl::param_info (const std::string &ident)
{
  using namespace AudioSignal;
  const CString cident = CString::lookup (ident);
  if (!cident.empty()) // ident must be seen before
    {
      const ParamInfoPVec &piv = list_params();
      for (const ParamInfoP info : piv)
        if (info->ident == cident)
          return info;
    }
  return nullptr;
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
