// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bcore.hh"
#include "platform.hh"
#include <unistd.h>     // _exit
#include <sys/time.h>   // gettimeofday

namespace Bse {
using namespace Rapicorn;


namespace Internal {

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

void RAPICORN_NORETURN
force_abort ()
{
  // ensure the program halts on error conditions
  abort();
  _exit (-1);
}

void
diagnostic (char kind, const std::string &message)
{
  const char buf[2] = { kind, 0 };
  String prefix;
  switch (kind) {
  case 'W':     prefix = "WARNING: ";   break;
  case 'D':     prefix = "DEBUG: ";     break;
  case ' ':     prefix = "";            break;
  case 'I':
    prefix = program_alias() + ": ";
    break;
  case 'F':
    prefix = program_alias() + ": FATAL: ";
    break;
  default:
    prefix = program_alias() + ": " + buf + ": ";
    break;
  }
  const char *const newline = !message.empty() && message.data()[message.size() - 1] == '\n' ? "" : "\n";
  printerr ("%s%s%s", prefix, message, newline);
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

} // Internal

} // Bse
