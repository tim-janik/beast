// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "driver.hh"
#include "internal.hh"

namespace Bse {

Driver::Driver (const String &devid) :
  devid_ (devid)
{}

Driver::~Driver ()
{}

DriverP
Driver::open (const Entry &entry, const DriverConfig &config, Error *ep)
{
  assert_return (entry.create, nullptr);
  Error error = Error::DEVICE_NOT_AVAILABLE;
  DriverP driver = entry.create (entry.devid);
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

typedef void (*DriverListFunc)  (Driver::EntryVec&);
using DriverListFuncs = std::vector<DriverListFunc>;
static DriverListFuncs *driver_list_funcs = nullptr;

bool
Driver::register_driver (Type, void (*func) (EntryVec&))
{
  static DriverListFuncs driver_list_funcs_mem;
  if (!driver_list_funcs)
    driver_list_funcs = new (&driver_list_funcs_mem) DriverListFuncs();
  driver_list_funcs->push_back (func);
  return true;
}

Driver::EntryVec
Driver::list_drivers (Type type)
{
  EntryVec entries;
  for (const auto &f : *driver_list_funcs)
    {
      EntryVec dentries;
      f (dentries);
      entries.insert (entries.end(), std::make_move_iterator (dentries.begin()), std::make_move_iterator (dentries.end()));
    }
  std::sort (entries.begin(), entries.end(), [] (const Entry &a, const Entry &b) {
      return a.priority < b.priority;
    });
  return entries;
}

} // Bse
