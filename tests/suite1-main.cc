// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <bse/testing.hh>
#include <bse/bse.hh>
#include <bse/bsemain.hh> // FIXME: bse_init_test

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

static Bse::ServerS bse_server;

static void
bench_aida()
{
  using namespace Bse;
  double calls = 0, slowest = 0, fastest = 9e+9;
  for (uint j = 0; j < 37; j++)
    {
      bse_server.test_counter_set (0);
      const int count = 2999;
      const uint64_t ts0 = timestamp_benchmark();
      for (int i = 0; i < count; i++)
        bse_server.test_counter_inc_fetch ();
      const uint64 ts1 = timestamp_benchmark();
      TASSERT (bse_server.test_counter_get() == count);
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

int
main (int   argc,
      char *argv[])
{
  const Bse::StringVector args = Bse::cstrings_to_vector ("stand-alone=1", "wave-chunk-padding=1", NULL);
  // "wave-chunk-big-pad=2", "dcache-block-size=16"

  Bse::StringVector test_names;

  for (ssize_t i = 1; i < argc; i++)
    if (argv[i])
      {
        if (strcmp (argv[i], "--bench-aida") == 0)
          {
            Bse::init_async (&argc, argv, argv[0], args);
            Aida::ClientConnectionP connection = Bse::init_server_connection();
            bse_server = Bse::init_server_instance();
            bench_aida();
            return 0;
          }
        else if (argv[i][0] == '-')
          {
            Bse::printerr ("%s: unknown option: %s\n", argv[0], argv[i]);
            return 7;
          }
        test_names.push_back (argv[i]);
      }

  bse_init_test (&argc, argv, args);

  return test_names.size() ? Bse::Test::run (test_names) : Bse::Test::run();
}
