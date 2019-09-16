// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "driver.hh"
#include "internal.hh"

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
        if (devid == "auto" && (entry.priority & 0x0000ffff))
          continue;     // ignore secondary devices during auto-selection
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

} // Bse
