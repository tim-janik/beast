// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "platform.hh"
#include "path.hh"
#include <unistd.h>
#include <cstring>
#include <sys/wait.h>
#include <sys/time.h>
#include <execinfo.h>           // _EXECINFO_H
#include <sys/syscall.h>        // SYS_gettid
#if defined __APPLE__
#include <mach-o/dyld.h>        // _NSGetExecutablePath
#endif // __APPLE__
#ifdef  _WIN32                  // includes _WIN64
#include <windows.h>
#endif  // _WIN32

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
  const ssize_t max_size = 4000;
  char system_result[max_size + 1 + 1] = { 0, };
  ssize_t system_result_size = -1;

#if defined __linux__ || defined __CYGWIN__ || defined __MSYS__
  system_result_size = readlink ("/proc/self/exe", system_result, max_size);
  if (system_result_size < 0)
    {
      strcpy (system_result, "/proc/self/exe");
      system_result_size = 0;
    }
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

// == TaskStatus ==
TaskStatus::TaskStatus (int pid, int tid) :
  process_id (pid), task_id (tid >= 0 ? tid : pid), state (UNKNOWN), processor (-1), priority (0),
  utime (0), stime (0), cutime (0), cstime (0),
  ac_stamp (0), ac_utime (0), ac_stime (0), ac_cutime (0), ac_cstime (0)
{}

static bool
update_task_status (TaskStatus &self)
{
  static long clk_tck = 0;
  if (!clk_tck)
    {
      clk_tck = sysconf (_SC_CLK_TCK);
      if (clk_tck <= 0)
        clk_tck = 100;
    }
  int pid = -1, ppid = -1, pgrp = -1, session = -1, tty_nr = -1, tpgid = -1;
  int exit_signal = 0, processor = 0;
  long cutime = 0, cstime = 0, priority = 0, nice = 0, dummyld = 0;
  long itrealvalue = 0, rss = 0;
  unsigned long flags = 0, minflt = 0, cminflt = 0, majflt = 0, cmajflt = 0;
  unsigned long utime = 0, stime = 0, vsize = 0, rlim = 0, startcode = 0;
  unsigned long endcode = 0, startstack = 0, kstkesp = 0, kstkeip = 0;
  unsigned long signal = 0, blocked = 0, sigignore = 0, sigcatch = 0;
  unsigned long wchan = 0, nswap = 0, cnswap = 0, rt_priority = 0, policy = 0;
  unsigned long long starttime = 0;
  char state = 0, command[8192 + 1] = { 0 };
  String filename = string_format ("/proc/%u/task/%u/stat", self.process_id, self.task_id);
  FILE *file = fopen (filename.c_str(), "r");
  if (!file)
    return false;
  int n = fscanf (file,
                  "%d %8192s %c "
                  "%d %d %d %d %d "
                  "%lu %lu %lu %lu %lu %lu %lu "
                  "%ld %ld %ld %ld %ld %ld "
                  "%llu %lu %ld "
                  "%lu %lu %lu %lu %lu "
                  "%lu %lu %lu %lu %lu "
                  "%lu %lu %lu %d %d "
                  "%lu %lu",
                  &pid, command, &state, // n=3
                  &ppid, &pgrp, &session, &tty_nr, &tpgid, // n=8
                  &flags, &minflt, &cminflt, &majflt, &cmajflt, &utime, &stime, // n=15
                  &cutime, &cstime, &priority, &nice, &dummyld, &itrealvalue, // n=21
                  &starttime, &vsize, &rss, // n=24
                  &rlim, &startcode, &endcode, &startstack, &kstkesp, // n=29
                  &kstkeip, &signal, &blocked, &sigignore, &sigcatch, // n=34
                  &wchan, &nswap, &cnswap, &exit_signal, &processor, // n=39
                  &rt_priority, &policy // n=41
                  );
  fclose (file);
  const double jiffies_to_usecs = 1000000.0 / clk_tck;
  if (n >= 3)
    self.state = TaskStatus::State (state);
  if (n >= 15)
    {
      self.ac_utime = utime * jiffies_to_usecs;
      self.ac_stime = stime * jiffies_to_usecs;
    }
  if (n >= 17)
    {
      self.ac_cutime = cutime * jiffies_to_usecs;
      self.ac_cstime = cstime * jiffies_to_usecs;
    }
  if (n >= 18)
    self.priority = priority;
  if (n >= 39)
    self.processor = 1 + processor;
  return true;
}

