// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "storage.hh"
#include "internal.hh"
#include <stdlib.h>     // mkdtemp
#include <sys/stat.h>   // mkdir
#include <unistd.h>     // rmdir
#include <fcntl.h>      // O_EXCL
#include <filesystem>

#define SDEBUG(...)     Bse::debug ("storage", __VA_ARGS__)

namespace Bse {

/// Create process specific string for a `.pid` guard file.
static std::string
pid_string (int pid)
{
  static std::string boot_id = []() {
    auto id = string_strip (Path::stringread ("/proc/sys/kernel/random/boot_id"));
    if (id.empty())
      id = string_strip (Path::stringread ("/etc/machine-id"));
    if (id.empty())
      id = string_format ("%08x", gethostid());
    return id;
  } ();
  std::string text = string_format ("%u %s ", pid, boot_id);
  std::string exename;
  if (exename.empty() && Path::check ("/proc/self/exe", "r"))
    {
      const ssize_t max_size = 8100;
      char exepath[max_size + 1 + 1] = { 0, };
      ssize_t exepath_size = -1;
      exepath_size = readlink (string_format ("/proc/%u/exe", pid).c_str(), exepath, max_size);
      if (exepath_size > 0)
        exename = exepath;
    }
  else if (exename.empty() && Path::check ("/proc/self/comm", "r"))
    exename = Path::stringread (string_format ("/proc/%u/comm", pid));
  else
    {
      if (getpgid (pid) >= 0 || errno != ESRCH)
        exename = string_format ("%u", pid); // assume process `pid` exists
    }
  text += exename;
  text += "\n";
  return text;
}

/// Prefix for temporary cache directories, also used for pruning of stale directories.
static std::string
tmpdir_prefix ()
{
  return string_format ("beastbse-%x", getuid());
}

/// Find base directory for the creation of temporary caches.
static std::string
beastbse_cachedir_base (bool createbase = false)
{
  // try ~/.cache/beast/
  std::string basedir = Bse::Path::cache_home() + "/beast";
  if (Bse::Path::check (basedir, "dw"))
    return basedir;
  else if (createbase) // !Bse::Path::check (basedir, "dw")
    {
      int err = mkdir (basedir.c_str(), 0700);
      SDEBUG ("mkdir: %s: %s", basedir, strerror (err ? errno : 0));
      if (Bse::Path::check (basedir, "dw"))
        return basedir;
    }
  // try /tmp/
  basedir = std::filesystem::temp_directory_path().string();
  if (Bse::Path::check (basedir, "dw")) // sets errno
    return basedir;
  return "";
}

/// Recursively delete directory tree.
static void
rmrf_dir (const std::string &dir)
{
  std::error_code ec;
  std::filesystem::remove_all (dir, ec);
  SDEBUG ("rm-rf: %s: %s", dir, ec.message());
}

static std::vector<std::string> cachedirs_list;
static std::mutex               cachedirs_mutex;

/// Clean temporary caches of this process.
static void
atexit_clean_cachedirs()
{
  std::lock_guard<std::mutex> locker (cachedirs_mutex);
  while (cachedirs_list.size())
    {
      const std::string dir = cachedirs_list.back();
      cachedirs_list.pop_back();
      rmrf_dir (dir);
    }
}

/// Create exclusive cache directory for this process' runtime.
std::string
beastbse_cachedir_create()
{
  std::string cachedir = beastbse_cachedir_base (true); // sets errno
  if (cachedir.empty())
    return "";
  cachedir += "/" + tmpdir_prefix() + "XXXXXX";
  char *tmpchars = cachedir.data();
  char *result = mkdtemp (tmpchars);
  SDEBUG ("mkdtemp: %s: %s", tmpchars, strerror (result ? 0 : errno));
  if (result)
    {
      std::string guardfile = cachedir + "/guard.pid";
      std::string guardstring = pid_string (getpid());
      const int guardfd = open (guardfile.c_str(), O_EXCL | O_CREAT, 0600);
      if (guardfd >= 0 && Path::stringwrite (guardfile, guardstring))
        {
          close (guardfd);
          std::lock_guard<std::mutex> locker (cachedirs_mutex);
          cachedirs_list.push_back (cachedir);
          static bool needatexit = true;
          if (needatexit)
            needatexit = std::atexit (atexit_clean_cachedirs);
          SDEBUG ("create: %s: %s", guardfile, strerror (0));
          return cachedir;
        }
      SDEBUG ("create: %s: %s", guardfile, strerror (errno));
      const int err = errno;
      close (guardfd);
      rmrf_dir (cachedir);
      errno = err;
    }
  return ""; // errno is set
}

/// Clean stale cache directories from past runtimes, may be called from any thread.
void
beastbse_cachedir_cleanup()
{
  const std::string cachedir = beastbse_cachedir_base (false);
  const std::string tmpprefix = tmpdir_prefix();
  if (!cachedir.empty())
    for (auto &direntry : std::filesystem::directory_iterator (cachedir))
      if (direntry.is_directory())
        {
          const std::string dirname = direntry.path().filename();
          if (dirname.size() == tmpprefix.size() + 6 && string_startswith (dirname, tmpprefix))
            {
              const std::string guardfile = direntry / "guard.pid";
              if (Path::check (guardfile, "frw"))
                {
                  std::string guardstring = Path::stringread (guardfile, 3 * 4096);
                  const int guardpid = string_to_int (guardstring);
                  if (guardpid > 0 && guardstring == pid_string (guardpid))
                    {
                      SDEBUG ("skip: %s", guardfile);
                      continue;
                    }
                  rmrf_dir (direntry.path().string());
                }
            }
        }
}
} // Bse
