// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "path.hh"
#include "platform.hh"
#include <unistd.h>     // getuid
#include <sys/stat.h>   // lstat
#include <cstring>      // strchr
#include <mutex>

#define IS_DIRCHAR(c)                   ((c) == BSE_DIRCHAR || (c) == BSE_DIRCHAR2)
#define IS_SEARCHPATH_SEPARATOR(c)      ((c) == BSE_SEARCHPATH_SEPARATOR || (c) == ';') // make ';' work under Windows and Unix

// == CxxPasswd =
namespace { // Anon
struct CxxPasswd {
  std::string pw_name, pw_passwd, pw_gecos, pw_dir, pw_shell;
  uid_t pw_uid;
  gid_t pw_gid;
  CxxPasswd (std::string username = "");
};
} // Anon

namespace Bse {

namespace Path {

/// Retrieve the directory part of the filename @ path.
String
dirname (const String &path)
{
  char *gdir = g_path_get_dirname (path.c_str());
  String dname = gdir;
  g_free (gdir);
  return dname;
}

/// Strips all directory components from @a path and returns the resulting file name.
String
basename (const String &path)
{
  char *gbase = g_path_get_basename (path.c_str());
  String bname = gbase;
  g_free (gbase);
  return bname;
}

/// Resolve links and directory references in @a path and provide a canonicalized absolute pathname.
String
realpath (const String &path)
{
  char *const cpath = ::realpath (path.c_str(), NULL);
  if (cpath)
    {
      const String result = cpath;
      free (cpath);
      errno = 0;
      return result;
    }
  // error case
  return path;
}

/**
 * @param path  a filename path
 * @param incwd optional current working directory
 *
 * Complete @a path to become an absolute file path. If neccessary, @a incwd or
 * the real current working directory is prepended.
 */
String
abspath (const String &path, const String &incwd)
{
  if (isabs (path))
    return path;
  if (!incwd.empty())
    return abspath (join (incwd, path), "");
  String pcwd = program_cwd();
  if (!pcwd.empty())
    return join (pcwd, path);
  return join (cwd(), path);
}

/// Return wether @a path is an absolute pathname.
bool
isabs (const String &path)
{
  return g_path_is_absolute (path.c_str());
}

/// Return wether @a path is pointing to a directory component.
bool
isdirname (const String &path)
{
  uint l = path.size();
  if (path == "." || path == "..")
    return true;
  if (l >= 1 && IS_DIRCHAR (path[l-1]))
    return true;
  if (l >= 2 && IS_DIRCHAR (path[l-2]) && path[l-1] == '.')
    return true;
  if (l >= 3 && IS_DIRCHAR (path[l-3]) && path[l-2] == '.' && path[l-1] == '.')
    return true;
  return false;
}

/// Create the directories in `dirpath` with `mode`, check errno on false returns.
bool
mkdirs (const String &dirpath, uint mode)
{
  return g_mkdir_with_parents (dirpath.c_str(), mode) == 0;
}

/// Get a @a user's home directory, uses $HOME if no @a username is given.
String
user_home (const String &username)
{
  if (username.empty())
    {
      // $HOME gets precedence over getpwnam(3), like '~/' vs '~username/' expansion
      const char *homedir = getenv ("HOME");
      if (homedir && isabs (homedir))
        return homedir;
    }
  CxxPasswd pwn (username);
  return pwn.pw_dir;
}

/// Get the $XDG_DATA_HOME directory, see: https://specifications.freedesktop.org/basedir-spec/latest
String
data_home ()
{
  const char *var = getenv ("XDG_DATA_HOME");
  if (var && isabs (var))
    return var;
  return expand_tilde ("~/.local/share");
}

/// Get the $XDG_CONFIG_HOME directory, see: https://specifications.freedesktop.org/basedir-spec/latest
String
config_home ()
{
  const char *var = getenv ("XDG_CONFIG_HOME");
  if (var && isabs (var))
    return var;
  return expand_tilde ("~/.config");
}

/// Get the $XDG_CACHE_HOME directory, see: https://specifications.freedesktop.org/basedir-spec/latest
String
cache_home ()
{
  const char *var = getenv ("XDG_CACHE_HOME");
  if (var && isabs (var))
    return var;
  return expand_tilde ("~/.cache");
}

/// Get the $XDG_RUNTIME_DIR directory, see: https://specifications.freedesktop.org/basedir-spec/latest
String
runtime_dir ()
{
  const char *var = getenv ("XDG_RUNTIME_DIR");
  if (var && isabs (var))
    return var;
  return string_format ("/run/user/%u", getuid());
}

/// Get the $XDG_CONFIG_DIRS directory list, see: https://specifications.freedesktop.org/basedir-spec/latest
String
config_dirs ()
{
  const char *var = getenv ("XDG_CONFIG_DIRS");
  if (var && var[0])
    return var;
  else
    return "/etc/xdg";
}

/// Get the $XDG_DATA_DIRS directory list, see: https://specifications.freedesktop.org/basedir-spec/latest
String
data_dirs ()
{
  const char *var = getenv ("XDG_DATA_DIRS");
  if (var && var[0])
    return var;
  else
    return "/usr/local/share:/usr/share";
}

static String
access_config_names (const String *newval)
{
  static std::mutex mutex;
  static std::lock_guard<std::mutex> locker (mutex);
  static String cfg_names;
  if (newval)
    cfg_names = *newval;
  if (cfg_names.empty())
    {
      String names = Path::basename (program_alias());
      if (program_alias() != names)
        names = searchpath_join (names, program_alias());
      return names;
    }
  else
    return cfg_names;
}

/// Get config names as set with config_names(), if unset defaults to program_alias().
String
config_names ()
{
  return access_config_names (NULL);
}

/// Set a colon separated list of names for this application to find configuration settings and files.
void
config_names (const String &names)
{
  access_config_names (&names);
}

StringPair
split_extension (const std::string &filepath, const bool lastdot)
{
  const char *const fullpath = filepath.c_str();
  const char *const slash1 = strrchr (fullpath, '/'), *const slash2 = strrchr (fullpath, '\\');
  const char *const slash = slash2 > slash1 ? slash2 : slash1;
  const char *const dot = lastdot ? strrchr (slash ? slash : fullpath, '.') : strchr (slash ? slash : fullpath, '.');
  if (dot)
    return std::make_pair (filepath.substr (0, dot - fullpath), filepath.substr (dot - fullpath));
  return std::make_pair (filepath, "");
}

/// Expand a "~/" or "~user/" @a path which refers to user home directories.
String
expand_tilde (const String &path)
{
  if (path[0] != '~')
    return path;
  const size_t dir1 = path.find (BSE_DIRCHAR);
  const size_t dir2 = BSE_DIRCHAR == BSE_DIRCHAR2 ? String::npos : path.find (BSE_DIRCHAR2);
  const size_t dir = MIN (dir1, dir2);
  String username;
  if (dir != String::npos)
    username = path.substr (1, dir - 1);
  else
    username = path.substr (1);
  const String userhome = user_home (username);
  return join (userhome, dir == String::npos ? "" : path.substr (dir));
}

String
skip_root (const String &path)
{
  const char *frag = g_path_skip_root (path.c_str());
  return frag;
}

String
join (const String &frag0, const String &frag1,
      const String &frag2, const String &frag3,
      const String &frag4, const String &frag5,
      const String &frag6, const String &frag7,
      const String &frag8, const String &frag9,
      const String &frag10, const String &frag11,
      const String &frag12, const String &frag13,
      const String &frag14, const String &frag15)
{
  const char dirsep[2] = { BSE_DIRCHAR, 0 };
  gchar *cpath = g_build_path (dirsep, frag0.c_str(),
                               frag1.c_str(), frag2.c_str(), frag3.c_str(), frag4.c_str(),
                               frag5.c_str(), frag6.c_str(), frag7.c_str(), frag8.c_str(),
                               frag9.c_str(), frag10.c_str(), frag11.c_str(), frag12.c_str(),
                               frag13.c_str(), frag14.c_str(), frag15.c_str(), NULL);
  String path (cpath);
  g_free (cpath);
  return path;
}

static int
errno_check_file (const char *file_name, const char *mode)
{
  uint access_mask = 0, nac = 0;

  if (strchr (mode, 'e'))       // exists
    nac++, access_mask |= F_OK;
  if (strchr (mode, 'r'))       // readable
    nac++, access_mask |= R_OK;
  if (strchr (mode, 'w'))       // writable
    nac++, access_mask |= W_OK;
  bool check_exec = strchr (mode, 'x') != NULL;
  if (check_exec)               // executable
    nac++, access_mask |= X_OK;

  /* on some POSIX systems, X_OK may succeed for root without any
   * executable bits set, so we also check via stat() below.
   */
  if (nac && access (file_name, access_mask) < 0)
    return -errno;

  const bool check_file = strchr (mode, 'f') != NULL;   // open as file
  const bool check_dir  = strchr (mode, 'd') != NULL;   // open as directory
  const bool check_link = strchr (mode, 'l') != NULL;   // open as link
  const bool check_char = strchr (mode, 'c') != NULL;   // open as character device
  const bool check_block = strchr (mode, 'b') != NULL;  // open as block device
  const bool check_pipe = strchr (mode, 'p') != NULL;   // open as pipe
  const bool check_socket = strchr (mode, 's') != NULL; // open as socket

  if (check_exec || check_file || check_dir || check_link || check_char || check_block || check_pipe || check_socket)
    {
      struct stat st;

      if (check_link)
        {
          if (lstat (file_name, &st) < 0)
            return -errno;
        }
      else if (stat (file_name, &st) < 0)
        return -errno;

      if (0)
        printerr ("file-check(\"%s\",\"%s\"): %s%s%s%s%s%s%s\n",
                  file_name, mode,
                  S_ISREG (st.st_mode) ? "f" : "",
                  S_ISDIR (st.st_mode) ? "d" : "",
                  S_ISLNK (st.st_mode) ? "l" : "",
                  S_ISCHR (st.st_mode) ? "c" : "",
                  S_ISBLK (st.st_mode) ? "b" : "",
                  S_ISFIFO (st.st_mode) ? "p" : "",
                  S_ISSOCK (st.st_mode) ? "s" : "");

      if (S_ISDIR (st.st_mode) && (check_file || check_link || check_char || check_block || check_pipe))
        return -EISDIR;
      if (check_file && !S_ISREG (st.st_mode))
        return -EINVAL;
      if (check_dir && !S_ISDIR (st.st_mode))
        return -ENOTDIR;
      if (check_link && !S_ISLNK (st.st_mode))
        return -EINVAL;
      if (check_char && !S_ISCHR (st.st_mode))
        return -ENODEV;
      if (check_block && !S_ISBLK (st.st_mode))
        return -ENOTBLK;
      if (check_pipe && !S_ISFIFO (st.st_mode))
        return -ENXIO;
      if (check_socket && !S_ISSOCK (st.st_mode))
        return -ENOTSOCK;
      if (check_exec && !(st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)))
        return -EACCES; // for root executable, any +x bit is good enough
    }

