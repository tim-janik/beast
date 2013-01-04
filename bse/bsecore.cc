// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsetype.hh"        /* import all required types first */
#include "bsemain.hh"
#include "bsecxxplugin.hh" /* includes bsecore.genidl.hh for us */
namespace Bse {
/* export definitions follow */
BSE_CXX_DEFINE_EXPORTS();
BSE_CXX_REGISTER_ALL_TYPES_FROM_BSECORE_IDL();
} // Bse
/* compile and initialize generated C stubs */
#include "bsegencore.cc"
void
_bse_init_c_wrappers (void)
{
  sfidl_types_init ();
}
