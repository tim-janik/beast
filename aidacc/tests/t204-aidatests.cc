// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "testing.hh"

using std::ptrdiff_t;
using namespace Aida;

enum TestEnum { TEST_COFFEE_COFFEE = -0xc0ffeec0ffeeLL };

#if 1 // Manually adding TestEnum introspection information
inline const char* operator->* (::Aida::IntrospectionTypename, TestEnum) { return "TestEnum"; }
static const IntrospectionRegistry __aida_aux_data_srvt__Aida_TestEnum_ = {
  "TestEnum\0"
  "ENUM\0"
  "TEST_COFFEE_COFFEE.value=-0xc0ffeec0ffee\0"
};
#endif

namespace {

template<class, class = void> struct has_complex_member : std::false_type {};      // false case, picked on SFINAE
template<class T> struct has_complex_member<T, void_t<typename T::complex_member>> : std::true_type {}; // !SFINAE

static void
test_aida_cxxaux()
{
  struct SimpleType {
    SimpleType () {}
    bool operator== (const SimpleType&);
  };
  std::shared_ptr<SimpleType> simple = std::make_shared<SimpleType>();

  struct ComplexType {
    ComplexType (int, double, void*) {}
    typedef int complex_member;
  };
  std::shared_ptr<ComplexType> complex = std::make_shared<ComplexType> (77, -0.5, (void*) 0);

  static_assert (has_complex_member<SimpleType>::value == false, "testing void_t");
  static_assert (has_complex_member<ComplexType>::value == true, "testing void_t");
  static_assert (DerivesSharedPtr<SimpleType>::value == false, "testing DerivesSharedPtr");
  // static_assert (IsWeakPtr<std::shared_ptr<SimpleType> >::value == false, "testing IsWeakPtr");
  static_assert (DerivesSharedPtr<std::shared_ptr<SimpleType> >::value == true, "testing DerivesSharedPtr");
  // static_assert (IsWeakPtr<std::weak_ptr<SimpleType> >::value == true, "testing IsWeakPtr");
  static_assert (DerivesSharedPtr<std::weak_ptr<SimpleType> >::value == false, "testing DerivesSharedPtr");
  static_assert (IsComparable<int>::value == true, "testing IsComparable");
  static_assert (IsComparable<SimpleType>::value == true, "testing IsComparable");
  static_assert (IsComparable<std::shared_ptr<SimpleType> >::value == true, "testing IsComparable");
  static_assert (IsComparable<ComplexType>::value == false, "testing IsComparable");
  static_assert (IsComparable<std::shared_ptr<ComplexType> >::value == true, "testing IsComparable");
  // check Has__aida_from_any__
  struct AnyConvertible {
    void __aida_from_any__ (const Aida::Any &any);
  };
  static_assert (Has__aida_from_any__<SimpleType>::value == false, "testing Has__aida_from_any__ == 0");
  static_assert (Has__aida_from_any__<AnyConvertible>::value == true, "testing Has__aida_from_any__ == 1");
}
TEST_ADD (test_aida_cxxaux);

// Dummy handles for test purposes
struct TestOrbObject : OrbObject {
  TestOrbObject (ptrdiff_t x) : OrbObject (x) {}
};
struct OneHandle : RemoteHandle {
  OneHandle (OrbObjectP orbo) : RemoteHandle (orbo) {}
  static OneHandle down_cast (RemoteHandle smh) { return make_handle (smh.__aida_orbid__()); }
  static OneHandle make_handle (ptrdiff_t id)
  {
    std::shared_ptr<TestOrbObject> torbo = std::make_shared<TestOrbObject> (id);
    return OneHandle (torbo);
  }
};
class OneIface : public virtual ImplicitBase {
  int64 testid_;
public:
  virtual /*Des*/ ~OneIface () override     {}
  explicit         OneIface (int64 id) : testid_ (id) {}
  typedef std::shared_ptr<OneIface> OneIfaceP;
  // static Aida::BaseConnection* __aida_connection__();
  virtual std::string                    __typename__       () const override { return "Rapicorn::OneIface"; }
  virtual Aida::TypeHashList             __aida_typelist__  () const override { return TypeHashList(); }
  virtual const std::vector<String>&     __aida_aux_data__  () const override { static std::vector<String> sv; return sv; }
  virtual std::vector<String>            __aida_dir__ () const override                             { return std::vector<String>(); }
  virtual Any                            __aida_get__ (const String &name) const override           { return Any(); }
  virtual bool                           __aida_set__ (const String &name, const Any &any) override { return false; }
  int64 test_id() const { return testid_; }
  static OneIfaceP make_one_iface (int64 id)
  {
    typedef std::shared_ptr<OneIface> OneIfaceP;
    OneIfaceP oiface = std::make_shared<OneIface> (id);
    static std::vector<OneIfaceP> statics;
    statics.push_back (oiface);
    return oiface;
  }
};
typedef OneIface::OneIfaceP OneIfaceP;

static void
test_enum_info()
{
  const Aida::EnumInfo tke1 = Aida::enum_info<Aida::TypeKind>();
  const Aida::EnumInfo tke = tke1;
  TASSERT (not tke.name().empty() && tke.has_values());
  Aida::EnumValue ev = tke.find_value ("UNTYPED");
  TASSERT (ev.value == Aida::UNTYPED);
  ev = tke.find_value (Aida::STRING);
  TASSERT (ev.ident && String ("STRING") == ev.ident);
  TASSERT (type_kind_name (Aida::VOID) == String ("VOID"));
}
TEST_ADD (test_enum_info);

static void
test_handles()
{
  // RemoteHandle
  TASSERT (RemoteHandle::__aida_null_handle__() == NULL);
  TASSERT (!RemoteHandle::__aida_null_handle__());
  TASSERT (RemoteHandle::__aida_null_handle__().__aida_orbid__() == 0);
  std::shared_ptr<TestOrbObject> torbo = std::make_shared<TestOrbObject> (1);
  TASSERT (OneHandle (torbo) != NULL);
  TASSERT (OneHandle (torbo));
  TASSERT (OneHandle (torbo).__aida_orbid__() == 1);
  TASSERT (OneHandle (torbo).__aida_null_handle__() == NULL);
  TASSERT (!OneHandle (torbo).__aida_null_handle__());
  TASSERT (OneHandle (torbo).__aida_null_handle__().__aida_orbid__() == 0);
}
TEST_ADD (test_handles);

static void
test_any_basics()
{
  Any f7 (7);
  TASSERT (f7 == f7);
  // TASSERT (Foo { 7 } == f7.get<Foo>());
  Any b (Any { "BAR" });
  TASSERT (f7 != b);
  TASSERT (b == b);
  // TASSERT (f7.get<Foo>() == Foo { 7 });
  // TASSERT (f7.get<Foo>().i == 7);
  // TASSERT (b.get<Bar>().s == "BAR");
  Any f3 (3);
  TASSERT (f3 == f3);
  TASSERT (f3 != f7);
  f3.set (7);
  TASSERT (f3.get<int>() == 7);
  TASSERT (f3 == f7);
  f7.swap (b);
  TASSERT (f3 != f7);
  std::swap (f7, b); // uses move assignment
  TASSERT (f3 == f7);
  b.set (true);
  TASSERT (b.get<bool>() == true);
  b.clear();
  TASSERT (b.get<bool>() == false); // b is cleared, so get() yields default value
  const Any c (Any (5));
  TASSERT (c == Any (Any (5)));
  TASSERT (c != Any (Any (4)));
  b.set (TEST_COFFEE_COFFEE);
  TASSERT (b.kind() == ENUM);
  TASSERT (b.get<TestEnum>() == TEST_COFFEE_COFFEE);
  Any dup = b;
  TASSERT (dup.get_enum_typename() == "TestEnum");
  TASSERT (dup.get<TestEnum>() == TEST_COFFEE_COFFEE);
}
TEST_ADD (test_any_basics);

static void
test_any_conversions()
{
  Any a;
  a.set (bool (0));         assert (a.kind() == BOOL && a.get<int64>() == 0);
  a.set (bool (1));         assert (a.kind() == BOOL && a.get<int64>() == 1);
  a.set (1.);               assert (a.kind() == FLOAT64 && a.get<double>() == +1.0);
  a.set (-1.);              assert (a.kind() == FLOAT64 && a.get<double>() == -1.0);
  a.set (16.5e+6);          assert (a.get<double>() > 16000000.0 && a.get<double>() < 17000000.0);
  a.set (1);                assert (a.kind() == INT64 && a.get<int64>() == 1 && a.get<double>() == 1);
  a.set (-1);               assert (a.kind() == INT64 && a.get<int64>() == -1 && a.get<double>() == -1);
  a.set (int64_t (1));      assert (a.kind() == INT64 && a.get<int64>() == 1 && a.get<double>() == 1);
  a.set (int64_t (-1));     assert (a.kind() == INT64 && a.get<int64>() == -1 && a.get<double>() == -1);
  a.set (0);                assert (a.kind() == INT64 && a.get<int64>() == 0 && a.get<double>() == 0);
  a.set (32767199);         assert (a.kind() == INT64 && a.get<int64>() == 32767199);
  a.set ("");               assert (a.kind() == STRING && a.get<String>() == "" && a.get<bool>() == 0);
  a.set ("f");              assert (a.kind() == STRING && a.get<String>() == "f" && a.get<bool>() == 1);
  a.set ("123456789");      assert (a.kind() == STRING && a.get<String>() == "123456789" && a.get<bool>() == 1);
}
TEST_ADD (test_any_conversions);

static void AIDA_UNUSED
P (const Any &any, const String &name = "")
{
  if (!name.empty())
    printf ("%s=", name.c_str());
  printf ("%s\n", any.repr().c_str());
}

static void
test_any_equality()
{
  Any a, b, c, d;
  a.set (-3);               assert (a != b); assert (!(a == b));  c.set (a); d.set (b); assert (c != d); assert (!(c == d));
  a.set (Any());            assert (a != b); assert (!(a == b));  c.set (a); d.set (b); assert (c != d); assert (!(c == d));
  a = Any();                assert (a == b); assert (!(a != b));  c.set (a); d.set (b); assert (c == d); assert (!(c != d));
  b.set (Any());            assert (a != b); assert (!(a == b));  c.set (a); d.set (b); assert (c != d); assert (!(c == d));
  a.set (Any());            assert (a == b); assert (!(a != b));  c.set (a); d.set (b); assert (c == d); assert (!(c != d));
  a.set (13);  b.set (13);  assert (a == b); assert (!(a != b));  c.set (a); d.set (b); assert (c == d); assert (!(c != d));
  a.set (14);  b.set (15);  assert (a != b); assert (!(a == b));  c.set (a); d.set (b); assert (c != d); assert (!(c == d));
  a.set ("1"); b.set ("1"); assert (a == b); assert (!(a != b));  c.set (a); d.set (b); assert (c == d); assert (!(c != d));
  a.set ("1"); b.set (1);   assert (a != b); assert (!(a == b));  c.set (a); d.set (b); assert (c != d); assert (!(c == d));
  a.set (1.4); b.set (1.5); assert (a != b); assert (!(a == b));  c.set (a); d.set (b); assert (c != d); assert (!(c == d));
  a.set (1.6); b.set (1.6); assert (a == b); assert (!(a != b));  c.set (a); d.set (b); assert (c == d); assert (!(c != d));
}
TEST_ADD (test_any_equality);

static const double test_double_value = 7.76576e-306;

static void
any_test_set (Any &a, int what)
{
  switch (what)
    {
      typedef unsigned char uchar;
    case 0:  a.set (bool (0));                                     break;
    case 1:  a.set (bool (true));                                  break;
    case 2:  a.set (char (-117));                                  break;
    case 3:  a.set (uchar (250));                                  break;
    case 4:  a.set (int (-134217728));                             break;
    case 5:  a.set (uint (4294967295U));                           break;
    case 6:  a.set (long (-2147483648));                           break;
    case 7:  a.set (ulong (4294967295U));                          break;
    case 8:  a.set (int64_t (-0xc0ffeec0ffeeLL));                  break;
    case 9:  a.set (uint64_t (0xffffffffffffffffULL));             break;
    case 10: a.set ("Test4test");                                  break;
    case 11: a.set (test_double_value);                            break;
    case 12: { Any a2; a2.set ("SecondAny"); a.set (a2); }         break;
    case 13: a.set (TEST_COFFEE_COFFEE);                           break;
    case 14: a.set (OneHandle::make_handle (0x1f1f0c0c));          break;
    case 15: a.set (*OneIface::make_one_iface (0x2eeeabcd).get()); break;
    case 16: a.set (OneIface::make_one_iface (0x37afafaf));        break;
#define ANY_TEST_COUNT    17
    }
  // printerr ("SET: %d) Any=%p %i %s: %u\n", what, &a, a.kind(), type_kind_name(a.kind()), a.get<int64>());
}

static bool
any_test_get (const Any &a, int what)
{
  std::string s;
  EnumValue e;
  // printerr ("GET: %d) Any=%p %i %s: %u\n", what, &a, a.kind(), type_kind_name(a.kind()), a.get<int64>());
  OneHandle thandle (0);
  switch (what)
    {
      typedef unsigned char uchar;
      bool b; char c; uchar uc; int i; uint ui; long l; ulong ul; int64_t i6; uint64_t u6; double d;
    case 0:  b  = a.get<bool>();              assert (b == 0);                      break;
    case 1:  b  = a.get<bool>();              assert (b == true);                   break;
    case 2:  c  = a.get<char>();              assert (c == -117);                   break;
    case 3:  uc = a.get<uchar>();             assert (uc == 250);                   break;
    case 4:  i  = a.get<int>();               assert (i == -134217728);             break;
    case 5:  ui = a.get<uint>();              assert (ui == 4294967295U);           break;
    case 6:  l  = a.get<long>();              assert (l == -2147483648);            break;
    case 7:  ul = a.get<ulong>();             assert (ul == 4294967295U);           break;
    case 8:  i6 = a.get<int64_t>();           assert (i6 == -0xc0ffeec0ffeeLL);     break;
    case 9:  u6 = a.get<uint64_t>();          assert (u6 == 0xffffffffffffffffULL); break;
    case 10: s  = a.get<String>();            assert (s == "Test4test");            break;
    case 11: d  = a.get<double>();            assert (d == test_double_value);      break;
    case 12: s  = a.get<Any>().get<String>(); assert (s == "SecondAny");            break;
    case 13: i6 = a.get<TestEnum>();          assert (i6 == TEST_COFFEE_COFFEE);    break;
    case 14:
      assert (a.kind() == REMOTE);
      thandle = a.get<OneHandle>();
      assert (thandle.__aida_orbid__() == 0x1f1f0c0c);
      break;
    case 15:
      {
        assert (a.kind() == INSTANCE);
        OneIface &oiface = a.get<OneIface>();
        assert (oiface.test_id() == 0x2eeeabcd);
      }
      break;
    case 16:
      {
        assert (a.kind() == INSTANCE);
        OneIfaceP oiface = a.get<OneIfaceP>();
        assert (oiface && oiface->test_id() == 0x37afafaf);
      }
      break;
    }
  return true;
}

static Any any5 () { return Any(5); }

static void
test_any_storage()
{
  {
    Any ax1 = any5();
    TASSERT (ax1 == any5());
    Any a5 = any5();
    Any ax2 = std::move (a5);
    TASSERT (ax2 == any5());
  }
  String s;
  const size_t cases = ANY_TEST_COUNT;
  for (size_t j = 0; j <= cases; j++)
    for (size_t k = 0; k <= cases; k++)
      {
        size_t cs[2] = { j, k };
        for (size_t cc = 0; cc < 2; cc++)
          {
            Any a;
            any_test_set (a, cs[cc]);
            const bool any_getter_successfull = any_test_get (a, cs[cc]);
            assert (any_getter_successfull == true);
            Any a2 (a);
            TASSERT (a2.kind() == a.kind());
            const bool any_copy_successfull = any_test_get (a2, cs[cc]);
            assert (any_copy_successfull == true);
            Any a3;
            a3 = a2;
            const bool any_assignment_successfull = any_test_get (a2, cs[cc]);
            assert (any_assignment_successfull == true);
          }
      }
}
TEST_ADD (test_any_storage);

static void
test_any_containers()
{
  // -- AnyDict --
  Any any1 ("any1"), any2;
  any2.set (any1);
  Any::AnyDict fv;
  fv.push_back (Any::Field ("otto", 7.7));
  fv.push_back (Any::Field ("anna", 3));
  fv.push_back (Any::Field ("ida", "ida"));
  fv.push_back (Any::Field ("any2", any2));
  assert (fv[0].name == "otto" && fv[0].get<double>() == 7.7);
  assert (fv[1].name == "anna" && fv[1].get<int64>() == 3);
  assert (fv[2].name == "ida" && fv[2].get<String>() == "ida");
  assert (fv[3].name == "any2" && fv[3].get<Any>().get<String>() == "any1");
  Any::AnyDict gv = fv;
  assert (fv == gv);
  gv[1].set (5);
  assert (fv != gv);
  gv[1].set (int64 (3));
  assert (fv == gv);
  // -- AnyList --
  Any::AnyList av;
  av.push_back (Any (7.7));
  av.push_back (Any (3));
  av.push_back (Any ("ida"));
  av.push_back (any2);
  assert (av[0].get<double>() == 7.7);
  assert (av[1].get<int64>() == 3);
  assert (av[2].get<String>() == "ida");
  assert (av[3].kind() == ANY);
  Any::AnyList bv;
  assert (av != bv);
  for (auto const &f : fv)
    bv.push_back (f);
  if (0)
    for (size_t i = 0; i < av.size(); i++)
      dprintf (2, "%s: av[%zd]==bv[%zd]) %p %p | %s %s | %ld %ld\n", __func__, i, i,
               &av[i], &bv[i],
               type_kind_name(av[i].kind()), type_kind_name(bv[i].kind()),
               av[i].get<int64>(), bv[i].get<int64>());
  assert (av == bv);
  // -- AnyDict & AnyList --
  if (0)        // compare av (DynamicSequence) with fv (DynamicRecord)
    dprintf (2, "test-compare: %s == %s\n", Any (av).to_string().c_str(), Any (fv).to_string().c_str());
  Any::AnyList cv (fv.begin(), fv.end());     // initialize AnyList with { 7.7, 3, "ida" } from AnyDict (Field is-a Any)
  assert (av == cv);                            // as AnyList (AnyDict) copy, both vectors contain { 7.7, 3, "ida" }
  Any arec (fv), aseq (av);
  assert (arec != aseq);
  const Any::AnyDict *arv = arec.get<const Any::AnyDict*>();
  assert (*arv == fv);
  const Any::AnyList *asv = aseq.get<const Any::AnyList*>();
  assert (*asv == av);
}
TEST_ADD (test_any_containers);

} // Anon

int
main (int argc, const char **argv)
{
  return TestChain::run (argc, argv);
}
