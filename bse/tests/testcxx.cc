#include <bse/bsecxxbase.h>
#include <bse/bsemain.h>
#include <bse/examplepeakfilter.h>

namespace {
using namespace Bse;

struct FooTest : public CxxBase {};

struct Foo {
  void bar (GParamSpec *p,int,float,int,int,int,int,int)
  {
    g_print ("notify: %s\n", p->name);
  }
  float baz (int, String s)
  {
    g_print ("notify: \"%s\"\n", s.c_str());
    return 0;
  }
};

} // namespace

int
main (int   argc,
      char *argv[])
{
  g_thread_init (NULL);
  bse_init_intern (&argc, &argv, NULL);

  // test closure Arg types
  Arg<FooTest*> a1;
  a1.token();
#if 0  // produce template error
  Arg<int**> a2;
  a2.token();
#endif
  Arg<int> a3;
  a3.token();
  Arg<char*> a4;
  a4.token();
  Arg<BseSource*> a5;
  a5.token();
  // tokenize_gtype (0);

#if 0   // need non-abstract C++ object in the core for this test
  GObject *o = (GObject*) g_object_new (BSE_TYPE_CXX_BASE, NULL);
  CxxBase *b = cast (o);
  Foo f;
  b->connect ("notify", Closure (&f, &Foo::bar));
  b->connect ("notify", Closure (&f, &Foo::baz));
#endif
  
  return 0;
}
