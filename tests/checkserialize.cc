// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "explore.hh"
#include <bse/testing.hh>
#include <bse/serialize.hh>

using namespace Bse;

enum Unconvertible { SMALL = -2, BIG = 0x1234beefc0ffe007LL };
enum Counts { ZERO, ONE, TWO, THREE };

inline std::string
to_string (Counts e)
{
  switch (e) {
  case THREE:   return "THREE";
  case TWO:     return "TWO";
  case ONE:     return "ONE";
  case ZERO:    return "ZERO";
  default:      return "(invlaid)";
  }
}
inline bool
from_string (const std::string &s, Counts &e)
{
  e = ZERO;
  if (s == "THREE")     e = THREE;
  if (s == "TWO")       e = TWO;
  if (s == "ONE")       e = ONE;
  return true;
}

static_assert (HasEnumAsString<Unconvertible>::value == false, "");
static_assert (HasEnumAsString<Counts>::value == true, "");

template<class T> bool
approximately_equal (T a, T b, T epsilon)
{
  using std::abs;
  if (0)
    printerr ("%s: %g == %g : %d (diff=%g, eps=%g)\n", __func__, a, b,
              abs (a - b) <= epsilon * (abs (a) < abs (b) ? abs (b) : abs (a)),
              abs (a - b), epsilon);
  return abs (a - b) <= epsilon * (abs (a) < abs (b) ? abs (b) : abs (a));
}
template<class T> bool
deref_equal (T *a, T *b)
{
  return a && b ? *a == *b : a == b;
}

// == Multiple Inheritance Test ==
#define VDTOR(CLASS)    virtual ~CLASS() = default // provide dummy virtual dtor to silence compiler
#define VMF(CLASS)      CLASS() = default; CLASS& operator=(const CLASS&) = default; VDTOR(CLASS)
struct A                { String name() { return "A"; } virtual String vname() { return name(); } VMF (A); };   //            A
struct B                { String name() { return "B"; } virtual String vname() { return name(); } VMF (B); };   //           / \    B
struct C : virtual A    { String name() { return "C"; } virtual String vname() { return name(); } VMF (C); };   //          C   \   |
struct D : virtual A    { String name() { return "D"; } virtual String vname() { return name(); } VMF (D); };   //           \   D  |
struct E :         B    { String name() { return "E"; } virtual String vname() { return name(); } };            //            \ /   E
struct F :         C, D { String name() { return "F"; } virtual String vname() { return name(); } };            //             F   /
struct G :         F, E { String name() { return "G"; } virtual String vname() { return name(); } VMF (G); };   //              `G'
using Bse::serialize_content; // also keep using the serialize_content<>() template

// BSE_SERIALIZATION_EXPORT (C); // <- not working, C lacks serialize_content()

static void
serialize_content (A &self, SerializeContext &sc)
{
  std::string str = "Class A uses BSE_SERIALIZATION_EXPORT";
  sc["Note"] ->* str;
  str = self.name(); sc["name"] ->* str;
  str = self.vname(); sc["vname"] ->* str;
  TASSERT (self.vname() == str);
}
BSE_SERIALIZATION_EXPORT (A);

BSE_SERIALIZATION_EXPORT (C); // <- works, falls back to serialize_content(A&)

static void
serialize_content (B &self, SerializeContext &sc)
{
  std::string str = "Class B is not exported as serialization type";
  sc["Note"] ->* str;
  str = self.name(); sc["name"] ->* str;
  str = self.vname(); sc["vname"] ->* str;
}
BSE_SERIALIZATION_EXPORT (B);

static void
serialize_content (G &self, SerializeContext &sc)
{
  std::string str = "Class G uses BSE_SERIALIZATION_EXPORT";
  sc["Note"] ->* str;
  std::string name_ = self.name();
  sc["name"] ->* name_;
  std::string vname_ = self.vname();
  sc["vname"] ->* vname_;
  TASSERT (self.vname() == vname_);
  // FIXME: TASSERT (name_ == vname_);
}
BSE_SERIALIZATION_EXPORT (G);

