/* Birnet
 * Copyright (C) 2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
// #define TEST_VERBOSE
#include <birnet/birnettests.h>

static void
print_ring_ints (BirnetRing *ring)
{
  g_print ("BirnetRing(%p): {", ring);
  BirnetRing *node;
  for (node = ring; node; node = birnet_ring_walk (node, ring))
    g_print (" %d,", (ptrdiff_t) node->data);
  g_print (" };");
}

static void
print_rings_side_by_side (BirnetRing *ring1,
                          BirnetRing *ring2)
{
  BirnetRing *r1 = ring1, *r2 = ring2;
  g_printerr ("\nRing=%p Ring=%p\n", r1, r2);
  while (r1 || r2)
    {
      if (r1 && r2)
        g_printerr ("  %10p  %10p\n", r1->data, r2->data);
      else if (r1)
        g_printerr ("  %10p\n", r1->data);
      else
        g_printerr ("              %10p\n", r2->data);
      if (r1)
        r1 = birnet_ring_walk (r1, ring1);
      if (r2)
        r2 = birnet_ring_walk (r2, ring2);
    }
}

static void
test_birnet_ring (void)
{
  TSTART ("RingBasics:");
  (void) print_ring_ints;

  BirnetRing *r1 = NULL, *r2 = NULL, *d = NULL;

  r1= birnet_ring_append (r1, (void*) 3);
  r1= birnet_ring_append (r1, (void*) 7);
  r1= birnet_ring_append (r1, (void*) 8);
  r1= birnet_ring_append (r1, (void*) 13);
  r1= birnet_ring_append (r1, (void*) 18);
  TASSERT (birnet_ring_length (r1) == 5);
  TASSERT (birnet_ring_equals (r1, r1, birnet_pointer_cmp, NULL));

  d = birnet_ring_append (d, (void*) 13);
  d = birnet_ring_append (d, (void*) 7);
  d = birnet_ring_append (d, (void*) 18);
  d = birnet_ring_append (d, (void*) 3);
  d = birnet_ring_append (d, (void*) 8);
  TASSERT (birnet_ring_equals (d, d, birnet_pointer_cmp, NULL));
  TASSERT (birnet_ring_min (d, birnet_pointer_cmp, NULL) == (void*) 3);
  TASSERT (birnet_ring_max (d, birnet_pointer_cmp, NULL) == (void*) 18);

  TASSERT (birnet_ring_equals (r1, d, birnet_pointer_cmp, NULL) == FALSE);
  d = birnet_ring_sort (d, birnet_pointer_cmp, NULL);
  TASSERT (birnet_ring_equals (r1, d, birnet_pointer_cmp, NULL));
  TASSERT (birnet_ring_includes (r1, d, birnet_pointer_cmp, NULL));
  TASSERT (birnet_ring_includes (d, r1, birnet_pointer_cmp, NULL));
  birnet_ring_free (d);

  r2 = birnet_ring_append (r2, (void*) 4);
  r2 = birnet_ring_append (r2, (void*) 7);
  r2 = birnet_ring_append (r2, (void*) 13);
  TASSERT (birnet_ring_length (r2) == 3);
  d = birnet_ring_sort (birnet_ring_copy (r2), birnet_pointer_cmp, NULL);
  TASSERT (birnet_ring_equals (r2, d, birnet_pointer_cmp, NULL));
  TASSERT (birnet_ring_equals (r1, r2, birnet_pointer_cmp, NULL) == FALSE);
  TASSERT (birnet_ring_includes (r1, r2, birnet_pointer_cmp, NULL) == FALSE);
  birnet_ring_free (d);

  TDONE ();
  TSTART ("RingMath:");

  d = birnet_ring_difference (r1, r2, birnet_pointer_cmp, NULL);
  TASSERT (birnet_ring_pop_head (&d) == (void*) 3);
  TASSERT (birnet_ring_pop_head (&d) == (void*) 8);
  TASSERT (birnet_ring_pop_head (&d) == (void*) 18);
  TASSERT (d == NULL);

  d = birnet_ring_symmetric_difference (r1, r2, birnet_pointer_cmp, NULL);
  TASSERT (birnet_ring_pop_head (&d) == (void*) 3);
  TASSERT (birnet_ring_pop_head (&d) == (void*) 4);
  TASSERT (birnet_ring_pop_head (&d) == (void*) 8);
  TASSERT (birnet_ring_pop_head (&d) == (void*) 18);
  TASSERT (d == NULL);

  BirnetRing *t1 = birnet_ring_symmetric_difference (r1, r2, birnet_pointer_cmp, NULL);
  BirnetRing *t2 = birnet_ring_intersection (r1, r2, birnet_pointer_cmp, NULL);
  d = birnet_ring_intersection (t1, t2, birnet_pointer_cmp, NULL);
  TASSERT (d == NULL);
  d = birnet_ring_union (t1, t2, birnet_pointer_cmp, NULL);
  TASSERT (birnet_ring_includes (d, t1, birnet_pointer_cmp, NULL));
  TASSERT (birnet_ring_includes (d, t2, birnet_pointer_cmp, NULL));
  birnet_ring_free (t1);
  birnet_ring_free (t2);
  TASSERT (birnet_ring_includes (d, r1, birnet_pointer_cmp, NULL));
  TASSERT (birnet_ring_includes (d, r2, birnet_pointer_cmp, NULL));

  d = birnet_ring_union (r1, r2, birnet_pointer_cmp, NULL);
  TASSERT (birnet_ring_length (d) == 6);
  t1 = r1, t2 = d;
  birnet_ring_mismatch (&t1, &t2, birnet_pointer_cmp, NULL);
  TASSERT (t1->data == (void*) 7);
  TASSERT (t2->data == (void*) 4);
  t2 = birnet_ring_concat (birnet_ring_copy (r1), birnet_ring_copy (r2));
  TASSERT (birnet_ring_length (t2) == 8);
  t2 = birnet_ring_sort (t2, birnet_pointer_cmp, NULL);
  TASSERT (birnet_ring_length (t2) == 8);
  t1 = birnet_ring_copy_uniq (t2, birnet_pointer_cmp, NULL);
  TASSERT (birnet_ring_length (t1) == 6);
  TASSERT (birnet_ring_equals (d, t1, birnet_pointer_cmp, NULL));
  birnet_ring_free (t1);
  t1 = birnet_ring_uniq (t2, birnet_pointer_cmp, NULL);
  TASSERT (birnet_ring_length (t1) == 6);
  TASSERT (birnet_ring_equals (d, t1, birnet_pointer_cmp, NULL));
  birnet_ring_free (t1);
  birnet_ring_free (d);

  birnet_ring_free (r1);
  birnet_ring_free (r2);

  TDONE ();
  TSTART ("RingReorder:");

  r1 = NULL;
  r1 = birnet_ring_append (r1, (void*) 5);
  r1 = birnet_ring_append (r1, (void*) 7);
  r1 = birnet_ring_append (r1, (void*) 4);
  r1 = birnet_ring_append (r1, (void*) 8);
  r1 = birnet_ring_append (r1, (void*) 1);
  r2 = birnet_ring_sort (birnet_ring_copy (r1), birnet_pointer_cmp, NULL);
  t1 = birnet_ring_reorder (birnet_ring_copy (r2), r1);
  if (0)
    print_rings_side_by_side (t1, r1);
  TASSERT (birnet_ring_equals (t1, r1, birnet_pointer_cmp, NULL));
  birnet_ring_free (t1);
  r2 = birnet_ring_remove (r2, (void*) 4);
  r2 = birnet_ring_append (r2, (void*) 9);
  t1 = birnet_ring_reorder (birnet_ring_copy (r2), r1);
  r1 = birnet_ring_remove (r1, (void*) 4);
  r1 = birnet_ring_append (r1, (void*) 9);
  if (0)
    print_rings_side_by_side (t1, r1);
  TASSERT (birnet_ring_equals (t1, r1, birnet_pointer_cmp, NULL));
  birnet_ring_free (r1);
  birnet_ring_free (r2);
  birnet_ring_free (t1);
  r1 = NULL;
  r2 = NULL;
  r1 = birnet_ring_append (r1, (void*) 0x75);
  r1 = birnet_ring_append (r1, (void*) 0x4c);
  r1 = birnet_ring_append (r1, (void*) 0x5e);
  r2 = birnet_ring_append (r2, (void*) 0x4c);
  r2 = birnet_ring_append (r2, (void*) 0x5e);
  r2 = birnet_ring_append (r2, (void*) 0x68);
  r2 = birnet_ring_append (r2, (void*) 0x68);
  r2 = birnet_ring_reorder (r2, r1);
  if (0)
    print_rings_side_by_side (r2, r1);
  TASSERT (birnet_ring_pop_head (&r2) == (void*) 0x4c);
  TASSERT (birnet_ring_pop_head (&r2) == (void*) 0x5e);
  TASSERT (birnet_ring_pop_head (&r2) == (void*) 0x68);
  TASSERT (birnet_ring_pop_head (&r2) == (void*) 0x68);
  TASSERT (r2 == NULL);
  birnet_ring_free (r1);

  r1 = NULL;
  r1 = birnet_ring_append (r1, (void*) 0x11);
  r1 = birnet_ring_append (r1, (void*) 0x16);
  r1 = birnet_ring_append (r1, (void*) 0x15);
  r1 = birnet_ring_append (r1, (void*) 0x14);
  r1 = birnet_ring_append (r1, (void*) 0x13);
  r1 = birnet_ring_append (r1, (void*) 0x12);
  r1 = birnet_ring_append (r1, (void*) 0x03);
  r1 = birnet_ring_append (r1, (void*) 0x02);
  r1 = birnet_ring_append (r1, (void*) 0x01);
  r2 = NULL;
  r2 = birnet_ring_append (r2, (void*) 0x16);
  r2 = birnet_ring_append (r2, (void*) 0x15);
  r2 = birnet_ring_append (r2, (void*) 0x14);
  r2 = birnet_ring_append (r2, (void*) 0x13);
  r2 = birnet_ring_append (r2, (void*) 0x12);
  r2 = birnet_ring_append (r2, (void*) 0x11);
  r2 = birnet_ring_append (r2, (void*) 0x01);
  r2 = birnet_ring_append (r2, (void*) 0x02);
  r2 = birnet_ring_append (r2, (void*) 0x03);
  r1 = birnet_ring_reorder (r1, r2);
  TASSERT (birnet_ring_equals (r1, r2, birnet_pointer_cmp, NULL));
  birnet_ring_free (r1);
  birnet_ring_free (r2);

  TDONE ();
}


int
main (int   argc,
      char *argv[])
{
  birnet_init_test (&argc, &argv);

  test_birnet_ring();

  return 0;
}

/* vim:set ts=8 sts=2 sw=2: */