  return 0;
}

/**
 * @param file  possibly relative filename
 * @param mode  feature string
 * @return      true if @a file adhears to @a mode
 *
 * Perform various checks on @a file and return whether all
 * checks passed. On failure, errno is set appropriately, and
 * FALSE is returned. Available features to be checked for are:
 * @li @c e - @a file must exist
 * @li @c r - @a file must be readable
 * @li @c w - @a file must be writable
 * @li @c x - @a file must be executable
 * @li @c f - @a file must be a regular file
 * @li @c d - @a file must be a directory
 * @li @c l - @a file must be a symbolic link
 * @li @c c - @a file must be a character device
 * @li @c b - @a file must be a block device
 * @li @c p - @a file must be a named pipe
 * @li @c s - @a file must be a socket.
 */
bool
check (const String &file, const String &mode)
{
  int err = file.size() && mode.size() ? errno_check_file (file.c_str(), mode.c_str()) : -EFAULT;
  errno = err < 0 ? -err : 0;
  return errno == 0;
}

/**
 * @param file1  possibly relative filename
 * @param file2  possibly relative filename
 * @return       TRUE if @a file1 and @a file2 are equal
 *
 * Check whether @a file1 and @a file2 are pointing to the same inode
 * in the same file system on the same device.
 */
bool
equals (const String &file1, const String &file2)
{
  if (!file1.size() || !file2.size())
    return file1.size() == file2.size();
  struct stat st1 = { 0, }, st2 = { 0, };
  int err1 = 0, err2 = 0;
  errno = 0;
  if (stat (file1.c_str(), &st1) < 0 && stat (file1.c_str(), &st1) < 0)
    err1 = errno;
  errno = 0;
  if (stat (file2.c_str(), &st2) < 0 && stat (file2.c_str(), &st2) < 0)
    err2 = errno;
  if (err1 || err2)
    return err1 == err2;
  return (st1.st_dev  == st2.st_dev &&
          st1.st_ino  == st2.st_ino &&
          st1.st_rdev == st2.st_rdev);
}

/// Return the current working directoy, including symlinks used in $PWD if available.
String
cwd ()
{
#ifdef  _GNU_SOURCE
  {
    char *dir = get_current_dir_name();
    if (dir)
      {
        const String result = dir;
        free (dir);
        return result;
      }
  }
#endif
  size_t size = 512;
  do
    {
      char *buf = (char*) malloc (size);
      if (!buf)
        break;
      const char *const dir = getcwd (buf, size);
      if (dir)
        {
          const String result = dir;
          free (buf);
          return result;
        }
      free (buf);
      size *= 2;
    }
  while (errno == ERANGE);
  // system must be in a bad shape if we get here...
  return "./";
}

StringVector
searchpath_split (const String &searchpath)
{
  StringVector sv;
  uint i, l = 0;
  for (i = 0; i < searchpath.size(); i++)
    if (IS_SEARCHPATH_SEPARATOR (searchpath[i]))
      {
        if (i > l)
          sv.push_back (searchpath.substr (l, i - l));
        l = i + 1;
      }
  if (i > l)
    sv.push_back (searchpath.substr (l, i - l));
  return sv;
}

/// Check if @a searchpath contains @a element, a trailing slash searches for directories.
bool
searchpath_contains (const String &searchpath, const String &element)
{
  const bool dirsearch = element.size() > 0 && IS_DIRCHAR (element[element.size() - 1]);
  const String needle = dirsearch && element.size() > 1 ? element.substr (0, element.size() - 1) : element; // strip trailing slash
  size_t pos = searchpath.find (needle);
  while (pos != String::npos)
    {
      size_t end = pos + needle.size();
      if (pos == 0 || IS_SEARCHPATH_SEPARATOR (searchpath[pos - 1]))
        {
          if (dirsearch && IS_DIRCHAR (searchpath[end]))
            end++; // skip trailing slash in searchpath segment
          if (searchpath[end] == 0 || IS_SEARCHPATH_SEPARATOR (searchpath[end]))
            return true;
        }
      pos = searchpath.find (needle, end);
    }
  return false;
}

/// Find the first @a file in @a searchpath which matches @a mode (see check()).
String
searchpath_find (const String &searchpath, const String &file, const String &mode)
{
  if (isabs (file))
    return check (file, mode) ? file : "";
  StringVector sv = searchpath_split (searchpath);
  for (size_t i = 0; i < sv.size(); i++)
    if (check (join (sv[i], file), mode))
      return join (sv[i], file);
  return "";
}

/// Find all @a searchpath entries matching @a mode (see check()).
StringVector
searchpath_list (const String &searchpath, const String &mode)
{
  StringVector v;
  for (const auto &file : searchpath_split (searchpath))
    if (check (file, mode))
      v.push_back (file);
  return v;
}

static String
searchpath_join1 (const String &a, const String &b)
{
  if (a.empty())
    return b;
  if (b.empty())
    return a;
  if (IS_SEARCHPATH_SEPARATOR (a[a.size()-1]) || IS_SEARCHPATH_SEPARATOR (b[0]))
    return a + b;
  const char searchsep[2] = { BSE_SEARCHPATH_SEPARATOR, 0 };
  return a + String (searchsep) + b;
}

/// Yield a new searchpath by combining each element of @a searchpath with each element of @a postfixes.
String
searchpath_multiply (const String &searchpath, const String &postfixes)
{
  String newpath;
  for (const auto &e : searchpath_split (searchpath))
    for (const auto &p : searchpath_split (postfixes))
      newpath = searchpath_join1 (newpath, join (e, p));
  return newpath;
}

String
searchpath_join (const String &frag0, const String &frag1, const String &frag2, const String &frag3,
                 const String &frag4, const String &frag5, const String &frag6, const String &frag7,
                 const String &frag8, const String &frag9, const String &frag10, const String &frag11,
                 const String &frag12, const String &frag13, const String &frag14, const String &frag15)
{
  String result = searchpath_join1 (frag0, frag1);
  result = searchpath_join1 (result, frag2);
  result = searchpath_join1 (result, frag3);
  result = searchpath_join1 (result, frag4);
  result = searchpath_join1 (result, frag5);
  result = searchpath_join1 (result, frag6);
  result = searchpath_join1 (result, frag7);
  result = searchpath_join1 (result, frag8);
  result = searchpath_join1 (result, frag9);
  result = searchpath_join1 (result, frag10);
  result = searchpath_join1 (result, frag11);
  result = searchpath_join1 (result, frag12);
  result = searchpath_join1 (result, frag13);
  result = searchpath_join1 (result, frag14);
  result = searchpath_join1 (result, frag15);
  return result;
}

String
searchpath_join (const StringVector &string_vector)
{
  const char searchsep[2] = { BSE_SEARCHPATH_SEPARATOR, 0 };
  return string_join (searchsep, string_vector);
}

String
vpath_find (const String &file, const String &mode)
{
  String result = searchpath_find (".", file, mode);
  if (!result.empty())
    return result;
  const char *vpath = getenv ("VPATH");
  if (vpath)
    {
      result = searchpath_find (vpath, file, mode);
      if (!result.empty())
        return result;
    }
  return file;
}

static char* // return malloc()-ed buffer containing a full read of FILE
file_memread (FILE *stream, size_t *lengthp, ssize_t maxlength)
{
  size_t sz = 4096;
  char *buffer = (char*) malloc (sz);
  if (!buffer)
    return NULL;
  char *current = buffer;
  errno = 0;
  while (!feof (stream))
    {
      size_t bytes = fread (current, 1, sz - (current - buffer), stream);
      if (bytes <= 0 && ferror (stream) && errno != EAGAIN)
        {
          current = buffer; // error/0-data
          break;
        }
      current += bytes;
      if (maxlength >= 0 && current - buffer >= maxlength)
        {
          current = buffer + maxlength; // shorten if needed
          break;
        }
      if (current == buffer + sz)
        {
          bytes = current - buffer;
          sz *= 2;
          char *newstring = (char*) realloc (buffer, sz);
          if (!newstring)
            {
              current = buffer; // error/0-data
              break;
            }
          buffer = newstring;
          current = buffer + bytes;
        }
    }
  int savederr = errno;
  *lengthp = current - buffer;
  if (!*lengthp)
    {
      free (buffer);
      buffer = NULL;
    }
  errno = savederr;
  return buffer;
}

char*
memread (const String &filename, size_t *lengthp, ssize_t maxlength)
{
  FILE *file = fopen (filename.c_str(), "r");
  if (!file)
    {
      *lengthp = 0;
      return strdup ("");
    }
  char *contents = file_memread (file, lengthp, maxlength);
  int savederr = errno;
  fclose (file);
  errno = savederr;
  return contents;
}

void
memfree (char *memread_mem)
{
  if (memread_mem)
    free (memread_mem);
}

bool
memwrite (const String &filename, size_t len, const uint8 *bytes)
{
  FILE *file = fopen (filename.c_str(), "w");
  if (!file)
    return false;
  const size_t nbytes = fwrite (bytes, 1, len, file);
  bool success = ferror (file) == 0 && nbytes == len;
  success = fclose (file) == 0 && success;
  if (!success)
    unlink (filename.c_str());
  return success;
}

// Read `filename` into a std::string, check `errno` for empty returns.
String
stringread (const String &filename, ssize_t maxlength)
{
  String s;
  size_t length = 0;
  errno = 0;
  char *data = memread (filename, &length);
  if (data)
    {
      s = String (data, length);
      memfree (data);
      errno = 0;
    }
  return s;
}

// Write `data` into `filename`, check `errno` for false returns.
bool
stringwrite (const String &filename, const String &data, bool mkdirs_)
{
  if (mkdirs_)
    mkdirs (dirname (filename), 0750);
  return memwrite (filename, data.size(), (const uint8*) data.data());
}

} // Path
} // Bse