struct MiContainer {
  B                  b;
  C                  c;
  G                 *g = NULL;
  E                 *e = NULL;
  std::shared_ptr<D> dp = NULL;
  std::shared_ptr<G> gp = NULL;
  MiContainer (std::shared_ptr<G> px = NULL) :
    g (px.get()), e (g), dp (px), gp (px)
  {
#if 0
    Bse::printerr ("Testing slow_cast...\n");
    G g;
    Bse::printerr ("have   G: %p (void: %p)\n", &g, dynamic_cast<void*> (&g));
    E *p = &g;
    try {
      Bse::printerr ("throw  …: %p (void: %p)\n", p, dynamic_cast<void*> (p));
      throw p;
    }
    catch (G *v) { Bse::printerr ("caught G: %p\n", v); }
    catch (F *v) { Bse::printerr ("caught F: %p\n", v); }
    catch (E *v) { Bse::printerr ("caught E: %p\n", v); }
    catch (D *v) { Bse::printerr ("caught D: %p\n", v); }
    catch (C *v) { Bse::printerr ("caught C: %p\n", v); }
    catch (B *v) { Bse::printerr ("caught B: %p\n", v); }
    catch (A *v) { Bse::printerr ("caught A: %p\n", v); }
    catch (...) {
      Bse::printerr ("caught ...\n");
    }
#endif
  }
  virtual void
  serialize_content (SerializeContext &sc) {
    sc["b"] ->* b;
    sc["c"] ->* c;
    sc["dp"] ->* dp;    // First serializatoin of G() through serialize_content(D&)
    sc["e"] ->* e;
    sc["g"] ->* g;
    sc["gp"] ->* gp;
    // NOTE, sc.disown_pointer() is not needed here, sincs ownership is assumed by dp and e, g, gp point to the same
  }
  VDTOR (MiContainer);
  MiContainer& operator= (const MiContainer&) = default;
};
BSE_SERIALIZATION_EXPORT (MiContainer);

static void
serialize_multiple_inheritance()
{
  // for later tests, we're asserting that different pointers to the same MI object have different addresses
  std::shared_ptr<G> shared_g = std::make_shared<G>();
  G &impl = *shared_g;
  A *a = &impl;
  B *b = &impl;
  C *c = &impl;
  D *d = &impl;
  E *e = &impl;
  F *f = &impl;
  G *g = &impl;
  TASSERT (ptrdiff_t (a) != ptrdiff_t (b));
  TASSERT (ptrdiff_t (c) != ptrdiff_t (d));
  TASSERT (ptrdiff_t (d) != ptrdiff_t (e));
  TASSERT (ptrdiff_t (e) != ptrdiff_t (a));
  TASSERT (ptrdiff_t (e) == ptrdiff_t (b));
  TASSERT (ptrdiff_t (e) != ptrdiff_t (c));
  TASSERT (ptrdiff_t (e) != ptrdiff_t (d));
  TASSERT (ptrdiff_t (e) != ptrdiff_t (f));
  TASSERT (ptrdiff_t (f) == ptrdiff_t (a));
  TASSERT (ptrdiff_t (g) != ptrdiff_t (d));
  TASSERT (e->name() == "E" && e->vname() == "G");
  TASSERT (f->name() == "F" && f->vname() == "G");

  MiContainer mic (shared_g);
  TASSERT (mic.e == e && mic.e == mic.g);
  TASSERT (ptrdiff_t (mic.e) != ptrdiff_t (mic.g));
  TASSERT (mic.e->name() == "E" && mic.e->vname() == "G");
  TASSERT (mic.g->name() == "G" && mic.g->vname() == "G");
  TASSERT (mic.dp.get() == mic.g && mic.dp.get() == mic.gp.get());
  TASSERT (ptrdiff_t (mic.dp.get()) != ptrdiff_t (mic.gp.get()));
  TASSERT (mic.dp->name() == "D" && mic.dp->vname() == "G");
  TASSERT (mic.gp->name() == "G" && mic.dp->vname() == "G");

  SerializeToXML so ("bse", version());
  so.save (mic);
  printout ("<?xml version='1.0' encoding='UTF-8'?>\n%s", so.to_xml());

  mic = MiContainer();
  TASSERT (mic.e == NULL && mic.g == NULL && mic.dp == NULL && mic.gp == NULL);

  SerializeFromXML si (so.to_xml());
  if (si.in_error())
    warning ("SerializeFromXML: %s", si.error());

  bool success = si.load (mic);
  TASSERT (success == true);
  TASSERT (mic.e != NULL && mic.g != NULL && mic.dp != NULL && mic.gp != NULL);
  TASSERT (mic.e != e && mic.e == mic.g);
  TASSERT (ptrdiff_t (mic.e) != ptrdiff_t (mic.g));
  TASSERT (mic.e->name() == "E" && mic.e->vname() == "G");
  TASSERT (mic.g->name() == "G" && mic.g->vname() == "G");
  TASSERT (mic.dp.get() == mic.g && mic.dp.get() == mic.gp.get());
  TASSERT (ptrdiff_t (mic.dp.get()) != ptrdiff_t (mic.gp.get()));
  TASSERT (mic.dp->name() == "D" && mic.dp->vname() == "G");
  TASSERT (mic.gp->name() == "G" && mic.gp->vname() == "G");
}
TEST_ADD (serialize_multiple_inheritance);

