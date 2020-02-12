// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <bse/bsemain.hh>
#include <bse/testing.hh>
#include "bse/internal.hh"

using Bse::printerr;
typedef Bse::IntegrityCheck::TestFunc TestFunc;

// == BSE_INTEGRITY_TEST Registry ==
struct TestEntry {
  TestFunc    test;
  const char *func;
  const char *file;
  int         line;
  TestEntry (const char *_file, int _line, const char *_func, TestFunc _test) :
    test (_test), func (_func), file (_file), line (_line)
  {}
};
static std::vector<TestEntry> *tests = NULL; // NOTE, this must be available for high priority early constructors

// == BSE_INTEGRITY_CHECK Activation ==
namespace Bse {
// Override Bse weak symbol to enable Bse's internal integrity tests, see bcore.hh
const bool IntegrityCheck::enabled = true;
// Registration function called for all integrity tests
void
IntegrityCheck::Test::register_test (const char *file, int line, const char *func, TestFunc test)
{
  if (!tests)
    tests = new std::vector<TestEntry>();
  tests->push_back (TestEntry (file, line, func, test));
}
} // Bse

// == Main test program ==
static int
test_main (int argc, char *argv[])
{
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

  // integrity tests
  assert_return (Bse::IntegrityCheck::checks_enabled() == true, -1);

  if (!tests)
    return 0;

  std::sort (tests->begin(), tests->end(), [] (const TestEntry &a, const TestEntry &b) { return strcmp (a.func ? a.func : "", b.func ? b.func : "") < 0; });
  for (const auto &te : *tests)
    { // note, more than one space after "TESTING:" confuses emacs file:line matches
      printerr ("  TESTING: %s:%u: %s…\n", te.file, te.line, te.func);
      te.test();
      printerr ("    …DONE  (%s)\n", te.func);
    }

  return 0;
}

int
main (int argc, char *argv[])
{
  Bse::set_debug_flags (Bse::DebugFlags::SIGQUIT_ON_ABORT);
  return bse_init_and_test (&argc, argv, [&]() { return test_main (argc, argv); });
}
