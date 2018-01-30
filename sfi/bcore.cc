// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bcore.hh"
#include "platform.hh"
#include <cstring>
#include <unistd.h>     // _exit
#include <sys/time.h>   // gettimeofday

// == limits.h & float.h checks ==
// assert several assumptions the code makes
static_assert (CHAR_BIT     == +8, "");
static_assert (SCHAR_MIN    == -128, "");
static_assert (SCHAR_MAX    == +127, "");
static_assert (UCHAR_MAX    == +255, "");
static_assert (SHRT_MIN     == -32768, "");
static_assert (SHRT_MAX     == +32767, "");
static_assert (USHRT_MAX    == +65535, "");
static_assert (INT_MIN      == -2147483647 - 1, "");
static_assert (INT_MAX      == +2147483647, "");
static_assert (UINT_MAX     == +4294967295U, "");
static_assert (INT64_MIN    == -9223372036854775807LL - 1, "");
static_assert (INT64_MAX    == +9223372036854775807LL, "");
static_assert (UINT64_MAX   == +18446744073709551615LLU, "");
static_assert (LDBL_MIN     <= 1E-37, "");
static_assert (LDBL_MAX     >= 1E+37, "");
static_assert (LDBL_EPSILON <= 1E-9, "");
static_assert (FLT_MIN      <= 1E-37, "");
static_assert (FLT_MAX      >= 1E+37, "");
static_assert (FLT_EPSILON  <= 1E-5, "");
static_assert (DBL_MIN      <= 1E-37, "");
static_assert (DBL_MAX      >= 1E+37, "");
static_assert (DBL_EPSILON  <= 1E-9, "");

