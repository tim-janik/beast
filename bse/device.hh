// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_DEVICE_HH__
#define __BSE_DEVICE_HH__

#include <bse/module.hh>
#include <bse/processor.hh>
#include <bse/combo.hh>

namespace Bse {

// == DeviceImpl ==
class DeviceImpl : public ObjectImpl, public virtual DeviceIface {
  const String device_uri_;
  std::vector<ModuleImplP> modules_;
protected:
  using ParamInfoP = AudioSignal::ParamInfoP;
  using Processor = AudioSignal::Processor;
  using ProcessorP = AudioSignal::ProcessorP;
  using ParamInfoPVec = Processor::ParamInfoPVec;
  friend class           FriendAllocator<DeviceImpl>;
  virtual               ~DeviceImpl        ();
  virtual void           xml_serialize     (SerializationNode &xs) override;
  virtual void           xml_reflink       (SerializationNode &xs) override;
  explicit               DeviceImpl        (const String &uuiduri);
public:
  virtual ModuleSeq      list_modules      () override;
  virtual ModuleIfaceP   create_module     (const String &module_id) override;
  virtual ModuleTypeInfo module_type_info  (const String &module_id) override { return ModuleImpl::module_type_info (module_id); }
  virtual StringSeq      list_module_types () override                        { return ModuleImpl::list_module_types (); }
  virtual String         device_uri        () override  { return device_uri_; }
  virtual DeviceInfo     device_info       () override  { return device_info (device_uri()); }
  virtual StringSeq      list_properties   () override;
  virtual PropertyIfaceP access_property   (const std::string &ident) override;
  virtual PropertySeq    access_properties (const std::string &hints) override;
  virtual ProcessorP     processor         () = 0;
  ParamInfoP             param_info        (const std::string &ident);
  virtual const ParamInfoPVec& list_params () const = 0;
  static DeviceInfo      device_info       (const String &uuiduri);
  static StringSeq       list_device_types ();
  using DeviceImplP = std::shared_ptr<DeviceImpl>;
  static DeviceImplP     create_single_device (const String &uri);
};
using DeviceImplP = DeviceImpl::DeviceImplP;
using DeviceImplW = std::weak_ptr<DeviceImpl>;

// == DeviceContainerImpl ==
class DeviceContainerImpl : public DeviceImpl, public virtual DeviceContainerIface {
  using DeviceVec = std::vector<DeviceImplP>;
  DeviceVec devices_;
protected:
  friend class FriendAllocator<DeviceContainerImpl>;
  virtual                    ~DeviceContainerImpl  ();
  virtual void                xml_serialize        (SerializationNode &xs) override;
  virtual void                xml_reflink          (SerializationNode &xs) override;
  explicit                    DeviceContainerImpl  (const String &uuiduri);
  virtual void                added_device         (size_t idx) = 0;
  virtual void                removing_device      (size_t idx) = 0;
  const DeviceVec&            device_vec           () const;
  void                        commit_job           (const std::function<void()> &lambda);
public:
  using DeviceContainerImplP = std::shared_ptr<DeviceContainerImpl>;
  virtual DeviceInfoSeq       list_device_types    () override;
  virtual DeviceSeq           list_devices         () override;
  virtual DeviceIfaceP        create_device        (const String &uuiduri) override;
  virtual DeviceIfaceP        create_device_before (const String &uuiduri, DeviceIface &sibling) override;
  virtual bool                remove_device        (DeviceIface &containee) override;
};
using DeviceContainerImplP = DeviceContainerImpl::DeviceContainerImplP;

// == AspDeviceImpl ==
class AspDeviceImpl : public DeviceImpl {
  ProcessorP   procp_;
  const ParamInfoPVec params_;
public:
  ProcessorP              processor   () override       { return procp_; }
  const ParamInfoPVec&    list_params () const override { return params_; }
  explicit AspDeviceImpl (const String &uri, ProcessorP procp);
};

// == AspDeviceContainerImpl ==
class AspDeviceContainerImpl : public DeviceContainerImpl {
  AudioSignal::ChainP chainp_;
  const ParamInfoPVec params_;
protected:
  void added_device (size_t idx) override;
  void removing_device (size_t idx) override;
public:
  ProcessorP           processor   () override       { return chainp_; }
  const ParamInfoPVec& list_params () const override { return params_; }
  explicit AspDeviceContainerImpl (const String &uri, AudioSignal::ChainP chainp);
};

} // Bse

#endif // __BSE_DEVICE_HH__