// == OtherObject ==
namespace { // Anon
struct OtherObject {
  char a = 0;
  float f = 0;
  int64 i = 0;
  String s = "";
  bool b = false;
  Counts c = ZERO;
  virtual ~OtherObject () {}
  bool
  operator== (const OtherObject &o) const
  {
    return o.a == a && o.f == f && o.i == i && o.s == s && o.b == b && o.c == c;
  }
  virtual void
  serialize_content (SerializeContext &sc)
  {
    sc["count"] ->* c;
    sc["a"] ->* a;
    sc["f"] ->* f;
    sc["i"] ->* i;
    sc["s"] ->* s;
    sc["toggle"] ->* b;
  }
  void
  populate()
  {
    static int counter = 0;
    f = -1234.56 + counter++;
    a = 'a' + ++counter;
    c = Counts (counter++);
    i = 990 + counter++;
    s = string_format ("\tHello %x\t", ++counter);
    b = counter % 2;
  }
};

BSE_SERIALIZATION_NAMED_EXPORT (OtherObject, "OtherO");
} // Anon

struct DerivedObject : virtual OtherObject {
  String msg = " DerivedObject *only* exports its own properties! ";
  virtual void
  serialize_content (SerializeContext &sc)
  {
    if (0) // we avoid chaining to get only msg across
      ::serialize_content (static_cast<OtherObject&> (*this), sc);
    sc["msg"] ->* msg;
  }
};
BSE_SERIALIZATION_EXPORT (DerivedObject);

struct StupidBase {
  virtual     ~StupidBase        () {}
  virtual bool operator==        (const StupidBase &o) const { return true; }
  virtual void serialize_content (SerializeContext &sc) {}
  virtual void populate          () {}
};

struct CleverObject : virtual StupidBase {
  String slogan = "___________________________________________________________-";
  virtual void
  populate() override
  {
    StupidBase::populate();
    slogan = " CleverObject requires BSE_SERIALIZATION_EXPORT to be serialized ";
  }
  virtual bool
  operator== (const StupidBase &o) const override
  {
    const CleverObject *p = dynamic_cast<const CleverObject*> (&o);
    return p && p->slogan == slogan && StupidBase::operator== (o);
  }
  virtual void
  serialize_content (SerializeContext &sc) override
  {
    // chain to base type in order to serialize ancestry properties as well
    StupidBase::serialize_content (sc);
    sc["slogan"] ->* slogan;
  }
};
BSE_SERIALIZATION_EXPORT (CleverObject); // required, saved through base pointer

// == ExampleObject ==
class ExampleObject;
#define PROPS(sc, Class, propertyname)   serialize_property (sc, #propertyname, this, &Class::propertyname, &Class::propertyname)

struct LinkRecord {
  ExampleObject *circularobject = NULL;
  float          circularfactor = 0;
};
static void serialize_content (LinkRecord &self, SerializeContext &sc); // Has_serialize_content<> via ADL
static_assert (Has_serialize_content<LinkRecord>::value == true, "");

