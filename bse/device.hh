// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_DEVICE_HH__
#define __BSE_DEVICE_HH__

#include <bse/bsesource.hh>

namespace Bse {

class DeviceImpl : public ObjectImpl, public virtual DeviceIface {
  const String device_type_;
protected:
  friend class           FriendAllocator<DeviceImpl>;
  virtual               ~DeviceImpl        ();
public:
  explicit               DeviceImpl        (const String &device_type);
  virtual StringSeq      list_module_types () override;
  virtual ModuleTypeInfo module_type_info  (const String &module_type) override;
  virtual ModuleIfaceP   create_module     (const String &module_type) override;
  virtual String         get_device_type   () override;
  virtual DeviceTypeInfo device_type_info  () override;
};
using DeviceImplP = std::shared_ptr<DeviceImpl>;

} // Bse

#endif // __BSE_DEVICE_HH__
