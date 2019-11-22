// Licensed CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0
#include "explore.hh"
#include <cstdio>
#include <cmath>
#include <bse/testing.hh>
#include <bse/bseutils.hh>
using namespace Aida;

static void
test_explore_seq()
{
  Any any;
  const A1::BoolSeq b1 { 0, 1, 1, 0, 1, 0, 1, 1, 0, 1 };
  A1::BoolSeq b2;
  TASSERT (b1 != b2);
  any.set (b1);
  b2 = any.get<A1::BoolSeq>();
  TASSERT (b1 == b2);
}
TEST_ADD (test_explore_seq);

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

struct A1DerivedImpl : public virtual A1::DerivedIface, public virtual EnableSharedFromThis<A1DerivedImpl> {
  virtual void                       self_args                (DerivedIface &self) override {}
  virtual bool                       __access__               (const std::string &propertyname, const PropertyAccessorPred&) override { return false; }
  virtual Aida::IfaceEventConnection __attach__               (const String &eventselector, EventHandlerF handler) override
  { Aida::IfaceEventConnection *c = NULL; AIDA_ASSERT_RETURN_UNREACHED (*c); }
};

void
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
  big.any1.set ("Foohoo");
  big.anys.push_back (Any (7));
  big.anys.push_back (Any ("seven"));
  big.anys.push_back (Any (std::make_shared<A1DerivedImpl>()));
  big.derived = std::make_shared<A1DerivedImpl>();
  big.bases.push_back (std::make_shared<A1DerivedImpl>());
  big.bases.push_back (std::make_shared<A1DerivedImpl>());
}

static void
test_explore_any()
{
  // record <-> Any
  A1::BigDataPack big, b2;
  TASSERT (big == b2);
  fill_big_data_pack (big);
  TASSERT (big != b2);
  Aida::Any any;
  any.set (big);
  b2 = any.get<A1::BigDataPack>();
  TASSERT (big == b2);
  TASSERT (b2.anys.size() == 3);
  TASSERT (b2.bases.size() == 2);
}
TEST_ADD (test_explore_any);

static void
test_explore_enums()
{
  TASSERT (  0 == Aida::enum_value_from_string<A1::CountEnum> ("ZERO"));
  TASSERT (  1 == Aida::Introspection::enumerator_to_value ("A1.CountEnum.ONE"));
  TASSERT (  2 == Aida::enum_value_from_string<A1::CountEnum> ("TWO"));
  TASSERT (  3 == Aida::Introspection::enumerator_to_value ("THREE", "A1.CountEnum"));
  TASSERT (  8 == Aida::enum_value_from_string<A1::CountEnum> ("EIGHT"));
  TASSERT (  9 == Aida::Introspection::enumerator_to_value ("NINE", "::A1::CountEnum"));
  TASSERT (-23 == Aida::enum_value_from_string<A1::CountEnum> ("ODD"));
  TASSERT (+9223372036854775807   == Aida::enum_value_from_string<A1::CountEnum> ("BIG"));
  TASSERT (-9223372036854775807-1 == Aida::enum_value_from_string<A1::CountEnum> ("SMALL"));
  // TASSERT (0xa     == Aida::enum_value_from_string<A1::CountEnum> ("TWO,EIGHT"));
  // TASSERT (0xb     == Aida::enum_value_from_string<A1::CountEnum> ("ONE TWO EIGHT"));
  // TASSERT (0xb     == Aida::enum_value_from_string<A1::CountEnum> ("THREE | EIGHT"));
  // TASSERT (0       == Aida::enum_value_from_string<A1::CountEnum> ("ZERO + ZERO"));
  // TASSERT (0x1235  == Aida::enum_value_from_string<A1::CountEnum> ("0x1235"));
  // TASSERT (-171717 == Aida::enum_value_from_string<A1::CountEnum> ("-171717"));
  TASSERT ("ZERO"  == Aida::enum_value_to_short_string (A1::CountEnum (0)));
  TASSERT ("ONE"   == Aida::enum_value_to_short_string (A1::CountEnum (1)));
  TASSERT ("TWO"   == Aida::enum_value_to_short_string (A1::CountEnum (2)));
  TASSERT ("THREE" == Aida::enum_value_to_short_string (A1::CountEnum (3)));
  TASSERT ("EIGHT" == Aida::enum_value_to_short_string (A1::CountEnum (8)));
  TASSERT ("NINE"  == Aida::enum_value_to_short_string (A1::CountEnum (9)));
  TASSERT ("ODD"   == Aida::enum_value_to_short_string (A1::CountEnum (-23)));
  TASSERT ("BIG"   == Aida::enum_value_to_short_string (A1::CountEnum (9223372036854775807)));
  TASSERT ("SMALL" == Aida::enum_value_to_short_string (A1::CountEnum (-9223372036854775807-1)));
  // FIXME: TASSERT ("TWO|EIGHT" == Aida::enum_values_to_string ("A1.CountEnum", 0xa, "|"));
  // TASSERT ("-171717"   == Aida::enum_value_to_short_string (A1::CountEnum (-171717)));
  // TASSERT ("0x1235"    == Aida::enum_value_to_short_string (A1::CountEnum (0x1235)));
  TASSERT ("ONE"   == Aida::enum_value_to_short_string (A1::CountEnum (1)));
  TASSERT ("ZERO"  == Aida::enum_value_to_short_string (A1::CountEnum (0)));
  TASSERT ("TWO"   == Aida::enum_value_to_short_string (A1::CountEnum (2)));
  TASSERT ("THREE" == Aida::enum_value_to_short_string (A1::CountEnum (3)));
  TASSERT ("EIGHT" == Aida::enum_value_to_short_string (A1::CountEnum (8)));
  TASSERT ("NINE"  == Aida::enum_value_to_short_string (A1::CountEnum (9)));
  TASSERT ("ODD"   == Aida::enum_value_to_short_string (A1::CountEnum (-23)));
  TASSERT ("BIG"   == Aida::enum_value_to_short_string (A1::CountEnum (9223372036854775807)));
  TASSERT ("SMALL" == Aida::enum_value_to_short_string (A1::CountEnum (-9223372036854775807-1)));
  TASSERT (""      == Aida::enum_value_to_short_string (A1::CountEnum (0xa)));
  TASSERT (""      == Aida::enum_value_to_short_string (A1::CountEnum (0x1235)));
  TASSERT (""      == Aida::enum_value_to_short_string (A1::CountEnum (-171717)));
  auto enumerators = Introspection::list_enumerators ("A1.CountEnum");
  std::vector<int64> enum_values;
  std::transform (enumerators.begin(), enumerators.end(),
                  std::back_inserter (enum_values), [] (auto name) {
                    return Introspection::enumerator_to_value (name);
                  });
  const std::vector<int64> reference = { 0, 1, 2, 3, 8, 9, -23, 9223372036854775807, -9223372036854775807-1 };
  TASSERT (enum_values == reference);
}
TEST_ADD (test_explore_enums);

#include "explore_interfaces.cc"
