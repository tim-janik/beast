// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <bse/bsemain.hh>
#include <sfi/testing.hh>
#include "sfi/private.hh"

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

// == Run integrity tests ==
int
main (int argc, char *argv[])
{
  bse_init_test (&argc, argv);
  assert_return (Bse::IntegrityCheck::checks_enabled() == true, -1);

  if (tests)
    for (const auto &te : *tests)
      { // note, more than one space after "TESTING:" confuses emacs file:line matches
        printerr ("  TESTING: %s:%u: %s…\n", te.file, te.line, te.func);
        te.test();
        printerr ("    …DONE  (%s)\n", te.func);
      }

  return 0;
}
