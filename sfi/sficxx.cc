// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "sficxx.hh"
#include "sfi.hh"
namespace Sfi {
static void
sfi_init_cxx (void)
{
  g_type_init ();       /* just in case this hasn't been called already */
  _sfi_init_values ();
  _sfi_init_params ();
  _sfi_init_time ();
  _sfi_init_glue ();
  _sfi_init_file_crawler ();
}
static Birnet::InitHook sfi_init_hook (sfi_init_cxx);
} // Sfi
/* vim:set ts=8 sts=2 sw=2: */
