/* Birnet
 * Copyright (C) 2010 Stefan Westerfeld
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include <birnet/birnettests.h>

using namespace Birnet;

struct Foo
{
  static int destructor_calls;
  int value;
  Foo()
  {
    value = 42;
  }
  ~Foo()
  {
    TASSERT (value == 42);
    value = 0;
    destructor_calls++;
  }
};

template<class T, int N>
void aligned_array_test()
{
  const size_t cache_line_size = 64; // sync with Birnet::malloc_aligned

  size_t len = g_random_int_range (1, 1000);
  AlignedArray<T, N> array (len);
  TCHECK (array.size() == len);
  for (size_t i = 0; i < array.size(); i++)
    TCHECK (array[i] == 0);
  TCHECK (size_t (&array[0]) % N == 0);
  if (cache_line_size % N == 0) // cases where we expect 64-byte alignment
    TCHECK (size_t (&array[0]) % cache_line_size == 0);
  TOK();
};

int Foo::destructor_calls = 0;

static void
test_aligned_array (void)
{
  TSTART ("AlignedArray");
  TOK();
  // try different alignments (char is needed where size is not a multiple of 4)
  aligned_array_test<char, 1> ();
  aligned_array_test<char, 2> ();
  aligned_array_test<char, 3> ();
  aligned_array_test<int, 4> ();
  aligned_array_test<char, 7> ();
  aligned_array_test<int, 8> ();
  aligned_array_test<char, 13> ();
  aligned_array_test<int, 16> ();
  aligned_array_test<char, 23> ();
  aligned_array_test<int, 32> ();
  aligned_array_test<char, 47> ();
  aligned_array_test<int, 64> ();
  aligned_array_test<char, 99> ();
  aligned_array_test<int, 128> ();
  aligned_array_test<char, 199> ();
  aligned_array_test<int, 256> ();
  aligned_array_test<char, 311> ();
  aligned_array_test<int, 512> ();
  aligned_array_test<char, 777> ();
  aligned_array_test<int, 1024> ();
  aligned_array_test<char, 1234> ();
  aligned_array_test<int, 2048> ();
  aligned_array_test<int, 65540> ();
    {
      AlignedArray<Foo, 40> foo_array (5);
      TASSERT (size_t (&foo_array[0]) % 40 == 0);
      for (size_t i = 0; i < foo_array.size(); i++)
        TASSERT (foo_array[i].value == 42);
    }
  TASSERT (Foo::destructor_calls == 5);   // check that all elements have been destructed
  TOK();
  TDONE();
}

int
main (int   argc,
      char *argv[])
{
  birnet_init_test (&argc, &argv);

  test_aligned_array();
}
