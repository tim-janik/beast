// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "platform.hh"
#include "path.hh"
#include <unistd.h>
#include <cstring>
#include <setjmp.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/syscall.h>        // SYS_gettid
#if defined __APPLE__
#include <mach-o/dyld.h>        // _NSGetExecutablePath
#endif // __APPLE__
#ifdef  _WIN32                  // includes _WIN64
#include <windows.h>
#endif  // _WIN32

namespace Bse {

// == AnsiColors ==
namespace AnsiColors {

static std::atomic<Colorize> colorize_cache { Colorize::AUTO };

/// Override the environment variable $BSE_COLOR (which may contain "always", "never" or "auto").
void
configure (Colorize colorize)
{
  atomic_store (&colorize_cache, colorize);
}

/// Check whether the tty @a fd should use colorization, checks BSE_COLOR if @a fd == -1.
bool
colorize_tty (int fd)
{
  if (fd >= 0)
    return isatty (fd);
  const Colorize colcache = atomic_load (&colorize_cache);
  return_unless (colcache == Colorize::AUTO, bool (colcache));
  const char *ev = getenv ("BSE_COLOR");
  if (ev)
    {
      while (ev[0] == ' ')
        ev++;
      if (strncasecmp (ev, "always", 6) == 0)
        {
          atomic_store (&colorize_cache, Colorize::ALWAYS);
          return true;
        }
      else if (strncasecmp (ev, "never", 5) == 0)
        {
          atomic_store (&colorize_cache, Colorize::NEVER);
          return false;
        }
    }
  const bool tty_output = isatty (1) && isatty (2);
  atomic_store (&colorize_cache, tty_output ? Colorize::ALWAYS : Colorize::NEVER);
  return tty_output;
}

/// Return ANSI code for the specified color if stdout & stderr should be colorized, see colorize_tty().
const char*
color (Colors acolor)
{
  return colorize_tty() ? color_code (acolor) : "";
}

/// Return ANSI code for the specified color.
const char*
color_code (Colors acolor)
{
  switch (acolor)
    {
    default: ;
    case NONE:              return "";
    case RESET:             return "\033[0m";
    case BOLD:              return "\033[1m";
    case BOLD_OFF:          return "\033[22m";
    case ITALICS:           return "\033[3m";
    case ITALICS_OFF:       return "\033[23m";
    case UNDERLINE:         return "\033[4m";
    case UNDERLINE_OFF:     return "\033[24m";
    case INVERSE:           return "\033[7m";
    case INVERSE_OFF:       return "\033[27m";
    case STRIKETHROUGH:     return "\033[9m";
    case STRIKETHROUGH_OFF: return "\033[29m";
    case FG_BLACK:          return "\033[30m";
    case FG_RED:            return "\033[31m";
    case FG_GREEN:          return "\033[32m";
    case FG_YELLOW:         return "\033[33m";
    case FG_BLUE:           return "\033[34m";
    case FG_MAGENTA:        return "\033[35m";
    case FG_CYAN:           return "\033[36m";
    case FG_WHITE:          return "\033[37m";
    case FG_DEFAULT:        return "\033[39m";
    case BG_BLACK:          return "\033[40m";
    case BG_RED:            return "\033[41m";
    case BG_GREEN:          return "\033[42m";
    case BG_YELLOW:         return "\033[43m";
    case BG_BLUE:           return "\033[44m";
    case BG_MAGENTA:        return "\033[45m";
    case BG_CYAN:           return "\033[46m";
    case BG_WHITE:          return "\033[47m";
    case BG_DEFAULT:        return "\033[49m";
    }
}

} // AnsiColors


// == cpu_info ==
// figure architecture name from compiler
static const char*
get_arch_name (void)
{
#if     defined  __alpha__
  return "Alpha";
#elif   defined __frv__
  return "frv";
#elif   defined __s390__
  return "s390";
#elif   defined __m32c__
  return "m32c";
#elif   defined sparc
  return "Sparc";
#elif   defined __m32r__
  return "m32r";
#elif   defined __x86_64__ || defined __amd64__
  return "AMD64";
#elif   defined __ia64__
  return "Intel Itanium";
#elif   defined __m68k__
  return "mc68000";
#elif   defined __powerpc__ || defined PPC || defined powerpc || defined __PPC__
  return "PPC";
#elif   defined __arc__
  return "arc";
#elif   defined __arm__
  return "Arm";
#elif   defined __mips__ || defined mips
  return "Mips";
#elif   defined __tune_i686__ || defined __i686__
  return "i686";
#elif   defined __tune_i586__ || defined __i586__
  return "i586";
#elif   defined __tune_i486__ || defined __i486__
  return "i486";
#elif   defined i386 || defined __i386__
  return "i386";
#else
  return "unknown-arch";
#warning platform.cc needs updating for this processor type
#endif
}

/// Acquire information about the runtime architecture and CPU type.
struct CPUInfo {
  // architecture name
  const char *machine;
  // CPU Vendor ID
  char cpu_vendor[13];
  // CPU features on X86
  uint x86_fpu : 1, x86_ssesys : 1, x86_tsc   : 1, x86_htt      : 1;
  uint x86_mmx : 1, x86_mmxext : 1, x86_3dnow : 1, x86_3dnowext : 1;
  uint x86_sse : 1, x86_sse2   : 1, x86_sse3  : 1, x86_ssse3    : 1;
  uint x86_cx16 : 1, x86_sse4_1 : 1, x86_sse4_2 : 1, x86_rdrand : 1;
};

static jmp_buf cpu_info_jmp_buf;

static void BSE_NORETURN
cpu_info_sigill_handler (int dummy)
{
  longjmp (cpu_info_jmp_buf, 1);
}

#if     defined __i386__
#  define x86_has_cpuid()       ({                              \
  unsigned int __eax = 0, __ecx = 0;                            \
  __asm__ __volatile__                                          \
    (                                                           \
     /* copy EFLAGS into eax and ecx */                         \
     "pushf ; pop %0 ; mov %0, %1 \n\t"                         \
     /* toggle the ID bit and store back to EFLAGS */           \
     "xor $0x200000, %0 ; push %0 ; popf \n\t"                  \
     /* read back EFLAGS with possibly modified ID bit */       \
     "pushf ; pop %0 \n\t"                                      \
     : "=a" (__eax), "=c" (__ecx)                               \
     : /* no inputs */                                          \
     : "cc"                                                     \
     );                                                         \
  bool __result = (__eax ^ __ecx) & 0x00200000;                 \
  __result;                                                     \
})
/* save EBX around CPUID, because gcc doesn't like it to be clobbered with -fPIC */
#  define x86_cpuid(input, count, eax, ebx, ecx, edx) \
  __asm__ __volatile__ (                        \
    /* save ebx in esi */                       \
    "mov %%ebx, %%esi \n\t"                     \
    /* get CPUID with eax=input */              \
    "cpuid \n\t"                                \
    /* swap ebx and esi */                      \
    "xchg %%ebx, %%esi"                         \
    : "=a" (eax), "=S" (ebx),                   \
      "=c" (ecx), "=d" (edx)                    \
    : "0" (input), "2" (count)                  \
    : "cc")
