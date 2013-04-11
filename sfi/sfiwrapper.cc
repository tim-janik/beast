// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "sfiwrapper.hh"
#include <birnet/birnet.hh>
#include <errno.h>

namespace Bse {

/// Caching flag to inhibit useless bse_debug() calls.
bool volatile _cached_bse_debug = true;

/// Issue a debugging message, configurable via #$BSE_DEBUG.
void
bse_debug (const char *key, const char *file_path, const int line, const char *format, ...)
{
  va_list vargs;
  va_start (vargs, format);
  envkey_debug_message ("BSE_DEBUG", key, file_path, line, format, vargs, &_cached_rapicorn_debug);
  va_end (vargs);
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
sfi_init (int            *argcp,
          char         ***argvp,
          const char     *app_name,
          SfiInitValue    sivalues[])
{
  BIRNET_STATIC_ASSERT (sizeof (SfiInitValue) == sizeof (BirnetInitValue));
  BIRNET_STATIC_ASSERT (offsetof (SfiInitValue, value_name) == offsetof (BirnetInitValue, value_name));
  BIRNET_STATIC_ASSERT (offsetof (SfiInitValue, value_string) == offsetof (BirnetInitValue, value_string));
  BIRNET_STATIC_ASSERT (offsetof (SfiInitValue, value_num) == offsetof (BirnetInitValue, value_num));
  Birnet::birnet_init (argcp, argvp, app_name, (BirnetInitValue*) sivalues);
}
bool
sfi_init_value_bool (SfiInitValue *value)
{
  return Birnet::init_value_bool ((BirnetInitValue*) value);
}
double
sfi_init_value_double (SfiInitValue *value)
{
  return Birnet::init_value_double ((BirnetInitValue*) value);
}
gint64
sfi_init_value_int (SfiInitValue *value)
{
  return Birnet::init_value_int ((BirnetInitValue*) value);
}
SfiInitSettings
sfi_init_settings (void)
{
  return ::Birnet::init_settings();
}
/* --- file testing --- */
bool
birnet_file_check (const char *file,
                   const char *mode)
{
  return Birnet::Path::check (file ? file : "", mode ? mode : "");
}
bool
birnet_file_equals (const char *file1,
                    const char *file2)
{
  return Birnet::Path::equals (file1 ? file1 : "", file2 ? file2 : "");
}

/* --- url handling --- */
void
sfi_url_show (const char *url)
{
  return Birnet::url_show (url);
}
void
sfi_url_show_with_cookie (const char *url,
                          const char *url_title,
                          const char *cookie)
{
  return Birnet::url_show_with_cookie (url, url_title, cookie);
}
bool
sfi_url_test_show (const char *url)
{
  return Birnet::url_test_show (url);
}
bool
sfi_url_test_show_with_cookie (const char *url,
                               const char *url_title,
                               const char *cookie)
{
  return Birnet::url_test_show_with_cookie (url, url_title, cookie);
}

void
sfi_runtime_problem (char        ewran_tag,
                     const char *domain,
                     const char *file,
                     int         line,
                     const char *funcname,
                     const char *msgformat,
                     ...)
{
  va_list args;
  va_start (args, msgformat);
  ::Birnet::birnet_runtime_problemv (ewran_tag, domain, file, line, funcname, msgformat, args);
  va_end (args);
}
/* vim:set ts=8 sts=2 sw=2: */
