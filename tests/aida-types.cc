// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <bse/testing.hh>
#include <bse/bseutils.hh>

using std::ptrdiff_t;
using namespace Aida;

enum TestEnum { TEST_COFFEE_COFFEE = -0xc0ffeec0ffeeLL };

// Manually adding TestEnum introspection information for basic testing
static const IntrospectionRegistry __aida_aux_data_srvt__Aida_TestEnum_ = {
  "typename=TestEnum\0"
  "type=ENUM\0"
  "enumerators=TEST_COFFEE_COFFEE\0"
  "TEST_COFFEE_COFFEE.value=-0xc0ffeec0ffee\0"
};

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
}
TEST_ADD (test_aida_cxxaux);

// Dummy handles for test purposes
class OneIface :
    public virtual ImplicitBase,
    public virtual EnableSharedFromThis<OneIface>
{
  int64 testid_;
public:
  virtual /*Des*/ ~OneIface () override     {}
  explicit         OneIface (int64 id) : testid_ (id) {}
  typedef std::shared_ptr<OneIface> OneIfaceP;
  // static Aida::BaseConnection* __aida_connection__();
  virtual Aida::StringVector         __typelist_mt__          () const override { return { "OneIface" }; }
  virtual Aida::ExecutionContext&    __execution_context_mt__ () const override    { return Bse::execution_context(); }
  virtual bool                       __access__               (const std::string &propertyname, const PropertyAccessorPred&) override { return false; }
  virtual Aida::IfaceEventConnection __attach__               (const String &eventselector, EventHandlerF handler) override
  { Aida::IfaceEventConnection *c = NULL; AIDA_ASSERT_RETURN_UNREACHED (*c); }
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
test_aida_enum_info()
{
  const StringVector &atk = Aida::Introspection::find_type ("Aida.TypeKind");
  TASSERT (atk.size() > 0);
  const StringVector &atk_cxx = Aida::Introspection::find_type ("Aida::TypeKind");
  TASSERT (atk_cxx.size() > 0 && atk_cxx == atk);
  Aida::TypeKind k = enum_value_from_string<Aida::TypeKind> ("UNTYPED");
  TASSERT (k == Aida::TypeKind::UNTYPED);
  const String enumerators_value = Aida::Introspection::find_value ("enumerators", atk);
  const StringVector enumerators = Bse::string_split (enumerators_value, ";");
  TASSERT (enumerators.size() && enumerators[0] == "UNTYPED");
  const String STRING_name = Aida::Introspection::enumerator_from_value ("Aida.TypeKind", Aida::TypeKind::STRING);
  TASSERT (STRING_name == "Aida.TypeKind.STRING");
  const char *STRING_c_str = Aida::Introspection::legacy_enumerator ("Aida.TypeKind", Aida::TypeKind::STRING);
  TASSERT (STRING_name == STRING_c_str);
  const String STRING_name2 = Aida::enum_value_to_string<Aida::TypeKind> (Aida::TypeKind::STRING);
  TASSERT (STRING_name == STRING_name2);
  const String FLOAT64_value = Aida::Introspection::find_value ("FLOAT64.value", atk);
  const int64_t FLOAT64_int = string_to_int (FLOAT64_value);
  TASSERT (Aida::TypeKind::FLOAT64 == FLOAT64_int);
  const String VOID_name = Aida::Introspection::enumerator_from_value ("Aida.TypeKind", Aida::TypeKind::VOID);
  TASSERT (VOID_name == "Aida.TypeKind.VOID");
  const String VOID_name2 = type_kind_name (Aida::VOID);
  TASSERT (VOID_name == VOID_name2);
  TASSERT (Introspection::match_enumerator ("VOID", "Aida.TypeKind") == "Aida.TypeKind.VOID");
  TASSERT (Introspection::match_enumerator ("Aida.TypeKind.VOID", "") == "Aida.TypeKind.VOID");
  TASSERT (Introspection::match_enumerator ("Aida.TypeKind.VOID", "Anonymous.No.Such.Thing") == "Aida.TypeKind.VOID");
  int64 i;
  i = Introspection::enumerator_to_value ("Aida.TypeKind.INT64");
  TASSERT (i == Aida::TypeKind::INT64);
  i = Introspection::enumerator_to_value ("INT64", "Aida.TypeKind");
  TASSERT (i == Aida::TypeKind::INT64);
  i = Introspection::enumerator_to_value ("INT64", "::Aida::TypeKind");
  TASSERT (i == Aida::TypeKind::INT64);
  i = Introspection::enumerator_to_value ("INT64", "Aida.TypeKind.SOME_SIBLING");
  TASSERT (i == Aida::TypeKind::INT64);
  i = Introspection::enumerator_to_value ("Aida.TypeKind.bool");
  TASSERT (i == Aida::TypeKind::BOOL);
  i = Introspection::enumerator_to_value ("NT64", "Aida.TypeKind");
  TASSERT (i != Aida::TypeKind::INT64);
}
TEST_ADD (test_aida_enum_info);

static void
test_aida_any_basics()
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
  TASSERT (dup.get<ENUM>() == "TestEnum.TEST_COFFEE_COFFEE");
  TASSERT (dup.get<TestEnum>() == TEST_COFFEE_COFFEE);
}
TEST_ADD (test_aida_any_basics);

