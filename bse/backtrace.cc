// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "platform.hh"
#include "path.hh"
#include <unistd.h>
#include <cstring>
#include <setjmp.h>
#include <sys/wait.h>
#include <sys/time.h>

#if __has_include(<execinfo.h>)
#include <execinfo.h>           // _EXECINFO_H
#endif

namespace Bse {

// == Prototypes ==
static StringVector addr2line_backtrace_symbols (void **pointers, const int nptrs);
static bool         backtrace_may_ptrace        ();
static bool         backtrace_have_gdb          ();
static bool         backtrace_print_gdb         (bool full);

// == backtrace() ==
static BSE_UNUSED int
dummy_backtrace (void **buffer, int size)
{
  if (size)
    {
      buffer[0] = __builtin_return_address (0);
      return 1;
    }
  return 0;
}
#ifdef _EXECINFO_H
int (*Internal::backtrace_pointers) (void **buffer, int size) = &::backtrace; // GLibc only
#else  // !_EXECINFO_H
int (*Internal::backtrace_pointers) (void **buffer, int size) = &dummy_backtrace;
#endif // !_EXECINFO_H

// == backtrace_print_frames ==
/// Print out a backtrace based on @a ptrs.
bool
Internal::backtrace_print_frames (const char *file, int line, const char *func, void **ptrs, ssize_t nptrs)
{
  // print intro
  char buffer[512];
  fflush (stdout);
  fputs ("Backtrace[", stderr);
  snprintf (buffer, sizeof (buffer), "%u", getpid());
  fputs (buffer, stderr);
  fputs ("] at ", stderr);
  snprintf (buffer, sizeof (buffer), "0x%016llx", (unsigned long long) ptrdiff_t (ptrs[0]));
  fputs (buffer, stderr);
  if (file && file[0])
    {
      fputs (" (from ", stderr);
      fputs (file, stderr);
      if (line > 0)
        {
          snprintf (buffer, sizeof (buffer), ":%u", line);
          fputs (buffer, stderr);
        }
      if (func && func[0])
        {
          fputs (":", stderr);
          fputs (func, stderr);
        }
      fputs (")", stderr);
    }
  fputs (":\n", stderr);
  // try addr2line backtrace
  const char *missingnote = NULL;
  if (nptrs)
    {
      const StringVector symbols = addr2line_backtrace_symbols (ptrs, nptrs);
      if (!symbols.empty())
        {
          for (size_t i = 0; i < symbols.size(); i++)
            {
              fputs ("  ", stderr);
              fputs (symbols[i].c_str(), stderr);
              fputs ("\n", stderr);
            }
          fflush (stderr);
          return true;
        }
      else
        missingnote = missingnote ? missingnote : "need addr2line for backtrace frame names";
    }
  // try gdb backtrace
  if (!backtrace_have_gdb())
    missingnote = missingnote ? missingnote : "need working gdb for a detailed backtrace";
  else if (!backtrace_may_ptrace())
    missingnote = missingnote ? missingnote : "need ptrace permissions for a detailed backtrace, try: echo 0 > /proc/sys/kernel/yama/ptrace_scope";
  else
    {
      fflush (stderr);
      memset (buffer, 0, sizeof (buffer)); // shortens gdb output
      return backtrace_print_gdb (true);
    }
  // note about missing tools
  if (missingnote)
    {
      fputs ("  NOTE: ", stderr);
      fputs (missingnote, stderr);
      fputs ("\n", stderr);
    }
  // try backtrace_symbols_fd
#ifdef _EXECINFO_H
  if (nptrs)
    {
      fflush (stderr);
      backtrace_symbols_fd (ptrs, nptrs, STDERR_FILENO);
      return true;
    }
#endif // _EXECINFO_H
  // give up
  fputs ("  <no backtrace method available>\n", stderr);
  fflush (stderr);
  return false;
}

// == GDB Backtrace ==
static const char *const bin_gdb = "/usr/bin/gdb";
static const char *const ptrace_scope = "/proc/sys/kernel/yama/ptrace_scope";

/// Check /proc/sys/kernel/yama/ptrace_scope for working ptrace().
static bool
backtrace_may_ptrace()
{
  bool allow_ptrace = false;
#ifdef  __linux__
  FILE *f = fopen (ptrace_scope, "r");
  size_t flag = -1;
  if (fscanf (f, "%zx", &flag) == 1)
    allow_ptrace = flag == 0;
  fclose (f);
#else
  allow_ptrace = true;
#endif
  return allow_ptrace;
}

/// Check for /usr/bin/gdb and /proc/sys/kernel/yama/ptrace_scope for working ptrace().
static bool
backtrace_have_gdb()
{
  return access (bin_gdb, X_OK) == 0;
}

static bool
backtrace_print_gdb (bool full)
{
  std::string cmd = string_format ("%s -p %u --batch -ex 'thread apply all backtrace %s' >&2",
                                   bin_gdb, getpid(), full ? "full" : "");
  if (system (cmd.c_str()) == 0)
    return true;
  return false;
}

// == Sub-Process ==
struct PExec {
  String data_in, data_out, data_err;
  uint64 usec_timeout; // max child runtime
  StringVector args;
  StringVector evars;
public:
  inline int  execute (); // returns -errno
};

static String
exec_cmd (const String &cmd, bool witherr, StringVector fix_env, uint64 usec_timeout = 1000000 / 2)
{
  String btout;
  PExec sub;
  sub.data_in = "";
  sub.usec_timeout = usec_timeout;
  sub.args = string_split (cmd, " ");
  if (sub.args.size() < 1)
    sub.args.push_back ("");
  sub.evars = fix_env;
  int eret = sub.execute();
  if (eret < 0)
    warning ("executing '%s' failed: %s\n", sub.args[0].c_str(), strerror (-eret));
  if (!witherr)
    sub.data_err = "";
  return sub.data_err + sub.data_out;
}

static int // 0 on success
kill_child (int pid, int *status, int patience)
{
  int wr;
  if (patience >= 3)    // try graceful reap
    {
      if (waitpid (pid, status, WNOHANG) > 0)
        return 0;
    }
  if (patience >= 2)    // try SIGHUP
    {
      kill (pid, SIGHUP);
      if (waitpid (pid, status, WNOHANG) > 0)
        return 0;
      usleep (20 * 1000); // give it some scheduling/shutdown time
      if (waitpid (pid, status, WNOHANG) > 0)
        return 0;
      usleep (50 * 1000); // give it some scheduling/shutdown time
      if (waitpid (pid, status, WNOHANG) > 0)
        return 0;
      usleep (100 * 1000); // give it some scheduling/shutdown time
      if (waitpid (pid, status, WNOHANG) > 0)
        return 0;
    }
  if (patience >= 1)    // try SIGTERM
    {
      kill (pid, SIGTERM);
      if (waitpid (pid, status, WNOHANG) > 0)
        return 0;
      usleep (200 * 1000); // give it some scheduling/shutdown time
      if (waitpid (pid, status, WNOHANG) > 0)
        return 0;
      usleep (400 * 1000); // give it some scheduling/shutdown time
      if (waitpid (pid, status, WNOHANG) > 0)
        return 0;
    }
  // finish it off
  kill (pid, SIGKILL);
  do
    wr = waitpid (pid, status, 0);
  while (wr < 0 && errno == EINTR);
  return wr;
}

static inline ssize_t
string_must_read (String &string, int fd)
{
  ssize_t n_bytes;
  do
    {
      char buf[4096];
      n_bytes = read (fd, buf, sizeof (buf));
      if (n_bytes == 0)
        return 0; // EOF, calling this function assumes data is available
      else if (n_bytes > 0)
        {
          string.append (buf, n_bytes);
          return n_bytes;
        }
    }
  while (n_bytes < 0 && errno == EINTR);
  // n_bytes < 0
  warning ("failed to read() from child process: %s", strerror (errno));
  return -1;
}

#define RETRY_ON_EINTR(expr)    ({ ssize_t __r; do __r = ssize_t (expr); while (__r < 0 && errno == EINTR); __r; })

static inline size_t
string_must_write (const String &string, int outfd, size_t *stringpos)
{
  if (*stringpos < string.size())
    {
      ssize_t n = RETRY_ON_EINTR (write (outfd, string.data() + *stringpos, string.size() - *stringpos));
      *stringpos += MAX (n, 0);
    }
  return *stringpos < string.size(); // remainings
}

int
PExec::execute ()
{
  int fork_pid = -1;
  int stdin_pipe[2] = { -1, -1 };
  int stdout_pipe[2] = { -1, -1 };
  int stderr_pipe[2] = { -1, -1 };
  ssize_t i;
  const int parent_pid = getpid();
  if (pipe (stdin_pipe) < 0 || pipe (stdout_pipe) < 0 || pipe (stderr_pipe) < 0)
    goto return_errno;
  if (signal (SIGCHLD, SIG_DFL) == SIG_ERR)
    goto return_errno;
  char parent_pid_str[64];
  if (snprintf (parent_pid_str, sizeof (parent_pid_str), "%u", parent_pid) < 0)
    goto return_errno;
  char self_exe[1024]; // PATH_MAX
  i = readlink ("/proc/self/exe", self_exe, sizeof (self_exe));
  if (i < 0)
    goto return_errno;
  errno = ENOMEM;
  if (size_t (i) >= sizeof (self_exe))
    goto return_errno;
  fork_pid = fork ();
  if (fork_pid < 0)
    goto return_errno;
  if (fork_pid == 0)    // child
    {
      for (const String &evar : evars)
        {
          const char *var = evar.c_str(), *eq = strchr (var, '=');
          if (!eq)
            {
              unsetenv (var);
              continue;
            }
          setenv (evar.substr (0, eq - var).c_str(), eq + 1, true);
        }
      close (stdin_pipe[1]);
      close (stdout_pipe[0]);
      close (stderr_pipe[0]);
      if (RETRY_ON_EINTR (dup2 (stdin_pipe[0], 0)) < 0 ||
          RETRY_ON_EINTR (dup2 (stdout_pipe[1], 1)) < 0 ||
          RETRY_ON_EINTR (dup2 (stderr_pipe[1], 2)) < 0)
        {
          kill (getpid(), SIGSYS);
          while (1)
            _exit (-128);
        }
      if (stdin_pipe[0] >= 3)
        close (stdin_pipe[0]);
      if (stdout_pipe[1] >= 3)
        close (stdout_pipe[1]);
      if (stderr_pipe[1] >= 3)
        close (stderr_pipe[1]);
      const ssize_t exec_nargs = args.size() + 1;
      const char *exec_args[exec_nargs];
      for (i = 0; i < exec_nargs - 1; i++)
        if (args[i] == "\uFFFDexe\u001A")
          exec_args[i] = self_exe;
        else if (args[i] == "\uFFFDpid\u001A")
          exec_args[i] = parent_pid_str;
        else
          exec_args[i] = args[i].c_str();
      exec_args[exec_nargs - 1] = NULL;
      execvp (exec_args[0], (char**) exec_args);
      kill (getpid(), SIGSYS);
      while (1)
        _exit (-128);
    }
  else                  // parent
    {
      size_t data_in_pos = 0, need_wait = true;
      close (stdin_pipe[0]);
      close (stdout_pipe[1]);
      close (stderr_pipe[1]);
      uint last_status = 0;
      uint64 sstamp = timestamp_realtime();
      // read data until we get EOF on all pipes
      while (stdout_pipe[0] >= 0 || stderr_pipe[0] >= 0)
        {
          fd_set readfds, writefds;
          FD_ZERO (&readfds);
          FD_ZERO (&writefds);
          if (stdin_pipe[1] >= 0)
            FD_SET (stdin_pipe[1], &writefds);
          if (stdout_pipe[0] >= 0)
            FD_SET (stdout_pipe[0], &readfds);
          if (stderr_pipe[0] >= 0)
            FD_SET (stderr_pipe[0], &readfds);
          int maxfd = MAX (stdin_pipe[1], MAX (stdout_pipe[0], stderr_pipe[0]));
          struct timeval tv;
          tv.tv_sec = 0; // sleep at most 0.5 seconds to catch clock skews, etc.
          tv.tv_usec = MIN (usec_timeout ? usec_timeout : 1000000, 100 * 1000);
          int ret = select (maxfd + 1, &readfds, &writefds, NULL, &tv);
          if (ret < 0 && errno != EINTR)
            goto return_errno;
          if (stdin_pipe[1] >= 0 &&
              FD_ISSET (stdin_pipe[1], &writefds) &&
              string_must_write (data_in, stdin_pipe[1], &data_in_pos) == 0)
            {
              close (stdin_pipe[1]);
              stdin_pipe[1] = -1;
            }
          if (stdout_pipe[0] >= 0 && FD_ISSET (stdout_pipe[0], &readfds) && string_must_read (data_out, stdout_pipe[0]) == 0)
            {
              close (stdout_pipe[0]);
              stdout_pipe[0] = -1;
            }
          if (stderr_pipe[0] >= 0 && FD_ISSET (stderr_pipe[0], &readfds) && string_must_read (data_err, stderr_pipe[0]) == 0)
            {
              close (stderr_pipe[0]);
              stderr_pipe[0] = -1;
            }
          if (usec_timeout)
            {
              uint64 nstamp = timestamp_realtime();
              int status = 0;
              sstamp = MIN (sstamp, nstamp); // guard against backwards clock skews
              if (usec_timeout < nstamp - sstamp)
                {
                  // timeout reached, need to abort the child now
                  kill_child (fork_pid, &status, 3);
                  last_status = 1024; // timeout
                  if (WIFSIGNALED (status))
                    ; // debug_msg ("%s: child timed out and received: %s\n", __func__, strsignal (WTERMSIG (status)));
                  need_wait = false;
                  break;
                }
            }
        }
      close (stdout_pipe[0]);
      close (stderr_pipe[0]);
      if (need_wait)
        {
          int status = 0;
          pid_t wr;
          do
            wr = waitpid (fork_pid, &status, 0);
          while (wr < 0 && errno == EINTR);
          if (WIFEXITED (status)) // normal exit
            last_status = WEXITSTATUS (status); // 0..255
          else if (WIFSIGNALED (status))
            last_status = (WTERMSIG (status) << 12); // signalled
          else // WCOREDUMP (status)
            last_status = 512; // coredump
        }
      return last_status;
    }
 return_errno:
  const int error_errno = errno;
  if (fork_pid > 0) // child still alive?
    {
      int status = 0;
      kill_child (fork_pid, &status, 1);
    }
  close (stdin_pipe[0]);
  close (stdin_pipe[1]);
  close (stdout_pipe[0]);
  close (stdout_pipe[1]);
  close (stderr_pipe[0]);
  close (stderr_pipe[1]);
  errno = error_errno;
  return -error_errno;
}

// == Backtrace ==
struct Mapping { size_t addr, end; String exe; };
typedef std::vector<Mapping> MappingVector;

static MappingVector
read_maps ()
{
  MappingVector mv;
  FILE *fmaps = fopen ("/proc/self/maps", "r");
  if (!fmaps)
    return mv;
  int count = 0;
  while (!feof (fmaps))
    {
      size_t addr = 0, end = 0, offset, inode = 0;
      char perms[9 + 1], device[9 + 1] = "";
      count = fscanf (fmaps, "%zx-%zx %9s %zx %9s %zu", &addr, &end, perms, &offset, device, &inode);
      if (count < 1)
        break;
      int c = fgetc (fmaps);
      while (c == ' ')
        c = fgetc (fmaps);
      std::string filename;
      if (!feof (fmaps))
        while (c != EOF && c && c != '\n')
          {
            filename += c;
            c = fgetc (fmaps);
          }
      if (filename[0])
        mv.push_back (Mapping { addr, end, filename });
    }
  fclose (fmaps);
  return mv;
}

/// Generate a list of strings describing backtrace frames from the given frame pointers using addr2line(1).
static StringVector
addr2line_backtrace_symbols (void **pointers, const int nptrs)
{
  // fetch process maps to correlate pointers
  MappingVector maps = read_maps();
  // reduce mappings to DSOs
  char self_exe[1024 + 1] = { 0, }; // PATH_MAX
  ssize_t l = readlink ("/proc/self/exe", self_exe, sizeof (self_exe));
  if (l > 0)
    {
      // filter out mapping entry for no-PIE binary which addr2line expects absolute addresses for
      for (size_t i = 0; i < maps.size(); i++)
        if (maps[i].exe.size() < 2 || maps[i].exe[0] != '/' ||  // erase "[heap]" and other non-dso
            (maps[i].exe == self_exe &&                         // find /proc/self/exe
             (maps[i].addr == 0x08048000 ||                     // statically linked elf_i386 binary
              maps[i].addr == 0x00400000)))                     // statically linked elf_x86_64 binary
          {
            maps.erase (maps.begin() + i);
            i--;
          }
    }
  // resolve pointers to function, file and line
  StringVector symbols;
  const char *addr2line = "/usr/bin/addr2line";
  const bool have_addr2line = access (addr2line, X_OK) == 0;
  for (ssize_t i = 0; i < nptrs; i++)
    {
      const size_t addr = size_t (pointers[i]);
      String dso;
      size_t dso_offset = 0;
      // find DSO for pointer
      for (size_t j = 0; j < maps.size(); j++)
        if (maps[j].addr < addr && addr < maps[j].end)
          {
            dso = maps[j].exe;
            dso_offset = addr - maps[j].addr;
            break;
          }
      if (dso.empty())
        {
          // no DSO, resort to executable
          dso = self_exe;
          dso_offset = addr;
        }
      // resolve through addr2line
      String entry;
      if (have_addr2line)
        {
          // resolve to code location *before* return address
          const size_t caller_addr = dso_offset - 1;
          entry = exec_cmd (string_format ("%s -C -f -s -i -p -e %s 0x%zx", addr2line, dso.c_str(), caller_addr), true,
                            cstrings_to_vector ("LC_ALL=C", "LANGUAGE", NULL));
        }
      // polish entry
      while (entry.size() && strchr ("\n \t\r", entry[entry.size() - 1]))
        entry.resize (entry.size() - 1);
      if (string_endswith (entry, ":?"))
        entry.resize (entry.size() - 2);
      if (string_endswith (entry, " at ??"))
        entry.resize (entry.size() - 6);
      if (entry.compare (0, 2, "??") == 0)
        entry = "";
      if (entry.empty())
        entry = string_format ("%s@0x%zx", dso.c_str(), dso_offset);
      // format backtrace symbol output
      String symbol = string_format ("0x%016zx", addr);
      if (!entry.empty())
        symbol += " in " + entry;
      for (auto sym : string_split (symbol, "\n"))
        {
          if (sym.compare (0, 14, " (inlined by) ") == 0)
            sym = "           inlined by " + sym.substr (14);
          symbols.push_back (sym);
        }
    }
  return symbols;
}

} // Bse
