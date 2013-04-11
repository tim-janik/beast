// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include <glib.h>
#include "birnetutils.hh"
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <vector>
#include <algorithm>
#include <cxxabi.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#ifndef _
#define _(s)    s
#endif
namespace Birnet {
static const InitSettings *birnet_init_settings = NULL;
InitSettings
init_settings ()
{
  return *birnet_init_settings;
}
/* --- InitHooks --- */
static void    (*run_init_hooks) () = NULL;
static InitHook *init_hooks = NULL;
InitHook::InitHook (InitHookFunc _func,
                    int          _priority) :
  next (NULL), priority (_priority), hook (_func)
{
  BIRNET_ASSERT (birnet_init_settings == NULL);
  /* the above assertion guarantees single-threadedness */
  next = init_hooks;
  init_hooks = this;
  run_init_hooks = invoke_hooks;
}
void
InitHook::invoke_hooks (void)
{
  std::vector<InitHook*> hv;
  struct Sub {
    static int
    init_hook_cmp (const InitHook *const &v1,
                   const InitHook *const &v2)
    {
      return v1->priority < v2->priority ? -1 : v1->priority > v2->priority;
    }
  };
  for (InitHook *ihook = init_hooks; ihook; ihook = ihook->next)
    hv.push_back (ihook);
  stable_sort (hv.begin(), hv.end(), Sub::init_hook_cmp);
  for (std::vector<InitHook*>::iterator it = hv.begin(); it != hv.end(); it++)
    (*it)->hook();
}
/* --- initialization --- */
static InitSettings global_init_settings = {
  false,        /* stand_alone */
  false,        /* perf_test */
};
static void
birnet_parse_settings_and_args (InitValue *value,
                                int       *argc_p,
                                char    ***argv_p)
{
  bool parse_test_args = false;
  /* apply settings */
  if (value)
    while (value->value_name)
      {
        if (strcmp (value->value_name, "stand-alone") == 0)
          global_init_settings.stand_alone = init_value_bool (value);
        else if (strcmp (value->value_name, "test-quick") == 0)
          global_init_settings.test_quick = init_value_bool (value);
        else if (strcmp (value->value_name, "test-slow") == 0)
          global_init_settings.test_slow = init_value_bool (value);
        else if (strcmp (value->value_name, "test-perf") == 0)
          global_init_settings.test_perf = init_value_bool (value);
        else if (strcmp (value->value_name, "birnet-test-parse-args") == 0)
          parse_test_args = init_value_bool (value);
        value++;
      }
  /* parse args */
  uint argc = *argc_p;
  char **argv = *argv_p;
  for (uint i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "--g-fatal-warnings") == 0)
        {
          uint fatal_mask = g_log_set_always_fatal (GLogLevelFlags (G_LOG_FATAL_MASK));
          fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
          g_log_set_always_fatal (GLogLevelFlags (fatal_mask));
          argv[i] = NULL;
        }
      else if (parse_test_args && strcmp ("--test-quick", argv[i]) == 0)
        {
          global_init_settings.test_quick = true;
          argv[i] = NULL;
        }
      else if (parse_test_args && strcmp ("--test-slow", argv[i]) == 0)
        {
          global_init_settings.test_slow = true;
          argv[i] = NULL;
        }
      else if (parse_test_args && strcmp ("--test-perf", argv[i]) == 0)
        {
          global_init_settings.test_perf = true;
          argv[i] = NULL;
        }
    }
  /* fallback handling for tests */
  if (parse_test_args && !global_init_settings.test_quick && !global_init_settings.test_slow && !global_init_settings.test_perf)
    global_init_settings.test_quick = true;
  /* collapse args */
  uint e = 1;
  for (uint i = 1; i < argc; i++)
    if (argv[i])
      {
        argv[e++] = argv[i];
        if (i >= e)
          argv[i] = NULL;
      }
  *argc_p = e;
}
void
birnet_init (int        *argcp,
             char     ***argvp,
             const char *app_name,
             InitValue   ivalues[])
{

  /* mandatory initial initialization */
  if (!g_threads_got_initialized)
    g_thread_init (NULL);
  /* update program/application name upon repeated initilization */
  char *prg_name = argcp && *argcp ? g_path_get_basename ((*argvp)[0]) : NULL;

  // ensure we have Rapicorn setup early
  Rapicorn::init_core (app_name ? app_name : prg_name, argcp, *argvp);

  if (birnet_init_settings != NULL)
    {
      if (prg_name && !g_get_prgname ())
        g_set_prgname (prg_name);
      g_free (prg_name);
      if (app_name && !g_get_application_name())
        g_set_application_name (app_name);
      return;   /* simply ignore repeated initializations */
    }
  /* normal initialization */
  birnet_init_settings = &global_init_settings;
  birnet_parse_settings_and_args (ivalues, argcp, argvp);
  if (prg_name)
    g_set_prgname (prg_name);
  g_free (prg_name);
  if (app_name && (!g_get_application_name() || g_get_application_name() == g_get_prgname()))
    g_set_application_name (app_name);
  /* initialize random numbers */
  {
    struct timeval tv;
    gettimeofday (&tv, NULL);
    srand48 (tv.tv_usec + (tv.tv_sec << 16));
    srand (lrand48());
  }
  /* initialize sub systems */
  if (run_init_hooks)
    run_init_hooks();
}
bool
init_value_bool (InitValue *value)
{
  if (value->value_string)
    switch (value->value_string[0])
      {
      case 0:   // FIXME: use string_to_bool()
      case '0': case 'f': case 'F':
      case 'n': case 'N':               /* false assigments */
        return FALSE;
      default:
        return TRUE;
      }
  else
    return ABS (value->value_num) >= 0.5;
}
double
init_value_double (InitValue *value)
{
  if (value->value_string && value->value_string[0])
    return g_strtod (value->value_string, NULL);
  return value->value_num;
}
int64
init_value_int (InitValue *value)
{
  if (value->value_string && value->value_string[0])
    return strtoll (value->value_string, NULL, 0);
  return int64 (value->value_num + 0.5);
}
/* --- limits.h & float.h checks --- */
/* assert several assumptions the code makes */
BIRNET_STATIC_ASSERT (CHAR_BIT     == +8);
BIRNET_STATIC_ASSERT (SCHAR_MIN    == -128);
BIRNET_STATIC_ASSERT (SCHAR_MAX    == +127);
BIRNET_STATIC_ASSERT (UCHAR_MAX    == +255);
BIRNET_STATIC_ASSERT (SHRT_MIN     == -32768);
BIRNET_STATIC_ASSERT (SHRT_MAX     == +32767);
BIRNET_STATIC_ASSERT (USHRT_MAX    == +65535);
BIRNET_STATIC_ASSERT (INT_MIN      == -2147483647 - 1);
BIRNET_STATIC_ASSERT (INT_MAX      == +2147483647);
BIRNET_STATIC_ASSERT (UINT_MAX     == +4294967295U);
BIRNET_STATIC_ASSERT (INT64_MIN    == -9223372036854775807LL - 1);
BIRNET_STATIC_ASSERT (INT64_MAX    == +9223372036854775807LL);
BIRNET_STATIC_ASSERT (UINT64_MAX   == +18446744073709551615LLU);
BIRNET_STATIC_ASSERT (FLT_MIN      <= 1E-37);
BIRNET_STATIC_ASSERT (FLT_MAX      >= 1E+37);
BIRNET_STATIC_ASSERT (FLT_EPSILON  <= 1E-5);
BIRNET_STARTUP_ASSERT (DBL_MIN      <= 1E-37);
BIRNET_STARTUP_ASSERT (DBL_MAX      >= 1E+37);
BIRNET_STARTUP_ASSERT (DBL_EPSILON  <= 1E-9);
BIRNET_STATIC_ASSERT (LDBL_MIN     <= 1E-37);
BIRNET_STATIC_ASSERT (LDBL_MAX     >= 1E+37);
BIRNET_STATIC_ASSERT (LDBL_EPSILON <= 1E-9);

