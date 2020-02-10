// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "monitor.hh"
#include "bseengine.hh"
#include "bseserver.hh"
#include "bseblockutils.hh"
#include "bse/internal.hh"

namespace Bse {

class MonitorModule;

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
SignalMonitorImpl::get_shm_offset (MonitorField fld)
{
  SharedBlock sb = source_->cmon_get_block();
  char *fields0 = source_->cmon_monitor_field_start (0);
  assert_return (sb.mem_start == (void*) fields0, 0);
  char *fieldsn = source_->cmon_monitor_field_start (ochannel_);
  const size_t channel_offset = fieldsn - fields0;
  return sb.mem_offset + channel_offset + ptrdiff_t (fld);
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
  uint           probe_range = 0;
  uint           probe_energy = 0;
  uint           probe_samples = 0;
  uint           probe_fft = 0;
  MonitorModule *module = NULL;
  bool           needs_module ()  { return probe_range || probe_energy || probe_samples || probe_fft; }
  /*des*/       ~ChannelMonitor()
  {
    assert_return (module == NULL);
  }
};

SourceImpl::ChannelMonitor&
SourceImpl::cmon_get (uint ochannel)
{
  if (ochannel >= size_t (n_ochannels()))
    fatal_error ("Bse::SourceImpl::cmon_get: ochannel must be valid");
  if (!cmons_)
    cmons_ = new ChannelMonitor[n_ochannels()];
  return cmons_[ochannel];
}

static constexpr const size_t aligned_sizeof_MonitorFields = BSE_ALIGN (MonitorField::END_BYTE, FastMemoryArea::minimum_alignment);

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

char*
SourceImpl::cmon_monitor_field_start (uint ochannel)
{
  assert_return (ochannel < size_t (n_ochannels()), NULL);
  const SharedBlock sb = cmon_get_block();
  char *mfields = (char*) sb.mem_start;
  return mfields + aligned_sizeof_MonitorFields * ochannel;
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
  if (pf.probe_range)
    cmon.probe_range += 1;
  if (pf.probe_energy)
    cmon.probe_energy += 1;
  if (pf.probe_samples)
    cmon.probe_samples += 1;
  if (pf.probe_fft)
    cmon.probe_fft += 1;
  if (cmon.needs_module())
    cmon_activate();
}

void
SourceImpl::cmon_sub_probe (uint ochannel, const ProbeFeatures &pf)
{
  assert_return (ochannel < size_t (n_ochannels()));
  ChannelMonitor &cmon = cmon_get (ochannel);
  const bool needed_module = cmon.needs_module();
  if (pf.probe_range)
    cmon.probe_range -= 1;
  if (pf.probe_energy)
    cmon.probe_energy -= 1;
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

#define MIN_DB_SPL      -140    // -140dB is beyond float mantissa precision

class MonitorModule : public Bse::Module {
  float          *fblock_ = NULL;
  int64 counter_ = 0;
  float db_tip_ = MIN_DB_SPL;
  union {
    char  *char8_;
    double *f64_;
    float *f32_;
  };
  bool need_minmax_ = false, need_dbspl_ = false;
  inline float&  f32 (MonitorField mf)   { return f32_[size_t (mf) / 4]; }
  inline double& f64 (MonitorField mf)   { return f64_[size_t (mf) / 8]; }
public:
  MonitorModule (char *mfields) :
    Module (monitor_module_class),
    char8_ (mfields)
  {
    fblock_ = (float*) fast_mem_alloc (BSE_STREAM_MAX_VALUES * sizeof (float));
    assert_return (fblock_ != nullptr);
  }
  virtual ~MonitorModule()
  {
    fast_mem_free (fblock_);
  }
  virtual void
  reset () override
  {}
  void
  configure (bool probe_range, bool probe_energy, bool probe_samples, bool probe_fft) // EngineThread
  {
    need_minmax_ = probe_range;
    need_dbspl_ = probe_energy;
    // TODO: probe_samples
    // TODO: probe_fft
  }
  inline float
  calc_features (uint n_values, const float *ivalues, float *vmin, float *vmax)
  {
    if (need_minmax_ && need_dbspl_)
      return bse_block_calc_float_range_and_square_sum (n_values, ivalues, vmin, vmax);
    else if (need_minmax_)
      bse_block_calc_float_range (n_values, ivalues, vmin, vmax);
    else if (need_dbspl_)
      return bse_block_calc_float_square_sum (n_values, ivalues);
    return 0;
  }
  virtual void
  process (uint n_values) override // EngineThread
  {
    const BseJStream &jstream = BSE_MODULE_JSTREAM (this, 0);
    float vmin = 0, vmax = 0, vsqsum = 0;
    if (jstream.n_connections == 1)
      vsqsum = calc_features (n_values, jstream.values[0], &vmin, &vmax);
    else if (!need_dbspl_ && jstream.n_connections > 1)
      {
        for (int j = 0; j < int (jstream.n_connections); j++)
          {
            float tmin = vmin, tmax = vmax;
            vsqsum = calc_features (n_values, jstream.values[j], &tmin, &tmax);
            vmin = MIN (vmin, tmin);
            vmax = MAX (vmax, tmax);
          }
      }
    else if (need_dbspl_ && jstream.n_connections > 1)
      {
        // blocks must be added before we can calc square sums
        bse_block_copy_float (n_values, fblock_, jstream.values[0]);
        for (int j = 1; j < int (jstream.n_connections); j++)
          bse_block_add_floats (n_values, fblock_, jstream.values[j]);
        vsqsum = calc_features (n_values, fblock_, &vmin, &vmax);
      }
    f32 (MonitorField::F32_MIN) = vmin;
    f32 (MonitorField::F32_MAX) = vmax;
    const float avg_sqsum = vsqsum / n_values;
    const float db_spl = avg_sqsum > 0.0 ? 10 * log10 (avg_sqsum) : MIN_DB_SPL;
    f32 (MonitorField::F32_DB_SPL) = db_spl;
    constexpr float DBOFFSET = ABS (MIN_DB_SPL) * 1.5;
    db_tip_ = MAX ((DBOFFSET + db_tip_) * 0.999, DBOFFSET + db_spl) - DBOFFSET;
    f32 (MonitorField::F32_DB_TIP) = db_tip_;
    counter_ += 1;
    f64 (MonitorField::F64_GENERATION) = counter_;
    if (0)
      Bse::printout ("Monitor(%p): counter=%x [%+1.5f, %+1.5f] %+.2f (%+.2f) nj=%d nv=%d\n",
                     char8_, counter_, vmin, vmax, db_spl, db_tip_,
                     jstream.n_connections, n_values);
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
  struct Flags { bool probe_range = 0, probe_energy = 0, probe_samples = 0, probe_fft = 0; };
  for (size_t i = 0; i < noc; i++)
    if (cmons_[i].needs_module())
      {
        if (!cmons_[i].module)
          {
            cmons_[i].module = new MonitorModule (cmon_monitor_field_start (i));
            bse_trans_add (trans, bse_job_integrate (cmons_[i].module));
            bse_trans_add (trans, bse_job_set_consumer (cmons_[i].module, TRUE));
            for (auto omodule : omodules)
              bse_trans_add (trans, bse_job_jconnect (omodule, i, cmons_[i].module, 0));
          }
        Flags f;
        f.probe_range = cmons_[i].probe_range > 0;
        f.probe_energy = cmons_[i].probe_energy > 0;
        f.probe_samples = cmons_[i].probe_samples > 0;
        f.probe_fft = cmons_[i].probe_fft > 0;
        MonitorModule *monitor_module = cmons_[i].module;
        auto monitor_module_configure = [monitor_module, f] () {
          monitor_module->configure (f.probe_range, f.probe_energy, f.probe_samples, f.probe_fft);
        };
        bse_trans_add (trans, bse_job_access (cmons_[i].module, monitor_module_configure));
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
