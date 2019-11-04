// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_DEVICE_HH__
#define __BSE_DEVICE_HH__

#include <bse/module.hh>

namespace Bse {

class DeviceImpl : public ObjectImpl, public virtual DeviceIface {
  const String device_type_;
  std::vector<ModuleImplP> modules_;
protected:
  friend class           FriendAllocator<DeviceImpl>;
  virtual               ~DeviceImpl        ();
  virtual void           xml_serialize     (SerializationNode &xs) override;
  virtual void           xml_reflink       (SerializationNode &xs) override;
public:
  explicit               DeviceImpl        (const String &device_type);
  virtual ModuleSeq      list_modules      () override;
  virtual ModuleIfaceP   create_module     (const String &module_id) override;
  virtual ModuleTypeInfo module_type_info  (const String &module_id) override { return ModuleImpl::module_type_info (module_id); }
  virtual StringSeq      list_module_types () override                        { return ModuleImpl::list_module_types (); }
  virtual String         get_device_type   () override  { return device_type_; }
  virtual DeviceTypeInfo device_type_info  () override  { return device_type_info (get_device_type()); }
  static DeviceTypeInfo  device_type_info  (const String &device_id);
  static StringSeq       list_device_types ();
};
using DeviceImplP = std::shared_ptr<DeviceImpl>;

} // Bse

#endif // __BSE_DEVICE_HH__