static void
test_aida_any_conversions()
{
  Any a;
  a.set (bool (0));         TASSERT (a.kind() == BOOL && a.get<int64>() == 0);
  a.set (bool (1));         TASSERT (a.kind() == BOOL && a.get<int64>() == 1);
  a.set (1.);               TASSERT (a.kind() == FLOAT64 && a.get<double>() == +1.0);
  a.set (-1.);              TASSERT (a.kind() == FLOAT64 && a.get<double>() == -1.0);
  a.set (16.5e+6);          TASSERT (a.get<double>() > 16000000.0 && a.get<double>() < 17000000.0);
  a.set (1);                TASSERT (a.kind() == INT64 && a.get<int64>() == 1 && a.get<double>() == 1);
  a.set (-1);               TASSERT (a.kind() == INT64 && a.get<int64>() == -1 && a.get<double>() == -1);
  a.set (int64_t (1));      TASSERT (a.kind() == INT64 && a.get<int64>() == 1 && a.get<double>() == 1);
  a.set (int64_t (-1));     TASSERT (a.kind() == INT64 && a.get<int64>() == -1 && a.get<double>() == -1);
  a.set (0);                TASSERT (a.kind() == INT64 && a.get<int64>() == 0 && a.get<double>() == 0);
  a.set (32767199);         TASSERT (a.kind() == INT64 && a.get<int64>() == 32767199);
  a.set ("");               TASSERT (a.kind() == STRING && a.get<String>() == "" && a.get<bool>() == 0);
  a.set ("f");              TASSERT (a.kind() == STRING && a.get<String>() == "f" && a.get<bool>() == 1);
  a.set ("123456789");      TASSERT (a.kind() == STRING && a.get<String>() == "123456789" && a.get<bool>() == 1);
  a.set<TypeKind> (INT32);  TASSERT (a.kind() == ENUM && a.get<String>() == "Aida.TypeKind.INT32" && a.get<TypeKind>() == INT32);
  a.set<TypeKind> (FLOAT64); TASSERT (a.kind() == ENUM && a.get<String>() == "Aida.TypeKind.FLOAT64" && a.get<TypeKind>() == FLOAT64);
  a.set<ENUM> ("Aida.TypeKind.FLOAT64"); TASSERT (a.kind() == ENUM && a.get<String>() == "Aida.TypeKind.FLOAT64");
  a.set<ENUM> ("INT64");    TASSERT (a.kind() == ENUM && a.get<String>() == "INT64" && a.get<TypeKind>() == INT64);
}
TEST_ADD (test_aida_any_conversions);

