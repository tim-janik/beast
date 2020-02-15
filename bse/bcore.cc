// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bcore.hh"
#include "platform.hh"
#include "bse/internal.hh"
#include <cstring>
#include <unistd.h>     // _exit
#include <sys/time.h>   // gettimeofday
#include <fcntl.h>      // open()

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

// == Event Loop ==
GMainContext *bse_main_context = NULL; // initialized by bse_init_intern()

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

/// Check if `feature` is enabled via $BSE_FEATURE.
bool
feature_check (const char *feature)
{
  const char *const bsefeature = getenv ("BSE_FEATURE");
  return bsefeature ? feature_toggle_bool (bsefeature, feature) : false;
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

// == GDB Backtrace ==
BacktraceCommand::BacktraceCommand()
{}

static const char *const usr_bin_gdb = "/usr/bin/gdb";
static const char *const ptrace_scope = "/proc/sys/kernel/yama/ptrace_scope";

/// Check /proc/sys/kernel/yama/ptrace_scope for working ptrace().
static bool
backtrace_may_ptrace()
{
  bool allow_ptrace = false;
#ifdef  __linux__
  int fd = open (ptrace_scope, 0);
  char b[8] = { 0 };
  if (read (fd, b, 8) > 0)
    allow_ptrace = b[0] == '0';
  close (fd);
#else
  allow_ptrace = true;
#endif
  return allow_ptrace;
}

/// Check for executable /usr/bin/gdb
static bool
backtrace_have_gdb()
{
  return access (usr_bin_gdb, X_OK) == 0;
}

bool
BacktraceCommand::can_backtrace ()
{
  return backtrace_may_ptrace() && backtrace_have_gdb();
}

const char*
BacktraceCommand::command ()
{
  txtbuf_[0] = 0;
  if (can_backtrace())
    snprintf (txtbuf_, tlen_,
              "%s -q -n -p %u --batch "
              "-iex 'set auto-load python-scripts off' "
              "-iex 'set script-extension off' "
              "-ex 'set print address off' "
              // "-ex 'set print frame-arguments none' "
              "-ex 'thread apply all backtrace 25' >&2 2>/dev/null",
              usr_bin_gdb, this_thread_gettid());
  return txtbuf_;
}

const char*
BacktraceCommand::message ()
{
  txtbuf_[0] = 0;
  if (!backtrace_have_gdb())
    {
      strncat (txtbuf_, "Backtrace requires a debugger, e.g.: ", tlen_);
      strncat (txtbuf_, usr_bin_gdb, tlen_);
      strncat (txtbuf_, "\n", tlen_);
    }
  else if (!backtrace_may_ptrace())
    {
      strncat (txtbuf_, "Backtrace needs ptrace permissions, ", tlen_);
      strncat (txtbuf_, "try: echo 0 > /proc/sys/kernel/yama/ptrace_scope\n", tlen_);
    }
  return txtbuf_;
}

///< Heading to print before backtrace.
const char*
BacktraceCommand::heading (const char *const file, const int line, const char *const func,
                           const char *prefix, const char *postfix)
{
  txtbuf_[0] = 0;
  strncat (txtbuf_, prefix, tlen_);
  int l = strlen (txtbuf_);
  snprintf (txtbuf_ + l, tlen_ - l, "Backtrace[%u]", getpid());
  if (!file)
    {
      strncat (txtbuf_, ":", tlen_);
      strncat (txtbuf_, postfix, tlen_);
      strncat (txtbuf_, "\n", tlen_);
      return txtbuf_;
    }
  strncat (txtbuf_, " from ", tlen_);
  strncat (txtbuf_, file, tlen_);
  strncat (txtbuf_, ":", tlen_);
  if (line > 0)
    {
      l = strlen (txtbuf_);
      snprintf (txtbuf_ + l, tlen_ - l, "%d:", line);
    }
  if (func)
    {
      strncat (txtbuf_, func, tlen_);
      strncat (txtbuf_, "():", tlen_);
    }
  strncat (txtbuf_, postfix, tlen_);
  strncat (txtbuf_, "\n", tlen_);
  return txtbuf_;
}

// == Diagnostics ==
static ::std::string
diag_format (bool with_executable, const char *file, int line, const char *func, char kind, const std::string &info, bool will_abort = false)
{
  std::string executable = with_executable ? executable_path() : "";
  std::string sout;
  using namespace AnsiColors;
  bool need_reset = true;
  if (kind == 'F' or will_abort)
    sout += color (BG_RED, FG_WHITE, BOLD);
  else if (kind == 'W')
    sout += color (FG_YELLOW);
  else if (kind == 'A' or kind == 'E')
    sout += color (FG_RED, BOLD);
  else
    need_reset = false;
  if (!executable.empty())
    {
      // Fixup *stupid* executable names so errors stay parsable
      size_t sep = executable.find (" ");
      if (sep != std::string::npos)
        executable = executable.substr (0, sep);        // strips electron command line args
      sep = executable.rfind ("/");
      if (sep != std::string::npos)
        executable = executable.substr (sep + 1);       // use simple basename
      if (!executable.empty())
        sout += executable + ": ";
    }
  if (file && file[0])
    {
      sout += file;
      if (line > 0)
        sout += string_format (":%u", line);
      sout += ": ";
    }
  if (func && func[0])
    {
      sout += func;
      sout += ": ";
    }
  switch (kind) {
  case 'A':     sout += "assertion failed: "; break;
  case 'F':     sout += "fatal: ";      break;
  case 'W':     sout += "warning: ";    break;
  case 'E':
    if (will_abort)
      sout += "fatal-error: ";
    else
      sout += "error: ";
    break;
  default: ;
  }
  const bool need_space = sout.size() && sout[sout.size() - 1] == ' ';
  if (need_reset)
    sout = string_rstrip (sout) + color (RESET) + (need_space ? " " : "");
  sout += info;
  if (!sout.empty() && sout[sout.size() - 1] != '\n')
    sout += "\n";
  return sout;
}

/// Check if `conditional` is enabled by $BSE_DEBUG.
bool
debug_key_enabled (const char *conditional)
{
  const std::string value = debug_key_value (conditional);
  return !value.empty() && (strchr ("123456789yYtT", value[0]) || strncasecmp (value.c_str(), "on", 2) == 0);
}

/// Check if `conditional` is enabled by $BSE_DEBUG.
bool
debug_key_enabled (const ::std::string &conditional)
{
  return debug_key_enabled (conditional.c_str());
}

bool Internal::debug_enabled_flag = true;

static uint64 global_debug_flags = 0;

void
set_debug_flags (DebugFlags flags)
{
  global_debug_flags = global_debug_flags | flags;
}

/// Retrieve the value assigned to debug key `conditional` in $BSE_DEBUG.
::std::string
debug_key_value (const char *conditional)
{
  // cache $BSE_DEBUG and setup debug_any_enabled;
  static const std::string debug_flags = [] () {
    const char *f = getenv ("BSE_DEBUG");
    const std::string flags = !f ? "" : ":" + std::string (f) + ":";
    Internal::debug_enabled_flag = !flags.empty() && flags != ":none:";
    const ssize_t fw = flags.rfind (":fatal-warnings:");
    const ssize_t nf = flags.rfind (":no-fatal-warnings:");
    if (fw >= 0 && nf <= fw)
      global_debug_flags = global_debug_flags | Bse::DebugFlags::FATAL_WARNINGS;
    const ssize_t sq = flags.rfind (":sigquit-on-abort:");
    const ssize_t nq = flags.rfind (":no-sigquit-on-abort:");
    if (sq >= 0 && nq <= sq)
      global_debug_flags = global_debug_flags | Bse::DebugFlags::SIGQUIT_ON_ABORT;
    return flags;
  } ();
  // find key in colon-separated debug flags
  const ::std::string key = conditional ? conditional : "";
  static const std::string all = ":all:", none = ":none:";
  const std::string condr = ":no-" + key + ":";
  const std::string condc = ":" + key + ":";
  const std::string conde = ":" + key + "=";
  const ssize_t pa = debug_flags.rfind (all);
  const ssize_t pn = debug_flags.rfind (none);
  const ssize_t pr = debug_flags.rfind (condr);
  const ssize_t pc = debug_flags.rfind (condc);
  const ssize_t pe = debug_flags.rfind (conde);
  const ssize_t pmax = std::max (pr, std::max (std::max (pa, pn), std::max (pc, pe)));
  if (pn == pmax || pr == pmax)
    return "false";     // found no key or ':none:' or ':no-key:'
  if (pa == pmax || pc == pmax)
    return "true";      // last setting is ':key:' or ':all:'
  // pe == pmax, assignment via equal sign
  const ssize_t pv = pe + conde.size();
  const ssize_t pw = debug_flags.find (":", pv);
  const std::string value = debug_flags.substr (pv, pw < 0 ? pw : pw - pv);
  return value;
}

void
diag_info (const ::std::string &message)
{
  diag_printerr (diag_format (true, NULL, 0, NULL, 'I', message));
}

void
diag_printout (const ::std::string &message)
{
  // some platforms (_WIN32) don't properly flush on '\n'
  fflush (stderr); // preserve ordering
  fputs (message.c_str(), stdout);
  fflush (stdout);
}

void
diag_printerr (const ::std::string &message)
{
  // some platforms (_WIN32) don't properly flush on '\n'
  fflush (stdout); // preserve ordering
  fputs (message.c_str(), stderr);
  fflush (stderr);
}

static std::function<void (const ::std::string&)> global_abort_hook;

/// Call `hook` for fatal_error() and diag_failed_assert().
void
diag_abort_hook (const std::function<void (const ::std::string&)> &hook)
{
  global_abort_hook = hook;
}

void
diag_debug_message (const char *file, int line, const char *func, const char *cond, const ::std::string &message)
{
  if (!cond || debug_key_enabled (cond))
    {
      struct timeval tv = { 0, };
      gettimeofday (&tv, NULL);
      const char *const newline = !message.empty() && message.data()[message.size() - 1] == '\n' ? "" : "\n";
      using namespace AnsiColors;
      const std::string col = color (FG_CYAN, BOLD), reset = color (RESET);
      const std::string ul = cond ? color (UNDERLINE) : "", nl = cond ? color (UNDERLINE_OFF) : "";
      std::string sout;
      sout += string_format ("%s%u.%06u ", col, tv.tv_sec, tv.tv_usec); // cyan timestamp
      sout += string_format ("%s%s%s:", ul, cond ? cond : executable_name().c_str(), nl); // underlined cond
      sout += string_format ("%s %s", reset, message); // normal print message
      printerr ("%s%s", sout, newline);
    }
}

#ifndef NDEBUG
#define PRINT_BACKTRACE(file, line, func)                          do { \
  using namespace AnsiColors;                                           \
  const std::string col = color (FG_YELLOW /*, BOLD*/), reset = color (RESET); \
  BacktraceCommand btrace;                                              \
  bool btrace_ok = false;                                               \
  if (btrace.can_backtrace()) {                                         \
    const char *heading = btrace.heading (file, line, func, col.c_str(), reset.c_str()); \
    diag_printerr (heading);                                            \
    const char *btrace_cmd = btrace.command();                          \
    btrace_ok = btrace_cmd[0] && system (btrace_cmd) == 0;              \
  }                                                                     \
  const char *btrace_msg = btrace.message();                            \
  if (!btrace_ok && btrace_msg[0])                                      \
    diag_printerr (btrace_msg); /* print bt errors */                   \
  } while (0)
