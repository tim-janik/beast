/* BseFFT-test
 * Copyright (C) 2001-2002 Tim Janik
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
#include <bse/bsecxxbase.hh>
#include <bse/bsemain.h>
#include <bse/bseamplifier.genidl.hh>

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
  std::set_terminate (__gnu_cxx::__verbose_terminate_handler);
  // g_log_set_always_fatal ((GLogLevelFlags) (G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL | (int) g_log_set_always_fatal ((GLogLevelFlags) G_LOG_FATAL_MASK)));
   
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

  GObject *o = (GObject*) g_object_new (BSE_TYPE_AMPLIFIER, NULL);
  CxxBase *b = cast (o);
  Foo f;
  b->connect ("notify", Closure (&f, &Foo::bar));
  b->connect ("notify", Closure (&f, &Foo::baz));
  
  return 0;
}
