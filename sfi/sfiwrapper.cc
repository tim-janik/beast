// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "sfiwrapper.hh"
#include "sficxx.hh"
#include <sfi/sfi.hh>
#include <rapicorn-test.hh>
#include <errno.h>

namespace Bse {

/// Caching flag to inhibit useless bse_debug() calls.
bool volatile _cached_bse_debug = true;

/// Issue a debugging message, configurable via #$BSE_DEBUG.
void
bse_debug (const char *key, const char *file_path, const int line, const String &message)
{
  envkey_debug_message ("BSE_DEBUG", key, file_path, line, message, &_cached_bse_debug);
}

#ifdef DOXYGEN
/** Check if debugging is enabled for @a key.
 * This function checks if #$BSE_DEBUG contains @a key or "all" and returns true
 * if debugging is enabled for the given key. The @a key argument may be NULL in which
 * case the function checks if general debugging is enabled.
 */
bool bse_debug_enabled (const char *key);
#endif // DOXYGEN

bool
_bse_debug_enabled (const char *key)
{
  return envkey_debug_check ("BSE_DEBUG", key, &_cached_bse_debug);
}

/// Check if the feature toggle @a key is enabled in #$BSE_FLIPPER.
bool
bse_flipper_check (const char *key)
{
  return envkey_flipper_check ("BSE_FLIPPER", key);
}

} // Bse

/* --- initialization --- */
void
sfi_init (int *argcp, char **argv, const char *app_name, const Bse::StringVector &args)
{
  static bool initialized = false;
  if (initialized)
    return;
  char *prg_name = argcp && *argcp ? g_path_get_basename (argv[0]) : NULL;
  if (args.size() == 1 && args[0] == "rapicorn-test-initialization=1")
    Rapicorn::init_core_test (app_name ? app_name : prg_name, argcp, argv);
  else
    Rapicorn::init_core (app_name ? app_name : prg_name, argcp, argv);

  g_type_init ();       /* just in case this hasn't been called already */
  _sfi_init_values ();
  _sfi_init_params ();
  _sfi_init_time ();
  _sfi_init_glue ();
  _sfi_init_file_crawler ();
  initialized = true;
}

/* --- file testing --- */
bool
birnet_file_check (const char *file,
                   const char *mode)
{
  return Rapicorn::Path::check (file ? file : "", mode ? mode : "");
}
bool
birnet_file_equals (const char *file1,
                    const char *file2)
{
  return Rapicorn::Path::equals (file1 ? file1 : "", file2 ? file2 : "");
}

/* --- url handling --- */
void
sfi_url_show (const char *url)
{
  if (!Rapicorn::url_show (url))
    g_warning ("Failed to start browser for URL: %s", url);
}
