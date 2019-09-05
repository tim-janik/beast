// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __BSE_DEVICECRAWLER_HH__
#define __BSE_DEVICECRAWLER_HH__

#include <bse/bseobject.hh>

namespace Bse {

class DeviceCrawlerImpl : public virtual DeviceCrawlerIface, public virtual Aida::EnableSharedFromThis<DeviceCrawlerImpl> {
  Aida::EventDispatcher      event_dispatcher_;
protected:
  virtual                   ~DeviceCrawlerImpl  ();
  friend class               FriendAllocator<DeviceCrawlerImpl>;
public:
  virtual DeviceEntry        list_device_origin (DeviceOrigin origin) override;
  static  DeviceCrawlerImplP instance_p         ();
  static  DeviceCrawlerImpl& instance           () { return *instance_p(); }
  virtual Aida::ExecutionContext&    __execution_context_mt__ () const override { return execution_context(); }
  virtual Aida::IfaceEventConnection __attach__               (const String &eventselector, EventHandlerF handler) override
  {
    return event_dispatcher_.attach (eventselector, handler);
  }
};

} // Bse

#endif /* __BSE_DEVICECRAWLER_HH__ */