#include <pwd.h>        // getpwuid

// == CxxPasswd =
namespace { // Anon
CxxPasswd::CxxPasswd (std::string username) :
  pw_uid (-1), pw_gid (-1)
{
  const int strbuf_size = 5 * 1024;
  char strbuf[strbuf_size + 256]; // work around Darwin getpwnam_r overwriting buffer boundaries
  struct passwd pwnambuf, *p = NULL;
  if (username.empty())
    {
      int ret = 0;
      errno = 0;
      do
        {
          if (1) // HAVE_GETPWUID_R
            ret = getpwuid_r (getuid(), &pwnambuf, strbuf, strbuf_size, &p);
          else   // HAVE_GETPWUID
            p = getpwuid (getuid());
        }
      while ((ret != 0 || p == NULL) && errno == EINTR);
      if (ret != 0)
        p = NULL;
    }
  else // !username.empty()
    {
      int ret = 0;
      errno = 0;
      do
        ret = getpwnam_r (username.c_str(), &pwnambuf, strbuf, strbuf_size, &p);
      while ((ret != 0 || p == NULL) && errno == EINTR);
      if (ret != 0)
        p = NULL;
    }
  if (p)
    {
      pw_name = p->pw_name;
      pw_passwd = p->pw_passwd;
      pw_uid = p->pw_uid;
      pw_gid = p->pw_gid;
      pw_gecos = p->pw_gecos;
      pw_dir = p->pw_dir;
      pw_shell = p->pw_shell;
    }
}
} // Anon

