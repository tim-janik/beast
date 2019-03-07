// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <bse/testing.hh>
#include <bse/bsemain.hh> // FIXME: bse_init_test

int
main (int   argc,
      char *argv[])
{
  const Bse::StringVector args = Bse::cstrings_to_vector ("stand-alone=1", "wave-chunk-padding=1", NULL);
  // "wave-chunk-big-pad=2", "dcache-block-size=16"

  // Bse::Test::init (&argc, argv, args); // <- this lacks BSE initialization

  bse_init_test (&argc, argv, args);

  Bse::StringVector test_names;

  for (ssize_t i = 1; i < argc; i++)
    if (argv[i])
      {
        if (argv[i][0] == '-')
          {
            Bse::printerr ("%s: unknown option: %s\n", argv[0], argv[i]);
            exit (7);
          }
        test_names.push_back (argv[i]);
      }

  return test_names.size() ? Bse::Test::run (test_names) : Bse::Test::run();
}
