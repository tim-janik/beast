// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bcore.hh"

namespace Bse {

// weak symbols should be declared in a translation unit
// other than the one the symbols are used in:
// https://stackoverflow.com/a/13100465
__attribute__ ((__weak__)) const bool IntegrityCheck::enabled = false;

} // Bse
