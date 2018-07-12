// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "monitor.hh"

namespace Bse {

SignalMonitorImpl::SignalMonitorImpl (SourceImplP source, uint ochannel) :
  source_ (source), ochannel_ (ochannel)
{
  assert_return (source != NULL);
  assert_return (ochannel < size_t (source->n_ochannels()));
}

SignalMonitorImpl::~SignalMonitorImpl ()
{}

int64
SignalMonitorImpl::get_shm_id ()
{
  return 0;
}

SourceIfaceP
SignalMonitorImpl::get_osource ()
{
  return source_;
}

int64
SignalMonitorImpl::get_shm_offset()
{
  return 0;
}

int32
SignalMonitorImpl::get_ochannel()
{
  return ochannel_;
}

int64
SignalMonitorImpl::get_mix_freq ()
{
  return 0;
}

int64
SignalMonitorImpl::get_frame_duration ()
{
  return 0;
}

void
SignalMonitorImpl::set_probe_features (const ProbeFeatures &pf)
{}

ProbeFeatures
SignalMonitorImpl::get_probe_features ()
{
  return ProbeFeatures();
}

SignalMonitorIfaceP
SourceImpl::create_signal_monitor (int32 ochannel)
{
  assert_return (ochannel >= 0, NULL);
  assert_return (ochannel < n_ochannels(), NULL);
  SignalMonitorImplP mon = FriendAllocator<SignalMonitorImpl>::make_shared (this->as<SourceImplP>(), ochannel);
  return mon;
}

} // Bse