#else // NDEBUG
#define PRINT_BACKTRACE(file, line, func)               do { } while (0)
#endif

// Mimick relevant parts of glibc's abort_msg_s
struct AbortMsg {
  const char *msg = NULL;
};
static AbortMsg abort_msg;

#define ABORT_WITH_MESSAGE(abort_message, file, line, func)        do { \
  Bse::diag_printerr (abort_message);                                   \
  __sync_synchronize();                                                 \
  if (Bse::global_abort_hook)                                           \
    Bse::global_abort_hook (abort_message);                             \
  Bse::abort_msg.msg = abort_message.c_str();                           \
  __sync_synchronize();                                                 \
  PRINT_BACKTRACE (file, line, func);                                   \
  if (Bse::global_debug_flags & Bse::DebugFlags::SIGQUIT_ON_ABORT)      \
    raise (SIGQUIT);                                                    \
  ::abort();   /* default action for SIGABRT is core dump */            \
  ::_exit (-1);  /* ensure noreturn */                                  \
} while (0)

void
assertion_failed (const std::string &msg, const char *const file, const int line, const char *const func)
{
  const ::std::string abort_message = diag_format (true, file, line, func, 'A', msg.empty() ? "state unreachable" : msg);
#ifdef NDEBUG
  if (!(global_debug_flags & Bse::DebugFlags::FATAL_WARNINGS))
    {
      diag_printerr (abort_message);
      return;
    }
#endif
  ABORT_WITH_MESSAGE (abort_message, file, line, func);
}

