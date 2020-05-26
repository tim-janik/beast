// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <bse/testing.hh>
#include <bse/bsemain.hh>
#include <bse/bse.hh>
#include <bse/bseserver.hh>
#include "ipc.hh"
#include "testresampler.hh"
#include "bse/internal.hh" // for Bse::Test::IntegrityCheck integration

#define DEBUG(...)              do { break; Bse::printerr (__VA_ARGS__); } while (0)

static int      jobserver (const char *const argv0, Bse::StringVector &tests);
static int      jobclient (int jobfd);

static void
print_int_ring (SfiRing *ring)
{
  SfiRing *node;
  TNOTE ("{");
  for (node = ring; node; node = sfi_ring_walk (node, ring))
    TNOTE ("%c", char (size_t (node->data)));
  TNOTE ("}");
}

static gint
ints_cmp (gconstpointer d1,
	  gconstpointer d2,
          gpointer      data)
{
  size_t i1 = size_t (d1);
  size_t i2 = size_t (d2);
  return i1 < i2 ? -1 : i1 > i2;
}

static void
test_sfi_ring (void)
{
  gint data_array[][64] = {
    { 0, },
    { 1, 'a', },
    { 2, 'a', 'a', },
    { 2, 'a', 'b', },
    { 2, 'z', 'a', },
    { 3, 'a', 'c', 'z' },
    { 3, 'a', 'z', 'c' },
    { 3, 'c', 'a', 'z' },
    { 3, 'z', 'c', 'a' },
    { 3, 'a', 'a', 'a' },
    { 3, 'a', 'a', 'z' },
    { 3, 'a', 'z', 'a' },
    { 3, 'z', 'a', 'a' },
    { 10, 'g', 's', 't', 'y', 'x', 'q', 'i', 'n', 'j', 'a' },
    { 15, 'w', 'k', 't', 'o', 'c', 's', 'j', 'd', 'd', 'q', 'p', 'v', 'q', 'r', 'a' },
    { 26, 'z', 'y', 'x', 'w', 'v', 'u', 't', 's', 'r', 'q', 'p', 'o', 'n', 'm'
      ,   'l', 'k', 'j', 'i', 'h', 'g', 'f', 'e', 'd', 'c', 'b', 'a', },
  };

  for (uint n = 0; n < G_N_ELEMENTS (data_array); n++)
    {
      uint l = data_array[n][0];
      SfiRing *ring = NULL;
      for (uint i = 1; i <= l; i++)
	ring = sfi_ring_append (ring, (void*) size_t (data_array[n][i]));
      TNOTE ("source: ");
      print_int_ring (ring);
      ring = sfi_ring_sort (ring, ints_cmp, NULL);
      TNOTE (" sorted: ");
      print_int_ring (ring);
      TNOTE ("\n");
      sfi_ring_free (ring);
    }
}
TEST_ADD (test_sfi_ring);

static void
bench_aida()
{
  using namespace Bse;
  double calls = 0, slowest = 0, fastest = 9e+9;
  for (uint j = 0; j < 37; j++)
    {
      Bse::jobs += [] () { BSE_SERVER.test_counter_set (0); };
      const int count = 2999;
      const uint64_t ts0 = timestamp_benchmark();
      for (int i = 0; i < count; i++)
        Bse::jobs += [] () { BSE_SERVER.test_counter_inc_fetch(); };
      const uint64 ts1 = timestamp_benchmark();
      TASSERT (count == (Bse::jobs += [] () { return BSE_SERVER.test_counter_get(); }));
      double t0 = ts0 / 1000000000.;
      double t1 = ts1 / 1000000000.;
      double call1 = (t1 - t0) / count;
      slowest = MAX (slowest, call1 * 1000000.);
      fastest = MIN (fastest, call1 * 1000000.);
      double this_calls = 1 / call1;
      calls = MAX (calls, this_calls);
    }
  double err = (slowest - fastest) / slowest;
  printout ("  BENCH    Aida: %g calls/s; fastest: %.2fus; slowest: %.2fus; err: %.2f%%\n",
            calls, fastest, slowest, err * 100);
}

