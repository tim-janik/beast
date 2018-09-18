// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "entropy.hh"
#include "randomhash.hh"
#include <random>
#include <setjmp.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <sys/time.h>           // gettimeofday
#include <linux/random.h>       // GRND_NONBLOCK
#include <sys/syscall.h>        // __NR_getrandom
#include <sys/utsname.h>        // uname
#if defined (__i386__) || defined (__x86_64__)
#  include <x86intrin.h>        // __rdtsc
#endif

namespace Bse {

static int
getrandom (void *buffer, size_t count, unsigned flags)
{
#ifdef __NR_getrandom
  const long ret = syscall (__NR_getrandom, buffer, count, flags);
  if (ret > 0)
    return ret;
#endif
  FILE *file = fopen ("/dev/urandom", "r");     // fallback
  if (file)
    {
      const size_t l = fread (buffer, 1, count, file);
      fclose (file);
      if (l > 0)
        return 0;
    }
  errno = ENOSYS;
  return 0;
}

static bool
hash_getrandom (KeccakRng &pool)
{
  uint64_t buffer[25];
  int flags = 0;
#ifdef GRND_NONBLOCK
  flags |= GRND_NONBLOCK;
#endif
  int l = getrandom (buffer, sizeof (buffer), flags);
  if (l > 0)
    {
      pool.xor_seed (buffer, l / sizeof (buffer[0]));
      return true;
    }
  return false;
}

template<class Data> static void
hash_anything (KeccakRng &pool, const Data &data)
{
  const uint64_t *d64 = (const uint64_t*) &data;
  uint len = sizeof (data);
  uint64_t dummy;
  if (sizeof (Data) < sizeof (uint64_t))
    {
      dummy = 0;
      memcpy (&dummy, &data, sizeof (Data));
      d64 = &dummy;
      len = 1;
    }
  pool.xor_seed (d64, len / sizeof (d64[0]));
}

static bool
hash_macs (KeccakRng &pool)
{
  // query devices for the AF_INET family which might be the only one supported
  int sockfd = socket (AF_INET, SOCK_DGRAM, 0);         // open IPv4 UDP socket
  if (sockfd < 0)
    return false;
  // discover devices by index, might include devices that are 'down'
  String devices;
  int ret = 0;
  for (size_t j = 0; j <= 1 || ret >= 0; j++)           // try [0] and [1]
    {
      struct ifreq iftmp = { { { 0 }, }, };
      iftmp.ifr_ifindex = j;
      ret = ioctl (sockfd, SIOCGIFNAME, &iftmp);
      if (ret < 0)
        continue;
      if (!devices.empty())
        devices += ",";
      devices += iftmp.ifr_name;                        // found name
      devices += "//";                                  // no inet address
      if (ioctl (sockfd, SIOCGIFHWADDR, &iftmp) >= 0)   // query MAC
        {
          const uint8_t *mac = (const uint8_t*) iftmp.ifr_hwaddr.sa_data;
          devices += string_format ("%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        }
    }
  // discover devices that are 'up'
  char ifcbuffer[8192] = { 0, };                        // buffer space for returning interfaces
  struct ifconf ifc;
  ifc.ifc_len = sizeof (ifcbuffer);
  ifc.ifc_buf = ifcbuffer;
  ret = ioctl (sockfd, SIOCGIFCONF, &ifc);
  for (size_t i = 0; ret >= 0 && i < ifc.ifc_len / sizeof (struct ifreq); i++)
    {
      const struct ifreq *iface = &ifc.ifc_req[i];
      if (!devices.empty())
        devices += ",";
      devices += iface->ifr_name;                       // found name
      devices += "/";
      const uint8_t *addr = (const uint8_t*) &((struct sockaddr_in*) &iface->ifr_addr)->sin_addr;
      devices += string_format ("%u.%u.%u.%u", addr[0], addr[1], addr[2], addr[3]);
      devices += "/";                                   // added inet address
      if (ioctl (sockfd, SIOCGIFHWADDR, iface) >= 0)    // query MAC
        {
          const uint8_t *mac = (const uint8_t*) iface->ifr_hwaddr.sa_data;
          devices += string_format ("%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        }
    }
  close (sockfd);
  devices.resize (((devices.size() + 7) / 8) * 8); // align to uint64_t
  pool.xor_seed ((const uint64_t*) devices.data(), devices.size() / 8);
  // printerr ("SEED(MACs): %s\n", devices.c_str());
  return !devices.empty();
}

static bool
hash_stat (KeccakRng &pool, const char *filename)
{
  struct {
    struct stat stat;
    uint64_t padding;
  } s = { { 0 }, };
  if (stat (filename, &s.stat) == 0)
    {
      pool.xor_seed ((const uint64_t*) &s.stat, sizeof (s.stat) / sizeof (uint64_t));
      // printerr ("SEED(%s): atime=%u mtime=%u ctime=%u size=%u...\n", filename, s.stat.st_atime, s.stat.st_atime, s.stat.st_atime, s.stat.st_size);
      return true;
    }
  return false;
}

static bool
hash_file (KeccakRng &pool, const char *filename, const size_t maxbytes = 16384)
{
  FILE *file = fopen (filename, "r");
  if (file)
    {
      uint64_t buffer[maxbytes / 8];
      const size_t l = fread (buffer, sizeof (buffer[0]), sizeof (buffer) / sizeof (buffer[0]), file);
      fclose (file);
      if (l > 0)
        {
          pool.xor_seed (buffer, l);
          // printerr ("SEED(%s): %s\n", filename, String ((const char*) buffer, std::min (l * 8, size_t (48))));
          return true;
        }
    }
  return false;
}

static bool __attribute__ ((__unused__))
hash_glob (KeccakRng &pool, const char *fileglob, const size_t maxbytes = 16384)
{
  glob_t globbuf = { 0, };
  glob (fileglob, GLOB_NOSORT, NULL, &globbuf);
  bool success = false;
  for (size_t i = globbuf.gl_offs; i < globbuf.gl_pathc; i++)
    success |= hash_file (pool, globbuf.gl_pathv[i], maxbytes);
  globfree (&globbuf);
  return success;
}

struct HashStamp {
  uint64_t bstamp;
#ifdef  CLOCK_PROCESS_CPUTIME_ID
  uint64_t tcpu;
#endif
#if     defined (__i386__) || defined (__x86_64__)
  uint64_t tsc;
#endif
};

static void __attribute__ ((__noinline__))
hash_time (HashStamp *hstamp)
{
  asm (""); // enfore __noinline__
  hstamp->bstamp = timestamp_benchmark();
#ifdef  CLOCK_PROCESS_CPUTIME_ID
  {
    struct timespec ts;
    clock_gettime (CLOCK_PROCESS_CPUTIME_ID, &ts);
    hstamp->tcpu = ts.tv_sec * uint64_t (1000000000) + ts.tv_nsec;
  }
#endif
#if     defined (__i386__) || defined (__x86_64__)
  hstamp->tsc = __rdtsc();
#endif
}

static void
hash_cpu_usage (KeccakRng &pool)
{
  union {
    uint64_t      ui64[24];
    struct {
      struct rusage rusage;     // 144 bytes
      struct tms    tms;        //  32 bytes
      clock_t       clk;        //   8 bytes
    };
  } u = { { 0 }, };
  getrusage (RUSAGE_SELF, &u.rusage);
  u.clk = times (&u.tms);
  pool.xor_seed (u.ui64, sizeof (u.ui64) / sizeof (u.ui64[0]));
}

static void
hash_sys_structs (KeccakRng &pool)
{
  struct SysStructs {
    uint64 alignment_dummy1;
    struct timezone tz;
    struct timeval  tv;
    struct utsname uts;
    uint64 alignment_dummy2;
  };
  SysStructs sst = { 0, };
  gettimeofday (&sst.tv, &sst.tz);
  uname (&sst.uts);
  pool.xor_seed ((uint64*) &sst, sizeof (sst) / sizeof (uint64));
}

static bool
get_rdrand (uint64 *u, uint count)
{
#if defined (__i386__) || defined (__x86_64__)
  if (strstr (cpu_info().c_str(), " rdrand"))
    for (uint i = 0; i < count; i++)
      __asm__ __volatile__ ("rdrand %0" : "=r" (u[i]));
  else
    for (uint i = 0; i < count; i++)
      {
        uint64_t d = __rdtsc();       // fallback
        u[i] = pcg_hash64 ((const uint8*) &d, sizeof (d), 0xeaeaeaea113377ccULL);
      }
  return true;
#endif
  return false;
}

static void
get_arc4random (uint64 *u, uint count)
{
#ifdef __OpenBSD__
  for (uint i = 0; i < count; i++)
    u[i] = uint64_t (arc4random()) << 32;
  arc4random_stir();
  for (uint i = 0; i < count; i++)
    u[i] |= arc4random();
#endif
}

static void
runtime_entropy (KeccakRng &pool)
{
  HashStamp hash_stamps[64] = { { 0 }, };
  HashStamp *stamp = &hash_stamps[0];
  hash_time (stamp++);
  uint64_t uint_array[64] = { 0, };
  uint64_t *uintp = &uint_array[0];
  hash_time (stamp++);  *uintp++ = timestamp_realtime();
  hash_time (stamp++);  hash_cpu_usage (pool);
  hash_time (stamp++);  *uintp++ = timestamp_benchmark();
  hash_time (stamp++);  get_rdrand (uintp, 8); uintp += 8;
  hash_time (stamp++);  get_arc4random (uintp, 8); uintp += 8;
  hash_time (stamp++);  *uintp++ = this_thread_gettid();
  hash_time (stamp++);  *uintp++ = getuid();
  hash_time (stamp++);  *uintp++ = geteuid();
  hash_time (stamp++);  *uintp++ = getgid();
  hash_time (stamp++);  *uintp++ = getegid();
  hash_time (stamp++);  *uintp++ = getpid();
  hash_time (stamp++);  *uintp++ = getsid (0);
  int ppid;
  hash_time (stamp++);  *uintp++ = ppid = getppid();
  hash_time (stamp++);  *uintp++ = getsid (ppid);
  hash_time (stamp++);  *uintp++ = getpgrp();
  hash_time (stamp++);  *uintp++ = tcgetpgrp (0);
  hash_time (stamp++);  hash_getrandom (pool);
  hash_time (stamp++);  { *uintp++ = std::random_device()(); } // may open devices, so destroy early on
  hash_time (stamp++);  hash_anything (pool, std::chrono::high_resolution_clock::now().time_since_epoch().count());
  hash_time (stamp++);  hash_anything (pool, std::chrono::steady_clock::now().time_since_epoch().count());
  hash_time (stamp++);  hash_anything (pool, std::chrono::system_clock::now().time_since_epoch().count());
  hash_time (stamp++);  hash_anything (pool, std::this_thread::get_id());
  String compiletime = __DATE__ __TIME__ __FILE__ __TIMESTAMP__;
  hash_time (stamp++);  *uintp++ = fnv1a_consthash64 (compiletime.c_str());     // compilation entropy
  hash_time (stamp++);  *uintp++ = size_t (compiletime.data());                 // heap address
  static int dummy;
  hash_time (stamp++);  *uintp++ = size_t (&dummy);                             // data segment
  hash_time (stamp++);  *uintp++ = size_t ("PATH");                             // const data segment
  hash_time (stamp++);  *uintp++ = size_t (getenv ("PATH"));                    // a.out address
  hash_time (stamp++);  *uintp++ = size_t (&stamp);                             // stack segment
  hash_time (stamp++);  *uintp++ = size_t (&runtime_entropy);                   // code segment
  hash_time (stamp++);  *uintp++ = size_t (&::fopen);                           // libc code segment
  hash_time (stamp++);  *uintp++ = size_t (&std::string::npos);                 // stl address
  hash_time (stamp++);  *uintp++ = fnv1a_consthash64 (cpu_info().c_str());      // CPU type influence
  hash_time (stamp++);  hash_sys_structs (pool);
  hash_time (stamp++);  *uintp++ = timestamp_benchmark();
  hash_time (stamp++);  hash_cpu_usage (pool);
  hash_time (stamp++);  *uintp++ = timestamp_realtime();
  BSE_ASSERT_RETURN (uintp <= &uint_array[sizeof (uint_array) / sizeof (uint_array[0])]);
  pool.xor_seed (&uint_array[0], uintp - &uint_array[0]);
  hash_time (stamp++);
  BSE_ASSERT_RETURN (stamp <= &hash_stamps[sizeof (hash_stamps) / sizeof (hash_stamps[0])]);
  pool.xor_seed ((uint64_t*) &hash_stamps[0], (stamp - &hash_stamps[0]) * sizeof (hash_stamps[0]) / sizeof (uint64_t));
  // printerr ("%s(): duration=%fµsec\n", __func__, (stamp[-1].bstamp - hash_stamps[0].bstamp) / 1000.0);
}

static void
system_entropy (KeccakRng &pool)
{
  HashStamp hash_stamps[64] = { { 0 }, };
  HashStamp *stamp = &hash_stamps[0];
  hash_time (stamp++);
  uint64_t uint_array[64] = { 0, };
  uint64_t *uintp = &uint_array[0];
  hash_time (stamp++);  *uintp++ = timestamp_realtime();
  hash_time (stamp++);  *uintp++ = timestamp_benchmark();
  hash_time (stamp++);  *uintp++ = timestamp_startup();
  hash_time (stamp++);  hash_cpu_usage (pool);
  hash_time (stamp++);  hash_file (pool, "/proc/sys/kernel/random/boot_id");
  hash_time (stamp++);  hash_file (pool, "/proc/version");
  hash_time (stamp++);  hash_file (pool, "/proc/cpuinfo");
  hash_time (stamp++);  hash_file (pool, "/proc/devices");
  hash_time (stamp++);  hash_file (pool, "/proc/meminfo");
  hash_time (stamp++);  hash_file (pool, "/proc/buddyinfo");
  hash_time (stamp++);  hash_file (pool, "/proc/diskstats");
  hash_time (stamp++);  hash_file (pool, "/proc/1/stat");
  hash_time (stamp++);  hash_file (pool, "/proc/1/sched");
  hash_time (stamp++);  hash_file (pool, "/proc/1/schedstat");
  hash_time (stamp++);  hash_file (pool, "/proc/self/stat");
  hash_time (stamp++);  hash_file (pool, "/proc/self/sched");
  hash_time (stamp++);  hash_file (pool, "/proc/self/schedstat");
  hash_time (stamp++);  hash_macs (pool);
  // hash_glob: "/sys/devices/**/net/*/address", "/sys/devices/*/*/*/ieee80211/phy*/*address*"
  hash_time (stamp++);  hash_file (pool, "/proc/uptime");
  hash_time (stamp++);  hash_file (pool, "/proc/user_beancounters");
  hash_time (stamp++);  hash_file (pool, "/proc/driver/rtc");
  hash_time (stamp++);  hash_stat (pool, "/var/log/syslog");            // for mtime
  hash_time (stamp++);  hash_stat (pool, "/var/log/auth.log");          // for mtime
  hash_time (stamp++);  hash_stat (pool, "/var/tmp");                   // for mtime
  hash_time (stamp++);  hash_stat (pool, "/tmp");                       // for mtime
  hash_time (stamp++);  hash_stat (pool, "/dev");                       // for mtime
  hash_time (stamp++);  hash_stat (pool, "/var/lib/ntp/ntp.drift");     // for mtime
  hash_time (stamp++);  hash_stat (pool, "/var/run/utmp");              // for mtime & atime
  hash_time (stamp++);  hash_stat (pool, "/var/log/wtmp");              // for mtime & atime
  hash_time (stamp++);  hash_stat (pool, "/sbin/init");                 // for atime
  hash_time (stamp++);  hash_stat (pool, "/var/spool");                 // for atime
  hash_time (stamp++);  hash_stat (pool, "/var/spool/cron");            // for atime
  hash_time (stamp++);  hash_stat (pool, "/var/spool/anacron");         // for atime
  hash_time (stamp++);  hash_file (pool, "/dev/urandom", 400);
  hash_time (stamp++);  hash_file (pool, "/proc/sys/kernel/random/uuid");
  hash_time (stamp++);  hash_file (pool, "/proc/schedstat");
  hash_time (stamp++);  hash_file (pool, "/proc/sched_debug");
  hash_time (stamp++);  hash_file (pool, "/proc/fairsched");
  hash_time (stamp++);  hash_file (pool, "/proc/interrupts");
  hash_time (stamp++);  hash_file (pool, "/proc/loadavg");
  hash_time (stamp++);  hash_file (pool, "/proc/softirqs");
  hash_time (stamp++);  hash_file (pool, "/proc/stat");
  hash_time (stamp++);  hash_file (pool, "/proc/net/fib_triestat");
  hash_time (stamp++);  hash_file (pool, "/proc/net/netstat");
  hash_time (stamp++);  hash_file (pool, "/proc/net/dev");
  hash_time (stamp++);  hash_file (pool, "/proc/vz/vestat");
  hash_time (stamp++);  hash_cpu_usage (pool);
  hash_time (stamp++);  *uintp++ = timestamp_realtime();
  BSE_ASSERT_RETURN (uintp <= &uint_array[sizeof (uint_array) / sizeof (uint_array[0])]);
  pool.xor_seed (&uint_array[0], uintp - &uint_array[0]);
  hash_time (stamp++);
  BSE_ASSERT_RETURN (stamp <= &hash_stamps[sizeof (hash_stamps) / sizeof (hash_stamps[0])]);
  pool.xor_seed ((uint64_t*) &hash_stamps[0], (stamp - &hash_stamps[0]) * sizeof (hash_stamps[0]) / sizeof (uint64_t));
  // printerr ("%s(): duration=%fµsec\n", __func__, (stamp[-1].bstamp - hash_stamps[0].bstamp) / 1000.0);
}

/**
 * To provide good quality random number seeds, this function gathers entropy from a variety
 * of process specific sources. Under Linux, this includes the CPU counters, clocks and
 * random devices.
 * In combination with well established techniques like syscall timings (see Entropics13
 * @cite Entropics13) and a SHA3 algorithm derived random number generator for the mixing,
 * the entropy collection is designed to be fast and good enough for all non-cryptographic
 * uses.
 * On an Intel Core i7, this function takes around 25µs.
 */
void
collect_runtime_entropy (uint64 *data, size_t n)
{
  KeccakRng pool (128, 8);
  runtime_entropy (pool);
  for (size_t i = 0; i < n; i++)
    data[i] = pool();
}

/**
 * This function adds to collect_runtime_entropy() by collecting entropy from aditional
 * but potentially slower system sources, such as interrupt counters, disk + network statistics,
 * system load, execution + pipelining + scheduling latencies and device MACs.
 * The function is designed to yield random number seeds good enough to generate
 * cryptographic tokens like session keys.
 * On an Intel Core i7, this function takes around 2ms, so it's roughly 80 times slower
 * than collect_runtime_entropy().
 */
void
collect_system_entropy (uint64 *data, size_t n)
{
  KeccakRng pool (512, 24);
  hash_cpu_usage (pool);
  runtime_entropy (pool);
  system_entropy (pool);
  hash_cpu_usage (pool);
  for (size_t i = 0; i < n; i++)
    data[i] = pool();
}

} // Bse
