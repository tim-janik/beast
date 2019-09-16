// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "driver.hh"
#include "internal.hh"
#include "bsesequencer.hh"

#define DDEBUG(...)     Bse::debug ("driver", __VA_ARGS__)

namespace Bse {

// == Driver ==
static std::atomic<uint32> driver_id_counter { 1 };

Driver::Driver (const String &devid) :
  devid_ (devid)
{}

Driver::~Driver ()
{}

// == RegisteredDriver ==
template<typename DriverP>
struct RegisteredDriver {
  std::function<DriverP (const String&)>          create_;
  std::function<void (Driver::EntryVec&, uint32)> list_;
  uint32                                          driver_id_ = 0;
  using RegisteredDriverVector = std::vector<RegisteredDriver>;
  static RegisteredDriverVector&
  registered_driver_vector()
  {
    static RegisteredDriverVector registered_driver_vector_;
    return registered_driver_vector_;
  }
  static DriverP
  open (const Driver::Entry &entry, Driver::IODir iodir, Error *ep,
        const std::function<Error (DriverP, Driver::IODir)> &opener)
  {
    std::function<DriverP (const String&)> create;
    for (const auto &driver : registered_driver_vector())
      if (driver.driver_id_ == entry.driverid)
        {
          create = driver.create_;
          break;
        }
    Error error = Error::DEVICE_NOT_AVAILABLE;
    DriverP driver = create ? create (entry.devid) : NULL;
    if (driver)
      {
        error = opener (driver, iodir);
        if (ep)
          *ep = error;
        if (error == Error::NONE)
          {
            assert_return (driver->opened() == true, nullptr);
            assert_return (!(iodir & Driver::READONLY) || driver->readable(), nullptr);
            assert_return (!(iodir & Driver::WRITEONLY) || driver->writable(), nullptr);
          }
        else
          driver = nullptr;
      }
    else if (ep)
      *ep = error;
    return driver;
  }
  static uint32
  register_driver (const std::function<DriverP (const String&)> &create,
                   const std::function<void (Driver::EntryVec&, uint32)> &list)
  {
    auto &vec = registered_driver_vector();
    RegisteredDriver rd = { create, list, driver_id_counter++ };
    vec.push_back (rd);
    return rd.driver_id_;
  }
  static Driver::EntryVec
  list_drivers (const Driver::EntryVec &pseudos)
  {
    Driver::EntryVec entries;
    std::copy (pseudos.begin(), pseudos.end(), std::back_inserter (entries));
    auto &vec = registered_driver_vector();
    for (const auto &rd : vec)
      {
        Driver::EntryVec dentries;
        rd.list_ (dentries, rd.driver_id_);
        entries.insert (entries.end(), std::make_move_iterator (dentries.begin()), std::make_move_iterator (dentries.end()));
      }
    std::sort (entries.begin(), entries.end(), [] (const Driver::Entry &a, const Driver::Entry &b) {
        return a.priority < b.priority;
      });
    return entries;
  }
};

// == PcmDriver ==
PcmDriver::PcmDriver (const String &devid) :
  Driver (devid)
{}

PcmDriverP
PcmDriver::open (const String &devid, IODir desired, IODir required, const PcmDriverConfig &config, Error *ep)
{
  Driver::EntryVec entries = list_drivers();
  for (const auto &entry : entries)
    if (entry.driverid && (entry.devid == devid || devid == "auto"))
      {
        if (devid == "auto" && (entry.priority >= PSEUDO ||
                                (entry.priority & 0x0000ffff))) // ignore secondary devices during auto-selection
          continue;
        PcmDriverP pcm_driver = RegisteredDriver<PcmDriverP>::open (entry, desired, ep,
                                                                    [&config] (PcmDriverP d, IODir iodir) {
                                                                      return d->open (iodir, config);
                                                                    });
        if (!pcm_driver && required && desired != required)
          pcm_driver = RegisteredDriver<PcmDriverP>::open (entry, required, ep,
                                                           [&config] (PcmDriverP d, IODir iodir) {
                                                             return d->open (iodir, config);
                                                           });
        if (pcm_driver)
          return pcm_driver;
      }
  return nullptr;
}

uint32
PcmDriver::register_driver (const std::function<PcmDriverP (const String&)> &create,
                            const std::function<void (EntryVec&, uint32)> &list)
{
  return RegisteredDriver<PcmDriverP>::register_driver (create, list);
}

Driver::EntryVec
PcmDriver::list_drivers ()
{
  Driver::Entry entry;
  entry.devid = "auto";
  entry.name = _("Automatic PCM card selection");
  entry.readonly = false;
  entry.writeonly = false;
  entry.priority = Driver::PSEUDO + Driver::WCARD * 1 + Driver::WDEV * 1 + Driver::WSUB * 1;
  entry.driverid = 0;
  Driver::EntryVec pseudos;
  pseudos.push_back (entry);
  return RegisteredDriver<PcmDriverP>::list_drivers (pseudos);
}

// == MidiDriver ==
MidiDriver::MidiDriver (const String &devid) :
  Driver (devid)
{}

MidiDriverP
MidiDriver::open (const String &devid, IODir iodir, Error *ep)
{
  Driver::EntryVec entries = list_drivers();
  for (const auto &entry : entries)
    if (entry.driverid && (entry.devid == devid || devid == "auto"))
      {
        if (devid == "auto" && entry.priority >= PSEUDO)
          continue;
        MidiDriverP midi_driver = RegisteredDriver<MidiDriverP>::open (entry, iodir, ep,
                                                                       [] (MidiDriverP d, IODir iodir) {
                                                                         return d->open (iodir);
                                                                       });
        if (midi_driver)
          return midi_driver;
      }
  return nullptr;
}

uint32
MidiDriver::register_driver (const std::function<MidiDriverP (const String&)> &create,
                            const std::function<void (EntryVec&, uint32)> &list)
{
  return RegisteredDriver<MidiDriverP>::register_driver (create, list);
}

Driver::EntryVec
MidiDriver::list_drivers ()
{
  Driver::Entry entry;
  entry.devid = "auto";
  entry.name = _("Automatic MIDI driver selection");
  entry.readonly = false;
  entry.writeonly = false;
  entry.priority = Driver::PSEUDO + Driver::WCARD * 1 + Driver::WDEV * 1 + Driver::WSUB * 1;
  entry.driverid = 0;
  Driver::EntryVec pseudos;
  pseudos.push_back (entry);
  return RegisteredDriver<MidiDriverP>::list_drivers (pseudos);
}

// == NullPcmDriver ==
class NullPcmDriver : public PcmDriver {
  uint          n_channels_ = 0;
  uint          mix_freq_ = 0;
  uint          busy_us_ = 0;
  uint          sleep_us_ = 0;
public:
  explicit      NullPcmDriver (const String &devid) : PcmDriver (devid) {}
  static PcmDriverP
  create (const String &devid)
  {
    auto pdriverp = std::make_shared<NullPcmDriver> (devid);
    return pdriverp;
  }
  virtual float
  pcm_frequency () const override
  {
    return mix_freq_;
  }
  virtual void
  close () override
  {
    assert_return (opened());
    flags_ &= ~size_t (Flags::OPENED | Flags::READABLE | Flags::WRITABLE);
  }
  virtual Error
  open (IODir iodir, const PcmDriverConfig &config) override
  {
    assert_return (!opened(), Error::INTERNAL);
    // setup request
    const bool nosleep = true;
    const bool require_readable = iodir == READONLY || iodir == READWRITE;
    const bool require_writable = iodir == WRITEONLY || iodir == READWRITE;
    flags_ |= Flags::READABLE * require_readable;
    flags_ |= Flags::WRITABLE * require_writable;
    n_channels_ = config.n_channels;
    mix_freq_ = config.mix_freq;
    busy_us_ = 0;
    sleep_us_ = nosleep ? 0 : 10 * 1000;
    flags_ |= Flags::OPENED;
    DDEBUG ("NULL-PCM: opening\"%s\" freq=%f channels=%d: %s", devid_, mix_freq_, n_channels_, bse_error_blurb (Error::NONE));
    return Error::NONE;
  }
  virtual bool
  pcm_check_io (long *timeoutp) override
  {
    // keep the sequencer busy or we will constantly timeout
    Sequencer::instance().wakeup();
    *timeoutp = 1;
    // ensure sequencer fairness
    return !Sequencer::instance().thread_lagging (2);
  }
  virtual uint
  pcm_latency () const override
  {
    // total latency in frames
    return mix_freq_ / 10;
  }
  virtual size_t
  pcm_read (size_t n, float *values) override
  {
    memset (values, 0, sizeof (values[0]) * n);
    return n;
  }
  virtual void
  pcm_write (size_t n, const float *values) override
  {
    busy_us_ += n * 1000000 / mix_freq_;
    if (busy_us_ >= 100 * 1000)
      {
        busy_us_ = 0;
        // give cpu to other applications (we might run at nice level -20)
        if (sleep_us_)
          g_usleep (sleep_us_);
      }
  }
  static void
  list_drivers (Driver::EntryVec &entries, uint32 driverid)
  {
    Driver::Entry entry;
    entry.devid = "null";
    entry.name = "Null PCM Driver";
    entry.blurb = _("Discard all PCM output and provide zeros as PCM input");
    entry.readonly = false;
    entry.writeonly = false;
    entry.priority = Driver::DNULL;
    entry.driverid = driverid;
    entries.push_back (entry);
  }
};

static const uint32 null_pcm_driverid = PcmDriver::register_driver (NullPcmDriver::create, NullPcmDriver::list_drivers);

// == NullMidiDriver ==
class NullMidiDriver : public MidiDriver {
public:
  explicit      NullMidiDriver (const String &devid) : MidiDriver (devid) {}
  static MidiDriverP
  create (const String &devid)
  {
    auto pdriverp = std::make_shared<NullMidiDriver> (devid);
    return pdriverp;
  }
  virtual void
  close () override
  {
    assert_return (opened());
    flags_ &= ~size_t (Flags::OPENED | Flags::READABLE | Flags::WRITABLE);
  }
  virtual Error
  open (IODir iodir) override
  {
    assert_return (!opened(), Error::INTERNAL);
    // setup request
    const bool require_readable = iodir == READONLY || iodir == READWRITE;
    const bool require_writable = iodir == WRITEONLY || iodir == READWRITE;
    flags_ |= Flags::READABLE * require_readable;
    flags_ |= Flags::WRITABLE * require_writable;
    flags_ |= Flags::OPENED;
    DDEBUG ("NULL-MIDI: opening\"%s\": %s", devid_, bse_error_blurb (Error::NONE));
    return Error::NONE;
  }
  static void
  list_drivers (Driver::EntryVec &entries, uint32 driverid)
  {
    Driver::Entry entry;
    entry.devid = "null";
    entry.name = "Null MIDI Driver";
    entry.blurb = _("Discard all MIDI events");
    entry.readonly = false;
    entry.writeonly = false;
    entry.priority = Driver::DNULL;
    entry.driverid = driverid;
    entries.push_back (entry);
  }
};

static const uint32 null_midi_driverid = MidiDriver::register_driver (NullMidiDriver::create, NullMidiDriver::list_drivers);

} // Bse
