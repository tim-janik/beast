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
#ifndef __BSE_CXX_SMART_H__
#define __BSE_CXX_SMART_H__

#include <vector>
#include <sfi/sfi.h>

/* Provide a string identifying the current code position */
#if defined (__GNUC__)
#  define G_STRFUNC     ((const char*) (__PRETTY_FUNCTION__))
#elif defined (G_HAVE_ISO_VARARGS)
#  define G_STRFUNC     ((const char*) (__func__))
#elif
#  define G_STRFUNC     ((const char*) ("???"))
#endif

namespace Bse {

/* implementation detail of smart pointers. dictates API of
 * reference countable types
 */
template<class Type>
class CountablePointer {
  Type *pointer;
  static Type*
  refsink (Type *t)
  {
    t->ref();
    t->sink();
    return t;
  }
  CountablePointer (const CountablePointer&);
protected:
  typedef Type ValueType;
  CountablePointer (Type *t) :
    pointer (t ? refsink (t) : NULL)
  {
  }
  ~CountablePointer()
  {
    if (pointer)
      pointer->unref();
  }
  Type*
  get () const
  {
    return (Type*) pointer;
  }
  void
  set (Type *t)
  {
    if (t)
      refsink (t);
    if (pointer)
      pointer->unref();
    pointer = t;
  }
};

/* reference count implementation helper.
 * initial state is "floating", i.e. indicating that the initial
 * ref count isnÄt "owned" by anyone. objects shouldn't be left
 * around floating, using smart pointers like FooPtr fp = new Foo;
 * takes care of adapting the initial ref count.
 */
class RefCountable {
  mutable guint ref_count : 31;
  mutable guint floating : 1;
  friend class CountablePointer<RefCountable>;
  friend class CountablePointer<const RefCountable>;
  void
  ref() const
  {
    g_return_if_fail (ref_count >= 1);
    ref_count++;
  }
  void
  sink() const
  {
    if (floating)
      {
        floating = 0;
        unref();
      }
  }
  void
  unref() const
  {
    g_return_if_fail (ref_count >= 1);
    ref_count--;
    if (!ref_count)
      delete this;
  }
protected:
  virtual
  ~RefCountable ()
  {
    g_return_if_fail (ref_count < 1);
  }
public:
  RefCountable () :
    ref_count (1), floating (1)
  {
    /* could enter leak-list here to debug
     * floating objects upon exit.
     */
  }
};

/* helper class to allow smart pointer to const smart pointer assignments */
template<class VType> struct SmartPtrConstExchange {
  VType *v;
  SmartPtrConstExchange (VType *p) : v (p) {}
};
/* smart pointer template, supports ->, *, [0] and is_null() */
template<class Type, class ParentPtr>
class SmartPtr : public ParentPtr {
protected:
  typedef Type ValueType;
public:
  SmartPtr ()                                 : ParentPtr (NULL)              {}
  SmartPtr (Type                       *t)    : ParentPtr (t)                 {}
  SmartPtr (const SmartPtr             &src)  : ParentPtr ((Type*) src.get()) {}
  SmartPtr (SmartPtrConstExchange<Type> p)    : ParentPtr (p.v)               {}
  SmartPtr&
  operator= (SmartPtrConstExchange<Type> p)
  {
    set (p.v);
    return *this;
  }
  operator SmartPtrConstExchange<const Type> () const
  {
    return SmartPtrConstExchange<const Type> ((const Type*) get());
  }
  SmartPtr&
  operator= (Type *t)
  {
    set (t);
    return *this;
  }
  SmartPtr&
  operator= (const SmartPtr &src)
  {
    set (src.get());
    return *this;
  }
  Type&
  operator[] (guint n)
  {
    if (n)
      g_critical ("%s: invalid array subscript: %u", G_STRFUNC, n);
    return *(n ? NULL : operator-> ());
  }
  Type*
  operator-> () const
  {
    if (!get())
      g_critical ("%s: dereferencing NULL pointer", G_STRFUNC);
    return (Type*) get();
  }
  Type&
  operator* ()
  {
    return *operator-> ();
  }
  bool
  is_null()
  {
    return !get();
  }
  operator bool ()
  {
    return !is_null();
  }
};

/* sequence type template. supports:
 * ::Iter, begin(), end(), rbegin(), rend(), length(), get(), [0]
 */
template<class ValPtr>
class Sequence {
  std::vector<ValPtr> values;
public:
  class Iter : public ValPtr {
    friend class Sequence;
    Sequence *seq;
    int nth;
    int dir;
    void
    set_nth (int i)
    {
      nth = i;
      *this = seq->get (nth);
    }
    Iter (Sequence *s, int n, int d = 1) : seq (s), nth (~0), dir (d) { set_nth (n); }
  public:
    Iter (const Iter &src) : seq (src.seq), nth (~0), dir (src.dir) { set_nth (src.nth); }
    using ValPtr::operator=;
    Iter& operator= (const Iter &src)
    {
      seq = src.seq;
      set_nth (src.nth);
      dir = src.dir;
    }
    Iter& operator+= (int i)
    {
      set_nth (nth + dir * i);
      return *this;
    }
    Iter& operator++ (int)   { return *this += 1; }
    Iter& operator++ ()      { return *this += 1; }
    Iter& operator-= (int i) { return *this += -i; }
    Iter& operator-- (int)   { return *this -= 1; }
    Iter& operator-- ()      { return *this -= 1; }
    Iter  operator+  (int i) const { return Iter (*this) += i; }
    Iter  operator-  (int i) const { return Iter (*this) -= i; }
    gint
    cmp (const Iter &src) const
    {
      return dir * (seq < src.seq ? -1 : seq > src.seq ? 1 : nth < src.nth ? -1 : nth > src.nth);
    }
    bool operator<  (const Iter &src) const { return cmp (src) < 0; }
    bool operator<= (const Iter &src) const { return cmp (src) <= 0; }
    bool operator>  (const Iter &src) const { return cmp (src) > 0; }
    bool operator>= (const Iter &src) const { return cmp (src) >= 0; }
    bool operator!= (const Iter &src) const { return cmp (src) != 0; }
    bool operator== (const Iter &src) const { return cmp (src) == 0; }
  };
  typedef Iter RIter;
  ValPtr
  get (int index)
  {
    if (index >= 0 && index < length())
      return values[index];
    return ValPtr (NULL);
  }
  ValPtr&
  operator[] (guint index)
  {
    return values.at (index);
  }
  int
  length ()
  {
    return values.size();
  }
  Iter
  rend()
  {
    return Iter (this, -1, -1);
  }
  Iter
  begin()
  {
    return Iter (this, 0);
  }
  Iter
  rbegin()
  {
    return Iter (this, length () - 1, -1);
  }
  Iter
  end()
  {
    return Iter (this, length ());
  }
  void
  push_back (ValPtr v)
  {
    values.push_back (v);
  }
};

}
#endif /* __BSE_CXX_SMART_H__ */
