// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bcore.hh"

namespace Bse {

// weak symbols should be declared in a translation unit
// other than the one the symbols are used in:
// https://stackoverflow.com/a/13100465
__attribute__ ((__weak__)) const bool IntegrityCheck::enabled = false;


#include "bse/buildid.cc"

// from platform.hh
std::string
version_buildid ()
{
  return static_bse_version_buildid;
}

// from platform.hh
std::string
version_date ()
{
  return static_bse_version_date;
}

} // Bse