namespace Bse {

// == Memory Utilities ==
/**
 * The fmsb() function returns the position of the most significant bit set in the word @a val.
 * The least significant bit is position 1 and the most significant position is, for example, 32 or 64.
 * @returns The position of the most significant bit set is returned, or 0 if no bits were set.
 */
int // 0 or 1..64
fmsb (uint64 val)
{
  if (val >> 32)
    return 32 + fmsb (val >> 32);
  int nb = 32;
  do
    {
      nb--;
      if (val & (1U << nb))
        return nb + 1;  /* 1..32 */
    }
  while (nb > 0);
  return 0; /* none found */
}

/// Allocate a block of memory aligned to at least @a alignment bytes.
void*
aligned_alloc (size_t total_size, size_t alignment, uint8 **free_pointer)
{
  assert_return (free_pointer != NULL, NULL);
  uint8 *aligned_mem = new uint8[total_size];
  *free_pointer = aligned_mem;
  if (aligned_mem && (!alignment || 0 == size_t (aligned_mem) % alignment))
    return aligned_mem;
  if (aligned_mem)
    delete[] aligned_mem;
  aligned_mem = new uint8[total_size + alignment - 1];
  assert_return (aligned_mem != NULL, NULL);
  *free_pointer = aligned_mem;
  if (size_t (aligned_mem) % alignment)
    aligned_mem += alignment - size_t (aligned_mem) % alignment;
  return aligned_mem;
}

/// Release a block of memory allocated through aligned_malloc().
void
aligned_free (uint8 **free_pointer)
{
  assert_return (free_pointer != NULL);
  if (*free_pointer)
    {
      uint8 *data = *free_pointer;
      *free_pointer = NULL;
      delete[] data;
    }
}

/// Find @a feature in @a config, return its value or @a fallback.
String
feature_toggle_find (const String &config, const String &feature, const String &fallback)
{
  String haystack = ":" + config + ":";
  String needle0 = ":no-" + feature + ":";
  String needle1 = ":" + feature + ":";
  String needle2 = ":" + feature + "=";
  const char *n0 = g_strrstr (haystack.c_str(), needle0.c_str());
  const char *n1 = g_strrstr (haystack.c_str(), needle1.c_str());
  const char *n2 = g_strrstr (haystack.c_str(), needle2.c_str());
  if (n0 && (!n1 || n0 > n1) && (!n2 || n0 > n2))
    return "0";         // ":no-feature:" is the last toggle in config
  if (n1 && (!n2 || n1 > n2))
    return "1";         // ":feature:" is the last toggle in config
  if (!n2)
    return fallback;    // no "feature" variant found
  const char *value = n2 + strlen (needle2.c_str());
  const char *end = strchr (value, ':');
  return end ? String (value, end - value) : String (value);
}

/// Check for @a feature in @a config, if @a feature is empty, checks for *any* feature.
bool
feature_toggle_bool (const char *config, const char *feature)
{
  if (feature && feature[0])
    return string_to_bool (feature_toggle_find (config ? config : "", feature));
  // check if *any* feature is enabled in config
  if (!config || !config[0])
    return false;
  const size_t l = strlen (config);
  for (size_t i = 0; i < l; i++)
    if (config[i] && !strchr (": \t\n\r=", config[i]))
      return true;      // found *some* non-space and non-separator config item
  return false;         // just whitespace
}

// == External Helpers ==
/**
 * Find a suitable WWW user agent (taking user configurations into account) and
 * start it to display @a url. Several user agents are tried before giving up.
 * @returns @a True if a user agent could be launched successfuly.
 */
bool
url_show (const char *url)
{
  static struct {
    const char   *prg, *arg1, *prefix, *postfix;
    bool          asyncronous; /* start asyncronously and check exit code to catch launch errors */
    volatile bool disabled;
  } www_browsers[] = {
    /* program */               /* arg1 */      /* prefix+URL+postfix */
    /* configurable, working browser launchers */
    { "gnome-open",             NULL,           "", "", 0 }, /* opens in background, correct exit_code */
    { "exo-open",               NULL,           "", "", 0 }, /* opens in background, correct exit_code */
    /* non-configurable working browser launchers */
    { "kfmclient",              "openURL",      "", "", 0 }, /* opens in background, correct exit_code */
    { "gnome-moz-remote",       "--newwin",     "", "", 0 }, /* opens in background, correct exit_code */
#if 0
    /* broken/unpredictable browser launchers */
    { "browser-config",         NULL,            "", "", 0 }, /* opens in background (+ sleep 5), broken exit_code (always 0) */
    { "xdg-open",               NULL,            "", "", 0 }, /* opens in foreground (first browser) or background, correct exit_code */
    { "sensible-browser",       NULL,            "", "", 0 }, /* opens in foreground (first browser) or background, correct exit_code */
    { "htmlview",               NULL,            "", "", 0 }, /* opens in foreground (first browser) or background, correct exit_code */
#endif
    /* direct browser invocation */
    { "x-www-browser",          NULL,           "", "", 1 }, /* opens in foreground, browser alias */
    { "firefox",                NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
    { "mozilla-firefox",        NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
    { "mozilla",                NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
    { "konqueror",              NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
    { "opera",                  "-newwindow",   "", "", 1 }, /* opens in foreground, correct exit_code */
    { "galeon",                 NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
    { "epiphany",               NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
    { "amaya",                  NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
    { "dillo",                  NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
  };
  uint i;
  for (i = 0; i < ARRAY_SIZE (www_browsers); i++)
    if (!www_browsers[i].disabled)
      {
        char *args[128] = { 0, };
        uint n = 0;
        args[n++] = (char*) www_browsers[i].prg;
        if (www_browsers[i].arg1)
          args[n++] = (char*) www_browsers[i].arg1;
        char *string = g_strconcat (www_browsers[i].prefix, url, www_browsers[i].postfix, NULL);
        args[n] = string;
        GError *error = NULL;
        char fallback_error[64] = "Ok";
        bool success;
        if (!www_browsers[i].asyncronous) /* start syncronously and check exit code */
          {
            int exit_status = -1;
            success = g_spawn_sync (NULL, /* cwd */
                                    args,
                                    NULL, /* envp */
                                    G_SPAWN_SEARCH_PATH,
                                    NULL, /* child_setup() */
                                    NULL, /* user_data */
                                    NULL, /* standard_output */
                                    NULL, /* standard_error */
                                    &exit_status,
                                    &error);
            success = success && !exit_status;
            if (exit_status)
              g_snprintf (fallback_error, sizeof (fallback_error), "exitcode: %u", exit_status);
          }
        else
          success = g_spawn_async (NULL, /* cwd */
                                   args,
                                   NULL, /* envp */
                                   G_SPAWN_SEARCH_PATH,
                                   NULL, /* child_setup() */
                                   NULL, /* user_data */
                                   NULL, /* child_pid */
                                   &error);
        g_free (string);
        Bse::debug ("URL", "show \"%s\": %s: %s", url, args[0], error ? error->message : fallback_error);
        g_clear_error (&error);
        if (success)
          return true;
        www_browsers[i].disabled = true;
      }
  /* reset all disabled states if no browser could be found */
  for (i = 0; i < ARRAY_SIZE (www_browsers); i++)
    www_browsers[i].disabled = false;
  return false;
}

// == Internal ==
namespace Internal {

void
printout_string (const String &string)
{
  // some platforms (_WIN32) don't properly flush on '\n'
  fflush (stderr); // preserve ordering
  fputs (string.c_str(), stdout);
  fflush (stdout);
}

void
printerr_string (const String &string)
{
  // some platforms (_WIN32) don't properly flush on '\n'
  fflush (stdout); // preserve ordering
  fputs (string.c_str(), stderr);
  fflush (stderr);
}

bool debug_any_enabled = true; // initialized by debug_key_enabled()

bool
debug_key_enabled (const char *conditional)
{
  // cache $BSE_DEBUG and setup debug_any_enabled;
  static const char *const debug_flags = [] () {
    const char *f = getenv ("BSE_DEBUG");
    std::string flags = !f ? "" : ":" + std::string (f) + ":";
    char *result = new char [flags.size() + 1];
    if (result)
      strcpy (result, flags.data());
    debug_any_enabled = result && result[0];
    return result;
  } ();
  // find conditional in colon-separated $BSE_DEBUG
  if (conditional && debug_flags)
    {
      const char *const flag = strstr (debug_flags, conditional);
      const int l = strlen (conditional);
      if (flag && flag > debug_flags && flag[-1] == ':' && l)
        {
          if (flag[l] == ':' || // also allow =1 =yes =true =on
              (flag[l] == '=' && (strchr ("123456789yYtT", flag[l + 1]) ||
                                  strncasecmp (flag + l, "=on", 3) == 0)))
            return true;
        }
      else if (strstr (debug_flags, ":all:") != NULL)
        return true;
    }
  return false;
}

void
diagnostic (const char *file, int line, const char *func, char kind, const std::string &info)
{
  String msg = Aida::diagnostic_message (file, line, func, kind, info, 0);
  fflush (stdout);
  printerr ("%s", msg);
  fflush (stderr);
}

void
debug_diagnostic (const char *prefix, const std::string &message)
{
  struct timeval tv = { 0, };
  gettimeofday (&tv, NULL);
  const char *const newline = !message.empty() && message.data()[message.size() - 1] == '\n' ? "" : "\n";
  const String pprefix = prefix ? prefix : executable_name();
  printerr ("%u.%06u %s: %s%s", tv.tv_sec, tv.tv_usec, pprefix, message, newline);
}

struct EarlyStartup101 {
  EarlyStartup101()
  {
    if (debug_key_enabled ("") ||       // force debug_any_enabled initialization
        debug_any_enabled)              // print startup time if *any* debugging is enabled
      {
        const time_t now = time (NULL);
        struct tm gtm = { 0, };
        gmtime_r (&now, &gtm);
        char buffer[1024] = { 0, };
        strftime (buffer, sizeof (buffer) - 1, "%Y-%m-%d %H:%M:%S UTC", &gtm);
        debug_diagnostic (NULL, "startup: " + String() + buffer);
      }
  }
};

static EarlyStartup101 _early_startup_101 __attribute__ ((init_priority (101)));

} // Internal

} // Bse
