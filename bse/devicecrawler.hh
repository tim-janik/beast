// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __BSE_DEVICECRAWLER_HH__
#define __BSE_DEVICECRAWLER_HH__

#include <bse/bseobject.hh>

namespace Bse {

class DeviceCrawlerImpl : public ObjectImpl, public virtual DeviceCrawlerIface {
protected:
  virtual                   ~DeviceCrawlerImpl  ();
  friend class               FriendAllocator<DeviceCrawlerImpl>;
public:
  virtual DeviceEntry        list_device_origin (DeviceOrigin origin) override;
  static  DeviceCrawlerImplP instance_p         ();
  static  DeviceCrawlerImpl& instance           () { return *instance_p(); }
};

} // Bse

#endif /* __BSE_DEVICECRAWLER_HH__ */
