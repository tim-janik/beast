// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "driver.hh"

namespace Bse {

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