#elif   defined __x86_64__ || defined __amd64__
/* CPUID is always present on AMD64, see:
 * http://www.amd.com/us-en/assets/content_type/white_papers_and_tech_docs/24594.pdf
 * "AMD64 Architecture Programmer's Manual Volume 3",
 * "Appendix D: Instruction Subsets and CPUID Feature Sets"
 */
#  define x86_has_cpuid()                       (1)
/* save EBX around CPUID, because gcc doesn't like it to be clobbered with -fPIC */
#  define x86_cpuid(input, count, eax, ebx, ecx, edx) \
  __asm__ __volatile__ (                        \
    /* save ebx in esi */                       \
    "mov %%rbx, %%rsi \n\t"                     \
    /* get CPUID with eax=input */              \
    "cpuid \n\t"                                \
    /* swap ebx and esi */                      \
    "xchg %%rbx, %%rsi"                         \
    : "=a" (eax), "=S" (ebx),                   \
      "=c" (ecx), "=d" (edx)                    \
    : "0" (input), "2" (count)                  \
    : "cc")
#else
#  define x86_has_cpuid()                       (false)
#  define x86_cpuid(input, count, eax, ebx, ecx, edx)  do {} while (0)
#endif

static bool
get_x86_cpu_features (CPUInfo *ci)
{
  memset (ci, 0, sizeof (*ci));
  /* check if the CPUID instruction is supported */
  if (!x86_has_cpuid ())
    return false;

  /* query intel CPUID range */
  unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
  x86_cpuid (0, 0, eax, ebx, ecx, edx);
  unsigned int v_ebx = ebx, v_ecx = ecx, v_edx = edx;
  char *vendor = ci->cpu_vendor;
  *((unsigned int*) &vendor[0]) = ebx;
  *((unsigned int*) &vendor[4]) = edx;
  *((unsigned int*) &vendor[8]) = ecx;
  vendor[12] = 0;
  if (eax >= 1)                 /* may query version and feature information */
    {
      x86_cpuid (1, 0, eax, ebx, ecx, edx);
      if (ecx & (1 << 0))
        ci->x86_sse3 = true;
      if (ecx & (1 << 9))
        ci->x86_ssse3 = true;
      if (ecx & (1 << 13))
        ci->x86_cx16 = true;
      if (ecx & (1 << 19))
        ci->x86_sse4_1 = true;
      if (ecx & (1 << 20))
        ci->x86_sse4_2 = true;
      if (ecx & (1 << 30))
        ci->x86_rdrand = true;
      if (edx & (1 << 0))
        ci->x86_fpu = true;
      if (edx & (1 << 4))
        ci->x86_tsc = true;
      if (edx & (1 << 23))
        ci->x86_mmx = true;
      if (edx & (1 << 25))
        {
          ci->x86_sse = true;
          ci->x86_mmxext = true;
        }
      if (edx & (1 << 26))
        ci->x86_sse2 = true;
      if (edx & (1 << 28))
        ci->x86_htt = true;
      /* http://www.intel.com/content/www/us/en/processors/processor-identification-cpuid-instruction-note.html
       * "Intel Processor Identification and the CPUID Instruction"
       */
    }

  /* query extended CPUID range */
  x86_cpuid (0x80000000, 0, eax, ebx, ecx, edx);
  if (eax >= 0x80000001 &&      /* may query extended feature information */
      v_ebx == 0x68747541 &&    /* AuthenticAMD */
      v_ecx == 0x444d4163 && v_edx == 0x69746e65)
    {
      x86_cpuid (0x80000001, 0, eax, ebx, ecx, edx);
      if (edx & (1 << 31))
        ci->x86_3dnow = true;
      if (edx & (1 << 22))
        ci->x86_mmxext = true;
      if (edx & (1 << 30))
        ci->x86_3dnowext = true;
      /* www.amd.com/us-en/assets/content_type/white_papers_and_tech_docs/25481.pdf
       * "AMD CPUID Specification"
       */
    }

  /* check system support for SSE */
  if (ci->x86_sse)
    {
      struct sigaction action, old_action;
      action.sa_handler = cpu_info_sigill_handler;
      sigemptyset (&action.sa_mask);
      action.sa_flags = SA_NOMASK;
      sigaction (SIGILL, &action, &old_action);
      if (setjmp (cpu_info_jmp_buf) == 0)
        {
#if     defined __i386__ || defined __x86_64__ || defined __amd64__
          unsigned int mxcsr;
          __asm__ __volatile__ ("stmxcsr %0 ; sfence ; emms" : "=m" (mxcsr));
          /* executed SIMD instructions without exception */
          ci->x86_ssesys = true;
#endif // x86
        }
      else
        {
          /* signal handler jumped here */
          // g_printerr ("caught SIGILL\n");
        }
      sigaction (SIGILL, &old_action, NULL);
    }

  return true;
}

