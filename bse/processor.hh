// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_PROCESSOR_HH__
#define __BSE_PROCESSOR_HH__

#include <bse/object.hh>
#include <bse/midievent.hh>
#include <bse/floatutils.hh>
#include <any>

namespace Bse {

class ProcessorImpl;
using ProcessorImplP = std::shared_ptr<ProcessorImpl>;
class ComboImpl;

namespace AudioSignal {

/// Maximum number of sample frames to calculate in Processor::render().
constexpr const uint MAX_RENDER_BLOCK_SIZE = 128;

/// Main handle for Processor administration and audio rendering.
class Engine;

/// ID type for Processor parameters, the ID numbers are user assignable.
enum class ParamId : uint32 {};

/// ID type for Processor input buses, buses are numbered with increasing index.
enum class IBusId : uint16 {};

/// ID type for Processor output buses, buses are numbered with increasing index.
enum class OBusId : uint16 {};

/// ID type for the Processor registry.
struct RegistryId { struct Entry; const Entry &entry; };

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
  CString version;      ///< Version identifier for de-/serialization.
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

/// An in-memory icon representation.
struct IconStr : std::string {
};

/// One possible choice for selection parameters.
struct ChoiceDetails {
  const CString ident;          ///< Identifier used for serialization (can be derived from label).
  const CString label;          ///< Preferred user interface name.
  const CString subject;        ///< Subject line, a brief one liner or elaborate title.
  const IconStr icon;           ///< Stringified icon, SVG and PNG should be supported (64x64 pixels recommended).
  bool    operator== (const ChoiceDetails &o) const     { return ident == o.ident; }
  bool    operator!= (const ChoiceDetails &o) const     { return !operator== (o); }
  ChoiceDetails (CString label, CString subject = "");
  ChoiceDetails (IconStr icon, CString label, CString subject = "");
};

/// List of choices for ParamInfo.set_choices().
struct ChoiceEntries : std::vector<ChoiceDetails> {
  ChoiceEntries& operator+= (const ChoiceDetails &ce);
  using base_t = std::vector<ChoiceDetails>;
  ChoiceEntries (std::initializer_list<base_t::value_type> __l) : base_t (__l) {}
  ChoiceEntries () {}
};

/// Detailed information and common properties of parameters.
struct ParamInfo {
  CString    ident;        ///< Identifier used for serialization.
  CString    label;        ///< Preferred user interface name.
  CString    nick;         ///< Abbreviated user interface name, usually not more than 6 characters.
  CString    unit;         ///< Units of the values within range.
  CString    hints;        ///< Hints for parameter handling.
  GroupId    group;        ///< Group for parameters of similar function.
  CString    blurb;        ///< Short description for user interface tooltips.
  CString    description;  ///< Elaborate description for help dialogs.
  using MinMax = std::pair<double,double>;
  void       clear       ();
  MinMax     get_minmax  () const;
  double     get_stepping() const;
  void       get_range   (double &fmin, double &fmax, double &fstep) const;
  void       set_range   (double fmin, double fmax, double fstep = 0);
  void       set_choices (const ChoiceEntries &centries);
  void       set_choices (ChoiceEntries &&centries);
  ChoiceEntries
  const&     get_choices () const;
  void       copy_fields (const ParamInfo &src);
  /*ctor*/   ParamInfo   (ParamId pid = ParamId (0), uint porder = 0);
  virtual   ~ParamInfo   ();
  // BSE thread accessors
  size_t     add_notify  (ProcessorP proc, const std::function<void()> &callback);
  bool       del_notify  (ProcessorP proc, size_t callbackid);
  void       call_notify ();
  const ParamId id;
  const uint    order;
private:
  uint union_tag = 0;
  union {
    struct { double fmin, fmax, fstep; };
    uint64_t mem[sizeof (ChoiceEntries) / sizeof (uint64_t)];
    ChoiceEntries* centries() const { return (ChoiceEntries*) mem; }
  } u;
  /*copy*/   ParamInfo   (const ParamInfo&) = delete;
  void       release     ();
  using Callback = std::function<void()>;
  static constexpr uint32 MAX_NOTIFIER = ~uint32 (0);
  uint32 next_notifier_ = MAX_NOTIFIER;
  std::vector<Callback*> notifiers_;
};
using ParamInfoP = std::shared_ptr<ParamInfo>;

/// Structure providing supplementary information about input/output buses.
struct BusInfo {
  CString            ident;     ///< Identifier used for serialization.
  CString            label;     ///< Preferred user interface name.
  CString            hints;     ///< Hints for parameter handling.
  CString            blurb;     ///< Short description for user interface tooltips.
  SpeakerArrangement speakers = SpeakerArrangement::NONE; ///< Channel to speaker arrangement.
  uint               n_channels () const;
};

/// Audio signal Processor base class, implemented by all effects and instruments.
class Processor : public std::enable_shared_from_this<Processor>, public FastMemory::NewDeleteBase {
  struct IBus;
  struct OBus;
  struct EventStreams;
  union  PBus;
  struct PParam;
  class FloatBuffer;
  friend class ProcessorManager;
  friend class Engine;
  struct OConnection {
    Processor *proc = nullptr; IBusId ibusid = {};
    bool operator== (const OConnection &o) const { return proc == o.proc && ibusid == o.ibusid; }
  };
  using OBRange = std::pair<FloatBuffer*,FloatBuffer*>;
protected:
#ifndef DOXYGEN
  // Inherit `AudioSignal` concepts in derived classes from other namespaces
  using Engine = AudioSignal::Engine;
  using GroupId = AudioSignal::GroupId;
  using ParamId = AudioSignal::ParamId;
  using ParamInfo = AudioSignal::ParamInfo;
  using ChoiceEntries = AudioSignal::ChoiceEntries;
  using IBusId = AudioSignal::IBusId;
  using OBusId = AudioSignal::OBusId;
  using SpeakerArrangement = AudioSignal::SpeakerArrangement;
  using ProcessorInfo = AudioSignal::ProcessorInfo;
  using MinMax = std::pair<double,double>;
#endif
  enum { INITIALIZED   = 1 << 0,
         PARAMCHANGE   = 1 << 3,
         BUSCONNECT    = 1 << 4,
         BUSDISCONNECT = 1 << 5,
         INSERTION     = 1 << 6,
         REMOVAL       = 1 << 7, };
  std::atomic<uint32>      flags_ = 0;
private:
  uint32                   output_offset_ = 0;
  FloatBuffer             *fbuffers_ = nullptr;
  std::vector<PBus>        iobuses_;
  std::vector<PParam>      params_;
  std::vector<OConnection> outputs_;
  EventStreams            *estreams_ = nullptr;
  uint64_t                 done_frames_ = 0;
  static void        registry_init      ();
  const PParam*      find_pparam        (Id32 paramid) const;
  const PParam*      find_pparam_       (ParamId paramid) const;
  void               assign_iobufs      ();
  void               release_iobufs     ();
  void               reconfigure        (IBusId ibus, SpeakerArrangement ipatch, OBusId obus, SpeakerArrangement opatch);
  void               ensure_initialized ();
  const FloatBuffer& float_buffer       (IBusId busid, uint channelindex) const;
  FloatBuffer&       float_buffer       (OBusId busid, uint channelindex, bool resetptr = false);
  static
  const FloatBuffer& zero_buffer        ();
  void               render_block       ();
  void               reset_state        ();
  void               enqueue_deps       ();
  /*copy*/           Processor          (const Processor&) = delete;
  virtual void       render             (uint n_frames) = 0;
  virtual void       reset              () = 0;
protected:
  Engine                          &engine_;
  explicit      Processor         ();
  virtual      ~Processor         ();
  virtual void  initialize        ();
  virtual void  configure         (uint n_ibuses, const SpeakerArrangement *ibuses,
                                   uint n_obuses, const SpeakerArrangement *obuses) = 0;
  // Parameters
  virtual void  adjust_param      (Id32 tag) {}
  virtual void  enqueue_children  () {}
  ParamId       nextid            () const;
  ParamId       add_param         (Id32 id, const ParamInfo &infotmpl, double value);
  ParamId       add_param         (Id32 id, const std::string &clabel, const std::string &nickname,
                                   double pmin, double pmax, double value,
                                   const std::string &unit = "", std::string hints = "",
                                   const std::string &blurb = "", const std::string &description = "");
  ParamId       add_param         (Id32 id, const std::string &clabel, const std::string &nickname,
                                   ChoiceEntries &&centries, double value, std::string hints = "",
                                   const std::string &blurb = "", const std::string &description = "");
  ParamId       add_param         (Id32 id, const std::string &clabel, const std::string &nickname,
                                   bool boolvalue, std::string hints = "",
                                   const std::string &blurb = "", const std::string &description = "");
  void          start_param_group (const std::string &groupname) const;
  ParamId       add_param         (const std::string &clabel, const std::string &nickname,
                                   double pmin, double pmax, double value,
                                   const std::string &unit = "", std::string hints = "",
                                   const std::string &blurb = "", const std::string &description = "");
  ParamId       add_param         (const std::string &clabel, const std::string &nickname,
                                   ChoiceEntries &&centries, double value, std::string hints = "",
                                   const std::string &blurb = "", const std::string &description = "");
  ParamId       add_param         (const std::string &clabel, const std::string &nickname,
                                   bool boolvalue, std::string hints = "",
                                   const std::string &blurb = "", const std::string &description = "");
  double        peek_param_mt     (Id32 paramid) const;
  // Buses
  IBusId        add_input_bus     (CString uilabel, SpeakerArrangement speakerarrangement,
                                   const std::string &hints = "", const std::string &blurb = "");
  OBusId        add_output_bus    (CString uilabel, SpeakerArrangement speakerarrangement,
                                   const std::string &hints = "", const std::string &blurb = "");
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
  // event stream handling
  void          prepare_event_input    ();
  EventRange    get_event_input        ();
  void          prepare_event_output   ();
  EventStream&  get_event_output       ();
public:
  using RegistryList = std::vector<ProcessorInfo>;
  using MakeProcessor = ProcessorP (*) (const std::any*);
  using ParamInfoPVec = std::vector<ParamInfoP>;
  using MaybeParamId = std::pair<ParamId,bool>;
  static const std::string STANDARD; ///< ":G:S:r:w:" - GUI STORAGE READABLE WRITABLE
  Engine&       engine            () const;
  uint          sample_rate       () const BSE_CONST;
  double        nyquist           () const BSE_CONST;
  double        inyquist          () const BSE_CONST;
  virtual void  query_info        (ProcessorInfo &info) const = 0;
  String        debug_name        () const;
  // Parameters
  double              get_param             (Id32 paramid);
  void                set_param             (Id32 paramid, double value);
  ParamInfoP          param_info            (Id32 paramid) const;
  MaybeParamId        find_param            (const std::string &identifier) const;
  ParamInfoPVec       list_params           () const;
  MinMax              param_range           (Id32 paramid) const;
  bool                check_dirty           (Id32 paramid) const;
  void                adjust_params         (bool include_nondirty = false);
  virtual std::string param_value_to_text   (Id32 paramid, double value) const;
  virtual double      param_value_from_text (Id32 paramid, const std::string &text) const;
  virtual double      value_to_normalized   (Id32 paramid, double value) const;
  virtual double      value_from_normalized (Id32 paramid, double normalized) const;
  double              get_normalized        (Id32 paramid);
  void                set_normalized        (Id32 paramid, double normalized);
  bool                is_initialized    () const;
  // Buses
  IBusId        find_ibus         (const std::string &name) const;
  OBusId        find_obus         (const std::string &name) const;
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
  bool          has_event_input   ();
  bool          has_event_output  ();
  void          connect_event_input    (Processor &oproc);
  void          disconnect_event_input ();
  ProcessorImplP access_processor () const;
  // Registration and factory
  static RegistryList  registry_list      ();
  static ProcessorP    registry_create    (Engine &engine, const std::string &uuiduri);
  static ProcessorP    registry_create    (Engine &engine, RegistryId rid, const std::any &any);
  static RegistryId    registry_enroll    (MakeProcessor create, const char *bfile = __builtin_FILE(), int bline = __builtin_LINE());
  // MT-Safe accessors
  static double param_peek_mt     (const ProcessorP proc, Id32 paramid);
  static void   param_notifies_mt (ProcessorP proc, Id32 paramid, bool need_notifies);
  static bool   has_notifies_mt   ();
  static void   call_notifies_mt  ();
private:
  void          enqueue_notify_mt (uint32 pushmask);
  std::atomic<Processor*> nqueue_next_ { nullptr }; ///< No notifications queued while == nullptr
  ProcessorP              nqueue_guard_;            ///< Only used while nqueue_next_ != nullptr
  std::weak_ptr<Bse::ProcessorImpl> bproc_;
  static constexpr uint32 NOTIFYMASK = PARAMCHANGE | BUSCONNECT | BUSDISCONNECT | INSERTION | REMOVAL;
  static __thread uint64  tls_timestamp;
};

/// Timing information around AudioSignal processing.
struct AudioTiming {
  double bpm = 0;                       ///< Current tempo in beats per minute.
  uint64 frame_stamp = ~uint64 (0);     ///< Number of sample frames processed since playback start.
};

/// Audio processing setup and engine for concurrent rendering.
class Engine {
  const double       nyquist_;  ///< Half the `sample_rate`.
  const double       inyquist_; ///< Inverse Nyquist frequency, i.e. 1.0 / nyquist_;
  const uint         sample_rate_; ///< Sample rate (mixing frequency) in Hz used for Processor::render().
  uint64_t           frame_counter_;
  std::atomic<uint>  flags_;
  uint               scheduler_depth_;
  std::vector<Processor*> schedule_;
  std::vector<ProcessorP> roots_;
  std::mutex              mutex_;
public:
  const AudioTiming &timing;
  explicit      Engine           (uint32 samplerate, AudioTiming &atiming);
  uint          sample_rate      () const BSE_CONST      { return sample_rate_; }
  double        nyquist          () const BSE_CONST      { return nyquist_; }
  double        inyquist         () const BSE_CONST      { return inyquist_; }
  uint64_t      frame_counter    () const                { return frame_counter_; }
  void          add_root         (ProcessorP rootproc);
  bool          del_root         (ProcessorP rootproc);
  bool          in_schedule      (Processor &proc);
  void          enqueue          (Processor &proc);
  void          reschedule       ();
  void          make_schedule    ();
  void          render_block     ();
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
  static auto pm_connect           (Processor &p, IBusId i, Processor &d, OBusId o)
                                   { return p.connect (i, d, o); }
  static auto pm_connect_events    (Processor &oproc, Processor &iproc)
                                   { return iproc.connect_event_input (oproc); }
  static auto pm_reconfigure       (Processor &p, IBusId i, SpeakerArrangement ip, OBusId o, SpeakerArrangement op)
                                   { return p.reconfigure (i, ip, o, op); }
};

// == Inlined Internals ==
struct Processor::IBus : BusInfo {
  Processor *proc = {};
  OBusId     obusid = {};
  explicit IBus (const std::string &ident, const std::string &label, SpeakerArrangement sa);
};
struct Processor::OBus : BusInfo {
  uint fbuffer_concounter = 0;
  uint fbuffer_count = 0;
  uint fbuffer_index = ~0;
  explicit OBus (const std::string &ident, const std::string &label, SpeakerArrangement sa);
};
// Processor internal input/output bus book keeping
union Processor::PBus {
  IBus    ibus;
  OBus    obus;
  BusInfo pbus;
  explicit PBus (const std::string &ident, const std::string &label, SpeakerArrangement sa);
};

// Processor internal input/output event stream book keeping
struct Processor::EventStreams {
  static constexpr auto EVENT_ISTREAM = IBusId (0xff01); // *not* an input bus, ID is used for OConnection
  Processor  *oproc = nullptr;
  EventStream estream;
  bool        has_event_input = false;
  bool        has_event_output = false;
};

// Processor internal parameter book keeping
struct Processor::PParam {
  explicit PParam          (ParamId id);
  explicit PParam          (ParamId id, uint order, const ParamInfo &pinfo);
  /*copy*/ PParam          (const PParam &);
  PParam& operator=        (const PParam &);
  double   fetch_and_clean ()       { clear_dirty(); return value_; }
  double   peek            () const { return value_; }
  bool     is_dirty        () const { return flags_ & 1; }
  void     mark_dirty      ()       { flags_ |= 1; }
  void     clear_dirty     ()       { flags_ &= ~uint32 (1); }
  bool     has_updated     () const { return flags_ & 2; }
  void     mark_updated    ()       { flags_ |= 2; }
  void     clear_updated   ()       { flags_ &= ~uint32 (2); }
  void     must_notify_mt  (bool n) { if (n) flags_ |= 4; else flags_ &= ~uint32 (4); }
  bool     must_notify     () const { return flags_ & 4; }
  void
  assign (double f)
  {
    const double old = value_;
    value_ = f;
    if (BSE_ISLIKELY (old != value_))
      mark_dirty();
  }
  static int // Helper to keep PParam structures sorted.
  cmp (const PParam &a, const PParam &b)
  {
    return a.id < b.id ? -1 : a.id > b.id;
  }
public:
  ParamId             id = {};  ///< Tag to identify parameter in APIs.
private:
  std::atomic<uint32> flags_ = 1;
  std::atomic<double> value_ = FP_NAN;
public:
  ParamInfoP          info;
};

/// Number of channels described by `speakers`.
inline uint
BusInfo::n_channels () const
{
  return speaker_arrangement_count_channels (speakers);
}

/// Retrieve AudioSignal::Engine handle for this Processor.
inline Engine&
Processor::engine () const
{
  return engine_;
}

/// Sample rate mixing frequency in Hz as unsigned, used for render().
inline uint
Processor::sample_rate () const
{
  return engine_.sample_rate();
}

/// Half the sample rate in Hz as double, used for render().
inline double
Processor::nyquist () const
{
  return engine_.nyquist();
}

/// Inverse Nyquist frequency, i.e. 1.0 / nyquist().
inline double
Processor::inyquist () const
{
  return engine_.inyquist();
}

/// Returns `true` if this Processor has an event input stream.
inline bool
Processor::has_event_input()
{
  return estreams_ && estreams_->has_event_input;
}

/// Returns `true` if this Processor has an event output stream.
inline bool
Processor::has_event_output()
{
  return estreams_ && estreams_->has_event_output;
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

// Call adjust_param() for all or just dirty parameters.
inline void
Processor::adjust_params (bool include_nondirty)
{
  for (const PParam &p : params_)
    if (include_nondirty || p.is_dirty())
      adjust_param (p.id);
}

// Find parameter for internal access.
inline const Processor::PParam*
Processor::find_pparam (Id32 paramid) const
{
  // fast path via sequential ids
  const size_t idx = paramid.id - 1;
  if (BSE_ISLIKELY (idx < params_.size()) && BSE_ISLIKELY (params_[idx].id == ParamId (paramid.id)))
    return &params_[idx];
  return find_pparam_ (ParamId (paramid.id));
}

/// Fetch `value` of parameter `id` and clear its `dirty` flag.
inline double
Processor::get_param (Id32 paramid)
{
  const PParam *pparam = find_pparam (ParamId (paramid.id));
  return BSE_ISLIKELY (pparam) ? const_cast<PParam*> (pparam)->fetch_and_clean() : FP_NAN;
}

/// Check if the parameter `dirty` flag is set.
/// Return `true` if set_param() changed the parameter value since the last get_param() call.
inline bool
Processor::check_dirty (Id32 paramid) const
{
  const PParam *param = this->find_pparam (ParamId (paramid.id));
  return BSE_ISLIKELY (param) ? param->is_dirty() : false;
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

template<typename Class> extern inline AudioSignal::RegistryId
enroll_asp (const char *bfile = __builtin_FILE(), int bline = __builtin_LINE())
{
  AudioSignal::Processor::MakeProcessor makeasp = nullptr;
  if constexpr (std::is_constructible<Class>::value)
    {
      makeasp = [] (const std::any*) -> AudioSignal::ProcessorP {
        return std::make_shared<Class>();
      };
    }
  else
    {
      makeasp = [] (const std::any *any) -> AudioSignal::ProcessorP {
        return any ? std::make_shared<Class> (*any) : nullptr;
      };
    }
  return AudioSignal::Processor::registry_enroll (makeasp, bfile, bline);
}

class ProcessorImpl : public NotifierImpl, public virtual ProcessorIface {
  std::shared_ptr<AudioSignal::Processor> proc_;
  std::weak_ptr<Bse::ComboImpl> bcombo_;
public:
  explicit       ProcessorImpl     (AudioSignal::Processor &proc);
  DeviceInfo     processor_info    () override;
  StringSeq      list_properties   () override;
  PropertyIfaceP access_property   (const std::string &ident) override;
  PropertySeq    access_properties (const std::string &hints) override;
  ComboIfaceP    access_combo      () override;
  AudioSignal::ProcessorP
  const          audio_signal_processor () const;
};

} // Bse

namespace std {
template<>
struct hash<::Bse::AudioSignal::ParamInfo> {
  /// Hash value for Bse::AudioSignal::ParamInfo.
  size_t
  operator() (const ::Bse::AudioSignal::ParamInfo &pi) const
  {
    size_t h = ::std::hash<::Bse::CString>() (pi.ident);
    // h ^= ::std::hash (pi.label);
    // h ^= ::std::hash (pi.nick);
    // h ^= ::std::hash (pi.description);
    h ^= ::std::hash<::Bse::CString>() (pi.unit);
    h ^= ::std::hash<::Bse::CString>() (pi.hints);
    // min, max, step
    return h;
  }
};
} // std

#endif // __BSE_PROCESSOR_HH__
