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

namespace Bse {

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

struct EarlyStartup102 {
  EarlyStartup102()
  {
    program_cwd(); // initialize early, i.e. before main() changes cwd
  }
};

static EarlyStartup102 _early_startup_102 __attribute__ ((init_priority (102)));

} // Bse