/** The returned string contains: number of online CPUs, a string
 * describing the CPU architecture, the vendor and finally
 * a number of flag words describing CPU features plus a trailing space.
 * This allows checks for CPU features via a simple string search for
 * " FEATURE ".
 * @return Example: "4 AMD64 GenuineIntel FPU TSC HTT CMPXCHG16B MMX MMXEXT SSESYS SSE SSE2 SSE3 SSSE3 SSE4.1 SSE4.2 "
 */
String
cpu_info()
{
  static String cpu_info_string = []() {
    CPUInfo cpu_info = { 0, };
    if (!get_x86_cpu_features (&cpu_info))
      strcat (cpu_info.cpu_vendor, "Unknown");
    cpu_info.machine = get_arch_name();
    String info;
    // cores
    info += string_format ("%d", sysconf (_SC_NPROCESSORS_ONLN));
    // architecture
    info += String (" ") + cpu_info.machine;
    // vendor
    info += String (" ") + cpu_info.cpu_vendor;
    // processor flags
    if (cpu_info.x86_fpu)
      info += " FPU";
    if (cpu_info.x86_tsc)
      info += " TSC";
    if (cpu_info.x86_htt)
      info += " HTT";
    if (cpu_info.x86_cx16)
      info += " CMPXCHG16B";
    // MMX flags
    if (cpu_info.x86_mmx)
      info += " MMX";
    if (cpu_info.x86_mmxext)
      info += " MMXEXT";
    // SSE flags
    if (cpu_info.x86_ssesys)
      info += " SSESYS";
    if (cpu_info.x86_sse)
      info += " SSE";
    if (cpu_info.x86_sse2)
      info += " SSE2";
    if (cpu_info.x86_sse3)
      info += " SSE3";
    if (cpu_info.x86_ssse3)
      info += " SSSE3";
    if (cpu_info.x86_sse4_1)
      info += " SSE4.1";
    if (cpu_info.x86_sse4_2)
      info += " SSE4.2";
    if (cpu_info.x86_rdrand)
      info += " rdrand";
    // 3DNOW flags
    if (cpu_info.x86_3dnow)
      info += " 3DNOW";
    if (cpu_info.x86_3dnowext)
      info += " 3DNOWEXT";
    info += " ";
    return String (info.c_str());
  }();
  return cpu_info_string;
}

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
  const ssize_t max_size = 8100;
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
  { // int _NSGetExecutablePath(char* buf, uint32_t* bufsize);
    uint32_t bufsize = max_size;
    if (_NSGetExecutablePath (system_result, &bufsize) == 0 &&
        bufsize <= max_size)
      system_result_size = bufsize;
  }
#elif defined _WIN32
  // DWORD GetModuleFileNameA (HMODULE hModule, LPSTR lpFileName, DWORD size);
  system_result_size = GetModuleFileNameA (0, system_result, max_size);
  if (system_result_size <= 0 || system_result_size >= max_size)
    system_result_size = -1;    // error, possibly not enough space
  else
    {
      system_result[system_result_size] = 0;
      // early conversion to unix slashes
      char *winslash;
      while ((winslash = strchr (system_result, '\\')) != NULL)
        *winslash = '/';
    }
#else
#error "Platform lacks executable_path() implementation"
#endif

  if (system_result_size < 0)
    system_result[0] = 0;
  return std::string (system_result);
}

/// Retrieve the path to the currently running executable.
std::string
executable_path()
{
  static std::string cached_executable_path = get_executable_path();
  return cached_executable_path;
}

//// Retrieve the name part of executable_path().
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

} // Bse
