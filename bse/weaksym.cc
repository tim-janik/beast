// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bcore.hh"
#include "internal.hh"

namespace Bse::Test {

// weak symbols should be declared in a translation unit
// other than the one the symbols are used in:
// https://stackoverflow.com/a/13100465
__attribute__ ((__weak__)) const bool IntegrityCheck::enable_testing = false; // see internal.hh

} // Bse::Test
