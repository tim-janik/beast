// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "sfiwrapper.hh"
#include "sficxx.hh"
#include <sfi/sfi.hh>
#include <rapicorn-test.hh>
#include <errno.h>

/* --- initialization --- */
void
sfi_init (int *argcp, char **argv, const Bse::StringVector &args)
{
  static bool initialized = false;
  if (initialized)
    return;
  char *prg_name = argcp && *argcp ? g_path_get_basename (argv[0]) : NULL;
  if (args.size() == 1 && args[0] == "rapicorn-test-initialization=1")
    Rapicorn::init_core_test (prg_name, argcp, argv);
  else
    Rapicorn::parse_init_args (argcp, argv);

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
  if (!Rapicorn::url_show (url))
    g_warning ("Failed to start browser for URL: %s", url);
}