// Override weak symbol to enable integrity tests
namespace Bse::Test {
/* NOT __weak__: */ const bool IntegrityCheck::enable_testing = true; // see internal.hh
} // Bse::Test

static int
test_main (int argc, char *argv[])
{
  // Parse args
  Bse::Test::TestEntries test_entries;
  int64 jobs = 0;
  int jobfd = -1;
  int64 testflags = 0;

  for (ssize_t i = 1; i < argc; i++)
    if (argv[i])
      {
        if (std::string ("--broken") == argv[i])
          {
            testflags |= Bse::Test::BROKEN;
          }
        else if (std::string ("--slow") == argv[i])
          {
            testflags |= Bse::Test::SLOW;
          }
        else if (std::string ("--bench") == argv[i])
          {
            testflags |= Bse::Test::BENCH;
          }
        else if (std::string ("--jobfd") == argv[i] && i + 1 < argc)
          {
            jobfd = Bse::string_to_int (argv[++i]);
          }
        else if (std::string ("-j") == argv[i])
          {
            jobs = 1;
          }
        else if ('-' == argv[i][0])
          {
            Bse::printerr ("%s: unknown option: %s\n", argv[0], argv[i]);
            return 7;
          }
        else
          test_entries.push_back (Bse::Test::TestEntry (argv[i]));
      }

  // Process test list fetched via IPC
  if (jobfd != -1)
    return jobclient (jobfd);

  // Fallback to run *all* tests
  if (test_entries.size() == 0)
    test_entries = Bse::Test::list_tests();

  int serverstatus = 0;
  if (jobs)
    {
      Bse::StringVector tests;
      for (const Bse::Test::TestEntry &entry : test_entries)
        if (0 == (entry.flags & ~testflags) ||
            (testflags == 0 && (entry.flags & Bse::Test::INTEGRITY)))
          tests.push_back (entry.ident);
      serverstatus = jobserver (argv[0], tests);
    }
  else
    for (Bse::Test::TestEntry entry : test_entries)
      if (0 == (entry.flags & ~testflags) ||
          (testflags == 0 && (entry.flags & Bse::Test::INTEGRITY)))
        {
          const int result = Bse::Test::run_test (entry.ident);
          if (result < 0)
            {
              Bse::printout ("  RUN…     %s\n", entry.ident);
              Bse::printout ("  FAIL     %s - test missing\n", entry.ident);
              serverstatus = -1;
              break;
            }
        }
  Bse::printout ("  EXIT     %d - %s\n", serverstatus, serverstatus ? "FAIL" : "OK");
  return serverstatus;
}

int
main (int argc, char *argv[])
{
  Bse::StringVector args = Bse::init_args (&argc, argv);
  args.push_back ("stand-alone=1");
  Bse::set_debug_flags (Bse::DebugFlags::SIGQUIT_ON_ABORT);
  assert_return (Bse::Test::IntegrityCheck::enable_testing == true, -1);
  // special case aida-bench
  if (argc >= 2 && argv[1] && std::string ("--aida-bench") == argv[1])
    {
      Bse::init_async (argv[0], args);
      bench_aida();
      return 0;
    }
  if (argc >= 2 && argv[1] && std::string ("--resampler") == argv[1])
    {
      Bse::init_async (argv[0], args);
      return test_resampler (argc, argv);
    }
  // initialize BSE and continue in BSE thread
  const int status = Bse::init_and_test (args, [&]() {
    // check-assertions
    if (argc >= 2 && String ("--assert_return1") == argv[1])
      {
        assert_return (1, 0);
        return 0;
      }
    else if (argc >= 2 && String ("--assert_return0") == argv[1])
      {
        assert_return (0, 0);
        return 0;
      }
    else if (argc >= 2 && String ("--assert_return_unreached") == argv[1])
      {
        assert_return_unreached (0);
        return 0;
      }
    else if (argc >= 2 && String ("--fatal_error") == argv[1])
      {
        Bse::fatal_error ("got argument --fatal_error");
        return 0;
      }
    else if (argc >= 2 && String ("--return_unless0") == argv[1])
      {
        return_unless (0, 7);
        return 0;
      }
    else if (argc >= 2 && String ("--return_unless1") == argv[1])
      {
        return_unless (1, 8);
        return 0;
      }
    // run tests
    return test_main (argc, argv);
  });
  // clean shutdown
  if (status == 0)
    Bse::shutdown_async();
  return status;
}

