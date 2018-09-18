// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <bse/testing.hh>

int
main (int   argc,
      char *argv[])
{
  Bse::Test::init (&argc, argv);

  return Bse::Test::run();
}
