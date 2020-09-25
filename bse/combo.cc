// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "combo.hh"
#include "bseserver.hh"
#include "internal.hh"

#define PDEBUG(...)     Bse::debug ("processor", __VA_ARGS__)

namespace Bse {
namespace AudioSignal {

// == LogState ==
enum LogState { MSG, HAVE, REQUEST, MATCH, MISS };
static const char*
lstate (LogState ls)
{
  switch (ls)
    {
    case MSG:           return "MSG";
    case HAVE:          return "HAVE";
    case REQUEST:       return "REQUEST";
    case MATCH:         return "MATCH";
    case MISS:          return "MISS";
    }
  return nullptr;
}

static std::string
strname (Processor *proc)
{
  return_unless (proc, "NONE");
  ProcessorInfo pinfo;
  proc->query_info (pinfo);
  return pinfo.label.empty() ? "UNKNOWN" : pinfo.label;
}

static std::string
strname (Processor *proc, OBusId ob, const SpeakerArrangement *spa = nullptr)
{
  return_unless (proc, "NONE·  ");
  if (!uint (ob))
    return string_format ("%s·  ", strname (proc));
  if (!spa)
    return string_format ("%s·-%u>>", strname (proc), ob);
  return string_format ("%s·%s-%u>>", strname (proc), speaker_arrangement_desc (*spa), ob);
}

static std::string
strname (Processor *proc, IBusId ib, const SpeakerArrangement *spa = nullptr)
{
  return_unless (proc, "  ·NONE");
  if (!uint (ib))
    return string_format ("  ·%s", strname (proc));
  if (!spa)
    return string_format ("<<-%u·%s", ib, strname (proc));
  return string_format ("<<%s-%u·%s", speaker_arrangement_desc (*spa), ib, strname (proc));
}

static void
logstate (LogState ls, Processor *p, OBusId ob, const SpeakerArrangement *osa, Processor *n, IBusId ib, const SpeakerArrangement *isa)
{
  std::string join;
  switch (ls)
    {
    case MSG:           join = "   "; break;
    case HAVE:          join = "-:-"; break;
    case REQUEST:       join = "???"; break;
    case MATCH:         join = ">-<"; break;
    case MISS:          join = "-/-"; break;
    }
  const auto msg = string_format ("  %-9s %40s  %s  %s", lstate (ls) + std::string (":"),
                                  strname (p, ob, osa), join,
                                  strname (n, ib, isa));
  const bool verbose_ = true;
  if (!msg.empty() && verbose_)
    PDEBUG ("%s%s", msg, msg.back() != '\n' ? "\n" : "");
}

// == Inlet ==
class Chain::Inlet : public Processor {
  Chain &chain_;
public:
  Inlet (const std::any &any) :
    chain_ (*std::any_cast<Chain*> (any))
  {
    assert_return (nullptr != std::any_cast<Chain*> (any));
  }
  void query_info (ProcessorInfo &info) const override  { info.label = "Bse.AudioSignal.Chain.Inlet"; }
  void initialize () override                           {}
  void reset      () override                           {}
  void
  configure (uint n_ibusses, const SpeakerArrangement *ibusses, uint n_obusses, const SpeakerArrangement *obusses) override
  {
    remove_all_buses();
    auto output = add_output_bus ("Output", chain_.ispeakers_);
    (void) output;
  }
  void
  render (uint n_frames) override
  {
    const IBusId i1 = IBusId (1);
    const OBusId o1 = OBusId (1);
    const uint ni = chain_.n_ichannels (i1);
    const uint no = this->n_ochannels (o1);
    assert_return (ni == no);
    for (size_t i = 0; i < ni; i++)
      redirect_oblock (o1, i, chain_.ifloats (i1, i));
  }
};

// == Chain ==
Chain::Chain (SpeakerArrangement iobuses) :
  ispeakers_ (iobuses), ospeakers_ (iobuses)
{
  static const auto reg_id = enroll_asp<AudioSignal::Chain::Inlet>();
  assert_return (speaker_arrangement_count_channels (iobuses) > 0);
  ProcessorP inlet = Processor::registry_create (engine_, reg_id, this);
  assert_return (inlet != nullptr);
  inlet_ = std::dynamic_pointer_cast<Inlet> (inlet);
  assert_return (inlet_ != nullptr);
}

Chain::~Chain()
{
  pm_remove_all_buses (*inlet_);
  inlet_ = nullptr;
  eproc_ = nullptr;
}

void
Chain::query_info (ProcessorInfo &info) const
{
  info.uri = "Bse.AudioSignal.Chain";
  info.label = "Bse::AudioSignal::Chain";
}
static auto bseaudiosignalchain = Bse::enroll_asp<AudioSignal::Chain>();

/// Assign event source for future auto-connections of chld processors.
void
Chain::set_event_source (ProcessorP eproc)
{
  if (eproc)
    assert_return (eproc->has_event_output());
  eproc_ = eproc;
}

void
Chain::initialize ()
{}

void
Chain::configure (uint n_ibusses, const SpeakerArrangement *ibusses, uint n_obusses, const SpeakerArrangement *obusses)
{
  remove_all_buses();
  auto monoin  = add_input_bus ("Input", ispeakers_);
  auto monoout = add_output_bus ("Output", ospeakers_);
  (void) monoin;
  (void) monoout;
}

void
Chain::reset()
{}

void
Chain::enqueue_children ()
{
  last_output_ = nullptr;
  engine_.enqueue (*inlet_);
  const ProcessorVec &cprocessors = processors_mt_;
  for (auto procp : cprocessors)
    {
      engine_.enqueue (*procp);
      if (procp->n_obuses())
        last_output_ = procp.get();
    }
  // last_output_ is only valid during render()
}

void
Chain::render (uint n_frames)
{
  // make the last processor output the chain output
  constexpr OBusId OUT1 = OBusId (1);
  const size_t nlastchannels = last_output_ ? last_output_->n_ochannels (OUT1) : 0;
  const size_t n_och = n_ochannels (OUT1);
  for (size_t c = 0; c < n_och; c++)
    {
      // an enqueue_children() call is guranteed *before* render(), so last_output_ is valid
      if (last_output_)
        redirect_oblock (OUT1, c, last_output_->ofloats (OUT1, std::min (c, nlastchannels - 1)));
      else
        assign_oblock (OUT1, c, 0);
    }
}

/// Return the number of Processor instances in the Chain.
size_t
Chain::size ()
{
  const ProcessorVec &cprocessors = processors_mt_;
  return cprocessors.size();
}

/// Return the Processor at position `nth` in the Chain.
ProcessorP
Chain::at (uint nth)
{
  return_unless (nth < size(), {});
  const ProcessorVec &cprocessors = processors_mt_;
  return cprocessors[nth];
}

/// Return the index of Processor `proc` in the Chain.
size_t
Chain::find_pos (Processor &proc)
{
  const ProcessorVec &cprocessors = processors_mt_;
  for (size_t i = 0; i < cprocessors.size(); i++)
    if (cprocessors[i].get() == &proc)
      return i;
  return  ~size_t (0);
}

/// Remove a previously added Processor `proc` from the Chain.
bool
Chain::remove (Processor &proc)
{
  std::vector<Processor*> unconnected;
  ProcessorP processorp;
  const ProcessorVec &cprocessors = processors_mt_;
  size_t pos; // find proc
  for (pos = 0; pos < cprocessors.size(); pos++)
    if (cprocessors[pos].get() == &proc)
      {
        processorp = cprocessors[pos]; // and remove...
        std::lock_guard<std::mutex> locker (mt_mutex_);
        processors_mt_.erase (processors_mt_.begin() + pos);
        break;
      }
  if (!processorp)
    return false;
  // clear stale connections
  pm_disconnect_ibuses (*processorp);
  pm_disconnect_obuses (*processorp);
  // fixup following connections
  reconnect (pos);
  return true;
}

/// Add a new Processor `proc` at position `pos` to the Chain.
/// The Processor `proc` must not be previously contained by any other ProcessorManager.
void
Chain::insert (ProcessorP proc, size_t pos)
{
  assert_return (proc != nullptr);
  const ProcessorVec &cprocessors = processors_mt_;
  const size_t index = CLAMP (pos, 0, cprocessors.size());
  {
    std::lock_guard<std::mutex> locker (mt_mutex_);
    processors_mt_.insert (processors_mt_.begin() + index, proc);
  }
  // fixup following connections
  reconnect (index);
  engine_.reschedule();
}

/// Reconnect Chain processors at start and after.
void
Chain::reconnect (size_t start)
{
  const ProcessorVec &cprocessors = processors_mt_;
  // clear stale inputs
  for (size_t i = start; i < cprocessors.size(); i++)
    pm_disconnect_ibuses (*cprocessors[i]);
  // reconnect pairwise
  for (size_t i = start; i < cprocessors.size(); i++)
    chain_up (*(i ? cprocessors[i - 1] : inlet_), *cprocessors[i]);
}

/// Connect the main audio input of `next` to audio output of `prev`.
uint
Chain::chain_up (Processor &prev, Processor &next)
{
  assert_return (this != &prev, 0);
  assert_return (this != &next, 0);
  const uint ni = next.n_ibuses();
  const uint no = prev.n_obuses();
  // assign event source
  if (eproc_)
    {
      if (prev.has_event_input())
        pm_connect_events (*eproc_, prev);
      if (next.has_event_input())
        pm_connect_events (*eproc_, next);
    }
  // check need for audio connections
  if (ni == 0 || no == 0)
    {
      logstate (MISS, &prev, OBusId (no ? 1 : 0), nullptr, &next, IBusId (ni ? 1 : 0), nullptr);
      return 0; // nothing to do
    }
  uint n_connected = 0;
  // try to connect prev main obus (1) with next main ibus (1)
  const OBusId obusid { 1 };
  const IBusId ibusid { 1 };
  SpeakerArrangement ospa = speaker_arrangement_channels (prev.bus_info (obusid).speakers);
  SpeakerArrangement ispa = speaker_arrangement_channels (next.bus_info (ibusid).speakers);
  // logstate (HAVE, &prev, obusid, &ospa, &next, ibusid, &ispa);
  if (ospa != ispa)
    {
#if 0
      // try to increase output channels if possible
      if (speaker_arrangement_count_channels (ospa) < speaker_arrangement_count_channels (ispa) &&
          is_unconnected (prev)) // avoid reconfiguration of existing connections
        {
          logstate (REQUEST, &prev, obusid, &ispa, &next, ibusid, &ispa);
          prev.reconfigure (IBusId (0), SpeakerArrangement::NONE, obusid, ispa);
          ospa = speaker_arrangement_channels (prev.bus_info (obusid).speakers);
          logstate (HAVE, &prev, obusid, &ospa, &next, ibusid, &ispa);
        }
#endif
    }
  // try to adjust input channels
  if (ospa != ispa)
    {
      logstate (REQUEST, &prev, obusid, &ospa, &next, ibusid, &ospa);
      pm_reconfigure (next, ibusid, ospa, OBusId (0), SpeakerArrangement::NONE);
      ispa = speaker_arrangement_channels (next.bus_info (ibusid).speakers);
      logstate (HAVE, &prev, obusid, &ospa, &next, ibusid, &ispa);
    }
  if (0 == (uint64_t (ispa) & ~uint64_t (ospa)) || // exact match
      (ospa == SpeakerArrangement::MONO &&         // allow MONO -> STEREO connections
       ispa == SpeakerArrangement::STEREO))
    {
      logstate (MATCH, &prev, obusid, &ospa, &next, ibusid, &ispa);
      n_connected += speaker_arrangement_count_channels (ispa);
      pm_connect (next, ibusid, prev, obusid);
    }
  else
    logstate (MISS, &prev, obusid, &ospa, &next, ibusid, &ispa);
  return n_connected;
}

Chain::ProcessorVec
Chain::list_processors_mt () const
{
  std::lock_guard<std::mutex> locker (const_cast<Chain*> (this)->mt_mutex_);
  return processors_mt_; // copy with guard
}

// == Engine ==
enum {
  RESCHEDULE = 1,
};
Engine::Engine (uint32 samplerate, AudioTiming &atiming) :
  nyquist_ (samplerate * 0.5), inyquist_ (1.0 / nyquist_), sample_rate_ (samplerate),
  frame_counter_ (MAX_RENDER_BLOCK_SIZE), flags_ (0), scheduler_depth_ (0),
  timing { atiming }
{
  assert_return (samplerate > 0);
  assert_return (nyquist_ > 0 && nyquist_ == (samplerate >> 1));
  assert_return (0 == (samplerate & 3));
  schedule_.reserve (256);
  reschedule();
}

void
Engine::add_root (ProcessorP rootproc)
{
  assert_return (rootproc);
  std::lock_guard<std::mutex> locker (mutex_);
  auto pos = std::find (roots_.begin(), roots_.end(), rootproc);
  bool was_root = pos != roots_.end();
  assert_return (!was_root);
  roots_.push_back (rootproc);
  reschedule();
}

bool
Engine::del_root (ProcessorP rootproc)
{
  assert_return (rootproc, false);
  std::lock_guard<std::mutex> locker (mutex_);
  const bool was_root = vector_erase_element (roots_, rootproc);
  reschedule();
  return was_root;
}

// Cause the engine to re-schedule Processors before the next process() cycle.
void
Engine::reschedule()
{
  flags_ |= RESCHEDULE;
}

bool
Engine::in_schedule (Processor &proc)
{
  // TODO: this can be optimized via unsorted_set when profiled
  for (auto procp : schedule_)
    if (procp == &proc)
      return true;
  return false;
}

void
Engine::make_schedule ()
{
  assert_return (scheduler_depth_ == 0);
  return_unless (flags_ & RESCHEDULE);
  std::lock_guard<std::mutex> locker (mutex_);
  if (0 == (flags_ & RESCHEDULE))
    return;
  flags_ &= ~uint (RESCHEDULE);
  schedule_.clear();
  scheduler_depth_ += 1;
  for (auto root : roots_)
    enqueue (*root);
  scheduler_depth_ -= 1;
  for (auto proc : schedule_)
    proc->reset_state();
}

void
Engine::enqueue (Processor &proc)
{
  assert_return (this == &proc.engine_);
  assert_return (scheduler_depth_ > 0 && scheduler_depth_ <= 999);
  scheduler_depth_ += 1;
  proc.enqueue_deps();
  scheduler_depth_ -= 1;
  if (!in_schedule (proc))
    schedule_.push_back (&proc);
}

/// Render a block of MAX_RENDER_BLOCK_SIZE in all Processors connected to this Engine.
void
Engine::render_block()
{
  assert_return (!(flags_ & RESCHEDULE));
  frame_counter_ += MAX_RENDER_BLOCK_SIZE;
  for (auto procp : schedule_)
    procp->render_block();
  /* TODO: concurrent rendering:
     - rework schedule_ into a dependency tree, each node reflects a processor and its dependencies
     - a worker starts on a node, processes dependencies in depth first fashion
     - per level, 1: process all dependencies that are not yet done and not processing (needs atomic flag)
     - per level, 2: wait for all nodes (in turn) that are not yet done
     - per level, 3: process node itself, return
     - experiment: *also* maintain linear schedule_, have unused workers race over schedule_ concurrently
   */
}

} // AudioSignal

// == ComboImpl ==
ComboImpl::ComboImpl (AudioSignal::ChainP combop) :
  combo_ (combop)
{
  assert_return (combop != nullptr);
}

DeviceInfoSeq
ComboImpl::list_processor_types ()
{
  using namespace AudioSignal;
  DeviceInfoSeq iseq;
  const auto rlist = Processor::registry_list();
  iseq.reserve (rlist.size());
  for (const auto &entry : rlist)
    {
      DeviceInfo info;
      info.uri          = entry.uri;
      info.name         = entry.label;
      info.category     = entry.category;
      info.description  = entry.description;
      info.website_url  = entry.website_url;
      info.creator_name = entry.creator_name;
      info.creator_url  = entry.creator_url;
      iseq.push_back (info);
    }
  return iseq;
}

ProcessorSeq
ComboImpl::list_processors ()
{
  ProcessorSeq seq;
  for (auto &p : combo_->list_processors_mt())
    {
      Bse::ProcessorImplP procimplp = p->access_processor();
      seq.push_back (procimplp);
    }
  return seq;
}

ProcessorIfaceP
ComboImpl::create_processor (const std::string &uuiduri)
{
  AudioSignal::ProcessorP procp = AudioSignal::Processor::registry_create (BSE_SERVER.global_engine(), uuiduri);
  return_unless (procp, nullptr);
  std::shared_ptr<AudioSignal::Chain> combo = combo_;
  BSE_SERVER.commit_job ([combo, procp] () {
    combo->insert (procp);
  });
  return procp->access_processor();
}

ProcessorIfaceP
ComboImpl::create_processor_before (const std::string &uuiduri, ProcessorIface &sibling)
{
  Bse::ProcessorImplP procimplp = std::dynamic_pointer_cast<ProcessorImpl> (sibling.shared_from_this());
  AudioSignal::ProcessorP siblingp = procimplp ? procimplp->audio_signal_processor() : nullptr;
  return_unless (siblingp, nullptr);
  AudioSignal::ProcessorP procp = AudioSignal::Processor::registry_create (BSE_SERVER.global_engine(), uuiduri);
  return_unless (procp, nullptr);
  std::shared_ptr<AudioSignal::Chain> combo = combo_;
  BSE_SERVER.commit_job ([combo, procp, siblingp] () {
    combo->insert (procp, combo->find_pos (*siblingp));
  });
  return procp->access_processor();
}

bool
ComboImpl::remove_processor (ProcessorIface &sub)
{
  Bse::ProcessorImplP procimplp = std::dynamic_pointer_cast<ProcessorImpl> (sub.shared_from_this());
  AudioSignal::ProcessorP procp = procimplp ? procimplp->audio_signal_processor() : nullptr;
  return_unless (procp, false);
  for (auto &p : combo_->list_processors_mt())
    if (p.get() == procp.get())
      {
        std::shared_ptr<AudioSignal::Chain> combo = combo_;
        BSE_SERVER.commit_job ([combo, procp] () {
          if (!combo->remove (*procp))
            Bse::warning ("Child of AudioSignal::Processor vanished during removal: %s", combo->debug_name());
        });
        return true;
      }
  return false;
}

const AudioSignal::ProcessorP
ComboImpl::audio_signal_processor () const
{
  return combo_;
}

} // Bse
