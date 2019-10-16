// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_DEVICE_HH__
#define __BSE_DEVICE_HH__

#include <bse/bsesource.hh>

namespace Bse {

class DeviceImpl : public virtual DeviceIface {
  const String device_type_;
protected:
  virtual               ~DeviceImpl       ();
public:
  explicit               DeviceImpl       (const String &device_type);
  virtual String         get_device_type  () override;
  virtual DeviceTypeInfo device_type_info () override;
};

} // Bse

#endif // __BSE_DEVICE_HH__
