// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include <bse/bsecxxbase.hh>
#include <bse/bsemain.hh>
#include <bse/bsebusmodule.genidl.hh>

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
  bse_init_test (&argc, argv);
  std::set_terminate (__gnu_cxx::__verbose_terminate_handler);
  // g_log_set_always_fatal ((GLogLevelFlags) (G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL | (int) g_log_set_always_fatal ((GLogLevelFlags) G_LOG_FATAL_MASK)));

  /* work around known C++ binding bugs (critical warnings from GClosure) */
  unsigned int flags = g_log_set_always_fatal ((GLogLevelFlags) G_LOG_FATAL_MASK);
  g_log_set_always_fatal ((GLogLevelFlags) (flags & ~(G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING)));

  // test closure Arg types
  Arg<FooTest*> a1;
  a1.token();
  Arg<int> a3;
  a3.token();
  Arg<char*> a4;
  a4.token();
  Arg<BseSource*> a5;
  a5.token();
  // tokenize_gtype (0);

#if 0  // produce template error
  Arg<int**> errorarg;
  errorarg.token();
#endif

  GObject *o = (GObject*) g_object_new (BSE_TYPE_BUS_MODULE, NULL);
  CxxBase *b = cast (o);
  Foo f;
  b->connect ("notify", Closure (&f, &Foo::bar));
  b->connect ("notify", Closure (&f, &Foo::baz));

  return 0;
}
