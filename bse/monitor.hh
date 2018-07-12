// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_SIGNAL_MONITOR_HH__
#define __BSE_SIGNAL_MONITOR_HH__

#include <bse/bseutils.hh>
#include <bse/bsesnooper.hh>

namespace Bse {

class SignalMonitorImpl : public virtual SignalMonitorIface {
  SourceImplP            source_;
  uint32                 ochannel_;
protected:
  virtual               ~SignalMonitorImpl  ();
  friend class           FriendAllocator<SignalMonitorImpl>;
public:
  explicit               SignalMonitorImpl  (SourceImplP source, uint ochannel);
  virtual int64          get_shm_id         () override;
  virtual SourceIfaceP   get_osource        () override;
  virtual int64          get_shm_offset     () override;
  virtual int32          get_ochannel       () override;
  virtual int64          get_mix_freq       () override;
  virtual int64          get_frame_duration () override;
  virtual void           set_probe_features (const ProbeFeatures &pf) override;
  virtual ProbeFeatures  get_probe_features () override;
};
typedef std::shared_ptr<SignalMonitorImpl> SignalMonitorImplP;

} // Bse

#endif // __BSE_SIGNAL_MONITOR_HH__
