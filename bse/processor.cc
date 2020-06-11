// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "processor.hh"
#include "bse/bseserver.hh"
#include "bse/internal.hh"
#include <shared_mutex>

#define PDEBUG(...)     Bse::debug ("processor", __VA_ARGS__)

namespace Bse {

/** This namespace provides the components for all audio signal processing modules.
 * All audio processing classes derive from AudioSignal::Processor.
 * Other components in this namespace are either used by the Processor base class
 * or help to drive instances of Processor subclasses.
 */
namespace AudioSignal {

// == SpeakerArrangement ==
// Count the number of channels described by the SpeakerArrangement.
uint8
speaker_arrangement_count_channels (SpeakerArrangement spa)
{
  const uint64_t bits = uint64_t (speaker_arrangement_channels (spa));
  if_constexpr (sizeof (bits) == sizeof (long))
    return __builtin_popcountl (bits);
  return __builtin_popcountll (bits);
}

// Check if the SpeakerArrangement describes auxillary channels.
bool
speaker_arrangement_is_aux (SpeakerArrangement spa)
{
  return uint64_t (spa) & uint64_t (SpeakerArrangement::AUX);
}

// Retrieve the bitmask describing the SpeakerArrangement channels.
SpeakerArrangement
speaker_arrangement_channels (SpeakerArrangement spa)
{
  const uint64_t bits = uint64_t (spa) & uint64_t (speaker_arrangement_channels_mask);
  return SpeakerArrangement (bits);
}

const char*
speaker_arrangement_bit_name (SpeakerArrangement spa)
{
  switch (spa)
    { // https://wikipedia.org/wiki/Surround_sound
    case SpeakerArrangement::NONE:              	return "-";
      // case SpeakerArrangement::MONO:                 return "Mono";
    case SpeakerArrangement::FRONT_LEFT:        	return "FL";
    case SpeakerArrangement::FRONT_RIGHT:       	return "FR";
    case SpeakerArrangement::FRONT_CENTER:      	return "FC";
    case SpeakerArrangement::LOW_FREQUENCY:     	return "LFE";
    case SpeakerArrangement::BACK_LEFT:         	return "BL";
    case SpeakerArrangement::BACK_RIGHT:                return "BR";
    case SpeakerArrangement::AUX:                       return "AUX";
    case SpeakerArrangement::STEREO:                    return "Stereo";
    case SpeakerArrangement::STEREO_21:                 return "Stereo-2.1";
    case SpeakerArrangement::STEREO_30:	                return "Stereo-3.0";
    case SpeakerArrangement::STEREO_31:	                return "Stereo-3.1";
    case SpeakerArrangement::SURROUND_50:	        return "Surround-5.0";
    case SpeakerArrangement::SURROUND_51:	        return "Surround-5.1";
#if 0 // TODO: dynamic multichannel support
    case SpeakerArrangement::FRONT_LEFT_OF_CENTER:      return "FLC";
    case SpeakerArrangement::FRONT_RIGHT_OF_CENTER:     return "FRC";
    case SpeakerArrangement::BACK_CENTER:               return "BC";
    case SpeakerArrangement::SIDE_LEFT:	                return "SL";
    case SpeakerArrangement::SIDE_RIGHT:	        return "SR";
    case SpeakerArrangement::TOP_CENTER:	        return "TC";
    case SpeakerArrangement::TOP_FRONT_LEFT:	        return "TFL";
    case SpeakerArrangement::TOP_FRONT_CENTER:	        return "TFC";
    case SpeakerArrangement::TOP_FRONT_RIGHT:	        return "TFR";
    case SpeakerArrangement::TOP_BACK_LEFT:	        return "TBL";
    case SpeakerArrangement::TOP_BACK_CENTER:	        return "TBC";
    case SpeakerArrangement::TOP_BACK_RIGHT:	        return "TBR";
    case SpeakerArrangement::SIDE_SURROUND_50:	        return "Side-Surround-5.0";
    case SpeakerArrangement::SIDE_SURROUND_51:	        return "Side-Surround-5.1";
#endif
    }
  return nullptr;
}

std::string
speaker_arrangement_desc (SpeakerArrangement spa)
{
  const bool isaux = speaker_arrangement_is_aux (spa);
  const SpeakerArrangement chan = speaker_arrangement_channels (spa);
  const char *chname = SpeakerArrangement::MONO == chan ? "Mono" : speaker_arrangement_bit_name (chan);
  std::string s (chname ? chname : "<INVALID>");
  if (isaux)
    s = std::string (speaker_arrangement_bit_name (SpeakerArrangement::AUX)) + "(" + s + ")";
  return s;
}

// == ChoiceEntries ==
ChoiceEntries&
ChoiceEntries::operator+= (const ChoiceDetails &ce)
{
  push_back (ce);
  return *this;
}

// == ParamInfo ==
static constexpr uint PTAG_FLOATS = 1;
static constexpr uint PTAG_CENTRIES = 2;

ParamInfo::ParamInfo (ParamId pid) :
  id (pid), union_tag (0)
{
  memset (&u, 0, sizeof (u));
}

ParamInfo::~ParamInfo()
{
  release();
  while (!notifiers_.empty())
    {
      Callback *c = notifiers_.back();
      notifiers_.pop_back();
      delete c;
    }
  assert_return (next_notifier_ == MAX_NOTIFIER); // notifies should *not* be running
}

void
ParamInfo::copy_fields (const ParamInfo &src)
{
  identifier = src.identifier;
  display_name = src.display_name;
  short_name = src.short_name;
  description = src.description;
  unit = src.unit;
  hints = src.hints;
  group = src.group;
  switch (src.union_tag)
    {
    case PTAG_FLOATS:
      set_range (src.u.fmin, src.u.fmax, src.u.fstep);
      break;
    case PTAG_CENTRIES:
      set_choices (*src.u.centries());
      break;
    }
  assert_return (notifiers_.empty());
  assert_return (next_notifier_ == MAX_NOTIFIER);
}

void
ParamInfo::release()
{
  const bool destroy = union_tag == PTAG_CENTRIES;
  union_tag = 0;
  if (destroy)
    u.centries()->~ChoiceEntries();
}

/// Clear all ParamInfo fields.
void
ParamInfo::clear ()
{
  identifier = "";
  display_name = "";
  short_name = "";
  description = "";
  unit = "";
  hints = "";
  group = "";
  release();
}

/// Get parameter range minimum and maximum.
ParamInfo::MinMax
ParamInfo::get_minmax () const
{
  switch (union_tag)
    {
    case PTAG_FLOATS:
      return { u.fmin, u.fmax };
    case PTAG_CENTRIES:
      return { 0, u.centries()->size() };
    default:
      return { NAN, NAN };
    }
}

/// Get parameter range properties.
void
ParamInfo::get_range (double &fmin, double &fmax, double &fstep) const
{
  switch (union_tag)
    {
    case PTAG_FLOATS:
      fmin = u.fmin;
      fmax = u.fmax;
      fstep = u.fstep;
      break;
    case PTAG_CENTRIES:
      fmin = 0;
      fmax = u.centries()->size();
      fstep = 1;
      break;
    default:
      fmin = NAN;
      fmax = NAN;
      fstep = NAN;
      break;
    }
}

/// Assign range properties to parameter.
void
ParamInfo::set_range (double fmin, double fmax, double fstep)
{
  release();
  union_tag = PTAG_FLOATS;
  u.fmin = fmin;
  u.fmax = fmax;
  u.fstep = fstep;
}

/// Get parameter choice list.
const ChoiceEntries&
ParamInfo::get_choices () const
{
  if (union_tag == PTAG_CENTRIES)
    return *u.centries();
  static const ChoiceEntries empty;
  return empty;
}

/// Assign choice list to parameter via vector move.
void
ParamInfo::set_choices (ChoiceEntries &&centries)
{
  release();
  union_tag = PTAG_CENTRIES;
  new (u.centries()) ChoiceEntries (std::move (centries));
}

/// Assign choice list to parameter via deep copy.
void
ParamInfo::set_choices (const ChoiceEntries &centries)
{
  set_choices (ChoiceEntries (centries));
}

size_t
ParamInfo::add_notify (ProcessorP proc, const std::function<void()> &callback)
{
  assert_return (this_thread_is_bse(), -1);
  assert_return (callback, -1);
  assert_return (proc, -1);
  assert_return (proc->is_initialized(), -1);
  if (notifiers_.empty())
    Processor::param_notifies_mt (proc, id, true);
  notifiers_.push_back (new Callback (callback));
  return size_t (notifiers_.back());
}

bool
ParamInfo::del_notify (ProcessorP proc, size_t callbackid)
{
  assert_return (this_thread_is_bse(), false);
  assert_return (proc, false);
  for (size_t i = 0; i < notifiers_.size(); ++i)
    {
      Callback *c = notifiers_[i];
      static_assert (sizeof (size_t) == sizeof (c));
      if (size_t (c) != callbackid)
        continue;
      if (next_notifier_ != MAX_NOTIFIER && next_notifier_ > i)
        next_notifier_ -= 1;
      notifiers_.erase (notifiers_.begin() + i);
      delete c;
      if (notifiers_.empty())
        Processor::param_notifies_mt (proc, id, false);
      return true;
    }
  return false;
}

void
ParamInfo::call_notify ()
{
  assert_return (this_thread_is_bse());
  if (next_notifier_ != MAX_NOTIFIER)
    {
      next_notifier_ = 0; // restart notifications
      return;
    }
  next_notifier_ = 0;
  while (next_notifier_ < notifiers_.size())
    {
      Callback *c = notifiers_[next_notifier_];
      next_notifier_ += 1;
      (*c) (); // may change next_notifier_
    }
  next_notifier_ = MAX_NOTIFIER;
}

// == PBus ==
Processor::PBus::PBus (const std::string &ident, SpeakerArrangement sa) :
  ibus (ident, sa)
{}

Processor::IBus::IBus (const std::string &ident, SpeakerArrangement sa)
{
  speakers = sa;
  proc = nullptr;
  obusid = {};
  identifier = ident;
  assert_return (identifier != "");
}

Processor::OBus::OBus (const std::string &ident, SpeakerArrangement sa)
{
  speakers = sa;
  fbuffer_concounter = 0;
  fbuffer_count = 0;
  fbuffer_index = ~0;
  identifier = ident;
  assert_return (identifier != "");
}

// == RenderSetup ==
RenderSetup::RenderSetup (uint32 samplerate, AudioTiming &atiming) :
  mix_freq (samplerate), sample_rate (samplerate), timing { atiming }
{
  assert_return (sample_rate > 0);
}

// == Processor ==
uint64 __thread Processor::tls_timestamp = 0;

/// Constructor for Processor
Processor::Processor ()
{}

/// The destructor is called when the last std::shared_ptr<> reference drops.
Processor::~Processor ()
{
  release_iobufs();
}

const Processor::FloatBuffer&
Processor::zero_buffer()
{
  static const FloatBuffer const_zero_float_buffer {};
  return const_zero_float_buffer;
}

/// Get internal output bus handle.
Processor::OBus&
Processor::iobus (OBusId obusid)
{
  const size_t busindex = size_t (obusid) - 1;
  if (BSE_ISLIKELY (busindex < n_obuses()))
    return iobuses_[output_offset_ + busindex].obus;
  static const OBus dummy { "?", {} };
  assert_return (busindex < n_obuses(), const_cast<OBus&> (dummy));
  return const_cast<OBus&> (dummy);
}

/// Get internal input bus handle.
Processor::IBus&
Processor::iobus (IBusId ibusid)
{
  const size_t busindex = size_t (ibusid) - 1;
  if (BSE_ISLIKELY (busindex < n_ibuses()))
    return iobuses_[busindex].ibus;
  static const IBus dummy { "?", {} };
  assert_return (busindex < n_ibuses(), const_cast<IBus&> (dummy));
  return const_cast<IBus&> (dummy);
}

// Release buffers allocated for input/output channels.
void
Processor::release_iobufs()
{
  disconnect_ibuses();
  disconnect_obuses();
  for (OBusId ob = OBusId (1); size_t (ob) <= n_obuses(); ob = OBusId (size_t (ob) + 1))
    {
      OBus &bus = iobus (ob);
      bus.fbuffer_index = ~0;
      bus.fbuffer_count = 0;
    }
  // trivial destructors (of FloatBuffer) need not be called
  static_assert (std::is_trivially_destructible<decltype (fbuffers_[0])>::value);
  fast_mem_free (fbuffers_);
  fbuffers_ = nullptr;
}

// Allocate and assign output channel buffers.
void
Processor::assign_iobufs ()
{
  ssize_t ochannel_count = 0;
  for (OBusId ob = OBusId (1); size_t (ob) <= n_obuses(); ob = OBusId (size_t (ob) + 1))
    {
      OBus &bus = iobus (ob);
      bus.fbuffer_index = ochannel_count;
      bus.fbuffer_count = bus.n_channels();
      ochannel_count += bus.fbuffer_count;
    }
  fast_mem_free (fbuffers_);
  if (ochannel_count > 0)
    {
      fbuffers_ = (FloatBuffer*) fast_mem_alloc (ochannel_count * sizeof (FloatBuffer));
      for (ssize_t i = 0; i < ochannel_count; i++)
        new (fbuffers_ + i) FloatBuffer();
    }
  else
    fbuffers_ = nullptr;
}

static __thread CString tls_param_group;

/// Introduce a `ParamInfo.group` to be used for the following add_param() calls.
void
Processor::start_param_group (const std::string &groupname) const
{
  tls_param_group = groupname;
}

/// Add a new parameter with unique `ParamInfo.identifier`.
/// The returned `ParamId` is forced to match `id` (and must be unique).
ParamId
Processor::add_param (ParamId id, const ParamInfo &infotmpl, double value)
{
  assert_return (!is_initialized(), {});
  assert_return (infotmpl.identifier != "", {});
  PParam param { id, infotmpl };
  if (param.info->group.empty())
    param.info->group = tls_param_group;
  using P = decltype (params_);
  std::pair<P::iterator, bool> existing_parameter_position =
    binary_lookup_insertion_pos (params_.begin(), params_.end(), PParam::cmp, param);
  assert_return (existing_parameter_position.second == false, {});
  params_.insert (existing_parameter_position.first, std::move (param));
  set_param (param.id, value); // forces dirty
  return param.id;
}

/// Add new range parameter with most `ParamInfo` fields as inlined arguments.
/// The returned `ParamId` is newly assigned and increases with the number of parameters added.
ParamId
Processor::add_param (const std::string &identifier, const std::string &display_name,
                      const std::string &short_name, double pmin, double pmax,
                      const std::string &hints, double value, const std::string &unit)
{
  ParamInfo info;
  info.identifier = identifier;
  info.display_name = display_name;
  info.short_name = short_name;
  info.hints = hints;
  info.unit = unit;
  info.set_range (pmin, pmax);
  ParamId id = ParamId (1 + params_.size());
  return add_param (id, info, value);
}

/// Add new choice parameter with most `ParamInfo` fields as inlined arguments.
ParamId
Processor::add_param (const std::string &identifier, const std::string &display_name,
                      const std::string &short_name, ChoiceEntries &&centries,
                      const std::string &hints, double value, const std::string &unit)
{
  ParamInfo info;
  info.identifier = identifier;
  info.display_name = display_name;
  info.short_name = short_name;
  info.hints = hints;
  info.unit = unit;
  info.set_choices (std::move (centries));
  ParamId id = ParamId (1 + params_.size());
  return add_param (id, info, value);
}

/// List all Processor parameters.
auto
Processor::list_params() const -> ParamInfoPVec
{
  assert_return (is_initialized(), {});
  ParamInfoPVec iv;
  iv.reserve (params_.size());
  for (const PParam &p : params_)
    iv.push_back (p.info);
  return iv;
}

/// Return the ParamId for parameter `identifier` or else 0.
auto
Processor::find_param (const std::string &identifier) const -> MaybeParamId
{
  auto ident = CString::lookup (identifier);
  if (!ident.empty())
    for (const PParam &p : params_)
      if (p.info->identifier == ident)
        return std::make_pair (p.id, true);
  return std::make_pair (ParamId (0), false);
}

// Non-fastpath implementation of find_param().
Processor::PParam*
Processor::find_pparam_ (ParamId paramid)
{
  // lookup id with gaps
  const PParam key { paramid };
  auto iter = binary_lookup (params_.begin(), params_.end(), PParam::cmp, key);
  const bool found_paramid = iter != params_.end();
  if (ISLIKELY (found_paramid))
    return &*iter;
  assert_return (found_paramid == true, nullptr);
  return nullptr;
}

/// Retrieve supplemental information for parameters, usually to enhance the user interface.
ParamInfoP
Processor::param_info (ParamId paramid) const
{
  PParam *param = const_cast<Processor*> (this)->find_pparam (paramid);
  return BSE_ISLIKELY (param) ? param->info : nullptr;
}

/// Fetch the current parameter value of a Processor from any thread.
/// This function is MT-Safe after proper Processor initialization.
double
Processor::peek_param_mt (ProcessorP proc, ParamId pid)
{
  assert_return (proc, FP_NAN);
  assert_return (proc->is_initialized(), FP_NAN);
  PParam *param = proc->find_pparam (pid);
  return BSE_ISLIKELY (param) ? param->peek_value() : FP_NAN;
}

/// Enable/disable notification sending for a Processor parameter.
/// This function is MT-Safe after proper Processor initialization.
void
Processor::param_notifies_mt (ProcessorP proc, ParamId pid, bool need_notifies)
{
  assert_return (proc);
  assert_return (proc->is_initialized());
  PParam *param = proc->find_pparam (pid);
  if (BSE_ISLIKELY (param))
    param->must_notify (need_notifies);
}

/// Check if Processor has been properly intiialized (so the set parameters is fixed).
bool
Processor::is_initialized () const
{
  return flags_ & INITIALIZED;
}

/// Retrieve the minimum / maximum values for a parameter.
Processor::MinMax
Processor::param_range (ParamId paramid) const
{
  ParamInfo *info = param_info (paramid).get();
  return info ? info->get_minmax() : MinMax { FP_NAN, FP_NAN };
}

String
Processor::debug_name () const
{
  ProcessorInfo info;
  const_cast<Processor*> (this)->query_info (info);
  return info.label.empty() ? info.uri : info.label;
}

/// Mandatory method that provides unique URI, display label and registration information.
/// Depending on the host, this method may be called often or only once per subclass type.
/// The field `uri` must be a globally unique URI (or UUID), that is can be used as unique
/// identifier for reliable (de-)serialization.
void
Processor::query_info (ProcessorInfo &info)
{}

/// Mandatory method to setup parameters (see add_param()) and initialize internal structures.
/// This method will be called once per instance after construction.
void
Processor::initialize ()
{
  assert_return (n_ibuses() + n_obuses() == 0);
}

/// Mandatory method to setup IO buses (see add_input_bus() / add_output_bus()).
/// Depending on the host, this method may be called multiple times with different arrangements.
void
Processor::configure (uint n_ibuses, const SpeakerArrangement *ibuses,
                      uint n_obuses, const SpeakerArrangement *obuses)
{}

/// Add an input bus with `name` and channels configured via `speakerarrangement`.
IBusId
Processor::add_input_bus (CString name, SpeakerArrangement speakerarrangement)
{
  const IBusId zero {0};
  assert_return (name != "", zero);
  assert_return (uint64_t (speaker_arrangement_channels (speakerarrangement)) > 0, zero);
  assert_return (iobuses_.size() < 65535, zero);
  if (n_ibuses())
    assert_return (name != iobus (IBusId (n_ibuses())).identifier, zero); // easy CnP error
  PBus pbus { name, speakerarrangement };
  iobuses_.insert (iobuses_.begin() + output_offset_, pbus);
  output_offset_ += 1;
  const IBusId id = IBusId (n_ibuses());
  return id; // 1 + index
}

/// Add an output bus with `name` and channels configured via `speakerarrangement`.
OBusId
Processor::add_output_bus (CString name, SpeakerArrangement speakerarrangement)
{
  const OBusId zero {0};
  assert_return (name != "", zero);
  assert_return (uint64_t (speaker_arrangement_channels (speakerarrangement)) > 0, zero);
  assert_return (iobuses_.size() < 65535, zero);
  if (n_obuses())
    assert_return (name != iobus (OBusId (n_obuses())).identifier, zero); // easy CnP error
  PBus pbus { name, speakerarrangement };
  iobuses_.push_back (pbus);
  const OBusId id = OBusId (n_obuses());
  return id; // 1 + index
}

/// Return the IBusId for input bus `name` or else 0.
IBusId
Processor::find_ibus (const std::string &name) const
{
  auto ident = CString::lookup (name);
  if (!ident.empty())
    for (IBusId ib = IBusId (1); size_t (ib) <= n_ibuses(); ib = IBusId (size_t (ib) + 1))
      if (iobus (ib).identifier == ident)
        return ib;
  return IBusId (0);
}

/// Return the OBusId for output bus `name` or else 0.
OBusId
Processor::find_obus (const std::string &name) const
{
  auto ident = CString::lookup (name);
  if (!ident.empty())
    for (OBusId ob = OBusId (1); size_t (ob) <= n_obuses(); ob = OBusId (size_t (ob) + 1))
      if (iobus (ob).identifier == ident)
        return ob;
  return OBusId (0);
}

/// Retrieve an input channel float buffer.
/// The returned FloatBuffer is not modifyable, for Processor implementations
/// that need to modify the FloatBuffer of an output channel, see float_buffer(OBusId,uint).
const Processor::FloatBuffer&
Processor::float_buffer (IBusId busid, uint channelindex) const
{
  const size_t ibusindex = size_t (busid) - 1;
  assert_return (ibusindex < n_ibuses(), zero_buffer());
  const IBus &ibus = iobus (busid);
  if (ibus.proc)
    {
      const Processor &oproc = *ibus.proc;
      const OBus &obus = oproc.iobus (ibus.obusid);
      if (BSE_UNLIKELY (channelindex >= obus.fbuffer_count))
        channelindex = obus.fbuffer_count;
      return oproc.fbuffers_[obus.fbuffer_index + channelindex];
    }
  return zero_buffer();
}

/// Retrieve an output channel float buffer.
/// Note that this function reassigns FloatBuffer.buffer pointer to FloatBuffer.fblock,
/// resetting any previous redirections. Assignments to FloatBuffer.buffer
/// enable new redirections until the next call to this function or oblock().
Processor::FloatBuffer&
Processor::float_buffer (OBusId busid, uint channelindex, bool resetptr)
{
  const size_t obusindex = size_t (busid) - 1;
  FloatBuffer *const fallback = const_cast<FloatBuffer*> (&zero_buffer());
  assert_return (obusindex < n_obuses(), *fallback);
  const OBus &obus = iobus (busid);
  assert_return (channelindex < obus.fbuffer_count, *fallback);
  FloatBuffer &fbuffer = fbuffers_[obus.fbuffer_index + channelindex];
  if (resetptr)
    fbuffer.buffer = &fbuffer.fblock[0];
  return fbuffer;
}

/// Redirect output buffer of bus `b`, channel `c` to point to `block`.
void
Processor::redirect_oblock (OBusId busid, uint channelindex, const float *block)
{
  const size_t obusindex = size_t (busid) - 1;
  assert_return (obusindex < n_obuses());
  const OBus &obus = iobus (busid);
  assert_return (channelindex < obus.fbuffer_count);
  FloatBuffer &fbuffer = fbuffers_[obus.fbuffer_index + channelindex];
  assert_return (block != nullptr);
  fbuffer.buffer = const_cast<float*> (block);
}

/// Fill the output buffer of bus `b`, channel `c` with `v`.
void
Processor::assign_oblock (OBusId b, uint c, float v)
{
  float *const buffer = oblock (b, c);
  floatfill (buffer, v, MAX_RENDER_BLOCK_SIZE);
  // TODO: optimize assign_oblock() via redirect to const value blocks
}

/// Indicator for connected output buses.
/// Not connected output bus buffers do not need to be filled.
bool
Processor::connected (OBusId obusid) const
{
  const size_t obusindex = size_t (obusid) - 1;
  assert_return (obusindex < n_obuses(), false);
  return iobus (obusid).fbuffer_concounter;
}

/// Remove existing bus configurations, useful at the start of configure().
void
Processor::remove_all_buses ()
{
  release_iobufs();
  iobuses_.clear();
  output_offset_ = 0;
}

/// Reset input bus buffer data.
void
Processor::disconnect_ibuses()
{
  for (size_t i = 0; i < n_ibuses(); i++)
    disconnect (IBusId (1 + i));
}

/// Disconnect inputs of all Processors that are connected to outputs of `this`.
void
Processor::disconnect_obuses()
{
  return_unless (fbuffers_);
  while (outputs_.size())
    {
      const auto o = outputs_.back();
      o.proc->disconnect (o.ibusid);
    }
}

/// Disconnect input `ibusid`.
void
Processor::disconnect (IBusId ibusid)
{
  const size_t ibusindex = size_t (ibusid) - 1;
  assert_return (ibusindex < n_ibuses());
  IBus &ibus = iobus (ibusid);
  // disconnect
  return_unless (ibus.proc != nullptr);
  Processor &oproc = *ibus.proc;
  const size_t obusindex = size_t (ibus.obusid) - 1;
  assert_return (obusindex < oproc.n_obuses());
  OBus &obus = oproc.iobus (ibus.obusid);
  assert_return (obus.fbuffer_concounter > 0);
  obus.fbuffer_concounter -= 1; // connection counter
  const bool backlink = vector_erase_element (oproc.outputs_, { this, ibusid });
  ibus.proc = nullptr;
  ibus.obusid = {};
  assert_return (backlink == true);
}

/// Connect input `ibusid` to output `obusid` of Processor `prev`.
void
Processor::connect (IBusId ibusid, Processor &oproc, OBusId obusid)
{
  const size_t ibusindex = size_t (ibusid) - 1;
  assert_return (ibusindex < n_ibuses());
  const size_t obusindex = size_t (obusid) - 1;
  assert_return (obusindex < oproc.n_obuses());
  disconnect (ibusid);
  IBus &ibus = iobus (ibusid);
  const uint n_ichannels = ibus.n_channels();
  OBus &obus = oproc.iobus (obusid);
  const uint n_ochannels = obus.n_channels();
  // match channels
  assert_return (n_ichannels <= n_ochannels ||
                 (ibus.speakers == SpeakerArrangement::STEREO &&
                  obus.speakers == SpeakerArrangement::MONO));
  // connect
  ibus.proc = &oproc;
  ibus.obusid = obusid;
  obus.fbuffer_concounter += 1; // conection counter
  oproc.outputs_.push_back ({ this, ibusid });
}

/// Ensure `Processor::initialize()` has been called, so the parameters are fixed.
void
Processor::ensure_initialized()
{
  if (!is_initialized())
    {
      tls_param_group = "";
      initialize();
      tls_param_group = "";
      flags_ |= INITIALIZED;
      const SpeakerArrangement ibuses = SpeakerArrangement::STEREO;
      const SpeakerArrangement obuses = SpeakerArrangement::STEREO;
      configure (1, &ibuses, 1, &obuses);
      if (n_ibuses() + n_obuses() == 0)
        throw std::logic_error (string_format ("Processor::%s: failed to setup input or output bus", __func__));
      assign_iobufs();
    }
}

/// Reset internal states.
/// This method is called to assign the sample rate and
/// to switch between realtime and offline rendering.
void
Processor::reset_state (const RenderSetup &rs)
{
  assert_return (rs.mix_freq > 0);
  const uint samplerate = rs.sample_rate;
  assert_return (rs.mix_freq == samplerate);
  assert_return (0 == (samplerate & 3));
  sample_rate_ = samplerate;
  ensure_initialized();
  flags_ |= HAS_RESET;
  reset (rs);
}

/// Reset all state variables and handle `sample_rate()` changes.
void
Processor::reset (const RenderSetup &rs)
{}

/** Method called for every audio buffer to be processed.
 * Each connected output bus needs to be filled with `n_frames`,
 * i.e. `n_frames` many floating point samples per channel.
 * Using the AudioSignal::OBusId (see add_output_bus()),
 * the floating point sample buffers can be addressed via the BusConfig structure as:
 * `bus[obusid].channel[nth].buffer`, see FloatBuffer for further details.
 * The AudioSignal::IBusId (see add_input_bus()) can be used
 * correspondingly to retrieve input channel values.
 */
void
Processor::render (const RenderSetup &rs, uint32 n_frames)
{}

/// Invoke Processor::configure() with `ipatch`/`opatch` applied to the current configuration.
void
Processor::reconfigure (IBusId ibusid, SpeakerArrangement ipatch, OBusId obusid, SpeakerArrangement opatch)
{
  const size_t ibus = size_t (ibusid) - 1;
  const size_t obus = size_t (obusid) - 1;
  if (uint64_t (ipatch))
    assert_return (ibus <= n_ibuses());
  if (uint64_t (opatch))
    assert_return (obus <= n_obuses());
  assert_return (n_ibuses() + n_obuses() == iobuses_.size());
  const uint sacount = iobuses_.size() + 1 + 1;
  SpeakerArrangement *sai = new SpeakerArrangement[sacount];
  SpeakerArrangement *sao = sai + n_ibuses() + 1;
  size_t i;
  for (i = 0; i < n_ibuses(); i++)
    sai[i] = iobuses_[i].ibus.speakers;
  sai[i] = SpeakerArrangement (0);
  for (i = 0; i < n_obuses(); i++)
    sao[i] = iobuses_[output_offset_ + i].obus.speakers;
  sao[i] = SpeakerArrangement (0);
  bool need_configure = false;
  if (size_t (ibus) && uint64_t (ipatch) &&
      sai[size_t (ibus) - 1] != ipatch)
    {
      sai[size_t (ibus) - 1] = ipatch;
      need_configure = true;
    }
  if (size_t (obus) && uint64_t (opatch) &&
      sao[size_t (obus) - 1] != opatch)
    {
      sao[size_t (obus) - 1] = opatch;
      need_configure = true;
    }
  if (!need_configure)
    {
      delete[] sai;
      return;
    }
  release_iobufs();
  configure (n_ibuses(), sai, n_obuses(), sao);
  delete[] sai;
  assign_iobufs();
}

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
  explicit Inlet (Chain &chain) : chain_ (chain) {}
  void query_info (ProcessorInfo &info) override        { info.label = "Bse::AudioSignal::Chain::Inlet"; }
  void initialize () override                           {}
  void reset      (const RenderSetup &rs) override      {}
  void
  configure (uint n_ibusses, const SpeakerArrangement *ibusses, uint n_obusses, const SpeakerArrangement *obusses) override
  {
    remove_all_buses();
    auto output = add_output_bus ("Output", chain_.ispeakers_);
    (void) output;
  }
  void
  render (const RenderSetup &rs, uint n_frames) override
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
  assert_return (speaker_arrangement_count_channels (iobuses) > 0);
  inlet = std::make_shared<Inlet> (*this);
}

Chain::~Chain()
{
  pm_remove_all_buses (*inlet);
  inlet = nullptr;
}

void
Chain::query_info (ProcessorInfo &info)
{
  info.uri = "Bse.AudioSignal.Chain";
  info.label = "Bse::AudioSignal::Chain";
}
static auto bseaudiosignalchain = Bse::enroll_asp<AudioSignal::Chain>();

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
Chain::reset (const RenderSetup &rs)
{
  render_setup_ = &rs;
  inlet->reset_state (*render_setup_);
  for (size_t i = 0; i < processors_.size(); i++)
    processors_[i]->reset_state (*render_setup_);
}

void
Chain::render (const RenderSetup &rs, uint n_frames)
{
  constexpr OBusId OUT1 = OBusId (1);
  render_setup_ = &rs;
  // render inlet and all processors in chain
  inlet->render (*render_setup_, n_frames);
  Processor *last = nullptr;
  for (size_t i = 0; i < processors_.size(); i++)
    {
      Processor &proc = *processors_[i];
      pm_render (proc, *render_setup_, n_frames);
      if (proc.n_obuses())
        last = &proc;
    }
  // make the last processor output the chain output
  const size_t nochannels = n_ochannels (OUT1);
  const size_t nlastchannels = last ? last->n_ochannels (OUT1) : 0;
  for (size_t c = 0; c < nochannels; c++)
    {
      if (last)
        redirect_oblock (OUT1, c, last->ofloats (OUT1, std::min (c, nlastchannels - 1)));
      else
        assign_oblock (OUT1, c, 0);
    }
}

/// Return the number of Processor instances in the Chain.
size_t
Chain::size ()
{
  return processors_.size();
}

/// Return the Processor at position `nth` in the Chain.
ProcessorP
Chain::at (uint nth)
{
  return_unless (nth < size(), {});
  return processors_[nth];
}

/// Render `n_frames` audio frames.
void
Chain::render_frames (uint n_frames)
{
  assert_return (flags_ & HAS_RESET);
  while (n_frames)
    {
      const uint blocksize = std::min (n_frames, MAX_RENDER_BLOCK_SIZE);
      render (*render_setup_, blocksize);
      n_frames -= blocksize;
    }
}

/// Remove a previously added Processor `proc` from the Chain.
bool
Chain::remove (Processor &proc)
{
  std::vector<Processor*> unconnected;
  ProcessorP processorp;
  size_t pos; // find proc
  for (pos = 0; pos < processors_.size(); pos++)
    if (processors_[pos].get() == &proc)
      {
        processorp = processors_[pos]; // and remove...
        processors_.erase (processors_.begin() + pos);
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
  const size_t index = CLAMP (pos, 0, processors_.size());
  processors_.insert (processors_.begin() + index, proc);
  if (render_setup_)
    proc->reset_state (*render_setup_);
  // fixup following connections
  reconnect (index);
}

/// Reconnect Chain processors at start and after.
void
Chain::reconnect (size_t start)
{
  // clear stale inputs
  for (size_t i = start; i < processors_.size(); i++)
    pm_disconnect_ibuses (*processors_[i]);
  // reconnect pairwise
  for (size_t i = start; i < processors_.size(); i++)
    chain_up (*(i ? processors_[i - 1] : inlet), *processors_[i]);
}

/// Connect the main audio input of `next` to audio output of `prev`.
uint
Chain::chain_up (Processor &prev, Processor &next)
{
  assert_return (this != &prev, 0);
  assert_return (this != &next, 0);
  const uint ni = next.n_ibuses();
  const uint no = prev.n_obuses();
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
          if (render_setup_)
            prev.reset_state (*render_setup_);
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
      if (render_setup_)
        next.reset_state (*render_setup_);
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

// == RegistryEntry ==
using CreateProcessor = std::function<ProcessorP ()>;

struct ProcessorRegistryHook {
  ProcessorRegistryHook *next = nullptr;
  const CreateProcessor create = nullptr;
  const CString file = "";
  const int line = 0;
};

static std::atomic<ProcessorRegistryHook*> processor_registry_hooks = nullptr;

/// Add a new type to the Processor type registry.
/// New Processor types must have a unique URI (see `query_info()`) and will be created
/// with the factory function `create()`.
uint
Processor::registry_enroll (const std::function<ProcessorP ()> &create,
                            const char *bfile, int bline)
{
  assert_return (create != nullptr, 0);
  auto *prh = new ProcessorRegistryHook { nullptr, create, bfile ? bfile : "", bline };
  // push_front
  while (!std::atomic_compare_exchange_strong (&processor_registry_hooks,
                                               &prh->next,
                                               prh))
    ; // when failing, compare_exchange automatically does *&arg2 = *&arg1
  static std::atomic<uint> dummy_id = 0;
  return ++dummy_id;
}

Processor::RegistryEntry::RegistryEntry (const std::function<ProcessorP ()> &func) :
  create (func)
{}

using ProcessorRegistryTable = std::unordered_map<CString,Processor::RegistryEntry>;
static std::shared_mutex processor_registry_mutex;
static PersistentStaticInstance<ProcessorRegistryTable> processor_registry_table;

// Ensure all registration hooks have been executed.
void
Processor::registry_init()
{
  while (processor_registry_hooks)
    {
      std::unique_lock wlock (processor_registry_mutex);
      ProcessorRegistryHook *node = nullptr, *nullnode = nullptr;
      // pop_all
      while (!std::atomic_compare_exchange_strong (&processor_registry_hooks, &node, nullnode))
        ; // when failing, compare_exchange automatically does *&arg2 = *&arg1
      // register all
      while (node)
        {
          Processor::RegistryEntry entry { node->create };
          entry.file = node->file;
          entry.num = uint32_t (node->line); // upper uint32 is 0 for line numbers
          ProcessorP testproc = node->create ();
          if (testproc)
            {
              testproc->query_info (entry);
              testproc = nullptr;
              if (entry.uri.empty())
                warning ("invalid empty URI for Processor: %s:%d", entry.file, entry.num);
              else
                {
                  if (processor_registry_table->find (entry.uri) != processor_registry_table->end())
                    warning ("duplicate Processor URI: %s", entry.uri);
                  else
                    (*processor_registry_table)[entry.uri] = entry;
                }
            }
          ProcessorRegistryHook *const old = node;
          node = old->next;
          delete old;
        }
    }
}

/// Retrieve the registry entry of a Processor type via its unique URI (or UUID).
Processor::RegistryEntry
Processor::registry_lookup (const std::string &uuiduri)
{
  registry_init();
  std::shared_lock rlock (processor_registry_mutex);
  auto it = processor_registry_table->find (uuiduri);
  if (it != processor_registry_table->end())
    return it->second;
  return {};
}

/// Create a new Processor object of the type specified by `uuiduri`.
ProcessorP
Processor::registry_create (const std::string &uuiduri)
{
  registry_init();
  RegistryEntry rentry;
  { // read-lock scope
    std::shared_lock rlock (processor_registry_mutex);
    auto it = processor_registry_table->find (uuiduri);
    if (it != processor_registry_table->end())
      rentry = it->second;
  }
  ProcessorP procp;
  if (rentry.uri != "")
    {
      procp = rentry.create();
      if (procp)
        procp->ensure_initialized();
    }
  return procp;
}

/// List the registry entries of all known Processor types.
Processor::RegistryList
Processor::registry_list()
{
  registry_init();
  RegistryList rlist;
  std::shared_lock rlock (processor_registry_mutex);
  rlist.reserve (processor_registry_table->size());
  for (std::pair<CString,RegistryEntry> el : *processor_registry_table)
    rlist.push_back (el.second);
  return rlist;
}

// == Processor::PParam ==
Processor::PParam::PParam (ParamId _id, const ParamInfo &pinfo) :
  id (_id), info (std::make_shared<ParamInfo> (_id))
{
  info->copy_fields (pinfo);
}

Processor::PParam::PParam (ParamId _id) :
  id (_id)
{}

Processor::PParam::PParam (const PParam &src)
{
  *this = src;
}

Processor::PParam&
Processor::PParam::operator= (const PParam &src)
{
  id = src.id;
  flags_ = src.flags_.load();
  value_ = src.value_.load();
  info = src.info;
  return *this;
}

// == FloatBuffer ==
/// Check for end-of-buffer overwrites
void
Processor::FloatBuffer::check ()
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
  // verify cache-line aligned struct layout
  static_assert (0 == (offsetof (FloatBuffer, fblock[0]) & 63));
  static_assert (0 == (offsetof (FloatBuffer, canary0_) & 63));
  static_assert (0 == ((sizeof (buffer) + offsetof (FloatBuffer, buffer)) & 63));
#pragma GCC diagnostic pop
  // verify cache-line aligned runtime layout
  assert_return (0 == (uintptr_t (&buffer[0]) & 63));
  // failing canaries indicate end-of-buffer overwrites
  assert_return (canary0_ == const_canary);
  assert_return (canary1_ == const_canary);
}

} // AudioSignal
} // Bse
