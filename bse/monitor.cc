// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "monitor.hh"
#include "bseengine.hh"

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

// == SourceImpl ==
struct SourceImpl::ChannelMonitor {
};

void
SourceImpl::cmon_delete ()
{
  if (cmons_)
    {
      delete[] cmons_;
      cmons_ = NULL;
    }
}

SignalMonitorIfaceP
SourceImpl::create_signal_monitor (int32 ochannel)
{
  assert_return (ochannel >= 0, NULL);
  assert_return (ochannel < n_ochannels(), NULL);
  SignalMonitorImplP mon = FriendAllocator<SignalMonitorImpl>::make_shared (this->as<SourceImplP>(), ochannel);
  return mon;
}

static void /* Engine Thread */
check_module (BseModule *module, void *data)
{
  // we only get here if the module's ENGINE_NODE_IS_INTEGRATED
}

void
SourceImpl::cmon_omodule_changed (BseModule *module, bool added, BseTrans *trans)
{
  bse_trans_add (trans, bse_job_access (module, check_module, NULL, NULL));
}

void // called on BseSource.prepare
SourceImpl::cmon_activate ()
{
  BseSource *self = as<BseSource*>();
  std::vector<BseModule*> omodules;
  bse_source_list_omodules (self, omodules);
}

void // called on BseSource.reset
SourceImpl::cmon_deactivate ()
{
}

} // Bse