/* --- assertions/warnings/errors --- */
static void
stderr_print (bool        bail_out,
              const char *prefix,
              const char *domain,
              const char *file,
              int         line,
              const char *funcname,
              const char *pmsg,
              const char *str)
{
  fflush (stdout);
  String msg (bail_out ? "\n" : "");
  if (domain)
    msg += domain + String ("-") + prefix;
  else
    msg += prefix;
  if (file)
    {
      char buffer[64];
      sprintf (buffer, "%d", line);
      msg += String (":") + file + String (":") + String (buffer);
    }
  if (funcname)
    msg += String (":") + funcname + "()";
  if (pmsg)
    msg += String (": ") + pmsg;
  if (str)
    msg += String (": ") + str;
  msg += "\n";
  if (bail_out)
    msg += "aborting...\n";
  fputs (msg.c_str(), stderr);
  fflush (stderr);
}
void
birnet_runtime_problem (char        ewran_tag,
                        const char *domain,
                        const char *file,
                        int         line,
                        const char *funcname,
                        const char *msgformat,
                        ...)
{
  va_list args;
  va_start (args, msgformat);
  birnet_runtime_problemv (ewran_tag, domain, file, line, funcname, msgformat, args);
  va_end (args);
}
void
birnet_runtime_problemv (char        ewran_tag,
                         const char *domain,
                         const char *file,
                         int         line,
                         const char *funcname,
                         const char *msgformat,
                         va_list     args)
{
  const bool noreturn_case = ewran_tag == 'E' || ewran_tag == 'A' || ewran_tag == 'N';
  char *msg = NULL;
  if (msgformat && msgformat[0])
    msg = g_strdup_vprintf (msgformat, args);
  const char *prefix, *pmsg = NULL;
  switch (ewran_tag)
    {
    case 'E':
      prefix = "ERROR";
      break;
    case 'W':
      prefix = "WARNING";
      break;
    case 'R':
      prefix = "WARNING:";
      pmsg = "Check failed";
      break;
    case 'A':
      prefix = "ERROR";
      pmsg = "Assertion failed";
      break;
    default:
    case 'N':
      prefix = "ERROR";
      pmsg = "Assertion should not be reached";
      break;
    }
  stderr_print (noreturn_case, prefix, domain, file, line, funcname, pmsg, msg);
  g_free (msg);
  if (noreturn_case)
    {
      RAPICORN_BREAKPOINT();
      abort();
    }
}

