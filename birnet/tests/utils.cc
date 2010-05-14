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

int Foo::destructor_calls = 0;

static void
test_aligned_array (void)
{
  TSTART ("AlignedArray");
  TOK();
  AlignedArray<int, 65540> array (3);      // choose an alignment that is unlikely to occur by chance
  TASSERT (array[0] == 0);
  TASSERT (array[1] == 0);
  TASSERT (array[2] == 0);
  TASSERT (size_t (&array[0]) % 65540 == 0);
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
