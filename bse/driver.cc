// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "driver.hh"
#include "internal.hh"

namespace Bse {

static std::atomic<uint32> driver_id_counter { 1 };

Driver::Driver (const String &devid) :
  devid_ (devid)
{}

Driver::~Driver ()
{}

PcmDriver::PcmDriver (const String &devid) :
  Driver (devid)
{}

struct RegisteredPcmDriver {
  std::function<PcmDriverP (const String&)> create;
  std::function<void (Driver::EntryVec&, uint32)> list;
  uint32 driver_id = 0;
};
using RegisteredPcmDriverVec = std::vector<RegisteredPcmDriver>;
static RegisteredPcmDriverVec *registered_pcm_drivers = nullptr;

PcmDriverP
PcmDriver::open (const Entry &entry, const DriverConfig &config, Error *ep)
{
  std::function<PcmDriverP (const String&)> create;
  for (const auto &driver : *registered_pcm_drivers)
    if (driver.driver_id == entry.driverid)
      {
        create = driver.create;
        break;
      }
  Error error = Error::DEVICE_NOT_AVAILABLE;
  PcmDriverP driver = create ? create (entry.devid) : NULL;
  if (driver)
    {
      error = driver->open (config);
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

uint32
PcmDriver::register_driver (const std::function<PcmDriverP (const String&)> &create,
                            const std::function<void (EntryVec&, uint32)> &list)
{
  static RegisteredPcmDriverVec registered_pcm_driver_vector_mem;
  if (!registered_pcm_drivers)
    registered_pcm_drivers = new (&registered_pcm_driver_vector_mem) RegisteredPcmDriverVec();
  RegisteredPcmDriver rpd = { create, list, driver_id_counter++ };
  registered_pcm_drivers->push_back (rpd);
  return 1 + registered_pcm_drivers->size();
}

Driver::EntryVec
PcmDriver::list_drivers ()
{
  EntryVec entries;
  const size_t n_drivers = registered_pcm_drivers ? registered_pcm_drivers->size() : 0;
  for (size_t i = 0; i < n_drivers; i++)
    {
      EntryVec dentries;
      (*registered_pcm_drivers)[i].list (dentries, 1 + i);
      entries.insert (entries.end(), std::make_move_iterator (dentries.begin()), std::make_move_iterator (dentries.end()));
    }
  std::sort (entries.begin(), entries.end(), [] (const Entry &a, const Entry &b) {
      return a.priority < b.priority;
    });
  return entries;
}

} // Bse
