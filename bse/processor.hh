// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_PROCESSOR_HH__
#define __BSE_PROCESSOR_HH__

#include <bse/floatutils.hh>

namespace Bse {

namespace AudioSignal {

/// Maximum number of sample frames to calculate in Processor::render().
constexpr const uint MAX_RENDER_BLOCK_SIZE = 256;

/// ID type for Processor parameters, the ID numbers are user assignable.
enum class ParamId : uint32 {};

/// ID type for Processor input buses, buses are numbered with increasing index.
enum class IBusId : uint16 {};

/// ID type for Processor output buses, buses are numbered with increasing index.
enum class OBusId : uint16 {};

/// Flags to indicate channel arrangements of a bus.
/// See also: https://en.wikipedia.org/wiki/Surround_sound
enum class SpeakerArrangement : uint64_t {
  NONE                  = 0,
  FRONT_LEFT            =         0x1,  ///< Stereo Left (FL)
  FRONT_RIGHT           =         0x2,  ///< Stereo Right (FR)
  FRONT_CENTER          =         0x4,  ///< (FC)
  LOW_FREQUENCY         =         0x8,  ///< Low Frequency Effects (LFE)
  BACK_LEFT             =        0x10,  ///< (BL)
  BACK_RIGHT            =        0x20,  ///< (BR)
  // WAV reserved       =  0xyyy00000,
  AUX                   = uint64_t (1) << 63,   ///< Flag for side chain uses
  MONO                  = FRONT_LEFT,   ///< Single Channel (M)
  STEREO                = FRONT_LEFT | FRONT_RIGHT,
  STEREO_21             = STEREO | LOW_FREQUENCY,
  STEREO_30             = STEREO | FRONT_CENTER,
  STEREO_31             = STEREO_30 | LOW_FREQUENCY,
  SURROUND_50           = STEREO_30 | BACK_LEFT | BACK_RIGHT,
  SURROUND_51           = SURROUND_50 | LOW_FREQUENCY,
#if 0 // TODO: dynamic multichannel support
  FRONT_LEFT_OF_CENTER  =        0x40,  ///< (FLC)
  FRONT_RIGHT_OF_CENTER =        0x80,  ///< (FRC)
  BACK_CENTER           =       0x100,  ///< (BC)
  SIDE_LEFT             =       0x200,  ///< (SL)
  SIDE_RIGHT            =       0x400,  ///< (SR)
  TOP_CENTER            =       0x800,  ///< (TC)
  TOP_FRONT_LEFT        =      0x1000,  ///< (TFL)
  TOP_FRONT_CENTER      =      0x2000,  ///< (TFC)
  TOP_FRONT_RIGHT       =      0x4000,  ///< (TFR)
  TOP_BACK_LEFT         =      0x8000,  ///< (TBL)
  TOP_BACK_CENTER       =     0x10000,  ///< (TBC)
  TOP_BACK_RIGHT        =     0x20000,  ///< (TBR)
  SIDE_SURROUND_50      = STEREO_30 | SIDE_LEFT | SIDE_RIGHT,
  SIDE_SURROUND_51      = SIDE_SURROUND_50 | LOW_FREQUENCY,
#endif
};
constexpr SpeakerArrangement speaker_arrangement_channels_mask { ~size_t (SpeakerArrangement::AUX) };
uint8              speaker_arrangement_count_channels (SpeakerArrangement spa);
SpeakerArrangement speaker_arrangement_channels       (SpeakerArrangement spa);
bool               speaker_arrangement_is_aux         (SpeakerArrangement spa);
const char*        speaker_arrangement_bit_name       (SpeakerArrangement spa);
std::string        speaker_arrangement_desc           (SpeakerArrangement spa);

/// Detailed information and common properties of Processor subclasses.
struct ProcessorInfo {
  CString uri;          ///< Unique identifier for de-/serialization.
  CString label;        ///< Preferred user interface name.
  CString category;     ///< Category to allow grouping for processors of similar function.
  CString blurb;        ///< Short description for overviews.
  CString description;  ///< Elaborate description for help dialogs.
  CString website_url;  ///< Website of/about this Processor.
  CString creator_name; ///< Name of the creator.
  CString creator_url;  ///< Internet contact of the creator.
};

/// A named ID used to group parameters.
struct GroupId : CString {
  using CString::CString;
  using CString::operator=;
  using CString::operator==;
  using CString::operator!=;
};

/// One possible choice for selection parameters.
struct ChoiceDetails {
  CString name;
};

/// List of choices for ParamInfo.set_choices().
struct ChoiceEntries : std::vector<ChoiceDetails> {
  ChoiceEntries& operator+= (const ChoiceDetails &ce);
};

/// Detailed information and common properties of parameters.
struct ParamInfo {
  ParamId    id;           ///< Tag to identify parameter in APIs.
  CString    identifier;   ///< Identifier used for serialization.
  CString    display_name; ///< Preferred user interface name.
  CString    short_name;   ///< Abbreviated user interface name.
  CString    description;  ///< Elaborate description for help dialogs.
  CString    unit;         ///< Units of the values within range.
  CString    hints;        ///< Hints for parameter handling.
  GroupId    group;        ///< Group for parameters of similar function.
  ParamInfo& operator=   (const ParamInfo &src);
  using MinMax = std::pair<float,float>;
  void       clear       ();
  MinMax     get_minmax  () const;
  void       get_range   (float &fmin, float &fmax, float &fstep) const;
  void       set_range   (float fmin, float fmax, float fstep = 0);
  void       set_choices (const ChoiceEntries &centries);
  void       set_choices (ChoiceEntries &&centries);
  ChoiceEntries
  const&     get_choices () const;
  /*ctor*/   ParamInfo   ();
  /*copy*/   ParamInfo   (const ParamInfo &src);
  virtual   ~ParamInfo   ();
private:
  void       release     ();
  union {
    struct { float fmin, fmax, fstep; };
    uint64_t mem[sizeof (ChoiceEntries) / sizeof (uint64_t)];
    ChoiceEntries* centries() const { return (ChoiceEntries*) mem; }
  } u;
  uint union_tag = 0;
};

/// Structure providing supplementary information about input/output buses.
struct BusInfo {
  CString            identifier;   ///< Identifier used for serialization.
  CString            display_name; ///< Preferred user interface name.
  CString            short_name;   ///< Abbreviated user interface name.
  CString            description;  ///< Elaborate description for help dialogs.
  CString            hints;        ///< Hints for parameter handling.
  SpeakerArrangement speakers = SpeakerArrangement::NONE; ///< Channel to speaker arrangement.
  uint               n_channels () const;
};

// Prototypes
struct RenderSetup;

/// Audio signal Processor base class, implemented by all effects and instruments.
class Processor : public std::enable_shared_from_this<Processor>, public FastMemory::NewDeleteBase {
  struct IBus;
  struct OBus;
  union  PBus;
  struct PParam;
  class FloatBuffer;
  friend class ProcessorManager;
  struct FloatBlock { uint total = 0, next = 0; float *floats = nullptr; };
  struct OConnection {
    Processor *proc = nullptr; IBusId ibusid = {};
    bool operator== (const OConnection &o) const { return proc == o.proc && ibusid == o.ibusid; }
  };
  using OBRange = std::pair<FloatBuffer*,FloatBuffer*>;
  FloatBuffer             *fbuffers_ = nullptr;
  uint32                   sample_rate_ = 0;
  uint32                   output_offset_ = 0;
  std::vector<PBus>        iobuses_;
  std::vector<PParam>      params_;
  std::vector<FloatBlock>  float_blocks_;
  std::vector<OConnection> outputs_;
  static __thread uint64   tls_timestamp;
  static void registry_init   ();
  float*      alloc_float     ();
  void        set_param_      (ParamId paramid, float value);
  float       get_param_      (ParamId paramid);
  bool        check_dirty_    (ParamId paramid) const;
  void        assign_iobufs   ();
  void        release_iobufs  ();
  void        reconfigure     (IBusId ibus, SpeakerArrangement ipatch, OBusId obus, SpeakerArrangement opatch);
  const FloatBuffer& float_buffer (IBusId busid, uint channelindex) const;
  FloatBuffer&       float_buffer (OBusId busid, uint channelindex, bool resetptr = false);
  static
  const FloatBuffer& zero_buffer ();
  /*copy*/    Processor       (const Processor&) = delete;
protected:
  Processor();
#ifndef DOXYGEN
  // Spare subclasses from using the `AudioSignal::` namespace prefix
  using GroupId = AudioSignal::GroupId;
  using ParamId = AudioSignal::ParamId;
  using ParamInfo = AudioSignal::ParamInfo;
  using ChoiceEntries = AudioSignal::ChoiceEntries;
  using IBusId = AudioSignal::IBusId;
  using OBusId = AudioSignal::OBusId;
  using SpeakerArrangement = AudioSignal::SpeakerArrangement;
  using ProcessorInfo = AudioSignal::ProcessorInfo;
  using RenderSetup = AudioSignal::RenderSetup;
#endif
  virtual      ~Processor         ();
  virtual void  initialize        ();
  virtual void  configure         (uint n_ibuses, const SpeakerArrangement *ibuses,
                                   uint n_obuses, const SpeakerArrangement *obuses) = 0;
  virtual void  reset             (const RenderSetup &rs) = 0;
  virtual void  render            (const RenderSetup &rs, uint n_frames) = 0;
  // Parameters
  ParamId       add_param         (ParamId id, const ParamInfo &pinfo, float value);
  ParamId       add_param         (const std::string &identifier, const std::string &display_name,
                                   const std::string &short_name, float pmin, float pmax,
                                   const std::string &hints, float value, const std::string &unit = "");
  ParamId       add_param         (const std::string &identifier, const std::string &display_name,
                                   const std::string &short_name, ChoiceEntries &&centries,
                                   const std::string &hints, float value, const std::string &unit = "");
  void          start_param_group (const std::string &groupname) const;
  // Buses
  IBusId        add_input_bus     (CString name, SpeakerArrangement speakerarrangement);
  OBusId        add_output_bus    (CString name, SpeakerArrangement speakerarrangement);
  void          remove_all_buses  ();
  OBus&         iobus             (OBusId busid);
  IBus&         iobus             (IBusId busid);
  const OBus&   iobus             (OBusId busid) const { return const_cast<Processor*> (this)->iobus (busid); }
  const IBus&   iobus             (IBusId busid) const { return const_cast<Processor*> (this)->iobus (busid); }
  void          disconnect_ibuses ();
  void          disconnect_obuses ();
  void          disconnect        (IBusId ibus);
  void          connect           (IBusId ibus, Processor &oproc, OBusId obus);
  float*        oblock            (OBusId b, uint c);
  void          assign_oblock     (OBusId b, uint c, float val);
  void          redirect_oblock   (OBusId b, uint c, const float *block);
public:
  struct RegistryEntry;
  using RegistryList = std::vector<RegistryEntry>;
  [[gnu::const]]
  uint          sample_rate       () const;
  void          reset_state       (const RenderSetup &rs);
  virtual void  query_info        (ProcessorInfo &info) = 0;
  String        debug_name        () const;
  // Parameters
  ParamId       find_param        (const std::string &identifier) const;
  ParamInfo     param_info        (ParamId paramid) const;
  float         get_param         (ParamId paramid);
  void          set_param         (ParamId paramid, float value);
  bool          check_dirty       (ParamId paramid) const;
  // Buses
  IBusId        find_input_bus    (const std::string &name) const;
  OBusId        find_output_bus   (const std::string &name) const;
  uint          n_ibuses          () const;
  uint          n_obuses          () const;
  uint          n_ichannels       (IBusId busid) const;
  uint          n_ochannels       (OBusId busid) const;
  BusInfo       bus_info          (IBusId busid) const;
  BusInfo       bus_info          (OBusId busid) const;
  bool          connected         (OBusId obusid) const;
  bool          iseemless         (IBusId b, uint c, uint n_frames = MAX_RENDER_BLOCK_SIZE) const;
  bool          iconst            (IBusId b, uint c, uint n_frames = MAX_RENDER_BLOCK_SIZE) const;
  const float*  ifloats           (IBusId b, uint c) const;
  const float*  ofloats           (OBusId b, uint c) const;
  static uint64 timestamp         ();
  // Registration and factory
  static RegistryList  registry_list      ();
  static RegistryEntry registry_lookup    (const std::string &uuiduri);
  static ProcessorP    registry_create    (const std::string &uuiduri);
  static uint          registry_enroll    (const std::function<ProcessorP ()> &create,
                                           const char *bfile = __builtin_FILE(), int bline = __builtin_LINE());
};

/// Timing information around AudioSignal processing.
struct AudioTiming {
  double bpm = 0;                       ///< Current tempo in beats per minute.
  uint64 frame_stamp = ~uint64 (0);     ///< Number of sample frames processed since playback start.
};

/// An (almost) endless stream of incoming or outgoing events.
struct EventStream {
  // struct Event { uint offset; /* in n_frames */ union Data { ... }; };
};

/// Processor configuration and setup for event and audio calculations.
struct RenderSetup {
  const double       mix_freq;    ///< Same as `sample_rate` cast to double.
  const uint         sample_rate; ///< Sample rate (mixing frequency) in Hz used for Processor::render().
  const AudioTiming &timing;
  EventStream        input_events;
  EventStream        output_events;
  explicit RenderSetup (uint32 samplerate, AudioTiming &atiming);
};

/// Aggregate structure for input/output buffer state and values in Processor::render().
/// The floating point #buffer array is cache-line aligned (to 64 byte) to optimize
/// SIMD access and avoid false sharing.
class Processor::FloatBuffer {
  void          check      ();
  /// Floating point memory when #buffer is not redirected, 64-byte aligned.
  alignas (64) float fblock[MAX_RENDER_BLOCK_SIZE] = { 0, };
  const uint64       canary0_ = const_canary;
  const uint64       canary1_ = const_canary;
  struct { uint64 d1, d2, d3, d4; }; // dummy mem
  SpeakerArrangement speaker_arrangement_ = SpeakerArrangement::NONE;
  SpeakerArrangement speaker_arrangement () const;
  static constexpr uint64 const_canary = 0xE14D8A302B97C56F;
  friend class Processor;
  /// Pointer to the IO samples, this can be redirected or point to #fblock.
  float             *buffer = &fblock[0];
};

// == ProcessorManager ==
/// Interface for management, connecting and processing of Processor instances.
class ProcessorManager {
protected:
  static auto pm_remove_all_buses  (Processor &p)       { return p.remove_all_buses(); }
  static auto pm_disconnect_ibuses (Processor &p)       { return p.disconnect_ibuses(); }
  static auto pm_disconnect_obuses (Processor &p)       { return p.disconnect_obuses(); }
  static auto pm_render            (Processor &p, const RenderSetup &r, uint n)
                                   { return p.render (r, n); }
  static auto pm_connect           (Processor &p, IBusId i, Processor &d, OBusId o)
                                   { return p.connect (i, d, o); }
  static auto pm_reconfigure       (Processor &p, IBusId i, SpeakerArrangement ip, OBusId o, SpeakerArrangement op)
                                   { return p.reconfigure (i, ip, o, op); }
};

// == Chain ==
/// Container for connecting multiple Processors in a chain.
class Chain : public Processor, ProcessorManager {
  class Inlet;
  using InletP = std::shared_ptr<Inlet>;
  InletP inlet;
  vector<ProcessorP> processors_;
  const RenderSetup *render_setup_ = nullptr;
  const SpeakerArrangement ispeakers_ = SpeakerArrangement (0);
  const SpeakerArrangement ospeakers_ = SpeakerArrangement (0);
  void   initialize () override;
  void   configure (uint n_ibusses, const SpeakerArrangement *ibusses, uint n_obusses, const SpeakerArrangement *obusses) override;
  void   reset     (const RenderSetup &rs) override;
  void   render    (const RenderSetup &rs, uint n_frames) override;
  uint   chain_up  (Processor &pfirst, Processor &psecond);
  void   reconnect (size_t start);
public:
  explicit   Chain          (SpeakerArrangement iobuses = SpeakerArrangement::STEREO);
  virtual   ~Chain          ();
  void       query_info     (ProcessorInfo &info) override;
  void       insert         (ProcessorP proc, size_t pos = ~size_t (0));
  bool       remove         (Processor &proc);
  ProcessorP at             (uint nth);
  size_t     size           ();
  void       render_frames  (uint n_frames);
};
using ChainP = std::shared_ptr<Chain>;

// == RegistryEntry ==
struct Processor::RegistryEntry : ProcessorInfo {
  CString file;
  uint64  num = 0;
  RegistryEntry () = default;
private:
  RegistryEntry (const std::function<ProcessorP ()>&);
  std::function<ProcessorP ()> create;
  friend class Processor;
};

// == Inlined Internals ==
struct Processor::IBus : BusInfo {
  Processor *proc = {};
  OBusId     obusid = {};
  explicit IBus (const std::string &ident, SpeakerArrangement sa);
};
struct Processor::OBus : BusInfo {
  uint fbuffer_concounter = 0;
  uint fbuffer_count = 0;
  uint fbuffer_index = ~0;
  explicit OBus (const std::string &ident, SpeakerArrangement sa);
};
// Processor internal input/output bus book keeping
union Processor::PBus {
  IBus    ibus;
  OBus    obus;
  BusInfo pbus;
  explicit PBus (const std::string &ident, SpeakerArrangement sa);
};

// Processor internal parameter book keeping
struct Processor::PParam {
  PParam (ParamId i);
  PParam (ParamId i, const ParamInfo &pinfo, float *p);
  bool
  get_dirty () const
  {
    return ptr_ & TAG_MASK;
  }
  float
  get_value_and_clean ()
  {
    const float f = *valuep();
    set_dirty (false);
    return f;
  }
  void
  set_dirty (bool b)
  {
    BSE_ASSERT_RETURN (0 == (b & PTR_MASK));
    ptr_ &= PTR_MASK;
    ptr_ |= b;
  }
  void
  assign (float f)
  {
    const float old = *valuep();
    if (BSE_ISLIKELY (old != f))
      {
        *valuep() = f;
        set_dirty (true);
      }
  }
  static int // Helper to keep PParam structures sorted.
  cmp (const PParam &a, const PParam &b)
  {
    return a.info.id < b.info.id ? -1 : a.info.id > b.info.id;
  }
private:
  uintptr_t ptr_ = 0;
  static const uintptr_t TAG_MASK = sizeof (float) - 1;
  static const uintptr_t PTR_MASK = ~TAG_MASK;
  void set_ptr (float *p);
  float*
  valuep () const
  {
    return reinterpret_cast<float*> (ptr_ & PTR_MASK);
  }
public:
  ParamInfo info;
};

/// Number of channels described by `speakers`.
inline uint
BusInfo::n_channels () const
{
  return speaker_arrangement_count_channels (speakers);
}

/// Sample rate in Hz used for render().
inline uint
Processor::sample_rate () const
{
  return sample_rate_;
}

/// Number of input buses configured for this Processor.
inline uint
Processor::n_ibuses () const
{
  return output_offset_;
}

/// Number of output buses configured for this Processor.
inline uint
Processor::n_obuses () const
{
  return iobuses_.size() - output_offset_;
}

/// Retrieve BusInfo for an input bus.
inline BusInfo
Processor::bus_info (IBusId busid) const
{
  return iobus (busid);
}

/// Retrieve BusInfo for an output bus.
inline BusInfo
Processor::bus_info (OBusId busid) const
{
  return iobus (busid);
}

/// Number of channels of input bus `busid` configured for this Processor.
inline uint
Processor::n_ichannels (IBusId busid) const
{
  const IBus &ibus = iobus (busid);
  return ibus.n_channels();
}

/// Number of channels of output bus `busid` configured for this Processor.
inline uint
Processor::n_ochannels (OBusId busid) const
{
  const OBus &obus = iobus (busid);
  return obus.n_channels();
}

/// Set parameter `id` to `value`.
inline void
Processor::set_param (ParamId paramid, float value)
{
  // fast path for sequential ids
  const size_t idx = size_t (paramid) - 1;
  if (BSE_ISLIKELY (idx < params_.size()) && BSE_ISLIKELY (params_[idx].info.id == paramid))
    params_[idx].assign (value);
  return set_param_ (paramid, value);
}

/// Fetch `value` of parameter `id` and clear its `dirty` flag.
inline float
Processor::get_param (ParamId paramid)
{
  // fast path for sequential ids
  const size_t idx = size_t (paramid) - 1;
  if (BSE_ISLIKELY (idx < params_.size()) && BSE_ISLIKELY (params_[idx].info.id == paramid))
    return params_[idx].get_value_and_clean();
  // lookup id with gaps
  return get_param_ (paramid);
}

/// Check if the parameter `dirty` flag is set.
/// Return `true` if set_param() changed the parameter value since the last get_param() call.
inline bool
Processor::check_dirty (ParamId paramid) const
{
  // fast path for sequential ids
  const size_t idx = size_t (paramid) - 1;
  if (BSE_ISLIKELY (idx < params_.size()) && BSE_ISLIKELY (params_[idx].info.id == paramid))
    return params_[idx].get_dirty();
  // lookup id with gaps
  return check_dirty_ (paramid);
}

/// Access readonly float buffer of input bus `b`, channel `c`, see also ofloats().
inline const float*
Processor::ifloats (IBusId b, uint c) const
{
  return float_buffer (b, c).buffer;
}

/// Access readonly float buffer of output bus `b`, channel `c`, see also oblock().
inline const float*
Processor::ofloats (OBusId b, uint c) const
{
  return const_cast<Processor*> (this)->float_buffer (b, c).buffer;
}

/// Reset buffer redirections and access float buffer of output bus `b`, channel `c`.
/// See also ofloats() for readonly access and redirect_oblock() for redirections.
inline float*
Processor::oblock (OBusId b, uint c)
{
  return float_buffer (b, c, true).buffer;
}

/// The current timestamp in sample frames
inline uint64
Processor::timestamp ()
{
  return tls_timestamp;
}

/// Fast check that tests if the first and last frame of input bus `b`, channel `c` are the same.
inline bool
Processor::iseemless (IBusId b, uint c, uint n_frames) const
{
  const float *const buffer = ifloats (b, c);
  return buffer[0] == buffer[n_frames - 1];
}

/// Checks that *all* `n_frames` values of input bus `b`, channel `c` are exactly equal.
inline bool
Processor::iconst (IBusId b, uint c, uint n_frames) const
{
  const float *const buffer = ifloats (b, c);
  return floatisconst (buffer + 1, n_frames - 1, buffer[0]);
}

/// Retrieve the speaker assignment.
inline SpeakerArrangement
Processor::FloatBuffer::speaker_arrangement () const
{
  return speaker_arrangement_;
}

} // AudioSignal

template<typename Class> extern inline uint
enroll_asp (const char *bfile = __builtin_FILE(), int bline = __builtin_LINE())
{
  auto new_asp = [] () { return std::make_shared<Class> (); };
  return AudioSignal::Processor::registry_enroll (new_asp, bfile, bline);
}

} // Bse

#endif // __BSE_PROCESSOR_HH__
