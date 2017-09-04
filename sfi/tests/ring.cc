// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
// #define TEST_VERBOSE
#include <sfi/testing.hh>
#include <sfi/sfi.hh>

using namespace Bse;

static void
print_ring_ints (SfiRing *ring)
{
  printout ("SfiRing(%p): {", ring);
  SfiRing *node;
  for (node = ring; node; node = sfi_ring_walk (node, ring))
    printout (" %zd,", (size_t) node->data);
  printout (" };");
}

static void
print_rings_side_by_side (SfiRing *ring1,
                          SfiRing *ring2)
{
  SfiRing *r1 = ring1, *r2 = ring2;
  printerr ("\nRing=%p Ring=%p\n", r1, r2);
  while (r1 || r2)
    {
      if (r1 && r2)
        printerr ("  %10p  %10p\n", r1->data, r2->data);
      else if (r1)
        printerr ("  %10p\n", r1->data);
      else
        printerr ("              %10p\n", r2->data);
      if (r1)
        r1 = sfi_ring_walk (r1, ring1);
      if (r2)
        r2 = sfi_ring_walk (r2, ring2);
    }
}

static void
test_sfi_ring (void)
{
  TSTART ("RingBasics");
  (void) print_ring_ints;

  SfiRing *r1 = NULL, *r2 = NULL, *d = NULL;

  r1= sfi_ring_append (r1, (void*) 3);
  r1= sfi_ring_append (r1, (void*) 7);
  r1= sfi_ring_append (r1, (void*) 8);
  r1= sfi_ring_append (r1, (void*) 13);
  r1= sfi_ring_append (r1, (void*) 18);
  TASSERT (sfi_ring_length (r1) == 5);
  TASSERT (sfi_ring_equals (r1, r1, sfi_pointer_cmp, NULL));

  d = sfi_ring_append (d, (void*) 13);
  d = sfi_ring_append (d, (void*) 7);
  d = sfi_ring_append (d, (void*) 18);
  d = sfi_ring_append (d, (void*) 3);
  d = sfi_ring_append (d, (void*) 8);
  TASSERT (sfi_ring_equals (d, d, sfi_pointer_cmp, NULL));
  TASSERT (sfi_ring_min (d, sfi_pointer_cmp, NULL) == (void*) 3);
  TASSERT (sfi_ring_max (d, sfi_pointer_cmp, NULL) == (void*) 18);

  TASSERT (sfi_ring_equals (r1, d, sfi_pointer_cmp, NULL) == FALSE);
  d = sfi_ring_sort (d, sfi_pointer_cmp, NULL);
  TASSERT (sfi_ring_equals (r1, d, sfi_pointer_cmp, NULL));
  TASSERT (sfi_ring_includes (r1, d, sfi_pointer_cmp, NULL));
  TASSERT (sfi_ring_includes (d, r1, sfi_pointer_cmp, NULL));
  sfi_ring_free (d);

  r2 = sfi_ring_append (r2, (void*) 4);
  r2 = sfi_ring_append (r2, (void*) 7);
  r2 = sfi_ring_append (r2, (void*) 13);
  TASSERT (sfi_ring_length (r2) == 3);
  d = sfi_ring_sort (sfi_ring_copy (r2), sfi_pointer_cmp, NULL);
  TASSERT (sfi_ring_equals (r2, d, sfi_pointer_cmp, NULL));
  TASSERT (sfi_ring_equals (r1, r2, sfi_pointer_cmp, NULL) == FALSE);
  TASSERT (sfi_ring_includes (r1, r2, sfi_pointer_cmp, NULL) == FALSE);
  sfi_ring_free (d);

  TDONE ();
  TSTART ("RingMath");

  d = sfi_ring_difference (r1, r2, sfi_pointer_cmp, NULL);
  TASSERT (sfi_ring_pop_head (&d) == (void*) 3);
  TASSERT (sfi_ring_pop_head (&d) == (void*) 8);
  TASSERT (sfi_ring_pop_head (&d) == (void*) 18);
  TASSERT (d == NULL);

  d = sfi_ring_symmetric_difference (r1, r2, sfi_pointer_cmp, NULL);
  TASSERT (sfi_ring_pop_head (&d) == (void*) 3);
  TASSERT (sfi_ring_pop_head (&d) == (void*) 4);
  TASSERT (sfi_ring_pop_head (&d) == (void*) 8);
  TASSERT (sfi_ring_pop_head (&d) == (void*) 18);
  TASSERT (d == NULL);

  SfiRing *t1 = sfi_ring_symmetric_difference (r1, r2, sfi_pointer_cmp, NULL);
  SfiRing *t2 = sfi_ring_intersection (r1, r2, sfi_pointer_cmp, NULL);
  d = sfi_ring_intersection (t1, t2, sfi_pointer_cmp, NULL);
  TASSERT (d == NULL);
  d = sfi_ring_union (t1, t2, sfi_pointer_cmp, NULL);
  TASSERT (sfi_ring_includes (d, t1, sfi_pointer_cmp, NULL));
  TASSERT (sfi_ring_includes (d, t2, sfi_pointer_cmp, NULL));
  sfi_ring_free (t1);
  sfi_ring_free (t2);
  TASSERT (sfi_ring_includes (d, r1, sfi_pointer_cmp, NULL));
  TASSERT (sfi_ring_includes (d, r2, sfi_pointer_cmp, NULL));

  d = sfi_ring_union (r1, r2, sfi_pointer_cmp, NULL);
  TASSERT (sfi_ring_length (d) == 6);
  t1 = r1, t2 = d;
  sfi_ring_mismatch (&t1, &t2, sfi_pointer_cmp, NULL);
  TASSERT (t1->data == (void*) 7);
  TASSERT (t2->data == (void*) 4);
  t2 = sfi_ring_concat (sfi_ring_copy (r1), sfi_ring_copy (r2));
  TASSERT (sfi_ring_length (t2) == 8);
  t2 = sfi_ring_sort (t2, sfi_pointer_cmp, NULL);
  TASSERT (sfi_ring_length (t2) == 8);
  t1 = sfi_ring_copy_uniq (t2, sfi_pointer_cmp, NULL);
  TASSERT (sfi_ring_length (t1) == 6);
  TASSERT (sfi_ring_equals (d, t1, sfi_pointer_cmp, NULL));
  sfi_ring_free (t1);
  t1 = sfi_ring_uniq (t2, sfi_pointer_cmp, NULL);
  TASSERT (sfi_ring_length (t1) == 6);
  TASSERT (sfi_ring_equals (d, t1, sfi_pointer_cmp, NULL));
  sfi_ring_free (t1);
  sfi_ring_free (d);

  sfi_ring_free (r1);
  sfi_ring_free (r2);

  TDONE ();
  TSTART ("RingReorder");

  r1 = NULL;
  r1 = sfi_ring_append (r1, (void*) 5);
  r1 = sfi_ring_append (r1, (void*) 7);
  r1 = sfi_ring_append (r1, (void*) 4);
  r1 = sfi_ring_append (r1, (void*) 8);
  r1 = sfi_ring_append (r1, (void*) 1);
  r2 = sfi_ring_sort (sfi_ring_copy (r1), sfi_pointer_cmp, NULL);
  t1 = sfi_ring_reorder (sfi_ring_copy (r2), r1);
  if (0)
    print_rings_side_by_side (t1, r1);
  TASSERT (sfi_ring_equals (t1, r1, sfi_pointer_cmp, NULL));
  sfi_ring_free (t1);
  r2 = sfi_ring_remove (r2, (void*) 4);
  r2 = sfi_ring_append (r2, (void*) 9);
  t1 = sfi_ring_reorder (sfi_ring_copy (r2), r1);
  r1 = sfi_ring_remove (r1, (void*) 4);
  r1 = sfi_ring_append (r1, (void*) 9);
  if (0)
    print_rings_side_by_side (t1, r1);
  TASSERT (sfi_ring_equals (t1, r1, sfi_pointer_cmp, NULL));
  sfi_ring_free (r1);
  sfi_ring_free (r2);
  sfi_ring_free (t1);
  r1 = NULL;
  r2 = NULL;
  r1 = sfi_ring_append (r1, (void*) 0x75);
  r1 = sfi_ring_append (r1, (void*) 0x4c);
  r1 = sfi_ring_append (r1, (void*) 0x5e);
  r2 = sfi_ring_append (r2, (void*) 0x4c);
  r2 = sfi_ring_append (r2, (void*) 0x5e);
  r2 = sfi_ring_append (r2, (void*) 0x68);
  r2 = sfi_ring_append (r2, (void*) 0x68);
  r2 = sfi_ring_reorder (r2, r1);
  if (0)
    print_rings_side_by_side (r2, r1);
  TASSERT (sfi_ring_pop_head (&r2) == (void*) 0x4c);
  TASSERT (sfi_ring_pop_head (&r2) == (void*) 0x5e);
  TASSERT (sfi_ring_pop_head (&r2) == (void*) 0x68);
  TASSERT (sfi_ring_pop_head (&r2) == (void*) 0x68);
  TASSERT (r2 == NULL);
  sfi_ring_free (r1);

  r1 = NULL;
  r1 = sfi_ring_append (r1, (void*) 0x11);
  r1 = sfi_ring_append (r1, (void*) 0x16);
  r1 = sfi_ring_append (r1, (void*) 0x15);
  r1 = sfi_ring_append (r1, (void*) 0x14);
  r1 = sfi_ring_append (r1, (void*) 0x13);
  r1 = sfi_ring_append (r1, (void*) 0x12);
  r1 = sfi_ring_append (r1, (void*) 0x03);
  r1 = sfi_ring_append (r1, (void*) 0x02);
  r1 = sfi_ring_append (r1, (void*) 0x01);
  r2 = NULL;
  r2 = sfi_ring_append (r2, (void*) 0x16);
  r2 = sfi_ring_append (r2, (void*) 0x15);
  r2 = sfi_ring_append (r2, (void*) 0x14);
  r2 = sfi_ring_append (r2, (void*) 0x13);
  r2 = sfi_ring_append (r2, (void*) 0x12);
  r2 = sfi_ring_append (r2, (void*) 0x11);
  r2 = sfi_ring_append (r2, (void*) 0x01);
  r2 = sfi_ring_append (r2, (void*) 0x02);
  r2 = sfi_ring_append (r2, (void*) 0x03);
  r1 = sfi_ring_reorder (r1, r2);
  TASSERT (sfi_ring_equals (r1, r2, sfi_pointer_cmp, NULL));
  sfi_ring_free (r1);
  sfi_ring_free (r2);
  TDONE ();
}
int
main (int   argc,
      char *argv[])
{
  Bse::Test::init (&argc, argv);
  test_sfi_ring();
  return 0;
}
/* vim:set ts=8 sts=2 sw=2: */
