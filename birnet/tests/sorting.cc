/* Birnet Sorting Test
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
#include <math.h>

namespace {
using namespace Birnet;

static int
compare_floats (float f1,
                float f2)
{
  return f1 < f2 ? -1 : f1 > f2;
}

static int
smaller_float (float f1,
               float f2)
{
  return f1 < f2;
}

static void
test_binary_lookup()
{
  bool seen_inexact;
  vector<float> fv;
  vector<float>::iterator fit;
  std::pair<vector<float>::iterator,bool> pit;
  const uint count = birnet_init_settings->test_quick ? 90000 : 1000000;
  TSTART ("Corner case lookups");
  fv.resize (count + (rand() % 10000));
  if (fv.size() % 2)
    TACK();
  else
    TICK();
  for (uint i = 0; i < fv.size(); i++)
    {
      fv[i] = rand();
      if (i % 100000 == 99999)
        TICK();
    }
  TICK();
  vector<float> sv = fv;
  stable_sort (sv.begin(), sv.end(), smaller_float);
  TICK();
  /* failed lookups */
  fit = binary_lookup (sv.begin(), sv.end(), compare_floats, -INFINITY);
  TASSERT (fit == sv.end());
  fit = binary_lookup_sibling (sv.begin(), sv.end(), compare_floats, -INFINITY);
  TASSERT (fit != sv.end());
  /* 0-size lookups */
  vector<float> ev;
  fit = binary_lookup (ev.begin(), ev.end(), compare_floats, 0);
  TASSERT (fit == ev.end());
  fit = binary_lookup_sibling (ev.begin(), ev.end(), compare_floats, 0);
  TASSERT (fit == ev.end());
  pit = binary_lookup_insertion_pos (ev.begin(), ev.end(), compare_floats, 0);
  TASSERT (pit.first == ev.end() && pit.second == false);
  TDONE();
  TSTART ("Binary lookup");
  for (uint i = 0; i < fv.size(); i++)
    {
      fit = binary_lookup (sv.begin(), sv.end(), compare_floats, fv[i]);
      TCHECK (fit != sv.end());
      TCHECK (fv[i] == *fit);           /* silent assertion */
      if (i % 10000 == 9999)
        TASSERT (fv[i] == *fit);        /* verbose assertion */
    }
  TDONE();
  TSTART ("Sibling lookup");
  for (uint i = 1; i < sv.size(); i++)
    {
      double target = (sv[i - 1] + sv[i]) / 2.0;
      fit = binary_lookup_sibling (sv.begin(), sv.end(), compare_floats, target);
      TCHECK (fit != sv.end());
      TCHECK (sv[i] == *fit || sv[i - 1] == *fit);
      if (i % 10000 == 9999)
        TICK();
    }
  TDONE();
  TSTART ("Insertion lookup1");
  seen_inexact = false;
  for (uint i = 0; i < fv.size(); i++)
    {
      pit = binary_lookup_insertion_pos (sv.begin(), sv.end(), compare_floats, fv[i]);
      fit = pit.first;
      seen_inexact |= pit.second == false;
      TCHECK (fit != sv.end());
      TCHECK (fv[i] == *fit);
      if (i % 10000 == 9999)
        TICK();
    }
  TASSERT (seen_inexact == false);
  TDONE();
  TSTART ("Insertion lookup2");
  seen_inexact = false;
  for (uint i = 1; i < sv.size(); i++)
    {
      double target = (sv[i - 1] + sv[i]) / 2.0;
      pit = binary_lookup_insertion_pos (sv.begin(), sv.end(), compare_floats, target);
      fit = pit.first;
      seen_inexact |= pit.second == false;
      TCHECK (fit != sv.end());
      TCHECK (sv[i] == *fit || sv[i - 1] == *fit);
      if (i % 10000 == 9999)
        TICK();
    }
  TASSERT (seen_inexact == true);
  TDONE();
}

} // Anon

int
main (int   argc,
      char *argv[])
{
  birnet_init_test (&argc, &argv);
  test_binary_lookup();
  return 0;
}
