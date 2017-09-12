// Licensed CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0
#include <cstdio>
#include <cmath>
#include "t303-explore-srvt.hh"
#include "t303-explore-clnt.hh"
using namespace Aida;

static void
fill_simple_data_pack (A1::SimpleDataPack &simple)
{
  simple.vbool = true;
  simple.vi32 = 32;
  simple.vi64t = 64;
  simple.vf64 = 123456789e-11;
  simple.vstr = "donkeykong";
  simple.count = A1::CountEnum::THREE;
  simple.location.x = -3;
  simple.location.y = -3;
  simple.strings.push_back ("ABCDEFGHI");
  simple.strings.push_back ("jklmnopqr");
  simple.strings.push_back ("STUVWXYZ");
}

static void
fill_big_data_pack (A1::BigDataPack &big)
{
  const double TstNAN = 0 ? NAN : 909.909; // "NAN" is useful to test but breaks assert (in == out)
  fill_simple_data_pack (big.simple_pack);
  std::vector<bool> bools = { false, false, false, true, true, true, false, false, false };
  bools.swap (big.bools);
  std::vector<int32_t> ints32 = { -3, -2, -1, 0, 1, 2, 3 };
  ints32.swap (big.ints32);
  std::vector<int64_t> ints64 = { -1, 0x11111111deadbeefLL, -2 };
  ints64.swap (big.ints64);
  std::vector<double> floats = { -TstNAN, -INFINITY, -5.5, -0.75, -0, +0, +0.25, +1.25, +INFINITY, +TstNAN };
  floats.swap (big.floats);
  std::vector<A1::CountEnum> counts = { A1::CountEnum::THREE, A1::CountEnum::TWO, A1::CountEnum::ONE };
  counts.swap (big.counts);
  A1::Location l;
  l.x = -3; l.y = -5; big.locations.push_back (l);
  l.x = -1; l.y = -2; big.locations.push_back (l);
  l.x =  0; l.y =  0; big.locations.push_back (l);
  l.x = +1; l.y = +1; big.locations.push_back (l);
  l.x = +9; l.y = +2; big.locations.push_back (l);
  // any1;
  // anys;
  // objects;
  // other;
  // derived;
}


// == main ==
int
main (int   argc,
      char *argv[])
{
  // rich data type test setup
  A1::BigDataPack big;
  fill_big_data_pack (big);

  return 0;
}


#include "t303-explore-srvt.cc"
#include "t303-explore-clnt.cc"
