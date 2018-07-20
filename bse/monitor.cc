// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "monitor.hh"
#include "bseengine.hh"
#include "bseserver.hh"

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
  SharedBlock sb = source_->cmon_get_block();
  return sb.shm_id;
}

int64
SignalMonitorImpl::get_shm_offset()
{
  SharedBlock sb = source_->cmon_get_block();
  MonitorFields *fields0 = source_->cmon_get_fields (0);
  assert_return (sb.mem_start == (void*) fields0, 0);
  MonitorFields *fieldsn = source_->cmon_get_fields (ochannel_);
  const size_t channel_offset = ((char*) fieldsn) - ((char*) fields0);
  return sb.mem_offset + channel_offset;
  static_assert (sizeof (Bse::MonitorFields) == sizeof (double) * 4 + sizeof (int64), "");
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
  BseModule    *module = NULL;
  bool          needs_module ()  { return probe_range || probe_energie || probe_samples || probe_fft; }
  /*des*/      ~ChannelMonitor()
  {
    assert_return (module == NULL);
  }
};

SourceImpl::ChannelMonitor&
SourceImpl::cmon_get (uint ochannel)
{
  assert_return (ochannel < size_t (n_ochannels()), *(ChannelMonitor*) NULL);
  if (!cmons_)
    cmons_ = new ChannelMonitor[n_ochannels()];
  return cmons_[ochannel];
}

static constexpr const size_t aligned_sizeof_MonitorFields = BSE_ALIGN (sizeof (MonitorFields), BSE_CACHE_LINE_ALIGNMENT);

SharedBlock
SourceImpl::cmon_get_block ()
{
  if (!cmon_block_.mem_length)
    {
      const size_t size_needed = aligned_sizeof_MonitorFields * n_ochannels();
      cmon_block_ = BSE_SERVER.allocate_shared_block (size_needed);
    }
  return cmon_block_;
}

MonitorFields*
SourceImpl::cmon_get_fields (uint ochannel)
{
  assert_return (ochannel < size_t (n_ochannels()), NULL);
  const SharedBlock sb = cmon_get_block();
  MonitorFields *mfields = (MonitorFields*) sb.mem_start;
  return &mfields[ochannel];
}

void
SourceImpl::cmon_delete ()
{
  if (cmons_)
    {
      delete[] cmons_;
      cmons_ = NULL;
    }
  if (cmon_block_.mem_length)
    {
      const SharedBlock sb = cmon_block_;
      cmon_block_ = SharedBlock();
      BSE_SERVER.release_shared_block (sb);
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

bool
SourceImpl::cmon_needed ()
{
  return_unless (cmons_ != NULL, false);
  BseSource *self = as<BseSource*>();
  return_unless (BSE_SOURCE_PREPARED (self), false);
  const uint noc = n_ochannels();
  for (size_t i = 0; i < noc; i++)
    if (cmons_[i].needs_module())
      return true;
  return false;
}

static const BseModuleClass monitor_module_class = {
  0,                            // n_istreams
  1,                            // n_jstreams
  0,                            // n_ostreams
  NULL,                         // process
  NULL,                         // process_defer
  NULL,                         // reset
  NULL,                         // free
  Bse::ModuleFlag::NORMAL,      // mflags
};

class MonitorModule : public Bse::Module {
  MonitorFields *mfields_ = NULL;
public:
  MonitorModule (MonitorFields *fields) : Module (monitor_module_class), mfields_ (fields) {}
  virtual void
  reset () override
  {}
  virtual void
  process (uint n_values) override
  {
    const BseJStream &jstream = BSE_MODULE_JSTREAM (this, 0);
    if (jstream.n_connections)
      {
        const float *wave_in = jstream.values[0];
        float min = wave_in[0], max = wave_in[0];
        float avg = wave_in[0], first = wave_in[0], last = wave_in[n_values - 1];
        bool seen_nan = false, seen_pinf = false, seen_ninf = false, seen_subn = false;
        for (uint j = 0; j < jstream.n_connections; j++)
          for (uint i = 0; i < n_values; i++)
            {
              const float v = wave_in[i];
              max = max > v ? max : v;
              min = min < v ? min : v;
              avg += v;
              if (UNLIKELY (BSE_FLOAT_IS_NANINF (v)))
                {
                  seen_nan |= BSE_FLOAT_IS_NAN (v);
                  seen_pinf |= BSE_FLOAT_IS_INF_POSITIVE (v);
                  seen_ninf |= BSE_FLOAT_IS_INF_NEGATIVE (v);
                }
              else if (UNLIKELY (BSE_FLOAT_IS_SUBNORMAL (v)))
                seen_subn = true;
            }
        avg /= double (n_values) * jstream.n_connections;
        mfields_->min = min;
        mfields_->max = max;
        mfields_->energy = avg; // FIXME: calc energy properly
        mfields_->tip = MAX (mfields_->tip * 0.98, avg);
        __sync_fetch_and_add (&mfields_->gen, 1);
        if (0)
          Bse::printout ("Monitor: max=%+1.5f min=%+1.5f avg=%+1.5f cons=%u values=%u [%+1.5f,..,%+1.5f] freq=%+1.2f %s%s%s%s\r",
                         max, min, avg,
                         jstream.n_connections, n_values,
                         first, last,
                         BSE_FREQ_FROM_VALUE (avg),
                         seen_nan ? " +NAN" : "",
                         seen_pinf ? " +PINF" : "",
                         seen_ninf ? " +NINF" : "",
                         seen_subn ? " +SUBNORM" : "");
      }
  }
};

void // called on BseSource.prepare
SourceImpl::cmon_activate ()
{
  return_unless (cmon_needed());
  BseSource *self = as<BseSource*>();
  BseTrans *trans = bse_trans_open ();
  std::vector<BseModule*> omodules;
  bse_source_list_omodules (self, omodules);
  const uint noc = n_ochannels();
  for (size_t i = 0; i < noc; i++)
    if (cmons_[i].needs_module() && !cmons_[i].module)
      {
        cmons_[i].module = new MonitorModule (cmon_get_fields (i));
        bse_trans_add (trans, bse_job_integrate (cmons_[i].module));
        bse_trans_add (trans, bse_job_set_consumer (cmons_[i].module, TRUE));
        for (auto omodule : omodules)
          bse_trans_add (trans, bse_job_jconnect (omodule, i, cmons_[i].module, 0));
      }
  bse_trans_commit (trans);
}

void
SourceImpl::cmon_omodule_changed (BseModule *module, bool added, BseTrans *trans)
{
  return_unless (cmons_ != NULL);
  const uint noc = n_ochannels();
  for (size_t i = 0; i < noc; i++)
    if (cmons_[i].module)
      {
        if (added)
          bse_trans_add (trans, bse_job_jconnect (module, i, cmons_[i].module, 0));
        else
          bse_trans_add (trans, bse_job_jdisconnect (cmons_[i].module, 0, module, i));
      }
}

void // called on BseSource.reset
SourceImpl::cmon_deactivate ()
{
  return_unless (cmons_ != NULL);
  BseTrans *trans = NULL;
  const uint noc = n_ochannels();
  for (size_t i = 0; i < noc; i++)
    if (cmons_[i].module)
      {
        if (!trans)
          trans = bse_trans_open ();
	bse_trans_add (trans, bse_job_discard (cmons_[i].module));
        cmons_[i].module = NULL;
      }
  if (trans)
    bse_trans_commit (trans);
}

} // Bse
