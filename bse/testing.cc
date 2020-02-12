// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "testing.hh"
#include "bse/internal.hh"
#include <algorithm>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <bse/randomhash.hh>

#define TDEBUG(...)     Bse::debug ("Test", __VA_ARGS__)

namespace Bse {

/** The Test namespace offers utilities for unit tests.
 * The Test namespace is made available by <code> \#include <bse/testing.hh> </code> <br/>
 * See also bse/testing.hh.
 */
namespace Test {

/** Initialize the Bse core for a test program.
 * Initializes Bse to execute unit tests by calling parse_settings_and_args()
 * with args "autonomous=1" and "testing=1" and setting the application name.
 * See also #$BSE_DEBUG.
 */
void
init (int *argcp, char **argv, const StringVector &args)
{
  Bse::set_debug_flags (Bse::DebugFlags::FATAL_WARNINGS);
  unsigned int flags = g_log_set_always_fatal (GLogLevelFlags (G_LOG_FATAL_MASK));
  g_log_set_always_fatal (GLogLevelFlags (flags | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL));
  sfi_init (argcp, argv, Bse::cstrings_to_vector (":autonomous:testing:fatal-warnings:", NULL));
}

Timer::Timer (double deadline_in_secs) :
  deadline_ (deadline_in_secs), test_duration_ (0), n_reps_ (0)
{}

Timer::~Timer ()
{}

double
Timer::bench_time ()
{
  /* timestamp_benchmark() counts nano seconds since program start, so
   * it's not going to exceed the 52bit double mantissa too fast.
   */
  return timestamp_benchmark() / 1000000000.0;
}

#define DEBUG_LOOPS_NEEDED(...) while (0) printerr (__VA_ARGS__)

int64
Timer::loops_needed ()
{
  if (samples_.size() < 3)
    {
      n_reps_ = MAX (1, n_reps_);
      DEBUG_LOOPS_NEEDED ("loops_needed: %d\n", n_reps_);
      return n_reps_;           // force significant number of test runs
    }
  double resolution = timestamp_resolution() / 1000000000.0;
  const double deadline = MAX (deadline_ == 0.0 ? 0.005 : deadline_, resolution * 10000.0);
  if (test_duration_ < deadline * 0.2)
    {
      // increase the number of tests per run to gain more accuracy
      n_reps_ = MAX (n_reps_ + 1, int64 (n_reps_ * 1.5)) | 1;
      DEBUG_LOOPS_NEEDED ("loops_needed: %d\n", n_reps_);
      return n_reps_;
    }
  if (test_duration_ < deadline)
    {
      DEBUG_LOOPS_NEEDED ("loops_needed: %d\n", n_reps_);
      return n_reps_;
    }
  DEBUG_LOOPS_NEEDED ("loops_needed: %d\n", 0);
  return 0;
}

void
Timer::submit (double elapsed, int64 repetitions)
{
  test_duration_ += elapsed;
  double resolution = timestamp_resolution() / 1000000000.0;
  if (elapsed >= resolution * 500.0) // force error below 5%
    samples_.push_back (elapsed / repetitions);
  else
    n_reps_ = (n_reps_ + n_reps_) | 1; // double n_reps_ to yield significant times
}

void
Timer::reset()
{
  samples_.resize (0);
  test_duration_ = 0;
  n_reps_ = 0;
}

double
Timer::min_elapsed () const
{
  double m = DBL_MAX;
  for (size_t i = 0; i < samples_.size(); i++)
    m = MIN (m, samples_[i]);
  return m;
}

double
Timer::max_elapsed () const
{
  double m = 0;
  for (size_t i = 0; i < samples_.size(); i++)
    m = MAX (m, samples_[i]);
  return m;
}

static String
ensure_newline (const String &s)
{
  if (!s.size() || s[s.size()-1] != '\n')
    return s + "\n";
  return s;
}

static __thread String *thread_test_start = NULL;

void
test_output (int kind, const String &msg)
{
  if (!thread_test_start)
    thread_test_start = new String();
  String &test_start = *thread_test_start;
  String prefix, sout;
  bool aborting = false;
  switch (kind)
    {
    case 'S':                   // TSTART()
      if (!test_start.empty())
        return test_output ('F', string_format ("Unfinished Test: %s\n", test_start));
      test_start = msg;
      sout = "  START…   " + ensure_newline (msg);
      break;
    case 'D':                   // TDONE() - test passed
      if (test_start.empty())
        return test_output ('F', "Extraneous TDONE() call");
      test_start = "";
      sout = "  …DONE    " + ensure_newline (msg);
      break;
    case 'I':                   // TNOTE() - verbose test message
      if (verbose())
        sout = "  NOTE     " + ensure_newline (msg);
      break;
    case 'P':
      sout = "  PASS     " + ensure_newline (msg);
      break;
    case 'B':
      sout = "  BENCH    " + ensure_newline (msg);
      break;
    case 'F':
      sout = "  FAIL     " + ensure_newline (msg);
      aborting = true;
      break;
    default:
      sout = "  INFO     " + ensure_newline (msg);
      break;
    }
  if (!sout.empty())            // test message output
    {
      fflush (stderr);
      fputs (sout.c_str(), stdout);
      fflush (stdout);
    }
  if (aborting)
    {
      breakpoint();
    }
}

bool
slow()
{
  static bool cached_slow = feature_toggle_bool (getenv ("BSE_TEST"), "slow");
  return cached_slow;
}

bool
verbose()
{
  static bool cached_verbose = feature_toggle_bool (getenv ("BSE_TEST"), "verbose");
  return cached_verbose;
}

uint64_t
random_int64 ()
{
  return Bse::random_int64();
}

int64_t
random_irange (int64_t begin, int64_t end)
{
  return Bse::random_irange (begin, end);
}

double
random_float ()
{
  return Bse::random_float();
}

double
random_frange (double begin, double end)
{
  return Bse::random_frange (begin, end);
}

// == TestChain ==
static const TestChain *global_test_chain = NULL;

TestChain::TestChain (std::function<void()> tfunc, const std::string &tname, Kind kind) :
  name_ (tname), func_ (tfunc), next_ (global_test_chain), kind_ (kind)
{
  assert_return (next_ == global_test_chain);
  global_test_chain = this;
}

static bool
match_testname (const std::string &name, const StringVector &test_names)
{
  for (const auto &tname : test_names)
    if (name == tname)
      return true;
  return false;
}

void
TestChain::run (ptrdiff_t internal_token, const StringVector *test_names)
{
  assert_return (internal_token == ptrdiff_t (global_test_chain));
  for (const TestChain *t = global_test_chain; t; t = t->next_)
    {
      if (test_names && !match_testname (t->name_, *test_names))
        continue;
      if (!test_names && (t->kind_ == SLOW ||
                          t->kind_ == BENCH ||
                          t->kind_ == BROKEN))
        continue;
      fflush (stderr);
      printout ("  RUN…     %s\n", t->name_);
      fflush (stdout);
      t->func_();
      fflush (stderr);
      printout ("  PASS     %s\n", t->name_);
      fflush (stdout);
    }
}

int
run (const StringVector &test_names)
{
  TestChain::run (ptrdiff_t (global_test_chain), &test_names);
  return 0;
}

int
run (void)
{
  TestChain::run (ptrdiff_t (global_test_chain), NULL);
  return 0;
}

TestEntries
list_tests ()
{
  TestEntries entries;
  for (const TestChain *t = global_test_chain; t; t = t->next())
    {
      entries.resize (entries.size() + 1);
      TestEntry &entry = entries.back();
      entry.ident = t->name();
      entry.flags = t->flags();
    }
  std::stable_sort (entries.begin(), entries.end(), // cmp_lesser
                    [] (const TestEntry &a, const TestEntry &b) {
                      return a.ident < b.ident;
                    });
  std::string last;
  for (const auto &entry : entries)
    if (last == entry.ident)
      Bse::fatal_error ("duplicate test entry: %s", entry.ident);
    else
      last = entry.ident;
  return entries;
}

int
run_test (const std::string &test_identifier)
{
  for (const TestChain *t = global_test_chain; t; t = t->next())
    if (test_identifier == t->name())
      {
        fflush (stderr);
        printout ("  RUN…     %s\n", t->name());
        fflush (stdout);
        t->run();
        fflush (stderr);
        printout ("  PASS     %s\n", t->name());
        fflush (stdout);
        return 1; // ran and passed
      }
  return -1; // none found
}

} } // Bse::Test
