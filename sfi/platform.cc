// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "platform.hh"
#include "path.hh"
#include <unistd.h>
#if defined __APPLE__
#include <mach-o/dyld.h>        // _NSGetExecutablePath
#endif // __APPLE__
#ifdef  _WIN32                  // includes _WIN64
#include <windows.h>
#endif  // _WIN32
#include <cstring>
#include <sys/wait.h>
#include <sys/time.h>
#include <execinfo.h>           // _EXECINFO_H

namespace Bse {

// == Timestamps ==
static clockid_t monotonic_clockid = CLOCK_REALTIME;
static uint64    monotonic_start = 0;
static uint64    monotonic_resolution = 1000;   // assume 1µs resolution for gettimeofday fallback
static uint64    realtime_start = 0;

static void
timestamp_init_ ()
{
  static const bool initialize = [] () {
    realtime_start = timestamp_realtime();
    struct timespec tp = { 0, 0 };
    if (clock_getres (CLOCK_REALTIME, &tp) >= 0)
      monotonic_resolution = tp.tv_sec * 1000000000ULL + tp.tv_nsec;
    uint64 mstart = realtime_start;
#ifdef CLOCK_MONOTONIC
    // CLOCK_MONOTONIC_RAW cannot slew, but doesn't measure SI seconds accurately
    // CLOCK_MONOTONIC may slew, but attempts to accurately measure SI seconds
    if (monotonic_clockid == CLOCK_REALTIME && clock_getres (CLOCK_MONOTONIC, &tp) >= 0)
      {
        monotonic_clockid = CLOCK_MONOTONIC;
        monotonic_resolution = tp.tv_sec * 1000000000ULL + tp.tv_nsec;
        mstart = timestamp_benchmark(); // here, monotonic_start=0 still
      }
#endif
    monotonic_start = mstart;
    return true;
  } ();
  (void) initialize;
}
namespace { static struct Timestamper { Timestamper() { timestamp_init_(); } } realtime_startup; } // Anon

/// Provides the timestamp_realtime() value from program startup.
uint64
timestamp_startup ()
{
  timestamp_init_();
  return realtime_start;
}

/// Return the current time as uint64 in µseconds.
uint64
timestamp_realtime ()
{
  struct timespec tp = { 0, 0 };
  if (ISLIKELY (clock_gettime (CLOCK_REALTIME, &tp) >= 0))
    return tp.tv_sec * 1000000ULL + tp.tv_nsec / 1000;
  else
    {
      struct timeval now = { 0, 0 };
      gettimeofday (&now, NULL);
      return now.tv_sec * 1000000ULL + now.tv_usec;
    }
}

/// Provide resolution of timestamp_benchmark() in nano-seconds.
uint64
timestamp_resolution ()
{
  timestamp_init_();
  return monotonic_resolution;
}

/// Returns benchmark timestamp in nano-seconds, clock starts around program startup.
uint64
timestamp_benchmark ()
{
  struct timespec tp = { 0, 0 };
  uint64 stamp;
  if (ISLIKELY (clock_gettime (monotonic_clockid, &tp) >= 0))
    {
      stamp = tp.tv_sec * 1000000000ULL + tp.tv_nsec;
      stamp -= monotonic_start;                 // reduce number of significant bits
    }
  else
    {
      stamp = timestamp_realtime() * 1000;
      stamp -= MIN (stamp, monotonic_start);    // reduce number of significant bits
    }
  return stamp;
}

/// Convert @a stamp into a string, adding µsecond fractions if space permits.
String
timestamp_format (uint64 stamp, uint maxlength)
{
  const size_t fieldwidth = std::max (maxlength, 1U);
  const String fsecs = string_format ("%u", size_t (stamp) / 1000000);
  const String usecs = string_format ("%06u", size_t (stamp) % 1000000);
  String r = fsecs;
  if (r.size() < fieldwidth)
    r += '.';
  if (r.size() < fieldwidth)
    r += usecs.substr (0, fieldwidth - r.size());
  return r;
}

/// A monotonically increasing counter, increments are atomic and visible in all threads.
uint64
monotonic_counter ()
{
  static std::atomic<uint64> global_monotonic_counter { 4294967297 };
  return global_monotonic_counter++;
}

// == program and executable names ==
static std::string
get_executable_path()
{
  const ssize_t max_size = 4096;
  char system_result[max_size + 1] = { 0, };
  ssize_t system_result_size = -1;

#if defined __linux__ || defined __CYGWIN__ || defined __MSYS__
  system_result_size = readlink ("/proc/self/exe", system_result, max_size);
#elif defined __APPLE__
  {
    uint32_t bufsize = max_size;
    if (_NSGetExecutablePath (system_result, &bufsize) == 0)
      system_result_size = bufsize;
  }
#elif defined _WIN32
  unsigned long bufsize = max_size;
  system_result_size = GetModuleFileNameA (0, system_result, bufsize);
  if (system_result_size == 0 || system_result_size == bufsize)
    system_result_size = -1;    /* Error, possibly not enough space. */
  else
    {
      system_result[system_result_size] = '\0';
      /* Early conversion to unix slashes instead of more changes everywhere else .. */
      char *winslash;
      while ((winslash = strchr (system_result, '\\')) != NULL)
        *winslash = '/';
    }
#else
#warning "Platform lacks executable_path() implementation"
#endif

  if (system_result_size != -1)
    return std::string (system_result);
  return std::string();
}

std::string
executable_path()
{
  static std::string cached_executable_path = get_executable_path();
  return cached_executable_path;
}

std::string
executable_name()
{
  static std::string cached_executable_name = [] () {
    std::string path = executable_path();
    const char *slash = strrchr (path.c_str(), '/');
    return slash ? slash + 1 : path;
  } ();
  return cached_executable_name;
}

static String cached_program_alias;

String
program_alias ()
{
  return cached_program_alias.empty() ? executable_name() : cached_program_alias;
}

void
program_alias_init (String customname)
{
  assert_return (cached_program_alias.empty() == true);
  cached_program_alias = customname;
}

static String cached_application_name;

String
application_name ()
{
  return cached_application_name.empty() ? program_alias() : cached_application_name;
}

void
application_name_init (String desktopname)
{
  assert_return (cached_application_name.empty() == true);
  cached_application_name = desktopname;
}

String
program_cwd ()
{
  static String cached_program_cwd = Path::cwd();
  return cached_program_cwd;
}

// == Early Startup ctors ==
namespace {
struct EarlyStartup {
  EarlyStartup()
  {
    timestamp_init_();
    program_cwd(); // initialize early, i.e. before main() changes cwd
  }
};
static EarlyStartup _early_startup __attribute__ ((init_priority (101)));
} // Anon

// == backtraces ==
#ifdef _EXECINFO_H
int (*backtrace_pointers) (void **buffer, int size) = &::backtrace; // GLibc only
#else  // !_EXECINFO_H
static int
dummy_backtrace (void **buffer, int size)
{
  if (size)
    {
      buffer[0] = __builtin_return_address (0);
      return 1;
    }
  return 0;
}
int (*backtrace_pointers) (void **buffer, int size) = &dummy_backtrace;
#endif // !_EXECINFO_H

} // Bse