static void AIDA_UNUSED
P (const Any &any, const String &name = "")
{
  if (!name.empty())
    printf ("%s=", name.c_str());
  printf ("%s\n", any.repr().c_str());
}

static void
test_aida_any_equality()
{
  Any a, b, c, d;
  a.set (-3);               TASSERT (a != b); TASSERT (!(a == b));  c.set (a); d.set (b); TASSERT (c != d); TASSERT (!(c == d));
  a.set (Any());            TASSERT (a != b); TASSERT (!(a == b));  c.set (a); d.set (b); TASSERT (c != d); TASSERT (!(c == d));
  a = Any();                TASSERT (a == b); TASSERT (!(a != b));  c.set (a); d.set (b); TASSERT (c == d); TASSERT (!(c != d));
  b.set (Any());            TASSERT (a != b); TASSERT (!(a == b));  c.set (a); d.set (b); TASSERT (c != d); TASSERT (!(c == d));
  a.set (Any());            TASSERT (a == b); TASSERT (!(a != b));  c.set (a); d.set (b); TASSERT (c == d); TASSERT (!(c != d));
  a.set (13);  b.set (13);  TASSERT (a == b); TASSERT (!(a != b));  c.set (a); d.set (b); TASSERT (c == d); TASSERT (!(c != d));
  a.set (14);  b.set (15);  TASSERT (a != b); TASSERT (!(a == b));  c.set (a); d.set (b); TASSERT (c != d); TASSERT (!(c == d));
  a.set ("1"); b.set ("1"); TASSERT (a == b); TASSERT (!(a != b));  c.set (a); d.set (b); TASSERT (c == d); TASSERT (!(c != d));
  a.set ("1"); b.set (1);   TASSERT (a != b); TASSERT (!(a == b));  c.set (a); d.set (b); TASSERT (c != d); TASSERT (!(c == d));
  a.set (1.4); b.set (1.5); TASSERT (a != b); TASSERT (!(a == b));  c.set (a); d.set (b); TASSERT (c != d); TASSERT (!(c == d));
  a.set (1.6); b.set (1.6); TASSERT (a == b); TASSERT (!(a != b));  c.set (a); d.set (b); TASSERT (c == d); TASSERT (!(c != d));
}
TEST_ADD (test_aida_any_equality);

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
#define ANY_TEST_COUNT    13
    }
  // printerr ("SET: %d) Any=%p %i %s: %u\n", what, &a, a.kind(), type_kind_name(a.kind()), a.get<int64>());
}

static bool
any_test_get (const Any &a, int what)
{
  std::string s;
  // printerr ("GET: %d) Any=%p %i %s: %u\n", what, &a, a.kind(), type_kind_name(a.kind()), a.get<int64>());
  switch (what)
    {
      typedef unsigned char uchar;
      bool b; char c; uchar uc; int i; uint ui; long l; ulong ul; int64_t i6; uint64_t u6; double d;
    case 0:  b  = a.get<bool>();              TASSERT (b == 0);                      break;
    case 1:  b  = a.get<bool>();              TASSERT (b == true);                   break;
    case 2:  c  = a.get<char>();              TASSERT (c == -117);                   break;
    case 3:  uc = a.get<uchar>();             TASSERT (uc == 250);                   break;
    case 4:  i  = a.get<int>();               TASSERT (i == -134217728);             break;
    case 5:  ui = a.get<uint>();              TASSERT (ui == 4294967295U);           break;
    case 6:  l  = a.get<long>();              TASSERT (l == -2147483648);            break;
    case 7:  ul = a.get<ulong>();             TASSERT (ul == 4294967295U);           break;
    case 8:  i6 = a.get<int64_t>();           TASSERT (i6 == -0xc0ffeec0ffeeLL);     break;
    case 9:  u6 = a.get<uint64_t>();          TASSERT (u6 == 0xffffffffffffffffULL); break;
    case 10: s  = a.get<String>();            TASSERT (s == "Test4test");            break;
    case 11: d  = a.get<double>();            TASSERT (d == test_double_value);      break;
    case 12: s  = a.get<Any>().get<String>(); TASSERT (s == "SecondAny");            break;
    case 13: i6 = a.get<TestEnum>();          TASSERT (i6 == TEST_COFFEE_COFFEE);    break;
    }
  return true;
}

