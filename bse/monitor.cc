// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "monitor.hh"

namespace Bse {

SignalMonitorImpl::SignalMonitorImpl (BseSnooper *snooper) :
  snooper_ (snooper)
{
  assert_return (snooper != NULL);
  g_object_ref (snooper_);
}

SignalMonitorImpl::~SignalMonitorImpl ()
{
  g_object_unref (snooper_);
  snooper_ = NULL;
}

int64
SignalMonitorImpl::get_shm_id ()
{
  return 0;
}

SourceIfaceP
SignalMonitorImpl::get_osource ()
{
  return NULL;
}

int64
SignalMonitorImpl::get_shm_offset()
{
  return 0;
}

int32
SignalMonitorImpl::get_ochannel()
{
  return 0;
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
  return NULL;
}

} // Bse
