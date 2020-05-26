// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsestartup.hh"
#include "bsemain.hh"
#include "bse/internal.hh"
#include "bse/bseserver.hh"
#include <bse/bse.hh>

namespace Bse {

// == BSE Initialization ==

/** Initialize and start BSE.
 * Initialize the BSE library and start the main BSE thread. Arguments specific to BSE are removed
 * from @a argc / @a argv.
 */
void
init_async (int *argc, char **argv, const char *app_name, const StringVector &args)
{
  _bse_init_async (argc, argv, app_name, args);
}

/// Check wether init_async() still needs to be called.
bool
init_needed ()
{
  return _bse_initialized() == false;
}

/// Retrieve a handle for the Bse::Server instance managing the Bse thread.
#if 0 // FIXME
ServerHandle
init_server_instance () // bse.hh
{
  ServerH server;
  server = BSE_SERVER.__handle__();
  return server;
}
#endif

} // Bse

// #include "bse/bseapi_handles.cc"        // build IDL client interface