void
diag_failed_assert (const char *file, int line, const char *func, const char *stmt)
{
  const ::std::string abort_message = diag_format (true, file, line, func, 'A', stmt ? stmt : "state unreachable");
  if (global_debug_flags & Bse::DebugFlags::FATAL_WARNINGS)
    ABORT_WITH_MESSAGE (abort_message, file, line, func);
  diag_printerr (abort_message);
}

void
diag_warning (const ::std::string &message)
{
  const ::std::string msg = diag_format (true, NULL, 0, NULL, 'W', message);
  if (global_debug_flags & Bse::DebugFlags::FATAL_WARNINGS)
    ABORT_WITH_MESSAGE (msg, NULL, 0, NULL);
  diag_printerr (diag_format (true, NULL, 0, NULL, 'W', message));
}

void
diag_fatal_error (const ::std::string &message)
{
  const ::std::string abort_message = diag_format (true, NULL, 0, NULL, 'E', message);
  ABORT_WITH_MESSAGE (abort_message, NULL, 0, NULL);
}

struct EarlyStartup101 {
  EarlyStartup101()
  {
    if (debug_key_enabled ("") ||       // force debug_any_enabled initialization
        debug_enabled())                // print startup time if *any* debugging is enabled
      {
        const time_t now = time (NULL);
        struct tm gtm = { 0, };
        gmtime_r (&now, &gtm);
        char buffer[1024] = { 0, };
        strftime (buffer, sizeof (buffer) - 1, "%Y-%m-%d %H:%M:%S UTC", &gtm);
        diag_debug_message (NULL, 0, NULL, "startup", ::std::string (buffer));
      }
  }
};