#define ACCOUNTING_MSECS        50

bool
TaskStatus::update ()
{
  const TaskStatus old (*this);
  const uint64 now = timestamp_realtime();              // usecs
  if (ac_stamp + ACCOUNTING_MSECS * 1000 >= now)
    return false;                                       // limit accounting to a few times per second
  if (!update_task_status (*this))
    return false;
  const double delta = 1000000.0 / MAX (1, now - ac_stamp);
  utime = uint64 (MAX (ac_utime - old.ac_utime, 0) * delta);
  stime = uint64 (MAX (ac_stime - old.ac_stime, 0) * delta);
  cutime = uint64 (MAX (ac_cutime - old.ac_cutime, 0) * delta);
  cstime = uint64 (MAX (ac_cstime - old.ac_cstime, 0) * delta);
  ac_stamp = now;
  return true;
}

String
TaskStatus::string ()
{
  return
    string_format ("pid=%d task=%d state=%c processor=%d priority=%d perc=%.2f%% utime=%.3fms stime=%.3fms cutime=%.3f cstime=%.3f",
                   process_id, task_id, state, processor, priority, (utime + stime) * 0.0001,
                   utime * 0.001, stime * 0.001, cutime * 0.001, cstime * 0.001);
}

// == Thread Info ==
ThreadId
this_thread_self ()
{
  return std::this_thread::get_id();
}

void
this_thread_set_name (const String &name16chars)
{
  pthread_setname_np (pthread_self(), name16chars.c_str());
}

String
this_thread_get_name ()
{
  char buffer[1024] = { 0, };
  pthread_getname_np (pthread_self(), buffer, sizeof (buffer) - 1);
  buffer[sizeof (buffer) - 1] = 0;
  return buffer;
}

int
this_thread_getpid ()
{
  return getpid();
}

int
this_thread_gettid ()
{
  int tid = -1;
#ifdef  __linux__       // SYS_gettid is present on linux >= 2.4.20
  tid = syscall (SYS_gettid);
#endif
  if (tid < 0)
    tid = this_thread_getpid();
  return tid;
}

int
this_thread_online_cpus ()
{
  static int cpus = 0;
  if (!cpus)
    cpus = sysconf (_SC_NPROCESSORS_ONLN);
  return cpus;
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
      char perms[9 + 1], filename[1234 + 1] = "", device[9 + 1] = "";
      count = fscanf (fmaps, "%zx-%zx %9s %zx %9s %zu", &addr, &end, perms, &offset, device, &inode);
      if (count < 1)
        break;
      char c = fgetc (fmaps);
      while (c == ' ')
        c = fgetc (fmaps);
      if (c != '\n')
        {
          ungetc (c, fmaps);
          count = fscanf (fmaps, "%1234s\n", filename);
        }
      if (filename[0])
        mv.push_back (Mapping { addr, end, filename });
    }
  fclose (fmaps);
  return mv;
}

/// Generate a list of strings describing backtrace frames from the given frame pointers using addr2line(1).
StringVector
pretty_backtrace_symbols (void **pointers, const int nptrs)
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

/// Generate a pretty backtrace string, given backtrace pointers and using pretty_backtrace_symbols().
String
pretty_backtrace (void **ptrs, ssize_t nptrs, const char *file, int line, const char *func)
{
  void *fallback = __builtin_return_address (0);
  if (nptrs < 0)
    {
      ptrs = &fallback;
      nptrs = 1;
    }
  StringVector symbols = pretty_backtrace_symbols (ptrs, nptrs);
  String where;
  if (file && file[0])
    where += file;
  if (!where.empty() && line > 0)
    where += string_format (":%u", line);
  if (func && func[0])
    {
      if (!where.empty())
        where += ":";
      where += func;
    }
  if (!where.empty())
    where = " (from " + where + ")";
  return string_format ("Backtrace at 0x%016x%s:\n  %s\n", ptrs[0], where, string_join ("\n  ", symbols));
}

} // Bse
