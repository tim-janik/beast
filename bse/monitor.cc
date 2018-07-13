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
{
  source_->cmon_sub_probe (ochannel_, probe_features_); // potentially deletes module
  probe_features_ = ProbeFeatures();
}

SourceIfaceP
SignalMonitorImpl::get_osource ()
{
  return source_;
}

int32
SignalMonitorImpl::get_ochannel()
{
  return ochannel_;
}

int64
SignalMonitorImpl::get_shm_id ()
{
  return 0;
}

int64
SignalMonitorImpl::get_shm_offset()
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
{
  return_unless (pf != probe_features_);
  source_->cmon_add_probe (ochannel_, pf);              // potentially creates new module
  source_->cmon_sub_probe (ochannel_, probe_features_); // potentially deletes module
  probe_features_ = pf;
}

ProbeFeatures
SignalMonitorImpl::get_probe_features ()
{
  return probe_features_;
}

// == SourceImpl ==
struct SourceImpl::ChannelMonitor {
  uint          probe_range = 0;
  uint          probe_energie = 0;
  uint          probe_samples = 0;
  uint          probe_fft = 0;
  bool          needs_module ()  { return probe_range || probe_energie || probe_samples || probe_fft; }
};

SourceImpl::ChannelMonitor&
SourceImpl::cmon_get (uint ochannel)
{
  assert_return (ochannel < size_t (n_ochannels()), *(ChannelMonitor*) NULL);
  if (!cmons_)
    cmons_ = new ChannelMonitor[n_ochannels()];
  return cmons_[ochannel];
}

void
SourceImpl::cmon_delete ()
{
  if (cmons_)
    {
      delete[] cmons_;
      cmons_ = NULL;
    }
}

void
SourceImpl::cmon_add_probe (uint ochannel, const ProbeFeatures &pf)
{
  assert_return (ochannel < size_t (n_ochannels()));
  ChannelMonitor &cmon = cmon_get (ochannel);
  const bool needed_module = cmon.needs_module();
  if (pf.probe_range)
    cmon.probe_range += 1;
  if (pf.probe_energie)
    cmon.probe_energie += 1;
  if (pf.probe_samples)
    cmon.probe_samples += 1;
  if (pf.probe_fft)
    cmon.probe_fft += 1;
  if (needed_module != cmon.needs_module())
    {
      // add module
    }
}

void
SourceImpl::cmon_sub_probe (uint ochannel, const ProbeFeatures &pf)
{
  assert_return (ochannel < size_t (n_ochannels()));
  ChannelMonitor &cmon = cmon_get (ochannel);
  const bool needed_module = cmon.needs_module();
  if (pf.probe_range)
    cmon.probe_range -= 1;
  if (pf.probe_energie)
    cmon.probe_energie -= 1;
  if (pf.probe_samples)
    cmon.probe_samples -= 1;
  if (pf.probe_fft)
    cmon.probe_fft -= 1;
  if (needed_module != cmon.needs_module())
    {
      // kill module
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