static Any any5 () { return Any(5); }

static void
test_aida_any_storage()
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
            TASSERT (any_getter_successfull == true);
            Any a2 (a);
            TASSERT (a2.kind() == a.kind());
            const bool any_copy_successfull = any_test_get (a2, cs[cc]);
            TASSERT (any_copy_successfull == true);
            Any a3;
            a3 = a2;
            const bool any_assignment_successfull = any_test_get (a2, cs[cc]);
            TASSERT (any_assignment_successfull == true);
          }
      }
}
TEST_ADD (test_aida_any_storage);

static void
test_aida_any_containers()
{
  // -- AnyRec --
  Any any1 ("any1"), any2;
  any2.set (any1);
  Any::AnyRec fv;
  fv.push_back (Any::Field ("otto", 7.7));
  fv.push_back (Any::Field ("anna", 3));
  fv.push_back (Any::Field ("ida", "ida"));
  fv.push_back (Any::Field ("any2", any2));
  TASSERT (fv[0].name == "otto" && fv[0].get<double>() == 7.7);
  TASSERT (fv[1].name == "anna" && fv[1].get<int64>() == 3);
  TASSERT (fv[2].name == "ida" && fv[2].get<String>() == "ida");
  TASSERT (fv[3].name == "any2" && fv[3].get<Any>().get<String>() == "any1");
  Any::AnyRec gv = fv;
  TASSERT (fv == gv);
  gv[1].set (5);
  TASSERT (fv != gv);
  gv[1].set (int64 (3));
  TASSERT (fv == gv);
  // -- AnySeq --
  Any::AnySeq av;
  av.push_back (Any (7.7));
  av.push_back (Any (3));
  av.push_back (Any ("ida"));
  av.push_back (any2);
  TASSERT (av[0].get<double>() == 7.7);
  TASSERT (av[1].get<int64>() == 3);
  TASSERT (av[2].get<String>() == "ida");
  TASSERT (av[3].kind() == ANY);
  Any::AnySeq bv;
  TASSERT (av != bv);
  for (auto const &f : fv)
    bv.push_back (f);
  if (0)
    for (size_t i = 0; i < av.size(); i++)
      dprintf (2, "%s: av[%zd]==bv[%zd]) %p %p | %s %s | %ld %ld\n", __func__, i, i,
               &av[i], &bv[i],
               type_kind_name(av[i].kind()), type_kind_name(bv[i].kind()),
               av[i].get<int64>(), bv[i].get<int64>());
  TASSERT (av == bv);
  // -- AnyRec & AnySeq --
  if (0)        // compare av (DynamicSequence) with fv (DynamicRecord)
    dprintf (2, "test-compare: %s == %s\n", Any (av).to_string().c_str(), Any (fv).to_string().c_str());
  Any::AnySeq cv (fv.begin(), fv.end());     // initialize AnySeq with { 7.7, 3, "ida" } from AnyRec (Field is-a Any)
  TASSERT (av == cv);                            // as AnySeq (AnyRec) copy, both vectors contain { 7.7, 3, "ida" }
  Any arec (fv), aseq (av);
  TASSERT (arec != aseq);
  const Any::AnyRec *arv = arec.get<const Any::AnyRec*>();
  TASSERT (*arv == fv);
  const Any::AnySeq *asv = aseq.get<const Any::AnySeq*>();
  TASSERT (*asv == av);
}
TEST_ADD (test_aida_any_containers);

} // Anon
