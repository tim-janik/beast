// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "storage.hh"
#include "internal.hh"
#include "minizip.h"
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

/// Retrieve (or create) the temporary cache directory for this runtime.
std::string
beastbse_cachedir_current ()
{
  static std::string current_cachedir = beastbse_cachedir_create();
  if (current_cachedir.empty())
    fatal_error ("failed to create temporary cache directory: %s", strerror (errno));
  return current_cachedir;
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

Storage::Storage () :
  impl_ (std::make_shared<Storage::Impl>())
{}

Storage::~Storage ()
{}

class Storage::Impl {
  String tmpdir_;
  std::vector<String> members_;
  String
  tmpdir ()
  {
    if (tmpdir_.empty())
      tmpdir_ = beastbse_cachedir_create();
    return tmpdir_;
  }
public:
  ~Impl()
  {
    if (!tmpdir_.empty())
      {
        rmrf_dir (tmpdir_);
        std::lock_guard<std::mutex> locker (cachedirs_mutex);
        auto it = std::find (cachedirs_list.begin(), cachedirs_list.end(), tmpdir_);
        if (it != cachedirs_list.end())
          cachedirs_list.erase (it);
      }
  }
  bool
  rm_file (const String &filename)
  {
    errno = ENOENT;
    assert_return (!Path::isabs (filename), -1);
    printerr ("%s: rm=%s first=%s\n", __func__, filename,
              members_.size() ? members_[0] : "");
    if (!tmpdir_.empty())
      {
        std::error_code ec;
        std::filesystem::remove (tmpdir_ + "/" + filename, ec);
        auto it = std::find (members_.begin(), members_.end(), filename);
        if (it != members_.end())
          members_.erase (it);
        return !ec;
      }
    return false;
  }
  int
  store_file_fd (const String &filename)
  {
    errno = EINVAL;
    assert_return (!Path::isabs (filename), -1);
    rm_file (filename);
    const int fd = open ((tmpdir() + "/" + filename).c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd >= 0)
      {
        // keep mimetype as first member for 'file(1)'
        if ("mimetype" == filename)
          members_.insert (members_.begin(), filename);
        else
          members_.push_back (filename);
      }
    return fd;
  }
  bool
  store_file_buffer (const String &filename, const String &data, int64_t epoch_seconds)
  {
    const int fd = store_file_fd (filename);
    if (fd >= 0)
      {
        long l;
        do
          l = write (fd, data.data(), data.size());
        while (l < 0 && errno == EINTR);
        if (l >= 0)
          {
            struct timespec times[2] = { { 0, 0, }, { 0, 0, }, };
            times[0].tv_sec = epoch_seconds ? epoch_seconds : time (NULL);
            times[1].tv_sec = times[0].tv_sec;
            if (futimens (fd, times) < 0)
              SDEBUG ("Bse::Storage::%s: %s: futimens: %s\n", __func__, filename, strerror (errno));
            if (close (fd) >= 0)
              return true;
          }
      }
    const int saved = errno;
    close (fd);
    rm_file (filename);
    errno = saved;
    return false;
  }
  bool
  set_mimetype_bse ()
  {
    const int64_t bse_project_start = 844503964;
    return store_file_buffer ("mimetype", "application/x-bse", bse_project_start);
  }
  bool
  export_as (const String &filename)
  {
    void *writer = NULL;
    mz_zip_writer_create (&writer);
    mz_zip_writer_set_zip_cd (writer, false);
    mz_zip_writer_set_password (writer, NULL);
    mz_zip_writer_set_store_links (writer, false);
    mz_zip_writer_set_follow_links (writer, true);
    mz_zip_writer_set_compress_level (writer, MZ_COMPRESS_LEVEL_BEST);
    mz_zip_writer_set_compress_method (writer, MZ_COMPRESS_METHOD_DEFLATE);
    mz_zip_file file_info = { 0, };
    file_info.version_madeby = BSE_MZ_VERSION_MADEBY; // MZ_VERSION_MADEBY_ZIP_VERSION;
    file_info.zip64 = MZ_ZIP64_DISABLE;  // match limagic's ZIP-with-mimetype
    file_info.flag = 0; // MZ_ZIP_FLAG_UTF8;
    file_info.compression_method = MZ_COMPRESS_METHOD_DEFLATE;
    int err = mz_zip_writer_open_file (writer, filename.c_str(), 0, false);
    for (auto it = members_.begin(); err == MZ_OK && it != members_.end(); ++it)
      {
        const std::string fname = *it;
        const bool plain = it == members_.begin() && fname == "mimetype";
        if (plain)
          mz_zip_writer_set_compress_method (writer, MZ_COMPRESS_METHOD_STORE);
        err = mz_zip_writer_add_file (writer, (tmpdir() + "/" + fname).c_str(), fname.c_str());
        if (plain)
          mz_zip_writer_set_compress_method (writer, MZ_COMPRESS_METHOD_DEFLATE);
      }
    if (err == MZ_OK)
      err = mz_zip_writer_close (writer);
    if (err != MZ_OK)
      unlink (filename.c_str());
    mz_zip_writer_delete (&writer);
    return err == MZ_OK;
  }
  int
  open_r (const String &filename)
  {
    errno = EINVAL;
    assert_return (!Path::isabs (filename), -1);
    const int fd = open ((tmpdir() + "/" + filename).c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    return fd;
  }
  String
  fetch_file (const String &filename)
  {
    if (tmpdir_.empty())
      return "";
    // FIXME: extract
    return tmpdir_ + "/" + filename;
  }
  String
  fetch_file_buffer (const String &filename, ssize_t maxlength)
  {
    errno = ENOENT;
    assert_return (!Path::isabs (filename), "");
    if (tmpdir_.empty())
      return "";
    // FIXME: extract
    return Path::stringread (tmpdir_ + "/" + filename, maxlength);
  }
  bool     import_from (const String &filename) { return false; }
};

int      Storage::store_file_fd     (const String &filename)    { return impl_->store_file_fd (filename); }
bool     Storage::store_file_buffer (const String &filename, const String &buffer, int64_t epoch_seconds)
{ return impl_->store_file_buffer (filename, buffer, epoch_seconds); }
bool     Storage::rm_file           (const String &filename)    { return impl_->rm_file (filename); }
bool     Storage::set_mimetype_bse  ()                          { return impl_->set_mimetype_bse(); }
bool     Storage::export_as         (const String &filename)    { return impl_->export_as (filename); }
bool     Storage::import_from       (const String &filename)    { return impl_->import_from (filename); }
String   Storage::fetch_file_buffer (const String &filename, ssize_t maxlength)
{ return impl_->fetch_file_buffer (filename, maxlength); }
String   Storage::fetch_file        (const String &filename)    { return impl_->fetch_file (filename); }

} // Bse