class ExampleObject : public SerializableBase, public virtual Aida::EnableSharedFromThis<ExampleObject> {
  int asint_ = 0;             int          asint   () const { return asint_; }   void asint   (int           val) { asint_ = val; }
  double asfloat_ = 0;        double       asfloat () const { return asfloat_; } void asfloat (double        val) { asfloat_ = val; }
  String astr_ = "";          String       astr    () const { return astr_; }    void astr    (const String &val) { astr_ = val; }
  Counts count_ = ZERO;       Counts       count   () const { return count_; }   void count   (Counts        val) { count_ = val; }
  StupidBase *stupid_ = NULL; StupidBase*  stupid  () const { return stupid_; }  void stupid  (StupidBase   *val) { stupid_ = val; }
  OtherObject *other_ = NULL; OtherObject* other   () const { return other_; }   void other   (OtherObject  *val) { other_ = val; }
  OtherObject &do_ = *new DerivedObject();
  float          circularfactor_ = -9999.9999999;
  ExampleObject *circularobject_ = NULL;
  Unconvertible uc1_ = SMALL, uc2_ = BIG;
  uint64             uint64_ = 0;
  std::vector<int64> int64s_;
  std::vector< std::vector<long> > longs_;
  std::vector<OtherObject*> parts_;
public:
  ExampleObject() :
    circularobject_ (this)
  {}
  virtual
  ~ExampleObject()
  {
    delete &do_;
  }
  void
  populate()
  {
    static int64 seed = 0x1234feac;
    dynamic_cast<DerivedObject*> (&do_)->msg += string_format (" (%d) ", seed++ - 0x1234feac);
    asint_ = seed++ % 13;
    asfloat_ = seed++ / -1e13;
    astr_ = string_format (" %p ", (void*) ++seed);
    uint64_ = 18446744073709551615ull;
    int64s_ = { -9223372036854775807-1, +9223372036854775807 };
    longs_.push_back (std::vector<long> { 9, 8, 7 });
    longs_.push_back (std::vector<long> { seed++ });
    count_ = Counts (seed++ % 4);
    if (stupid_)
      delete stupid_;
    stupid_ = new CleverObject();
    stupid_->populate();
    if (other_)
      delete other_;
    other_ = new OtherObject();
    other_->populate();
    circularfactor_ = -(seed % 79) + -1.0 / seed;
    parts_.push_back (new OtherObject());
    parts_.push_back (new OtherObject());
    parts_.push_back (new OtherObject());
    parts_.push_back (NULL);
    parts_.push_back (parts_[0]);
    parts_[0]->i = 1;
    parts_[0]->s = " ** FIRST of parts_ ** ";
    parts_[0]->c = ONE;
    parts_[0]->f = 0.0;
    parts_[1]->i = 2;
    parts_[1]->b = 1;
    parts_[1]->c = TWO;
    parts_[1]->f = 1.0;
    parts_[2]->i = 3;
    parts_[2]->s = "Last in ExampleObject.parts_";
    parts_[2]->c = THREE;
    parts_[2]->f = 2.0;
    parts_[4]->f += 0.444;
  }
  bool
  operator== (const ExampleObject &o) const
  {
    const bool parts_eq = o.parts_.size() == parts_.size() &&
                          std::equal (std::begin (o.parts_), std::end (o.parts_), std::begin (parts_),
                                      [] (const OtherObject *lhs, const OtherObject *rhs) {
                                        return deref_equal (lhs, rhs);
                                      });
    return (parts_eq &&
            o.asint_ == asint_ &&
            approximately_equal (o.asfloat_, asfloat_, 1e-6) &&
            o.astr_ == astr_ &&
            o.count_ == count_ &&
            deref_equal (o.stupid_, stupid_) &&
            deref_equal (o.other_, other_) &&
            approximately_equal (o.circularfactor_, circularfactor_, 1e-5f) &&
            (&o == o.circularobject_) == (this == circularobject_) &&
            o.uc1_ == uc1_ &&
            o.uc2_ == uc2_ &&
            o.uint64_ == uint64_ &&
            o.int64s_ == int64s_ &&
            o.longs_ == longs_);
  }
  virtual void
  serialize_content (SerializeContext &sc) override
  {
    SerializableBase::serialize_content (sc);
    sc["uc1"] ->* uc1_;
    sc["uc2"] ->* uc2_;
    serialize_property (sc, "asint", this, &ExampleObject::asint, &ExampleObject::asint);
    PROPS (sc, ExampleObject, astr);
    sc["uint64"] ->* uint64_;
    sc["int64s"] ->* int64s_;
    sc["longs"] ->* longs_;
    PROPS (sc, ExampleObject, count);
    sc["different"] ->* do_;
    PROPS (sc, ExampleObject, asfloat);
    PROPS (sc, ExampleObject, stupid);
    PROPS (sc, ExampleObject, other);
    if (0)
      {
        std::vector<long double> floats = { -1e+10, -1e+100, -1e+1000L };
        sc["floats"] ->* floats;
        std::vector<String> strings = { "a", " lt=< and=& gt=> ", "", "d" };
        sc["strings"] ->* strings;
        std::vector<Counts> counts = { THREE, TWO, ONE };
        sc["counts"] ->* counts;
      }
    LinkRecord lrec;
    if (sc.in_save())
      {
        lrec.circularobject = circularobject_;
        lrec.circularfactor = circularobject_ ? circularfactor_ : 0;
      }
    sc["link"] ->* lrec;
    if (sc.in_load())
      {
        circularobject_ = lrec.circularobject;
        circularfactor_ = lrec.circularfactor;
      }
    sc["parts"] ->* parts_;
  }
};
BSE_SERIALIZATION_EXPORT (ExampleObject);

