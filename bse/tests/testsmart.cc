/* BSE - Bedevilled Sound Engine                        -*-mode: c++;-*-
 * Copyright (C) 2003 Tim Janik, Stefan Westerfeld
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include <bse/bsecxxsmart.h>

using Bse::CountablePointer;
using Bse::RefCountable;
using Bse::SmartPtr;
using Bse::Sequence;

static int globalInstances = 0;

class Base : public RefCountable {
public:
  int b;
  Base()
  {
    globalInstances++;
  }
  virtual ~Base ()
  {
    globalInstances--;
    g_print ("base is properly being destroyed: %p (%d base instances left)\n",
	     this, globalInstances);
  }
};

class Foo : public Base {
public:
  int f;
  Foo (int x=0) : f (x) {}
  ~Foo()
  {
    g_print ("foo is properly destroyed: %p\n", this);
  }
};

class Zonk : public Foo {
public:
  int z;
  ~Zonk()
  {
    g_print ("Zonk says byebye: %p\n", this);
  }
};

typedef SmartPtr<Base,CountablePointer<RefCountable> > BasePtr;
typedef SmartPtr<const Base,CountablePointer<const RefCountable> > BaseCPtr;

typedef SmartPtr<Foo,BasePtr> FooPtr;
typedef SmartPtr<const Foo,BaseCPtr> FooCPtr;

typedef SmartPtr<Zonk,FooPtr> ZonkPtr;
typedef SmartPtr<const Zonk,FooCPtr> ZonkCPtr;

typedef Sequence<FooPtr> FooSeq;

void
test()
{
  BasePtr test;

  BasePtr bp = new Base;

  if (test || !bp)
    g_error ("urgs");

  bp->b = 8;

  FooPtr fp = new Foo;
  fp->f = 7;
  fp[0].f = 8;

  bp = fp;

  FooCPtr fc = FooCPtr (fp);
  fc = fp;

  BaseCPtr bc = (BaseCPtr) bp;
  bc = fp;

  ZonkPtr zz = new Zonk;
  zz->f = 99;
  ZonkPtr mp3 = zz;
  ZonkCPtr cp3 = (ZonkCPtr) zz;
  FooPtr  mp2 = zz;
  FooCPtr  cp2 = (FooCPtr) zz;
  BasePtr mp1 = zz;
  BaseCPtr cp1 = (BaseCPtr) zz;
  
  FooSeq fs;
  fs.push_back (zz);
  fs.push_back (fp);
  fs.push_back (new Foo (4));
  fs.push_back (zz);
  fs.push_back (new Foo (1));
  int i = 0;
  for (int i = 0; i < fs.length (); i++)
    g_print ("%d) get(%d)=%d [%d]=%d\n", i, i, fs.get(i)->f, i, fs[i]->f);
  i = 0;
  for (FooSeq::Iter fi = fs.begin(); fi != fs.end(); fi++)
    g_print ("%d) %d\n", i++, fi->f);
  i = 0;
  for (FooSeq::RIter ri = fs.rbegin(); ri != fs.rend(); ri++)
    g_print ("%d) %d\n", i++, ri->f);
}

int
main()
{
  std::set_terminate (__gnu_cxx::__verbose_terminate_handler);
  g_log_set_always_fatal ((GLogLevelFlags) (G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL | (int) g_log_set_always_fatal ((GLogLevelFlags) G_LOG_FATAL_MASK)));

  test();
  if (globalInstances != 0)
    g_error ("leak: didn't free %d base instances", globalInstances);

  return 0;
}

/* vim:set ts=8 sts=2 sw=2: */