static EarlyStartup101 _early_startup_101 __attribute__ ((init_priority (101)));

} // Bse

// == Testing ==
#include "testing.hh"
namespace { // Anon
using namespace Bse;

BSE_INTEGRITY_TEST (test_feature_toggles);
static void
test_feature_toggles()
{
  String r;
  r = feature_toggle_find ("a:b", "a"); TCMP (r, ==, "1");
  r = feature_toggle_find ("a:b", "b"); TCMP (r, ==, "1");
  r = feature_toggle_find ("a:b", "c"); TCMP (r, ==, "0");
  r = feature_toggle_find ("a:b", "c", "7"); TCMP (r, ==, "7");
  r = feature_toggle_find ("a:no-b", "b"); TCMP (r, ==, "0");
  r = feature_toggle_find ("no-a:b", "a"); TCMP (r, ==, "0");
  r = feature_toggle_find ("no-a:b:a", "a"); TCMP (r, ==, "1");
  r = feature_toggle_find ("no-a:b:a=5", "a"); TCMP (r, ==, "5");
  r = feature_toggle_find ("no-a:b:a=5:c", "a"); TCMP (r, ==, "5");
  bool b;
  b = feature_toggle_bool ("", "a"); TCMP (b, ==, false);
  b = feature_toggle_bool ("a:b:c", "a"); TCMP (b, ==, true);
  b = feature_toggle_bool ("no-a:b:c", "a"); TCMP (b, ==, false);
  b = feature_toggle_bool ("no-a:b:a=5:c", "b"); TCMP (b, ==, true);
  b = feature_toggle_bool ("x", ""); TCMP (b, ==, true); // *any* feature?
}

} // Anon

// == aidacc/aida.cc ==
static void
aida_diagnostic_impl (const char *file, int line, const char *func, char kind, const char *msg, bool will_abort)
{
  if (kind == 'D' and not will_abort)
    {
      Bse::diag_debug_message (file, line, func, "aida", msg);
      return;
    }
  const ::std::string diag_message = Bse::diag_format (true, file, line, func, kind, msg);
  if (kind != 'D' && Bse::global_debug_flags & Bse::DebugFlags::FATAL_WARNINGS)
    ABORT_WITH_MESSAGE (diag_message, file, line, func);
  Bse::diag_printerr (diag_message);
}

#define AIDA_DEFER_GARBAGE_COLLECTION(msecs, func, data)        aida_defer_handler (msecs, func, data)
#define AIDA_DEFER_GARBAGE_COLLECTION_CANCEL(id)                g_source_remove (id)
#define AIDA_DIAGNOSTIC_IMPL(file, line, func, kind, message, will_abort) \
  aida_diagnostic_impl (file, line, func, kind, message, will_abort)
#include "aidacc/aida.cc"

// == __abort_msg ==
::Bse::AbortMsg *bse_abort_msg = &::Bse::abort_msg;
#ifdef  __ELF__
// allow 'print __abort_msg->msg' when debugging core files for apport/gdb to pick up
extern "C" ::Bse::AbortMsg *__abort_msg __attribute__ ((weak, alias ("bse_abort_msg")));
#endif // __ELF__