static int
jobclient (int jobfd)
{
  IpcSharedMem *sm = IpcSharedMem::acquire_shared (jobfd);
  Bse::StringVector tests;
  const std::string testlist = sm->get_string();
  for (const auto &line : Bse::string_split (testlist, "\n"))
    if (!line.empty())
      tests.push_back (line);
  for (int64 v = sm->counter.fetch_add (-1); v > 0; v = sm->counter.fetch_add (-1))
    {
      const size_t test_index = v - 1;
      TASSERT (test_index < tests.size());
      const int result = Bse::Test::run_test (tests[test_index]);
      if (result < 0)
        {
          Bse::printout ("  RUN…     %s\n", tests[test_index]);
          Bse::printout ("  FAIL     %s - test missing\n", tests[test_index]);
          return -1;
        }
    }
  IpcSharedMem::release_shared (sm, jobfd);
  DEBUG ("JOBDONE (%u)\n", Bse::this_thread_getpid());
  return 0;
}

#include <unistd.h>     // fork exec
#include <sys/wait.h>   // waitpid

static int
jobserver (const char *const argv0, Bse::StringVector &tests)
{
  std::reverse (tests.begin(), tests.end());    // the IpcSharedMem counter is decremented
  const std::string testlist = Bse::string_join ("\n", tests);
  int fd = -1;
  IpcSharedMem *sm = IpcSharedMem::create_shared (testlist, &fd, tests.size());
  if (fd < 0 || !sm)
    die ("failed to create shared memory segment: %s", Bse::strerror (errno));
  DEBUG ("jobserver: pid=%u\n", Bse::this_thread_getpid());
  const size_t n_jobs = std::max (1, Bse::this_thread_online_cpus());
  std::vector<pid_t> jobs;
  for (size_t i = 0; i < n_jobs; i++)
    if (sm->counter > 0)        // avoid spawning jobs if there's no work
      {
        const pid_t child = fork();
        if (child == 0)
          {
            execl (argv0, argv0, "--jobfd", Bse::string_from_int (fd).c_str(), NULL);
            die ("failed to execl(\"%s\"): %s", argv0, Bse::strerror (errno));
          }
        else if (child < 0)
          {
            DEBUG ("%s: failed to fork: %s\n", argv0, Bse::strerror (errno));
          }
        else
          jobs.push_back (child);
      }
  int serverstatus = 0;
  while (jobs.size())
    {
      int wstatus = 0;
      const int pid = waitpid (-1, &wstatus, 0);
      if (pid < 0)
        {
          DEBUG ("%s: waitpid(%d) failed: %s\n", argv0, pid, Bse::strerror (errno));
          if (errno == ECHILD)
            abort(); // lost track of jobs
          continue;
        }
      const int exitstatus = WIFEXITED (wstatus) ? WEXITSTATUS (wstatus) :
                             WIFSIGNALED (wstatus) ? -WTERMSIG (wstatus) :
                             -128;
      auto it = std::find (jobs.begin(), jobs.end(), pid);
      if (it == jobs.end())
        DEBUG ("%s: orphan (%d) exited: %d\n", argv0, pid, exitstatus);
      else
        {
          jobs.erase (it);
          if (!serverstatus)
            serverstatus = exitstatus;
          DEBUG ("%s: child (%d) exited: %d\n", argv0, pid, exitstatus);
        }
    }
  const ssize_t remaining_tests = sm->counter;
  TASSERT (remaining_tests <= 0);
  IpcSharedMem::destroy_shared (sm, fd);
  return serverstatus;
}
