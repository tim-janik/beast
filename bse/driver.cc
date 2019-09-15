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
  template<typename OpenerConfig> static DriverP
  open (const Driver::Entry &entry, const OpenerConfig &config, Error *ep,
        const std::function<Error (DriverP, const OpenerConfig&)> &opener)
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
        error = opener (driver, config);
        if (ep)
          *ep = error;
        if (error == Error::NONE)
          {
            assert_return (driver->opened() == true, nullptr);
            assert_return (!config.require_readable || driver->readable(), nullptr);
            assert_return (!config.require_writable || driver->writable(), nullptr);
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
  list_drivers ()
  {
    auto &vec = registered_driver_vector();
    Driver::EntryVec entries;
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
PcmDriver::open (const Entry &entry, const DriverConfig &config, Error *ep)
{
  return RegisteredDriver<PcmDriverP>::open<DriverConfig> (entry, config, ep,
                                                           [] (PcmDriverP d, const DriverConfig &c) {
                                                             return d->open (c);
                                                           });
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
  return RegisteredDriver<PcmDriverP>::list_drivers();
}

// == MidiDriver ==
MidiDriver::MidiDriver (const String &devid) :
  Driver (devid)
{}

MidiDriverP
MidiDriver::open (const Entry &entry, const DriverConfig &config, Error *ep)
{
  return RegisteredDriver<MidiDriverP>::open<DriverConfig> (entry, config, ep,
                                                            [] (MidiDriverP d, const DriverConfig &c) {
                                                              return d->open (c);
                                                            });
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
  return RegisteredDriver<MidiDriverP>::list_drivers();
}

} // Bse