static void
serialize_content (LinkRecord &self, SerializeContext &sc)
{
  sc["circularobject"] ->* self.circularobject;
  sc["circularfactor"] ->* self.circularfactor;
}


static void
test_serialize_content()
{
  TASSERT (SerializeContext::version_after ("xy1", "xy1") == true);
  TASSERT (SerializeContext::version_after ("xy1", "xy2") == false);
  TASSERT (SerializeContext::version_after ("xy1", "xy0") == true);
  TASSERT (SerializeContext::version_after ("a01", "a001") == true);
  TASSERT (SerializeContext::version_after ("a10", "a9") == true);
  TASSERT (SerializeContext::version_after ("a10", "a90") == false);
  TASSERT (SerializeContext::version_after ("1.2.4", "1.2.3") == true);
  TASSERT (SerializeContext::version_after ("1.2.3", "1.2.3") == true);
  TASSERT (SerializeContext::version_after ("1.2.2", "1.2.3") == false);
  TASSERT (SerializeContext::version_after ("1.2.22", "1.2.3") == true);
  TASSERT (SerializeContext::version_after (version(), "0.11.1") == true);
  ExampleObject ex;
  ex.populate();

  String xml;
  {
    SerializeToXML so ("bse", version());
    so.save (ex);
    xml = so.to_xml();
    printout ("%s", xml);
  }

  ExampleObject ex2;
  TASSERT (!(ex == ex2));

  VoidPtrVector pointer_heap;

  {
    SerializeFromXML si (xml);
    bool success = si.load (ex2);
    TASSERT (success == true);
    si.copy_all_pointers (pointer_heap);
  }

  std::string xml2stream;
  {
    SerializeToXML so ("bse", version());
    so.save (ex2);
    xml2stream = so.to_xml();
  }
  TASSERT (ex == ex2);
  TASSERT (xml == xml2stream);

  ExampleObject ex3;
  TASSERT (!(ex == ex3));
  {
    SerializeFromXML si (xml2stream);
    bool success = si.load (ex3);
    TASSERT (success == true);
    si.copy_all_pointers (pointer_heap);
  }
  TASSERT (ex == ex3);
}
TEST_ADD (test_serialize_content);

static void
test_big_data_pack()
{
  // setup test data
  A1::BigDataPack big, other;
  TASSERT (big == other);
  fill_big_data_pack (big);
  TASSERT (big != other);
  // serialize to XML
  SerializeToXML so ("A1.BigDataPack");
  so.save (big);
  const std::string xmldata = so.to_xml();
  printout ("%s", xmldata);
  // serialize from XML
  SerializeFromXML si (xmldata);
  TASSERT (big != other);
  const bool success = si.load (other);
  TASSERT (success == true);
  TASSERT (!!big.derived == !!other.derived);           // avoid direct RemoteHandle comparisons
  TASSERT (big.bases.size() == other.bases.size());
  // remove known unserializable members
  auto clearbits = [] (A1::BigDataPack &p) {
    p.derived = A1::DerivedHandle();
    p.any1.clear();
    p.anys.clear();
    p.bases.clear();
  };
  clearbits (big);
  clearbits (other);
  TASSERT (big == other);
 }
TEST_ADD (test_big_data_pack);