// == Testing ==
#include "testing.hh"
#include "internal.hh"

namespace { // Anon
using namespace Bse;

BSE_INTEGRITY_TEST (bse_test_paths);
static void
bse_test_paths()
{
  String p, s;
  // Path::join
  s = Path::join ("0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "a", "b", "c", "d", "e", "f");
#if BSE_DIRCHAR == '/'
  p = "0/1/2/3/4/5/6/7/8/9/a/b/c/d/e/f";
#else
  p = "0\\1\\2\\3\\4\\5\\6\\7\\8\\9\\a\\b\\c\\d\\e\\f";
#endif
  TCMP (s, ==, p);
  // Path::searchpath_join
  s = Path::searchpath_join ("0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "a", "b", "c", "d", "e", "f");
#if BSE_SEARCHPATH_SEPARATOR == ';'
  p = "0;1;2;3;4;5;6;7;8;9;a;b;c;d;e;f";
#else
  p = "0:1:2:3:4:5:6:7:8:9:a:b:c:d:e:f";
#endif
  TCMP (s, ==, p);
  // Path
  bool b = Path::isabs (p);
  TCMP (b, ==, false);
  const char dirsep[2] = { BSE_DIRCHAR, 0 };
#if BSE_DIRCHAR == '/'
  s = Path::join (dirsep, s);
#else
  s = Path::join ("C:\\", s);
#endif
  b = Path::isabs (s);
  TCMP (b, ==, true);
  s = Path::skip_root (s);
  TCMP (s, ==, p);
  // TASSERT (Path::dir_separator == "/" || Path::dir_separator == "\\");
  TASSERT (BSE_SEARCHPATH_SEPARATOR == ':' || BSE_SEARCHPATH_SEPARATOR == ';');
  TCMP (Path::basename ("simple"), ==, "simple");
  TCMP (Path::basename ("skipthis" + String (dirsep) + "file"), ==, "file");
  TCMP (Path::basename (String (dirsep) + "skipthis" + String (dirsep) + "file"), ==, "file");
  TCMP (Path::dirname ("file"), ==, ".");
  TCMP (Path::dirname ("dir" + String (dirsep)), ==, "dir");
  TCMP (Path::dirname ("dir" + String (dirsep) + "file"), ==, "dir");
  TCMP (Path::cwd(), !=, "");
  TCMP (Path::check (Path::join (Path::cwd(), "..", Path::basename (Path::cwd())), "rd"), ==, true); // ../. should be a readable directory
  TCMP (Path::isdirname (""), ==, false);
  TCMP (Path::isdirname ("foo"), ==, false);
  TCMP (Path::isdirname ("foo/"), ==, true);
  TCMP (Path::isdirname ("/foo"), ==, false);
  TCMP (Path::isdirname ("foo/."), ==, true);
  TCMP (Path::isdirname ("foo/.."), ==, true);
  TCMP (Path::isdirname ("foo/..."), ==, false);
  TCMP (Path::isdirname ("foo/..../"), ==, true);
  TCMP (Path::isdirname ("/."), ==, true);
  TCMP (Path::isdirname ("/.."), ==, true);
  TCMP (Path::isdirname ("/"), ==, true);
  TCMP (Path::isdirname ("."), ==, true);
  TCMP (Path::isdirname (".."), ==, true);
  TCMP (Path::expand_tilde (""), ==, "");
  const char *env_home = getenv ("HOME");
  if (env_home)
    TCMP (Path::expand_tilde ("~"), ==, env_home);
  const char *env_logname = getenv ("LOGNAME");
  if (env_home && env_logname)
    TCMP (Path::expand_tilde ("~" + String (env_logname)), ==, env_home);
  TCMP (Path::searchpath_multiply ("/:/tmp", "foo:bar"), ==, "/foo:/bar:/tmp/foo:/tmp/bar");
  const String abs_basedir = Path::abspath (Bse::runpath (Bse::RPath::PREFIXDIR));
  TCMP (Path::searchpath_list ("/:" + abs_basedir, "e"), ==, StringVector ({ "/", abs_basedir }));
  TCMP (Path::searchpath_contains ("/foo/:/bar", "/"), ==, false);
  TCMP (Path::searchpath_contains ("/foo/:/bar", "/foo"), ==, false); // false because "/foo" is file search
  TCMP (Path::searchpath_contains ("/foo/:/bar", "/foo/"), ==, true); // true because "/foo/" is dir search
  TCMP (Path::searchpath_contains ("/foo/:/bar", "/bar"), ==, true); // file search matches /bar
  TCMP (Path::searchpath_contains ("/foo/:/bar", "/bar/"), ==, true); // dir search matches /bar
}

} // Anon
