// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __BSE_PLATFORM_HH__
#define __BSE_PLATFORM_HH__

#include <bse/cxxaux.hh>
#include <thread>

namespace Bse {

// == Translate i18n strings ==
const char* (_) (const char *string) __attribute__ ((__format_arg__ (1)));
std::string (_) (const std::string &string);
const char* (_) (const char *string, const char *plural, int64_t n) __attribute__ ((__format_arg__ (1), __format_arg__ (2)));
std::string (_) (const std::string &string, const std::string &plural, int64_t n);

// == INSTALLPATH ==
enum class RPath {
  PREFIXDIR = 1,
  INSTALLDIR,
  LOCALEDIR,
  DRIVERDIR,
  PLUGINDIR,
  IMAGEDIR,
  DOCDIR,
  DEMODIR,
  EFFECTDIR,
  INSTRUMENTDIR,
  SAMPLEDIR,
  LADSPADIRS,
};
std::string runpath (RPath rpath);      ///< Retrieve various resource paths at runtime.

// == AnsiColors ==
/// The AnsiColors namespace contains utility functions for colored terminal output
namespace AnsiColors {
/// ANSI color symbols.
enum Colors {
  NONE,
  RESET,                ///< Reset combines BOLD_OFF, ITALICS_OFF, UNDERLINE_OFF, INVERSE_OFF, STRIKETHROUGH_OFF.
  BOLD, BOLD_OFF,
  ITALICS, ITALICS_OFF,
  UNDERLINE, UNDERLINE_OFF,
  INVERSE, INVERSE_OFF,
  STRIKETHROUGH, STRIKETHROUGH_OFF,
  FG_BLACK, FG_RED, FG_GREEN, FG_YELLOW, FG_BLUE, FG_MAGENTA, FG_CYAN, FG_WHITE,
  FG_DEFAULT,
  BG_BLACK, BG_RED, BG_GREEN, BG_YELLOW, BG_BLUE, BG_MAGENTA, BG_CYAN, BG_WHITE,
  BG_DEFAULT,
};
enum class Colorize : int8 { NEVER = 0, ALWAYS = 1, AUTO = 2 };
const char*     color_code      (Colors acolor);
std::string     color           (Colors acolor, Colors c1 = Colors::NONE, Colors c2 = Colors::NONE, Colors c3 = Colors::NONE,
                                 Colors c4 = Colors::NONE, Colors c5 = Colors::NONE, Colors c6 = Colors::NONE);
void            configure       (Colorize colorize);
bool            colorize_tty    (int fd = -1);
} // AnsiColors

// == Timestamp Handling ==
uint64  timestamp_startup    ();        // µseconds
uint64  timestamp_realtime   ();        // µseconds
uint64  timestamp_benchmark  ();        // nseconds
uint64  timestamp_resolution ();        // nseconds
String  timestamp_format     (uint64 stamp, uint maxlength = 8);
uint64  monotonic_counter    ();

// == process names ==
String      program_alias         ();                   ///< Retrieve the program name as used for logging or debug messages.
void        program_alias_init    (String customname);  ///< Set program_alias to a non-localized alias other than program_argv0 if desired.
String      application_name      ();                   ///< Retrieve the localized program name intended for user display.
void        application_name_init (String desktopname); ///< Set the application_name to a name other than program_alias if desired.
String      program_cwd           ();                   ///< The current working directory during startup.
std::string executable_name       () BSE_PURE;          ///< Retrieve the name part of executable_path().
std::string executable_path       () BSE_PURE;          ///< Retrieve the path to the currently running executable.
std::string cpu_info              ();                   ///< Retrieve string identifying the runtime CPU type.
std::string cpu_arch              ();                   ///< Retrieve string identifying the CPU architecture.
std::string version               ();                   ///< Provide a string containing the BSE library build.

// == Thread Status ==
/// Acquire information about a task (process or thread) at runtime.
struct TaskStatus {
  enum State { UNKNOWN = '?', RUNNING = 'R', SLEEPING = 'S', DISKWAIT = 'D', STOPPED = 'T', PAGING = 'W', ZOMBIE = 'Z', DEBUG = 'X', };
  int           process_id;     ///< Process ID.
  int           task_id;        ///< Process ID or thread ID.
  String        name;           ///< Thread name (set by user).
  State         state;          ///< Thread state.
  int           processor;      ///< Rrunning processor number.
  int           priority;       ///< Priority or nice value.
  uint64        utime;          ///< Userspace time.
  uint64        stime;          ///< System time.
  uint64        cutime;         ///< Userspace time of dead children.
  uint64        cstime;         ///< System time of dead children.
  uint64        ac_stamp;       ///< Accounting stamp.
  uint64        ac_utime, ac_stime, ac_cutime, ac_cstime;
  explicit      TaskStatus (int pid, int tid = -1); ///< Construct from process ID and optionally thread ID.
  bool          update     ();  ///< Update status information, might return false if called too frequently.
  String        string     ();  ///< Retrieve string representation of the status information.
};

// == Thread Info ==
using ThreadId = std::thread::id;
ThreadId this_thread_self        ();
void     this_thread_set_name    (const String &name16chars);
String   this_thread_get_name    ();
int      this_thread_getpid      ();
int      this_thread_gettid      ();
int      this_thread_online_cpus ();

// == Debugging Aids ==
extern inline void breakpoint               () BSE_ALWAYS_INLINE;       ///< Cause a debugging breakpoint, for development only.

// == Memory Barriers ==
#if defined __x86_64__ || defined __amd64__
#define  BSE_MFENCE __sync_synchronize()
#define  BSE_SFENCE __asm__ __volatile__ ("sfence" ::: "memory")
#define  BSE_LFENCE __asm__ __volatile__ ("lfence" ::: "memory")
#else // !x86/64
#define  BSE_SFENCE __sync_synchronize() ///< Store Fence - prevent processor (and compiler) from reordering stores (write barrier).
#define  BSE_LFENCE __sync_synchronize() ///< Load Fence  - prevent processor (and compiler) from reordering loads (read barrier).
/// Memory Fence - prevent processor (and compiler) from reordering loads/stores (read/write barrier), see also std::atomic_thread_fence().
#define  BSE_MFENCE __sync_synchronize()
#endif
/// Compiler Fence, prevent compiler from reordering non-volatile loads/stores, see also std::atomic_signal_fence().
#define  BSE_CFENCE __asm__ __volatile__ ("" ::: "memory")

// == Implementation Details ==
#if (defined __i386__ || defined __x86_64__)
inline void breakpoint() { __asm__ __volatile__ ("int $03"); }
#elif defined __alpha__ && !defined __osf__
inline void breakpoint() { __asm__ __volatile__ ("bpt"); }
#else   // !__i386__ && !__alpha__
inline void breakpoint() { __builtin_trap(); }
#endif

} // Bse

#endif // __BSE_PLATFORM_HH__