/* --- url handling --- */
bool
url_test_show (const char *url)
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
  for (i = 0; i < G_N_ELEMENTS (www_browsers); i++)
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
        // FIXME: Msg::display (debug_browser, "show \"%s\": %s: %s", url, args[0], error ? error->message : fallback_error);
        g_clear_error (&error);
        if (success)
          return true;
        www_browsers[i].disabled = true;
      }
  /* reset all disabled states if no browser could be found */
  for (i = 0; i < G_N_ELEMENTS (www_browsers); i++)
    www_browsers[i].disabled = false;
  return false;
}
static void
browser_launch_warning (const char *url)
{
#if 0 // FIXME
  Msg::display (Msg::WARNING,
                Msg::Title (_("Launch Web Browser")),
                Msg::Text1 (_("Failed to launch a web browser executable")),
                Msg::Text2 (_("No suitable web browser executable could be found to be executed and to display the URL: %s"), url),
                Msg::Check (_("Show messages about web browser launch problems")));
#endif
}
void
url_show (const char *url)
{
  bool success = url_test_show (url);
  if (!success)
    browser_launch_warning (url);
}
static void
unlink_file_name (gpointer data)
{
  char *file_name = (char*) data;
  while (unlink (file_name) < 0 && errno == EINTR);
  g_free (file_name);
}
static const gchar*
url_create_redirect (const char    *url,
                     const char    *url_title,
                     const char    *cookie)
{
  const char *ver = "0.5";
  gchar *tname = NULL;
  gint fd = -1;
  while (fd < 0)
    {
      g_free (tname);
      tname = g_strdup_printf ("/tmp/Url%08X%04X.html", (int) lrand48(), getpid());
      fd = open (tname, O_WRONLY | O_CREAT | O_EXCL, 00600);
      if (fd < 0 && errno != EEXIST)
        {
          g_free (tname);
          return NULL;
        }
    }
  char *text = g_strdup_printf ("<!DOCTYPE HTML SYSTEM>\n"
                                "<html><head>\n"
                                "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n"
                                "<meta http-equiv=\"refresh\" content=\"0; URL=%s\">\n"
                                "<meta http-equiv=\"set-cookie\" content=\"%s\">\n"
                                "<title>%s</title>\n"
                                "</head><body>\n"
                                "<h1>%s</h1>\n"
                                "<b>Document Redirection</b><br>\n"
                                "Your browser is being redirected.\n"
                                "If it does not support automatic redirections, try <a href=\"%s\">%s</a>.\n"
                                "<hr>\n"
                                "<address>BirnetUrl/%s file redirect</address>\n"
                                "</body></html>\n",
                                url, cookie, url_title, url_title, url, url, ver);
  int w, c, l = strlen (text);
  do
    w = write (fd, text, l);
  while (w < 0 && errno == EINTR);
  g_free (text);
  do
    c = close (fd);
  while (c < 0 && errno == EINTR);
  if (w != l || c < 0)
    {
      while (unlink (tname) < 0 && errno == EINTR)
        {}
      g_free (tname);
      return NULL;
    }
  cleanup_add (60 * 1000, unlink_file_name, tname); /* free tname */
  return tname;
}
bool
url_test_show_with_cookie (const char *url,
                           const char *url_title,
                           const char *cookie)
{
  const char *redirect = url_create_redirect (url, url_title, cookie);
  if (redirect)
    return url_test_show (redirect);
  else
    return url_test_show (url);
}
void
url_show_with_cookie (const char *url,
                      const char *url_title,
                      const char *cookie)
{
  bool success = url_test_show_with_cookie (url, url_title, cookie);
  if (!success)
    browser_launch_warning (url);
}

} // Birnet
