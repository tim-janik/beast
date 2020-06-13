// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __BSE_DEVICECRAWLER_HH__
#define __BSE_DEVICECRAWLER_HH__

#include <bse/bseobject.hh>

namespace Bse {

class ResourceCrawlerImpl : public ObjectImpl, public virtual ResourceCrawlerIface {
protected:
  virtual     ~ResourceCrawlerImpl  ();
  friend class FriendAllocator<ResourceCrawlerImpl>;
public:
  ResourceList list_files   (ResourceType file_type, ResourceOrigin file_origin) override;
  ResourceList list_devices (ResourceType rtype) override;
  static ResourceCrawlerImplP instance_p ();
  static ResourceCrawlerImpl& instance   () { return *instance_p(); }
};

} // Bse

#endif /* __BSE_DEVICECRAWLER_HH__ */
