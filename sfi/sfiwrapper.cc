// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "sfiwrapper.hh"
#include "sficxx.hh"
#include <sfi/sfi.hh>
#include <sfi/testing.hh>
#include <errno.h>

/* --- initialization --- */
void
sfi_init (int *argcp, char **argv, const Bse::StringVector &args)
{
  static bool initialized = false;
  if (initialized)
    return;
  _sfi_init_values ();
  _sfi_init_params ();
  _sfi_init_time ();
  _sfi_init_glue ();
  _sfi_init_file_crawler ();
  initialized = true;
}

/* --- url handling --- */
void
sfi_url_show (const char *url)
{
  if (!Bse::url_show (url))
    Bse::warning ("Failed to start browser for URL: %s", url);
}
