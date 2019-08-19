// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_SIGNAL_MONITOR_HH__
#define __BSE_SIGNAL_MONITOR_HH__

#include <bse/bseutils.hh>
#include <bse/bsesnooper.hh>

namespace Bse {

class SignalMonitorImpl : public virtual SignalMonitorIface, public virtual Aida::EnableSharedFromThis<SignalMonitorImpl> {
  SourceImplP            source_;
  uint32                 ochannel_;
  ProbeFeatures          probe_features_;
  Aida::EventDispatcher  event_dispatcher_;
protected:
  virtual               ~SignalMonitorImpl  ();
  friend class           FriendAllocator<SignalMonitorImpl>;
public:
  explicit               SignalMonitorImpl  (SourceImplP source, uint ochannel);
  virtual SourceIfaceP   get_osource        () override;
  virtual int64          get_shm_offset     (MonitorField fld) override;
  virtual int32          get_ochannel       () override;
  virtual int64          get_mix_freq       () override;
  virtual int64          get_frame_duration () override;
  virtual void           set_probe_features (const ProbeFeatures &pf) override;
  virtual ProbeFeatures  get_probe_features () override;
  virtual Aida::ExecutionContext& __execution_context_mt__ () const override;
  virtual Aida::IfaceEventConnection __attach__            (const String &eventselector, EventHandlerF handler) override;
};
typedef std::shared_ptr<SignalMonitorImpl> SignalMonitorImplP;

} // Bse

#endif // __BSE_SIGNAL_MONITOR_HH__
