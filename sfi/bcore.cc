// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bcore.hh"
#include <cstring>
#include "platform.hh"
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

void BSE_NORETURN
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
  case 'I':     prefix = "INFO: ";      break;
  case 'D':     prefix = "DEBUG: ";     break;
  case ' ':     prefix = "";            break;
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
